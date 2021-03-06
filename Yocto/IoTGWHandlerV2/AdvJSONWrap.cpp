#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "AdvJSON.h"
#include "MqttHal.h"
#include "IoTGWHandler.h"
#include "AdvLog.h"

#ifdef __cplusplus
extern "C" {
#endif
extern char            g_GWInfMAC[MAX_MACADDRESS_LEN];
struct node* g_pNodeListHead=NULL;

int AddNodeList(char* pVirtualGatewayDevID, char* pConnectivityType, char* pConnectivityDevID, char* pConnectivityInfo, int iConnectivityInfoSize, int devType, int iOSInfo, char* pSensorHubDevID);
int UpdateNodeList(char* devID, int devType, struct node* pNode);
struct node * GetNode(char* devID, int devType);
int DeleteNodeList(char* devID, int devType);
int AddNodeList_ConnectivityNodeCapability(char* data);
int UpdateNodeList_ConnectivityNodeInfo(char* data);
void AddNodeList_SensorHubNodeInfo(const char* data);
int AddNodeList_VirtualGatewayNodeInfo(char* data, int iOSInfo);
int BuildData_IPBaseConnectivityCapability(char* info_data);
int BuildNodeList_GatewayUpdateInfo(char* data, char* info_data, int osInfo);
int GetOSInfoType(char* data);
int GetAgentID(char* data, char* out_agent_id, int out_agent_id_size);
int GetConnectivityResponseData(char* data, char* response_data);
int GetRequestCmd(int cmdID, char* out_CmdData);
int BuildNodeList_IPBaseGatewayInfo(char* info_data);
void aTest(const char* mydata);
void printNodeInfo();

#ifdef __cplusplus
}
#endif

int GetAgentID(char* data, char* out_agent_id, int out_agent_id_size){
    
    if ( out_agent_id_size < 5 ){
        return -1;
    }

    AdvJSON json(data);
    //Get virtual gateway devID
    strncpy(out_agent_id,json["susiCommData"]["agentID"].Value().c_str(), out_agent_id_size-1);

    if ( strcmp(out_agent_id,"NULL") == 0){
        return -1;
    }

    return 0;
}

void printNodeInfo(struct node * r){

    ADV_DEBUG("-------------------node info------------------------------\n");
    ADV_DEBUG("[%s][%s] node type=%d, os info=%d\n", __FILE__, __func__, r->nodeType, r->virtualGatewayOSInfo);
    ADV_DEBUG("[%s][%s] virtualGatewayDevID=%s\n", __FILE__, __func__, r->virtualGatewayDevID);
    ADV_DEBUG("[%s][%s] connectivityDevID=%s\n", __FILE__, __func__, r->connectivityDevID);
    ADV_DEBUG("[%s][%s] sensorHubDevID=%s\n", __FILE__, __func__, r->sensorHubDevID);
    ADV_DEBUG("-------------------------------------------------\n");
}

struct node * GetNode(char* devID, int devType)
{
    struct node * r;
    r=g_pNodeListHead;

    if(r==NULL)
    {
    return NULL;
    }
    while(r!=NULL)
    {
        switch(devType){
            case TYPE_GATEWAY:
            {

                if (strcmp(r->virtualGatewayDevID, devID) == 0){
                    if ( r->nodeType == TYPE_GATEWAY) {
                        return r;
                    }
                }

                break;
            }
            case TYPE_CONNECTIVITY:
            {
                if (strcmp(r->connectivityDevID, devID) == 0){
                    if ( r->nodeType == TYPE_CONNECTIVITY) {
                        return r;
                    }
                }
                break;
            }
            case TYPE_SENSOR_HUB:
            {
                if (strcmp(r->sensorHubDevID, devID) == 0){
                    if ( r->nodeType == TYPE_SENSOR_HUB) {
                        return r;
                    }
                }
                break;
            }
            default:
                ADV_DEBUG("[%s][%s]Unknown device type\n", __FILE__, __func__);
                break;
        }
        r=r->next;
    }

    return NULL;
    //printf("\n");
}

