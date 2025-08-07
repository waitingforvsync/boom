#include "lz.h"
#include "utils.h"
#include <assert.h>


uint32_t get_elias_gamma_cost(uint32_t value) {
    assert(value > 0 && value <= 256);  // 256 will be read as 0
    return get_bit_width(value) * 2 - 1;
}


uint32_t get_hybrid_cost(uint32_t value, uint32_t num_fixed_bits) {
    return get_elias_gamma_cost((value >> num_fixed_bits) + 1) + num_fixed_bits;
}


static uint32_t get_tally_cost(uint32_t value) {
    // tally is a simple elias gamma encoding of the length
    // the sentinel at the end has a tally of 0, so handle that appropriately
    return value ? get_elias_gamma_cost(value) : 0;
}


static uint32_t get_token_cost(token_t t, uint32_t num_fixed_bits) {
    return token_is_literal(t) ?
        8 :
        get_hybrid_cost(t.offset - 1, num_fixed_bits) + get_elias_gamma_cost(t.length_minus_one);
}


lz_result_t lz_parse(const refs_t *refs, uint32_t num_fixed_bits, arena_t *arena, arena_t scratch) {
    // Make a list of optimal tokens for each source index
    lz_item_array_span_t items = lz_item_array_span_make(refs_num(refs) + 1, &scratch);

    uint32_t max_offset = 256 << num_fixed_bits;

    for (uint32_t i = refs_num(refs); i-- > 0;) {
        lz_item_t *item = lz_item_array_span_at(items, i);
        item->total_cost = UINT32_MAX;

        token_array_view_t tokens = refs_get_tokens(refs, i);

        for (uint32_t j = 0; j < tokens.num; j++) {
            token_t token = token_array_view_get(tokens, j);

            if (token_is_literal(token) || token.offset <= max_offset) {
                do {
                    const lz_item_t *next_item = lz_item_array_span_at(items, i + token_get_length(token));

                    uint32_t tally = token_are_same_type(token, next_item->token) ?
                        (next_item->tally % 256) + 1 :
                        1;

                    uint32_t cost = 
                        get_token_cost(token, num_fixed_bits) +
                        get_tally_cost(tally) +
                        next_item->total_cost -
                        get_tally_cost(next_item->tally);
                    
                    if (cost < item->total_cost) {
                        item->token = token;
                        item->total_cost = cost;
                        item->tally = tally;
                    }
                }
                while (token.length_minus_one-- > 1);
            }
        }
    }

    // Now build the final token stream by walking the token list from the first element
    lz_item_array_t result = lz_item_array_make(refs_num(refs), arena);
    for (uint32_t i = 0; i < refs_num(refs); i += token_get_length(lz_item_array_span_get(items, i).token)) {
        lz_item_array_add(&result, lz_item_array_span_get(items, i), arena);
    }

    return (lz_result_t) {
        .items = result.view
    };
}
