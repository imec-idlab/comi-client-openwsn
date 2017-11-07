#ifndef __CBOR_H
#define __CBOR_H

#include "opendefs.h"

//=========================== define ==========================================

//=========================== typedef =========================================


typedef struct {
    /** Array containing CBOR encoded data */
    uint8_t *data;
    /** Size of the array */
    uint8_t size;
    /** Index to the next free byte */
    size_t pos;
} cbor_stream_t;


//=========================== variables =======================================


//=========================== Data Model =======================================


//=========================== prototypes ======================================

uint8_t cbor_serialize_uint8(uint8_t* payload, uint8_t value);
uint8_t cbor_serialize_int8(uint8_t* payload, int8_t value);
uint8_t cbor_serialize_negint8(uint8_t* payload, int8_t value);
uint8_t cbor_deserialize_int(const cbor_stream_t *stream, uint8_t offset, int *val);
uint8_t cbor_serialize_array(uint8_t array_length);
uint8_t cbor_deserialize_array(const cbor_stream_t *s, uint8_t offset, uint8_t *array_length);
uint8_t cbor_serialize_map(uint8_t map_length);
uint8_t cbor_serialize_byte(uint8_t* payload, uint8_t* value, uint8_t length);
uint8_t cbor_deserialize_byte(const cbor_stream_t *stream, uint8_t offset, uint8_t *val);
uint8_t cbor_serialize_uint16(uint8_t* payload, uint16_t value);
/**
\}
\}
*/

#endif
