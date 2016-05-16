#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <getopt.h>
#include "platform.h"
#include "tool.h"
#include "configure.h"

#include "AdvJSON.h"

using namespace std;

typedef struct {
		//information
		int pid;
		int flush;
		char configureFile[256];
		char configureName[256];
		
		//configure
		int limit;
		char path[256];
		int staticLevel;
		int staticInfo;
		int staticGray;
		int dynamicLevel;
		int dynamicInfo;
		int dynamicGray;
		char hide[256];
} AdvLogConf;

AdvLogConf defaultconf = {
	-1,					//pid
	0,					//flush
	"",					//configureFile
	"",					//configureName
	102400,				//limit
	"./logs",			//path
	5,					//staticLevel
	1,					//staticInfo
	0,					//StaticGray
	5,					//dynamicLevel
	0,					//dynamicInfo
	0,					//dynamicGray
	""					//hide
};

void *mountKeyMemory(unsigned int sharedKey, int size) {
	char *ptr = NULL;
	//printf("sharedKey = 0x%08X\n",sharedKey);
	if (sharedKey == 0xFFFFFFFF) {
		printf("<%s,%s,%d> key:%08X failed\n", __FILE__, __func__, __LINE__, sharedKey);
		return (void *)ptr;
	}
	long shm_id;
	shm_id = shmget(sharedKey, size, 0666 | IPC_CREAT | IPC_EXCL);
	//printf("shm_id = %d\n",shm_id);
	if (shm_id == -1) {
		//printf("size = %d\n",size);
		shm_id = shmget(sharedKey, size, 0666);
		//printf("shm_id = %d\n",shm_id);
		if (shm_id == -1) {
			printf("<%s,%s,%d> key:%08X failed\n", __FILE__, __func__, __LINE__, sharedKey);
			return (void *)ptr;
		}
		else {
			ptr = (char *)shmat(shm_id, NULL, 0);
			return (void *)ptr;
		}
	}
	else {
		ptr = (char *)shmat(shm_id, NULL, 0);
		memset(ptr, 0, size);
		return (void *)ptr;
	}
	return (void *)ptr;
}

/*
void unmountKeyMemory(void *addr) {

#if defined(_WINDOWS)//windows
	BOOL fIgnore;
	char *ptr = (char *)addr - sizeof(HANDLE);
	HANDLE hMapObject = *(HANDLE *)ptr;

	fIgnore = UnmapViewOfFile(ptr);
	fIgnore = CloseHandle(hMapObject);
#elif defined(__linux)//linux

#endif

}
*/






AdvLogConf *configure = &defaultconf;
char *jsonbuffer = NULL;
AdvJSON jsonconf;



inline static AdvLogConf *AdvLog_Configure_Attach(int pid) {
	if(configure == &defaultconf) {
		int sharedKey = 0x20150707 - pid;
		configure = (AdvLogConf *)mountKeyMemory(sharedKey, sizeof(AdvLogConf));
		if(configure->pid != pid) {
			configure->pid = pid;
		}
	}
	return configure;
}


inline static void AdvLog_Configure_Default(AdvLogConf *conf) {
	strcpy(conf->path,"./logs");
	conf->staticLevel = 5;
	conf->staticInfo = 1;
	conf->staticGray = 0;
	conf->dynamicLevel = 5;
	conf->dynamicInfo = 0;
	conf->dynamicGray = 0;
	memset(conf->hide,0,sizeof(conf->hide));
	conf->limit = 102400;
	conf->flush = 0;
}

int AdvLog_Configure_Init(int pid) {
	AdvLogConf *conf = AdvLog_Configure_Attach(pid);
	if(conf != NULL) {
		AdvLog_Configure_Default(conf);
		return pid;
	} else return 0;
}

