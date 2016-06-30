#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MqttHal.h"

static struct gw_node* g_pGWNodeListHead=NULL;

void test();
int GW_list_AddNode(char* pVirtualGatewayDevID );
struct gw_node * GW_list_GetNode(char* devID);
struct gw_node * GW_list_GetHeadNode();
int GW_list_DeleteNode(char* devID);
int GW_list_ClearAll();


void test(){
    //add1
    printf( "Add1, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode("1_123");
    printf( "Node1=%p\n", g_pGWNodeListHead);
    //add2
    printf( "Add2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode("2_123");
    printf( "Node2=%p\n", g_pGWNodeListHead);
    //add3
    printf( "Add3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode("3_123");
    printf( "Node3=%p\n", g_pGWNodeListHead);
    printf( "    , g_pGWNodeListHead=%p\n", g_pGWNodeListHead);

#if 0
    GW_list_ClearAll(&g_pGWNodeListHead);
    printf( "GW_list_ClearAll: g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
#endif

#if 0
    //delete
    printf( "Del2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode("2_123");
    printf( "node3=%p\n", GW_list_GetNode("3_123"));
    printf( "node1=%p\n", GW_list_GetNode("1_123"));

    printf( "Del3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    int res=GW_list_DeleteNode("3_123");
    printf( "res=%d\n", res);
    printf( "node1=%p\n", GW_list_GetNode("1_123"));

    printf( "Del1, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode("1_123");
    printf( "    , g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
#endif

#if 1
    //delete
    struct gw_node* tmp;
    //printf( "Del2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    tmp=GW_list_GetHeadNode();
    printf( "Del address=%p\n", tmp);
    GW_list_DeleteNode(tmp->virtualGatewayDevID);
    printf( "node2=%p\n", GW_list_GetNode("2_123"));
    printf( "node1=%p\n", GW_list_GetNode("1_123"));
    //
    tmp=GW_list_GetHeadNode();
    printf( "Del address=%p\n", tmp);
    int res=GW_list_DeleteNode(tmp->virtualGatewayDevID);
    printf( "res=%d\n", res);
    //
    tmp=GW_list_GetHeadNode();
    printf( "Del address=%p\n", tmp);
    GW_list_DeleteNode(tmp->virtualGatewayDevID);
    //
    tmp=GW_list_GetHeadNode();
    printf( "Del address=%p\n", tmp);
    printf( "g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
#endif
    
}

struct gw_node * GW_list_GetHeadNode(){
    return g_pGWNodeListHead;
}

int GW_list_ClearAll(){

    struct gw_node *temp, *prev;
    temp=g_pGWNodeListHead;

    while(temp!=NULL)
    {
            if(temp==g_pGWNodeListHead){
                g_pGWNodeListHead=temp->next;
                free(temp);
                temp=g_pGWNodeListHead;
            }
            else{
                prev->next=temp->next;
                free(temp);
                temp= prev->next;
            }
    }

    return 0;
}

struct gw_node * GW_list_GetNode(char* devID)
{
    struct gw_node * r;
    r=g_pGWNodeListHead;

    if(r==NULL)
    {
        return NULL;
    }
    while(r!=NULL)
    {
        if (strcmp(r->virtualGatewayDevID, devID) == 0){
            return r;
        }
        r=r->next;
    }

    return NULL;
}

int GW_list_DeleteNode(char* devID)
{
    struct gw_node *temp, *prev;
    temp=g_pGWNodeListHead;
    char tmp_devID[32]={0};

    while(temp!=NULL)
    {
        //
        if( strcmp(temp->virtualGatewayDevID,devID) == 0 )
        {
            if(temp==g_pGWNodeListHead)
            {
                g_pGWNodeListHead=temp->next;
                free(temp);
                return 0;
            }
            else
            {
                prev->next=temp->next;
                free(temp);
                return 0;
            }
        }
        else{
            prev=temp;
            temp= temp->next;
        }
    }

    return -1;
}

int GW_list_AddNode(char* pVirtualGatewayDevID )
{

    struct gw_node *temp=NULL;
    char tmp_devID[32]={0};

    temp=(struct gw_node *)malloc(sizeof(struct gw_node));
    if ( temp == NULL){
        return -1;
    }

    memset(temp,0,sizeof(struct gw_node));
   
    strcpy(temp->virtualGatewayDevID,pVirtualGatewayDevID);

    if ( g_pGWNodeListHead == NULL)
    {
        g_pGWNodeListHead=temp;
        g_pGWNodeListHead->next=NULL;
    }
    else
    {
        temp->next=g_pGWNodeListHead;
        g_pGWNodeListHead=temp;
    }

    return 0;

}
