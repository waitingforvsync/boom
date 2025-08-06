#include "byte_array.h"
#include <stdlib.h>


byte_view_t byte_view_make(const byte_array_t *byte_array) {
    return (byte_view_t) {
        .data = byte_array->data,
        .num = byte_array->num
    };
}


byte_array_t byte_array_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (byte_array_t) {
        .data = arena_alloc(arena, initial_capacity),
        .num = 0,
        .capacity = initial_capacity
    };
}


uint32_t byte_array_add(byte_array_t *array, uint8_t value, arena_t *arena) {
    assert(array);
    assert(array->data);
    if (array->num >= array->capacity) {
        if (!arena) {
            abort();
        }
        uint32_t new_capacity = (array->capacity * 2 >= 16) ? array->capacity * 2 : 16;
        array->data = arena_realloc(arena, array->data, array->capacity, new_capacity);
        array->capacity = new_capacity;
    }
    array->data[array->num] = value;
    return array->num++;
}
