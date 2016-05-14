#include "IpMgWrapper.h"
#include "DustWsnDrv.h"
#include "MoteListAccess.h"
#include "AdvLog.h"

#if defined(WIN32) //windows
#include "AdvPlatform.h"
#endif

extern BOOL	g_doSensorScan;

//static int MacArrayToString(uint8_t *macArray, uint8_t arrayLen, char *pOutBuf, uint8_t outBufLen)
int MacArrayToString(uint8_t *macArray, uint8_t arrayLen, char *pOutBuf, uint8_t outBufLen)
{
	snprintf(pOutBuf, outBufLen,
			"%02x%02x%02x%02x%02x%02x%02x%02x",
			macArray[0], macArray[1], macArray[2], macArray[3],
			macArray[4], macArray[5], macArray[6], macArray[7]
			);
}

int MoteList_GetMoteMacListString(Mote_List_t *pList, char *pOutBuf, const int outBufLen)
{
	Mote_Info_t *item = NULL;
	char *MoteMacString;
	int iRet = -1;
	int targetLen = 0;
	int usedBufLen = 0;
	BOOL isFirstItem = TRUE;
	
	if (NULL==pList || NULL==pOutBuf)
		return iRet;

	MoteMacString = malloc(MAX_MAC_LEN);

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL == item->state) {
			if (FALSE == isFirstItem) {
				strncat(pOutBuf, ",", 1);
				usedBufLen += 1;
			} else {
				isFirstItem = FALSE;
			}
			
			memset(MoteMacString, 0, MAX_MAC_LEN);
			MacArrayToString(item->macAddress, 8, MoteMacString, MAX_MAC_LEN);
			targetLen = strlen(MoteMacString);
			if ((usedBufLen + targetLen + 1) > outBufLen) {
				ADV_ERROR("Not enough buffer size!\n");
				goto SAFE_EXIT;
			}
			strncat(pOutBuf, MoteMacString, targetLen);
			usedBufLen += targetLen;
			//strncat(pOutBuf, ",", 1);
			//usedBufLen += 1;
			//printf("current string=%s\n", pOutBuf);
		}

		item = item->next;
	}	
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL == pList->item[i].state)	{
			MacArrayToString(pList->item[i].macAddress, 8, MoteMacString, MAX_MAC_LEN);
			targetLen = strlen(MoteMacString);
			if ((usedBufLen + targetLen + 1) > outBufLen) {
				ADV_ERROR("Not enough buffer size!\n");
				goto SAFE_EXIT;
			}
			strncat(pOutBuf, MoteMacString, targetLen);
			usedBufLen += targetLen;
			strncat(pOutBuf, ",", 1);
			usedBufLen += 1;
		}
	}
	}
#endif

	free(MoteMacString);
	iRet = 0;

SAFE_EXIT:
	return iRet;	
}

int MoteList_ResetMoteSensorList(Mote_Info_t *pMote)
{
	int i;

	if (NULL == pMote)
		return -1;
	
#ifdef SENSOR_LIST
	ADV_TRACE("MoteList_ResetMoteSensorList: SENSOR_LIST not implemented\n");
#else
	for (i=0; i<pMote->sensorList.total; i++) {
		memset(&pMote->sensorList.item[i], 0, sizeof(Sensor_Info_t));
	}
	pMote->sensorList.total = 0;
#endif

	return 0;
}

Mote_Info_t* MoteList_GetFirstScanItem(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL==item->state && 
			!item->isAP && 
			Mote_Query_NotStart==item->MoteQueryState && 
			Mote_Manufacturer_Advantech==item->manufacturer) 
		{
			target = item;
			break;
		}
		item = item->next;
	}
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL==pList->item[i].state && 
			!pList->item[i].isAP && 
			Mote_Query_NotStart==pList->item[i].MoteQueryState &&
			Mote_Manufacturer_Advantech==pList->item[i].manufacturer) 
		{
			target = &pList->item[i];
			break;
		}
	}
	}
