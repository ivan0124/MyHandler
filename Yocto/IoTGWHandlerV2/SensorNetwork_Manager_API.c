/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/07 by Eric Liang															     */
/* Modified Date: 2015/07/07 by Eric Liang															 */
/* Abstract : Sensor Network Manager API ( Core )         										*/
/* Reference    : None																									 */
/****************************************************************************/
//#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <dlfcn.h>
// Common Sensor Netowrk Header Files
#include "SensorNetwork_Manager_API.h"
#include "inc/SensorNetwork_API.h"

#include "SensorNetwork_Manager.h"

// Advantech JSON Sensor Format
#include "AdvJSONx.h"
#include "SnMJSONParser.h"
#include "LoadSNAPI.h"
// include lib Header Files
#include "BasicFun_Tool.h"
#include "HashTable.h"
#include "BaseLib.h"
#include "cJSON.h"

// AdvLog
#include "AdvLog.h"

//-----------------------------------------------------------------------------
// Types and defines:
//-----------------------------------------------------------------------------
#define MAX_URI_PATH = 128
#define MAX_DATA_SIZE 8192


//-----------------------------------------------------------------------------
// Internal Prototypes:
//-----------------------------------------------------------------------------

typedef struct _SN_NET_DRIVER
{
	int									bReady;
	int									nIndex;
	SNMultiInfInfoSpec	  *pMultiInfoSpec;
}SN_NET_DRIVER, *pSN_NET_DRIVER;

typedef struct SNM_SYSTEM 
{
	int								  binit;							// Is Sensor Network Manager API Library Init ?	
	SNAPI_Interface		*pSnApiInterface;    // Sensor Netowrk API Librarys interface ( Dynamic allocate with the number of librarys )
	SN_NET_DRIVER       *pSNNetDrivers;
	int                                 nSnModules;
	HashTable                 *pSenHubHashTable;
	HashTable                 *pDataHashTable;
	cJSON							*pCapabilityJsonRoot;
	CHAR			                *pszCapabilityBuf;
	CHAR			                *pDataBuffer;
	MUTEX						 mutexCbfMutex;   
}SNM_SYSTEM,*pSNM_SYSTEM;




typedef struct _SetParam {
	SNAPI_Interface                    *pSnInter; // Driver pFn
	char                   szUID[MAX_UID_SIZE];
	char               szType[MAX_TYPE_SIZE];
    char     szSetBuf[MAX_SET_BUF_SIZE];
	SN_CTL_ID                                 setcmd;
	RS_STATUS                                  status;
	TaskHandle_t                      set_thread; 
	void                                     *pUserData;
}SetParam;


typedef struct CacheBuffer {
	cJSON				  *pJsonItem;
	JSON_Type              JsType;
	SetParam				 SetInfo;
} CacheBuffer;


// SenHubID/SenHub
typedef struct SenHubCache {
	void								      *pUserData;
	SN_NET_DRIVER       *pSNNetDriver;
	cJSON							      *pJsonRoot;
	RS_STATUS		  		                status;
	HashTable          *pCacheHashTable;
} SenHubCache;




//-----------------------------------------------------------------------------
// Global Variables
//-----------------------------------------------------------------------------
static SNM_SYSTEM		  g_snmSystem;


static ReportSNManagerDataCbf  g_ReportSNManagerCbf = NULL;			// SN Manager callback (JSON format) to Application	
//static char											g_JSonBuffer[MAX_DATA_SIZE];
//static char                                         *g_pszCapabilityBuf = NULL;
//MUTEX												g_ReportCbfMutex;

/****************************************************************************/
/* Below Functions are source code                   			                 */
/****************************************************************************/
static void FreeSenHubResource(SenHubCache *pSenHubCache);
static void UpdateJSONValue(SenML_V_Type VType, cJSON *pInstance, cJSON *pNewValue );

static void FreeSNMultiInfInfoSpecResource( SNMultiInfInfoSpec *pSNMultiInfInfoSpec );

static CacheBuffer *GetCacheBuffer( const HashTable *pHashTable, const char *HashKey);

static int SaveSenHubInfoSpecInCache( const char *Id, const void *pInputData, SenHubCache *pSenHubCache, const SNAPI_Interface *pSNInterface, const char *pDataBuffer, const int len );
static int SaveSenHubDataInCache( const char *Id,  const void *pInputData, SenHubCache *pSenHubCache, const char *pDataBuffer, const int len );

static int SaveInterfaceInfoSpecInNetDriver( SN_NET_DRIVER *pSN_NET_DRIVER, const void *pInData );

static int SaveCapabilitySpecInCache( const char *pDataBuffer, const int len, const void *pInData, HashTable *pCacheHashTable,  const int num,const SNAPI_Interface *pSNInterface, cJSON *pJsonRoot );
static int SaveInfCapabilityDataInCache( const void *pInputData, HashTable *pCacheHashTable, const char *Name, const char *pDataBuffer, const int len );

// RESTful
static void ProcGetSenHubRESTfulAPI( const char *uri, const ACTION_MODE mode, char *result, int size ); // Uri -> Id/SenHub/SenData/door name
static void ProcGetIoTGWRESTfulFAPI( const char *uri, const ACTION_MODE mode, char *result, int size ); // Uri -> Id/SenHub/SenData/door name


static void ProcSetSenHubRESTfulAPI( const char *uri /* Id/SenHub/SenData/dout */, 
																	const char *pszInValue, /*"bv": 1*/ const void *pUserData,
																	char *pOutBuf, int nOutSize );


static void ProcSetIoTGWRESTfulFAPI( const char *uri /* Id/SenHub/SenData/dout */, 
																	const char *pszInValue, /*"bv": 1*/ const void *pUserData,
																	char *pOutBuf, int nOutSize );


static void PRINTF_DataClass( const void *pDataClass, const int size );

// Set Thread
void set_sn_data(void *arg);
void get_sn_data(void *arg);
//void set_gw_data(void *arg);

int LoadcSNAPILib( const char *path, SNAPI_Interface *pAPI_Loader )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
    pAPI_Loader->Handler    = OpenLib( path );
 
    if( pAPI_Loader->Handler ) {
		pAPI_Loader->SN_Initialize = ( SN_Initialize_API ) dlsym( pAPI_Loader->Handler, "SN_Initialize");
		pAPI_Loader->SN_Uninitialize = ( SN_Uninitialize_API ) dlsym ( pAPI_Loader->Handler, "SN_Uninitialize");
		pAPI_Loader->SN_GetVersion = ( SN_GetVersion_API ) dlsym ( pAPI_Loader->Handler, "SN_GetVersion");
		pAPI_Loader->SN_SetUpdateDataCbf = (SN_SetUpdateDataCbf_API  )dlsym (pAPI_Loader->Handler , "SN_SetUpdateDataCbf" );
		pAPI_Loader->SN_GetCapability         =  ( SN_GetCapability_API ) dlsym( pAPI_Loader->Handler, "SN_GetCapability");
		pAPI_Loader->SN_GetData         =  ( SN_GetData_API ) dlsym( pAPI_Loader->Handler, "SN_GetData");
		pAPI_Loader->SN_SetData         =  ( SN_SetData_API ) dlsym( pAPI_Loader->Handler, "SN_SetData");	
	} else {
		ADV_ERROR("Failed to Open Lib=%s\n", path );
	}
	if( pAPI_Loader->Handler != NULL && pAPI_Loader->SN_Initialize != NULL ) {
		pAPI_Loader->Workable = 1;
		return 1;
	} else {
		ADV_ERROR("Failed to Open Lib=%s\n", path );
	}

	return 0;
}

