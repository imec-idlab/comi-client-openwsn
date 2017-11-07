#include "umonitor.h"

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

umonitor_vars_t umonitor_vars;
uint16_t     last_asn_last_octet;
uint8_t 	 timerStatus;

static const uint8_t umonitor_serv_addr[]   = {
   0xbb, 0xbb, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01
};

//=========================== prototypes ======================================
void umonitor_send(void);

void umonitor_timer_cb(opentimer_id_t id);
void umonitor_task_cb(void);
void umonitor_button_cb(uint8_t value);
//=========================== public ==========================================

void umonitor_init() {

   REMOTE_FIRE_REGISTER_INT(umonitor_button_cb);
   // clear local variables
   memset(&umonitor_vars,0,sizeof(umonitor_vars_t));
      // register at UDP stack
   umonitor_vars.desc.port              = WKP_UDP_MONITOR;
   umonitor_vars.desc.callbackReceive   = &umonitor_receive;
   umonitor_vars.desc.callbackSendDone  = &umonitor_sendDone;
   umonitor_vars.counter=0;
   umonitor_vars.packetnum=UMONITOR_PACKET_NUM;
   umonitor_vars.period = UMONITOR_PERIOD_MS;
   last_asn_last_octet=0;

   openudp_register(&umonitor_vars.desc);
}

void umonitor_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
   openqueue_freePacketBuffer(msg);
}

void umonitor_receive(OpenQueueEntry_t* pkt) {
   
   uint8_t  asnArray[5];
   ieee154e_getAsn(asnArray);
   uint16_t sendASN = (((pkt->payload[1] & 0x00ff)<<8) & 0xff00) + (pkt->payload[0]&0x00ff);
   uint16_t recvASN = (((asnArray[1] & 0x00ff)<<8) & 0xff00) + (asnArray[0] & 0x00ff);
   uint16_t delay=recvASN-sendASN;
   uint8_t code = pkt->payload[5];
   uint16_t counter = pkt->payload[6]+ ((pkt->payload[7]<<8)&0xff00);

   openserial_printError(
	   COMPONENT_UMONITOR,
	   ERR_RECV_UMONITOR_MESSAGE,
      (errorparameter_t) delay, (errorparameter_t)counter);


   openqueue_freePacketBuffer(pkt);
   debugpins_slot_toggle();
}

//=========================== private =========================================

void umonitor_timer_cb(opentimer_id_t id){
   scheduler_push_task(umonitor_task_cb,TASKPRIO_COAP);
}

void umonitor_resetTimer() {
	if(umonitor_vars.period<200){
		umonitor_vars.period = UMONITOR_PERIOD_MS;
		umonitor_vars.packetnum=UMONITOR_PACKET_NUM;
	}
	else{
		umonitor_vars.period=umonitor_vars.period/2;
		umonitor_vars.packetnum=umonitor_vars.packetnum*2;
	}

	opentimers_stop(umonitor_vars.timerId);
	opentimers_setPeriod(umonitor_vars.timerId,TIME_MS,umonitor_vars.period);
	   openserial_printError(
	      COMPONENT_UMONITOR,
		  ERR_INFO_TRANSMIT,
	      (errorparameter_t)1111,
	      (errorparameter_t)umonitor_vars.period
	   );
	opentimers_restart(umonitor_vars.timerId);

}

void umonitor_task_cb() {
	umonitor_send();
	if(umonitor_vars.packetnum > 0 && umonitor_vars.counter>umonitor_vars.packetnum){
		umonitor_vars.counter=0;
		umonitor_resetTimer();
	}
}

void umonitor_button_cb(uint8_t value) {

	uint8_t              asnArray[5];

	ieee154e_getAsn(asnArray);
	uint16_t recvASN = (((asnArray[1] & 0x00ff)<<8) & 0xff00) + (asnArray[0] & 0x00ff);

	//Debounce
	if(last_asn_last_octet <= recvASN && last_asn_last_octet+20 > recvASN){ // duration between 2 push is 200ms
		return;
	}
	last_asn_last_octet = recvASN;

	if(umonitor_vars.period==0){
		umonitor_send();
	}
	else{
		if(timerStatus==0){
			// start periodic timer
			umonitor_vars.timerId = opentimers_start(
					umonitor_vars.period,
					TIMER_PERIODIC,TIME_MS,
					umonitor_timer_cb
			);
			timerStatus=1;
		}
		else if(timerStatus==1){ // runnning
			opentimers_stop(umonitor_vars.timerId);
			timerStatus=2;
			umonitor_vars.counter=0;
		}
		else{					// stopped
			 opentimers_restart(umonitor_vars.timerId);
			 timerStatus=1;
		}
	}
}

void umonitor_send(void) {
   OpenQueueEntry_t*    pkt;
   uint8_t              asnArray[5];

   ieee154e_getAsn(asnArray);

   // get a free packet buffer
   pkt = openqueue_getFreePacketBuffer(COMPONENT_UMONITOR);
   if (pkt==NULL) {
      openserial_printError(
    		  COMPONENT_UMONITOR,
         ERR_NO_FREE_PACKET_BUFFER,
         (errorparameter_t)0,
         (errorparameter_t)0
      );
      return;
   }
   
   debugpins_slot_toggle();

   pkt->owner                         = COMPONENT_UMONITOR;
   pkt->creator                       = COMPONENT_UMONITOR;
   pkt->label                         = 0;//COMPONENT_UMONITOR;
   pkt->l4_protocol                   = IANA_UDP;
   pkt->l4_destination_port           = WKP_UDP_MONITOR;
   pkt->l4_sourcePortORicmpv6Type     = WKP_UDP_MONITOR;
   pkt->l3_destinationAdd.type        = ADDR_128B;
   memcpy(&pkt->l3_destinationAdd.addr_128b[0],&umonitor_serv_addr[0],16);

   umonitor_vars.counter++;

   // add payload
   packetfunctions_reserveHeaderSize(pkt,sizeof(uint16_t));
   pkt->payload[1] = (uint8_t)((umonitor_vars.counter & 0xff00)>>8);
   pkt->payload[0] = (uint8_t)(umonitor_vars.counter & 0x00ff);


   openserial_printError(
      COMPONENT_UMONITOR,
	  ERR_INFO_TRANSMIT,
      (errorparameter_t)1,
      (errorparameter_t)umonitor_vars.counter
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