static void AdvLog_Configure_Assign_From_JSON(AdvLogConf *conf, AdvJSON json) {
	if(json.IsNULL()) return;
	
	string value;
	//json.Print();
	
	//printf("Assing: conf->configureName = %s\n", conf->configureName);
	
	value = json[conf->configureName]["path"].Value();
	if(value != "NULL") {
		strcpy(conf->path,value.c_str());
	}
	//printf("Assing: value = %s\n", value.c_str());
	//printf("Assing: conf->path = %s\n", conf->path);
	value = json[conf->configureName]["limit"].Value();
	if(value != "NULL") {
		conf->limit = atoi(value.c_str());
	}
	
	value = json[conf->configureName]["static"]["level"].Value();
	if(value != "NULL") {
		conf->staticLevel = atoi(value.c_str());
		if(conf->staticLevel < 0) {
			conf->staticGray = 1;
			conf->staticLevel = -conf->staticLevel;
		} else {
			conf->staticGray = 0;
		}
	}
	
	value = json[conf->configureName]["static"]["information"].Value();
	if(value != "NULL") {
		conf->staticInfo = atoi(value.c_str());
	}
	
	
	value = json[conf->configureName]["dynamic"]["level"].Value();
	if(value != "NULL") {
		conf->dynamicLevel = atoi(value.c_str());
		if(conf->dynamicLevel < 0) {
			conf->dynamicGray = 1;
			conf->dynamicLevel = -conf->dynamicLevel;
		} else {
			conf->dynamicGray = 0;
		}
	}
	
	value = json[conf->configureName]["dynamic"]["information"].Value();
	if(value != "NULL") {
		conf->dynamicInfo = atoi(value.c_str());
	}
	//printf("Assing: json[conf->configureName][dynamic][information].Value() = %s\n", json[conf->configureName]["dynamic"]["information"].Value().c_str());
	//printf("Assing: conf->dynamicInfo = %d\n", conf->dynamicInfo);
	
	value = json[conf->configureName]["dynamic"]["hide"].Value();
	if(value != "NULL") {
		strcpy(conf->hide,value.c_str());
	}
}

static void AdvLog_Configure_Export_To_File(AdvLogConf *conf, char *filename) {
	AdvJSON json("{}");
#if __cplusplus > 199711L
	AdvJSONCreator C(json);
	json.New()["default"][{"path","limit", "static", "dynamic"}] = {
		conf->path,
		conf->limit,
		C[{"level","information"}]({
			(conf->staticGray == 1 ? -conf->staticLevel : conf->staticLevel),
			conf->staticInfo
			}),
		C[{"level","information", "hide"}]({
			(conf->dynamicGray == 1 ? -conf->dynamicLevel : conf->dynamicLevel),
			conf->dynamicInfo,
			conf->hide
			}),
	};
#else
	json.New()["default"]["path"] = conf->path;
	json.New()["default"]["limit"] = conf->limit;
	json.New()["default"]["static"]["level"] = (conf->staticGray == 1 ? -conf->staticLevel : conf->staticLevel);
	json.New()["default"]["static"]["information"] = conf->staticInfo;
	json.New()["default"]["dynamic"]["level"] = (conf->dynamicGray == 1 ? -conf->dynamicLevel : conf->dynamicLevel);
	json.New()["default"]["dynamic"]["information"] = conf->dynamicInfo;
	json.New()["default"]["dynamic"]["hide"] = conf->hide;
#endif
	//json.Print();
	
	char buffer[4096] = {0};
	json.Print(buffer, sizeof(buffer),0);
	
	FILE *fp = fopen(filename,"w");
	if(fp == NULL) fp = fopen(filename,"r+");
	fprintf(fp,buffer);
	fclose(fp);
}

static void PrintHelp() {
	printf("Option -p [pid]: Set pid (must be the first parameter)\n");
	printf("\n");
	printf("Option -s [level]: Set STATIC level (default:5)(negative means gray mode)\n");
	printf("Option -i [0|1]: Enable STATIC info (default:1)\n");
	printf("\n");
	printf("Option -d [level]: Set DYNAMIC level (default:5)(negative means gray mode)\n");
	printf("Option -j [0|1]: Enable DYNAMIC info (default:0)\n");
	printf("Option -b {level string}: Hide DYNAMIC message (default:\"\", example:\"1,3,5\")\n");
	printf("\n");
	printf("Option -l [limit]: set file max size (unit: byte)(default:102400)\n");
	printf("Option -x {path}: set log files path (default:./logs)\n");
	printf("Option -f: import configure file\n");
	printf("Option -e: export configure file\n");
	printf("Option -n: assign the template name\n");
	printf("Option -v: Show all parameter\n");
	
}

