#include "opendefs.h"
#include "opencoap.h"
#include "openqueue.h"
#include "openserial.h"
#include "openrandom.h"
#include "packetfunctions.h"
#include "idmanager.h"
#include "opentimers.h"
#include "scheduler.h"

//=========================== defines =========================================

//=========================== variables =======================================

opencoap_vars_t opencoap_vars;

//=========================== prototype =======================================

//=========================== public ==========================================

//===== from stack

/**
\brief Initialize this module.
*/
void opencoap_init() {
   // initialize the resource linked list
   opencoap_vars.resources     = NULL;
   
   // initialize the messageID
   opencoap_vars.messageID     = openrandom_get16b();

   // register at UDP stack
   opencoap_vars.desc.port              = WKP_UDP_COAP;
   opencoap_vars.desc.callbackReceive   = &opencoap_receive;
   opencoap_vars.desc.callbackSendDone  = opencoap_sendDone;
   openudp_register(&opencoap_vars.desc);
}

/**
\brief Indicate a CoAP messages was received.

A "CoAP message" is simply a UDP datagram received on the CoAP UDP port.

This function will call the appropriate resource, and send back its answer. The
received packetbuffer is reused to contain the response (or error code).

\param[in] msg The received CoAP message.
*/
void opencoap_receive(OpenQueueEntry_t* msg) {
   uint16_t                  temp_l4_destination_port;
   uint8_t                   i;
   uint8_t                   index;
   coap_option_t             last_option;
   coap_resource_desc_t*     temp_desc;
   bool                      found;
   owerror_t                 outcome = 0;
   coap_type_t               response_type;
   // local variables passed to the handlers (with msg)
   coap_header_iht           coap_header;
   coap_option_iht           coap_options[MAX_COAP_OPTIONS];
   
   // take ownership over the received packet
   msg->owner                = COMPONENT_OPENCOAP;
   
   //=== step 1. parse the packet
   
   // parse the CoAP header and remove from packet
   index = 0;
   coap_header.Ver           = (msg->payload[index] & 0xc0) >> 6;
   coap_header.T             = (coap_type_t)((msg->payload[index] & 0x30) >> 4);
   coap_header.TKL           = (msg->payload[index] & 0x0f);
   index++;
   coap_header.Code          = (coap_code_t)(msg->payload[index]);
   index++;
   coap_header.messageID     = msg->payload[index]*256+msg->payload[index+1];
   index+=2;
   
   // reject unsupported header
   if (coap_header.Ver!=COAP_VERSION || coap_header.TKL>COAP_MAX_TKL) {
      openserial_printError(
         COMPONENT_OPENCOAP,ERR_WRONG_TRAN_PROTOCOL,
         (errorparameter_t)0,
         (errorparameter_t)coap_header.Ver
      );
      openqueue_freePacketBuffer(msg);
      return;
   }
   
   // record the token
   memcpy(&coap_header.token[0], &msg->payload[index], coap_header.TKL);
   index += coap_header.TKL;
   
   // initialize the coap_options
   for (i=0;i<MAX_COAP_OPTIONS;i++) {
      coap_options[i].type = COAP_OPTION_NONE;
   }
   
   // find first URI Path
   uint8_t posURIPath=0;
   bool URIFound=FALSE;

   // fill in the coap_options
   last_option = COAP_OPTION_NONE;
   for (i=0;i<MAX_COAP_OPTIONS;i++) {
      
      // detect when done parsing options
      if (msg->payload[index]==COAP_PAYLOAD_MARKER) {
         // found the payload marker, done parsing options.
         index++; // skip marker and stop parsing options
         break;
      }
      if (msg->length<=index) {
         // end of message, no payload
         break;
      }
      
      // parse this option
      coap_options[i].type        = (coap_option_t)((uint8_t)last_option+(uint8_t)((msg->payload[index] & 0xf0) >> 4));
      last_option                 = coap_options[i].type;
      coap_options[i].length      = (msg->payload[index] & 0x0f);
      index++;
      coap_options[i].pValue      = &(msg->payload[index]);
      index                      += coap_options[i].length; //includes length as well

      if(URIFound==FALSE &&  coap_options[i].type == COAP_OPTION_NUM_URIPATH){
    	   posURIPath=i;
    	   URIFound=TRUE;
      }
   }
   
   // remove the CoAP header+options
   packetfunctions_tossHeader(msg,index);
   
   //=== step 2. find the resource to handle the packet
   
   // find the resource this applies to
   found = FALSE;
   
   if (
         coap_header.Code>=COAP_CODE_REQ_GET &&
         coap_header.Code<=COAP_CODE_REQ_DELETE
      ) {
      // this is a request: target resource is indicated as COAP_OPTION_LOCATIONPATH option(s)
      // find the resource which matches
      
      // start with the first resource in the linked list
      temp_desc = opencoap_vars.resources;

      // iterate until matching resource found, or no match
      while (found==FALSE) {
    	  if (           coap_options[posURIPath].type==COAP_OPTION_NUM_URIPATH    &&
    	                 coap_options[posURIPath+1].type==COAP_OPTION_NUM_URIPATH    &&
    	                 coap_options[posURIPath+2].type==COAP_OPTION_NUM_URIPATH){
    		  if(		 temp_desc->path0len>0                            &&
    	                 temp_desc->path0val!=NULL                        &&
    	                 temp_desc->path1len>0                            &&
    	                 temp_desc->path1val!=NULL                        &&
    	                 temp_desc->path2len>0                            &&
    	                 temp_desc->path2val!=NULL
    	              ) {
    	              // resource has a path of form path0/path1/path2

    	              if (
    	                    coap_options[posURIPath].length==temp_desc->path0len                               &&
    	                    memcmp(coap_options[posURIPath].pValue,temp_desc->path0val,temp_desc->path0len)==0 &&
    	                    coap_options[posURIPath+1].length==temp_desc->path1len                               &&
    	                    memcmp(coap_options[posURIPath+1].pValue,temp_desc->path1val,temp_desc->path1len)==0 &&
    	                    coap_options[posURIPath+2].length==temp_desc->path2len                               &&
    	                    memcmp(coap_options[posURIPath+2].pValue,temp_desc->path2val,temp_desc->path2len)==0
    	                 ) {
    	                 found = TRUE;
    	              };
    		  }
    	  } else if (
               coap_options[posURIPath].type==COAP_OPTION_NUM_URIPATH    &&
               coap_options[posURIPath+1].type==COAP_OPTION_NUM_URIPATH){
    		  if(  temp_desc->path0len>0                            &&
				   temp_desc->path0val!=NULL                        &&
				   temp_desc->path1len>0                            &&
				   temp_desc->path1val!=NULL						&&
				   temp_desc->path2len<=0
    		  ) {
				// resource has a path of form path0/path1

				if (
					  coap_options[posURIPath].length==temp_desc->path0len                               &&
					  memcmp(coap_options[posURIPath].pValue,temp_desc->path0val,temp_desc->path0len)==0 &&
					  coap_options[posURIPath+1].length==temp_desc->path1len                               &&
					  memcmp(coap_options[posURIPath+1].pValue,temp_desc->path1val,temp_desc->path1len)==0
				   ) {
				   found = TRUE;
				};
            }
         } else if ( coap_options[posURIPath].type==COAP_OPTION_NUM_URIPATH){

        	 if( 	temp_desc->path0len>0                            &&
        			 temp_desc->path0val!=NULL						&&
					 temp_desc->path1len<=0							&&
					 temp_desc->path2len<=0
        	 ) {
            // resource has a path of form path0
               
				if (
					  coap_options[posURIPath].length==temp_desc->path0len                               &&
					  memcmp(coap_options[posURIPath].pValue,temp_desc->path0val,temp_desc->path0len)==0
				   ) {
				   found = TRUE;
				};
            }
         };
         
         // iterate to next resource, if not found
         if (found==FALSE) {
            if (temp_desc->next!=NULL) {
               temp_desc = temp_desc->next;
            } else {
               break;
            }
         }
      }
   
   } else {
      // this is a response: target resource is indicated by token, and message ID
      // if an ack for a confirmable message, or a reset
      // find the resource which matches
      
      // start with the first resource in the linked list
      temp_desc = opencoap_vars.resources;
      // iterate until matching resource found, or no match
      while (found==FALSE) {

    	  if (
                coap_header.TKL==temp_desc->last_request.TKL                                       &&
                memcmp(&coap_header.token[0],&temp_desc->last_request.token[0],coap_header.TKL)==0
            ) {
            if (coap_header.T==COAP_TYPE_ACK || coap_header.T==COAP_TYPE_RES) {
                if (coap_header.messageID==temp_desc->last_request.messageID) {
                    found=TRUE;
                }
            } else {
                found=TRUE;
            }
            // call the resource's callback
            if (found==TRUE && temp_desc->callbackRx!=NULL) {
               temp_desc->callbackRx(msg,&coap_header,&coap_options[0]);
            }
         }
         
         // iterate to next resource, if not found
         if (found==FALSE) {
            if (temp_desc->next!=NULL) {
               temp_desc = temp_desc->next;
            } else {
               break;
            }
         }
      };
      // free the received packet
      openqueue_freePacketBuffer(msg);
      
      // stop here: will will not respond to a response
      return;
   }

   //=== step 3. ask the resource to prepare response
   
   if (found==TRUE) {
      // call the resource's callback
      outcome = temp_desc->callbackRx(msg,&coap_header,&coap_options[0]);
   } else {
      // reset packet payload (DO NOT DELETE, we will reuse same buffer for response)
      msg->payload                     = &(msg->packet[127]);
      msg->length                      = 0;
      // set the CoAP header
    //  coap_header.TKL                  = 0;	//A-K Changed.I think there was a bug
      coap_header.Code                 = COAP_CODE_RESP_NOTFOUND;
   }

   if (outcome==E_FAIL) {
      // reset packet payload (DO NOT DELETE, we will reuse same buffer for response)
      msg->payload                     = &(msg->packet[127]);
      msg->length                      = 0;
      // set the CoAP header
      coap_header.TKL                  = 0;
      coap_header.Code                 = COAP_CODE_RESP_METHODNOTALLOWED;
   }

   if (coap_header.T == COAP_TYPE_CON) {
       response_type = COAP_TYPE_ACK;
   } else {
       response_type = COAP_TYPE_NON;
   }
   
   //=== step 4. send that packet back
   
   // fill in packet metadata
   if (found==TRUE) {
      msg->creator                     = temp_desc->componentID;
   } else {
      msg->creator                     = COMPONENT_OPENCOAP;
   }
   msg->l4_protocol                    = IANA_UDP;
   temp_l4_destination_port            = msg->l4_destination_port;
   msg->l4_destination_port            = msg->l4_sourcePortORicmpv6Type;
   msg->l4_sourcePortORicmpv6Type      = temp_l4_destination_port;
   
   // set destination address as the current source
   msg->l3_destinationAdd.type         = ADDR_128B;
   memcpy(&msg->l3_destinationAdd.addr_128b[0],&msg->l3_sourceAdd.addr_128b[0],LENGTH_ADDR128b);
   
   // fill in CoAP header
   packetfunctions_reserveHeaderSize(msg,4+coap_header.TKL);
   msg->payload[0]                  = (COAP_VERSION    << 6) |
                                      (response_type   << 4) |
                                      (coap_header.TKL << 0);
   msg->payload[1]                  = coap_header.Code;
   msg->payload[2]                  = coap_header.messageID/256;
   msg->payload[3]                  = coap_header.messageID%256;
   memcpy(&msg->payload[4], &coap_header.token[0], coap_header.TKL);

   if ((openudp_send(msg))==E_FAIL) {
      openqueue_freePacketBuffer(msg);
   }
}

