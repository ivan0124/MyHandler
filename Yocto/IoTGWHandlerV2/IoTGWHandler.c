/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : IoTSensors_Handler                                     										*/
/* Reference    : None																									 */
/****************************************************************************/
//#include <stdbool.h>
#include <stddef.h>

#include "platform.h"
#include "susiaccess_handler_api.h"

// IoTSensorHandler Header Files
#include "IoTGWHandler.h"
#include "IoTGWFunction.h"
#include "inc/IoTGW_Def.h"
#include "inc/SensorNetwork_BaseDef.h"
#include "inc/SensorNetwork_API.h"
#include "common.h"
#include "SnGwParser.h"
// include lib Header Files
#include "BasicFun_Tool.h"
#include "MqttHal.h"
#include "AdvLog.h"

// include AdvApiMux
#include "inc/AdvApiMux/AdvAPIMuxServer.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2002
#define cagent_custom_action 30002
const char strPluginName[MAX_TOPIC_LEN] = {"IoTGW"};
const int iRequestID = cagent_request_custom;
const int iActionID = cagent_custom_action;

//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   void* threadHandler;
   bool isThreadRunning;
}handler_context_t;

CAGENT_MUTEX_TYPE g_NodeListMutex;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static void* g_loghandle = NULL;
static bool g_bEnableLog = TRUE;
static char agentConfigFilePath[MAX_PATH] = {0};
static char g_mac_to_register[32]={0};

static Handler_info  g_PluginInfo;
static handler_context_t g_HandlerContex;
//static HANDLER_THREAD_STATUS g_status = handler_no_init;
static bool m_bAutoReprot = false;
static time_t g_monitortime;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendinfospeccbf = NULL;		


// SenHub
static Handler_info  g_SenPluginInfo[MAX_SENNODES];  
static cagent_agent_info_body_t g_SenHubAgentInfo[MAX_SENNODES];
static cagent_agent_info_body_t g_gw;


int runing = 0;

extern struct node* g_pNodeListHead;
//extern char g_connectivity_capability[1024];

//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------

void update_sendata(void *arg);
static int SendMsgToSUSIAccess(  const char* Data, unsigned int const DataLen, void *pRev1, void* pRev2 );

//#ifdef _MSC_VER
//BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
//{
//	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
//	{
//		printf("DllInitializer\r\n");
//		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
//		if (reserved == NULL) // Dynamic load
//		{
//			// Initialize your stuff or whatever
//			// Return FALSE if you don't want your module to be dynamically loaded
//		}
//		else // Static load
//		{
//			// Return FALSE if you don't want your module to be statically loaded
//			return FALSE;
//		}
//	}
//
//	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
//	{
//		printf("DllFinalizer\r\n");
//		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
//		{
//			// Cleanup
//			Handler_Uninitialize();
//		}
//		else // Process is terminating
//		{
//			// Cleanup
//			Handler_Uninitialize();
//		}
//	}
//	return TRUE;
//}
//#else
//__attribute__((constructor))
///**
// * initializer of the shared lib.
// */
//static void Initializer(int argc, char** argv, char** envp)
//{
//    printf("DllInitializer\r\n");
//}
//
//__attribute__((destructor))
///** 
// * It is called when shared lib is being unloaded.
// * 
// */
//static void Finalizer()
//{
//    printf("DllFinalizer\r\n");
//	Handler_Uninitialize();
//}
//#endif

void Handler_CustMessageRecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	printf(" %s> Topic:%s, Data:%s\r\n", strPluginName, topic, (char*)data);
}

static CAGENT_PTHREAD_ENTRY(ThreadCheckNodeList, args)
{

    handler_context_t * pHandlerCtx = ( handler_context_t * )args;
    char gateway_capability[2048]={0};
    char info_data[2048]={0};

    while( pHandlerCtx->isThreadRunning )
    {
        struct gw_node* tmp_node=NULL;
#if 1
        time(&g_monitortime);
        app_os_sleep(CHECK_NODE_LIST_TIME);
#if 1	
        time_t tv;
	time(&tv);
	float diff_time=difftime(tv, g_monitortime);
        //printf("[%s][%s] wake up...(difftime=%f)\n", __FILE__, __func__, diff_time);
        ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #wake up to check...(difftime=%f)# \033[0m\n", __FILE__, __func__, diff_time);
#endif
	

#if 1
        struct node * r;
        r=g_pNodeListHead;

        if(r==NULL)
        {
            continue;
        }

        app_os_mutex_lock(&g_NodeListMutex);
        while(r!=NULL)
        {
            if ( r->nodeType == TYPE_GATEWAY) {
                    switch(r->state){
                        case STATUS_CONNECTED:
                        {
                            if ( r->last_hb_time != 0){
		                diff_time=difftime(tv, r->last_hb_time);
                                ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #CHECK %s HB last time=%f# \033[0m\n", __FILE__, __func__, r->virtualGatewayDevID,diff_time);
			        if ( diff_time > HEART_BEAT_TIMEOUT ){
				    r->state = STATUS_DISCONNECTED;
                                    time(&(r->start_connecting_time));
                                    ADV_C_DEBUG(COLOR_YELLOW,"[%s][%s]\033[33m #Set %s as disconnect # \033[0m\n", __FILE__, __func__, r->virtualGatewayDevID);
				    GW_list_AddNode(r->virtualGatewayDevID);
			        }
                            }
                            break;
                        }
                        case STATUS_CONNECTING:
                        {
#if 0
                            diff_time=difftime(tv, r->start_connecting_time);
                            printf("[%s][%s] connecting time (difftime=%f)\n", __FILE__, __func__, diff_time);
                            if ( diff_time > 10){
                                r->state = STATUS_DISCONNECTED;
                                r->start_connecting_time = 0;
                            }
#endif
                            break;
                        }
                        case STATUS_DISCONNECTED:
                        {
                            break;
                        }
                    }

            }
            r=r->next;
        }
#endif
        tmp_node=GW_list_GetHeadNode();

        if ( tmp_node ){
            //Disconnect all disconnected sensor hub
            DisconnectToRMM_AllDisconnectedSensorHubNode();
            //Delete all disconnected dvice id node
            DeleteNodeList_AllDisconnectedGatewayUIDNode();
            //Rebuild gateway capability and send to RMM
            memset(gateway_capability,0,sizeof(gateway_capability));
            BuildNodeList_GatewayCapabilityInfo(g_pNodeListHead, gateway_capability);

            memset(info_data, 0, sizeof(info_data));
            BuildNodeList_GatewayCapabilityInfoWithData(g_pNodeListHead, info_data);
        }

	app_os_mutex_unlock(&g_NodeListMutex);

#if 1
        if ( tmp_node ){
            ADV_C_DEBUG(COLOR_YELLOW, "[%s][%s] Register Gateway Capability!!!\n", __FILE__, __func__);
            ADV_DEBUG("[%s][%s] gateway_capability=%s\n", __FILE__, __func__, gateway_capability);
            if ( RegisterToRMM_GatewayCapabilityInfo(gateway_capability, strlen(gateway_capability)) < 0){
                        ADV_C_ERROR(COLOR_RED, "[%s][%s] Register Gateway Capability FAIL !!!\n", __FILE__, __func__);
            }

            ADV_C_DEBUG(COLOR_YELLOW, "[%s][%s] Update Gateway Info!!!\n", __FILE__, __func__);
            ADV_DEBUG("[%s][%s] info_data=%s\n", __FILE__, __func__, info_data);
            UpdateToRMM_GatewayUpdateInfo(info_data);
        }

        //char mydata[512]={"{\"susiCommData\":{\"commCmd\":125,\"handlerName\":\"general\",\"response\":{\"statuscode\":4,\"msg\": \"Reconnect\"}}}"};
        //Send Re-connect
        char data[128]={0};
        char VirtualGatewayUID[128]={0};
	tmp_node=GW_list_GetHeadNode();

        char request_cmd[512]={0};
        GetRequestCmd(IOTGW_HANDLER_GET_CAPABILITY_REQUEST, request_cmd);

        while ( tmp_node != NULL){
            ADV_DEBUG("[%s][%s] Re-connect devID=%s\n", tmp_node->virtualGatewayDevID, __FILE__, __func__);
            SendRequestToWiseSnail(tmp_node->virtualGatewayDevID,request_cmd);
            GW_list_DeleteNode(tmp_node->virtualGatewayDevID);
	    tmp_node=GW_list_GetHeadNode();
        }
#endif
        
#endif
    }
    
    ADV_DEBUG("[%s][%s] thread exit\n", __FILE__, __func__);
    app_os_thread_exit(0);

    return 0;
}


