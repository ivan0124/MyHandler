#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <algorithm>
#include <vector>
#include <deque>
#include <set>
#include <map>
#include <string>
#include "platform.h"
#include "SimuParser.h"
#include "SimuTools.h"
#include "AdvLog.h"
IOT_Sensor gWsnTypeList[WSN_TYPE_NUMBER] = 
	{
		{strlen(WSN_TEMPERATURE), WSN_TEMPERATURE, WSN_TEMPERATURE_PATTERN, WSN_TEMPERATURE_SPEC, WSN_TEMPERATURE_FLAG, VTYPE_NUM},
		{strlen(WSN_GPI), WSN_GPI, WSN_GPI_PATTERN, WSN_GPI_SPEC, WSN_GPI_FLAG, VTYPE_BIN},
		{strlen(WSN_GPO), WSN_GPO, WSN_GPO_PATTERN, WSN_GPO_SPEC, WSN_GPO_FLAG, VTYPE_BIN},
		{strlen(WSN_CO2), WSN_CO2, WSN_CO2_PATTERN, WSN_CO2_SPEC, WSN_CO2_FLAG, VTYPE_NUM},
		{strlen(WSN_CO), WSN_CO, WSN_CO_PATTERN,WSN_CO_SPEC, WSN_CO_FLAG, VTYPE_NUM},
		{strlen(WSN_SONAR), WSN_SONAR, WSN_SONAR_PATTERN, WSN_SONAR_SPEC, WSN_SONAR_FLAG, VTYPE_NUM},
		{strlen(WSN_GSENSOR), WSN_GSENSOR, WSN_GSENSOR_PATTERN, WSN_GSENSOR_SPEC, WSN_GSENSOR_FLAG, VTYPE_STRING},
		{strlen(WSN_MAGNETIC), WSN_MAGNETIC, WSN_MAGNETIC_PATTERN, WSN_MAGNETIC_SPEC, WSN_MAGNETIC_FLAG, VTYPE_STRING},
		{strlen(WSN_LIGHT), WSN_LIGHT, WSN_LIGHT_PATTERN, WSN_LIGHT_SPEC, WSN_LIGHT_FLAG, VTYPE_NUM},
		{strlen(WSN_HUMIDITY), WSN_HUMIDITY, WSN_HUMIDITY_PATTERN, WSN_HUMIDITY_SPEC, WSN_HUMIDITY_FLAG, VTYPE_NUM},
	};
	
char *ReplaceChar(char *str, char find, char replace) {
    char *ret=str;
    char *wk, *s;
    wk = s = strdup(str);

    while (*s != 0) {
        if (*s == find){
			if(replace != (char)-1) {
				*str++ = replace;
			}
            ++s;
        } else
            *str++ = *s++;
    }
    *str = '\0';
    free(wk);
    return ret;
}

int GetSensorTypeIndex(char *type, IOT_Sensor *table) {

	int i;
	int len = strlen(type);
	for(i = 0 ; i < WSN_TYPE_NUMBER ; i++) {
		if(len == table[i].len) {
			if(strcmp(type, table[i].type) == 0) return i;
		}
	}
	return -1;
}

char *SplitStringValue(char *string, char *copyto) {
	char *temp;
	char *ptr;
	if(*string == '[') {
		temp = strdup(string+1);
		ptr = strpbrk(temp,"]");
		*ptr = '\0';
		strcpy(copyto,temp);
		free(temp);
		return strpbrk(string,"]")+1;
	} else {
		temp = strdup(string);
		ptr = strpbrk(temp,",");
		if(ptr != NULL) *ptr = '\0';
		strcpy(copyto,temp);
		free(temp);
		return strpbrk(string,",");
	}

}


void ElementParser(char *string, IOT_Sensor* sensor) {
	ADV_DEBUG("[head]string = %s\n",string);
	ReplaceChar(string, ' ', -1);
	string=strpbrk(string,"nuvbsm");
	do {
		if(strncmp(string,"n",1) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%255[^,],", sensor->n);
			ReplaceChar(sensor->n, '\"', -1);
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"u",1) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%31[^,],", sensor->u);
			ReplaceChar(sensor->u, '\"', -1);
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"v",1) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%255[^,],", sensor->v);
			ReplaceChar(sensor->v, '\"', -1);
			sensor->vtype = VTYPE_NUM;
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"bv",2) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%255[^,],", sensor->v);
			ReplaceChar(sensor->v, '\"', -1);
			sensor->vtype = VTYPE_BIN;
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"sv",2) == 0) {
			//sscanf(string+3, "[%255[^]]]", sensor->v);
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			string = SplitStringValue(string, sensor->v);
			ReplaceChar(sensor->v, '\"', -1);
			sensor->vtype = VTYPE_STRING;
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"min",3) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%31[^,],", sensor->min);
			ReplaceChar(sensor->min, '\"', -1);
			if(string != NULL) string = strchr(string,',');
		} else if(strncmp(string,"max",3) == 0) {
			string=strpbrk(string,":")+1;
			if(*string == '\"') string++;
			sscanf(string, "%31[^,],", sensor->max);
			ReplaceChar(sensor->max, '\"', -1);
			if(string != NULL) string = strchr(string,',');
		} else {
			string = strchr(string,',');
		}
		
		if(string == NULL) break;
		else string=strpbrk(string,"nuvbsm");
		if(string == NULL) break;
		ADV_DEBUG("string = %s\n",string);
	} while(1);
	
}

