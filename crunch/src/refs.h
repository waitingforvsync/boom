#ifndef REFS_H_
#define REFS_H_

#include "arena.h"
#include "byte_array.h"
#include "token.h"
#include <assert.h>


// A [start, end) pair of indices
typedef struct index_range_t {
    uint32_t start;
    uint32_t end;
} index_range_t;


// A list of index_ranges
typedef struct index_range_view_t {
    const index_range_t *data;
    uint32_t num;
} index_range_view_t;


// This is the manager object which maps source data indices to possible token representations
typedef struct refs_t {
    index_range_view_t ranges;
    token_view_t tokens;
} refs_t;


// Make an initialised refs_t from the data provided
refs_t refs_make(byte_view_t data, arena_t *arena, arena_t scratch);

// Get a list of tokens for the given index
token_view_t refs_get_tokens(const refs_t *refs, uint32_t index);

// Get number of indices
static inline uint32_t refs_num(const refs_t *refs) {
    assert(refs);
    return refs->ranges.num;
}


#endif // ifndef REFS_H_