static CAGENT_PTHREAD_ENTRY(ThreadSendSenHubConnect, args)
{
	int i=0;
	int len = 0;
	Handler_info  *pSenHander = NULL;
	char buffer[MAX_BUFFER_SIZE]={0};
	//cagent_agent_info_body_t *pSenAgentInfo = NULL;
#if 1
        char info_data[1024]={0};
        BuildNodeList_GatewayCapabilityInfoWithData(g_pNodeListHead, info_data);
#if 1
        ADV_DEBUG("------------------------------------------------\n");
        ADV_DEBUG("[%s][%s]@@@@@@@@@@@@@@@@@@@@@@@@ BuildNodeList_GatewayCapabilityInfoWithData, info_data=%s\n", __FILE__, __func__, info_data);
        ADV_DEBUG("------------------------------------------------\n");
#endif
        UpdateToRMM_GatewayUpdateInfo(info_data);
#endif

#if 1
	ADV_DEBUG("[%s][%s] Start send SenHub connect message\r\n", __FILE__, __func__);
	for( i = 0; i< MAX_SENNODES ; i ++ ) {
		if(g_SenHubAgentInfo[i].status == 1)
		{
			pSenHander = &g_SenPluginInfo[i];

			// 1. Register to WISECloud
			ADV_DEBUG("Send SenHub %s\r\n", pSenHander->agentInfo->hostname);
			SenHubConnectToWISECloud( pSenHander );
		}
	}
	ADV_DEBUG("[%s][%s] Finish send SenHub connect message\r\n", __FILE__, __func__);
#endif
	app_os_thread_exit(0);
	return 0;
}

/* **************************************************************************************
 *  Function Name: Handler_Initialize
 *  Description: Init any objects or variables of this handler
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Initialize( HANDLER_INFO *pluginfo )
{
	if( pluginfo == NULL )
		return handler_fail;

	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\n\r\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	if(g_bEnableLog)
	{
		g_loghandle = pluginfo->loghandle;
	}

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;


	g_sendinfospeccbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;

	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
	//g_status = handler_init;
	/*Create mutex*/
	if ( app_os_mutex_setup( &g_NodeListMutex ) != 0 ) {
		printf("[%s][%s] Create HDDInfo mutex failed!\n", __FILE__, __func__ );
		return handler_fail;
	}

	if( InitSNGWHandler() < 0 )
	    return handler_fail;
 
        MqttHal_Proc();

	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Uninitialize
 *  Description: Release the objects or variables used in this handler
 *  Input :  None
 *  Output: None
 *  Return:  void
 * ***************************************************************************************/
