/*
Copyright (c) 2014, Advantech.  All rights reserved.

C library to connect to a SmartMesh IP manager.

This library is a "wrapper" around the generic SmartMesh C library.

This library will:
- Connect to the SmartMesh IP manager.
- Subscribe to data notifications.
- Get the MAC address of all nodes in the network.
- Send an OAP command to each node.
  
*/
#include <unistd.h>
#include "IpMgWrapper.h"
#include "dn_ipmg.h"
#ifdef USE_SERIALMUX
#include "dn_socket.h"
#else
#include "dn_uart.h"
#endif
#include "dn_serial_mg.h"
#include "MoteListAccess.h"
#include "adv_coap.h"
#include "adv_payload_parser.h"
#include "uri_cmd.h"
#include "ipso_id.h"
#include "SensorNetwork_API.h"
#ifdef ENABLE_PARKING
#include "alg_parking.h"
#endif
#include "AdvLog.h"

#if defined(WIN32) //windows
#include "AdvPlatform.h"
#endif
//=========================== define ==========================================
//#define SHOW_LIST

//app_vars.status
#define ST_SEARCH				0
#define ST_FINISH				1
#define DN_UDP_PORT_ADVMOTE		60002

#define MAX_MGR_CONNECT_RETRY	3

//#define IO_SENSOR_NUM           4	
#define IO_SENSOR_NUM           12	

typedef struct {
	// fsm
	TIME_T               fsmPreviousEvent;
	BOOL                 fsmArmed;
	TIME_T               fsmDelay;
	fsm_timer_callback   fsmCb;
	// module
#ifdef USE_SERIALMUX
	dn_socket_rxByte_cbt   ipmg_socket_rxByte_cb;
#else
	dn_uart_rxByte_cbt   ipmg_uart_rxByte_cb;
#endif
	// reply
	fsm_reply_callback   replyCb;
	// api
	uint8_t              currentMac[8];
	uint8_t              replyBuf[MAX_FRAME_LENGTH];        // holds notifications from ipmg
	uint8_t              notifBuf[MAX_FRAME_LENGTH];        // notifications buffer internal to ipmg
#ifdef USE_SERIALMUX
	unsigned char        ipaddr[16];
	uint16_t             port;
#else
	uint8_t              comport[32];
#endif
	int                  status;
	uint16_t             pathId;
} app_vars_t;

//=========================== variables =======================================
extern DustWsn_Context_t	g_DustWsnContext;
//extern SenData				g_SenData;
extern Mote_Info_t* 		g_pMoteToScan;
extern BOOL					g_doSensorScan;
extern BOOL					g_doUpdateInterface;

static char 				g_ManagerSWVer[12]; // format: x.x.x.x
static Mote_Info_t*         g_pCurrentMote = NULL;
static int                  g_IOMoteId=0;
static int                  g_MgrReConnectTimes = 0;

app_vars_t              	app_vars;
char mode[]={'8','N','1',0};
BOOL g_MgrConnected = FALSE;
BOOL g_MgrReConnect = FALSE;

// OAP define
#define OAP_NOTIF_TEMP_MSG_LEN			27
#define OAP_NOTIF_TEMP_ADDR_LEN			1
#define OAP_NOTIF_TEMP_ADDR_VALUE		5
#define OAP_NOTIF_TEMP_VALUE_ADDR_1		25
#define OAP_NOTIF_TEMP_VALUE_ADDR_2		26
#define OAP_NOTIF_PUT_RESP_LEN			8

// Parking define
#ifdef ENABLE_PARKING
#define PARKING_MODEL_NAME			"WISE-3010"
#define PARKING_SENSOR_MAGNETIC_SID	"3314"
#define PARKING_SENSOR_DIN_NAME		"parking_din"
#define PARKING_SENSOR_DIN_MODEL	"iParking"
#define PARKING_SENSOR_DIN_SID		"3200"
#endif


//=========================== prototypes ======================================
void dn_ipmg_notif_cb(uint8_t cmdId, uint8_t subCmdId);
void dn_ipmg_reply_cb(uint8_t cmdId);
void dn_ipmg_status_cb(uint8_t newStatus);

unsigned long long millis() {
	struct timeval te; 
	long long milliseconds;
	gettimeofday(&te, NULL); // get current time
	milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
	return milliseconds;
}

//=========================== public ==========================================

int IpMgWrapper_getIOMoteId(void)
{
	return g_IOMoteId;
}

int IpMgWrapper_Scheduling(void)
{
	return app_vars.fsmArmed;
}

void IpMgWrapper_done(void) {
	ADV_TRACE("%s !\n", __func__);
}

int IpMgWrapper_MoteGetMotePath(Mote_Info_t *pMote)
{
	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));
	g_pCurrentMote = pMote;
#if 0
	ADV_INFO("%s: app_vars on ", __func__);
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
	ADV_INFO("\n");
#endif
	if(pMote->state == MOTE_STATE_LOST) {
		pMote->neighborList.total = 0;
		pMote->bPathUpdate = FALSE;
		return 0;
	}
	app_vars.pathId = 0;
	pMote->neighborList.total = 0;

	IpMgWrapper_fsm_scheduleEvent(
    	INTER_FRAME_PERIOD,
		&IpMgWrapper_api_getNextPathInfo
	);

	return 0;
}

int IpMgWrapper_MoteGetMoteInfo(Mote_Info_t *pMote)
{
	if(pMote == NULL)
		return -1;

	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));
	g_pCurrentMote = pMote;

	IpMgWrapper_fsm_scheduleEvent(
		INTER_FRAME_PERIOD,
		&IpMgWrapper_api_getMoteInfo
	);
	while(IpMgWrapper_Scheduling()) {
		usleep(100);
		IpMgWrapper_loop();
	};
	usleep(3000);
	//IpMgWrapper_api_getSenHubInfo();
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_GET_SENSORHUB_INFO, 0, 0);

	return 0;
}

int IpMgWrapper_MoteGetSensorIndex(Mote_Info_t *pMote)
{
	usleep(500);

	//IpMgWrapper_api_getSensorIndex();
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_GET_SENSOR_INDEX, 0, 0);
#if 0
	IpMgWrapper_fsm_scheduleEvent(
		1000,
		&IpMgWrapper_api_getSensorIndex
	);
#endif

	return 0;
}

int IpMgWrapper_MoteGetSensorInfo(Mote_Info_t *pMote, int sensorIndex)
{

	//IpMgWrapper_api_getSensorInfo(sensorIndex);
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_GET_SENSOR_INFO, sensorIndex, 0);

	return 0;
}

int IpMgWrapper_MoteGetSensorStandard(Mote_Info_t *pMote, int sensorIndex)
{
	usleep(500);

	time(&pMote->sensorList.item[sensorIndex].QuerySentTime);
	//IpMgWrapper_api_getSensorStandard(sensorIndex, atoi(pMote->sensorList.item[sensorIndex].objId));
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_GET_SENSOR_STANDRAD, sensorIndex, atoi(pMote->sensorList.item[sensorIndex].objId));

	return 0;
}

int IpMgWrapper_MoteGetSensorHardware(Mote_Info_t *pMote, int sensorIndex)
{
	usleep(500);

	time(&pMote->sensorList.item[sensorIndex].QuerySentTime);
	//IpMgWrapper_api_getSensorHardware(sensorIndex);
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_GET_SENSOR_HARDWARE, sensorIndex, 0);

	return 0;
}

int IpMgWrapper_MoteSendDataObserve(Mote_Info_t *pMote)
{
	usleep(500);

	//IpMgWrapper_api_observe();
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_OBSERVE, 0, 0);

	return 0;
}

int IpMgWrapper_MoteSendDataCancelObserve(Mote_Info_t *pMote)
{
	usleep(500);
	//IpMgWrapper_api_cancelObserve();
	IpMgWrapper_api_sendCmd(pMote, SEND_CMD_CANCEL_OBSERVE, 0, 0);

	return 0;
}


int IpMgWrapper_MoteSendDataCmd(Mote_Info_t *pMote)
{
	g_pCurrentMote = pMote;

	ADV_INFO("%s: cmdType=%d\n", __func__, pMote->cmdType);
	switch (pMote->cmdType) {
		case Mote_Cmd_SetSensorValue:
			if(pMote->manufacturer == Mote_Manufacturer_Dust) {
				IpMgWrapper_api_setMasterIOByName(pMote);
			} else {
				//IpMgWrapper_api_setSensorValByName(pMote);
				IpMgWrapper_api_sendCmd(pMote, SEND_CMD_SET_SENSOR_VALUEBYNAME, 0, 0);
			}
			pMote->MoteQueryState = Mote_Query_SendCmd_Started;
			break;
		case Mote_Cmd_SetMoteName:
			//IpMgWrapper_api_setMoteName(pMote);
			IpMgWrapper_api_sendCmd(pMote, SEND_CMD_SET_SENSORHUB_NAME, 0, 0);
			pMote->MoteQueryState = Mote_Query_SendCmd_Started;
			break;
		case Mote_Cmd_SetMoteReset:
			//IpMgWrapper_api_setMoteReset(pMote);
			IpMgWrapper_api_sendCmd(pMote, SEND_CMD_SET_SENSORHUB_RESET, 0, 0);
			pMote->MoteQueryState = Mote_Query_SendCmd_Started;
			break;
		case Mote_Cmd_SetAutoReport:
			IpMgWrapper_api_setMoteAutoReport(pMote);
			break;
		default:
			ADV_ERROR("Warning: Unknown Set command:%d\n", pMote->cmdType);
			break;
	}
	//if (pMote->cmdType != Mote_Cmd_SetAutoReport)
	//	pMote->MoteQueryState = Mote_Query_SendCmd_Started;

	return 0;
}

int IpMgWrapper_MoteSetMoteReset(Mote_Info_t *pMote)
{
	usleep(500);
	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));
	IpMgWrapper_api_resetSystem();
}

/*
int IpMgWrapper_MoteInitMasterGPIO(Mote_Info_t *pMote)
{
	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));
	IpMgWrapper_api_init_master_gpio();

	return 0;
} */

