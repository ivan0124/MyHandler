/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2014/06/18 by Eric Liang															     */
/* Modified Date: 2014/06/27 by Eric Liang															 */
/* Abstract     : Handler API                                     													*/
/* Reference    : None																									 */
/****************************************************************************/
//#include <stdio.h>
//#include <dlfcn.h>
//#include <unistd.h>
//#include <string.h>
//#include <malloc.h>
#include "platform.h"
#include <cJSON.h>
#include "susiaccess_handler_api.h"
#include "common.h"
#include "DeviceMessageGenerate.h"
#include "SAClient.h"
#include "IoTMessageGenerate.h"
#include "Log.h"
/*ivan test*/
#include "SensorNetwork_API.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define cagent_request_custom 2002
#define cagent_custom_action 30002
const char strPluginName[MAX_TOPIC_LEN] = {"MyHandler"};
const int iRequestID = cagent_request_custom;
const int iActionID = cagent_custom_action;

MSG_CLASSIFY_T *g_Capability = NULL;
//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------
//
typedef struct{
   void* threadHandler;
   bool isThreadRunning;
}handler_context_t;

//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static Handler_info  g_PluginInfo;
static handler_context_t g_HandlerContex;
static HANDLER_THREAD_STATUS g_status = handler_status_no_init;
static bool m_bAutoReprot = false;
static time_t g_monitortime;
static HandlerSendCbf  g_sendcbf = NULL;						// Client Send information (in JSON format) to Cloud Server	
static HandlerSendCustCbf  g_sendcustcbf = NULL;			    // Client Send information (in JSON format) to Cloud Server with custom topic	
static HandlerSubscribeCustCbf g_subscribecustcbf = NULL;
static HandlerAutoReportCbf g_sendreportcbf = NULL;				// Client Send report (in JSON format) to Cloud Server with AutoReport topic
static HandlerSendCapabilityCbf g_sendcapabilitycbf = NULL;		
static HandlerSendEventCbf g_sendeventcbf = NULL;
//-----------------------------------------------------------------------------
//ivan test
//-----------------------------------------------------------------------------
#define LIB_NAME		"./libMqttDrv.so"
#define SW_VERSION_LEN	16
#define MAX_SENDATA_LEN	256

LOGHANDLE SUSIAccessAgentLogHandle;
#ifdef SUSIACCESSAGENT_LOG_ENABLE
#define SUSIAccessAgentLog(level, fmt, ...)  do { if (SUSIAccessAgentLogHandle != NULL)   \
	WriteLog(SUSIAccessAgentLogHandle, DEF_SUSIACCESSAGENT_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define SUSIAccessAgentLog(level, fmt, ...)
#endif

SN_Initialize_API g_pFunc_SNInit;
SN_Uninitialize_API g_pFunc_SNUninit;
SN_GetVersion_API g_pFunc_SNGetSWVersion;
SN_SetUpdateDataCbf_API g_pFunc_SNSetUpdateCbf;
SN_GetCapability_API g_pFunc_SNGetCapability;
SN_GetData_API g_pFunc_SNGetData;
SN_SetData_API g_pFunc_SNSetData;
//-----------------------------------------------------------------------------
// Function:
//-----------------------------------------------------------------------------
void Handler_Uninitialize();

#ifdef _MSC_VER
BOOL WINAPI DllMain(HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved)
{
	if (reason_for_call == DLL_PROCESS_ATTACH) // Self-explanatory
	{
		printf("DllInitializer\r\n");
		DisableThreadLibraryCalls(module_handle); // Disable DllMain calls for DLL_THREAD_*
		if (reserved == NULL) // Dynamic load
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

	if (reason_for_call == DLL_PROCESS_DETACH) // Self-explanatory
	{
		printf("DllFinalizer\r\n");
		if (reserved == NULL) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			Handler_Uninitialize();
		}
		else // Process is terminating
		{
			// Cleanup
			Handler_Uninitialize();
		}
	}
	return TRUE;
}
#else
__attribute__((constructor))
/**
 * initializer of the shared lib.
 */
static void Initializer(int argc, char** argv, char** envp)
{
    printf("DllInitializer\r\n");
}

__attribute__((destructor))
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer()
{
    printf("DllFinalizer\r\n");
	Handler_Uninitialize();
}
#endif

MSG_CLASSIFY_T * CreateCapability()
{
#if 0
	MSG_CLASSIFY_T* myCapability = IoT_CreateRoot((char*) strPluginName);
	MSG_CLASSIFY_T*myGroup = IoT_AddGroup(myCapability, "TEST");
	MSG_ATTRIBUTE_T* attr1 = NULL;
	MSG_ATTRIBUTE_T* attr = IoT_AddGroupAttribute(myGroup, "bu"); /*Add base unit attribute*/
	if(attr)
		IoT_SetStringValue(attr, "%", IoT_NODEFINE);

	attr1 = IoT_AddSensorNode(myGroup, "test");
	if(attr1)
		IoT_SetFloatValueWithMaxMin(attr1, 10, IoT_READONLY, 100, 10, "%");
#endif
        	
	MSG_CLASSIFY_T* myCapability = NULL;
	return myCapability;
}

void on_msgrecv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2)
{
	printf("Packet received, %s\r\n", data);
}