#endif
	
	return target;
}

Mote_Info_t* MoteList_GetFirstItemWithSensorList(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;
	
#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL==item->state && !item->isAP && Mote_Query_SensorList_Received==item->MoteQueryState) {
			target = item;
			break;
		}
		item = item->next;
	}
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL==pList->item[i].state && !pList->item[i].isAP && Mote_Query_SensorList_Received==pList->item[i].MoteQueryState) {
			target = &pList->item[i];
			break;
		}
	}
	}
#endif
	
	return target;
}

Mote_Info_t* MoteList_GetFirstItemToSendObserve(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	{
	int j;
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL==item->state && !item->isAP &&
			 	Mote_Query_Observe_Start_Sent == item->MoteQueryState) {

			if(item->sensorList.total != 0 &&
				 	item->scannedSensor == item->sensorList.total) {
				target = item;
			}

			if (target != NULL)
				break;
		}
		item = item->next;
	}
	}
#else
	{
	int i, j;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL==pList->item[i].state && !pList->item[i].isAP && Mote_Query_Observe_Started!=pList->item[i].MoteQueryState) {
			item = &pList->item[i];
			if(item->sensorList.total != 0 && item->scannedSensor == item->sensorList.total)
				target = item;

			if (target != NULL)
				break;
		}
	}
	}
#endif

	return target;
}

Mote_Info_t* MoteList_GetFirstItemToSendCancelObserve(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	{
	int j;
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL==item->state && !item->isAP && Mote_Query_Observe_Stop_Sent == item->MoteQueryState) {
				target = item;
				if (target != NULL)
					break;
		}
		item = item->next;
	}
	}
#else
	{
	int i, j;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL==pList->item[i].state && !pList->item[i].isAP && Mote_Query_Observe_Stop_Sent==pList->item[i].MoteQueryState) {
			item = &pList->item[i];
			target = item;

			if (target != NULL)
				break;
		}
	}
	}
#endif

	return target;
}

Mote_Info_t* MoteList_GetFirstItemToSendCmd(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	{
	int j;
	item = pList->item;
	while (NULL != item) {
		if (MOTE_STATE_OPERATIONAL==item->state && !item->isAP && Mote_Query_SendCmd_Sent==item->MoteQueryState) {
			target = item;
			break;
		}
		item = item->next;
	}
	}
#else
	{
	int i, j;
	for (i=0; i<pList->total; i++) {
		if (MOTE_STATE_OPERATIONAL==pList->item[i].state && !pList->item[i].isAP && Mote_Query_SendCmd_Sent==pList->item[i].MoteQueryState) {
			item = &pList->item[i];
			target = item;
			break;
		}
	}
	}
#endif

	return target;
}

Mote_Info_t* MoteList_GetFirstItemToPathUpdate(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	{
	item = pList->item;
	while (NULL != item) {
		//if (MOTE_STATE_OPERATIONAL == item->state && item->bPathUpdate) {
		if (MOTE_STATE_OPERATIONAL == item->state || MOTE_STATE_LOST == item->state) {
			if(item->bPathUpdate) {
				target = item;
				break;
			}
		}
		item = item->next;
	}
	}
#else
	{
	int i, j;
	for (i=0; i<pList->total; i++) {
		//if (MOTE_STATE_OPERATIONAL==pList->item[i].state && pList->item[i].bPathUpdate) {
		if (MOTE_STATE_OPERATIONAL == pList->item[i].state || MOTE_STATE_LOST == pList->item[i].state) {
			if(pList->item[i].bPathUpdate) {
				item = &pList->item[i];
				target = item;
				break;
			}
		}
	}
	}
#endif

	return target;
}


Mote_Info_t* MoteList_GetLastItem(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		target = item;
		item = item->next;
	}	
#else
	{
	int idx = pList->total - 1;
	target = &pList->item[idx];
	}
