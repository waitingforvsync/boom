#include "huffman.h"
#include <stdbool.h>


typedef struct huffman_node_t {
    uint16_t symbol;
    uint16_t frequency;
    uint16_t left_child;
    uint16_t right_child;
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


static uint32_t huffman_get_sorted_frequencies(uint16_array_view_t freqs, huffman_node_array_t *tree) {
    // Get the non-zero frequency symbols 
    for (uint32_t i = 0; i < freqs.num; i++) {
        uint32_t freq = uint16_array_view_get(freqs, i);
        if (freq) {
            huffman_node_array_add(
                tree,
                (huffman_node_t) {
                    .symbol = i,
                    .frequency = freq,
                    .left_child = 0,    // none
                    .right_child = 0    // none
                },
                0   // don't pass an arena; we know that we have enough space reserved
            );
        }
    }
    sort_huffman_node(tree->span);
    return tree->num;
}


static uint16_t huffman_get_frequency(huffman_node_array_view_t nodes, uint32_t index, uint32_t max_index) {
    if (index < max_index) {
        return huffman_node_array_view_get(nodes, index).frequency;
    }
    return 0xFFFF;  // "infinite" frequency
}


static void huffman_build_tree(huffman_node_array_t *tree) {
    uint32_t num_leafs = tree->num;

    // Now we start growing the parent nodes of the tree:
    // We need to pick the two lowest frequency nodes and create a single parent for them.
    // We can either pick from the pool of leaf nodes, or from the parent nodes we are creating.
    uint32_t leaf_index = 0;
    uint32_t tree_index = num_leafs;
    while (leaf_index < num_leafs || tree_index < tree->num - 1) {
        huffman_node_t new_node = {
            .symbol = 0xFFFF
        };

        uint16_t leaf_freq = huffman_get_frequency(tree->view, leaf_index, num_leafs);
        uint16_t tree_freq = huffman_get_frequency(tree->view, tree_index, tree->num);

        if (leaf_freq <= tree_freq) {
            new_node.left_child = leaf_index++;
            new_node.frequency = leaf_freq;
            leaf_freq = huffman_get_frequency(tree->view, leaf_index, num_leafs);
        }
        else {
            new_node.left_child = tree_index++;
            new_node.frequency = tree_freq;
            tree_freq = huffman_get_frequency(tree->view, tree_index, tree->num);
        }

        if (leaf_freq <= tree_freq) {
            new_node.right_child = leaf_index++;
            new_node.frequency += leaf_freq;
        }
        else {
            new_node.right_child = tree_index++;
            new_node.frequency += tree_freq;
        }

        assert(tree->num == num_leafs || huffman_node_array_get(tree, tree->num - 1).frequency <= new_node.frequency);
        huffman_node_array_add(tree, new_node, 0);
    }
}


static void huffman_walk_tree(huffman_node_array_view_t tree, byte_array_span_t lengths, uint32_t index, uint32_t level) {
    const huffman_node_t *node = huffman_node_array_view_at(tree, index);
    if (!node->left_child && !node->right_child) {
        // index is an index to leaf nodes sorted by frequency
        // thus we expect the lengths array to be ordered longest->shortest huffman code lengths
        byte_array_span_set(lengths, index, level);
    }
    else {
        huffman_walk_tree(tree, lengths, node->left_child, level + 1);
        huffman_walk_tree(tree, lengths, node->right_child, level + 1);
    }
}


static void huffman_limit_length(byte_array_span_t lengths, uint32_t max_code_length) {
    // Algorithm from https://glinscott.github.io/lz/index.html#toc3.2
    uint64_t k = 0ULL;
    uint64_t maxk = 1ULL << max_code_length;

    // Limit the bit lengths to the maximum allowed
    for (uint32_t i = 0; i < lengths.num; i++) {
        uint32_t length = byte_array_span_get(lengths, i);
        uint32_t limited_length = min_uint32(length, max_code_length);
        byte_array_span_set(lengths, i, limited_length);
        k += (1ULL << (max_code_length - limited_length));
    }

    // Redistribute error just introduced, first pass
    for (uint32_t i = 0; i < lengths.num && k >= maxk; i++) {
        while (true) {
            uint32_t length = byte_array_span_get(lengths, i);
            if (length == max_code_length) {
                break;
            }
            length++;
            byte_array_span_set(lengths, i, length);
            k -= (1ULL << (max_code_length - length));
        }
    }

    // Redistribute error just introduced, second pass
    for (uint32_t i = lengths.num; i-- > 0; ) {
        while (true) {
            uint32_t length = byte_array_span_get(lengths, i);
            uint64_t dk = (1ULL << (max_code_length - length));
            if (k + dk >= maxk) {
                break;
            }
            length--;
            byte_array_span_set(lengths, i, length);
            k += dk;
        }
    }
}


static uint8_t huffman_highest_bit_length(byte_array_span_t lengths) {
    assert(lengths.num > 0);
    uint8_t result = byte_array_span_get(lengths, 0);
    for (uint32_t i = 1; i < lengths.num; i++) {
        uint32_t length = byte_array_span_get(lengths, i);
        if (length > result) {
            result = length;
        }
    }
    return result;
}


#define TEMPLATE_SORT_NAME uint16
#include "sort.template.h"


huffman_code_t huffman_code_make(uint16_array_view_t freqs, uint32_t max_code_length, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(freqs.data);

    // @todo: handle edge cases, e.g. zero or one symbols
    assert(freqs.num > 1);

    // Create an array for holding the huffman tree
    // This goes into scratch space as it will be discarded at the end
    huffman_node_array_t tree = huffman_node_array_make(freqs.num * 2, &scratch);

    // Get a list of all the symbols sorted in frequency order
    // These are the leaf nodes of the tree.
    uint32_t num_leafs = huffman_get_sorted_frequencies(freqs, &tree);

    // Now build the rest of the tree from the leaf nodes
    huffman_build_tree(&tree);

    // We now have a fully built tree, with its root node at tree.num-1.
    // Now traverse the tree, getting the bit lengths of the symbols in the leaf nodes.
    // We index this lengths array by symbol frequency order (as per the index in the tree)
    byte_array_span_t lengths = byte_array_span_make(num_leafs, &scratch);
    huffman_walk_tree(tree.view, lengths, tree.num - 1, 0);

    // If we have specified a max code length, apply length limiting
    if (max_code_length > 0 && byte_array_span_get(lengths, 0) > max_code_length) {
        huffman_limit_length(lengths, max_code_length);
    }

    // Use this to store bit lengths by symbol index 
    byte_array_span_t symbol_lengths = byte_array_span_make(freqs.num, arena);

    // Use this to record number of symbol values per bit length
    byte_array_span_t num_symbols_per_bit_length = byte_array_span_make(
        huffman_highest_bit_length(lengths),
        arena
    );

    for (uint32_t i = 0; i < num_leafs; i++) {
        // Get symbol value which corresponds to leaf index
        uint16_t symbol_index = huffman_node_array_get(&tree, i).symbol;
        assert(symbol_index < freqs.num);

        // Write symbol length for this symbol value
        uint8_t symbol_length = byte_array_span_get(lengths, i);
        byte_array_span_set(symbol_lengths, symbol_index, symbol_length);

        // Increment num_symbols for the found bit length (indexed by length-1)
        uint32_t num_symbols = byte_array_span_get(num_symbols_per_bit_length, symbol_length - 1);
        assert(num_symbols < 255);
        byte_array_span_set(num_symbols_per_bit_length, symbol_length - 1, num_symbols + 1);
    }

    // Build the dictionary for looking up symbol from code
    // First copy the symbol values directly from the leaf array in highest frequency order
    uint16_array_span_t dictionary = uint16_array_span_make(num_leafs, arena);
    for (uint32_t i = 0; i < num_leafs; i++) {
        uint16_array_span_set(
            dictionary,
            i,
            huffman_node_array_get(&tree, num_leafs - 1 - i).symbol
        );
    }
    // Now within each span of equal bit length, sort the symbol values
    // to satisfy the canonical code property
    uint32_t start = 0;
    for (uint32_t i = 1; i < num_symbols_per_bit_length.num; i++) {
        uint32_t end = start + byte_array_span_get(num_symbols_per_bit_length, i);
        sort_uint16(uint16_array_span_make_subspan(dictionary, start, end));
        start = end;
    }

    return (huffman_code_t) {
        .symbol_lengths = symbol_lengths.view,
        .num_symbols_per_bit_length = num_symbols_per_bit_length.view,
        .dictionary = dictionary.view
    };
}
