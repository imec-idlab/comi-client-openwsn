/**
\brief CoAP Management Interface Application.
\author Abdulkadir Karaagac <abdulkadir.karaagac@ugent.be>, February 2017.
 */

#include "c6top.h"

#include "base64/base64.h"
#include "comi/comi.h"
#include "comi/cbor/cbor.h"
#include "opendefs.h"
#include "sixtop.h"
#include "idmanager.h"
#include "openqueue.h"
#include "neighbors.h"
#include "scheduler.h"
#include "schedule.h"
#include "opentimers.h"


//=========================== defines =========================================
 uint8_t comi_path_celllist[COMI_PATH_SIZE];			// 4001
 uint8_t comi_path_neighborlist[COMI_PATH_SIZE];		// 4011
//=========================== variables =======================================

c6top_vars_t c6top_vars;
//=========================== prototypes ======================================

owerror_t comi_celllist_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);
owerror_t comi_neighborlist_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);


void comi_celllist_register(c6top_vars_t* c6top_vars);
void comi_neighborlist_register(c6top_vars_t* c6top_vars);
void comi_send_neighbor_notification(void);
void comi_neighbor_observer_cb(opentimer_id_t id);
void c6top_register_observer(observer_t* neighbor_observer, OpenQueueEntry_t* msg, coap_header_iht*  coap_header);
void c6top_deregister_observer(observer_t* neighbor_observer);

//=========================== public ==========================================

void c6top_init() {
	if(idmanager_getIsDAGroot()==TRUE) return;

	comi_init();
	memset(&c6top_vars,0,sizeof(c6top_vars_t));

	// retrieve the schedule table
	comi_celllist_register(&c6top_vars);
	comi_neighborlist_register(&c6top_vars);
}

//=========================== private =========================================

/**
   \brief Register cell list resources into opencoap.
 */
void comi_celllist_register(
		c6top_vars_t* c6top_vars) {

	c6top_vars->comi_celllist.schedule_vars=schedule_getSchedule_Vars();

	uint8_t path1len;
	// Register Cell List
	path1len=base64_encode(SID_comi_celllist,&comi_path_celllist[0]);
	c6top_vars->comi_celllist.desc.path0len   = sizeof(comi_path)-1;
	c6top_vars->comi_celllist.desc.path0val   = (uint8_t*)(&comi_path);
	c6top_vars->comi_celllist.desc.path1len   = path1len;
	c6top_vars->comi_celllist.desc.path1val   = &comi_path_celllist[0];
	c6top_vars->comi_celllist.desc.path2len   = 0;
	c6top_vars->comi_celllist.desc.path2val   = NULL;
	c6top_vars->comi_celllist.desc.componentID      = COMPONENT_COMI;
	c6top_vars->comi_celllist.desc.discoverable     = TRUE;
	c6top_vars->comi_celllist.desc.callbackRx       = &comi_celllist_receive;
	c6top_vars->comi_celllist.desc.callbackSendDone = &c6top_sendDone;
	// register with the CoAP module
	opencoap_register(&c6top_vars->comi_celllist.desc);
}

/**
   \brief Register neigbor list resources into opencoap.
 */
void comi_neighborlist_register(
		c6top_vars_t* c6top_vars) {

	c6top_vars->comi_neighborlist.neighbors_vars=schedule_getNeighbor_Vars();

	uint8_t path1len;

	// Register Neighbor List
	path1len=base64_encode(SID_comi_neighborlist,&comi_path_neighborlist[0]);
	c6top_vars->comi_neighborlist.desc.path0len   = sizeof(comi_path)-1;
	c6top_vars->comi_neighborlist.desc.path0val   = (uint8_t*)(&comi_path);
	c6top_vars->comi_neighborlist.desc.path1len   = path1len;
	c6top_vars->comi_neighborlist.desc.path1val   = &comi_path_neighborlist[0];
	c6top_vars->comi_neighborlist.desc.path2len   = 0;
	c6top_vars->comi_neighborlist.desc.path2val   = NULL;
	c6top_vars->comi_neighborlist.desc.componentID      = COMPONENT_COMI;
	c6top_vars->comi_neighborlist.desc.discoverable     = TRUE;
	c6top_vars->comi_neighborlist.desc.callbackRx       = &comi_neighborlist_receive;
	c6top_vars->comi_neighborlist.desc.callbackSendDone = &c6top_sendDone;
	// register with the CoAP module
	opencoap_register(&c6top_vars->comi_neighborlist.desc);
}

//=========================== RECEIVE =========================================

