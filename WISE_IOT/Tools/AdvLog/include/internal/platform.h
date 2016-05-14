#ifndef __PLATFORM_H__
#define __PLATFORM_H__

#if defined(WIN32) //windows

#include <AdvPlatform.h>

	#define _W_COLOR_NONE			0

	#define _W_COLOR_RED		FOREGROUND_INTENSITY | FOREGROUND_RED
	#define _W_COLOR_GREEN		FOREGROUND_INTENSITY | FOREGROUND_GREEN
	#define _W_COLOR_YELLOW		FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN
	#define _W_COLOR_BLUE		FOREGROUND_INTENSITY | FOREGROUND_BLUE
	#define _W_COLOR_PURPLE		FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE
	#define _W_COLOR_CYAN		FOREGROUND_INTENSITY | FOREGROUND_GREEN | FOREGROUND_BLUE
	#define _W_COLOR_GRAY		FOREGROUND_INTENSITY

	#define _W_COLOR_RED_BG		BACKGROUND_INTENSITY | BACKGROUND_RED
	#define _W_COLOR_GREEN_BG	BACKGROUND_INTENSITY | BACKGROUND_GREEN
	#define _W_COLOR_YELLOW_BG	BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_GREEN
	#define _W_COLOR_BLUE_BG	BACKGROUND_INTENSITY | BACKGROUND_BLUE
	#define _W_COLOR_PURPLE_BG	BACKGROUND_INTENSITY | BACKGROUND_RED | BACKGROUND_BLUE
	#define _W_COLOR_CYAN_BG	BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE
	#define _W_COLOR_WHITE_BG	BACKGROUND_INTENSITY | BACKGROUND_GREEN | BACKGROUND_BLUE | BACKGROUND_RED

#elif defined(__linux)//linux
	
#endif

#define __COLOR_RED			"\033[31m"
#define __COLOR_GREEN		"\033[32m"
#define __COLOR_YELLOW		"\033[33m"
#define __COLOR_BLUE		"\033[34;100m"
#define __COLOR_PURPLE		"\033[35m"
#define __COLOR_CYAN		"\033[36m"
#define __COLOR_GRAY		"\033[1;30m"

#define __COLOR_RED_BG		"\033[0;41m"
#define __COLOR_GREEN_BG	"\033[30;42m"
#define __COLOR_YELLOW_BG	"\033[30;43m"
#define __COLOR_BLUE_BG		"\033[0;44m"
#define __COLOR_PURPLE_BG	"\033[30;45m"
#define __COLOR_CYAN_BG		"\033[30;46m"
#define __COLOR_WHITE_BG	"\033[30;47m"

#define __COLOR_NONE 		"\033[0m"

#define ADV_HTML_CSS_STYLE	"<style type=\"text/css\">\
							  body { color: white; background-color: black}\
							  red { color: red; background-color: black }\
							  green { color: green; background-color: black }\
							  yellow { color: yellow; background-color: black }\
							  blue { color: #1569C7; background-color: black }\
							  purple { color: purple; background-color: black }\
							  cyan { color: cyan; background-color: black }\
							  gray { color: gray; background-color: black }\
							  redbg { color: white; background-color: red }\
							  greenbg { color: white; background-color: green }\
							  yellowbg { color: black; background-color: yellow }\
							  bluebg { color: white; background-color: blue }\
							  purplebg { color: white; background-color: purple }\
							  cyanbg { color: black; background-color: cyan }\
							  whitebg { color: black; background-color: white }\
							</style>"

#endif //__ADV_PLATFORM_H__
