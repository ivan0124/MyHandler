/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/03/19 by Fred Chang									*/
/* Modified Date: 2015/03/19 by Fred Chang									*/
/* Abstract     : Regarding parser of ini     								*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __SIMU_PARSER_H__
#define __SIMU_PARSER_H__

#include "WSNType.h"
char *ReplaceChar(char *str, char find, char replace);
void ElementParser(char *string, IOT_Sensor* sensor);
int ParserStringValue(char *dest, int size, char *sv, IOT_Sensor *sensor);
int LoadListFile(const char *path, IOT_Sensor* sensor);
int LoadIniDir(const char *path, IOT_Device* devices, IOT_Sensor* defaultvalue);
void SyncVariable(IOT_Sensor *sensor, IOT_Device *device);
int SetInterface(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
int GetInterface(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
int SetInfo(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
int GetInfo(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
int SetSensor(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
int GetSensor(char *mac, IOT_Sensor sensor, IOT_Device *device, OutBaseData *output);
#endif //__SIMU_PARSER_H__