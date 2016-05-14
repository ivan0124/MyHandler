#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include "platform.h"
#include "tool.h"
#include "advstring.h"
#include "AdvLog.h"
#include "configure.h"
#include "rollingfile.h"



#define TIME_STRING_FMT "\t[%Y/%m/%d %H:%M:%S]"

//Normal
static pthread_mutex_t *pmutex = NULL;
static pthread_mutex_t mutex;
advstring strin(1024);
advstring strout(1024);

char LevelTag[LOG_MAX][256] =
{
	{ "" },
	{ __COLOR_RED_BG "[CRASH]" __COLOR_NONE __COLOR_GRAY },	//1
	{ __COLOR_RED_BG "[ERROR]" __COLOR_NONE __COLOR_GRAY },	//2
	{ __COLOR_YELLOW "[WARN]" __COLOR_NONE __COLOR_GRAY },	//3
	{ __COLOR_YELLOW "[NOTICE]" __COLOR_NONE __COLOR_GRAY },	//4
	{ "[INFO]" },										//5
	{ "" },											//6
	{ __COLOR_WHITE_BG "[DEBUG]" __COLOR_NONE __COLOR_GRAY },//7
	{ __COLOR_WHITE_BG "[TRACE]" __COLOR_NONE __COLOR_GRAY },//8
	{ __COLOR_WHITE_BG "[DUMP]" __COLOR_NONE __COLOR_GRAY }	//9
};

char LevelNoColor[LOG_MAX][256] =
{
	{ "" },
	{ "[CRASH]" },	//1
	{ "[ERROR]" },	//2
	{ "[WARN]" },		//3
	{ "[NOTICE]" },	//4
	{ "[INFO]" },		//5
	{ "" },			//6
	{ "[DEBUG]" },	//7
	{ "[TRACE]" },	//8
	{ "[DUMP]" }		//9
};

char LevelHtml[LOG_MAX][256] =
{
	{ "" },
	{ __COLOR_RED_BG_HTML "[CRASH]" __COLOR_RED_BG_END_HTML },	//1
	{ __COLOR_RED_BG_HTML "[ERROR]" __COLOR_RED_BG_END_HTML },	//2
	{ __COLOR_YELLOW_HTML "[WARN]" __COLOR_YELLOW_END_HTML },	//3
	{ __COLOR_YELLOW_HTML "[NOTICE]" __COLOR_YELLOW_END_HTML },	//4
	{ "[INFO]" },										//5
	{ "" },											//6
	{ __COLOR_WHITE_BG_HTML "[DEBUG]" __COLOR_WHITE_BG_END_HTML },//7
	{ __COLOR_WHITE_BG_HTML "[TRACE]" __COLOR_WHITE_BG_END_HTML },//8
	{ __COLOR_WHITE_BG_HTML "[DUMP]" __COLOR_WHITE_BG_END_HTML }	//9
};

