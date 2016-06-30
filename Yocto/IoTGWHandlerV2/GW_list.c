#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "MqttHal.h"

struct gw_node* g_pGWNodeListHead=NULL;

void test();
void GW_list_AddNode(struct gw_node ** head, char* pVirtualGatewayDevID );
struct gw_node * GW_list_GetNode(struct gw_node ** head, char* devID);
int GW_list_DeleteNode(struct gw_node ** head, char* devID);

void test(){
    //add1
    printf( "Add1, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "1_123");
    printf( "Node1=%p\n", g_pGWNodeListHead);
    //add2
    printf( "Add2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "2_123");
    printf( "Node2=%p\n", g_pGWNodeListHead);
    //add3
    printf( "Add3, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_AddNode(&g_pGWNodeListHead, "3_123");
    printf( "Node3=%p\n", g_pGWNodeListHead);
    printf( "    , g_pGWNodeListHead=%p\n", g_pGWNodeListHead);

    //delete
    printf( "Del2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode(&g_pGWNodeListHead, "2_123");
    printf( "node3=%p\n", GW_list_GetNode(&g_pGWNodeListHead,"3_123"));
    printf( "node1=%p\n", GW_list_GetNode(&g_pGWNodeListHead,"1_123"));

    printf( "Del2, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    int res=GW_list_DeleteNode(&g_pGWNodeListHead, "2_123");
    printf( "res=%d\n", res);
    printf( "node1=%p\n", GW_list_GetNode(&g_pGWNodeListHead,"1_123"));

    printf( "Del1, g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    GW_list_DeleteNode(&g_pGWNodeListHead, "1_123");
    printf( "    , g_pGWNodeListHead=%p\n", g_pGWNodeListHead);
    
}

struct gw_node * GW_list_GetNode(struct gw_node ** head, char* devID)
{
    struct gw_node * r;
    r=(*head);

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

int GW_list_DeleteNode(struct gw_node ** head, char* devID)
{
    struct gw_node *temp, *prev;
    temp=(*head);
    char tmp_devID[32]={0};

    while(temp!=NULL)
    {
        //
        if( strcmp(temp->virtualGatewayDevID,devID) == 0 )
        {
            if(temp==(*head))
            {
                (*head)=temp->next;
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