int IpMgWrapper_MoteMasterSetIO(Mote_Info_t *pMote, MOTE_MASTER_IO ioPin, int value)
{
	unsigned char payload[10];

	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));

	payload[0] = 0x05;			// control
	payload[1] = 0x00;			// ID
	payload[2] = 0x02;			// command set
	payload[3] = 0xff;			// Address TLV
	payload[4] = 0x02;          // with length 2
	if (ioPin > Mote_Master_AI) {
		// Analog In Address
		payload[5] = 0x04;
		payload[6] = ioPin - Mote_Master_AI - 1;
	} else if (ioPin > Mote_Master_DO) {
		// Digital Out Address
		payload[5] = 0x03;
		payload[6] = ioPin - Mote_Master_DO - 1;
	} else {	// Mote_Master_DI
		// Digital In Address
		payload[5] = 0x02;
		payload[6] = ioPin - Mote_Master_DI - 1;
	}
	payload[7] = 0x00;			// (T)
	payload[8] = 0x01;			// (L)
	if (1 == value) {
		payload[9] = 0x01;		// (V)
	} else { 
		payload[9] = 0x00;		// (V)
	}
	//ADV_TRACE("sizeof(payload)=%d\n", sizeof(payload));
	IpMgWrapper_Oap_SendData(payload, 10);

	return 0;
}

void IpMgWrapper_Oap_SendData(unsigned char *payload, int payloadLen)
{
	dn_err_t   err;

	ADV_INFO("\nINFO: 	%s MAC=", __func__);
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
	ADV_INFO("... returns ");

	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_sendData_reply);

	err = dn_ipmg_sendData( 	 
		app_vars.currentMac,						// macAddress	   
		0x00,										// priority 	 
		DN_UDP_PORT_OAP,							// srcPort		
		DN_UDP_PORT_OAP,							// dstPort		
		0x00,										// options		
		payload,									// data 	 
		payloadLen,									// dataLen		
		(dn_ipmg_sendData_rpt*)(app_vars.replyBuf)	// reply   
	);		

	ADV_INFO(" %d\n",err);

	IpMgWrapper_fsm_scheduleEvent(		
		SERIAL_RESPONSE_TIMEOUT,	  
		&IpMgWrapper_api_response_timeout	
	);
	
}

int IpMgWrapper_getMotesIdByMac(uint8_t* macAddress)
{
	Mote_Info_t *pMote = NULL;
	Mote_List_t *pMoteList = &g_DustWsnContext.SmartMeshIpInfo.MoteList;

	pMote = MoteList_GetItemByMac(pMoteList, macAddress);
	if (NULL == pMote) {
		ADV_ERROR("%s - target Mote not found\n", __func__);
		return -1;
	}

	return pMote->moteId;
}

uint8_t* IpMgWrapper_GetMotesMacById(int id)
{
	return NULL;
}

#ifdef ENABLE_PARKING
int FillParkingPara(Sensor_Info_t *_pSensorInfo, iParkingPara_t *_pParkingPara)
{
	char valueStr[MAX_VALUE_LEN];
	char *pDelim = " ";
	char *pTok;
	int itemNum = 0;
	
	memset(valueStr, 0, sizeof(valueStr));
	
	memcpy(valueStr, _pSensorInfo->value, sizeof(valueStr));
	pTok = strtok(valueStr, pDelim);
	while (pTok != NULL) {
		itemNum++;
		switch (itemNum) {
			case 1:
				_pParkingPara->tMag.AXIS_X = atoi(pTok);
				//printf("X value=%d\n", _pParkingPara->tMag.AXIS_X);
				break;
			case 2:
				_pParkingPara->tMag.AXIS_Y = atoi(pTok);
				//printf("Y value=%d\n", _pParkingPara->tMag.AXIS_Y);
				break;
			case 3:
				_pParkingPara->tMag.AXIS_Z = atoi(pTok);
				//printf("Z value=%d\n", _pParkingPara->tMag.AXIS_Z);
				break;
			default:
				break;
		}
		pTok = strtok(NULL, pDelim);
	}

	return 0;
}
#endif

void Application_Package_Init()
{
#ifdef ENABLE_PARKING
	int ret = 0;
	ret = PkgAlg_i16Init();
	if (0 != ret)
		ADV_ERROR("Parking Init Failed:%d\n", ret);
#endif
}

void Application_Package_UnInit()
{
#ifdef ENABLE_PARKING
	int ret = 0;
	ret = PkgAlg_i16UnInit();
	if (0 != ret)
		ADV_ERROR("Parking UnInit Failed:%d\n", ret);
#endif
}

void Application_Package_CheckMoteInfo(Mote_Info_t *_pMote)
{

}

void Application_Package_CheckSensorList(Mote_Info_t *_pMote)
{

}

void Application_Package_CheckSensorInfo(Mote_Info_t *_pMote)
{
#ifdef ENABLE_PARKING
	if (0 == strncmp(_pMote->productName, PARKING_MODEL_NAME, strlen(_pMote->productName))) {
		Sensor_List_t *pSensorList = &_pMote->sensorList;
		if (1==pSensorList->total && 0==strncmp(pSensorList->item[0].objId, PARKING_SENSOR_MAGNETIC_SID, strlen(pSensorList->item[0].objId))) {
			strncpy(pSensorList->item[0].name, PARKING_SENSOR_DIN_NAME, sizeof(pSensorList->item[0].name));
			strncpy(pSensorList->item[0].model, PARKING_SENSOR_DIN_MODEL, sizeof(pSensorList->item[0].model));
			strncpy(pSensorList->item[0].objId, PARKING_SENSOR_DIN_SID, sizeof(pSensorList->item[0].objId));
			add_sensor_spec(&pSensorList->item[0]);
			_pMote->AppPackage = Mote_AppPackage_Advantech_iParking;			
		}
	} else {
		ADV_ERROR("Not Advantech Parking Product\n");
	}
#endif
}

Application_Package_CheckSensorData(Mote_Info_t *_pMote)
{
#ifdef ENABLE_PARKING
	if (Mote_AppPackage_Advantech_iParking == _pMote->AppPackage) {
		int ret = 0;
		BOOL result = FALSE;
		Sensor_List_t *pSensorList = &_pMote->sensorList;
		iParkingPara_t iparkingPara;
		memset(&iparkingPara, 0, sizeof(iParkingPara_t));

		ret = FillParkingPara(&pSensorList->item[0], &iparkingPara);
		// handle FillParkingPara() error

		if (Mote_AppPackage_NotInit == _pMote->AppPkgState) {
			ret = PkgAlg_i16NewNode(&iparkingPara);
			if (ret >= 0) {
				_pMote->reserved = ret;
				_pMote->AppPkgState = Mote_AppPackage_Inited;
			}

			strncpy(pSensorList->item[0].value, "0", sizeof(pSensorList->item[0].value)-1);
			return;
		} 

		result = PkgAlg_bDetecting(_pMote->reserved, &iparkingPara);
		//printf("result=%d\n", result);
		if (0 == result)
			strncpy(pSensorList->item[0].value, "0", sizeof(pSensorList->item[0].value)-1);
		else
			strncpy(pSensorList->item[0].value, "1", sizeof(pSensorList->item[0].value)-1);
	}
#endif
}

int IpMgWrapper_GetManagerSWVersion(char *pManagerSWVer)
{
	ADV_INFO("INFO:     %s\n", __func__);

	memcpy(pManagerSWVer, g_ManagerSWVer, sizeof(g_ManagerSWVer));

	return 0;
}

/**
\brief Setting up the instance.
*/
void IpMgWrapper_Init(unsigned char *port)
{
	// reset local variables
	memset(&app_vars,    0, sizeof(app_vars));

	// initialize variables
#ifdef USE_SERIALMUX
	memcpy(app_vars.ipaddr, "127.0.0.1", 9);
	app_vars.port = atoi(port);
#else
	strcpy(app_vars.comport, port);
#endif

	// initialize the ipmg module
	dn_ipmg_init(
		dn_ipmg_notif_cb,                // notifCb
		app_vars.notifBuf,               // notifBuf
		sizeof(app_vars.notifBuf),       // notifBufLen
		dn_ipmg_reply_cb,                // replyCb
		dn_ipmg_status_cb                // statusCb
	);

	// print banner
	ADV_INFO("IpMgWrapper Library, base on SmartMesh C library ver:%d.%d.%d.%d\n",VER_MAJOR,VER_MINOR,VER_PATCH,VER_BUILD);
	ADV_INFO("port = %s\n", port);
	IpMgWrapper_api_initiateConnect();

	while (!g_MgrConnected) {
		//printf("DEBUG:	IpMgWrapper_Init loop continue\n");
		if(g_MgrReConnectTimes < 3) {
			usleep(10000);
			IpMgWrapper_loop();
		} else {
			return;
		}
	}
	
	Application_Package_Init();
}

void IpMgWrapper_UnInit()
{
#ifdef USE_SERIALMUX
	serialmux_close();
#else
	RS232_CloseComport();
#endif

	Application_Package_UnInit();
}

void IpMgWrapper_loop() {
	TIME_T  currentTime;
	uint8_t byte;
	int n;

#ifdef USE_SERIALMUX
	// check and recv Socket frame
	app_vars.ipmg_socket_rxByte_cb(byte);
#else
	// receive and react to HDLC frames
	while( n = RS232_PollComport(&byte, 1) ) {
		// hand over byte to ipmg module
		app_vars.ipmg_uart_rxByte_cb(byte);
	}
#endif

	currentTime = millis();
	if (app_vars.fsmArmed==TRUE && (currentTime-app_vars.fsmPreviousEvent>app_vars.fsmDelay)) {
		app_vars.fsmArmed=FALSE;
		(*app_vars.fsmCb)();
	}
}

