#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#include <pthread.h>
#include <string>
#include "wise_version.h"
#include "platform.h"
#include "SimuParser.h"
#include "SimuTools.h"
#include "SensorNetwork_API.h"
#include "AdvLog.h"

static void *UserData;
static IOT_Sensor sensor[WSN_TYPE_NUMBER];
static IOT_Device device;
static UpdateSNDataCbf moteCb;
static pthread_t simulator = 0;



void PrintAll(IOT_Device* sample) {	
	int iter = 0, ith = 0;
	ADV_DEBUG("=========================================================\n");
	for(iter = 0 ; iter < sample->moteNum ; iter++) {
		ADV_DEBUG("device.mote[%d].info.sProduct = %s\n", iter, sample->mote[iter].info.sProduct);
		ADV_DEBUG("device.mote[%d].info.sUID = %s\n", iter, sample->mote[iter].info.sUID);
		ADV_DEBUG("device.mote[%d].info.sHostName = %s\n", iter, sample->mote[iter].info.sHostName);
		ADV_DEBUG("device.mote[%d].info.sSN = %s\n", iter, sample->mote[iter].info.sSN);
		
		ADV_DEBUG("device.mote[%d].connect = %s\n", iter, sample->mote[iter].connect);
		ADV_DEBUG("device.mote[%d].sensorNum = %d\n", iter, sample->mote[iter].sensorNum);
		for(ith = 0 ; ith < sample->mote[iter].sensorNum ; ith++) {
			ADV_DEBUG("device.mote[%d].sensor[%d].type = %s\n", iter, ith, sample->mote[iter].sensor[ith].type);
			ADV_DEBUG("device.mote[%d].sensor[%d].n = %s\n", iter, ith, sample->mote[iter].sensor[ith].n);
			ADV_DEBUG("device.mote[%d].sensor[%d].u = %s\n", iter, ith, sample->mote[iter].sensor[ith].u);
			ADV_DEBUG("device.mote[%d].sensor[%d].v = %s\n", iter, ith, sample->mote[iter].sensor[ith].v);
			ADV_DEBUG("device.mote[%d].sensor[%d].vtype = %d\n", iter, ith, sample->mote[iter].sensor[ith].vtype);
			ADV_DEBUG("device.mote[%d].sensor[%d].min = %s\n", iter, ith, sample->mote[iter].sensor[ith].min);
			ADV_DEBUG("device.mote[%d].sensor[%d].max = %s\n", iter, ith, sample->mote[iter].sensor[ith].max);
			ADV_DEBUG("device.mote[%d].sensor[%d].trigger = %d\n", iter, ith, sample->mote[iter].sensor[ith].trigger);
			ADV_DEBUG("device.mote[%d].sensor[%d].pattern = %s\n", iter, ith, sample->mote[iter].sensor[ith].pattern);
			ADV_DEBUG("device.mote[%d].sensor[%d].standard = %s\n", iter, ith, sample->mote[iter].sensor[ith].standard);
		}
	}
	ADV_DEBUG("=========================================================\n");
}