owerror_t comi_celllist_receive(
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

			uint8_t pos=0;

			uint16_t maxActiveSlots=schedule_getMaxActiveSlots();
			uint8_t comi_payload[MAX_PAYLOAD_SIZE];

			uint8_t key=0;
			if (comi_getKey(&coap_options[2],&key)==1){
					if(key<maxActiveSlots){
						pos=pos+comi_serialize_cell(&comi_payload[pos],key);
					}
					else{
						openserial_printError(COMPONENT_COMI,ERR_AK_COMI,(errorparameter_t)33, (errorparameter_t)33);
					}
			}else{
				uint8_t i;
				pos=1;
				for(i=0;i<maxActiveSlots;i++){
					if(c6top_vars.comi_celllist.schedule_vars->scheduleBuf[i].next==NULL || pos>=MAX_PAYLOAD_SIZE-1){
						if(i>0){
							comi_payload[0]= cbor_serialize_array(i);
						}
						i=maxActiveSlots;
					}
					else{
						pos=pos+comi_serialize_cell(&comi_payload[pos],i);
					}
				}
			}
			packetfunctions_reserveHeaderSize(msg,pos);
			memcpy(&msg->payload[0],&comi_payload[0],pos);

			packetfunctions_reserveHeaderSize(msg,1);
			msg->payload[0]  = COAP_PAYLOAD_MARKER;
				// add return option
			packetfunctions_reserveHeaderSize(msg,2);
			msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
			msg->payload[1]     = COAP_MEDTYPE_CBOR;

				// set the CoAP header
			coap_header->Code                = COAP_CODE_RESP_CONTENT;
			outcome                          = E_SUCCESS;

			break;

		case COAP_CODE_REQ_POST:


			// reset packet payload
	          msg->payload                  = &(msg->packet[127]);
	          msg->length                   = 0;
			if( 1 ){

				//schedule_addActiveSlotByID(uint8_t cellID, slotOffset_t slotOffset, cellType_t type, bool shared,channelOffset_t channelOffset,open_addr_t* neighbor);

	          // set the CoAP header
		          coap_header->Code             = COAP_CODE_RESP_CREATED;
		          outcome                       = E_SUCCESS;
			}
			else{
			// set the CoAP header
	          coap_header->Code                = COAP_CODE_RESP_CONFLICT;
	          outcome                          = E_FAIL;
			}

			break;
		default:
			outcome = E_FAIL;
			break;
	}

	return outcome;
}

owerror_t comi_neighborlist_receive(
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

				uint8_t pos=0;
				uint8_t NeighborNum=neighbors_getNumNeighbors();
				uint8_t comi_payload[MAX_PAYLOAD_SIZE];
				uint8_t i;
				if(NeighborNum>0){
					if(NeighborNum==1){
						pos=pos+comi_serialize_neighbor(&comi_payload[pos],0);
					}
					else{
						comi_payload[0]= cbor_serialize_array(NeighborNum);
						pos=pos+1;

						for(i=0;i<NeighborNum;i++){
							pos=pos+comi_serialize_neighbor(&comi_payload[pos],i);
						}
					}
				}

				packetfunctions_reserveHeaderSize(msg,pos);
				memcpy(&msg->payload[0],&comi_payload[0],pos);

				packetfunctions_reserveHeaderSize(msg,1);
				msg->payload[0]  = COAP_PAYLOAD_MARKER;


				// add return option
				if (coap_options[0].type == COAP_OPTION_NUM_OBSERVE) {

					if(coap_options[0].length==0){
						c6top_register_observer(&c6top_vars.comi_neighborlist.neighbor_observer, msg,&coap_header[0]);
					}
					else{
						c6top_deregister_observer(&c6top_vars.comi_neighborlist.neighbor_observer);
					}
					packetfunctions_reserveHeaderSize(msg,2);
					msg->payload[0]     = (COAP_OPTION_NUM_CONTENTFORMAT-COAP_OPTION_NUM_OBSERVE) << 4 | 1;
					msg->payload[1]     = COAP_MEDTYPE_CBOR;

					if(((c6top_vars.comi_neighborlist.neighbor_observer.optionID>>8) & 0x00ff)==0){
						packetfunctions_reserveHeaderSize(msg,2);
						msg->payload[0]     = COAP_OPTION_NUM_OBSERVE << 4 | 1;
						msg->payload[1]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>0) & 0x00ff;
					}
					else{
						packetfunctions_reserveHeaderSize(msg,3);
						msg->payload[0]     = COAP_OPTION_NUM_OBSERVE << 4 | 2;
						msg->payload[1]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>8) & 0x00ff;
						msg->payload[2]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>0) & 0x00ff;
					}
					 if (c6top_vars.comi_neighborlist.neighbor_observer.optionID++ == 0xffff) {
						 c6top_vars.comi_neighborlist.neighbor_observer.optionID = 0;
					   }
				}
				else{
					packetfunctions_reserveHeaderSize(msg,2);
					msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
					msg->payload[1]     = COAP_MEDTYPE_CBOR;
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



/* serialize cell with cellid */
uint8_t comi_serialize_cell(uint8_t* comi_payload_curser, uint8_t cell_id)
{
	uint8_t size=0;
	comi_payload_curser[size] = cbor_serialize_map(0x08);
	size++;

	// write cellid
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_cellid-SID_comi_celllist);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], cell_id);

	// write slotframeid
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_slotframeid-SID_comi_celllist);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->frameNumber);

	// write slot offset
	size = size + comi_serialize_slotoffset(&comi_payload_curser[size],SID_comi_celllist,cell_id);

	// write channel offset
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_choffset-SID_comi_celllist);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].channelOffset);

	// write linkoption
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_linkoption-SID_comi_celllist);
	uint8_t linkoption=get_linkoption(&c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id]);
	size = size + cbor_serialize_byte(&comi_payload_curser[size],&linkoption,1);

	// write addr
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_nodeaddr-SID_comi_celllist);
	if(c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].neighbor.type==ADDR_64B){
		//size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].neighbor.addr_64b[0],8);
		size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].neighbor.addr_64b[7],1);
	}
	else{
		size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].neighbor.addr_64b[0],1);
	}

	// write statistics
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_stats-SID_comi_celllist);
	uint8_t stats=(uint8_t)(100*c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].numTxACK/c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].numTx);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size],stats );

	// write last byte of last asn
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_lastasn-SID_comi_celllist);
	size = size + cbor_serialize_byte(&comi_payload_curser[size],&c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].lastUsedAsn.byte4,1);
	return size;
}