int ParserStringValue(char *dest, int size, char *sv, IOT_Sensor *sensor) {
	char rule[1024] = {0};
	char value[1024] = {0};
	char data[256] = {0};
	int index = 0;
	strcpy(data,sv);
	
	char *str = value;
				
	char *set;
	
	set = strtok(sv, "}");
	while(set != NULL) {
		set = strchr(set,'{') + 1;
		ADV_TRACE("set = %s\n",set);

		str+=sprintf(str, "{\\\"n\\\":\\\"");
		
		char ename[256] = {0};
		char evalue[256] = {0};
		sscanf(set+2, "%255[^,],", ename);
		
		str+=sprintf(str,"%s\\\",\\\"v\\\":",ename);
		
		set = strchr(set,',') + 1;
		sscanf(set+2, "%255s", evalue);
		
		if(strncmp(evalue,"(#",2) == 0) {
			sscanf(evalue,"(#%[^#]#)", rule);
			if(strlen(rule) != 0) {
				index = ParserRule(rule, sensor);
				str+=sprintf(str,"(#%d#)",index);
			} else {
				str+=sprintf(str,"(##)");
			}
		} else {
			str+=sprintf(str,"%s",evalue);
		}
		str+=sprintf(str,"},");
		set = strtok(NULL, "}");
	}
	int len = strlen(value);
	value[len <= 0 ? 0 : len-1] = 0;
	return snprintf(dest,size,"%s",value);
}

int LoadListFile(const char *path, IOT_Sensor* sensor) {
	FILE* base = NULL;
	char *line = NULL;
	char *pos;
	int ret;
	int count = -1;
	int iter = 0;
	char parserBuf[4096] = {0};
	int typeId = -1;
	size_t len = 0;
	ssize_t read;

	base = fopen(path,"r");
	
	while ((read = getline(&line, &len, base)) != -1) {
		ADV_INFO("@@line = %s\n",line);
		if(strncmp(line,"Sensor_Type=",12) == 0) {
			count++;
			if(count >= WSN_TYPE_NUMBER) break;
			sscanf(line+12,"\"%31[^\"]\"",sensor[count].type);
			typeId = GetSensorTypeIndex(sensor[count].type, gWsnTypeList);
			if(typeId == -1) {
				count--;
				goto list_re_entry;
			}
			sensor[count].len = gWsnTypeList[typeId].len;
			sensor[count].vtype = gWsnTypeList[typeId].vtype;
			strcpy(sensor[count].pattern, gWsnTypeList[typeId].pattern);
			if((read = getline(&line, &len, base)) == -1) break;
			if(strncmp(line,"Sensor_Default_Info=",20) == 0) {
				parserBuf[0] = 0;
				ret = sscanf(line+20,"\"%4095[^\"]\"",parserBuf);
				if(ret > 0) ElementParser(parserBuf, &sensor[count]);
				if((read = getline(&line, &len, base)) == -1) break;
				if(strncmp(line,"Sensor_Default_Value=",21) == 0) {
					parserBuf[0] = 0;
					ret = sscanf(line+21,"\"%4095[^\"]\"",parserBuf);
					if(ret > 0) ElementParser(parserBuf, &sensor[count]);
					if((read = getline(&line, &len, base)) == -1) break;
					if(strncmp(line,"Sensor_Default_Trigger_Mode=",28) == 0) {
						sscanf(line+28,"\"%31[^\"]\"",parserBuf);
						if(strcmp(parserBuf,"Edge") == 0) {
							sensor[count].trigger = TRIGGER_EDGE;
							sensor[count].tsec = 1;
						} else {
							sensor[count].trigger = TRIGGER_LEVEL;
							pos = strchr(parserBuf, '@');
							if(pos == NULL) {
								sensor[count].tsec = 1;
							} else {
								sensor[count].tsec = atoi(pos+1);
							}
						}
						if((read = getline(&line, &len, base)) == -1) break;
						if(strncmp(line,"Sensor_Default_Response_Time=",29) == 0) {
							sscanf(line+29,"\"%31[^\"]\"",parserBuf);
							ADV_DEBUG("@@@@@@@@@@@@@@@parserBuf = %s\n",parserBuf);
							sensor[count].responsesec = atoi(parserBuf);
						}
					} else break;
				} else break;
			} else break;
		}
list_re_entry:
		continue;
	}
	

	ADV_TRACE("count = %d\n", count);
	if(count > 0) {
		int i = 0;
		for(i = 0 ; i <= count ; i++) {
			ADV_DEBUG("sensor[%d].type = %s\n", i, sensor[i].type);
			ADV_DEBUG("sensor[i].n = %s\n", sensor[i].n);
			ADV_DEBUG("sensor[i].u = %s\n", sensor[i].u);
			ADV_DEBUG("sensor[i].v = %s\n", sensor[i].v);
			ADV_DEBUG("sensor[i].vtype = %d\n", sensor[i].vtype);
			ADV_DEBUG("sensor[i].min = %s\n", sensor[i].min);
			ADV_DEBUG("sensor[i].max = %s\n", sensor[i].max);
			ADV_DEBUG("sensor[i].trigger = %d\n\n", sensor[i].trigger);
		}
	}


	free(line);
	fclose(base);
	
	return ++count;
}