SN_CODE ProcUpdateSNDataCbf( const int cmdId, const void *pInData, const int InDatalen, void *pUserData, void *pOutParam, void *pRev1, void *pRev2 )
{
	printf("[%s][%s] ****************************************>>>> \n", __FILE__, __func__);
#if 1
	int rc = SN_ER_FAILED;	
	int len = 0;
	MUTEX_LOCK( &g_snmSystem.mutexCbfMutex );
	memset( g_snmSystem.pDataBuffer, 0, MAX_DATA_SIZE );

	switch( cmdId)
	{
	case SN_Inf_SendInfoSpec:
		{
			ADV_TRACE("Cmd = SN_Inf_SendInfoSpec\n");
			SN_NET_DRIVER* pSN_NET_DRIVER = (SN_NET_DRIVER*) pUserData;
			SaveInterfaceInfoSpecInNetDriver( pSN_NET_DRIVER, pInData );
			rc = SN_OK;
		}
		break;
	case SN_Inf_UpdateInterface_Data: // Ok
		{			
			ADV_TRACE("Cmd  UpdateInterface_Data \n");

			SNInterfaceData *pSNInterfaceData = (SNInterfaceData*)pInData;

		   char HashKey[MAX_PATH]={0};
			snprintf(HashKey,sizeof(HashKey),"%s/%s/%s",IOTGW_NAME, pSNInterfaceData->sComType, pSNInterfaceData->sInfID); // IoTGW/WSN/bn(WSN0's MAC)
			CacheBuffer *pCacheBuf = GetCacheBuffer( g_snmSystem.pDataHashTable, HashKey );

			if( pCacheBuf == NULL ) { 
				MUTEX_UNLOCK( &g_snmSystem.mutexCbfMutex );
				return (SN_CODE)rc;
			}

			if( ( len = PackageSNInterfaceData( g_snmSystem.pDataBuffer, MAX_DATA_SIZE , pCacheBuf->SetInfo.szSetBuf /*Name (WSN0)*/, (void *)pInData )) > 0 ) {

				SaveInfCapabilityDataInCache( pInData, g_snmSystem.pDataHashTable, pCacheBuf->SetInfo.szSetBuf  /*Name (WSN0)*/, g_snmSystem.pDataBuffer,len+1);

				if( g_ReportSNManagerCbf != NULL ) {
					g_ReportSNManagerCbf(cmdId, g_snmSystem.pDataBuffer, len, (void **)NULL, (void *)pInData, NULL );
					rc = SN_OK;
				}
			}
		}
		break;
	case SN_SenHub_Register:  // Ok
		{
			void *pOutAddres = NULL;
			SenHubCache *pSenHubCache = NULL;
			static int SenHubCacheSize = sizeof( SenHubCache );
		    SenHubInfo *pSenHubInfo = (SenHubInfo*)pInData;

			if( pSenHubInfo != NULL ) {
				ADV_TRACE("Cmd = SenHub_Registe  MAC=%s HostName=%s SN=%s Product=%s\n",		pSenHubInfo->sUID,
																																													pSenHubInfo->sHostName,
																																													pSenHubInfo->sSN,
																																													pSenHubInfo->sProduct);
				// To Prevent Register twice
				pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable,pSenHubInfo->sUID );

				if( pSenHubCache == NULL && g_ReportSNManagerCbf ) {
					rc = g_ReportSNManagerCbf(cmdId, NULL, 0, &pOutAddres, (void *)pInData, NULL );
				}


				if( pSenHubCache == NULL ) {
					pSenHubCache = (SenHubCache *)AllocateMemory( SenHubCacheSize );
					if( pSenHubCache ) {
						memset( pSenHubCache, 0, SenHubCacheSize );
						pSenHubCache->pCacheHashTable = HashTableNew( ONE_SENHUYB_HASH_TABLE_SZIE );
						pSenHubCache->status = STA_INIT;

						pSenHubCache->pUserData = pOutAddres;
						pSenHubCache->pSNNetDriver = (SN_NET_DRIVER*) pUserData;
						HashTablePut( g_snmSystem.pSenHubHashTable, pSenHubInfo->sUID, pSenHubCache  );
						rc = SN_OK;
					} // End of pSenHubCache
				} else {
					ADV_TRACE("Re register =%s\n",pSenHubInfo->sUID);
				}
				
			} // End of  If ( pSenHubInfo != NULL )
		}
		break;
	case SN_SenHub_SendInfoSpec:  // Ok
		{
			InSenData *pSenInfoSpec = (InSenData*) pInData;
			SenHubCache *pSenHubCache = NULL;
			SN_NET_DRIVER *pSNDriverInfo = (SN_NET_DRIVER*) pUserData;
			int  index = pSNDriverInfo->nIndex;
			SNAPI_Interface *pSNAPI_Interface = g_snmSystem.pSnApiInterface;

			pSNAPI_Interface += index;

			if( pSenInfoSpec != NULL ) {
				pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable,pSenInfoSpec->sUID );
				
				ADV_TRACE("Cmd = SenHub_SendInfoSpec  UUID=%s\n",pSenInfoSpec->sUID );
				PRINTF_DataClass( (void*) &pSenInfoSpec->inDataClass,  sizeof(pSenInfoSpec->inDataClass) );

				if( pSenHubCache && ( len = PackageSenHubData( g_snmSystem.pDataBuffer, MAX_DATA_SIZE, (void *)pInData )) > 0 ) {
					
					// Id/SenHub, Id/SenHub/SenData/n,  Id/SenHub/Info/Health,Name		
					if( SaveSenHubInfoSpecInCache( pSenInfoSpec->sUID, pInData, pSenHubCache, pSNAPI_Interface, g_snmSystem.pDataBuffer, len+1 ) == 0 ) {
						if( g_ReportSNManagerCbf != NULL )
							rc = g_ReportSNManagerCbf(cmdId, g_snmSystem.pDataBuffer, len, &pSenHubCache->pUserData, (void *)pInData, NULL );
					}

					if(pSenHubCache == NULL) {
							ADV_ERROR("SenHub is NULL=%s\n",pSenInfoSpec->sUID);

					}else {
					pSenHubCache->status = STA_READ;
					rc = SN_OK;
					}
				} // End of PackageSenHubInfoSpec
			} // End of pSenInfoSpec != NULL
		}
		break;
	case SN_SenHub_AutoReportData: // Ok
		{
			InSenData *pSenData = (InSenData*)pInData;
			SenHubCache *pSenHubCache = NULL;

			if( pSenData != NULL ) {
				pSenHubCache = (SenHubCache *)HashTableGet( g_snmSystem.pSenHubHashTable,pSenData->sUID );

				ADV_TRACE("Cmd = SenHub_AutoReportData  UUID=%s\n",pSenData->sUID );
				PRINTF_DataClass( (void*) &pSenData->inDataClass,  sizeof(pSenData->inDataClass) );

				if( pSenHubCache && ( len = PackageSenHubData( g_snmSystem.pDataBuffer, MAX_DATA_SIZE , (void *)pInData )) > 0 ) {

					SaveSenHubDataInCache( pSenData->sUID, pSenData, pSenHubCache, g_snmSystem.pDataBuffer, len+1 );

					if( g_ReportSNManagerCbf != NULL )
						rc = g_ReportSNManagerCbf(cmdId, g_snmSystem.pDataBuffer, len, &pSenHubCache->pUserData, (void *)pInData, NULL );					

					rc = SN_OK;
				} // End of  PackageSenHubData
			} // End of pSenData != NULL
		}
		break;
	case SN_SenHub_Disconnect:  // Ok
		{
			InSenData *pInSenData = (InSenData*)pInData;
			SenHubCache *pSenHubCache = NULL;

			if( pInSenData != NULL ) {
				pSenHubCache = (SenHubCache *)HashTableGet( g_snmSystem.pSenHubHashTable, pInSenData->sUID );

				ADV_TRACE("Cmd = SenHub_Disconnect  MAC=%s\n",pInSenData->sUID);

				if( g_ReportSNManagerCbf && pSenHubCache ) {
					rc = g_ReportSNManagerCbf(cmdId, NULL, 0, &pSenHubCache->pUserData, (void *)pInData, NULL );					 
					FreeSenHubResource( pSenHubCache );
					HashTablePut( g_snmSystem.pSenHubHashTable, pInSenData->sUID, NULL  );
					rc = SN_OK;
				}
			}
		}
		break;
	default:
		ADV_WARN("ReportSNManagerDtatCbf Cmd Not Support = %d\n", cmdId);
	}
	MUTEX_UNLOCK( &g_snmSystem.mutexCbfMutex );
	return (SN_CODE)rc;
#endif
    return 0;
}


static CacheBuffer *GetCacheBuffer( const HashTable *pHashTable, const char *HashKey)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	CacheBuffer *pCacheBuffer = NULL;

	pCacheBuffer = (CacheBuffer*)HashTableGet( (HashTable *)pHashTable, (char *)HashKey );

	if( pCacheBuffer == NULL ) {
		pCacheBuffer = (CacheBuffer *)AllocateMemory( sizeof(CacheBuffer) );
		memset( pCacheBuffer, 0, sizeof(CacheBuffer) );
	}		
	return pCacheBuffer;
}

static int SaveDataInCacheBuffer( HashTable *pHashTable, const char *pHashkey, CacheBuffer *pCacheBuf )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	int rc = -1;

	if( pHashTable && pCacheBuf ) {  
			ADV_DEBUG("Key=%s \nValue=%s\n",pHashkey, cJSON_Print(pCacheBuf->pJsonItem) );
			HashTablePut( pHashTable, (char *)pHashkey, (void*) pCacheBuf  );
			rc = 0;
	}
	return rc;
}

static int SaveSenHubInfoSpecInCache( const char *Id, const void *pInputData, SenHubCache *pSenHubCache, const SNAPI_Interface *pSNInterface, const char *pDataBuffer, const int len )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if( pInputData==NULL || pSenHubCache == NULL || pSenHubCache->pCacheHashTable == NULL ) return -1;

	InSenData *pInSenData = (InSenData*) pInputData;
	InDataClass *pInDataClass = NULL;
	InBaseData  *pInBaseData = NULL;

	pInDataClass = &pInSenData->inDataClass;
	pInBaseData = pInDataClass->pInBaseDataArray;
	
	char HashKey[MAX_PATH]={0};
	cJSON *pNextItem = NULL;
	CacheBuffer *pCacheBuf = NULL;
	cJSON *pElement = NULL;
	cJSON *pSubItem = NULL;
	int nCount = 0;
	int i = 0;
	int j =0;

	// 1. Save SenHub's InfoSpec in Cache
	pSenHubCache->pJsonRoot = cJSON_Parse((char *)pDataBuffer);

	if( pSenHubCache->pJsonRoot == NULL ) {
		ADV_ERROR("Error SaveSenHubInfoSpecInCache Parse DataBuffer  = %s\n", pDataBuffer );
		 return -1;
	}

	// SENHUB_NAME : "SenHub"
	memset(HashKey, 0, sizeof(HashKey) );
	snprintf(HashKey,sizeof(HashKey),"%s/%s", Id, SENHUB_NAME );  // MAC/SenHub
	pCacheBuf = GetCacheBuffer( pSenHubCache->pCacheHashTable, HashKey );
	pCacheBuf->pJsonItem = cJSON_GetObjectItem( pSenHubCache->pJsonRoot, SENHUB_NAME );
	pCacheBuf->SetInfo.pSnInter = (SNAPI_Interface *)pSNInterface;

	if( pCacheBuf->pJsonItem == NULL ) {
		ADV_ERROR("Error SaveSenHubInfoSpecInCache Parse SenHub   = %s\n", pDataBuffer );
		 return -1;
	}

	SaveDataInCacheBuffer( pSenHubCache->pCacheHashTable, HashKey, pCacheBuf );
	pNextItem = pCacheBuf->pJsonItem;
	pCacheBuf->JsType = JSON_ROOT;
	pCacheBuf->SetInfo.status = STA_READ;
	pCacheBuf->SetInfo.pSnInter = (SNAPI_Interface *)pSNInterface;


	// Parse Class
	for( i = 0; i< pInDataClass->iTypeCount ; i++ ) {
		if( pInBaseData == NULL ) break;
		memset(HashKey, 0, sizeof(HashKey) );
		snprintf(HashKey,sizeof(HashKey),"%s/%s/%s", Id, SENHUB_NAME, pInBaseData->psType );
		pCacheBuf = GetCacheBuffer( pSenHubCache->pCacheHashTable, HashKey );
		pCacheBuf->pJsonItem = cJSON_GetObjectItem( pNextItem, pInBaseData->psType ); // SenData, Info, Net ...
		pCacheBuf->JsType = JSON_CHLID;
		pCacheBuf->SetInfo.status = STA_READ;
		pCacheBuf->SetInfo.pSnInter = (SNAPI_Interface *)pSNInterface;
		SaveDataInCacheBuffer( pSenHubCache->pCacheHashTable, HashKey, pCacheBuf );
		pCacheBuf->JsType = JSON_CHLID;
		pCacheBuf->SetInfo.status = STA_READ;
		pCacheBuf->SetInfo.pSnInter = (SNAPI_Interface *)pSNInterface;
		pElement = cJSON_GetObjectItem( pCacheBuf->pJsonItem, SENML_E_FLAG ); // "e"

		nCount = cJSON_GetArraySize(pElement);
		
		for( j = 0; j<nCount ; j++ ) {
			pSubItem = cJSON_GetArrayItem(pElement, j);
			cJSON* jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
			memset(HashKey, 0, sizeof(HashKey) );
			snprintf(HashKey,sizeof(HashKey),"%s/%s/%s/%s", Id, SENHUB_NAME, pInBaseData->psType, jsName->valuestring );
			pCacheBuf = GetCacheBuffer( pSenHubCache->pCacheHashTable, HashKey );
			pCacheBuf->pJsonItem = pSubItem;
			pCacheBuf->JsType = RESOURCE;
			pCacheBuf->SetInfo.status = STA_READ;
			pCacheBuf->SetInfo.pSnInter = (SNAPI_Interface *)pSNInterface;
			ADV_TRACE("SenHub Info Spec=%s\n",HashKey);
			SaveDataInCacheBuffer( pSenHubCache->pCacheHashTable, HashKey, pCacheBuf );
		}
		pInBaseData++;
	}
	
	return 0;

}

