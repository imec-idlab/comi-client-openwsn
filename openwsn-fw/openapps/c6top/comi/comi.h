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