int DeleteNodeList(char* devID, int devType)
{
    struct node *temp, *prev;
    temp=g_pNodeListHead;
    char tmp_devID[32]={0};

    while(temp!=NULL)
    {
        switch(devType){
            case TYPE_GATEWAY:
            {
                strcpy(tmp_devID, temp->virtualGatewayDevID);
                break;
            }
            case TYPE_CONNECTIVITY:
            {
                strcpy(tmp_devID, temp->connectivityDevID);
                break;
            }
            case TYPE_SENSOR_HUB:
            {
                strcpy(tmp_devID, temp->sensorHubDevID);
                break;
            }
            default:
                ADV_DEBUG("[%s][%s]Unknown device type\n", __FILE__, __func__);
                break;
        }
        //
        if( strcmp(tmp_devID,devID) == 0 && temp->nodeType == devType)
        {
            if(temp==g_pNodeListHead)
            {
                g_pNodeListHead=temp->next;
                if (temp->connectivityInfo){
                    free(temp->connectivityInfo);
                }
                //
                if (temp->connectivityInfoWithData){
                    free(temp->connectivityInfoWithData);
                }
                free(temp);
                return 1;
            }
            else
            {
                prev->next=temp->next;
                if (temp->connectivityInfo){
                    free(temp->connectivityInfo);
                }
                //
                if (temp->connectivityInfoWithData){
                    free(temp->connectivityInfoWithData);
                }
                free(temp);
                return 1;
            }
        }
        else{
            prev=temp;
            temp= temp->next;
        }
    }

    return 0;
}
int AddNodeList_VirtualGatewayNodeInfo(char* data, int iOSInfo){

    struct node *temp=NULL;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    
    AdvJSON json(data);

    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());

    return AddNodeList(virtualGatewayDevID,NULL,NULL,NULL, 0, TYPE_GATEWAY, iOSInfo, NULL);

}

int AddNodeList(char* pVirtualGatewayDevID, char* pConnectivityType, char* pConnectivityDevID, char* pConnectivityInfo, int iConnectivityInfoSize, int devType, int iOSInfo, char* pSensorHubDevID)
{
    struct node *temp=NULL;
    char tmp_devID[32]={0};
    
    switch(devType)
    {
        case TYPE_GATEWAY:
        {
            if (pVirtualGatewayDevID != NULL){
                strcpy(tmp_devID,pVirtualGatewayDevID);
            }
            break;
        }
        case TYPE_CONNECTIVITY:
        {
            if (pConnectivityDevID != NULL){
                strcpy(tmp_devID,pConnectivityDevID);
            }
            break;
        }
        case TYPE_SENSOR_HUB:
        {
            if  ( pSensorHubDevID != NULL){
                strcpy(tmp_devID,pSensorHubDevID);
            }
            break;
        }
        default:
            ADV_DEBUG("[%s][%s]Unknown device type\n", __FILE__, __func__);
            break;
    }
    //
    temp=GetNode(tmp_devID, devType);
    //
    if (temp != NULL){
        ADV_C_DEBUG(COLOR_PURPLE,"[%s][%s] Node exist, delete node in list(devID:%s,devType=%d.\n", __FILE__, __func__, tmp_devID, devType);
        DeleteNodeList(tmp_devID, devType);
        temp=NULL;
    }
    //
    temp=(struct node *)malloc(sizeof(struct node));
    if ( temp == NULL){
        ADV_C_ERROR(COLOR_RED,"[%s][%s] memory allocate FAIL.\n", __FILE__, __func__);
        return -1;
    }
    
    memset(temp,0,sizeof(struct node));
 
    temp->nodeType = devType;
    temp->virtualGatewayOSInfo = iOSInfo;

    if (pConnectivityInfo != NULL){
        temp->connectivityInfo=(char*)malloc(iConnectivityInfoSize+1);
        strcpy(temp->connectivityInfo,pConnectivityInfo);
    }

    if (pConnectivityDevID != NULL){
        strcpy(temp->connectivityDevID,pConnectivityDevID);
    }
   
    if ( pVirtualGatewayDevID != NULL){
        strcpy(temp->virtualGatewayDevID, pVirtualGatewayDevID);
    }

    if  ( pSensorHubDevID != NULL){
        strcpy(temp->sensorHubDevID, pSensorHubDevID);
    }
    
    if ( pConnectivityType != NULL){
        strcpy(temp->connectivityType,pConnectivityType);
    }

    //temp->data=num;
    if (g_pNodeListHead == NULL)
    {
        g_pNodeListHead=temp;
        g_pNodeListHead->next=NULL;
    }
    else
    {
        temp->next=g_pNodeListHead;
        g_pNodeListHead=temp;
    }

    return 0;
}

