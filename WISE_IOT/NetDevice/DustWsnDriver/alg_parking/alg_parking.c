
#include <stdio.h>
#include <pthread.h>            // For pthread
#include <string.h>
#include "IpMgColData.h"
//#include "IpMgWrapper.h"
#include "alg_parking.h"

static MoteInfo_t g_atMoteBuf[MOTE_NUM];
static pthread_mutex_t g_tLock;
MagAxesRaw_t g_atDataFromManager[MOTE_NUM];
MAC_t g_atMoteMac[MOTE_NUM];

INT16 i16Init(void)
{
    if(pthread_mutex_init(&g_tLock, NULL) != 0) {
        printf("mutex init failed\n");
        return 1;
    }    

    //memset((void*)g_atMoteMac, HEX_FF, sizeof(g_atMoteMac));
    //memset((void*)g_atDataFromManager, HEX_FF, sizeof(g_atDataFromManager));

    return 0;
}

INT16 i16Sync(INT16 _i16MoteN)
{
    MagAxesRaw_t tData;

    i16GetData(_i16MoteN, &tData);

    printf("UNDEFINED_RAWDATA_VALUE == %02x, tData.AXIS_X == %02x, INT16 == %d",
                UNDEFINED_RAWDATA_VALUE,  tData.AXIS_X, sizeof(INT16));

    if((tData.AXIS_X&0x0000ffff) != UNDEFINED_RAWDATA_VALUE) {
        printf(" _i16MoteN=%d ,sync ok!, ", _i16MoteN);
        return 0;
    }
    else {
        printf(" _i16MoteN=%d ,sync failed!, ", _i16MoteN);
        return -1;
    }
}

INT16 i16SearchIdx(MAC_t *_ptMAC)
{
    INT16 i;
    MAC_t tMAC;

    memset(&tMAC, NO_USED_MAC_ID, sizeof(tMAC));
    
    pthread_mutex_lock(&g_tLock);
    for(i=0; i<MOTE_NUM; i++) {
        //if(g_atManagerBuf[i].tMAC== (INT16)_tMACCode) {
        if(memcmp((const void*)g_atMoteMac[i].ai8MAC, (const void*)&_ptMAC, MAC_LEN) == 0) {            
            pthread_mutex_unlock(&g_tLock);
            return i;
        }
        
        //if(g_atManagerBuf[i].tMAC == NO_USED_MAC_ID) {
        if(memcmp((const void*)&g_atMoteMac[i].ai8MAC, (const void*)&tMAC, MAC_LEN) == 0) {            
            memcpy((void*)g_atMoteMac[i].ai8MAC, (const void*)&_ptMAC->ai8MAC, MAC_LEN);
            pthread_mutex_unlock(&g_tLock);
            return i;
        }
    }
    pthread_mutex_unlock(&g_tLock);

    //printf("return 03 i16SearchIdx\n");
    return -1;
}

INT16 i16PutData(INT16 i16MoteN, MagAxesRaw_t *_ptData)
{
    pthread_mutex_lock(&g_tLock);
    g_atDataFromManager[i16MoteN].AXIS_X = _ptData->AXIS_X;
    g_atDataFromManager[i16MoteN].AXIS_Y = _ptData->AXIS_Y;
    g_atDataFromManager[i16MoteN].AXIS_Z = _ptData->AXIS_Z;
/*
#ifdef X_AXIS    
    g_atDataFromManager[i16MoteN].AXIS_X = _ptData->AXIS_X;
#else
    g_atDataFromManager[i16MoteN].AXIS_Y = _ptData->AXIS_Y;
#endif
*/
    pthread_mutex_unlock(&g_tLock);
    
    return 0;
}

INT16 i16GetData(INT16 i16MoteN, MagAxesRaw_t *_ptData)
{
    pthread_mutex_lock(&g_tLock);
    _ptData->AXIS_X = g_atDataFromManager[i16MoteN].AXIS_X;
    _ptData->AXIS_Y = g_atDataFromManager[i16MoteN].AXIS_Y;
    _ptData->AXIS_Z = g_atDataFromManager[i16MoteN].AXIS_Z;
/*    
#ifdef X_AXIS        
    _ptData->AXIS_X = g_atDataFromManager[i16MoteN].AXIS_X;
#else
    _ptData->AXIS_Y = g_atDataFromManager[i16MoteN].AXIS_Y;
#endif
*/
    pthread_mutex_unlock(&g_tLock);
    
    return 0;
}

