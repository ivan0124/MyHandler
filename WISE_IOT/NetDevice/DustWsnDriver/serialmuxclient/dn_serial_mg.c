/*
Copyright (c) 2014, Dust Networks. All rights reserved.

HDLC library.

\license See attached DN_LICENSE.txt.
*/

#include <stdio.h>
#include "dn_serial_mg.h"

//=========================== variables =======================================

typedef struct {
   // admin
   uint8_t                   status;
   // packet IDs
   uint8_t                   txPacketId;
   bool                      rxPacketIdInit;
   uint8_t                   rxPacketId;
   // reply callback
   uint8_t                   replyCmdId;
   dn_serial_reply_cbt       replyCb;
   // callbacks
   dn_serial_request_cbt     requestCb;
   dn_status_change_cbt      statusChangeCb;
} dn_serial_mg_vars_t;

dn_serial_mg_vars_t dn_serial_mg_vars;

//=========================== prototype =======================================

dn_err_t dn_serial_sendRequestNoCheck(
   uint8_t              cmdId,
   bool                 isAck,
   bool                 shouldBeAcked,
   uint8_t*             payload,
   uint8_t              length,
   dn_serial_reply_cbt  replyCb
);
void     dn_serial_mg_rxSocketFrame(uint8_t* rxFrame, uint8_t rxFrameLen);
void     dn_serial_mg_dispatch_response(uint8_t cmdId, uint8_t *payload, uint8_t length);

//=========================== public ==========================================

/**
\brief Setting up the instance.
*/
void dn_serial_mg_init(dn_serial_request_cbt requestCb, dn_status_change_cbt statusChangeCb) 
{
	// reset local variables
	memset(&dn_serial_mg_vars, 0, sizeof(dn_serial_mg_vars));

	// initialize variables
	dn_serial_mg_vars.txPacketId      = 0x00;
	dn_serial_mg_vars.rxPacketIdInit  = FALSE;
	dn_serial_mg_vars.rxPacketId      = 0x00;

	dn_serial_mg_vars.requestCb       = requestCb;
	dn_serial_mg_vars.statusChangeCb  = statusChangeCb;

	// initialize the Socket module
	serialmux_init(dn_serial_mg_rxSocketFrame);
}

dn_err_t dn_serial_mg_initiateConnect()
{
	uint8_t payload[9];
   
	// prepare hello packet
	payload[0] = DN_API_VERSION;             // version
	payload[1] = 0x30; // authentication data
	payload[2] = 0x31;
	payload[3] = 0x32;
	payload[4] = 0x33;
	payload[5] = 0x34;
	payload[6] = 0x35;
	payload[7] = 0x36;
	payload[8] = 0x37;
   
	// send hello packet
	dn_serial_sendRequestNoCheck(
		SERIAL_CMID_HELLO,     // cmdId
		FALSE,                 // isAck
		FALSE,                 // shouldBeAcked
		payload,               // payload
		sizeof(payload),       // length
		NULL                   // replyCb
	);
   
	// remember state
	dn_serial_mg_vars.status = DN_SERIAL_ST_HELLO_SENT;
   
	return DN_ERR_NONE;
}

dn_err_t dn_serial_mg_sendRequest(uint8_t cmdId, bool isAck, uint8_t* payload, uint8_t length, dn_serial_reply_cbt replyCb) {
   // abort if not connected
   if (dn_serial_mg_vars.status!=DN_SERIAL_ST_CONNECTED) {
      return DN_ERR_NOT_CONNECTED;
   }
   
   // send the request
   return dn_serial_sendRequestNoCheck(
      cmdId,                 // cmdId
      isAck,                 // isAck
      !isAck,                // shouldBeAcked
      payload,               // payload
      length,                // length
      replyCb                // replyCb
   );
}

//=========================== private =========================================

dn_err_t dn_serial_sendRequestNoCheck(uint8_t cmdId, bool isAck, bool shouldBeAcked, uint8_t* payload, uint8_t length, dn_serial_reply_cbt replyCb)
{
	uint8_t i;

	//printf("send request no check (cmdId=%d)!\n", cmdId);
	// register reply callback
	dn_serial_mg_vars.replyCmdId      = cmdId;
	dn_serial_mg_vars.replyCb         = replyCb;

	// send packet over tcp
	if(serialmux_write(dn_serial_mg_vars.txPacketId, cmdId, length, payload) != 0)
		return -1;

	// increment the txPacketId
	dn_serial_mg_vars.txPacketId++;
   
	return DN_ERR_NONE;
}

