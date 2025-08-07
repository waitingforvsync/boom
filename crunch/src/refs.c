#include "refs.h"
#include <stdbool.h>


// Define indices (an array of uint32)
#define TEMPLATE_ARRAY_NAME indices
#define TEMPLATE_ARRAY_TYPE uint32_t
#include "array.template.h"


static uint32_t indices_view_find(indices_view_t indices, uint32_t to_find) {
    assert(indices.data);
    for (uint32_t i = 0; i < indices.num; i++) {
        if (indices_view_get(indices, i) == to_find) {
            return i;
        }
    }
    assert(false);
    return 0;
}


// Define indices_array (essentially an array of array of indices)
#define TEMPLATE_ARRAY_NAME indices_array
#define TEMPLATE_ARRAY_TYPE indices_t
#include "array.template.h"


// sequence_cache implementation

// byte_pair_cache is a cache of all the occurrences of a pair of byte values in the source data.
// It is indexed by a 16-bit value (0...0x10000), and returns an array of indices within the source data
// in which those two bytes occur consecutively.
// It is used to improve the speed of match finding.

typedef struct sequence_cache_t {
    indices_array_span_t indices_array;
} sequence_cache_t;


static sequence_cache_t sequence_cache_make(byte_array_view_t src, arena_t *arena) {
    assert(src.data);
    assert(arena);
    indices_array_span_t indices_array = indices_array_span_make(0x10000, arena);

    for (uint32_t i = 0; i < src.num - 1; i++) {
        uint32_t key = byte_array_view_get(src, i) | (byte_array_view_get(src, i + 1) << 8);
        indices_add(indices_array_span_at(indices_array, key), i, arena);
    }

    return (sequence_cache_t) {
        .indices_array = indices_array
    };
}


static indices_view_t sequence_cache_get_indices(const sequence_cache_t *sc, uint32_t key) {
    assert(sc);
    assert(key < 0x10000);
    return indices_array_span_get(sc->indices_array, key).view;
}


// refs implementation

// For each element in the source data, we build a range of equivalent forms.
// This is either a literal, or a reference to a previous run of values (with offset and length).
// The idea here is that longer matches are better, but shorter offsets are preferable.
// So look at all the matches backwards from the current position.
// Only add a match if it is longer than the current longest one found.
// We only add the longest match; when parsing we can look at the cost of all the shorter length matches.

refs_t refs_make(byte_array_view_t src, arena_t *arena, arena_t scratch) {
    // Use the scratch arena for the byte_pair_cache as we discard it when we exit
    sequence_cache_t sequence_cache = sequence_cache_make(src, &scratch);

    range_array_span_t ranges = range_array_span_make(src.num, arena);
    token_array_t tokens = token_array_make(src.num * 16, arena);

    for (uint32_t i = 0; i < src.num; i++) {
        uint32_t token_array_start = tokens.num;

        // Add literal token
        token_array_add(
            &tokens,
            token_make_literal(byte_array_view_get(src, i)),
            arena
        );

        // Now find all the references
        // They will be stored from shortest to longest
        if (i < src.num - 1) {
            uint32_t key = byte_array_view_get(src, i) | (byte_array_view_get(src, i + 1) << 8);
            indices_view_t indices = sequence_cache_get_indices(&sequence_cache, key);

            uint32_t best_length = 1;

            // We have a list of all the indices containing the current two byte pair.
            // The current index will be in this list, so find it, and then iterate backwards to the start for previous matches.
            uint32_t match_index = indices_view_find(indices, i);
            while (match_index-- > 0) {
                uint32_t j = indices_view_get(indices, match_index);

                // i is the higher of the two vertices, so this is the upper limit of the length we can compare to
                // Meanwhile 256 is the fixed upper limit as we use a byte for the length.
                uint32_t dist_to_end = src.num - i;
                uint32_t max_length = (dist_to_end < 256) ? dist_to_end : 256;

                uint32_t length = 2;
                while (length < max_length && byte_array_view_get(src, i + length) == byte_array_view_get(src, j + length)) {
                    length++;
                }

                if (length > best_length) {
                    token_array_add(&tokens, token_make_ref(i - j, length - 1), arena);
                    best_length = length;
                }
            }
        }

        // Set the index range for this source data index
        range_array_span_set(
            ranges,
            i,
            (range_t) {
                .start = token_array_start,
                .end = tokens.num
            }
        );
    }

    return (refs_t) {
        .ranges = ranges.view,
        .tokens = tokens.view
    };
}


token_array_view_t refs_get_tokens(const refs_t *refs, uint32_t index) {
    range_t range = range_array_view_get(refs->ranges, index);
    return token_array_view_make_subview(refs->tokens, range.start, range.end);
}
