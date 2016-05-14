#if 0

#if defined(WIN32) //windows
#pragma comment(lib, "AdvLog.lib")
#endif

#include <unistd.h>

#include "AdvLog.h"
int main(int argc, char **argv) {

	AdvLog_Init();
	AdvLog_Default("-d 9 -j 1");
	AdvLog_Arg(argc, argv);
	
	ADV_CRASH("Hello World!!\n");
	ADV_ERROR("Hello World!!\n");
	ADV_WARN("Hello World!!\n");
	ADV_NOTICE("Hello World!!\n");
	ADV_INFO("Hello World!!\n");
	ADV_DEBUG("Hello World!!\n");
	ADV_TRACE("Hello World!!\n");
	ADV_DUMP("Hello World!!\n");

	while(1) {
		usleep(1000000);
	}

}


#else

#include <stdio.h>
#include <stdlib.h>
#if defined(WIN32) //windows
#include <AdvPlatform.h>
#pragma comment(lib, "AdvLog.lib")
#endif
#include <unistd.h>
#include <dlfcn.h>
#include "SensorNetwork_BaseDef.h"
#include "SensorNetwork_API.h"
#include "AdvLog.h"


#include <string>
#include <iostream>
using namespace std;;

SN_CODE SNCALL callback( const int cmdId, const void *pInData, const int InDatalen, void *pUserData, void *pOutParam, void *pRev1, void *pRev2 ) {
	
	return SN_OK;
}

int main(int argc, char **argv) 
{
	printf("My pid = %d\n", getpid());
	/*FILE* base = NULL;
	if ((base = fopen(".\\testmote2", "w+")) == NULL)
	{
		printf("Cannot open file.\n");
		puts(strerror(errno));

		getchar();
		exit(1);
	}
	

	fwrite("0123456789", 10, 1, base);
	fclose(base);

	cout << "my directory is " << ExePath() << "\n";*/
	AdvLog_Init();
	//AdvLog_Default("-d 9 -j 1");
	AdvLog_Arg(argc, argv);
	
	/*ADV_CRASH("Hello World!!\n");
	ADV_ERROR("Hello World!!\n");
	ADV_WARN("Hello World!!\n");
	ADV_NOTICE("Hello World!!\n");
	ADV_INFO("Hello World!!\n");
	ADV_DEBUG("Hello World!!\n");
	ADV_TRACE("Hello World!!\n");
	ADV_DUMP("Hello World!!\n");
	exit(0);*/
	
	
	void *handle;	/* shared library 的 'handle' 指標 */
	SN_Initialize_API init;
	SN_Uninitialize_API uninit;
	SN_SetUpdateDataCbf_API registercb; 
	SN_GetCapability_API getcap;
	
    char *error;	/* 記錄 dynamic loader 的錯誤訊息 */
	
    /* 開啟 shared library 'libm' */
#if defined(WIN32) //windows
	handle = dlopen("./MoteSim.dll", RTLD_LAZY);
#elif defined(__linux)//linux
	handle = dlopen ("./libMoteSim.so", RTLD_LAZY);
#endif
    if (!handle) {
        fprintf (stderr, "%s\n", dlerror());
        exit(1);
    }
	
    dlerror();    /* Clear any existing error */
	
    /* 在 handle 指向的 shared library 裡找到 "cos" 函數,
     * 並傳回他的 memory address 
     */
    init = (SN_Initialize_API)dlsym(handle, "SN_Initialize");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
	
	uninit = (SN_Uninitialize_API)dlsym(handle, "SN_Uninitialize");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
	
	registercb = (SN_SetUpdateDataCbf_API)dlsym(handle, "SN_SetUpdateDataCbf");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
	
	getcap = (SN_GetCapability_API)dlsym(handle, "SN_GetCapability");
    if ((error = dlerror()) != NULL)  {
        fprintf (stderr, "%s\n", error);
        exit(1);
    }
	
    /* indirect function call (函數指標呼叫),
     * 呼叫所指定的函數
     */
	
	init(NULL);
	
	registercb( callback );

	SNInfInfos outSNInfInfo;
	outSNInfInfo.iNum = 2;
	outSNInfInfo.SNInfs[0].outDataClass.iTypeCount = 1;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData));
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeType = 256;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psType = (char *)malloc(256);
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData = (int *)malloc(sizeof(int));
	*(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData) = 256;
	outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psData = (char *)malloc(256);
	outSNInfInfo.SNInfs[1].outDataClass.iTypeCount = 1;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray = (OutBaseData *)malloc(sizeof(OutBaseData));
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeType = 256;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psType = (char *)malloc(256);
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData = (int *)malloc(sizeof(int));
	*(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData) = 256;
	outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psData = (char *)malloc(256);
	getcap( &outSNInfInfo );
	
	
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psType);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->iSizeData);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray->psData);
	free(outSNInfInfo.SNInfs[0].outDataClass.pOutBaseDataArray);
	
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psType);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->iSizeData);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray->psData);
	free(outSNInfInfo.SNInfs[1].outDataClass.pOutBaseDataArray);
	
	//parent
	do {
		sleep(2);
		ADV_ERROR("running\n");
		
	} while(1);

	return 0;	



    dlclose(handle);
    return 0;
}
#endif