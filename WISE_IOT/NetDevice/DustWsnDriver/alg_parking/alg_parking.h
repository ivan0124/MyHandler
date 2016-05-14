
#ifndef _PARKING_ALGORITHM_H_
#define _PARKING_ALGORITHM_H_

#include <pthread.h>            // For pthread
#include <math.h>
#include "IpMgColData.h"

//#define PTHREAD_FLOWCHART   1
#define ALG_API                 1

//#define HEADING 1
//#define HEAD_YZ 1
//#define HEAD_XZ 1
//#define HEAD_XY 1

//#define X_AXIS  1
#define Y_AXIS  1
//#define Y_Z_AXIS 1
//#define Z_AXIS  1


#define MOTE_NUM	            14
#ifdef HEADING
#define THRESHOLD	            30
#define CAR_THRESHOLD           60
#else
#define THRESHOLD	            10
#define CAR_THRESHOLD           28    
#endif
#define STABLE_CNT	            5	
#define AUTOCMD_SCHEDULE_TIME   5
#define FAST_SCHEDULE_TIME      1
#define HEX_FF                  0xFF
#define NO_USED_MAC_ID          HEX_FF
#define UNDEFINED_RAWDATA_VALUE (((INT16)HEX_FF << 8) | (INT16)HEX_FF) 
#define MAC_LEN                 2 // Unit: byte

//#define LOGFILE 1
#ifdef LOGFILE
#define LOG_PATH    "/run/mote"
extern FILE *g_fp;
#endif

#define HEX_VIP001   0x29
#define HEX_VIP002   0xDA
#define HEX_VIP003   0xBC
#define HEX_VIP004   0x5E
#define HEX_VIP005   0x16
#define HEX_VIP006   0xF2
#define HEX_VIP007   0xEA
#define HEX_VIP008   0x0F
#define HEX_VIP009   0x1F
#define HEX_VIP010   0x67
#define HEX_VIP011   0x40
#define HEX_VIP012   0x91

//#define HEX_VIP013   0xE2
//#define HEX_VIP014   0x46

//#define HEX_VIP013   0x75
#define HEX_VIP013   0xD7
#define HEX_VIP014   0x50

#define __PI    3.141592653589793238462643

#define ALG_IPARKING_DEBUG  1
#if defined(ALG_IPARKING_DEBUG) && defined(LOGFILE)
#define DMSG(s, args...) \
    do{ \
        fprintf(g_fp, s, ##args); \
    }while(0)
#elif defined(ALG_IPARKING_DEBUG)
#define DMSG(s, args...) \
    do{ \
        printf(s, ##args); \
    }while(0)
#else
#define DMSG(...) do{}while(0)
#endif

//printf("%s[%d]:%s()->", __FILE__, __LINE__, __FUNCTION__); \
//printf("\n"); \

#ifdef X_AXIS
#define pGET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes->AXIS_X; \
            }while(0);
#define GET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes.AXIS_X; \
            }while(0);            
#elif defined(Y_AXIS)
#define pGET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes->AXIS_Y; \
            }while(0);
#define GET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes.AXIS_Y; \
            }while(0);  
#elif defined(Z_AXIS)
#define pGET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes->AXIS_Z; \
            }while(0);
#define GET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes.AXIS_Z; \
            }while(0);
#else
#define pGET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes->AXIS_X; \
            }while(0);
#define GET_MAIN_AXIS(pAxes, tAxis) \
            do{ \
                tAxis = pAxes.AXIS_X; \
            }while(0);
#endif             

enum 
{
	eDS_Stable      = 0,
	eDS_Detecting   = 1,
	
	eDS_P1FStable   = 2,
    eDS_P1TStable   = 3,
 /*   
	eDS_P1FRing,
	eDS_P1TStable,
	eDS_P1TRing,
    eDS_P2FUnstable,
    eDS_P2TUnstable,
	eDS_P2FDetecting,
	eDS_P2TDetecting,
*/
    eDS_P2Unstable  = 4,
	eDS_P2Detecting = 5,
	eDS_P3Result    = 6,
/*    
	eDS_P2Stable,
	eDS_P3FResult,
	eDS_P3TResult,
*/	
	eDS_NoUsed      = 7,
	eDS_End	
};