void Handler_Uninitialize()
{
	g_sendcbf = NULL;
	g_sendcustcbf = NULL;
	g_sendreportcbf = NULL;
	g_sendinfospeccbf = NULL;
	g_subscribecustcbf = NULL;
	
        // Release mutex
        app_os_mutex_cleanup( &g_NodeListMutex );
	//<Eric>
	UnInitSNGWHandler();
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Status
 *  Description: Get Handler Threads Status. CAgent will restart current Handler or restart CAgent self if busy.
 *  Input :  None
 *  Output: char * : pOutStatus       // cagent handler status
 *  Return:  handler_success  : Success Init Handler
 *			 handler_fail : Fail Init Handler
 * **************************************************************************************/
int HANDLER_API Handler_Get_Status( HANDLER_THREAD_STATUS * pOutStatus )
{
	*pOutStatus = handler_status_start;
	return handler_success;
/*
	int iRet = handler_fail; 

	if(!pOutStatus) return iRet;

	switch (g_status)
	{
	default:
	case handler_no_init:
	case handler_init:
	case handler_stop:
		*pOutStatus = g_status;
		break;
	case handler_start:
	case handler_busy:
		{
			time_t tv;
			time(&tv);
			if(difftime(tv, g_monitortime)>5)
				g_status = handler_busy;
			else
				g_status = handler_busy;
			*pOutStatus = g_status;
		}
		break;
	}
	
	iRet = handler_success;
	return iRet;
*/
}

/* **************************************************************************************
 *  Function Name: Handler_OnStatusChange
 *  Description: Agent can notify handler the status is changed.
 *  Input :  PLUGIN_INFO *pluginfo
 *  Output: None
 *  Return:  None
 * ***************************************************************************************/
void HANDLER_API Handler_OnStatusChange( HANDLER_INFO *pluginfo )
{
	int i=0;
	Handler_info  *pSenHander = NULL;
	//cagent_agent_info_body_t *pSenAgentInfo = NULL;

	printf(" %s> Update Status\r\n", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
	}
	if(pluginfo->agentInfo->status == 1)
	{
		void* threadHandler;
		if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
		{
			app_os_thread_detach(threadHandler);
		}
	}
}

/* **************************************************************************************
 *  Function Name: Handler_Start
 *  Description: Start Running
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Start( void )
{
	printf("> %s Handler_Start\r\n", strPluginName);
#if 1
	if (app_os_thread_create(&g_HandlerContex.threadHandler, ThreadCheckNodeList, &g_HandlerContex) == 0)
        {
            app_os_thread_detach(g_HandlerContex.threadHandler);
        }
        else
	{
		g_HandlerContex.isThreadRunning = false;
		printf("[%s][%s] start handler thread failed!\r\n", __FILE__, __func__);	
		return handler_fail;
        }
#endif
	g_HandlerContex.isThreadRunning = true;
	//g_status = handler_start;
	time(&g_monitortime);

	// <Eric>
	//StartSNGWHandler();
	return handler_success;
}


/* **************************************************************************************
 *  Function Name: Handler_Stop
 *  Description: Stop the handler
 *  Input :  None
 *  Output: None
 *  Return:  0  : Success Init Handler
 *              -1 : Fail Init Handler
 * ***************************************************************************************/
int HANDLER_API Handler_Stop( void )
{
	if(g_HandlerContex.isThreadRunning == true)
	{
		g_HandlerContex.isThreadRunning = false;
		//app_os_thread_join(g_HandlerContex.threadHandler);
		g_HandlerContex.threadHandler = NULL;
	}
	//g_status = handler_stop;

	// <Eric>
	//StopSNGWHandler();
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *					void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
        ADV_DEBUG("[%s][%s] Handler_Recv Enter\n", __FILE__, __func__);
	int cmdID = 0;
	int len = 0;
        int osInfo;
	char szSessionId[MAX_SIZE_SESSIONID]={0};
	char buffer[MAX_BUFFER_SIZE]={0};
        char VirtualGatewayUID[64]={0};
        char tmp_uid[64]={0};
	// Recv Remot Server Command to process
	PRINTF("%s>Recv Topic [%s] Data %s\r\n", strPluginName, topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID,szSessionId,sizeof(szSessionId)))
		return;

	PRINTF("Cmd ID=%d\r\n",cmdID);
        

        app_os_mutex_lock(&g_NodeListMutex);
        if ( GetVirtualGatewayUIDfromData(g_pNodeListHead, data, VirtualGatewayUID, sizeof(VirtualGatewayUID)) < 0){
            ADV_C_ERROR(COLOR_RED, "[%s][%s] Can't find VirtualGatewayUID Topic=%s\r\n",__FILE__, __func__, topic );
            goto exit;
        }
        //printf("VirtualGatewayUID = %s\n", VirtualGatewayUID);
        osInfo=GetVirtualGatewayUIDOSInfo(VirtualGatewayUID);

	switch (cmdID)
	{
	case IOTGW_GET_CAPABILITY_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN, "[%s][%s]#Get Gateway Capability:%s# \n", __FILE__, __func__);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     char capability[MAX_DATA_SIZE]={0};
			
		     if ( BuildNodeList_GatewayCapabilityInfo(g_pNodeListHead, capability) < 0){
                         g_sendcbf(&g_PluginInfo, IOTGW_ERROR_REPLY, g_szErrorCapability, strlen( g_szErrorCapability )+1, NULL, NULL);
                     }
                     else{
                         //send gateway capability
		         g_sendcbf(&g_PluginInfo, IOTGW_GET_CAPABILITY_REPLY, capability, strlen( capability )+1, NULL, NULL);

		         void* threadHandler;
			 if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
			 {
			     app_os_thread_detach(threadHandler);
			 }

                     }
		}
		break;
	case IOTGW_GET_SENSOR_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN,"[%s][%s]#Get Gateway Request# \n", __FILE__, __func__);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);
                    if ( OS_NONE_IP_BASE == osInfo){
                        app_os_mutex_unlock(&g_NodeListMutex);
                        SendRequestToWiseSnail(VirtualGatewayUID,data);
                        return;
                    }
                    else if ( OS_IP_BASE == osInfo ){

                        char response_data[1024]={0};

                        GetConnectivityResponseData(data, response_data);

                        ADV_DEBUG("response_data=%s\n", response_data);

		        g_sendcbf(&g_PluginInfo, IOTGW_GET_SENSOR_REPLY, response_data, strlen( response_data )+1, NULL, NULL);
                    } 
                    printf("---------------------------------------------------------------\n");
		}
		break;
	case IOTGW_SET_SENSOR_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN,"[%s][%s]#Set Gateway Request:%s# \n", __FILE__, __func__);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);
                    if ( OS_NONE_IP_BASE == osInfo){
                        app_os_mutex_unlock(&g_NodeListMutex);
                        SendRequestToWiseSnail(VirtualGatewayUID,data);
                        return;
                    }
                    printf("---------------------------------------------------------------\n");
	
		}
		break;
	default:
		{
#if 1
			ADV_DEBUG("Unknow CMD ID=%d\r\n", cmdID );
			/*  {"sessionID":"1234","errorRep":"Unknown cmd!"}  */
			if(strlen(szSessionId)>0)
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\",\"sessionID\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!", szSessionId);
			else
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!");
			g_sendcbf(&g_PluginInfo, IOTGW_ERROR_REPLY, buffer, strlen(buffer)+1, NULL, NULL);
			break;
#endif
		}
		break;
	}

