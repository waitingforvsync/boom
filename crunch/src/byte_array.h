#ifndef BYTE_ARRAY_H_
#define BYTE_ARRAY_H_

#include "arena.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


// A read-only buffer of bytes
typedef struct byte_view_t {
    const uint8_t *data;
    uint32_t num;
} byte_view_t;


// A dynamic byte array
typedef struct byte_array_t {
    uint8_t *data;
    uint32_t num;
    uint32_t capacity;
} byte_array_t;


// Make a byte_view
byte_view_t byte_view_make(const byte_array_t *byte_array);

// Get an element from a byte view
static inline uint8_t byte_view_get(byte_view_t view, uint32_t index) {
    assert(index < view.num);
    return view.data[index];
}

// Make a byte_array
byte_array_t byte_array_make(arena_t *arena, uint32_t initial_capacity);

// Get an element from a byte_array
static inline uint8_t byte_array_get(const byte_array_t *array, uint32_t index) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    return array->data[index];
}

// Set an element in a byte_array
static inline void byte_array_set(byte_array_t *array, uint32_t index, uint8_t value) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    array->data[index] = value;
}

// Add an element to a byte_array, dynamically resizing if necessary
uint32_t byte_array_add(byte_array_t *array, uint8_t value, arena_t *arena);


#endif // ifndef BYTE_ARRAY_H_