static int SaveSenHubDataInCache( const char *Id,  const void *pInputData, SenHubCache *pSenHubCache, const char *pDataBuffer, const int len )
{
    printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if(  pInputData==NULL || pSenHubCache == NULL || pSenHubCache->pCacheHashTable == NULL ) return -1;

	InSenData *pInSenData = (InSenData*) pInputData;
	InDataClass *pInDataClass = &pInSenData->inDataClass;
	InBaseData  *pInBaseData = pInDataClass->pInBaseDataArray;

	char HashKey[MAX_PATH]={0};
	char *pTempBuf= NULL;
	cJSON *root = NULL;
	cJSON *pItem = NULL;
	cJSON *pItem2 = NULL;
	cJSON *pEItem = NULL;
	cJSON* jsName = NULL;
	cJSON *jsNewValue = NULL;
	cJSON *jsInstValue = NULL;
	int i = 0;
	int j=0;
	int rc = -1;

	SenML_V_Type vt = SenML_Unknow_Type;

	CacheBuffer *pCacheBuf = NULL;

	root = cJSON_Parse((char *)pDataBuffer);

	if(!root) return rc;

	pItem= cJSON_GetObjectItem( root, SENHUB_NAME );

	if(!pItem) {
		cJSON_Delete(root);
		return rc;
	}

	ADV_DEBUG("Update Count =%d SenHub Data=%s\n",pInDataClass->iTypeCount, pDataBuffer);

	for( i = 0; i < pInDataClass->iTypeCount; i++ ) {

		if( pInBaseData == NULL ) {
			ADV_ERROR("Error SaveSenHubDataInCache pInBaseData is NULL\n" );
			rc = -1;
			break;
		}
		pItem2= cJSON_GetObjectItem( pItem, pInBaseData->psType );

		if(!pItem2) {
			cJSON_Delete(root);
			continue;
		}
		pEItem = cJSON_GetObjectItem( pItem2, SENML_E_FLAG ); // "e"

		if(!pEItem) {
			cJSON_Delete(root);
			continue;
		}
		int nCount = cJSON_GetArraySize(pEItem);
		for( j = 0; j<nCount ; j ++ ) {
			pItem2 = cJSON_GetArrayItem(pEItem, j );
			if( !pItem2) continue;
			jsName = cJSON_GetObjectItem( pItem2, SENML_N_FLAG ); // "n"
			if( !jsName) continue;
			vt = GetSenMLValueType(pItem2);
			memset(HashKey, 0, sizeof(HashKey) );
			snprintf(HashKey,sizeof(HashKey),"%s/%s/%s/%s", Id, SENHUB_NAME, pInBaseData->psType, jsName->valuestring );
			pCacheBuf = (CacheBuffer*)HashTableGet( pSenHubCache->pCacheHashTable, HashKey );
			if( pCacheBuf && vt != SenML_Unknow_Type ) {
				jsNewValue = cJSON_GetObjectItem( pItem2, SenML_Types[vt] );

				if(pCacheBuf->pJsonItem)				{
					jsInstValue = cJSON_GetObjectItem( pCacheBuf->pJsonItem, SenML_Types[vt] );
				}

				if( jsNewValue && jsInstValue ) {
					//UpdateJSONValue(vt, jsInstValue, jsNewValue );
					UpdateJSONValue(vt, pCacheBuf->pJsonItem, jsNewValue );
				}
				
				
				ADV_TRACE("Update Value Key=%s\n Value=%s\n",HashKey, cJSON_Print(jsNewValue) );
			} // End of If pCacheBuf && vt != SenML_Unknow_Type
		} // End of For nCount

		pInBaseData++;
		rc = 0;
	} // End of For iTypeCount

	cJSON_Delete(root);

	return rc;
}

static int SaveInterfaceInfoSpecInNetDriver( SN_NET_DRIVER *pSN_NET_DRIVER, const void *pInData )
{
	    printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		SNMultiInfInfoSpec	  *psrcMultiInfoSpec = ( SNMultiInfInfoSpec* ) pInData;
		SNMultiInfInfoSpec	  *pdstMultiInfoSpec = NULL;
		SNInfSpec                    *psrcSNInfSpec = NULL;
		SNInfSpec                    *pdstSNInfSpec = NULL;
		InDataClass                 *psrcInDataClass = NULL;
		InDataClass                 *pdstInDataClass = NULL;
		InBaseData                  *psrcInBaseData = NULL;
		InBaseData                  *pdstInBaseData = NULL;

		int rc = SN_ER_FAILED;
		int i = 0;
		int j = 0;

		if( pSN_NET_DRIVER == NULL || pSN_NET_DRIVER->pMultiInfoSpec == NULL || psrcMultiInfoSpec == NULL ) return rc;

		if( pSN_NET_DRIVER->bReady == 1 ) return SN_INITILIZED;

		pdstMultiInfoSpec =pSN_NET_DRIVER->pMultiInfoSpec;

		// Net Type
		snprintf( pdstMultiInfoSpec->sComType, sizeof(pdstMultiInfoSpec->sComType), "%s", psrcMultiInfoSpec->sComType );

		// Num
		pdstMultiInfoSpec->iNum = psrcMultiInfoSpec->iNum;

		// InfoSpec
		psrcSNInfSpec  = psrcMultiInfoSpec->aSNInfSpec;
		pdstSNInfSpec = pdstMultiInfoSpec->aSNInfSpec;


		for( i = 0; i< psrcMultiInfoSpec->iNum; i ++ ) {
			
			if( psrcSNInfSpec == NULL || pdstSNInfSpec == NULL ) continue;			

			// Interface Name
			snprintf( pdstSNInfSpec->sInfName, sizeof( pdstSNInfSpec->sInfName ), "%s", psrcSNInfSpec->sInfName );
			// Interface UID
			snprintf( pdstSNInfSpec->sInfID, sizeof( pdstSNInfSpec->sInfID ), "%s", psrcSNInfSpec->sInfID );

			psrcInDataClass  = &psrcSNInfSpec->inDataClass;
			pdstInDataClass = &pdstSNInfSpec->inDataClass;

			psrcInBaseData = psrcInDataClass->pInBaseDataArray;

			pdstInDataClass->iTypeCount = psrcInDataClass->iTypeCount;
			pdstInDataClass->pInBaseDataArray = (InBaseData *)AllocateMemory( ( sizeof(InBaseData)*pdstInDataClass->iTypeCount) );
			pdstInBaseData = pdstInDataClass->pInBaseDataArray;

			if( psrcInBaseData == NULL || pdstInBaseData == NULL ) continue;

			for( j = 0; j < psrcInDataClass->iTypeCount; j++ ) {

				pdstInBaseData->psType = (char *)AllocateMemory( psrcInBaseData->iSizeType );
				if( pdstInBaseData->psType )
					snprintf( pdstInBaseData->psType, psrcInBaseData->iSizeType, "%s", psrcInBaseData->psType );

				pdstInBaseData->psData = (char *)AllocateMemory( psrcInBaseData->iSizeData );
				if( pdstInBaseData->psData )
					snprintf( pdstInBaseData->psData, psrcInBaseData->iSizeData, "%s", psrcInBaseData->psData );

				psrcInBaseData++;
				pdstInBaseData++;
			} // End of InDataClass

			psrcSNInfSpec++;
			pdstSNInfSpec++;

		} // End of For (  psrcMultiInfoSpec->iNum )

		pSN_NET_DRIVER->bReady = 1;
		rc = SN_OK;
		return rc;
}

static int SaveCapabilitySpecInCache( const char *pDataBuffer, const int len, const void *pInData,  HashTable *pCacheHashTable, const int num,
																   const SNAPI_Interface *pInSNInterface, cJSON *pJsonRoot )
{
	 printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	int rc = -1;
	if( pDataBuffer == NULL || pInData == NULL || pInSNInterface == NULL ) return rc;

	char HashKey[MAX_PATH]={0};
	char tmp[MAX_UID_SIZE]={0};
	char uid[MAX_UID_SIZE]={0};
	cJSON *pNextItem = NULL;
	CacheBuffer *pCacheBuf = NULL;
	cJSON *pJsonTmp = NULL;
	cJSON *pJsonbn  = NULL;
	cJSON *pElement = NULL;
	cJSON *pSubItem = NULL;
	int nCount = 0;
	int i = 0;
	int j=0;
	int k = 0;
	int l = 0;


	InBaseData		 *pInBaseData = NULL;
	SN_NET_DRIVER *pSN_NET_DRIVER = (SN_NET_DRIVER*)pInData;
	SNMultiInfInfoSpec   *pSNMultiInfInfoSpec   = NULL;
	SNAPI_Interface *pSNAPI_Interface = (SNAPI_Interface *)pInSNInterface;
	// 0. Parse Capability
	pJsonRoot = cJSON_Parse((char *)pDataBuffer);
	memset(HashKey, 0, sizeof(HashKey) );

	if( pJsonRoot == NULL ) {
		ADV_ERROR("Error SaveCapabilitySpecInCache = %s\n", pDataBuffer );
		 return rc;
	}

	// 1. IoTGW
	snprintf(HashKey,sizeof(HashKey),"%s",IOTGW_NAME );
	pCacheBuf = GetCacheBuffer( pCacheHashTable, HashKey );
	pCacheBuf->pJsonItem = cJSON_GetObjectItem( pJsonRoot, IOTGW_NAME );
	pCacheBuf->JsType = JSON_ROOT;
	pCacheBuf->SetInfo.status = STA_READ;
	pCacheBuf->SetInfo.pSnInter = NULL; // NULL Not assign 
	SaveDataInCacheBuffer( pCacheHashTable, HashKey, pCacheBuf );

	pNextItem = pCacheBuf->pJsonItem;  // "IoTGW"

	if( pNextItem == NULL )  return rc;

	for( i = 0; i < num; i ++ ) {

		if( pSNAPI_Interface ==  NULL || pSN_NET_DRIVER == NULL ) {
			rc = -1;
			break;
		}

		if( pSN_NET_DRIVER->bReady == 0 ) {
			pSNAPI_Interface++;
			pSN_NET_DRIVER++;
			continue;
		}

		pSNMultiInfInfoSpec = pSN_NET_DRIVER->pMultiInfoSpec;

		// 2. IoTGW/WSN
		memset(HashKey, 0, sizeof(HashKey) );
		snprintf(HashKey,sizeof(HashKey),"%s/%s", IOTGW_NAME, pSNMultiInfInfoSpec->sComType );     // IoTGW/WSN
		pCacheBuf = GetCacheBuffer( pCacheHashTable, HashKey );

		pJsonbn = cJSON_GetObjectItem( pNextItem, pSNMultiInfInfoSpec->sComType ); // WSN
		pCacheBuf->JsType = JSON_CHLID;
		pCacheBuf->SetInfo.status = STA_READ;
		pCacheBuf->SetInfo.pSnInter = pSNAPI_Interface;
		pCacheBuf->pJsonItem = pJsonbn;

		SaveDataInCacheBuffer( pCacheHashTable, HashKey, pCacheBuf );

		if( pCacheBuf->pJsonItem == NULL )  continue;

		for( j = 0; j < pSNMultiInfInfoSpec->iNum; j ++ ) {

			//3. IoTGW/WSN/WSN0(bn)
			memset(HashKey, 0, sizeof(HashKey) );
			memset(uid, 0, sizeof( uid ));
			snprintf(uid, sizeof(uid),"%s",pSNMultiInfInfoSpec->aSNInfSpec[j].sInfID );

			pJsonTmp = cJSON_GetObjectItem( pJsonbn, pSNMultiInfInfoSpec->aSNInfSpec[j].sInfName ); // WSN0
			if( pJsonTmp == NULL )  continue;
			snprintf(HashKey,sizeof(HashKey),"%s/%s/%s",IOTGW_NAME, pSNMultiInfInfoSpec->sComType, uid); // IoTGW/WSN/bn(WSN0's MAC)
			pCacheBuf = GetCacheBuffer( pCacheHashTable, HashKey );

			pCacheBuf->pJsonItem = pJsonTmp;
			pCacheBuf->JsType = JSON_CHLID;
			pCacheBuf->SetInfo.status = STA_READ;
			pCacheBuf->SetInfo.pSnInter = pSNAPI_Interface;
			snprintf(pCacheBuf->SetInfo.szSetBuf, sizeof(pCacheBuf->SetInfo.szSetBuf), "%s", pSNMultiInfInfoSpec->aSNInfSpec[j].sInfName ); // Name
			snprintf(pCacheBuf->SetInfo.szUID, sizeof(pCacheBuf->SetInfo.szUID), "%s", uid);

			SaveDataInCacheBuffer( pCacheHashTable, HashKey, pCacheBuf );
			if( pCacheBuf->pJsonItem == NULL )  continue;
			pInBaseData = pSNMultiInfInfoSpec->aSNInfSpec[j].inDataClass.pInBaseDataArray;

			// 4. IoTGW/WSN/WSN0(bn)/Info
			for ( k =0; k< pSNMultiInfInfoSpec->aSNInfSpec[j].inDataClass.iTypeCount ; k ++ ) {		

				if( pInBaseData == NULL ) continue;
				
				memset(HashKey, 0, sizeof(HashKey) );

				// IoTGW/WSN/bn(WSN0's MAC)/info
				snprintf(HashKey,sizeof(HashKey),"%s/%s/%s/%s", IOTGW_NAME, pSNMultiInfInfoSpec->sComType, uid, pInBaseData->psType ); 
				pCacheBuf = GetCacheBuffer( pCacheHashTable, HashKey );

				pCacheBuf->pJsonItem = cJSON_GetObjectItem( pJsonTmp, pInBaseData->psType );
				pCacheBuf->JsType = JSON_CHLID;
				pCacheBuf->SetInfo.status = STA_READ;
				pCacheBuf->SetInfo.pSnInter = pSNAPI_Interface;

				SaveDataInCacheBuffer( pCacheHashTable, HashKey, pCacheBuf );

				if( pCacheBuf->pJsonItem == NULL )  continue;

				// 5. IoTGW/WSN/WSN0(bn)/Info/e
				pElement = cJSON_GetObjectItem( pCacheBuf->pJsonItem, SENML_E_FLAG ); // "e"
				nCount = cJSON_GetArraySize(pElement);
				for( l = 0; l<nCount ; l ++ ) {
					pSubItem = cJSON_GetArrayItem( pElement, l );
					cJSON* jsName = cJSON_GetObjectItem( pSubItem, SENML_N_FLAG );
					memset(HashKey, 0, sizeof(HashKey) );
					// IoTGW/WSN/bn(WSN0's MAC)/info/SenHubList
					snprintf(HashKey,sizeof(HashKey),"%s/%s/%s/%s/%s", IOTGW_NAME, pSNMultiInfInfoSpec->sComType, uid, pInBaseData->psType, jsName->valuestring );
					pCacheBuf = GetCacheBuffer( pCacheHashTable, HashKey );
					pCacheBuf->pJsonItem = pSubItem;
					pCacheBuf->JsType = RESOURCE;
					pCacheBuf->SetInfo.status = STA_READ;
					pCacheBuf->SetInfo.pSnInter = pSNAPI_Interface;
					ADV_TRACE("Interface InfoSpec=%s\n",HashKey);
					SaveDataInCacheBuffer( pCacheHashTable, HashKey, pCacheBuf );
				}  // End of For "l"

				pInBaseData++;

			} // End of For "k"

		}  // End of For "j"

		pSNAPI_Interface++;
		pSN_NET_DRIVER++;

	} // End of For "i"
	
	return rc;
}

