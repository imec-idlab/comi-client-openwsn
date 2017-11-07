/**
\brief Applications running on top of the OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, September 2014.
*/

#include "opendefs.h"

// CoAP
#include "c6t.h"
#include "c6top.h"
#include "cinfo.h"
#include "csensors.h"
#include "cleds.h"
#include "cexample.h"
#include "cstorm.h"
#include "cwellknown.h"
#include "lwm2m.h"
#include "lwm2m_dev.h"
#include "coap_rd.h"
#include "rrt.h"
// TCP
#include "techo.h"
// UDP
#include "uecho.h"
#include "uinject.h"
#include "ufirealarm.h"
#include "umonitor.h"

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

//=========================== private =========================================

void openapps_init(void) {

   // CoAP
  // c6t_init();
 //  cinfo_init();
  // cexample_init();
  // cleds__init();
   //cstorm_init();
   //csensors_init();
   cwellknown_init();
  // ufirealarm_init();
   umonitor_init();
  // rrt_init();
   // TCP
  // techo_init();
   c6top_init();
   coap_rd_init();
   // lwm2m_init();
 //  lwm2m_dev_init();

}