/* serialize neighbor with neighborid */
uint8_t comi_serialize_neighbor(uint8_t* comi_payload_curser, uint8_t neighbor_id)
{
	uint8_t size=0;

	comi_payload_curser[size] = cbor_serialize_map(0x04);
	size++;

	// write last byte of neighboraddress
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_neighborAddr-SID_comi_neighborlist);
	size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].addr_64b.addr_64b[7],1);

	// write rssi
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_rssi-SID_comi_neighborlist);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].rssi);

	// write statistics
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_lqi-SID_comi_neighborlist);
	uint8_t stats=(uint8_t)(100*c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].numTxACK/c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].numTx);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size],stats );

	// write asn
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_asn-SID_comi_neighborlist);
	size = size + cbor_serialize_uint16(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->currentScheduleEntry->lastUsedAsn.bytes0and1-c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].asn.bytes0and1);


	return size;
}

/* serialize slotoffset with cellid */
uint8_t comi_serialize_slotoffset(uint8_t* comi_payload_curser, sid_t baseSID, uint8_t cell_id)
{
	uint8_t size=0;
	// write slot offset
	if(baseSID!=SID_comi_slotoffset){
		size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_slotoffset-baseSID);
	}
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].slotOffset);
	return size;
}


uint8_t get_linkoption(scheduleEntry_t* schedule)
{
	uint8_t linkoption=0x00;
	if(schedule->type==CELLTYPE_TX){
		linkoption=linkoption + 0x80;
	}else if(schedule->type==CELLTYPE_TXRX){
		linkoption=linkoption + 0x80;
		linkoption=linkoption + 0x40;
	}
	else if(schedule->type==CELLTYPE_RX)	{
		linkoption=linkoption + 0x40;
	}

	if(schedule->shared==TRUE){
		linkoption=linkoption + 0x20;
	}
	// todo soft/hard
	// normal/advertising to be added
	return linkoption;
}


//timer fired, but we don't want to execute task in ISR mode
//instead, push task to scheduler with COAP priority, and let scheduler take care of it
void comi_neighbor_observer_cb(opentimer_id_t id){

		 scheduler_push_task(comi_send_neighbor_notification,TASKPRIO_COAP);
}

