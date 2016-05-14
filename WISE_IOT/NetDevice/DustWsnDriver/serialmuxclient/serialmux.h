/*
Copyright (c) 2015, Advantech Co. All rights reserved.

serialmux library.

*/

#ifndef DN_SERIALMUX_H
#define DN_SERIALMUX_H

#include "dn_common.h"

//=========================== defines =========================================

//=========================== typedef =========================================

typedef void (*dn_hdlc_rxFrame_cbt)(uint8_t* rxFrame, uint8_t rxFrameLen);

//=========================== variables =======================================

//=========================== prototypes ======================================

#ifdef __cplusplus
 extern "C" {
#endif

void serialmux_init(dn_hdlc_rxFrame_cbt rxFrame_cb);
// output
int serialmux_open(unsigned char *_ipaddr, unsigned int _iportnum);
int	serialmux_write(uint8_t _itxPacketId, uint8_t _icmdId, uint8_t _ilength, uint8_t *payload);
int serialmux_close();

#ifdef __cplusplus
}
#endif

#endif