void TestCustMsg()
{
#if 0
	char topicStr[128] = {0};
	/* Add  subscribe topic Callback*/
	sprintf(topicStr, "/cagent/admin/%s/testreq", g_PluginInfo.agentInfo->devId);
	g_subscribecustcbf(topicStr, on_msgrecv);

	if(g_Capability)
	{
		char* message = IoT_PrintData(g_Capability);

		if(g_sendcustcbf)
			g_sendcustcbf(&g_PluginInfo, 123, topicStr, message, strlen(message), NULL, NULL);
		free(message);
	}
#endif
}

static CAGENT_PTHREAD_ENTRY(SampleHandlerThreadStart, args)
{
#if 0
	char *cPayload = NULL;
	MSG_CLASSIFY_T *myEvent = NULL;

	handler_context_t *pHandlerContex = (handler_context_t *)args;

	while (g_PluginInfo.agentInfo->status == 0)
	{
		if(!pHandlerContex->isThreadRunning)
			return 0;
		app_os_sleep(1000);
	}

	/*Send notify*/
	{
		myEvent = DEV_CreateEventNotify("message", "test message for event notify!!");
		cPayload = DEV_PrintUnformatted(myEvent);
		DEV_ReleaseDevice(myEvent);

		if(g_sendeventcbf)
			g_sendeventcbf( &g_PluginInfo, Severity_Informational, cPayload, strlen(cPayload), NULL, NULL);
		free(cPayload);
	}
		
	/* Update InfoSpec test*/
	if(!g_Capability)
		g_Capability =  CreateCapability();
		 
	{
		cPayload = IoT_PrintCapability(g_Capability);
		
		if(g_sendcapabilitycbf)
			g_sendcapabilitycbf(&g_PluginInfo, cPayload, strlen(cPayload), NULL, NULL);
		free(cPayload);
	}
	
	TestCustMsg();

	while(pHandlerContex->isThreadRunning)
	{
		app_os_sleep(5000);


		if(m_bAutoReprot)
		{
			/* Send AutoReport test*/
			if(g_Capability)
			{
				cPayload = IoT_PrintData(g_Capability);
				if(g_sendreportcbf)
					g_sendreportcbf(&g_PluginInfo, cPayload, strlen(cPayload), NULL, NULL);
				free(cPayload);
			}			
		}
		/*update timer for status check*/
		time(&g_monitortime);
	}
#endif	
   return 0;
}

/****************************************************************************************
 *[ivan test]
 *Function Name: Start
 ****************************************************************************************/
UpdateSNDataCbf printSNcallbackData(const int cmdId, const void *pInData, const int InDatalen, void *pUserData,	void *pOutParam, void *pRev1, void *pRev2)
{
    return 0;
}

int printSNInfInfos(SNInfInfos *pSNInfos)
{
	int i;
	printf("# Main - Got SNInfInfos: #\n\t sComType=%s\n\t iNum=%d\n", pSNInfos->sComType, pSNInfos->iNum);
	for (i=0; i<pSNInfos->iNum; i++) {
		printf("\t SNInfs[%d].sInfName=%s\n", i, pSNInfos->SNInfs[i].sInfName);
		printf("\t SNInfs[%d].sInfID=%s\n", i, pSNInfos->SNInfs[i].sInfID);
	}
	printf("##########\n");
	
	return 0;
}

