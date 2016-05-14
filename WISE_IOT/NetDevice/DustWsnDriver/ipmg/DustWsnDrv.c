#include <unistd.h>
#include <pthread.h>
#include "SensorNetwork_API.h"
#include "SensorNetwork_BaseDef.h"
#include "DustWsnDrv.h"
#include "IpMgWrapper.h"
#include "MoteListAccess.h"
#include "AdvLog.h"

#if defined(WIN32) //windows
#include "AdvPlatform.h"
#endif

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
//#if defined(WIN32) //windows
//#define DUSTWSN_DEV_NODE		"\\\\.\\COM3"
//#elif defined(__linux)
//#define DUSTWSN_DEV_NODE		"/dev/ttyUSB0"	// on RISC
//#endif
//#define DUSTWSN_DEV_NODE		"/dev/ttyUSB3"		// on VM
#define DUSTWSN_SERIALMUX_PORT	"9900"	

#ifdef USE_WSN_DEV2
#define DUSTWSN_DEV_NODE		"/dev/ttyUSB1"	// on RISC
#define DUSTWSN_CONTYPE			"WSN2"
#else
#define DUSTWSN_DEV_NODE		"/dev/ttyUSB0"	// on RISC
#define DUSTWSN_CONTYPE			"WSN"
#endif

#define DUSTWSN_NAME			"WSN0"
#define WISE1020_NAME			"WISE1020"

#define RULE_SRC_SENSOR_CO2_ID		"9001"
#define RULE_SRC_SENSOR_CO_ID		"9002"
#define RULE_SRC_SENSOR_LIGHT_ID	"3301"
#define RULE_SRC_CO2_THR_1			1500
#define RULE_SRC_CO_THR_1			510
#define RULE_SRC_LIGHT_THR_1		50

#define SENHUBLIST_HDR "{\"n\":\"SenHubList\",\"sv\":\""
#define SENHUBLIST_END "}"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
// for SUSIAccess
SNInfInfo                       g_InfInfo;
static SNInterfaceData			g_SNInfData;			// need to free memory
static InSenData				g_SenInfoSpec;			// need to free memory
//static UpdateSNDataCbf			g_UpdateDataCbf;
UpdateSNDataCbf			        g_UpdateDataCbf;
InSenData						g_SenData;				// need to free memory
InSenData	                    g_SenInfoAction;

DustWsn_Context_t				g_DustWsnContext;
BOOL							g_doSensorScan;
Mote_Info_t* 					g_pMoteToScan;
BOOL							g_doUpdateInterface;

static BOOL						g_GetCapability;
static Mote_Info_t				*g_MgrInfo = NULL;

//static time_t					g_MonitorTime;
//static int						g_NetMoteNum;			// change to operational mote number?

extern BOOL						g_MgrConnected;

static void						*g_UserData;

char 					*g_ManagerSWVer = NULL; // format: x.x.x.x

// IO control 
Mote_Info_t *g_pActMote = NULL;
Sensor_Info_t *g_pActIoFan1Info = NULL;
Sensor_Info_t *g_pActIoFan2Info = NULL;
Sensor_Info_t *g_pActIoLedInfo = NULL;

Mote_Info_t *g_pSenCO2Mote = NULL;
Mote_Info_t *g_pSenCOMote = NULL;
Mote_Info_t *g_pSenLightMote = NULL;
Sensor_Info_t *g_pSenCO2Info = NULL;
Sensor_Info_t *g_pSenCOInfo = NULL;
Sensor_Info_t *g_pSenLightInfo = NULL;

//-----------------------------------------------------------------------------
// Private Function:
//-----------------------------------------------------------------------------
int getNeighborMacList(Mote_Info_t *pMote, char *MacList)
{
	char *curpos;
	int i;

	curpos = MacList;
	ADV_TRACE("%s: neighborList total=%d\n", __func__, pMote->neighborList.total);
	for(i=0; i<pMote->neighborList.total; i++) {
		if(i != 0) {
			*curpos = ',';
			curpos++;
		}
		// current mac string ex: 00170d0000582a4f (length = 16)
		MacArrayToString(pMote->neighborList.item[i].macAddress, 8, curpos, MAX_MAC_LEN);
		curpos += strlen(curpos);
	}
}

bool isResponseTimeout(time_t _inTime)
{
	time_t curretTime;
	time(&curretTime);

	if (difftime(curretTime, _inTime) > DUSTWSN_RESPONSE_TIMEOUT) {
		return TRUE;
	} else {
		return FALSE;
	}
}

