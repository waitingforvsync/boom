#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "arena.h"
#include "uint16_array.h"
#include "uint8_array.h"
#include <stdint.h>


typedef struct huffman_code_t {
    uint8_array_view_t symbol_lengths;
    uint16_array_view_t symbol_codes;
    uint8_array_view_t num_symbols_per_bit_length;
    uint16_array_view_t dictionary;
} huffman_code_t;


// Build a huffman encoding, mapping symbol id to a huffman code
huffman_code_t huffman_code_make(uint16_array_view_t freqs, uint32_t max_code_length, arena_t *arena, arena_t scratch);


#endif // ifndef HUFFMAN_H_
