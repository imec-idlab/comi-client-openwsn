/**
\brief CoAP RD registration applications

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.
*/

#include "coap_rd.h"
#include "opendefs.h"

#include "scheduler.h"
#include "neighbors.h"
#include "idmanager.h"
#include "openqueue.h"

//=========================== defines =========================================

#ifndef COAP_DEVICE_TYPE
#define COAP_DEVICE_TYPE 2
#endif
#define TO_HEX(i) (i <= 9 ? '0' + i : 'A' - 10 + i)

static const 	uri_t URI_oma_rd[] = "rd";
static const uint8_t ipAddr_rdserver[] = {0x20, 0x01, 0x06, 0xa8, 0x1d, 0x80, 0x11, 0x28, \
                                        0x02, 0x0d, 0xb9, 0xff, 0xfe, 0x40, 0x31, 0x14};

//=========================== variables =======================================

coap_rd_server_t coap_rd_server;
uint8_t endpoint[6]="ep=  ";
//=========================== prototypes ======================================


//=========================== public ==========================================

void coap_rd_init() {
   if(idmanager_getIsDAGroot()==TRUE) return; 

	uint8_t x = idmanager_getMyID(ADDR_64B)->addr_64b[7];
	endpoint[3] = TO_HEX(((x & 0xF0) >> 4));
	endpoint[4] = TO_HEX(((x & 0x0F) >> 0));

   memset(&coap_rd_server,0,sizeof(coap_rd_server_t));
   memcpy(&coap_rd_server.ipv6address[0], &ipAddr_rdserver, 16);

   coap_rd_server.desc.path0len   = sizeof(URI_oma_rd)-1;
   coap_rd_server.desc.path0val   = (uint8_t*)(&URI_oma_rd);
   coap_rd_server.desc.path1len   = 0;
   coap_rd_server.desc.path1val   = NULL;
   coap_rd_server.desc.path2len   = 0;
   coap_rd_server.desc.path2val   = NULL;
   coap_rd_server.desc.componentID      = COMPONENT_COAP_RD;
   coap_rd_server.desc.discoverable     = FALSE;
   coap_rd_server.desc.callbackRx       = &coap_rd_receive;
   coap_rd_server.desc.callbackSendDone = &coap_rd_sendDone;
	opencoap_register(&coap_rd_server.desc);


   coap_rd_server.period=RDPERIOD;
   coap_rd_server.timerId   = opentimers_start(coap_rd_server.period,
                                                TIMER_PERIODIC,TIME_MS,
												coap_register_server_cb);
}

owerror_t coap_rd_receive(
      OpenQueueEntry_t* msg,
      coap_header_iht*  coap_header,
      coap_option_iht*  coap_options
   ) {

   owerror_t            outcome;
   switch (coap_header->Code) {

   	   case COAP_CODE_RESP_CREATED:
   		   opentimers_stop(coap_rd_server.timerId );

           break;

      default:
         outcome = E_FAIL;
         break;
   }

   return outcome;
}

//=========================== private =========================================

//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void coap_register_server_cb(opentimer_id_t id){

	 if(idmanager_getIsDAGroot()==TRUE) return;



	 if ((ieee154e_isSynch()==FALSE) || (icmpv6rpl_getMyDAGrank()==DEFAULTDAGRANK)){
		 return;
	 }
		 scheduler_push_task(coap_long_register_rd,TASKPRIO_COAP);
}


void coap_simple_register_rd() {

      OpenQueueEntry_t* pkt;
      owerror_t outcome;

      uint8_t numOptions;

      pkt = openqueue_getFreePacketBuffer(COMPONENT_COAP_RD);
      if (pkt == NULL) {
          openserial_printError(COMPONENT_COAP_RD,ERR_BUSY_SENDING,
                                (errorparameter_t)0,
                                (errorparameter_t)0);
          openqueue_freePacketBuffer(pkt);
          return;
      }

      pkt->creator   = COMPONENT_COAP_RD;
      pkt->owner      = COMPONENT_COAP_RD;
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
      memcpy(&pkt->l3_destinationAdd.addr_128b[0], &coap_rd_server.ipv6address[0], 16);
      //send
      outcome = opencoap_send(
              pkt,
              COAP_TYPE_CON,
			  COAP_CODE_REQ_POST,
			  numOptions,
              &(coap_rd_server.desc)
              );

      if (outcome == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
      }

}