static void UpdateJSONStringValue(SenML_V_Type VType, cJSON *pInstance, cJSON *pNewValue, const char *key )
{
    printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
}

static int SaveInfCapabilityDataInCache( const void *pInputData, HashTable *pCacheHashTable, const char *Name, const char *pDataBuffer, const int len )
{
    printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if( pCacheHashTable == NULL || pInputData == NULL ) return -1;

	SNInterfaceData *pSNInterfaceData = (SNInterfaceData*)pInputData;
	InDataClass *pInDataClass = &pSNInterfaceData->inDataClass;
	InBaseData  *pInBaseData = pInDataClass->pInBaseDataArray;

	int i = 0;
	int rc = -1;
	char HashKey[MAX_PATH]={0};
	char *pTempBuf= NULL;
	cJSON *root = NULL;
	cJSON *pItem = NULL;
	cJSON *pItem2 = NULL;
	cJSON* jsName = NULL;
	cJSON *jsNewValue = NULL;
	cJSON *jsInstValue = NULL;


	SenML_V_Type vt = SenML_Unknow_Type;

	CacheBuffer *pCacheBuf = NULL;

	root = cJSON_Parse((char *)pDataBuffer);

	if(!root) {
		return rc;
	}

	pItem= cJSON_GetObjectItem( root, IOTGW_NAME );

	if(!pItem) {
		cJSON_Delete(root);
		return rc;
	}

	pItem2= cJSON_GetObjectItem( pItem, pSNInterfaceData->sComType ); // WSN

	if(!pItem2) {
		cJSON_Delete(root);
		return rc;
	}

	pItem= cJSON_GetObjectItem( pItem2, Name ); // WSN0

	if(!pItem) {
		cJSON_Delete(root);
		return rc;
	}

	for( i = 0; i < pInDataClass->iTypeCount; i++ ) {

		if( pInBaseData == NULL ) {
			ADV_ERROR("Error SaveSenHubDataInCache pInBaseData is NULL\n" );
			rc = -1;
			break;
		}

		pItem2= cJSON_GetObjectItem( pItem, pInBaseData->psType ); // Info

		if(!pItem2) {
			cJSON_Delete(root);
			continue;
		}

		pItem = cJSON_GetObjectItem( pItem2, SENML_E_FLAG ); // "e"

		if(!pItem) {
			cJSON_Delete(root);
			continue;
		}

		
		int nCount = cJSON_GetArraySize(pItem);
		for( i = 0; i<nCount ; i ++ ) {
			pItem2 = cJSON_GetArrayItem(pItem, i);  // new
			if( !pItem2) continue;
			jsName = cJSON_GetObjectItem( pItem2, SENML_N_FLAG );
			if( !jsName) continue;
			vt = GetSenMLValueType(pItem2);
			memset(HashKey, 0, sizeof(HashKey) );
			snprintf(HashKey,sizeof(HashKey),"%s/%s/%s/%s/%s", IOTGW_NAME, pSNInterfaceData->sComType, pSNInterfaceData->sInfID, pInBaseData->psType, jsName->valuestring );
			pCacheBuf = (CacheBuffer*)HashTableGet( pCacheHashTable, HashKey );
			if( pCacheBuf && vt != SenML_Unknow_Type ) {
				jsNewValue = cJSON_GetObjectItem( pItem2, SenML_Types[vt] );
				if(pCacheBuf->pJsonItem)				
					jsInstValue = cJSON_GetObjectItem( pCacheBuf->pJsonItem, SenML_Types[vt] ); // old

				if( jsNewValue && jsInstValue ) {
					//UpdateJSONValue(vt, jsInstValue, jsNewValue );
					UpdateJSONValue(vt, pCacheBuf->pJsonItem, jsNewValue );
				}
				ADV_TRACE("Update Value Key=%s\n Value=%s\n",HashKey, cJSON_Print(jsNewValue) );
			} // End of  IF ( pCacheBuf && vt != SenML_Unknow_Type  )
		} // End of For nCount
		pInBaseData++;
		rc = 0;
	} 

	cJSON_Delete(root);

	return rc;
}



static void UpdateJSONValue(SenML_V_Type VType, cJSON *pInstance, cJSON *pNewValue )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	switch(VType)
	{
		case SenML_Type_V:
			cJSON_ReplaceItemInObject(pInstance,"v",cJSON_CreateNumber(pNewValue->valueint));
			break;
		case SenML_Type_BV:
			cJSON_ReplaceItemInObject(pInstance,"bv",cJSON_CreateNumber(pNewValue->valueint));
			break;
		case SenML_Type_SV:
				cJSON_ReplaceItemInObject(pInstance,"sv",cJSON_CreateString(pNewValue->valuestring));
			break;
		default:
			ADV_WARN("Update JSON Value Unknow Type\n");
			
	}
}

static void FreeSNMultiInfInfoSpecResource( SNMultiInfInfoSpec *pSNMultiInfInfoSpec )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	int i = 0; 
	int j = 0;
	SNInfSpec *pSNInfSpec = pSNMultiInfInfoSpec->aSNInfSpec;
	InDataClass *pInDataClass = NULL;
	InBaseData *pInBaseData = NULL;

	for( i = 0; i< pSNMultiInfInfoSpec->iNum; i ++ ) {
		pInDataClass = &pSNInfSpec->inDataClass;
		pInBaseData  = pInDataClass->pInBaseDataArray;

		for( j =0; j < pInDataClass->iTypeCount; j++ ) {
			FreeMemory( pInBaseData->psType );
			FreeMemory( pInBaseData->psData );
			pInBaseData++;
		}

		pSNInfSpec++;
	}
}

static void FreeSenHubResource( SenHubCache *pSenHubCache )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if( pSenHubCache == NULL ) return;
	int i, j;

	HashTable *table = pSenHubCache->pCacheHashTable;
	CacheBuffer *pCacheBuffer = NULL;
	CacheBuffer *pCurBuf = NULL, *pNextBuf = NULL;



	for (i=0; i<table->count; i++) {
		Array *hits = (Array *)table->item[i];
		for (j=0; j<hits->count; j++) {
			Entry *e = (Entry *)hits->item[j];
			pCacheBuffer = (CacheBuffer*) e->data;
			if( pCacheBuffer )  {
				if( i == 0 && pCacheBuffer->pJsonItem)
					cJSON_Delete(pCacheBuffer->pJsonItem);

				FreeMemory( pCacheBuffer );
			} // End of pCacheBuffer
		} // End of j
	} // End of i

	//if( pSenHubCache->pJsonRoot ) {
	//	cJSON_Delete(pSenHubCache->pJsonRoot);
	//	pSenHubCache->pJsonRoot = NULL;
	//}

	FreeMemory( pSenHubCache ); // Free
}

void FreeDataHashTable( HashTable *pTable )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	HashTable *table = g_snmSystem.pSenHubHashTable;
	CacheBuffer *pCacheBuffer = NULL;
	int i, j;
	for (i=0; i<pTable->count; i++) {
		Array *hits = (Array *)pTable->item[i];
		for (j=0; j<hits->count; j++) {
			Entry *e = (Entry *)hits->item[j];
			pCacheBuffer = (CacheBuffer*) e->data;
			if( pCacheBuffer )  {
				if( i == 0 && pCacheBuffer->pJsonItem)
					cJSON_Delete(pCacheBuffer->pJsonItem);

				FreeMemory( pCacheBuffer );
			} // End of pCacheBuffer
		} // End of j
	} // End of i

}

void DisconnectAllSenHub( HashTable *table )
{
   printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
   SenHubCache *pSenHubCache = NULL;
  int i, j;
  for (i=0; i<table->count; i++) {
    Array *hits = (Array *)table->item[i];
    for (j=0; j<hits->count; j++) {
      Entry *e = (Entry *)hits->item[j];
	  pSenHubCache = (SenHubCache*) e->data;


	  if( g_ReportSNManagerCbf && pSenHubCache ) // Send disconnect message to Cloud
		  g_ReportSNManagerCbf( SN_SenHub_Disconnect, NULL, 0, &pSenHubCache->pUserData, NULL, NULL );     
      
	  FreeSenHubResource( pSenHubCache );
    } // End of j
  } // End of i
}