exit:
    app_os_mutex_unlock(&g_NodeListMutex);
    return;

}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: char * pOutReply
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStart(char *pInQuery)
{
	/*{"susiCommData":{"catalogID":4,"autoUploadIntervalSec":30,"requestID":1001,"requestItems":["all"],"commCmd":2053,"type":"WSN"}}*/
	/*TODO: Parsing received command*/
	m_bAutoReprot = true;
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStop
 *  Description: Stop Auto Report
 *  Input : None
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_AutoReportStop(char *pInQuery)
{
	/*TODO: Parsing received command*/
	m_bAutoReprot = false;
}

/* **************************************************************************************
 *  Function Name: Handler_Get_Capability
 *  Description: Get Handler Information specification. 
 *  Input :  None
 *  Output: char * : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{
    int len = 0; // Data length of the pOutReply 
    char *pBuffer = NULL;
    char capability[MAX_DATA_SIZE]={0};

    app_os_mutex_lock(&g_NodeListMutex);
    BuildNodeList_GatewayCapabilityInfo(g_pNodeListHead, capability);
    app_os_mutex_unlock(&g_NodeListMutex);

#if 0
    printf("Handler_Get_Capability\n");
    printf("---------------Gateway capability----------------------------\n");
    printf(capability);
    printf("\n-------------------------------------------\n");	
#endif
    len = strlen( capability );

    *pOutReply = (char *)malloc(len + 1);
    memset(*pOutReply, 0, len + 1);
    strcpy(*pOutReply, capability);

    {
        void* threadHandler;
	if (app_os_thread_create(&threadHandler, ThreadSendSenHubConnect, NULL) == 0)
        {
            app_os_thread_detach(threadHandler);
	}
    }

    return len;
}

/* **************************************************************************************
 *  Function Name: Handler_MemoryFree
 *  Description: free the mamory allocated for Handler_Get_Capability
 *  Input : char *pInData.
 *  Output: None
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_MemoryFree(char *pInData)
{
	if(pInData)
	{
		free(pInData);
		pInData = NULL;
	}
}

/****************************************************************************/
/* Below Functions are IoTGW Handler source code                   			 */
/****************************************************************************/

static int UpdateInterfaceData(  const char *pInJson, const int InDatalen );

static int Register_SenHub( const char *pJSON, const int nLen, void **pOutParam, void *pRev1, void *pRev2 );

static int Disconnect_SenHub( void *pOutParam );

static int SendInfoSpec_SenHub( const char *pInJson, const int InDataLen, void *pOutParam, void *pRev1 );

static int ProcSet_Result( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 );

static int SendMsgToSUSIAccess(  const char* Data, unsigned int const DataLen, void *pRev1, void* pRev2 )
{
	if( g_sendreportcbf ) {
		g_sendreportcbf( &g_PluginInfo, Data, DataLen, pRev1, pRev2);
		return 0;
	}
	return -1;
}


// topic: /cagent/admin/%s/agentcallbackreq
void HandlerCustMessageRecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	int cmdID = 0;
	char SenHubUID[MAX_SN_UID]={0};
	char buffer[MAX_BUFFER_SIZE]={0};
	char Topic[MAX_TOPIC_SIZE]={0};
	char szSessionId[MAX_SIZE_SESSIONID]={0};
	int index = -1;
	int len = 0;
	Handler_info  *pSenHander = NULL;
	Handler_info  SenHubHandler;

	int i = 0;
	// Recv Remot Server Command to process
	PRINTF("[%s][%s]%s>Recv Cust Topic [%s] Data %s\r\n", __FILE__, __func__, strPluginName, topic, (char*) data );

	if(!ParseReceivedData(data, datalen, &cmdID,szSessionId,sizeof(szSessionId))) {
		ADV_C_ERROR(COLOR_RED, "[%s][%s] Can't find cmdID\r\n", __FILE__, __func__ );
		return;
	}

	if( GetSenHubUIDfromTopic( topic, SenHubUID, sizeof(SenHubUID) ) == 0 ) {
		ADV_C_ERROR(COLOR_RED, "[%s][%s] Can't find SenHubID Topic=%s\r\n",__FILE__, __func__, topic );
		return;
	}

	if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, SenHubUID) ) == -1 ) {
		ADV_C_ERROR(COLOR_RED, "[%s][%s] Can't find SenHub UID in Table =%s\r\n",__FILE__, __func__, SenHubUID );
		return;
	}

	pSenHander = &g_SenPluginInfo[index];

	if( pSenHander ) {
		memset(&SenHubHandler,0,sizeof(SenHubHandler));
		memcpy(&SenHubHandler,pSenHander, sizeof(SenHubHandler));
		snprintf(SenHubHandler.Name,MAX_TOPIC_LEN, "%s", SENHUB_HANDLER_NAME); // "SenHub"
		pSenHander = &SenHubHandler;
	} else 
		return;
	// "/cagent/admin/%s/agentinfoack"
	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic), SENHUB_CALLBACKREQ_TOPIC, SenHubUID );
		
	ADV_DEBUG("[%s][%s] HandlerCustMessageRecv Cmd =%d\r\n",__FILE__, __func__, cmdID);


#if 1
	switch (cmdID)
	{
	case IOTGW_HANDLER_GET_CAPABILITY_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN,"[%s][%s]#Get Sensor Capability:%s#\n", __FILE__, __func__, SenHubUID);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);
                     SendRequestToWiseSnail(SenHubUID,data);
                     ADV_DEBUG("---------------------------------------------------------------\n");
		}
                break;
#if 0
	case IOTGW_GET_CAPABILITY_REQUEST:
		{
			// { "sessionID":"XXX", "StatusCode":200, "SenHub": { xxxx_JSON_Object } }
			printf("[%s][%s] IOTGW_GET_CAPABILITY_REQUEST\r\n", __FILE__, __func__ );

			//len = ProcGetSenHubCapability(SenHubUID, data, buffer, sizeof(buffer));
			//PRINTF("len=%d Ret=%s\n",len,buffer);
			//if( len > 0 )
				g_sendcustcbf(pSenHander,IOTGW_GET_CAPABILITY_REPLY, Topic, mydata, strlen(mydata)+1 , NULL, NULL);

		}
		break;
