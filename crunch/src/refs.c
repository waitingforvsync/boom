#include "refs.h"
#include <stdbool.h>


// index_range_slice implementation

typedef struct index_range_slice_t {
    index_range_t *data;
    uint32_t num;
} index_range_slice_t;


static index_range_slice_t index_range_slice_make(arena_t *arena, uint32_t num) {
    assert(arena);
    return (index_range_slice_t) {
        .data = arena_calloc(arena, sizeof(index_range_t) * num),
        .num = num
    };
}


static index_range_view_t index_range_view_make(index_range_slice_t slice) {
    return (index_range_view_t) {slice.data, slice.num};
}


static void index_range_slice_set(index_range_slice_t slice, uint32_t index, index_range_t range) {
    assert(index < slice.num);
    slice.data[index] = range;
}


// index_range_view implementation

static index_range_t index_range_view_get(index_range_view_t ranges, uint32_t index) {
    assert(index < ranges.num);
    return ranges.data[index];
}


// token_array implementation

typedef struct token_array_t {
    token_t *data;
    uint32_t num;
    uint32_t capacity;
} token_array_t;


static token_array_t token_array_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (token_array_t) {
        .data = arena_alloc(arena, sizeof(token_t) * initial_capacity),
        .num = 0,
        .capacity = initial_capacity
    };
}


static uint32_t token_array_add(token_array_t *array, token_t value, arena_t *arena) {
    assert(array);
    assert(array->data);
    if (array->num < array->capacity) {
        array->data[array->num] = value;
        return array->num++;
    }

    if (!arena) {
        abort();
    }

    uint32_t new_capacity = (array->capacity * 2 >= 16) ? array->capacity * 2 : 16;
    array->data = arena_realloc(arena, array->data, sizeof(token_t) * array->capacity, sizeof(token_t) * new_capacity);
    array->capacity = new_capacity;
    array->data[array->num] = value;
    return array->num++;
}


// token_view implementation

static token_view_t token_view_make(const token_array_t *array) {
    return (token_view_t) {
        .data = array->data,
        .num = array->num
    };
}


// indices_array implementation

// indices_array is just a list of uint32_t, used as indices to the source data

typedef struct indices_array_t {
    uint32_t *data;
    uint32_t num;
    uint32_t capacity;
} indices_array_t;


static indices_array_t indices_array_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (indices_array_t) {
        .data = arena_alloc(arena, sizeof(uint32_t) * initial_capacity),
        .num = 0,
        .capacity = initial_capacity
    };
}


static uint32_t indices_array_get(const indices_array_t *array, uint32_t index) {
    assert(array);
    assert(index < array->num);
    return array->data[index];
}


static uint32_t indices_array_find(const indices_array_t *array, uint32_t to_find) {
    assert(array);
    for (uint32_t i = 0; i < array->num; i++) {
        if (array->data[i] == to_find) {
            return i;
        }
    }
    assert(false);
    return 0;
}


static uint32_t indices_array_add(indices_array_t *array, uint32_t value, arena_t *arena) {
    assert(array);
    assert(array->data);
    if (array->num < array->capacity) {
        array->data[array->num] = value;
        return array->num++;
    }

    if (!arena) {
        abort();
    }

    uint32_t new_capacity = (array->capacity * 2 >= 16) ? array->capacity * 2 : 16;
    array->data = arena_realloc(arena, array->data, sizeof(uint32_t) * array->capacity, sizeof(uint32_t) * new_capacity);
    array->capacity = new_capacity;
    array->data[array->num] = value;
    return array->num++;
}


// byte_pair_cache implementation

// byte_pair_cache is a cache of all the occurrences of a pair of byte values in the source data.
// It is indexed by a 16-bit value (0...0x10000), and returns an array of indices within the source data
// in which those two bytes occur consecutively.
// It is used to improve the speed of match finding.

typedef struct byte_pair_cache_t {
    indices_array_t *data;
    uint32_t num;
} byte_pair_cache_t;

#define INITIAL_INDICES_ARRAY_CAPACITY 16

