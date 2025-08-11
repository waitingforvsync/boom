#ifndef REFS_H_
#define REFS_H_

#include "arena.h"
#include "byte_array.h"
#include "token.h"
#include <assert.h>


// A [start, end) pair of indices
typedef struct range_t {
    uint32_t start;
    uint32_t end;
} range_t;


// Define range array types
#define TEMPLATE_ARRAY_NAME range_array
#define TEMPLATE_ARRAY_TYPE range_t
#include "array.template.h"



// This is the manager object which maps source data indices to possible token representations
typedef struct refs_t {
    range_array_view_t ranges;
    token_array_view_t tokens;
} refs_t;


// Make an initialised refs_t from the data provided
refs_t refs_make(byte_array_view_t data, arena_t *arena, arena_t scratch);

// Get a list of tokens for the given index
token_array_view_t refs_get_tokens(const refs_t *refs, uint32_t index);

// Get number of indices
static inline uint32_t refs_num(const refs_t *refs) {
    assert(refs);
    return refs->ranges.num;
}


#endif // ifndef REFS_H_
