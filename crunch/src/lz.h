#ifndef LZ_H_
#define LZ_H_

#include "arena.h"
#include "refs.h"
#include "token.h"


typedef struct lz_result_t {
    token_array_view_t tokens;
    uint32_t cost;
} lz_result_t;


lz_result_t lz_parse(
    const refs_t *refs,
    uint32_t max_cost,
    uint32_t num_fixed_offset_bits,
    arena_t *arena,
    arena_t scratch
);


#endif // ifndef LZ_H_
