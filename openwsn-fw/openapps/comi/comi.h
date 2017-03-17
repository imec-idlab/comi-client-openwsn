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


#define NUMCELLS 11
//=========================== typedef =========================================
typedef uint8_t clotframeid_t;
typedef uint8_t clotframeid_t;
typedef uint32_t sid_t;
typedef enum {
   LINK_NORMAL                  = 0,
   LINK_ADVERTISING             = 1,
} linktype_t;

typedef struct {
   uint8_t numofstatistics;

} comi_statistics_container;

typedef struct {
   coap_resource_desc_t desc;
   clotframeid_t slotframeid;
   uint16_t slotoffset;
   uint16_t channeloffset;
   linktype_t linktype;
   cellType_t celltype;
   open_addr_t nodeaddress;
} comi_cell_t;

typedef struct {
   coap_resource_desc_t desc;
   comi_cell_t comi_cell[NUMCELLS]; //   uint16_t cellID: is the key!!
} comi_celllist_res_t;

typedef struct {
   coap_resource_desc_t desc;
   uint16_t freecell;
} comi_freecell_res_t;


//=========================== variables =======================================


typedef struct {
   coap_resource_desc_t desc;
   comi_celllist_res_t comi_celllist;
} comi_vars_t;

//=========================== Data Model =======================================

static const 	sid_t SID_comi_celllist = 60001;
static const	sid_t SID_comi_slotframeid = 60002;
static const 	sid_t SID_comi_slotoffset = 60003;
static const 	sid_t SID_comi_channeloffset = 60004;
static const 	sid_t SID_comi_linkoption = 60005;
static const 	sid_t SID_comi_linktype = 60006;
static const 	sid_t SID_comi_celltype = 60007;
static const 	sid_t SID_comi_nodeaddress = 60008;
//=========================== prototypes ======================================

void comi_init(void);

/**
\}
\}
*/
void comi_resource_register(comi_vars_t* comi_vars);
uint8_t comi_get_allTXandRX_Cells(void);
#endif