//=========================== private =========================================
int MakeIOSensorData(Mote_Info_t *pMote)
{
	int i;
	Sensor_Info_t *pSensorInfo;

	//printf("%s: ", __func__);
	pMote->scannedSensor = IO_SENSOR_NUM;
	pMote->MoteQueryState = Mote_Query_Observe_Started;
	sprintf(pMote->hostName, "IO");
	sprintf(pMote->productName, "WISE1020");
	sprintf(pMote->softwareVersion, "1.0.0.0");
	Susiaccess_Report_SenNodeRegister(pMote);

	pMote->sensorList.total = IO_SENSOR_NUM;
	pSensorInfo = &pMote->sensorList.item[0];
	sprintf(pSensorInfo->name, "Temperature");
	sprintf(pSensorInfo->model, "1");
	sprintf(pSensorInfo->objId, "3303");
	sprintf(pSensorInfo->value, "0");
	pSensorInfo->SensorQueryState = Sensor_Query_SensorInfo_Received;

	pSensorInfo = &pMote->sensorList.item[1];
	sprintf(pSensorInfo->name, RULE_DST_DO1_NAME);
	sprintf(pSensorInfo->model, "1");
	sprintf(pSensorInfo->objId, "3201");
	sprintf(pSensorInfo->value, "0");
	pSensorInfo->SensorQueryState = Sensor_Query_SensorInfo_Received;

	pSensorInfo = &pMote->sensorList.item[2];
	sprintf(pSensorInfo->name, RULE_DST_DO2_NAME);
	sprintf(pSensorInfo->model, "1");
	sprintf(pSensorInfo->objId, "3201");
	sprintf(pSensorInfo->value, "0");
	pSensorInfo->SensorQueryState = Sensor_Query_SensorInfo_Received;

	pSensorInfo = &pMote->sensorList.item[3];
	sprintf(pSensorInfo->name, RULE_DST_DO3_NAME);
	sprintf(pSensorInfo->model, "1");
	sprintf(pSensorInfo->objId, "3201");
	sprintf(pSensorInfo->value, "0");
	pSensorInfo->SensorQueryState = Sensor_Query_SensorInfo_Received;

	if(IO_SENSOR_NUM == 12) {
	for(i=4; i<12; i++) {
		pSensorInfo = &pMote->sensorList.item[i];
	// Digital in
		if(i<8) {
			sprintf(pSensorInfo->name, "DigiIn %d", (i-4));
			sprintf(pSensorInfo->model, "1");
			sprintf(pSensorInfo->objId, "3200");
			sprintf(pSensorInfo->value, "0");
		} else {
	// Analog in
			sprintf(pSensorInfo->name, "AnalogIn %d", (i-8));
			sprintf(pSensorInfo->model, "1");
			sprintf(pSensorInfo->objId, "3202");
			sprintf(pSensorInfo->value, "0");
		}
		pSensorInfo->SensorQueryState = Sensor_Query_SensorInfo_Received;
	}
	}

	for(i=0; i<pMote->sensorList.total; i++) {
		pSensorInfo = &pMote->sensorList.item[i];
#if 0
		printf("1.addr:%p, name:%s, model:%s, objId:%s, value:%s\n", pSensorInfo,
					pSensorInfo->name,
					pSensorInfo->model,
					pSensorInfo->objId,
					pSensorInfo->value);
#endif
		switch(atoi(pSensorInfo->objId)) {
			case OBJ_ID_IPSO_DIGITAl_INPUT:
				sprintf(pSensorInfo->resType, "gpio.din");
				sprintf(pSensorInfo->format, "ipso");
				sprintf(pSensorInfo->valueType, "b");
				sprintf(pSensorInfo->mode, "r");
				sprintf(pSensorInfo->maxValue, "1");
				sprintf(pSensorInfo->minValue, "0");
			break;
			case OBJ_ID_IPSO_DIGITAl_OUTPUT:
				sprintf(pSensorInfo->resType, "gpio.dout");
				sprintf(pSensorInfo->format, "ipso");
				sprintf(pSensorInfo->valueType, "b");
				sprintf(pSensorInfo->mode, "rw");
				sprintf(pSensorInfo->maxValue, "1");
				sprintf(pSensorInfo->minValue, "0");
			break;
			case OBJ_ID_IPSO_ANALOGUE_INPUT:
				sprintf(pSensorInfo->resType, "gpio.ain");
				sprintf(pSensorInfo->format, "ipso");
				sprintf(pSensorInfo->valueType, "b");
				sprintf(pSensorInfo->mode, "r");
				sprintf(pSensorInfo->maxValue, "1");
				sprintf(pSensorInfo->minValue, "0");
			break;
			case OBJ_ID_IPSO_TEMPERATURE_SENSOR:
				sprintf(pSensorInfo->unit, "Cel");
				sprintf(pSensorInfo->resType, "ucum.Cel");
				sprintf(pSensorInfo->format, "ipso");
				sprintf(pSensorInfo->valueType, "d");
				sprintf(pSensorInfo->mode, "r");
				sprintf(pSensorInfo->maxValue, "150");
				sprintf(pSensorInfo->minValue, "-20");
			break;
			default:
			break;
		}
#if 0
		printf("2.addr:%p, unit:%s, resType:%s, format:%s, valueType:%s, mode:%s, max:%s, min:%s\n",
					pSensorInfo,
					pSensorInfo->unit,
					pSensorInfo->resType,
					pSensorInfo->format,
					pSensorInfo->valueType,
					pSensorInfo->mode,
					pSensorInfo->maxValue,
					pSensorInfo->minValue);
#endif
	}	

	return 0;
}

//===== fsm

void IpMgWrapper_fsm_scheduleEvent(uint16_t delay, fsm_timer_callback cb) {
	app_vars.fsmArmed         = TRUE;
	app_vars.fsmPreviousEvent = millis();
	app_vars.fsmDelay         = delay;
	app_vars.fsmCb            = cb;
	app_vars.status           = ST_SEARCH;
}

void IpMgWrapper_fsm_setCallback(fsm_reply_callback cb) {
	app_vars.replyCb     = cb;
}

void IpMgWrapper_api_response_timeout(void) {
	// log
	ADV_ERROR("ERROR:    serial timeout!\n");

	// issue cancel command
	dn_ipmg_cancelTx();

	// schedule first event
	g_MgrReConnect = TRUE;
	memset(app_vars.currentMac,0,sizeof(app_vars.currentMac));
	if(g_MgrReConnectTimes < 3) {
		if(!g_MgrConnected) {
			g_MgrReConnectTimes++;
		}
		IpMgWrapper_fsm_scheduleEvent(
			BACKOFF_AFTER_TIMEOUT,
			&IpMgWrapper_api_initiateConnect
		);
	} else {
		return;
	}
}

void IpMgWrapper_sendData_response_timeout(void) {
	// log
	ADV_ERROR("\nWARN:    sendData response timeout!\n");
   
}

void IpMgWrapper_sendData_received(void) {
	ADV_INFO("%s:  Receivced!\n", __func__);
}

//===== api_initiateConnect
void IpMgWrapper_api_initiateConnect(void) {
	dn_err_t err;

	ADV_INFO("\nINFO:     dn_ipmg_initiateConnect... returns ");

	// schedule
	err = dn_ipmg_initiateConnect();

	ADV_INFO(" %d\n",err);

	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

//===== api_subscribe
void IpMgWrapper_api_subscribe(void) {
	dn_err_t err;

	ADV_INFO("\nINFO:     %s... returns ", __func__);

	// arm callback
	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_subscribe_reply);

	// issue function
	err = dn_ipmg_subscribe(
		SUBSC_FILTER_EVENT | SUBSC_FILTER_LOG | SUBSC_FILTER_DATA | SUBSC_FILTER_IPDATA | SUBSC_FILTER_HR,	// filter
		0x00000000,										// unackFilter
		(dn_ipmg_subscribe_rpt*)(app_vars.replyBuf)		// reply
	);

	ADV_INFO(" %d\n",err);

	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

void IpMgWrapper_api_subscribe_reply() {
	dn_ipmg_subscribe_rpt* reply;

	ADV_INFO("INFO:     %s\n", __func__);

	// cast reply
	reply = (dn_ipmg_subscribe_rpt*)app_vars.replyBuf;

	ADV_INFO("INFO:     RC=0x%02x\n", reply->RC);

	if (0 == reply->RC) {
		ADV_INFO("INFO: manager connected set\n");
		g_MgrConnected = TRUE;
		g_MgrReConnectTimes = 0;
		g_MgrReConnect = FALSE;

		IpMgWrapper_fsm_scheduleEvent(
			1000,
			//CMD_PERIOD,
			&IpMgWrapper_done
		);
	}
}

//===== api_resetSystem
void IpMgWrapper_api_resetSystem(void) {
   dn_err_t err;
      
   // log
   ADV_INFO("INFO:     api_resetSystem... returns ");
   
   // arm callback
   IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_resetSystem_reply);
   
   // issue function
   err = dn_ipmg_reset(
      0,                                           
      app_vars.currentMac,                            // macAddress
      (dn_ipmg_reset_rpt*)(app_vars.replyBuf)     // reply
   );
   
   // log
   ADV_INFO("%d\n",err);
   
   // schedule timeout event
   IpMgWrapper_fsm_scheduleEvent(
      SERIAL_RESPONSE_TIMEOUT,
      &IpMgWrapper_api_response_timeout
   );
}

void IpMgWrapper_api_resetSystem_reply() {
   dn_ipmg_reset_rpt* reply;
      
   // log
   ADV_INFO("INFO:     api_resetSystem_reply\n");
   
   // cast reply
   reply = (dn_ipmg_reset_rpt*)app_vars.replyBuf;
   
   // log
   ADV_INFO("INFO:     RC=0x%02x\n", reply->RC);

   IpMgWrapper_fsm_scheduleEvent(
      INTER_FRAME_PERIOD,
      &IpMgWrapper_done
   );
}

//===== api_getNextMoteConfig
void IpMgWrapper_api_getNextMoteConfig(void) {
	dn_err_t err;

	ADV_INFO("\nINFO:     %s MAC=", __func__);
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));

	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_getNextMoteConfig_reply);

	err = dn_ipmg_getMoteConfig(
		app_vars.currentMac,                            	// macAddress
		TRUE,                                           	// next
		(dn_ipmg_getMoteConfig_rpt*)(app_vars.replyBuf) 	// reply buf
	);

	ADV_INFO(" %d\n",err);
	ADV_TRACE("INFO:     %s end\n", __func__);

	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