void AssignValue(char *dest, int size, IOT_Sensor *sensor) {
	char value[1024] = {0};
	char temp[256] = {0};
	char *str;
	int index;
	int result;
	int min = atoi(sensor->min);
	int max = atoi(sensor->max);
	switch(sensor->vtype) {
		case VTYPE_NUM:
			if(strncmp(sensor->v,"(#", 2) == 0) {
				sscanf(sensor->v,"(#%[^#]#)", value);
				if(strlen(value) != 0) {
					index = ParserRule(value, sensor);
					result = GetRuleValue(sensor, index, 0);
					ValueIsDifferent(sensor, index, result);
					snprintf(value,sizeof(value),"%d",result);
					snprintf(sensor->v,sizeof(sensor->v),"(#%d#)",index);
				} else {
					snprintf(value,sizeof(value),"%d",rand()%(max - min + 1) + min);
				}
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"u\":\"%s\",\"v\":%s,\"min\": %s,\"max\":%s,%s}", sensor->n, sensor->u, value, sensor->min, sensor->max, sensor->standard);
		break;
		case VTYPE_BIN:
			if(strncmp(sensor->v,"(#", 2) == 0) {
				sscanf(sensor->v,"(#%[^#]#)", value);
				if(strlen(value) != 0) {
					index = ParserRule(value, sensor);
					result = GetRuleValue(sensor, index, 0);
					ValueIsDifferent(sensor, index, result);
					snprintf(value,sizeof(value),"%d",result);
					snprintf(sensor->v,sizeof(sensor->v),"(#%d#)",index);
				} else {
					snprintf(value,sizeof(value),"%d",rand()%(max - min + 1) + min);
				}
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"u\":\"%s\",\"bv\":%s,\"min\": %s,\"max\":%s,%s}", sensor->n, sensor->u, value, sensor->min, sensor->max, sensor->standard);
		break;
		case VTYPE_STRING:
			ParserStringValue(value, sizeof(value), sensor->v, sensor);
			PrintRule(sensor);
			strcpy(sensor->v, value);
			sensor->vtype = VTYPE_VSTRING;
		case VTYPE_VSTRING:	
			strcpy(temp,sensor->v);
			str = value;
			char *set = strtok(temp, "(#)");
			if(set != NULL) {
				str += sprintf(str,"%s",set);
				do {
					set = strtok(NULL, "(#)");
					if(set != NULL) {
						if(isdigit(set[0])) {
							index = atoi(set);
							result = GetRuleValue(sensor, index, 0);
							ValueIsDifferent(sensor, index, result);
							str += sprintf(str,"%d",result);
							set = strtok(NULL, "(#)");
							str += sprintf(str,"%s",set);
						} else {
							str += sprintf(str,"%d",rand()%(max - min + 1) + min);
							str += sprintf(str,"%s",set);
						}
					} else break;
				} while(1);
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"u\":\"%s\",\"sv\":\"[%s]\",\"min\": %s,\"max\":%s,%s}", sensor->n, sensor->u, value, sensor->min, sensor->max, sensor->standard);
		break;
	}
}

int UpdateValue(char *dest, int size, IOT_Sensor *sensor, int second) {
	char value[1024] = {0};
	char temp[256] = {0};
	char *str;
	int index;
	int result;
	int diff = 0; 
	int min = atoi(sensor->min);
	int max = atoi(sensor->max);
	switch(sensor->vtype) {
		case VTYPE_NUM:
			if(strncmp(sensor->v,"(#", 2) == 0) {
				if(strcmp(sensor->v,"(##)") == 0) {
					snprintf(value,sizeof(value),"%d",rand()%(max - min + 1) + min);
				} else {
					sscanf(sensor->v,"(#%d#)",&index);
					result = GetRuleValue(sensor, index, second);
					diff = ValueIsDifferent(sensor, index, result);
					if(sensor->trigger == TRIGGER_EDGE && !diff) return 0;
					//diff = 1;
					snprintf(value,sizeof(value),"%d",result);
				}
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"v\":%s}", sensor->n, value);
		break;
		case VTYPE_BIN:
			if(strncmp(sensor->v,"(#", 2) == 0) {
				if(strcmp(sensor->v,"(##)") == 0) {
					snprintf(value,sizeof(value),"%d",rand()%(max - min + 1) + min);
				} else {
					sscanf(sensor->v,"(#%d#)",&index);
					result = GetRuleValue(sensor, index, second);
					diff = ValueIsDifferent(sensor, index, result);
					if(!diff) return 0;
					//diff = 1;
					snprintf(value,sizeof(value),"%d",result);
				}
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"bv\":%s}", sensor->n, value);
		break;
		case VTYPE_VSTRING:	
			strcpy(temp,sensor->v);
			str = value;
			char *set = strtok(temp, "(#)");
			if(set != NULL) {
				str += sprintf(str,"%s",set);
				do {
					set = strtok(NULL, "(#)");
					if(set != NULL) {
						if(isdigit(set[0])) {
							index = atoi(set);
							result = GetRuleValue(sensor, index, second);
							diff |= ValueIsDifferent(sensor, index, result);
							//if(ValueIsDifferent(sensor, index, result)) diff = 1;
							str += sprintf(str,"%d",result);
							set = strtok(NULL, "(#)");
							str += sprintf(str,"%s",set);
						} else {
							str += sprintf(str,"%d",rand()%(max - min + 1) + min);
							str += sprintf(str,"%s",set);
						}
					} else break;
				} while(1);
			} else {
				snprintf(value,sizeof(value),"%s",sensor->v);
			}
			snprintf(dest,size,"{\"n\":\"%s\",\"sv\":\"[%s]\"}", sensor->n, value);
		break;
		
	}
	//PrintAll(&device);
	return diff;
}

