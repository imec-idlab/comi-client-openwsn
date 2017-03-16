/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/

#include "lwm2m.h"

#include "opendefs.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"
#include "scheduler.h"
#include "icmpv6rpl.h"
#include "opensensors.h"
#include "sensors.h"
#include "leds.h"
#include "IEEE802154E.h"


//=========================== defines =========================================

const   uint8_t endpoint[] = "?ep=L1";
#define LESHANPERIOD  50000

const 	uri_t URI_oma_device_object[] = "3";
const	uri_t URI_ipso_temp_object[] = "3303";
const	uri_t URI_ipso_hum_object[] = "3304";
const	uri_t URI_ipso_led_object[] = "3311";

static const 	uri_t OMA_instance_id[]="0";
static const 	uri_t URI_oma_res_sensor_value[] = "5700";
static const 	uri_t URI_oma_res_sensor_units[] = "5701";
static const 	uri_t URI_oma_res_min_range_value[] = "5603";
static const 	uri_t URI_oma_res_max_range_value[] = "5604";
static const 	uri_t URI_oma_res_on_off_value[] = "5850";
static const 	uri_t URI_oma_res_manufacturer[] = "0";
static const 	uri_t URI_oma_res_model_number[] = "3";
static const 	uri_t URI_oma_rd[] = "rd";


static const uint8_t ipAddr_lwm2mserver[] = {0x20, 0x01, 0x06, 0xa8, 0x1d, 0x80, 0x11, 0x28, \
                                        0x95, 0xf7, 0xb7, 0x88, 0x2c, 0x00, 0xdd, 0x4e};
//static const uint8_t ipAddr_lwm2mserver[] = {0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, \
                                           0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};


//=========================== variables =======================================

lwm2m_server_t lwm2m_server;

//=========================== prototypes ======================================




//=========================== public ==========================================

void lwm2m_init() {
   if(idmanager_getIsDAGroot()==TRUE) return; 
   
   memset(&lwm2m_server,0,sizeof(lwm2m_server_t));
   int i;

   lwm2m_device_register(&(lwm2m_server.device_object));
 //  lwm2m_temp_register(&(lwm2m_server.temp_objects));
 //  lwm2m_hum_register(&(lwm2m_server.hum_objects));
   lwm2m_led_register(&(lwm2m_server.led_objects));

   opensensors_init();

   lwm2m_server.desc.path0len   = sizeof(URI_oma_rd)-1;
   lwm2m_server.desc.path0val   = (uint8_t*)(&URI_oma_rd);
   lwm2m_server.desc.path1len   = 0;
   lwm2m_server.desc.path1val   = NULL;
   lwm2m_server.desc.path2len   = 0;
   lwm2m_server.desc.path2val   = NULL;
   lwm2m_server.desc.componentID      = COMPONENT_LWM2M;
   lwm2m_server.desc.discoverable     = TRUE;

   lwm2m_server.leshan.timerId   = opentimers_start(LESHANPERIOD,
                                                TIMER_PERIODIC,TIME_MS,
												lwm2m_register_server_cb);
}

//=========================== private =========================================

/**
   \brief
*/
void lwm2m_device_register(
		OMA_Device_Object* oma_device_object) {

	oma_device_object->desc.path0len   = sizeof(URI_oma_device_object)-1;
	oma_device_object->desc.path0val   = (uint8_t*)(&URI_oma_device_object);
	oma_device_object->desc.path1len   = 0;
	oma_device_object->desc.path1val   = NULL;
	oma_device_object->desc.path2len   = 0;
	oma_device_object->desc.path2val   = NULL;
	oma_device_object->desc.componentID      = COMPONENT_LWM2M_DEVICE;
	oma_device_object->desc.discoverable     = TRUE;
	oma_device_object->desc.callbackRx       = &lwm2m_device_receive;
	oma_device_object->desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&oma_device_object->desc);

	oma_device_object->manufacturer.desc.path0len   = sizeof(URI_oma_device_object)-1;
	oma_device_object->manufacturer.desc.path0val   = (uint8_t*)(&URI_oma_device_object);
	oma_device_object->manufacturer.desc.path1len   = sizeof(OMA_instance_id)-1;
	oma_device_object->manufacturer.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	oma_device_object->manufacturer.desc.path2len   = sizeof(URI_oma_res_manufacturer)-1;
	oma_device_object->manufacturer.desc.path2val   = (uint8_t*)(&URI_oma_res_manufacturer);
	oma_device_object->manufacturer.desc.componentID      = COMPONENT_LWM2M_DEVICE;
	oma_device_object->manufacturer.desc.discoverable     = TRUE;
	oma_device_object->manufacturer.desc.callbackRx       = &lwm2m_device_receive;
	oma_device_object->manufacturer.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&oma_device_object->manufacturer.desc);

	oma_device_object->model_number.desc.path0len   = sizeof(URI_oma_device_object)-1;
	oma_device_object->model_number.desc.path0val   = (uint8_t*)(&URI_oma_device_object);
	oma_device_object->model_number.desc.path1len   = sizeof(OMA_instance_id)-1;
	oma_device_object->model_number.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	oma_device_object->model_number.desc.path2len   = sizeof(URI_oma_res_model_number)-1;
	oma_device_object->model_number.desc.path2val   = (uint8_t*)(&URI_oma_res_model_number);
	oma_device_object->model_number.desc.componentID      = COMPONENT_LWM2M_DEVICE;
	oma_device_object->model_number.desc.discoverable     = TRUE;
	oma_device_object->model_number.desc.callbackRx       = &lwm2m_device_receive;
	oma_device_object->model_number.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&oma_device_object->model_number.desc);
}