int LoadInterfaceFile(const char *path, IOT_Device* device) {
	ADV_TRACE("path = %s\n",path);
	
	FILE* base = NULL;
	char *line = NULL;
	int iter = 0;
	int typeId = -1;
	char parserBuf[4096] = {0};
	IOT_Device* sample = NULL;
	IOT_Mote* mote = NULL;
	size_t len = 0;
	ssize_t read;
	base = fopen(path,"r");
	if(base != NULL) {
		sample = device;
		while ((read = getline(&line, &len, base)) != -1) {

			if(strncmp(line,"Communication_Type=",19) == 0) {
				sscanf(line+19,"\"%31[^\"]\"",sample->infos.sComType);
			}

			if(sample != NULL) {
				if(strncmp(line,"[Interface]",11) == 0) {
					if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
					if(strncmp(line,"Interface_Number=",17) == 0) {
						parserBuf[0] = 0;
						sscanf(line+17,"\"%4095[^\"]\"",parserBuf);
						sample->infos.iNum = atoi(parserBuf);
						sample->ifaceNum = sample->infos.iNum;
						iter = 0;
						while ((read = getline(&line, &len, base)) != -1) {
							if(strncmp(line,"Interface_Name=",15) == 0) {
								sscanf(line+15,"\"%15[^\"]\"",sample->infos.SNInfs[iter].sInfName);
								if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
								if(strncmp(line,"Interface_Id=",13) == 0) {
									sscanf(line+13,"\"%31[^\"]\"",sample->infos.SNInfs[iter].sInfID);
									memcpy(&sample->iface[iter].sComType , &sample->infos.sComType , MAX_SN_COM_NAME);
									memcpy(&sample->iface[iter].sInfID , &sample->infos.SNInfs[iter].sInfID , MAX_SN_INF_ID);
									//memcpy(&sample->iface[iter].SNInf , &sample->infos.SNInfs[iter] , sizeof(SNInfInfo));
									if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
									if(strncmp(line,"Interface_Health=",17) == 0) {
										parserBuf[0] = 0;
										sscanf(line+17,"\"%4095[^\"]\"",parserBuf);
										//sample->iface[iter].iHealth = atoi(parserBuf);
										iter++;
									}
								}
							}
							
							if(iter == sample->ifaceNum) break;
						}
					} else goto load_ini_error;
								
					for(iter = 0 ; iter < sample->ifaceNum ; iter++) {
						ADV_DEBUG("device.iface[%d].sComType = %s\n", iter, sample->iface[iter].sComType);
						ADV_DEBUG("device.iface[%d].sInfID = %s\n", iter, sample->iface[iter].sInfID);
						//ADV_DEBUG("device.iface[%d].psTopology = %s\n", iter, sample->iface[iter].psTopology);
						//ADV_DEBUG("device.iface[%d].iSizeTopology = %d\n", iter, sample->iface[iter].iSizeTopology);
						//ADV_DEBUG("device.iface[%d].psSenNodeList = %s\n", iter, sample->iface[iter].psSenNodeList);
						//ADV_DEBUG("device.iface[%d].iSizeSenNodeList = %d\n", iter, sample->iface[iter].iSizeSenNodeList);
						//ADV_DEBUG("device.iface[%d].iHealth = %d\n", iter, sample->iface[iter].iHealth);
					}

					
				}
			}
		}
		
		fclose(base);
		return sample->ifaceNum;


load_ini_error:
		fclose(base);
	}
	return 0;
}


