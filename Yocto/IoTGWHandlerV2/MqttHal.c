/*
 * Copyright (c) 2016, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 * ChangeLog:
 *  2016/01/18 Chinchen: Initial version
 */

#if defined(WIN32) //windows
#define _WINSOCKAPI_
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>
#include <Ws2tcpip.h>
#else
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>   //ifreq
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <mosquitto.h>
#include "list.h"
#include "AdvJSON.h"
#include "AdvLog.h"
#include "mqtt_client_shared.h"
#include "MqttHal.h"
#include "SensorNetwork_APIex.h"
#include "unistd.h"
#include "IoTGWHandler.h"

#define MQTT_RESPONSE_TIMEOUT 3

static pthread_t       g_tid = 0;
static bool            g_doUpdateInterface;
static bool            process_messages = true;
static int             msg_count = 0;
static struct          mosquitto *g_mosq = NULL;
static struct          mosq_config g_mosq_cfg;

//static int g_qos = 0;
//static int g_retain = 0;
static int g_mid_sent = 0;
static int g_pubResp = 0;
static char g_sessionID[34];

static senhub_list_t   *g_SensorHubList;
static char            g_GWInfMAC[MAX_MACADDRESS_LEN];
char g_connectivity_capability[1024]={0};

#define SET_SENHUB_V_JSON "{\"susiCommData\":{\"sensorIDList\":{\"e\":[{\"v\":%s,\"n\":\"SenHub/%s/%s\"}]},\"handlerName\":\"SenHub\",\"commCmd\":525,\"sessionID\":\"%s\"}}"
#define SET_SENHUB_BV_JSON "{\"susiCommData\":{\"sensorIDList\":{\"e\":[{\"bv\":%s,\"n\":\"SenHub/%s/%s\"}]},\"handlerName\":\"SenHub\",\"commCmd\":525,\"sessionID\":\"%s\"}}"
#define SET_SENHUB_SV_JSON "{\"susiCommData\":{\"sensorIDList\":{\"e\":[{\"sv\":\"%s\",\"n\":\"SenHub/%s/%s\"}]},\"handlerName\":\"SenHub\",\"commCmd\":525,\"sessionID\":\"%s\"}}"

#define SET_DEVNAME_JSON "{\"susiCommData\":{\"commCmd\":113,\"catalogID\":4,\"handlerName\":\"general\",\"sessionID\":\"%s\",\"devName\":\"%s\"}}"

#define SET_SUCCESS_CODE 200

//-----------------------------------------------------------------------------
// Private Function:
//-----------------------------------------------------------------------------
#define NAME_START "{\"n\":\"Name\""
#define NAME_SV ",\"sv\":"
#define NAME_ASM ",\"asm\""
int replaceName(char *_input, char *_strName)
{
	char tmp[1024];
	char *cur;
	int seek;

	memset(tmp, 0, sizeof(tmp));
	cur = strstr(_input, NAME_START);
	cur += strlen(NAME_START);
	
	seek = cur - _input;
	strncpy(tmp, _input, seek);
	strcat(tmp, NAME_SV);
	strcat(tmp, "\"");
	strcat(tmp, _strName);
	strcat(tmp, "\"");
	cur = strstr(cur, NAME_ASM);
	strcat(tmp,cur);
	//printf("..............strlen=%d\n", strlen(_input));
	memset(_input, 0, strlen(_input));
	strncpy(_input, tmp, strlen(tmp));

	return 0;
}

bool isResponseTimeout(time_t _inTime)
{
	time_t curretTime;

	time(&curretTime);

	if (difftime(curretTime, _inTime) > MQTT_RESPONSE_TIMEOUT) {
		return true;
	} else {
		return false;
	}
}

int GetSusiCommand(JSONode *json, int* piSusiCommand){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_SUSI_COMMAND, nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }

    *piSusiCommand=atoi(nodeContent);
    return 0;
}