#endif
	case IOTGW_GET_SENSOR_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN, "[%s][%s]#Get Sensor Request:%s#\n", __FILE__, __func__, SenHubUID);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);

                     SendRequestToWiseSnail(SenHubUID,data);
                     ADV_DEBUG("---------------------------------------------------------------\n");		
		}
		break;
	case IOTGW_SET_SENSOR_REQUEST:
		{
                     ADV_DEBUG("---------------------------------------------------------------\n");
                     ADV_C_DEBUG(COLOR_GREEN, "[%s][%s]#Set Sensor Request:%s#\n", __FILE__, __func__, SenHubUID);
                     ADV_DEBUG("[%s][%s] topic = %s\n", __FILE__, __func__, topic);
                     ADV_DEBUG("[%s][%s] message=%s\n",__FILE__, __func__, data);
                     SendRequestToWiseSnail(SenHubUID,data);
                     ADV_DEBUG("---------------------------------------------------------------\n");
		}
		break;
	default:
		{
			ADV_DEBUG("[%s][%s]Unknow CMD ID=%d\r\n", __FILE__, __func__, cmdID );

			/*  {"sessionID":"1234","errorRep":"Unknown cmd!"}  */
			if(strlen(szSessionId)>0)
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\",\"sessionID\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!", szSessionId);
			else
				snprintf(buffer, sizeof(buffer), "{\"%s\":\"%s\"}", SN_ERROR_REP, "Unknown cmd!");
			g_sendcustcbf(pSenHander,IOTGW_ERROR_REPLY, Topic, buffer, strlen(buffer)+1 , NULL, NULL);
		}
		break;
	}
#endif
}


int SenHubConnectToWISECloud( Handler_info *pSenHubHandler)
{
        PRINTF("[%s][%s]SenHubConnectToWISECloud Enter\n", __FILE__, __func__);

	char Topic[MAX_TOPIC_SIZE]={0};
	char JSONData[MAX_FUNSET_DATA_SIZE]={0};
	int   datalen = 0;
	int   rc = 1;


	datalen = GetAgentInfoData(JSONData,sizeof(JSONData),pSenHubHandler);

	if(datalen <20 )
		rc = 0;


        datalen=strlen(JSONData);
	// 1. Subscribe Topic2 -> Create SenHub <-> WISECloud communication channel
	memset(Topic,0,sizeof(Topic));
	snprintf( Topic, sizeof(Topic), "/cagent/admin/%s/agentcallbackreq", pSenHubHandler->agentInfo->devId );
	if( g_subscribecustcbf ){
		g_subscribecustcbf( Topic, &HandlerCustMessageRecv );
        }
	PRINTF("[%s][%s]Subscribe SenHub Topic:%s \n", __FILE__, __func__, Topic);
	// 2. SendAgentInfo online
	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic),"/cagent/admin/%s/agentinfoack",pSenHubHandler->agentInfo->devId);
	if( g_sendcustcbf ){
		g_sendcustcbf(pSenHubHandler,1,Topic,JSONData, datalen, NULL, NULL);
        }
	printf("[%s][%s]Send SenHub AgentInfo:%s \n", __FILE__, __func__, JSONData);
	PRINTF("SenHubConnectToWISECloud Leave\n");

	return 1;

}
int SenHubDisconnectWISECloud( Handler_info *pSenHubHandler)
{
	char Topic[MAX_TOPIC_SIZE]={0};
	char JSONData[MAX_FUNSET_DATA_SIZE]={0};
	int   datalen = 0;
	int   rc = 1;

	memset(Topic,0,sizeof(Topic));
	snprintf(Topic,sizeof(Topic),"/cagent/admin/%s/agentinfoack",pSenHubHandler->agentInfo->devId);

	datalen = GetAgentInfoData(JSONData,sizeof(JSONData),pSenHubHandler);

	if(datalen <20 )
		rc = 0;

	if( g_sendcustcbf )
		g_sendcustcbf(pSenHubHandler,1,Topic,JSONData, datalen, NULL, NULL);
	else 
		rc = 0;

	return rc;
}

static int UpdateInterfaceData(  const char *pInJson, const int InDatalen )
{
	return SendMsgToSUSIAccess( pInJson, InDatalen, NULL, NULL );
}

static int Register_SenHub( const char *pJSON, const int nLen, void **pOutParam, void *pRev1, void *pRev2 )
{
	int rc = 0;
	int index = 0;

	SenHubInfo *pSenHubInfo = (SenHubInfo*)pRev1;

	Handler_info  *pSenHander = NULL;

	if( pSenHubInfo == NULL ) {
		PRINTF("[%s][%s] SenHub is NULL\r\n", __FILE__, __func__);
		return rc;
	}
;
	// 1. Find empty SenHub Array
	if(  (index = GetUsableIndex(&g_SenHubAgentInfo, MAX_SENNODES ) )  == -1 ) 
	{
		PRINTF("GW Handler is Full \r\n");
		return rc;
	}
        
	pSenHander = &g_SenPluginInfo[index];
	//*pOutParam    = &g_SenPluginInfo[index];

	// 2. Prepare Sensor Node Handler_info data
	PackSenHubPlugInfo( pSenHander, &g_PluginInfo, pSenHubInfo->sUID, pSenHubInfo->sHostName, pSenHubInfo->sProduct, 1 );

	// 3. Register to WISECloud
	rc = SenHubConnectToWISECloud( pSenHander );

	return rc;
}

static int Disconnect_SenHub( void *pInParam )
{
        printf("[ivan] Disconnect_SenHub ==================>\n");
	int rc = 0;
#if 0
	Handler_info *pHandler_info = (Handler_info*)pInParam;
	if( pHandler_info == NULL )  {
		PRINTF("[Disconnect_SenHub]: pOutParam is NULL\r\n");
		return rc;
	}
#endif
        Handler_info *pHandler_info = NULL;
        pHandler_info = &g_SenPluginInfo[1];
	pHandler_info->agentInfo->status = 0;
	rc = SenHubDisconnectWISECloud( pHandler_info );
	return rc;
}


static int SendInfoSpec_SenHub( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 )
{
	int rc = 0;

	Handler_info *pHandler_info = (Handler_info*)pInParam;

	if( pHandler_info == NULL ) return rc;

	EnableSenHubAutoReport( pHandler_info->agentInfo->devId, 1 );

	if( g_sendinfospeccbf ) {
		g_sendinfospeccbf( pHandler_info, pInJson, InDataLen, NULL, NULL );
		rc = 1;
	}

	return rc;
}