int GetSensorMoteByType(Mote_List_t *pMoteList, unsigned char *strType, int *moteId, int *SensorId)
{
	Mote_Info_t *pItem;
	int i,j;

	if (NULL == pMoteList)
		return -1;

	pItem = pMoteList->item;

	j = 1;
	while (NULL != pItem) {
		if (MOTE_STATE_OPERATIONAL==pItem->state && pItem->sensorList.total!=0) {
			for (i=0; i<pItem->sensorList.total; i++) {
				if (strncmp(pItem->sensorList.item[i].objId, strType, 
							strlen(pItem->sensorList.item[i].objId)) == 0) {
					*moteId = j;
					*SensorId = i; 
					//printf("Found %s sensor mote!!!!!!!!!!\n", strType);
					return 0;
				}
			}
		}
		j++;
		pItem = pItem->next;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Main Function:
//-----------------------------------------------------------------------------
static void * DustWsnThread(void *args)
{
	int sleepTime = 100;
	time_t currentTime, prevTime;
	DustWsn_Context_t *pWsnDrvContext = (DustWsn_Context_t *)args;
	time(&prevTime);
	
	ADV_INFO("**************** DustWsnThread ******************!\n");
	if(g_DustWsnContext.thread_nr > 1) return;
	ADV_INFO("**************** DustWsnThread started ******************!\n");
	while (pWsnDrvContext->isThreadRunning) {
		usleep(sleepTime*1000);

		UpdateIoTGWIntf();
		
		GetAllSensorCapability();

		MotePathUpdate();

		SendMoteCommand();

		SendMoteObserveCommand();

		//ExecuteIoRules(); // for demo

		IpMgWrapper_loop();
		
		// auto-report
		
		//time(&g_MonitorTime);
		time(&currentTime);

		if (difftime(currentTime, prevTime) > DUSTWSN_RESPONSE_TIMEOUT) {
			ADV_INFO("%s(%s): Still Runing!\n", __func__, DUSTWSN_CONTYPE);
			prevTime = currentTime;
		}
	}
	
	// release mutex
	// free mote list
	ADV_INFO("**************** DustWsnThread quit!!! ******************!\n");
}

int UpdateIoTGWIntf()
{
	// callback to Update IoTGW interface data
	if (g_doUpdateInterface) {
		// CmdID = 1000
		if(fillSNInfData(&g_SNInfData) == 0) {
			g_UpdateDataCbf(SN_Inf_UpdateInterface_Data, &g_SNInfData, sizeof(SNInterfaceData), g_UserData, NULL, NULL, NULL);
			g_doUpdateInterface = FALSE;
		}
	}
	return 0;
}

int MotePathUpdate()
{
	Mote_Info_t *pMote;

	if(!g_GetCapability || IpMgWrapper_Scheduling())
		return -1;

	pMote = MoteList_GetFirstItemToPathUpdate(&g_DustWsnContext.SmartMeshIpInfo.MoteList);

	if (pMote != NULL) {
		ADV_INFO("%s: =========== start on Mote(bPathUpdate=%d) ", __func__, pMote->bPathUpdate);
		IpMgWrapper_printByteArray(pMote->macAddress, 8);
		ADV_INFO("===========\n");
		if(IpMgWrapper_MoteGetMotePath(pMote) == 0) {
			#if 0
			ADV_INFO("%s: ========== end on Mote(bPathUpdate=%d) ", __func__, pMote->bPathUpdate);
			IpMgWrapper_printByteArray(pMote->macAddress, 8);
			ADV_INFO("===========\n");
			#endif
			if(pMote->moteId == 1) // id 1 is manager interface
				g_doUpdateInterface = TRUE;
		}
	}
	return 0;
}

int SendMoteCommand()
{
	Mote_Info_t *pMoteToSendCmd;

	if(IpMgWrapper_Scheduling())
		return -1;

	pMoteToSendCmd = MoteList_GetFirstItemToSendCmd(&g_DustWsnContext.SmartMeshIpInfo.MoteList);

	if (NULL == pMoteToSendCmd) { //|| Mote_Manufacturer_Dust == pMoteToSendCmd->manufacturer) {
		//printf("No mote need to send\n");
		return 0;
	} else {
		if(pMoteToSendCmd->cmdType == Mote_Cmd_None)
			return 0;
		ADV_INFO("%s: Sending command(cmdType=%d) on Mote ", __func__, pMoteToSendCmd->cmdType);
		IpMgWrapper_printByteArray(pMoteToSendCmd->macAddress, 8);
		ADV_INFO(" \n");
		IpMgWrapper_MoteSendDataCmd(pMoteToSendCmd);
	}
}

int ExecuteIoRules()
{
	Mote_List_t *pMoteList = &g_DustWsnContext.SmartMeshIpInfo.MoteList;
	static BOOL isD4On = FALSE;
	static BOOL isD5On = FALSE;
	static BOOL isD6On = FALSE;
	BOOL isReportRequired = FALSE;
	int ivalue;
	int i;
	int iMoteId, iSenId;

	if(IpMgWrapper_Scheduling())
		return -1;

	// Find IO mote
	if(g_pActMote == NULL) {
		if((iMoteId = IpMgWrapper_getIOMoteId()) == 0)
			return -1;
		if((g_pActMote = MoteList_GetItemById(pMoteList, iMoteId)) == NULL)
			return -1;

		for (i=0; i<g_pActMote->sensorList.total; i++) {
			if (NULL==g_pActIoFan1Info && strncmp(g_pActMote->sensorList.item[i].name, RULE_DST_DO1_NAME, strlen(g_pActMote->sensorList.item[i].name)) == 0) {
				g_pActIoFan1Info = &(g_pActMote->sensorList.item[i]);
				ADV_INFO("Found fan1...................\n");
				continue;
			}
			if (NULL==g_pActIoFan2Info && strncmp(g_pActMote->sensorList.item[i].name, RULE_DST_DO2_NAME, strlen(g_pActMote->sensorList.item[i].name)) == 0) {
				g_pActIoFan2Info = &(g_pActMote->sensorList.item[i]);
				ADV_INFO("Found fan2...................\n");
				continue;
			}
			if (NULL==g_pActIoLedInfo && strncmp(g_pActMote->sensorList.item[i].name, RULE_DST_DO3_NAME, strlen(g_pActMote->sensorList.item[i].name)) == 0) {
				g_pActIoLedInfo = &(g_pActMote->sensorList.item[i]);
				ADV_INFO("Found light..................\n");
				continue;
			}
		}
	}

	// Find Sensor Mote for detect
	if(g_pSenCO2Mote == NULL) {
		if(GetSensorMoteByType(pMoteList, RULE_SRC_SENSOR_CO2_ID, &iMoteId, &iSenId) == 0) {
			ADV_INFO("Found CO2 Mote(id=%d/%d)...................\n", iMoteId, iSenId);
			g_pSenCO2Mote = MoteList_GetItemById(pMoteList, iMoteId);
			g_pSenCO2Info = &(g_pSenCO2Mote->sensorList.item[iSenId]);
		}
	}
	if(g_pSenCOMote == NULL) {
		if(GetSensorMoteByType(pMoteList, RULE_SRC_SENSOR_CO_ID, &iMoteId, &iSenId) == 0) {
			ADV_INFO("Found CO Mote(id=%d/%d)....................\n", iMoteId, iSenId);
			g_pSenCOMote = MoteList_GetItemById(pMoteList, iMoteId);
			g_pSenCOInfo = &(g_pSenCOMote->sensorList.item[iSenId]);
		}
	}

	if(g_pSenLightMote == NULL) {
		if(GetSensorMoteByType(pMoteList, RULE_SRC_SENSOR_LIGHT_ID, &iMoteId, &iSenId) == 0) {
			ADV_INFO("Found Light Mote(id=%d/%d).................\n", iMoteId, iSenId);
			g_pSenLightMote = MoteList_GetItemById(pMoteList, iMoteId);
			g_pSenLightInfo = &(g_pSenLightMote->sensorList.item[iSenId]);
		}
	}

	// execute rule
	// CO2 & Fan1
	if (NULL != g_pSenCO2Info && g_pSenCO2Mote->MoteQueryState == Mote_Query_Observe_Started) { 
		ivalue = atoi(g_pSenCO2Info->value);
		if (ivalue >= RULE_SRC_CO2_THR_1) {
			if (FALSE == isD6On) {
				ADV_TRACE("turn on D6\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D6, 1);
				isD6On = TRUE;
				memcpy(g_pActIoFan1Info->value, "1", strlen(g_pActIoFan1Info->value));
				isReportRequired = TRUE;
			}
		} else {
			if (TRUE == isD6On) {
				ADV_TRACE("turn off D6\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D6, 0);
				isD6On = FALSE;
				memcpy(g_pActIoFan1Info->value, "0", strlen(g_pActIoFan1Info->value));
				isReportRequired = TRUE;
			}
		}
		if (TRUE == isReportRequired) {
			Susiaccess_Report_SensorData(g_pActMote);
			return 0;
		}
	}

	// CO & Fan2
	if (NULL != g_pSenCOInfo && g_pSenCOMote->MoteQueryState == Mote_Query_Observe_Started) {
		ivalue = atoi(g_pSenCOInfo->value);
		if (ivalue >= RULE_SRC_CO_THR_1) {
			if (FALSE == isD5On) {
				ADV_TRACE("turn on D5\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D5, 1);
				isD5On = TRUE;
				memcpy(g_pActIoFan2Info->value, "1", strlen(g_pActIoFan2Info->value));
				isReportRequired = TRUE;
			}
		} else {
			if (TRUE == isD5On) {
				ADV_TRACE("turn off D5\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D5, 0);
				isD5On = FALSE;
				memcpy(g_pActIoFan2Info->value, "0", strlen(g_pActIoFan2Info->value));
				isReportRequired = TRUE;
			}
		}
		if (TRUE == isReportRequired) {
			Susiaccess_Report_SensorData(g_pActMote);
			return 0;
		}
	}

	// Light 
	if (NULL != g_pSenLightInfo && g_pSenLightMote->MoteQueryState == Mote_Query_Observe_Started) { 
		ivalue = atoi(g_pSenLightInfo->value);
		//ADV_ERROR("Turn Light valstr=%s, val=%d......................\n",g_pSenLightInfo->value,ivalue);
		if (ivalue < RULE_SRC_LIGHT_THR_1) {
			if (FALSE == isD4On) {
				ADV_TRACE("turn on D4\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D4, 1);
				isD4On = TRUE;
				memcpy(g_pActIoLedInfo->value, "1", strlen(g_pActIoLedInfo->value));
				isReportRequired = TRUE;
			}
		} else {
			if (TRUE == isD4On) {
				ADV_TRACE("turn off D4\n");
				IpMgWrapper_MoteMasterSetIO(g_pActMote, Mote_Master_D4, 0);
				isD4On = FALSE;
				memcpy(g_pActIoLedInfo->value, "0", strlen(g_pActIoLedInfo->value));
				isReportRequired = TRUE;
			}
		}
		if (TRUE == isReportRequired) {
			Susiaccess_Report_SensorData(g_pActMote);
			return 0;
		}
	} else {	// no Sensor mote, do on/off demo
#if 0
		time_t curretTime;
		static time_t prevTime = 0;
		static BOOL switchOn = FALSE;

		time(&curretTime);
		if (prevTime == curretTime)
			return 0;

		if ((curretTime % 33) == 0) {
			if (FALSE == switchOn) {
				switchOn = TRUE;
				ADV_TRACE("turn on D4\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D4, 1);
			 } else {
				switchOn = FALSE;
				ADV_TRACE("turn off D4\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D4, 0);
			 }
		}

		if ((curretTime % 35) == 0) {
			if (FALSE == switchOn) {
				switchOn = TRUE;
				ADV_TRACE("turn on D5\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D5, 1);
			 } else {
				switchOn = FALSE;
				ADV_TRACE("turn off D5\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D5, 0);
			 }
		}

		if ((curretTime % 37) == 0) {
			if (FALSE == switchOn) {
				switchOn = TRUE;
				ADV_TRACE("turn on D6\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D6, 1);
			 } else {
				switchOn = FALSE;
				ADV_TRACE("turn off D6\n");
				IpMgWrapper_MoteMasterSetIO(pActMote, Mote_Master_D6, 0);
			 }
		}

		prevTime = curretTime;
#endif
	}

	return 0;
}

int SendMoteObserveCommand()
{
	int i;
	int totalMote;
	int totalSensor;
	Mote_Info_t *pMote;

	if(IpMgWrapper_Scheduling())
		return -1;

	// Find the mote need to cancel observe
	pMote = MoteList_GetFirstItemToSendCancelObserve(&g_DustWsnContext.SmartMeshIpInfo.MoteList);
	if (NULL != pMote) {
		ADV_INFO("%s: Sending Cancel Observ command on Mote ", __func__ );
		IpMgWrapper_printByteArray(pMote->macAddress, 8);
		ADV_INFO(" \n");
		IpMgWrapper_MoteSendDataCancelObserve(pMote);
		time(&pMote->QuerySentTime);
		return 0;
	}

	pMote = MoteList_GetFirstItemToSendObserve(&g_DustWsnContext.SmartMeshIpInfo.MoteList);
	if (NULL == pMote) {
		return -2;
	}

	ADV_INFO("%s: Sending Observ command on Mote ", __func__ );
	IpMgWrapper_printByteArray(pMote->macAddress, 8);
	ADV_INFO(" \n");
	IpMgWrapper_MoteSendDataObserve(pMote);

	return 0;
}

int MoteGetSensorInfo()
{
	int i;
	int target = -1;

	if (NULL == g_pMoteToScan) {
		ADV_TRACE("Get Sensor Info failed: Mote NULL\n");
		return -1;
	}

	for (i=0; i<g_pMoteToScan->sensorList.total; i++) {
		if (Sensor_Query_NotStart == g_pMoteToScan->sensorList.item[i].SensorQueryState) {
			IpMgWrapper_MoteGetSensorInfo(g_pMoteToScan, g_pMoteToScan->sensorList.item[i].index);
			g_pMoteToScan->sensorList.item[i].SensorQueryState = Sensor_Query_SensorInfo_Sent;
			time(&g_pMoteToScan->sensorList.item[i].QuerySentTime);
			break;
		} else if (Sensor_Query_SensorInfo_Sent == g_pMoteToScan->sensorList.item[i].SensorQueryState) {
			if (isResponseTimeout(g_pMoteToScan->sensorList.item[i].QuerySentTime)) {
				ADV_INFO("%s: Mote ", __func__);
				IpMgWrapper_printByteArray(g_pMoteToScan->macAddress, 8);
				ADV_INFO(" sensor %d (total=%d) info request timeout!\n", g_pMoteToScan->sensorList.item[i].index, g_pMoteToScan->sensorList.total);
				//g_pMoteToScan->MoteQueryState = Mote_Query_SensorList_Received;
				//g_pMoteToScan->sensorList.item[i].SensorQueryState = Sensor_Query_NotStart;
				g_pMoteToScan->MoteQueryState = Mote_Query_NotStart;
				g_pMoteToScan->scannedSensor = 0;
				MoteList_ResetMoteSensorList(g_pMoteToScan);
				
				break;		// do only once, not to take too long.
			}
		}	
	}
	
	return 0;
}

// CmdID = 2000
int Susiaccess_Report_SenNodeRegister(Mote_Info_t *pTargetMote)
{
	SenHubInfo nodeInfo;	// use global variable?

	if(pTargetMote == NULL) {
		ADV_TRACE("pTargetMote is NULL\n");
		return -1;
	}

	if(g_UpdateDataCbf == NULL) {
		ADV_TRACE("g_UpdateDataCbf is NULL\n");
		return -1;
	}

	//printf("\n%s: moteId=%d\n", __func__, pTargetMote->moteId);
	memset(&nodeInfo, 0, sizeof(SenHubInfo));

	fillSenHubInfo(&nodeInfo, pTargetMote);
	g_UpdateDataCbf(SN_SenHub_Register, &nodeInfo, sizeof(SenHubInfo), g_UserData, NULL, NULL, NULL);
}

// CmdID = 2001
int Susiaccess_Report_SenNodeSendInfoSpec(Mote_Info_t *pTargetMote)
{
	if(g_UpdateDataCbf == NULL) {
		ADV_TRACE("%s: g_UpdateDataCbf is NULL\n", __func__);
		return -1;
	}
	fillSenInfoSpec(&g_SenInfoSpec, pTargetMote);
#if 0
	printf("%s: sUID=%s TypeCount=%d\n", __func__, g_SenInfoSpec.sUID, g_SenInfoSpec.inDataClass.iTypeCount);
	if (g_SenInfoSpec.inDataClass.pInBaseDataArray!=NULL && g_SenInfoSpec.inDataClass.iTypeCount>0) {
		int i;
		for(i=0; i<g_SenInfoSpec.inDataClass.iTypeCount; i++) {
			printf("\t iSizeType=%d, psType=%s, iSizeSenData=%d\n", 
					g_SenInfoSpec.inDataClass.pInBaseDataArray[i].iSizeType, 
					g_SenInfoSpec.inDataClass.pInBaseDataArray[i].psType, 
					g_SenInfoSpec.inDataClass.pInBaseDataArray[i].iSizeData);
			printf("\t psSenData=%s\n", g_SenInfoSpec.inDataClass.pInBaseDataArray[i].psData);
		}
	}
#endif
	g_UpdateDataCbf(SN_SenHub_SendInfoSpec, &g_SenInfoSpec, sizeof(InSenData), g_UserData, NULL, NULL, NULL);
}

int GetAllSensorCapability(void)
{
	time_t curretTime;

	if(!g_GetCapability) 
		return 0;

	if(IpMgWrapper_Scheduling()) {
		return 0;
	}

	if (NULL == g_pMoteToScan) { 
		pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
		g_pMoteToScan = MoteList_GetFirstScanItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList);
		pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	}

	if (NULL != g_pMoteToScan) {	// in case target mote disconnected
		switch(g_pMoteToScan->MoteQueryState) {
			case Mote_Query_NotStart:
				if(g_pMoteToScan->bPathUpdate == FALSE) {
					IpMgWrapper_MoteGetMoteInfo(g_pMoteToScan);		// send "GET /dev/info?action=list"
					g_pMoteToScan->MoteQueryState = Mote_Query_MoteInfo_Sent;
					time(&g_pMoteToScan->QuerySentTime);
				}
			break;
			case Mote_Query_MoteInfo_Received:
				Susiaccess_Report_SenNodeRegister(g_pMoteToScan);			
			
				IpMgWrapper_MoteGetSensorIndex(g_pMoteToScan);	// send "GET /sen/info?action=list&field=id"
				g_pMoteToScan->MoteQueryState = Mote_Query_SensorList_Sent;
				time(&g_pMoteToScan->QuerySentTime);
			break;
			case Mote_Query_SensorList_Received:
			case Mote_Query_SensorStandard_Received:
			case Mote_Query_SensorHardware_Received:
				if (g_pMoteToScan->sensorList.total > 0) {
					//printf("scannedSensor=%d, %d=sensorList.total\n", g_pMoteToScan->scannedSensor, g_pMoteToScan->sensorList.total);
					if (g_pMoteToScan->scannedSensor < g_pMoteToScan->sensorList.total) {
						MoteGetSensorInfo();
					} else if (g_pMoteToScan->scannedSensor == g_pMoteToScan->sensorList.total) {
						Susiaccess_Report_SenNodeSendInfoSpec(g_pMoteToScan);
						g_pMoteToScan = NULL;
					} else {
						ADV_ERROR("%s: Discover Sensor Info Error!\n", __func__);
					}
				}
			break;
			case Mote_Query_Observe_Started: // Master mode only
				ADV_INFO("%s: Mote_Query_Observe_Started (Master Mode)!\n", __func__);
				g_pMoteToScan = NULL;
			break;
			default:
				if (isResponseTimeout(g_pMoteToScan->QuerySentTime)) {
					//ADV_INFO("%s: Mote ", __func__);
					//IpMgWrapper_printByteArray(g_pMoteToScan->macAddress, 8);
					if (Mote_Query_MoteInfo_Sent == g_pMoteToScan->MoteQueryState) {
						Mote_Info_t *pMote = g_pMoteToScan;
						ADV_INFO("%s: get mote info timeout, try next mote!\n", __func__);
						pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
						g_pMoteToScan = MoteList_GetFirstScanItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList);
						pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
						pMote->MoteQueryState = Mote_Query_NotStart;
					} else if (Mote_Query_SensorList_Sent == g_pMoteToScan->MoteQueryState) {
						g_pMoteToScan->MoteQueryState = Mote_Query_MoteInfo_Received;
						//ADV_INFO(" get sensor list timeout!");
					}
					//ADV_INFO("\n");
					usleep(10000);
				}
			break;
		}
	}
	return 0;

}

static int getTotalMoteNum(void)
{
	int totalMote = 0;
	pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	totalMote = g_DustWsnContext.SmartMeshIpInfo.MoteList.total;
	pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);

	return totalMote;
}

static void printMoteList(void)
{
	int i;
	int totoalMote;
	int curretMoteId = 0;
	Mote_Info_t *item = NULL;
	
	//pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	ADV_INFO("Motes List - Total:%d\n", g_DustWsnContext.SmartMeshIpInfo.MoteList.total);
	for (i=1; i<=g_DustWsnContext.SmartMeshIpInfo.MoteList.total; i++) {
		if (1 == i) {
			item = MoteList_GetItemById(&g_DustWsnContext.SmartMeshIpInfo.MoteList, i);
		} else {
			item = MoteList_GetNextOperItemById(&g_DustWsnContext.SmartMeshIpInfo.MoteList, curretMoteId);
		}
		if (NULL != item) {
			curretMoteId = item->moteId;
			ADV_INFO("\t %d: ID:%d mac:", i, curretMoteId);
			IpMgWrapper_printByteArray(item->macAddress, 8);
			ADV_INFO(" state:%d AP:%d isRouting:%d MoteQueryState:%d\n", item->state, item->isAP, item->isRouting, item->MoteQueryState);
		}
	}
	//pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
}

// Create Interface Data 
static void fillSNInfInfos(SNInfInfos *pSNInfInfos)
{
	int bufSize;
	
	//printf("%s: in\n", __func__);
	snprintf(pSNInfInfos->sComType, sizeof(pSNInfInfos->sComType), "%s", DUSTWSN_CONTYPE);

	pSNInfInfos->iNum = 1;

	// Detail Information of Sensor Network Interface
	snprintf(pSNInfInfos->SNInfs[0].sInfName, sizeof(pSNInfInfos->SNInfs[0].sInfName), "%s", DUSTWSN_NAME);

	g_MgrInfo = MoteList_GetItemById(&g_DustWsnContext.SmartMeshIpInfo.MoteList, 1);
	if (NULL == g_MgrInfo) {
		ADV_TRACE("NotifData: Mote ID=%d not found\n", 1);
		//return -1;
		return;
	}
	// ID0 is manager id
	snprintf(pSNInfInfos->SNInfs[0].sInfID, sizeof(pSNInfInfos->SNInfs[0].sInfID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			g_MgrInfo->macAddress[0], g_MgrInfo->macAddress[1], g_MgrInfo->macAddress[2], g_MgrInfo->macAddress[3],
			g_MgrInfo->macAddress[4], g_MgrInfo->macAddress[5], g_MgrInfo->macAddress[6], g_MgrInfo->macAddress[7]
			);

	pSNInfInfos->SNInfs[0].outDataClass.iTypeCount = 1;
#if 1
	if(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray == NULL) {
		printf("xxxxxxxxxxxxxxxxxxxxxxxxxxx OutBaseDataArray is NULL xxxxxxxxxxxxxxxxxxxxxxxxxx\n");
		pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData) * pSNInfInfos->SNInfs[0].outDataClass.iTypeCount);
	}
#endif
	pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psType = strdup("Info");
	pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeType = sizeof(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psType);

#if 1
	if(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData == NULL) {
		printf("xxxxxxxxxxxxxxxxxxxxxxxxxxx OutBaseDataArray psData is NULL xxxxxxxxxxxxxxxxxxxxxxxxxx\n");
		bufSize = 512;
		pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData = malloc(bufSize);
		if (NULL == pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData)
			return ;
		memset(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData, 0, bufSize);
		snprintf(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData, bufSize,
			"{\"n\":\"SenHubList\", \"sv\":\"\", \"asm\":\"r\"},{\"n\":\"Neighbor\", \"sv\":\"\", \"asm\":\"r\"},{\"n\":\"Health\", \"v\":-1, \"asm\":\"r\"},{\"n\":\"Name\", \"sv\":\"%s\", \"asm\":\"r\"},{\"n\":\"sw\", \"sv\":\"%s\", \"asm\":\"r\"},{\"n\":\"reset\", \"bv\":0, \"asm\":\"rw\"}",
			DUSTWSN_NAME, 
			g_ManagerSWVer
			);
		pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeData = malloc(sizeof(int));
		*pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeData = strlen(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData);
	} else 
#endif
	{
	sprintf(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData,
			"{\"n\":\"SenHubList\", \"sv\":\"\", \"asm\":\"r\"},{\"n\":\"Neighbor\", \"sv\":\"\", \"asm\":\"r\"},{\"n\":\"Health\", \"v\":-1, \"asm\":\"r\"},{\"n\":\"Name\", \"sv\":\"%s\", \"asm\":\"r\"},{\"n\":\"sw\", \"sv\":\"%s\", \"asm\":\"r\"},{\"n\":\"reset\", \"bv\":0, \"asm\":\"rw\"}",
			DUSTWSN_NAME, 
			g_ManagerSWVer
			);
	*pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeData = strlen(pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData);
	}
	
	ADV_INFO("=============================================\n");
	ADV_INFO("sComType=%s, ", pSNInfInfos->sComType);
	ADV_INFO("iNum=%d, ", pSNInfInfos->iNum);
	ADV_INFO("InfName=%s, ", pSNInfInfos->SNInfs[0].sInfName);
	ADV_INFO("InfID=%s\n", pSNInfInfos->SNInfs[0].sInfID);
	ADV_INFO("out type count=%d, ", pSNInfInfos->SNInfs[0].outDataClass.iTypeCount);
	ADV_INFO("out type=%s, ", pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psType);
	ADV_INFO("size=%d\n", pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeType);
	ADV_INFO("out data=%s, ", pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].psData);
	ADV_INFO("size=%d\n", *pSNInfInfos->SNInfs[0].outDataClass.pOutBaseDataArray[0].iSizeData);
	ADV_INFO("=============================================\n");

}

static void freeSNInfData(SNInterfaceData *pSNInfData)
{
	int i;

	ADV_TRACE("%s: \n", __func__);
	if (NULL != pSNInfData) {
		for(i=0; i < pSNInfData->inDataClass.iTypeCount; i++) {
			if (NULL != pSNInfData->inDataClass.pInBaseDataArray[i].psType) {
				free(pSNInfData->inDataClass.pInBaseDataArray[i].psType);	
			}
			if (NULL != pSNInfData->inDataClass.pInBaseDataArray[i].psData) {
				free(pSNInfData->inDataClass.pInBaseDataArray[i].psData);	
			}
		}
		if (NULL != pSNInfData->inDataClass.pInBaseDataArray) {
			free(pSNInfData->inDataClass.pInBaseDataArray);	
		}

		if (NULL != pSNInfData->pExtened)
			free(pSNInfData->pExtened);
		
		memset(pSNInfData, 0, sizeof(SNInterfaceData));
	}
}

// Create Sensor Network Interface Data
int fillSNInfData(SNInterfaceData *pSNInfData)
{
	int totalMote;
	int bufSize;
	char *MacList;
	char *NeighborMacList;
	
	if(IpMgWrapper_Scheduling()) {
		//printf("%s: @@@@@@@@@@@@@@@@@ is scheduling!\n", __func__);
		return -1;
	}

	ADV_INFO("%s: moteId(%d), isAP(%d), neighbors(%d)\n", __func__, g_MgrInfo->moteId, g_MgrInfo->isAP, g_MgrInfo->neighborList.total);

	freeSNInfData(pSNInfData);
	
	totalMote = getTotalMoteNum();

	snprintf(pSNInfData->sComType, sizeof(pSNInfData->sComType), "%s", DUSTWSN_CONTYPE);
	snprintf(pSNInfData->sInfID, sizeof(pSNInfData->sInfID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			g_MgrInfo->macAddress[0], g_MgrInfo->macAddress[1], g_MgrInfo->macAddress[2], g_MgrInfo->macAddress[3],
			g_MgrInfo->macAddress[4], g_MgrInfo->macAddress[5], g_MgrInfo->macAddress[6], g_MgrInfo->macAddress[7]
			);



	pSNInfData->inDataClass.iTypeCount = 2;
	pSNInfData->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pSNInfData->inDataClass.iTypeCount);
	pSNInfData->inDataClass.pInBaseDataArray[0].psType = strdup("Info");
	pSNInfData->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pSNInfData->inDataClass.pInBaseDataArray[0].psType);

	// Sensor Hub List
	bufSize = totalMote * MAX_MAC_LEN;
	MacList = malloc(bufSize);
	if (NULL == MacList)
		return -2;
	memset(MacList, 0, bufSize);
	pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	MoteList_GetMoteMacListString(&g_DustWsnContext.SmartMeshIpInfo.MoteList, MacList, bufSize);
	pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);

	bufSize = 512;
	pSNInfData->inDataClass.pInBaseDataArray[0].psData = malloc(bufSize);
	if (NULL == pSNInfData->inDataClass.pInBaseDataArray[0].psData)
		return -3;
	memset(pSNInfData->inDataClass.pInBaseDataArray[0].psData, 0, bufSize);

	NeighborMacList = malloc(512);
	if (NULL == NeighborMacList)
		return -4;
	memset(NeighborMacList, 0, 512);
	getNeighborMacList(g_MgrInfo, NeighborMacList);

	sprintf(pSNInfData->inDataClass.pInBaseDataArray[0].psData,
			"{\"n\":\"SenHubList\",\"sv\":\"%s\"},{\"n\":\"Neighbor\", \"sv\":\"%s\"},{\"n\":\"Health\",\"v\":%d},{\"n\":\"sw\", \"sv\":\"%s\"},{\"n\":\"reset\", \"bv\":%d}",
			MacList, NeighborMacList, g_DustWsnContext.SmartMeshIpInfo.NetReliability, g_ManagerSWVer, 0);

	pSNInfData->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pSNInfData->inDataClass.pInBaseDataArray[0].psData);
	free(MacList);
	free(NeighborMacList);

	// Action
	pSNInfData->inDataClass.pInBaseDataArray[1].psType = strdup("Action");
	pSNInfData->inDataClass.pInBaseDataArray[1].iSizeType = sizeof(pSNInfData->inDataClass.pInBaseDataArray[1].psType);
	bufSize = 512;
	pSNInfData->inDataClass.pInBaseDataArray[1].psData = malloc(bufSize);
	if (NULL == pSNInfData->inDataClass.pInBaseDataArray[1].psData)
		return -5;
	memset(pSNInfData->inDataClass.pInBaseDataArray[1].psData, 0, bufSize);
	sprintf(pSNInfData->inDataClass.pInBaseDataArray[1].psData,
			"{\"n\":\"AutoReport\",\"bv\":1,\"asm\":\"rw\"}");
	pSNInfData->inDataClass.pInBaseDataArray[1].iSizeData = strlen(pSNInfData->inDataClass.pInBaseDataArray[1].psData);

	ADV_INFO("=============================================\n");
	ADV_INFO("%s: ", __func__);
	ADV_INFO("Total mote=%d\n", totalMote);
	ADV_INFO("sComType=%s, ", pSNInfData->sComType);
	//ADV_INFO("InfName=%s, ", pSNInfData->SNInf.sInfName);
	ADV_INFO("InfID=%s, ", pSNInfData->sInfID);
	ADV_INFO("Type count=%d, ", pSNInfData->inDataClass.iTypeCount);
	ADV_INFO("type=%s, ", pSNInfData->inDataClass.pInBaseDataArray[0].psType);
	ADV_INFO("type size=%d, ", pSNInfData->inDataClass.pInBaseDataArray[0].iSizeType);
	ADV_INFO("data=%s, ", pSNInfData->inDataClass.pInBaseDataArray[0].psData);
	ADV_INFO("data size=%d\n", pSNInfData->inDataClass.pInBaseDataArray[0].iSizeData);
	ADV_INFO("=============================================\n");

	return 0;
}

int fillSenHubInfo(SenHubInfo *pNodeInfo, Mote_Info_t *pMote)
{
	if (NULL==pNodeInfo || NULL==pMote)
		return -1;
	
	snprintf(pNodeInfo->sUID, sizeof(pNodeInfo->sUID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);

	snprintf(pNodeInfo->sSN, sizeof(pNodeInfo->sSN),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);

	snprintf(pNodeInfo->sHostName, sizeof(pNodeInfo->sHostName), "%s", pMote->hostName);

	snprintf(pNodeInfo->sProduct, sizeof(pNodeInfo->sProduct), "%s", pMote->productName);
	
	return 0;
}

static void freeSenInfoSpec(InSenData *pSenInfo)
{
	int i;

	ADV_TRACE("%s: \n", __func__);
	if (NULL != pSenInfo) {
		for(i=0; i < pSenInfo->inDataClass.iTypeCount; i++) {
			if (NULL != pSenInfo->inDataClass.pInBaseDataArray[i].psType)
				free(pSenInfo->inDataClass.pInBaseDataArray[i].psType);	
			if (NULL != pSenInfo->inDataClass.pInBaseDataArray[i].psData)
				free(pSenInfo->inDataClass.pInBaseDataArray[i].psData);	
		}
		free(pSenInfo->inDataClass.pInBaseDataArray);	

		if (NULL != pSenInfo->pExtened)
			free(pSenInfo->pExtened);
		
		memset(pSenInfo, 0, sizeof(InSenData));
	}
}

int fillSenInfoSpec(InSenData *pSenInfo, Mote_Info_t *pMote)
{
	int i;
	int bufSize;
	int totalSensor;
	char senDataStr[MAX_SENDATA_LEN];
	Sensor_Info_t *pSensor;
	char *NeighborMacList;

	if (NULL==pSenInfo || NULL==pMote) {
		ADV_TRACE("Fill SenInfoSpec failed: NULL\n");
		return -1;
	}

	totalSensor = pMote->sensorList.total;

	freeSenInfoSpec(pSenInfo);

	snprintf(pSenInfo->sUID, sizeof(pSenInfo->sUID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);

	pSenInfo->inDataClass.iTypeCount = 4;
	pSenInfo->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pSenInfo->inDataClass.iTypeCount);
	// psType = Info
	pSenInfo->inDataClass.pInBaseDataArray[0].psType = strdup("Info");
	pSenInfo->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pSenInfo->inDataClass.pInBaseDataArray[0].psType);

	bufSize = MAX_SENDATA_LEN;
	pSenInfo->inDataClass.pInBaseDataArray[0].psData = malloc(bufSize);
	if (NULL == pSenInfo->inDataClass.pInBaseDataArray[0].psData)
		return -2;
	memset(pSenInfo->inDataClass.pInBaseDataArray[0].psData, 0, bufSize);

	snprintf(pSenInfo->inDataClass.pInBaseDataArray[0].psData, bufSize,
			"{\"n\":\"Name\", \"sv\":\"%s\", \"asm\":\"rw\"},{\"n\":\"sw\", \"sv\":\"%s\", \"asm\":\"r\"},{\"n\":\"reset\", \"bv\":%d, \"asm\":\"rw\"}",
			pMote->hostName, 
			pMote->softwareVersion,
			pMote->bReset
			);
	pSenInfo->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pSenInfo->inDataClass.pInBaseDataArray[0].psData);
	
	// psType = SenData
	pSenInfo->inDataClass.pInBaseDataArray[1].psType = strdup("SenData");
	pSenInfo->inDataClass.pInBaseDataArray[1].iSizeType = strlen(pSenInfo->inDataClass.pInBaseDataArray[1].psType);
	bufSize = totalSensor * MAX_SENDATA_LEN;
	pSenInfo->inDataClass.pInBaseDataArray[1].psData = malloc(bufSize);
	if (NULL == pSenInfo->inDataClass.pInBaseDataArray[1].psData)
		return -3;
	memset(pSenInfo->inDataClass.pInBaseDataArray[1].psData, 0, bufSize);

	for (i=0; i<totalSensor; i++) {
		if (i != 0)
			strncat(pSenInfo->inDataClass.pInBaseDataArray[1].psData, ",", 1);
		pSensor = &pMote->sensorList.item[i];
		if (Sensor_Query_SensorInfo_Received == pSensor->SensorQueryState) {
			memset(senDataStr, 0, sizeof(senDataStr));
			if (strncmp(pSensor->valueType, "i", strlen(pSensor->valueType)) == 0 || 
				strncmp(pSensor->valueType, "d", strlen(pSensor->valueType)) == 0) {
				snprintf(senDataStr, sizeof(senDataStr),
						"{\"n\":\"%s\", \"u\":\"%s\", \"v\":0, \"min\":%s, \"max\":%s, \"asm\":\"%s\", \"type\":\"%s\", \"rt\":\"%s\", \"st\":\"%s\", \"exten\":\"\"}",
						pSensor->name, pSensor->unit, pSensor->minValue, pSensor->maxValue, pSensor->mode, 
						pSensor->valueType, pSensor->resType, pSensor->format, pSensor->objId
						);
			} else if (strncmp(pSensor->valueType, "b", strlen(pSensor->valueType)) == 0) {
				snprintf(senDataStr, sizeof(senDataStr),
						"{\"n\":\"%s\", \"u\":\"%s\", \"bv\":0, \"min\":%s, \"max\":%s, \"asm\":\"%s\", \"type\":\"%s\", \"rt\":\"%s\", \"st\":\"%s\", \"exten\":\"\"}",
						pSensor->name, pSensor->unit, pSensor->minValue, pSensor->maxValue, pSensor->mode, 
						pSensor->valueType, pSensor->resType, pSensor->format, pSensor->objId
						);
			} else if (strncmp(pSensor->valueType, "s", strlen(pSensor->valueType)) == 0 || 
						strncmp(pSensor->valueType, "h", strlen(pSensor->valueType)) == 0 ||
						strncmp(pSensor->valueType, "o", strlen(pSensor->valueType)) == 0) {
				snprintf(senDataStr, sizeof(senDataStr),
						"{\"n\":\"%s\", \"u\":\"%s\", \"s\":0, \"min\":%s, \"max\":%s, \"asm\":\"%s\", \"type\":\"%s\", \"rt\":\"%s\", \"st\":\"%s\", \"exten\":\"\"}",
						pSensor->name, pSensor->unit, pSensor->minValue, pSensor->maxValue, pSensor->mode, 
						pSensor->valueType, pSensor->resType, pSensor->format, pSensor->objId
						);
			} else if (strncmp(pSensor->valueType, "e", strlen(pSensor->valueType)) == 0) {	// TODO : X, Y, Z parsing
				snprintf(senDataStr, sizeof(senDataStr),
						"{\"n\":\"%s\", \"u\":\"%s\", \"sv\":\"[{\\\"n\\\":\\\"x\\\",\\\"v\\\":0},{\\\"n\\\":\\\"y\\\",\\\"v\\\":0},{\\\"n\\\":\\\"z\\\",\\\"v\\\":0}]\", \"min\":%s, \"max\":%s, \"asm\":\"%s\", \"type\":\"%s\", \"rt\":\"%s\", \"st\":\"%s\", \"exten\":\"\"}",
						pSensor->name, pSensor->unit, pSensor->minValue, pSensor->maxValue, pSensor->mode, 
						pSensor->valueType, pSensor->resType, pSensor->format, pSensor->objId
						);
			} else {
				ADV_TRACE("Unknown Sensor Value Type\n");
				continue;
			}

			strncat(pSenInfo->inDataClass.pInBaseDataArray[1].psData, senDataStr, strlen(senDataStr));
		}
	}

	pSenInfo->inDataClass.pInBaseDataArray[1].iSizeData = strlen(pSenInfo->inDataClass.pInBaseDataArray[1].psData);

	// psType = Net
	pSenInfo->inDataClass.pInBaseDataArray[2].psType = strdup("Net");
	pSenInfo->inDataClass.pInBaseDataArray[2].iSizeType = sizeof(pSenInfo->inDataClass.pInBaseDataArray[2].psType);

	bufSize = MAX_SENDATA_LEN;
	pSenInfo->inDataClass.pInBaseDataArray[2].psData = malloc(bufSize);
	if (NULL == pSenInfo->inDataClass.pInBaseDataArray[2].psData)
		return -4;
	memset(pSenInfo->inDataClass.pInBaseDataArray[2].psData, 0, bufSize);

	NeighborMacList = malloc(512);
	if (NULL == NeighborMacList)
		return -5;
	memset(NeighborMacList, 0, 512);
	getNeighborMacList(pMote, NeighborMacList);

	snprintf(pSenInfo->inDataClass.pInBaseDataArray[2].psData, bufSize,
			"{\"n\":\"Health\", \"v\":%d, \"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"%s\",\"asm\":\"r\"},{\"n\":\"sw\", \"sv\":\"%s\", \"asm\":\"r\"}",
			pMote->reliability, NeighborMacList, pMote->softwareVersion);
	pSenInfo->inDataClass.pInBaseDataArray[2].iSizeData = strlen(pSenInfo->inDataClass.pInBaseDataArray[2].psData);
	free(NeighborMacList);
	
	// psType = AutoReport
	pSenInfo->inDataClass.pInBaseDataArray[3].psType = strdup("Action");
	pSenInfo->inDataClass.pInBaseDataArray[3].iSizeType = sizeof(pSenInfo->inDataClass.pInBaseDataArray[3].psType);

	bufSize = MAX_SENDATA_LEN;
	pSenInfo->inDataClass.pInBaseDataArray[3].psData = malloc(bufSize);
	if (NULL == pSenInfo->inDataClass.pInBaseDataArray[3].psData)
		return -6;
	memset(pSenInfo->inDataClass.pInBaseDataArray[3].psData, 0, bufSize);

	if(pMote->manufacturer == Mote_Manufacturer_Dust) {
		snprintf(pSenInfo->inDataClass.pInBaseDataArray[3].psData, bufSize,
			"{\"n\":\"AutoReport\",\"bv\":1,\"asm\":\"r\"}");
	} else {
		snprintf(pSenInfo->inDataClass.pInBaseDataArray[3].psData, bufSize,
			"{\"n\":\"AutoReport\",\"bv\":0,\"asm\":\"rw\"}");
	}
	pSenInfo->inDataClass.pInBaseDataArray[3].iSizeData = strlen(pSenInfo->inDataClass.pInBaseDataArray[3].psData);

	return 0;
}

static void freeSenData(InSenData *pSenData)
{
	int i;

	ADV_TRACE("%s: \n", __func__);
	if (NULL != pSenData) {
		for(i=0; i < pSenData->inDataClass.iTypeCount; i++) {
			if (NULL != pSenData->inDataClass.pInBaseDataArray[i].psType)
				free(pSenData->inDataClass.pInBaseDataArray[i].psType);	
			if (NULL != pSenData->inDataClass.pInBaseDataArray[i].psData)
				free(pSenData->inDataClass.pInBaseDataArray[i].psData);	
		}
		if (NULL != pSenData->inDataClass.pInBaseDataArray) {
			free(pSenData->inDataClass.pInBaseDataArray);
		}

		if (NULL != pSenData->pExtened)
			free(pSenData->pExtened);
		
		memset(pSenData, 0, sizeof(InSenData));
	}
}

int fillSenData(InSenData *pSenData, Mote_Info_t *pMote)
{
	int i;
	int bufSize;
	char senDataStr[MAX_SENDATA_LEN];
	int totalSensor;
	Sensor_Info_t *pSensor;
	char *NeighborMacList;

	if (NULL==pSenData || NULL==pMote) {
		ADV_ERROR("Fill SenData failed: NULL\n");
		return -1;
	}

	if (pMote->MoteQueryState != Mote_Query_Observe_Started) {
		ADV_ERROR("Error: fillSenData - Mote does not receive observe data yet\n");
		return -1;
	}

	totalSensor = pMote->sensorList.total;

	freeSenData(pSenData);

	snprintf(pSenData->sUID, sizeof(pSenData->sUID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);

	pSenData->inDataClass.iTypeCount = 2;
	pSenData->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pSenData->inDataClass.iTypeCount);
	pSenData->inDataClass.pInBaseDataArray[0].psType = strdup("SenData");
	pSenData->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pSenData->inDataClass.pInBaseDataArray[0].psType);

	bufSize = totalSensor * MAX_SENDATA_LEN;
	pSenData->inDataClass.pInBaseDataArray[0].psData = malloc(bufSize);
	if (NULL == pSenData->inDataClass.pInBaseDataArray[0].psData)
		return -1;
	memset(pSenData->inDataClass.pInBaseDataArray[0].psData, 0, bufSize);

	for (i=0; i<totalSensor; i++) {
		if (i != 0)
			strncat(pSenData->inDataClass.pInBaseDataArray[0].psData, ",", 1);
		pSensor = &pMote->sensorList.item[i];
		memset(senDataStr, 0, sizeof(senDataStr));
		if (strncmp(pSensor->valueType, "i", strlen(pSensor->valueType)) == 0 || 
			strncmp(pSensor->valueType, "d", strlen(pSensor->valueType)) == 0) {
			snprintf(senDataStr, sizeof(senDataStr),
					"{\"n\":\"%s\", \"v\":%s}",
					pSensor->name, pSensor->value
					);			
		} else if (strncmp(pSensor->valueType, "b", strlen(pSensor->valueType)) == 0) {
			snprintf(senDataStr, sizeof(senDataStr),
					"{\"n\":\"%s\", \"bv\":%s}",
					pSensor->name, pSensor->value
					);
		} else if (strncmp(pSensor->valueType, "s", strlen(pSensor->valueType)) == 0 || 
					strncmp(pSensor->valueType, "h", strlen(pSensor->valueType)) == 0 ||
					strncmp(pSensor->valueType, "o", strlen(pSensor->valueType)) == 0) {
			snprintf(senDataStr, sizeof(senDataStr),
					"{\"n\":\"%s\", \"s\":%s}",
					pSensor->name, pSensor->value
					);
		} else if (strncmp(pSensor->valueType, "e", strlen(pSensor->valueType)) == 0) {
			char valueStr[MAX_VALUE_LEN];
			char svStr[MAX_VALUE_LEN];
			char tmpStr[MAX_VALUE_LEN];
			char *pDelim = " ";
			char *pTok;
			int itemNum = 0;

			memset(valueStr, 0, sizeof(valueStr));
			memset(svStr, 0, sizeof(svStr));
			memset(tmpStr, 0, sizeof(tmpStr));

			strcat(svStr, "\"[");

			memcpy(valueStr, pSensor->value, sizeof(valueStr));
			pTok = strtok(valueStr, pDelim);
			while (pTok != NULL) {
				itemNum++;
				memset(tmpStr, 0, sizeof(tmpStr));
				switch (itemNum) {
					case 1:
						snprintf(tmpStr, sizeof(tmpStr), "{\\\"n\\\":\\\"x\\\",\\\"v\\\":%s}", pTok);
						break;
					case 2:
						snprintf(tmpStr, sizeof(tmpStr), ",{\\\"n\\\":\\\"y\\\",\\\"v\\\":%s}", pTok);
						break;
					case 3:
						snprintf(tmpStr, sizeof(tmpStr), ",{\\\"n\\\":\\\"z\\\",\\\"v\\\":%s}", pTok);
						break;
					default:
						break;
				}
				strcat(svStr, tmpStr);
				pTok = strtok(NULL, pDelim);
			}

			strcat(svStr, "]\"");
			
			snprintf(senDataStr, sizeof(senDataStr),
					"{\"n\":\"%s\", \"sv\":%s}",
					pSensor->name, svStr
					);
		} else {
			ADV_TRACE("Unknown Sensor Value Type\n");
			continue;
		}
		strncat(pSenData->inDataClass.pInBaseDataArray[0].psData, senDataStr, strlen(senDataStr));
	}

	pSenData->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pSenData->inDataClass.pInBaseDataArray[0].psData);

	// psType = Net
	pSenData->inDataClass.pInBaseDataArray[1].psType = strdup("Net");
	pSenData->inDataClass.pInBaseDataArray[1].iSizeType = sizeof(pSenData->inDataClass.pInBaseDataArray[1].psType);

	bufSize = MAX_SENDATA_LEN;
	pSenData->inDataClass.pInBaseDataArray[1].psData = malloc(bufSize);
	if (NULL == pSenData->inDataClass.pInBaseDataArray[1].psData)
		return -1;
	memset(pSenData->inDataClass.pInBaseDataArray[1].psData, 0, bufSize);

	NeighborMacList = malloc(512);
	if (NULL == NeighborMacList)
		return -2;
	memset(NeighborMacList, 0, 512);
	getNeighborMacList(pMote, NeighborMacList);

	snprintf(pSenData->inDataClass.pInBaseDataArray[1].psData, bufSize,
			"{\"n\":\"Health\", \"v\":%d},{\"n\":\"Neighbor\", \"sv\":\"%s\"}",
			pMote->reliability, NeighborMacList);
	pSenData->inDataClass.pInBaseDataArray[1].iSizeData = strlen(pSenData->inDataClass.pInBaseDataArray[1].psData);
	free(NeighborMacList);

	return 0;
}

static void freeSenInfoAction(InSenData *pSenData)
{
	int i;

	ADV_TRACE("%s: \n", __func__);
	if (NULL != pSenData) {
		for(i=0; i < pSenData->inDataClass.iTypeCount; i++) {
			if (NULL != pSenData->inDataClass.pInBaseDataArray[i].psType)
				free(pSenData->inDataClass.pInBaseDataArray[i].psType);	
			if (NULL != pSenData->inDataClass.pInBaseDataArray[i].psData)
				free(pSenData->inDataClass.pInBaseDataArray[i].psData);	
		}
		if (NULL != pSenData->inDataClass.pInBaseDataArray) {
			free(pSenData->inDataClass.pInBaseDataArray);
		}

		if (NULL != pSenData->pExtened)
			free(pSenData->pExtened);
		
		memset(pSenData, 0, sizeof(InSenData));
	}
}

int fillSenInfoAction(InSenData *pSenData, Mote_Info_t *pMote)
{
	int bufSize;

	if (NULL==pSenData || NULL==pMote) {
		ADV_TRACE("Fill SenInfoSpec failed: NULL\n");
		return -1;
	}

	freeSenInfoAction(pSenData);

	snprintf(pSenData->sUID, sizeof(pSenData->sUID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);

	pSenData->inDataClass.iTypeCount = 1;
	// psType = AutoReport
	pSenData->inDataClass.pInBaseDataArray = (InBaseData *)malloc(sizeof(InBaseData) * pSenData->inDataClass.iTypeCount);
	pSenData->inDataClass.pInBaseDataArray[0].psType = strdup("Action");
	pSenData->inDataClass.pInBaseDataArray[0].iSizeType = sizeof(pSenData->inDataClass.pInBaseDataArray[0].psType);

	bufSize = MAX_SENDATA_LEN;
	pSenData->inDataClass.pInBaseDataArray[0].psData = malloc(bufSize);
	if (NULL == pSenData->inDataClass.pInBaseDataArray[0].psData)
		return -1;
	memset(pSenData->inDataClass.pInBaseDataArray[0].psData, 0, bufSize);

	if(pMote->MoteQueryState == Mote_Query_Observe_Started) {
		snprintf(pSenData->inDataClass.pInBaseDataArray[0].psData, bufSize,
			"{\"n\":\"AutoReport\",\"bv\":1,\"asm\":\"rw\"}");
	} else {
		snprintf(pSenData->inDataClass.pInBaseDataArray[0].psData, bufSize,
			"{\"n\":\"AutoReport\",\"bv\":0,\"asm\":\"rw\"}");
	}
	pSenData->inDataClass.pInBaseDataArray[0].iSizeData = strlen(pSenData->inDataClass.pInBaseDataArray[0].psData);

	return 0;
}

// CmdID = 2002
int Susiaccess_Report_SensorData(Mote_Info_t *pMote)
{
	if(g_UpdateDataCbf == NULL) {
		ADV_ERROR("%s: g_UpdateDataCbf is NULL\n", __func__);
		return -1;
	}
	fillSenData(&g_SenData, pMote);
	ADV_ERROR("%s: %s moteId=%d\n", __func__, DUSTWSN_CONTYPE, pMote->moteId);
	g_UpdateDataCbf(SN_SenHub_AutoReportData, &g_SenData, sizeof(InSenData), g_UserData, NULL, NULL, NULL);
}

// CmdID = 2003
int Susiaccess_Report_MoteLost(Mote_Info_t *pMote)
{
	if (NULL == pMote) {
		ADV_ERROR("%s: Mote lost report failed: NULL\n", __func__);
		return -1;
	}

	if(g_UpdateDataCbf == NULL) {
		ADV_ERROR("%s: g_UpdateDataCbf is NULL\n", __func__);
		return -1;
	}
	snprintf(g_SenData.sUID, sizeof(g_SenData.sUID),
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			pMote->macAddress[0], pMote->macAddress[1], pMote->macAddress[2], pMote->macAddress[3],
			pMote->macAddress[4], pMote->macAddress[5], pMote->macAddress[6], pMote->macAddress[7]
			);
	g_SenData.inDataClass.iTypeCount = 0;
	g_SenData.pExtened = NULL;

	g_UpdateDataCbf(SN_SenHub_Disconnect, &g_SenData, sizeof(InSenData), g_UserData, NULL, NULL, NULL);	

	return 0;
}

int Susiaccess_Report_UpdateAutoReport(Mote_Info_t *pTargetMote)
{
	if(g_UpdateDataCbf == NULL) {
		ADV_TRACE("%s: g_UpdateDataCbf is NULL\n", __func__);
		return -1;
	}
	fillSenInfoAction(&g_SenInfoAction, pTargetMote);
	g_UpdateDataCbf(SN_SenHub_AutoReportData, &g_SenInfoAction, sizeof(InSenData), g_UserData, NULL, NULL, NULL);
}

// Private function
void trimDQ(char * const a)
{
	char *p = a, *q = a;
	while (*q == '\"') ++q;
	while (*q) *p++ = *q++;
	*p = '\0';
	--p;
	while (p > a && (*p)=='\"' || (*p)==' ') {
		*p = '\0';
		--p;
	}
}

void MacStrToHex(unsigned char* _payload, unsigned char *_str)
{
    sscanf( _str, "%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx%2hhx", &_payload[0], &_payload[1], &_payload[2], &_payload[3], &_payload[4], &_payload[5], &_payload[6], &_payload[7]);
}

void GetJsonN(unsigned char* _name, unsigned char *_str)
{
	sscanf( _str, "%*[^:]:\"%[^\"]%*2c", _name);
}

void GetJsonNV(unsigned char* _name, unsigned char* _val, unsigned char *_str)
{
	sscanf( _str, "%*[^:]:\"%[^\"]%*2c%*[^:]:%[^}]", _name, _val);
	// remove double quotes ""
	trimDQ(_val);
}

int GetInfData(unsigned char *_sUID, InBaseData *_input, OutBaseData *output)
{
	unsigned char ucName[32];
	char buffer[1024];

	if(strcmp(g_InfInfo.sInfID, _sUID) != 0) {
		return -1;
	}
	if( strcmp( "Info", _input->psType) != 0 ) {
		return -2;
	}

	ADV_INFO("%s: name(%s), Relibility(%d)\n", __func__, g_InfInfo.sInfName, g_DustWsnContext.SmartMeshIpInfo.NetReliability);

	memset(ucName, 0, sizeof(ucName));
	GetJsonN((unsigned char*)ucName, _input->psData);

	if(strcmp("Health", ucName) == 0) {
		sprintf(buffer, "{\"n\":\"%s\", \"v\":%d}", ucName, g_DustWsnContext.SmartMeshIpInfo.NetReliability);
	} else if(strcmp("Name", ucName) == 0) {
        sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}", ucName, g_InfInfo.sInfName);
	}

	if(*output->iSizeData < strlen(buffer)+1) {
		*output->iSizeData = strlen(buffer)+1;
		output->psData = (char *)realloc(output->psData, *output->iSizeData);
	}
	strcpy(output->psData, buffer);

	return SN_OK;
}

int GetSenData(unsigned char *_sUID, InBaseData *_input, OutBaseData *output)
{
	Mote_Info_t *pMoteInfo;
	unsigned char ucMacAddr[8];
	unsigned char ucName[32];
	char buffer[1024];

	if( strcmp( "Info", _input->psType) != 0 && strcmp( "SenData", _input->psType) != 0) {
		return -1;
	}
	MacStrToHex((unsigned char*)&ucMacAddr, _sUID);
	pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, ucMacAddr);
	if (NULL == pMoteInfo) {
		ADV_ERROR("%s: Mote info not found!\n", __func__);
		return -2;
	}

	ADV_INFO("%s:psType(%s), manufacturer(%d), hostname(%s), moteId(%d), isAP(%d), Relibility(%d)\n", __func__, _input->psType, pMoteInfo->manufacturer, pMoteInfo->hostName, pMoteInfo->moteId, pMoteInfo->isAP, pMoteInfo->reliability);
	if(pMoteInfo->MoteQueryState < Mote_Query_SensorList_Received) {
		ADV_ERROR("%s: Mote not ready !\n", __func__);
		return -3;
	}

	memset(ucName, 0, sizeof(ucName));
	GetJsonN((unsigned char*)ucName, _input->psData);

	if(strcmp("Health", ucName) == 0) {
		sprintf(buffer, "{\"n\":\"%s\", \"v\":%d}", ucName, pMoteInfo->reliability);
	} else if(strcmp("Name", ucName) == 0) {
        sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}", ucName, pMoteInfo->hostName);
	}

	if(*output->iSizeData < strlen(buffer)+1) {
		*output->iSizeData = strlen(buffer)+1;
		output->psData = (char *)realloc(output->psData, *output->iSizeData);
	}
	strcpy(output->psData, buffer);

	return SN_OK;
}

