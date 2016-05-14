/*
 * Copyright (c) 2015, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 */
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "iniparser.h"

#include "uri_cmd.h"
#include "ipso_id.h"
#include "DustWsnDrv.h"
#include "adv_payload_parser.h"

#define INI_FILE_PATH "./ini"

//============================================
//   Local functions
//============================================
char *removeSpaces(char *str)
{
	char *p1 = str, *p2 = str;
	do
	while (*p2 == ' ')
		p2++;
	while (*p1++ = *p2++);
}

unsigned char* skip_ws(unsigned char *str)
{
	while( *str == ' ') {
		str++;
	}

	return str;
}

int add_sensor_spec(Sensor_Info_t *_psensorInfo)
{
    dictionary  *ini;
	unsigned char ini_name[10] = {0}; // xxxx.ini
	Sensor_Info_t *pSenInfo;

    /* Some temporary variables to hold query results */
    int          b;
    int          i;
    double       d;
    char        *s;

	pSenInfo = _psensorInfo;
	//printf("%s: addr:%p, %s/%s.ini\n", __func__, pSenInfo, INI_FILE_PATH, pSenInfo->objId);
	sprintf(ini_name, "%s/%s.ini", INI_FILE_PATH, pSenInfo->objId);
    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }

	switch (atoi(pSenInfo->objId)) {
		case OBJ_ID_IPSO_DIGITAl_INPUT: // 3200

		case OBJ_ID_IPSO_DIGITAl_OUTPUT: // 3201

		case OBJ_ID_IPSO_ANALOGUE_INPUT: // 3202

		case OBJ_ID_IPSO_LUMINOSITY_SENSOR: // 3301

		case OBJ_ID_IPSO_TEMPERATURE_SENSOR: // 3303
		case OBJ_ID_IPSO_HUMIDITY_SENSOR: // 3304
		case OBJ_ID_IPSO_ACCELERO_METER: //3313
		case OBJ_ID_IPSO_MAGNETO_METER: // 3314
		case OBJ_ID_CO2_METER: // 9001

		case OBJ_ID_CO_METER: // 9002

		case OBJ_ID_ULTRASONIC_METER: // 9200
		    /* Get ml.1 attributes */
	    	s = iniparser_getstring(ini, "ml.1:units", NULL);
	    	sprintf(pSenInfo->unit, "%s", s ? s : "");
		    s = iniparser_getstring(ini, "ml.1:rt", NULL);
    		sprintf(pSenInfo->resType, "%s", s ? s : "");
		    s = iniparser_getstring(ini, "ml.1:st", NULL);
    		sprintf(pSenInfo->format, "%s", s ? s : "");
	    	s = iniparser_getstring(ini, "ml.1:type", NULL);
	    	sprintf(pSenInfo->valueType, "%s", s ? s : "");
	    	s = iniparser_getstring(ini, "ml.1:asm", NULL);
	    	sprintf(pSenInfo->mode, "%s", s ? s : "");
		    i = iniparser_getint(ini, "ml.1:minrangevalue", -1);
   	 		sprintf(pSenInfo->minValue,"%d", i);
    		i = iniparser_getint(ini, "ml.1:maxrangevalue", -1);
	    	sprintf(pSenInfo->maxValue, "%d", i);
		break;

		default:
			printf("Error: sensor spec not found!\n");
	}
    iniparser_freedict(ini);

    return 0 ;
}

unsigned char* next_sensor_id(unsigned char *str, unsigned int *nextid)
{
	unsigned char sensoridx[2] = {0};
	int i=0;

	// skip start ','
	if( *str == ',' )
		str++;
	while( *str != ',' && *str != '\0' && isdigit(*str) ) {
		sensoridx[i++] = *str++;
	}
	//printf("%s: sensoridx=%s->%d\n", __func__, sensoridx, atoi(sensoridx));
	*nextid = atoi(sensoridx);
	return str;
}

// Get index of list by sensor id
int get_sensor_index(unsigned int _isensorid, Sensor_List_t *_sensorlist)
{
	int i;

	for(i=0; i<_sensorlist->total; i++) {
		if(_sensorlist->item[i].index == _isensorid) {
			return i;
		}
	}
	return -1;
}

//============================================
//   Public functions
//============================================

int adv_payloadParser_sensor_list(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	unsigned char *cur_pos;
	unsigned int nid;
	int total_sensors = 0;

	assert(_payload);
	assert(_sensorlist);

	cur_pos = _payload;

	if( memcmp(URI_SENSOR_LIST_HDR, cur_pos, URI_SENSOR_LIST_HDR_LEN) != 0 ) {
		printf("Error: sensor list header not found!");
		return -1;
	}
	cur_pos += URI_SENSOR_LIST_HDR_LEN;
	cur_pos = skip_ws(cur_pos);

	if(*cur_pos != ';')
		return -2;
	cur_pos++;

	while(*cur_pos != '\0') {
		cur_pos = next_sensor_id(cur_pos, &nid);
		//printf("cur_pos=%s, nid=%d\n", cur_pos, nid);
		_sensorlist->item[total_sensors].index = nid;
		//_sensorlist->item[nid].index = nid;
		total_sensors++;
	}
	_sensorlist->total = total_sensors;

	return 0;
}

