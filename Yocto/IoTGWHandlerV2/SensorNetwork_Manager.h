/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/13 by Eric Liang															     */
/* Modified Date: 2015/03/13 by Eric Liang															 */
/* Abstract       :  Sensor Network Manager Internal Header File	    		             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __SensorNetwork_Manager_H__
#define __SensorNetwork_Manager_H__


/*#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once

#ifndef SAMPLE_API
	#define SAMPLE_API WINAPI
#endif
#else
	#define SAMPLE_API
#endif*/

#ifdef __cplusplus 
extern "C" { 
#endif 

//#define LINUX 

//#define HARD_CODE_FOR_DEMOKIT 1
#define _MULTI

// Note Modify The Hash Table Key not fixed

#define ONE_SENHUYB_HASH_TABLE_SZIE  50

#define MAX_SENHUB_HASH_TABLE_SIZE  1000    // 1000 * 4 Bytes (void*) = 4 KB

#define MAX_DATA_HASH_TABLE_SIZE       100   // 100 * 4 Bytes (void*) = 400 Bytes

#define MAX_NET_DRIVER_NUM                   4

#define BN_INIT_INDEX 2000
#define N_INIT_INDEX 0

// SN Module Config INI
#define SN_MODULE_CONFIG "SNModuleConfig.ini"
#define APPNAME_SN "SensorNetwork"
#define KEY_SN_NUM "NumberofModule"
#define KEY_SN_PATH "ModulePath"

#if defined(__linux)//linux
#define SN_MANAGER_LIB_NAME		"libIoTSensorManager.so"
#define DUSTLINK_SN_LIB_NAME	"libDustWsnDrv.so"
#define WSN_SIMULATOR_NAME		"libWsnSimulator.so"
#elif defined(WIN32) //windows
	#define SN_MANAGER_LIB_NAME		"SNManagerAPI.dll"
	#define DUSTLINK_SN_LIB_NAME	"libDustWsnDrv.dll"
	#define WSN_SIMULATOR_NAME		"MoteSim.dll"
#endif

#define RETURN_RT_FORMAT "{ \"StatusCode\": %s, \"Result\": {\"sv\":\"%s\"}}"  // { "StatusCode": 200, "Result": "Success" }

#define RETURN_FORMAT "{ \"StatusCode\": %s, \"Result\": %s }"  // { "StatusCode": 200, "Result": { JSON Object } }

#define GET_SET_JSON_FORMAT "{\"n\":\"%s\", %s}"   // {"n":"Name", "sv":"ROOM1"}
#define GET_JSON_FORMAT "{\"n\":\"%s\"}"   // {"n":"Name"}

#define MAX_UID_SIZE          64
#define MAX_TYPE_SIZE          32
#define MAX_DEF_DATA_SIZE 1024
#define MAX_SET_BUF_SIZE 256


static const char *g_nHttp_Code[14] = {"200", "500", "405", "405", "404", "426", "410", "400", "415", "422", "416", "503", "408", "501"}; 
static const char *g_szHttp_Status[ ] =	{	"Success",                  // 0  
																		"Fail",						    // -1
																		"Writ Only",               // -2
																		"Read Only",				// -3
																		"Not Found",				// -4
																		"Resource Locked",	// -5
																		"Resource Lose",		// -6
																		"Request Error",		// -7
																		"Format Error",			// -8
																		"Syntax Error",			// -9
																		"Out of Range",			// -10
																		"Sys Busy",					// -11
																		"Request Timeout",	// -12
																		"Not Implement",		// -13
																    };

// HTTP Status
#define HTTP_RC_200 "200" //"200 OK"
#define HTTP_RC_201 "201" //"201 Created"
#define HTTP_RC_202 "202" //"202 Accepted"
#define HTTP_RC_203 "203" //"203 Non-Authoritative Information"
#define HTTP_RC_204 "204" //"204 No Content"

// Client Error Code
#define HTTP_RC_400 "400" //"400 Bad Request"
#define HTTP_RC_401 "401" //"401 Unauthorized"
#define HTTP_RC_403 "403" //"403 Forbidden"
#define HTTP_RC_404 "404" //"404 Not Found"
#define HTTP_RC_405 "405" //"405 Method Not Allowed"
#define HTTP_RC_406 "406" //"406 Not Acceptable"
#define HTTP_RC_408 "408" //"408 Reuest Timeout"
#define HTTP_RC_409 "409" //"409 Conflict"
#define HTTP_RC_410 "410" //"410 Gone"
#define HTTP_RC_415 "415" //"415 Unsupported Media Type"
#define HTTP_RC_416 "416" //"416 Requested Range Not Satisfiable"
#define HTTP_RC_426 "426" //"426 Locked"
// Server Error Code
#define HTTP_RC_500 "500" //"500 Internal Server Error"
#define HTTP_RC_501 "501" //"501 Not Implemented"
#define HTTP_RC_503 "503" //"503 Service Unavailable"

#define REPLY_RESULT_200		"Success"					// 200 ( Note: Success to handle this command )
#define REPLY_RESULT_202		"Accepted"				// 202 ( Note: Accepted this command )
#define REPLY_RESULT_500		"Fail"							// 500 ( Note: Fail to Get / Set )
#define REPLY_RESULT_400		"Request Error"			//  400 ( Note: RESTful API syntax or format Error )               
#define REPLY_RESULT_405R 	"Read Only"				// 405 ( Note: This resource is read only )
#define REPLY_RESULT_405W	"Writ Only"				// 405 ( Note: This resource is write only )

#define REPLY_RESULT_416		"Out of Range"			// 416 ( Note: Set value range is out of range )
#define REPLY_RESULT_426		"Resource Locked"  // 426 ( Note: This resource be setting now by another client )
#define REPLY_RESULT_503		"Sys Busy"					// 503 ( Note: System is busy        )
#define REPLY_RESULT_415		"Format Error"			// 415 ( Note: Not JSON Format )
#define REPLY_RESULT_408		"Request Timeout" // 408 ( Note: WSN Setting Failed Timeout )
#define REPLY_RESULT_422		"Syntax Error"			// 422 ( Note: format is correct but syntax error )
#define REPLY_RESULT_404		"Not Found"				// 404 ( Note: Resource Not Found )
#define REPLY_RESULT_410		"Resource Lose"      // 410 ( Note: SenHub disconnect )
#define REPLY_RESULT_501		"Not Implement"    // 501 ( Note: This version wsn driver can't support this command )

#define MAX_SET_TIMEOUT         90000   // 90 sec ( 90000 ms) => WISE Cloud UI Timeout => 120 sec
#define SET_SHORT_WAIT_TIME 200        //  200 ms
//static const char g_

typedef enum
{
	STA_BUSY            =-3,   // System is busying. This system can't config
	STA_SETTING       = -2,
	STA_LOCKED       = -1,  // In setting
	STA_INIT               = 0,   // Init
	STA_READ            = 1,   //  Read to use
}RS_STATUS; // Resource Status


SN_CODE ProcUpdateSNDataCbf( const int cmdId, const void *pInData, const int InDatalen, void *pUserData, void *pOutParam, void *pRev1, void *pRev2 );

#ifdef __cplusplus 
} 
#endif 



#endif // __SensorNetwork_Manager_H__
																				