int LoadAllSnModules( SNAPI_Interface **pSnApiInterface)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	char AppNameFormat[64]={0};
	char KeyPath[64]={0};
	char szIniConfigPath[MAX_PATH]={0};
	char szApPath[MAX_PATH]={0};
	char ModulePath[128]={0};
	char FullModulePath[MAX_PATH]={0};
	SNAPI_Interface *pSnInter = NULL;


	int nNum = 0;
	int nModules = 0;
	int i = 0;


	//if( pSnApiInterface == NULL ) return 0;
	
	// Format Applicaton Name
	snprintf(AppNameFormat,sizeof(AppNameFormat), "%s", APPNAME_SN );  // [SensorNetwork]

	snprintf(szApPath, sizeof(szApPath),"%s",GetLibaryDir(SN_MANAGER_LIB_NAME));

	snprintf(szIniConfigPath, sizeof(szIniConfigPath),"%s/%s",szApPath,SN_MODULE_CONFIG);

	// Number of Sensor Network API Modules
	nNum = GetPrivateProfileInt( AppNameFormat, KEY_SN_NUM, 0, szIniConfigPath );

	if( 0 == nNum ) return nNum;

	*pSnApiInterface = (SNAPI_Interface*) malloc ( (sizeof(SNAPI_Interface)*nNum) );
	
	pSnInter = *pSnApiInterface;

	for (  i=1; i<= nNum; i ++ ) {
		snprintf(KeyPath,sizeof(KeyPath),"%s%d",KEY_SN_PATH,i);
		GetPrivateProfileString(AppNameFormat,KeyPath,"",ModulePath,sizeof(ModulePath), szIniConfigPath );

		snprintf(FullModulePath,sizeof(FullModulePath),"%s/%s",szApPath,ModulePath);
		//snprintf(FullModulePath,sizeof(FullModulePath),"%s",ModulePath);
		ADV_PRINT(0,"Module Path=%s\n",FullModulePath);
		if( LoadcSNAPILib( FullModulePath, pSnInter ) == 0 )
			ADV_PRINT(0,"Failed to Load SN Module :%s\n", FullModulePath );
		else {
			ADV_PRINT(0,"Success to Load SN Module:%s\n", FullModulePath );
			++nModules;
			++pSnInter;
		}

		memset(KeyPath,0,sizeof(KeyPath));
		memset(ModulePath,0,sizeof(ModulePath));
		memset(FullModulePath,0,sizeof(FullModulePath));
	}
	return nModules;
}



// 0: Failed
// >0 : Succes to Initial number is the initial Support Sensor Network Library
SN_CODE SNCALL SN_Manager_Initialize(void)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	SN_CODE rc = SN_ER_FAILED;
	int    i = 0;
	char libpath[MAX_PATH]={0};
	SNAPI_Interface *pSnInter = NULL;
	SN_NET_DRIVER *pSNNetDriver = NULL;

	if ( g_snmSystem.binit == 1 )	return SN_INITILIZED;
	
	// 0 Initial API related resources : Hash Table / Data Buffer / Mutex
	g_snmSystem.pSenHubHashTable = HashTableNew( MAX_SENHUB_HASH_TABLE_SIZE );
	g_snmSystem.pDataHashTable       = HashTableNew( MAX_DATA_HASH_TABLE_SIZE );

	if( g_snmSystem.pSenHubHashTable == NULL ) {
		ADV_ERROR("Failed: SN_Manager_Initialize: New HashTable Failed\r\n");
		return rc;
	}

	if( g_snmSystem.pDataHashTable == NULL ) {
		ADV_ERROR("Failed: SN_Manager_Initialize: New HashTable Failed\r\n");
		return rc;
	}

	g_snmSystem.pDataBuffer = ( CHAR *) malloc ( MAX_DATA_SIZE );

	if( g_snmSystem.pDataBuffer == NULL ) {
		SN_Manager_Uninitialize();
		ADV_ERROR("Failed: SN_Manager_Initialize: Allocated Data Buffer Failed\r\n");
		return rc;
	}


	memset( g_snmSystem.pDataBuffer, 0, MAX_DATA_SIZE);
	MUTEX_INIT( &g_snmSystem.mutexCbfMutex, NULL );

	//snprintf(g_szAdvJSONIniPath,sizeof(g_szAdvJSONIniPath),"%s/%s",GetLibaryDir(SN_MANAGER_LIB_NAME),ADV_JSON_INI);


	g_snmSystem.nSnModules = LoadAllSnModules( &g_snmSystem.pSnApiInterface );

	g_snmSystem.pSNNetDrivers = ( SN_NET_DRIVER* ) malloc ( sizeof( SN_NET_DRIVER ) * g_snmSystem.nSnModules );

	if( g_snmSystem.pSNNetDrivers == NULL ) {
		SN_Manager_Uninitialize();
		ADV_ERROR("Failed: SN_Manager_Initialize: Allocated SN NET DRIVER\r\n");
		return rc;
	}

	memset( g_snmSystem.pSNNetDrivers, 0, sizeof( SN_NET_DRIVER ) * g_snmSystem.nSnModules );
	ADV_PRINT(0, "SN_Manager_Initialize Modules=%d \r\n",g_snmSystem.nSnModules);

	pSnInter			  = g_snmSystem.pSnApiInterface;
	pSNNetDriver = g_snmSystem.pSNNetDrivers;

	for( i = 0; i< g_snmSystem.nSnModules; i++ ) {
		ADV_DEBUG("SN_Manager_Initialize index=%d\r\n", i );
		pSNNetDriver->nIndex = i;

		if( pSnInter ) {
			if( pSnInter->SN_Initialize( pSNNetDriver ) == SN_OK ) {
				ADV_DEBUG("SN_Manager_Initialize: Initial OK \r\n");
				if(pSnInter->SN_SetUpdateDataCbf  != NULL ) {
					pSNNetDriver->pMultiInfoSpec = (SNMultiInfInfoSpec *)AllocateMemory(sizeof(SNMultiInfInfoSpec));
					pSnInter->SN_SetUpdateDataCbf( &ProcUpdateSNDataCbf );							
				} else
					ADV_ERROR("Fail: SN_Manager_Initialize: Can't Find Callback Function point\r\n");
				
			}else {
				ADV_ERROR("Failed: SN_Manager_Initialize: Initial Fail \r\n");
				pSNNetDriver->bReady = 0;
			}
		} // End of If ( pSnInter )

		++pSnInter;
		++pSNNetDriver;
	}

	rc = SN_OK; 
	Sleep(2000);
	g_snmSystem.binit = 1;

	ADV_PRINT(0, "Success: SN_Manager_Initialize\r\n");
	return rc;
}

SN_CODE SNCALL SN_Manager_Uninitialize(void)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		SN_CODE rc = SN_ER_FAILED;
		SNAPI_Interface *pSnInter = NULL;
		SN_NET_DRIVER *pSNNetDriver = NULL;

		int i = 0;
		int j = 0;

		if( g_snmSystem.binit == 0 ) return rc;

		pSnInter			  = g_snmSystem.pSnApiInterface;
		pSNNetDriver = g_snmSystem.pSNNetDrivers;

		ADV_PRINT(0, "Start SN_Manager_Uninitialize \r\n");	

		for( i = 0; i< g_snmSystem.nSnModules; i++ ) {
			if( pSnInter && pSnInter->SN_Uninitialize( pSNNetDriver ) == 1 )
				ADV_PRINT(0, "Success: SN_Manager_Uninitialize: SN_Uninitialize\r\n");			
			else
				ADV_PRINT(0, "Failed: SN_Manager_Uninitialize: SN_Uninitialize\r\n");	

			// Free pMultiInfoSpec 
			if( pSNNetDriver ) {
				if( pSNNetDriver-> pMultiInfoSpec ) {
					FreeSNMultiInfInfoSpecResource( pSNNetDriver->pMultiInfoSpec );
					FreeMemory( pSNNetDriver->pMultiInfoSpec );
				}
			}

			++pSnInter;
			++pSNNetDriver;
		}


		// Send Disconnect
		DisconnectAllSenHub( g_snmSystem.pSenHubHashTable );

		// Free DataHashTable
		FreeDataHashTable( g_snmSystem.pDataHashTable );


		// Free Hash Table
		if( g_snmSystem.pSenHubHashTable != NULL )
			HashTableFree( g_snmSystem.pSenHubHashTable );

		if( g_snmSystem.pDataHashTable != NULL )
			HashTableFree( g_snmSystem.pDataHashTable );

		MUTEX_DESTROY(&g_snmSystem.mutexCbfMutex);


		FreeMemory( g_snmSystem.pszCapabilityBuf );
		FreeMemory( g_snmSystem.pDataBuffer );
		FreeMemory( g_snmSystem.pSnApiInterface );
		FreeMemory( g_snmSystem.pSNNetDrivers );

		ADV_PRINT(0, "Success to SN_Manager_Uninitialize \r\n");	
		return rc;
}

SN_CODE SNCALL SN_Manager_GetVersion(char *psVersion, int BufLen)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if( psVersion == NULL ) return SN_ER_FAILED;
	snprintf(psVersion, BufLen, "1.0.1" );
	return SN_OK;
}

SN_CODE SNCALL SN_Manager_ActionProc(const int cmdId, ReportSNManagerDataCbf pParam1, void *pParam2, void *pRev1, void *pRev2)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		SN_CODE rc = SN_ER_FAILED;

		//if( g_snmSystem.binit == 0 ) return rc;

		switch (cmdId)
		{
		case SN_Set_ReportSNManagerDataCbf:
			{
				if( pParam1 != NULL ) {
					g_ReportSNManagerCbf = pParam1;
					ADV_INFO("ProcSN_Manager_ActionProc Cmd SN_Set_ReportSNManagerDataCbf =%d\n",  g_ReportSNManagerCbf);
					rc = SN_OK;
				}
			}
			break;
		default:
			ADV_WARN("SN_Manager_ActionProc Cmd Not Support = %d\n", cmdId);
		}
		return rc;
}

void TransferSNMInfo2SNInfoSpec( const SNMultiInfInfo *psrcSNMultiInfInfo, SNMultiInfInfoSpec *pdstSNMultiInfInfoSpec )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	int i = 0;
	int j = 0;
	SNInfInfo *psrcSNInfInfo = NULL;
	SNInfSpec *pdstSNInfSpec= NULL;
	OutBaseData   *psrcOutBase = NULL;
	InBaseData       *pdstInBase = NULL;
	// Communication Type
	snprintf(pdstSNMultiInfInfoSpec->sComType, sizeof(pdstSNMultiInfInfoSpec->sComType), "%s", psrcSNMultiInfInfo->sComType );

	// iNum
	pdstSNMultiInfInfoSpec->iNum = psrcSNMultiInfInfo->iNum;

	// SNInfInfo 2 InfoSpec 
	psrcSNInfInfo   = (SNInfInfo *)psrcSNMultiInfInfo->SNInfs;
	pdstSNInfSpec = pdstSNMultiInfInfoSpec->aSNInfSpec;

	for( i = 0; i< pdstSNMultiInfInfoSpec->iNum; i++ ) {
		// Interface Name
		snprintf(pdstSNInfSpec->sInfName, sizeof(pdstSNInfSpec->sInfName),"%s",psrcSNInfInfo->sInfName);
		// UID
		snprintf(pdstSNInfSpec->sInfID, sizeof(pdstSNInfSpec->sInfID),"%s",psrcSNInfInfo->sInfID);
		// Data
		psrcOutBase = psrcSNInfInfo->outDataClass.pOutBaseDataArray;
		pdstSNInfSpec->inDataClass.pInBaseDataArray = (InBaseData *)AllocateMemory( ( sizeof(InBaseData)*psrcSNInfInfo->outDataClass.iTypeCount) );
		pdstInBase    = pdstSNInfSpec->inDataClass.pInBaseDataArray;
		pdstSNInfSpec->inDataClass.iTypeCount = psrcSNInfInfo->outDataClass.iTypeCount;

		for( j =0; j < psrcSNInfInfo->outDataClass.iTypeCount; j ++ ) {

			if( pdstInBase == NULL || psrcOutBase == NULL ) break;

			
			pdstInBase->iSizeType = psrcOutBase->iSizeType+1;
            pdstInBase->psType = (char *)AllocateMemory( pdstInBase->iSizeType );


			if( pdstInBase->psType )
			{
				snprintf(pdstInBase->psType,pdstInBase->iSizeType,"%s",psrcOutBase->psType);
				//PRINTF("Type=%s   %s\n",pdstInBase->psType, psrcOutBase->psType);
			}


			pdstInBase->psData = (char *)AllocateMemory( *psrcOutBase->iSizeData + 1 );
			pdstInBase->iSizeData = *psrcOutBase->iSizeData + 1;

			if( pdstInBase->psData ) {
				snprintf(pdstInBase->psData, pdstInBase->iSizeData,"%s",psrcOutBase->psData);
				ADV_TRACE("Data=%s\n",pdstInBase->psData);
			}

			psrcOutBase++;
			pdstInBase++;
		}

	}
}