// return idx
int adv_payloadParser_sensor_info(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	unsigned char *cur_pos;
	unsigned char sensorID[4] = {0};
	int iSensorID = 0;
	int idx = -1;
	int i = 0;

	assert(_payload);
	assert(_sensorlist);

	cur_pos = _payload;

	if(*cur_pos != '<')
		return -1;
	cur_pos++; // skip '<'
	cur_pos++; // skip '/'
	i=0;
	while(*cur_pos != '>' && *cur_pos != '\0' && isdigit(*cur_pos)) {
		sensorID[i++] = *cur_pos++;
	}
	//printf("sensor id = %s\n", sensorID);
	iSensorID = atoi(sensorID); // convert to int
	cur_pos++; // skip '>'

	if(*cur_pos != ';')
		return -2;
	cur_pos++;

	if( memcmp(URI_NAME_STR, cur_pos, URI_NAME_STR_LEN) != 0 ) {
		printf("Error: name string not found!\n");
		return -3;
	}
	cur_pos += URI_NAME_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -4;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	if((idx = get_sensor_index(iSensorID, _sensorlist)) == -1) {
		return -5;
	}

	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
	//while(*cur_pos != '"' && *cur_pos != '\0' && isalpha(*cur_pos)) {
		//_sensorlist->item[iSensorID].name[i++] = *cur_pos++;
		_sensorlist->item[idx].name[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -6;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_MODEL_STR, cur_pos, URI_MODEL_STR_LEN) != 0 ) {
		printf("Error: model string not found!\n");
		return -7;
	}
	cur_pos += URI_MODEL_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -8;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	i=0;
	//while(*cur_pos != '"' && *cur_pos != '\0' && isdigit(*cur_pos)) {
	while(*cur_pos != '"' && *cur_pos != '\0') {
		//_sensorlist->item[iSensorID].model[i++] = *cur_pos++;
		_sensorlist->item[idx].model[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -9;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_SENSOR_SID_STR, cur_pos, URI_SENSOR_SID_STR_LEN) != 0 ) {
		printf("Error: sid string not found!\n");
		return -10;
	}
	cur_pos += URI_SENSOR_SID_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -11;
	}
	cur_pos += URI_EQUAL_STR_LEN;
	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
		//_sensorlist->item[iSensorID].objId[i++] = *cur_pos++;
		_sensorlist->item[idx].objId[i++] = *cur_pos++;
	}

	/*_sensorlist->item[idx].SensorQueryState = Sensor_Query_SensorInfo_Received;
	//add_sensor_spec(&_sensorlist->item[iSensorID]);
	add_sensor_spec(&_sensorlist->item[idx]);*/
	return iSensorID;
}