// pInJson: 
static int ProcSet_Result( const char *pInJson, const int InDataLen, void *pInParam, void *pRev1 )
{
	int rc = 0;	
	int len = 0;
	int index = 0;
	char buffer[MAX_BUFFER_SIZE]={0};
	Handler_info  *pSenHander = NULL;
	Handler_info  SenHubHandler;

	AsynSetParam *pAsynSetParam = (AsynSetParam*)pInParam;
	API_MUX_ASYNC_DATA *pAsynApiMux = (API_MUX_ASYNC_DATA*)pInParam;

	PRINTF("RESTful Set Result\r\n");
	if( pAsynApiMux && !memcmp( pAsynApiMux->magickey, ASYNC_MAGIC_KEY, sizeof( ASYNC_MAGIC_KEY )) ) {
		if( pAsynApiMux->nDone == 0 ) {
			snprintf(pAsynApiMux->buf, pAsynApiMux->bufsize,"%s",pInJson);
			pAsynApiMux->nDone = 1;
			PRINTF("RESTful Set success\n");
			rc = 1;
			return rc;
		} else if ( pAsynApiMux->nDone == -1 )
			FreeMemory(pAsynApiMux);
		PRINTF("RESTful Set Time Out\r\n");
		return rc;
	} 	

	if( pAsynSetParam == NULL ) return rc;

	index = pAsynSetParam->index;

	if( index ==IOTGW_INFTERFACE_INDEX ) {

		memset(buffer, 0, sizeof(buffer));

		len = ProcAsynSetSenHubValueEvent( pInJson,  InDataLen, pAsynSetParam, buffer, sizeof( buffer ) );

		if( len > 0 ) {
			g_sendcbf(&g_PluginInfo,IOTGW_SET_SENSOR_REPLY, buffer, len+1, NULL, NULL);
			rc = 1;
		}


	} else if( index < 0 || index > MAX_SENNODES)
		goto Exit_ProcSet;
	else {

		pSenHander = &g_SenPluginInfo[index];

		if( pSenHander ) {
			memset(&SenHubHandler,0,sizeof(SenHubHandler));
			memcpy(&SenHubHandler,pSenHander, sizeof(SenHubHandler));
			snprintf(SenHubHandler.Name,MAX_TOPIC_LEN, "%s", SENHUB_HANDLER_NAME); // "SenHub"
			pSenHander = &SenHubHandler;
		} else 
			goto Exit_ProcSet;
		// need to check UID
		// pHandler_info->

		memset(buffer, 0, sizeof(buffer));

		len = ProcAsynSetSenHubValueEvent( pInJson,  InDataLen, pAsynSetParam, buffer, sizeof( buffer ) );

		if( g_sendcustcbf && len > 0 ) { 
			g_sendcustcbf(pSenHander, IOTGW_SET_SENSOR_REPLY, pAsynSetParam->szTopic, buffer, len+1 , NULL, NULL);
			rc = 1;
		}
	}
Exit_ProcSet:

	if( pInParam )
		FreeMemory( pInParam );

	return rc;
}

int RegisterToRMM_GatewayCapabilityInfo(char* pCapability, int iCapabilitySize){
#if 0
char Capability[MAX_DATA_SIZE]={"{\"IoTGW\":{\"WSN\":{\"WSN0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD02\",\"ver\":1},\"WSN1\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"WSN0\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD01\",\"ver\":1}, \"bn\":\"WSN\",\"ver\":1},\"ver\":1}}"};
     g_sendinfospeccbf( &g_PluginInfo, Capability, strlen(Capability), NULL, NULL);
#else
     g_sendinfospeccbf( &g_PluginInfo, pCapability, iCapabilitySize, NULL, NULL);
#endif

    return 0;
}

int UpdateToRMM_GatewayUpdateInfo(char* data){

#if 0
char ConnectivityInfo[1024]={"{\"IoTGW\":{\"WSN\":{\"WSN0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"0017000E40000000\"},{\"n\":\"Neighbor\",\"sv\":\"0017000E40000000\"},{\"n\":\"Name\",\"sv\":\"WSN0\"},{\"n\":\"Health\",\"v\":\"100.000000\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\"},{\"n\":\"reset\",\"bv\":\"0\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD01\",\"ver\":1},\"bn\":\"WSN\"},\"ver\":1}}"};

    UpdateInterfaceData(ConnectivityInfo, strlen(ConnectivityInfo));

char ConnectivityInfo2[1024]={"{\"IoTGW\":{\"WSN\":{\"WSN0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"0017000E40000000\"},{\"n\":\"Neighbor\",\"sv\":\"0017000E40000000\"},{\"n\":\"Name\",\"sv\":\"WSN0\"},{\"n\":\"Health\",\"v\":\"100.000000\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\"},{\"n\":\"reset\",\"bv\":\"0\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD02\",\"ver\":1},\"bn\":\"WSN\"},\"ver\":1}}"};

    UpdateInterfaceData(ConnectivityInfo2, strlen(ConnectivityInfo2));

char ConnectivityInfo3[1024]={"{\"IoTGW\":{\"BLE\":{\"BLE0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"0017000E40000000\"},{\"n\":\"Neighbor\",\"sv\":\"0017000E40000000\"},{\"n\":\"Name\",\"sv\":\"WSN0\"},{\"n\":\"Health\",\"v\":\"100.000000\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\"},{\"n\":\"reset\",\"bv\":\"0\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD05\",\"ver\":1},\"bn\":\"BLE\"},\"ver\":1}}"};

    UpdateInterfaceData(ConnectivityInfo3, strlen(ConnectivityInfo3));

char ConnectivityInfo4[1024]={"{\"IoTGW\":{\"BLE\":{\"BLE0\":{\"Info\":{\"e\":[{\"n\":\"SenHubList\",\"sv\":\"0017000E40000000\"},{\"n\":\"Neighbor\",\"sv\":\"0017000E40000000\"},{\"n\":\"Name\",\"sv\":\"WSN0\"},{\"n\":\"Health\",\"v\":\"100.000000\"},{\"n\":\"sw\",\"sv\":\"1.2.1.12\"},{\"n\":\"reset\",\"bv\":\"0\"}],\"bn\":\"Info\"},\"bn\":\"0007000E40ABCD06\",\"ver\":1},\"bn\":\"BLE\"},\"ver\":1}}"};

    UpdateInterfaceData(ConnectivityInfo4, strlen(ConnectivityInfo4));
    return 0;

#else
    //printf(data);
    return UpdateInterfaceData(data, strlen(data));
#endif

}


