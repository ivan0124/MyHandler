#ifndef _DUSTWSNDRV_H_
#define _DUSTWSNDRV_H_

#include <pthread.h>

#define MOTE_LIST
//#define SENSOR_LIST
/* **************************************************************************************
 *
 *	Type and Define
 *
 * ***************************************************************************************/

#ifndef BOOL
typedef int   BOOL; 
#define  FALSE  0
#define  TRUE   1
#endif


#define MAX_MAC_LEN					32
#define MAX_NAME_LEN				128
#define MAX_TYPE_LEN				16
#define MAX_VALUE_LEN				512
#define MAX_UNIT_LEN				512
#define MAX_OID_LEN					512
#define MAX_RESOURCETYPE_LEN 		512
#define MAX_MODEL_LEN				512
#define MAX_MODE_LEN				512
#define MAX_RULE_LEN				512
#define MAX_FORMAT_LEN				512
#define MAX_FORMATVER_LEN 			512
#define MAX_HOSTNAME_LEN			32
#define MAX_PRODUCTNAME_LEN			32

#define MAX_SENDATA_LEN				256

#define MAX_MOTE_NUM				100
#define MAX_SENSOR_NUM				20
#define DUSTWSN_RESPONSE_TIMEOUT	(SENDDATA_RESPONSE_TIMEOUT / 1000)
//#define DUSTWSN_RESPONSE_TIMEOUT	(SENDDATA_RESPONSE_TIMEOUT / 500)

#define MAX_SOTFWAREVERSION_LEN		12	
#define MAX_NEIGHBOR_NUM		    10	

#define RULE_DST_DO1_NAME		"Fan 1"
#define RULE_DST_DO2_NAME		"Fan 2"
#define RULE_DST_DO3_NAME		"Light"

typedef enum {
	Mote_Query_NotStart = 0,
	Mote_Query_MoteInfo_Sent,
	Mote_Query_MoteInfo_Received,
	Mote_Query_SensorList_Sent,
	Mote_Query_SensorList_Received,
	Mote_Query_SensorStandard_Received,
	Mote_Query_SensorHardware_Received,
	Mote_Query_SendCmd_Sent,
	Mote_Query_SendCmd_Started,
	Mote_Query_Observe_Start_Sent,
	Mote_Query_Observe_Started,
	Mote_Query_Observe_Stop_Sent
} MOTE_QUERY_STATE;

typedef enum {
	Sensor_Query_NotStart = 0,
	Sensor_Query_SensorInfo_Sent,
	Sensor_Query_SensorInfo_Received
//	Sensor_Query_SensorStandard_Sent,
//	Sensor_Query_SensorStandard_Received,
//	Sensor_Query_SensorHardware_Sent,
//	Sensor_Query_SensorHardware_Received
} SENSOR_QUERY_STATE;

typedef enum {
	Mote_Cmd_None = 0,
	Mote_Cmd_SetSensorValue,
	Mote_Cmd_SetMoteName,
	Mote_Cmd_SetMoteReset,
	Mote_Cmd_SetAutoReport
} MOTE_CMD_TYPE;

typedef enum {
	Mote_Manufacturer_Advantech = 0,
	Mote_Manufacturer_Dust,
} MOTE_MANUFACTURER;

typedef enum {
	Mote_AppPackage_Advantech_General = 0,
	Mote_AppPackage_Advantech_iParking,
} MOTE_APP_PACKAGE;

typedef enum {
	Mote_AppPackage_NotInit = 0,
	Mote_AppPackage_Inited,
	//Mote_Master_IoInit_D4_Enable,
	//Mote_Master_IoInit_D4_Enabled,
	//Mote_Master_IoInit_D5_Enable,
	//Mote_Master_IoInit_D5_Enabled,
	//Mote_Master_IoInit_D6_Enable,
	//Mote_Master_IoInit_D6_Enabled,	
} MOTE_APPPACKAGE_STATE;

typedef enum {
	Mote_Master_DI,
	Mote_Master_D0,
	Mote_Master_D1,
	Mote_Master_D2,
	Mote_Master_D3,

	Mote_Master_DO,
	Mote_Master_D4,
	Mote_Master_D5,
	Mote_Master_D6,

	Mote_Master_AI,
	Mote_Master_A0,
	Mote_Master_A1,
	Mote_Master_A2,
	Mote_Master_A3,
} MOTE_MASTER_IO;

