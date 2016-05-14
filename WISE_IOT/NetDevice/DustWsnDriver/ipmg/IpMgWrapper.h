/*
Copyright (c) 2014, Dust Networks.  All rights reserved.

This library is an "wrapper" around the generic SmartMesh C library.

This library will:
- Connect to the SmartMesh IP manager.
- Subscribe to data notifications.
- Get the MAC address of all nodes in the network.
- Send an OAP(On-Chip Application Protocol) command to blink each node's LED in a round-robin fashion.
  
\license See attached DN_LICENSE.txt.
*/

#ifndef IPMGWRAPPER_H
#define IPMGWRAPPER_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>

#include "DustWsnDrv.h"

#include "dn_ipmg.h"

//=========================== defines =========================================

#define TIME_T                    unsigned long

#define BAUDRATE_CLI              9600
#define BAUDRATE_API              115200

// mote state
#define MOTE_STATE_LOST           0x00
#define MOTE_STATE_NEGOTIATING    0x01
#define MOTE_STATE_OPERATIONAL    0x04

// service types
#define SERVICE_TYPE_BW           0x00

#define IPv6ADDR_LEN              16
#define DEFAULT_SOCKETID          22

// subscription
#define SUBSC_FILTER_EVENT        0x02
#define SUBSC_FILTER_LOG          0x02
#define SUBSC_FILTER_DATA         0x10
#define SUBSC_FILTER_IPDATA       0x20
#define SUBSC_FILTER_HR           0x40

#define DN_UDP_PORT_OAP           0xf0b9

//===== fsm

#define CMD_PERIOD                0          // number of ms between two commands being sent
#define INTER_FRAME_PERIOD        1          // min number of ms between two TX frames over serial port
//#define SERIAL_RESPONSE_TIMEOUT   100        // max number of ms to wait for serial response
#define SERIAL_RESPONSE_TIMEOUT   5000        // max number of ms to wait for serial response
#define BACKOFF_AFTER_TIMEOUT     1000       // number of ms to back off after a timeout occurs
#define OAP_RESPONSE_TIMEOUT      10000      // max number of ms to wait for OAP response
#define SENDDATA_RESPONSE_TIMEOUT      10000  // max number of ms to wait for SendData response

typedef enum {
	SEND_CMD_NONE = 0,
	SEND_CMD_GET_SENSORHUB_INFO,
	SEND_CMD_GET_SENSOR_INDEX,
	SEND_CMD_GET_SENSOR_INFO,
	SEND_CMD_GET_SENSOR_STANDRAD,
	SEND_CMD_GET_SENSOR_HARDWARE,

	SEND_CMD_SET_SENSOR_VALUEBYNAME,
	SEND_CMD_SET_SENSORHUB_NAME,
	SEND_CMD_SET_SENSORHUB_RESET,

	SEND_CMD_OBSERVE,
	SEND_CMD_CANCEL_OBSERVE
} SendCmdID;

//=========================== typedef =========================================

typedef void (*fsm_timer_callback)(void);
typedef void (*fsm_reply_callback)(void);
typedef void (*data_generator)(uint16_t* returnVal);

//=========================== IpMgWrapper object ==============================
void IpMgWrapper_loop();
void IpMgWrapper_fsm_scheduleEvent(uint16_t delay,fsm_timer_callback  cb);
void IpMgWrapper_fsm_setCallback(fsm_reply_callback cb);
void IpMgWrapper_api_response_timeout();
void IpMgWrapper_oap_response_timeout();
void IpMgWrapper_api_initiateConnect();
void IpMgWrapper_api_subscribe();
void IpMgWrapper_api_subscribe_reply();
void IpMgWrapper_api_resetSystem();
void IpMgWrapper_api_resetSystem_reply();
void IpMgWrapper_api_getNextMoteConfig();
void IpMgWrapper_api_getNextMoteConfig_reply();
void IpMgWrapper_api_getNextPathInfo();
void IpMgWrapper_api_getNextPathInfo_reply();
void IpMgWrapper_api_getMoteInfo(void);
void IpMgWrapper_api_getMoteInfo_reply(void);
void IpMgWrapper_api_setMasterIOByName(Mote_Info_t *pMote);
void IpMgWrapper_api_sendData_reply();
void IpMgWrapper_Oap_SendData(unsigned char *payload, int payloadLen);
void IpMgWrapper_api_getSystemInfo(void);
void IpMgWrapper_api_getSystemInfo_reply(void);
void IpMgWrapper_api_getNetworkInfo(void);
void IpMgWrapper_api_getNetworkInfo_reply(void);
void IpMgWrapper_api_setMoteAutoReport(Mote_Info_t *pMote);
void IpMgWrapper_api_sendCmd(Mote_Info_t *pMote, SendCmdID _cmdId, int _param1, int _param2);
//=== helpers
void IpMgWrapper_printState(uint8_t state);
void IpMgWrapper_printByteArray(uint8_t* payload, uint8_t length);
int IpMgWrapper_PrintMoteList(void);

//int IpMgWrapper_SaveMoteInfo(Mote_Info_t *pMoteInfo);	// not used
// public function
void IpMgWrapper_Init(unsigned char *port);
void IpMgWrapper_UnInit(void);
int IpMgWrapper_GetTotalMotes(void);
int IpMgWrapper_MoteGetSensorIndex(Mote_Info_t *pMote);
int IpMgWrapper_MoteGetSensorInfo(Mote_Info_t *pMote, int sensorIndex);
int IpMgWrapper_MoteGetMotePath(Mote_Info_t *pMote);
int IpMgWrapper_MoteGetMoteInfo(Mote_Info_t *pMote);
int IpMgWrapper_MoteSetMoteReset(Mote_Info_t *pMote);
int IpMgWrapper_MoteSendDataObserve(Mote_Info_t *pMote);
int IpMgWrapper_GetManagerSWVersion(char *pManagerSWVer);
int IpMgWrapper_Scheduling(void);
int IpMgWrapper_getIOMoteId(void);

uint8_t* IpMgWrapper_GetMotesMacById(int id);
int IpMgWrapper_GetFSRValue(uint8_t* macAddress);

#endif