/**
   \brief
*/
void lwm2m_temp_register(
		IPSO_Temp_Object* ipso_temp_object) {

	ipso_temp_object->desc.path0len   = sizeof(URI_ipso_temp_object)-1;
	ipso_temp_object->desc.path0val   = (uint8_t*)(&URI_ipso_temp_object);
	ipso_temp_object->desc.path1len   = 0;
	ipso_temp_object->desc.path1val   = NULL;
	ipso_temp_object->desc.path2len   = 0;
	ipso_temp_object->desc.path2val   = NULL;
	ipso_temp_object->desc.componentID      = COMPONENT_LWM2M_TEMP;
	ipso_temp_object->desc.discoverable     = TRUE;
	ipso_temp_object->desc.callbackRx       = &lwm2m_temp_receive;
	ipso_temp_object->desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_temp_object->desc);

	ipso_temp_object->sensor_value.desc.path0len   = sizeof(URI_ipso_temp_object)-1;
	ipso_temp_object->sensor_value.desc.path0val   = (uint8_t*)(&URI_ipso_temp_object);
	ipso_temp_object->sensor_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_temp_object->sensor_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_temp_object->sensor_value.desc.path2len   = sizeof(URI_oma_res_sensor_value)-1;
	ipso_temp_object->sensor_value.desc.path2val   = (uint8_t*)(&URI_oma_res_sensor_value);
	ipso_temp_object->sensor_value.desc.componentID      = COMPONENT_LWM2M_TEMP;
	ipso_temp_object->sensor_value.desc.discoverable     = TRUE;
	ipso_temp_object->sensor_value.desc.callbackRx       = &lwm2m_temp_receive;
	ipso_temp_object->sensor_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_temp_object->sensor_value.desc);

	ipso_temp_object->sensor_units.desc.path0len   = sizeof(URI_ipso_temp_object)-1;
	ipso_temp_object->sensor_units.desc.path0val   = (uint8_t*)(&URI_ipso_temp_object);
	ipso_temp_object->sensor_units.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_temp_object->sensor_units.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_temp_object->sensor_units.desc.path2len   = sizeof(URI_oma_res_sensor_units)-1;
	ipso_temp_object->sensor_units.desc.path2val   = (uint8_t*)(&URI_oma_res_sensor_units);
	ipso_temp_object->sensor_units.desc.componentID      = COMPONENT_LWM2M_TEMP;
	ipso_temp_object->sensor_units.desc.discoverable     = TRUE;
	ipso_temp_object->sensor_units.desc.callbackRx       = &lwm2m_temp_receive;
	ipso_temp_object->sensor_units.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_temp_object->sensor_units.desc);

	ipso_temp_object->max_range_value.desc.path0len   = sizeof(URI_ipso_temp_object)-1;
	ipso_temp_object->max_range_value.desc.path0val   = (uint8_t*)(&URI_ipso_temp_object);
	ipso_temp_object->max_range_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_temp_object->max_range_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_temp_object->max_range_value.desc.path2len   = sizeof(URI_oma_res_max_range_value)-1;
	ipso_temp_object->max_range_value.desc.path2val   = (uint8_t*)(&URI_oma_res_max_range_value);
	ipso_temp_object->max_range_value.desc.componentID      = COMPONENT_LWM2M_TEMP;
	ipso_temp_object->max_range_value.desc.discoverable     = TRUE;
	ipso_temp_object->max_range_value.desc.callbackRx       = &lwm2m_temp_receive;
	ipso_temp_object->max_range_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_temp_object->max_range_value.desc);

	ipso_temp_object->min_range_value.desc.path0len   = sizeof(URI_ipso_temp_object)-1;
	ipso_temp_object->min_range_value.desc.path0val   = (uint8_t*)(&URI_ipso_temp_object);
	ipso_temp_object->min_range_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_temp_object->min_range_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_temp_object->min_range_value.desc.path2len   = sizeof(URI_oma_res_min_range_value)-1;
	ipso_temp_object->min_range_value.desc.path2val   = (uint8_t*)(&URI_oma_res_min_range_value);
	ipso_temp_object->min_range_value.desc.componentID      = COMPONENT_LWM2M_TEMP;
	ipso_temp_object->min_range_value.desc.discoverable     = TRUE;
	ipso_temp_object->min_range_value.desc.callbackRx       = &lwm2m_temp_receive;
	ipso_temp_object->min_range_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_temp_object->min_range_value.desc);
}