/**
\brief Indicates that the CoAP response has been sent.

\param[in] msg A pointer to the message which was sent.
\param[in] error The outcome of the send function.
*/
void opencoap_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   coap_resource_desc_t* temp_resource;
   // take ownership over that packet
   msg->owner = COMPONENT_OPENCOAP;
   
   // indicate sendDone to creator of that packet
   //=== mine
   if (msg->creator==COMPONENT_OPENCOAP) {
      openqueue_freePacketBuffer(msg);
      return;
   }
   //=== someone else's
   temp_resource = opencoap_vars.resources;
   while (temp_resource!=NULL) {
      if (
         temp_resource->componentID==msg->creator &&
         temp_resource->callbackSendDone!=NULL
         ) {
         temp_resource->callbackSendDone(msg,error);
         return;
      }
      temp_resource = temp_resource->next;
   }
   
   // if you get here, no valid creator was found
   
   openserial_printError(
      COMPONENT_OPENCOAP,ERR_UNEXPECTED_SENDDONE,
      (errorparameter_t)0,
      (errorparameter_t)0
   );
   openqueue_freePacketBuffer(msg);
}

//===== from CoAP resources

/**
\brief Writes the links to all the resources on this mote into the message.

\param[out] msg The messge to write the links to.
\param[in] componentID The componentID calling this function.

\post After this function returns, the msg contains 
*/
void opencoap_writeLinks(OpenQueueEntry_t* msg, uint8_t componentID) {
   coap_resource_desc_t* temp_resource;
   
   // start with the first resource in the linked list
   temp_resource = opencoap_vars.resources;
   
   bool is_write=FALSE;

   // iterate through all resources
   while (temp_resource!=NULL) {
      
      if (  
            (temp_resource->discoverable==TRUE) &&
            (
               ((componentID==COMPONENT_CWELLKNOWN) )
               || 
               ((componentID==temp_resource->componentID) && (temp_resource->path1len!=0))
            )
         ) {
          if(is_write!=FALSE){
              // write separator between links
                 packetfunctions_reserveHeaderSize(msg,1);
                 msg->payload[0] = ',';

          }
          else{
    	  is_write=TRUE;
          }
         // write ending '>'
         packetfunctions_reserveHeaderSize(msg,1);
         msg->payload[0] = '>';
         

         // write path2
         if (temp_resource->path2len>0) {
             packetfunctions_reserveHeaderSize(msg,temp_resource->path2len);
             memcpy(&msg->payload[0],temp_resource->path2val,temp_resource->path2len);
             packetfunctions_reserveHeaderSize(msg,1);
             msg->payload[0] = '/';
         }

         // write path1
         if (temp_resource->path1len>0) {
            packetfunctions_reserveHeaderSize(msg,temp_resource->path1len);
            memcpy(&msg->payload[0],temp_resource->path1val,temp_resource->path1len);
            packetfunctions_reserveHeaderSize(msg,1);
            msg->payload[0] = '/';
         }
         
         // write path0
         packetfunctions_reserveHeaderSize(msg,temp_resource->path0len);
         memcpy(msg->payload,temp_resource->path0val,temp_resource->path0len);
         packetfunctions_reserveHeaderSize(msg,2);
         msg->payload[1] = '/';
         
         // write opening '>'
         msg->payload[0] = '<';
         
         }

      // iterate to next resource
      temp_resource = temp_resource->next;
   }
}