void coap_long_register_rd() {

      OpenQueueEntry_t* pkt;
      owerror_t outcome;

      uint8_t numOptions;

      pkt = openqueue_getFreePacketBuffer(COMPONENT_COAP_RD);
      if (pkt == NULL) {
          openserial_printError(COMPONENT_COAP_RD,ERR_BUSY_SENDING,
                                (errorparameter_t)0,
                                (errorparameter_t)0);
          openqueue_freePacketBuffer(pkt);
          return;
      }

      pkt->creator   = COMPONENT_COAP_RD;
      pkt->owner      = COMPONENT_COAP_RD;
      pkt->l4_protocol  = IANA_UDP;


      uint8_t myPayload[] = "</c>;rt=\"core.c\"";
      packetfunctions_reserveHeaderSize(pkt,sizeof(myPayload)-1);
      memcpy(&pkt->payload[0],&myPayload,sizeof(myPayload)-1);

      opencoap_addPayloadMarker(pkt);

      numOptions=0;
//      uint8_t limetime[] = "lt=86400";
//      // query option
//      packetfunctions_reserveHeaderSize(pkt,sizeof(limetime)-1);
//      memcpy(&pkt->payload[0],&limetime,sizeof(limetime)-1);
//      packetfunctions_reserveHeaderSize(pkt,1);
//      pkt->payload[0] = (COAP_OPTION_NUM_URIQUERY-COAP_OPTION_NUM_URIQUERY) << 4 |
//    		  ((sizeof(limetime))-1);

      // query option
//      packetfunctions_reserveHeaderSize(pkt,sizeof(device_domain)-1);
//      memcpy(&pkt->payload[0],&device_domain,sizeof(device_domain)-1);
//      packetfunctions_reserveHeaderSize(pkt,1);
//      pkt->payload[0] = (COAP_OPTION_NUM_URIQUERY-COAP_OPTION_NUM_URIQUERY) << 4 |
//               ((sizeof(device_domain))-1);

      // query option
      packetfunctions_reserveHeaderSize(pkt,sizeof(endpoint)-1);
      memcpy(&pkt->payload[0],&endpoint,sizeof(endpoint)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIQUERY-COAP_OPTION_NUM_CONTENTFORMAT) << 4 |
         ((sizeof(endpoint))-1);

      // content-type option
          packetfunctions_reserveHeaderSize(pkt,2);
          pkt->payload[0]                  = (COAP_OPTION_NUM_CONTENTFORMAT-COAP_OPTION_NUM_URIPATH) << 4 |1;
          pkt->payload[1]                  = COAP_MEDTYPE_APPLINKFORMAT;

      // location-path option
      packetfunctions_reserveHeaderSize(pkt,sizeof(URI_oma_rd)-1);
      memcpy(&pkt->payload[0],&URI_oma_rd,sizeof(URI_oma_rd)-1);
      packetfunctions_reserveHeaderSize(pkt,1);
      pkt->payload[0] = (COAP_OPTION_NUM_URIPATH) << 4 |
         (sizeof(URI_oma_rd)-1);
      numOptions++;

      pkt->l4_destination_port   = WKP_UDP_COAP;
      pkt->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
      pkt->l3_destinationAdd.type = ADDR_128B;
      // set destination address here
      memcpy(&pkt->l3_destinationAdd.addr_128b[0], &coap_rd_server.ipv6address[0], 16);

      //send
      outcome = opencoap_send(
              pkt,
              COAP_TYPE_CON,
			  COAP_CODE_REQ_POST,
			  1,
              &(coap_rd_server.desc)
              );

      if (outcome == E_FAIL) {
        openqueue_freePacketBuffer(pkt);
      }
}

void coap_rd_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);

   // to be added: check reply if not successfull reschedule to closer time if it is reschedule a later time..

}