/**
   \brief
*/
void lwm2m_hum_register(
		IPSO_Hum_Object* ipso_object) {

	ipso_object->desc.path0len   = sizeof(URI_ipso_hum_object)-1;
	ipso_object->desc.path0val   = (uint8_t*)(&URI_ipso_hum_object);
	ipso_object->desc.path1len   = 0;
	ipso_object->desc.path1val   = NULL;
	ipso_object->desc.path2len   = 0;
	ipso_object->desc.path2val   = NULL;
	ipso_object->desc.componentID      = COMPONENT_LWM2M_HUM;
	ipso_object->desc.discoverable     = TRUE;
	ipso_object->desc.callbackRx       = &lwm2m_hum_receive;
	ipso_object->desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->desc);

	ipso_object->sensor_value.desc.path0len   = sizeof(URI_ipso_hum_object)-1;
	ipso_object->sensor_value.desc.path0val   = (uint8_t*)(&URI_ipso_hum_object);
	ipso_object->sensor_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_object->sensor_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_object->sensor_value.desc.path2len   = sizeof(URI_oma_res_sensor_value)-1;
	ipso_object->sensor_value.desc.path2val   = (uint8_t*)(&URI_oma_res_sensor_value);
	ipso_object->sensor_value.desc.componentID      = COMPONENT_LWM2M_HUM;
	ipso_object->sensor_value.desc.discoverable     = TRUE;
	ipso_object->sensor_value.desc.callbackRx       = &lwm2m_hum_receive;
	ipso_object->sensor_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->sensor_value.desc);

	ipso_object->sensor_units.desc.path0len   = sizeof(URI_ipso_hum_object)-1;
	ipso_object->sensor_units.desc.path0val   = (uint8_t*)(&URI_ipso_hum_object);
	ipso_object->sensor_units.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_object->sensor_units.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_object->sensor_units.desc.path2len   = sizeof(URI_oma_res_sensor_units)-1;
	ipso_object->sensor_units.desc.path2val   = (uint8_t*)(&URI_oma_res_sensor_units);
	ipso_object->sensor_units.desc.componentID      = COMPONENT_LWM2M_HUM;
	ipso_object->sensor_units.desc.discoverable     = TRUE;
	ipso_object->sensor_units.desc.callbackRx       = &lwm2m_hum_receive;
	ipso_object->sensor_units.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->sensor_units.desc);

	ipso_object->max_range_value.desc.path0len   = sizeof(URI_ipso_hum_object)-1;
	ipso_object->max_range_value.desc.path0val   = (uint8_t*)(&URI_ipso_hum_object);
	ipso_object->max_range_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_object->max_range_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_object->max_range_value.desc.path2len   = sizeof(URI_oma_res_max_range_value)-1;
	ipso_object->max_range_value.desc.path2val   = (uint8_t*)(&URI_oma_res_max_range_value);
	ipso_object->max_range_value.desc.componentID      = COMPONENT_LWM2M_HUM;
	ipso_object->max_range_value.desc.discoverable     = TRUE;
	ipso_object->max_range_value.desc.callbackRx       = &lwm2m_hum_receive;
	ipso_object->max_range_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->max_range_value.desc);

	ipso_object->min_range_value.desc.path0len   = sizeof(URI_ipso_hum_object)-1;
	ipso_object->min_range_value.desc.path0val   = (uint8_t*)(&URI_ipso_hum_object);
	ipso_object->min_range_value.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_object->min_range_value.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_object->min_range_value.desc.path2len   = sizeof(URI_oma_res_min_range_value)-1;
	ipso_object->min_range_value.desc.path2val   = (uint8_t*)(&URI_oma_res_min_range_value);
	ipso_object->min_range_value.desc.componentID      = COMPONENT_LWM2M_HUM;
	ipso_object->min_range_value.desc.discoverable     = TRUE;
	ipso_object->min_range_value.desc.callbackRx       = &lwm2m_hum_receive;
	ipso_object->min_range_value.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->min_range_value.desc);
}