//===== from CoAP resources

/**
\brief Writes the links with certain path number
*/
void opencoap_writeObjects(OpenQueueEntry_t* msg, uint8_t componentID) {
   coap_resource_desc_t* temp_resource;

   // start with the first resource in the linked list
   temp_resource = opencoap_vars.resources;

   // iterate through all resources
   while (temp_resource!=NULL) {

      if (
            (temp_resource->discoverable==TRUE) && (componentID==temp_resource->componentID)
         ) {
         // write path2
         if (temp_resource->path2len==0 && temp_resource->path1len==0) {
             // write ending '>'
             packetfunctions_reserveHeaderSize(msg,1);
             msg->payload[0] = '>';

             packetfunctions_reserveHeaderSize(msg,2);
             msg->payload[1] = '0';
             msg->payload[0] = '/';

             // write path0
             packetfunctions_reserveHeaderSize(msg,temp_resource->path0len);
             memcpy(msg->payload,temp_resource->path0val,temp_resource->path0len);
             packetfunctions_reserveHeaderSize(msg,2);
             msg->payload[1] = '/';

             // write opening '>'
             msg->payload[0] = '<';
             temp_resource=NULL;
         }
      }
      if(temp_resource!=NULL){
		  // iterate to next resource
		  temp_resource = temp_resource->next;
      }
   }
}


