/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/03/16 by Fred Chang									*/
/* Modified Date: 2015/03/16 by Fred Chang									*/
/* Abstract     : Mote Manager Simulator main functions     				*/
/* Reference    : None														*/
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SensorNetwork_API.h"
#include "TopologyTranslate.h"
#include "AdvLog.h"
SN_CODE callback( const int cmdId, const void *pInData, const int InDatalen, void *pUserData, void *pOutParam, void *pRev1, void *pRev2 ) {
	
	
}



int main() {

	SN_Initialize(NULL);
	
	SN_SetUpdateDataCbf( callback );
	
	SNInfInfos outSNInfInfo;
	SN_GetCapability( &outSNInfInfo );
	
	
	
	
	//parent
	do {
		
		getchar();
		
		/*Topology input;
		Topology output;
		
		input.list = strdup("0000000EC6F0F831,0000000EC6F0F830,0000000EC6F0F832");
		input.listlen = strlen(input.list)+1;
		input.mesh = strdup("0000852CF4B7B0E8<=>0000000EC6F0F832,0000000EC6F0F830<=>0000000EC6F0F831,0000000EC6F0F831<=>0000000EC6F0F832,0000000EC6F0F831<=>0000852CF4B7B0E8");
		input.meshlen = strlen(input.mesh)+1;
		
		output.list = strdup("NULL");
		output.listlen = strlen(output.list)+1;
		output.mesh = strdup("NULL");
		output.meshlen = strlen(output.mesh)+1;
		printf("@@@@@@@@@@@@@@\n");
		SN_Translate("0001852CF4B7B0E8", &input, &output);
		
		printf("output.list = %s\n", output.list);
		printf("output.mesh = %s\n", output.mesh);
		
		free(input.list);
		free(input.mesh);
		free(output.list);
		free(output.mesh);*/
		
		
		/*SenData input;
		OutRS_Data output; 
		strcpy(input.sUID,"0001000EC6F0F831");
		//input.psSenData = strdup("{\"n\":\"Room Temp\",\"v\":37}");
		input.psSenData = strdup("{\"n\":\"Name\",\"sv\":\"AAA\"}");
		input.iSizeSenData = strlen(input.psSenData)+1;
		input.pExtened = 0;
		
		output.psRSData = strdup("NULL");
		output.iSizeRSData = (int *)malloc(sizeof(int));
		*output.iSizeRSData = strlen(output.psRSData)+1;
		
		//SN_SetData( SN_SenHub_Set_SenData, &input, &output );
		SN_SetData( SN_SenHub_Set_Info, &input, &output );
		ADV_DEBUG("set. @@@@@@@@@@@@@@@@@output.psRSData = %s\n", output.psRSData);
		SN_GetData( SN_SenHub_Set_Info, &input, &output );
		ADV_DEBUG("get. @@@@@@@@@@@@@@@@@output.psRSData = %s\n", output.psRSData);
		free(input.psSenData);
		free(output.psRSData);
		free(output.iSizeRSData);*/
		
		sleep(0x7FFFFFFF);
		//printf("parent\n");
		
	} while(1);

	return 0;	

}