int LoadSensorFile(const char *path, IOT_Device* device, IOT_Sensor* defaultvalue, int index) {
	ADV_TRACE("path = %s\n",path);
	int ret;
	FILE* base = NULL;
	char *line = NULL;
	char *pos = NULL;
	int ith = 0;
	int typeId = -1;
	char parserBuf[4096] = {0};
	IOT_Device* sample = NULL;
	IOT_Mote* mote = NULL;
	size_t len = 0;
	ssize_t read;
	base = fopen(path,"r");
	if(base != NULL) {
		sample = device;
		while ((read = getline(&line, &len, base)) != -1) {
			if(strncmp(line,"[SensorHub]",11) == 0) {
				if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
				if(strncmp(line,"Sensor_Product=",15) == 0) {
					mote = &(sample->mote[index]);
					sscanf(line+15,"\"%31[^\"]\"",mote->info.sProduct);
					if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
					if(strncmp(line,"Sensor_Id=",10) == 0) {
						sscanf(line+10,"\"%31[^\"]\"",mote->info.sUID);
						strcpy(mote->info.sSN, mote->info.sUID);
						snprintf(mote->info.sHostName, MAX_SN_HOSTNAME, "%s-%s", mote->info.sProduct, mote->info.sUID+strlen(mote->info.sUID)-4);
						if((read = getline(&line, &len, base)) == -1) break;
						if(strncmp(line,"Sensor_Connect_To=",18) == 0) {
							sscanf(line+18,"\"%4095[^\"]\"",parserBuf);
							mote->connect = strdup(parserBuf);
							mote->reset = 0;
							while((read = getline(&line, &len, base)) != -1) {
								if(strncmp(line,"Sensor_Element_Size=",20) == 0) {
									sscanf(line+20,"\"%4095[^\"]\"",parserBuf);
									mote->sensorNum = atoi(parserBuf);
									mote->sensor = (IOT_Sensor *)malloc(mote->sensorNum * sizeof(IOT_Sensor));
									memset(mote->sensor, 0, mote->sensorNum * sizeof(IOT_Sensor));
									ith = 0;

									while ((read = getline(&line, &len, base)) != -1) {
										if(strncmp(line,"Sensor_Type=",12) == 0) {
											sscanf(line+12,"\"%4095[^\"]\"",parserBuf);
											typeId = GetSensorTypeIndex(parserBuf, gWsnTypeList);
											
											//reset to default
											memcpy(&mote->sensor[ith], &defaultvalue[typeId], sizeof(IOT_Sensor));
											
											strcpy(mote->sensor[ith].type, parserBuf);
											strcpy(mote->sensor[ith].pattern, gWsnTypeList[typeId].pattern);
											ReplaceChar(mote->sensor[ith].pattern,'|','\"');
											strcpy(mote->sensor[ith].standard, gWsnTypeList[typeId].standard);
											ReplaceChar(mote->sensor[ith].standard,'|','\"');
											
											if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
											if(strncmp(line,"Sensor_Info=",12) == 0) {
												parserBuf[0] = 0;
												ret = sscanf(line+12,"\"%4095[^\"]\"",parserBuf);
												if(ret > 0) ElementParser(parserBuf, &mote->sensor[ith]);
												
												
												
												
												
												if((read = getline(&line, &len, base)) == -1) goto load_ini_error;
												if(strncmp(line,"Sensor_Value=",13) == 0) {
													parserBuf[0] = 0;
													ret = sscanf(line+13,"\"%4095[^\"]\"",parserBuf);
													if(ret > 0) ElementParser(parserBuf, &mote->sensor[ith]);
													if((read = getline(&line, &len, base)) == -1) break;
													if(strncmp(line,"Sensor_Trigger_Mode=",20) == 0) {
														sscanf(line+20,"\"%31[^\"]\"",parserBuf);
														mote->sensor[ith].secoff = -1;
														if(strcmp(parserBuf,"Edge") == 0) {
															mote->sensor[ith].trigger = TRIGGER_EDGE;
															mote->sensor[ith].tsec = 1;
														} else {
															mote->sensor[ith].trigger = TRIGGER_LEVEL;
															pos = strchr(parserBuf, '@');
															if(pos == NULL) {
																mote->sensor[ith].tsec = 1;
															} else {
																mote->sensor[ith].tsec = atoi(pos+1);
															}
														}
														ith++;
													} else goto load_ini_error;
												} else goto load_ini_error;
											} else goto load_ini_error;
										}
										
										if(ith == mote->sensorNum) break;
									}
								}
							}
						} else goto load_ini_error;
					} else goto load_ini_error;
					break;
				}
				

				ADV_DEBUG("device.mote[%d].info.sProduct = %s\n", index, mote->info.sProduct);
				ADV_DEBUG("device.mote[%d].info.sUID = %s\n", index, mote->info.sUID);
				ADV_DEBUG("device.mote[%d].info.sHostName = %s\n", index, mote->info.sHostName);
				ADV_DEBUG("device.mote[%d].info.sSN = %s\n", index, mote->info.sSN);
				
				ADV_DEBUG("device.mote[%d].connect = %s\n", index, mote->connect);
				ADV_DEBUG("device.mote[%d].sensorNum = %d\n", index, mote->sensorNum);
				for(ith = 0 ; ith < sample->mote[index].sensorNum ; ith++) {
					ADV_DEBUG("device.mote[%d].sensor[%d].type = %s\n", index, ith, mote->sensor[ith].type);
					ADV_DEBUG("device.mote[%d].sensor[%d].n = %s\n", index, ith, mote->sensor[ith].n);
					ADV_DEBUG("device.mote[%d].sensor[%d].u = %s\n", index, ith, mote->sensor[ith].u);
					ADV_DEBUG("device.mote[%d].sensor[%d].v = %s\n", index, ith, mote->sensor[ith].v);
					ADV_DEBUG("device.mote[%d].sensor[%d].vtype = %d\n", index, ith, mote->sensor[ith].vtype);
					ADV_DEBUG("device.mote[%d].sensor[%d].min = %s\n", index, ith, mote->sensor[ith].min);
					ADV_DEBUG("device.mote[%d].sensor[%d].max = %s\n", index, ith, mote->sensor[ith].max);
					ADV_DEBUG("device.mote[%d].sensor[%d].trigger = %d\n", index, ith, mote->sensor[ith].trigger);
					ADV_DEBUG("device.mote[%d].sensor[%d].pattern = %s\n", index, ith, mote->sensor[ith].pattern);
					ADV_DEBUG("device.mote[%d].sensor[%d].standard = %s\n", index, ith, mote->sensor[ith].standard);
				}

			}
			
		}
		
		fclose(base);
		return sample->ifaceNum;


load_ini_error:
		fclose(base);
	}
	return 0;
}