void UpdateCapability(IOT_Device *device) {
	//update capability
	moteCb(SN_Inf_SendInfoSpec,&(device->specs), sizeof(SNMultiInfInfoSpec), UserData, NULL, NULL, NULL);
}

void UpdateInterface(IOT_Device *device) {
	int iter;
	//update interface
	for(iter = 0 ; iter < device->ifaceNum ; iter++) {
		//ADV_C_INFO(COLOR_YELLOW,"device->iface[%d].psSenNodeList  = %s\n", iter, device->iface[iter].psSenNodeList);
		//ADV_C_INFO(COLOR_YELLOW,"device->iface[%d].psTopology     = %s\n", iter, device->iface[iter].psTopology);
		moteCb(SN_Inf_UpdateInterface_Data,&(device->iface[iter]), sizeof(SNInterfaceData), UserData, NULL, NULL, NULL);
	}
	
}

void UpdateSensorInfo(IOT_Device *device) {
	int iter;
	int ith;
	char temp[4096] = {0};
	
	char *pos;
	char result[4096] = {0};
	int len;
	int gap;
	
	std::string senDataType("SenData");
	std::string senData;
	std::string infoType("Info");
	std::string info;
	std::string netType("Net");
	std::string net;
	
	InSenData spec;
	InBaseData dataArray[3];
	spec.inDataClass.iTypeCount = 3;
	spec.inDataClass.pInBaseDataArray = dataArray;
	
	//register sensor
	for(iter = 0 ; iter < device->moteNum ; iter++) {
		moteCb(SN_SenHub_Register,&(device->mote[iter].info), sizeof(SenHubInfo), UserData, NULL, NULL, NULL);
		
		strcpy(spec.sUID,device->mote[iter].info.sUID);
		
		
		//SenData
		spec.inDataClass.pInBaseDataArray[0].psType = (char *)senDataType.c_str();
		spec.inDataClass.pInBaseDataArray[0].iSizeType = senDataType.size() +1 ;
		memset(result,0,sizeof(result));
		pos = result;
		ADV_INFO("\ndevice->mote[iter].sensorNum = %d\n", device->mote[iter].sensorNum);
		for(ith = 0 ; ith < device->mote[iter].sensorNum ; ith++) {
			AssignValue(temp, sizeof(temp), &(device->mote[iter].sensor[ith]));
			pos += sprintf(pos, "%s,", temp);

		}
		len = strlen(result);
		len = len <= 0 ? 0 : len - 1;
		result[len] = 0;
		senData = result;
		spec.inDataClass.pInBaseDataArray[0].psData = (char *)senData.c_str();
		spec.inDataClass.pInBaseDataArray[0].iSizeData = senData.size()+1;
		//strncpy(spec.psSenData, result, len);
		//spec.psSenData[len] = 0;
		ADV_INFO("result = %s\n", spec.inDataClass.pInBaseDataArray[0].psData);
		
		
		//Info
		spec.inDataClass.pInBaseDataArray[1].psType = (char *)infoType.c_str();
		spec.inDataClass.pInBaseDataArray[1].iSizeType = infoType.size() +1 ;
		len = sizeof(result);
		memset(result,0,len);
		pos = result;
		gap = snprintf(pos,len,"{\"n\":\"Name\",\"sv\":\"%s\",\"asm\":\"rw\"},", device->mote[iter].info.sHostName);
		pos += gap; len -= gap;
		gap = snprintf(pos,len,"{\"n\":\"sw\",\"sv\":\"%s\",\"asm\":\"r\"},", WISE_VERSION);
		pos += gap; len -= gap;
		gap = snprintf(pos,len,"{\"n\":\"reset\",\"bv\":0,\"asm\":\"rw\"}");
		pos += gap; len -= gap;
		len = strlen(result);
		//spec.psInfo = (char *)malloc(len);
		//strcpy(spec.psInfo, result);
		info = result;
		spec.inDataClass.pInBaseDataArray[1].psData = (char *)info.c_str();
		spec.inDataClass.pInBaseDataArray[1].iSizeData = info.size()+1;
		ADV_INFO("info = %s\n\n", spec.inDataClass.pInBaseDataArray[1].psData);
		
		
		//Net
		spec.inDataClass.pInBaseDataArray[2].psType = (char *)netType.c_str();
		spec.inDataClass.pInBaseDataArray[2].iSizeType = netType.size() +1 ;
		len = sizeof(result);
		memset(result,0,len);
		pos = result;
		gap = snprintf(pos,len,"{\"n\":\"Health\",\"v\": 80,\"asm\":\"r\"},");
		pos += gap; len -= gap;
		gap = snprintf(pos,len,"{\"n\":\"Neighbor\",\"sv\": \"\",\"asm\":\"r\"},");
		pos += gap; len -= gap;
		gap = snprintf(pos,len,"{\"n\":\"sw\",\"sv\":\"1.0.0.1\",\"asm\":\"r\"}");
		pos += gap; len -= gap;
		len = strlen(result);
		net = result;
		spec.inDataClass.pInBaseDataArray[2].psData = (char *)net.c_str();
		spec.inDataClass.pInBaseDataArray[2].iSizeData = net.size()+1;
		ADV_INFO("net = %s\n\n", spec.inDataClass.pInBaseDataArray[2].psData);
		
		moteCb(SN_SenHub_SendInfoSpec,&spec, sizeof(InSenData), UserData, NULL, NULL, NULL);
		//free(spec.psSenData);
		//free(spec.psInfo);
	}
	
	
}