/**
   \brief
*/
void lwm2m_led_register(
		IPSO_Light_Object* ipso_object) {

	ipso_object->desc.path0len   = sizeof(URI_ipso_led_object)-1;
	ipso_object->desc.path0val   = (uint8_t*)(&URI_ipso_led_object);
	ipso_object->desc.path1len   = 0;
	ipso_object->desc.path1val   = NULL;
	ipso_object->desc.path2len   = 0;
	ipso_object->desc.path2val   = NULL;
	ipso_object->desc.componentID      = COMPONENT_LWM2M_LED;
	ipso_object->desc.discoverable     = TRUE;
	ipso_object->desc.callbackRx       = &lwm2m_led_receive;
	ipso_object->desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->desc);

	ipso_object->on_off_res.desc.path0len   = sizeof(URI_ipso_led_object)-1;
	ipso_object->on_off_res.desc.path0val   = (uint8_t*)(&URI_ipso_led_object);
	ipso_object->on_off_res.desc.path1len   = sizeof(OMA_instance_id)-1;
	ipso_object->on_off_res.desc.path1val   = (uint8_t*)(&OMA_instance_id);
	ipso_object->on_off_res.desc.path2len   = sizeof(URI_oma_res_on_off_value)-1;
	ipso_object->on_off_res.desc.path2val   = (uint8_t*)(&URI_oma_res_on_off_value);
	ipso_object->on_off_res.desc.componentID      = COMPONENT_LWM2M_LED;
	ipso_object->on_off_res.desc.discoverable     = TRUE;
	ipso_object->on_off_res.desc.callbackRx       = &lwm2m_led_receive;
	ipso_object->on_off_res.desc.callbackSendDone = &lwm2m_sendDone;
	opencoap_register(&ipso_object->on_off_res.desc);
}



/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t lwm2m_device_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {
   
   owerror_t            outcome;
   
   switch (coap_header->Code) {

   	   case COAP_CODE_REQ_GET:
            // reset packet payload
            msg->payload                     = &(msg->packet[127]);
            msg->length                      = 0;
            if (coap_options[1].type != COAP_OPTION_NUM_URIPATH) {

                        // have CoAP module write links to lwm2m resources
                        opencoap_writeLinks(msg,COMPONENT_LWM2M_DEVICE);

                        packetfunctions_reserveHeaderSize(msg,1);
                        msg->payload[0]     = COAP_PAYLOAD_MARKER;

                        // add return option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
                        msg->payload[1]     = COAP_MEDTYPE_APPLINKFORMAT;
    					openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)0);
            }else{
            	if (memcmp(coap_options[1].pValue,
            							OMA_instance_id ,
										sizeof(OMA_instance_id)-1
            					                  )==0
            					                  ) {
					if (memcmp(coap_options[2].pValue,
							lwm2m_server.device_object.manufacturer.desc.path2val ,
							lwm2m_server.device_object.manufacturer.desc.path2len
									  )==0
									  ) {
						packetfunctions_reserveHeaderSize(msg,sizeof(manufacturerName));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&manufacturerName,sizeof(manufacturerName)-1);


						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)1);
					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_server.device_object.model_number.desc.path2val ,
							lwm2m_server.device_object.model_number.desc.path2len
									  )==0
									  ) {

						packetfunctions_reserveHeaderSize(msg,sizeof(infoStackName)+5);

						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&infoStackName,sizeof(infoStackName)-1);
								msg->payload[sizeof(infoStackName)-1+5-5] = '0'+OPENWSN_VERSION_MAJOR;
								msg->payload[sizeof(infoStackName)-1+5-4] = '.';
								msg->payload[sizeof(infoStackName)-1+5-3] = '0'+OPENWSN_VERSION_MINOR;
								msg->payload[sizeof(infoStackName)-1+5-2] = '.';
								msg->payload[sizeof(infoStackName)-1+5-1] = '0'+OPENWSN_VERSION_PATCH;

								openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)2);

					}
					else{
						uint8_t text_message[] 	= "Device Resource Not Supported";
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
					}
            	}
            	else{
            		uint8_t text_message[] 	= "Instance does not exist!";
            								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
            								msg->payload[0] = COAP_PAYLOAD_MARKER;
            								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
            	}
            }

            // content-type option
                                    packetfunctions_reserveHeaderSize(msg,2);
                                    msg->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
                                       1;
                                    msg->payload[1]                  = COAP_MEDTYPE_TEXTPLAIN;
            // set the CoAP header
            coap_header->Code                = COAP_CODE_RESP_CONTENT;
            outcome                          = E_SUCCESS;
            break;
         
      default:
         outcome = E_FAIL;
         break;
   }
   
   return outcome;
}