int SetInfData(unsigned char *_sUID, InBaseData *_input, OutBaseData *output)
{
	unsigned char ucName[32];
	unsigned char ucVal[32];
	char buffer[1024];
	int ival = 0;

	ADV_INFO("%s: sUID/InfID = %s/%s, psType = %s, name = %s\n", __func__, _sUID, g_InfInfo.sInfID, _input->psType, g_InfInfo.sInfName);

	if(strcmp(g_InfInfo.sInfID, _sUID) != 0) {
		return -1;
	}

	if( strcmp( "Info", _input->psType) != 0 ) {
		return -2;
	}

	memset(ucName, 0, sizeof(ucName));
	memset(ucVal, 0, sizeof(ucVal));
	GetJsonNV((unsigned char*)ucName, (unsigned char*)ucVal, _input->psData);

	ADV_INFO("%s: psData=%s ucName=%s, ucVal(%d)=%s\n", __func__, _input->psData, ucName, strlen(ucVal), ucVal);
	// Set Interface 
	if(strcmp("reset", ucName) == 0) {
		ival = atoi(ucVal);
		if(ival == 1) {
			IpMgWrapper_MoteSetMoteReset(g_MgrInfo);
		}
		sprintf(buffer, "{\"n\":\"%s\",\"bv\":%d}", ucName, ival);
	} else if(strcmp("Name", ucName) == 0) {
        sprintf(buffer, "{\"n\":\"%s\", \"sv\":\"%s\"}", ucName, g_InfInfo.sInfName);
	}

	if(*output->iSizeData < strlen(buffer)+1) {
		*output->iSizeData = strlen(buffer)+1;
		output->psData = (char *)realloc(output->psData, *output->iSizeData);
	}
	strcpy(output->psData, buffer);

	return SN_OK;
}