int ConnectToRMM_SensorHub(JSONode *json)
{
	int rc = -1;
	int index = 0;
        senhub_info_t shinfo;
	senhub_info_t *pSenHubInfo = NULL;
	Handler_info  *pSenHander = NULL;

        memset(&shinfo,0,sizeof(senhub_info_t));
        if ( PrepareInfoToRegisterSensorHub(json,&shinfo) < 0){
            return -1;
        }

	pSenHubInfo = &shinfo;

	if( pSenHubInfo == NULL ) {
		ADV_C_ERROR(COLOR_RED,"[%s][%s] SensorHub is NULL\r\n", __FILE__, __func__);
		return rc;
	}
;
	// 1. Find empty SenHub Array
	if(  (index = GetUsableIndex(&g_SenHubAgentInfo, MAX_SENNODES ) )  == -1 ) 
	{
		ADV_C_ERROR(COLOR_RED,"GW Handler is Full \r\n");
		return rc;
	}
        
	pSenHander = &g_SenPluginInfo[index];

	// 2. Prepare Sensor Node Handler_info data
        PackSensorHubPlugInfo(pSenHander, &g_PluginInfo, pSenHubInfo);

	// 3. Register to WISECloud
	rc = SenHubConnectToWISECloud( pSenHander );

	return 0;
}

int RegisterToRMM_SensorHubCapability(char* ptopic, JSONode *json){

    int index=-1;
    char nodeContent[MAX_JSON_NODE_SIZE]={0};
    char infosepc_data[MAX_JSON_NODE_SIZE]={0};
    char sensorHubUID[64]={0};
    Handler_info  *pSenHander = NULL;

    if ( GetUIDfromTopic(ptopic, sensorHubUID, sizeof(sensorHubUID)) < 0){
        printf("[%s][%s] Can't find SenHubID Topic=%s\r\n",__FILE__, __func__, ptopic );
	return -1;
    }

    if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, sensorHubUID) ) == -1 ) {
        printf("[%s][%s] Can't find SenHub UID in Table =%s\r\n",__FILE__, __func__, sensorHubUID );
	return -1;
    }

#if 0
//SensorHub capability sample:
        char mydata[2024]={"{\"infoSpec\":{\"SenHub\":{\"SenData\":{\"e\":[{\"n\":\"Temperature\",\"u\":\"Cel\",\"v\":0.000000,\"min\":-100.000000,\"max\":200.000000,\"asm\":\"r\",\"type\":\"d\",\"rt\":\"ucum.Cel\",\"st\":\"ipso\",\"exten\":\"\"},{\"n\":\"Humidity\",\"u\":\"%\",\"v\":0.000000,\"min\":0.000000,\"max\":100.000000,\"asm\":\"r\",\"type\":\"d\",\"rt\":\"ucum.%\",\"st\":\"ipso\",\"exten\":\"\"},{\"n\":\"GPIO1\",\"u\":\"\",\"bv\":0,\"min\":0.000000,\"max\":1.000000,\"asm\":\"rw\",\"type\":\"b\",\"rt\":\"\",\"st\":\"ipso\",\"exten\":\"\"},{\"n\":\"GPIO2\",\"u\":\"\",\"bv\":0,\"min\":0.000000,\"max\":1.000000,\"asm\":\"rw\",\"type\":\"b\",\"rt\":\"\",\"st\":\"ipso\",\"exten\":\"\"}],\"bn\":\"SenData\"},\"Info\":{\"e\":[{\"n\":\"Name\",\"sv\":\"AAA\",\"asm\":\"rw\"},{\"n\":\"sw\",\"sv\":\"1.0.00\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"Net\":{\"e\":[{\"n\":\"sw\",\"sv\":\"1.0.00\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"}],\"bn\":\"Net\"},\"Action\":{\"e\":[{\"n\":\"AutoReport\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Action\"},\"ver\":1}}}"};
#endif

    PRINTF("sensorHubUID=%s, index=%d\n", sensorHubUID, index);
    pSenHander = &g_SenPluginInfo[index];

    //Get SensorHub capability
    memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
    JSON_Get(json, OBJ_INFO_SPEC, nodeContent, sizeof(nodeContent));
    if(strcmp(nodeContent, "NULL") == 0){
        printf("[%s][%s] Can't find SensorHub capability\r\n",__FILE__, __func__);
        return -1;
    }
    PRINTF("%s: info spec=%s\n", __func__, nodeContent);
    //Prepare data to RMM
    sprintf(infosepc_data, "{\"infoSpec\":%s}",nodeContent);
			
    Handler_info tmpHandler;
    memcpy(&tmpHandler, pSenHander, sizeof(Handler_info));
    strcpy(tmpHandler.Name, "general");
    tmpHandler.RequestID=1001;
    tmpHandler.ActionID=2001;

    g_sendcustcbf(&tmpHandler,IOTGW_HANDLER_GET_CAPABILITY_REPLY, ptopic, infosepc_data, strlen(infosepc_data) , NULL, NULL);

    return 0;
}

int DisconnectToRMM_SensorHub(char* SensorHubUID)
{
        printf("[%s][%s] DisconnectSensorHub(%s)==================>\n", __FILE__, __func__, SensorHubUID);
	int rc = 0;
        int index=-1;
        //
	if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, SensorHubUID) ) == -1 ) {
		PRINTF("[%s][%s] Can't find SenHub UID in Table =%s\r\n",__FILE__, __func__, SensorHubUID );
		return -1;
	}
        //
        Handler_info *pHandler_info = NULL;
        pHandler_info = &g_SenPluginInfo[index];
	pHandler_info->agentInfo->status = 0;
	rc = SenHubDisconnectWISECloud( pHandler_info );
	return rc;
}

