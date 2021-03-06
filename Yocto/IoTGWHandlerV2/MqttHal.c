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
#include <netdb.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <mosquitto.h>
#include "AdvJSON.h"
#include "AdvLog.h"
#include "mqtt_client_shared.h"
#include "MqttHal.h"
#include "unistd.h"
#include "IoTGWHandler.h"
#include "common.h"

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

//static senhub_list_t   *g_SensorHubList;
char            g_GWInfMAC[MAX_MACADDRESS_LEN];
//char g_connectivity_capability[1024]={0};

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

/******************************************************/
struct connectivityInfoNode{
    int index;
    char type[MAX_CONNECTIVITY_TYPE_LEN];
    char Info[1024];
};

struct connectivityInfoNode g_ConnectivityInfoNodeList[256]={0};

extern CAGENT_MUTEX_TYPE g_NodeListMutex;
extern struct node* g_pNodeListHead;


int DeleteNodeList_AllGatewayUIDNode(char* devID)
{
    struct node *temp, *prev;
    temp=g_pNodeListHead;
    while(temp!=NULL)
    {
        if( strcmp(temp->virtualGatewayDevID,devID) == 0 ){
            if(temp==g_pNodeListHead){
                g_pNodeListHead=temp->next;
                if (temp->connectivityInfo){
                    free(temp->connectivityInfo);
                }
                free(temp);
                temp=g_pNodeListHead;
            }
            else{
                prev->next=temp->next;
                if (temp->connectivityInfo){
                    free(temp->connectivityInfo);
                }
                free(temp);
                temp= prev->next;
            }
        }
        else{
            prev=temp;
            temp= temp->next;
        }
    }

    return 0;
}

int DeleteNodeList_AllDisconnectedGatewayUIDNode(){

        struct node * r;
        ADV_DEBUG("[%s][%s] Delete all gateway dvice id node START\n", __FILE__, __func__);
        r=g_pNodeListHead;

        while(r!=NULL)
        {
            if ( r->state == STATUS_DISCONNECTED ){
                if ( r->nodeType == TYPE_GATEWAY) {
                    ADV_C_DEBUG(COLOR_YELLOW, "[%s][%s] Delete all gateway dvice id node (GW:%s)\n", __FILE__, __func__, r->virtualGatewayDevID);
                    DeleteNodeList_AllGatewayUIDNode(r->virtualGatewayDevID);
                    if ( g_pNodeListHead != NULL){
                        r=g_pNodeListHead;
                    }
                    else{
                        break;
                    }
                }
            }
            r=r->next;
        }
        ADV_DEBUG("[%s][%s] Delete all gateway dvice id node END\n", __FILE__, __func__);

}

int GetJSONValue(JSONode *json, char* pPath, char* pResult){
	
    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    memset(nodeContent,0,sizeof(nodeContent));
    JSON_Get(json, pPath, nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "NULL") == 0 ){
        return -1;
    }
    strcpy(pResult,nodeContent);
    return 0;
}

void  DisplayAllVirtualGatewayDataListNode(struct node* head, struct node *r)
{
    r=head;
    if(r==NULL)
    {
        printf("[%s][%s] node list is NULL\n", __FILE__, __func__);
        return;
    }
    while(r!=NULL)
    {
        printf("[%s][%s]\n----------------------------------\n", __FILE__, __func__);
        printf("nodeType:%d\n", r->nodeType);
        printf("virtualGatewayDevID:%s\n",r->virtualGatewayDevID);
        printf("virtualGatewayOSInfo:%d\n", r->virtualGatewayOSInfo);
        printf("connectivityType:%s\n",r->connectivityType);
        printf("connectivityDevID:%s\n",r->connectivityDevID);
        printf("connectivityInfo:%s\n",r->connectivityInfo);
        printf("sensorHubDevID:%s\n",r->sensorHubDevID);
        printf("----------------------------------\n");
        r=r->next;
    }
    printf("\n");
}
 

int CountAllVirtualGatewayDataListNode(struct node* head)
{
    struct node *n;
    int c=0;
    n=head;
    while(n!=NULL)
    {
    n=n->next;
    c++;
    }
    return c;
}

