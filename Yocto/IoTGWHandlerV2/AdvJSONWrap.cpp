#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "AdvJSON.h"
#include "MqttHal.h"

#ifdef __cplusplus
extern "C" {
#endif

struct node* g_pVirtualGatewayDataListHead=NULL;

struct node * GetVirtualGatewayDataListNode(char* devID);
int DeleteVirtualGatewayDataListNode(char* devID);
void AddVirtualGatewayDataListNode(char* pVirtualGatewayDevID, char* pConnectivityType, char* pConnectivityDevID, char* pConnectivityInfo, int iConnectivityInfoSize);
int UpdateVirtualGatewayDataListNode(char* data);
void aTest(const char* mydata);

#ifdef __cplusplus
}
#endif

struct node * GetVirtualGatewayDataListNode(char* devID)
{
    struct node * r;
    r=g_pVirtualGatewayDataListHead;

    if(r==NULL)
    {
    return NULL;
    }
    while(r!=NULL)
    {
    if (strcmp(r->connectivityDevID, devID) == 0){
        //printf("[%s][%s] found [%s],msg:%s\n", __FILE__, __func__, r->connectivityDevID, r->connectivityInfo );
        return r;
    }
    r=r->next;
    }

    return NULL;
    //printf("\n");
}

int DeleteVirtualGatewayDataListNode(char* devID)
{
    struct node *temp, *prev;
    temp=g_pVirtualGatewayDataListHead;
    while(temp!=NULL)
    {
    if( strcmp(temp->connectivityDevID,devID) == 0 )
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

void AddVirtualGatewayDataListNode(char* pVirtualGatewayDevID, char* pConnectivityType, char* pConnectivityDevID, char* pConnectivityInfo, int iConnectivityInfoSize)
{
    struct node *temp=NULL;
    //
    temp=GetVirtualGatewayDataListNode(pConnectivityDevID);
    //
    if (temp != NULL){
        DeleteVirtualGatewayDataListNode(pConnectivityDevID);
        temp=NULL;
    }
    //
    temp=(struct node *)malloc(sizeof(struct node));
    memset(temp,0,sizeof(struct node));
    temp->connectivityInfo=(char*)malloc(iConnectivityInfoSize+1);
    strcpy(temp->connectivityDevID,pConnectivityDevID);
    strcpy(temp->connectivityInfo,pConnectivityInfo);
    strcpy(temp->virtualGatewayDevID, pVirtualGatewayDevID);
    strcpy(temp->connectivityType,pConnectivityType);

    //temp->data=num;
    if (g_pVirtualGatewayDataListHead == NULL)
    {
    g_pVirtualGatewayDataListHead=temp;
    g_pVirtualGatewayDataListHead->next=NULL;
    }
    else
    {
    temp->next=g_pVirtualGatewayDataListHead;
    g_pVirtualGatewayDataListHead=temp;
    }

} 

int UpdateVirtualGatewayDataListNode(char* data){

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
#if 0
		printf("********************************************\n");
		printf("virtualGateway DevID: "); printf(virtualGatewayDevID); printf("\n");
		printf("connectivity Info: "); printf(connectivityInfo); printf("\n");
		printf("connectivity DevID: "); printf(connectivityDevID);
		printf("\n********************************************\n");
#endif
                //Add Node
                AddVirtualGatewayDataListNode(virtualGatewayDevID,type,connectivityDevID,connectivityInfo, strlen(connectivityInfo));
            }
        }

    }
    
    return 0;
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
#if 1
		printf("********************************************\n");
		printf("virtualGateway DevID: "); printf(virtualGatewayDevID); printf("\n");
		printf("connectivity Info: "); printf(connectivityInfo); printf("\n");
		printf("connectivity DevID: "); printf(connectivityDevID);
		printf("\n********************************************\n");
#endif
		//get connectivity node
#if 1
		temp=GetVirtualGatewayDataListNode(connectivityDevID);
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

                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());
                             strcpy(temp->connectivitySensorHubList, IoTGW_device_info[k][l+1].Value().c_str());
                        }
                        //Get Neighbor value
                        if ( strcmp("n",IoTGW_device_info[k][l].Key().c_str()) == 0 &&
                             strcmp("Neighbor",IoTGW_device_info[k][l].Value().c_str()) == 0 && 
                             strcmp("sv",IoTGW_device_info[k][l+1].Key().c_str()) == 0){

                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l].Key().c_str(), IoTGW_device_info[k][l].Value().c_str()); 
                             printf("@@@@@@@@@@@ Key=%s, Value=%s\n", IoTGW_device_info[k][l+1].Key().c_str(), IoTGW_device_info[k][l+1].Value().c_str());
                             strcpy(temp->connectivityNeighborList, IoTGW_device_info[k][l+1].Value().c_str());
                        }
                    }
                }

            }
        }

    }
}
