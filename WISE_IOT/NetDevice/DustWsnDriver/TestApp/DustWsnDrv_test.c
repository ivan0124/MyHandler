#include <stdio.h>
#include <signal.h>
#include "DustWsnDrv_test.h"
#include "AdvLog.h"

#define DUSTWSNLIB_NAME		"libDustWsnDrv.so"
#define SW_VERSION_LEN	16
#define MAX_SENDATA_LEN	256

SN_Initialize_API g_pFunc_SNInit;
SN_Uninitialize_API g_pFunc_SNUninit;
SN_GetVersion_API g_pFunc_SNGetSWVersion;
SN_SetUpdateDataCbf_API g_pFunc_SNSetUpdateCbf;
SN_GetCapability_API g_pFunc_SNGetCapability;
SN_GetData_API g_pFunc_SNGetData;
SN_SetData_API g_pFunc_SNSetData;

CAGENT_COND_TYPE  srvcStopEvent;
CAGENT_MUTEX_TYPE srvcStopMux;
BOOL isDrvRun = FALSE;

#define MANAGER_MAC "00170d00006035dd"
#define MOTE_MAC    "00170d0000582a4f"
//#define MOTE_MAC    "00170d0000606427"

#define HelpPrintf() printf("\n"  \
   "-------------------------------------------------------------\n" \
   "|                    Test Functions                         |\n" \
   "-------------------------------------------------------------\n" \
   "-h    show help menu \n" \
   "-r    Start WSNDRV service \n" \
   "-q    Stop WSNDRV service and Quit \n" \
   ">> ")

static int ExecuteCmd(char* cmd)
{
	int iRet = 0;
	if (NULL == cmd) {
		printf(">> ");
	} else if (strcmp(cmd, "-h") == 0) {
		HelpPrintf();
	} else if (strcmp(cmd, "-r") == 0) {
		if (!isDrvRun) {
			isDrvRun = WsnDrv_Start() == 0;
		}
		printf(">> ");
	} else if (strcmp(cmd, "-q") == 0) {
		if (isDrvRun) {
			WsnDrv_Stop();
		}
		iRet = -1;
	} else {
		printf(">> ");
	}
	
	return iRet;
}

static void sig_hdl(int sig)
{ 
	int iRet = 0;
	app_os_mutex_lock(&srvcStopMux);
	if(isDrvRun)
		WsnDrv_Stop();
	isDrvRun = FALSE;
	printf("Sending stop signal!");
	app_os_cond_signal(&srvcStopEvent);
	app_os_mutex_unlock(&srvcStopMux);
}

int TestGetAPI()
{
	InSenData *pInSenData;
	OutSenData *pOutSenData;

	printf("################## Main - %s ###################\n", __func__);
	// Test data
	pInSenData = (InSenData *)malloc(sizeof(InSenData));
	sprintf(pInSenData->sUID, MANAGER_MAC);
	pInSenData->inDataClass.iTypeCount = 1;
	pInSenData->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pInSenData->inDataClass.iTypeCount);
	pInSenData->inDataClass.pInBaseDataArray[0].psType = strdup("Info");
	pInSenData->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pInSenData->inDataClass.pInBaseDataArray[0].psType);
	pInSenData->inDataClass.pInBaseDataArray[0].psData = malloc(MAX_SENDATA_LEN);
	if (NULL == pInSenData->inDataClass.pInBaseDataArray[0].psData)
		return -1;
	memset(pInSenData->inDataClass.pInBaseDataArray[0].psData, 0, MAX_SENDATA_LEN);
	snprintf(pInSenData->inDataClass.pInBaseDataArray[0].psData, MAX_SENDATA_LEN,	"{\"n\":\"Health\"}");
	pInSenData->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pInSenData->inDataClass.pInBaseDataArray[0].psData);
	//
	pOutSenData = (OutSenData *)malloc(sizeof(OutSenData));
	pOutSenData->outDataClass.iTypeCount = 1;
	pOutSenData->outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData) * pOutSenData->outDataClass.iTypeCount);
	pOutSenData->outDataClass.pOutBaseDataArray[0].psType = strdup("Info");
	pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeType = sizeof(pOutSenData->outDataClass.pOutBaseDataArray[0].psType);

	pOutSenData->outDataClass.pOutBaseDataArray[0].psData = malloc(32);
	if (NULL == pOutSenData->outDataClass.pOutBaseDataArray[0].psData)
		return ;
	memset(pOutSenData->outDataClass.pOutBaseDataArray[0].psData, 0, 32);
	pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeData = malloc(sizeof(int));
	*pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeData = 32;
	
	if (g_pFunc_SNGetData(SN_Inf_Get, pInSenData, pOutSenData) == -1) {
		printf("Error: Lib set data\n");
	}

	memset(pInSenData->sUID, 0, sizeof(pInSenData->sUID));
	sprintf(pInSenData->sUID, MOTE_MAC);
	memset(pInSenData->inDataClass.pInBaseDataArray[0].psData, 0, MAX_SENDATA_LEN);
	snprintf(pInSenData->inDataClass.pInBaseDataArray[0].psData, MAX_SENDATA_LEN,	"{\"n\":\"Name\"}");
	if (g_pFunc_SNGetData(SN_SenHub_Get, pInSenData, pOutSenData) == -1) {
		printf("Error: Lib set data\n");
	}
	printf("###################################################\n", __func__);
}