int AdvLog_Configure_OptParser(int argc, char **argv, int pid) {
	int index;
	int c;
	AdvLogConf *conf = &defaultconf;
	if(argc <= 1) {
		PrintHelp();
	}
	
	if(pid != -1) {
		conf = AdvLog_Configure_Attach(pid);
	}
	
	while ((c = getopt (argc, argv, "p:s:i:d:j:b:l:x:vf:e:n:h?")) != -1) {
		switch (c)
		{
			case 'p':
				pid = atoi(optarg);
				conf = AdvLog_Configure_Attach(pid);
			break;
			case 's':
				if(pid > 0) {
					conf->staticLevel = atoi(optarg);
					if(conf->staticLevel < 0) {
						conf->staticGray = 1;
					} else {
						conf->staticGray = 0;
					}
					conf->staticLevel = ABS(conf->staticLevel);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'i':
				if(pid > 0) {
					conf->staticInfo = atoi(optarg);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'd':
				if(pid > 0) {
					conf->dynamicLevel = atoi(optarg);
					if(conf->dynamicLevel < 0) {
						conf->dynamicGray = 1;
					} else {
						conf->dynamicGray = 0;
					}
					conf->dynamicLevel = ABS(conf->dynamicLevel);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'j':
				if(pid > 0) {
					conf->dynamicInfo = atoi(optarg);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'b':
				if(pid > 0) {
					strncpy(conf->hide, optarg, sizeof(conf->hide));
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'l':
				if(pid > 0) {
					conf->limit = atoi(optarg);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'x':
				if(pid > 0) {
					strncpy(conf->path, optarg, sizeof(conf->path));
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'v':
				if(pid > 0) {
					printf("[x]Path: %s\n", conf->path);
					printf("[s]Static Level: %d\n", conf->staticGray == 0 ? conf->staticLevel : -conf->staticLevel);
					printf("[i]Static Info: %d\n", conf->staticInfo);
					printf("[d]Dynamic Level: %d\n", conf->dynamicGray == 0 ? conf->dynamicLevel : -conf->dynamicLevel);
					printf("[j]Dynamic Info: %d\n", conf->dynamicInfo);
					printf("[l]Limit: %d\n", conf->limit);
					printf("[f]Configure file: %s\n", conf->configureFile);
					printf("[n]Configure name: %s\n", conf->configureName);
					printf("[b]Hide: %s\n", conf->hide);
				} else {
					printf("Error: pid must be the first parameter.\n");
				}
			break;
			case 'f':
				{
					if(pid > 0) {
						strncpy(conf->configureFile,optarg,sizeof(conf->configureFile));
						if(jsonbuffer != NULL) free(jsonbuffer);
						jsonbuffer = fmalloc(conf->configureFile);
						if(jsonbuffer == NULL) {
							AdvLog_Configure_Export_To_File(conf, conf->configureFile);
							jsonbuffer = fmalloc(conf->configureFile);
						}
						jsonconf.Release();
						jsonconf = jsonbuffer;
						
						if(jsonbuffer != NULL) free(jsonbuffer);
						jsonbuffer = NULL;
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
				}
			break;
			case 'e':
				{
					if(pid > 0) {
						AdvLog_Configure_Export_To_File(conf, optarg);
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
				}
			break;
			case 'n':
				{
					if(pid > 0) {
						strncpy(conf->configureName,optarg,sizeof(conf->configureName));
						AdvLog_Configure_Assign_From_JSON(conf, jsonconf);
					} else {
						printf("Error: pid must be the first parameter.\n");
					}
				}
			break;
			case '?':
			case 'h':
				PrintHelp();
			return 1;
			default:
				return 1;
		}
	}

	for (index = optind; index < argc; index++)
		printf ("Non-option argument %s\n", argv[index]);
	return 0;
}

int AdvLog_Configure_GetPid() {
	return configure->pid;
}

int AdvLog_Configure_GetStaticLevel() {
	return configure->staticLevel;
}

int AdvLog_Configure_GetStaticGray() {
	return configure->staticGray;
}

int AdvLog_Configure_GetStaticInfo() {
	return configure->staticInfo;
}

int AdvLog_Configure_GetDynamicLevel() {
	return configure->dynamicLevel;
}

int AdvLog_Configure_GetDynamicGray() {
	return configure->dynamicGray;
}

int AdvLog_Configure_GetDynamicInfo() {
	return configure->dynamicInfo;
}
const char *AdvLog_Configure_GetPath() {
	return configure->path;
}
int AdvLog_Configure_GetLimit() {
	return configure->limit;
}
int AdvLog_Configure_Hide_Enable() {
	return configure->hide[0] == 0 ? 0 : 1;
}

int AdvLog_Configure_Is_Hiden(int level) {
	if(configure->hide[0] == 0) return 0;
	return strchr(configure->hide,level+48) == 0 ? 0 : 1;
}
