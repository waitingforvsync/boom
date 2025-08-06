#ifndef TOKEN_H_
#define TOKEN_H_

#include "arena.h"
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>


// Representation of a token, i.e. an element of the compressed data.
// It is either a literal, or a reference to a repeated string.
typedef struct token_t {
    union {
        uint8_t value;
        uint16_t offset;
    };
    uint8_t length_minus_one;
} token_t;


// A list of tokens
typedef struct token_view_t {
    const token_t *data;
    uint32_t num;
} token_view_t;


// A fixed array of tokens
typedef struct token_slice_t {
    token_t *data;
    uint32_t num;
} token_slice_t;


// A dynamic array of tokens
typedef struct token_array_t {
    token_t *data;
    uint32_t num;
    uint32_t capacity;
} token_array_t;


// Make a token_view from an array
static inline token_view_t token_view_from_array(const token_array_t *array) {
    return (token_view_t) {
        .data = array->data,
        .num = array->num
    };
}

// Make a token_view from a slice
static inline token_view_t token_view_from_slice(token_slice_t slice) {
    return (token_view_t) {
        .data = slice.data,
        .num = slice.num
    };
}

// Get a token from a token_view
static inline token_t token_view_get(token_view_t tokens, uint32_t index) {
    assert(index < tokens.num);
    return tokens.data[index];
}

// Make a token_slice
token_slice_t token_slice_make(arena_t *arena, uint32_t num);

// Get a token in a token_slice
static inline token_t token_slice_get(token_slice_t tokens, uint32_t index) {
    assert(index < tokens.num);
    return tokens.data[index];
}

// Set a token in a token_slice
static inline void token_slice_set(token_slice_t tokens, uint32_t index, token_t value) {
    assert(index < tokens.num);
    tokens.data[index] = value;
}

// Make a token_array
token_array_t token_array_make(arena_t *arena, uint32_t initial_capacity);

// Add an item to a token_array
uint32_t token_array_add(token_array_t *array, token_t value, arena_t *arena);

// Make a token literal
static inline token_t token_make_literal(uint8_t c) {
    return (token_t) {
        .value = c,
        .length_minus_one = 0
    };
}

// Make a token ref
static inline token_t token_make_ref(uint16_t offset, uint8_t length_minus_one) {
    assert(offset > 0);
    assert(length_minus_one > 0);
    return (token_t) {
        .offset = offset,
        .length_minus_one = length_minus_one
    };
}

// Check if a token is a literal
static inline bool token_is_literal(token_t t) {
    return t.length_minus_one == 0;
}

// Get the byte length represented by this token
static inline uint32_t token_get_length(token_t t) {
    return t.length_minus_one + 1;
}


#endif // ifndef TOKEN_H_
