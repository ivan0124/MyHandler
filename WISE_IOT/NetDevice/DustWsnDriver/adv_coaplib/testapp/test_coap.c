#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "DustWsnDrv.h"
#include "adv_coap.h"
#include "adv_payload_parser.h"
#include "uri_cmd.h"

main()
{
	int i;
	adv_coap_packet_t *pdu, *input;
	adv_coap_packet_t *output;
	Sensor_List_t *sensor_list;
	Mote_Info_t moteinfo;
	int iret;

	// make get mote info
	printf(" ================================================\n");
	printf(" ===========Test make mote info packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_MOTE_INFO, URI_PARAM_ACTION_LIST, NULL );
	if( pdu == NULL ) {
		printf("make_adv_coap fail!\n");
		return -1;
	}
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);

	printf(" =========make return msg===========\n");
	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</dev>;n=\"Room1\", mdl=\"WISE1020\",sw=\"1.0.0.2\", reset=0", pdu->hdr.tid);
	if( input == NULL ) {
		printf("make_adv_retcoap fail!\n");
		return -1;
	}
	printf(" version: %d\n", input->hdr.version);
	printf(" type: %d\n", input->hdr.type);
	printf(" token_length: %d\n", input->hdr.token_length);
	printf(" code: %d\n", input->hdr.code);
	printf(" tid: %d\n", input->hdr.tid);
	printf(" length: %d\n", input->length);
	printf(" payload: %s\n", input->payload);

	if( (output = malloc(sizeof(adv_coap_packet_t))) == NULL ) {
		printf("malloc fail\n");
		return -1;
	}
	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	memset(&moteinfo, '\0', sizeof(Mote_Info_t));
	if((iret = adv_payloadParser_mote_info(output->payload, &moteinfo)) != 0) {
		printf("Error(%d): not recognized\n", iret);
	}
	printf(" hostname: %s\n", moteinfo.hostName);
	printf(" productname: %s\n", moteinfo.productName);
	printf(" software version: %s\n", moteinfo.softwareVersion);
	printf(" reset: %d\n", moteinfo.bReset);

	// make get net info
	printf(" ================================================\n");
	printf(" ===========Test make net info packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_NET_INFO, URI_PARAM_ACTION_LIST, NULL );
	if( pdu == NULL ) {
		printf("make_adv_coap fail!\n");
		return -1;
	}
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);

	printf(" =========make return msg===========\n");
	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</net>;n=\"dust\", sw=\"1.0.0.1\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("make_adv_retcoap fail!\n");
		return -1;
	}
	printf(" version: %d\n", input->hdr.version);
	printf(" type: %d\n", input->hdr.type);
	printf(" token_length: %d\n", input->hdr.token_length);
	printf(" code: %d\n", input->hdr.code);
	printf(" tid: %d\n", input->hdr.tid);
	printf(" length: %d\n", input->length);
	printf(" payload: %s\n", input->payload);

	if( (output = malloc(sizeof(adv_coap_packet_t))) == NULL ) {
		printf("malloc fail\n");
		return -1;
	}
	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	memset(&moteinfo, '\0', sizeof(Mote_Info_t));
	if((iret = adv_payloadParser_net_info(output->payload, &moteinfo)) != 0) {
		printf("Error(%d): not recognized\n", iret);
	}
	printf(" netname: %s\n", moteinfo.netName);
	printf(" netSWversion: %s\n", moteinfo.netSWversion);

	// make get sensors list
	printf(" ================================================\n");
	printf(" =========Test make sensor list packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO, URI_PARAM_ACTION_LIST, URI_PARAM_FIELD_ID );
	if( pdu == NULL ) {
		printf("make_adv_coap fail!\n");
		return -1;
	}

	printf("coap header size: %ld\n", sizeof(coap_header_t));
	printf("coap packet size: %ld\n", sizeof(adv_coap_packet_t));
	printf("coap:\n");
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);

	printf(" payload raw:");
	for(i=0; i<pdu->length; i++) {
		printf("%02x ", pdu->payload[i]);
	}
	printf("\n");

	printf(" =========make return msg===========\n");
	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</sen>;0,6,2,3,4,5", pdu->hdr.tid);
	//input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</sen>;0,1,2,3,4,5", pdu->hdr.tid);
	if( input == NULL ) {
		printf("make_adv_retcoap fail!\n");
		return -1;
	}

	printf(" version: %d\n", input->hdr.version);
	printf(" type: %d\n", input->hdr.type);
	printf(" token_length: %d\n", input->hdr.token_length);
	printf(" code: %d\n", input->hdr.code);
	printf(" tid: %d\n", input->hdr.tid);
	printf(" length: %d\n", input->length);
	printf(" payload: %s\n", input->payload);

	printf(" =========parse & check return packet===========\n");
	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if(pdu->hdr.tid != output->hdr.tid) {
		printf("Error message id\n");
		return -1;
	}

	printf(" version: %d\n", output->hdr.version);
	printf(" type: %d\n", output->hdr.type);
	printf(" token_length: %d\n", output->hdr.token_length);
	printf(" code: %d\n", output->hdr.code);
	printf(" tid: %d\n", output->hdr.tid);
	printf(" length: %d\n", output->length);
	printf(" payload: %s\n", output->payload);

	if( (sensor_list = malloc(sizeof(Sensor_List_t))) == NULL ) {
		printf("malloc fail\n");
		return -1;
	}
	memset(sensor_list, '\0', sizeof(Sensor_List_t));

	if( adv_payloadParser_sensor_list(output->payload, sensor_list) != 0) {
		printf("Error: not recognized\n");
	}
	printf("total sensors:%d\n", sensor_list->total);
#if 0
	for(i=0; i<sensor_list->total; i++) {
		printf("sensor#%d: idx=%d\n", i, sensor_list->item[i].index);
	}
#endif

	// make get sensors info
	printf(" ================================================\n");
	printf(" =========Test make sensor info packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO, URI_PARAM_ACTION_LIST, URI_PARAM_ID(0) );
	if( pdu == NULL ) {
		printf("make_adv_coap fail!\n");
		return -1;
	}
	printf("coap:\n");
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);

	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</0>;n=\"Board Tempature\", mdl=\"1\", sid=\"3303\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("make_adv_retcoap fail!\n");
		return -1;
	}

	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if( (iret=adv_payloadParser_sensor_info(output->payload, sensor_list)) < 0) {
		printf("%d Error(%d): not recognized\n", __LINE__,iret);
	}

	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</3>;n=\"Accelermeter\", mdl=\"mp6050\", sid=\"3313\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("make_adv_retcoap fail!\n");
		return -1;
	}

	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if( (iret=adv_payloadParser_sensor_info(output->payload, sensor_list)) < 0) {
		printf("%d Error(%d): not recognized\n", __LINE__,iret);
	}

	for(i=0; i<sensor_list->total; i++) {
		printf("sensor#%d: idx=%d, name=%s, objId=%s, unit=%s, format=%s, valueType=%s, resType=%s, min=%s, max=%s, model=%s, mode=%s \n", i,
               sensor_list->item[i].index,
               sensor_list->item[i].name,
               sensor_list->item[i].objId,
               sensor_list->item[i].unit,
               sensor_list->item[i].format,
               sensor_list->item[i].resType,
               sensor_list->item[i].valueType,
               sensor_list->item[i].minValue,
               sensor_list->item[i].maxValue,
               sensor_list->item[i].model,
               sensor_list->item[i].mode
              );
	}
	free(output);
	free(sensor_list);

	return 0;
}