int SetSenData(unsigned char *_sUID, InBaseData *_input, OutBaseData *output)
{
	Mote_Info_t *pMoteInfo;
	unsigned char ucMacAddr[8];
	unsigned char ucName[32];
	unsigned char ucVal[32];
	char buffer[1024];

	if( strcmp( "Info", _input->psType) != 0 &&
			strcmp( "SenData", _input->psType) != 0 &&
			strcmp( "Action", _input->psType) != 0) {
		return -1;
	}
	MacStrToHex((unsigned char*)&ucMacAddr, _sUID);
	pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, ucMacAddr);
	if (NULL == pMoteInfo) {
		ADV_ERROR("%s: Mote info not found!\n", __func__);
		return -2;
	}
	ADV_INFO("%s:psType(%s), manufacturer(%d), hostname(%s), moteId(%d), isAP(%d), Relibility(%d)\n", __func__, _input->psType, pMoteInfo->manufacturer, pMoteInfo->hostName, pMoteInfo->moteId, pMoteInfo->isAP, pMoteInfo->reliability);

	// Set SenData
	memset(ucName, 0, sizeof(ucName));
	memset(ucVal, 0, sizeof(ucVal));
	GetJsonNV((unsigned char*)ucName, (unsigned char*)ucVal, _input->psData);

	ADV_INFO("%s: psData=%s ucName=%s, ucVal(%d)=%s\n", __func__, _input->psData, ucName, strlen(ucVal), ucVal);
	if(pMoteInfo->manufacturer != Mote_Manufacturer_Dust) {
	if(pMoteInfo->MoteQueryState == Mote_Query_Observe_Started) {
		if(strcmp("AutoReport", ucName) != 0) {
			ADV_ERROR("%s: Mote is at Observe state, please cancel observe first!\n", __func__);
			return -3;
		}
	} else if(pMoteInfo->MoteQueryState != Mote_Query_SensorList_Received &&
			pMoteInfo->sensorList.total != 0 && 
			pMoteInfo->scannedSensor == pMoteInfo->sensorList.total) {
		ADV_ERROR("%s: Mote not ready for set!\n", __func__);
		return -4;
	}
	}

	memset(pMoteInfo->cmdName, 0, sizeof(pMoteInfo->cmdName));
	memset(pMoteInfo->cmdParam, 0, sizeof(pMoteInfo->cmdParam));
	memset(buffer, 0, sizeof(buffer));
	// Set SenHub name
	if(strcmp( "Info", _input->psType) == 0) {
		if(strcmp("Name", ucName) == 0) {
			ADV_INFO("%s: Set sensor hub name\n", __func__);
			//  send cmd to set sensor hub name
			sprintf(buffer, "{\"n\":\"%s\",\"sv\":\"%s\"}", ucName, ucVal);
			pMoteInfo->cmdType = Mote_Cmd_SetMoteName;
		} else if(strcmp("reset", ucName) == 0 && strlen(ucVal)>0) {
			ADV_INFO("%s: reset sensor hub\n", __func__);
			//  send cmd to reset sensor hub 
			sprintf(buffer, "{\"n\":\"%s\",\"bv\":%d}", ucName, atoi(ucVal));
			pMoteInfo->cmdType = Mote_Cmd_SetMoteReset;
		} else {
			ADV_INFO("%s: not define this cmd\n", __func__);
			pMoteInfo->cmdType = Mote_Cmd_None;
		}
	} else if(strcmp( "SenData", _input->psType) == 0) {
		ADV_INFO("%s: Set %s to %s\n", __func__, ucName, ucVal);
		sprintf(buffer, "{\"n\":\"%s\",\"bv\":%d}", ucName, atoi(ucVal));
		pMoteInfo->cmdType = Mote_Cmd_SetSensorValue;
	} else if(strcmp( "Action", _input->psType) == 0) {
		if(strcmp("AutoReport", ucName) == 0) {
			ADV_INFO("%s: Set %s to %s\n", __func__, ucName, ucVal);
			sprintf(buffer, "{\"n\":\"%s\",\"bv\":%d}", ucName, atoi(ucVal));
			pMoteInfo->cmdType = Mote_Cmd_SetAutoReport;
		} else {
			ADV_INFO("%s: not define this cmd\n", __func__);
			pMoteInfo->cmdType = Mote_Cmd_None;
		}
	} else {
		ADV_INFO("%s: not define this cmd\n", __func__);
		pMoteInfo->cmdType = Mote_Cmd_None;
	}

	if(pMoteInfo->cmdType != Mote_Cmd_None) {
       	strcpy(pMoteInfo->cmdName, ucName);
       	strcpy(pMoteInfo->cmdParam, ucVal);
		pMoteInfo->MoteQueryState = Mote_Query_SendCmd_Sent;
	} else {
		return SN_ER_FAILED;
	}

	if(*output->iSizeData < strlen(buffer)+1) {
		*output->iSizeData = strlen(buffer)+1;
		output->psData = (char *)realloc(output->psData, *output->iSizeData);
	}
	strcpy(output->psData, buffer);

	return SN_OK;
}