void UpdateSensorData(IOT_Device *device, int second) {
	int iter;
	int ith;
	char temp[4096] = {0};
	
	char *pos;
	char result[4096] = {0};
	int len;
	int gap;
	int t;
	int value;
	std::string senDataType("SenData");
	std::string senData;
	std::string netType("Net");
	std::string net;
	InSenData data;
	InBaseData dataArray[2];
	data.inDataClass.pInBaseDataArray = dataArray;
	
	//update data
	for(iter = 0 ; iter < device->moteNum ; iter++) {
		
		data.inDataClass.iTypeCount = 0;
		//update data
		memset(result,0,sizeof(result));
		pos = result;
		for(ith = 0 ; ith < device->mote[iter].sensorNum ; ith++) {
			value = UpdateValue(temp, sizeof(temp), &(device->mote[iter].sensor[ith]), second);
			ADV_DEBUG("value = %d\n", value);
			if(device->mote[iter].sensor[ith].secoff == -1) {
				device->mote[iter].sensor[ith].secoff = second%device->mote[iter].sensor[ith].tsec;
			}
			t = (second - device->mote[iter].sensor[ith].secoff)%device->mote[iter].sensor[ith].tsec;
			
			if(value || (device->mote[iter].sensor[ith].trigger == TRIGGER_LEVEL && t == 0)) {
				pos += sprintf(pos, "%s,", temp);
			}
		}
		
		len = strlen(result);
		if(len == 0) {
			ADV_DEBUG("result = \n");
		} else {
			len = len <= 0 ? 0 : len - 1;
			result[len] = 0;
			
			data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psData = result;
			data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].iSizeData = len;
			data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psType = (char *)senDataType.c_str();
			data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].iSizeType = senDataType.size() + 1;
			
			//data.psSenData = (char *)malloc(len);
			//strncpy(data.psSenData, result, len);
			//data.psSenData[len] = 0;
			ADV_C_INFO(COLOR_CYAN,"update data = %s\n", data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psData);
			
			data.inDataClass.iTypeCount++;
		}
		
		//free(data.psSenData);
		
		
		//Update Net
		if(device->mote[iter].modify == 1) {
			len = sizeof(result);
			memset(result,0,len);
			pos = result;
			gap = snprintf(pos,len,"{\"n\":\"Health\",\"v\": 81},");
			pos += gap; len -= gap;
			gap = snprintf(pos,len,"{\"n\":\"Neighbor\",\"sv\": \"%s\"},", device->mote[iter].connect);
			pos += gap; len -= gap;
			len = strlen(result);
			if(len == 0) {
				ADV_DEBUG("result = \n");
			} else {
				len = len <= 0 ? 0 : len - 1;
				result[len] = 0;
				
				data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psData = result;
				data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].iSizeData = len;
				data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psType = (char *)netType.c_str();
				data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].iSizeType = netType.size() + 1;
				
				//data.psSenData = (char *)malloc(len);
				//strncpy(data.psSenData, result, len);
				//data.psSenData[len] = 0;
				ADV_C_INFO(COLOR_CYAN,"update net = %s\n", data.inDataClass.pInBaseDataArray[data.inDataClass.iTypeCount].psData);
				
				data.inDataClass.iTypeCount++;
			}
			device->mote[iter].modify = 0;
		}
		
		if(data.inDataClass.iTypeCount != 0) {
			strcpy(data.sUID, device->mote[iter].info.sUID);
			if(device->mote[iter].reset < 1) {
				moteCb(SN_SenHub_AutoReportData,&data, sizeof(InSenData), UserData, NULL, NULL, NULL);
			}
		}
	}
}