int ReplyGetSetRequest(char* ptopic, JSONode *json, int cmdID){

    char nodeContent[MAX_JSON_NODE_SIZE];
    char sessionID[256]={0};
    char sensorInfoList[256]={0};
    char tmp_sensorHubUID[64]={0};
    char sensorHubUID[64]={0};
    char respone_data[1024]={0};

    //message topic
    printf("topic = %s\n", ptopic);

    //Get sensorHub UID
    sscanf(ptopic, "/%*[^/]/%*[^/]%s", tmp_sensorHubUID);
    printf("tmp_sensorHubUID: %s \n", tmp_sensorHubUID);
    //sscanf(tmp_sensorHubUID,"%*[^/]/%[^/]%s", sensorHubUID);
    strcpy(sensorHubUID,tmp_sensorHubUID+1);
    char* ptmp_ptr=strstr(sensorHubUID,"/");
    if ( ptmp_ptr == NULL){
        return -1;
    }   
    *ptmp_ptr=0; 
    printf("sensorHubUID=%s, len=%d\n", sensorHubUID, strlen(sensorHubUID));

    //Get sessionID
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, "[susiCommData][sessionID]", nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    strcpy(sessionID,nodeContent);
    printf("sessionID = %s\n", sessionID);

    //Get sensorInfoList
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, "[susiCommData][sensorInfoList]", nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    strcpy(sensorInfoList,nodeContent);
    printf("sensorInfoList = %s\n", sensorInfoList);

    //
    sprintf(respone_data,"{\"sessionID\":\"%s\",\"sensorInfoList\":%s}",sessionID,sensorInfoList);
    printf("response data=%s\n",respone_data);
    ResponseGetData(ptopic, cmdID, sensorHubUID, respone_data, strlen(respone_data));

    return 0;

}

int isRegisterGatewayCapability(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    JSON_Get(json, OBJ_IOTGW_INFO_SPEC, nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }

    return 0;
}

