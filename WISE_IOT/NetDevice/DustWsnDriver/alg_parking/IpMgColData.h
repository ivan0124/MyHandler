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

#ifndef IPMGCOLDATA_H
#define IPMGCOLDATA_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>
#include <time.h>


//=========================== defines =========================================

#ifndef BOOL
typedef int   BOOL; 
#define  FALSE  0
#define  TRUE   1
#endif

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
#define BACKOFF_AFTER_TIMEOUT     1000       // number of ms to back off after a timeout occurs
#define OAP_RESPONSE_TIMEOUT      10000      // max number of ms to wait for OAP response

//=========================== typedef =========================================

#define MAX_SAMPLE_NUM      1
//#define MAX_SAMPLE_NUM      10

#define TRUE	1
#define FALSE	0

typedef char INT8;
typedef unsigned char UINT8;
typedef unsigned short int UINT16;
typedef unsigned int UINT32;
typedef short int INT16;
//typedef unsigned char		BOOL;
typedef short int AXIS_t;

typedef struct {
  INT16 AXIS_X;
  INT16 AXIS_Y;
  INT16 AXIS_Z;
}MagAxesRaw_t;

typedef struct {
    UINT8 u8Month;
    UINT8 u8Day;
    UINT8 u8Hour;
    UINT8 u8Min;
    UINT8 u8Sec;    
}TTimeStamp;

typedef struct {
    UINT8 u8Hdr;
    TTimeStamp tTimeStamp;
    INT16 i16Temperature;
    UINT16 u16Heading;
    UINT16 u16Gauss;
    UINT16 u16PacketID;  
    INT16 iXBase;
    INT16 iYBase;
    INT16 iZBase;
    INT16 i16HeadBase;
    INT16 i16HeadData;
    INT16 i16HeadGap;
    BOOL bXYZResult;
    BOOL bHeadingResult;
    BOOL bSingleResult;
    MagAxesRaw_t atMagData[MAX_SAMPLE_NUM];
    UINT16 dXYZBase;
    UINT16 dXYZData;
}TSensorDataSet;

/*
typedef struct {
    UINT8 u8Hdr;
    TTimeStamp tTimeStamp;
    INT16 i16Temperature;
    UINT16 u16Heading;
    UINT16 u16Gauss;
    UINT16 u16PacketID;  
    MagAxesRaw_t atMagData[MAX_SAMPLE_NUM];            
}TSensorDataSet;
*/
/*
typedef struct {
    UINT8 u8Hdr;
    INT16 i16Temperature;
    TTimeStamp tTimeStamp;
    MagAxesRaw_t atMagData[MAX_SAMPLE_NUM];
    UINT16 u16Heading;
    UINT16 u16Gauss;
    UINT16 u16PacketID;              
}TSensorDataSet;
*/

typedef void (*fsm_timer_callback)(void);
typedef void (*fsm_reply_callback)(void);
typedef void (*data_generator)(uint16_t* returnVal);

//=========================== IpMgWrapper object ==============================
void IpMgWrapper_setup(unsigned char *port);
void IpMgWrapper_loop();
void IpMgWrapper_fsm_scheduleEvent(uint16_t delay,fsm_timer_callback  cb);
void IpMgWrapper_fsm_setCallback(fsm_reply_callback cb);
void IpMgWrapper_api_response_timeout();
void IpMgWrapper_oap_response_timeout();
void IpMgWrapper_api_initiateConnect();
void IpMgWrapper_api_subscribe();
void IpMgWrapper_api_subscribe_reply();
void IpMgWrapper_api_getNextMoteConfig();
void IpMgWrapper_api_getNextMoteConfig_reply();
void IpMgWrapper_api_coldata();
void IpMgWrapper_api_coldata_reply();
//=== helpers
void IpMgWrapper_printState(uint8_t state);
void IpMgWrapper_printByteArray(uint8_t* payload, uint8_t length);

#endif