void UpdateDisconnectEvent(IOT_Device *device) {
	int iter;
	int count = 0;
	InSenData data;
	memset(&data,0,sizeof(InSenData));
	
	for(iter = 0 ; iter < device->moteNum ; iter++) {
		if(device->mote[iter].reset == 1) {
			device->mote[iter].reset = 0;
			device->mote[iter].modify = 1;
			strncpy(data.sUID,device->mote[iter].info.sUID,MAX_SN_UID);
			moteCb(SN_SenHub_Disconnect,&data, sizeof(InSenData), UserData, NULL, NULL, NULL);
		}
	}
	
	for(iter = 0 ; iter < device->infos.iNum ; iter++) {
		if(device->faceinfo[iter].reset == 1) {
			device->faceinfo[iter].reset = 0;
			count++;
		}
	}
	
	if(count != 0) {
		//UpdateCapability(device);
		UpdateInterface(device);
		UpdateSensorInfo(device);
	}
}

void *Simulator(void *arg) {
	IOT_Device* device = (IOT_Device*)arg;
	int second;
	
	while(moteCb == NULL) {
		ADV_WARN("Callback function is not registered.");
		sleep(1);
	}
	
	UpdateCapability(device);
	sleep(10);
	UpdateInterface(device);
	UpdateSensorInfo(device);
	do {
		second = (int)time(NULL);
		ADV_TRACE("second = %d\n",second);
		if(second%5 == 0) {
			UpdateInterface(device);
			//UpdateSensorInfo(device);
		}
		UpdateSensorData(device, second);
		sleep(1);
		UpdateDisconnectEvent(device);
	} while(1);
	
}

//=========================SN interface=========================================//
extern "C" SN_CODE SNCALL SN_GetVersion(char *psVersion, int BufLen) {
	if((unsigned int)BufLen < strlen(WISE_VERSION)+1) {
		psVersion = (char *)realloc(psVersion,strlen(WISE_VERSION)+1);
	}
	strcpy(psVersion,WISE_VERSION);
	return SN_OK;
}

