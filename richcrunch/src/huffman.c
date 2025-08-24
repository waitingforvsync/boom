#include "bitreader.h"
#include "bitwriter.h"
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

        huffman_node_array_add(
            tree,
            new_node,
            0           // don't pass an arena
        );
    }
}


static void huffman_walk_tree(huffman_node_array_view_t tree, uint8_array_span_t lengths, uint32_t index, uint32_t level) {
    const huffman_node_t *node = huffman_node_array_view_at(tree, index);
    if (!node->left_child && !node->right_child) {
        // index is an index to leaf nodes sorted by frequency
        // thus we expect the lengths array to be ordered longest->shortest huffman code lengths
        uint8_array_span_set(lengths, index, level);
    }
    else {
        huffman_walk_tree(tree, lengths, node->left_child, level + 1);
        huffman_walk_tree(tree, lengths, node->right_child, level + 1);
    }
}


static void huffman_limit_length(uint8_array_span_t lengths, uint32_t max_code_length) {
    // Algorithm from https://glinscott.github.io/lz/index.html#toc3.2
    uint64_t k = 0ULL;
    uint64_t maxk = 1ULL << max_code_length;

    // Limit the bit lengths to the maximum allowed
    for (uint32_t i = 0; i < lengths.num; i++) {
        uint32_t length = uint8_array_span_get(lengths, i);
        uint32_t limited_length = min_uint32(length, max_code_length);
        uint8_array_span_set(lengths, i, limited_length);
        k += (1ULL << (max_code_length - limited_length));
    }

    // Redistribute error just introduced, first pass
    for (uint32_t i = 0; i < lengths.num && k >= maxk; i++) {
        while (true) {
            uint32_t length = uint8_array_span_get(lengths, i);
            if (length == max_code_length) {
                break;
            }
            length++;
            uint8_array_span_set(lengths, i, length);
            k -= (1ULL << (max_code_length - length));
        }
    }

    // Redistribute error just introduced, second pass
    for (uint32_t i = lengths.num; i-- > 0; ) {
        while (true) {
            uint32_t length = uint8_array_span_get(lengths, i);
            uint64_t dk = (1ULL << (max_code_length - length));
            if (k + dk >= maxk) {
                break;
            }
            length--;
            uint8_array_span_set(lengths, i, length);
            k += dk;
        }
    }
}


uint8_array_view_t huffman_build_code_lengths(uint16_array_view_t symbol_counts, uint32_t max_code_length, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(symbol_counts.data);

    // @todo: handle edge cases, e.g. zero or one symbols
    assert(symbol_counts.num > 1);

    // Create an array for holding the huffman tree
    // This goes into scratch space as it will be discarded at the end
    huffman_node_array_t tree = huffman_node_array_make(symbol_counts.num * 2, &scratch);

    // Get a list of all the symbols sorted in frequency order
    // These are the leaf nodes of the tree.
    // num_leafs is the number of symbols which are actually used (i.e. have non-zero count)
    uint32_t num_leafs = huffman_get_sorted_frequencies(symbol_counts, &tree);

    // Now build the rest of the tree from the leaf nodes
    huffman_build_tree(&tree);

    // We now have a fully built tree, with its root node at tree.num-1.
    // Now traverse the tree, getting the bit lengths of the symbols in the leaf nodes.
    // We index this lengths array by symbol frequency order (as per the index in the tree)
    uint8_array_span_t lengths = uint8_array_span_make(num_leafs, &scratch);
    huffman_walk_tree(tree.view, lengths, tree.num - 1, 0);

    // If we have specified a max code length, apply length limiting
    if (max_code_length == 0) {
        max_code_length = 15;
    }

    if (uint8_array_span_get(lengths, 0) > max_code_length) {
        huffman_limit_length(lengths, max_code_length);
    }

    // Make the array which stores bit lengths by symbol index, which we will return.
    // This is zero-initialised.
    uint8_array_span_t symbol_lengths = uint8_array_span_make(symbol_counts.num, arena);

    // Iterate through tree leaves (used symbols), get the code length, and store it in the symbol_lengths array
    for (uint32_t i = 0; i < num_leafs; i++) {
        // Get symbol value which corresponds to leaf index
        uint16_t symbol_index = huffman_node_array_get(&tree, i).symbol;
        assert(symbol_index < symbol_counts.num);

        // Write symbol length for this symbol value
        uint8_t code_length = uint8_array_span_get(lengths, i);
        uint8_array_span_set(symbol_lengths, symbol_index, code_length);
    }

    return symbol_lengths.view;
}


uint16_array_view_t huffman_get_canonical_encoding(uint8_array_view_t code_lengths, arena_t *arena) {
    assert(arena);
    assert(code_lengths.data);

    uint16_array_span_t symbol_codes = uint16_array_span_make(code_lengths.num, arena);
    uint16_t code = 0;
    for (uint32_t length = 1; length < 16; length++) {
        for (uint32_t i = 0; i < code_lengths.num; i++) {
            uint8_t code_length = uint8_array_view_get(code_lengths, i);
            if (code_length == length) {
                uint16_array_span_set(symbol_codes, i, code | (1 << length));
                code++;
            }
        }
        code <<= 1;
    }
    return symbol_codes.view;
}


