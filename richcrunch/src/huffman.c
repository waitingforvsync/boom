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


huffman_code_t huffman_code_make(uint16_array_view_t freqs, uint32_t max_code_length, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(freqs.data);

    // @todo: handle edge cases, e.g. zero or one symbols

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

    // Output bit lengths by symbol index
    byte_array_span_t symbol_lengths = byte_array_span_make(freqs.num, arena);
    for (uint32_t i = 0; i < num_leafs; i++) {
        uint16_t symbol_index = huffman_node_array_get(&tree, i).symbol;
        assert(symbol_index < freqs.num);
        byte_array_span_set(symbol_lengths, symbol_index, byte_array_span_get(lengths, i));
    }

    return (huffman_code_t) {
        .symbol_lengths = symbol_lengths.view
    };
}
