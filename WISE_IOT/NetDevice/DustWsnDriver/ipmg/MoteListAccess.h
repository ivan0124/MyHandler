#ifndef _MOTELIST_ACCESS_H_
#define _MOTELIST_ACCESS_H_

#include "dn_ipmg.h"
#include "DustWsnDrv.h"


#ifdef __cplusplus
extern "C" {
#endif

int 			MacArrayToString(uint8_t *macArray, uint8_t arrayLen, char *pOutBuf, uint8_t outBufLen);

int 			MoteList_GetMoteMacListString(Mote_List_t *pList, char *pOutBuf, int bufLen);

int 			MoteList_ResetMoteSensorList(Mote_Info_t *pMote);

Mote_Info_t*	MoteList_GetFirstScanItem(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetFirstItemWithSensorList(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetFirstItemToSendObserve(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetFirstItemToSendCmd(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetFirstItemToPathUpdate(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetLastItem( Mote_List_t *pList );

//Mote_Info_t*	MoteList_GetLastOperItem(Mote_List_t *pList);

Mote_Info_t*	MoteList_GetItemByMac( Mote_List_t *pList, uint8_t *pTargetMac );

Mote_Info_t*	MoteList_GetItemById( Mote_List_t *pList, const int targetId );

Mote_Info_t*	MoteList_GetNextOperItemById(Mote_List_t *pList, const int targetId);

int 			MoteList_AddItem(Mote_List_t *pList, Mote_Info_t *pMote);

int				MoteList_AddOrUpdateItem( Mote_List_t *pList, Mote_Info_t *pMoteConfig );	// not used

int				MoteList_RemoveItemByMac( Mote_List_t *pList, uint8_t *pTargetMac );

int				MoteList_RemoveItemById( Mote_List_t *pList, const int targetId );

int				MoteList_RemoveList( Mote_List_t *pList );

#ifdef __cplusplus 
} 
#endif 

#endif /* _MOTELIST_ACCESS_H_ */
