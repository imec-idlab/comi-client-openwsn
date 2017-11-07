/**
\brief This project runs the full OpenWSN stack.

\author Thomas Watteyne <watteyne@eecs.berkeley.edu>, August 2010
*/

#include "board.h"
#include "crypto_engine.h"
#include "scheduler.h"
#include "openstack.h"
#include "opendefs.h"
#include "idmanager.h"

int mote_main(void) {
   
   // initialize
   board_init();
   CRYPTO_ENGINE.init();
   scheduler_init();
   openstack_init();
   
   // indicate
//   open_addr_t  myPrefix;
//   myPrefix.type = ADDR_PREFIX;
//   myPrefix.prefix[0]   = 0xbb;
//   myPrefix.prefix[1]   = 0xbb;
//   myPrefix.prefix[2]   = 0x00;
//   myPrefix.prefix[3]   = 0x00;
//   myPrefix.prefix[4]   = 0x00;
//   myPrefix.prefix[5]   = 0x00;
//   myPrefix.prefix[6]   = 0x00;
//   myPrefix.prefix[7]   = 0x00;
//   idmanager_triggerRoot(&myPrefix);
   

   // start
   scheduler_start();

   return 0; // this line should never be reached
}

void sniffer_setListeningChannel(uint8_t channel){return;}
