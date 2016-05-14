#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h> 
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
	unsigned char coap_payload[32];
	int iret = 0;

	// make get sensors list
	printf(" ================================================\n");
	printf(" =========Test make sensor list packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO, URI_PARAM_ACTION_LIST, URI_PARAM_FIELD_ID );
	if( pdu == NULL ) {
		printf("adv_coap_make_pkt fail!\n");
		return -1;
	}

	printf("coap header size: %ld\n", sizeof(coap_header_t));
	printf("coap packet size: %ld\n", sizeof(adv_coap_packet_t));
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);

	printf(" =========make return msg===========\n");
	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</sen>;1,5,2,3,4,0", pdu->hdr.tid);
	//input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "</sen>;0,1,2,3,4,5", pdu->hdr.tid);
	if( input == NULL ) {
		printf("adv_coap_make_retpkt fail!\n");
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

	printf(" =========parse & check return packet===========\n");
	if(pdu->hdr.tid != output->hdr.tid) {
		printf("Error message id\n");
		return -1;
	}

	if(output->hdr.type != COAP_TYPE_ACK || output->hdr.code != COAP_RSPCODE_CONTENT) {
		printf("Error return type or code\n");
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
		printf("Error: header not found\n");
	}
	printf("total sensors:%d\n", sensor_list->total);
	adv_coap_delete_pkt(input);

	// make get sensors info
	printf(" ================================================\n");
	printf(" =========Test make sensor info packet===========\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON,COAP_METHOD_GET, URI_SENSOR_INFO, URI_PARAM_ACTION_LIST, URI_PARAM_ID(0) );
	if( pdu == NULL ) {
		printf("make_adv_coap fail!\n");
		return -1;
	}

	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "<0>;n=\"Board Tempature\", mdl=\"1\", sid=\"3303\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("adv_coap_make_retpkt fail!\n");
		return -1;
	}

	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if( adv_payloadParser_sensor_info(output->payload, sensor_list) != 0) {
		printf("Error: header not found\n");
	}

	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "<3>;n=\"Accelermeter\", mdl=\"1\", sid=\"3313\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("adv_coap_make_retpkt fail!\n");
		return -1;
	}

	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if( adv_payloadParser_sensor_info(output->payload, sensor_list) != 0) {
		printf("Error: header not found\n");
	}

	input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "<5>;n=\"Magnetic\", mdl=\"1\", sid=\"3314\"", pdu->hdr.tid);
	if( input == NULL ) {
		printf("adv_coap_make_retpkt fail!\n");
		return -1;
	}

	memset(output, '\0', sizeof(adv_coap_packet_t));

	if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
		printf("Parse coap fail!\n");
		return -1;
	}
	adv_coap_delete_pkt(input);

	if( adv_payloadParser_sensor_info(output->payload, sensor_list) != 0) {
		printf("Error: header not found\n");
	}

	// make observe
	printf(" ================================================\n");
	printf(" =========Test make observe packet===============\n");
	printf(" ================================================\n");
	pdu = adv_coap_make_pkt(COAP_TYPE_CON, COAP_METHOD_GET, URI_SENSOR_STATUS, URI_PARAM_ACTION_OBSERVE, URI_PARAM_ID(all) );
	if( pdu == NULL ) {
		printf("adv_coap_make_pkt fail!\n");
		return -1;
	}
	printf(" version: %d\n", pdu->hdr.version);
	printf(" type: %d\n", pdu->hdr.type);
	printf(" token_length: %d\n", pdu->hdr.token_length);
	printf(" code: %d\n", pdu->hdr.code);
	printf(" tid: %d\n", pdu->hdr.tid);
	printf(" length: %d\n", pdu->length);
	printf(" payload: %s\n", pdu->payload);



	while(1) {
		system("clear");
		time_t tm = time(0);
		printf("%s", ctime(&tm));
		i = rand();
		printf("%d,%d,%d,%d,%d %d %d\n", i&0xf, i>>2&0xf, i>>3&0xfff, i>>4&0xff, i>>5&0xff, i>>6&0xff, i>>7&0xff);
		sprintf(coap_payload, "0 36.%d,1 26.%d,2 %d,3 %d,4 0, 5 %d %d %d", i&0xf, i>>2&0xf, i>>3&0xfff, i>>4&0xff, i>>5&0xff, i>>6&0xff, i>>7&0xff);
		// first message type of observation is ACK, others is CON
		//input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, "0 36.2,1 26.5,2 9000,3 100,4 0, 5 10 20 30", pdu->hdr.tid);
		input = adv_coap_make_retpkt(COAP_TYPE_ACK, COAP_RSPCODE_CONTENT, coap_payload, pdu->hdr.tid);
		if( input == NULL ) {
			printf("adv_coap_make_retpkt fail!\n");
			return -1;
		}

		memset(output, '\0', sizeof(adv_coap_packet_t));

		if( adv_coap_parse_pkt((unsigned char*)input, output) != 0 ) {
			printf("Parse coap fail!\n");
			return -1;
		}
		adv_coap_delete_pkt(input);

		if((iret = adv_payloadParser_sensor_data(output->payload, sensor_list)) != 0) {
			printf("Error(%d): illegal data\n", iret );
		}

		for(i=0; i<sensor_list->total; i++) {
			printf("sensor#%d: idx=%d, name=%s, objId=%s, unit=%s, format=%s, valueType=%s, resType=%s, min=%s, max=%s, model=%s, mode=%s, value=%s \n", i,
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
               sensor_list->item[i].mode,
               sensor_list->item[i].value
              );
		}
		sleep(1);
	}
	free(output);
	free(sensor_list);

	return 0;
}