INT16 i16GetMAC(INT16 i16MoteN, MAC_t *_ptMAC)
{
    pthread_mutex_lock(&g_tLock);
    memcpy((void*)_ptMAC->ai8MAC, (const void*)g_atMoteMac[i16MoteN].ai8MAC, MAC_LEN);
    pthread_mutex_unlock(&g_tLock);
    
    return 0;
}

static INT16 i16CalulateHeadGap(INT16 _i16HeadBase, INT16 _i16Data)
{
    INT16 i16Data2;        

    if(_i16Data > _i16HeadBase)
        _i16Data = _i16Data - _i16HeadBase;
    else
        _i16Data = _i16HeadBase - _i16Data;

    i16Data2 = 3600 - _i16Data;
    if(_i16Data > i16Data2) _i16Data = i16Data2;

    return _i16Data;
}

static INT16 i16CalulateGap(INT16 _i16BaseAxis, INT16 _i16Data)
{
    if(_i16BaseAxis < 0) {
            if(_i16Data < 0) {
                if(_i16Data < _i16BaseAxis) {
                    _i16Data = _i16BaseAxis - _i16Data; // -- > +
                }
                else {
                    _i16Data = _i16Data - _i16BaseAxis; // -- > +
                }
            }
            else { // _i16Data >= 0
                _i16Data -= _i16BaseAxis; // +- > + 
            }
    }
    else { // _i16BaseAxis >= 0
            if(_i16Data < 0) {
                _i16Data = _i16BaseAxis - _i16Data; // +- > +
            }
            else { // _i16Data >= 0
                if(_i16Data < _i16BaseAxis) {
                    _i16Data = _i16BaseAxis - _i16Data; // ++ > +
                }
                else {
                    _i16Data = _i16Data - _i16BaseAxis; // ++ > +
                }
            }
    }
      
    return _i16Data;
}

static INT16 i16GetFreeIdx(void)
{
    static INT16 i16NewIdx = 0;
    INT16 i16Idx;

    pthread_mutex_lock(&g_tLock);
    i16Idx = i16NewIdx;
    i16NewIdx++;
    pthread_mutex_unlock(&g_tLock);

    return i16Idx;
}

static int Heading(short i16MX,short i16MY,short i16MZ,float fDX,float fDY)  
{
    float fFixDX,fFixDY,fFixDXEx,fFixDXEx1,fFixDYEx,fFixDYEx1,Head,HC1,HC2;
    short Heading_10;

    fFixDX = asin(-fDX);
    fFixDY = asin(-fDY);
    fFixDXEx = cos(fFixDX);
    fFixDXEx1 = sin(fFixDX);
    fFixDYEx = sin(fFixDY);
    fFixDYEx1 = cos(fFixDY);
    HC1 = fFixDXEx * i16MX - fFixDXEx1 * fFixDYEx * i16MY - fFixDXEx1 * fFixDYEx1 * i16MZ;
    HC2 = -fFixDYEx1 * i16MY + fFixDYEx *i16MZ;
    Head = -atan2(HC2,HC1);
    if(Head < 0)
    {
        Head = Head + __PI * 2;
    }
    Head *= 180 / __PI;
    Head *= 10;
    Heading_10 = (int)Head;

    return(Heading_10);
}

INT16 PkgAlg_i16Init(void) 
{
    INT16 i;
    MoteInfo_t tMoteInfo;

    tMoteInfo.i16State = eDS_NoUsed;
        
    for(i=0; i < MOTE_NUM; i++) {
        PkgAlg_i16SetNodeInfo(i, &tMoteInfo);        
    }

    if(pthread_mutex_init(&g_tLock, NULL) != 0) {
        printf("mutex init failed\n");
        return 1;
    } 

    return 0;
}

INT16 PkgAlg_i16UnInit(void) 
{
    return 0;
}

