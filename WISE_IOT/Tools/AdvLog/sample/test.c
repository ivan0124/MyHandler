#include <unistd.h>
#include "AdvLog.h"



int main(int argc, char **argv) {
	AdvLog_Init();
	AdvLog_Arg(argc,argv);


do {
	ADV_C_INFO(COLOR_RED,"Hello World!!\n");
	ADV_C_INFO(COLOR_GREEN,"Hello World!!\n");	
	ADV_C_INFO(COLOR_YELLOW,"Hello World!!\n");
	ADV_C_INFO(COLOR_BLUE,"Hello World!!\n");
	ADV_C_INFO(COLOR_PURPLE,"Hello World!!\n");	
	ADV_C_INFO(COLOR_CYAN,"Hello World!!\n");
	ADV_C_INFO(COLOR_GRAY,"Hello World!!\n");

	ADV_C_INFO(COLOR_RED_BG,"Hello World!!\n");
	ADV_C_INFO(COLOR_GREEN_BG,"Hello World!!\n");	
	ADV_C_INFO(COLOR_YELLOW_BG,"Hello World!!\n");
	ADV_C_INFO(COLOR_BLUE_BG,"Hello World!!\n");
	ADV_C_INFO(COLOR_PURPLE_BG,"Hello World!!\n");	
	ADV_C_INFO(COLOR_CYAN_BG,"Hello World!!\n");
	ADV_C_INFO(COLOR_WHITE_BG,"Hello World!!\n");
	ADV_INFO("\n\n");
	sleep(1);
} while(1);
/*
	do {
		ADV_INFO("a = %d, b = %d",100, 200);
		ADV_C_WARN(COLOR_GREEN,"a = %d, b = %d",100, 200);
		sleep(1);
	} while(1);
*/
}