int UpdateNodeList(char* devID, int devType, struct node* pNode){

#if 0
    printf("@@@@@@@@@@@@ UpdateNodeList @@@@@@@@@@@@@\n");
#endif
    struct node * r;
    r=g_pNodeListHead;

    if(r==NULL)
    {
        return -1;
    }
    while(r!=NULL)
    {
        switch(devType){
            case TYPE_GATEWAY:
            {

                if (strcmp(r->virtualGatewayDevID, devID) == 0){
                    if ( r->nodeType == TYPE_GATEWAY) {
                        if ( pNode->state != 0 ){ 
                            r->state = pNode->state;
                            ADV_DEBUG("[%s][%s] update state = %d\n", __FILE__, __func__, r->state);
                        }
                        if ( pNode->virtualGatewayOSInfo != 0 ){ 
                            r->virtualGatewayOSInfo = pNode->virtualGatewayOSInfo;
                            ADV_DEBUG("[%s][%s] update virtualGatewayOSInfo\n", __FILE__, __func__);
                        }
                        if ( pNode->connectivityInfo != 0 ){ 
                            ADV_DEBUG("[%s][%s] update connectivityInfo\n", __FILE__, __func__);
                        }
                        ADV_DEBUG("[%s][%s] last_hb_time=%ld\n", __FILE__, __func__, pNode->last_hb_time);
                        if ( pNode->last_hb_time != 0 ){ 
                            r->last_hb_time = pNode->last_hb_time;
                            ADV_DEBUG("[%s][%s] update last_hb_time\n", __FILE__, __func__);
                            ADV_DEBUG("[%s][%s] r->last_hb_time=%ld\n", __FILE__, __func__, r->last_hb_time);
                        }
                        return 0;
                    }
                }

                break;
            }
            case TYPE_CONNECTIVITY:
            {
                if (strcmp(r->connectivityDevID, devID) == 0){
                    if ( r->nodeType == TYPE_CONNECTIVITY) {
                        if ( pNode->connectivityInfoWithData != 0 ){
                            ADV_DEBUG("pNode->connectivityInfoWithData=%s\n", pNode->connectivityInfoWithData);
                            if ( r->connectivityInfoWithData != 0){
                                free(r->connectivityInfoWithData);
                                r->connectivityInfoWithData = 0;
                            }

                            int len=strlen(pNode->connectivityInfoWithData);
                            r->connectivityInfoWithData = (char*) malloc(len+1);

                            if ( r->connectivityInfoWithData ){
                                strcpy(r->connectivityInfoWithData, pNode->connectivityInfoWithData);
                                ADV_DEBUG("r->connectivityInfoWithData=%s\n", r->connectivityInfoWithData);
                            }
                            else{
                                return -1;
                            }

                        }
                        return 0;
                    }
                }
                break;
            }
            case TYPE_SENSOR_HUB:
            {
                if (strcmp(r->sensorHubDevID, devID) == 0){
                    if ( r->nodeType == TYPE_SENSOR_HUB) {
                        return 0;
                    }
                }
                break;
            }
            default:
                ADV_DEBUG("[%s][%s]Unknown device type\n", __FILE__, __func__);
                break;
        }
        r=r->next;
    }

    return -1;
}

int GetSensorHubList(char* sensorHubList, int osInfo, char* connectivityDevID){

    struct node * r;
    r=g_pNodeListHead;
    int cnt=0;

    if(r==NULL)
    {
        return -1;
    }

    while(r!=NULL)
    {
        switch (osInfo)
        {
            case OS_IP_BASE:
            {
                if( r->virtualGatewayOSInfo == OS_IP_BASE &&
                    r->nodeType == TYPE_SENSOR_HUB ){

                    if ( strlen(r->sensorHubDevID) ==0 || strcmp(r->sensorHubDevID,"NULL") == 0){
                        printNodeInfo(r);
                    }
                    else{
                        cnt++;
		        if (strlen(sensorHubList) == 0){
				strcat(sensorHubList,r->sensorHubDevID);
			}
			else{
				strcat(sensorHubList,",");
				strcat(sensorHubList,r->sensorHubDevID);
			}
                    }
                }
                break;
            }
            case OS_NONE_IP_BASE:
            {
                break;
            }
            default:
                ADV_DEBUG("[%s][%s] Unknown os info\n",__FILE__, __func__);
                break;
        } 
        r=r->next;
    }

    return cnt;
}

int BuildNodeList_IPBaseGatewayUpdateInfo(char* info_data){

    char tmp[1024]={0};

    sprintf(tmp,"{\"IoTGW\":{\"%s\":",IP_BASE_CONNECTIVITY_NAME);
    strcat(info_data,tmp);
   
    memset(tmp,0,sizeof(tmp));
    sprintf(tmp,"{\"%s\":",IP_BASE_CONNECTIVITY_NAME);
    strcat(info_data,tmp);
    //
    memset(tmp,0,sizeof(tmp));
    if ( BuildNodeList_IPBaseGatewayInfo(tmp) < 0){
        return -1;
    }

    strcat(info_data,tmp);
    //
    strcat(info_data,",");

    memset(tmp,0,sizeof(tmp));
    sprintf(tmp,"\"bn\":\"%s\"},\"ver\":1}",IP_BASE_CONNECTIVITY_NAME);
    strcat(info_data,tmp);
    strcat(info_data,"}");

    return 0;

}

int BuildData_NoneIPBaseGatewayUpdateInfo(char* data, char* info_data){

    char nodeContent[MAX_JSON_NODE_SIZE]={0};

    AdvJSON json(data);

    strcpy(info_data,json["susiCommData"]["data"].Value().c_str());
    if(strcmp(info_data, "NULL") == 0){
        return -1;
    }

    return 0;
}