typedef struct SENSOR_INFO {
	SENSOR_QUERY_STATE SensorQueryState;
	time_t QuerySentTime;

	char name[MAX_NAME_LEN];				// "n"
	int index;
	char value[MAX_VALUE_LEN];				// "v"
	char unit[MAX_UNIT_LEN];				// "u"
	char valueType[MAX_TYPE_LEN];			// "type"
	BOOL hasMinMax;
	char minValue[MAX_VALUE_LEN];			// "min"
	char maxValue[MAX_VALUE_LEN];			// "max"
	char objId[MAX_OID_LEN];				// sid
	char resType[MAX_RESOURCETYPE_LEN];		// "rt"
	char model[MAX_MODEL_LEN];				// mdl
	char mode[MAX_MODE_LEN];				// "asm"
	char rule[MAX_RULE_LEN];
	char format[MAX_FORMAT_LEN];			// "st"
	char formatVer[MAX_FORMATVER_LEN];
	char x[MAX_VALUE_LEN];			// "X"
	char y[MAX_VALUE_LEN];			// "Y"
	char z[MAX_VALUE_LEN];			// "Z"
#ifdef SENSOR_LIST
	struct SENSOR_INFO *prev;
	struct SENSOR_INFO *next;
#endif
} Sensor_Info_t;

typedef struct SENSOR_LIST {
	int total;
#ifdef SENSOR_LIST
	Sensor_Info_t *item;
#else
	Sensor_Info_t item[MAX_SENSOR_NUM];
#endif
} Sensor_List_t;

typedef struct NEIGHBOR_INFO {
	unsigned char macAddress[8];
} Neighbor_Info_t;

typedef struct NEIGHBOR_LIST {
	int total;
	Neighbor_Info_t item[MAX_NEIGHBOR_NUM];
} Neighbor_List_t;

typedef struct MOTE_INFO {
	Sensor_List_t sensorList;
	int scannedSensor;

	unsigned char macAddress[8];
	int moteId;
	BOOL isAP;
	int state;
	int dustReserved;
	BOOL isRouting;

	int reserved;

	MOTE_MANUFACTURER manufacturer;
	MOTE_APP_PACKAGE AppPackage;
	MOTE_APPPACKAGE_STATE AppPkgState;

	MOTE_QUERY_STATE MoteQueryState;
	time_t QuerySentTime;

	char format[MAX_FORMAT_LEN];
	char formatVer[MAX_FORMATVER_LEN];

	char hostName[MAX_HOSTNAME_LEN];
	char productName[MAX_PRODUCTNAME_LEN];

	char softwareVersion[MAX_SOTFWAREVERSION_LEN];
	BOOL bReset;
	char netName[MAX_HOSTNAME_LEN];
	char netSWversion[MAX_SOTFWAREVERSION_LEN];
	int reliability;
	MOTE_CMD_TYPE cmdType;
	char cmdName[16];
	char cmdParam[16];
	BOOL bPathUpdate;
	Neighbor_List_t neighborList;

#ifdef MOTE_LIST
	struct MOTE_INFO *prev;
	struct MOTE_INFO *next;
#endif
} Mote_Info_t;

typedef struct MOTE_LIST {
	int total;
#ifdef MOTE_LIST
	Mote_Info_t *item;
#else
	Mote_Info_t item[MAX_MOTE_NUM];
#endif	
} Mote_List_t;

typedef struct SMARTMESH_IP_INFO {		/* 2 WSN manager case */
	pthread_mutex_t moteListMutex;
	Mote_List_t MoteList;
	/* manager & network info */
	int NetReliability;
} SmartMeshIP_Info_t;

typedef struct WSNDRV_CONTEXT{
   pthread_t threadHandler;
   //pthread_t threadHandler;
   BOOL isThreadRunning;
   //report_context_t reportctx;
   SmartMeshIP_Info_t SmartMeshIpInfo;
   int thread_nr;
} DustWsn_Context_t;

int Susiaccess_Report_SenNodeRegister(Mote_Info_t *pTargetMote);
int Susiaccess_Report_SenNodeSendInfoSpec(Mote_Info_t *pTargetMote);
int Susiaccess_Report_SensorData(Mote_Info_t *pMote);
int Susiaccess_Report_MoteLost(Mote_Info_t *pMote);

#endif /* _DUSTWSNDRV_H_ */
