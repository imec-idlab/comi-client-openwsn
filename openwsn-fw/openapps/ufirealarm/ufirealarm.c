#include "ufirealarm.h"

#include "opendefs.h"
#include "openqueue.h"
#include "opentimers.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "scheduler.h"
#include "IEEE802154E.h"
#include "idmanager.h"
#include "debugpins.h"

//=========================== variables =======================================

ufire_vars_t ufire_vars;
uint16_t     last_asn_last_octet;
uint8_t 	 sendStatus;

static const uint8_t ufire_dst_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x12, 0x4b, 0x00, 0x06, 0x0d, 0x9e, 0xbe
};

//=========================== prototypes ======================================
void ufire_sendalarm(void);

void ufire_button_cb(uint8_t value);
void ieee154_frame_start_cb(void);
//=========================== public ==========================================

void ufirealarm_init() {

   REMOTE_FIRE_REGISTER_INT(ufire_button_cb);
   // clear local variables
   memset(&ufire_vars,0,sizeof(ufire_vars_t));
      // register at UDP stack
   ufire_vars.desc.port              = WKP_UDP_FIRE;
   ufire_vars.desc.callbackReceive   = &ufire_receive;
   ufire_vars.desc.callbackSendDone  = &ufire_sendDone;
   ufire_vars.counter=0;
   last_asn_last_octet=0;
   ufire_vars.frameCounter=0;
   sendStatus=0;
   openudp_register(&ufire_vars.desc);
}

void ufire_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void ufire_receive(OpenQueueEntry_t* pkt) {
   
   uint8_t  asnArray[5];
   ieee154e_getAsn(asnArray);
   uint16_t sendASN = (((pkt->payload[1] & 0x00ff)<<8) & 0xff00) + (pkt->payload[0]&0x00ff);
   uint16_t recvASN = (((asnArray[1] & 0x00ff)<<8) & 0xff00) + (asnArray[0] & 0x00ff);
   uint16_t delay=recvASN-sendASN;
   uint8_t code = pkt->payload[5];
   uint16_t counter = pkt->payload[6]+ ((pkt->payload[7]<<8)&0xff00);

   openserial_printError(
	   COMPONENT_UFIRE,
	   ERR_RECV_UFIRE_MESSAGE,
      (errorparameter_t) delay, (errorparameter_t)counter);


   openqueue_freePacketBuffer(pkt);
   debugpins_slot_toggle();
}

//=========================== private =========================================
void ieee154_frame_start_cb(void){
	ufire_vars.frameCounter++;
	if(ufire_vars.frameCounter>=UFIRE_PERIOD_FRAME){
		ufire_vars.frameCounter=0;
		ufire_sendalarm();
	}
}

void ufire_button_cb(uint8_t value) {

	uint8_t              asnArray[5];

	ieee154e_getAsn(asnArray);
	uint16_t recvASN = (((asnArray[1] & 0x00ff)<<8) & 0xff00) + (asnArray[0] & 0x00ff);

	//Debounce
	if(last_asn_last_octet <= recvASN && last_asn_last_octet+20 > recvASN){ // duration between 2 push is 200ms
		return;
	}
	last_asn_last_octet = recvASN;

	if(UFIRE_PERIOD_FRAME==0){
		ufire_sendalarm();
	}
	else{
		if(sendStatus==0){
			IEEE_FRAME_START_REGISTER(ieee154_frame_start_cb);
			sendStatus=1;
		}
		else if(sendStatus==1){ // runnning
			IEEE_FRAME_START_REGISTER(NULL);
			sendStatus=0;
			ufire_vars.counter=0;
			ufire_vars.frameCounter=0;
		}

	}
}

void ufire_sendalarm(void) {
   OpenQueueEntry_t*    pkt;
   uint8_t              asnArray[5];

   ieee154e_getAsn(asnArray);

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UFIRE);
   if (pkt==NULL) {
      openserial_printError(
    		  COMPONENT_UFIRE,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   debugpins_slot_toggle();

   pkt->owner                         = COMPONENT_UFIRE;
   pkt->creator                       = COMPONENT_UFIRE;
   pkt->label                         = COMPONENT_UFIRE;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_FIRE;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_FIRE;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&ufire_dst_addr[0],16);

   ufire_vars.counter++;

   // add payload
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
   pkt->payload[1] = (uint8_t)((ufire_vars.counter & 0xff00)>>8);
   pkt->payload[0] = (uint8_t)(ufire_vars.counter & 0x00ff);


   openserial_printError(
      COMPONENT_UFIRE,
	  ERR_INFO_TRANSMIT,
      (errorparameter_t)1,
      (errorparameter_t)ufire_vars.counter
   );


   packetfunctions_reserveHeaderSize(pkt,1);
   uint8_t x = idmanager_getMyID(ADDR_64B)->addr_64b[7];
   pkt->payload[0] = x;

   packetfunctions_reserveHeaderSize(pkt,sizeof(asn_t));
   pkt->payload[0] = asnArray[0];
   pkt->payload[1] = asnArray[1];
   pkt->payload[2] = asnArray[2];
   pkt->payload[3] = asnArray[3];
   pkt->payload[4] = asnArray[4];
   pkt->l2_frameType = IEEE154_TYPE_PRIORITY;
   
   if ((openudp_send(pkt))==E_FAIL) {
      openqueue_freePacketBuffer(pkt);
   }
}
