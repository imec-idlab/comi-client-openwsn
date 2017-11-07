#include "opendefs.h"
#include "topology.h"
#include "idmanager.h"

//=========================== defines =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//=========================== public ==========================================

/**
\brief Force a topology.

This function is used to force a certain topology, by hard-coding the list of
acceptable neighbors for a given mote. This function is invoked each time a
packet is received. If it returns FALSE, the packet is silently dropped, as if
it were never received. If it returns TRUE, the packet is accepted.

Typically, filtering packets is done by analyzing the IEEE802.15.4 header. An
example body for this function which forces a topology is:

   switch (idmanager_getMyID(ADDR_64B)->addr_64b[7]) {
      case TOPOLOGY_MOTE1:
         if (ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE2) {
            returnVal=TRUE;
         } else {
            returnVal=FALSE;
         }
         break;
      case TOPOLOGY_MOTE2:
         if (ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE1 ||
             ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE3) {
            returnVal=TRUE;
         } else {
            returnVal=FALSE;
         }
         break;
      default:
         returnVal=TRUE;
   }
   return returnVal;

By default, however, the function should return TRUE to *not* force any
topology.

\param[in] ieee802514_header The parsed IEEE802.15.4 MAC header.

\return TRUE if the packet can be received.
\return FALSE if the packet should be silently dropped.
*/

#define TOPOLOGY_MOTE1 0x38
#define TOPOLOGY_MOTE2 0x8b
#define TOPOLOGY_MOTE3 0xbe
#define TOPOLOGY_MOTE4 0x22
#define TOPOLOGY_MOTE5 0x55
#define TOPOLOGY_MOTE6 0x2a
#define TOPOLOGY_MOTE7 0x4e


bool topology_isAcceptablePacket(ieee802154_header_iht* ieee802514_header) {
#ifdef FORCETOPOLOGY
   bool returnVal;
   
   returnVal=FALSE;
   switch (idmanager_getMyID(ADDR_64B)->addr_64b[7]) {
      case TOPOLOGY_MOTE1:
         if (
               ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE2 || ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE3
            ) {
            returnVal=TRUE;
         }
         break;
      case TOPOLOGY_MOTE2:
         if (
        		 ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE1 ||
				 ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE4 ||
				 ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE5
            ) {
            returnVal=TRUE;
         }
         break;
      case TOPOLOGY_MOTE3:
         if (
               ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE1
            ) {
            returnVal=TRUE;
         }
         break;   
      case TOPOLOGY_MOTE4:
         if (
               ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE2 ||
               ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE6 ||
               ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE7
            ) {
            returnVal=TRUE;
         }
         break;
      case TOPOLOGY_MOTE5:
    	  if (
    			  ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE2
    	  ) {
    		  returnVal=TRUE;
    	  }
    	  break;
      case TOPOLOGY_MOTE6:
    	  if (
    			  ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE4
    	  ) {
    		  returnVal=TRUE;
    	  }
    	  break;
      case TOPOLOGY_MOTE7:
    	  if (
    			  ieee802514_header->src.addr_64b[7]==TOPOLOGY_MOTE4
    	  ) {
    		  returnVal=TRUE;
    	  }
    	  break;
   }
   return returnVal;
#else
	return TRUE;
#endif
}

//=========================== private =========================================