int TestSetAPI()
{
	InSenData *pInSenData;
	OutSenData *pOutSenData;

	printf("################## Main - %s ###################\n", __func__);
	// Test data
	pInSenData = (InSenData *)malloc(sizeof(InSenData));
	sprintf(pInSenData->sUID, MOTE_MAC);
	pInSenData->inDataClass.iTypeCount = 1;
	pInSenData->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pInSenData->inDataClass.iTypeCount);
	pInSenData->inDataClass.pInBaseDataArray[0].psType = strdup("Info");
	pInSenData->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pInSenData->inDataClass.pInBaseDataArray[0].psType);
	pInSenData->inDataClass.pInBaseDataArray[0].psData = malloc(MAX_SENDATA_LEN);
	if (NULL == pInSenData->inDataClass.pInBaseDataArray[0].psData)
		return -1;
	memset(pInSenData->inDataClass.pInBaseDataArray[0].psData, 0, MAX_SENDATA_LEN);
	snprintf(pInSenData->inDataClass.pInBaseDataArray[0].psData, MAX_SENDATA_LEN,	"{\"n\":\"reset\",\"bv\":1}");
	pInSenData->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pInSenData->inDataClass.pInBaseDataArray[0].psData);
	//
	pOutSenData = (OutSenData *)malloc(sizeof(OutSenData));
	pOutSenData->outDataClass.iTypeCount = 1;
	pOutSenData->outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData) * pOutSenData->outDataClass.iTypeCount);
	pOutSenData->outDataClass.pOutBaseDataArray[0].psType = strdup("Info");
	pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeType = sizeof(pOutSenData->outDataClass.pOutBaseDataArray[0].psType);

	pOutSenData->outDataClass.pOutBaseDataArray[0].psData = malloc(32);
	if (NULL == pOutSenData->outDataClass.pOutBaseDataArray[0].psData)
		return ;
	memset(pOutSenData->outDataClass.pOutBaseDataArray[0].psData, 0, 32);
	pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeData = malloc(sizeof(int));
	*pOutSenData->outDataClass.pOutBaseDataArray[0].iSizeData = 32;
	
#if 1
	if (g_pFunc_SNSetData(SN_SenHub_Set, pInSenData, pOutSenData) == -1) {
		printf("Error: Lib set data\n");
	}
#endif

#if 0
	memset(pInSenData->sUID, 0, sizeof(pInSenData->sUID));
	sprintf(pInSenData->sUID, MOTE_MAC);
	memset(pInSenData->inDataClass.pInBaseDataArray[0].psData, 0, MAX_SENDATA_LEN);
	snprintf(pInSenData->inDataClass.pInBaseDataArray[0].psData, MAX_SENDATA_LEN,	"{\"n\":\"Name\",\"sv\":\"ROOM1\"}");
	if (g_pFunc_SNSetData(SN_SenHub_Set, pInSenData, pOutSenData) == -1) {
		printf("Error: Lib set data\n");
	}
#endif
	printf("###################################################\n", __func__);
}

