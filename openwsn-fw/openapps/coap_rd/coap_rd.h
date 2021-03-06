#ifndef __COAP_RD_H
#define __COAP_RD_H

/**
\brief CoAP RD registration applications

\author Abdulkadir Karaagac <abdulkadir.karaagac@intec.ugent.be>, February 2017.
*/

/**
\addtogroup AppCoAP
\{
\addtogroup coap_rd
\{
*/

#include "opendefs.h"
#include "opentimers.h"
#include "opencoap.h"
#include "opensensors.h"
#include "sensors.h"

typedef char uri_t;

//=========================== define ==========================================

#define RDPERIOD  50000
//=========================== typedef =========================================

typedef struct {
   coap_resource_desc_t desc;
   uint8_t lifetime;
   opentimer_id_t timerId;
   uint16_t period;
   uint8_t ipv6address[16];
} coap_rd_server_t;

//=========================== variables =======================================


//=========================== prototypes ======================================
void coap_rd_init();

/**
\}
\}
*/

void coap_register_server_cb(opentimer_id_t id);
void coap_simple_register_rd(void);
void coap_long_register_rd(void);
owerror_t coap_rd_receive(OpenQueueEntry_t* msg, coap_header_iht*  coap_header, coap_option_iht*  coap_options);
void coap_rd_sendDone(OpenQueueEntry_t* msg, owerror_t error);

#endif
