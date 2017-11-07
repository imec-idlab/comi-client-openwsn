#ifndef __UMONITOR_H
#define __UMONITOR_H

/**
\addtogroup AppUdp
\{
\addtogroup umonitor
\{
*/

#include "opentimers.h"
#include "openudp.h"

//=========================== define ==========================================
#define UMONITOR_PERIOD_MS 500
#define UMONITOR_PACKET_NUM 1000
//=========================== typedef =========================================

//=========================== variables =======================================

typedef struct {
   uint16_t             packetnum;  ///< update packetnum for new period in order to make timing same for each period
   uint16_t             counter;  ///< incrementing counter which is written into the packet
   uint16_t              period;  ///< uinject packet sending period>
   opentimer_id_t       timerId;  ///< periodic timer which triggers transmission
   udp_resource_desc_t     desc;  ///< resource descriptor for this module, used to register at UDP stack
} umonitor_vars_t;

//=========================== prototypes ======================================

void umonitoralarm_init(void);
void umonitor_sendDone(OpenQueueEntry_t* msg, owerror_t error);
void umonitor_receive(OpenQueueEntry_t* msg);

/**
\}
\}
*/

#endif
