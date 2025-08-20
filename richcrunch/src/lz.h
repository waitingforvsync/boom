#ifndef LZ_H_
#define LZ_H_

#include "arena.h"
#include "byte_array.h"
#include "refs.h"
#include "token.h"


typedef struct lz_item_t {
    token_t token;
    uint32_t total_cost;
    uint32_t tally;
} lz_item_t;


#define TEMPLATE_ARRAY_NAME lz_item_array
#define TEMPLATE_ARRAY_TYPE lz_item_t
#include "array.template.h"



typedef struct lz_parse_result_t {
    lz_item_array_view_t items;
    uint32_t cost;
    uint32_t num_fixed_bits;
} lz_parse_result_t;


// Perform an optimal lz parse
lz_parse_result_t lz_parse(const refs_t *refs, arena_t *arena, arena_t scratch);

// Dump the lz result in a readable format
void lz_dump(const lz_parse_result_t *lz, const char *filename);

// Serialise the lz result to a bitstream
byte_array_view_t lz_serialise(const lz_parse_result_t *lz, arena_t *arena);

// Deserialise the compressed bitstream
byte_array_view_t lz_deserialise(byte_array_view_t compressed, arena_t *arena);


#endif // ifndef LZ_H_
