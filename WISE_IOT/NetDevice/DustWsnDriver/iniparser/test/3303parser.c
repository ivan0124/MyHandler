#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "iniparser.h"

int  parse_ini_file(char * ini_name);

int main(int argc, char * argv[])
{
    int     status ;

    status = parse_ini_file(argv[1]);
    return status ;
}

int parse_ini_file(char * ini_name)
{
    dictionary  *   ini ;

    /* Some temporary variables to hold query results */
    int             b ;
    int             i ;
    double          d ;
    char        *   s ;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }
    iniparser_dump(ini, stderr);

    /* Get ml.1 attributes */
    printf("ml.1:\n");

    s = iniparser_getstring(ini, "ml.1:units", NULL);
    printf("Units        :  [%s]\n", s ? s : "UNDEF");
    s = iniparser_getstring(ini, "ml.1:rt", NULL);
    printf("rt           :  [%s]\n", s ? s : "UNDEF");
    
    i = iniparser_getint(ini, "ml.1:minrangevalue", -1);
    printf("MinRangeValue:  [%d]\n", i);
    i = iniparser_getint(ini, "ml.1:maxrangevalue", -1);
    printf("MaxRangeValue:  [%d]\n", i);

    iniparser_freedict(ini);
    return 0 ;
}


