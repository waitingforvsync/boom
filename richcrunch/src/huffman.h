#ifndef HUFFMAN_H_
#define HUFFMAN_H_

#include <stdint.h>


typedef struct huffman_code_t {
    uint32_t code;
    uint8_t length;
} huffman_code_t;


#define TEMPLATE_ARRAY_NAME huffman_code_array
#define TEMPLATE_ARRAY_TYPE huffman_code_t
#include "array.template.h"


typedef struct huffman_tree_t {
    huffman_code_array_view_t codes;
} huffman_tree_t;


#define TEMPLATE_ARRAY_NAME huffman_freq
#define TEMPLATE_ARRAY_TYPE uint16_t
#include "array.template.h"


// Build a huffman encoding, mapping symbol id to a huffman code
huffman_tree_t huffman_tree_make(huffman_freq_view_t freqs, uint32_t max_code_length);


#endif // ifndef HUFFMAN_H_