int LoadIniDir(const char *path, IOT_Device* device, IOT_Sensor* defaultvalue) {


	DIR           *d;
	struct dirent *dir;
	char file[256] = {0};
	int num = 0;
	int iter = 0;
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(dir->d_name[0] != '.' && dir->d_name[0] != '@') {
				num++;
			}
		}
		closedir(d);
		
		device->moteNum = num;
		device->mote = (IOT_Mote *)malloc(device->moteNum * sizeof(IOT_Mote));
		memset(device->mote, 0, device->moteNum * sizeof(IOT_Mote));
	}
	
	d = opendir(path);
	if (d) {
		while ((dir = readdir(d)) != NULL) {
			if(dir->d_name[0] != '.') {
				if(dir->d_name[0] != '@') {
					snprintf(file,256,"%s/%s",path,dir->d_name);
					LoadSensorFile(file, device, defaultvalue, iter++);
				} else {
					snprintf(file,256,"%s/%s",path,dir->d_name);
					LoadInterfaceFile(file, device);
				}
			}
		}
		closedir(d);
	}
	return ++num;
}

int GetNodeIndex(char *str, char **nodeArray, int nodeNum) {
	int i ;
	for(i = 0 ; i < nodeNum ; i++) {
		if(strncmp(nodeArray[i], str, strlen(str)) == 0) {
			return i;
		}
	}
	return -1;
}

/*******************************************************/
static std::map<std::string, int> sensorIdList;
void SetSensorIdByMac(char *mac, int id) {
	std::string key(mac);
	sensorIdList[key] = id;
}

int GetSensorIdByMac(char *mac) {
	std::string key(mac);
	return sensorIdList[key];
}

static std::map<std::string, int> interfaceIdList;
void SetInterfaceIdByMac(char *mac, int id) {
	std::string key(mac);
	interfaceIdList[key] = id;
}

int GetInterfaceIdByMac(char *mac) {
	std::string key(mac);
	return interfaceIdList[key];
}
/*******************************************************/

bool QSortCompare(IOT_Link right, IOT_Link left)
{
	  if(left.a == -1 && left.b == -1) return 1;
	  if(right.a > left.a) return 0;
	  else if(right.a == left.a) {
		  if(right.b > left.b) return 0;
		  else if(right.b == left.b) return 0;
		  else return 1;
	  } else return 1;
}


void ClearRuleValue(IOT_Sensor *sensor) {
	IOT_SimRule *previous;
	IOT_SimRule *rule = sensor->rule;
	while(rule != NULL) {
		previous = rule;
		rule = rule->next;
		delete previous->process;
		delete previous;
	}
	sensor->rule = NULL;
}

int SetInterface(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetInterfaceIdByMac(mac);
	int i = 0;
	char buffer[1024];
	
	strcpy(output->psType,"Info");
	
	if(strcmp("Name", sensor.n) == 0) {
		strcpy(device->infos.SNInfs[id].sInfID, sensor.v);
		sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, sensor.v);
		if(*output->iSizeData < (int)strlen(buffer)+1) {
			*output->iSizeData = strlen(buffer)+1;
			output->psData = (char *)realloc(output->psData,*output->iSizeData);
		}
		strcpy(output->psData,buffer);
	} else {
		IOT_InterfaceInfo *target = &device->faceinfo[id];
		if(strcmp("reset", sensor.n) == 0) {
			if(target->reset != -1) {
				target->reset = atoi(sensor.v);
			}
			sprintf(buffer, "{\"n\":\"%s\", \"v\":\"%s\"}",sensor.n, sensor.v);
			if(*output->iSizeData < (int)strlen(buffer)+1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
			
			//Reset All motes of this interface
			char *set = strtok(target->senHubList, ",\"");
			while(set != NULL) {
				int mid = GetSensorIdByMac(set);
				ADV_INFO("Reset: id = %d, mac = %s\n",mid, set);
				device->mote[mid].reset = 1;
				set = strtok(NULL, ",\"");
			}
			
		} else if(strcmp("SenHubList", sensor.n) == 0) {
			return SN_ER_READ_ONLY;
		} else if(strcmp("Neighbor", sensor.n) == 0) {
			return SN_ER_READ_ONLY;
		} else if(strcmp("Health", sensor.n) == 0) {
			return SN_ER_READ_ONLY;
		} else if(strcmp("sw", sensor.n) == 0) {
			return SN_ER_READ_ONLY;
		}
	}
	return SN_OK;
}


int GetInterface(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetInterfaceIdByMac(mac);
	int i = 0;
	char buffer[1024];
	
	strcpy(output->psType,"Info");
	
	if(strcmp("Name", sensor.n) == 0) {
		sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, device->infos.SNInfs[id].sInfID);
		if(*output->iSizeData < (int)strlen(buffer)+1) {
			*output->iSizeData = strlen(buffer)+1;
			output->psData = (char *)realloc(output->psData,*output->iSizeData);
		}
		strcpy(output->psData,buffer);
	} else {
		IOT_InterfaceInfo *target = &device->faceinfo[id];
		if(strcmp("reset", sensor.n) == 0) {
			if(target->reset != -1) {
				target->reset = atoi(sensor.v);
			}
			sprintf(buffer, "{\"n\":\"%s\", \"v\":\"%d\"}",sensor.n, target->reset < 1 ? 0 : 1);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
		} else if(strcmp("SenHubList", sensor.n) == 0) {
			sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, target->senHubList);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
		} else if(strcmp("Neighbor", sensor.n) == 0) {
			sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, target->neighbor);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
		} else if(strcmp("Health", sensor.n) == 0) {
			sprintf(buffer, "{\"n\":\"%s\", \"v\":\"%d\"}",sensor.n, target->health);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
		} else if(strcmp("sw", sensor.n) == 0) {
			sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, target->version);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
		}
	}
	return SN_OK;
}


