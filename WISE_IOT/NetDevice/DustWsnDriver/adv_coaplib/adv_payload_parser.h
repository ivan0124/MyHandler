/*
 * Copyright (c) 2015, Advantech Co.,Ltd.
 * All rights reserved.
 * Authur: Chinchen-Lin <chinchen.lin@advantech.com.tw>
 */
#ifndef ADV_COAP_PAYLOAD_PARSER_H
#define ADV_COAP_PAYLOAD_PARSER_H

// =============================================== 
// Public functions
// =============================================== 

/*
 * This function parse the CoAP payload for sensor list
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _sensorlist Sensor list
 *
 */
int adv_payloadParser_sensor_list(unsigned char *_payload, Sensor_List_t *_sensorlist);

/*
 * This function parse the CoAP payload for sensor info
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _sensorlist Sensor list
 *
 */
int adv_payloadParser_sensor_info(unsigned char *_payload, Sensor_List_t *_sensorlist);

/*
 * This function parse the CoAP payload for sensor standard
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _sensorlist Sensor list
 *
 */
int adv_payloadParser_sensor_standard(unsigned char *_payload, Sensor_List_t *_sensorlist);

/*
 * This function parse the CoAP payload for sensor hardware
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _sensorlist Sensor list
 *
 */
int adv_payloadParser_sensor_hardware(unsigned char *_payload, Sensor_List_t *_sensorlist);

/*
 * This function parse the CoAP payload for sensor data
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _sensorlist Sensor list
 *
 */
int adv_payloadParser_sensor_data(unsigned char *_payload, Sensor_List_t *_sensorlist);

/*
 * This function parse the CoAP payload for mote info 
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _motelist Mote info structure
 *
 */
int adv_payloadParser_mote_info(unsigned char *_payload, Mote_Info_t *_moteinfo);

/*
 * This function parse the CoAP payload for net info 
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _motelist Mote info structure
 *
 */
int adv_payloadParser_net_info(unsigned char *_payload, Mote_Info_t *_moteinfo);

/*
 * This function parse the CoAP payload for command back data
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _motelist Mote info structure
 *
 */
int adv_payloadParser_sensor_cmdback(unsigned char *_payload, Mote_Info_t *_moteinfo);

/*
 * This function parse the CoAP payload for cancel observation back
 *
 * \param _payload Payload of CoAP packet
 *
 * \param _motelist Mote info structure
 *
 */
int adv_payloadParser_sensor_cancelback(unsigned char *_payload, Mote_Info_t *_moteinfo);

#endif // ADV_COAP_PAYLOAD_PARSER_H