typedef struct {
  INT16 i16State;
  BOOL  bTrue;
  INT16 i16MaxAxis;
  INT16 i16MaxGap;
  INT16 i16HeadTrueBase;
  INT16 i16HeadFalseBase;
  INT16 i16PrevHead;
  MagAxesRaw_t tTrueBase;
  MagAxesRaw_t tFalseBase;
  MagAxesRaw_t tPrevData;
}MoteInfo_t;

typedef struct {
  INT8 ai8MAC[MAC_LEN];
}MAC_t;

typedef struct {
  MAC_t tMAC;
}ManagerInfo_t;

typedef struct {
  INT16 i16Thr;
  INT16 i16CarThr;
}iParkingCfg_t;

typedef struct {
  MagAxesRaw_t tMag;
  INT16 i16Temp;
}iParkingPara_t;

extern MagAxesRaw_t g_atDataFromManager[MOTE_NUM];

INT16 i16Init(void);
//INT16 i16CalulateGap(INT16 _i16BaseAxis, INT16 _i16Data);
INT16 i16PutData(INT16 i16MoteN, MagAxesRaw_t *_ptData);
INT16 i16GetData(INT16 i16MoteN, MagAxesRaw_t *_ptData);
INT16 i16GetMAC(INT16 i16MoteN, MAC_t *_ptMAC);
INT16 i16SearchIdx(MAC_t *_ptMAC);
void* pvDetectingThread(void *_pvArg);
INT16 i16Sync(INT16 _i16MoteN);

/*!
  \brief
	Initiation for iParking algorithm.
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16Init(void);

/*!
  \brief
	Uninitiation for iParking algorithm.
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16UnInit(void);

/*!
  \brief
	Set configuration related to algorithm.
  \param        _pCfg configuration you specified
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16SetCfg(iParkingCfg_t *_pCfg);

/*!
  \brief
	Read configuration related to algorithm.
  \param        _pCfg configuration to read
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16ReadCfg(iParkingCfg_t *_pCfg);

/*!
  \brief
	Reset calculating process of algorithm and back to initial for one parking node.
  \param        _i16NodeN node number
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16ResetNode(INT16 _i16NodeN);

/*!
  \brief
	Create and init for one parking node.
  \param        _ptData node parameter
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16NewNode(iParkingPara_t *_ptData);

/*!
  \brief
	Delete one node from calculating process of algorithm.
  \param        _i16NodeN node number
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16DeleteNode(INT16 _i16NodeN);

/*!
  \internal
  \brief
	Check threshold.
  \param        _ptBase base threshold
  \param        _ptData magnetic data currently get from sensor
  \return       TRUE   door open for threshold
  \return       FALSE  nothing
  \endinternal
 */
BOOL PkgAlg_bChkThrP1(MagAxesRaw_t *_ptBase, MagAxesRaw_t *_ptData);

/*!
  \internal
  \brief
	Copy magnetic data from soruce to target.
  \param        _ptSrc magnetic data to copy from
  \param        _ptTarget magnetic data to copy to
  \return       0   success
  \return       !0  failed
  \endinternal
 */
INT16 PkgAlg_i16CopyMagData(MagAxesRaw_t *_ptSrc, MagAxesRaw_t *_ptTarget);

/*!
  \internal
  \brief
	Calculating and update result for one parking node.
  \param        _ptInfo node information
  \param        _ptData magnetic data currently get from sensor
  \return       0   success
  \return       !0  failed
  \endinternal
 */
INT16 PkgAlg_i16UpdateResult(MoteInfo_t *_ptInfo, MagAxesRaw_t *_ptData);

/*!
  \brief
	Calculating and return result currently for one parking node.
  \param        _i16NodeN node number
  \param        _ptData node parameter
  \return       0   success
  \return       !0  failed
 */
BOOL PkgAlg_bDetecting(INT16 _i16NodeN, iParkingPara_t *_ptData);

/*!
  \brief
	Copy node information to array depend on node number.
  \param        _i16NodeN node number
  \param        _ptInfo node information
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16SetNodeInfo(INT16 _i16NodeN, MoteInfo_t *_ptInfo);

/*!
  \brief
	Get node information from array depend on node number.
  \param        _i16NodeN node number
  \param        _ptInfo node information
  \return       0   success
  \return       !0  failed
 */
INT16 PkgAlg_i16GetNodeInfo(INT16 _i16NodeN, MoteInfo_t *_ptInfo);

#endif /* _PARKING_ALGORITHM_H_ */
