/****************************************************************************/
/* Copyright(C) : Advantech Technologies, Inc.								*/
/* Create Date  : 2015/03/19 by Fred Chang									*/
/* Modified Date: 2015/03/19 by Fred Chang									*/
/* Abstract     : Regarding tool functions of rule     						*/
/* Reference    : None														*/
/****************************************************************************/
#ifndef __SIMU_TOOLS_H__
#define __SIMU_TOOLS_H__

#include "WSNType.h"
int GetRuleValue(IOT_Sensor *sensor, int index, int second);
int ValueIsDifferent(IOT_Sensor *sensor, int index, int value);
int ParserRule(char *rulestr, IOT_Sensor *sensor);
int PrintRule(IOT_Sensor *sensor);

void SetInBaseData(InBaseData *base, const char *tag, const char *data);
void ReleaseInBaseData(InBaseData *base);

void SetOutBaseData(OutBaseData *base, const char *tag, const char *data);
void ReleaseOutBaseData(OutBaseData *base);
void CopyOutBaseData(OutBaseData *dest, OutBaseData *source);
#endif //__SIMU_TOOLS_H__