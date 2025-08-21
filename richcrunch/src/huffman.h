#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include "arena.h"
#include "byte_array.h"
#include "uint16_array.h"
#include "uint8_array.h"
#include <stdint.h>


// Builds a huffman tree from the given symbol counts and returns the encoding bit lengths for each symbol
uint8_array_view_t huffman_build_code_lengths(uint16_array_view_t symbol_counts, uint32_t max_code_length, arena_t *arena, arena_t scratch);

// Gets the canonical huffman encoding for an alphabet with the given huffman code lengths.
// Note, the most significant set bit is not part of the code - it exists in order to define the canonical code length.
// e.g. huffman code '01' will be stored as 0b101 (5)
//      huffman code '1010' will be stored as 0b11010 (26)
// It should therefore be written with length get_bit_width(code) - 1
uint16_array_view_t huffman_get_canonical_encoding(uint8_array_view_t code_lengths, arena_t *arena);

// Provide a way of looking up an alphabet symbol from a code of an unknown number of bits
// This can be done by consuming one bit at a time and checking whether the current value is within
// the bounds of the dictionary size given by the number of codes for the current bit length.
// See bitreader_get_huffman_value() for an example of how this is done.
typedef struct huffman_decoder_t {
    uint8_array_view_t num_codes_of_length;
    uint16_array_view_t dictionary;
} huffman_decoder_t;

huffman_decoder_t huffman_decoder_make(uint8_array_view_t code_lengths, arena_t *arena);

// Serialise a huffman encoded block to a bitstream
byte_array_view_t huffman_serialise(byte_array_view_t src, arena_t *arena, arena_t scratch);

// Deserialise a huffman encoded block from a bitstream
byte_array_view_t huffman_deserialise(byte_array_view_t compressed, arena_t *arena, arena_t scratch);


#endif // ifndef HUFFMAN_H_