int PrepareInfoToRegisterSensorHub(JSONode *json, senhub_info_t* pshinfo){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    if ( pshinfo == NULL || json == NULL ){
        return -1;
    }

    pshinfo->state=Mote_Report_CMD2001;
    pshinfo->jsonNode = NULL;
    pshinfo->id=1;

    /*Get devID*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_ID, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: DevID=%s\n", __func__, nodeContent);
    strcpy(pshinfo->devID, nodeContent);

    /*Get MAC*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_MAC, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: mac=%s\n", __func__, nodeContent);
    strcpy(pshinfo->macAddress, nodeContent);

    /*Get SN*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_SN, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: sn=%s\n", __func__, nodeContent);
    strcpy(pshinfo->sn, nodeContent);

    /*Get hostname*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_HOSTNAME, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: hostName=%s\n", __func__, nodeContent);
    strcpy(pshinfo->hostName, nodeContent);

    //Get productName
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_PRODUCTNAME, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: productName=%s\n", __func__, nodeContent);
    strcpy(pshinfo->productName, nodeContent);

    //Get version
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_VERSION, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    printf("%s: version=%s\n", __func__, nodeContent);
    strcpy(pshinfo->version, nodeContent);

    return 0;
}

int ParseAgentactionreqTopic(JSONode *json){

    int SusiCommand=-1;

    if ( GetSusiCommand(json, &SusiCommand) == 0){
        printf("SusiCommand = %d\n", SusiCommand);
        switch(SusiCommand){
            case IOTGW_GET_SENSOR_REPLY:
                return REPLY_GET_SENSOR_REQUEST;
            case IOTGW_SET_SENSOR_REPLY:
                return REPLY_SET_SENSOR_REQUEST;
            case IOTGW_HANDLER_GET_CAPABILITY_REPLY:
                {
                    printf("Get capability reply\n");
		    if(isRegisterGatewayCapability(json) == 0){
                        return REGISTER_GATEWAY_CAPABILITY;
		    }
                    break;
                }
            default:
                {
                    printf("SusiCommand = %d not supported\n", SusiCommand);
                    break;
                }
        }
    }
    return -1;
}

int ParseDeviceinfoTopic(JSONode *json){
    
    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_IOTGW_DATA, nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "NULL") != 0){
        return UPDATE_GATEWAY_DATA;
    }
    //
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_SENHUB_DATA, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") != 0){
        return UPDATE_SENSOR_HUB_DATA;
    }

    return -1;
}

int ParseMQTTMessage(char* ptopic, JSONode *json){

    int res = -1;
    int SusiCommand=-1;

    //agentactionreq topic
    if(strcmp(ptopic, AGENTACTIONREQ_TOPIC) == 0) {
        res = ParseAgentactionreqTopic(json);
        if ( res >= 0 ){
            return res;
        }
    }
    //deviceinfo topic
    if(strcmp(ptopic, DEVICEINFO_TOPIC) == 0) {
        res = ParseDeviceinfoTopic(json);
        if ( res >= 0){
            return res;
        }
    }

    return res;
}

int MqttHal_Message_Process(const struct mosquitto_message *message)
{
	char topicType[32];
	JSONode *json;
	char nodeContent[MAX_JSON_NODE_SIZE];
	senhub_info_t *pshinfo;
	int action = 0;
        //printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
	if((json = JSON_Parser(message->payload)) == NULL) {
		printf("json parse err!\n");
		return -1;
	}

	sscanf(message->topic, "/%*[^/]/%*[^/]/%*[^/]/%s", topicType);
	ADV_TRACE("Topic type: %s \n", topicType);
	printf("Topic type: %s \n", topicType);

        action = ParseMQTTMessage(topicType,json);
#if 0
        if ( action < 0){
            JSON_Destory(&json);
            return -1;
        }
#endif
        switch(action){
            case REGISTER_GATEWAY_CAPABILITY:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Register Gateway Capability# \033[0m\n", __FILE__, __func__);
                    RegisterGatewayCapability(json);
                    printf("------------------------------------------------\n");
                    break;
                }
            case REPLY_GET_SENSOR_REQUEST:
                {
                    ReplyGetSetRequest(message->topic,json, IOTGW_GET_SENSOR_REPLY);
                    break;
                }
            case REPLY_SET_SENSOR_REQUEST:
                {
                    ReplyGetSetRequest(message->topic,json, IOTGW_SET_SENSOR_REPLY);
                    break;
                }
            case UPDATE_GATEWAY_DATA:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Update Gateway Data# \033[0m\n", __FILE__, __func__);
                    UpdateGatewayData(json);
                    printf("------------------------------------------------\n");
                    break;
                }
            case UPDATE_SENSOR_HUB_DATA:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Update SensorHub Data# \033[0m\n", __FILE__, __func__);
                    UpdateSensorHubData(json);
                    printf("------------------------------------------------\n");
                }
            default:
                break;
        }

        if(strcmp(topicType, WA_PUB_CONNECT_TOPIC) == 0) {

		memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
		JSON_Get(json, OBJ_DEVICE_TYPE, nodeContent, sizeof(nodeContent));
                if(strcmp(nodeContent, "SenHub") == 0){
                    printf("------------------------------------------------\n");
                    printf("[%s][%s]\033[33m #Register SensorHub# \033[0m\n", __FILE__, __func__);
                    printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);

		    senhub_info_t shinfo;
                    memset(&shinfo,0,sizeof(senhub_info_t));
                    if ( PrepareInfoToRegisterSensorHub(json,&shinfo) < 0){
                        JSON_Destory(&json);
                        return -1;
                    }
                    RegisterSensorHub(&shinfo);
                    printf("\n------------------------------------------------\n");
                }

	} else if(strcmp(topicType, WA_PUB_WILL_TOPIC) == 0) {
		// CmdID=2003
		PRINTF("[%s][%s] Receive messages from will topic!!\n", __FILE__, __func__);
		char macAddr[MAX_MACADDRESS_LEN];
		//JSON_Print(json);
		memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
		JSON_Get(json, OBJ_DEVICE_MAC, nodeContent, sizeof(nodeContent));
		memset(macAddr, 0, MAX_MACADDRESS_LEN);
		sprintf(macAddr, "0017%s" , nodeContent);
		//printf("%s: mac=%s!!\n", __func__, macAddr);
		pshinfo = (senhub_info_t *)senhub_list_find_by_mac(g_SensorHubList, macAddr);
		if(NULL != pshinfo) {
			g_doUpdateInterface = 1;
			SensorHub_DisConn(pshinfo);
			//printf("%s: id=%d!!\n", __func__, pshinfo->id);
			g_SensorHubList = SENHUB_LIST_RM(g_SensorHubList, pshinfo->id, &pshinfo);
			free(pshinfo);
		}
	}
	
	// CmdID=1000
#if 0
	if(g_doUpdateInterface) {
		GatewayIntf_Update();
		g_doUpdateInterface = 0;
	}
#endif

	JSON_Destory(&json);

	return 0;
}

void MqttHal_Message_Callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
	struct mosq_config *cfg;
	int i;
	bool res;

	if(process_messages == false) return;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(message->retain && cfg->no_retain) return;
	if(cfg->filter_outs){
		for(i=0; i<cfg->filter_out_count; i++){
			mosquitto_topic_matches_sub(cfg->filter_outs[i], message->topic, &res);
			if(res) return;
		}
	}

	if(message->payloadlen){
		MqttHal_Message_Process(message);
#if 0
		fwrite(message->payload, 1, message->payloadlen, stdout);
		if(cfg->eol){
			printf("\n");
		}
		fflush(stdout);
#endif
	}

	if(cfg->msg_count>0){
		msg_count++;
		if(cfg->msg_count == msg_count){
			process_messages = false;
			mosquitto_disconnect(mosq);
		}
	}
}

void MqttHal_Connect_Callback(struct mosquitto *mosq, void *obj, int result)
{
	int i;
	struct mosq_config *cfg;

	assert(obj);
	cfg = (struct mosq_config *)obj;

	if(!result){
		for(i=0; i<cfg->topic_count; i++){
			mosquitto_subscribe(mosq, NULL, cfg->topics[i], cfg->qos);
		}
	}else{
		if(result && !cfg->quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void MqttHal_Disconnect_Callback(struct mosquitto *mosq, void *obj, int rc)
{
	//connected = false;
}

void MqttHal_Publish_Callback(struct mosquitto *mosq, void *obj, int mid)
{
#if 0
	last_mid_sent = mid;
	if(mode == MSGMODE_STDIN_LINE){
		if(mid == last_mid){
			mosquitto_disconnect(mosq);
			disconnect_sent = true;
		}
	}else if(disconnect_sent == false){
		mosquitto_disconnect(mosq);
		disconnect_sent = true;
	}
#endif
}

static void *MqttHal_Thread(void *arg) {
	//int tmp = (int *)arg;
	int rc;
/*ivan del start 20160521*/
#if 0	
	while(UpdateCbf_IsSet() != 0) {
		//ADV_WARN("Callback function is not registered.\n");
		sleep(1);
	}
#endif
/*ivan del end*/
	ADV_INFO("Callback function registered.\n");
	
	MqttHal_Init();
	rc = mqtt_client_connect(g_mosq, &g_mosq_cfg);
	if(rc) {
		ADV_WARN("MQTT Connect Fail(%d).\n", rc);
	} else {
		ADV_INFO("MQTT Client start.\n");
		//printf("MQTT Client start.\n");
		mosquitto_loop_forever(g_mosq, -1, 1);
		ADV_INFO("MQTT Client leave.\n");
		//printf("MQTT Client leave.\n");
	}
	MqttHal_Uninit();

	pthread_exit(NULL);
}

