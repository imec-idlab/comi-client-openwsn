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
#include "coap_rd.h"

//=========================== defines =========================================

// URIs for available resources
uint8_t comi_path_celllist[COMI_PATH_SIZE];			// 4001
uint8_t comi_path_neighborlist[COMI_PATH_SIZE];		// 4011
uint8_t comi_path_routingtable[COMI_PATH_SIZE];		// 4021

//=========================== variables =======================================
c6top_vars_t c6top_vars;

//=========================== prototypes ======================================

// coap resource registration
void comi_celllist_register(c6top_vars_t* c6top_vars);
void comi_neighborlist_register(c6top_vars_t* c6top_vars);
void comi_routingtable_register(c6top_vars_t* c6top_vars);

// request and response reception for certain resources
owerror_t comi_celllist_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);
owerror_t comi_neighborlist_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);
owerror_t comi_routingtable_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);

// neighbor observer functions
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
	comi_routingtable_register(&c6top_vars);
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

/**
   \brief Register routing table resources into opencoap.
 */
void comi_routingtable_register(
		c6top_vars_t* c6top_vars) {

	c6top_vars->comi_routingtable.icmpv6rpl_vars=get_icmpv6rpl_vars();

	uint8_t path1len;

	// Register Routing Table
	path1len=base64_encode(SID_comi_routingtable,&comi_path_routingtable[0]);
	c6top_vars->comi_routingtable.desc.path0len   = sizeof(comi_path)-1;
	c6top_vars->comi_routingtable.desc.path0val   = (uint8_t*)(&comi_path);
	c6top_vars->comi_routingtable.desc.path1len   = path1len;
	c6top_vars->comi_routingtable.desc.path1val   = &comi_path_routingtable[0];
	c6top_vars->comi_routingtable.desc.path2len   = 0;
	c6top_vars->comi_routingtable.desc.path2val   = NULL;
	c6top_vars->comi_routingtable.desc.componentID      = COMPONENT_COMI;
	c6top_vars->comi_routingtable.desc.discoverable     = TRUE;
	c6top_vars->comi_routingtable.desc.callbackRx       = &comi_routingtable_receive;
	c6top_vars->comi_routingtable.desc.callbackSendDone = &c6top_sendDone;
	// register with the CoAP module
	opencoap_register(&c6top_vars->comi_routingtable.desc);
}

//=========================== RECEIVE =========================================

