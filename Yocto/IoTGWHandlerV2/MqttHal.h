/*
 * Copyright (c) 2016, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 * ChangeLog:
 *  2016/01/18 Chinchen: Initial version
 */

#ifndef _MQTT_HAL_H
#define _MQTT_HAL_H

#include "AdvJSON.h"

//-----------------------------------------------------------------------------
// User API
//-----------------------------------------------------------------------------
#define NOTIFY_TOPIC  "notify"
#define AGENTINFOACK_TOPIC "agentinfoack"
#define AGENTACTIONREQ_TOPIC  "agentactionreq"
#define DEVICEINFO_TOPIC      "deviceinfo"
#define WA_PUB_WILL_TOPIC    "willmessage"
#define WA_SUB_CBK_TOPIC     "agentcallbackreq"

#define OBJ_INFO_SPEC              "[susiCommData][infoSpec]"
#define OBJ_IOTGW_INFO_SPEC        "[susiCommData][infoSpec][IoTGW]"
#define OBJ_SENHUB_INFO_SPEC       "[susiCommData][infoSpec][SenHub]"
#define OBJ_NAME_SENHUB_INFO_SPEC  "[susiCommData][infoSpec][SenHub][Info][e][0][sv]"
#define OBJ_DATA                   "[susiCommData][data]"
#define OBJ_IOTGW_DATA             "[susiCommData][data][IoTGW]"
#define OBJ_SENHUB_DATA            "[susiCommData][data][SenHub]"
#define OBJ_AGENT_ID              "[susiCommData][agentID]"
#define OBJ_DEVICE_TYPE            "[susiCommData][type]"
#define OBJ_DEVICE_STATUS          "[susiCommData][status]"
#define OBJ_DEVICE_ID              "[susiCommData][devID]"
#define OBJ_DEVICE_MAC             "[susiCommData][mac]"
#define OBJ_DEVICE_SN              "[susiCommData][sn]"
#define OBJ_DEVICE_HOSTNAME        "[susiCommData][hostname]"
#define OBJ_DEVICE_PRODUCTNAME     "[susiCommData][product]"
#define OBJ_DEVICE_VERSION         "[susiCommData][version]"
#define OBJ_AGENT_ID               "[susiCommData][agentID]"
#define OBJ_STATUS_CODE            "[susiCommData][sensorInfoList][e][0][StatusCode]"
#define OBJ_SESSION_ID             "[susiCommData][sessionID]"
#define OBJ_RESULT_STR             "[susiCommData][result]"
#define OBJ_E_SENDATA_SENHUB       "[SenHub][SenData][e]"
#define OBJ_E_INFO_SENHUB          "[SenHub][Info][e]"
#define OBJ_E_NET_SENHUB           "[SenHub][Net][e]"
#define OBJ_E_NET_HEALTH_SENHUB    "[SenHub][Net][e][0][v]"
#define OBJ_E_NET_NEIGHBORS_SENHUB "[SenHub][Net][e][1][sv]"
#define OBJ_E_NET_SWVER_SENHUB     "[SenHub][Net][e][2][sv]"
#define OBJ_SUSI_COMMAND           "[susiCommData][commCmd]"

#define MAX_HOSTNAME_LEN         32
#define MAX_PRODUCTNAME_LEN	     32
#define MAX_SOFTWAREVERSION_LEN  12	
#define MAX_MACADDRESS_LEN       18
#define MAX_DEVICE_ID_LEN        18
#define MAX_DEVICE_SN_LEN        18	
#define MAX_CONNECTIVITY_TYPE_LEN        16
#define MAX_SENDATA_LEN          256

#define MAX_JSON_NODE_SIZE       1024

#define IFACENAME		"eth0"
#define CONTYPE			"LAN"
#define CONNAME(d)		CONTYPE#d

#define CONNECTIVITY_CAPABILITY  1
#define SENSOR_HUB_CAPABILITY    2

#define HEART_BEAT_TIMEOUT 3*60
#define CHECK_NODE_LIST_TIME 5000  //5 seconds
#define IP_BASE_CONNECTIVITY_NAME  "LAN"