char* SNCALL SN_Manager_GetCapability()
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	ADV_PRINT(0, "SN_Manager_GetCapability Enter\n");
	if( g_snmSystem.binit == 0 ) return NULL;

	int len = 0;
	int i = 0;
	int num = 0;

	char buffer[MAX_DATA_SIZE] = {0};
	char data[MAX_DATA_SIZE] = {0};
	
	SN_NET_DRIVER *pSN_NET_DRIVER = NULL;
	SNAPI_Interface *pSnInter = NULL;	
	SNMultiInfInfo oneSNInfInfos;
	SNMultiInfInfoSpec	oneMultiInfoSpec;

	OutBaseData outBaseData[MAX_SN_INF_NUM];
	char types[MAX_SN_INF_NUM][MAX_TYPE_SIZE];
	int nSizeData[MAX_SN_INF_NUM];

	memset(&oneSNInfInfos,0,sizeof(SNMultiInfInfo));

	for( i = 0; i < MAX_SN_INF_NUM ; i++ ) {
		oneSNInfInfos.SNInfs[i].outDataClass.iTypeCount = 1;
		outBaseData[i].psType = types[i];
		outBaseData[i].iSizeType = MAX_TYPE_SIZE;
		outBaseData[i].psData = (char *)AllocateMemory(MAX_DEF_DATA_SIZE);
		nSizeData[i]=MAX_DEF_DATA_SIZE;
		outBaseData[i].iSizeData = &nSizeData[i];
		memset(types[i], 0 , MAX_TYPE_SIZE );

		oneSNInfInfos.SNInfs[i].outDataClass.pOutBaseDataArray = &outBaseData[i];
	}


	pSnInter                 = g_snmSystem.pSnApiInterface;
	pSN_NET_DRIVER = g_snmSystem.pSNNetDrivers;

	// Get All Types of SN Capability

	if( g_snmSystem.pszCapabilityBuf && strlen(g_snmSystem.pszCapabilityBuf) > 0 ) {
			ADV_DEBUG("IoTGWCapability data is Ready\n");
	} else  {
			for( i = 0; i < g_snmSystem.nSnModules; i++ ) {
				if( pSN_NET_DRIVER && pSN_NET_DRIVER->bReady == 0 ) {
					//memset(&oneSNInfInfos,0,sizeof(SNMultiInfInfo));
					if( pSnInter && pSnInter->SN_GetCapability && pSnInter->SN_GetCapability(&oneSNInfInfos)  == SN_OK ) {							
						// Transfer to SNMultiInfInfoSpec
						TransferSNMInfo2SNInfoSpec( &oneSNInfInfos, pSN_NET_DRIVER->pMultiInfoSpec );
						pSN_NET_DRIVER->bReady = 1;
						num++;
						PackageSNGWCapability( buffer, sizeof(buffer), pSN_NET_DRIVER->pMultiInfoSpec );
					}
				} else if ( pSN_NET_DRIVER && pSN_NET_DRIVER->bReady == 1 ) {
					ADV_DEBUG("Capability data has Ready from Callback\n");
					num++;
					PackageSNGWCapability( buffer, sizeof(buffer), pSN_NET_DRIVER->pMultiInfoSpec );	
				}

				pSnInter++;
				pSN_NET_DRIVER++;
			} // End of For i < g_snmSystem.nSnModules

			// Add Handler Name { "IoTGW":{},"ver":1}
			len = PackageGWCapability( data, sizeof(data), buffer );

			// Save Capability in cache					
			SaveCapabilitySpecInCache( data, len+1, g_snmSystem.pSNNetDrivers, g_snmSystem.pDataHashTable,  g_snmSystem.nSnModules, g_snmSystem.pSnApiInterface ,g_snmSystem.pCapabilityJsonRoot);		

			g_snmSystem.pszCapabilityBuf =  (char *)malloc(len + 1);
			snprintf( g_snmSystem.pszCapabilityBuf,len+1,"%s",data );
	} // End of If ( g_snmSystem.pszCapabilityBuf &&


	// Free Resource
	for( i = 0; i< MAX_SN_INF_NUM; i ++ ) {
		FreeMemory( outBaseData[i].psData );
	}

	ADV_DEBUG("GWCapability =%s\n",g_snmSystem.pszCapabilityBuf );
	ADV_PRINT(0, "SN_Manager_GetCapability Leave\n");
	return g_snmSystem.pszCapabilityBuf;
}



/* **************************************************************************************
 *  Function Name: SN_Manager_SetData
 *  Description       : Set IoT Sensor Value by URI path
 *  Input                  : const char *pszInUriPath : The path of sensor resource
 *                                                    ex:  SenHub'ID/SenHub/SenData/dout
 *                             : const char *pszInValue : Set value ex: {"bv":1}
 *  Output              : None
 *  Return               : cahr *     Value in JSON Format, ex: {"sv":"Success"}, {"sv":"Read Only"}
 * Modified Date : 2015/05/14 by Eric Liang
 * ***************************************************************************************/
char* SNCALL  SN_Manager_SetData(const char *pszInUriPath /*URI of Resource*/, const char *pszInValue, const void *pUserData)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	char result[MAX_DATA_SIZE]={0};
	memset( result, 0, sizeof( result ) );

	ADV_DEBUG("SN_Manager_SetData URI=%s Value=%s\n",pszInUriPath,pszInValue);

	if( pszInUriPath || strlen( pszInUriPath ) <= 0 ) {
		snprintf(result, MAX_DATA_SIZE, RETURN_RT_FORMAT,  HTTP_RC_400, REPLY_RESULT_400 ); // Error Msg
	}
	
	// Check SenHub or IoTGW
	if( strstr( pszInUriPath, IOTGW_NAME) ) {//, sizeof(IOTGW_NAME) ) ) {         
		ProcSetIoTGWRESTfulFAPI( pszInUriPath, pszInValue, pUserData, result, MAX_DATA_SIZE );  // IoTGW
	}else if ( strstr( pszInUriPath, SENHUB_NAME ) ) { 		
		ProcSetSenHubRESTfulAPI(  pszInUriPath, pszInValue, pUserData, result, MAX_DATA_SIZE );  // SenHub
	} else { // Error Not Found
		snprintf(result, MAX_DATA_SIZE, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); //NOT Found
	}
	return result;
}

/* **************************************************************************************
 *  Function Name: SN_Manager_GetData
 *  Description       : Get IoT Sensor Data by URI path
 *  Input                  : const char *pszInUriPath,  
 *                                     ex:  IoTGW/WSN/Interface'MAC/Info/SenHubList
 *                                            SenHub'ID/SenHub/SenData/door temp
 *										      SenHub'ID/SenHub/Info/Name
 *  Output              : None
 *  Return               : cahr *     Value in JSON Format, ex: { "StatusCode":  200, "Result": {"n":"temp","v":26} } 
 * Modified Date : 2015/04/27 by Eric Liang
 * ***************************************************************************************/
char* SNCALL  SN_Manager_GetData(const char *pszInUriPath /*URI of Resource*/, const ACTION_MODE mode /* Cache or Direct */)
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	char result[MAX_DATA_SIZE]={0};

	memset( result, 0, sizeof( result ) );

	if( pszInUriPath || strlen( pszInUriPath ) <= 0 ) {
		snprintf(result, MAX_DATA_SIZE, RETURN_RT_FORMAT,  HTTP_RC_400, REPLY_RESULT_400 ); // Error Msg
	}

	// Check SenHub or IoTGW
	if( strstr( pszInUriPath, IOTGW_NAME) ) {//, sizeof(IOTGW_NAME) ) ) {       
		ProcGetIoTGWRESTfulFAPI( pszInUriPath, mode, result, MAX_DATA_SIZE );  // IoTGW
	}else if ( strstr( pszInUriPath, SENHUB_NAME ) ) { 
		ProcGetSenHubRESTfulAPI(  pszInUriPath, mode, result, MAX_DATA_SIZE );  // SenHub
	} else { // Error Not Found
		snprintf(result, MAX_DATA_SIZE, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); // Error Msg
	}
	return result;
}

static void ProcGetIoTGWRESTfulFAPI( const char *uri /* IoTGW/WSN/0000852CF4B7B0E8/Info/SenHubList */, const ACTION_MODE mode,
																	 char *pOutBuf, int nOutSize ) 
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		if( pOutBuf == NULL ) return;

		char Id[MAX_UID_SIZE]={0};
		int len = 0;
	    char *sp = NULL, *sp1 = NULL;

		memset(Id,0,MAX_UID_SIZE);
		CacheBuffer *pCacheBuffer = NULL;

		pCacheBuffer = (CacheBuffer*)HashTableGet(g_snmSystem.pDataHashTable, (char *)uri );

		if( pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL ) {
			snprintf( pOutBuf, nOutSize, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); // Error Msg
		}else {
			if(mode == DIRECT_MODE) {
				sp = (char *)strstr( uri, "/" ); // IoTGW/

				if( sp == NULL ){
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
					return;
				}
		
				sp1 = strstr( sp+1, "/"); // WSN/

				if(sp1 == NULL ) {
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
					return;
				}

				sp = strstr(sp1+1,"/"); // MAC/

				len = sp - sp1;

				if( len-1 <= MAX_UID_SIZE )
					memcpy(Id,sp1+1,len-1);
				else
					memcpy(Id, sp1+1,MAX_UID_SIZE);

				// 1. Check it is a resource "n"
				if( pCacheBuffer->JsType != RESOURCE ) {
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
				}
				else 
				{   // 2. Check this Sensor Hub is busy ( Blocking )
					//if( pSenHubCache->status == STA_BUSY ) {
					//	snprintf(pOutBuf,nOutSize, RETURN_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
					//	return;
					//}
					// 3. Check this resource status is read to set
					if( pCacheBuffer->SetInfo.status != STA_READ ) { 
						snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
					} else {
						char szTmp[256]={0};
						char *pName = (char *)strrchr(uri, '/');
						char *pType = NULL;
						int nSize = 0;

						if( pName ) { 
							nSize = pName - uri;
							memcpy(szTmp,uri, nSize);
							pType = strrchr(szTmp, '/');
							nSize = pType - szTmp;
							memcpy( pCacheBuffer->SetInfo.szType, pType+1, nSize );
						}

						if( pName && pType ) {
							if( pCacheBuffer->SetInfo.set_thread == NULL ) {
								// {"n":"Name"}
								snprintf( pCacheBuffer->SetInfo.szSetBuf, sizeof(pCacheBuffer->SetInfo.szSetBuf), GET_JSON_FORMAT, pName+1 );
								snprintf( pCacheBuffer->SetInfo.szUID,sizeof(pCacheBuffer->SetInfo.szUID),"%s",Id);

								pCacheBuffer->SetInfo.status = STA_LOCKED;
								//pCacheBuffer->SetInfo.pUserData = (void *)pUserData;
								pCacheBuffer->SetInfo.setcmd = SN_Inf_Get;

								pCacheBuffer->SetInfo.pUserData = (void *)pOutBuf;

								// CreateThread and 
								//pCacheBuffer->SetInfo.set_thread = SUSIThreadCreate("Get_IoIGW", 8192, 61, get_sn_data, &pCacheBuffer->SetInfo );
								get_sn_data(&pCacheBuffer->SetInfo);
							}else{
								snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
							}
						} else { // 
							snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_415, REPLY_RESULT_415 ); // Unsupport Media Type
						}							 
					}	// End of pCacheBuffer->SetInfo.status != STA_READ
				} // End of pCacheBuffer->JsType != RESOURCE
			} else {
			// { "Status":200, "Result":{"n":"SenHubList","sv":"0000000EC6F0F830,0000000EC6F0F831"}}
			snprintf(pOutBuf,nOutSize,RETURN_FORMAT,  HTTP_RC_200, cJSON_Print( pCacheBuffer->pJsonItem ) ); 
			}
		}
		return;
}