void IpMgWrapper_api_getNextMoteConfig_reply() {
	dn_ipmg_getMoteConfig_rpt* reply;

	ADV_INFO("INFO:     %s\n", __func__);

	reply = (dn_ipmg_getMoteConfig_rpt *) app_vars.replyBuf;

	ADV_INFO("INFO:     RC=0x%02x\n", reply->RC);
	ADV_INFO("INFO:     MAC=");
	IpMgWrapper_printByteArray(reply->macAddress, sizeof(reply->macAddress));
	ADV_INFO(" \n");
	ADV_INFO("INFO:     state=");
	IpMgWrapper_printState(reply->state);
	ADV_INFO("INFO:     isAP=%d\n", reply->isAP);

	if (0 == reply->RC) {
		Mote_Info_t *pMoteInfo;

		pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, reply->macAddress);

		if (NULL == pMoteInfo) {
			pMoteInfo = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
			if (NULL == pMoteInfo)
				return;
			memset(pMoteInfo, 0, sizeof(Mote_Info_t));

			memcpy(pMoteInfo->macAddress, reply->macAddress, sizeof(pMoteInfo->macAddress));
			pMoteInfo->moteId = reply->moteId;
			pMoteInfo->isAP = reply->isAP;
			pMoteInfo->state = reply->state;
			pMoteInfo->dustReserved = reply->reserved;
			pMoteInfo->isRouting = reply->isRouting;
			pMoteInfo->neighborList.total = 0;
			//pMoteInfo->bPathUpdate = TRUE;
			MoteList_AddItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pMoteInfo);
			free(pMoteInfo);
		} else {
			pMoteInfo->moteId = reply->moteId;
			pMoteInfo->isAP = reply->isAP;
			pMoteInfo->state = reply->state;
			pMoteInfo->dustReserved = reply->reserved;
			pMoteInfo->isRouting = reply->isRouting;			
			pMoteInfo->neighborList.total = 0;
			//pMoteInfo->bPathUpdate = TRUE;
		}

		g_pCurrentMote = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, reply->macAddress);

		memcpy(app_vars.currentMac, reply->macAddress, sizeof(app_vars.currentMac));

		app_vars.pathId = 0;
		
		if(!g_MgrConnected || pMoteInfo->bPathUpdate) {
			IpMgWrapper_fsm_scheduleEvent(
				INTER_FRAME_PERIOD,
				&IpMgWrapper_api_getNextPathInfo
			);
		} else {
			IpMgWrapper_fsm_scheduleEvent(
    		 	INTER_FRAME_PERIOD,
				&IpMgWrapper_api_getNextMoteConfig
			);
		}
	} else {
		ADV_TRACE("%s: <end of list>\n", __func__);
		if(!g_MgrConnected) {
			IpMgWrapper_fsm_scheduleEvent(
				INTER_FRAME_PERIOD,
				&IpMgWrapper_api_getSystemInfo
			);
		} else if(g_MgrReConnect) {
			IpMgWrapper_fsm_scheduleEvent(
				INTER_FRAME_PERIOD,
				&IpMgWrapper_api_subscribe
			);
		} else {
			IpMgWrapper_fsm_scheduleEvent(
				CMD_PERIOD,
				&IpMgWrapper_done
			);
		}
	}
}

//===== api_getNextPathInfo
void IpMgWrapper_api_getNextPathInfo(void) {
	dn_err_t err;
#if 0
	ADV_INFO("\nINFO:     %s MAC=", __func__);
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
	ADV_INFO("... returns ");
#endif
	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_getNextPathInfo_reply);

	err = dn_ipmg_getNextPathInfo(
		app_vars.currentMac,                            	// macAddress
		0,                                              	// filter, 0 is all
		app_vars.pathId,                                   	// pathId, 0 is first path
		(dn_ipmg_getNextPathInfo_rpt*)(app_vars.replyBuf) 	// reply buf
	);
#if 0
	ADV_INFO(" %d\n",err);
	ADV_TRACE("\nINFO:     %s pathId=%d end\n", __func__, app_vars.pathId);
#endif
	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

void IpMgWrapper_api_getNextPathInfo_reply() {
	dn_ipmg_getNextPathInfo_rpt* reply;

	ADV_INFO("INFO:     %s ", __func__);

	reply = (dn_ipmg_getNextPathInfo_rpt *) app_vars.replyBuf;

	ADV_INFO(" RC=0x%02x ", reply->RC);
	ADV_TRACE(" pathId=%d ", reply->pathId);
	IpMgWrapper_printByteArray(reply->source, sizeof(reply->source));
	ADV_INFO("->");
	IpMgWrapper_printByteArray(reply->dest, sizeof(reply->dest));
	ADV_TRACE(" direction=%d ", reply->direction);
	ADV_TRACE(" numLinks=%d ", reply->numLinks);
	ADV_INFO(" \n");

	if (0 == reply->RC) {
		app_vars.pathId = reply->pathId;
		IpMgWrapper_fsm_scheduleEvent(
    		INTER_FRAME_PERIOD,
			&IpMgWrapper_api_getNextPathInfo
		);
		if (reply->direction > 1 ) { // upstream(2) and downstream(3)
			memcpy(g_pCurrentMote->neighborList.item[g_pCurrentMote->neighborList.total].macAddress,
							reply->dest, sizeof(reply->dest));
			g_pCurrentMote->neighborList.total++;
		}
	} else {
//		if (0x0b == reply->RC) {
			ADV_TRACE("%s: <end of list>\n", __func__);

			if(!g_MgrConnected || g_MgrReConnect) {
				IpMgWrapper_fsm_scheduleEvent(
		    	 	INTER_FRAME_PERIOD,
					&IpMgWrapper_api_getNextMoteConfig
				);
			} else {
				IpMgWrapper_fsm_scheduleEvent(
					CMD_PERIOD,
					&IpMgWrapper_done
				);
			}

			if(g_pCurrentMote->state == MOTE_STATE_LOST) {
				g_pCurrentMote->neighborList.total = 0;
			}
			g_pCurrentMote->bPathUpdate = FALSE;

//		}
	}
}

//===== api_getMoteInfo

void IpMgWrapper_api_getMoteInfo(void) {
   dn_err_t err;
      
   // log
   ADV_INFO("\nINFO:     %s MAC=", __func__);
   IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
   ADV_INFO("... returns ");

   // arm callback
   IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_getMoteInfo_reply);

   // issue function
   err = dn_ipmg_getMoteInfo(
      app_vars.currentMac,                            // macAddress
      (dn_ipmg_getMoteInfo_rpt*)(app_vars.replyBuf) // reply
   );
   // log
   ADV_INFO(" %d\n",err);

   // schedule timeout event
   IpMgWrapper_fsm_scheduleEvent(
      SERIAL_RESPONSE_TIMEOUT,
      &IpMgWrapper_api_response_timeout
   );
}

void IpMgWrapper_api_getMoteInfo_reply() {
   dn_ipmg_getMoteInfo_rpt* reply;

   // log
   ADV_INFO("INFO:     %s", __func__);

   // cast reply
   reply = (dn_ipmg_getMoteInfo_rpt*)app_vars.replyBuf;

   // log
   ADV_INFO(" RC=0x%02x", reply->RC);
   //ADV_INFO("INFO:     MAC=");
   //IpMgWrapper_printByteArray(reply->macAddress,sizeof(reply->macAddress));
   //ADV_INFO("\n");
   ADV_INFO(" state=");
   IpMgWrapper_printState(reply->state);
   ADV_INFO(" \n");
   ADV_TRACE("INFO:     numGoodNbrs/numNbrs=%d/%d\n", reply->numGoodNbrs, reply->numNbrs);
   ADV_TRACE("INFO:     packetsRecv=%d\n", reply->packetsReceived);
   ADV_TRACE("INFO:     packetsLost=%d\n", reply->packetsLost);
   ADV_TRACE("INFO:     avgLatency=%d\n", reply->avgLatency);

   if(reply->RC == 0) {
      g_pCurrentMote->state = reply->state;
      if(reply->packetsReceived != 0) {
            g_pCurrentMote->reliability = reply->packetsReceived/(reply->packetsReceived+reply->packetsLost) * 100;
      }
      IpMgWrapper_fsm_scheduleEvent(
         CMD_PERIOD,
         &IpMgWrapper_done
      );
   }
}

void IpMgWrapper_api_sendCmd(Mote_Info_t *pMote, SendCmdID _cmdId, int _param1, int _param2) {
	dn_err_t   err;
	adv_coap_packet_t *payload;
	unsigned char strTmp[20];
   
	memset(strTmp, 0, sizeof(strTmp));
	memcpy(app_vars.currentMac, pMote->macAddress, sizeof(app_vars.currentMac));

	ADV_INFO("INFO:     %s", __func__);

	switch(_cmdId) {
		//instead of IpMgWrapper_api_getSenHubInfo
		case SEND_CMD_GET_SENSORHUB_INFO:
			ADV_INFO(" GET_SENSORHUB_INFO ");
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_MOTE_INFO,
							URI_PARAM_ACTION_LIST, NULL );
			break;
		//instead of IpMgWrapper_api_getSensorIndex
		case SEND_CMD_GET_SENSOR_INDEX:
			ADV_INFO(" GET_SENSOR_INDEX ");
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO,
						   	URI_PARAM_ACTION_LIST, URI_PARAM_FIELD_ID );
			break;
		//instead of IpMgWrapper_api_getSensorInfo
		case SEND_CMD_GET_SENSOR_INFO:
			ADV_INFO(" GET_SENSOR_INFO(id=%d) ", _param1);
			snprintf(strTmp, sizeof(strTmp), "id=%d", _param1);
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO,
						   	URI_PARAM_ACTION_LIST, strTmp );
			break;
		//instead of IpMgWrapper_api_getSensorStandard
		case SEND_CMD_GET_SENSOR_STANDRAD:
			ADV_INFO(" GET_SENSOR_STANDARD(sid=%d&id=%d) ", _param2, _param1);
			snprintf(strTmp, sizeof(strTmp), "sid=%d&id=%d", _param2, _param1);
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO,
						   	URI_PARAM_ACTION_QUERY, strTmp );
			break;
		//instead of IpMgWrapper_api_getSensorHardware
		case SEND_CMD_GET_SENSOR_HARDWARE:
			ADV_INFO(" GET_SENSOR_HARDWARE(id=%d) ", _param1);
			snprintf(strTmp, sizeof(strTmp), "id=%d", _param1);
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO,
						   	URI_PARAM_ACTION_QUERY, strTmp );
			break;
		//instead of IpMgWrapper_api_setSensorValByName
		case SEND_CMD_SET_SENSOR_VALUEBYNAME:
			ADV_INFO(" SET_SENSOR_VALUEBYNAME(n=%s) ", pMote->cmdParam);
			snprintf(strTmp, sizeof(strTmp), "n=%s", pMote->cmdParam);
			// ex: PUT /sen/status?n=dout&bv=1
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_PUT, URI_SENSOR_STATUS,
						   	strTmp, URI_PARAM_BOOL_VALUE_TRUE );
			break;
		//instead of IpMgWrapper_api_setMoteName
		case SEND_CMD_SET_SENSORHUB_NAME:
			ADV_INFO(" SET_SENSORHUB_NAME(n=\"%s\") ", pMote->cmdParam);
			snprintf(strTmp, sizeof(strTmp), "n=\"%s\"", pMote->cmdParam);
			// ex: PUT /dev/info?n="Demo 1"
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_PUT, URI_MOTE_INFO,
						   	strTmp, NULL );
			break;
		//instead of IpMgWrapper_api_setMoteReset
		case SEND_CMD_SET_SENSORHUB_RESET:
			ADV_INFO(" SET_SENSORHUB_RESET(reset=%s) ", pMote->cmdParam);
			// ex: PUT /dev/info?reset=1
			sprintf(strTmp, "reset=%s", pMote->cmdParam);
			payload = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_PUT, URI_MOTE_INFO,
						   	strTmp, NULL );
			break;
		//instead of IpMgWrapper_api_observe
		case SEND_CMD_OBSERVE: // All
			ADV_INFO(" SET_OBSERVE ");
			payload = adv_coap_make_pkt(COAP_TYPE_CON, COAP_METHOD_GET, URI_SENSOR_STATUS,
						   	URI_PARAM_ACTION_OBSERVE, URI_PARAM_ID(all) );
			break;
		//instead of IpMgWrapper_api_cancelObserve
		case SEND_CMD_CANCEL_OBSERVE: // All
			ADV_INFO(" SET_CANCEL_OBSERVE ");
			// GET /sen/status?action=cancel&id=all
			payload = adv_coap_make_pkt(COAP_TYPE_CON, COAP_METHOD_GET, URI_SENSOR_STATUS,
						   	URI_PARAM_ACTION_CANCEL, URI_PARAM_ID(all) );
			break;
		default:
			ADV_INFO(" UNKNOW ");
			break;
	}
	ADV_INFO("MAC=");
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
	ADV_INFO("... returns ");

	// arm callback
	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_sendData_reply);
   
	// issue function
	err = dn_ipmg_sendData(
		app_vars.currentMac,						// macAddress
		0x00,										// priority
		DN_UDP_PORT_ADVMOTE,						// srcPort
		DN_UDP_PORT_ADVMOTE,						// dstPort
		0x00,										// options
		(uint8_t*)payload,							// data
		sizeof(adv_coap_packet_t),					// dataLen
		(dn_ipmg_sendData_rpt*)(app_vars.replyBuf)	// reply
	);
	adv_coap_delete_pkt(payload);
   
   // log
	ADV_INFO("%d(%ld)\n",err, sizeof(adv_coap_packet_t));
   
	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

