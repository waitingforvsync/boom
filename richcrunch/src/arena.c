#include "arena.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define ARENA_ALIGNMENT 16


static uint32_t get_aligned_size(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1);
}


arena_t arena_make(uint32_t size) {
    arena_t arena = {0};
    arena_init(&arena, size);
    return arena;
}


void arena_init(arena_t *arena, uint32_t total_size) {
    assert(arena);
    if (arena->base) {
        arena_deinit(arena);
    }

    total_size = get_aligned_size(total_size, ARENA_ALIGNMENT);
    void *block = calloc(total_size, 1);
    if (!block) {
        abort();
    }
    assert(((uintptr_t)block & (ARENA_ALIGNMENT - 1)) == 0);

    arena->base = block;
    arena->next = 0;
    arena->end  = total_size;
}


void arena_deinit(arena_t *arena) {
    assert(arena);
    if (arena->base) {
        free(arena->base);
    }
    arena->base = 0;
    arena->next = 0;
    arena->end  = 0;
}


void *arena_alloc(arena_t *arena, uint32_t size) {
    assert(arena);
    assert(arena->base);
    uint32_t aligned_size = get_aligned_size(size, ARENA_ALIGNMENT);
    if (arena->next + aligned_size > arena->end) {
        abort();
    }
    
    void *ptr = (char *)arena->base + arena->next;
    arena->next += aligned_size;
    return ptr;
}


void *arena_calloc(arena_t *arena, uint32_t size) {
    void *ptr = arena_alloc(arena, size);
    memset(ptr, 0, get_aligned_size(size, ARENA_ALIGNMENT));
    return ptr;
}


void *arena_realloc(arena_t *arena, void *oldptr, uint32_t old_size, uint32_t new_size) {
    assert(arena);
    assert(arena->base);
    if (!oldptr || old_size == 0) {
        return arena_alloc(arena, new_size);
    }

    uint32_t old_aligned_size = get_aligned_size(old_size, ARENA_ALIGNMENT);
    uint32_t new_aligned_size = get_aligned_size(new_size, ARENA_ALIGNMENT);
    if ((char *)oldptr + old_aligned_size == (char *)arena->base + arena->next) {
        arena->next += (new_aligned_size - old_aligned_size);
        if (arena->next > arena->end) {
            abort();
        }
        memset((char *)oldptr + old_aligned_size, 0, new_aligned_size - old_aligned_size);
        return oldptr;
    }
    else {
        if (arena->next + new_aligned_size > arena->end) {
            abort();
        }
        void *newptr = (char *)arena->base + arena->next;
        arena->next += new_aligned_size;
        memcpy(newptr, oldptr, old_aligned_size);
        memset((char *)newptr + old_aligned_size, 0, new_aligned_size - old_aligned_size);
        return newptr;
    }
}


arena_t arena_alloc_subarena(arena_t *arena, uint32_t size) {
    assert(arena);
    size = get_aligned_size(size, ARENA_ALIGNMENT);
    return (arena_t) {
        .base = arena_alloc(arena, size),
        .next = 0,
        .end = size
    };
}


void arena_free(arena_t *arena, void *ptr, uint32_t size) {
    assert(arena);
    assert(arena->base);
    uint32_t aligned_size = get_aligned_size(size, ARENA_ALIGNMENT);
    if ((char *)ptr + aligned_size == (char *)arena->base + arena->next) {
        arena->next -= aligned_size;
    }
}


void arena_reset(arena_t *arena) {
    assert(arena);
    arena->next = 0;
}

