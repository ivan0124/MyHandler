#ifndef _SUSI_CONTROL_LOG_H_
#define _SUSI_CONTROL_LOG_H_

#include <Log.h>

#define DEF_LOG_NAME    "SUSIControlLog.txt"   //default log file name
#define LOG_ENABLE
//#define DEF_LOG_MODE    (LOG_MODE_NULL_OUT)
//#define DEF_LOG_MODE    (LOG_MODE_FILE_OUT)
#define DEF_LOG_MODE    (LOG_MODE_CONSOLE_OUT|LOG_MODE_FILE_OUT)

#ifdef LOG_ENABLE
#define IoTGWLog(handle, level, fmt, ...)  do { if (handle != NULL)   \
	WriteLog(handle, DEF_LOG_MODE, level, fmt, ##__VA_ARGS__); } while(0)
#else
#define SUSICtrlLog(level, fmt, ...)
#endif


#endif