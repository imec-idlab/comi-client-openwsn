#ifndef __CBOR_H
#define __CBOR_H

#include "opendefs.h"

//=========================== define ==========================================

//=========================== typedef =========================================





//=========================== variables =======================================


//=========================== Data Model =======================================


//=========================== prototypes ======================================

uint8_t cbor_serialize_uint8(uint8_t* payload, uint8_t value);
uint8_t cbor_serialize_array(uint8_t array_length);
uint8_t cbor_serialize_map(uint8_t map_length);
uint8_t cbor_serialize_byte(uint8_t* payload, uint8_t* value, uint8_t length);
uint8_t cbor_serialize_uint16(uint8_t* payload, uint16_t value);
/**
\}
\}
*/

#endif