owerror_t comi_celllist_receive(
		OpenQueueEntry_t* msg,
		coap_header_iht*  coap_header,
		coap_option_iht*  coap_options
) {
	owerror_t           outcome;
	uint8_t 			comi_payload[MAX_COAP_PAYLOAD_SIZE];
	uint8_t 			comi_payload_length=0;
	uint16_t 			maxActiveSlots = schedule_getMaxActiveSlots();
	uint8_t 			posQuery =getOptionPosition(coap_options,COAP_OPTION_NUM_URIQUERY);

	switch (coap_header->Code) {
		case COAP_CODE_REQ_GET:
			// reset packet payload
			msg->payload                     = &(msg->packet[127]);
			msg->length                      = 0;

			blockwise_var_t blockwiseVar;
			blockwiseVar.blocksize = COAP_PREF_BLOCK_SIZE;
			blockwiseVar.posBlock=getOptionPosition(coap_options,COAP_OPTION_NUM_BLOCKWISE2);

			//STEP 1: Check if the request is for available blocks or new resource reading
			// Checking if the client has requested blockwise operation
			if(blockwiseVar.posBlock<MAX_COAP_OPTIONS){
				processBlockWiseRequest(&coap_options[blockwiseVar.posBlock], &blockwiseVar);

				if(blockwiseVar.NUM>0){	// already existing data is asked
					uint8_t len=(16 << blockwiseVar.SZX);
					uint8_t curser=(blockwiseVar.NUM<<(blockwiseVar.SZX+4));

					if(curser<c6top_vars.coap_payload_cache.length){
						if((c6top_vars.coap_payload_cache.length-curser)<=len){
							len=(c6top_vars.coap_payload_cache.length-curser);
							blockwiseVar.M=0;
						}
						else{
							blockwiseVar.M=1;
						}

						packetfunctions_reserveHeaderSize(msg,len);
						memcpy(&msg->payload[0],&c6top_vars.coap_payload_cache.cache[curser],len);
					}

					opencoap_addPayloadMarker(msg);
					opencoap_addBlockWiseOption(msg,&blockwiseVar,COAP_OPTION_NUM_CONTENTFORMAT);

					packetfunctions_reserveHeaderSize(msg,2);
					msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
					msg->payload[1]     = COAP_MEDTYPE_CBOR;

					coap_header->Code                = COAP_CODE_RESP_CONTENT;
					outcome = E_SUCCESS;
					break;
				}
					// check if the client has asked for another block size
					if(((0x0010 << blockwiseVar.SZX))<COAP_PREF_BLOCK_SIZE){
						blockwiseVar.blocksize=(0x0010 << blockwiseVar.SZX);
					}
					blockwiseVar.SZX=calculateSZX(blockwiseVar.blocksize);
			}

			//STEP 2: Check for query, return values with corresponding key
			if(posQuery<MAX_COAP_OPTIONS){
				uint8_t key=0;
				if (comi_getKey(&coap_options[posQuery],&key)==E_FAIL || key>=maxActiveSlots){
					outcome = E_FAIL;
					break;
				}
				comi_payload_length=comi_payload_length+comi_serialize_cell(&comi_payload[comi_payload_length],key);
			}
			else{
				//No query
				uint8_t i=0;
				uint8_t numCell=0;
				comi_payload_length=1;

				while (i<maxActiveSlots) {

					if(comi_payload_length>MAX_COAP_PAYLOAD_SIZE-1){
						break;
					}
					else if(c6top_vars.comi_celllist.schedule_vars->scheduleBuf[i].type!=CELLTYPE_OFF){
						comi_payload_length=comi_payload_length+comi_serialize_cell(&comi_payload[comi_payload_length],i);
						numCell++;
					}
					i++;
				}
				if(numCell>0){
					comi_payload[0]= cbor_serialize_array(numCell);
				}
			}

			if(comi_payload_length>=MAX_COAP_PAYLOAD_SIZE){
				openserial_printError(COMPONENT_COMI,ERR_LONG_PACKET,(errorparameter_t)33, (errorparameter_t)comi_payload_length);
				outcome = E_FAIL;
				break;
			}

			// step 3 write into packet
			if(comi_payload_length>=blockwiseVar.blocksize){
					blockwiseVar.isBlock=1;
					blockwiseVar.NUM=0;
					blockwiseVar.SZX=calculateSZX(blockwiseVar.blocksize);
					memcpy(&c6top_vars.coap_payload_cache.cache[0], &comi_payload[0],comi_payload_length);
					c6top_vars.coap_payload_cache.length=comi_payload_length;
					packetfunctions_reserveHeaderSize(msg,blockwiseVar.blocksize);
					memcpy(&msg->payload[0],&comi_payload[0],blockwiseVar.blocksize);
					blockwiseVar.M=1;
			}
			else{
				packetfunctions_reserveHeaderSize(msg,comi_payload_length);
				memcpy(&msg->payload[0],&comi_payload[0],comi_payload_length);
				if(blockwiseVar.isBlock){
					blockwiseVar.M=0;
				}
			}

			opencoap_addPayloadMarker(msg);
			opencoap_addBlockWiseOption(msg,&blockwiseVar,COAP_OPTION_NUM_CONTENTFORMAT);

			packetfunctions_reserveHeaderSize(msg,2);
			msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
			msg->payload[1]     = COAP_MEDTYPE_CBOR;

			// set the CoAP header
			coap_header->Code                = COAP_CODE_RESP_CONTENT;
			outcome                          = E_SUCCESS;

			break;

		case COAP_CODE_REQ_PUT: 	// Putting a new Schedule

			if(posQuery<MAX_COAP_OPTIONS){
				uint8_t key=0;
				if (comi_getKey(&coap_options[posQuery],&key)==E_FAIL || key>=maxActiveSlots){
					outcome = E_FAIL;
					break;
				}

				int baseSID=4001;
				scheduleEntry_t cell;
				uint8_t cellID;
				if(comi_deserialize_cell(msg,&cell,&cellID,baseSID)==E_FAIL){
					coap_header->Code             = COAP_CODE_RESP_NOTFOUND;
					outcome                       = E_FAIL;
				}

				msg->payload                     = &(msg->packet[127]);
				msg->length                      = 0;

				if(key!=cellID){
					coap_header->Code             = COAP_CODE_RESP_BADREQ;
					outcome                       = E_FAIL;
					break;
				}

				if(schedule_addActiveSlotByID(key,cell.slotOffset,cell.channelOffset,cell.type,cell.isHard,cell.shared,cell.label,&cell.neighbor)==E_SUCCESS){
					// set the CoAP header
					coap_header->Code             = COAP_CODE_RESP_CHANGED;
					outcome                       = E_SUCCESS;
				}
				else{
					coap_header->Code             = COAP_CODE_RESP_NOTFOUND;
					outcome                       = E_FAIL;
				}
			}
			else{
				outcome                       = E_FAIL;
			}
			break;

		case COAP_CODE_REQ_DELETE:

			if(posQuery<MAX_COAP_OPTIONS){
				uint8_t key=0;
				if (comi_getKey(&coap_options[posQuery],&key)==E_FAIL || key>=maxActiveSlots){
					outcome = E_FAIL;
					break;
				}

				msg->payload                     = &(msg->packet[127]);
				msg->length                      = 0;

				if(schedule_removeActiveSlotByID(key)==E_SUCCESS){
					// set the CoAP header
					coap_header->Code             = COAP_CODE_RESP_DELETED;
					outcome                       = E_SUCCESS;
				}
				else{
					coap_header->Code             = COAP_CODE_RESP_NOTFOUND;
					outcome                       = E_FAIL;
				}

				break;
			}
			outcome = E_FAIL;
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
				uint8_t comi_payload[MAX_COAP_PAYLOAD_SIZE];
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

				opencoap_addPayloadMarker(msg);

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

owerror_t comi_routingtable_receive(
		OpenQueueEntry_t* msg,
		coap_header_iht*  coap_header,
		coap_option_iht*  coap_options
) {

	owerror_t            outcome;
	uint8_t 			comi_payload[MAX_COAP_PAYLOAD_SIZE];
	uint8_t 			comi_payload_length=0;
	uint8_t 			posQuery =getOptionPosition(coap_options,COAP_OPTION_NUM_URIQUERY);


	switch (coap_header->Code) {
		case COAP_CODE_REQ_GET:

			// reset packet payload
			msg->payload                     = &(msg->packet[127]);
			msg->length                      = 0;

			//STEP 2: Check for query, return values with corresponding key
			if(posQuery<MAX_COAP_OPTIONS){
				uint8_t key=0;
				if (comi_getKey(&coap_options[posQuery],&key)==E_FAIL || key>0){ // todo change 0 with max route number
					outcome = E_FAIL;
					break;
				}
					comi_payload_length=comi_payload_length+comi_serialize_route(&comi_payload[comi_payload_length],key);
			}
			else{
				uint8_t i;
				for(i=0;i<1;i++){
					comi_payload_length=comi_payload_length+comi_serialize_route(&comi_payload[comi_payload_length],i);
				}
			}

			packetfunctions_reserveHeaderSize(msg,comi_payload_length);
			memcpy(&msg->payload[0],&comi_payload[0],comi_payload_length);

			opencoap_addPayloadMarker(msg);

			packetfunctions_reserveHeaderSize(msg,2);
			msg->payload[0]     = COAP_OPTION_NUM_CONTENTFORMAT << 4 | 1;
			msg->payload[1]     = COAP_MEDTYPE_CBOR;


			// set the CoAP header
			coap_header->Code                = COAP_CODE_RESP_CONTENT;
			outcome = E_SUCCESS;
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

	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_diffasn-SID_comi_celllist);
	size = size + cbor_serialize_uint16(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->currentScheduleEntry->lastUsedAsn.bytes0and1-c6top_vars.comi_celllist.schedule_vars->scheduleBuf[cell_id].lastUsedAsn.bytes0and1);
	return size;
}


/* deserialize cell */
owerror_t comi_deserialize_cell(OpenQueueEntry_t* msg, scheduleEntry_t* cell, uint8_t* cellID, int baseSID)
{
	cell->isHard=TRUE;				//default
	cell->shared=FALSE; 			//default
	cell->label=0; 					//default
	cell->type = CELLTYPE_OFF; 		//default
	uint8_t numObjects=0;
	cbor_stream_t cbor_stream;
	cbor_stream.data=&msg->payload[0];
	cbor_stream.size=msg->length;
	cbor_stream.pos=0;

	cbor_stream.pos = cbor_stream.pos + cbor_deserialize_map(&cbor_stream, cbor_stream.pos, &numObjects);

	uint8_t curser=0;
	int value;
	uint8_t sid=0;
	uint8_t readBytes;
	uint8_t myoption[1];
	uint8_t neighboraddress[1];
	while(curser<numObjects){
			cbor_stream.pos=cbor_stream.pos+cbor_deserialize_int(&cbor_stream,cbor_stream.pos,&value);
			sid=(uint8_t) value;
			switch(sid){
			case 1:
					readBytes=cbor_deserialize_int(&cbor_stream,cbor_stream.pos,&value);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;
						*cellID = (uint8_t) value;					// cell id

					}
					else{
						return E_FAIL;
					}
				break;
			case 2:
					readBytes=cbor_deserialize_int(&cbor_stream,cbor_stream.pos,&value);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;
					//	uint8_t slotframeID = (uint8_t) value;			// slotframe id
					}
					else{
						return E_FAIL;
					}
				break;
			case 3:
					readBytes=cbor_deserialize_int(&cbor_stream,cbor_stream.pos,&value);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;
						cell->slotOffset = (uint8_t) value;				// slotoffset
					}
					else{
						return E_FAIL;
					}
				break;
			case 4:
					readBytes=cbor_deserialize_int(&cbor_stream,cbor_stream.pos,&value);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;
						cell->channelOffset = (uint8_t) value;				// choffset
					}
					else{
						return E_FAIL;
					}
				break;
			case 5:
					readBytes=cbor_deserialize_byte(&cbor_stream,cbor_stream.pos,&myoption[0]);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;				// link options
						cell->type = get_cellType(myoption[0]);
						cell->isHard=TRUE;
						cell->shared=((myoption[0] & 0x20)>0);

						if((myoption[0] & 0x08)>0){
							cell->label=COMPONENT_UFIRE;
						}
					}
					else{
						return E_FAIL;
					}
				break;
			case 6:
				readBytes=cbor_deserialize_byte(&cbor_stream,cbor_stream.pos,&neighboraddress[0]);
					if(readBytes > 0){
						cbor_stream.pos=cbor_stream.pos+readBytes;				// address
						if(getNeighborAddressFromLastByte(&cell->neighbor, neighboraddress[0])==FALSE){
							openserial_printError(COMPONENT_NEIGHBORS,ERR_NO_NEIGHBOR,
							                               (errorparameter_t)neighboraddress[0],
							                               (errorparameter_t)0);
							return E_FAIL;
						}
					}
					else{
						return E_FAIL;
					}
				break;
			default:
				return E_FAIL;
				break;

			}

			curser++;
	}
	return E_SUCCESS;

}

