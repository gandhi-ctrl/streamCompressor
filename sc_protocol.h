#ifndef _SC_PROTOCOL_HEADER_H_
#define _SC_PROTOCOL_HEADER_H_

#include <stdint.h>

#define SC_DATA_HEADER      0xDA7A
#define SC_HUFFMAN_DECODER  0x40FF
#define SC_EXIT             0xE817

typedef struct{
    uint16_t header;
    uint16_t size;
} sc_header_t;

typedef struct{
    uint8_t bits;
    uint32_t pattern;
} sc_encoder_entry_t;

typedef struct{
    uint16_t size;
    sc_encoder_entry_t entries[1];
} sc_encoder_table_t;

#define SC_DECODER_JUMP_FLAG   0x80
#define SC_DECODER_BITS_MASK   0x0F

typedef struct{
    uint8_t flag_bits;
    uint8_t code;
} sc_decoder_entry_t;

typedef struct{
    uint16_t size;
    sc_decoder_entry_t entries[1];
} sc_decoder_table_t;

#endif // _SC_PROTOCOL_HEADER_H_