void IpMgWrapper_api_setMasterIOByName(Mote_Info_t *pMote) {
	int i;

	for (i=0; i<pMote->sensorList.total; i++) {
		if (strncmp(pMote->sensorList.item[i].name, pMote->cmdName, strlen(pMote->sensorList.item[i].name)) == 0) {
			if (strncmp(pMote->cmdName, RULE_DST_DO3_NAME, strlen(pMote->cmdName)) == 0) {
				IpMgWrapper_MoteMasterSetIO(pMote, Mote_Master_D4, atoi(pMote->cmdParam));
				memcpy(pMote->sensorList.item[i].value, pMote->cmdParam, strlen(pMote->sensorList.item[i].value));
			} else 
			if (strncmp(pMote->cmdName, RULE_DST_DO2_NAME, strlen(pMote->cmdName)) == 0) {
				IpMgWrapper_MoteMasterSetIO(pMote, Mote_Master_D5, atoi(pMote->cmdParam));
				memcpy(pMote->sensorList.item[i].value, pMote->cmdParam, strlen(pMote->sensorList.item[i].value));
			} else
			if (strncmp(pMote->cmdName, RULE_DST_DO1_NAME, strlen(pMote->cmdName)) == 0) {
				IpMgWrapper_MoteMasterSetIO(pMote, Mote_Master_D6, atoi(pMote->cmdParam));
				memcpy(pMote->sensorList.item[i].value, pMote->cmdParam, strlen(pMote->sensorList.item[i].value));
			}
			break;
		}
	}
}

void IpMgWrapper_api_setMoteAutoReport(Mote_Info_t *pMote) {
	if (NULL == pMote)
		return;

	if(atoi(pMote->cmdParam) == 0) {
		// cancel
		pMote->MoteQueryState = Mote_Query_Observe_Stop_Sent;
	} else if(atoi(pMote->cmdParam) == 1) {
		// observe
		pMote->MoteQueryState = Mote_Query_Observe_Start_Sent;
	}
}

/*
void IpMgWrapper_api_init_master_gpio(void)
{
	dn_err_t   err;
	uint8_t    payload[22];

	printf("\nINFO:     api_init_gpio MAC=");
	IpMgWrapper_printByteArray(app_vars.currentMac,sizeof(app_vars.currentMac));
	printf("... returns ");

	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_sendData_reply);
	// prepare OAP payload "05 00 02 ff 02 04 03 00 01 01 01 04 00 04 93 e0 02 01 01 03 01 00"
	payload[0] = 0x05;
	payload[1] = 0x00;			// ID
	payload[2] = 0x02;			// command (0x02==PUT)
	payload[3] = 0xff;			// (T) address
	payload[4] = 0x02;			// (L) address
	payload[5] = 0x04;			// (V) address
	payload[6] = 0x03;			// (V) address
	payload[7] = 0x00;			// (T) value
	payload[8] = 0x01;			// (L) value
	payload[9] = 0x01;			// (V) value
	payload[10] = 0x01;   
	payload[11] = 0x04;   
	payload[12] = 0x00;   
	payload[13] = 0x04;   
	payload[14] = 0x93;   
	payload[15] = 0xe0;   
	payload[16] = 0x02;   
	payload[17] = 0x01;   
	payload[18] = 0x01;   
	payload[19] = 0x03;   
	payload[20] = 0x01;   
	payload[21] = 0x00;      

	err = dn_ipmg_sendData(      
		app_vars.currentMac,						// macAddress      
		0x00,										// priority      
		DN_UDP_PORT_OAP,							// srcPort      
		DN_UDP_PORT_OAP,							// dstPort      
		0x00,										// options      
		payload,									// data      
		sizeof(payload),							// dataLen      
		(dn_ipmg_sendData_rpt*)(app_vars.replyBuf)	// reply   
	);      

	printf("%d\n",err);

	IpMgWrapper_fsm_scheduleEvent(      
		SERIAL_RESPONSE_TIMEOUT,      
		&IpMgWrapper_api_response_timeout   
	);
}
*/

void IpMgWrapper_api_sendData_reply() {
	dn_ipmg_sendData_rpt* reply;
   
	// log
	ADV_INFO("INFO:     %s ", __func__);
   
	// cast reply
	reply = (dn_ipmg_sendData_rpt*)app_vars.replyBuf;
   
	// log
	ADV_INFO(" RC=0x%02x ",reply->RC);
	ADV_TRACE(" callbackId=%d ",reply->callbackId);
	ADV_INFO(" \n");
   
	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SENDDATA_RESPONSE_TIMEOUT,
		&IpMgWrapper_sendData_response_timeout
	);
}

void IpMgWrapper_api_getSystemInfo(void)
{
	dn_err_t err;

	ADV_INFO("INFO:     %s", __func__);
	ADV_INFO("... returns ");

	IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_getSystemInfo_reply);

	err = dn_ipmg_getSystemInfo(
		(dn_ipmg_getSystemInfo_rpt*)(app_vars.replyBuf) 	// reply buf
	);

	ADV_INFO( "%d\n",err);

	// schedule timeout event
	IpMgWrapper_fsm_scheduleEvent(
		SERIAL_RESPONSE_TIMEOUT,
		&IpMgWrapper_api_response_timeout
	);
}

void IpMgWrapper_api_getSystemInfo_reply() {
	dn_ipmg_getSystemInfo_rpt* reply;

	ADV_INFO("INFO:     %s\n", __func__);

	reply = (dn_ipmg_getSystemInfo_rpt *) app_vars.replyBuf;

	ADV_INFO("INFO:     RC=0x%02x\n", reply->RC);
	ADV_INFO("INFO:     MAC=");
	IpMgWrapper_printByteArray(reply->macAddress, sizeof(reply->macAddress));
	ADV_INFO(" \n");
	ADV_INFO("INFO:     hwModel=%d, hwRev=%d, swMajor=%d, swMinor=%d, swPatch=%d, swBuild=%d\n", reply->hwModel, reply->hwRev, reply->swMajor, reply->swMinor, reply->swPatch, reply->swBuild);

	if (0 == reply->RC) {
		sprintf(g_ManagerSWVer, "%d.%d.%d.%d", reply->swMajor, reply->swMinor, reply->swPatch, reply->swBuild);

		IpMgWrapper_fsm_scheduleEvent(
			INTER_FRAME_PERIOD,
			&IpMgWrapper_api_getNetworkInfo
		);
	}

}

//===== api_getNetworkInfo

void IpMgWrapper_api_getNetworkInfo(void) {
   dn_err_t err;
      
   // log
   ADV_INFO("\nINFO:     %s... returns ", __func__);
   
   // arm callback
   IpMgWrapper_fsm_setCallback(&IpMgWrapper_api_getNetworkInfo_reply);
   
   // issue function
   err = dn_ipmg_getNetworkInfo(
      (dn_ipmg_getNetworkInfo_rpt*)(app_vars.replyBuf)     // reply
   );
   
   // log
   ADV_INFO(" %d\n",err);
   
   // schedule timeout event
   IpMgWrapper_fsm_scheduleEvent(
      SERIAL_RESPONSE_TIMEOUT,
      &IpMgWrapper_api_response_timeout
   );
}

void IpMgWrapper_api_getNetworkInfo_reply() {
   dn_ipmg_getNetworkInfo_rpt *currNetworkInfo;
   // log
   ADV_INFO("INFO:     %s\n", __func__);
   
   // cast reply
   currNetworkInfo = (dn_ipmg_getNetworkInfo_rpt*)app_vars.replyBuf;
   
   // log
   ADV_INFO("INFO:     RC=0x%02x\n", currNetworkInfo->RC);
   ADV_TRACE("INFO:     numMotes=%d\n", currNetworkInfo->numMotes);
   ADV_TRACE("INFO:     netReliability=%d\n", currNetworkInfo->netReliability);
   ADV_TRACE("INFO:     netPathStability=%d\n", currNetworkInfo->netPathStability);
   ADV_TRACE("INFO:     netLatency=%d\n", currNetworkInfo->netLatency);

   if(currNetworkInfo->RC == 0) {
		g_DustWsnContext.SmartMeshIpInfo.NetReliability = currNetworkInfo->netReliability ;

		IpMgWrapper_fsm_scheduleEvent(
			INTER_FRAME_PERIOD,
			&IpMgWrapper_api_subscribe
		);
   }
}

//=========================== helpers =========================================

void IpMgWrapper_printState(uint8_t state) {
	switch (state) {
		case MOTE_STATE_LOST:
			ADV_INFO("MOTE_STATE_LOST\n");
			break;
		case MOTE_STATE_NEGOTIATING:
			ADV_INFO("MOTE_STATE_NEGOTIATING\n");
			break;
		case MOTE_STATE_OPERATIONAL:
			ADV_INFO("MOTE_STATE_OPERATIONAL\n");
			break;
		default:
			printf("<unknown>\n");
			break;
	}
}

void IpMgWrapper_printByteArray(uint8_t* payload, uint8_t length) {
	uint8_t i;
   
//   printf(" ");
	for (i=0;i<length;i++) {
		ADV_INFO("%02x",payload[i]);
		if (i<length-1) {
			//printf("-");
		}
	}
}