INT16 PkgAlg_i16SetCfg(iParkingCfg_t *_pCfg) 
{
    return 0;
}

INT16 PkgAlg_i16ReadCfg(iParkingCfg_t *_pCfg) 
{
    return 0;
}

INT16 PkgAlg_i16ResetNode(INT16 _i16NodeN) 
{
    return 0;
}

INT16 PkgAlg_i16NewNode(iParkingPara_t *_ptData)
{
    INT16 i;
    MoteInfo_t tMoteInfo;

    for(i=0; i < MOTE_NUM; i++) {
        PkgAlg_i16GetNodeInfo(i, &tMoteInfo);
        if(tMoteInfo.i16State == eDS_NoUsed)
            break;
    }

    /* No space avaliable */
    if(i >= MOTE_NUM) return -1;

    tMoteInfo.bTrue = FALSE;
    tMoteInfo.i16State = eDS_P1FStable;
    tMoteInfo.tFalseBase.AXIS_X = _ptData->tMag.AXIS_X;
    tMoteInfo.tFalseBase.AXIS_Y = _ptData->tMag.AXIS_Y;
    tMoteInfo.tFalseBase.AXIS_Z = _ptData->tMag.AXIS_Z;
    tMoteInfo.tTrueBase.AXIS_X = 0;
    tMoteInfo.tTrueBase.AXIS_Y = 0;
    tMoteInfo.tTrueBase.AXIS_Z = 0;
    tMoteInfo.tPrevData.AXIS_X = 0;
    tMoteInfo.tPrevData.AXIS_Y = 0;
    tMoteInfo.tPrevData.AXIS_Z = 0;
    
    PkgAlg_i16SetNodeInfo(i, &tMoteInfo);

    return i;
}

INT16 PkgAlg_i16DeleteNode(INT16 _i16NodeN)
{
    return 0;
}

INT16 PkgAlg_i16SetNodeInfo(INT16 _i16NodeN, MoteInfo_t *_ptInfo)
{
    if(_i16NodeN < 0 || _i16NodeN >= MOTE_NUM)
        return -1;

    pthread_mutex_lock(&g_tLock);
    memcpy((void*)&g_atMoteBuf[_i16NodeN], (void*)_ptInfo, sizeof(MoteInfo_t));
    pthread_mutex_unlock(&g_tLock);
    
    return 0;
}

INT16 PkgAlg_i16GetNodeInfo(INT16 _i16NodeN, MoteInfo_t *_ptInfo)
{
    if(_i16NodeN < 0 || _i16NodeN >= MOTE_NUM)
        return -1;

    pthread_mutex_lock(&g_tLock);
    memcpy((void*)_ptInfo, (void*)&g_atMoteBuf[_i16NodeN], sizeof(MoteInfo_t));
    pthread_mutex_unlock(&g_tLock);

    return 0;
}

BOOL PkgAlg_bChkThrP1(MagAxesRaw_t *_ptBase, MagAxesRaw_t *_ptData)
{
    AXIS_t tAxis, tGap, tBaseAxis;

    pGET_MAIN_AXIS(_ptBase, tBaseAxis);
    pGET_MAIN_AXIS(_ptData, tAxis);
    tGap = i16CalulateGap(_ptBase->AXIS_X , _ptData->AXIS_X);

    DMSG(", Data:%04d, PrevData:%04d, Gap:%04d", tAxis, tBaseAxis, tGap);

    if(tGap < THRESHOLD) {
        return FALSE;
    }
    else {
        return TRUE;
    }
}

INT16 PkgAlg_i16CopyMagData(MagAxesRaw_t *_ptSrc, MagAxesRaw_t *_ptTarget)
{
    _ptTarget->AXIS_X = _ptSrc->AXIS_X;
    _ptTarget->AXIS_Y = _ptSrc->AXIS_Y;
    _ptTarget->AXIS_Z = _ptSrc->AXIS_Z;

    return 0;        
}

