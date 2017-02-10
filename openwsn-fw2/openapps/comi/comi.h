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

//=========================== variables =======================================

typedef struct {
   coap_resource_desc_t desc;
} comi_vars_t;

//=========================== prototypes ======================================

void comi_init(void);

/**
\}
\}
*/

#endif
