#include "lzhuff.h"
#include "refs.h"
#include "uint16_array.h"
#include <assert.h>
#include <stdbool.h>


static uint32_t lzhuff_find_unused_symbol(byte_array_view_t src) {
    uint16_t freqs[257] = {0};
    for (uint32_t i = 0; i < src.num; i++) {
        freqs[byte_array_view_get(src, i)]++;
    }
    for (uint32_t i = 0; i < 257; i++) {
        if (freqs[i] == 0) {
            return i;
        }
    }
    return 256;
}


lzhuff_result_t lzhuff_parse(byte_array_view_t src, arena_t *arena, arena_t scratch) {
    assert(src.data);
    assert(arena);

    // Find an unused symbol that will represent a reference
    uint32_t ref_symbol = lzhuff_find_unused_symbol(src);

    // Find all the back-references in the source data
    refs_t refs = refs_make(src, arena, scratch);

    // Get symbol frequencies based on an initial greedy parse
    uint16_t freqs[257] = {0};
    for (uint32_t i = 0; i < refs_num(&refs); ) {
        token_array_view_t tokens = refs_get_tokens(&refs, i);
        token_t biggest = token_array_view_get(tokens, tokens.num - 1);
        if (token_is_literal(biggest)) {
            freqs[biggest.value]++;
        }
        else {
            freqs[ref_symbol]++;
        }
    }

    // Build huffman tree based on this initial symbol frequency estimate
    huffman_code_t huff = huffman_code_make((uint16_array_view_t) VIEW(freqs), 0, arena, scratch);

    return (lzhuff_result_t) {0};
}