void test_link_list(){

    int cnt=0;
    struct node *n;
    printf("initital g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //add1
    AddNodeList("0000772233445599","WSN","0007112233445501","12345", strlen("12345"), TYPE_SENSOR_HUB, OS_TYPE_UNKNOWN, "0017112233445501");
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("add1, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //add2
    AddNodeList("0000772233445599","WSN","0007112233445502","67",strlen("67"), TYPE_SENSOR_HUB, OS_TYPE_UNKNOWN, "0017112233445502");
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("add2, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //add3
    AddNodeList("0000772233445599","WSN","0007112233445503","890",strlen("890"), TYPE_SENSOR_HUB, OS_TYPE_UNKNOWN, "0017112233445503");
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("add3, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //add3 again: we will delete3 then add3 again
    AddNodeList("0000772233445599","WSN","0007112233445503","77777",strlen("77777"), TYPE_SENSOR_HUB, OS_TYPE_UNKNOWN, "0017112233445503");
    printf("add3 again, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    DisplayAllVirtualGatewayDataListNode(g_pNodeListHead, n);

     
    n = GetNode("0000772233445599",TYPE_GATEWAY);
    if ( n ){
        printf("virtualGatewayDevID = %s\n",n->virtualGatewayDevID);
    }
    //del3
    DeleteNodeList("0017112233445503", TYPE_SENSOR_HUB);
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("del3, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //del2
    DeleteNodeList("0017112233445502", TYPE_SENSOR_HUB);
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("del2, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    //del1
    DeleteNodeList("0017112233445501", TYPE_SENSOR_HUB);
    cnt=CountAllVirtualGatewayDataListNode(g_pNodeListHead);
    printf("del1, g_pVirtualGatewayDataListHead = %p\n",g_pNodeListHead);
    printf("-----------count = %d\n", cnt);
    DisplayAllVirtualGatewayDataListNode(g_pNodeListHead, n);
}

/******************************************************/

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

int ReplyToRMM_SensorHubGetSetRequest(char* ptopic, JSONode *json, int cmdID){

    char nodeContent[MAX_JSON_NODE_SIZE];
    char sessionID[256]={0};
    char sensorInfoList[256]={0};
    char tmp_sensorHubUID[64]={0};
    char sensorHubUID[64]={0};
    char respone_data[1024]={0};

    //message topic
    printf("topic = %s\n", ptopic);

    //Get sensorHub UID
    if ( GetUIDfromTopic(ptopic, sensorHubUID, sizeof(sensorHubUID)) < 0){
        printf("[%s][%s] Can't find sensorHubUID Topic=%s\r\n",__FILE__, __func__, ptopic );
	return -1;
    }

    //Get sessionID
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, "[susiCommData][sessionID]", nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        printf("[%s][%s] [susiCommData][sessionID] value is NULL\n", __FILE__, __func__);
        return -1;
    }
    strcpy(sessionID,nodeContent);
    printf("sessionID = %s\n", sessionID);

    //Get sensorInfoList
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, "[susiCommData][sensorInfoList]", nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        printf("[%s][%s] [susiCommData][sensorInfoList] value is NULL\n", __FILE__, __func__);
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

int isOSIPbase(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    JSON_Get(json, "[susiCommData][osInfo][IP]", nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }

    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, nodeContent, &(sa.sin_addr));
    
    printf("IP=%s, result=%d\n", nodeContent, result);
    if ( result == 0 ){
        return -1;
    }

    return 0;
}


int isRegisterGatewayCapability(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    if ( GetJSONValue(json, OBJ_IOTGW_INFO_SPEC, nodeContent) < 0){
        return -1;
    }

    return 0;
}

int isRegisterSensorHubCapability(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    if ( GetJSONValue(json, OBJ_SENHUB_INFO_SPEC, nodeContent) < 0){
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
    ADV_DEBUG("%s: DevID=%s\n", __func__, nodeContent);
    strcpy(pshinfo->devID, nodeContent);

    /*Get MAC*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_MAC, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    ADV_DEBUG("%s: mac=%s\n", __func__, nodeContent);
    strcpy(pshinfo->macAddress, nodeContent);

    /*Get SN*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_SN, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    ADV_DEBUG("%s: sn=%s\n", __func__, nodeContent);
    strcpy(pshinfo->sn, nodeContent);

    /*Get hostname*/
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_HOSTNAME, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    ADV_DEBUG("%s: hostName=%s\n", __func__, nodeContent);
    strcpy(pshinfo->hostName, nodeContent);

    //Get productName
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_PRODUCTNAME, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    ADV_DEBUG("%s: productName=%s\n", __func__, nodeContent);
    strcpy(pshinfo->productName, nodeContent);

    //Get version
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_VERSION, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        return -1;
    }
    ADV_DEBUG("%s: version=%s\n", __func__, nodeContent);
    strcpy(pshinfo->version, nodeContent);

    return 0;
}

int ParseNotifyTopic(JSONode *json){
    
    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    if ( GetJSONValue(json, "[hb][devID]", nodeContent) < 0 ){
        return -1;
    }

    return GATEWAY_HEART_BEAT;
}

int ParseAgentactionreqTopic(JSONode *json){

    int SusiCommand=-1;
    
    if ( GetSusiCommand(json, &SusiCommand) < 0){
        printf("[%s][%s] get susi command FAIL!\n",__FILE__, __func__);
        return -1;
    }

    switch(SusiCommand){
        case IOTGW_GET_SENSOR_REPLY:
            return REPLY_GET_SENSOR_REQUEST;
        case IOTGW_SET_SENSOR_REPLY:
            return REPLY_SET_SENSOR_REQUEST;
        case IOTGW_QUERY_HEART_BEAT_VALUE_REPLY:
            return IOTGW_QUERY_HEART_BEAT_VALUE_REPLY;
        case IOTGW_CHANGE_HEART_BEAT_VALUE_REPLY:
            return IOTGW_CHANGE_HEART_BEAT_VALUE_REPLY;
        case IOTGW_OS_INFO:
            {
                return GATEWAY_OS_INFO;
            }
        case IOTGW_HANDLER_GET_CAPABILITY_REPLY:
            {
		if(isRegisterGatewayCapability(json) == 0){
                    return REGISTER_GATEWAY_CAPABILITY;
		}

                if(isRegisterSensorHubCapability(json) == 0){
                    return REGISTER_SENSOR_HUB_CAPABILITY;
                }
                 
                printf("[%s][%s] susi cmd=%d, unknown capability type.\n", __FILE__, __func__, IOTGW_HANDLER_GET_CAPABILITY_REPLY );
                break;
            }
        default:
            {
                printf("[%s][%s]SusiCommand = %d not supported\n", __FILE__, __func__, SusiCommand);
                break;
            }
    }

    return -1;
}

int ParseDeviceinfoTopic(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    int SusiCommand=-1;

    if ( GetSusiCommand(json, &SusiCommand) < 0){
        printf("[%s][%s] get susi command FAIL!\n",__FILE__, __func__);
        return -1;
    }

    switch(SusiCommand){
        case IOTGW_UPDATE_DATA:
            {
		memset(nodeContent, 0, MAX_JSON_NODE_SIZE);	
                if ( GetJSONValue(json, OBJ_IOTGW_DATA, nodeContent) == 0){
                    return UPDATE_GATEWAY_DATA;
                }

		memset(nodeContent, 0, MAX_JSON_NODE_SIZE);	
                if ( GetJSONValue(json, OBJ_SENHUB_DATA, nodeContent) == 0){
                    return UPDATE_SENSOR_HUB_DATA;
                }

                printf("[%s][%s] susi cmd=%d, get value FAIL\n", __FILE__, __func__, IOTGW_UPDATE_DATA);
                break;
            }
        default:
            {
                printf("[%s][%s]SusiCommand = %d not supported\n", __FILE__, __func__, SusiCommand);
                break;
            }
    }


    return -1;
}

int ParseAgentinfoackTopic(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    int SusiCommand=-1;

    if ( GetSusiCommand(json, &SusiCommand) < 0){
        printf("[%s][%s] get susi command FAIL!\n",__FILE__, __func__);
        return -1;
    }

    switch(SusiCommand){
        case IOTGW_CONNECT_STATUS:
            {
		memset(nodeContent, 0, MAX_JSON_NODE_SIZE);	
                if ( GetJSONValue(json, OBJ_DEVICE_TYPE, nodeContent) < 0){
                    printf("[%s][%s] susi cmd=%d, get %s value FAIL\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, OBJ_DEVICE_TYPE);
                    return -1;
                }

		if(strcmp(nodeContent, "SenHub") == 0){
                    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
		    if ( GetJSONValue(json, OBJ_DEVICE_STATUS, nodeContent) < 0){
		        printf("[%s][%s] susi cmd=%d, get %s value FAIL\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, OBJ_DEVICE_STATUS);
			return -1;
		    }
                    
                    if ( strcmp(nodeContent,"1") == 0 ){
		        return SENSOR_HUB_CONNECT;
                    }

                    if ( strcmp(nodeContent,"0") == 0 ){
		        return SENSOR_HUB_DISCONNECT;
                    }

                    printf("[%s][%s] susi cmd=%d, UNKNOWN status %s value\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, nodeContent);
		}
                else if(strcmp(nodeContent, "IoTGW") == 0){
                    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
		    if ( GetJSONValue(json, OBJ_DEVICE_STATUS, nodeContent) < 0){
		        printf("[%s][%s] susi cmd=%d, get %s value FAIL\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, OBJ_DEVICE_STATUS);
			return -1;
		    }
                    
                    if ( strcmp(nodeContent,"1") == 0 ){
		        return GATEWAY_CONNECT;
                    }

                    if ( strcmp(nodeContent,"0") == 0 ){
		        return GATEWAY_DISCONNECT;
                    }

                    printf("[%s][%s] susi cmd=%d, UNKNOWN status %s value\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, nodeContent);
                }
                else{
                    printf("[%s][%s] susi cmd=%d, %s wrong type value(%s)\n", __FILE__, __func__, IOTGW_CONNECT_STATUS, OBJ_DEVICE_TYPE, nodeContent);
                    return -1;
                }
                break;
            }
        default:
            {
                printf("[%s][%s]SusiCommand = %d not supported\n", __FILE__, __func__, SusiCommand);
                break;
            }
    }

    return -1;
}

int ParseMQTTMessage(char* ptopic, JSONode *json){

    int res = -1;

    //notify topic
    if(strcmp(ptopic, NOTIFY_TOPIC) == 0) {
        res = ParseNotifyTopic(json);
        if ( res >= 0 ){
            return res;
        }
        printf("[%s][%s]\033[31m #parse notify topic FAIL !# \033[0m\n", __FILE__, __func__);
    }
    
    //agentactionreq topic
    if(strcmp(ptopic, AGENTACTIONREQ_TOPIC) == 0) {
        res = ParseAgentactionreqTopic(json);
        if ( res >= 0 ){
            return res;
        }
        printf("[%s][%s]\033[31m #parse agentactionreq topic FAIL !# \033[0m\n", __FILE__, __func__);
    }
    //deviceinfo topic
    if(strcmp(ptopic, DEVICEINFO_TOPIC) == 0) {
        res = ParseDeviceinfoTopic(json);
        if ( res >= 0){
            return res;
        };
        printf("[%s][%s]\033[31m #parse deviceinfo topic FAIL !# \033[0m\n", __FILE__, __func__);
    }
    //agentinfoack topic
    if(strcmp(ptopic, AGENTINFOACK_TOPIC) == 0) {
        res = ParseAgentinfoackTopic(json);
        if ( res >= 0){
            return res;
        }
        printf("[%s][%s]\033[31m #parse agentinfoack topic FAIL !# \033[0m\n", __FILE__, __func__);
    }

    return res;
}

int FindConnectivityInfoNodeListIndex(char* pType){
    
    int i=0;
    int max_size=sizeof(g_ConnectivityInfoNodeList)/sizeof(struct connectivityInfoNode);

    for(i=0 ; i < max_size; i++){
        if (strlen(g_ConnectivityInfoNodeList[i].type) == 0){
            strcpy(g_ConnectivityInfoNodeList[i].type, pType);
            return i;
        }
        if(strcmp(g_ConnectivityInfoNodeList[i].type, pType) == 0){
            return i;
        }
    }

    return -1;
}


int BuildNodeList_GatewayCapabilityInfo(struct node* head, char* pResult){

    memset(&g_ConnectivityInfoNodeList,0,sizeof(g_ConnectivityInfoNodeList));

    int i=0,index=-1;
    char tmp[1024]={0};
    char capability[1024]={0};
    struct node *r;
    int IPbaseConnectivityNotAdd=1;

    r=head;
    if(r==NULL)
    {
        printf("[%s][%s] node list is null\n",__FILE__, __func__);
        strcpy(pResult, "{\"IoTGW\":{\"ver\":1}}");
        return -1;
    }
    while(r!=NULL)
    {
#if 0
	printf("[%s][%s]\n----------------------------------\n", __FILE__, __func__);
        printf("nodeType:%d\n", r->nodeType);
	printf("virtualGatewayDevID:%s\n",r->virtualGatewayDevID);
        printf("osInfo:%d\n",r->virtualGatewayOSInfo);
	printf("connectivityType:%s\n",r->connectivityType);
	printf("connectivityDevID:%s\n",r->connectivityDevID);
	printf("connectivityInfo:%s\n",r->connectivityInfo);
        printf("sensorHubDevID:%s",r->sensorHubDevID);
	printf("\n----------------------------------\n");
#endif
        //WSN0:{Info...}
        //sprintf(tmp,"\"%s%d\":%s",r->connectivityType, i, r->connectivityInfo);
        index=FindConnectivityInfoNodeListIndex(r->connectivityType);
        if ( index >= 0 ){
            if (r->nodeType == TYPE_CONNECTIVITY){
                if (r->virtualGatewayOSInfo == OS_NONE_IP_BASE){
                    sprintf(tmp,"\"%s%d\":%s",r->connectivityType, g_ConnectivityInfoNodeList[index].index, r->connectivityInfo);
                    strcat(g_ConnectivityInfoNodeList[index].Info,tmp);
                    strcat(g_ConnectivityInfoNodeList[index].Info,",");
                    strcpy(g_ConnectivityInfoNodeList[index].type,r->connectivityType);
                    g_ConnectivityInfoNodeList[index].index++;
                }
                else if (r->virtualGatewayOSInfo == OS_IP_BASE && IPbaseConnectivityNotAdd){

                    IPbaseConnectivityNotAdd=0;

                    sprintf(tmp,"\"%s%d\":%s",IP_BASE_CONNECTIVITY_NAME, g_ConnectivityInfoNodeList[index].index, r->connectivityInfo);
                    strcat(g_ConnectivityInfoNodeList[index].Info,tmp);
                    strcat(g_ConnectivityInfoNodeList[index].Info,",");
                    strcpy(g_ConnectivityInfoNodeList[index].type,IP_BASE_CONNECTIVITY_NAME);
                    g_ConnectivityInfoNodeList[index].index++;
                }
            }
        }
        else{
            printf("[%s][%s] can not find Info node list index\n",__FILE__, __func__);
        }
        //
	r=r->next;
    }
    printf("\n");

    //Pack all connectivity type capability 
    int max=sizeof(g_ConnectivityInfoNodeList)/sizeof(struct connectivityInfoNode);
    for(i=0; i < max ; i++){
        //index=FindConnectivityInfoNodeListIndex(cType[i]);
        if (strlen(g_ConnectivityInfoNodeList[i].Info) != 0 ){
            //sprintf(wsn_capability,"\"WSN\":{%s\"bn\":\"WSN\",\"ver\":1}",g_ConnectivityInfoNodeList[index].Info);
            sprintf(capability,"\"%s\":{",g_ConnectivityInfoNodeList[i].type);
            strcat(capability,g_ConnectivityInfoNodeList[i].Info);
            sprintf(tmp,"\"bn\":\"%s\",",g_ConnectivityInfoNodeList[i].type);
            strcat(capability,tmp);
            strcat(capability, "\"ver\":1}");
            strcpy(g_ConnectivityInfoNodeList[i].Info,capability);
#if 0
            printf("---------------%s capability----------------------------\n", g_ConnectivityInfoNodeList[i].type);
            printf(g_ConnectivityInfoNodeList[i].Info);
            printf("\n-------------------------------------------\n");
#endif
        }
    }
    //Pack gateway capability
    strcpy(pResult,"{\"IoTGW\":{");
    for(i=0; i < max ; i++){
        if (strlen(g_ConnectivityInfoNodeList[i].Info) != 0 ){
            //sprintf(gateway_capability,"{\"IoTGW\":{%s,\"ver\":1}}", g_ConnectivityInfoNodeList[0].Info);
            strcat(pResult, g_ConnectivityInfoNodeList[i].Info);
            strcat(pResult, ",");
        }
    }
    strcat(pResult,"\"ver\":1}}");
    //
    return 0;
}
int GetVirtualGatewayUIDOSInfo(char* uid){

    char tmp_uid[64]={0};
 
    sprintf(tmp_uid, "%s%s",GATEWAY_ID_PREFIX, g_GWInfMAC );
    if ( strcmp(tmp_uid, uid) == 0){
        return OS_IP_BASE;
    }
    else{
        struct node* temp= GetNode(uid,TYPE_GATEWAY);
        if ( temp != NULL ){
            return temp->virtualGatewayOSInfo;
        }
    }

    return OS_TYPE_UNKNOWN;
}

int GetVirtualGatewayUIDfromData(struct node* head, const char *data, char *uid , const int size ){

#if 0 
//sample data to test  
    char mydata[1024]={"{\"susiCommData\":{\"sessionID\":\"350176BE533B485647559A09C0E9F686\",\"sensorIDList\":{\"e\":[{\"n\":\"IoTGW/WSN/0007000E40ABCDEF/Info/Health\"}]},\"commCmd\":523,\"requestID\":0,\"agentID\":\"\",\"handlerName\":\"IoTGW\",\"sendTS\":1465711233}}"}; 

    JSONode *json = JSON_Parser(mydata);
#endif
    JSONode *json = JSON_Parser(data);
    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    char connectivity_uid[64]={0};
    char ip_base_connectivityDevID[64]={0};
    char topic[128]={0};
    int osinfo = OS_NONE_IP_BASE;


    JSON_Get(json, "[susiCommData][sensorIDList][e][0][n]", nodeContent, sizeof(nodeContent));
    printf("\n---------------------------------------------------\n");
    sprintf(topic,"/%s",nodeContent);
    GetUIDfromTopic(topic, connectivity_uid, sizeof(connectivity_uid));
    printf("connectivity uid = %s\n", connectivity_uid);
    printf("---------------------------------------------------\n");

    sprintf(ip_base_connectivityDevID,"0007%s",g_GWInfMAC);
    if ( strcmp(ip_base_connectivityDevID, connectivity_uid) == 0){
        osinfo = OS_IP_BASE;
        sprintf(uid,"0000%s",g_GWInfMAC);
        return 0;
    }


    struct node *r;

    r=head;
    if(r==NULL)
    {
        return -1;
    }
    while(r!=NULL)
    {
        if (strcmp(r->connectivityDevID, connectivity_uid) == 0){
            strcpy(uid,r->virtualGatewayDevID);
            return 0;
        }
        //
	r=r->next;
    }
    printf("\n");
    //
    return -1;
}

int CheckUIDType(struct node* head, const char *uid){

    struct node *r;

    r=head;
    if(r==NULL)
    {
        return TYPE_UNKNOWN;
    }

    while(r!=NULL)
    {
        if (strcmp(r->virtualGatewayDevID, uid) == 0){
            return TYPE_VIRTUAL_GATEWAY;
        }
        //
	r=r->next;
    }
 
    return TYPE_UNKNOWN;
}

int DisconnectToRMM_AllSensorHubNode(struct node* head, char* VirtualGatewayUID){

    struct node *r;

    r=head;
    if(r==NULL)
    {
        return -1;
    }

    while(r!=NULL)
    {
        if (strcmp(r->virtualGatewayDevID, VirtualGatewayUID) == 0){
            if (r->nodeType == TYPE_SENSOR_HUB){
                DisconnectToRMM_SensorHub(r->sensorHubDevID);
            }
        }
        //
	r=r->next;
    }

    return 0;
}

int DisconnectToRMM_AllDisconnectedSensorHubNode(){

        //Disconnect all sensor hub
        struct node *r;

        ADV_DEBUG("[%s][%s] Disconnect all sensor hub START\n", __FILE__, __func__);
        r=g_pNodeListHead;

        while(r!=NULL)
        {
            if ( r->nodeType == TYPE_GATEWAY) {
                if ( r->state == STATUS_DISCONNECTED ){
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s] Disconnect all sensor hub(GW=%s)\n", __FILE__, __func__, r->virtualGatewayDevID);
                    DisconnectToRMM_AllSensorHubNode(g_pNodeListHead, r->virtualGatewayDevID);
                }
            }
            r=r->next;
        }
        ADV_DEBUG("[%s][%s] Disconnect all sensor hub END\n", __FILE__, __func__);
}

int DisconnectToRMM(char* DeviceUID){

                if ( CheckUIDType(g_pNodeListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){
#if 1
                    DisconnectToRMM_AllSensorHubNode(g_pNodeListHead, DeviceUID);
                    DeleteNodeList_AllGatewayUIDNode(DeviceUID);
#if 0
                    struct node* n;
                    DisplayAllVirtualGatewayDataListNode(g_pNodeListHead, n);
#endif
                    //
                    char gateway_capability[2048]={0};
                    BuildNodeList_GatewayCapabilityInfo(g_pNodeListHead, gateway_capability);

		    ADV_DEBUG("---------------Gateway capability----------------------------\n");
		    ADV_DEBUG(gateway_capability);
		    ADV_DEBUG("\n-------------------------------------------\n");
                    if ( RegisterToRMM_GatewayCapabilityInfo(gateway_capability, strlen(gateway_capability)) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s] Register Gateway Capability FAIL !!!\n", __FILE__, __func__);
                        //JSON_Destory(&json);
                        return -1;
                    }

                    char info_data[1024]={0};
                    BuildNodeList_GatewayCapabilityInfoWithData(g_pNodeListHead, info_data);
                    UpdateToRMM_GatewayUpdateInfo(info_data);
#endif
                }
                else{
                    DisconnectToRMM_SensorHub(DeviceUID);
                }
    return 0;
}

int MqttHal_Message_Process(const struct mosquitto_message *message)
{
	char topicType[32];
        char request_cmd[512]={0};
	JSONode *json;
	char nodeContent[MAX_JSON_NODE_SIZE];
	int action = 0;
        int ret=0;
        //printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
	if((json = JSON_Parser(message->payload)) == NULL) {
		ADV_C_ERROR(COLOR_RED, "[%s][%s] json parse err!\n", __FILE__, __func__);
		return -1;
	}
	sscanf(message->topic, "/%*[^/]/%*[^/]/%*[^/]/%s", topicType);
	ADV_DEBUG("Topic type: %s \n", topicType);

        action = ParseMQTTMessage(topicType,json);

        app_os_mutex_lock(&g_NodeListMutex);
        switch(action){
            case GATEWAY_HEART_BEAT:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #HeartBeat# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                     
                   char gateway_devID[128]={0};
                    if ( GetJSONValue(json, "[hb][devID]", gateway_devID) < 0 ){
                        ret=-1;
                        ADV_C_ERROR(COLOR_RED,"[%s][%s] get heart beat devID FAIL!\n", __FILE__, __func__);
                        goto exit;
                    }

                    struct node hb_data;
                    memset(&hb_data,0,sizeof(struct node));
                    time(&hb_data.last_hb_time);
                    if ( UpdateNodeList(gateway_devID, TYPE_GATEWAY, &hb_data) < 0){

                        /*Delete all gateway device ID*/
                        DeleteNodeList_AllGatewayUIDNode(gateway_devID);

                        /*Re-connect device*/ 
                         ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Re-Connect# \033[0m\n", __FILE__, __func__);
                        app_os_mutex_unlock(&g_NodeListMutex);

                        if ( GetRequestCmd(IOTGW_RECONNECT, request_cmd) < 0){
                            ADV_C_ERROR(COLOR_RED, "[%s][%s]GATEWAY_HEART_BEAT: GetRequestCmd FAIL.\n",__FILE__,__func__);
                        }
                        else{
                            if ( SendRequestToWiseSnail(gateway_devID,request_cmd) != 0){
                                ADV_C_ERROR(COLOR_RED, "[%s][%s]GATEWAY_HEART_BEAT: SendRequestToWiseSnail FAIL.\n",__FILE__,__func__);
                            }
                        }
                        JSON_Destory(&json);
                        return;
                    }

                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case GATEWAY_CONNECT:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW, "[%s][%s][%d]\033[33m #Gateway Connect# \033[0m\n", __FILE__, __func__, __LINE__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);


                    if ( AddNodeList_VirtualGatewayNodeInfo(message->payload, OS_TYPE_UNKNOWN) < 0){
                        ADV_C_ERROR(COLOR_RED,"[%s][%s]GATEWAY_CONNECT: Add Node FAIL.\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
                    //
#if 1
                    char gateway_devID[128]={0};
                    if ( GetAgentID(message->payload, gateway_devID, sizeof(gateway_devID)) == 0){

                        struct node node_data;
                        memset(&node_data,0,sizeof(struct node));
                        node_data.state = STATUS_CONNECTED;
                        //time(&node_data.last_hb_time);
                        UpdateNodeList(gateway_devID, TYPE_GATEWAY, &node_data);
#if 1
                        app_os_mutex_unlock(&g_NodeListMutex);
                        //Send "get capability" message to WiseSnail
                        //char mydata[512]={"{\"susiCommData\":{\"requestID\":1001,\"catalogID\": 4,\"commCmd\":2051,\"handlerName\":\"general\"}}"};
                        GetRequestCmd(IOTGW_HANDLER_GET_CAPABILITY_REQUEST, request_cmd);
                        SendRequestToWiseSnail(gateway_devID,request_cmd);
                        JSON_Destory(&json);
#endif
                        return 0;
                    }
                    else{
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]GATEWAY_CONNECT: GetAgentID() FAIL!\n",__FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case GATEWAY_DISCONNECT:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Gateway Disconnect# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    char DeviceUID[64]={0};
                    GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID));
                    //printf("DeviceUID = %s\n", DeviceUID);

		    if ( DisconnectToRMM(DeviceUID) < 0){
		        ADV_C_ERROR(COLOR_RED, "[%s][%s]GATEWAY_DISCONNECT: DisconnectToRMM FAIL !\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
		    }
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case GATEWAY_OS_INFO:
                {
                    ADV_DEBUG("------------------------------------------------\n");
		    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Gateway OS Info# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    int OSInfo=OS_NONE_IP_BASE;

                    if ( isOSIPbase(json) < 0 ){
                        ADV_DEBUG("[%s][%s]OS Info: none-IP base\n", __FILE__, __func__);
                    }
                    else{
                        ADV_DEBUG("[%s][%s]OS Info: IP base\n", __FILE__, __func__);
                        OSInfo=OS_IP_BASE;
                    }
                    //test_link_list();

                    char gateway_devID[128]={0};
                    if ( GetAgentID(message->payload, gateway_devID, sizeof(gateway_devID)) == 0){
                        struct node node_data;
                        memset(&node_data,0,sizeof(struct node));
                        node_data.virtualGatewayOSInfo = OSInfo;
                        UpdateNodeList(gateway_devID, TYPE_GATEWAY, &node_data);
                    }
                    else{
                        ADV_C_ERROR(COLOR_RED, "[%s][%s] case %d: GetAgentID() FAIL!\n",__FILE__, __func__, GATEWAY_OS_INFO);
                    }

                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case REGISTER_GATEWAY_CAPABILITY:
                {
                    ADV_DEBUG("------------------------------------------------\n");
		    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Register Gateway Capability# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    //test_link_list();
#if 1

                    AddNodeList_ConnectivityNodeCapability(message->payload);

                    char gateway_capability[2048]={0};
                    BuildNodeList_GatewayCapabilityInfo(g_pNodeListHead, gateway_capability);


		    ADV_DEBUG("---------------Gateway capability----------------------------\n");
		    ADV_DEBUG(gateway_capability);
		    ADV_DEBUG("\n-------------------------------------------\n");

                    if ( RegisterToRMM_GatewayCapabilityInfo(gateway_capability, strlen(gateway_capability)) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]REGISTER_GATEWAY_CAPABILITY: Register Gateway Capability FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case UPDATE_GATEWAY_DATA:
                {
                    ADV_DEBUG("------------------------------------------------\n");
		    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Update Gateway Data# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);

                    UpdateNodeList_ConnectivityNodeInfo(message->payload);
                    //
                    AddNodeList_SensorHubNodeInfo(message->payload);
#if 0
                    struct node* n;
                    DisplayAllVirtualGatewayDataListNode(g_pNodeListHead, n);
#endif

                    char info_data[1024]={0};
                    int osInfo=GetOSInfoType(message->payload);

                    //Build gateway update info
                    BuildNodeList_GatewayUpdateInfo(message->payload, info_data, osInfo);
#if 1
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_DEBUG("[%s][%s]@@@@@@@@@@@@ OS info type:%d packed info_data=%s\n", __FILE__, __func__, osInfo, info_data);
                    ADV_DEBUG("------------------------------------------------\n");
#endif

#if 1
                    if ( UpdateToRMM_GatewayUpdateInfo(info_data) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]UPDATE_GATEWAY_DATA: Update Gateway Data FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case SENSOR_HUB_CONNECT:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #SensorHub Connect# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
#if 1
                    if ( ConnectToRMM_SensorHub(json) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]SENSOR_HUB_CONNECT: SensorHub connect to RMM FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case SENSOR_HUB_DISCONNECT:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #SensorHub Disconnect# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    char DeviceUID[64]={0};
		    if ( GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID)) < 0){
		        ADV_C_ERROR(COLOR_RED, "[%s][%s]SENSOR_HUB_DISCONNECT: Can't find DeviceUID topic=%s\r\n",__FILE__, __func__, message->topic);
                        ret=-1;
                        goto exit;
		    }
                    DisconnectToRMM_SensorHub(DeviceUID);
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case REGISTER_SENSOR_HUB_CAPABILITY:
                {
                    ADV_DEBUG("------------------------------------------------\n");
		    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Register SensorHub Capability# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
#if 1
                    if(RegisterToRMM_SensorHubCapability(message->topic, json) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]REGISTER_SENSOR_HUB_CAPABILITY: Register SensorHub Capability FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case UPDATE_SENSOR_HUB_DATA:
                {
                    ADV_DEBUG("------------------------------------------------\n");
		   ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Update SensorHub Data# \033[0m\n", __FILE__, __func__);
#if 1
                    if ( UpdateToRMM_SensorHubData(json) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]UPDATE_SENSOR_HUB_DATA: Update SensorHub Data FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("\n------------------------------------------------\n");
                }
            case REPLY_GET_SENSOR_REQUEST:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Reply Get Sensor Request# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    char DeviceUID[64]={0};
		    if ( GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID)) < 0){
		        ADV_C_ERROR(COLOR_RED, "[%s][%s]REPLY_GET_SENSOR_REQUEST: Can't find DeviceUID topic=%s\r\n",__FILE__, __func__, message->topic);
                        ret=-1;
                        goto exit;
		    }
                    //ADV_DEBUG("DeviceUID = %s\n", DeviceUID);
#if 1

                    if ( CheckUIDType(g_pNodeListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){

                        ADV_C_ERROR(COLOR_RED, "[%s][%s]REPLY_GET_SENSOR_REQUEST: found virtual gateway device ID\n", __FILE__, __func__);
                        ReplyToRMM_GatewayGetSetRequest(message->topic,json, IOTGW_GET_SENSOR_REPLY);
                        ret=0;
                        goto exit;
                    }

#endif

#if 1
                    if ( ReplyToRMM_SensorHubGetSetRequest(message->topic,json, IOTGW_GET_SENSOR_REPLY) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]REPLY_GET_SENSOR_REQUEST: Reply Get Sensor Request FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case REPLY_SET_SENSOR_REQUEST:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Reply Set Sensor Request# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);

                    char DeviceUID[64]={0};
		    if ( GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID)) < 0){
		        ADV_C_ERROR(COLOR_RED, "[%s][%s]REPLY_SET_SENSOR_REQUEST: Can't find DeviceUID topic=%s\r\n",__FILE__, __func__, message->topic);
                        ret=-1;
                        goto exit;
		    }
                    //printf("DeviceUID = %s\n", DeviceUID);
#if 1

                    if ( CheckUIDType(g_pNodeListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){

                        ReplyToRMM_GatewayGetSetRequest(message->topic,json, IOTGW_SET_SENSOR_REPLY);
                        ret=0;
                        goto exit;
                    }

#endif
#if 1
                    if ( ReplyToRMM_SensorHubGetSetRequest(message->topic,json, IOTGW_SET_SENSOR_REPLY) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]REPLY_SET_SENSOR_REQUEST: Reply Set Sensor Request FAIL !!!\n", __FILE__, __func__);
                        ret=-1;
                        goto exit;
                    }
#endif
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case IOTGW_QUERY_HEART_BEAT_VALUE_REPLY:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Query HeartBeat Reply# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
		    char hb_rate[128]={0};
		    if ( GetJSONValue(json, "[susiCommData][heartbeatrate]", hb_rate) < 0 ){
		        ret=-1;
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]IOTGW_QUERY_HEART_BEAT_VALUE_REPLY: Reply Query HeartBeat FAIL !!!\n", __FILE__, __func__);
                        goto exit;
		    }
                    ADV_DEBUG("hb_rate=%s\n", hb_rate);
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            case IOTGW_CHANGE_HEART_BEAT_VALUE_REPLY:
                {
                    ADV_DEBUG("------------------------------------------------\n");
                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Change HeartBeat Reply# \033[0m\n", __FILE__, __func__);
                    ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, message->topic);
                    ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
		    char result[128]={0};
		    if ( GetJSONValue(json, "[susiCommData][result]", result) < 0 ){
		        ret=-1;
                        ADV_C_ERROR(COLOR_RED, "[%s][%s]IOTGW_CHANGE_HEART_BEAT_VALUE_REPLY: Change HeartBeat Value FAIL !!!\n", __FILE__, __func__);
                        goto exit;
		    }
                    ADV_DEBUG("result=%s\n", result);
                    ADV_DEBUG("------------------------------------------------\n");
                    break;
                }
            default:
                break;
        }

        if(strcmp(topicType, WA_PUB_WILL_TOPIC) == 0) {
		// CmdID=2003
		ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s] Receive messages from will topic!!\n", __FILE__, __func__);
                ADV_DEBUG("topic = %s\n", message->topic);
                ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                char DeviceUID[64]={0};
                GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID));
                //printf("DeviceUID = %s\n", DeviceUID);

                if ( DisconnectToRMM(DeviceUID) < 0){
                    ADV_C_ERROR(COLOR_RED, "[%s][%s]WA_PUB_WILL_TOPIC: DisconnectToRMM FAIL !\n", __FILE__, __func__);
                    ret=-1;
                    goto exit;
                }
                //
	}
	
exit:
    app_os_mutex_unlock(&g_NodeListMutex);
    JSON_Destory(&json);
    return ret;
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
#if 0
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
#endif
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
	//g_SensorHubList = NULL;
	
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
	//return (senhub_info_t *)senhub_list_find_by_mac(g_SensorHubList, strMacAddr);
        return 0;
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

int SendRequestToWiseSnail(char *macAddr, const char *pData)
{
	int rc = MOSQ_ERR_SUCCESS;
	char topic[128];
	char message[1024];
	int msglen = 0;
	time_t currTime;
	//char seesionID[34];
	#ifndef WIN32
	srandom(time(NULL));
	#endif
	memset(topic, 0, sizeof(topic));;
	memset(message, 0, sizeof(message));
        strcpy(message,pData);

	sprintf(topic, "/cagent/admin/%s/%s", macAddr, WA_SUB_CBK_TOPIC);
	msglen = strlen(message);
	rc = mosquitto_publish(g_mosq, &g_mid_sent, topic, msglen, message, g_mosq_cfg.qos, g_mosq_cfg.retain);

        if ( rc != MOSQ_ERR_SUCCESS ){
            ADV_C_ERROR(COLOR_RED, "[%s][%s] mosquitto_publish error code=%d\n", __FILE__, __func__, rc);
        }

	return rc;
}

int BuildNodeList_GatewayCapabilityInfoWithData(struct node* head, char* pResult){
  
    //printf("##################################################################\n");
    memset(&g_ConnectivityInfoNodeList,0,sizeof(g_ConnectivityInfoNodeList));
    //printf("BuildNodeList_GatewayCapabilityInfoWithData-----------------\n");
    int i=0,index=-1;
    char tmp[1024]={0};
    char capability[1024]={0};
    struct node *r;
    int IPbaseConnectivityNotAdd=1;

    int ip_base_sensor_hub_cnt=0;
    char info_data[1024]={0};

    ip_base_sensor_hub_cnt = BuildNodeList_IPBaseGatewayInfo(info_data);
    //printf("@@@@@@@info_data=%s\n", info_data);
#if 1
    if (ip_base_sensor_hub_cnt > 0){
        index=FindConnectivityInfoNodeListIndex(IP_BASE_CONNECTIVITY_NAME);
        if ( index >= 0 ){
            sprintf(tmp,"\"%s%d\":%s",IP_BASE_CONNECTIVITY_NAME, g_ConnectivityInfoNodeList[index].index, info_data);
            strcat(g_ConnectivityInfoNodeList[index].Info,tmp);
            strcat(g_ConnectivityInfoNodeList[index].Info,",");
            strcpy(g_ConnectivityInfoNodeList[index].type,IP_BASE_CONNECTIVITY_NAME);
            g_ConnectivityInfoNodeList[index].index++;
        }
    }
#endif

    //
    r=head;
    if(r==NULL && ip_base_sensor_hub_cnt <= 0)
    {
        printf("[%s][%s] node list is null\n",__FILE__, __func__);
        strcpy(pResult, "{\"IoTGW\":{\"ver\":1}}");
        return -1;
    }
    while(r!=NULL)
    {
#if 0
	printf("[%s][%s]\n----------------------------------\n", __FILE__, __func__);
        printf("nodeType:%d\n", r->nodeType);
	printf("virtualGatewayDevID:%s\n",r->virtualGatewayDevID);
        printf("osInfo:%d\n",r->virtualGatewayOSInfo);
	printf("connectivityType:%s\n",r->connectivityType);
	printf("connectivityDevID:%s\n",r->connectivityDevID);
	printf("connectivityInfoWithData:%s\n",r->connectivityInfoWithData);
        printf("sensorHubDevID:%s",r->sensorHubDevID);
	printf("\n----------------------------------\n");
#endif
        //WSN0:{Info...}
        //sprintf(tmp,"\"%s%d\":%s",r->connectivityType, i, r->connectivityInfo);
        index=FindConnectivityInfoNodeListIndex(r->connectivityType);
        if ( index >= 0 ){
            if (r->nodeType == TYPE_CONNECTIVITY){
                if (r->virtualGatewayOSInfo == OS_NONE_IP_BASE){
                    sprintf(tmp,"\"%s%d\":%s",r->connectivityType, g_ConnectivityInfoNodeList[index].index, r->connectivityInfoWithData);
                    strcat(g_ConnectivityInfoNodeList[index].Info,tmp);
                    strcat(g_ConnectivityInfoNodeList[index].Info,",");
                    strcpy(g_ConnectivityInfoNodeList[index].type,r->connectivityType);
                    g_ConnectivityInfoNodeList[index].index++;
                }
            }
        }
        else{
            printf("[%s][%s] can not find Info node list index\n",__FILE__, __func__);
        }
        //
	r=r->next;
    }
    printf("\n");

    //Pack all connectivity type capability 
    int max=sizeof(g_ConnectivityInfoNodeList)/sizeof(struct connectivityInfoNode);
    for(i=0; i < max ; i++){
        //index=FindConnectivityInfoNodeListIndex(cType[i]);
        if (strlen(g_ConnectivityInfoNodeList[i].Info) != 0 ){
            //sprintf(wsn_capability,"\"WSN\":{%s\"bn\":\"WSN\",\"ver\":1}",g_ConnectivityInfoNodeList[index].Info);
            sprintf(capability,"\"%s\":{",g_ConnectivityInfoNodeList[i].type);
            strcat(capability,g_ConnectivityInfoNodeList[i].Info);
            sprintf(tmp,"\"bn\":\"%s\",",g_ConnectivityInfoNodeList[i].type);
            strcat(capability,tmp);
            strcat(capability, "\"ver\":1}");
            strcpy(g_ConnectivityInfoNodeList[i].Info,capability);

        }
    }
    //Pack gateway capability
    strcpy(pResult,"{\"IoTGW\":{");
    for(i=0; i < max ; i++){
        if (strlen(g_ConnectivityInfoNodeList[i].Info) != 0 ){
            //sprintf(gateway_capability,"{\"IoTGW\":{%s,\"ver\":1}}", g_ConnectivityInfoNodeList[0].Info);
            strcat(pResult, g_ConnectivityInfoNodeList[i].Info);
            strcat(pResult, ",");
        }
    }
    strcat(pResult,"\"ver\":1}}");
    //
    return 0;
}