extern "C" SN_CODE SNCALL SN_Initialize(void *pInUserData) {
	int iter = 0, ith = 0;
	int index = 0;

	UserData = pInUserData;
	srand((unsigned int)time(NULL));
	if (access(INI_PATH"@mote_list.ini", F_OK) == 0) {
		LoadListFile(INI_PATH"@mote_list.ini", sensor);
	}
	else if (access("@mote_list.ini", F_OK) == 0) {
		LoadListFile("@mote_list.ini", sensor);
	}
	else {
		ADV_ERROR("Can't found file @mote_list.ini\n");
	}
	memset(&device, 0, sizeof(IOT_Device));


	if (access(INI_PATH"wsn", F_OK) == 0) {
		LoadIniDir(INI_PATH"wsn", &device, sensor);
	}
	else if (access("wsn", F_OK) == 0) {
		LoadIniDir("wsn", &device, sensor);
	}
	else {
		ADV_ERROR("Can't found wsn\n");
	}
	
	SyncVariable(sensor, &device);

	strncpy(device.specs.sComType,device.infos.sComType,MAX_SN_COM_NAME);
	device.specs.iNum = device.infos.iNum;

	for(int i = 0 ; i < device.infos.iNum ; i++) {
		//info spec
		OutDataClass *output = &device.infos.SNInfs[i].outDataClass;
		
		output->iTypeCount = 1;
		output->pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData) * output->iTypeCount);
		memset(output->pOutBaseDataArray,0,sizeof(OutBaseData) * output->iTypeCount);
		for(int j = 0 ; j < output->iTypeCount ; j++) {
			SetOutBaseData(&output->pOutBaseDataArray[j],"Info",WSN_FORMAT);
		}
		
		strncpy(device.specs.aSNInfSpec[i].sInfID, device.infos.SNInfs[i].sInfID, MAX_SN_INF_ID);
		strncpy(device.specs.aSNInfSpec[i].sInfName, device.infos.SNInfs[i].sInfName, MAX_SN_INF_NAME);
		InDataClass *input = &device.specs.aSNInfSpec[i].inDataClass;
		input->iTypeCount = 1;
		input->pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * input->iTypeCount);
		memset(input->pInBaseDataArray,0,sizeof(InBaseData) * input->iTypeCount);
		for(int j = 0 ; j < input->iTypeCount ; j++) {
			SetInBaseData(&input->pInBaseDataArray[j],"Info",WSN_FORMAT);
		}
		
		
	}

	IOT_Device* sample = NULL;
	sample = &device;	
	for(iter = 0 ; iter < sample->moteNum ; iter++) {
		ADV_DEBUG("device.mote[%d].info.sProduct = %s\n", iter, sample->mote[iter].info.sProduct);
		ADV_DEBUG("device.mote[%d].info.sUID = %s\n", iter, sample->mote[iter].info.sUID);
		ADV_DEBUG("device.mote[%d].info.sHostName = %s\n", iter, sample->mote[iter].info.sHostName);
		ADV_DEBUG("device.mote[%d].info.sSN = %s\n", iter, sample->mote[iter].info.sSN);
		
		ADV_DEBUG("device.mote[%d].connect = %s\n", iter, sample->mote[iter].connect);
		ADV_DEBUG("device.mote[%d].sensorNum = %d\n", iter, sample->mote[iter].sensorNum);
		for(ith = 0 ; ith < sample->mote[iter].sensorNum ; ith++) {
			ADV_DEBUG("device.mote[%d].sensor[%d].type = %s\n", iter, ith, sample->mote[iter].sensor[ith].type);
			ADV_DEBUG("device.mote[%d].sensor[%d].n = %s\n", iter, ith, sample->mote[iter].sensor[ith].n);
			ADV_DEBUG("device.mote[%d].sensor[%d].u = %s\n", iter, ith, sample->mote[iter].sensor[ith].u);
			ADV_DEBUG("device.mote[%d].sensor[%d].v = %s\n", iter, ith, sample->mote[iter].sensor[ith].v);
			ADV_DEBUG("device.mote[%d].sensor[%d].vtype = %d\n", iter, ith, sample->mote[iter].sensor[ith].vtype);
			ADV_DEBUG("device.mote[%d].sensor[%d].min = %s\n", iter, ith, sample->mote[iter].sensor[ith].min);
			ADV_DEBUG("device.mote[%d].sensor[%d].max = %s\n", iter, ith, sample->mote[iter].sensor[ith].max);
			ADV_DEBUG("device.mote[%d].sensor[%d].trigger = %d\n", iter, ith, sample->mote[iter].sensor[ith].trigger);
			ADV_DEBUG("device.mote[%d].sensor[%d].pattern = %s\n", iter, ith, sample->mote[iter].sensor[ith].pattern);
			ADV_DEBUG("device.mote[%d].sensor[%d].standard = %s\n", iter, ith, sample->mote[iter].sensor[ith].standard);
			ADV_DEBUG("device.mote[%d].sensor[%d].responsesec = %d\n", iter, ith, sample->mote[iter].sensor[ith].responsesec);
		}
	}
	
	if(simulator == 0) {
		ADV_INFO("@@@@@@@@@@@@@@@@@@@@SN_GetCapability(%d)\n", (int)simulator);
		pthread_create(&simulator, NULL, &Simulator, &device);
	}

	
	return SN_OK;
}

