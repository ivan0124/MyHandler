#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <string>
#include "platform.h"
#include "tool.h"
#include "configure.h"
#include "rollingfile.h"


//size_t strftime(char *s, size_t max, const char *format, const struct tm *tm);
static char pathname[256] = "logs";
static char lastfile[256];
static char filename[256];
static char fullname[256];
static FILE *logfile = NULL;
static int limit = 102400;
static int currentcount;
static int staticGray = 0;

inline static void RollFile_New() {
	int pid = AdvLog_Configure_GetPid();
	char timestr[256]= {0};
	char checkfilename[256] = {0};
	time_t t = time(NULL);
	struct tm timetm;
	localtime_r(&t, &timetm);
	strftime(timestr, sizeof(timestr), "%Y-%m-%d_%H-%M-%S", &timetm);
	mkdir(pathname,S_IRUSR | S_IWUSR | S_IXUSR | S_IWGRP | S_IRGRP | S_IXGRP | S_IWOTH | S_IXOTH);
	
	staticGray = AdvLog_Configure_GetStaticGray();


	snprintf(checkfilename, sizeof(checkfilename), "%s@%d", timestr, pid < 0 ? 0 : pid);
	if(strcmp(checkfilename,filename) == 0) {
		if(staticGray == 0) {
			snprintf(fullname, sizeof(fullname), "%s/%s@%s", pathname, filename, ".html");
		} else {
			snprintf(fullname, sizeof(fullname), "%s/%s@%s", pathname, filename, ".log");
		}
	} else {
		strncpy(filename,checkfilename,sizeof(filename));
		if(staticGray == 0) {
			snprintf(fullname, sizeof(fullname), "%s/%s%s", pathname, filename, ".html");
		} else {
			snprintf(fullname, sizeof(fullname), "%s/%s%s", pathname, filename, ".log");
		}
	}
	logfile = fopen(fullname,"w+");

	if(staticGray == 0) { 
		fprintf(logfile,"<!DOCTYPE html><html><head>" ADV_HTML_CSS_STYLE "</head><body><pre>");
	}
}

void RollFile_Open(const char *path) {
	strncpy(pathname,path,sizeof(pathname));
}

void RollFile_SetLimit(int byte) {
	limit = byte;
}

void RollFile_RefreshConfigure() {
	limit = AdvLog_Configure_GetLimit();
	strncpy(pathname,AdvLog_Configure_GetPath(),sizeof(pathname));
}

inline static void RollFile_Rolling() {
	
	if(strlen(lastfile) != 0) { 
		unlink(lastfile);
	}
	if(staticGray == 0) { 
		fprintf(logfile,"</pre></body></html>");
	}
	
	if(logfile != NULL) {
		fclose(logfile);
		logfile = NULL;
	}
	
	if(staticGray == 0) {
		sprintf(lastfile,"%s/__%s%s",pathname,filename,".html");
	} else {
		sprintf(lastfile,"%s/__%s%s",pathname,filename,".log");
	}
	rename(fullname,lastfile);
	
	RollFile_RefreshConfigure();
	
	RollFile_New();
}

void RollFile_StreamIn(const char *stream, int length) {
	if(logfile == NULL) {
		RollFile_New();
	}
	
	if(staticGray != AdvLog_Configure_GetStaticGray()) {
		RollFile_Rolling();
		currentcount = 0;
	}
	
	currentcount += length;
	fwrite(stream, length, 1, logfile);
	fflush(logfile);
	if(currentcount >= limit) {
		RollFile_Rolling();
		currentcount = 0;
	}
}

void RollFile_Flush() {
	
}

void RollFile_Close() {
	if(logfile != NULL) {
		fclose(logfile);
		logfile = NULL;
	}
}
