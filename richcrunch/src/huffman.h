#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "arena.h"
#include "byte_array.h"
#include "uint16_array.h"
#include <stdint.h>


typedef struct huffman_symbol_t {
    uint32_t code;
    uint8_t length;
} huffman_symbol_t;


#define TEMPLATE_ARRAY_NAME huffman_symbol_array
#define TEMPLATE_ARRAY_TYPE huffman_symbol_t
#include "array.template.h"


typedef struct huffman_code_t {
    byte_array_view_t symbol_lengths;
} huffman_code_t;


// Build a huffman encoding, mapping symbol id to a huffman code
huffman_code_t huffman_code_make(uint16_array_view_t freqs, uint32_t max_code_length, arena_t *arena, arena_t scratch);


#endif // ifndef HUFFMAN_H_
