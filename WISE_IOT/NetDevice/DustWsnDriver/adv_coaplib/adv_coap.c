/*
 * Copyright (c) 2015, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <arpa/inet.h>
#include "adv_coap.h"

#if defined(WIN32) //windows
#include "AdvPlatform.h"
#endif

//============================================
//   Local functions
//============================================

void coap_pdu_clear(adv_coap_packet_t *pdu) 
{
	assert(pdu);

	memset(pdu, 0, sizeof(adv_coap_packet_t));
	pdu->hdr.version = COAP_DEFAULT_VERSION;
	pdu->length = 0;
}

adv_coap_packet_t *coap_pdu_init(unsigned char type, unsigned char code,
          unsigned short id) 
{
	adv_coap_packet_t *pdu;

	pdu = malloc(sizeof(adv_coap_packet_t));
	if (pdu) {
		coap_pdu_clear(pdu);
		pdu->hdr.tid = id;
		pdu->hdr.type = type;
		pdu->hdr.code = code;
	}
	return pdu;
}

adv_coap_packet_t *coap_new_pdu(void) 
{
	adv_coap_packet_t *pdu;
	pdu = coap_pdu_init(0, 0, ntohs(COAP_INVALID_TID));
#ifdef DEBUG
	if (!pdu)
		print("coap_new_pdu: cannot allocate memory for new PDU\n");
#endif
	return pdu;
}

int coap_add_data(adv_coap_packet_t *pdu, unsigned int len, const unsigned char *data)
{
	unsigned char *cur;
	assert(pdu);

	if (len == 0)
		return 1;

	if (pdu->length + len + 1 > sizeof(pdu->payload)) {
		warn("coap_add_data: cannot add: data too large for PDU\n");
		return 0;
	}

	cur = pdu->payload;
	cur += pdu->length;
	memcpy(cur, data, len);
	pdu->length += len + 1;
	return 1;
}
#if 0
int coap_get_data(adv_coap_packet_t *pdu, size_t *len, unsigned char **data) 
{
	assert(pdu);
	assert(len);
	assert(data);

	if (pdu->length) {
		*len = pdu->length;
		*data = pdu->payload;
	} else {  /* no data, clear everything */
		*len = 0;
		*data = NULL;
	}

	return *data != NULL;
}
#endif

// =============================================== 
// Public functions
// =============================================== 

int adv_coap_parse_pkt(unsigned char *_pdata, adv_coap_packet_t *_ppdu)
{
	assert(_pdata);
	assert(_ppdu);

	memcpy(_ppdu, _pdata, sizeof(adv_coap_packet_t));

	if(_ppdu->hdr.version != COAP_DEFAULT_VERSION) {
        //printf("Error version!\n");
		return -1;
	}

	if(_ppdu->hdr.type != COAP_TYPE_ACK) {
        //printf("Error return type!\n");
        return -2;
    }

	if(_ppdu->hdr.code != COAP_RSPCODE_CONTENT && _ppdu->hdr.code != COAP_RSPCODE_CHANGED) {
        //printf("Error return code!\n");
        return -3;
    }

	return 0;
}

adv_coap_packet_t *adv_coap_make_pkt( int _itype, int _icode, unsigned char* _pgroup, unsigned char* _paction, unsigned char* _pparam )
{
	adv_coap_packet_t *pdu;
	static unsigned char buf[COAP_MAX_PAYLOAD];
	int len;
	unsigned char n;

	assert(_pgroup);
	assert(_paction);
	//assert(_pparam);

	if ( ! ( pdu = coap_new_pdu() ) )
		return NULL;

	pdu->hdr.type = _itype;
	pdu->hdr.code = _icode;
	pdu->hdr.tid = rand() & 0xFFFF;

	if(_pparam == NULL) {
		len = sprintf((char *)buf, "%s?%s", _pgroup, _paction);
		//n = '\0';
	} else {
		len = sprintf((char *)buf, "%s?%s&%s", _pgroup, _paction, _pparam);
		//n = '&';
	}
	
	//printf("%s: payload=%s\n", __func__,buf);
	if ( len > 0 ) {
		coap_add_data( pdu, len, buf );
	}
	return pdu;
}

adv_coap_packet_t *adv_coap_make_retpkt( int _itype, int _icode, unsigned char* _pdata, unsigned int _itid )
{
	adv_coap_packet_t *pdu;
	static unsigned char buf[COAP_MAX_PAYLOAD];
	int len;

	assert(_pdata);

	if ( ! ( pdu = coap_new_pdu() ) )
		return NULL;

	pdu->hdr.type = _itype;
	pdu->hdr.code = _icode;
	pdu->hdr.tid = _itid;

	len = sprintf((char *)buf, "%s", _pdata);
	//printf("payload length: %d\n", len);
	if ( len > 0 ) {
		coap_add_data( pdu, len, buf );
	}
	return pdu;
}

void adv_coap_delete_pkt(adv_coap_packet_t *_ppdu) 
{
	assert(_ppdu);
	free(_ppdu);
}