/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t lwm2m_temp_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {

   owerror_t            outcome;

   switch (coap_header->Code) {

   	   case COAP_CODE_REQ_GET:
            // reset packet payload
            msg->payload                     = &(msg->packet[127]);
            msg->length                      = 0;
            if (coap_options[1].type != COAP_OPTION_NUM_URIPATH) {

                        // have CoAP module write links to lwm2m resources
                        opencoap_writeLinks(msg,COMPONENT_LWM2M_TEMP);

                        packetfunctions_reserveHeaderSize(msg,1);
                        msg->payload[0]     = COAP_PAYLOAD_MARKER;

                        // add return option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
                        msg->payload[1]     = COAP_MEDTYPE_APPLINKFORMAT;
    					openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)0);
            }else{

            	if (memcmp(coap_options[1].pValue,
            	            							OMA_instance_id ,
            											sizeof(OMA_instance_id)-1
            	            					                  )==0
            	            					                  ) {

					if (memcmp(coap_options[2].pValue,
							lwm2m_server.temp_objects.sensor_value.desc.path2val ,
							lwm2m_server.temp_objects.sensor_value.desc.path2len
									  )==0
									  ) {


						uint16_t  value;
						char* outbuf;
						opensensors_resource_desc_t* opensensors_resource  = opensensors_getResource(0);
						value=opensensors_resource->callbackRead();
						int integer=value/1000;
						int	fraction=value-1000*((uint8_t)(value/1000));
						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)value,(errorparameter_t)0);
						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)integer,(errorparameter_t)fraction);
						char char_int[4];
						char char_frac[6];
					    itoa(integer,char_int,10);
					    itoa(fraction,char_frac,10);

						char value_text[12];
						strcpy(value_text, char_int);
						strcat(value_text, ".");
						if(fraction<100){
							strcat(value_text, "0");
						}
						if(fraction<10){
							strcat(value_text, "0");
						}
						strcat(value_text, char_frac);

					    uint8_t text_message[8];
					    memcpy(&text_message,&value_text,sizeof(text_message)-1);
						//uint8_t text_message[] 	= "23";
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],text_message,sizeof(text_message)-1);

						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)1);
					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_server.temp_objects.max_range_value.desc.path2val ,
							lwm2m_server.temp_objects.max_range_value.desc.path2len
									  )==0
									  ) {

								uint8_t text_message[] 	= "45.0";
								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
								msg->payload[0] = COAP_PAYLOAD_MARKER;
								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_server.temp_objects.min_range_value.desc.path2val ,
							lwm2m_server.temp_objects.min_range_value.desc.path2len
									  )==0
									  ) {

								uint8_t text_message[] 	= "18.0";
								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
								msg->payload[0] = COAP_PAYLOAD_MARKER;
								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else if (memcmp(coap_options[2].pValue,
												lwm2m_server.temp_objects.sensor_units.desc.path2val ,
												lwm2m_server.temp_objects.sensor_units.desc.path2len
														  )==0
														  ) {

													uint8_t text_message[] 	= "C";
													packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
													msg->payload[0] = COAP_PAYLOAD_MARKER;
													memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else{
						uint8_t text_message[] 	= "Resource not supported";
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
					}
            	}else{

            		uint8_t text_message[] 	= "Instance does not exist!";
            								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
            								msg->payload[0] = COAP_PAYLOAD_MARKER;
            								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
            	}
            }

            // content-type option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
                           1;
                        msg->payload[1]                  = COAP_MEDTYPE_TEXTPLAIN;
            // set the CoAP header
            coap_header->Code                = COAP_CODE_RESP_CONTENT;
            outcome                          = E_SUCCESS;
            break;

      default:
         outcome = E_FAIL;
         break;
   }

   return outcome;
}