static void ProcGetSenHubRESTfulAPI( const char *uri /* Id/SenHub/SenData/door name*/, const ACTION_MODE mode,
															char *pOutBuf, int nOutSize )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		if( pOutBuf == NULL ) return;

		char Id[MAX_UID_SIZE]={0};
		SenHubCache *pSenHubCache = NULL;
		CacheBuffer *pCacheBuffer = NULL;
		char *sp = NULL;
	    char *sp1 = NULL;
		int len = 0;
		memset(Id,0,MAX_UID_SIZE);

		sp = (char *)strstr( uri, "/" );
		if(sp)
			len = sp - uri;

		if(len <= MAX_UID_SIZE )
			memcpy(Id,uri,len);
		else
			memcpy(Id,uri,MAX_UID_SIZE);

		pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable,Id );
		if( pSenHubCache == NULL ) {
			snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); // Error Msg
			return;
		}

		pCacheBuffer = (CacheBuffer*)HashTableGet( pSenHubCache->pCacheHashTable, (char *)uri );

		if( pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL ) {
			snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); // Error Msg
		}else {
			if(mode == DIRECT_MODE) {
				// 1. Check it is a resource "n"
				if( pCacheBuffer->JsType != RESOURCE ) {
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
				}
				else 
				{   // 2. Check this Sensor Hub is busy ( Blocking )
					//if( pSenHubCache->status == STA_BUSY ) {
					//	snprintf(pOutBuf,nOutSize, RETURN_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
					//	return;
					//}
					// 3. Check this resource status is read to set
					if( pCacheBuffer->SetInfo.status != STA_READ ) { 
						snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
					} else {
						char szTmp[256]={0};
						char *pName = (char *)strrchr(uri, '/');
						char *pType = NULL;
						int nSize = 0;

						if( pName ) { 
							nSize = pName - uri;
							memcpy(szTmp,uri, nSize);
							pType = strrchr(szTmp, '/');
							nSize = pType - szTmp;
							memcpy( pCacheBuffer->SetInfo.szType, pType+1, nSize );
						}

						if( pName && pType ) {
							if( pCacheBuffer->SetInfo.set_thread == NULL ) {
								// {"n":"Name"}
								snprintf( pCacheBuffer->SetInfo.szSetBuf, sizeof(pCacheBuffer->SetInfo.szSetBuf), GET_JSON_FORMAT, pName+1 );
								snprintf( pCacheBuffer->SetInfo.szUID,sizeof(pCacheBuffer->SetInfo.szUID),"%s",Id);

								pCacheBuffer->SetInfo.status = STA_LOCKED;
								//pCacheBuffer->SetInfo.pUserData = (void *)pUserData;
								pCacheBuffer->SetInfo.setcmd = SN_SenHub_Get;

								pCacheBuffer->SetInfo.pUserData = (void *)pOutBuf;

								// CreateThread and 
								//pCacheBuffer->SetInfo.set_thread = SUSIThreadCreate("Get_IoIGW", 8192, 61, get_sn_data, &pCacheBuffer->SetInfo );
								get_sn_data(&pCacheBuffer->SetInfo);
							}else{
								snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
							}
						} else { // 
							snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_415, REPLY_RESULT_415 ); // Unsupport Media Type
						}							 
					}	// End of pCacheBuffer->SetInfo.status != STA_READ
				} // End of pCacheBuffer->JsType != RESOURCE
			} else {
			// { "StatusCode":200, "Result":{"n":"door","v":26,"min":0, "max":100} }			
				snprintf(pOutBuf,nOutSize, RETURN_FORMAT,  HTTP_RC_200, cJSON_Print( pCacheBuffer->pJsonItem ) ); 
			}
		}
}


static void ProcSetSenHubRESTfulAPI( const char *uri /* Id/SenHub/SenData/dout */, 
																	const char *pszInValue, /*{"bv": 1}*/ const void *pUserData,
																	char *pOutBuf, int nOutSize )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		if( pOutBuf == NULL ) return;

		char Id[MAX_UID_SIZE]={0};
		SenHubCache *pSenHubCache = NULL;
		CacheBuffer *pCacheBuffer = NULL;
		char *sp =NULL;
		int len = 0;
		memset(Id,0,MAX_UID_SIZE);

		sp = (char *)strstr( uri, "/" );
		if(sp)
			len = sp - uri;

		if(len <= MAX_UID_SIZE )
			memcpy(Id,uri,len);
		else
			memcpy(Id,uri,MAX_UID_SIZE);

		pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable,Id );
		if( pSenHubCache == NULL ) {
			snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_404, REPLY_RESULT_404 );
			return;
		}

		pCacheBuffer = (CacheBuffer*)HashTableGet( pSenHubCache->pCacheHashTable, (char *)uri );

		if( pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL ) {
			snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_404, REPLY_RESULT_404 ); // Not Found
		}
		else
		{
			// 1. Check it is a resource "n"
			if( pCacheBuffer->JsType != RESOURCE ) {
				snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
			}else 
			{   // 2. Check this Sensor Hub is busy ( Blocking )
				if( pSenHubCache->status == STA_BUSY ) {
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
					return;
				}

				// 3. Check this resource status is read to set
				if( pCacheBuffer->SetInfo.status != STA_READ ) { 
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
				} else {
					// 5. Check this Resource accessable 
					if( GetSenMLVarableRW( pCacheBuffer->pJsonItem ) < WRITE_ONLY ) {
						snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_405, REPLY_RESULT_405R );
					}	
					else 
					{
							char szValue[256]={0};
							char szTmp[256]={0};
							char *pName = (char *)strrchr(uri, '/');
							char *pType = NULL;
							char *psValue = (char *)strchr(pszInValue,'{');
							char *peValue = (char *)strrchr(pszInValue,'}');
							int nSize = 0;

							if( psValue && peValue ) {									
								nSize = peValue - psValue -1;
								memcpy( szValue,psValue+1,nSize); // "sv":"ROOM1"
							}
							if( pName ) { 
								nSize = pName - uri;
								memcpy(szTmp,uri, nSize);
								pType = strrchr(szTmp, '/');
								nSize = pType - szTmp;
								memcpy( pCacheBuffer->SetInfo.szType, pType+1, nSize );
							}

							if( pName && pType ) {
								if( pCacheBuffer->SetInfo.set_thread == NULL ) {
									// {"n":"Name", "sv":"ROOM1"}
									snprintf( pCacheBuffer->SetInfo.szSetBuf, sizeof(pCacheBuffer->SetInfo.szSetBuf), GET_SET_JSON_FORMAT, pName+1, szValue );																								
									snprintf( pCacheBuffer->SetInfo.szUID,sizeof(pCacheBuffer->SetInfo.szUID),"%s",Id);

									pCacheBuffer->SetInfo.status = STA_LOCKED;
									pCacheBuffer->SetInfo.pUserData = (void *)pUserData;
									pCacheBuffer->SetInfo.setcmd = SN_SenHub_Set; // SenData

									snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_202, REPLY_RESULT_202 );

									// CreateThread and 
									pCacheBuffer->SetInfo.set_thread = SUSIThreadCreate("Set_SenHub", 8192, 61, set_sn_data, &pCacheBuffer->SetInfo );									
								}else{
									snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
								}
							} else { // 
								snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_415, REPLY_RESULT_415 ); // Unsupport Media Type
							}							 
					} // End of GetSenMLVarableRW
				}
			}
		} // End of pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL

		ADV_DEBUG("SetSenHubRESTfulAPI ret=%s\n",pOutBuf);
}

static void ProcSetIoTGWRESTfulFAPI( const char *uri /* IoTGW/WSN/MAC/Info/reset */, 
																	const char *pszInValue, /*"bv": 1*/ const void *pUserData,
																	char *pOutBuf, int nOutSize )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
		if( pOutBuf == NULL ) return;

		char Id[MAX_UID_SIZE]={0};
		int len = 0;
	    char *sp = NULL, *sp1 = NULL;

		memset(Id,0,MAX_UID_SIZE);

		CacheBuffer *pCacheBuffer = NULL;

		pCacheBuffer = (CacheBuffer*)HashTableGet(g_snmSystem.pDataHashTable, (char *)uri );

		if( pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL ) {
			snprintf( pOutBuf, nOutSize, RETURN_RT_FORMAT,  HTTP_RC_404, REPLY_RESULT_404 ); // Error Msg
		}else {
			sp = (char *)strstr( uri, "/" ); // IoTGW/

			if( sp == NULL ){
				snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
				return;
			}
		
			sp1 = strstr( sp+1, "/"); // WSN/

			if(sp1 == NULL ) {
				snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
				return;
			}

			sp = strstr(sp1+1,"/"); // MAC/

			len = sp - sp1;

			if( len-1 <= MAX_UID_SIZE )
				memcpy(Id,sp1+1,len-1);
			else
				memcpy(Id, sp1+1,MAX_UID_SIZE);

			// 1. Check it is a resource "n"
			if( pCacheBuffer->JsType != RESOURCE ) {
				snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_400, REPLY_RESULT_400 );  // Bad Request only support set "n"
			}
			else 
			{   // 2. Check this Sensor Hub is busy ( Blocking )
				//if( pSenHubCache->status == STA_BUSY ) {
				//	snprintf(pOutBuf,nOutSize, RETURN_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
				//	return;
				//}
				// 3. Check this resource status is read to set
				if( pCacheBuffer->SetInfo.status != STA_READ ) { 
					snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
				} else {
					// 5. Check this Resource accessable 
					if( GetSenMLVarableRW( pCacheBuffer->pJsonItem ) < WRITE_ONLY ) {
						snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_405, REPLY_RESULT_405R );
					}	
					else 
					{
							char szValue[256]={0};
							char szTmp[256]={0};
							char *pName = (char *)strrchr(uri, '/');
							char *pType = NULL;
							char *psValue = (char *)strchr(pszInValue,'{');
							char *peValue = (char *)strrchr(pszInValue,'}');
							int nSize = 0;

							if( psValue && peValue ) {									
								nSize = peValue - psValue -1;
								memcpy( szValue,psValue+1,nSize); // "sv":"ROOM1"
							}
							if( pName ) { 
								nSize = pName - uri;
								memcpy(szTmp,uri, nSize);
								pType = strrchr(szTmp, '/');
								nSize = pType - szTmp;
								memcpy( pCacheBuffer->SetInfo.szType, pType+1, nSize );
							}

							if( pName && pType ) {
								if( pCacheBuffer->SetInfo.set_thread == NULL ) {
									// {"n":"Name", "sv":"ROOM1"}
									snprintf( pCacheBuffer->SetInfo.szSetBuf, sizeof(pCacheBuffer->SetInfo.szSetBuf), GET_SET_JSON_FORMAT, pName+1, szValue );
									snprintf( pCacheBuffer->SetInfo.szUID,sizeof(pCacheBuffer->SetInfo.szUID),"%s",Id);

									pCacheBuffer->SetInfo.status = STA_LOCKED;
									pCacheBuffer->SetInfo.pUserData = (void *)pUserData;
									pCacheBuffer->SetInfo.setcmd = SN_Inf_Set;

									snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_202, REPLY_RESULT_202 );

									// CreateThread and 
									pCacheBuffer->SetInfo.set_thread = SUSIThreadCreate("Set_IoIGW", 8192, 61, set_sn_data, &pCacheBuffer->SetInfo );
								}else{
									snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_503, REPLY_RESULT_503 );
								}
							} else { // 
								snprintf(pOutBuf,nOutSize, RETURN_RT_FORMAT, HTTP_RC_415, REPLY_RESULT_415 ); // Unsupport Media Type
							}							 
					} // End of GetSenMLVarableRW
			}	// End of pCacheBuffer->SetInfo.status != STA_READ
		} // End of pCacheBuffer->JsType != RESOURCE
	} // End of pCacheBuffer == NULL || pCacheBuffer->pJsonItem == NULL
	ADV_DEBUG("ProcSetIoTGWRESTfulFAPI ret=%s\n",pOutBuf);
}

