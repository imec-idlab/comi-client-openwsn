#include "opendefs.h"
#include "openqueue.h"
#include "openserial.h"
#include "packetfunctions.h"
#include "IEEE802154E.h"
#include "ieee802154_security_driver.h"


//=========================== defination =====================================

#define HIGH_PRIORITY_QUEUE_ENTRY 5

//=========================== variables =======================================

openqueue_vars_t openqueue_vars;

//=========================== prototypes ======================================
void openqueue_reset_entry(OpenQueueEntry_t* entry);

//=========================== public ==========================================

//======= admin

/**
\brief Initialize this module.
*/
void openqueue_init() {
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++){
      openqueue_reset_entry(&(openqueue_vars.queue[i]));
   }
}

/**
\brief Trigger this module to print status information, over serial.

debugPrint_* functions are used by the openserial module to continuously print
status information about several modules in the OpenWSN stack.

\returns TRUE if this function printed something, FALSE otherwise.
*/
bool debugPrint_queue() {
   debugOpenQueueEntry_t output[QUEUELENGTH];
   uint8_t i;
   for (i=0;i<QUEUELENGTH;i++) {
      output[i].creator = openqueue_vars.queue[i].creator;
      output[i].owner   = openqueue_vars.queue[i].owner;
   }
   openserial_printStatus(STATUS_QUEUE,(uint8_t*)&output,QUEUELENGTH*sizeof(debugOpenQueueEntry_t));
   return TRUE;
}

//======= called by any component

/**
\brief Request a new (free) packet buffer.

Component throughout the protocol stack can call this function is they want to
get a new packet buffer to start creating a new packet.

\note Once a packet has been allocated, it is up to the creator of the packet
      to free it using the openqueue_freePacketBuffer() function.

\returns A pointer to the queue entry when it could be allocated, or NULL when
         it could not be allocated (buffer full or not synchronized).
*/
OpenQueueEntry_t* openqueue_getFreePacketBuffer(uint8_t creator) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   
   // refuse to allocate if we're not in sync
   if (ieee154e_isSynch()==FALSE && creator > COMPONENT_IEEE802154E){
     ENABLE_INTERRUPTS();
     return NULL;
   }
   
   // if you get here, I will try to allocate a buffer for you
   
   // if there is no space left for high priority queue, don't reserve
   if (openqueue_isHighPriorityEntryEnough()==FALSE && creator>COMPONENT_SIXTOP_RES){
      ENABLE_INTERRUPTS();
      return NULL;
   }
   
   // walk through queue and find free entry
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_NULL) {
         openqueue_vars.queue[i].creator=creator;
         openqueue_vars.queue[i].owner=COMPONENT_OPENQUEUE;
         ENABLE_INTERRUPTS(); 
         return &openqueue_vars.queue[i];
      }
   }
   ENABLE_INTERRUPTS();
   return NULL;
}


/**
\brief Free a previously-allocated packet buffer.

\param pkt A pointer to the previsouly-allocated packet buffer.

\returns E_SUCCESS when the freeing was succeful.
\returns E_FAIL when the module could not find the specified packet buffer.
*/
owerror_t openqueue_freePacketBuffer(OpenQueueEntry_t* pkt) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++) {
      if (&openqueue_vars.queue[i]==pkt) {
         if (openqueue_vars.queue[i].owner==COMPONENT_NULL) {
            // log the error
            openserial_printCritical(COMPONENT_OPENQUEUE,ERR_FREEING_UNUSED,
                                  (errorparameter_t)0,
                                  (errorparameter_t)0);
         }
         openqueue_reset_entry(&(openqueue_vars.queue[i]));
         ENABLE_INTERRUPTS();
         return E_SUCCESS;
      }
   }
   // log the error
   openserial_printCritical(COMPONENT_OPENQUEUE,ERR_FREEING_ERROR,
                         (errorparameter_t)0,
                         (errorparameter_t)0);
   ENABLE_INTERRUPTS();
   return E_FAIL;
}

