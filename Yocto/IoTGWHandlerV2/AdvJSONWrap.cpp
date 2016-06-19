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
int StoreVirtualGatewayDataListNode(JSONode *json, char* json_path);
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
        printf("found [%s],msg:%s\n",r->connectivityDevID, r->connectivityInfo );
        return r;
    }
    r=r->next;
    }

    return NULL;
    printf("\n");
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

int StoreVirtualGatewayDataListNode(JSONode *json, char* json_path){
 
    struct node *n;
    int cnt=0;
    //char connectivity_type[256]={0};
    char connectivity_base_name[256]={0};
    //char nodeContent[MAX_JSON_NODE_SIZE]={0};
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};

#if 0
    //Get virtual gateway devID
    if ( GetJSONValue(json, OBJ_AGENT_ID, virtualGatewayDevID) < 0){
        return -1;
    }
    
    //Get connectivity Info
    memset(connectivityInfo,0,sizeof(connectivityInfo));
    if ( GetJSONValue(json, json_path, connectivityInfo) < 0){
        return -1;
    }
            
    //Get connectivity device ID
    sprintf(connectivity_base_name,"%s[bn]",json_path);
    memset(connectivityDevID,0,sizeof(connectivityDevID));
    if ( GetJSONValue(json, connectivity_base_name, connectivityDevID) < 0){
        return -1;
    }
    //
    printf("********************************************\n");
    printf("virtualGateway DevID: "); printf(virtualGatewayDevID); printf("\n");
    printf("connectivity Info: "); printf(connectivityInfo); printf("\n");
    printf("connectivity DevID: "); printf(connectivityDevID);
    printf("\n********************************************\n");

    //Add Node
    AddVirtualGatewayDataListNode(virtualGatewayDevID,g_connType,connectivityDevID,connectivityInfo, strlen(connectivityInfo));
#if 0
    cnt=CountAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead);
    printf("count = %d\n", cnt);
    //
    DisplayAllVirtualGatewayDataListNode(g_pVirtualGatewayDataListHead, n);
#endif
#endif
    return 0;
}

void aTest(const char* mydata){
    printf("call aTest\n");
    AdvJSON json(mydata);
    int i=0, j=0;
    int max=0;
    char connectivity_base_name[256]={0};
    //char nodeContent[MAX_JSON_NODE_SIZE]={0};
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN]={0};
    char connectivityInfo[MAX_JSON_NODE_SIZE]={0};
    char connectivityDevID[MAX_DEVICE_ID_LEN]={0};

    //Get virtual gateway devID
    strcpy(virtualGatewayDevID,json["susiCommData"]["agentID"].Value().c_str());
    //printf(virtualGatewayDevID);

    char a[128]={"susiCommData"};
    max=json["susiCommData"]["infoSpec"]["IoTGW"].Size();
    for(i=0; i < max ;i++){
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%d]=%s\n",i,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
        char type[128]={0};
        char device[128]={0};
        //strcpy(type,"123");

        strcpy(type,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
#if 0
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s]=%s\n",\
        type,
        json["susiCommData"]["infoSpec"]["IoTGW"][type].Value().c_str());
#endif        
        int max_conn_type=json["susiCommData"]["infoSpec"]["IoTGW"][type].Size();
        for(j=0; j<max_conn_type ; j++){
            strcpy(device,json["susiCommData"]["infoSpec"]["IoTGW"][type][j].Key().c_str());
            int tmp_value[1024]={0};
            if ( strcmp("NULL",json["susiCommData"]["infoSpec"]["IoTGW"][type][device]["Info"].Value().c_str()) != 0){
                printf("@@@@ json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s][%s]=%s\n",\
                type,device, \
                json["susiCommData"]["infoSpec"]["IoTGW"][type][device].Value().c_str());

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
}
