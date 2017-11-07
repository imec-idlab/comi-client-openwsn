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

#define CBOR_TYPE(stream, offset) (stream->data[offset] & CBOR_TYPE_MASK)
#define CBOR_ADDITIONAL_INFO(stream, offset) (stream->data[offset] & CBOR_INFO_MASK)

#define CBOR_ENSURE_SIZE_READ(stream, bytes) do { \
    if (bytes > stream->size) { return 0; } \
} while(0)


uint8_t decode_int(const cbor_stream_t *s, uint8_t offset, uint64_t *val);
unsigned char uint_bytes_follow(unsigned char additional_info);
//=========================== public ==========================================

uint8_t cbor_serialize_array(uint8_t array_length)
{
    /* serialize number of array items */
	uint8_t code=CBOR_ARRAY|array_length;
    return code;
}


uint8_t cbor_deserialize_array(const cbor_stream_t *s, uint8_t offset, uint8_t *array_length)
{
    if (CBOR_TYPE(s, offset) != CBOR_ARRAY) {
        return 0;
    }

    uint64_t val;
    uint8_t read_bytes = decode_int(s, offset, &val);
    *array_length = (uint8_t)val;
    return read_bytes;
}


uint8_t cbor_serialize_map(uint8_t map_length)
{
    /* serialize number of item key-value pairs */
	uint8_t code=CBOR_MAP|map_length;
    return code;
}

uint8_t cbor_deserialize_map(const cbor_stream_t *s, uint8_t offset, uint8_t *map_length)
{
    if (CBOR_TYPE(s, offset) != CBOR_MAP) {
        return 0;
    }
    uint64_t val;
    uint8_t read_bytes = decode_int(s, offset, &val);
    *map_length = (uint8_t)val;
    return read_bytes;
}

uint8_t cbor_serialize_byte(uint8_t* payload, uint8_t* value, uint8_t length){
	uint8_t size=0;
	payload[size]=CBOR_BYTES|length;
	size++;
	memcpy(&payload[size],&value[0],length);
	size=size+length;
	return size;
}

uint8_t cbor_serialize_int8(uint8_t* payload, int8_t value){
	if (value >= 0) {
	        /* Major type 0: an unsigned integer */
	        return cbor_serialize_uint8(payload, value);
	    }
	    else {
	        /* Major type 1: an negative integer */
	        return cbor_serialize_negint8(payload, value);
	}
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

uint8_t cbor_serialize_negint8(uint8_t* payload, int8_t value){
	uint8_t size=0;
	uint8_t posValue=-1-value;

	 if (posValue < CBOR_UINT8_FOLLOWS) {
			payload[size]=CBOR_NEGINT |  posValue;
			size++;
	    }
	 else if (posValue <= 0xff) {
			payload[size]=CBOR_NEGINT |  CBOR_UINT8_FOLLOWS;
			size++;
	 }
	payload[size]=(uint8_t)posValue & 0x00ff;
	size++;
	return size;
}
uint8_t cbor_deserialize_int(const cbor_stream_t *stream, uint8_t offset, int *val)
{
    CBOR_ENSURE_SIZE_READ(stream, offset + 1);

    if ((CBOR_TYPE(stream, offset) != CBOR_UINT && CBOR_TYPE(stream, offset) != CBOR_NEGINT) || !val) {
        return 0;
    }

    uint64_t buf;
    uint8_t read_bytes = decode_int(stream, offset, &buf);

    if (!read_bytes) {
        return 0;
    }

    if (CBOR_TYPE(stream, offset) == CBOR_UINT) {
        *val = buf; /* resolve as CBOR_UINT */
    }
    else {
        *val = -1 - buf; /* resolve as CBOR_NEGINT */
    }

    return read_bytes;
}

uint8_t cbor_deserialize_byte(const cbor_stream_t *stream, uint8_t offset, uint8_t *val)
{
    CBOR_ENSURE_SIZE_READ(stream, offset + 1);

    if (CBOR_TYPE(stream, offset) != CBOR_BYTES) {
        return 0;
    }

    CBOR_ENSURE_SIZE_READ(stream, offset + 1);

       if ((CBOR_TYPE(stream, offset) != CBOR_BYTES && CBOR_TYPE(stream, offset) != CBOR_TEXT) || !val) {
           return 0;
       }
       uint8_t length=CBOR_ADDITIONAL_INFO(stream, offset);
       uint64_t bytes_length;
       size_t bytes_start = decode_int(stream, offset, &bytes_length);

       if (!bytes_start) {
           return 0;
       }

       if (length + 1 < bytes_length) {
           return 0;
       }

       CBOR_ENSURE_SIZE_READ(stream, offset + bytes_start + bytes_length);

       memcpy(val, &stream->data[offset + bytes_start], bytes_length);
       val[bytes_length] = '\0';
       return (bytes_start + bytes_length);
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


//===================== Private ===============================

uint8_t decode_int(const cbor_stream_t *s, uint8_t offset, uint64_t *val)
{
    if (!s) {
        return 0;
    }
    *val = 0; /* clear val first */

    CBOR_ENSURE_SIZE_READ(s, offset + 1);

    unsigned char *in = &s->data[offset];
    unsigned char additional_info = CBOR_ADDITIONAL_INFO(s, offset);
    unsigned char bytes_follow = uint_bytes_follow(additional_info);

    CBOR_ENSURE_SIZE_READ(s, offset + 1 + bytes_follow);

    switch (bytes_follow) {
        case 0:
            *val = (in[0] & CBOR_INFO_MASK);
            break;

        case 1:
            *val = in[1];
            break;

        default:
            *val = ((in[1]<<8)&0x00ff00)+((in[2])&0x0000ff);
            break;
    }
    return bytes_follow + 1;
}

unsigned char uint_bytes_follow(unsigned char additional_info)
{
    if (additional_info < CBOR_UINT8_FOLLOWS || additional_info > CBOR_UINT64_FOLLOWS) {
        return 0;
    }

    static const unsigned char BYTES_FOLLOW[] = {1, 2, 4, 8};
    return BYTES_FOLLOW[additional_info - CBOR_UINT8_FOLLOWS];
}
