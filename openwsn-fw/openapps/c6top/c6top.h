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
#include "icmpv6rpl.h"

//=========================== define ==========================================


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
   coap_resource_desc_t desc;					// SID_comi_routingtable
   coap_resource_desc_t routeid_desc;			// SID_comi_routeid
   coap_resource_desc_t parentaddr_desc;		// SID_comi_parentaddr
   coap_resource_desc_t dagrank_desc;			// SID_comi_dagrank
   coap_resource_desc_t rankinc_desc;			// SID_comi_rankrincrease
   coap_resource_desc_t routeopt_desc;			// SID_comi_routeoptions
   icmpv6rpl_vars_t* icmpv6rpl_vars;
} comi_routingtable_res_t;

typedef struct {
   coap_resource_desc_t desc;					// SID_comi_celllist
   coap_resource_desc_t cellId_desc;			// SID_comi_cellid
   coap_resource_desc_t slotFrameId_desc;		// SID_comi_slotframeid
   coap_resource_desc_t slotOffset_desc;		// SID_comi_slotoffset
   coap_resource_desc_t chOffset_desc;			// SID_comi_channeloffset
   coap_resource_desc_t linkOpt_desc;			// SID_comi_linkoption
   coap_resource_desc_t nodeAddr_desc;			// SID_comi_nodeaddr
   coap_resource_desc_t stats_desc;				// SID_comi_stats
   coap_resource_desc_t lastasn_desc;			// SID_comi_lastasn
   schedule_vars_t* schedule_vars;
} comi_celllist_res_t;

typedef struct {
   coap_resource_desc_t desc;					// SID_comi_neighborlist
   coap_resource_desc_t neighborId_desc;		// SID_comi_neighborId
   coap_resource_desc_t neighborAddr_desc;		// SID_comi_neighborAddress
   coap_resource_desc_t rssi_desc;				// SID_comi_rssi
   coap_resource_desc_t lqi_desc;				// SID_comi_lqi
   coap_resource_desc_t asn_desc;				// SID_comi_asn
   observer_t neighbor_observer;
   neighbors_vars_t* neighbors_vars;
} comi_neighborlist_res_t;

typedef struct {
   uint8_t       length;                         // length in bytes of the payload
   uint8_t       cache[MAX_COAP_PAYLOAD_SIZE];            // 1B spi address, 1B length, 125B data, 2B CRC, 1B LQI
} payload_cache_t;

typedef struct {
   coap_resource_desc_t desc;
   comi_neighborlist_res_t comi_neighborlist;
   comi_celllist_res_t comi_celllist;
   comi_routingtable_res_t comi_routingtable;
   payload_cache_t coap_payload_cache;
} c6top_vars_t;

//=========================== variables =======================================

static const 	sid_t SID_comi_scheduler = 4000;
static const 	sid_t SID_comi_enabled = 4001;
static const 	sid_t SID_comi_celllist = 4002;
static const	sid_t SID_comi_cellid = 4003;
static const	sid_t SID_comi_slotframeid = 4004;
static const 	sid_t SID_comi_slotoffset = 4005;
static const 	sid_t SID_comi_choffset = 4006;
static const 	sid_t SID_comi_linkoption = 4007;
static const 	sid_t SID_comi_nodeaddr = 4008;
static const 	sid_t SID_comi_stats = 4009;
static const 	sid_t SID_comi_diffasn = 4010;

static const 	sid_t SID_comi_neighborlist = 4011;
static const 	sid_t SID_comi_neighborid = 4012;
static const	sid_t SID_comi_neighborAddr = 4013;
static const	sid_t SID_comi_rssi = 4014;
static const 	sid_t SID_comi_lqi = 4015;
static const 	sid_t SID_comi_neigh_diffasn = 4016;

static const 	sid_t SID_comi_routingtable = 4021;
static const	sid_t SID_comi_routeid = 4022;
static const	sid_t SID_comi_parentaddr = 4023;
static const	sid_t SID_comi_dagrank = 4024;
static const 	sid_t SID_comi_rankrincrease = 4025;
static const 	sid_t SID_comi_routeoptions = 4026;

//=========================== Data Model =======================================


//=========================== prototypes ======================================

void c6top_init(void);

uint8_t comi_serialize_cell(uint8_t* comi_payload_curser, uint8_t cell_id);
uint8_t comi_serialize_slotoffset(uint8_t* comi_payload_curser, sid_t baseSID, uint8_t cell_id);
uint8_t comi_serialize_neighbor(uint8_t* comi_payload_curser, uint8_t neighbor_id);
uint8_t comi_serialize_route(uint8_t* comi_payload_curser, uint8_t route_id);
owerror_t comi_deserialize_cell(OpenQueueEntry_t* msg, scheduleEntry_t* cell, uint8_t* cellID, int baseSID);
uint8_t get_linkoption(scheduleEntry_t* schedule);
uint8_t get_cellType(uint8_t linkoption);
void    c6top_sendDone(OpenQueueEntry_t* msg, owerror_t error);;

#endif
