#ifndef ARRAY_TEMPLATE_H_
#define ARRAY_TEMPLATE_H_

#include "arena.h"
#include "utils.h"
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define SPAN(a) {.data = a, .num = sizeof a / sizeof *a}
#define VIEW(a) {.data = a, .num = sizeof a / sizeof *a}

// Macros for concatenating multiple macros
#ifndef CONCAT
#define CONCAT(a, b) CONCAT2_(a, b)
#define CONCAT2_(a, b) a##b
#endif

#endif // ifndef ARRAY_TEMPLATE_H_


#ifndef TEMPLATE_ARRAY_NAME
#define TEMPLATE_ARRAY_NAME int_array   // for intellisense sanity
#error Must define TEMPLATE_ARRAY_NAME with the prefix of the array type name
#endif

#ifndef TEMPLATE_ARRAY_TYPE
#define TEMPLATE_ARRAY_TYPE int
#error Must define TEMPLATE_ARRAY_TYPE with the element type of the array
#endif


#define ARRAY_TYPE_t                   TEMPLATE_ARRAY_TYPE

#define ARRAY_NAME_view_t              CONCAT(TEMPLATE_ARRAY_NAME, _view_t)
#define ARRAY_NAME_view_make           CONCAT(TEMPLATE_ARRAY_NAME, _view_make)
#define ARRAY_NAME_view_make_subview   CONCAT(TEMPLATE_ARRAY_NAME, _view_make_subview)
#define ARRAY_NAME_view_get            CONCAT(TEMPLATE_ARRAY_NAME, _view_get)
#define ARRAY_NAME_view_at             CONCAT(TEMPLATE_ARRAY_NAME, _view_at)

#define ARRAY_NAME_span_t              CONCAT(TEMPLATE_ARRAY_NAME, _span_t)
#define ARRAY_NAME_span_make           CONCAT(TEMPLATE_ARRAY_NAME, _span_make)
#define ARRAY_NAME_span_make_subspan   CONCAT(TEMPLATE_ARRAY_NAME, _span_make_subspan)
#define ARRAY_NAME_span_get            CONCAT(TEMPLATE_ARRAY_NAME, _span_get)
#define ARRAY_NAME_span_set            CONCAT(TEMPLATE_ARRAY_NAME, _span_set)
#define ARRAY_NAME_span_at             CONCAT(TEMPLATE_ARRAY_NAME, _span_at)

#define ARRAY_NAME_t                   CONCAT(TEMPLATE_ARRAY_NAME, _t)
#define ARRAY_NAME_make                CONCAT(TEMPLATE_ARRAY_NAME, _make)
#define ARRAY_NAME_make_copy           CONCAT(TEMPLATE_ARRAY_NAME, _make_copy)
#define ARRAY_NAME_get                 CONCAT(TEMPLATE_ARRAY_NAME, _get)
#define ARRAY_NAME_set                 CONCAT(TEMPLATE_ARRAY_NAME, _set)
#define ARRAY_NAME_at                  CONCAT(TEMPLATE_ARRAY_NAME, _at)
#define ARRAY_NAME_reserve             CONCAT(TEMPLATE_ARRAY_NAME, _reserve)
#define ARRAY_NAME_resize              CONCAT(TEMPLATE_ARRAY_NAME, _resize)
#define ARRAY_NAME_grow                CONCAT(TEMPLATE_ARRAY_NAME, _grow)
#define ARRAY_NAME_add                 CONCAT(TEMPLATE_ARRAY_NAME, _add)


// Define array_view type:
// A read-only view of an array of objects

typedef struct ARRAY_NAME_view_t {
    const ARRAY_TYPE_t *data;
    uint32_t num;
} ARRAY_NAME_view_t;


// Define array_span type:
// A read/write fixed span of objects

typedef struct ARRAY_NAME_span_t {
    union {
        struct {
            ARRAY_TYPE_t *data;
            uint32_t num;
        };
        ARRAY_NAME_view_t view;
    };
} ARRAY_NAME_span_t;


// Define array type:
// An array of objects

typedef struct ARRAY_NAME_t {
    union {
        struct {
            ARRAY_TYPE_t *data;
            uint32_t num;
        };
        ARRAY_NAME_view_t view;
        ARRAY_NAME_span_t span;
    };
    uint32_t capacity;
} ARRAY_NAME_t;


// array_view implementation

static inline ARRAY_NAME_view_t ARRAY_NAME_view_make(const ARRAY_TYPE_t *ptr, uint32_t num) {
    assert(ptr);
    return (ARRAY_NAME_view_t) {
        .data = ptr,
        .num = num
    };
}

static inline ARRAY_NAME_view_t ARRAY_NAME_view_make_subview(ARRAY_NAME_view_t view, uint32_t start, uint32_t end) {
    assert(view.data);
    assert(start <= view.num);
    assert(end <= view.num);
    assert(start <= end);
    return (ARRAY_NAME_view_t) {
        .data = view.data + start,
        .num = end - start
    };
}

static inline ARRAY_TYPE_t ARRAY_NAME_view_get(ARRAY_NAME_view_t view, uint32_t index) {
    assert(view.data);
    assert(index < view.num);
    return view.data[index];
}

