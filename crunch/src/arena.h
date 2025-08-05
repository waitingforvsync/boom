#ifndef ARENA_H_
#define ARENA_H_

#include <stdint.h>

typedef struct arena_t {
    void *base;
    uint32_t next;
    uint32_t end;
} arena_t;


// Make a new arena
arena_t arena_make(uint32_t size);

// Initialise an existing arena
void arena_init(arena_t *arena, uint32_t total_size);

// Deinitialise an arena
void arena_deinit(arena_t *arena);

// Allocate a block from the arena
void *arena_alloc(arena_t *arena, uint32_t size);

// Allocate a zeroed block from the arena
void *arena_calloc(arena_t *arena, uint32_t size);

// Reallocate a block from the arena
void *arena_realloc(arena_t *arena, void *oldptr, uint32_t old_size, uint32_t new_size);

// Free a block from the arena, if it's the last
void arena_free(arena_t *arena, void *ptr, uint32_t size);

// Free the arena
void arena_reset(arena_t *arena);


#endif // ifndef ARENA_H_