int SetSensor(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetSensorIdByMac(mac);
	int i = 0;
	IOT_Mote *target = &device->mote[id];
	IOT_Sensor *element = NULL;
	char dest[4096];
	for(i = 0 ; i < device->mote[id].sensorNum ; i++) {
		if(strcmp(target->sensor[i].n, sensor.n) == 0) {
			if(target->sensor[i].responsesec >= 0) {
				element = &target->sensor[i];
				ClearRuleValue(element);
				strcpy(element->v, sensor.v);

				switch(element->vtype) {
					case VTYPE_NUM:
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"v\":%s}", element->n, element->v);
					break;
					case VTYPE_BIN:
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"bv\":%s}", element->n, element->v);
					break;
					case VTYPE_VSTRING:	
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"sv\":\"[%s]\"}", element->n, element->v);
					break;
					
				}
				
				if (*output->iSizeData < (int)strlen(dest) + 1) {
					*output->iSizeData = strlen(dest)+1;
					output->psData = (char *)realloc(output->psData,*output->iSizeData);
				}
				strcpy(output->psData,dest);

				if(target->sensor[i].responsesec != 0) {
					sleep(target->sensor[i].responsesec);
				}
				break;
			} else {
				return target->sensor[i].responsesec;
			}
		}
	}
	return SN_OK;
}

int GetSensor(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetSensorIdByMac(mac);
	int i = 0;
	IOT_Mote *target = &device->mote[id];
	IOT_Sensor *element = NULL;
	char dest[4096];
	for(i = 0 ; i < device->mote[id].sensorNum ; i++) {
		if(strcmp(target->sensor[i].n, sensor.n) == 0) {
			if(target->sensor[i].responsesec >= 0) {
				element = &target->sensor[i];

				switch(element->vtype) {
					case VTYPE_NUM:
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"v\":%s}", element->n, element->v);
					break;
					case VTYPE_BIN:
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"bv\":%s}", element->n, element->v);
					break;
					case VTYPE_VSTRING:	
						snprintf(dest,sizeof(dest),"{\"n\":\"%s\",\"sv\":\"[%s]\"}", element->n, element->v);
					break;
					
				}
				
				if (*output->iSizeData < (int)strlen(dest) + 1) {
					*output->iSizeData = strlen(dest)+1;
					output->psData = (char *)realloc(output->psData,*output->iSizeData);
				}
				strcpy(output->psData,dest);

				if(target->sensor[i].responsesec != 0) {
					sleep(target->sensor[i].responsesec);
				}
				break;
			} else {
				return target->sensor[i].responsesec;
			}
		}
	}
	return SN_OK;
}

int SetInfo(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetSensorIdByMac(mac);
	int i = 0;
	char buffer[1024];
	IOT_Mote *target = &device->mote[id];
	if(strcmp("Name", sensor.n) == 0) {
		if(target->sensor[i].responsesec >= 0) {
			strcpy(target->info.sHostName, sensor.v);
			sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, sensor.v);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
			
			if(target->sensor[i].responsesec != 0) {
				sleep(target->sensor[i].responsesec);
			}
		} else {
			return target->sensor[i].responsesec;
		}
	} else if(strcmp("reset", sensor.n) == 0) {
		if(target->reset != -1) {
			target->reset = atoi(sensor.v);
		}
		sprintf(buffer, "{\"n\":\"%s\", \"v\":\"%s\"}",sensor.n, sensor.v);
		if (*output->iSizeData < (int)strlen(buffer) + 1) {
			*output->iSizeData = strlen(buffer)+1;
			output->psData = (char *)realloc(output->psData,*output->iSizeData);
		}
		strcpy(output->psData,buffer);
	}
	return SN_OK;
}


int GetInfo(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output) {
	int id = GetSensorIdByMac(mac);
	int i = 0;
	char buffer[1024];
	IOT_Mote *target = &device->mote[id];
	if(strcmp("Name", sensor.n) == 0) {
		if(target->sensor[i].responsesec >= 0) {
			sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}",sensor.n, target->info.sHostName);
			if (*output->iSizeData < (int)strlen(buffer) + 1) {
				*output->iSizeData = strlen(buffer)+1;
				output->psData = (char *)realloc(output->psData,*output->iSizeData);
			}
			strcpy(output->psData,buffer);
			
			if(target->sensor[i].responsesec != 0) {
				sleep(target->sensor[i].responsesec);
			}
		} else {
			return target->sensor[i].responsesec;
		}
	} else if(strcmp("reset", sensor.n) == 0) {
		sprintf(buffer, "{\"n\":\"%s\", \"v\":\"%d\"}",sensor.n, target->reset < 1 ? 0 : 1);
		if (*output->iSizeData < (int)strlen(buffer) + 1) {
			*output->iSizeData = strlen(buffer)+1;
			output->psData = (char *)realloc(output->psData,*output->iSizeData);
		}
		strcpy(output->psData,buffer);
	}
	return SN_OK;
}


