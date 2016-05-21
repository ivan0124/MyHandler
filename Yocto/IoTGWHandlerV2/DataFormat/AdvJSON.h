/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/08 by Eric Liang															     */
/* Modified Date: 2015/03/08 by Eric Liang															 */
/* Abstract       :  Adv JSON Format Package / Parse   								             */
/* Reference    : None																									 */
/****************************************************************************/
#ifndef  __ADV_JSON_PARSER_PACKAGE_H__
#define __ADV_JSON_PARSER_PACKAGE_H__

#include "SensorNetwork_API.h"

#define ADV_JSON_INI  "AdvJSON.ini"


#define MAX_DATA_SIZE  4096
#define MAX_FUNSET_DATA_SIZE 2048

#define BASE_APPNAME "Base.Format.V1"
#define BASE_DATACLASS_KEY "DataClass.Format"

#define INF_APPNAME "Inf.Format.V1"
#define INF_GW_FORMAT "IoTGW.Format"
#define INF_DATAFORMAT_KEY "IoTGW.DataFormat"
#define INF_INFFORMAT_KEY "Interface.Format"


#define SENHUB_APPNAME "SenHub.Format.V1"
#define SENHUB_DATAFORMAT_KEY "Data.Format"

#define IOTGW_NAME    "IoTGW"
#define SENHUB_NAME  "SenHub"
#define SENHUB_DATA   "SenData"
#define SENHUB_INFO    "Info"


int PackageInDataFormat
( 
	const InDataClass *pInDataClass, 
	char *buffer, 
	const int buffersize
);

int PackageOutDataFormat
( 
	const OutDataClass *pOutDataClass, 
	char *buffer, 
	const int buffersize
);

int PackageSNGWCapability
(
   char *buf,				/* buffer point : Buffer of result */
   int buflen,				/*  buffer length */
   void *pInputData
 );

int PackageGWCapability
(
   char *buf,				/* buffer point : Buffer of result */
   int buflen,				/*  buffer length */
   char *inData
 );

int PackageSNInterfaceData
(
   char *buf,				 /* buffer point : Buffer of result */
   const int buflen,	 /*  buffer length */
   const char *name,  /* Interface Name WSN0 */
   void *pInputData  /* SNInterfaceData */
 );

int PackageSenHubData
(
   char *buf,				/* buffer point : Buffer of result */
   int buflen,				/*  buffer length */
   void *pInputData
 );



extern char   g_szAdvJSONIniPath[256];

#endif // __ADV_JSON_PARSER_PACKAGE_H__