#if defined(WIN32)
void AdvLog_Uninit() {
#else
void ADVLOG_CALL AdvLog_Uninit() {
#endif
	//AdvLog_Configure_Init(getpid());
	RollFile_Close();
}

void ADVLOG_CALL AdvLog_Init() {
	if(pmutex == NULL) {
		pthread_mutex_init(&mutex, NULL);
		pmutex = &mutex;
	}
	AdvLog_Configure_Init(getpid());
	RollFile_Open(AdvLog_Configure_GetPath());
	atexit(AdvLog_Uninit);
}

void ADVLOG_CALL AdvLog_Arg(int argc, char **argv) {
	AdvLog_Configure_OptParser(argc, argv, getpid());
	RollFile_RefreshConfigure();
}

void ADVLOG_CALL AdvLog_Default(char *arg) {
	char temp[1024];
	char *pos = temp;
	char *end;
	int number;
	strcpy(temp,arg);
	std::vector<std::string> args;
	char *saveptr;
	
	args.push_back("AdvLog");
	
	while(1) {
		if(*pos =='\"' || *pos == '\'') {
			pos++;
			number = strcspn(pos,"\"\'");
			pos[number] = 0;
			args.push_back(pos);
			pos += number+2;
		} else {
			if((end = strchr(pos,' ')) != NULL) {
				end[0] = 0;
				args.push_back(pos);
				pos = end+1;
			} else {
				args.push_back(pos);
				break;
			}
		}
	}

	int argc = args.size();
	char **argv = (char **)malloc(sizeof(char *)*argc);
	for (int i = 0; i < argc; i++) {
		argv[i] = (char *)args[i].c_str();
	}
	AdvLog_Arg(argc, argv);
	free(argv);
}

void ADVLOG_CALL AdvLog_Control(int argc, char **argv) {
	AdvLog_Configure_OptParser(argc, argv, -1);
}

int ADVLOG_CALL AdvLog_AllNone(int level) {
	return level > AdvLog_Configure_GetDynamicLevel() && level > AdvLog_Configure_GetStaticLevel();  
}

int ADVLOG_CALL AdvLog_Static_Is_On(int level) {
	return level <= AdvLog_Configure_GetStaticLevel();
}

inline int AdvLog_Static_Is_Gray_Mode(int level) {
	return AdvLog_Configure_GetStaticGray();
}

inline int AdvLog_Static_Info_Is_On() {
	return AdvLog_Configure_GetStaticInfo();
}

int ADVLOG_CALL AdvLog_Dynamic_Is_On(int level) {
	return level <= AdvLog_Configure_GetDynamicLevel();
}

inline int AdvLog_Dynamic_Is_Gray_Mode(int level) {
	return AdvLog_Configure_GetDynamicGray();
}

inline int AdvLog_Dynamic_Info_Is_On() {
	return AdvLog_Configure_GetDynamicInfo();
}

int ADVLOG_CALL AdvLog_Dynamic_Is_Hiden(int level) {
	return AdvLog_Configure_Is_Hiden(level);
}

#if defined(WIN32) //windows
HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
inline short getwcolor(int color) {
	switch (color) {
	case COLOR_RED:
		return _W_COLOR_RED;
	case COLOR_GREEN:
		return _W_COLOR_GREEN;
	case COLOR_YELLOW:
		return _W_COLOR_YELLOW;
	case COLOR_BLUE:
		return _W_COLOR_BLUE;
	case COLOR_PURPLE:
		return _W_COLOR_PURPLE;
	case COLOR_CYAN:
		return _W_COLOR_CYAN;
	case COLOR_GRAY:
		return _W_COLOR_GRAY;

	case COLOR_RED_BG:
		return _W_COLOR_RED_BG;
	case COLOR_GREEN_BG:
		return _W_COLOR_GREEN_BG;
	case COLOR_YELLOW_BG:
		return _W_COLOR_YELLOW_BG;
	case COLOR_BLUE_BG:
		return _W_COLOR_BLUE_BG;
	case COLOR_PURPLE_BG:
		return _W_COLOR_PURPLE_BG;
	case COLOR_CYAN_BG:
		return _W_COLOR_CYAN_BG;
	case COLOR_WHITE_BG:
		return _W_COLOR_WHITE_BG;
	case COLOR_NONE:
	default:
		return _W_COLOR_NONE;

	}
}
inline void setlevel(advstring& out, int level) {
	switch (level) {

	case LOG_CRASH:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_RED_BG));
		out.insert(1, "[CRASH]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_ERROR:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_RED_BG));
		out.insert(1, "[ERROR]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_WARN:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_YELLOW));
		out.insert(1, "[WARN]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_NOTICE:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_YELLOW));
		out.insert(1, "[NOTICE]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_INFO:
		out.insert(1, "[INFO]", NULL);
		printf(out.str());
		break;
	case LOG_DEBUG:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		out.insert(1, "[DEBUG]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_TRACE:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		out.insert(1, "[TRACE]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_DUMP:
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_WHITE_BG));
		out.insert(1, "[DUMP]", NULL);
		printf(out.str());
		SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
		break;
	case LOG_NONE:
	default:
		break;
	}
}

inline void __AdvLog_PrintContent(int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *content) {
	char timestr[256] = {0};

	CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
	WORD saved_attributes;
	GetConsoleScreenBufferInfo(hConsole, &consoleInfo);
	saved_attributes = consoleInfo.wAttributes;

	if(AdvLog_Dynamic_Info_Is_On()) {
		if(timestr[0] == 0) {
			//GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
			snprintf(timestr, sizeof(timestr), "[%s %s]",__DATE__,__TIME__);
		}
		if (AdvLog_Dynamic_Is_Gray_Mode(level)) {
			strout.insert(1, content, timestr, " ", LevelNoColor[level], " <", file, ",", func, ",", line, ">\n", NULL);
			printf("%s", strout.str());
		}
		else {
			if (color != NULL) {
				//strout.insert(1,color,content,COLOR_NONE COLOR_GRAY,timestr," ",LevelTag[level]," <",file,",",func,",",line,">" COLOR_NONE "\n",NULL);
				SetConsoleTextAttribute(hConsole, getwcolor(color));
				strout.insert(1, content, NULL);
				printf("%s", strout.str());
				SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
				strout.insert(1, timestr, " ", NULL);
				printf(strout.str());
				setlevel(strout, level);
				strout.insert(1, " <", file, ",", func, ",", line, ">", NULL);
				printf(strout.str());
				SetConsoleTextAttribute(hConsole, saved_attributes);
				strout.insert(1, "\n", NULL);
				printf(strout.str());
			}
			else {
				//strout.insert(1, content, COLOR_GRAY, timestr, " ", LevelTag[level], " <", file, ",", func, ",", line, ">" COLOR_NONE "\n", NULL);
				strout.insert(1, content, NULL);
				printf("%s",strout.str());
				SetConsoleTextAttribute(hConsole, getwcolor(COLOR_GRAY));
				strout.insert(1, timestr, " ", NULL);
				printf(strout.str());
				setlevel(strout, level);
				strout.insert(1, " <", file, ",", func, ",", line, ">", NULL);
				printf(strout.str());
				SetConsoleTextAttribute(hConsole, saved_attributes);
				strout.insert(1, "\n", NULL);
				printf(strout.str());
			}
		}
	} else {
		if (color != NULL && !AdvLog_Dynamic_Is_Gray_Mode(level)) {
			//strout.insert(1,color,content,COLOR_NONE,NULL);
			SetConsoleTextAttribute(hConsole, getwcolor(color));
			strout.insert(1, content, NULL);
			printf("%s", strout.str());
			SetConsoleTextAttribute(hConsole, saved_attributes);
		} else {
			//strout.insert(1,content,NULL);
			strout.insert(1, content, NULL);
			printf("%s", strout.str());
		}
	}
}

#elif defined(__linux)//linux

inline const char *getcolor(int color) {
        switch (color) {
        case COLOR_RED:
                return __COLOR_RED;
        case COLOR_GREEN:
                return __COLOR_GREEN;
        case COLOR_YELLOW:
                return __COLOR_YELLOW;
        case COLOR_BLUE:
                return __COLOR_BLUE;
        case COLOR_PURPLE:
                return __COLOR_PURPLE;
        case COLOR_CYAN:
                return __COLOR_CYAN;
        case COLOR_GRAY:
                return __COLOR_GRAY;

        case COLOR_RED_BG:
                return __COLOR_RED_BG;
        case COLOR_GREEN_BG:
                return __COLOR_GREEN_BG;
        case COLOR_YELLOW_BG:
                return __COLOR_YELLOW_BG;
        case COLOR_BLUE_BG:
                return __COLOR_BLUE_BG;
        case COLOR_PURPLE_BG:
                return __COLOR_PURPLE_BG;
        case COLOR_CYAN_BG:
                return __COLOR_CYAN_BG;
        case COLOR_WHITE_BG:
                return __COLOR_WHITE_BG;
        case COLOR_NONE:
        default:
                return __COLOR_NONE;
        }
}

inline void __AdvLog_PrintContent(int level, int color, const char *file, const char *func, const char *line, const char* levels, const char *content) {
	char timestr[256] = {0};
	char *string = NULL;
	ReduceBackGroundAndDumpString(content, &string);

	if(AdvLog_Dynamic_Info_Is_On()) {
		if(timestr[0] == 0) {
			GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
		}
		if(AdvLog_Dynamic_Is_Gray_Mode(level)) {
			strout.insert(1,(string==NULL ? content : string),timestr," ",LevelNoColor[level]," <",file,",",func,",",line,">\n",NULL);
		} else {
			if(color != COLOR_NONE) {
				strout.insert(1,getcolor(color),(string==NULL ? content : string),__COLOR_NONE __COLOR_GRAY,timestr,__COLOR_NONE " ",LevelTag[level]," <",file,",",func,",",line,">" __COLOR_NONE "\n",NULL);
			} else {
				strout.insert(1,(string==NULL ? content : string),__COLOR_GRAY,timestr,__COLOR_NONE " ",LevelTag[level]," <",file,",",func,",",line,">" __COLOR_NONE "\n",NULL);
			}
		}
	} else {
		if(color != COLOR_NONE && !AdvLog_Dynamic_Is_Gray_Mode(level)) {
			strout.insert(1,getcolor(color),(string==NULL ? content : string),__COLOR_NONE,NULL);
		} else {
			strout.insert(1,(string==NULL ? content : string),NULL);
		}
	}
	//std::cout << strout.str();
	fwrite(strout.str(),strout.size(),1,stdout);

	if(string != NULL) free(string);
}
#endif

inline void __AdvLog_WriteContent(int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels, const char *content) {
	char timestr[256] = {0};
	if(AdvLog_Static_Info_Is_On()) {
		if(timestr[0] == 0) {
			GetNowTimeString(timestr, sizeof(timestr), TIME_STRING_FMT);
		}
		
		if(AdvLog_Static_Is_Gray_Mode(level)) {
			strout.insert(1,content,timestr," ",LevelNoColor[level]," (",file,",",func,",",line,")\n",NULL);
		} else {
			if(color != NULL) {
				strout.insert(1,color,content,colorend,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
			} else {
				strout.insert(1,content,__COLOR_GRAY_HTML,timestr, " ",LevelHtml[level]," (",file,",",func,",",line,")" __COLOR_GRAY_END_HTML "\n",NULL);
			}
		}
		RollFile_StreamIn(strout.str(), strout.size());
	} else {
		if(color != NULL && !AdvLog_Static_Is_Gray_Mode(level)) {
			strout.insert(1,color,content,colorend,NULL);
		} else {
			strout.insert(1,content);
		}
		RollFile_StreamIn(strout.str(), strout.size());
	}
}

inline void __AdvLog_Print(int level, int color, const char *file, const char *func, const char *line, const char* levels) {
	pthread_mutex_lock(&mutex);
	__AdvLog_PrintContent(level, color, file, func, line, levels,strin.str());
	pthread_mutex_unlock(&mutex);
}

inline void __AdvLog_Write(int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels) {
	pthread_mutex_lock(&mutex);
	__AdvLog_WriteContent(level, color, colorend, file, func, line, levels,strin.str());
	pthread_mutex_unlock(&mutex);
}

void ADVLOG_CALL AdvLog_AssignContent(const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	strin.assign(fmt,ap);
	va_end(ap);
}

void ADVLOG_CALL AdvLog_Print(int level, int color, const char *file, const char *func, const char *line, const char* levels) {
	__AdvLog_Print(level, color, file, func, line, levels);
}

void ADVLOG_CALL AdvLog_Write(int level, const char *color, const char *colorend, const char *file, const char *func, const char *line, const char* levels) {
	__AdvLog_Write(level, color, colorend, file, func, line, levels);
}

void ADVLOG_CALL AdvLog_PrintAndWrite(int level, int color, const char *colorstr, const char *colorend, const char *file, const char *func, const char *line, const char* levels) {
	__AdvLog_Print(level, color, file, func, line, levels);
	__AdvLog_Write(level, colorstr, colorend, file, func, line, levels);
}