int BuildNodeList_GatewayUpdateInfo(char* data, char* info_data, int osInfo){

    switch(osInfo){
        case OS_IP_BASE:
        {
            return BuildNodeList_IPBaseGatewayUpdateInfo(info_data);
        }
        case OS_NONE_IP_BASE:
        {
            return BuildData_NoneIPBaseGatewayUpdateInfo(data, info_data);
        }
        default:
        {
            ADV_DEBUG("[%s][%s] unknown os info type:%d\n", __FILE__, __func__, osInfo);
            return -1;
        }
    }

    return 0;
}

int BuildData_IPBaseConnectivityCapability(char* info_data){

    char* e_array[]={"{\"n\":\"SenHubList\",\"sv\":\"%s\",\"asm\":\"%s\"}",
                     "{\"n\":\"Neighbor\",\"sv\":\"%s\",\"asm\":\"%s\"}"
#if 0
                     "{\"n\":\"Name\",\"sv\":\"Ethernet\",\"asm\":\"r\"}"//,
                     "{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"}",
                     "{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"}",
                     "{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}"
#endif
                   };
    char tmp[1024]={0};
    int i=0;
    int max_e_array_size=sizeof(e_array)/sizeof(char*);

    strcat(info_data,"{\"Info\":");
    //
    strcat(info_data,"{");
    //
    strcat(info_data,"\"e\":[");
#if 1
    for(i=0; i < max_e_array_size ; i++){
        memset(tmp,0,sizeof(tmp));
        AdvJSON json(e_array[i]);
        if ( strcmp("SenHubList", json[0].Value().c_str()) == 0){
            //sprintf(tmp,e_array[i],"\"\"","r");
            strcpy(tmp,"{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"}");
        }
        else if ( strcmp("Neighbor", json[0].Value().c_str()) == 0){
            //sprintf(tmp,e_array[i],"3333","r");
            strcpy(tmp,"{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"}");
        }
        else{
            strcpy(tmp,e_array[i]);
        }
        strcat(info_data,tmp);
        strcat(info_data,",");
    }
    int len=strlen(info_data);
    info_data[len-1]=0;
#endif
    strcat(info_data,"]");
    //
    strcat(info_data,",");
    strcat(info_data,"\"bn\":\"Info\"");
    //
    strcat(info_data,"}");
    //
    strcat(info_data,",");
    sprintf(tmp,"\"bn\":\"%s%s\"",CONNECTIVITY_ID_PREFIX,g_GWInfMAC);
    strcat(info_data,tmp);
    strcat(info_data,",");
    strcat(info_data,"\"ver\":1");
    strcat(info_data,"}");

    return 0;
}

int GetOSInfoType(char* data){

    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    int osInfo=OS_TYPE_UNKNOWN;

    AdvJSON json(data);
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());
    

    struct node* temp= GetNode(virtualGatewayDevID,TYPE_GATEWAY);
    if ( temp != NULL ){
        osInfo=temp->virtualGatewayOSInfo;
    }
      
    ADV_DEBUG("[%s][%s] virtualGatewayDevID=%s, os info type:%d\n", __FILE__, __func__, virtualGatewayDevID, osInfo);
    return osInfo;
}

int AddNodeList_ConnectivityNodeCapability(char* data){

#if 0
//sample message:
/*
//Only WSN
{"susiCommData":{"infoSpec":{"IoTGW":{"WSN":{"WSN0":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD01","ver":1},"WSN1":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD02","ver":1},"bn":"WSN","ver":1},"ver":1}},"commCmd":2052,"requestID":2001,"agentID":"0000000E40ABCDEF","handlerName":"general","sendTS":160081020}}

//WSN and BLE
{"susiCommData":{"infoSpec":{"IoTGW":{"WSN":{"WSN0":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD01","ver":1},"WSN1":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD02","ver":1},"bn":"WSN","ver":1},"BLE":{"BLE0":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD05","ver":1},"BLE1":{"Info":{"e":[{"n":"SenHubList","sv":"","asm":"r"},{"n":"Neighbor","sv":"","asm":"r"},{"n":"Name","sv":"WSN0","asm":"r"},{"n":"Health","v":"100.000000","asm":"r"},{"n":"sw","sv":"1.2.1.12","asm":"r"},{"n":"reset","bv":"0","asm":"rw"}],"bn":"Info"},"bn":"0007000E40ABCD06","ver":1},"bn":"BLE","ver":1},"ver":1}},"commCmd":2052,"requestID":2001,"agentID":"0000000E40ABCDEF","handlerName":"general","sendTS":160081020}}

*/
/*
    char mydata[1024]={"{\"susiCommData\":{\"infoSpec\":{\"IoTGW\":{\"WSN\":{\"WSN0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD01\",\"ver\":1},\"WSN1\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD02\",\"ver\":1},\"bn\":\"WSN\",\"ver\":1},\"ver\":1}},\"commCmd\":2052,\"requestID\":2001,\"agentID\":\"0000000E40ABCDEF\",\"handlerName\":\"general\",\"sendTS\":160081020}"};
*/
    char mydata[2024]={"{\"susiCommData\":{\"infoSpec\":{\"IoTGW\":{\"WSN\":{\"WSN0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD01\",\"ver\":1},\"WSN1\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD02\",\"ver\":1},\"bn\":\"WSN\",\"ver\":1},\"BLE\":{\"BLE0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD05\",\"ver\":1},\"BLE1\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD06\",\"ver\":1},\"bn\":\"BLE\",\"ver\":1},\"ver\":1}},\"commCmd\":2052,\"requestID\":2001,\"agentID\":\"0000000E40ABCDEF\",\"handlerName\":\"general\",\"sendTS\":160081020}}"};
    
#endif

    AdvJSON json(data);
    int i=0, j=0;
    int max=0;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};

    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());
    //printf(virtualGatewayDevID);

    //char a[128]={"susiCommData"};
    max=json["susiCommData"]["infoSpec"]["IoTGW"].Size();
    for(i=0; i < max ;i++){
#if 0
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%d]=%s\n",i,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
#endif
        char type[128]={0};
        char device[128]={0};

        strcpy(type,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
#if 0
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s]=%s\n",\
        type,
        json["susiCommData"]["infoSpec"]["IoTGW"][type].Value().c_str());
