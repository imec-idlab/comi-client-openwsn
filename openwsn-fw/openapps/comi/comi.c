/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/

#include "comi.h"

#include "opendefs.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"

//=========================== defines =========================================

const uint8_t comi_path0[] = "c";

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
   
   // prepare the resource descriptor for the /c path
   comi_vars.desc.path0len            = sizeof(comi_path0)-1;
   comi_vars.desc.path0val            = (uint8_t*)(&comi_path0);
   comi_vars.desc.path1len            = 0;
   comi_vars.desc.path1val            = NULL;
   comi_vars.desc.componentID         = COMPONENT_COMI;
   comi_vars.desc.discoverable        = TRUE;
   comi_vars.desc.callbackRx          = &comi_receive;
   comi_vars.desc.callbackSendDone    = &comi_sendDone;
   
   opencoap_register(&comi_vars.desc);
}

//=========================== private =========================================

/**
\brief Receives a command and a list of items to be used by the command.

\param[in] msg          The received message. CoAP header and options already
   parsed.
\param[in] coap_header  The CoAP header contained in the message.
\param[in] coap_options The CoAP options contained in the message.

\return Whether the response is prepared successfully.
*/
owerror_t comi_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {
   
   owerror_t            outcome;
   open_addr_t          neighbor;
   bool                 foundNeighbor;
   
   switch (coap_header->Code) {
      
      case COAP_CODE_REQ_PUT:
         // add a slot
         
         // reset packet payload
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         // get preferred parent
         foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
         if (foundNeighbor==FALSE) {
            outcome                    = E_FAIL;
            coap_header->Code          = COAP_CODE_RESP_PRECONDFAILED;
            break;
         }
         
         if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
            // one sixtop transcation is happening, only one instance at one time
            
            // set the CoAP header
            coap_header->Code             = COAP_CODE_RESP_CHANGED;
           
            outcome                       = E_FAIL;
            break;
         }
         // call sixtop
         sixtop_request(
            IANA_6TOP_CMD_ADD,
            &neighbor,
            1
         );
         
         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_CHANGED;
         
         outcome                       = E_SUCCESS;
         break;
      
      case COAP_CODE_REQ_DELETE:
         // delete a slot
         
         // reset packet payload
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         
         // get preferred parent
         foundNeighbor = icmpv6rpl_getPreferredParentEui64(&neighbor);
         if (foundNeighbor==FALSE) {
            outcome                    = E_FAIL;
            coap_header->Code          = COAP_CODE_RESP_PRECONDFAILED;
            break;
         }
         
         if (sixtop_setHandler(SIX_HANDLER_SF0)==FALSE){
            // one sixtop transcation is happening, only one instance at one time
            
            // set the CoAP header
            coap_header->Code             = COAP_CODE_RESP_CHANGED;
           
            outcome                       = E_FAIL;
            break;
         }
         // call sixtop
         sixtop_request(
            IANA_6TOP_CMD_DELETE,
            &neighbor,
            1
         );
         
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

void comi_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