//-----------------------------------------------------------------------------
// SensorNetwork API
//-----------------------------------------------------------------------------
SN_CODE SNCALL SN_Initialize( void *pInUserData )
{
	Mote_Info_t *pMote;
	g_UserData = pInUserData;
	
	// init global variable
	memset(&g_SNInfData, 0, sizeof(SNInterfaceData));
	memset(&g_SenInfoSpec, 0, sizeof(InSenData));
	memset(&g_SenData, 0, sizeof(InSenData));
	g_pMoteToScan = NULL;
	g_UpdateDataCbf = NULL;
	g_doUpdateInterface = FALSE;
	g_doSensorScan = TRUE;
	g_GetCapability = FALSE;
	memset(&g_DustWsnContext, 0, sizeof(DustWsn_Context_t));
	g_DustWsnContext.thread_nr = 0;

	// Create mutex
	if(pthread_mutex_init(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex, NULL) != 0){
		//MonitorLog(g_loghandle, Normal, " %s> Create HWMInfo mutex failed!", MyTopic);
		ADV_ERROR("Create Mote List mutex failed\n");
		return SN_ER_FAILED;
	}

	// check manager connection.
	#ifdef USE_SERIALMUX
	IpMgWrapper_Init(DUSTWSN_SERIALMUX_PORT);
	#else
	IpMgWrapper_Init(DUSTWSN_DEV_NODE);
	#endif
	
	ADV_TRACE("%s: setup done\n", __func__);

	if (!g_MgrConnected) {
		return SN_ER_FAILED;
	}
	
	g_ManagerSWVer = malloc(sizeof(char)*MAX_SOTFWAREVERSION_LEN);
	IpMgWrapper_GetManagerSWVersion(g_ManagerSWVer);
	//printf("Software Ver: %s\n", g_ManagerSWVer);

	ADV_TRACE("%s: search done\n", __func__);
	printMoteList();
	

	if (pthread_create(&g_DustWsnContext.threadHandler, NULL, DustWsnThread, &g_DustWsnContext) != 0) {
		
		g_DustWsnContext.isThreadRunning = FALSE;
		//MonitorLog(g_loghandle, Normal, " %s> start handler thread failed!", MyTopic);	
		return SN_ER_FAILED;
	}
	g_DustWsnContext.thread_nr++;
	g_DustWsnContext.isThreadRunning = TRUE;
	//ADV_INFO("%s: create pthread done\n", __func__);
	usleep(5000);
	
	return SN_OK;
}

