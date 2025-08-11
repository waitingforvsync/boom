#ifndef BITWRITER_H_
#define BITWRITER_H_

#include "arena.h"
#include "byte_array.h"


typedef struct bitwriter_t {
    byte_array_t data;
    uint32_t bit;
    uint32_t index_being_built;
} bitwriter_t;


// Make a new bitwriter
bitwriter_t bitwriter_make(uint32_t initial_capacity, arena_t *arena);

// Add a bit to the stream
void bitwriter_add_bit(bitwriter_t *bitwriter, uint32_t value, arena_t *arena);

// Add a value to the stream
void bitwriter_add_value(bitwriter_t *bitwriter, uint32_t value, uint32_t numbits, arena_t *arena);

// Add an aligned byte to the stream
void bitwriter_add_aligned_byte(bitwriter_t *bitwriter, uint32_t byte, arena_t *arena);

// Add an elias gamma value to the stream
void bitwriter_add_elias_gamma_value(bitwriter_t *bitwriter, uint32_t value, arena_t *arena);

// Add a hybrid (fixed+elias) value to the stream
void bitwriter_add_hybrid_value(bitwriter_t *bitwriter, uint32_t value, uint32_t fixed_bits, arena_t *arena);


#endif // ifndef BITWRITER_H_