UpdateSNDataCbf printSNcallbackData(const int cmdId, const void *pInData, const int InDatalen, void *pUserData,	void *pOutParam, void *pRev1, void *pRev2)
{
	//char *pMsgBuf;

	printf("\n############ Main - Receive Callback Data: ###########\n");
	printf("\t CmdId=%d\n", cmdId);
	//printf("\t Data=%s\n", pMsgBuf);
	switch (cmdId) {
		case SN_Inf_UpdateInterface_Data: {
			SNInterfaceData *pSNInfData = (SNInterfaceData *) pInData;
			printf("\t sComType=%s, sInfID=%s\n", pSNInfData->sComType, pSNInfData->sInfID);
#if 0
			if (pSNInfData->psTopology!=NULL && pSNInfData->iSizeTopology>0) {
				printf("\t iSizeTopology=%d\n", pSNInfData->iSizeTopology);
				printf("\t psTopology=%s\n", pSNInfData->psTopology);
			}
			if (pSNInfData->psSenNodeList!=NULL && pSNInfData->iSizeSenNodeList>0) {
				printf("\t iSizeSenNodeList=%d\n", pSNInfData->iSizeSenNodeList);
				printf("\t psSenNodeList=%s\n", pSNInfData->psSenNodeList);
			}
			printf("\t iHealth=%d\n", pSNInfData->iHealth);
#endif
			break;
		}
		case SN_SenHub_Register: {
			SenHubInfo *pSenHubInfo = (SenHubInfo *) pInData;
			printf("\t sUID=%s, sHostName=%s, sSN=%s, sProduct=%s\n", pSenHubInfo->sUID, pSenHubInfo->sHostName, pSenHubInfo->sSN, pSenHubInfo->sProduct);
			break;
		}
		// 2001
		case SN_SenHub_SendInfoSpec: {
			int i;
			InSenData *pSenInfoSpec = (InSenData *) pInData;
			printf("\t sUID=%s TypeCount=%d\n", pSenInfoSpec->sUID, pSenInfoSpec->inDataClass.iTypeCount);
			if (pSenInfoSpec->inDataClass.pInBaseDataArray!=NULL && pSenInfoSpec->inDataClass.iTypeCount>0) {
				for(i=0; i<pSenInfoSpec->inDataClass.iTypeCount; i++) {
					printf("\t iSizeType=%d, psType=%s, iSizeSenData=%d\n", 
							pSenInfoSpec->inDataClass.pInBaseDataArray[i].iSizeType, 
							pSenInfoSpec->inDataClass.pInBaseDataArray[i].psType, 
							pSenInfoSpec->inDataClass.pInBaseDataArray[i].iSizeData);
					printf("\t psSenData=%s\n", pSenInfoSpec->inDataClass.pInBaseDataArray[i].psData);
				}
			}
			break;
		}
		// 2002
		case SN_SenHub_AutoReportData:
		// 2003
		case SN_SenHub_Disconnect: {
			int i;
			InSenData *pSenData = (InSenData *) pInData;
			printf("\t sUID=%s TypeCount=%d\n", pSenData->sUID, pSenData->inDataClass.iTypeCount);
			if (pSenData->inDataClass.pInBaseDataArray!=NULL && pSenData->inDataClass.iTypeCount>0) {
				for(i=0; i<pSenData->inDataClass.iTypeCount; i++) {
					printf("\t iSizeType=%d, psType=%s, iSizeSenData=%d\n",
							pSenData->inDataClass.pInBaseDataArray[i].iSizeType,
						   	pSenData->inDataClass.pInBaseDataArray[i].psType,
						   	pSenData->inDataClass.pInBaseDataArray[i].iSizeData);
					printf("\t psSenData=%s\n", pSenData->inDataClass.pInBaseDataArray[i].psData);
				}
			}
			//if( pSenData->pExtened == NULL ) 
			//	printf("\t pExtened=%s\n", "NULL");
			break;
		}
		default:
			printf("Unknown cmdId=%d\n", cmdId);
			break;
	}
	printf("\n");

	//TestGetAPI();
	//TestSetAPI();
	
	printf("######################################################\n", __func__);
	return 0;
}

int printSNInfInfos(SNInfInfos *pSNInfos)
{
	int i;
	printf("############## Main - Got SNInfInfos: #############\n\t sComType=%s\n\t iNum=%d\n", pSNInfos->sComType, pSNInfos->iNum);
	for (i=0; i<pSNInfos->iNum; i++) {
		printf("\t SNInfs[%d].sInfName=%s\n", i, pSNInfos->SNInfs[i].sInfName);
		printf("\t SNInfs[%d].sInfID=%s\n", i, pSNInfos->SNInfs[i].sInfID);
	}
	printf("###################################################\n", __func__);
	
	return 0;
}