INT16 PkgAlg_i16UpdateResult(MoteInfo_t *_ptInfo, MagAxesRaw_t *_ptData)
{
    INT16 i16XGap, i16YGap, i16ZGap, i16FinalGap, i16FalseGap, i16TrueGap;

    if(_ptInfo->bTrue == FALSE) {
        _ptInfo->i16State = eDS_P1FStable;
        i16XGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_X , _ptData->AXIS_X);
        i16YGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_Y , _ptData->AXIS_Y);
        i16ZGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_Z , _ptData->AXIS_Z);
            
        if(i16XGap >= i16YGap && i16XGap >= i16ZGap) {
            i16FinalGap = i16XGap;
            _ptInfo->i16MaxAxis = 1;
            DMSG(", phase3: false base, %04d, data, %04d, Xgap, %04d", 
                    _ptInfo->tFalseBase.AXIS_X, _ptData->AXIS_X, i16FinalGap);                 
        }
        else if(i16YGap >= i16ZGap && i16YGap >= i16XGap) {
            i16FinalGap = i16YGap;
            _ptInfo->i16MaxAxis = 2;
            DMSG(", phase3: false base, %04d, data, %04d, Ygap, %04d", 
                    _ptInfo->tFalseBase.AXIS_Y, _ptData->AXIS_Y, i16FinalGap);                 
        }
        else {
            i16FinalGap = i16ZGap;
            _ptInfo->i16MaxAxis = 4;
            DMSG(", phase3: false base, %04d, data, %04d, Zgap, %04d", 
                    _ptInfo->tFalseBase.AXIS_Z, _ptData->AXIS_Z, i16FinalGap);                  
        }
            
        if(i16FinalGap > CAR_THRESHOLD)  {
            _ptInfo->bTrue = TRUE;
            _ptInfo->i16State = eDS_P1TStable;
            _ptInfo->tTrueBase.AXIS_X = _ptData->AXIS_X;
            _ptInfo->tTrueBase.AXIS_Y = _ptData->AXIS_Y;
            _ptInfo->tTrueBase.AXIS_Z = _ptData->AXIS_Z;
        }
    }
    else {
        _ptInfo->i16State = eDS_P1TStable;
        if(_ptInfo->i16MaxAxis == 1) {
            i16TrueGap = i16CalulateGap(_ptInfo->tTrueBase.AXIS_X , _ptData->AXIS_X);
            i16FalseGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_X , _ptData->AXIS_X);            
            DMSG(", phase3: true base, %04d, data, %04d, Xgap, %04d", 
                    _ptInfo->tTrueBase.AXIS_X, _ptData->AXIS_X, i16TrueGap);                   
        }
        else if(_ptInfo->i16MaxAxis == 2) {
            i16TrueGap = i16CalulateGap(_ptInfo->tTrueBase.AXIS_Y, _ptData->AXIS_Y);
            i16FalseGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_Y, _ptData->AXIS_Y);            
            DMSG(", phase3: true base, %04d, data, %04d, Ygap, %04d", 
                    _ptInfo->tTrueBase.AXIS_Y, _ptData->AXIS_Y, i16TrueGap);                  
        }
        else {
            i16TrueGap = i16CalulateGap(_ptInfo->tTrueBase.AXIS_Z, _ptData->AXIS_Z);
            i16FalseGap = i16CalulateGap(_ptInfo->tFalseBase.AXIS_Z, _ptData->AXIS_Z);            
            DMSG(", phase3: true base, %04d, data, %04d, Zgap, %04d", 
                    _ptInfo->tTrueBase.AXIS_Z, _ptData->AXIS_Z, i16TrueGap);           
        }

        if(i16FalseGap < i16TrueGap) {
            _ptInfo->bTrue = FALSE;
            _ptInfo->i16State = eDS_P1FStable;
        }

        if(i16TrueGap > THRESHOLD) {
            _ptInfo->bTrue = FALSE;
            _ptInfo->i16State = eDS_P1FStable;
        }
    }

    return 0;
}