/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t lwm2m_hum_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {

   owerror_t            outcome;

   switch (coap_header->Code) {

   	   case COAP_CODE_REQ_GET:
            // reset packet payload
            msg->payload                     = &(msg->packet[127]);
            msg->length                      = 0;
            if (coap_options[1].type != COAP_OPTION_NUM_URIPATH) {

                        // have CoAP module write links to lwm2m resources
                        opencoap_writeLinks(msg,COMPONENT_LWM2M_HUM);

                        packetfunctions_reserveHeaderSize(msg,1);
                        msg->payload[0]     = COAP_PAYLOAD_MARKER;

                        // add return option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
                        msg->payload[1]     = COAP_MEDTYPE_APPLINKFORMAT;
    					openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)0);
            }else{

            	if (memcmp(coap_options[1].pValue,
            	            							OMA_instance_id ,
            											sizeof(OMA_instance_id)-1
            	            					                  )==0
            	            					                  ) {

					if (memcmp(coap_options[2].pValue,
							lwm2m_server.hum_objects.sensor_value.desc.path2val ,
							lwm2m_server.hum_objects.sensor_value.desc.path2len
									  )==0
									  ) {


						uint16_t  value;
						char* outbuf;
						opensensors_resource_desc_t* opensensors_resource  = opensensors_getResource(1);
						value=opensensors_resource->callbackRead();
						int integer=value/1000;
						int	fraction=value-1000*((uint8_t)(value/1000));
						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)value,(errorparameter_t)0);
						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)integer,(errorparameter_t)fraction);
						char char_int[4];
						char char_frac[6];
						itoa(integer,char_int,10);
						itoa(fraction,char_frac,10);
						char value_text[12];
						strcpy(value_text, char_int);
						strcat(value_text, ".");
						if(fraction<100){
							strcat(value_text, "0");
						}
						if(fraction<10){
							strcat(value_text, "0");
						}
						strcat(value_text, char_frac);

						uint8_t text_message[8];
						memcpy(&text_message,&value_text,sizeof(text_message)-1);
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],text_message,sizeof(text_message)-1);

					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_server.hum_objects.max_range_value.desc.path2val ,
							lwm2m_server.hum_objects.max_range_value.desc.path2len
									  )==0
									  ) {

								uint8_t text_message[] 	= "100";
								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
								msg->payload[0] = COAP_PAYLOAD_MARKER;
								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_server.hum_objects.min_range_value.desc.path2val ,
							lwm2m_server.hum_objects.min_range_value.desc.path2len
									  )==0
									  ) {

								uint8_t text_message[] 	= "0";
								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
								msg->payload[0] = COAP_PAYLOAD_MARKER;
								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else if (memcmp(coap_options[2].pValue,
												lwm2m_server.hum_objects.sensor_units.desc.path2val ,
												lwm2m_server.hum_objects.sensor_units.desc.path2len
														  )==0
														  ) {

													uint8_t text_message[] 	= "Rh";
													packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
													msg->payload[0] = COAP_PAYLOAD_MARKER;
													memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);

					}
					else{
						uint8_t text_message[] 	= "Resource not supported";
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
					}
            	}else{

            		uint8_t text_message[] 	= "Instance does not exist!";
            								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
            								msg->payload[0] = COAP_PAYLOAD_MARKER;
            								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
            	}
            }

            // content-type option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
                           1;
                        msg->payload[1]                  = COAP_MEDTYPE_TEXTPLAIN;
            // set the CoAP header
            coap_header->Code                = COAP_CODE_RESP_CONTENT;
            outcome                          = E_SUCCESS;
            break;

      default:
         outcome = E_FAIL;
         break;
   }

   return outcome;
}