int WsnDrv_Start()
{
	void *pLibHandle = NULL;	
	SNInfInfos SNInfos;
	char *pSWVersion = NULL;

	pLibHandle = app_load_library(DUSTWSNLIB_NAME);
	if (NULL == pLibHandle) {
		char *errMsg = app_load_error();
		printf("Error: Failed to load %s: %s\n", DUSTWSNLIB_NAME, errMsg);
		free(errMsg);
		return -1;
	}
	g_pFunc_SNInit			= (SN_Initialize_API) app_get_proc_address(pLibHandle, "SN_Initialize");
	g_pFunc_SNUninit		= (SN_Uninitialize_API) app_get_proc_address(pLibHandle, "SN_Uninitialize");
	g_pFunc_SNGetCapability	= (SN_GetCapability_API) app_get_proc_address(pLibHandle, "SN_GetCapability");
	g_pFunc_SNSetUpdateCbf	= (SN_SetUpdateDataCbf_API) app_get_proc_address(pLibHandle, "SN_SetUpdateDataCbf");
	g_pFunc_SNGetSWVersion	= (SN_GetVersion_API) app_get_proc_address(pLibHandle, "SN_GetVersion");
	g_pFunc_SNGetData	= (SN_GetData_API) app_get_proc_address(pLibHandle, "SN_GetData");
	g_pFunc_SNSetData	= (SN_SetData_API) app_get_proc_address(pLibHandle, "SN_SetData");

	if (NULL==g_pFunc_SNInit || NULL==g_pFunc_SNUninit || 
		NULL==g_pFunc_SNGetCapability || NULL==g_pFunc_SNSetUpdateCbf ||
		NULL==g_pFunc_SNGetSWVersion || NULL== g_pFunc_SNGetData || NULL==g_pFunc_SNSetData) {
		printf("Error: Function load error from %s\n", DUSTWSNLIB_NAME);
		return -1;
	}

	memset(&SNInfos, 0, sizeof(SNInfInfos));
	
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
	if (g_pFunc_SNGetCapability(&SNInfos) == -1) {
		printf("Error: Lib get capability\n");
	} else {
		printSNInfInfos(&SNInfos);
	}

	if(pSWVersion != NULL)
		free(pSWVersion);

	return 0;
}

int WsnDrv_Stop()
{
	if (g_pFunc_SNUninit(NULL) == -1) {
		printf("Error: Lib uninit\n");
		return -1;
	}
	
	return 0;
}

int main(int argc, char *argv[])
{
	int iRet = -1;
	int rcCond = 0, rcMux = 0;
	struct sigaction sigAct;
	BOOL bRet = FALSE;
	char cmdChr[512] = {'\0'};

	AdvLog_Init();
	AdvLog_Arg(argc, argv);

	memset (&sigAct, '\0', sizeof(sigAct));
	sigAct.sa_handler = &sig_hdl;
	if (sigaction(SIGTERM, &sigAct, NULL) < 0) {
		printf("sigaction error!");                                                                                                                                                                                                                                                 
		return iRet;
	}

	rcMux = app_os_mutex_setup(&srvcStopMux);
	rcCond = app_os_cond_setup(&srvcStopEvent);
	if(rcCond != 0 || rcMux != 0)
		return iRet;
 

WsnDrv_Start();
	do {
#if 0
		bRet = ExecuteCmd(strtok(cmdChr, "\n")) == 0? TRUE:FALSE;
		if (!bRet)
			break;
		memset(cmdChr, 0, sizeof(cmdChr));
		gets_s(cmdChr, sizeof(cmdChr));	
#endif
		sleep(1);
	} while (TRUE);

	/*
	app_os_mutex_lock(&srvcStopMux);
	bRet = isSrvRun = WsnDrvStart();
	app_os_mutex_unlock(&srvcStopMux);
	while(bRet)
	{
		printf("Program is running!");
		app_os_mutex_lock(&srvcStopMux);
		app_os_cond_wait(&srvcStopEvent, &srvcStopMux);
		app_os_mutex_unlock(&srvcStopMux);
		printf("Get stop signal!");
		break;
	} */

	app_os_mutex_lock(&srvcStopMux);
	app_os_cond_signal(&srvcStopEvent);
	app_os_mutex_unlock(&srvcStopMux);
	
	app_os_mutex_cleanup(&srvcStopMux);
	app_os_cond_cleanup(&srvcStopEvent);
	
	return TRUE;
}
