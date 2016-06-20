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

/******************************************************/
char* cType[]={"WSN",
               "BLE"};
char g_json_path[1024]={0};
char g_connType[64]={0};

extern struct node* g_pVirtualGatewayDataListHead;


int DeleteDataListNodeByGatewayUID(char* devID)
{
    struct node *temp, *prev;
    temp=g_pVirtualGatewayDataListHead;
    while(temp!=NULL)
    {
    if( strcmp(temp->virtualGatewayDevID,devID) == 0 )
    {
        if(temp==g_pVirtualGatewayDataListHead)
        {
        g_pVirtualGatewayDataListHead=temp->next;
        free(temp->connectivityInfo);
        free(temp);
        return 1;
        }
        else
        {
        prev->next=temp->next;
        free(temp->connectivityInfo);
        free(temp);
        return 1;
        }
    }
    else
    {
        prev=temp;
        temp= temp->next;
    }
    }

    return 0;
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

void UpdateConnectivitySensorHubListNode(JSONode *json)
{
    int i=0,j=0, k=0, l=0;
    char json_path[256]={0};
    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    struct node *temp=NULL;

    for (j=0; j < sizeof(cType)/sizeof(char*); j++){
        i=0;
        while(1){
            k=0;
            //Check if connectivity exist
            sprintf(json_path,"[susiCommData][data][IoTGW][%s][%s%d][bn]",cType[j],cType[j],i);
            if ( GetJSONValue(json,json_path, nodeContent) < 0){
                break;
            }

            printf("\n--------------------------------------\n");
            printf("connectivity = %s\n", nodeContent);
            printf("--------------------------------------\n");

            //get connectivity node
	    temp=GetVirtualGatewayDataListNode(nodeContent);
	    if ( temp == NULL ){
                printf("can not find connectivity:%s node\n", nodeContent);
		break;
	    }

            //find the Info array size
            while(1){
		sprintf(json_path,"[susiCommData][data][IoTGW][%s][%s%d][Info][e][%d]",cType[j],cType[j],i,k);
                if ( GetJSONValue(json,json_path, nodeContent) < 0){
                    break;
                }
                k++;
            }
            //get SenHubList, Neighbor value
            for(l=0; l<k; l++){
		sprintf(json_path,"[susiCommData][data][IoTGW][%s][%s%d][Info][e][%d][n]",cType[j],cType[j],i,l);
                GetJSONValue(json,json_path, nodeContent);
                if( strcmp(nodeContent,"SenHubList") == 0 ){
		    sprintf(json_path,"[susiCommData][data][IoTGW][%s][%s%d][Info][e][%d][sv]",cType[j],cType[j],i,l);

                    if ( GetJSONValue(json,json_path, nodeContent) < 0){
                        printf("can not get SenHubList value\n");
                    }
                    else{
			strcpy(temp->connectivitySensorHubList, nodeContent);
			printf("\n--------------------------------------\n");
			printf("SenHubList = %s\n", temp->connectivitySensorHubList);
			printf("--------------------------------------\n");
                    }
                }
                if( strcmp(nodeContent,"Neighbor") == 0 ){
		    sprintf(json_path,"[susiCommData][data][IoTGW][%s][%s%d][Info][e][%d][sv]",cType[j],cType[j],i,l);

                    if ( GetJSONValue(json,json_path, nodeContent) < 0){
                        printf("can not get Neighbor value\n");
                    }
                    else{
			strcpy(temp->connectivityNeighborList, nodeContent);
			printf("\n--------------------------------------\n");
			printf("Neighbor = %s\n", temp->connectivityNeighborList);
			printf("--------------------------------------\n");
                    }
                }
            }
            i++;
        }
    }

}

void  DisplayAllVirtualGatewayDataListNode(struct node* head, struct node *r)
{
    r=head;
    if(r==NULL)
    {
    return;
    }
    while(r!=NULL)
    {
    printf("[%s][%s]\n----------------------------------\n", __FILE__, __func__);
    printf("virtualGatewayDevID:%s\n",r->virtualGatewayDevID);
    printf("connectivityType:%s\n",r->connectivityType);
    printf("connectivityDevID:%s\n",r->connectivityDevID);
    printf("connectivityInfo:%s\n",r->connectivityInfo);
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
    printf("initital g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //add1
    AddVirtualGatewayDataListNode("0000772233445599","WSN","0007112233445501","12345", strlen("12345"));
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("add1, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //add2
    AddVirtualGatewayDataListNode("0000772233445599","WSN","0007112233445502","67",strlen("67"));
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("add2, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //add3
    AddVirtualGatewayDataListNode("0000772233445599","WSN","0007112233445503","890",strlen("890"));
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("add3, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //add3 again: we will delete3 then add3 again
    AddVirtualGatewayDataListNode("0000772233445599","WSN","0007112233445503","77777",strlen("77777"));
    printf("-----------count = %d\n", cnt);
    DisplayAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead, n);

    //del3
    DeleteVirtualGatewayDataListNode("0007112233445503");
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("del3, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //del2
    DeleteVirtualGatewayDataListNode("0007112233445502");
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("del2, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    //del1
    DeleteVirtualGatewayDataListNode("0007112233445501");
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("del1, g_pVirtualGatewayDataListHead = %p\n",g_pVirtualGatewayDataListHead);
    printf("-----------count = %d\n", cnt);
    DisplayAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead, n);
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

int ReplySensorHubGetSetRequest(char* ptopic, JSONode *json, int cmdID){

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

int isRegisterSensorHubCapability(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    JSON_Get(json, OBJ_SENHUB_INFO_SPEC, nodeContent, sizeof(nodeContent));

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

                    if(isRegisterSensorHubCapability(json) == 0){
                        return REGISTER_SENSOR_HUB_CAPABILITY;
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

int ParseAgentinfoackTopic(JSONode *json){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_DEVICE_TYPE, nodeContent, sizeof(nodeContent));

    if(strcmp(nodeContent, "SenHub") == 0){
        return REGISTER_SENSOR_HUB;
    }

    return -1;
}

int ParseMQTTMessage(char* ptopic, JSONode *json){

    int res = -1;

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
    //agentinfoack topic
    if(strcmp(ptopic, AGENTINFOACK_TOPIC) == 0) {
        res = ParseAgentinfoackTopic(json);
        if ( res >= 0){
            return res;
        }
    }

    return res;
}


struct connectivityInfoNode{
    int index;
    char type[MAX_CONNECTIVITY_TYPE_LEN];
    char Info[1024];
};

struct connectivityInfoNode g_ConnectivityInfoNodeList[256]={0};

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


int BuildGatewayCapabilityInfo(struct node* head, char* pResult){
  
    printf("##################################################################\n");
    memset(&g_ConnectivityInfoNodeList,0,sizeof(g_ConnectivityInfoNodeList));
    printf("BuildGatewayCapabilityInfo-----------------\n");
    int i=0,index=-1;
    char tmp[1024]={0};
    char capability[1024]={0};
    struct node *r;

    r=head;
    if(r==NULL)
    {
        printf("[%s][%s] node list is null\n",__FILE__, __func__);
        strcpy(pResult, "{\"IoTGW\":{}}");
        return -1;
    }
    while(r!=NULL)
    {
	printf("[%s][%s]\n----------------------------------\n", __FILE__, __func__);
	printf("virtualGatewayDevID:%s\n",r->virtualGatewayDevID);
	printf("connectivityType:%s\n",r->connectivityType);
	printf("connectivityDevID:%s\n",r->connectivityDevID);
	printf("connectivityInfo:%s\n",r->connectivityInfo);
	printf("----------------------------------\n");
        //WSN0:{Info...}
        //sprintf(tmp,"\"%s%d\":%s",r->connectivityType, i, r->connectivityInfo);
        index=FindConnectivityInfoNodeListIndex(r->connectivityType);
        sprintf(tmp,"\"%s%d\":%s",r->connectivityType, g_ConnectivityInfoNodeList[index].index, r->connectivityInfo);
        strcat(g_ConnectivityInfoNodeList[index].Info,tmp);
        strcat(g_ConnectivityInfoNodeList[index].Info,",");
        strcpy(g_ConnectivityInfoNodeList[index].type,r->connectivityType);
        g_ConnectivityInfoNodeList[index].index++;
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
            printf("---------------%s capability----------------------------\n", g_ConnectivityInfoNodeList[i].type);
            printf(g_ConnectivityInfoNodeList[i].Info);
            printf("-------------------------------------------\n");
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

int GetVirtualGatewayUIDfromData(struct node* head, const char *data, char *uid , const int size ){
    
    //printf("[%s] data=%s\n", __func__, data);
    //strcpy(uid,"0000000E40ABCDEF");

#if 0 
//sample data to test  
    char mydata[1024]={"{\"susiCommData\":{\"sessionID\":\"350176BE533B485647559A09C0E9F686\",\"sensorIDList\":{\"e\":[{\"n\":\"IoTGW/WSN/0007000E40ABCDEF/Info/Health\"}]},\"commCmd\":523,\"requestID\":0,\"agentID\":\"\",\"handlerName\":\"IoTGW\",\"sendTS\":1465711233}}"}; 

    JSONode *json = JSON_Parser(mydata);
#endif
    JSONode *json = JSON_Parser(data);
    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    char connectivity_uid[64]={0};
    char topic[128]={0};


    JSON_Get(json, "[susiCommData][sensorIDList][e][0][n]", nodeContent, sizeof(nodeContent));
    printf("\n---------------------------------------------------\n");
    sprintf(topic,"/%s",nodeContent);
    GetUIDfromTopic(topic, connectivity_uid, sizeof(connectivity_uid));
    printf("uid = %s\n", connectivity_uid);
    printf("---------------------------------------------------\n");

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

int GetUIDType(struct node* head, const char *uid){

#if 0
    if ( strcmp(uid, "0000000E40ABCDEF") == 0 ){
        return TYPE_VIRTUAL_GATEWAY;
    }
#endif

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

int DisconnectSensorHubInVirtualGateway(struct node* head, char* VirtualGatewayUID){

    struct node *r;

    r=head;
    if(r==NULL)
    {
        return -1;
    }

    while(r!=NULL)
    {
        if (strcmp(r->virtualGatewayDevID, VirtualGatewayUID) == 0){
            char tmp[1024]={0};
            strcpy(tmp,r->connectivitySensorHubList);
	    char *SensorHubUID = strtok(tmp, ",");
	    while(SensorHubUID != NULL)
	    {
                DisconnectSensorHub(SensorHubUID);
		SensorHubUID = strtok(NULL, ",");
	    }
        }
        //
	r=r->next;
    }

    return 0;
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
                    printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    //test_link_list();
#if 1
                    UpdateVirtualGatewayDataListNode(message->payload);
                    char gateway_capability[2048]={0};
                    BuildGatewayCapabilityInfo(g_pVirtualGatewayDataListHead, gateway_capability);
		    printf("---------------Gateway capability----------------------------\n");
		    printf(gateway_capability);
		    printf("\n-------------------------------------------\n");
                    if ( RegisterGatewayCapability(gateway_capability, strlen(gateway_capability)) < 0){
                        printf("[%s][%s] Register Gateway Capability FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case REGISTER_SENSOR_HUB:
                {
                    printf("------------------------------------------------\n");
                    printf("[%s][%s]\033[33m #Register SensorHub# \033[0m\n", __FILE__, __func__);
#if 1
                    if ( RegisterSensorHub(json) < 0){
                        printf("[%s][%s] Register SensorHub FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case REGISTER_SENSOR_HUB_CAPABILITY:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Register SensorHub Capability# \033[0m\n", __FILE__, __func__);
#if 1
                    printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    if(RegisterSensorHubCapability(message->topic, json) < 0){
                        printf("[%s][%s] Register SensorHub Capability FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case REPLY_GET_SENSOR_REQUEST:
                {
                    printf("------------------------------------------------\n");
                    printf("[%s][%s]\033[33m #Reply Get Sensor Request# \033[0m\n", __FILE__, __func__);
                    char DeviceUID[64]={0};
		    if ( GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID)) < 0){
		        printf("[%s][%s] Can't find DeviceUID topic=%s\r\n",__FILE__, __func__, message->topic);
			return -1;
		    }
                    printf("DeviceUID = %s\n", DeviceUID);
#if 1
                    if ( GetUIDType(g_pVirtualGatewayDataListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){
                        printf("found virtual gateway device ID\n");
                        ReplyGatewayGetSetRequest(message->topic,json, IOTGW_GET_SENSOR_REPLY);
                        return 0;
                    }
#endif

#if 1
                    if ( ReplySensorHubGetSetRequest(message->topic,json, IOTGW_GET_SENSOR_REPLY) < 0){
                        printf("[%s][%s] Reply Get Sensor Request FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case REPLY_SET_SENSOR_REQUEST:
                {
                    printf("------------------------------------------------\n");
                    printf("[%s][%s]\033[33m #Reply Set Sensor Request# \033[0m\n", __FILE__, __func__);

                    char DeviceUID[64]={0};
		    if ( GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID)) < 0){
		        printf("[%s][%s] Can't find DeviceUID topic=%s\r\n",__FILE__, __func__, message->topic);
			return -1;
		    }
                    printf("DeviceUID = %s\n", DeviceUID);
#if 1
                    if ( GetUIDType(g_pVirtualGatewayDataListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){
                        printf("found 0000000E40ABCDEF connectivity mac\n");
                        ReplyGatewayGetSetRequest(message->topic,json, IOTGW_SET_SENSOR_REPLY);
                        return 0;
                    }
#endif

#if 1
                    if ( ReplySensorHubGetSetRequest(message->topic,json, IOTGW_SET_SENSOR_REPLY) < 0){
                        printf("[%s][%s] Reply Set Sensor Request FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case UPDATE_GATEWAY_DATA:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Update Gateway Data# \033[0m\n", __FILE__, __func__);
                    printf("[%s][%s] message=%s\n",__FILE__, __func__, message->payload);
                    //UpdateConnectivitySensorHubListNode(json);
                    aTest(message->payload);
#if 1
                    if ( UpdateGatewayData(json) < 0){
                        printf("[%s][%s] Update Gateway Data FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("------------------------------------------------\n");
                    break;
                }
            case UPDATE_SENSOR_HUB_DATA:
                {
                    printf("------------------------------------------------\n");
		    printf("[%s][%s]\033[33m #Update SensorHub Data# \033[0m\n", __FILE__, __func__);
#if 1
                    if ( UpdateSensorHubData(json) < 0){
                        printf("[%s][%s] Update SensorHub Data FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
#endif
                    printf("\n------------------------------------------------\n");
                }
            default:
                break;
        }

        if(strcmp(topicType, WA_PUB_WILL_TOPIC) == 0) {
		// CmdID=2003
		printf("[%s][%s] Receive messages from will topic!!\n", __FILE__, __func__);
                printf("topic = %s\n", message->topic);
                char DeviceUID[64]={0};
                GetUIDfromTopic(message->topic, DeviceUID, sizeof(DeviceUID));
                printf("DeviceUID = %s\n", DeviceUID);

                //
                if ( GetUIDType(g_pVirtualGatewayDataListHead, DeviceUID) == TYPE_VIRTUAL_GATEWAY ){
                    printf("found virtual gateway device ID\n");
                    DisconnectSensorHubInVirtualGateway(g_pVirtualGatewayDataListHead, DeviceUID);
                    DeleteDataListNodeByGatewayUID(DeviceUID);
                    //
                    char gateway_capability[2048]={0};
                    BuildGatewayCapabilityInfo(g_pVirtualGatewayDataListHead, gateway_capability);
		    printf("---------------Gateway capability----------------------------\n");
		    printf(gateway_capability);
		    printf("\n-------------------------------------------\n");
                    if ( RegisterGatewayCapability(gateway_capability, strlen(gateway_capability)) < 0){
                        printf("[%s][%s] Register Gateway Capability FAIL !!!\n", __FILE__, __func__);
                        JSON_Destory(&json);
                        return -1;
                    }
                }
                else{
                    DisconnectSensorHub(DeviceUID);
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
	#ifndef WIN32
	srandom(time(NULL));
	#endif
	memset(topic, 0, sizeof(topic));;
	memset(message, 0, sizeof(message));
        strcpy(message,pData);

	sprintf(topic, "/cagent/admin/%s/%s", macAddr, WA_SUB_CBK_TOPIC);
	msglen = strlen(message);
	rc = mosquitto_publish(g_mosq, &g_mid_sent, topic, msglen, message, g_mosq_cfg.qos, g_mosq_cfg.retain);

	return 0;
}