#endif        
        int max_device=json["susiCommData"]["infoSpec"]["IoTGW"][type].Size();
        for(j=0; j<max_device ; j++){
            strcpy(device,json["susiCommData"]["infoSpec"]["IoTGW"][type][j].Key().c_str());
            //int tmp_value[1024]={0};
            if ( strcmp("NULL",json["susiCommData"]["infoSpec"]["IoTGW"][type][device]["Info"].Value().c_str()) != 0){
#if 0
                printf("@@@@ json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s][%s]=%s\n",\
                type,device, \
                json["susiCommData"]["infoSpec"]["IoTGW"][type][device].Value().c_str());
#endif
                //Get connectivity Info
                strcpy(connectivityInfo,json["susiCommData"]["infoSpec"]["IoTGW"][type][device].Value().c_str());
                //Get connectivity device ID
                strcpy(connectivityDevID,json["susiCommData"]["infoSpec"]["IoTGW"][type][device]["bn"].Value().c_str());

                int osInfo=OS_TYPE_UNKNOWN;
                struct node* temp= GetNode(virtualGatewayDevID,TYPE_GATEWAY);
                if ( temp != NULL ){
                    osInfo = temp->virtualGatewayOSInfo;
                }
                //Add Node
#if 1
                if ( osInfo == OS_IP_BASE){
                    //sprintf(connectivityDevID,"0007%s",g_GWInfMAC);
                    char info_data[1024]={0};
                    BuildData_IPBaseConnectivityCapability(info_data);
                    strcpy(connectivityInfo,info_data); 
                }
#endif
                AddNodeList(virtualGatewayDevID,type,connectivityDevID,connectivityInfo, strlen(connectivityInfo), TYPE_CONNECTIVITY, osInfo, NULL);
            }
        }

    }
    
    return 0;
}

void AddNodeList_SensorHubNodeInfo(const char* data){

    AdvJSON json(data);
    int i=0, j=0, k=0;
    int max=0;
    int osInfo=OS_TYPE_UNKNOWN;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};
    char virtualConnectivityDevID[MAX_DEVICE_ID_LEN]={0};
    char packet_type[32]={"data"};
    struct node *temp=NULL;

    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());
    //check OS type
    temp=GetNode(virtualGatewayDevID, TYPE_GATEWAY);
    if (temp){
        osInfo = temp->virtualGatewayOSInfo;
    }

#if 0
    if ( temp->virtualGatewayOSInfo == OS_IP_BASE){
        printf("[%s][%s] IP base device, DO NOT update gateway Info\n", __FILE__, __func__);
        return;
    }