/**
\brief Register a new CoAP resource.

This function is called by a CoAP resource when it starts, allowing it to
receive data sent to that resource.

Registration consists in adding a new resource at the end of the linked list
of resources.

\param[in] desc The description of the CoAP resource.
*/
void opencoap_register(coap_resource_desc_t* desc) {
   coap_resource_desc_t* last_elem;
   
   // since this CoAP resource will be at the end of the list, its next element
   // should point to NULL, indicating the end of the linked list.
   desc->next = NULL;
   
   // if this is the first resource, simply have resources point to it
   if (opencoap_vars.resources==NULL) {
      opencoap_vars.resources = desc;
      return;
   }
   
   // if not, add to the end of the resource linked list
   last_elem = opencoap_vars.resources;
   while (last_elem->next!=NULL) {
      last_elem = last_elem->next;
   }
   last_elem->next = desc;
}

/**
\brief Send a CoAP request.

This function is called by a CoAP resource when it wants to send some data.
This function is NOT called for a response.

\param[in] msg The message to be sent. This messages should not contain the
   CoAP header.
\param[in] type The CoAP type of the message.
\param[in] code The CoAP code of the message.
\param[in] TKL  The Token Length of the message, sanitized to a max of COAP_MAX_TKL (8).
\param[out] descSender A pointer to the description of the calling CoAP
   resource.

\post After returning, this function will have written the messageID and TOKEN
   used in the descSender parameter.

\return The outcome of sending the packet.
*/
owerror_t opencoap_send(
      OpenQueueEntry_t*      msg,
      coap_type_t            type,
      coap_code_t            code,
      uint8_t                TKL,
      coap_resource_desc_t*  descSender
   ) {
   uint16_t token;
   uint8_t tokenPos=0;
   coap_header_iht* request;
   // increment the (global) messageID
   if (opencoap_vars.messageID++ == 0xffff) {
      opencoap_vars.messageID = 0;
   }

   // take ownership over the packet
   msg->owner                       = COMPONENT_OPENCOAP;
   
   // fill in packet metadata
   msg->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
   
   // update the last_request header
   request                          = &descSender->last_request;
   request->T                       = type;
   request->Code                    = code;
   request->messageID               = opencoap_vars.messageID;
   request->TKL                     = TKL<COAP_MAX_TKL ? TKL : COAP_MAX_TKL;

   while (tokenPos<request->TKL) {
       token = openrandom_get16b();
       memcpy(&request->token[tokenPos],&token,2);
       tokenPos+=2;
   }

   // pre-pend CoAP header (version,type,TKL,code,messageID,Token)
   packetfunctions_reserveHeaderSize(msg,4+request->TKL);
   msg->payload[0]                  = (COAP_VERSION   << 6) |
                                      (type           << 4) |
                                      (request->TKL   << 0);
   msg->payload[1]                  = code;
   msg->payload[2]                  = (request->messageID>>8) & 0xff;
   msg->payload[3]                  = (request->messageID>>0) & 0xff;

   memcpy(&msg->payload[4],&request->token[0],request->TKL); // A-K bugfix: it was only writing the first 2 bytes of token memcpy(&msg->payload[4],&token,request->TKL);
   return openudp_send(msg);
}

