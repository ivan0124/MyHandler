#include <stdio.h>
#include "AdvJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

void aTestFunc();

#ifdef __cplusplus
}
#endif

void aTestFunc(){
    printf("call aTestFunc\n");
    AdvJSON jos("{\"ABC\":{}}");
}
