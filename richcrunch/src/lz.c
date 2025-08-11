#include "bitreader.h"
#include "bitwriter.h"
#include "lz.h"
#include "utils.h"
#include <assert.h>
#include <stdio.h>


static uint32_t get_elias_gamma_cost(uint32_t value) {
    assert(value > 0 && value <= 256);  // 256 will be read as 0
    return get_bit_width(value) * 2 - 1;
}


static uint32_t get_hybrid_cost(uint32_t value, uint32_t num_fixed_bits) {
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


lz_result_t lz_parse(const refs_t *refs, arena_t *arena, arena_t scratch) {
    lz_item_array_span_t items_array[8] = {0};

    for (uint32_t n = 0; n < 8; n++) {
        uint32_t num_fixed_bits = n + 1;

        // Make a list of optimal tokens for each source index
        lz_item_array_span_t items = lz_item_array_span_make(refs_num(refs) + 1, &scratch);
        items_array[n] = items;

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
                            ((tally != 1) ? get_tally_cost(next_item->tally) : 0);
                        
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
    }

    // Find the parse with the lowest cost
    uint32_t best_cost = UINT32_MAX;
    uint32_t best_n = 0;
    for (uint32_t n = 0; n < 8; n++) {
        uint32_t cost = lz_item_array_span_get(items_array[n], 0).total_cost;
        if (cost < best_cost) {
            best_cost = cost;
            best_n = n;
        }
    }

    // Now build the final token stream by walking the token list from the first element
    lz_item_array_span_t items = items_array[best_n];
    lz_item_array_t result = lz_item_array_make(refs_num(refs), arena);
    for (uint32_t i = 0; i < refs_num(refs); i += token_get_length(lz_item_array_span_get(items, i).token)) {
        lz_item_array_add(&result, lz_item_array_span_get(items, i), arena);
    }

    return (lz_result_t) {
        .items = result.view,
        .cost = lz_item_array_get(&result, 0).total_cost,
        .num_fixed_bits = best_n + 1
    };
}


static uint32_t lz_get_block_count(const lz_result_t *lz) {
    uint32_t num_blocks = 0;
    for (uint32_t i = 0; i < lz->items.num; i += lz_item_array_view_get(lz->items, i).tally) {
        num_blocks++;
    }
    return num_blocks;
}


void lz_dump(const lz_result_t *lz, const char *filename) {
    FILE *file = 0;
    if (filename) {
        file = fopen(filename, "w");
    }
    if (!file) {
        file = stdout;
    }

    fprintf(file, "Block count: %d\n", lz_get_block_count(lz));
    uint32_t i = 0;
    uint32_t addr = 0;
    while (i < lz->items.num) {
        const lz_item_t *item = lz_item_array_view_at(lz->items, i);
        uint32_t num = item->tally;

        if (token_is_literal(item->token)) {
            fprintf(file, "Literals: %d\n", num);
            for (uint32_t n = 0; n < num; n += 16) {
                fprintf(file, "  %04X:  ", addr);
                for (uint32_t m = n; m < min_uint32(num, n + 16); m++, i++, addr++) {
                    item = lz_item_array_view_at(lz->items, i);
                    assert(token_is_literal(item->token));
                    fprintf(file, "%02X ", item->token.value);
                }
                fprintf(file, "\n");
            }
        }
        else {
            fprintf(file, "References: %d\n", num);
            for (uint32_t n = 0; n < num; n++, i++) {
                item = lz_item_array_view_at(lz->items, i);
                assert(!token_is_literal(item->token));
                uint32_t offset = item->token.offset;
                uint32_t length = item->token.length_minus_one + 1;
                fprintf(file, "  %04X:  Ref %04X, length %-3u\n", addr, addr - offset, length);
                addr += length;
            }
        }
    }

    if (file) {
        fclose(file);
    }
}


byte_array_view_t lz_serialise(const lz_result_t *lz, arena_t *arena) {
    assert(lz);
    assert(arena);

    uint32_t num_blocks = lz_get_block_count(lz);
    uint32_t num_bits = lz->cost + get_hybrid_cost(num_blocks, 8) + 4;
    bitwriter_t writer = bitwriter_make((num_bits + 7) / 8, arena);

    bitwriter_add_hybrid_value(&writer, num_blocks, 8, arena);
    bitwriter_add_value(&writer, lz->num_fixed_bits - 1, 3, arena);

    uint32_t i = 0;
    while (i < lz->items.num) {
        const lz_item_t *item = lz_item_array_view_at(lz->items, i);
        uint32_t num = item->tally;

        // Write number of things in this block
        bitwriter_add_elias_gamma_value(&writer, num, arena);

        if (token_is_literal(item->token)) {
            for (uint32_t n = 0; n < num; n++, i++) {
                item = lz_item_array_view_at(lz->items, i);
                assert(token_is_literal(item->token));
                bitwriter_add_value(&writer, item->token.value, 8, arena);
            }
        }
        else {
            for (uint32_t n = 0; n < num; n++, i++) {
                item = lz_item_array_view_at(lz->items, i);
                assert(!token_is_literal(item->token));
                bitwriter_add_hybrid_value(&writer, item->token.offset - 1, lz->num_fixed_bits, arena);
                bitwriter_add_elias_gamma_value(&writer, item->token.length_minus_one, arena);
            }
        }
    }

    return writer.data.view;
}


byte_array_view_t lz_deserialise(byte_array_view_t compressed, arena_t *arena) {
    assert(arena);
    byte_array_t buffer = byte_array_make(0x1000, arena);
    bitreader_t reader = bitreader_make(compressed);

    uint32_t num_blocks = bitreader_get_hybrid_value(&reader, 8);
    uint32_t num_fixed_bits = bitreader_get_value(&reader, 3) + 1;

    bool is_literal = true;
    while (num_blocks--) {
        uint32_t num_items = bitreader_get_elias_gamma_value(&reader);
        if (num_items == 0) {
            num_items = 256;
            is_literal = !is_literal;
        }
        if (is_literal) {
            for (uint32_t n = 0; n < num_items; n++) {
                byte_array_add(&buffer, bitreader_get_value(&reader, 8), arena);
            }
        }
        else {
            for (uint32_t n = 0; n < num_items; n++) {
                uint32_t offset = bitreader_get_hybrid_value(&reader, num_fixed_bits) + 1;
                uint32_t length = (uint32_t)bitreader_get_elias_gamma_value(&reader) + 1;
                for (uint32_t i = 0; i < length; i++) {
                    byte_array_add(&buffer, byte_array_get(&buffer, buffer.num - offset), arena);
                }
            }
        }
        is_literal = !is_literal;
    }

    return buffer.view;
}