/**
\brief Free all the packet buffers created by a specific module.

\param creator The identifier of the component, taken in COMPONENT_*.
*/
void openqueue_removeAllCreatedBy(uint8_t creator) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++){
      if (openqueue_vars.queue[i].creator==creator) {
         openqueue_reset_entry(&(openqueue_vars.queue[i]));
      }
   }
   ENABLE_INTERRUPTS();
}

/**
\brief Free all the packet buffers owned by a specific module.

\param owner The identifier of the component, taken in COMPONENT_*.
*/
void openqueue_removeAllOwnedBy(uint8_t owner) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++){
      if (openqueue_vars.queue[i].owner==owner) {
         openqueue_reset_entry(&(openqueue_vars.queue[i]));
      }
   }
   ENABLE_INTERRUPTS();
}

//======= called by RES

OpenQueueEntry_t* openqueue_sixtopGetSentPacket() {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_IEEE802154E_TO_SIXTOP &&
          openqueue_vars.queue[i].creator!=COMPONENT_IEEE802154E) {
         ENABLE_INTERRUPTS();
         return &openqueue_vars.queue[i];
      }
   }
   ENABLE_INTERRUPTS();
   return NULL;
}

OpenQueueEntry_t* openqueue_sixtopGetReceivedPacket() {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_IEEE802154E_TO_SIXTOP &&
          openqueue_vars.queue[i].creator==COMPONENT_IEEE802154E) {
         ENABLE_INTERRUPTS();
         return &openqueue_vars.queue[i];
      }
   }
   ENABLE_INTERRUPTS();
   return NULL;
}

//======= called by IEEE80215E

OpenQueueEntry_t* openqueue_macGetDataPacket(open_addr_t* toNeighbor, uint8_t isHard, uint8_t label) {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();

   if(isHard==0){ // Soft Cells can be used by any purpose
		// first to look the sixtop RES packet
		for (i=0;i<QUEUELENGTH;i++) {
		   if (openqueue_vars.queue[i].label==0 &&
			   openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
			   openqueue_vars.queue[i].creator==COMPONENT_SIXTOP_RES &&
			   (
				   (
					   toNeighbor->type==ADDR_64B &&
					   packetfunctions_sameAddress(toNeighbor,&openqueue_vars.queue[i].l2_nextORpreviousHop)
				   ) || toNeighbor->type==ADDR_ANYCAST
			   )
		   ){
			  ENABLE_INTERRUPTS();
			  return &openqueue_vars.queue[i];
		   }
		}
		 if (toNeighbor->type==ADDR_64B) {
		      // a neighbor is specified, look for a packet unicast to that neigbhbor
		      for (i=0;i<QUEUELENGTH;i++) {
		         if (openqueue_vars.queue[i].label==0 &&
		        	 openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
		             packetfunctions_sameAddress(toNeighbor,&openqueue_vars.queue[i].l2_nextORpreviousHop)
		          ) {
		            ENABLE_INTERRUPTS();
		            return &openqueue_vars.queue[i];
		         }
		      }
		   } else if (toNeighbor->type==ADDR_ANYCAST) {
		      // anycast case: look for a packet which is either not created by RES
		      // or an KA (created by RES, but not broadcast)
		      for (i=0;i<QUEUELENGTH;i++) {
		         if (openqueue_vars.queue[i].label==0 &&
		        		 openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
		             ( openqueue_vars.queue[i].creator!=COMPONENT_SIXTOP ||
		                (
		                   openqueue_vars.queue[i].creator==COMPONENT_SIXTOP &&
		                   packetfunctions_isBroadcastMulticast(&(openqueue_vars.queue[i].l2_nextORpreviousHop))==FALSE
		                )
		             )
		            ) {
		            ENABLE_INTERRUPTS();
		            return &openqueue_vars.queue[i];
		         }
		      }
		   }
   }
   else{ // Hard Cells can be only used by application data
	   if (toNeighbor->type==ADDR_64B) {
	    	  if(label==0){
	    		  // a neighbor is specified, look for a packet unicast to that neigbhbor created by application or forwarding messages
	  		      for (i=0;i<QUEUELENGTH;i++) {
	  		         if (openqueue_vars.queue[i].l2_frameType == IEEE154_TYPE_PRIORITY && openqueue_vars.queue[i].label==0 && (openqueue_vars.queue[i].creator > COMPONENT_SIXTOP_RES  || openqueue_vars.queue[i].creator == COMPONENT_IEEE802154E)&& openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
	  		            packetfunctions_sameAddress(toNeighbor,&openqueue_vars.queue[i].l2_nextORpreviousHop)
	  		          ) {
	  		            ENABLE_INTERRUPTS();
	  		            return &openqueue_vars.queue[i];
	  		         }
	  		      }
	    	  }
	    	  else{
	    		  for (i=0;i<QUEUELENGTH;i++) {
	    			  if (openqueue_vars.queue[i].l2_frameType == IEEE154_TYPE_PRIORITY && openqueue_vars.queue[i].label == label && openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
	    					  packetfunctions_sameAddress(toNeighbor,&openqueue_vars.queue[i].l2_nextORpreviousHop)
	    			  ) {
	    				  ENABLE_INTERRUPTS();
	    				  return &openqueue_vars.queue[i];
	    			  }
	    		  }
	    	  }
	  	}
   }

   ENABLE_INTERRUPTS();
   return NULL;
}

