#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "AdvJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

void aTest(const char* mydata);

#ifdef __cplusplus
}
#endif

void aTest(const char* mydata){
    printf("call aTest\n");
    AdvJSON json(mydata);
    int i=0, j=0;
    int max=json[0].Size();

    char a[128]={"susiCommData"};
    max=json["susiCommData"]["infoSpec"]["IoTGW"].Size();
    for(i=0; i < max ;i++){
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%d]=%s\n",i,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
        char type[128]={0};
        char device[128]={0};
        //strcpy(type,"123");

        strcpy(type,json["susiCommData"]["infoSpec"]["IoTGW"][i].Key().c_str());
#if 0
        printf("json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s]=%s\n",\
        type,
        json["susiCommData"]["infoSpec"]["IoTGW"][type].Value().c_str());
#endif        
        int max_conn_type=json["susiCommData"]["infoSpec"]["IoTGW"][type].Size();
        for(j=0; j<max_conn_type ; j++){
            strcpy(device,json["susiCommData"]["infoSpec"]["IoTGW"][type][j].Key().c_str());
            int tmp_value[1024]={0};
            if ( strcmp("NULL",json["susiCommData"]["infoSpec"]["IoTGW"][type][device]["Info"].Value().c_str()) != 0){
                printf("@@@@ json[\"susiCommData\"][\"infoSpec\"][\"IoTGW\"][%s][%s]=%s\n",\
                type,device, \
                json["susiCommData"]["infoSpec"]["IoTGW"][type][device].Value().c_str());
            }
        }

    }
}
