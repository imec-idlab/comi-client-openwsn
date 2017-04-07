#include "base64/base64.h"

#include "opendefs.h"

//=========================== defines =========================================

static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;

//=========================== variables =======================================


//=========================== prototypes ======================================
void build_decoding_table(void);
//=========================== public ==========================================



uint8_t base64_encode(uint16_t SID,
					uint8_t *encoded_data) {
    uint8_t output_length = (SID>=0)+((SID/64)>=1)+(((SID/64)/64)>=1);
    if (encoded_data == NULL) return NULL;
    uint8_t i;
    for (i = 0; i < output_length;i++) {
    	uint16_t pos=(output_length)-i-1;
    	if(pos<output_length){
    		encoded_data[pos] = (uint8_t)encoding_table[(SID >> (6*i)) & 0x3F];
       	}
    }

    return output_length;
}

uint16_t base64_decode(const char *data,uint8_t *input_length) {

    if (decoding_table == NULL) {
    	build_decoding_table();
    }
    uint16_t decoded_data = 0 ;
    uint8_t i;
    for (i = 0; i < *input_length;i++) {
        decoded_data = decoded_data + (decoding_table[*(data+i)] << (6*(*input_length-i-1)));
    }

    return decoded_data;
}



//=========================== private =========================================

void build_decoding_table() {

	memset(&decoding_table,0,256);
   // decoding_table = malloc(256);
    uint8_t i;
    for (i = 0; i < 64; i++)
        decoding_table[(unsigned char) encoding_table[i]] = i;
}


void base64_cleanup() {
    decoding_table=NULL;
}
