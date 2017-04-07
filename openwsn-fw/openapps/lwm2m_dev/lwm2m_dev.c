/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/

#include "lwm2m_dev.h"

#include "opendefs.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"
#include "scheduler.h"
#include "icmpv6rpl.h"
#include "IEEE802154E.h"


//=========================== defines =========================================

const 	uri_t URI_oma_device_object[] = "3";
static const 	uri_t OMA_instance_id[]="0";
static const 	uri_t URI_oma_res_manufacturer[] = "0";
static const 	uri_t URI_oma_res_model_number[] = "3";


//=========================== variables =======================================

lwm2m_device_t lwm2m_dev;

//=========================== prototypes ======================================




//=========================== public ==========================================

void lwm2m_dev_init() {
   if(idmanager_getIsDAGroot()==TRUE) return; 
   
   memset(&lwm2m_dev,0,sizeof(lwm2m_device_t));

   lwm2m_dev.desc.path0len   = sizeof(URI_oma_device_object)-1;
   lwm2m_dev.desc.path0val   = (uint8_t*)(&URI_oma_device_object);
   lwm2m_dev.desc.path1len   = 0;
   lwm2m_dev.desc.path1val   = NULL;
   lwm2m_dev.desc.path2len   = 0;
   lwm2m_dev.desc.path2val   = NULL;
   lwm2m_dev.desc.componentID      = COMPONENT_LWM2M_DEVICE;
   lwm2m_dev.desc.discoverable     = TRUE;
   lwm2m_dev.desc.callbackRx       = &lwm2m_device_receive;
   lwm2m_dev.desc.callbackSendDone = &lwm2m_dev_sendDone;
   opencoap_register(&lwm2m_dev.desc);

   lwm2m_device_register(&(lwm2m_dev.device_object));

}

//=========================== private =========================================

/**
   \brief
*/
void lwm2m_device_register(
		OMA_Device_Object* oma_device_object) {

	oma_device_object->desc.path0len   = sizeof(URI_oma_device_object)-1;
	oma_device_object->desc.path0val   = (uint8_t*)(&URI_oma_device_object);
	oma_device_object->desc.path1len   = sizeof(OMA_instance_id)-1;
	oma_device_object->desc.path1val   =(uint8_t*)(&OMA_instance_id);
	oma_device_object->desc.path2len   = 0;
	oma_device_object->desc.path2val   = NULL;
	oma_device_object->desc.componentID      = COMPONENT_LWM2M_DEVICE;
	oma_device_object->desc.discoverable     = TRUE;
	oma_device_object->desc.callbackRx       = &lwm2m_device_receive;
	oma_device_object->desc.callbackSendDone = &lwm2m_dev_sendDone;
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
	oma_device_object->manufacturer.desc.callbackSendDone = &lwm2m_dev_sendDone;
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
	oma_device_object->model_number.desc.callbackSendDone = &lwm2m_dev_sendDone;
	opencoap_register(&oma_device_object->model_number.desc);
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
            }else  if (coap_options[2].type != COAP_OPTION_NUM_URIPATH) {

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
							lwm2m_dev.device_object.manufacturer.desc.path2val ,
							lwm2m_dev.device_object.manufacturer.desc.path2len
									  )==0
									  ) {
						packetfunctions_reserveHeaderSize(msg,sizeof(manufacturerName));
						msg->payload[0] = COAP_PAYLOAD_MARKER;
						memcpy(&msg->payload[1],&manufacturerName,sizeof(manufacturerName)-1);


						openserial_printInfo(COMPONENT_LWM2M,ERR_AK_LWM2M,(errorparameter_t)0,(errorparameter_t)1);
					}
					else if (memcmp(coap_options[2].pValue,
							lwm2m_dev.device_object.model_number.desc.path2val ,
							lwm2m_dev.device_object.model_number.desc.path2len
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

void lwm2m_dev_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);

   // to be added: check reply if not successfull reschedule to closer time if it is reschedule a later time..

}