#endif

	return target;
}

//Mote_Info_t* MoteList_GetLastOperItem(Mote_List_t *pList);

Mote_Info_t* MoteList_GetItemByMac(Mote_List_t *pList, uint8_t *pTargetMac)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL==pList || NULL==pTargetMac)
		return NULL;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (0 == memcmp(pTargetMac, item->macAddress, 8)) {
			target = item;
			break;
		}
		item = item->next;
	}
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (0 == memcmp(pTargetMac, pList->item[i].macAddress, 8)) {
			target = &pList->item[i];
			break;
		}
	}
	}
#endif

	return target;
}

Mote_Info_t* MoteList_GetItemById(Mote_List_t *pList, const int targetId)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		//ADV_TRACE("MoteList_GetItemById: item->moteId=%d\n", item->moteId);
		if (targetId == item->moteId) {
			target = item;
			break;
		}
		item = item->next;
	}
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (targetId == pList->item[i].moteId) {
				target = &pList->item[i];
				break;				
		}
	}
	}
#endif

	return target;	
}

Mote_Info_t* MoteList_GetNextOperItemById(Mote_List_t *pList, const int targetId)
{
	BOOL isTargetIdFound = FALSE;
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return NULL;
	
#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		//ADV_TRACE("MoteList_GetItemById: item->moteId=%d\n", item->moteId);
		if (isTargetIdFound) {
			if (MOTE_STATE_OPERATIONAL == item->state) {
				target = item;
				break;				
			}
		}
			
		if (targetId == item->moteId) {
			isTargetIdFound = TRUE;
		}
		
		item = item->next;
	}
#else
	{
	int i;
	for (i=0; i<pList->total; i++) {
		if (isTargetIdFound) {
			if (MOTE_STATE_OPERATIONAL == pList->item[i].state) {
				target = &pList->item[i];
				break;				
			}
		}

		if (targetId == pList->item[i].moteId) {
			isTargetIdFound = TRUE;
		}
	}
	}
#endif

	
	return target;	
}

int MoteList_AddItem(Mote_List_t *pList, Mote_Info_t *pMote)
{
	Mote_Info_t *pItem = NULL;

	if (NULL==pList || NULL==pMote)
		return -1;

#ifdef MOTE_LIST
	pItem = (Mote_Info_t *) malloc(sizeof(Mote_Info_t));
	if (NULL == pItem)
		return -1;
	
	memset(pItem, 0, sizeof(Mote_Info_t));

	memcpy(pItem, pMote, sizeof(Mote_Info_t));
	/*
	memcpy(item->macAddress, pMoteConfig->macAddress, sizeof(pMoteConfig->macAddress));
	item->moteId = pMoteConfig->moteId;
	item->isAP = pMoteConfig->isAP;
	item->state = pMoteConfig->state;
	item->dustReserved = pMoteConfig->reserved;
	item->isRouting = pMoteConfig->isRouting;
	item->MoteQueryState = Mote_Query_NotStart;
	item->prev = NULL;
	item->next = NULL; */
	
	if (NULL == pList->item) {
		pList->item = pItem;
	} else {
		Mote_Info_t *lastItem = MoteList_GetLastItem(pList);
		lastItem->next = pItem;
		pItem->prev = lastItem;
	}
#else
	{
	int idx;
	if ((pList->total+1) > MAX_MOTE_NUM) {
		ADV_ERROR("MAX_MOTE_NUM reached. New mote add failed!\n");
		return -1;
	}
	idx = pList->total;
	memcpy(pList->item[idx].macAddress, pMote->macAddress, sizeof(pMote->macAddress));
	pList->item[idx].moteId = pMote->moteId;
	pList->item[idx].isAP = pMote->isAP;
	pList->item[idx].state = pMote->state;
	pList->item[idx].dustReserved = pMote->dustReserved;
	pList->item[idx].isRouting = pMote->isRouting;
	//pList->item[idx].MoteQueryState = Mote_Query_NotStart;
	pList->item[idx].scannedSensor = pMote->scannedSensor;
	pList->item[idx].MoteQueryState = pMote->MoteQueryState;
	memcpy(pList->item[idx].hostName, pMote->hostName, sizeof(pList->item[idx].hostName));
	memcpy(pList->item[idx].productName, pMote->productName, sizeof(pList->item[idx].productName));
	}
#endif

	pList->total++;
	return 0;
}