//=========================== private =========================================

//=========================== A-K =========================================

owerror_t opencoap_sendresponse(
      OpenQueueEntry_t*      msg,
      coap_type_t            type,
      coap_code_t            code,
      uint8_t                TKL,
      uint8_t*                token[]
   ) {
   // increment the (global) messageID
   if (opencoap_vars.messageID++ == 0xffff) {
      opencoap_vars.messageID = 0;
   }

   // take ownership over the packet
   msg->owner                       = COMPONENT_OPENCOAP;

   // fill in packet metadata
   msg->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
   // pre-pend CoAP header (version,type,TKL,code,messageID,Token)
   packetfunctions_reserveHeaderSize(msg,4+TKL);
   msg->payload[0]                  = (COAP_VERSION   << 6) |
                                      (type           << 4) |
                                      (TKL   << 0);
   msg->payload[1]                  = code;

   msg->payload[2]                  = (opencoap_vars.messageID>>8) & 0xff;
   msg->payload[3]                  = (opencoap_vars.messageID>>0) & 0xff;

   memcpy(&msg->payload[4],&token[0],TKL); // A-K bugfix: it was only writing the first 2 bytes of token memcpy(&msg->payload[4],&token,request->TKL);

   return openudp_send(msg);
}

uint8_t getOptionPosition(coap_option_iht*  coap_options, coap_option_t optionCode) {

	int i=0;
	for (i=0;i<MAX_COAP_OPTIONS;i++) {
		if( coap_options[i].type == optionCode){
			break;
		}
	}
	return i;
}

