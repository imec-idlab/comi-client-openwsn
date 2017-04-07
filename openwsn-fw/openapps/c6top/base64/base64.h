#ifndef __BASE64_H
#define __BASE64_H

#include <stdint.h>
#include <stdlib.h>

//=========================== define ==========================================

//=========================== typedef =========================================





//=========================== variables =======================================


//=========================== Data Model =======================================


//=========================== prototypes ======================================


uint8_t base64_encode(uint16_t SID,
					uint8_t *encoded_data);
uint16_t base64_decode(const char *data,
		uint8_t *input_length);
/**
\}
\}
*/

#endif
