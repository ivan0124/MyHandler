#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MqttHal.h"

struct gw_node* g_pGWNodeListHead=NULL;

void test();
void GW_list_AddNode(char* pVirtualGatewayDevID );
struct gw_node * GW_list_GetNode(char* devID);
int GW_list_DeleteNode(char* devID, int devType);

void test(){
}

struct gw_node * GW_list_GetNode(char* devID)
{
    printf("@@@@@@@@@@@@@@@@ GW_list_GetNode @@@@@@@@@@@@@\n");
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

int GW_list_DeleteNode(char* devID, int devType)
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
                return 1;
            }
            else
            {
                prev->next=temp->next;
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

void GW_list_AddNode(char* pVirtualGatewayDevID )
{
    struct gw_node *temp=NULL;
    char tmp_devID[32]={0};

    //temp->data=num;
    if (g_pGWNodeListHead == NULL)
    {
        g_pGWNodeListHead=temp;
        g_pGWNodeListHead->next=NULL;
    }
    else
    {
        temp->next=g_pGWNodeListHead;
        g_pGWNodeListHead=temp;
    }

}
