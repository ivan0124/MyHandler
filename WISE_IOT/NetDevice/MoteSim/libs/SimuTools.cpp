#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "platform.h"
#include "SimuTools.h"
#include "AdvLog.h"
int GetRuleValue(IOT_Sensor *sensor, int index, int second) {
	IOT_SimRule *rule = sensor->rule;
	IOT_SimProcess *process;
	int value = -1;
	
	while(rule != NULL) {
		if(rule->index != index) {
			if(rule->next == NULL) return -1;
			else {
				rule = rule->next;
			}
		} else {
			second = second % rule->total;
			//printf("[%d] sec = %d\n", index, second);
			
			process = rule->process;
			
			do {
				if(process->sec <= second) {
					value = process->value;
					process = process->next;
				} else break;
			} while(process != NULL);
			return value;
		}
	}
	return value;
}

int ValueIsDifferent(IOT_Sensor *sensor, int index, int value) {
	IOT_SimRule *rule = sensor->rule;
	while(rule != NULL) {
		if(rule->index != index) {
			if(rule->next == NULL) return -1;
			else {
				rule = rule->next;
			}
		} else {
			if(value == rule->oldvalue) return 0;
			else {
				rule->oldvalue = value;
				return 1;
			}
		}
	}
	return 1;
}

int ParserRule(char *rulestr, IOT_Sensor *sensor) {
	IOT_SimRule *rule;
	IOT_SimProcess *process;
	int total = 0;
	int result = -1;
	if(sensor->rule == NULL) {
		sensor->rule = new IOT_SimRule;
		rule = sensor->rule;
		rule->index = 1;
		rule->oldvalue = -1;
		rule->next = NULL;
		rule->process = new IOT_SimProcess;
		process = rule->process;
		process->next = NULL;
		process->sec = 0;
		process->value = -1;
	} else {
		rule = sensor->rule;
		while(rule->next != NULL) {
			rule = rule->next;
		}
		rule->next = new IOT_SimRule;
		rule->next->index = rule->index+1;
		rule->oldvalue = -1;
		rule = rule->next;
		rule->next = NULL;
		rule->process = new IOT_SimProcess;
		process = rule->process;
		process->next = NULL;
		process->sec = 0;
		process->value = -1;
	}
	
	//printf("rule->index = %d\n", rule->index);
	char secondStr[16];
	int second;
	char valueStr[16];
	int value;
	char *saveptr;
	char *set = strtok_r(rulestr, "-", &saveptr);
	while(set != NULL) {
		
		//printf("set = %s\n", set);
		
		sscanf(set,"%[^@]@%s",secondStr, valueStr);
		second = atoi(secondStr);
		value = atoi(valueStr);
		
		//printf("s = %d, v = %d\n",second, value);
		if(second == 0) {
			process->value = value;
			result = value;
		} else {
			process->next = new IOT_SimProcess;
			process = process->next;
			process->next = NULL;
			process->sec = second;
			process->value = value;
			result = value;
		}
		total = second;
		set = strtok_r(NULL, "-", &saveptr);
	}
	
	rule->process->value = result;
	rule->total = total+1;
	return rule->index;
}

int PrintRule(IOT_Sensor *sensor) {
	IOT_SimRule *rule;
	IOT_SimProcess *process;
	if(sensor->rule == NULL) {
		ADV_WARN("NONE\n");
		return 0;
	} else {
		rule = sensor->rule;
		do {
			ADV_TRACE("[%d]{%d} ", rule->index, rule->total);
			process = rule->process;
			do {
				ADV_TRACE("%d@%d,",process->sec, process->value);
				process = process->next;
			} while(process != NULL);
			ADV_TRACE("\n");
			rule = rule->next;
		} while(rule != NULL);
	}
	return 0;
}

void SetInBaseData(InBaseData *base, const char *tag, const char *data) {
	base->iSizeType = strlen(tag) + 1;
	base->psType = strdup(tag);
	base->iSizeData = strlen(data) + 1;
	base->psData = strdup(data);
}

void ReleaseInBaseData(InBaseData *base) {
	base->iSizeType = 0;
	if(base->psType != NULL) {
		free(base->psType);
		base->psType = NULL;
	}
	
	base->iSizeData = 0;
	if(base->psData != NULL) {
		free(base->psData);
		base->psData = NULL;
	}
}

void SetOutBaseData(OutBaseData *base, const char *tag, const char *data) {
	base->iSizeType = strlen(tag) + 1;
	base->psType = strdup(tag);
	base->iSizeData = (int *)malloc(sizeof(int));
	*base->iSizeData = strlen(data) + 1;
	base->psData = strdup(data);
}

void ReleaseOutBaseData(OutBaseData *base) {
	base->iSizeType = 0;
	if(base->psType != NULL) {
		free(base->psType);
		base->psType = NULL;
	}
	
	if(base->iSizeData != NULL) {
		free(base->iSizeData);
		base->iSizeData = NULL;
	}
	if(base->psData != NULL) {
		free(base->psData);
		base->psData = NULL;
	}
}

void CopyOutBaseData(OutBaseData *dest, OutBaseData *source) {

	if(source->iSizeType > dest->iSizeType) {
		dest->psType = (char *)realloc(dest->psType,source->iSizeType);
		dest->iSizeType = source->iSizeType;
	} 
	strcpy(dest->psType, source->psType);
	
	if(*source->iSizeData > *dest->iSizeData) {
		dest->psData = (char *)realloc(dest->psData,*source->iSizeData);
		*dest->iSizeData = *source->iSizeData;
	} 
	strcpy(dest->psData, source->psData);
}