#endif
    //

    AdvJSON IoTGW_json=json["susiCommData"][packet_type]["IoTGW"];
    max=IoTGW_json.Size();

    for(i=0; i < max ;i++){
        char type[128]={0};
        char device[128]={0};

        strcpy(type,IoTGW_json[i].Key().c_str());
 
        int max_device=IoTGW_json[type].Size();

        for(j=0; j<max_device ; j++){
            strcpy(device,IoTGW_json[type][j].Key().c_str());

            if ( strcmp("NULL",IoTGW_json[type][device]["Info"].Value().c_str()) != 0){

                //Get connectivity Info
                strcpy(connectivityInfo,IoTGW_json[type][device].Value().c_str());
                //Get connectivity device ID
                strcpy(connectivityDevID,IoTGW_json[type][device]["bn"].Value().c_str());
		
#if 0
                //get connectivity node
                if( OS_NONE_IP_BASE == osInfo){
		    temp=GetNode(connectivityDevID, TYPE_CONNECTIVITY);
		    if ( temp == NULL ){
			printf("[%s][%s]can not find connectivity:%s node\n", __FILE__, __func__, connectivityDevID);
			continue;
		    }
                }
#endif

                AdvJSON IoTGW_device_info=IoTGW_json[type][device]["Info"]["e"];
                int max_info_size=IoTGW_device_info.Size();

                for(k=0; k < max_info_size; k++){
                    int l=0;
                    int max_dev_info_size=IoTGW_device_info[k].Size();

                    for(l=0; l<max_dev_info_size ; l++){
                        
                        //Get SenHubList value
                        if ( strcmp("n",IoTGW_device_info[k][l].Key().c_str()) == 0 &&
                             strcmp("SenHubList",IoTGW_device_info[k][l].Value().c_str()) == 0 && 
                             strcmp("sv",IoTGW_device_info[k][l+1].Key().c_str()) == 0){

                             ADV_DEBUG("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             ADV_DEBUG("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());

                             char sensorHubDevID[MAX_DEVICE_ID_LEN]={0};
                             if( OS_TYPE_UNKNOWN != osInfo){
                                 //strcpy(temp->connectivitySensorHubList, IoTGW_device_info[k][l+1].Value().c_str());
#if 1
				 char tmp_data[1024]={0};
                                 char* pSave = NULL;

                                 if (OS_NONE_IP_BASE == osInfo){
				     strcpy(tmp_data,IoTGW_device_info[k][l+1].Value().c_str());
                                 }
                                 else if (OS_IP_BASE == osInfo){
                                     strcpy(tmp_data,IoTGW_device_info[k][l+1].Value().c_str());
                                 }

				 char *SensorHubUID = strtok_r(tmp_data, ",", &pSave);

				 while(SensorHubUID != NULL)
				 {
                                        //printf("=============== SensorHubUID:%s\n",SensorHubUID);
                                        strcpy(sensorHubDevID,SensorHubUID);
                                        if ( strcmp(sensorHubDevID,"NULL") != 0 && 
                                             strcmp(sensorHubDevID, connectivityDevID) !=0 &&
                                             strcmp(sensorHubDevID, virtualGatewayDevID) != 0 ){
					    AddNodeList(virtualGatewayDevID,NULL,connectivityDevID,NULL, 0, TYPE_SENSOR_HUB, osInfo, sensorHubDevID);
                                        }
					SensorHubUID = strtok_r(NULL, ",",&pSave);
				 } 
#endif

                             }
                             else{
                                 ADV_DEBUG("[%s][%s]Unknown OS Info: %d\n", __FILE__, __func__, osInfo);
                             }
                        }
#if 0
                        //Get Neighbor value
                        if ( strcmp("n",IoTGW_device_info[k][l].Key().c_str()) == 0 &&
                             strcmp("Neighbor",IoTGW_device_info[k][l].Value().c_str()) == 0 && 
                             strcmp("sv",IoTGW_device_info[k][l+1].Key().c_str()) == 0){

                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());

                             if( OS_NONE_IP_BASE == osInfo){
                                 strcpy(temp->connectivityNeighborList, IoTGW_device_info[k][l+1].Value().c_str());
                             }
                             else if ( OS_IP_BASE == osInfo){
                                 //ToDo
                             }
                             else{
                                 printf("[%s][%s]Unknown OS Info\n", __FILE__, __func__);
                             }                             
                        }
#endif
                    }
                }

            }
        }

    }
}