/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t lwm2m_led_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {

   owerror_t            outcome;
   switch (coap_header->Code) {

   	   case COAP_CODE_REQ_GET:
            // reset packet payload
            msg->payload                     = &(msg->packet[127]);
            msg->length                      = 0;
            if (coap_options[1].type != COAP_OPTION_NUM_URIPATH) {

                        // have CoAP module write links to lwm2m resources
                        opencoap_writeLinks(msg,COMPONENT_LWM2M_LED);

                        packetfunctions_reserveHeaderSize(msg,1);
                        msg->payload[0]     = COAP_PAYLOAD_MARKER;

                        // add return option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
                        msg->payload[1]     = COAP_MEDTYPE_APPLINKFORMAT;
            }else{

            	if (memcmp(coap_options[1].pValue,
            	            							OMA_instance_id ,
            											sizeof(OMA_instance_id)-1
            	            					                  )==0
            	            					                  ) {
					if (memcmp(coap_options[2].pValue,
							lwm2m_server.led_objects.on_off_res.desc.path2val ,
							lwm2m_server.led_objects.on_off_res.desc.path2len
									  )==0
									  ) {
						uint8_t  value=leds_error_isOn();
						if (value>0){
							uint8_t text_message[]="1";
							packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
							msg->payload[0] = COAP_PAYLOAD_MARKER;
							memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
						}
						else{
							uint8_t text_message[]="0";
							packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
							msg->payload[0] = COAP_PAYLOAD_MARKER;
							memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
						}
					}
					else{
						uint8_t text_message[] 	= "Resource not supported";
						packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
					}
            	}else{

            		uint8_t text_message[] 	= "Instance does not exist!";
            								packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
            								msg->payload[0] = COAP_PAYLOAD_MARKER;
            								memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
            	}
            }

            // content-type option
                        packetfunctions_reserveHeaderSize(msg,2);
                        msg->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
                           1;
                        msg->payload[1]                  = COAP_MEDTYPE_TEXTPLAIN;
            // set the CoAP header
            coap_header->Code                = COAP_CODE_RESP_CONTENT;
            outcome                          = E_SUCCESS;
            break;
   	   case COAP_CODE_REQ_PUT:

     	  if (coap_options[1].type == COAP_OPTION_NUM_URIPATH) {

     		  if (memcmp(coap_options[1].pValue,OMA_instance_id ,sizeof(OMA_instance_id)-1)==0) {
     	                  if ( memcmp(coap_options[2].pValue,lwm2m_server.led_objects.on_off_res.desc.path2val, lwm2m_server.led_objects.on_off_res.desc.path2len)==0
     	                          ) {

     								  uint8_t text[] 	= "0";
									  if (memcmp(&msg->payload[0],text,sizeof(text)-1)==0) {
										  leds_error_off();
									  }
									  else {
										  leds_error_on();
									  }
     	                  	  	  }
     	      }

 	  	  }else {

     	  }

          // reset packet payload
          msg->payload                  = &(msg->packet[127]);
          msg->length                   = 0;

          // set the CoAP header
          coap_header->Code             = COAP_CODE_RESP_CHANGED;

          outcome                       = E_SUCCESS;
   		   break;
      default:
         outcome = E_FAIL;
         break;
   }

   return outcome;
}



//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void lwm2m_register_server_cb(opentimer_id_t id){

	 if(idmanager_getIsDAGroot()==TRUE) return;

	 if ((ieee154e_isSynch()==FALSE) || (icmpv6rpl_getMyDAGrank()==DEFAULTDAGRANK)){
		 return;
	 }
		 scheduler_push_task(lwm2m_register_server,TASKPRIO_COAP);
		 scheduler_push_task(lwm2m_simple_register_rd,TASKPRIO_COAP);
}

