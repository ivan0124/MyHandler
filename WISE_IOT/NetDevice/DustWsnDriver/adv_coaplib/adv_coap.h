/*
 * Copyright (c) 2015, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 */
#ifndef ADV_COAP_H
#define ADV_COAP_H

#define COAP_DEFAULT_VERSION  1 /* version of CoAP supported */

#define COAP_INVALID_TID -1

/* CoAP request methods
 * http://tools.ietf.org/html/rfc7252#section-12.1.1
 */
typedef enum {
	COAP_METHOD_GET = 1,
	COAP_METHOD_POST,
	COAP_METHOD_PUT,
	COAP_METHOD_DELETE
} coap_method_t;

/* CoAP message types
 * http://tools.ietf.org/html/rfc7252#section-12.1.1
 */
typedef enum {
	COAP_TYPE_CON = 0,
	COAP_TYPE_NONCON,
	COAP_TYPE_ACK,
	COAP_TYPE_RESET
} coap_msg_type;

/* CoAP response codes
 * http://tools.ietf.org/html/rfc7252#section-5.2
 * http://tools.ietf.org/html/rfc7252#section-12.1.2
 */
#define MAKE_RSPCODE(clas, det) ((clas << 5) | (det))
typedef enum {
	COAP_RSPCODE_CHANGED = MAKE_RSPCODE(2, 4),
	COAP_RSPCODE_CONTENT = MAKE_RSPCODE(2, 5),
	COAP_RSPCODE_BAD_REQUEST = MAKE_RSPCODE(4, 0),
	COAP_RSPCODE_UNAUTHORIZED = MAKE_RSPCODE(4, 1),
	COAP_RSPCODE_NOT_FOUND = MAKE_RSPCODE(4, 4),
	COAP_RSPCODE_METHOD_NOT_ALLOWED = MAKE_RSPCODE(4, 5),
	COAP_RSPCODE_NOT_IMPLEMENTED = MAKE_RSPCODE(5, 1)
} coap_responsecode_t;

/* CoAP content formats
 * http://tools.ietf.org/html/rfc7252#section-12.3
 */
typedef enum {
	COAP_CONTENTTYPE_NONE = -1, // bodge to allow us not to send option block
	COAP_CONTENTTYPE_TEXT_PLAIN = 0,
	COAP_CONTENTTYPE_APPLICATION_LINKFORMAT = 40,
} coap_content_type_t;

/* Advantech COAP packet format */
#define COAP_MAX_PAYLOAD  70
typedef struct {
	unsigned short version:2;   /* protocol version */
	unsigned short type:2;      /* type flag */
	unsigned short token_length:4;  /* length of Token */
	unsigned short code:8;      /* request method (value 1--10) or response code (value 40-255) */
	unsigned short tid;       /* message id (2 bytes)*/
} coap_header_t;



#if defined(WIN32) 

#pragma pack(push,1)
typedef struct {
	coap_header_t hdr;
	unsigned int length;    /* payload length (4 bytes) */
	unsigned char payload[COAP_MAX_PAYLOAD];      /* payload */
} adv_coap_packet_t;
#pragma pack(pop)

#elif defined(__linux)

typedef struct {
	coap_header_t hdr;
	unsigned int length;    /* payload length (4 bytes) */
	unsigned char payload[COAP_MAX_PAYLOAD];      /* payload */
} __attribute__ ((packed)) adv_coap_packet_t;

#endif
// =============================================== 
// Public functions
// =============================================== 

/*
 * This function check the CoAP format
 *
 * \param _pdata input: received data
 *
 * \param _ppdu  output: convert to Advantech format
 *
 */
int adv_coap_parse_pkt(unsigned char *_pdata, adv_coap_packet_t *_ppdu);

/*
 * This function makes the CoAP packet 
 *
 * \param _itype CoAP message types
 *
 * \param _icode CoAP request methods
 *
 * \param _pgroup URI group
 *
 * \param _paction URI action
 *
 * \param _pparam URI parameter
 *
 */
adv_coap_packet_t *adv_coap_make_pkt( int _itype, int _icode, unsigned char* _pgroup, unsigned char* _paction, unsigned char* _pparam );

/*
 * This function makes the CoAP return packet 
 *
 * \param _itype CoAP message types
 *
 * \param _icode CoAP request methods
 *
 * \param _pdata CoAP payload data
 *
 * \param _itid message id
 *
 */
adv_coap_packet_t *adv_coap_make_retpkt( int _itype, int _icode, unsigned char* _pdata, unsigned int _itid );

/*
 * This function delete the CoAP packet to free memory
 *
 * \param _ppdu CoAP packet
 *
 */
void adv_coap_delete_pkt(adv_coap_packet_t *_ppdu);

#endif // ADV_COAP_H