int MoteList_AddOrUpdateItem(Mote_List_t *pList, Mote_Info_t *pMoteConfig)
{
	Mote_Info_t *item = NULL;

	if (NULL==pList || NULL==pMoteConfig)
		return -1;

	item = MoteList_GetItemByMac(pList, pMoteConfig->macAddress);
	if (NULL == item) {
		ADV_TRACE("MoteList_AddItemIfNeeded - New Mote found, call MoteList_AddItem()\n");
		MoteList_AddItem(pList, pMoteConfig);
	} else {
		ADV_TRACE("MoteList_AddItemIfNeeded - Mote already exists, update info\n");
		//item->moteId = pMoteConfig->moteId;
		//item->isAP = pMoteConfig->isAP;
		item->state = pMoteConfig->state;
		//item->dustReserved = pMoteConfig->reserved;
		//item->isRouting = pMoteConfig->isRouting;
	}

	if (MOTE_STATE_OPERATIONAL == pMoteConfig->state) {
		ADV_TRACE("do sensor scan\n");
		g_doSensorScan = TRUE;
	} else
		ADV_TRACE("pMoteConfig->state=%d\n", pMoteConfig->state);

	return 0;
}

int MoteList_RemoveItemByMac(Mote_List_t *pList, uint8_t *pTargetMac)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL==pList || NULL==pTargetMac)
		return -1;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (0 == memcmp(pTargetMac, item->macAddress, 8)) {
			if (item == pList->item)
				pList->item = item->next;
			if (item->prev != NULL)
				item->prev->next = item->next;
			if (item->next != NULL)
				item->next->prev = item->prev;
			
			target = item;
			break;
		}
		
		item = item->next;
	}	
	
	if (target != NULL) {
		pList->total--;
		free(target);		// when using item->sensorList, need to free sensorList first
		target = NULL;
	}
#else
	ADV_ERROR("MoteList_RemoveItemByMac - To Be implemented!\n");
#endif

	return 0;
}

int MoteList_RemoveItemById(Mote_List_t *pList, const int targetId)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;
	
	if (NULL == pList)
		return -1;

#ifdef MOTE_LIST
	item = pList->item;
	while (NULL != item) {
		if (targetId == item->moteId) {
			if (item == pList->item)
				pList->item = item->next;
			if (item->prev != NULL)
				item->prev->next = item->next;
			if (item->next != NULL)
				item->next->prev = item->prev;
			
			target = item;
			break;
		}
		
		item = item->next;
	}	
	
	if (target != NULL) {
		pList->total--;
		free(target);		// when using item->sensorList, need to free sensorList first
		target = NULL;
	}
#else
	ADV_ERROR("MoteList_RemoveItemById - To Be implemented!\n");
#endif

	return 0;
}

int MoteList_RemoveList(Mote_List_t *pList)
{
	Mote_Info_t *target = NULL;
	Mote_Info_t *item = NULL;

	ADV_TRACE("remove mote list!\n");

	if (NULL == pList)
		return -1;

#ifdef MOTE_LIST
	item = MoteList_GetLastItem(pList);
	while (NULL != item) {
		target = item;
		item = item->prev;

		pList->total--;
		free(target);
		target = NULL;
	}
	
	if (pList->total != 0) {
		ADV_ERROR("Remove Mote List Error!\n");
		return -1;
	}
#else
	pList->total = 0;
#endif

	return 0;
}

