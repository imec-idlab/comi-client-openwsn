/**
\brief CoAP Management Interface Application for 6top Protocol.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/
#ifndef __C6TOP_H
#define __C6TOP_H

/**
\addtogroup AppCoAP
\{
\addtogroup comi
\{
*/

#include "opendefs.h"
#include "comi/comi.h"
#include "opencoap.h"
#include "schedule.h"
#include "neighbors.h"
//=========================== define ==========================================


#define MAX_PAYLOAD_SIZE 128		// since no 6lowpan fragmentation and coap block transfer is implemented, we limit the max payload size


//=========================== typedef =========================================
typedef struct {
   uint8_t       TKL;
   uint8_t       token[COAP_MAX_TKL];
   open_addr_t   observer_Address;
   uint16_t   	 observer_port;
   uint16_t   	 optionID;
   opentimer_id_t observe_Neighbor_Timer_Id;
}observer_t;
typedef struct {
   coap_resource_desc_t desc;					// SID_comi_celllist
   coap_resource_desc_t cellId_desc;			// SID_comi_cellid
   coap_resource_desc_t slotFrameId_desc;		// SID_comi_slotframeid
   coap_resource_desc_t slotOffset_desc;		// SID_comi_slotoffset
   coap_resource_desc_t chOffset_desc;			// SID_comi_channeloffset
   coap_resource_desc_t linkOpt_desc;			// SID_comi_linkoption
   coap_resource_desc_t nodeAddr_desc;			// SID_comi_nodeaddr
   coap_resource_desc_t stats_desc;				// SID_comi_stats
   coap_resource_desc_t lastasn_desc;				// SID_comi_lastasn
   schedule_vars_t* schedule_vars;
} comi_celllist_res_t;

typedef struct {
   coap_resource_desc_t desc;					// SID_comi_neighborlist
   coap_resource_desc_t neighborAddr_desc;		// SID_comi_neighborAddress
   coap_resource_desc_t rssi_desc;				// SID_comi_rssi
   coap_resource_desc_t lqi_desc;				// SID_comi_lqi
   coap_resource_desc_t asn_desc;				// SID_comi_asn
   observer_t neighbor_observer;
   neighbors_vars_t* neighbors_vars;
} comi_neighborlist_res_t;


//=========================== variables =======================================

static const 	sid_t SID_comi_celllist = 4001;
static const	sid_t SID_comi_cellid = 4002;
static const	sid_t SID_comi_slotframeid = 4003;
static const 	sid_t SID_comi_slotoffset = 4004;
static const 	sid_t SID_comi_choffset = 4005;
static const 	sid_t SID_comi_linkoption = 4006;
static const 	sid_t SID_comi_nodeaddr = 4007;
static const 	sid_t SID_comi_stats = 4008;
static const 	sid_t SID_comi_lastasn = 4009;

static const 	sid_t SID_comi_neighborlist = 4011;
static const	sid_t SID_comi_neighborAddr = 4012;
static const	sid_t SID_comi_rssi = 4013;
static const 	sid_t SID_comi_lqi = 4014;
static const 	sid_t SID_comi_asn = 4015;


typedef struct {
   coap_resource_desc_t desc;
   comi_neighborlist_res_t comi_neighborlist;
   comi_celllist_res_t comi_celllist;
} c6top_vars_t;

//=========================== Data Model =======================================


//=========================== prototypes ======================================

void c6top_init(void);

uint8_t comi_serialize_cell(uint8_t* comi_payload_curser, uint8_t cell_id);
uint8_t comi_serialize_slotoffset(uint8_t* comi_payload_curser, sid_t baseSID, uint8_t cell_id);
uint8_t comi_serialize_neighbor(uint8_t* comi_payload_curser, uint8_t neighbor_id);
uint8_t get_linkoption(scheduleEntry_t* schedule);

void    c6top_sendDone(OpenQueueEntry_t* msg, owerror_t error);;

#endif
