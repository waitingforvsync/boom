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


// Define token array types
#define TEMPLATE_ARRAY_NAME token_array
#define TEMPLATE_ARRAY_TYPE token_t
#include "array.template.h"


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

// Check if two tokens are the same type
static inline bool token_are_same_type(token_t a, token_t b) {
    return token_is_literal(a) == token_is_literal(b);
}

// Get the byte length represented by this token
static inline uint32_t token_get_length(token_t t) {
    return t.length_minus_one + 1;
}


#endif // ifndef TOKEN_H_