void dn_serial_mg_rxSocketFrame(uint8_t* rxFrame, uint8_t rxFrameLen) 
{
	// fields in the API header
	uint8_t  control;
	uint8_t  cmdId;
	uint8_t  seqNum;
	uint8_t  length;
	uint8_t* payload;
	// misc
	uint8_t  isAck;
	uint8_t  shouldAck;
	uint8_t  isRepeatId = FALSE;
	uint8_t  headerToken[4] = {0xa7, 0x40, 0xa0, 0xf5};
   
	// assert length is OK
	if (rxFrameLen<4) {
		return;
	}

	// check header
	if(memcmp(rxFrame, headerToken, 4) != 0)
		return;

	// parse header
	cmdId      =  rxFrame[8];
	seqNum     =  rxFrame[6] << 4 | rxFrame[7];
	length     =  rxFrame[4] << 4 | rxFrame[5];

	if (cmdId != SERIAL_CMID_HELLO) {
		length     -= 3;  // 3 is size of (id+commandType)
	}

	payload    = &rxFrame[9];

	//printf("dn_serial_mg_rxSocketFrame cmdid = %d, seqNum = %d, length = %d \n", cmdId, seqNum, length);

	// ACK, dispatch
	if (length>0) {
		dn_serial_mg_dispatch_response(cmdId,payload,length);
	}
#if 0
	isAck      = ((control & DN_SERIAL_FLAG_ACK)!=0);
	shouldAck  = ((control & DN_SERIAL_FLAG_ACKNOWLEDGED)!=0);

	// check if valid packet ID
	if (isAck) {
		// ACK, dispatch
		if (length>0) {
			dn_serial_mg_dispatch_response(cmdId,payload,length);
		}
	} else {
		// DATA
		if (dn_serial_mg_vars.rxPacketIdInit==TRUE && seqNum==dn_serial_mg_vars.rxPacketId) {
			isRepeatId                         = TRUE;
		} else {
			isRepeatId                         = FALSE;
			dn_serial_mg_vars.rxPacketIdInit   = TRUE;
			dn_serial_mg_vars.rxPacketId       = seqNum;
		}

		// ACK
		if (shouldAck) {
			dn_hdlc_outputOpen();
         dn_hdlc_outputWrite(DN_SERIAL_FLAG_ACK | DN_SERIAL_FLAG_UNACKNOWLEDGED); // Control
         dn_hdlc_outputWrite(cmdId);                                              // Packet Type
         dn_hdlc_outputWrite(dn_serial_mg_vars.rxPacketId);                       // Seq. Number
         dn_hdlc_outputWrite(1);                                                  // Payload Length
         dn_hdlc_outputWrite(0);                                                  // Payload (RC==0x00)
			dn_hdlc_outputClose();
		}
#endif
      
      switch (cmdId) {
         //case SERIAL_CMID_HELLO_RESPONSE:
         case SERIAL_CMID_HELLO:
            if (
                  length>=5 &&
                  payload[HELLO_RESP_OFFS_RC]      == 0 &&
                  payload[HELLO_RESP_OFFS_VERSION] == DN_API_VERSION
               ) {
               // change state
               dn_serial_mg_vars.status = DN_SERIAL_ST_CONNECTED;
               // record manager sequence number
               dn_serial_mg_vars.rxPacketIdInit     = TRUE;
               dn_serial_mg_vars.rxPacketId         = payload[HELLO_RESP_OFFS_MGRSEQNO];
               // indicate state change
               if (dn_serial_mg_vars.statusChangeCb) {
                  dn_serial_mg_vars.statusChangeCb(dn_serial_mg_vars.status);
               }
            };
            break;
         case SERIAL_CMID_MGR_HELLO:
            if (
                  length>=2
               ) {
               // change state
               dn_serial_mg_vars.status = DN_SERIAL_ST_DISCONNECTED;
               // indicate state change
               if (dn_serial_mg_vars.statusChangeCb) {
                   dn_serial_mg_vars.statusChangeCb(dn_serial_mg_vars.status);
               }
            }
            break;
         default:
            // dispatch
            if (length>0 && dn_serial_mg_vars.requestCb!=NULL && isRepeatId==FALSE) {
               dn_serial_mg_vars.requestCb(cmdId,control,payload,length);
            }
            break;
      }
#if 0
   }
#endif
}

void dn_serial_mg_dispatch_response(uint8_t cmdId, uint8_t* payload, uint8_t length) {
   uint8_t rc;
   
   rc = payload[0];
   if (cmdId==dn_serial_mg_vars.replyCmdId && dn_serial_mg_vars.replyCb!=NULL) {
      
      // call the callback
      (dn_serial_mg_vars.replyCb)(cmdId,rc,&payload[1],length-1);
      
      // reset
      dn_serial_mg_vars.replyCmdId   = 0x00;
      dn_serial_mg_vars.replyCb      = NULL;
   }
}

//=========================== helpers =========================================
