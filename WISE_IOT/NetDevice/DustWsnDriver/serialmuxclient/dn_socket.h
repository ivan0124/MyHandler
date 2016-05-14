/*
Copyright (c) 2015, Advantech Co. All rights reserved.

*/

#ifndef DN_SOCKET_H
#define DN_SOCKET_H

#include "dn_common.h"

//=========================== defined =========================================

//=========================== typedef =========================================

typedef void (*dn_socket_rxByte_cbt)(uint8_t byte);

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

void dn_socket_init(dn_socket_rxByte_cbt rxByte_cb);
void dn_socket_txByte(uint8_t byte);
void dn_socket_txFlush();

#ifdef __cplusplus
}
#endif

#endif