extern void printBuf(uint8_t* buf, uint8_t bufLen) {
	uint8_t i;
   
	printf(" ");
	for (i=0;i<bufLen;i++) {
		printf("%02x",buf[i]);
		if (i<bufLen-1) {
			printf("-");
		}
	}
	printf("\n");
}

extern void printPoi() {
   printf("poipoi\n");
}

static int HandleNotifEvent(uint8_t subCmdId)
{
	ADV_TRACE("!!!!! %s:DN_NOTIFID_NOTIFEVENT !!!!!\n", __func__);

	switch (subCmdId) {
		case DN_EVENTID_EVENTMOTERESET: {
			dn_ipmg_eventMoteReset_nt *pEventMoteReset_nt;
			pEventMoteReset_nt = (dn_ipmg_eventMoteReset_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTERESET !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteReset_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteReset_nt->macAddress,sizeof(pEventMoteReset_nt->macAddress));
			ADV_INFO(" \n");
			break;
		}
		case DN_EVENTID_EVENTNETWORKRESET: {
			dn_ipmg_eventNetworkReset_nt *pEventNetworkReset_nt;
			pEventNetworkReset_nt = (dn_ipmg_eventNetworkReset_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTNETWORKRESET !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventNetworkReset_nt->eventId);
			break;
		}
		case DN_EVENTID_EVENTCOMMANDFINISHED: {
			dn_ipmg_eventCommandFinished_nt *pEventCommandFinished_nt;
			pEventCommandFinished_nt = (dn_ipmg_eventCommandFinished_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTCOMMANDFINISHED !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventCommandFinished_nt->eventId);
			ADV_TRACE("\t\t callbackId:%d\n", pEventCommandFinished_nt->callbackId);
			ADV_TRACE("\t\t rc:%d\n", pEventCommandFinished_nt->rc);
			break;
		}
		case DN_EVENTID_EVENTMOTEJOIN: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventMoteJoin_nt *pEventMoteJoin_nt;
			pEventMoteJoin_nt = (dn_ipmg_eventMoteJoin_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTEJOIN !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteJoin_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteJoin_nt->macAddress,sizeof(pEventMoteJoin_nt->macAddress));
			ADV_INFO(" \n");
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pEventMoteJoin_nt->macAddress);
			if (NULL == pMoteInfo) {
				ADV_TRACE("Got DN_EVENTID_EVENTMOTEJOIN, but Mote not found in list\n");
				pMoteInfo = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
				if (NULL == pMoteInfo)
					return;
				memset(pMoteInfo, 0, sizeof(Mote_Info_t));
			
				memcpy(pMoteInfo->macAddress, pEventMoteJoin_nt->macAddress, sizeof(pMoteInfo->macAddress));
				pMoteInfo->state = MOTE_STATE_NEGOTIATING;
				MoteList_AddItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pMoteInfo);
				free(pMoteInfo);
			} else {
				pMoteInfo->state = MOTE_STATE_NEGOTIATING;
				pMoteInfo->MoteQueryState = Mote_Query_NotStart;
				pMoteInfo->scannedSensor = 0;
				MoteList_ResetMoteSensorList(pMoteInfo);
			}

			g_doSensorScan = TRUE;
			break;
		}
		case DN_EVENTID_EVENTMOTEOPERATIONAL: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventMoteOperational_nt *pEventMoteOperational_nt;
			pEventMoteOperational_nt = (dn_ipmg_eventMoteOperational_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTEOPERATIONAL !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteOperational_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteOperational_nt->macAddress,sizeof(pEventMoteOperational_nt->macAddress));
			ADV_INFO(" \n");
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pEventMoteOperational_nt->macAddress);
			if (NULL == pMoteInfo) {
				ADV_TRACE("Got DN_EVENTID_EVENTMOTEOPERATIONAL, but Mote not found in list\n");
				pMoteInfo = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
				if (NULL == pMoteInfo)
					return;
				memset(pMoteInfo, 0, sizeof(Mote_Info_t));
			
				memcpy(pMoteInfo->macAddress, pEventMoteOperational_nt->macAddress, sizeof(pMoteInfo->macAddress));
				pMoteInfo->state = MOTE_STATE_OPERATIONAL;
				pMoteInfo->bPathUpdate = TRUE;
				MoteList_AddItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pMoteInfo);
				// handle numOperationalEvents
				free(pMoteInfo);
			} else {
				pMoteInfo->state = MOTE_STATE_OPERATIONAL;
				pMoteInfo->bPathUpdate = TRUE;
				pMoteInfo->MoteQueryState = Mote_Query_NotStart;
				pMoteInfo->scannedSensor = 0;
				MoteList_ResetMoteSensorList(pMoteInfo);

				// handle numOperationalEvents
			}

			g_doUpdateInterface = TRUE;
			g_doSensorScan = TRUE;
			break;
		}
		case DN_EVENTID_EVENTMOTELOST: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventMoteLost_nt *pEventMoteLost_nt;
			pEventMoteLost_nt = (dn_ipmg_eventMoteLost_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTELOST !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteLost_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteLost_nt->macAddress,sizeof(pEventMoteLost_nt->macAddress));
			ADV_INFO(" \n");
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pEventMoteLost_nt->macAddress);
			if (NULL == pMoteInfo) {
				ADV_TRACE("Got DN_EVENTID_EVENTMOTELOST, but Mote not found in list\n");
				pMoteInfo = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
				if (NULL == pMoteInfo)
					return;
				memset(pMoteInfo, 0, sizeof(Mote_Info_t));
			
				memcpy(pMoteInfo->macAddress, pEventMoteLost_nt->macAddress, sizeof(pMoteInfo->macAddress));
				pMoteInfo->state = MOTE_STATE_LOST;
				pMoteInfo->bPathUpdate = TRUE;
				pMoteInfo->MoteQueryState = Mote_Query_NotStart;
				MoteList_AddItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pMoteInfo);
				free(pMoteInfo);
			} else {
				pMoteInfo->state = MOTE_STATE_LOST;
				pMoteInfo->MoteQueryState = Mote_Query_NotStart;
				pMoteInfo->scannedSensor = 0;
				pMoteInfo->bPathUpdate = TRUE;
				MoteList_ResetMoteSensorList(pMoteInfo);
			}

			Susiaccess_Report_MoteLost(pMoteInfo);

			if (NULL != g_pMoteToScan) {
				if (0 == memcmp(g_pMoteToScan->macAddress, pEventMoteLost_nt->macAddress, 8)) {
					ADV_TRACE("NULL g_pMoteToScan with state:%d\n", pMoteInfo->state);
					g_pMoteToScan = NULL;
				}
			}
			
			// delete all related path

			g_doUpdateInterface = TRUE;
			break;
		}
		case DN_EVENTID_EVENTNETWORKTIME: {
			dn_ipmg_eventNetworkTime_nt *pEventNetworkTime_nt;
			pEventNetworkTime_nt = (dn_ipmg_eventNetworkTime_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTNETWORKTIME !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventNetworkTime_nt->eventId);
			ADV_TRACE("\t\t uptime:%d\n", pEventNetworkTime_nt->uptime);
			ADV_INFO("\t\t utcSecs:");
			IpMgWrapper_printByteArray(pEventNetworkTime_nt->utcSecs,sizeof(pEventNetworkTime_nt->utcSecs));
			ADV_INFO(" \n");
			ADV_TRACE("\t\t utcUsecs:%d\n", pEventNetworkTime_nt->utcUsecs);
			ADV_INFO("\t\t asn:");
			IpMgWrapper_printByteArray(pEventNetworkTime_nt->asn,sizeof(pEventNetworkTime_nt->asn));
			ADV_INFO(" \n");
			ADV_TRACE("\t\t asnOffset:%d\n", pEventNetworkTime_nt->asnOffset);
			break;
		}
		case DN_EVENTID_EVENTPINGRESPONSE: {
			dn_ipmg_eventPingResponse_nt *pEventPingResponse_nt;
			pEventPingResponse_nt = (dn_ipmg_eventPingResponse_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTPINGRESPONSE !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventPingResponse_nt->eventId);
			ADV_TRACE("\t\t callbackId:%d\n", pEventPingResponse_nt->callbackId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventPingResponse_nt->macAddress,sizeof(pEventPingResponse_nt->macAddress));
			ADV_INFO(" \n");
			ADV_TRACE("\t\t delay:%d\n", pEventPingResponse_nt->delay);
			ADV_TRACE("\t\t voltage:%d\n", pEventPingResponse_nt->voltage);
			ADV_TRACE("\t\t temperature:%d\n", pEventPingResponse_nt->temperature);
			break;
		}
		case DN_EVENTID_EVENTPATHCREATE: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventPathCreate_nt *pEventPathCreate_nt;
			pEventPathCreate_nt = (dn_ipmg_eventPathCreate_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTPATHCREATE !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventPathCreate_nt->eventId);
			ADV_INFO("\t\t ");
			IpMgWrapper_printByteArray(pEventPathCreate_nt->source,sizeof(pEventPathCreate_nt->source));
			ADV_INFO(" -> ");
			IpMgWrapper_printByteArray(pEventPathCreate_nt->dest,sizeof(pEventPathCreate_nt->dest));
			ADV_INFO(" \n");			
			ADV_TRACE("\t\t direction:%d\n", pEventPathCreate_nt->direction);
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList,
						   	pEventPathCreate_nt->source);
			if (NULL == pMoteInfo) {
				ADV_INFO("%s: Mote not found in list\n", __func__);
			} else {
				pMoteInfo->bPathUpdate = TRUE;
			}
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList,
						   	pEventPathCreate_nt->dest);
			if (NULL == pMoteInfo) {
				ADV_INFO("%s: Mote not found in list\n", __func__);
			} else {
				pMoteInfo->bPathUpdate = TRUE;
			}
			break;
		}
		case DN_EVENTID_EVENTPATHDELETE: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventPathDelete_nt *pEventPathDelete_nt;
			pEventPathDelete_nt = (dn_ipmg_eventPathDelete_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTPATHDELETE !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventPathDelete_nt->eventId);
			ADV_INFO("\t\t ");
			IpMgWrapper_printByteArray(pEventPathDelete_nt->source,sizeof(pEventPathDelete_nt->source));
			ADV_INFO(" -> ");
			IpMgWrapper_printByteArray(pEventPathDelete_nt->dest,sizeof(pEventPathDelete_nt->dest));
			ADV_INFO(" \n");			
			ADV_TRACE("\t\t direction:%d\n", pEventPathDelete_nt->direction);
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList,
						   	pEventPathDelete_nt->source);
			if (NULL == pMoteInfo) {
				ADV_INFO("%s: Mote not found in list\n", __func__);
			} else {
				pMoteInfo->bPathUpdate = TRUE;
			}
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList,
						   	pEventPathDelete_nt->dest);
			if (NULL == pMoteInfo) {
				ADV_INFO("%s: Mote not found in list\n", __func__);
			} else {
				pMoteInfo->bPathUpdate = TRUE;
			}
			break;
		}
		case DN_EVENTID_EVENTPACKETSENT: {
			dn_ipmg_eventPacketSent_nt *pEventPacketSent_nt;
			pEventPacketSent_nt = (dn_ipmg_eventPacketSent_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTPACKETSENT !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventPacketSent_nt->eventId);
			ADV_TRACE("\t\t callbackId:%d\n", pEventPacketSent_nt->callbackId);
			ADV_TRACE("\t\t rc:%d\n", pEventPacketSent_nt->rc);
			break;
		}
		case DN_EVENTID_EVENTMOTECREATE: {
			Mote_Info_t *pMoteInfo;
			dn_ipmg_eventMoteCreate_nt *pEventMoteCreate_nt;
			pEventMoteCreate_nt = (dn_ipmg_eventMoteCreate_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTECREATE !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteCreate_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteCreate_nt->macAddress,sizeof(pEventMoteCreate_nt->macAddress));
			ADV_INFO(" \n");
			ADV_INFO("\t\t moteId:%d\n", pEventMoteCreate_nt->moteId);
			pMoteInfo = MoteList_GetItemByMac(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pEventMoteCreate_nt->macAddress);
			if (NULL == pMoteInfo) {
				ADV_TRACE("Got DN_EVENTID_EVENTMOTECREATE, but Mote not found in list\n");
				pMoteInfo = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
				if (NULL == pMoteInfo)
					return;
				memset(pMoteInfo, 0, sizeof(Mote_Info_t));
			
				memcpy(pMoteInfo->macAddress, pEventMoteCreate_nt->macAddress, sizeof(pMoteInfo->macAddress));
				pMoteInfo->moteId = pEventMoteCreate_nt->moteId;
				MoteList_AddItem(&g_DustWsnContext.SmartMeshIpInfo.MoteList, pMoteInfo);
				free(pMoteInfo);
			} else {
				pMoteInfo->moteId = pEventMoteCreate_nt->moteId;
			}
			break;
		}
		case DN_EVENTID_EVENTMOTEDELETE: {
			dn_ipmg_eventMoteDelete_nt *pEventMoteDelete_nt;
			pEventMoteDelete_nt = (dn_ipmg_eventMoteDelete_nt *) app_vars.notifBuf;
			ADV_INFO("\t !!!!! DN_EVENTID_EVENTMOTEDELETE !!!!!\n");
			ADV_TRACE("\t\t eventId:%d\n", pEventMoteDelete_nt->eventId);
			ADV_INFO("\t\t macAddress:");
			IpMgWrapper_printByteArray(pEventMoteDelete_nt->macAddress,sizeof(pEventMoteDelete_nt->macAddress));
			ADV_INFO(" \n");
			ADV_INFO("\t\t moteId:%d\n", pEventMoteDelete_nt->moteId);
			break;
		}
		default:
			ADV_ERROR("\t Warning: Unknown Notif Event command:%d\n", subCmdId);
			break;
	}
}

