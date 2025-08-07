#include "lz.h"
#include "utils.h"
#include <assert.h>


typedef struct costs_t {
    uint32_t total_cost;
    uint32_t tally;
} costs_t;


typedef struct costs_slice_t {
    costs_t *data;
    uint32_t num;
} costs_slice_t;


static inline costs_slice_t costs_slice_make(arena_t *arena, uint32_t num) {
    return (costs_slice_t) {
        .data = arena_calloc(arena, sizeof(uint32_t) * num),
        .num = num
    };
}

static inline costs_t *costs_slice_at(costs_slice_t costs, uint32_t index) {
    assert(index < costs.num);
    return costs.data + index;
}

static inline costs_t costs_slice_get(costs_slice_t costs, uint32_t index) {
    assert(index < costs.num);
    return costs.data[index];
}

static inline void costs_slice_set(costs_slice_t costs, uint32_t index, costs_t value) {
    assert(index < costs.num);
    costs.data[index] = value;
}

/////////////////////////////////////////////////////////


static uint32_t get_tally_cost(uint32_t value) {
    // tally is a simple elias gamma encoding of the length
    return get_elias_gamma_cost(value);
}


static uint32_t get_token_cost(token_t t, uint32_t num_fixed_bits) {
    return token_is_literal(t) ?
        8 :
        get_hybrid_cost(t.offset - 1, num_fixed_bits) + get_elias_gamma_cost(t.length_minus_one);
}


lz_result_t lz_parse(const refs_t *refs, uint32_t max_cost, uint32_t num_fixed_offset_bits, arena_t *arena, arena_t scratch) {
    // Make a list of optimal tokens for each source index
    token_array_span_t tokens = token_array_span_make(refs_num(refs), &scratch);

    // Get array of costs: add an extra element for 'off the end'
    costs_slice_t costs = costs_slice_make(&scratch, refs_num(refs) + 1);

    uint32_t max_offset = 256 << num_fixed_offset_bits;

    for (uint32_t i = refs_num(refs); i-- > 0;) {
        uint32_t best_cost = UINT32_MAX;
        token_t best_token = {0};
        uint32_t best_tally = 0;

        token_array_view_t possible_tokens = refs_get_tokens(refs, i);

        for (uint32_t j = 0; j < possible_tokens.num; j++) {
            token_t token = token_array_view_get(possible_tokens, j);

            if (token_is_literal(token) || token.offset <= max_offset) {
                do {
                    uint32_t i_next = i + token.length_minus_one + 1;
                    token_t token_next = token_array_span_get(tokens, i_next);

                    uint32_t tally = (token_is_literal(token) == token_is_literal(token_next)) ?
                        (costs_slice_get(costs, i_next).tally % 256) + 1 :
                        1;

                    uint32_t cost = 
                        get_token_cost(token, num_fixed_offset_bits) -
                        get_tally_cost(costs_slice_get(costs, i_next).tally) +
                        get_tally_cost(tally);
                    
                    if (cost < best_cost) {
                        best_token = token;
                        best_cost = cost;
                        best_tally = tally;
                    }
                }
                while (token.length_minus_one-- > 1);
            }
        }
        token_array_span_set(tokens, i, best_token);
        costs_slice_set(costs, i,
            (costs_t) {
                .total_cost = best_cost,
                .tally = best_tally
            }
        );
    }

    // Now build the final token stream by walking the token list from the first element
    token_array_t token_stream = token_array_make(refs_num(refs), arena);
    for (uint32_t i = 0; i < tokens.num; i += token_array_span_get(tokens, i).length_minus_one + 1) {
        token_array_add(&token_stream, token_array_span_get(tokens, i), arena);
    }

    return (lz_result_t) {
        .tokens = token_stream.view,
        .cost = costs_slice_get(costs, 0).total_cost
    };
}