SN_CODE SNCALL SN_Uninitialize( void *pInParam )
{
	if (g_DustWsnContext.isThreadRunning) {
		//MonitorLog(g_loghandle, Normal, " %s> Wait Trhead stop!", MyTopic);	
		g_DustWsnContext.isThreadRunning = FALSE;
		pthread_join(g_DustWsnContext.threadHandler, NULL);
		g_DustWsnContext.threadHandler = NULL;
	}

	IpMgWrapper_UnInit();

	// Release mutex
	pthread_mutex_destroy(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	// free memory
	//pthread_mutex_lock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);
	MoteList_RemoveList(&g_DustWsnContext.SmartMeshIpInfo.MoteList);
	//pthread_mutex_unlock(&g_DustWsnContext.SmartMeshIpInfo.moteListMutex);

	// free global variable
	freeSNInfData(&g_SNInfData);
	freeSenInfoSpec(&g_SenInfoSpec);
	freeSenData(&g_SenData);
	if(g_ManagerSWVer != NULL) free(g_ManagerSWVer);

	return SN_OK;
}

SN_CODE SNCALL SN_GetCapability( SNMultiInfInfo *pOutSNMultiInfInfo )
{
	if (NULL == pOutSNMultiInfInfo || !g_MgrConnected)
		return SN_ER_FAILED;
	
	ADV_INFO("%s: \n", __func__);
	fillSNInfInfos(pOutSNMultiInfInfo);
	strcpy(g_InfInfo.sInfName, pOutSNMultiInfInfo->SNInfs[0].sInfName);
	strcpy(g_InfInfo.sInfID, pOutSNMultiInfInfo->SNInfs[0].sInfID);
	g_doUpdateInterface = TRUE;
	g_GetCapability = TRUE;

	return SN_OK;
}

void SNCALL SN_SetUpdateDataCbf( UpdateSNDataCbf UpdateDataCbf )
{
	ADV_INFO("%s...........................................!\n", __func__);
	g_UpdateDataCbf = UpdateDataCbf;
}

SN_CODE SNCALL SN_GetVersion( char *psVersion, int BufLen )
{
	if(BufLen < sizeof(g_ManagerSWVer)) {
		ADV_ERROR("Not enough memory for buffer!\n");
		return 0;
	}
	memcpy(psVersion, g_ManagerSWVer, sizeof(char)*MAX_SOTFWAREVERSION_LEN);
	return SN_OK;
}

SN_CODE SNCALL SN_GetData(SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData) 
{
	int ret = SN_ER_FAILED;
	int i = 0;
	InBaseData *input;
	OutBaseData *output;

	if(pInSenData->inDataClass.iTypeCount != pOutSenData->outDataClass.iTypeCount) 
		return SN_ER_VALUE_OUT_OF_RNAGE;

	ADV_INFO("%s: sUID = %s, CmdID = %d\n", __func__, pInSenData->sUID, CmdID);

	switch(CmdID) {
		// 6000
		case SN_Inf_Get:
		{
			// psType =  Info?
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				if(GetInfData(pInSenData->sUID, input, output) == SN_OK) {
					ADV_TRACE("1. input->psData = %s\n", input->psData);
					ADV_TRACE("2. output->psData = %s\n", output->psData);
				}
			}
			ret = SN_OK;
		}
	   	break;
		
		// 6020
		case SN_SenHub_Get:
		{
			// psType =  Info or SenData?
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				if(GetSenData(pInSenData->sUID, input, output) == SN_OK) {
					ADV_TRACE("1. input->psData = %s\n", input->psData);
					ADV_TRACE("2. output->psData = %s\n", output->psData);
				}
			}
			ret = SN_OK;
		}
	   	break;
		
		default:
			ADV_ERROR("@@@@@@@@@@@@@@ Unknow command - %d\n", CmdID);
			ret = SN_ER_NOT_IMPLEMENT;
		break;
	}
	return (SN_CODE)ret;
}