char *ReplaceChar(char *str, char find, char replace) {
    char *ret=str;
    char *wk, *s;
	char *r;
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

int adv_payloadParser_sensor_standard(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	//</0>;asm="r", rt="ucum.Cel", type="d", st="ipso", u="Cel"
	char *string = _payload;
	int idx = -1;
	ReplaceChar(string,' ', -1);
	string = strtok(string, ";,");
	while(string != NULL){
		switch(string[0]) {
			case '<':
				sscanf(string,"</%d>",&idx);
			break;
			case 'a':
				if(idx != -1) {
					sscanf(string,"asm=\"%[^\"]\"",_sensorlist->item[idx].mode);
				}
			break;
			case 'r':
				if(idx != -1) {
					sscanf(string,"rt=\"%[^\"]\"",_sensorlist->item[idx].resType);
				}
			break;
			case 't':
				if(idx != -1) {
					sscanf(string,"type=\"%[^\"]\"",_sensorlist->item[idx].valueType);
				}
			break;
			case 's':
				if(idx != -1) {
					sscanf(string,"st=\"%[^\"]\"",_sensorlist->item[idx].format);
				}
			break;
			case 'u':
				if(idx != -1) {
					sscanf(string,"u=\"%[^\"]\"",_sensorlist->item[idx].unit);
				}
			break;
		}
		string = strtok(NULL, ";,");
	}
	return idx;
}

int adv_payloadParser_sensor_hardware(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	//</0>;min="-20", max="150"
	char *string = _payload;
	int idx = -1;
	//char number[32] = {0};
	ReplaceChar(string,' ', -1);
	
	string = strtok(_payload, ";,");
	while(string != NULL){
		switch(string[0]) {
			case '<':
				sscanf(string,"</%d>",&idx);
				
			break;
			case 'm':
				switch(string[1]) {
					case 'i':
						if(idx != -1) {
							sscanf(string,"min=\"%[^\"]\"",_sensorlist->item[idx].minValue);
						}
					break;
					case 'a':
						if(idx != -1) {
							sscanf(string,"max=\"%[^\"]\"",_sensorlist->item[idx].maxValue);
						}
					break;
				}
			break;
			case 'X':
				if(idx != -1) {
					sscanf(string,"X=\"%[^\"]\"",_sensorlist->item[idx].x);
				}
				break;
			case 'Y':
				if(idx != -1) {
					sscanf(string,"Y=\"%[^\"]\"",_sensorlist->item[idx].y);
				}
				break;
			case 'Z':
				if(idx != -1) {
					sscanf(string,"Z=\"%[^\"]\"",_sensorlist->item[idx].z);
				}
				break;
		}
		string = strtok(NULL, ";,");
	}
	return idx;
}

int adv_payloadParser_sensor_data(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	unsigned char *cur_pos;
	unsigned char sensorID[4] = {0};
	int iSensorID = 0;
	int idx = -1;
	int i = 0;

	assert(_payload);
	assert(_sensorlist);

	cur_pos = _payload;

	if(*cur_pos == '\0') {
		return -1;
	}

	while(1) {
		i = 0;
		memset(sensorID, '\0', 4);
		//while(*cur_pos != ' ' && isdigit(*cur_pos)) {
		while(*cur_pos != ' ' && *cur_pos != '\0' && isdigit(*cur_pos)) {
			sensorID[i++] = *cur_pos++;
			if(i>3)
				return -2;
		}
		if(strlen(sensorID) == 0) {
			return -3;
		}
		iSensorID = atoi(sensorID); // convert to int
		//printf("sensor id = %s:%d\n", sensorID, iSensorID);
		cur_pos++; // skip ' '

		if((idx = get_sensor_index(iSensorID, _sensorlist)) == -1) {
			return -4;
		}

		i = 0;
		memset(_sensorlist->item[idx].value, '\0', MAX_VALUE_LEN);
		//memset(_sensorlist->item[iSensorID].value, '\0', MAX_VALUE_LEN);
		while(*cur_pos != ',' && *cur_pos != '\0') {
			//_sensorlist->item[iSensorID].value[i++] = *cur_pos++;
			_sensorlist->item[idx].value[i++] = *cur_pos++;
			if(i>MAX_VALUE_LEN)
				return -5;
		}
		//printf(" val = %s\n", _sensorlist->item[idx].value);
		if(*cur_pos == '\0') {
			break;
		}
		cur_pos++;
		cur_pos = skip_ws(cur_pos);
	}

	return 0;
}

int parse_cancel_observe(unsigned char *_payload, Sensor_List_t *_sensorlist)
{
	unsigned char *cur_pos;
	unsigned char sensorID[4] = {0};
	int i = 0;

	assert(_payload);
	assert(_sensorlist);

	cur_pos = _payload;

	if( memcmp(URI_CANCEL_OBSERVE_STR, cur_pos, URI_CANCEL_OBSERVE_STR_LEN) != 0 ) {
		printf("Error: cancel observe header not found!");
		return -1;
	}
	cur_pos += URI_CANCEL_OBSERVE_STR_LEN;

	while(*cur_pos != '\0') {
		sensorID[i++] = *cur_pos++;
		if(i>3)
			return -2;
	}
	printf("cancel id = %s:%d\n", sensorID, atoi(sensorID));

	return 0;
}

int adv_payloadParser_mote_info(unsigned char *_payload, Mote_Info_t *_moteinfo)
{
	unsigned char *cur_pos;
	int i = 0;

	assert(_payload);
	assert(_moteinfo);

	cur_pos = _payload;

	if( memcmp(URI_MOTE_INFO_HDR, cur_pos, URI_MOTE_INFO_HDR_LEN) != 0 ) {
		//printf("Error: dev info header not found!");
		return -1;
	}
	cur_pos += URI_MOTE_INFO_HDR_LEN;

	if(*cur_pos != ';')
		return -2;
	cur_pos++;

	if( memcmp(URI_NAME_STR, cur_pos, URI_NAME_STR_LEN) != 0 ) {
		return -3;
	}
	cur_pos += URI_NAME_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -4;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	i=0;
	memset(_moteinfo->hostName, 0, sizeof(_moteinfo->hostName));
	while(*cur_pos != '"' && *cur_pos != '\0') {
		_moteinfo->hostName[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -5;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_MODEL_STR, cur_pos, URI_MODEL_STR_LEN) != 0 ) {
		return -6;
	}
	cur_pos += URI_MODEL_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -7;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
		_moteinfo->productName[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -8;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_SW_STR, cur_pos, URI_SW_STR_LEN) != 0 ) {
		return -9;
	}
	cur_pos += URI_SW_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -10;
	}
	cur_pos += URI_EQUAL_STR_LEN;
	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
		_moteinfo->softwareVersion[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -11;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_RESET_STR, cur_pos, URI_RESET_STR_LEN) != 0 ) {
		return -12;
	}
	cur_pos += URI_RESET_STR_LEN;
	if( *cur_pos != '=' ) {
		return -13;
	}
	cur_pos++;
	_moteinfo->bReset = atoi(cur_pos);

	return 0;

}

