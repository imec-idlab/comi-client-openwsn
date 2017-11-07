#ifndef __ufire_H
#define __ufire_H

/**
\addtogroup AppUdp
\{
\addtogroup ufire
\{
*/

#include "opentimers.h"
#include "openudp.h"

//=========================== define ==========================================
#define UFIRE_PERIOD_FRAME 5
#define UFIRE_PACKET_NUM 1000
//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   uint8_t				frameCounter; /// number of
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   udp_resource_desc_t     desc;  ///< resource descriptor for this module, used to register at UDP stack
} ufire_vars_t;

//=========================== prototypes ======================================

void ufirealarm_init(void);
void ufire_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void ufire_receive(OpenQueueEntry_t* msg);

/**
\}
\}
*/

#endif