static void byte_pair_cache_element_init(byte_pair_cache_t byte_pair_cache, uint32_t index, arena_t *arena) {
    assert(index < byte_pair_cache.num);
    byte_pair_cache.data[index] = indices_array_make(arena, INITIAL_INDICES_ARRAY_CAPACITY);
}


static indices_array_t *byte_pair_cache_element_at(byte_pair_cache_t byte_pair_cache, uint32_t index) {
    assert(index < byte_pair_cache.num);
    return byte_pair_cache.data + index;
}


static byte_pair_cache_t byte_pair_cache_make(arena_t *arena, byte_view_t data) {
    assert(arena);
    byte_pair_cache_t byte_pair_cache = {
        .data = arena_calloc(arena, sizeof(indices_array_t) * 0x10000),
        .num = 0x10000
    };

    for (uint32_t i = 0; i < data.num - 1; i++) {
        uint32_t key = byte_view_get(data, i) | (byte_view_get(data, i + 1) << 8);
        if (!byte_pair_cache_element_at(byte_pair_cache, key)->data) {
            byte_pair_cache_element_init(byte_pair_cache, key, arena);
        }
        indices_array_add(byte_pair_cache_element_at(byte_pair_cache, key), i, arena);
    }

    return byte_pair_cache;
}


// refs implementation

// For each element in the source data, we build a range of equivalent forms.
// This is either a literal, or a reference to a previous run of values (with offset and length).
// The idea here is that longer matches are better, but shorter offsets are preferable.
// So look at all the matches backwards from the current position.
// Only add a match if it is longer than the current longest one found.
// We also add partial matches from the previous longest to the new longest.

refs_t refs_make(byte_view_t data, arena_t *arena, arena_t scratch) {
    // Use the scratch arena for the byte_pair_cache as we discard it when we exit
    byte_pair_cache_t byte_pair_cache = byte_pair_cache_make(&scratch, data);

    index_range_slice_t ranges = index_range_slice_make(arena, data.num);
    token_array_t tokens = token_array_make(arena, data.num * 16);

    for (uint32_t i = 0; i < data.num; i++) {
        uint32_t token_array_start = tokens.num;

        // Add literal token
        token_array_add(
            &tokens,
            token_make_literal(byte_view_get(data, i)),
            arena
        );

        // Now find all the references
        // They will be stored from shortest to longest
        if (i < data.num - 1) {
            uint32_t key = byte_view_get(data, i) | (byte_view_get(data, i + 1) << 8);
            const indices_array_t *indices = byte_pair_cache_element_at(byte_pair_cache, key);

            uint32_t best_length_minus_one = 0;

            // We have a list of all the indices containing the current two byte pair.
            // The current index will be in this list, so find it, and then iterate backwards to the start for previous matches.
            uint32_t match_index = indices_array_find(indices, i);
            while (match_index-- > 0) {
                uint32_t j = indices_array_get(indices, match_index);

                // i is the higher of the two vertices, so this is the upper limit of the length we can compare to
                // Meanwhile 256 is the fixed upper limit as we use a byte for the length.
                uint32_t dist_to_end = data.num - i;
                uint32_t max_length = (dist_to_end < 256) ? dist_to_end : 256;

                for (uint32_t n = 1; n < max_length && byte_view_get(data, i + n) == byte_view_get(data, j + n); n++) {
                    if (n > best_length_minus_one) {
                        token_array_add(
                            &tokens,
                            token_make_ref(i - j, n),
                            arena
                        );
                        best_length_minus_one = n;
                    }
                }
            }
        }

        // Set the index range for this source data index
        index_range_slice_set(
            ranges,
            i,
            (index_range_t) {
                .start = token_array_start,
                .end = tokens.num
            }
        );
    }

    return (refs_t) {
        .ranges = index_range_view_make(ranges),
        .tokens = token_view_make(&tokens)
    };
}


token_view_t refs_get_tokens(const refs_t *refs, uint32_t index) {
    index_range_t range = index_range_view_get(refs->ranges, index);
    return (token_view_t) {
        .data = refs->tokens.data + range.start,
        .num = range.end - range.start
    };
}