BOOL PkgAlg_bDetecting(INT16 _i16NodeN, iParkingPara_t *_ptData)
{
    static INT16 i16Cnt = 0;
    AXIS_t tAxis, tPrevAxis, tGap, tBaseAxis;
    MoteInfo_t tMoteInfo;
    BOOL bDoor;

    PkgAlg_i16GetNodeInfo(_i16NodeN, &tMoteInfo);

    switch(tMoteInfo.i16State)
    {
        case eDS_P1FStable:
            bDoor = PkgAlg_bChkThrP1(&tMoteInfo.tFalseBase, &_ptData->tMag);
            if(bDoor == FALSE) {
                PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tFalseBase);
                tMoteInfo.i16State = eDS_P1FStable;
            }
            else {
                PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tPrevData);
                //tMoteInfo.i16State = eDS_P2FUnstable;
                tMoteInfo.i16State = eDS_P2Unstable;
                i16Cnt = 0;
            }
            break;

        case eDS_P1TStable:
            bDoor = PkgAlg_bChkThrP1(&tMoteInfo.tTrueBase, &_ptData->tMag);
            if(bDoor == FALSE) {
                PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tTrueBase);
                tMoteInfo.i16State = eDS_P1TStable;
            }
            else {
                PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tPrevData);
                //tMoteInfo.i16State = eDS_P2TUnstable;
                tMoteInfo.i16State = eDS_P2Unstable;
                i16Cnt = 0;
            }
            break;

        case eDS_P2Unstable:
        case eDS_P2Detecting:
            bDoor = PkgAlg_bChkThrP1(&tMoteInfo.tPrevData, &_ptData->tMag);
            if(bDoor == TRUE) {
                i16Cnt = 0;
                PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tPrevData);
                tMoteInfo.i16State = eDS_P2Unstable;
            }
            else {
                if(i16Cnt < STABLE_CNT) {
                    i16Cnt++;
                    PkgAlg_i16CopyMagData(&_ptData->tMag, &tMoteInfo.tPrevData);
                    tMoteInfo.i16State = eDS_P2Detecting;
                }
                else {
                    tMoteInfo.i16State = eDS_P3Result;
                }
            }           
            break;            
#if 0            
        case eDS_P2FUnstable:
        case eDS_P2FDetecting:            
            bDoor = PkgAlg_bChkThrP1(&tMoteInfo.tPrevData, _ptData);
            if(bDoor == TRUE) {
                i16Cnt = 0;
                PkgAlg_i16CopyMagData(_ptData, &tMoteInfo.tPrevData);
                tMoteInfo.i16State = eDS_P2FUnstable;
            }
            else {
                if(i16Cnt < STABLE_CNT) {
                    i16Cnt++;
                    PkgAlg_i16CopyMagData(_ptData, &tMoteInfo.tPrevData);
                    tMoteInfo.i16State = eDS_P2FDetecting;
                }
                else {
                    tMoteInfo.i16State = eDS_P3FResult;
                }
            }           
            break;

        case eDS_P2TUnstable:
        case eDS_P2TDetecting:            
            bDoor = PkgAlg_bChkThrP1(&tMoteInfo.tPrevData, _ptData);
            if(bDoor == TRUE) {
                i16Cnt = 0;
                PkgAlg_i16CopyMagData(_ptData, &tMoteInfo.tPrevData);
                tMoteInfo.i16State = eDS_P2TUnstable;
            }
            else {
                if(i16Cnt < STABLE_CNT) {
                    i16Cnt++;
                    PkgAlg_i16CopyMagData(_ptData, &tMoteInfo.tPrevData);
                    tMoteInfo.i16State = eDS_P2TDetecting;
                }
                else {
                    tMoteInfo.i16State = eDS_P3TResult;
                }
            }           
            break;            
#endif /* if 0 */
        default:
            DMSG(", error state:%d", tMoteInfo.i16State);
            return tMoteInfo.bTrue;
            break;
    }

    if(tMoteInfo.i16State != eDS_P3Result) {
        PkgAlg_i16SetNodeInfo(_i16NodeN, &tMoteInfo);
        DMSG(", state:%d", tMoteInfo.i16State);
        return tMoteInfo.bTrue;
    }    

    PkgAlg_i16UpdateResult(&tMoteInfo, &_ptData->tMag);
    PkgAlg_i16SetNodeInfo(_i16NodeN, &tMoteInfo);

    DMSG(", state:%d", tMoteInfo.i16State);
    return tMoteInfo.bTrue;
}

