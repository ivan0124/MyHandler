/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/08 by Eric Liang															     */
/* Modified Date: 2015/03/08 by Eric Liang															 */
/* Abstract       :  Adv JSON Format Package / Parse   								             */
/* Reference    : None																									 */
/****************************************************************************/

#include <stdlib.h>
#include "platform.h"

#include "AdvJSONx.h"

// AdvLog
#include "AdvLog.h"

char   g_szAdvJSONIniPath[256]={0};


int PackageInDataFormat( const InDataClass *pInDataClass, char *buffer, const int buffersize)
{
	int len = 0;
	int i = 0;

	char sBaseClassFormat[MAX_FUNSET_DATA_SIZE] = {0};
	InBaseData *pInBaseData = NULL;
	char funtype_format[MAX_FUNSET_DATA_SIZE]={0};

	if( pInDataClass == NULL  || buffer == NULL ) return len;
	// BaseClass.Format
	GetPrivateProfileString(BASE_APPNAME,BASE_DATACLASS_KEY,"",sBaseClassFormat,sizeof(sBaseClassFormat), g_szAdvJSONIniPath );


	pInBaseData = pInDataClass->pInBaseDataArray;

	for( i=0; i< pInDataClass->iTypeCount; i++ ) {
		    if( pInBaseData == NULL ) break;
			snprintf(funtype_format,sizeof(funtype_format), sBaseClassFormat, pInBaseData->psType,  pInBaseData->psData, pInBaseData->psType );
			strcat(buffer,funtype_format);
			memset(funtype_format,0,sizeof(funtype_format));
			pInBaseData++;
	}

	return strlen(buffer);
}

int PackageOutDataFormat( const OutDataClass *pOutDataClass, char *buffer, const int buffersize)
{
	int len = 0;
	int i = 0;
	
	char sBaseClassFormat[MAX_FUNSET_DATA_SIZE] = {0};
	OutBaseData *pOutBaseData = NULL;
	char funtype_format[MAX_FUNSET_DATA_SIZE]={0};

	if( pOutDataClass == NULL || buffer == NULL ) return len;
	// BaseClass.Format
	GetPrivateProfileString(BASE_APPNAME,BASE_DATACLASS_KEY,"",sBaseClassFormat,sizeof(sBaseClassFormat), g_szAdvJSONIniPath );

	pOutBaseData = pOutDataClass->pOutBaseDataArray;

	for( i=0; i< pOutDataClass->iTypeCount; i++ ) {
		    if( pOutBaseData == NULL ) break;
			snprintf(funtype_format,sizeof(funtype_format), sBaseClassFormat, pOutBaseData->psType,  pOutBaseData->psData, pOutBaseData->psType );
			strcat(buffer,funtype_format);
			memset(funtype_format,0,sizeof(funtype_format));
			pOutBaseData++;
	}
	return strlen(buffer);
}

/*
 *    "WSN":{
*           "WSN0":{
*                      "Info":{"e":[{"n":"SenHubList","sv":,"","asm","r"}],"bn":"Info","ver":1},
*            "bn":0000852CF4B7B0E8","ver":1},
*           "WSN1":{
*                      "Info":{"e":[{"n":"SenHubList","sv":,"","asm","r"}],"bn":"Info","ver":1},
*            "bn":0000852CF4B7B0E9","ver":1},
*		}
 *
 *
*/
int PackageSNGWCapability ( char *buf,          /* buffer point : Put IoTGW's Capability*/
													  int buflen,       /*  buffer length */ 
													  void *pInputData /* SNMultiInfInfo */)
{
	SNMultiInfInfoSpec *pSNMultiInfInfoSpec = (SNMultiInfInfoSpec*)pInputData;

	int i = 0;
	int j = 0;
	char tmp_data[MAX_DATA_SIZE]={0};
	char tmp_data2[MAX_DATA_SIZE]={0};
	char tmp_data3[MAX_DATA_SIZE]={0};
	char funtype_format[MAX_FUNSET_DATA_SIZE]={0};

	char sIoTGWFormat[MAX_FUNSET_DATA_SIZE] = {0};
	char sInfFormat[MAX_FUNSET_DATA_SIZE] = {0};
	char sNetTypeFormat[MAX_FUNSET_DATA_SIZE] = {0};


	// Interface.Format "%s":{%s"bn":"%s","ver":1},
	GetPrivateProfileString(INF_APPNAME,INF_INFFORMAT_KEY,"",sInfFormat,sizeof(sInfFormat), g_szAdvJSONIniPath );

	// NetType.Format
	GetPrivateProfileString("Inf.Format.V1","Interface.Format","",sNetTypeFormat,sizeof(sNetTypeFormat), g_szAdvJSONIniPath );

	//PRINTF("IoTGWCapability %d       \n", pSNMultiInfInfoSpec->iNum );

	for ( i=0; i< pSNMultiInfInfoSpec->iNum ; i++ ) {
		PackageInDataFormat( &pSNMultiInfInfoSpec->aSNInfSpec[i].inDataClass, tmp_data, sizeof(tmp_data) );	

		//                                                                                           WSN0                                              "Info":{"bn":"Info"}          "bn":"UID"
		snprintf( tmp_data2, sizeof( tmp_data2 ), sInfFormat, pSNMultiInfInfoSpec->aSNInfSpec[i].sInfName, tmp_data, pSNMultiInfInfoSpec->aSNInfSpec[i].sInfID );
		memset( tmp_data, 0, sizeof( tmp_data ) );
		strcat( tmp_data3, tmp_data2 );
		memset( tmp_data2, 0, sizeof( tmp_data2 ) );
	}

	snprintf(tmp_data2, sizeof(tmp_data2), sNetTypeFormat, pSNMultiInfInfoSpec->sComType, tmp_data3, pSNMultiInfInfoSpec->sComType );

	strcat( buf, tmp_data2 );


	ADV_TRACE("%s InfoSpec:    %s\n",pSNMultiInfInfoSpec->sComType, buf);
	
	return strlen(buf);
}