huffman_decoder_t huffman_decoder_make(uint8_array_view_t code_lengths, arena_t *arena) {
    assert(arena);
    assert(code_lengths.data);

    // Count how many codes there are of each bit length
    // and build a dictionary of symbol values ordered by ascending canonical huffman code
    uint8_array_span_t num_codes_of_length = uint8_array_span_make(16, arena);
    uint16_array_t dictionary = uint16_array_make(code_lengths.num, arena);

    uint32_t highest_code_length = 0;
    for (uint32_t length = 1; length < 16; length++) {
        for (uint32_t i = 0; i < code_lengths.num; i++) {
            uint8_t code_length = uint8_array_view_get(code_lengths, i);

            if (code_length == length) {
                uint8_t *num_codes = uint8_array_span_at(num_codes_of_length, length - 1);
                assert(*num_codes < 255);
                (*num_codes)++;
                highest_code_length = max_uint32(highest_code_length, length);
                uint16_array_add(&dictionary, i, 0);
            }
        }
    }

    return (huffman_decoder_t) {
        .num_codes_of_length = (uint8_array_view_t) {
            .data = num_codes_of_length.data,
            .num = highest_code_length
        },
        .dictionary = dictionary.view
    };
}


byte_array_view_t huffman_serialise(byte_array_view_t src, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(src.data);

    arena_t local = arena_alloc_subarena(&scratch, 0x10000);

    // Count source symbols
    uint16_t counts[256] = {0};
    for (uint32_t i = 0; i < src.num; i++) {
        counts[byte_array_view_get(src, i)]++;
    }

    // Get huffman encoded length of each source symbol
    uint8_array_view_t huff = huffman_build_code_lengths(
        (uint16_array_view_t) VIEW(counts),
        0,
        &local,
        scratch
    );

    // Get canonical huffman codes
    uint16_array_view_t huff_codes = huffman_get_canonical_encoding(huff, &local); 

    // When we serialise, we have to first encode the huffman tree itself, or, in this case,
    // the lengths of the huffman code for each symbol (from which we can deduce the canonical code)

    // For compactness, we huffman encode the lengths array itself
    // The 'symbols' are the valid bit lengths 0...15 (remembering that we always length limit to 15)
    uint16_t huff_counts[16] = {0};
    for (uint32_t i = 0; i < huff.num; i++) {
        huff_counts[uint8_array_view_get(huff, i)]++;
    }

    // Get huffman encoded length of the huffman code dictionary
    uint8_array_view_t huffdict = huffman_build_code_lengths(
        (uint16_array_view_t) VIEW(huff_counts),
        7,      // length limit to 7 bits
        &local,
        scratch
    );

    // Get canonical huffman codes for the dictionary
    uint16_array_view_t huffdict_codes = huffman_get_canonical_encoding(huffdict, &local); 

    // Make a bitwriter    
    bitwriter_t writer = bitwriter_make(src.num, arena);

    // Write code lengths of dictionary symbols
    // They are 16 3-bit values (representing code lengths between 0 and 7)
    assert(huffdict.num == 16);
    for (uint32_t i = 0; i < huffdict.num; i++) {
        bitwriter_add_value(&writer, uint8_array_view_get(huffdict, i), 3, arena);
    }

    // Write huffman encoded dictionary
    assert(huff.num == 256);
    for (uint32_t i = 0; i < huff.num; i++) {
        bitwriter_add_huffman_code(
            &writer,
            huffdict_codes,
            uint8_array_view_get(huff, i),
            arena
        );
    }

    // Write length of src data
    bitwriter_add_value(&writer, src.num & 0xFF, 8, arena);
    bitwriter_add_value(&writer, src.num >> 8, 8, arena);

    // Write huffman encoded data
    for (uint32_t i = 0; i < src.num; i++) {
        bitwriter_add_huffman_code(
            &writer,
            huff_codes,
            byte_array_view_get(src, i),
            arena
        );
    }

    return writer.data.view;
}


byte_array_view_t huffman_deserialise(byte_array_view_t compressed, arena_t *arena, arena_t scratch) {
    assert(arena);
    assert(compressed.data);

    arena_t local = arena_alloc_subarena(&scratch, 0x10000);

    // Make bitreader from the compressed data
    bitreader_t reader = bitreader_make(compressed);

    // Read huffman tree used to compress dictionary
    uint8_t dict_lengths[16];
    for (uint32_t i = 0; i < 16; i++) {
        dict_lengths[i] = bitreader_get_value(&reader, 3);
    }

    huffman_decoder_t dict_decoder = huffman_decoder_make(
        (uint8_array_view_t) VIEW(dict_lengths),
        &local
    );

    uint8_t lengths[256];
    for (uint32_t i = 0; i < 256; i++) {
        lengths[i] = bitreader_get_huffman_code(&reader, dict_decoder);
    }

    huffman_decoder_t decoder = huffman_decoder_make(
        (uint8_array_view_t) VIEW(lengths),
        &local
    );

    // Get size to decompress
    uint8_t sizelo = bitreader_get_value(&reader, 8);
    uint8_t sizehi = bitreader_get_value(&reader, 8);
    uint32_t size = sizelo | (sizehi << 8);
    byte_array_t result = byte_array_make(size, arena);

    // Read and expand huffman compressed data
    for (uint32_t i = 0; i < size; i++) {
        byte_array_add(
            &result,
            bitreader_get_huffman_code(&reader, decoder),
            arena
        );
    }

    return result.view;
}
