#ifndef LZHUFF_H_
#define LZHUFF_H_

#include "arena.h"
#include "byte_array.h"
#include "huffman.h"
#include "token.h"
#include <stdint.h>


typedef struct lzhuff_item_t {
    token_t token;
    uint32_t total_cost;
} lzhuff_item_t;


#define TEMPLATE_ARRAY_NAME lzhuff_item_array
#define TEMPLATE_ARRAY_TYPE lzhuff_item_t
#include "array.template.h"


typedef struct lzhuff_result_t {
    huffman_code_t *huffman;
} lzhuff_result_t;


// Perform a lz+huffman parse
lzhuff_result_t lzhuff_parse(byte_array_view_t src, arena_t *arena, arena_t scratch);


#endif // ifndef LZHUFF_H_