static int HandleNotifLog(uint8_t subCmdId)
{
	ADV_INFO("!!!!! DN_NOTIFID_NOTIFLOG !!!!!\n");

}

static int HandleNotifData(uint8_t subCmdId)
{
	dn_ipmg_notifData_nt* notifData_notif;
	uint8_t i, j;
	Mote_Info_t *pMote = NULL;
	Mote_List_t *pMoteList = &g_DustWsnContext.SmartMeshIpInfo.MoteList;
	adv_coap_packet_t output;
	int iret = 0;


	notifData_notif = (dn_ipmg_notifData_nt*) app_vars.notifBuf;

#if 0
	ADV_INFO("INFO:	  notifData_notif->dataLen = %d\n", notifData_notif->dataLen);
	for (i=0;i<notifData_notif->dataLen;i++) {
		ADV_INFO("(%c)%02x ", notifData_notif->data[i], notifData_notif->data[i]);
	}
	ADV_INFO("\n");
#endif
	// add "end of string"
	notifData_notif->data[notifData_notif->dataLen] = '\0';

	pMote = MoteList_GetItemByMac(pMoteList, notifData_notif->macAddress);
	if (NULL == pMote) {
		ADV_ERROR("%s: target Mote not found\n", __func__);
		return 0;
	}

	if (DN_UDP_PORT_ADVMOTE==notifData_notif->srcPort && DN_UDP_PORT_ADVMOTE==notifData_notif->dstPort) {
		memset(&output, '\0', sizeof(adv_coap_packet_t));
		if ((iret = adv_coap_parse_pkt((unsigned char*)notifData_notif->data, &output)) == 0) {
			ADV_TRACE("%s: st=%d, payload=%s\n", __func__, pMote->MoteQueryState, output.payload);
			switch (pMote->MoteQueryState) {
				case Mote_Query_MoteInfo_Sent:
					if ((iret = adv_payloadParser_mote_info((unsigned char *)&output.payload, pMote)) != 0) {
						//ADV_ERROR("%s: Error(%d): Mote(id=%d) Info not recognized\n", __func__, iret, pMote->moteId);
						pMote->MoteQueryState = Mote_Query_Observe_Stop_Sent;
					} else {
						IpMgWrapper_fsm_scheduleEvent(
							CMD_PERIOD,
							&IpMgWrapper_done
						);
						pMote->MoteQueryState = Mote_Query_MoteInfo_Received;
						ADV_TRACE("%s: Mote Info name:%s, model:%s\n", __func__, pMote->hostName , pMote->productName);
						Application_Package_CheckMoteInfo(pMote);
					}
					break;
				case Mote_Query_SensorList_Sent:
					if ((iret = adv_payloadParser_sensor_list((unsigned char *)&output.payload, &pMote->sensorList)) != 0) {
						ADV_ERROR("%s: Error(%d): header not found\n", __func__, iret);
					} else {
						IpMgWrapper_fsm_scheduleEvent(
							CMD_PERIOD,
							&IpMgWrapper_done
						);
						pMote->MoteQueryState = Mote_Query_SensorList_Received;
						ADV_TRACE("%s: total sensors:%d\n", __func__, pMote->sensorList.total);
						Application_Package_CheckSensorList(pMote);
					}
					break;
				case Mote_Query_SensorList_Received: 
					if(pMote->scannedSensor != pMote->sensorList.total)
					{
						int idx = adv_payloadParser_sensor_info((unsigned char *)&output.payload, &pMote->sensorList);
						/*if (iRet == 0) {
						ADV_INFO("Sensor Info header found\n");
						Application_Package_CheckSensorInfo(pMote);
						pMote->scannedSensor++;
					} else {
						ADV_ERROR("Sensor Info header NOT found(%d)\n", iRet);
						}*/
						IpMgWrapper_MoteGetSensorStandard(pMote, idx);
						pMote->MoteQueryState = Mote_Query_SensorStandard_Received;
					}
					break;
					
				case Mote_Query_SensorStandard_Received:
					{
						int idx = adv_payloadParser_sensor_standard((unsigned char *)&output.payload, &pMote->sensorList);
						IpMgWrapper_MoteGetSensorHardware(pMote, idx);
						pMote->MoteQueryState = Mote_Query_SensorHardware_Received;
					}
					break;
					
				case Mote_Query_SensorHardware_Received:
					{
						int idx = adv_payloadParser_sensor_hardware((unsigned char *)&output.payload, &pMote->sensorList);
						//add_sensor_spec(&pMote->sensorList.item[idx]);

						pMote->sensorList.item[idx].SensorQueryState = Sensor_Query_SensorInfo_Received;
						pMote->MoteQueryState = Mote_Query_SensorList_Received;
						Application_Package_CheckSensorInfo(pMote);
						pMote->scannedSensor++;

						IpMgWrapper_fsm_scheduleEvent(
							CMD_PERIOD,
							&IpMgWrapper_done
						);
					}
					break;
				
				case Mote_Query_SendCmd_Sent:
				case Mote_Query_SendCmd_Started:
					{
						int iRet = adv_payloadParser_sensor_cmdback((unsigned char *)&output.payload, pMote);
						if (iRet == 0) {
							ADV_INFO("%s: Sensor Data received\n", __func__);
							pMote->MoteQueryState = Mote_Query_SensorList_Received;
							//printf("%s: Mote hostName=%s\n", __func__, pMote->hostName);
							if(pMote->cmdType == Mote_Cmd_SetMoteName) {
								//printf("%s: set Mote hostName=%s\n", __func__, pMote->hostName);
								pMote->MoteQueryState = Mote_Query_NotStart;
							}
						} else {
							ADV_ERROR("%s: Error(%d): Sensor Data error\n", __func__, iRet);
						}
					}
					break;
				case Mote_Query_Observe_Start_Sent:
				case Mote_Query_Observe_Started: {
					int iRet = adv_payloadParser_sensor_data((unsigned char *)&output.payload, &pMote->sensorList);
					if (iRet == 0) {
						ADV_TRACE("\n%s: Sensor Data received (Observe)\n", __func__);
#if 0
						for (i=0; i<pMote->sensorList.total; i++) {
							printf("sensor#%d: idx=%d, name=%s, objId=%s, unit=%s, resType=%s, min=%s, max=%s, model=%s, value=%s\n", i,
							pMote->sensorList.item[i].index,
							pMote->sensorList.item[i].name,
							pMote->sensorList.item[i].objId,
							pMote->sensorList.item[i].unit,
							pMote->sensorList.item[i].resType,
							pMote->sensorList.item[i].minValue,
							pMote->sensorList.item[i].maxValue,
							pMote->sensorList.item[i].model,
							pMote->sensorList.item[i].value
							);
						}
#endif
						if(pMote->MoteQueryState == Mote_Query_Observe_Start_Sent) {
							IpMgWrapper_fsm_scheduleEvent(
								CMD_PERIOD,
								&IpMgWrapper_done
							);
							ADV_INFO("\n%s: Observation received (moteId=%d)\n", __func__, pMote->moteId);
							pMote->MoteQueryState = Mote_Query_Observe_Started;
							Susiaccess_Report_UpdateAutoReport(pMote);
						}
						Application_Package_CheckSensorData(pMote);
						Susiaccess_Report_SensorData(pMote);
					} else {
						ADV_ERROR("%s: Error(%d): (MoteId=%d)Sensor Data error (Observe)\n", __func__, iRet, pMote->moteId);
					}
					break;
				}
				case Mote_Query_Observe_Stop_Sent:
					{
						int iRet = adv_payloadParser_sensor_cancelback((unsigned char *)&output.payload, pMote);
						if (iRet == 0) {
							IpMgWrapper_fsm_scheduleEvent(
								CMD_PERIOD,
								&IpMgWrapper_done
							);
							ADV_INFO("\n%s: Cancel observation received (moteId=%d)\n", __func__, pMote->moteId);
							Susiaccess_Report_UpdateAutoReport(pMote);
							// if all sensor data received
							if(pMote->sensorList.total != 0 && 
									pMote->scannedSensor == pMote->sensorList.total) {
								pMote->MoteQueryState = Mote_Query_SensorList_Received;
							} else {
								// no sensor data received
								pMote->MoteQueryState = Mote_Query_NotStart;
							}
						} else {
							//ADV_INFO("%s: Cancel mote(id=%d) Observation fail\n", __func__, pMote->moteId);
							if (isResponseTimeout(pMote->QuerySentTime)) {
								pMote->MoteQueryState = Mote_Query_MoteInfo_Sent;
								ADV_INFO("%s: Mote ", __func__);
								IpMgWrapper_printByteArray(pMote->macAddress, 8);
								ADV_INFO(" cancel observe timeout!\n");
							}
						}
					}
					break;
				default:
					//pMote->MoteQueryState = Mote_Query_Observe_Stop_Sent;
					break;
			}

#ifdef SHOW_LIST
			for (i=0; i<pMoteList->total; i++) {
				printf("mote(%d): ", i);
				IpMgWrapper_printByteArray(pMoteList->item[i].macAddress, 8);
				printf("\n");

				for (j=0; j<pMoteList->item[i].sensorList.total; j++) {
					printf("sensor#%d: idx=%d, name=%s, objId=%s, unit=%s, resType=%s, min=%s, max=%s, model=%s, value=%s\n", j,
						pMoteList->item[i].sensorList.item[j].index,
						pMoteList->item[i].sensorList.item[j].name,
						pMoteList->item[i].sensorList.item[j].objId,
						pMoteList->item[i].sensorList.item[j].unit,
						pMoteList->item[i].sensorList.item[j].resType,
						pMoteList->item[i].sensorList.item[j].minValue,
						pMoteList->item[i].sensorList.item[j].maxValue,
						pMoteList->item[i].sensorList.item[j].model,
						pMoteList->item[i].sensorList.item[j].value
						);
				}
			}
#endif

		} else {
			ADV_INFO("!!!!! %s : NOT Adv Coap data (%d)!!!!!\n", __func__, iret);
			pMote->MoteQueryState = Mote_Query_NotStart;
			pMote->scannedSensor = 0;
			MoteList_ResetMoteSensorList(pMote);
		}
	} 
	else if (DN_UDP_PORT_OAP==notifData_notif->srcPort && DN_UDP_PORT_OAP==notifData_notif->dstPort) {
		ADV_INFO("\n!!!!! %s : Dust OAP data !!!!!\n", __func__);
#if 0
		printf("INFO:	  notifData_notif->dataLen = %d\n", notifData_notif->dataLen);
		for (i=0;i<notifData_notif->dataLen;i++) {
			printf("%02x ", notifData_notif->data[i]);
		}
		printf("\n");
#endif
		pMote->manufacturer = Mote_Manufacturer_Dust;

		// report customized mote & sensor info to susiaccess
		if (0 == pMote->scannedSensor) {
			g_IOMoteId = pMote->moteId;
			MakeIOSensorData(pMote);
			Susiaccess_Report_SenNodeSendInfoSpec(pMote);
		}
		
		// parse OAP temperature
		if (OAP_NOTIF_TEMP_MSG_LEN == notifData_notif->dataLen &&
			OAP_NOTIF_TEMP_ADDR_LEN == notifData_notif->data[5] &&
			OAP_NOTIF_TEMP_ADDR_VALUE == notifData_notif->data[6] )
		{
			float tempValue = ((notifData_notif->data[OAP_NOTIF_TEMP_VALUE_ADDR_1] * 16 * 16) + notifData_notif->data[OAP_NOTIF_TEMP_VALUE_ADDR_2]) / (float)100;
			snprintf(pMote->sensorList.item[0].value, sizeof(pMote->sensorList.item[0].value), "%.2f", tempValue);
			Susiaccess_Report_SensorData(pMote);
		}
		else {
			pMote->MoteQueryState = Mote_Query_Observe_Started; 
			ADV_INFO("OAP data: not temperature data ");
			if (OAP_NOTIF_PUT_RESP_LEN == notifData_notif->dataLen) {
				ADV_INFO("(put response)!");
			}
			ADV_INFO(" \n");
		}
		if ( g_pCurrentMote->moteId == pMote->moteId ) {
			IpMgWrapper_fsm_scheduleEvent(
				CMD_PERIOD,
				&IpMgWrapper_done
			);
		}
	} 
	else {
		ADV_ERROR("!!!!! %s : Unknown data source !!!!!\n", __func__);
	}

}