void aTest(const char* mydata){

    AdvJSON json(mydata);
    int i=0, j=0, k=0;
    int max=0;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};
    char packet_type[32]={"data"};
    struct node *temp=NULL;


    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());

    AdvJSON IoTGW_json=json["susiCommData"][packet_type]["IoTGW"];
    max=IoTGW_json.Size();

    for(i=0; i < max ;i++){
        char type[128]={0};
        char device[128]={0};

        strcpy(type,IoTGW_json[i].Key().c_str());
 
        int max_device=IoTGW_json[type].Size();

        for(j=0; j<max_device ; j++){
            strcpy(device,IoTGW_json[type][j].Key().c_str());

            if ( strcmp("NULL",IoTGW_json[type][device]["Info"].Value().c_str()) != 0){

                //Get connectivity Info
                strcpy(connectivityInfo,IoTGW_json[type][device].Value().c_str());
                //Get connectivity device ID
                strcpy(connectivityDevID,IoTGW_json[type][device]["bn"].Value().c_str());
		//get connectivity node
#if 1
		temp=GetNode(connectivityDevID, TYPE_CONNECTIVITY);
		if ( temp == NULL ){
			printf("[%s][%s]can not find connectivity:%s node\n", __FILE__, __func__, connectivityDevID);
			continue;
		}
#endif

                AdvJSON IoTGW_device_info=IoTGW_json[type][device]["Info"]["e"];
                int max_info_size=IoTGW_device_info.Size();

                for(k=0; k < max_info_size; k++){
                    int l=0;
                    int max_dev_info_size=IoTGW_device_info[k].Size();

                    for(l=0; l<max_dev_info_size ; l++){
                        
                        //Get SenHubList value
                        if ( strcmp("n",IoTGW_device_info[k][l].Key().c_str()) == 0 &&
                             strcmp("SenHubList",IoTGW_device_info[k][l].Value().c_str()) == 0 && 
                             strcmp("sv",IoTGW_device_info[k][l+1].Key().c_str()) == 0){

                             ADV_DEBUG("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             ADV_DEBUG("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());
                             strcpy(temp->connectivitySensorHubList, IoTGW_device_info[k][l+1].Value().c_str());
                        }
#if 0
                        //Get Neighbor value
                        if ( strcmp("n",IoTGW_device_info[k][l].Key().c_str()) == 0 &&
                             strcmp("Neighbor",IoTGW_device_info[k][l].Value().c_str()) == 0 && 
                             strcmp("sv",IoTGW_device_info[k][l+1].Key().c_str()) == 0){

                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());
                             strcpy(temp->connectivityNeighborList, IoTGW_device_info[k][l+1].Value().c_str());
                        }
#endif
                    }
                }

            }
        }

    }
}

int GetConnectivityResponseData(char* data, char* response_data){

    AdvJSON json(data);

    char sessionID[512]={0};
    char sensorIDList[512]={0};
    char infoName[512]={0};
    char sensorHubList[1024]={0};


    //Get session ID
    strcpy(sessionID,json["susiCommData"]["sessionID"].Value().c_str());
    ADV_DEBUG("session ID = %s\n", sessionID);
    //Get sensorIDList
    strcpy(sensorIDList,json["susiCommData"]["sensorIDList"]["e"][0]["n"].Value().c_str());
    ADV_DEBUG("sensorIDList=%s\n", sensorIDList);

    sscanf(sensorIDList, "%*[^/]/%*[^/]/%*[^/]/%s", infoName);
    ADV_DEBUG("infoName=%s\n", infoName);
    if ( strcmp(infoName, "Info/SenHubList") == 0 ){

        if ( GetSensorHubList(sensorHubList, OS_IP_BASE, NULL) < 0 ){
            ADV_C_ERROR(COLOR_RED, "[%s][%s] get sensor hub list fail\n", __FILE__, __func__);
            return -1;
        }
        printf("sensorHubList=%s\n", sensorHubList);
        sprintf(response_data,"{\"sessionID\":\"%s\",\"sensorInfoList\":{\"e\":[{\"n\":\"/Info/SenHubList\",\"sv\":\"%s\",\"StatusCode\":200}]}}",sessionID,sensorHubList);
    }
    else if ( strcmp(infoName, "Info/Neighbor") == 0 ){

        if ( GetSensorHubList(sensorHubList, OS_IP_BASE, NULL) < 0 ){
            ADV_C_ERROR(COLOR_RED,"[%s][%s] get sensor hub list fail\n", __FILE__, __func__);
            return -1;
        }
        printf("sensorHubList=%s\n", sensorHubList);
        sprintf(response_data,"{\"sessionID\":\"%s\",\"sensorInfoList\":{\"e\":[{\"n\":\"/Info/Neighbor\",\"sv\":\"%s\",\"StatusCode\":200}]}}",sessionID,sensorHubList);
    }
    else if ( strcmp(infoName, "Info/Name") == 0 ){
        sprintf(response_data,"{\"sessionID\":\"%s\",\"sensorInfoList\":{\"e\":[{\"n\":\"/Info/Name\",\"sv\":\"%s\",\"StatusCode\":200}]}}",sessionID,"Ethernet");
    }

    return 0;
}

int GetRequestCmd(int cmdID, char* out_CmdData){

    switch(cmdID){
        case IOTGW_RECONNECT:
        {
            strcpy(out_CmdData,"{\"susiCommData\":{\"commCmd\":125,\"handlerName\":\"general\",\"response\":{\"statuscode\":4,\"msg\": \"Reconnect\"}}}");
            return 0; 
        }
        case IOTGW_QUERY_HEART_BEAT_VALUE_REQUEST:
        {
            strcpy(out_CmdData,"{\"susiCommData\":{\"commCmd\":127,\"handlerName\":\"general\",\"sessionID\":\"123465\"}}");
            return 0;
        }
        case IOTGW_CHANGE_HEART_BEAT_VALUE_REQUEST:
        {
            strcpy(out_CmdData,"{\"susiCommData\": {\"commCmd\":129,\"handlerName\":\"general\",\"heartbeatrate\":5,\"sessionID\":\"123465\"}}");
            return 0;
        }
        case IOTGW_HANDLER_GET_CAPABILITY_REQUEST:
        {
            strcpy(out_CmdData,"{\"susiCommData\":{\"requestID\":1001,\"catalogID\": 4,\"commCmd\":2051,\"handlerName\":\"general\"}}");
            return 0;
        }
        default:
            break;
    }
    return -1;
}

