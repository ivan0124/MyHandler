#include <stdio.h>
#include "AdvJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

void aTest();

#ifdef __cplusplus
}
#endif

void aTest(){
    printf("call aTest\n");
    AdvJSON jos("{\"ABC\":{}}");
}