void comi_send_neighbor_notification() {
	   OpenQueueEntry_t* pkt;
	   owerror_t outcome;
	   pkt = openqueue_getFreePacketBuffer(COMPONENT_LWM2M);
	   if (pkt == NULL) {
		   openserial_printError(COMPONENT_LWM2M,ERR_BUSY_SENDING,
				   (errorparameter_t)0,
				   (errorparameter_t)0);
		   openqueue_freePacketBuffer(pkt);
		   return;
	   }

	      pkt->creator   = COMPONENT_COMI;
	      pkt->owner      = COMPONENT_COMI;
	      pkt->l4_protocol  = IANA_UDP;

	      uint8_t pos=0;
	      uint8_t NeighborNum=neighbors_getNumNeighbors();
	      uint8_t comi_payload[MAX_PAYLOAD_SIZE];
	      uint8_t i;
	      if(NeighborNum>0){
	    	  if(NeighborNum==1){
	    		  pos=pos+comi_serialize_neighbor(&comi_payload[pos],0);
	    	  }
	    	  else{
	    		  comi_payload[0]= cbor_serialize_array(NeighborNum);
	    		  pos=pos+1;

	    		  for(i=0;i<NeighborNum;i++){
	    			  pos=pos+comi_serialize_neighbor(&comi_payload[pos],i);
	    		  }
	    	  }
	      }

	      packetfunctions_reserveHeaderSize(pkt,pos);
	      memcpy(&pkt->payload[0],&comi_payload[0],pos);

	      packetfunctions_reserveHeaderSize(pkt,1);
	      pkt->payload[0]  = COAP_PAYLOAD_MARKER;


	      packetfunctions_reserveHeaderSize(pkt,2);
	      pkt->payload[0]     = (COAP_OPTION_NUM_CONTENTFORMAT-COAP_OPTION_NUM_OBSERVE) << 4 | 1;
	      pkt->payload[1]     = COAP_MEDTYPE_CBOR;



			if(((c6top_vars.comi_neighborlist.neighbor_observer.optionID>>8) & 0x00ff)==0){
				packetfunctions_reserveHeaderSize(pkt,2);
				pkt->payload[0]     = COAP_OPTION_NUM_OBSERVE << 4 | 1;
				pkt->payload[1]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>0) & 0x00ff;
			}
			else{
				packetfunctions_reserveHeaderSize(pkt,3);
				pkt->payload[0]     = COAP_OPTION_NUM_OBSERVE << 4 | 2;
				pkt->payload[1]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>8) & 0x00ff;
				pkt->payload[2]     =(c6top_vars.comi_neighborlist.neighbor_observer.optionID>>0) & 0x00ff;
			}
			 if (c6top_vars.comi_neighborlist.neighbor_observer.optionID++ == 0xffff) {
				 c6top_vars.comi_neighborlist.neighbor_observer.optionID = 0;
			   }

			 if(c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type!=ADDR_NONE){
	    	  pkt->l4_destination_port   = c6top_vars.comi_neighborlist.neighbor_observer.observer_port;
	    	  pkt->l4_sourcePortORicmpv6Type   = WKP_UDP_COAP;
	    	  pkt->l3_destinationAdd.type = c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type;
	    	 	      // set destination address here
	    	 memcpy(&pkt->l3_destinationAdd.addr_128b[0], &c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.addr_128b[0], LENGTH_ADDR128b);

	    	 	      //send
	    	 	      outcome = opencoap_sendresponse(
	    	 	    		  pkt,
	    	 				  COAP_TYPE_RES,
	    	 				  COAP_CODE_RESP_CONTENT,
							  c6top_vars.comi_neighborlist.neighbor_observer.TKL,
							  &c6top_vars.comi_neighborlist.neighbor_observer.token[0]
							  );

	    	 	      if (outcome == E_FAIL) {
	    	 	    	  openqueue_freePacketBuffer(pkt);
	    	 	      }
	      }
}

void c6top_register_observer(observer_t* neighbor_observer, OpenQueueEntry_t* msg, coap_header_iht*  coap_header){

	if(c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type!=ADDR_NONE){
		opentimers_stop(c6top_vars.comi_neighborlist.neighbor_observer.observe_Neighbor_Timer_Id);
	}

	c6top_vars.comi_neighborlist.neighbor_observer.TKL=coap_header->TKL;
	memcpy(&c6top_vars.comi_neighborlist.neighbor_observer.token[0],&coap_header->token[0],coap_header->TKL);
	c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type = ADDR_128B;
	memcpy(&c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.addr_128b[0],&msg->l3_sourceAdd.addr_128b[0],LENGTH_ADDR128b);
	c6top_vars.comi_neighborlist.neighbor_observer.observer_port=msg->l4_sourcePortORicmpv6Type;
	c6top_vars.comi_neighborlist.neighbor_observer.observe_Neighbor_Timer_Id = opentimers_start(10000, TIMER_PERIODIC, TIME_MS, comi_neighbor_observer_cb);
	c6top_vars.comi_neighborlist.neighbor_observer.optionID=2;
}

void c6top_deregister_observer(observer_t* neighbor_observer){
	if(c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type!=ADDR_NONE){
	opentimers_stop(c6top_vars.comi_neighborlist.neighbor_observer.observe_Neighbor_Timer_Id);
	c6top_vars.comi_neighborlist.neighbor_observer.observer_Address.type=ADDR_NONE;
	}
}

void c6top_sendDone(OpenQueueEntry_t* msg, owerror_t error) {
	openqueue_freePacketBuffer(msg);
}