#define GATEWAY_ID_PREFIX  "0000"
#define CONNECTIVITY_ID_PREFIX  "0007"
#define SENSORHUB_ID_PREFIX  "0017"


typedef enum {
        GATEWAY_HEART_BEAT = 1,
        GATEWAY_OS_INFO,
	REGISTER_GATEWAY_CAPABILITY,
        REGISTER_SENSOR_HUB_CAPABILITY,
        GATEWAY_CONNECT,
        GATEWAY_DISCONNECT,
	UPDATE_GATEWAY_DATA,
	SENSOR_HUB_CONNECT,
        SENSOR_HUB_DISCONNECT,
	UPDATE_SENSOR_HUB_DATA,
        REPLY_GET_SENSOR_REQUEST,
        REPLY_SET_SENSOR_REQUEST
} MQTT_ACTION;

typedef enum {
        DEVICE_TYPE_UNKNOWN = 1,
        TYPE_GATEWAY,
	TYPE_CONNECTIVITY,
        TYPE_SENSOR_HUB
} NODE_TYPE;

typedef enum {
        OS_TYPE_UNKNOWN = 1,
        OS_NONE_IP_BASE,
	OS_IP_BASE
} GATEWAY_OS_INFO_TYPE;

typedef enum {
        STATUS_UNKNOWN = 1,
        STATUS_CONNECTING,
	STATUS_CONNECTED,
        STATUS_DISCONNECTED
} DEVICE_STATUS;

typedef enum {
	Mote_Report_CMD2000 = 0,
	Mote_Report_CMD2001,
	Mote_Report_CMD2002,
	Mote_Report_CMD2003
} MOTE_REPORT_STATE;

typedef enum {
	Mote_Cmd_None = 0,
	Mote_Cmd_SetSensorValue,
	Mote_Cmd_SetMoteName,
	Mote_Cmd_SetMoteReset,
	Mote_Cmd_SetAutoReport
} MOTE_CMD_TYPE;
	
typedef struct _senhub_info_t {
	struct _senhub_info_t *next;
	int id;                                        // index
        char devID[MAX_DEVICE_ID_LEN];         //device ID
        char sn[MAX_DEVICE_SN_LEN];            //serial number
	char macAddress[MAX_MACADDRESS_LEN];  // mac address
	char hostName[MAX_HOSTNAME_LEN];               // hostname
	char productName[MAX_PRODUCTNAME_LEN];         // product name
	char version[MAX_SOFTWAREVERSION_LEN]; // software version
	int state;                                     // state
	JSONode *jsonNode;                             // json data
} senhub_info_t;

struct node
{
    int nodeType;
    int state;
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN];
    int virtualGatewayOSInfo;
    char connectivityType[MAX_CONNECTIVITY_TYPE_LEN];
    char connectivityDevID[MAX_DEVICE_ID_LEN];
    char connectivitySensorHubList[1024];
    //char connectivityNeighborList[1024];
    char* connectivityInfo;
    char* connectivityInfoWithData;
    char sensorHubDevID[MAX_DEVICE_ID_LEN];
    time_t last_hb_time;
    time_t start_connecting_time;

    struct node *next;
};

struct gw_node
{
    char virtualGatewayDevID[MAX_DEVICE_ID_LEN];
    struct gw_node *next;
};


int MqttHal_GetNetworkIntfaceMAC(char *_ifname, char* _ifmac);
int MqttHal_Init();
int MqttHal_Uninit();
void MqttHal_Proc();
void MqttHal_UpdateIntf(int bUpdate);
void MqttHal_GatewayMAC(char* gatewayMac);
int MqttHal_GetMacAddrList(char *pOutBuf, const int outBufLen, int withHead);
senhub_info_t* MqttHal_GetMoteInfoByMac(char *strMacAddr);
int MqttHal_Publish(char *macAddr, int cmdType, char *strName, char *strValue);

#endif // _MQTT_HAL_H