void processBlockWiseRequest(coap_option_iht*  coap_option, blockwise_var_t* blockwiseVar) {
	blockwiseVar->isBlock=1;
	if(coap_option[0].length == 3){
		openserial_printError(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)1, (errorparameter_t)254);
		return;
	}
	else if(coap_option[0].length == 2){
		blockwiseVar->NUM= ((coap_option->pValue[0]<<8)& 0xff00) | ((coap_option->pValue[1] >> 4)& 0x000f);
		blockwiseVar->M=((coap_option->pValue[1]>>3)& 0x01);
		blockwiseVar->SZX=(coap_option->pValue[1])& 0x07;
	}
	else if(coap_option[0].length == 1){
		blockwiseVar->NUM=((coap_option->pValue[0] >> 4)& 0x000f);
		blockwiseVar->M=((coap_option->pValue[0]>>3)& 0x01);
		blockwiseVar->SZX=(coap_option->pValue[0])& 0x07;
	}
	else if(coap_option[0].length == 0){
		blockwiseVar->NUM=0;
		blockwiseVar->M=0;
		blockwiseVar->SZX=0;
	}
	else{
		openserial_printError(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)1, (errorparameter_t)255);
	}

	return;
}

uint8_t calculateSZX(uint8_t blocksize) {

	if(blocksize==16){
		return 0;
	}
	else if(blocksize==32){
		return 1;
	}
	else if(blocksize==64){
		return 2;
	}
	else if(blocksize==128){
		openserial_printError(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)3, (errorparameter_t)128);
		return 3;
	}
	return 0;
}

void opencoap_addPayloadMarker(OpenQueueEntry_t* msg) {
	packetfunctions_reserveHeaderSize(msg,1);
	msg->payload[0]  = COAP_PAYLOAD_MARKER;
   return;
}

void opencoap_addBlockWiseOption(OpenQueueEntry_t* msg, blockwise_var_t* blockwiseVar, coap_option_t last_option) {
	if(blockwiseVar->NUM<16){
		packetfunctions_reserveHeaderSize(msg,2);
		msg->payload[0]     = ((COAP_OPTION_NUM_BLOCKWISE2 - last_option )<< 4) | 1;
		msg->payload[1]     = (blockwiseVar->NUM<<4 & 0x00f0)  | (blockwiseVar->M<<3 & 0x0008)   | (blockwiseVar->SZX & 0x0007);
	}
	else{
		packetfunctions_reserveHeaderSize(msg,3);
		msg->payload[0]     = ((COAP_OPTION_NUM_BLOCKWISE2 - last_option) << 4) | 2;
		msg->payload[1]     = (blockwiseVar->NUM>>8 & 0x00ff);
		msg->payload[2]     = (blockwiseVar->NUM<<4 & 0x00f0)  | (blockwiseVar->M<<3 & 0x0008)   | (blockwiseVar->SZX & 0x0007);
	}
   return;
}