int Start()
{
	void *pLibHandle = NULL;	
	char *pSWVersion = NULL;
	SNInfInfos outSNInfInfo;
        
        printf("[MyHandler]My MQTT test Start  =================================>\n");
	pLibHandle = dlopen(LIB_NAME, RTLD_LAZY);
	if (NULL == pLibHandle) {
		printf("Error: Failed to load %s\n", LIB_NAME);
		return -1;
	}
	g_pFunc_SNInit			= (SN_Initialize_API) dlsym(pLibHandle, "SN_Initialize");
	g_pFunc_SNUninit		= (SN_Uninitialize_API) dlsym(pLibHandle, "SN_Uninitialize");
	g_pFunc_SNGetCapability	= (SN_GetCapability_API) dlsym(pLibHandle, "SN_GetCapability");
	g_pFunc_SNSetUpdateCbf	= (SN_SetUpdateDataCbf_API) dlsym(pLibHandle, "SN_SetUpdateDataCbf");
	g_pFunc_SNGetSWVersion	= (SN_GetVersion_API) dlsym(pLibHandle, "SN_GetVersion");
	g_pFunc_SNGetData	= (SN_GetData_API) dlsym(pLibHandle, "SN_GetData");
	g_pFunc_SNSetData	= (SN_SetData_API) dlsym(pLibHandle, "SN_SetData");

	if (NULL==g_pFunc_SNInit || NULL==g_pFunc_SNUninit || 
		NULL==g_pFunc_SNGetCapability || NULL==g_pFunc_SNSetUpdateCbf ||
		NULL==g_pFunc_SNGetSWVersion || NULL== g_pFunc_SNGetData || NULL==g_pFunc_SNSetData) {
		printf("Error: Function load error from %s\n", LIB_NAME);
		return -1;
	}

	if (g_pFunc_SNInit(NULL) == -1) {
		printf("Error: Lib init\n");
		return -1;
	}

	pSWVersion = malloc(sizeof(char)*SW_VERSION_LEN);
	if (g_pFunc_SNGetSWVersion(pSWVersion, sizeof(char)*SW_VERSION_LEN) == -1) {
		printf("Error: Lib get sw version\n");
		return -1;
	}

	printf("%s: SW Version=%s\n", __func__, pSWVersion);

	if (g_pFunc_SNSetUpdateCbf((UpdateSNDataCbf) printSNcallbackData) == -1) {
		printf("Error: Lib set CBF\n");
		return -1;
	}

	//sleep(1);
	outSNInfInfo.iNum = 2;
	outSNInfInfo.SNInfs[0].outDataClass.iTypeCount = 1;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData));
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeType = 256;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psType = (char *)malloc(256);
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData = (int *)malloc(sizeof(int));
	*(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData) = 256;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psData = (char *)malloc(256);
	outSNInfInfo.SNInfs[1].outDataClass.iTypeCount = 1;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData));
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeType = 256;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psType = (char *)malloc(256);
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData = (int *)malloc(sizeof(int));
	*(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData) = 256;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psData = (char *)malloc(256);

	if (g_pFunc_SNGetCapability(&outSNInfInfo) == -1) {
		printf("Error: Lib get capability\n");
	} else {
		printSNInfInfos(&outSNInfInfo);
		usleep(1000);
	}

	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psType);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psData);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray);
	
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psType);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psData);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray);

	if(pSWVersion != NULL)
		free(pSWVersion);

        printf("ivan test: Start() exit <========================\n");
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

        printf("[MyHandler]: Handler_Initialize sssssssssssssssssssssss]\n");
	if( pluginfo == NULL )
		return handler_fail;

        // 0.[ivan test] initalize mqtt function
        Start();
        //iRet = saclient_initialize(&config, &profile, SUSIAccessAgentLogHandle);
         
	// 1. Topic of this handler
	snprintf( pluginfo->Name, sizeof(pluginfo->Name), "%s", strPluginName );
	pluginfo->RequestID = iRequestID;
	pluginfo->ActionID = iActionID;
	printf(" >Name: %s\r\n", strPluginName);
	// 2. Copy agent info 
	memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	g_PluginInfo.agentInfo = pluginfo->agentInfo;

	// 3. Callback function -> Send JSON Data by this callback function
	g_sendcbf = g_PluginInfo.sendcbf = pluginfo->sendcbf;
	g_sendcustcbf = g_PluginInfo.sendcustcbf = pluginfo->sendcustcbf;
	g_subscribecustcbf = g_PluginInfo.subscribecustcbf = pluginfo->subscribecustcbf;
	g_sendreportcbf = g_PluginInfo.sendreportcbf = pluginfo->sendreportcbf;
	g_sendcapabilitycbf =g_PluginInfo.sendcapabilitycbf = pluginfo->sendcapabilitycbf;
	g_sendeventcbf = g_PluginInfo.sendeventcbf = pluginfo->sendeventcbf;

	g_HandlerContex.threadHandler = NULL;
	g_HandlerContex.isThreadRunning = false;
	g_status = handler_status_init;

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
	g_sendcapabilitycbf = NULL;
	g_subscribecustcbf = NULL;
