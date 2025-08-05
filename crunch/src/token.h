#ifndef TOKEN_H_
#define TOKEN_H_

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>


typedef struct token_t {
    union {
        uint8_t value;
        uint16_t offset;
    };
    uint8_t length_minus_one;
} token_t;


static inline token_t token_make_literal(uint8_t c) {
    return (token_t) {
        .value = c,
        .length_minus_one = 0
    };
}


static inline token_t token_make_ref(uint16_t offset, uint8_t length_minus_one) {
    assert(offset > 0);
    assert(length_minus_one > 0);
    return (token_t) {
        .offset = offset,
        .length_minus_one = length_minus_one
    };
}


static inline bool token_is_literal(token_t t) {
    return t.length_minus_one == 0;
}


#endif // ifndef TOKEN_H_
