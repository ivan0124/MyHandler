/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/03/16 by Fred Chang									*/
/* Modified Date: 2015/03/16 by Fred Chang									*/
/* Abstract     : WSN type definition     								    */
/* Reference    : None														*/
/****************************************************************************/
#ifndef __WSN_TYPE_H__
#define __WSN_TYPE_H__

#include <string.h>
#include "SensorNetwork_BaseDef.h"
#include "SensorNetwork_API.h"

#if defined(WIN32) //windows
#define INI_PATH "."
#elif defined(__linux)//linux
#define INI_PATH "/usr/lib/Advantech/iotgw/"
#endif

#define READ	1
#define WRITE	2

#define WSN_TEMPERATURE 			"Temperature"
#define WSN_TEMPERATURE_PATTERN 	"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_TEMPERATURE_SPEC 		"|asm|:|r|, |type|:|d|, |rt|:|ucum.Cel|, |st|:|ipso|, |exten|:|sid=3303|"
#define WSN_TEMPERATURE_FLAG		READ

#define WSN_GPI			 			"GPI"
#define WSN_GPI_PATTERN 			"|n|:|%s|, |u|:, |bv|:%s, |min|:%s, |max|:%s"
#define WSN_GPI_SPEC				"|asm|:|r|, |type|:|b|, |rt|:|gpio.din|, |st|:|ipso|, |exten|:|sid=3200|"
#define WSN_GPI_FLAG				READ

#define WSN_GPO			 			"GPO"
#define WSN_GPO_PATTERN 			"|n|:|%s|, |u|:, |bv|:%s, |min|:%s, |max|:%s"
#define WSN_GPO_SPEC				"|asm|:|rw|, |type|:|b|, |rt|:|gpio.dout|, |st|:|ipso|, |exten|:|sid=3201|"
#define WSN_GPO_FLAG				READ|WRITE

#define WSN_CO2			 			"CO2"
#define WSN_CO2_PATTERN 			"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_CO2_SPEC 				"|asm|:|r|, |type|:|d|, |rt|:|ucum.ppm|, |st|:|ipso|, |exten|:|sid=9001|"
#define WSN_CO2_FLAG				READ

#define WSN_CO			 			"CO"
#define WSN_CO_PATTERN 				"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_CO_SPEC					"|asm|:|r|, |type|:|d|, |rt|:|ucum.ppm|, |st|:|ipso|, |exten|:|sid=9002|"
#define WSN_CO_FLAG					READ

#define WSN_SONAR			 		"Sonar"
#define WSN_SONAR_PATTERN 			"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_SONAR_SPEC 				"|asm|:|r|, |type|:|d|, |rt|:|ucum.cm|, |st|:|ipso|, |exten|:|sid=9200|"
#define WSN_SONAR_FLAG				READ

#define WSN_GSENSOR			 		"G-Sensor"
#define WSN_GSENSOR_PATTERN 		"|n|:|%s|, |u|:|%s|, |sv|:|[{|n|:|x|,|v|:%s},{|n|:|y|,|v|:%s},{|n|:|z|,|v|:%s}]|, |min|:%s, |max|:%s"
#define WSN_GSENSOR_SPEC 			"|asm|:|r|, |type|:|s|, |rt|:|ucum.g|, |st|:|ipso|, |exten|:|sid=3133|"
#define WSN_GSENSOR_FLAG			READ

#define WSN_MAGNETIC			 	"Magnetic"
#define WSN_MAGNETIC_PATTERN 		"|n|:|%s|, |u|:|%s|, |sv|:|[{|n|:|x|,|v|:%s},{|n|:|y|,|v|:%s},{|n|:|z|,|v|:%s}]|, |min|:%s, |max|:%s"
#define WSN_MAGNETIC_SPEC			"|asm|:|r|, |type|:|s|, |rt|:|ucum.G|, |st|:|ipso|, |exten|:|sid=3134|"
#define WSN_MAGNETIC_FLAG			READ

#define WSN_LIGHT			 		"Light"
#define WSN_LIGHT_PATTERN 			"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_LIGHT_SPEC				"|asm|:|r|, |type|:|d|, |rt|:|ucum.lx|, |st|:|ipso|, |exten|:|sid=3301|"
#define WSN_LIGHT_FLAG				READ