int UpdateToRMM_SensorHubData( JSONode *json )
{
	int rc = 0;
        int index = -1;
        char nodeContent[MAX_JSON_NODE_SIZE]={0};
        char devID[MAX_DEVICE_ID_LEN]={0};

        //Get SensorHub ID
        memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
        JSON_Get(json, OBJ_AGENT_ID, nodeContent, sizeof(nodeContent));
        if(strcmp(nodeContent, "NULL") == 0){
            return -1;
        }
        strcpy(devID,nodeContent);
        PRINTF("%s: SensorHub devID=%s\n", __func__, nodeContent);

        //Get SensorHub Data
        memset(nodeContent, 0, MAX_JSON_NODE_SIZE);
        JSON_Get(json, OBJ_DATA, nodeContent, sizeof(nodeContent));
        if(strcmp(nodeContent, "NULL") == 0){
            return -1;
        }

	if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, 256, devID/*SenHubUID*/) ) == -1 ) {
		printf(" Can't find SenHub UID in Table =%s\r\n", devID );
		return -1;
	}

        PRINTF("find SenHub UID in Table =%s, index=%d\r\n",devID, index );
        PRINTF("Sensor Hub data=%s\n", nodeContent);

#if 0
	//Handler_info *pHandler_info = (Handler_info*)pInParam;
        char mydata[1024]={"{\"SenHub\":{\"SenData\":{\"e\":[{\"n\":\"Temperature\",\"v\":8.000000},{\"n\":\"Humidity\",\"v\":64.000000},{\"n\":\"GPIO1\",\"bv\":0},{\"n\":\"GPIO2\",\"bv\":0}],\"bn\":\"SenData\"},\"Info\":{\"e\":[{\"n\":\"Name\",\"sv\":\"123456789012345678901234567890\",\"asm\":\"rw\"},{\"n\":\"sw\",\"sv\":\"1.0.00\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Info\"},\"Net\":{\"e\":[{\"n\":\"sw\",\"sv\":\"1.0.00\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":\"100.000000\",\"asm\":\"r\"}],\"bn\":\"Net\"},\"Action\":{\"e\":[{\"n\":\"AutoReport\",\"bv\":\"0\",\"asm\":\"rw\"}],\"bn\":\"Action\"},\"ver\":1}}"};

#endif
        Handler_info *pHandler_info = (Handler_info*)&g_SenPluginInfo[index];

	if( pHandler_info == NULL ) return -1;

        printf(nodeContent);
	if( g_sendreportcbf ) {
		g_sendreportcbf( pHandler_info, nodeContent, strlen(nodeContent), NULL, NULL );
		rc = 1;
	}

	return 0;
}

int ResponseGetData(char* topic, int cmdID, char* sensor_hub_id, char* presponse_data, int response_data_size){

    int index=-1;
    if( ( index = GetSenHubAgentInfobyUID(&g_SenHubAgentInfo, MAX_SENNODES, sensor_hub_id) ) == -1 ) {
        printf("[%s][%s] Can't find SenHub UID in Table =%s\r\n",__FILE__, __func__, sensor_hub_id );
	return -1;
    }
    PRINTF("[ivan][%s][%s] g_SenPluginInfo[index] index = %d\n", __FILE__, __func__, index);
    Handler_info  *pSenHander = NULL;
    pSenHander = &g_SenPluginInfo[index];

#if 0
    char mydata[1024]={"{\"sessionID\":\"49D7B6965BE8C1B4FFABC32391CE3169\", \"sensorInfoList\":{\"e\":[{\"n\":\"/Info/sw\", \"sv\":\"1.0.00\",\"StatusCode\": 200 }]} }"};
    g_sendcustcbf(pSenHander,IOTGW_GET_SENSOR_REPLY, "/cagent/admin/0017000E40000000/agentactionreq", mydata, strlen(mydata) , NULL, NULL);
#else
    g_sendcustcbf(pSenHander,cmdID,topic, presponse_data, response_data_size , NULL, NULL);
#endif 

    return 0;
}

int ReplyToRMM_GatewayGetSetRequest(char* ptopic, JSONode *json, int cmdID){

    char nodeContent[MAX_JSON_NODE_SIZE];
    char sessionID[256]={0};
    char sensorInfoList[256]={0};
    char respone_data[1024]={0};

    //message topic
    printf("topic = %s\n", ptopic);

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

    g_sendcbf(&g_PluginInfo,cmdID, respone_data, strlen(respone_data), NULL, NULL);

    return 0;
}


// <IoTGW>
int InitSNGWHandler()
{	
	int i = 0;
        int rc = 0;
	// Allocate Handler_info array buffer point
	for( i = 0; i< MAX_SENNODES ; i ++ ) {
		memcpy(&g_SenPluginInfo[i],&g_PluginInfo,sizeof(Handler_info));
		memcpy(&g_SenHubAgentInfo[i],&g_PluginInfo.agentInfo,sizeof(cagent_agent_info_body_t));
		g_SenHubAgentInfo[i].status = 0;
		g_SenPluginInfo[i].agentInfo = &g_SenHubAgentInfo[i];
	}

	//Init AdvApiMux Server
#ifdef LINUX
	InitAdvAPIMux_Server();
#endif
	return rc;
}

int StartSNGWHandler()
{
	int rc = 0;

	return rc;
}

int StopSNGWHandler()
{
	int rc = 0;

	while( runing )
		TaskSleep(100);

	return rc;
}

void UnInitSNGWHandler()
{
	// 1. Send Disconnect Msg
	//if(pSNManagerAPI)
		//pSNManagerAPI->SN_Manager_Uninitialize( );
#ifdef LINUX
	UnInitAdvAPIMux_Server();
#endif
}


//-----------------------------------------------------------------------------
// UTIL Function:
//-----------------------------------------------------------------------------
#ifdef _MSC_VER
BOOL WINAPI DllMain( HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved )
{
	if ( reason_for_call == DLL_PROCESS_ATTACH ) // Self-explanatory
	{
		printf( "DllInitializer\n" );
		DisableThreadLibraryCalls( module_handle ); // Disable DllMain calls for DLL_THREAD_*
		if ( reserved == NULL ) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if ( reason_for_call == DLL_PROCESS_DETACH ) // Self-explanatory
	{
		printf( "DllFinalizer\n" );
		if ( reserved == NULL ) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize( );
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize( );
		}
	}
	return TRUE;
}
#else
__attribute__( ( constructor ) )
/**
 * initializer of the shared lib.
 */
static void Initializer( int argc, char** argv, char** envp )
{
    printf( "DllInitializer\n" );
}

__attribute__( ( destructor ) )
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer( )
{
    printf( "DllFinalizer\n" );
	Handler_Uninitialize( );
}
#endif