//-----------------------------------------------------------------------------
// Public Function:
//-----------------------------------------------------------------------------
int MqttHal_GetMacAddrList(char *pOutBuf, const int outBufLen, int withHead)
{
    senhub_list_t *target;
	char *pos = NULL;

	if (NULL==pOutBuf)
		return -1;

    target = g_SensorHubList;
	if(withHead == 0)
    	target = target->next;
	pos = pOutBuf;

    while (target != NULL) {
		//printf("%s: id=%d, mac=%s\n", __func__, target->id, target->macAddress);
		pos += sprintf(pos, "%s,", target->macAddress);
        target = target->next;
    }

	if(strlen(pOutBuf)) {
		pos--;
		*pos=0;
	}
	//printf("%s: string len=%d (%s)\n", __func__, strlen(pOutBuf), pOutBuf);

	return 0;
}

int MqttHal_GetNetworkIntfaceMAC(char *_ifname, char* _ifmac)
{
	#if defined(WIN32) //windows
	IP_ADAPTER_INFO AdapterInfo[16];			// Allocate information for up to 16 NICs
	DWORD dwBufLen = sizeof(AdapterInfo);		// Save the memory size of buffer

	DWORD dwStatus = GetAdaptersInfo(			// Call GetAdapterInfo
		AdapterInfo,							// [out] buffer to receive data
		&dwBufLen);								// [in] size of receive data buffer
	//assert(dwStatus == ERROR_SUCCESS);			// Verify return value is valid, no buffer overflow

	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;// Contains pointer to current adapter info
	//display mac address
	//printf("Mac : %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2], pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
	sprintf(_ifmac, "%02x%02x%02x%02x%02x%02x", pAdapterInfo->Address[0], pAdapterInfo->Address[1], pAdapterInfo->Address[2], pAdapterInfo->Address[3], pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
	#else
	int fd;
	struct ifreq ifr;
	char *iface = _ifname;
	unsigned char *mac = NULL;

	memset(&ifr, 0, sizeof(ifr));

	fd = socket(AF_INET, SOCK_DGRAM, 0);

	ifr.ifr_addr.sa_family = AF_INET;
	strncpy(ifr.ifr_name , iface , IFNAMSIZ-1);

	if (0 == ioctl(fd, SIOCGIFHWADDR, &ifr)) {
		mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
		//display mac address
		//printf("Mac : %.2X:%.2X:%.2X:%.2X:%.2X:%.2X\n" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
		sprintf(_ifmac, "%02x%02x%02x%02x%02x%02x" , mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
	}
	close(fd);

	return 0;
	#endif
}

int MqttHal_Init()
{
	int rc = 0;
	//struct mosq_config mosq_cfg;
	senhub_info_t *pshinfo;
	g_SensorHubList = NULL;
	
	//ADV_INFO("%s: \n", __func__);
	rc = mqtt_client_config_load(&g_mosq_cfg, CLIENT_SUB, 1, NULL);
	if(rc){
		mqtt_client_config_cleanup(&g_mosq_cfg);
		return 1;
	}

	mosquitto_lib_init();

	if(mqtt_client_id_generate(&g_mosq_cfg, "advmqttcli")){
		return 1;
	}

	g_mosq = mosquitto_new(g_mosq_cfg.id, g_mosq_cfg.clean_session, &g_mosq_cfg);
	if(!g_mosq){
		switch(errno){
			case ENOMEM:
				if(!g_mosq_cfg.quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!g_mosq_cfg.quiet) fprintf(stderr, "Error: Invalid id and/or clean_session.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	if(mqtt_client_opts_set(g_mosq, &g_mosq_cfg)){
		return 1;
	}

	mosquitto_connect_callback_set(g_mosq, MqttHal_Connect_Callback);
	mosquitto_message_callback_set(g_mosq, MqttHal_Message_Callback);

    mosquitto_disconnect_callback_set(g_mosq, MqttHal_Disconnect_Callback);
    mosquitto_publish_callback_set(g_mosq, MqttHal_Publish_Callback);

/*ivan del*/
#if 0
	// Create senhub root
	pshinfo = malloc(sizeof(senhub_info_t));
	memset(pshinfo, 0, sizeof(senhub_info_t));
	sprintf(pshinfo->macAddress, "0000%s" , g_GWInfMAC);
	pshinfo->jsonNode = NULL;
	pshinfo->id = senhub_list_newId(g_SensorHubList);
	//printf("%s: list add id=%d\n", __func__, pshinfo->id);
	g_SensorHubList = SENHUB_LIST_ADD(g_SensorHubList, pshinfo);
#endif 
	return rc;
}

int MqttHal_Uninit()
{
	mosquitto_destroy(g_mosq);
	mosquitto_lib_cleanup();

	return 0;
}

void MqttHal_Proc()
{
	int mqtt_context = 0;

	// manager id (LAN or WLAN MAC)
	memset(g_GWInfMAC, 0, MAX_MACADDRESS_LEN);
	MqttHal_GetNetworkIntfaceMAC(IFACENAME, g_GWInfMAC);
	printf("GW Intf Mac : %s\n" , g_GWInfMAC);

	if(g_tid == 0) {
		pthread_create(&g_tid, NULL, &MqttHal_Thread, &mqtt_context);
		ADV_INFO("%s: thread id(%d)\n", __func__, (int)g_tid);
	}

}

void MqttHal_UpdateIntf(int bUpdate)
{
	//printf("%s: %d\n", __func__, bUpdate);
	g_doUpdateInterface = bUpdate;
}

senhub_info_t* MqttHal_GetMoteInfoByMac(char *strMacAddr)
{
	return (senhub_info_t *)senhub_list_find_by_mac(g_SensorHubList, strMacAddr);
}

int MqttHal_Publish(char *macAddr, int cmdType, char *strName, char *strValue)
{
	int rc = MOSQ_ERR_SUCCESS;
	char topic[128];
	char message[1024];
	int msglen = 0;
	time_t currTime;
	//char seesionID[34];

	//printf("\033[33m %s(%d): cmdType=%d, strName=%s, strValue=%s \033[0m\n", __func__, __LINE__, cmdType, strName, strValue);
	#ifndef WIN32
	srandom(time(NULL));
	#endif
	memset(topic, 0, sizeof(topic));
	memset(message, 0, sizeof(message));
	memset(g_sessionID, 0, sizeof(g_sessionID));
	sprintf(g_sessionID, "99C21CCBBFE40F528C0EDDF9%08X", rand());
	switch(cmdType) {
		case Mote_Cmd_SetSensorValue:
			sprintf(message, SET_SENHUB_V_JSON, strValue, "SenData", strName, g_sessionID);
			break;
		case Mote_Cmd_SetMoteName:
			//sprintf(message, SET_SENHUB_SV_JSON, strValue, "Info", strName, g_sessionID);
			sprintf(message, SET_DEVNAME_JSON, g_sessionID, strValue);
			break;
		case Mote_Cmd_SetMoteReset:
			sprintf(message, SET_SENHUB_BV_JSON, strValue, "Info", strName, g_sessionID);
			break;
		case Mote_Cmd_SetAutoReport:
			sprintf(message, SET_SENHUB_BV_JSON, strValue, "Action", strName, g_sessionID);
			break;
		default:
			printf("%s: not support this cmd=%d!\n", __func__, cmdType);
			return -1;
	}
	
	//printf("\033[33m %s: %s \033[0m\n", __func__, message);
	sprintf(topic, "/cagent/admin/%s/%s", macAddr, WA_SUB_CBK_TOPIC);
	msglen = strlen(message);
	rc = mosquitto_publish(g_mosq, &g_mid_sent, topic, msglen, message, g_mosq_cfg.qos, g_mosq_cfg.retain);
	if(rc == MOSQ_ERR_SUCCESS) {
		time(&currTime);
		g_pubResp = 0;
		while(!g_pubResp) {
			//printf("%s: wait \n", __func__);
			if(isResponseTimeout(currTime)) {
				printf("%s: pub timeout!\n", __func__);
				break;
			}
		}
		//printf("\033[33m %s: got respone(%d) \033[0m\n", __func__, g_pubResp);
	}

	if(g_pubResp != SET_SUCCESS_CODE) {
		return -2;
	}

	return 0;
}

int MqttHal_PublishV2(char *macAddr, int cmdType, const char *pData)
{
	int rc = MOSQ_ERR_SUCCESS;
	char topic[128];
	char message[1024];
	int msglen = 0;
	time_t currTime;
	//char seesionID[34];
        printf("-----------------------------------------------------------------------------------\n");
	printf("\033[33m %s(%d):\033[0m\n", __func__, __LINE__);
	#ifndef WIN32
	srandom(time(NULL));
	#endif
	memset(topic, 0, sizeof(topic));;
	memset(message, 0, sizeof(message));
        strcpy(message,pData);
	//memset(g_sessionID, 0, sizeof(g_sessionID));
	//sprintf(g_sessionID, "99C21CCBBFE40F528C0EDDF9%08X", rand());
#if 0
	switch(cmdType) {
		case Mote_Cmd_SetSensorValue:
			sprintf(message, SET_SENHUB_V_JSON, strValue, "SenData", strName, g_sessionID);
			break;
		case Mote_Cmd_SetMoteName:
			//sprintf(message, SET_SENHUB_SV_JSON, strValue, "Info", strName, g_sessionID);
			sprintf(message, SET_DEVNAME_JSON, g_sessionID, strValue);
			break;
		case Mote_Cmd_SetMoteReset:
			sprintf(message, SET_SENHUB_BV_JSON, strValue, "Info", strName, g_sessionID);
			break;
		case Mote_Cmd_SetAutoReport:
			sprintf(message, SET_SENHUB_BV_JSON, strValue, "Action", strName, g_sessionID);
			break;
		default:
			printf("%s: not support this cmd=%d!\n", __func__, cmdType);
			return -1;
	}
#endif	
	printf("\033[33m %s: %s \033[0m\n", __func__, message);
	sprintf(topic, "/cagent/admin/%s/%s", macAddr, WA_SUB_CBK_TOPIC);
	msglen = strlen(message);
	rc = mosquitto_publish(g_mosq, &g_mid_sent, topic, msglen, message, g_mosq_cfg.qos, g_mosq_cfg.retain);
        printf("####-----------------------------------------------------------------------------------####\n");
#if 0
	if(rc == MOSQ_ERR_SUCCESS) {
		time(&currTime);
		g_pubResp = 0;
		while(!g_pubResp) {
			//printf("%s: wait \n", __func__);
			if(isResponseTimeout(currTime)) {
				printf("%s: pub timeout!\n", __func__);
				break;
			}
		}
		//printf("\033[33m %s: got respone(%d) \033[0m\n", __func__, g_pubResp);
	}

	if(g_pubResp != SET_SUCCESS_CODE) {
		return -2;
	}
#endif
	return 0;
}