#define WSN_HUMIDITY			 	"Humidity"
#define WSN_HUMIDITY_PATTERN 		"|n|:|%s|, |u|:|%s|, |v|:%s, |min|:%s, |max|:%s"
#define WSN_HUMIDITY_SPEC			"|asm|:|r|, |type|:|d|, |rt|:|ucum.%|, |st|:|ipso|, |exten|:|sid=3304|"
#define WSN_HUMIDITY_FLAG			READ

#define WSN_FORMAT					"{\"n\":\"SenHubList\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Neighbor\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"Health\",\"v\":0,\"asm\":\"r\"},{\"n\":\"Name\",\"sv\":\"\",\"asm\":\"r\"},\
									{\"n\":\"sw\",\"sv\":\"\",\"asm\":\"r\"},{\"n\":\"reset\",\"bv\":0,\"asm\":\"rw\"}"

enum {
	WSN_TEMPERATURE_ENUM,
	WSN_GPI_ENUM,
	WSN_GPO_ENUM,
	WSN_CO2_ENUM,
	WSN_CO_ENUM,
	WSN_SONAR_ENUM,
	WSN_GSENSOR_ENUM,
	WSN_MAGNETIC_ENUM,
	WSN_LIGHT_ENUM,
	WSN_HUMIDITY_ENUM,
	WSN_TYPE_NUMBER,
};



enum {
	VTYPE_NONE,
	VTYPE_NUM,
	VTYPE_BIN,
	VTYPE_STRING,
	VTYPE_VSTRING,
};

enum {
	TRIGGER_EDGE,
	TRIGGER_LEVEL,
};



typedef struct {
	int a;
	int b;
} IOT_Link;


typedef struct __IOT_SimProcess IOT_SimProcess;

struct __IOT_SimProcess {
	int sec;
	int value;
	IOT_SimProcess *next;
};

typedef struct __IOT_SimRule IOT_SimRule;
struct __IOT_SimRule {
	int index;
	int total;
	int oldvalue;
	IOT_SimProcess *process;
	IOT_SimRule *next;
};


typedef struct {
	int len;
	char type[32];
	char pattern[256];
	char standard[256];
	int flag;
	int vtype;
	char n[256];
	char u[32];
	char v[256];
	char min[32];
	char max[32];
	int trigger;
	int tsec;
	int secoff;
	int responsesec;
	IOT_SimRule *rule;
} IOT_Sensor;


typedef struct {
	SenHubInfo info;
	
	int modify;
	char *connect;
	int reset;
	
	int sensorNum;
	IOT_Sensor *sensor;
	
} IOT_Mote;


typedef struct {
	char *version;
	int health;
	char *senHubList;
	char *neighbor;
	int reset;
} IOT_InterfaceInfo;

typedef struct {
	SNMultiInfInfo infos;
	SNInterfaceInfoSpec specs;
	int ifaceNum;
	SNInterfaceData iface[MAX_SN_INF_NUM];
	IOT_InterfaceInfo faceinfo[MAX_SN_INF_NUM];
	int moteNum;
	IOT_Mote *mote;
} IOT_Device;



#define PRINT_RED "\033[31m"
#define PRINT_GREEN "\033[32m"
#define PRINT_YELLOW "\033[33m"
#define PRINT_BLUE "\033[34m"
#define PRINT_PURPLE "\033[35m"
#define PRINT_CYAN "\033[36m"
#define PRINT_GRAY "\033[1;30m"

#define PRINT_RED_BG "\033[30;41m"
#define PRINT_GREEN_BG "\033[30;42m"
#define PRINT_YELLOW_BG "\033[30;43m"
#define PRINT_BLUE_BG "\033[30;44m"
#define PRINT_PURPLE_BG "\033[30;45m"
#define PRINT_CYAN_BG "\033[30;46m"
#define PRINT_WHITE_BG "\033[30;47m"

#define PRINT_ENDLINE "\033[0m\n"


#define DEBUG
#endif //__WSN_TYPE_H__