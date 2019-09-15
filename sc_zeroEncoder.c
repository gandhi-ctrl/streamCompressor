#include <stdlib.h>

#include "sc_zeroEncoder.h"

sc_encoder_table_t* _sc_creatZeroEncoding(){
    sc_encoder_table_t* ret = malloc(sizeof(sc_encoder_table_t) +256 *sizeof(sc_encoder_entry_t));
    if(ret != NULL){
        ret->size = 256;
        for(int n = 0; n < 256; n++){
            ret->entries[n].bits = 8;
            ret->entries[n].pattern = n;
        }
    }
    return ret;
}

sc_decoder_table_t* _sc_creatZeroDecoding(){
    sc_decoder_table_t* ret = malloc(sizeof(sc_decoder_table_t) +256 *sizeof(sc_decoder_entry_t));
    if(ret != NULL){
        ret->size = 256;
        for(int n = 0; n < 256; n++){
            ret->entries[n].flag_bits = 8;
            ret->entries[n].code = n;
        }
    }
    return ret;
}