/* serialize neighbor with neighborid */
uint8_t comi_serialize_neighbor(uint8_t* comi_payload_curser, uint8_t neighbor_id)
{
	uint8_t size=0;

	comi_payload_curser[size] = cbor_serialize_map(0x05);
	size++;

	// write neighborid
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_neighborid-SID_comi_neighborlist);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], neighbor_id);

	// write last byte of neighboraddress
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_neighborAddr-SID_comi_neighborlist);
	size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].addr_64b.addr_64b[7],1);

	// write rssi
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_rssi-SID_comi_neighborlist);
	size = size + cbor_serialize_int8(&comi_payload_curser[size], c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].rssi);

	// write statistics
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_lqi-SID_comi_neighborlist);
	uint8_t stats=(uint8_t)(100*c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].numTxACK/c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].numTx);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size],stats );

	// write asn
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_neigh_diffasn-SID_comi_neighborlist);
	size = size + cbor_serialize_uint16(&comi_payload_curser[size], c6top_vars.comi_celllist.schedule_vars->currentScheduleEntry->lastUsedAsn.bytes0and1-c6top_vars.comi_neighborlist.neighbors_vars->neighbors[neighbor_id].asn.bytes0and1);

	return size;
}


/* serialize route with route id */
uint8_t comi_serialize_route(uint8_t* comi_payload_curser, uint8_t route_id)
{
	uint8_t size=0;

	comi_payload_curser[size] = cbor_serialize_map(0x05);
	size++;

	// write routeid
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_routeid-SID_comi_routingtable);
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], route_id);

	// write last byte of parentaddress
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_parentaddr-SID_comi_routingtable);
	size = size + cbor_serialize_byte(&comi_payload_curser[size], &c6top_vars.comi_neighborlist.neighbors_vars->neighbors[c6top_vars.comi_routingtable.icmpv6rpl_vars->ParentIndex].addr_64b.addr_64b[7],1);

	// write dagrank
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_dagrank-SID_comi_routingtable);
	size = size + cbor_serialize_uint16(&comi_payload_curser[size], c6top_vars.comi_routingtable.icmpv6rpl_vars->myDAGrank);

	// write rankincrease
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_rankrincrease-SID_comi_routingtable);
	size = size + cbor_serialize_uint16(&comi_payload_curser[size],c6top_vars.comi_routingtable.icmpv6rpl_vars->rankIncrease);

	// write route options
	size = size + cbor_serialize_uint8(&comi_payload_curser[size], SID_comi_routeoptions-SID_comi_routingtable);
	uint8_t options=0x00;
	size = size + cbor_serialize_byte(&comi_payload_curser[size], &options,1);

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

	if(schedule->isHard==TRUE){
		linkoption=linkoption + 0x04;
	}

	if(schedule->label>0){
		linkoption=linkoption + 0x08;
	}
	// normal/advertising to be added
	return linkoption;
}

uint8_t get_cellType(uint8_t linkoption)
{
	uint8_t cellType = CELLTYPE_OFF;
	if((linkoption & 0x80)>0 && (linkoption & 0x40)>0){
		cellType=CELLTYPE_TXRX;
	} else if((linkoption & 0x80)>0){
		cellType=CELLTYPE_TX;
	} else if((linkoption & 0x40)>0){
		cellType=CELLTYPE_RX;
	}
	return cellType;
}

uint8_t get_Label(uint8_t linkoption)
{
	uint8_t label = 0;
	if((linkoption & 0x08)>0){
		label=COMPONENT_UFIRE;
	}
	return label;
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
		   openserial_printError(COMPONENT_COMI,ERR_BUSY_SENDING,
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
	      uint8_t comi_payload[MAX_COAP_PAYLOAD_SIZE];
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

	      opencoap_addPayloadMarker(pkt);

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
