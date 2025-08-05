#ifndef BYTE_ARRAY_H_
#define BYTE_ARRAY_H_

#include "arena.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>


typedef struct byte_view_t {
    const uint8_t *data;
    uint32_t num;
} byte_view_t;


typedef struct byte_array_t {
    uint8_t *data;
    uint32_t num;
    uint32_t capacity;
} byte_array_t;


static inline byte_view_t byte_view_make(const byte_array_t *byte_array) {
    return (byte_view_t) {
        .data = byte_array->data,
        .num = byte_array->num
    };
}


static inline uint8_t byte_view_get(byte_view_t view, uint32_t index) {
    assert(index < view.num);
    return view.data[index];
}


static inline byte_array_t byte_array_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (byte_array_t) {
        .data = arena_alloc(arena, initial_capacity),
        .num = 0,
        .capacity = initial_capacity
    };
}


static inline uint8_t byte_array_get(const byte_array_t *array, uint32_t index) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    return array->data[index];
}


static inline void byte_array_set(byte_array_t *array, uint32_t index, uint8_t value) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    array->data[index] = value;
}


static inline uint32_t byte_array_add(byte_array_t *array, uint8_t value, arena_t *arena) {
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
    array->data = arena_realloc(arena, array->data, array->capacity, new_capacity);
    array->capacity = new_capacity;
    array->data[array->num] = value;
    return array->num++;
}


#endif // ifndef BYTE_ARRAY_H_
