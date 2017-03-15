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
#include "scheduler.h"

//=========================== defines =========================================

const uint8_t comi_path0[] = "c";
const uint8_t comi_cellist_path1[] = "s";



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
	comi_vars->comi_celllist.desc.path1len   = 0;
	comi_vars->comi_celllist.desc.path1val   = NULL;
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
            		uint8_t numOfCell=0;
            		uint8_t i;
            	    cellInfo_ht       cellList[MAXACTIVESLOTS];
            		numOfCell=sixtop_getAllCelllist(SCHEDULE_MINIMAL_6TISCH_DEFAULT_SLOTFRAME_HANDLE, &cellList);
					packetfunctions_reserveHeaderSize(msg,numOfCell*5+1);
					msg->payload[0]  = COAP_PAYLOAD_MARKER;
					  for(i=0;i<numOfCell;i++){
                	   openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)cellList[i].tsNum & 0x00ff,(errorparameter_t)0);
                	   openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)cellList[i].choffset & 0x00ff,(errorparameter_t)0);
                	   openserial_printInfo(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)cellList[i].linkoptions,(errorparameter_t)0);
					 //  memcpy(&msg->payload[0],&(cellList[i].tsNum),sizeof(cellList[i].tsNum)-1);
					 //  memcpy(&msg->payload[0],&(cellList[i].choffset),sizeof(cellList[i].choffset)-1);
					 //  memcpy(&msg->payload[0],&(cellList[i].linkoptions),sizeof(cellList[i].linkoptions)-1);
					   // add value
                	   msg->payload[5*i+1] = cellList[i].tsNum & 0x00ff;
					   msg->payload[5*i+2] = cellList[i].choffset & 0x00ff;
					   msg->payload[5*i+3] = cellList[i].linkoptions;
            		}
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
