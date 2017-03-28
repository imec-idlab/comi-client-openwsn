/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/

#include "comi.h"
#include "base64.h"
#include "opendefs.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"
#include "scheduler.h"
#include "schedule.h"

//=========================== defines =========================================

const uint8_t comi_path0[] = "c";
const uint8_t comi_cellist_path1[] = "+h"; //4001

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
   comi_vars.comi_celllist.schedule_vars=schedule_getSchedule_Vars();

    // prepare the resource descriptor for the /c path
   comi_vars.desc.path0len            = sizeof(comi_path0)-1;
   comi_vars.desc.path0val            = (uint8_t*)(&comi_path0);
   comi_vars.desc.path1len            = 0;
   comi_vars.desc.path1val            = NULL;
   comi_vars.desc.path2len            = 0;
   comi_vars.desc.path2val            = NULL;
   comi_vars.desc.componentID         = COMPONENT_COMI;
   comi_vars.desc.discoverable        = TRUE;
   comi_vars.desc.callbackRx          = &comi_receive;
   comi_vars.desc.callbackSendDone    = &comi_sendDone;
   opencoap_register(&comi_vars.desc);
   
   comi_resource_register(&comi_vars);
}

//=========================== private =========================================



/**
   \brief Register yang model resources into opencoap.
*/
void comi_resource_register(
		comi_vars_t* comi_vars
   ) {
	// Register Comi List Resource
	comi_vars->comi_celllist.desc.path0len   = sizeof(comi_path0)-1;
	comi_vars->comi_celllist.desc.path0val   = (uint8_t*)(&comi_path0);
	comi_vars->comi_celllist.desc.path1len   = sizeof(comi_cellist_path1)-1;
	comi_vars->comi_celllist.desc.path1val   = (uint8_t*)(&comi_cellist_path1);
	comi_vars->comi_celllist.desc.path2len   = 0;
	comi_vars->comi_celllist.desc.path2val   = NULL;
	comi_vars->comi_celllist.desc.componentID      = COMPONENT_COMI;
	comi_vars->comi_celllist.desc.discoverable     = TRUE;
	comi_vars->comi_celllist.desc.callbackRx       = &comi_receive;
	comi_vars->comi_celllist.desc.callbackSendDone = &comi_sendDone;

	   // register with the CoAP module
	opencoap_register(&comi_vars->comi_celllist.desc);
}

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

            }else{
            	if (memcmp(coap_options[1].pValue,
            		   comi_vars.comi_celllist.desc.path1val ,
					   comi_vars.comi_celllist.desc.path1len
				                  )==0
				                  ) {
            		uint8_t i;
            		uint16_t maxActiveSlots=schedule_getMaxActiveSlots();
					  for(i=0;i<maxActiveSlots;i++){
						  if(comi_vars.comi_celllist.schedule_vars->scheduleBuf[i].next==NULL){
							  i=maxActiveSlots;
						  }
						  else{
							  uint8_t tsnum=(uint8_t)comi_vars.comi_celllist.schedule_vars->scheduleBuf[i].slotOffset & 0x00ff;
							  uint8_t choffset=(uint8_t)comi_vars.comi_celllist.schedule_vars->scheduleBuf[i].channelOffset;
							  uint8_t addr=(uint8_t)comi_vars.comi_celllist.schedule_vars->scheduleBuf[i].neighbor.addr_64b[7];
							  uint8_t type=(uint8_t)comi_vars.comi_celllist.schedule_vars->scheduleBuf[i].type;

							//  openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)tsnum,(errorparameter_t)tsnum);
							//  openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)choffset,(errorparameter_t)tsnum);
							//  openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)addr,(errorparameter_t)tsnum);
							//  openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)type,(errorparameter_t)tsnum);

							  char tsnum_txt[2];
							  char choffset_txt[2];
							//  char linkoptions_txt[2];
							  char addr_txt[2];
							  char type_txt[1];

							  itoa(tsnum,tsnum_txt,10);
							  uint8_t tsnum_txt_size=1+(1<=tsnum/10)+(1<=tsnum/100);
							  itoa(choffset,choffset_txt,10);
							  uint8_t choffset_txt_size=1+(1<=choffset/10);
							  //itoa(linkoptions,linkoptions_txt,10);
							  itoa(addr,addr_txt,16);
							  uint8_t addr_txt_size=1+(1<=(addr>>4));
							  itoa(type,type_txt,10);


							 packetfunctions_reserveHeaderSize(msg,1);
							 msg->payload[0] = '\n';

							  packetfunctions_reserveHeaderSize(msg,choffset_txt_size);
							  memcpy(&msg->payload[0],&choffset_txt[0],choffset_txt_size);
							  packetfunctions_reserveHeaderSize(msg,1);
							  msg->payload[0] = ' ';
							  packetfunctions_reserveHeaderSize(msg,tsnum_txt_size);
							  memcpy(&msg->payload[0],&tsnum_txt[0],tsnum_txt_size);
							  packetfunctions_reserveHeaderSize(msg,1);
							  msg->payload[0] = ' ';
							  packetfunctions_reserveHeaderSize(msg,addr_txt_size);
							  memcpy(&msg->payload[0],&addr_txt[0],addr_txt_size);
							  packetfunctions_reserveHeaderSize(msg,1);
							  msg->payload[0] = ' ';
							  packetfunctions_reserveHeaderSize(msg,sizeof(type_txt));
							  memcpy(&msg->payload[0],&type_txt[0],sizeof(type_txt));
						  }
					  }
						packetfunctions_reserveHeaderSize(msg,1);
						msg->payload[0]  = COAP_PAYLOAD_MARKER;
            	}
            	else{
                    uint8_t text_message[] 	= "11";
                    packetfunctions_reserveHeaderSize(msg,sizeof(text_message));
                    msg->payload[0] = COAP_PAYLOAD_MARKER;
            	    memcpy(&msg->payload[1],&text_message,sizeof(text_message)-1);
            	}
            }
            // set the CoAP header
            coap_header->Code                = COAP_CODE_RESP_CONTENT;
            outcome                          = E_SUCCESS;
            break;

      case COAP_CODE_REQ_PUT:

    	  if (coap_options[1].type == COAP_OPTION_NUM_URIPATH) {
    	                  if ( 	     memcmp(
    	                             coap_options[1].pValue,
    								 comi_vars.comi_celllist.desc.path1val,
    								 comi_vars.comi_celllist.desc.path1len
    	                          )==0
    	                          ) {
    	                	  if (msg->payload[0]=='0') {
    	                	  	  	  sixtop_setHandler(SIX_HANDLER_NONE);
    	                	  }
    	                	  else if (msg->payload[0]=='1') {
    	                             //sixtop_setHandler(SIX_HANDLER_COMI);
    	                             schedule_remove_allTXandRX_Cells();
    	                      } else {
    	                             sixtop_setHandler(SIX_HANDLER_SF0);
    	                      }
    	                          break;
    	                      }
    	         } else {

    	         }


         // reset packet payload
         msg->payload                  = &(msg->packet[127]);
         msg->length                   = 0;
         


         // set the CoAP header
         coap_header->Code             = COAP_CODE_RESP_CHANGED;
         
         outcome                       = E_SUCCESS;
         break;
      
      case COAP_CODE_REQ_DELETE:
         // delete a slot
         
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



void comi_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}
