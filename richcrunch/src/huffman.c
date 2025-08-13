#include "huffman.h"
#include <stdbool.h>


typedef struct huffman_node_t {
    uint16_t symbol;
    uint16_t frequency;
} huffman_node_t;

#define TEMPLATE_ARRAY_NAME huffman_node_array
#define TEMPLATE_ARRAY_TYPE huffman_node_t
#include "array.template.h"


static bool huffman_node_compare_freq(const huffman_node_t *a, const huffman_node_t *b) {
    return a->frequency < b->frequency;
}


#define TEMPLATE_SORT_NAME huffman_node
#define TEMPLATE_SORT_LESS_FN huffman_node_compare_freq
#include "sort.template.h"


static huffman_node_array_view_t huffman_code_get_sorted_frequencies(huffman_freq_view_t freqs, arena_t *arena) {
    // Get the non-zero frequency symbols 
    huffman_node_array_t input_node_array = huffman_node_array_make(freqs.num, arena);
    for (uint32_t i = 0; i < freqs.num; i++) {
        uint32_t freq = huffman_freq_view_get(freqs, i);
        if (freq) {
            huffman_node_array_add(&input_node_array, (huffman_node_t) {i, freq}, 0);
        }
    }

    huffman_node_array_span_t nodes = input_node_array.span;
    sort_huffman_node(nodes);

    return nodes.view;
}


huffman_code_t huffman_code_make(huffman_freq_view_t freqs, uint32_t max_code_length, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(freqs.data);

    // Get a list of all the symbols sorted in frequency order
    huffman_node_array_view_t input = huffman_code_get_sorted_frequencies(freqs, &scratch);

    return (huffman_code_t) {
        .symbols = {0}
    };
}
