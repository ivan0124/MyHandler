#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "platform.h"
#include "tool.h"

void GetNowTimeString(char *string, int length, const char *fmt) {
	static time_t oldt = 0;
	static char timeStr[1024] = {0};
	time_t t = time(NULL);
	struct tm timetm;
	if(oldt != t) {
		localtime_r(&t, &timetm);
		strftime(timeStr, sizeof(timeStr), fmt, &timetm);
		oldt = t;
	}
	strncpy(string, timeStr, length);
}

void ReduceBackGroundAndDumpString(const char *buffer, char **output) {
	if(strchr(buffer,'\n') != NULL) {
		int count = 0;
		int len = strlen(buffer);
		char *saveptr;
		char *temp = strdup(buffer);
		char *set;
		char *pos = temp;
		
		pos = strchr(pos, '\n');
		while (pos != NULL) {
			pos++;
			count++;
			pos = strchr(pos, '\n');
		}
		*output = (char *)malloc(len + 6 * count + 1);
		pos = *output;
		set = strtok_r(temp, "\n", &saveptr);
		while (set != NULL) {
			pos += sprintf(pos,"%s%s\n",set,__COLOR_NONE);
			set = strtok_r(NULL, "\n", &saveptr);
		}
		free(temp);
		len = strlen(*output);
		(*output)[len] = 0;
	} else {
		*output = NULL;
	}

	return;
}


int GetFileSize(FILE *fp) {
	int fd = fileno(fp);
	struct stat buf;
	fstat(fd, &buf);
	return buf.st_size;
}

char *fmalloc(const char *filename) {
	FILE *fp = NULL;
	fp = fopen(filename,"r");
	if(fp == NULL) return NULL;
	int size = GetFileSize(fp);
	char *buffer;
	if(size > 10485760) return NULL; //10M
	buffer = (char *)malloc(size);
	fread(buffer,size,1,fp);
	fclose(fp);
	return buffer;
}
