#include <stdio.h>
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
    AdvJSON jos(mydata);
    printf("---------------------Json[0].Size=%d\n",jos[0].Size());
    printf("---------------------Json[0].Value().c_str()=%s\n",jos[0].Value().c_str());
}
