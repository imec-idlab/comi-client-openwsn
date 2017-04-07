/**
\brief CoAP Management Interface Application.
\author Abdulkadir Karaagac <abdulkadir.karaagac@ugent.be>, February 2017.
 */

#include "comi.h"

#include "base64/base64.h"
#include "opendefs.h"
#include "idmanager.h"
#include "openqueue.h"


//=========================== defines =========================================

//=========================== variables =======================================

comi_vars_t comi_vars;
//=========================== prototypes ======================================

owerror_t comi_receive(
		OpenQueueEntry_t* msg,
		coap_header_iht*  coap_header,
		coap_option_iht*  coap_options
);
void    comi_sendDone(
		OpenQueueEntry_t* msg,
		owerror_t         error
);

//=========================== public ==========================================

void comi_init() {
	if(idmanager_getIsDAGroot()==TRUE) return;

	memset(&comi_vars,0,sizeof(comi_vars_t));

	// prepare the resource descriptor for the /c path
	comi_vars.desc.path0len            = sizeof(comi_path)-1;
	comi_vars.desc.path0val            = (uint8_t*)(&comi_path);
	comi_vars.desc.path1len            = 0;
	comi_vars.desc.path1val            = NULL;
	comi_vars.desc.path2len            = 0;
	comi_vars.desc.path2val            = NULL;
	comi_vars.desc.componentID         = COMPONENT_COMI;
	comi_vars.desc.discoverable        = TRUE;
	comi_vars.desc.callbackRx          = &comi_receive;
	comi_vars.desc.callbackSendDone    = &comi_sendDone;
	opencoap_register(&comi_vars.desc);
}

//=========================== private =========================================

owerror_t comi_receive(
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

			// have CoAP module write links to comi resources
			opencoap_writeLinks(msg,COMPONENT_COMI);

			packetfunctions_reserveHeaderSize(msg,1);
			msg->payload[0]     = COAP_PAYLOAD_MARKER;

			// add return option
			packetfunctions_reserveHeaderSize(msg,2);
			msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
			msg->payload[1]     = COAP_MEDTYPE_APPLINKFORMAT;

		}
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

uint8_t comi_getKey(coap_option_iht* coap_option, uint8_t* id) {

	if (coap_option[0].type == COAP_OPTION_NUM_URIQUERY && (coap_option[0].length>sizeof(querypref)-1) && (memcmp(&coap_option[0].pValue[0],&querypref,sizeof(querypref)-1)==0) ) {
			*id =str2uint8((char*)&coap_option[0].pValue[2],coap_option[0].length-2);
			return 1;
	}

	return 0;
}

uint8_t str2uint8 (const char* str, uint8_t len)
{
	uint8_t i;
	uint8_t ret = 0;
    for(i = 0; i < len; ++i)
    {
        ret = ret * 10 + (str[i] - '0');
    }
    return ret;
}


void comi_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
	openqueue_freePacketBuffer(msg);
}