static int HandleNotifIpData(uint8_t subCmdId)
{
	ADV_INFO("!!!!! DN_NOTIFID_NOTIFIPDATA !!!!!\n");
}

static int HandleNotifHealthReport(uint8_t subCmdId)
{
#if 0
	int i;
	dn_ipmg_notifHealthReport_nt* healthReport_notif;

	ADV_INFO("!!!!! %s (subCmdId=%d)!!!!!\n", __func__, subCmdId);
	healthReport_notif = (dn_ipmg_notifHealthReport_nt*) app_vars.notifBuf;
	ADV_INFO("!!!!! %s payload lens=%d !!!!!\n", __func__, healthReport_notif->payloadLen);
	for (i=0;i<healthReport_notif->payloadLen;i++) {
		ADV_INFO("%02x ", healthReport_notif->payload[i]);
	}
	ADV_INFO("\n");
#else
	ADV_INFO("!!!!! DN_NOTIFID_NOTIF_HEALTHREPORT !!!!!\n");
#endif
}

//=========================== callback functions for ipmg =====================
void dn_ipmg_notif_cb(uint8_t cmdId, uint8_t subCmdId)
{
	switch (cmdId) {
		case DN_NOTIFID_NOTIFEVENT:
			HandleNotifEvent(subCmdId);
			break;
		case DN_NOTIFID_NOTIFLOG:
			HandleNotifLog(subCmdId);
			break;
		case DN_NOTIFID_NOTIFDATA:
			HandleNotifData(subCmdId);
			break;
		case DN_NOTIFID_NOTIFIPDATA:
			HandleNotifIpData(subCmdId);
			break;
		case DN_NOTIFID_NOTIFHEALTHREPORT:
			//HandleNotifHealthReport(subCmdId);
			break;
		default:
			ADV_WARN("%s: Warning: Unknown Notif command:%d\n", __func__, cmdId);
			break;
	}
}

extern  void dn_ipmg_reply_cb(uint8_t cmdId) {
	(*app_vars.replyCb)();
}

extern void dn_ipmg_status_cb(uint8_t newStatus)
{
	switch (newStatus) {
		case DN_SERIAL_ST_CONNECTED:
			ADV_INFO("INFO:     connected\n");
				IpMgWrapper_fsm_scheduleEvent(
					INTER_FRAME_PERIOD,
					&IpMgWrapper_api_getNextMoteConfig
				);
			break;
		case DN_SERIAL_ST_DISCONNECTED:
			ADV_WARN("WARNING:  disconnected\n");
			g_MgrReConnect = TRUE;
			memset(app_vars.currentMac,0,sizeof(app_vars.currentMac));
			IpMgWrapper_fsm_scheduleEvent(
				INTER_FRAME_PERIOD,
				&IpMgWrapper_api_initiateConnect
			);
			break;
		default:
			ADV_ERROR("ERROR:    unexpected newStatus\n");
	}
}

#ifdef USE_SERIALMUX
extern void dn_socket_init(dn_socket_rxByte_cbt rxByte_cb) {
   // remember function to call back
   app_vars.ipmg_socket_rxByte_cb = rxByte_cb;

   if( serialmux_open(app_vars.ipaddr, app_vars.port) != 0 )
        exit(-1);
}

extern void dn_socket_txByte(uint8_t byte) {
}

#else
//=========================== port to Arduino =================================

//===== definition of interface declared in uart.h

extern void dn_uart_init(dn_uart_rxByte_cbt rxByte_cb) {
   // remember function to call back
   app_vars.ipmg_uart_rxByte_cb = rxByte_cb;
   
   // open the serial 1 port on the Arduino Due
   if( RS232_OpenComport(app_vars.comport, 115200, mode) != 0 )
		exit(-1);
}

extern void dn_uart_txByte(uint8_t byte) {
   // write to the serial 1 port on the Arduino Due
//printf("%02x:", byte);
   RS232_SendByte(byte);
}
#endif // USE_SERIALMUX

extern void dn_uart_txFlush() {
   // nothing to do since Arduino Due serial 1 driver is byte-oriented
}

//===== definition of interface declared in lock.h

extern void dn_lock() {
   // this sample Arduino code is single threaded, no need to lock.
}

extern void dn_unlock() {
   // this sample Arduino code is single threaded, no need to lock.
}

//===== definition of interface declared in endianness.h

extern void dn_write_uint16_t(uint8_t* ptr, uint16_t val) {
   // arduino Due is a little-endian platform
   ptr[0]     = (val>>8)  & 0xff;
   ptr[1]     = (val>>0)  & 0xff;
}

extern void dn_write_uint32_t(uint8_t* ptr, uint32_t val) {
   // arduino Due is a little-endian platform
   ptr[0]     = (val>>24) & 0xff;
   ptr[1]     = (val>>16) & 0xff;
   ptr[2]     = (val>>8)  & 0xff;
   ptr[3]     = (val>>0)  & 0xff;
}

extern void dn_read_uint16_t(uint16_t* to, uint8_t* from) {
   // arduino Due is a little endian platform
   *to        = 0;
   *to       |= (from[1]<<0);
   *to       |= (from[0]<<8);
}
extern void dn_read_uint32_t(uint32_t* to, uint8_t* from) {
   // arduino Due is a little endian platform
   *to        = 0;
   *to       |= (from[3]<<0);
   *to       |= (from[2]<<8);
   *to       |= (from[1]<<16);
   *to       |= (from[0]<<24);
}
