/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.														 */
/* Create Date  : 2015/03/06 by Eric Liang															     */
/* Modified Date: 2015/03/06 by Eric Liang															 */
/* Abstract     : Load Sensor Network API                       										    */
/* Reference    : None																									 */
/****************************************************************************/
#include <stdio.h>
#include <dlfcn.h>
#include <unistd.h>
#include "platform.h"
#include "BasicFun_Tool.h"
#include "LoadSNAPI.h"

// AdvLog
#include "AdvLog.h"

// Global Variable

SNAPI_Interface           g_SNAPI_Interface;
SNAPI_Interface           *pSNAPI = NULL ;
//-----------------------------------------------------------------------------
// function:
//-----------------------------------------------------------------------------
//



int GetSNAPILibFn( const char *LibPath, void **pFunInfo )
{
	int Ret = 0;
	if( g_SNAPI_Interface.Workable == 1 ) {
		*pFunInfo = (void*)&g_SNAPI_Interface;
		Ret = 1;
	} else if( 1 == LoadcSNAPILib( LibPath, &g_SNAPI_Interface ) ) {
		*pFunInfo = (void*)&g_SNAPI_Interface;
		Ret = 1;
	}
	return Ret;
}