bool openqueue_isHighPriorityEntryEnough(){
    uint8_t i;
    uint8_t numberOfEntry;
    
    numberOfEntry = 0;
    for (i=0;i<QUEUELENGTH;i++) {
        if(openqueue_vars.queue[i].creator>COMPONENT_SIXTOP_RES){
            numberOfEntry++;
        }
    }
    
    if (numberOfEntry>QUEUELENGTH-HIGH_PRIORITY_QUEUE_ENTRY){
        return FALSE;
    } else {
        return TRUE;
    }
}

OpenQueueEntry_t* openqueue_macGetEBPacket() {
   uint8_t i;
   INTERRUPT_DECLARATION();
   DISABLE_INTERRUPTS();
   for (i=0;i<QUEUELENGTH;i++) {
      if (openqueue_vars.queue[i].owner==COMPONENT_SIXTOP_TO_IEEE802154E &&
          openqueue_vars.queue[i].creator==COMPONENT_SIXTOP              &&
          packetfunctions_isBroadcastMulticast(&(openqueue_vars.queue[i].l2_nextORpreviousHop))) {
         ENABLE_INTERRUPTS();
         return &openqueue_vars.queue[i];
      }
   }
   ENABLE_INTERRUPTS();
   return NULL;
}

//=========================== private =========================================

void openqueue_reset_entry(OpenQueueEntry_t* entry) {
   //admin
   entry->creator                      = COMPONENT_NULL;
   entry->owner                        = COMPONENT_NULL;
   entry->payload                      = &(entry->packet[127 - IEEE802154_SECURITY_TAG_LEN]); // Footer is longer if security is used
   entry->length                       = 0;
   entry->label                        = 0;
   //l4
   entry->l4_protocol                  = IANA_UNDEFINED;
   entry->l4_protocol_compressed       = FALSE;
   //l3
   entry->l3_destinationAdd.type       = ADDR_NONE;
   entry->l3_sourceAdd.type            = ADDR_NONE;
   //l2
   entry->l2_nextORpreviousHop.type    = ADDR_NONE;
   entry->l2_frameType                 = IEEE154_TYPE_UNDEFINED;
   entry->l2_retriesLeft               = 0;
   entry->l2_IEListPresent             = 0;
   entry->l2_payloadIEpresent          = 0;
   //l2-security
   entry->l2_securityLevel             = 0;
}
