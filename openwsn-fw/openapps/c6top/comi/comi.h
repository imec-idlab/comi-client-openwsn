/**
\brief CoAP Management Interface Application .

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


//=========================== define ==========================================

#define COMI_PATH_SIZE 2

// SIDs and corresponding URIs for different resources
static const uint8_t comi_path[]="c";
static const uint8_t querypref[]="k=";



//=========================== typedef =========================================

typedef uint16_t sid_t;


//=========================== variables =======================================


typedef struct {
   coap_resource_desc_t desc;
} comi_vars_t;

//=========================== Data Model =======================================


//=========================== prototypes ======================================

void comi_init(void);
uint8_t comi_getKey(coap_option_iht* coap_option, uint8_t* id);
uint8_t str2uint8 (const char* str, uint8_t len);
/**
\}
\}
*/


#endif
