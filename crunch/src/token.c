#include "token.h"
#include <stdlib.h>


token_slice_t token_slice_make(arena_t *arena, uint32_t num) {
    assert(arena);
    return (token_slice_t) {
        .data = arena_calloc(arena, sizeof(token_t) * num),
        .num = num
    };
}


token_array_t token_array_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (token_array_t) {
        .data = arena_alloc(arena, sizeof(token_t) * initial_capacity),
        .num = 0,
        .capacity = initial_capacity
    };
}


uint32_t token_array_add(token_array_t *array, token_t value, arena_t *arena) {
    assert(array);
    assert(array->data);
    if (array->num >= array->capacity) {
        if (!arena) {
            abort();
        }
        uint32_t new_capacity = (array->capacity * 2 >= 16) ? array->capacity * 2 : 16;
        array->data = arena_realloc(arena, array->data, sizeof(token_t) * array->capacity, sizeof(token_t) * new_capacity);
        array->capacity = new_capacity;
    }
    array->data[array->num] = value;
    return array->num++;
}