void lwm2m_register_server() {
	openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)76,(errorparameter_t)0);
      OpenQueueEntry_t* pkt;
      owerror_t outcome;

      uint8_t numOptions;

      pkt = openqueue_getFreePacketBuffer(COMPONENT_LWM2M);
      if (pkt == NULL) {
          openserial_printError(COMPONENT_LWM2M,ERR_BUSY_SENDING,
                                (errorparameter_t)0,
                                (errorparameter_t)0);
          openqueue_freePacketBuffer(pkt);
          return;
      }

      pkt->creator   = COMPONENT_LWM2M;
      pkt->owner      = COMPONENT_LWM2M;
      pkt->l4_protocol  = IANA_UDP;


      //  // have CoAP module write links to lwm2m resources
      //opencoap_writeObjects(pkt,COMPONENT_LWM2M_TEMP);
     // packetfunctions_reserveHeaderSize(pkt,1);
    //  pkt->payload[0] = ',';
      //opencoap_writeObjects(pkt,COMPONENT_LWM2M_DEVICE);
      // write separator between links


     char id[]= "</3311/0>,</3/0>";
     packetfunctions_reserveHeaderSize(pkt,sizeof(id)-1);
     memcpy(&pkt->payload[0],&id,sizeof(id)-1);
     packetfunctions_reserveHeaderSize(pkt,1);
     pkt->payload[0]     = COAP_PAYLOAD_MARKER;

      numOptions=0;
      // query option
      packetfunctions_reserveHeaderSize(pkt,sizeof(endpoint)-1);
      memcpy(&pkt->payload[0],&endpoint,sizeof(endpoint)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIQUERY-COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
         ((sizeof(endpoint))-1);
      numOptions++;
      // content-type option
            packetfunctions_reserveHeaderSize(pkt,2);
            pkt->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT-COAP_OPTION_NUM_URIPATH) << 4 |
               1;
            pkt->payload[1]                  = COAP_MEDTYPE_TEXTPLAIN;
            numOptions++;

      // location-path option
      packetfunctions_reserveHeaderSize(pkt,sizeof(URI_oma_rd)-1);
      memcpy(&pkt->payload[0],&URI_oma_rd,sizeof(URI_oma_rd)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIPATH) << 4 |
         (sizeof(URI_oma_rd)-1);
      numOptions++;


      pkt->l4_destination_port   = WKP_UDP_COAP+2;
      pkt->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
      pkt->l3_destinationAdd.type = ADDR_128B;
      // set destination address here
      memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipAddr_lwm2mserver, 16);
      //send
      outcome = opencoap_send(
              pkt,
              COAP_TYPE_CON,
			  COAP_CODE_REQ_POST,
			  numOptions,
              &(lwm2m_server.desc)
              );

      if (outcome == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
      }
}

void lwm2m_simple_register_rd() {

	openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)76,(errorparameter_t)1);

      OpenQueueEntry_t* pkt;
      owerror_t outcome;

      uint8_t numOptions;

      pkt = openqueue_getFreePacketBuffer(COMPONENT_LWM2M);
      if (pkt == NULL) {
          openserial_printError(COMPONENT_LWM2M,ERR_BUSY_SENDING,
                                (errorparameter_t)0,
                                (errorparameter_t)0);
          openqueue_freePacketBuffer(pkt);
          return;
      }

      pkt->creator   = COMPONENT_LWM2M;
      pkt->owner      = COMPONENT_LWM2M;
      pkt->l4_protocol  = IANA_UDP;

      numOptions=0;

      uri_t URI_rd_wellknown[] = ".well-known";
      uri_t URI_rd_wellknown_core[] = "core";

      // query option
      packetfunctions_reserveHeaderSize(pkt,sizeof(endpoint)-1);
      memcpy(&pkt->payload[0],&endpoint,sizeof(endpoint)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIQUERY-COAP_OPTION_NUM_URIPATH) << 4 |
         ((sizeof(endpoint))-1);

      // location-path option
      packetfunctions_reserveHeaderSize(pkt,sizeof(URI_rd_wellknown_core)-1);
      memcpy(&pkt->payload[0],&URI_rd_wellknown_core,sizeof(URI_rd_wellknown_core)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIPATH-COAP_OPTION_NUM_URIPATH) << 4 |
         (sizeof(URI_rd_wellknown_core)-1);
      numOptions++;

      // location-path option
      packetfunctions_reserveHeaderSize(pkt,sizeof(URI_rd_wellknown)-1);
      memcpy(&pkt->payload[0],&URI_rd_wellknown,sizeof(URI_rd_wellknown)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIPATH) << 4 |
         (sizeof(URI_rd_wellknown)-1);
      numOptions++;

      pkt->l4_destination_port   = WKP_UDP_COAP;
      pkt->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
      pkt->l3_destinationAdd.type = ADDR_128B;
      // set destination address here
      memcpy(&pkt->l3_destinationAdd.addr_128b[0], &ipAddr_lwm2mserver, 16);
      //send
      outcome = opencoap_send(
              pkt,
              COAP_TYPE_CON,
			  COAP_CODE_REQ_POST,
			  numOptions,
              &(lwm2m_server.desc)
              );

      if (outcome == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
      }
}


void lwm2m_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