SN_CODE SNCALL SN_SetData( SN_CTL_ID CmdID, InSenData *pInSenData, OutSenData *pOutSenData )
{
	int ret = SN_ER_FAILED;
	int i = 0;
	InBaseData *input;
	OutBaseData *output;

	ADV_INFO("%s: sUID = %s, CmdID = %d, inTypeCount = %d, outTypeCount = %d\n", __func__, pInSenData->sUID, CmdID, pInSenData->inDataClass.iTypeCount, pOutSenData->outDataClass.iTypeCount);

	if(pInSenData->inDataClass.iTypeCount != pOutSenData->outDataClass.iTypeCount)
		return SN_ER_VALUE_OUT_OF_RNAGE;

	switch(CmdID) {
		// 6001
		case SN_Inf_Set:
		{
			// psType =  Info?
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				if(SetInfData(pInSenData->sUID, input, output) == SN_OK) {
					ADV_TRACE("1. input->psData = %s\n", input->psData);
					ADV_TRACE("2. output->psData = %s\n", output->psData);
				}
			}
			ret = SN_OK;
		}
	   	break;
		
		// 6021
		case SN_SenHub_Set:
		{
			// psType =  Info or SenData?
			for(i = 0 ; i < pInSenData->inDataClass.iTypeCount ; i++) {
				input = &pInSenData->inDataClass.pInBaseDataArray[i];
				output = &pOutSenData->outDataClass.pOutBaseDataArray[i];
				
				if(SetSenData(pInSenData->sUID, input, output) == SN_OK) {
					ADV_TRACE("1. input->psData = %s\n", input->psData);
					ADV_TRACE("2. output->psData = %s\n", output->psData);
				}
			}
			ret = SN_OK;
		}
	   	break;
		
		default:
			ADV_ERROR("@@@@@@@@@@@@@@ Unknow command - %d\n", CmdID);
			ret = SN_ER_NOT_IMPLEMENT;
		break;
	}

	return (SN_CODE)ret;
}

/*
int SNCALL SN_GetStatus( WSNDRV_SERVICE_STATUS * pOutStatus )		// for detecting lock/busy in DustWsnThread
{
	return 0;
} */
