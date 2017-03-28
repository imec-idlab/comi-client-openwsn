/**
\brief CoAP Management Interface Application.

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.

*/
#ifndef __COMI_H
#define __COMI_H

/**
\addtogroup AppCoAP
\{
\addtogroup comi
\{
*/

#include "opendefs.h"
#include "opencoap.h"
#include "schedule.h"



//=========================== define ==========================================

//=========================== typedef =========================================

typedef uint16_t sid_t;


typedef struct {
   coap_resource_desc_t desc;
   schedule_vars_t* schedule_vars;
} comi_celllist_res_t;


//=========================== variables =======================================


typedef struct {
   coap_resource_desc_t desc;
   comi_celllist_res_t comi_celllist;
} comi_vars_t;

//=========================== Data Model =======================================

static const 	sid_t SID_comi_celllist = 4001;
static const	sid_t SID_comi_cellid = 4002;
static const	sid_t SID_comi_slotframeid = 4003;
static const 	sid_t SID_comi_slotoffset = 4004;
static const 	sid_t SID_comi_channeloffset = 4005;
static const 	sid_t SID_comi_linkoption = 4006;
static const 	sid_t SID_comi_linktype = 4007;
static const 	sid_t SID_comi_celltype = 4008;
static const 	sid_t SID_comi_nodeaddress = 4009;
//=========================== prototypes ======================================

void comi_init(void);

/**
\}
\}
*/
void comi_resource_register(comi_vars_t* comi_vars);

#endif