#if 0
	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}
#endif
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
	int iRet = handler_fail; 

	if(!pOutStatus) return iRet;

	switch (g_status)
	{
	default:
	case handler_status_no_init:
	case handler_status_init:
	case handler_status_stop:
		*pOutStatus = g_status;
		break;
	case handler_status_start:
	case handler_status_busy:
		{
			time_t tv;
			time(&tv);
			if(difftime(tv, g_monitortime)>5)
				g_status = handler_status_busy;
			else
				g_status = handler_status_start;
			*pOutStatus = g_status;
		}
		break;
	}
	
	iRet = handler_success;
	return iRet;
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
	printf(" %s> Update Status", strPluginName);
	if(pluginfo)
		memcpy(&g_PluginInfo, pluginfo, sizeof(HANDLER_INFO));
	else
	{
		memset(&g_PluginInfo, 0, sizeof(HANDLER_INFO));
		snprintf( g_PluginInfo.Name, sizeof( g_PluginInfo.Name), "%s", strPluginName );
		g_PluginInfo.RequestID = iRequestID;
		g_PluginInfo.ActionID = iActionID;
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
	
	if (app_os_thread_create(&g_HandlerContex.threadHandler, SampleHandlerThreadStart, &g_HandlerContex) != 0)
	{
		g_HandlerContex.isThreadRunning = false;
		printf("> start handler thread failed!\r\n");	
		return handler_fail;
    }
	g_HandlerContex.isThreadRunning = true;
	g_status = handler_status_start;
	time(&g_monitortime);
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
		app_os_thread_join(g_HandlerContex.threadHandler);
		g_HandlerContex.threadHandler = NULL;
	}
	g_status = handler_status_stop;
	return handler_success;
}

/* **************************************************************************************
 *  Function Name: Handler_Recv
 *  Description: Receive Packet from MQTT Server
 *  Input : char * const topic, 
 *			void* const data, 
 *			const size_t datalen
 *  Output: void *pRev1, 
 *			void* pRev2
 *  Return: None
 * ***************************************************************************************/
void HANDLER_API Handler_Recv(char * const topic, void* const data, const size_t datalen, void *pRev1, void* pRev2  )
{
	printf(" >Recv Topic [%s] Data %s", topic, (char*) data );
}

/* **************************************************************************************
 *  Function Name: Handler_AutoReportStart
 *  Description: Start Auto Report
 *  Input : char *pInQuery
 *  Output: None
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
 *  Output: char ** : pOutReply       // JSON Format
 *  Return:  int  : Length of the status information in JSON format
 *                :  0 : no data need to trans
 * **************************************************************************************/
int HANDLER_API Handler_Get_Capability( char ** pOutReply ) // JSON Format
{

//	char* result = NULL;
	int len = 0;
#if 0
	if(!pOutReply) return len;

	if(g_Capability)
	{
		IoT_ReleaseAll(g_Capability);
		g_Capability = NULL;
	}

	g_Capability = CreateCapability();

	result = IoT_PrintCapability(g_Capability);

	len = strlen(result);
	*pOutReply = (char *)malloc(len + 1);
	memset(*pOutReply, 0, len + 1);
	strcpy(*pOutReply, result);
	free(result);
#endif
	return len;
}