extern "C" SN_CODE SNCALL SN_Uninitialize(void *pInParam) {
	
	int i, j;
	for(i = 0 ; i < device.infos.iNum ; i++) {
		OutDataClass *output = &device.infos.SNInfs[i].outDataClass;
		for(j = 0 ; j < output->iTypeCount ; j++) {
			ReleaseOutBaseData(&output->pOutBaseDataArray[j]);
		}
	}
	
	for(i = 0 ; i < device.specs.iNum ; i++) {
		InDataClass *input = &device.specs.aSNInfSpec[i].inDataClass;
		for(j = 0 ; j < input->iTypeCount ; j++) {
			ReleaseInBaseData(&input->pInBaseDataArray[j]);
		}
	}
	
	for(i = 0 ; i < device.ifaceNum ; i++) {
		for(j = 0 ; j < device.iface[i].inDataClass.iTypeCount ; j++) {
			free(device.iface[i].inDataClass.pInBaseDataArray[j].psType);
			free(device.iface[i].inDataClass.pInBaseDataArray[j].psData);
		}
		free(device.iface[i].inDataClass.pInBaseDataArray);
		device.iface[i].inDataClass.iTypeCount = 0;
	}
	for(i = 0 ; i < device.moteNum ; i++) {
		if(device.mote[i].sensor != NULL) free(device.mote[i].sensor);
		if(device.mote[i].connect != NULL) free(device.mote[i].connect);
	}
	
	if(device.mote != NULL) free(device.mote);
	return SN_OK;
}



extern "C" SN_CODE SNCALL SN_GetCapability(SNMultiInfInfo *pOutSNMultiInfInfo){
	if(moteCb == NULL) return SN_ER_FAILED;

	strcpy(pOutSNMultiInfInfo->sComType,device.infos.sComType);
	pOutSNMultiInfInfo->iNum = device.infos.iNum;
	for(int i = 0 ; i < device.infos.iNum ; i++) {
		strcpy(pOutSNMultiInfInfo->SNInfs[i].sInfID,device.infos.SNInfs[i].sInfID);
		strcpy(pOutSNMultiInfInfo->SNInfs[i].sInfName,device.infos.SNInfs[i].sInfName);
		
		OutBaseData *internal = &device.infos.SNInfs[i].outDataClass.pOutBaseDataArray[0];
		OutBaseData *output = &pOutSNMultiInfInfo->SNInfs[i].outDataClass.pOutBaseDataArray[0];
		
		CopyOutBaseData(output, internal);
	}
	
	

	
	//memcpy(pOutSNMultiInfInfo,&(device.infos), sizeof(SNInfInfos));
	
	
	
	return SN_OK;
}

