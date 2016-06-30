#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MqttHal.h"

struct gw_node* g_pGWNodeListHead=NULL;

void test();
void GW_list_AddNode(struct gw_node ** head, char* pVirtualGatewayDevID );
struct gw_node * GW_list_GetNode(char* devID);
int GW_list_DeleteNode(char* devID);

void test(){
    //add
    printf( "Add1, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "1_123");
    printf( "Add2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "2_123");
    printf( "Add3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "3_123");
    printf( "g_pGWNodeListHead=%p\n", g_pGWNodeListHead);

    //delete
    printf( "Del3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode("3_123");
    printf( "Del2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode("2_123");
    printf( "Del3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode("1_123");
    printf( "g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    
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

void GW_list_AddNode(struct gw_node ** head, char* pVirtualGatewayDevID )
{

    struct gw_node *temp=NULL;
    char tmp_devID[32]={0};

    temp=(struct node *)malloc(sizeof(struct node));
    memset(temp,0,sizeof(struct node));
   
    strcpy(temp->virtualGatewayDevID,pVirtualGatewayDevID);

    if ( *head == NULL)
    {
        *head=temp;
        (*head)->next=NULL;
    }
    else
    {
        temp->next=(*head);
        (*head)=temp;
    }

}