int adv_payloadParser_net_info(unsigned char *_payload, Mote_Info_t *_moteinfo)
{
	unsigned char *cur_pos;
	int i = 0;

	assert(_payload);
	assert(_moteinfo);

	cur_pos = _payload;

	if( memcmp(URI_NET_INFO_HDR, cur_pos, URI_NET_INFO_HDR_LEN) != 0 ) {
		printf("Error: net info header not found!");
		return -1;
	}
	cur_pos += URI_NET_INFO_HDR_LEN;

	if(*cur_pos != ';')
		return -2;
	cur_pos++;

	if( memcmp(URI_NAME_STR, cur_pos, URI_NAME_STR_LEN) != 0 ) {
		return -3;
	}
	cur_pos += URI_NAME_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -4;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
		_moteinfo->netName[i++] = *cur_pos++;
	}
	cur_pos++; // skip '"'
	if(*cur_pos != ',')
		return -5;
	cur_pos++;
	cur_pos = skip_ws(cur_pos);
	if( memcmp(URI_SW_STR, cur_pos, URI_SW_STR_LEN) != 0 ) {
		return -6;
	}
	cur_pos += URI_SW_STR_LEN;
	if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
		return -7;
	}
	cur_pos += URI_EQUAL_STR_LEN;

	i=0;
	while(*cur_pos != '"' && *cur_pos != '\0') {
		_moteinfo->netSWversion[i++] = *cur_pos++;
	}

	return 0;

}

int adv_payloadParser_sensor_cmdback(unsigned char *_payload, Mote_Info_t *_moteinfo)
{
	unsigned char *cur_pos;
	int i;

	assert(_payload);
	assert(_moteinfo);

	cur_pos = _payload;

	switch (_moteinfo->cmdType) {
		case Mote_Cmd_SetSensorValue:
			
			break;
		case Mote_Cmd_SetMoteName:
			if( memcmp(URI_NAME_STR, cur_pos, URI_NAME_STR_LEN) != 0 ) {
				printf("Error: Unknow return data!");
				return -1;
			}
			cur_pos += URI_NAME_STR_LEN;
			if( memcmp(URI_EQUAL_STR, cur_pos, URI_EQUAL_STR_LEN) != 0 ) {
				return -2;
			}
			cur_pos += URI_EQUAL_STR_LEN;
			i=0;
			memset(_moteinfo->hostName, 0, sizeof(_moteinfo->hostName));
			while(*cur_pos != '"' && *cur_pos != '\0') {
				_moteinfo->hostName[i++] = *cur_pos++;
			}
			break;
		case Mote_Cmd_SetMoteReset:
			if( memcmp(URI_RESET_STR, cur_pos, URI_RESET_STR_LEN) != 0 ) {
				printf("Error: Unknow return data!");
				return -1;
			}
			cur_pos += URI_RESET_STR_LEN;
			if( *cur_pos != '=' ) {
				return -2;
			}
			cur_pos++;
			_moteinfo->bReset = atoi(cur_pos);
			break;
		case Mote_Cmd_SetAutoReport:
			
			break;
		default:
			printf("Warning: Unknown command:%d\n", _moteinfo->cmdType);
			break;
	}

	return 0;
}

int adv_payloadParser_sensor_cancelback(unsigned char *_payload, Mote_Info_t *_moteinfo)
{
	unsigned char *cur_pos;
	int i;
	unsigned char sensorID[4] = {0};

	assert(_payload);
	assert(_moteinfo);

	cur_pos = _payload;
	//printf("moteID=%d, _payload = %s\n", _moteinfo->moteId, _payload);

	if( memcmp(URI_CANCEL_OBSERVE_STR, cur_pos, URI_CANCEL_OBSERVE_STR_LEN) != 0 ) {
		//printf("Error: Unknow return data!\n");
		return -1;
	}
	cur_pos += URI_CANCEL_OBSERVE_STR_LEN;

	i=0;
	while(*cur_pos != '\0') {
		sensorID[i++] = *cur_pos++;
		if(i>3)
			return -2;
	}
	//printf("%s: cancel id = %s\n", __func__, sensorID);

	return 0;
}