extern "C" void SNCALL SN_SetUpdateDataCbf(UpdateSNDataCbf pUpdateSNDataCbf) {
	moteCb = pUpdateSNDataCbf;
	ADV_INFO("@@@@@@@@@@@@@@@@@@@@SN_SetUpdateDataCbf\n");
}

extern "C" SN_CODE SNCALL SN_GetData(SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData) {
	if(pInSenData->inDataClass.iTypeCount != pOutSenData->outDataClass.iTypeCount) return SN_ER_VALUE_OUT_OF_RNAGE;
	int ret = SN_ER_FAILED;
	int i = 0;
	InBaseData *input;
	OutBaseData *output;
	switch(CmdID) {
		case SN_Inf_Get:
		{
			IOT_Sensor key;
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				
				ADV_TRACE("1. input->psData = %s\n", input->psData);
				ReplaceChar(input->psData, '\"', -1);
				ReplaceChar(input->psData, '{', -1);
				ReplaceChar(input->psData, '}', -1);
				ADV_TRACE("2. input->psData = %s\n", input->psData);
				ElementParser(input->psData, &key);
				ADV_TRACE("key.n = %s, key.v = %s\n", key.n, key.v);
				if(strcmp("Info", input->psType) == 0) {
					ret = GetInterface(pInSenData->sUID, key, &device, output);
				}
			}
		} break;
		
		case SN_SenHub_Get:
		{
			IOT_Sensor key;
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				
				ADV_TRACE("1. input->psData = %s\n", input->psData);
				ReplaceChar(input->psData, '\"', -1);
				ReplaceChar(input->psData, '{', -1);
				ReplaceChar(input->psData, '}', -1);
				ADV_TRACE("2. input->psData = %s\n", input->psData);
				ElementParser(input->psData, &key);
				ADV_TRACE("key.n = %s, key.v = %s\n", key.n, key.v);
				if(strcmp("SenData", input->psType) == 0) {
					ret = GetSensor(pInSenData->sUID, key, &device, output);
				} else if(strcmp("Info", input->psType) == 0) {
					ret = GetInfo(pInSenData->sUID, key, &device, output);
				}
			}
		} break;
		
		default:
			break;
	}
	return (SN_CODE)ret;
}

extern "C" SN_CODE SNCALL SN_SetData(SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData) {
	if(pInSenData->inDataClass.iTypeCount != pOutSenData->outDataClass.iTypeCount) return SN_ER_VALUE_OUT_OF_RNAGE;
	int ret = SN_ER_FAILED;
	int i = 0;
	InBaseData *input;
	OutBaseData *output;
	switch(CmdID) {
		case SN_Inf_Set:
		{
			IOT_Sensor key;
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				
				ADV_TRACE("1. input->psData = %s\n", input->psData);
				ReplaceChar(input->psData, '\"', -1);
				ReplaceChar(input->psData, '{', -1);
				ReplaceChar(input->psData, '}', -1);
				ADV_TRACE("2. input->psData = %s\n", input->psData);
				ElementParser(input->psData, &key);
				ADV_TRACE("key.n = %s, key.v = %s\n", key.n, key.v);
				if(strcmp("Info", input->psType) == 0) {
					ret = SetInterface(pInSenData->sUID, key, &device, output);
				}
			}
		} break;
		
		case SN_SenHub_Set:
		{
			IOT_Sensor key;
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				
				ADV_TRACE("1. input->psData = %s\n", input->psData);
				ReplaceChar(input->psData, '\"', -1);
				ReplaceChar(input->psData, '{', -1);
				ReplaceChar(input->psData, '}', -1);
				ADV_TRACE("2. input->psData = %s\n", input->psData);
				ElementParser(input->psData, &key);
				ADV_TRACE("key.n = %s, key.v = %s\n", key.n, key.v);
				if(strcmp("SenData", input->psType) == 0) {
					ret = SetSensor(pInSenData->sUID, key, &device, output);
				} else if(strcmp("Info", input->psType) == 0) {
					ret = SetInfo(pInSenData->sUID, key, &device, output);
				}
			}
		} break;
		
		default:
			break;
	}
	return (SN_CODE)ret;
}