int UpdateNodeList_ConnectivityNodeInfo(char* data){

    AdvJSON json(data);
    int i=0, j=0;
    int max=0;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};

    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());
 
    AdvJSON IoTGW_json=json["susiCommData"]["data"]["IoTGW"];
    max=IoTGW_json.Size();
    for(i=0; i < max ;i++){

        char type[128]={0};
        char device[128]={0};

        strcpy(type,IoTGW_json[i].Key().c_str());

        int max_device=IoTGW_json[type].Size();
        for(j=0; j<max_device ; j++){
            strcpy(device,IoTGW_json[type][j].Key().c_str());

            if ( strcmp("NULL",IoTGW_json[type][device]["Info"].Value().c_str()) != 0){

                //Get connectivity Info
                strcpy(connectivityInfo,IoTGW_json[type][device].Value().c_str());
 
                //Get connectivity device ID
                strcpy(connectivityDevID,IoTGW_json[type][device]["bn"].Value().c_str());
                //printf("connectivityDevID = %s\n", connectivityDevID);
                //printf("connectivity Info = %s\n", connectivityInfo);

                struct node node_data;
                memset(&node_data,0,sizeof(struct node));
                node_data.connectivityInfoWithData = connectivityInfo; 
                if ( UpdateNodeList(connectivityDevID, TYPE_CONNECTIVITY, &node_data) < 0){
                    ADV_C_DEBUG(COLOR_RED,"[%s][%s] #UpdateNodeList FAIL(connectivityDevID=%s)!#\n", __FILE__, __func__, connectivityDevID);
                }

            }
        }

    }
    return 0;
}

int BuildNodeList_IPBaseGatewayInfo(char* info_data){

    char* e_array[]={"{\"n\":\"SenHubList\",\"sv\":\"%s\"}",
                     "{\"n\":\"Neighbor\",\"sv\":\"%s\"}"
#if 0
                     "{\"n\":\"Name\",\"sv\":\"Ethernet\"}"
                     "{\"n\":\"Health\",\"v\":\"100.000000\"}",
                     "{\"n\":\"sw\",\"sv\":\"1.2.1.12\"}",
                     "{\"n\":\"reset\",\"bv\":\"0\"}"
#endif
                    };
    int i=0;
    int max_e_array_size=sizeof(e_array)/sizeof(char*);
    char sensorHubList[1024]={0};
    char tmp[1024]={0};
    int sensor_hub_cnt=0;
     
    sensor_hub_cnt = GetSensorHubList(sensorHubList, OS_IP_BASE, NULL);
    if ( sensor_hub_cnt <= 0 ){
        ADV_DEBUG("[%s][%s] sensor hub list is null\n", __FILE__, __func__);
        return -1;
    }

    strcat(info_data,"{\"Info\":");
    //
    strcat(info_data,"{");
    //
    strcat(info_data,"\"e\":[");
#if 1
    for(i=0; i < max_e_array_size ; i++){
        memset(tmp,0,sizeof(tmp));
        AdvJSON json(e_array[i]);

        if ( strcmp("SenHubList", json[0].Value().c_str()) == 0){
            if ( strlen(sensorHubList) == 0){
                strcpy(tmp,"{\"n\":\"SenHubList\",\"sv\":\"\"}");
            }
            else{
                sprintf(tmp,e_array[i],sensorHubList);
            }
        }
        else if ( strcmp("Neighbor", json[0].Value().c_str()) == 0){
            if ( strlen(sensorHubList) == 0){
                strcpy(tmp,"{\"n\":\"Neighbor\",\"sv\":\"\"}");
            }
            else{
                sprintf(tmp,e_array[i],sensorHubList);
            };
        }
        else{
            strcpy(tmp,e_array[i]);
        }
        strcat(info_data,tmp);
        strcat(info_data,",");
    }
    int len=strlen(info_data);
    info_data[len-1]=0;
#endif
    strcat(info_data,"]");
    //
    strcat(info_data,",");
    strcat(info_data,"\"bn\":\"Info\"");
    //
    strcat(info_data,"}");
    //
    strcat(info_data,",");
    sprintf(tmp,"\"bn\":\"%s%s\"",CONNECTIVITY_ID_PREFIX,g_GWInfMAC);
    strcat(info_data,tmp);
    strcat(info_data,",");
    strcat(info_data,"\"ver\":1");
    strcat(info_data,"}");

    return sensor_hub_cnt;

}