int PackageGWCapability(  char *buf,				/* buffer point : Buffer of result */
												int     buflen,				/*  buffer length */
												char *inData  )
{
	char sIoTGWFormat[64]={0};

	//{"IoTGW":{%s "ver":1}}
	GetPrivateProfileString(INF_APPNAME,INF_GW_FORMAT,"",sIoTGWFormat,sizeof(sIoTGWFormat), g_szAdvJSONIniPath );

	snprintf(buf, buflen, sIoTGWFormat, inData );
	ADV_TRACE("Capability = %s\n", buf);
	return strlen(buf);

}


// For Update Interface Data
int PackageSNInterfaceData ( char *buf,							    /* buffer point : Put IoTGW's Capability*/
													 const int     buflen,				/*  buffer length */
													 const char *Name,              /* Interface Name WSN0 */
													 void *pInputData					/* SNInterfaceData */)
{
        printf("[ivan][%s][%s] +++++++++++++++++++++++++++++++++++++++++++++++++++>>>>>\n",__FILE__, __func__);
	SNInterfaceData *pSNInterfaceData = (SNInterfaceData*)pInputData;
	InDataClass *pInDataClass = NULL;

	char IoTGWDataFormat[MAX_FUNSET_DATA_SIZE] = {0};
	char tmp_data[MAX_DATA_SIZE]={0};

	if(pSNInterfaceData == NULL ) return 0;


	pInDataClass = &pSNInterfaceData->inDataClass;

	//IoTGWDataFormat: {"IoTGW": {"%s": {"%s": {%s"bn": "%s","ver": 1},"bn": "%s"},"ver": 1}}
	GetPrivateProfileString(INF_APPNAME,INF_DATAFORMAT_KEY,"",IoTGWDataFormat,sizeof(IoTGWDataFormat), g_szAdvJSONIniPath );

	 if( PackageInDataFormat( pInDataClass, tmp_data, sizeof(tmp_data) ) ) {
		 snprintf( buf, buflen,  IoTGWDataFormat, pSNInterfaceData->sComType, 
																				  Name,
																				  tmp_data, 
																				  pSNInterfaceData->sInfID,
																				  pSNInterfaceData->sComType );
	 }

	printf("%s Interface Data:    %s\n",pSNInterfaceData->sComType, buf);
        printf("[ivan][%s][%s] <<<<<+++++++++++++++++++++++++++++++++++++++++++++++++++\n",__FILE__, __func__);
	return strlen(buf);
}


// For package SenHub Data & InfoSpec
int PackageSenHubData
(
   char *buf,				/* buffer point : Buffer of result */
   int buflen,				/*  buffer length */
   void *pInputData
 )
{
	InSenData *pInSenData = (InSenData*) pInputData;
	InDataClass *pInDataClass = NULL;

	char sSenHubInfoFormat[MAX_FUNSET_DATA_SIZE] = {0};
	char tmp_data[MAX_DATA_SIZE]={0};

	int i =0;

	if(pInSenData == NULL ) return 0;
	

	// {"SenHub":{%s"ver":1}}
	GetPrivateProfileString(SENHUB_APPNAME,SENHUB_DATAFORMAT_KEY,"",sSenHubInfoFormat,sizeof(sSenHubInfoFormat), g_szAdvJSONIniPath );

	pInDataClass = &pInSenData->inDataClass;


	 if( PackageInDataFormat( pInDataClass, tmp_data, sizeof(tmp_data) ) ) {
		 snprintf( buf, buflen,  sSenHubInfoFormat, tmp_data );
	 }

	ADV_TRACE( "MAC=%s SenHub InfoSpec:    %s\n", pInSenData->sUID, buf );
	return strlen(buf);
}