void SyncVariable(IOT_Sensor *sensor, IOT_Device *device) {
	int i, j, count;
	int typeId = 0;
	IOT_Sensor *target;
	
	char *token = NULL;
	char buffer[4096];
	char *pos = NULL;
	char temp[4096];
	int nodeNum = device->ifaceNum + device->moteNum;
	IOT_Link connect;
	std::set<IOT_Link,bool(*)(IOT_Link,IOT_Link)> link(QSortCompare);
	std::map<std::string,int> nodeMap;
	std::vector<IOT_Link> *ifaceList = new std::vector<IOT_Link>[device->ifaceNum];
	std::map<int, int> *ifaceListMap = new std::map<int, int>[device->ifaceNum];
	std::map<int, std::string> *ifaceMacMap = new std::map<int, std::string>[device->ifaceNum];
	
	std::set<int> *ifaceSet = new std::set<int>[device->ifaceNum];
	std::pair<std::set<int>::iterator,bool> ret;

	for(i = 0 ; i < device->ifaceNum ; i++) {
		nodeMap[device->infos.SNInfs[i].sInfID] = i;
		SetInterfaceIdByMac(device->infos.SNInfs[i].sInfID, i);
	}

	for(i = 0 ; i < device->moteNum ; i++) {
		nodeMap[device->mote[i].info.sUID] = i+device->ifaceNum;
		SetSensorIdByMac(device->mote[i].info.sUID, i);
		for(j = 0 ; j < device->mote[i].sensorNum ; j++) {
			target = &device->mote[i].sensor[j];
			typeId = GetSensorTypeIndex(target->type, sensor);
			
			/*if(target->vtype == 0) {
				target->vtype = sensor[typeId].vtype;
			}*/
			
			if(sensor[typeId].n[0] == 0) {
				target->n[0] = 0;
			} /*else {
				if(target->n[0] == 0) {
					strcpy(target->n, sensor[typeId].n);
				}
			}*/
			
			if(sensor[typeId].u[0] == 0) {
				target->u[0] = 0;
			} /*else {
				if(target->u[0] == 0) {
					strcpy(target->u, sensor[typeId].u);
				}
			}*/
			
			if(sensor[typeId].v[0] == 0) {
				target->v[0] = 0;
				target->vtype = VTYPE_NONE;
			} /*else {
				if(target->v[0] == 0) {
					strcpy(target->v, sensor[typeId].v);
					target->vtype = sensor[typeId].vtype;
				}
			}*/
			
			if(sensor[typeId].min[0] == 0) {
				target->min[0] = 0;
			} /*else {
				if(target->min[0] == 0) {
					strcpy(target->min, sensor[typeId].min);
				}
			}*/
			
			if(sensor[typeId].max[0] == 0) {
				target->max[0] = 0;
			} /*else {
				if(target->max[0] == 0) {
					strcpy(target->max, sensor[typeId].max);
				}
			}*/
		}
		device->mote[i].modify = 1;
	}
	
	
	for(i = 0 ; i < device->moteNum ; i++) {
		typeId = nodeMap[device->mote[i].info.sUID];
		strncpy(buffer,device->mote[i].connect,sizeof(buffer));
		token = strtok(buffer, ",");
		while(token != NULL) {
			connect.a = typeId;
			connect.b = nodeMap[token];
			
			if(connect.a > connect.b) {
				j = connect.a;
				connect.a = connect.b;
				connect.b = j;
			}
			link.insert(connect);
			token = strtok(NULL, ",");
		}
	}
	
	/*for (std::set<IOT_Link>::iterator it=link.begin(); it!=link.end(); ++it) {
		printf("link: a = %d, b = %d\n",it->a,it->b);
	}*/
	//printf("\n");
	
	
	std::set<int>::iterator elm;
	do {
		for (std::set<IOT_Link,bool(*)(IOT_Link,IOT_Link)>::iterator it = link.begin(); it != link.end();) {
			if (it->a < device->ifaceNum) {
				ifaceList[it->a].push_back(*it);
				ret = ifaceSet[it->a].insert(it->b);
				link.erase(it++);
			}
			else {
				count = 0;
				for (i = 0; i < device->ifaceNum; i++) {
					elm = ifaceSet[i].find(it->a);
					if (elm != ifaceSet[i].end()) {
						ifaceList[i].push_back(*it);
						ret = ifaceSet[i].insert(it->b);
						count++;
						link.erase(it++);
						break;
					}
					elm = ifaceSet[i].find(it->b);
					if (elm != ifaceSet[i].end()) {
						ifaceList[i].push_back(*it);
						ifaceSet[i].insert(it->a);
						count++;
						link.erase(it++);
						break;
					}
				}
				if (count == 0) it++;
			}
		}
	} while (link.size() != 0);

	for(i = 0 ; i < device->ifaceNum ; i++) {
		ifaceListMap[i][i] = 0;
		for (std::set<int>::iterator it=ifaceSet[i].begin(); it!=ifaceSet[i].end();++it) {
			ifaceListMap[i][*it] = ifaceListMap[i].size() - 1;
		}
	}
	int len;
	for(i = 0 ; i < device->ifaceNum ; i++) {
		memset(buffer,0,sizeof(buffer));
		token = buffer;
		
		//Topology
		/*memset(temp,0,sizeof(temp));
		token = buffer;
		token += sprintf(token,"{\"n\":\"Topology\",\"sv\":\"");
		pos = temp;
		for (std::vector<IOT_Link>::iterator it=ifaceList[i].begin(); it!=ifaceList[i].end();it++) {
			pos += sprintf(pos,"%d-%d,"
						,ifaceListMap[i][it->a]
						,ifaceListMap[i][it->b]);
		}
		if(ifaceList[i].size() != 0) {
			pos--;
			*pos = 0;
		}

		token += sprintf(token,pos);
		token += sprintf(token,"\"}, ");
		
		ADV_DEBUG("topology = %s\n", pos);
		token = buffer;
		*/
		
		//device->iface[i].iSizeTopology = strlen(buffer);
		//device->iface[i].psTopology = strdup(buffer);
		
		
		
		//SenHubList
		memset(temp,0,sizeof(temp));
		token += sprintf(token,"{\"n\":\"SenHubList\",\"sv\":\"");
		pos = temp;
		count = 0;
		ifaceMacMap[i][count] = device->infos.SNInfs[i].sInfID;
		for (std::set<int>::iterator it=ifaceSet[i].begin(); it!=ifaceSet[i].end();it++) {
			ADV_DEBUG("if(%d): node = %d\n",i,*it);
			count++;
			ifaceMacMap[i][count] = *it < device->ifaceNum ? device->infos.SNInfs[*it].sInfID : device->mote[*it - device->ifaceNum].info.sUID;
			pos += sprintf(pos,"%s,"
						,ifaceMacMap[i][count].c_str());
			//printf("temp = %s\n", temp);
		}
		if(ifaceSet[i].size() != 0) {
			pos--;
			*pos = 0;
		}
		
		device->faceinfo[i].senHubList = strdup(temp);
		
		token += sprintf(token,"%s",temp);
		token += sprintf(token,"\"}, ");
		ADV_DEBUG("SenHubList = %s\n", buffer);
		
		
		//Neighbor
		memset(temp,0,sizeof(temp));
		token += sprintf(token,"{\"n\":\"Neighbor\",\"sv\":\"");
		pos = temp;
		for (std::vector<IOT_Link>::iterator it=ifaceList[i].begin(); it!=ifaceList[i].end();it++) {
			if(it->a == i) {
				pos += sprintf(pos,"%s,"
						,ifaceMacMap[i][ifaceListMap[i][it->b]].c_str());
			} else if(it->b == i) {
				pos += sprintf(pos,"%s,"
						,ifaceMacMap[i][ifaceListMap[i][it->a]].c_str());
			}
			
			//printf("%d-%d,",ifaceListMap[i][it->a], ifaceListMap[i][it->b]);
		}
		//printf("\n");
		if(ifaceList[i].size() != 0) {
			pos--;
			*pos = 0;
		}

		device->faceinfo[i].neighbor = strdup(temp);
		token += sprintf(token,"%s",temp);
		token += sprintf(token,"\"}, ");
		
		ADV_DEBUG("Neighbor = %s\n", buffer);
		
		
		//Health
		token += sprintf(token,"{\"n\": \"Health\",\"v\": 90}, ");
		device->faceinfo[i].health = 90;
		
		//sw version
		token += sprintf(token,"{\"n\": \"sw\",\"sv\":\"1.0.0.3\"}, ");
		device->faceinfo[i].version = strdup("1.0.0.3");
		
		//reset status
		token += sprintf(token,"{\"n\": \"reset\",\"bv\": 0}, ");
		device->faceinfo[i].reset = 0;
		
		len = strlen(buffer);
		if(token != buffer) {
			buffer[len-1]=0;
			buffer[len-2]=0;
		}
		
		ADV_DEBUG("result = %s\n", buffer);
		//device->iface[i].iSizeSenNodeList = strlen(buffer);
		//device->iface[i].psSenNodeList = strdup(buffer);
		
		device->iface[i].inDataClass.iTypeCount = 1;
		device->iface[i].inDataClass.pInBaseDataArray= (InBaseData *)malloc(sizeof(InBaseData));
		
		InBaseData *info = &device->iface[i].inDataClass.pInBaseDataArray[0];
		info->iSizeData = strlen(buffer);
		info->psData = strdup(buffer);
		
		info->psType = strdup("Info");
		info->iSizeType = sizeof(info->psType);
		
		//*device->infos.SNInfs[i].outDataClass.pOutBaseDataArray.iSizeData = *info->iSizeData;
		//strcpy(device->infos.SNInfs[i].outDataClass.pOutBaseDataArray.psData, info->psData);
		
	}


	delete[] ifaceList;
	delete[] ifaceListMap;
	delete[] ifaceMacMap;
	delete[] ifaceSet;
}




