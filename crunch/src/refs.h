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


// A list of tokens
// This is referenced by the list of index_ranges above, to give the range of tokens corresponding
// to each source data index.
typedef struct token_view_t {
    const token_t *data;
    uint32_t num;
} token_view_t;


static inline token_t token_view_get(token_view_t tokens, uint32_t index) {
    assert(index < tokens.num);
    return tokens.data[index];
}


// This is the manager object which maps source data indices to possible token representations
typedef struct refs_t {
    index_range_view_t ranges;
    token_view_t tokens;
} refs_t;


// Make an initialised refs_t from the data provided
refs_t refs_make(byte_view_t data, arena_t *arena, arena_t scratch);

// Get a list of tokens for the given index
token_view_t refs_get_tokens(const refs_t *refs, uint32_t index);


#endif // ifndef REFS_H_
