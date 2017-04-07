#include "cbor.h"

#include "opendefs.h"

//=========================== defines =========================================

#define CBOR_TYPE_MASK          0xE0    /* top 3 bits */
#define CBOR_INFO_MASK          0x1F    /* low 5 bits */

#define CBOR_BYTE_FOLLOWS       24      /* indicator that the next byte is part of this item */


#define CBOR_UINT       0x00            /* type 0 */
#define CBOR_NEGINT     0x20            /* type 1 */
#define CBOR_BYTES      0x40            /* type 2 */
#define CBOR_TEXT       0x60            /* type 3 */
#define CBOR_ARRAY      0x80            /* type 4 */
#define CBOR_MAP        0xA0            /* type 5 */
#define CBOR_TAG        0xC0            /* type 6 */
#define CBOR_7          0xE0            /* type 7 (float and other types) */


#define CBOR_UINT8_FOLLOWS      24      /* 0x18 */
#define CBOR_UINT16_FOLLOWS     25      /* 0x19 */
#define CBOR_UINT32_FOLLOWS     26      /* 0x1a */
#define CBOR_UINT64_FOLLOWS     27      /* 0x1b */

#define CBOR_VAR_FOLLOWS        31      /* 0x1f */

#define CBOR_DATETIME_STRING_FOLLOWS        0
#define CBOR_DATETIME_EPOCH_FOLLOWS         1


#define CBOR_FALSE      (CBOR_7 | 20)
#define CBOR_TRUE       (CBOR_7 | 21)
#define CBOR_NULL       (CBOR_7 | 22)
#define CBOR_UNDEFINED  (CBOR_7 | 23)

#define CBOR_FLOAT16    (CBOR_7 | 25)
#define CBOR_FLOAT32    (CBOR_7 | 26)
#define CBOR_FLOAT64    (CBOR_7 | 27)
#define CBOR_BREAK      (CBOR_7 | 31)
//=========================== variables =======================================


//=========================== prototypes ======================================

//=========================== public ==========================================

uint8_t cbor_serialize_array(uint8_t array_length)
{
    /* serialize number of array items */
	uint8_t code=CBOR_ARRAY|array_length;
    return code;
}

uint8_t cbor_serialize_map(uint8_t map_length)
{
    /* serialize number of item key-value pairs */
	uint8_t code=CBOR_MAP|map_length;
    return code;
}

uint8_t cbor_serialize_byte(uint8_t* payload, uint8_t* value, uint8_t length){
	uint8_t size=0;
	payload[size]=CBOR_BYTES|length;
	size++;
	memcpy(&payload[size],&value[0],length);
	size=size+length;
	return size;
}

uint8_t cbor_serialize_uint8(uint8_t* payload, uint8_t value){
	uint8_t size=0;
	if ((((value & 0x00ff) >> 4 ) & 0x0f) > 0){
		payload[size]=CBOR_UINT8_FOLLOWS;
		size++;
	}
	payload[size]=(uint8_t)value & 0x00ff;
	size++;
	return size;
}

uint8_t cbor_serialize_uint16(uint8_t* payload, uint16_t value){
	uint8_t size=0;
	if ((((value & 0x00ff) >> 8 ) & 0xff) > 0){
		payload[size]=CBOR_UINT16_FOLLOWS;
		size++;
		payload[size]=(uint8_t)(value >> 8 & 0x00ff);
		size++;
	}
	else if((((value & 0x00ff) >> 4 ) & 0x0f) > 0){
		payload[size]=CBOR_UINT8_FOLLOWS;
		size++;
	}
	payload[size]=(uint8_t)(value & 0x00ff);
	size++;
	return size;
}