static inline const ARRAY_TYPE_t *ARRAY_NAME_view_at(ARRAY_NAME_view_t view, uint32_t index) {
    assert(view.data);
    assert(index < view.num);
    return view.data + index;
}


// array_span implementation

static inline ARRAY_NAME_span_t ARRAY_NAME_span_make(uint32_t num, arena_t *arena) {
    assert(arena);
    return (ARRAY_NAME_span_t) {
        .data = arena_calloc(arena, num * sizeof(ARRAY_TYPE_t)),
        .num = num,
    };
}

static inline ARRAY_NAME_span_t ARRAY_NAME_span_make_subspan(ARRAY_NAME_span_t span, uint32_t start, uint32_t end) {
    assert(span.data);
    assert(start <= span.num);
    assert(end <= span.num);
    assert(start <= end);
    return (ARRAY_NAME_span_t) {
        .data = span.data + start,
        .num = end - start
    };
}

static inline ARRAY_TYPE_t ARRAY_NAME_span_get(ARRAY_NAME_span_t span, uint32_t index) {
    assert(span.data);
    assert(index < span.num);
    return span.data[index];
}

static inline void ARRAY_NAME_span_set(ARRAY_NAME_span_t span, uint32_t index, ARRAY_TYPE_t value) {
    assert(span.data);
    assert(index < span.num);
    span.data[index] = value;
}

static inline ARRAY_TYPE_t *ARRAY_NAME_span_at(ARRAY_NAME_span_t span, uint32_t index) {
    assert(span.data);
    assert(index < span.num);
    return span.data + index;
}


// array implementation

static inline ARRAY_NAME_t ARRAY_NAME_make(uint32_t initial_capacity, arena_t *arena) {
    assert(arena);
    return (ARRAY_NAME_t) {
        .data = arena_calloc(arena, initial_capacity * sizeof(ARRAY_TYPE_t)),
        .num = 0,
        .capacity = initial_capacity
    };
}

static inline ARRAY_NAME_t ARRAY_NAME_make_copy(ARRAY_NAME_view_t view, uint32_t initial_capacity, arena_t *arena) {
    assert(arena);
    assert(view.data);
    uint32_t capacity = max_uint32(view.num, initial_capacity);
    ARRAY_TYPE_t *data = arena_calloc(arena, capacity * sizeof(ARRAY_TYPE_t));
    memcpy(data, view.data, view.num * sizeof(ARRAY_TYPE_t));
    return (ARRAY_NAME_t) {
        .data = data,
        .num = view.num,
        .capacity = capacity
    };
}

static inline ARRAY_TYPE_t ARRAY_NAME_get(const ARRAY_NAME_t *array, uint32_t index) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    return array->data[index];
}

static inline void ARRAY_NAME_set(ARRAY_NAME_t *array, uint32_t index, ARRAY_TYPE_t value) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    array->data[index] = value;
}

static inline ARRAY_TYPE_t *ARRAY_NAME_at(ARRAY_NAME_t *array, uint32_t index) {
    assert(array);
    assert(array->data);
    assert(index < array->num);
    return array->data + index;
}

static inline void ARRAY_NAME_reserve(ARRAY_NAME_t *array, uint32_t capacity, arena_t *arena) {
    assert(array);
    if (capacity > array->capacity) {
        assert(arena);
        array->data = arena_realloc(arena, array->data, array->capacity * sizeof(ARRAY_TYPE_t), capacity * sizeof(ARRAY_TYPE_t));
        array->capacity = capacity;
    }
}

static inline ARRAY_NAME_span_t ARRAY_NAME_resize(ARRAY_NAME_t *array, uint32_t num, arena_t *arena) {
    assert(array);
    ARRAY_NAME_reserve(array, num, arena);
    array->num = num;
    return array->span;
}

static inline void ARRAY_NAME_grow(ARRAY_NAME_t *array, uint32_t num, arena_t *arena) {
    assert(array);
    if (num > array->capacity) {
        ARRAY_NAME_reserve(array, max_uint32(num * 2, 16), arena);
    }
}

static inline uint32_t ARRAY_NAME_add(ARRAY_NAME_t *array, ARRAY_TYPE_t value, arena_t *arena) {
    assert(array);
    ARRAY_NAME_grow(array, array->num + 1, arena);
    array->data[array->num] = value;
    return array->num++;
}


// Undefine everything

#undef ARRAY_TYPE_t
#undef ARRAY_NAME_view_t
#undef ARRAY_NAME_view_make
#undef ARRAY_NAME_view_make_subview
#undef ARRAY_NAME_view_get
#undef ARRAY_NAME_view_at
#undef ARRAY_NAME_span_t
#undef ARRAY_NAME_span_make
#undef ARRAY_NAME_span_make_subspan
#undef ARRAY_NAME_span_get
#undef ARRAY_NAME_span_set
#undef ARRAY_NAME_span_at
#undef ARRAY_NAME_t
#undef ARRAY_NAME_make
#undef ARRAY_NAME_make_copy
#undef ARRAY_NAME_get
#undef ARRAY_NAME_set
#undef ARRAY_NAME_at
#undef ARRAY_NAME_reserve
#undef ARRAY_NAME_resize
#undef ARRAY_NAME_grow
#undef ARRAY_NAME_add

#undef TEMPLATE_ARRAY_NAME
#undef TEMPLATE_ARRAY_TYPE
