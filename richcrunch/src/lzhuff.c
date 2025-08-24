#include "lzhuff.h"
#include "refs.h"
#include "uint16_array.h"
#include <assert.h>
#include <stdbool.h>


static uint32_t lzhuff_find_unused_symbol(byte_array_view_t src) {
    uint8_t used[257] = {0};
    for (uint32_t i = 0; i < src.num; i++) {
        used[byte_array_view_get(src, i)] |= 1;
    }
    for (uint32_t i = 0; i < 256; i++) {
        if (!used[i]) {
            return i;
        }
    }
    return 256;
}


static uint32_t get_token_cost(token_t t, uint8_array_view_t lengths, uint16_t lz_symbol, uint32_t num_fixed_bits) {
    if (token_is_literal(t)) {
        return uint8_array_view_get(lengths, t.value);
    }
    else {
        return uint8_array_view_get(lengths, lz_symbol) +
               get_hybrid_cost(t.offset - 1, num_fixed_bits) +
               get_elias_gamma_cost(t.length_minus_one);
    }
}


lzhuff_result_t lzhuff_parse(byte_array_view_t src, arena_t *arena, arena_t scratch) {
    assert(src.data);
    assert(arena);

    // Find an unused symbol that will represent a reference
    uint32_t ref_symbol = lzhuff_find_unused_symbol(src);

    // Reserve a piece of scratch space for holding the refs result
    arena_t local_arena = arena_alloc_subarena(&scratch, 0x400000);

    // Find all the back-references in the source data
    refs_t refs = refs_make(src, &local_arena, scratch);

    // Get symbol counts based on an initial greedy parse
    uint16_t counts[257] = {0};
    for (uint32_t i = 0; i < refs_num(&refs); ) {
        token_array_view_t tokens = refs_get_tokens(&refs, i);
        token_t biggest = token_array_view_get(tokens, tokens.num - 1);
        if (token_is_literal(biggest)) {
            counts[biggest.value]++;
        }
        else {
            counts[ref_symbol]++;
        }

        i += token_get_length(biggest);
    }

    // Build huffman tree based on this initial symbol frequency estimate
    uint8_array_view_t lengths = huffman_build_code_lengths(
        (uint16_array_view_t) VIEW(counts),
        0,
        &local_arena,
        scratch
    );

    // Parse the source data with differing numbers of fixed offset bits, and choose the best one at the end
    lzhuff_item_array_span_t items_array[8] = {0};

    for (uint32_t n = 0; n < 8; n++) {
        uint32_t num_fixed_bits = n + 1;

        // Make a list of optimal tokens for each source index
        // We reserve an extra element which represents the "off the end" element which previous elements can point to
        lzhuff_item_array_span_t items = lzhuff_item_array_span_make(src.num + 1, &scratch);
        items_array[n] = items;

        uint32_t max_offset = 256 << num_fixed_bits;

        for (uint32_t i = src.num; i-- > 0;) {
            lzhuff_item_t *item = lzhuff_item_array_span_at(items, i);
            item->total_cost = UINT32_MAX;

            token_array_view_t tokens = refs_get_tokens(&refs, i);

            for (uint32_t j = 0; j < tokens.num; j++) {
                token_t token = token_array_view_get(tokens, j);

                if (token_is_literal(token) || token.offset <= max_offset) {
                    do {
                        const lzhuff_item_t *next_item = lzhuff_item_array_span_at(items, i + token_get_length(token));

                        uint32_t cost = next_item->total_cost + get_token_cost(token, lengths, ref_symbol, num_fixed_bits);
                        
                        if (cost < item->total_cost) {
                            item->token = token;
                            item->total_cost = cost;
                        }
                    }
                    while (token.length_minus_one-- > 1);
                }
            }
        }
    }

    // Find the parse with the lowest cost
    uint32_t best_cost = UINT32_MAX;
    uint32_t best_n = 0;
    for (uint32_t n = 0; n < 8; n++) {
        uint32_t cost = lzhuff_item_array_span_get(items_array[n], 0).total_cost;
        if (cost < best_cost) {
            best_cost = cost;
            best_n = n;
        }
    }

    // Now build the final token stream by walking the token list from the first element
    lzhuff_item_array_span_t items = items_array[best_n];
    uint16_t new_counts[257] = {0};

    lzhuff_item_array_t result = lzhuff_item_array_make(src.num, arena);
    for (uint32_t i = 0; i < src.num; i += token_get_length(lzhuff_item_array_span_get(items, i).token)) {
        lzhuff_item_array_add(&result, lzhuff_item_array_span_get(items, i), arena);
    }

    return (lzhuff_result_t) {
        .items = result.view
    };
}