void get_sn_data(void *arg)
{
printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	SetParam *pSetParam = (SetParam*) arg;
	char Buffer[MAX_SET_BUF_SIZE]={0};
	SN_CODE rc = SN_ER_FAILED;
	SenHubCache *pSenHubCache = NULL;

	if( pSetParam == NULL ) {
		return;
	}

	InSenData inSenData;
	OutSenData outSenData;
	InBaseData  inBaseData;
	OutBaseData outBaseData;

	memset(&inSenData,0,sizeof(inSenData));
	memset(&outSenData,0,sizeof(outSenData));
	memset(&inBaseData,0,sizeof(inBaseData));
	memset(&outBaseData,0,sizeof(outBaseData));

	// Out Buffer
	char *pOutBuf = (char *)AllocateMemory(MAX_SET_BUF_SIZE);
	int nOutSize = MAX_SET_BUF_SIZE;

	if( pOutBuf )
		memset(pOutBuf,0,MAX_SET_BUF_SIZE);
	
	// In
	snprintf(inSenData.sUID,sizeof(inSenData.sUID),"%s",pSetParam->szUID);
	inSenData.inDataClass.iTypeCount = 1;
	inSenData.inDataClass.pInBaseDataArray = &inBaseData;

	inBaseData.psType = pSetParam->szType;
	inBaseData.iSizeType = MAX_TYPE_SIZE;
	inBaseData.psData  = pSetParam->szSetBuf;
	inBaseData.iSizeData = strlen(pSetParam->szSetBuf);

	// Out
	snprintf(outSenData.sUID,sizeof(outSenData.sUID),"%s",pSetParam->szUID);
	outSenData.outDataClass.iTypeCount = 1;
	outSenData.outDataClass.pOutBaseDataArray = &outBaseData;

	outBaseData.psData = pOutBuf;
	outBaseData.iSizeData = &nOutSize;
	outBaseData.psType = pSetParam->szType;
	outBaseData.iSizeType = MAX_TYPE_SIZE;
	
	rc = pSetParam->pSnInter->SN_GetData( pSetParam->setcmd, &inSenData, &outSenData );

	int code = ( rc ) * -1;

	if( rc < SN_OK ) { // Error
		ADV_DEBUG("Fail to Get\n");		
	} else {
		ADV_DEBUG("Success to Get\n");
	}
	ADV_DEBUG("rc=%d code=%d\n",rc, code);
	snprintf((char *)pSetParam->pUserData,MAX_DATA_SIZE, RETURN_FORMAT,  HTTP_RC_200, outBaseData.psData); 

#if 0
	snprintf(Buffer, MAX_SET_BUF_SIZE, RETURN_RT_FORMAT, g_nHttp_Code[code], g_szHttp_Status[code] ); // { "StatusCode":  200, "Result": "Success"}
	Sleep(500);

	printf("\033[33m %s(%d): Buffer=%s \033[0m\n", __func__,__LINE__,Buffer);
	MUTEX_LOCK( &g_snmSystem.mutexCbfMutex );
	//pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable, inSenData.sUID );

	if( g_ReportSNManagerCbf != NULL ) {
		//rc = g_ReportSNManagerCbf(SN_Inf_UpdateInterface_Data, Buffer, strlen(Buffer), (void **)NULL, NULL, NULL );
		rc = g_ReportSNManagerCbf(SN_Inf_UpdateInterface_Data, outBaseData.psData, strlen(outBaseData.psData), (void **)NULL, NULL, NULL );
	}else {
		ADV_DEBUG("Resource is disconnect=%s\n", inSenData.sUID );
	}

	MUTEX_UNLOCK( &g_snmSystem.mutexCbfMutex );
#endif	
	pSetParam->status = STA_READ;

	FreeMemory( pOutBuf );

	pSetParam->set_thread = NULL;
}

void set_sn_data(void *arg)
{
printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	SetParam *pSetParam = (SetParam*) arg;
	char Buffer[MAX_SET_BUF_SIZE]={0};
	SN_CODE rc = SN_ER_FAILED;
	SenHubCache *pSenHubCache = NULL;

	if( pSetParam == NULL ) {
		return;
	}

	InSenData inSenData;
	OutSenData outSenData;
	InBaseData  inBaseData;
	OutBaseData outBaseData;

	memset(&inSenData,0,sizeof(inSenData));
	memset(&outSenData,0,sizeof(outSenData));
	memset(&inBaseData,0,sizeof(inBaseData));
	memset(&outBaseData,0,sizeof(outBaseData));

	// Out Buffer
	char *pOutBuf = (char *)AllocateMemory(MAX_SET_BUF_SIZE);
	int nOutSize = MAX_SET_BUF_SIZE;

	if( pOutBuf )
		memset(pOutBuf,0,MAX_SET_BUF_SIZE);
	
	// In
	snprintf(inSenData.sUID,sizeof(inSenData.sUID),"%s",pSetParam->szUID);
	inSenData.inDataClass.iTypeCount = 1;
	inSenData.inDataClass.pInBaseDataArray = &inBaseData;

	inBaseData.psType = pSetParam->szType;
	inBaseData.iSizeType = MAX_TYPE_SIZE;
	inBaseData.psData  = pSetParam->szSetBuf;
	inBaseData.iSizeData = strlen(pSetParam->szSetBuf);

	// Out
	snprintf(outSenData.sUID,sizeof(outSenData.sUID),"%s",pSetParam->szUID);
	outSenData.outDataClass.iTypeCount = 1;
	outSenData.outDataClass.pOutBaseDataArray = &outBaseData;

	outBaseData.psData = pOutBuf;
	outBaseData.iSizeData = &nOutSize;
	outBaseData.psType = pSetParam->szType;
	outBaseData.iSizeType = MAX_TYPE_SIZE;
	
	rc = pSetParam->pSnInter->SN_SetData( pSetParam->setcmd, /*SN_SenHub_Set */&inSenData, &outSenData );

	int code = ( rc ) * -1;

	if( rc < SN_OK ) { // Error
		ADV_DEBUG("Fail to Set\n");		
	} else {
		ADV_DEBUG("Success to Set SenHub\n");
	}
	ADV_DEBUG("rc=%d code=%d\n",rc, code);

	snprintf(Buffer, MAX_SET_BUF_SIZE, RETURN_RT_FORMAT, g_nHttp_Code[code], g_szHttp_Status[code] ); // { "StatusCode":  200, "Result": "Success"}
	Sleep(500);

	MUTEX_LOCK( &g_snmSystem.mutexCbfMutex );
	pSenHubCache = (SenHubCache*)HashTableGet( g_snmSystem.pSenHubHashTable, inSenData.sUID );

	if( g_ReportSNManagerCbf != NULL ) {
		rc = g_ReportSNManagerCbf( SN_SetResult, Buffer, strlen( Buffer ), &pSetParam->pUserData, NULL, NULL );	
	}else {
		ADV_DEBUG("Resource is disconnect=%s\n", inSenData.sUID );
	}

	MUTEX_UNLOCK( &g_snmSystem.mutexCbfMutex );
	
	pSetParam->status = STA_READ;

	FreeMemory( pOutBuf );

	pSetParam->set_thread = NULL;
}

static void PRINTF_DataClass( const void *pDataClass, const int size )
{
	printf("[ivan][%s][%s] ****************************************>>>> \n", __FILE__, __func__);
	if( pDataClass == NULL || size != sizeof(InDataClass) ) return;

	InDataClass *pInDataClass =  (InDataClass*) pDataClass;
	InBaseData  *pInBaseData = NULL;

	int i = 0;
	pInBaseData = pInDataClass->pInBaseDataArray;

	if( pInBaseData == NULL ) return;

	ADV_TRACE("Class Number=%d %d\n\n\n", pInDataClass->iTypeCount, pInBaseData );

	for( i = 0; i< pInDataClass->iTypeCount; i ++ ) {
		if(pInBaseData && pInBaseData->psType && pInBaseData->psData )
			ADV_TRACE("Type=%s Data=%s\r\n", pInBaseData->psType, pInBaseData->psData );

		pInBaseData++;
	}

}
#if 0
#if defined(_MSC_VER)
BOOL WINAPI DllMain( HINSTANCE module_handle, DWORD reason_for_call, LPVOID reserved )
{
	if ( reason_for_call == DLL_PROCESS_ATTACH ) // Self-explanatory
	{
		printf( "DllInitializer\n" );
		DisableThreadLibraryCalls( module_handle ); // Disable DllMain calls for DLL_THREAD_*
		if ( reserved == NULL ) // Dynamic load
		{
			// Initialize your stuff or whatever
			// Return FALSE if you don't want your module to be dynamically loaded
		}
		else // Static load
		{
			// Return FALSE if you don't want your module to be statically loaded
			return FALSE;
		}
	}

	if ( reason_for_call == DLL_PROCESS_DETACH ) // Self-explanatory
	{
		printf( "DllFinalizer\n" );
		if ( reserved == NULL ) // Either loading the DLL has failed or FreeLibrary was called
		{
			// Cleanup
			SN_Manager_Uninitialize( );
		}
		else // Process is terminating
		{
			// Cleanup
			SN_Manager_Uninitialize( );
		}
	}
	return TRUE;
}
#else
__attribute__( ( constructor ) )
/**
 * initializer of the shared lib.
 */
static void Initializer( int argc, char** argv, char** envp )
{
    printf( "DllInitializer\n" );
}

__attribute__( ( destructor ) )
/** 
 * It is called when shared lib is being unloaded.
 * 
 */
static void Finalizer( )
{
    printf( "DllFinalizer\n" );
	SN_Manager_Uninitialize( );
}
#endif

#endif


