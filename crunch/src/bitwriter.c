#include "bitwriter.h"
#include <assert.h>


bitwriter_t bitwriter_make(arena_t *arena, uint32_t initial_capacity) {
    assert(arena);
    return (bitwriter_t) {
        .data = byte_array_make(arena, initial_capacity),
        .bit = 0,
        .index_being_built = 0
    };
}


void bitwriter_add_bit(bitwriter_t *bitwriter, uint32_t value, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    if (bitwriter->bit == 0) {
        bitwriter->index_being_built = byte_array_add(&bitwriter->data, 0, arena);
    }
    if (value) {
        uint8_t value = byte_array_get(&bitwriter->data, bitwriter->index_being_built) | (1 << bitwriter->bit);
        byte_array_set(&bitwriter->data, bitwriter->index_being_built, value);
    }
    bitwriter->bit = (bitwriter->bit + 1) & 0x07;
}


void bitwriter_add_value(bitwriter_t *bitwriter, uint32_t value, uint32_t numbits, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    assert(numbits <= 8);
    for (uint32_t n = numbits; n-- > 0;) {
        bitwriter_add_bit(bitwriter, value & (1 << n), arena);
    }
}


void bitwriter_add_aligned_byte(bitwriter_t *bitwriter, uint32_t byte, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    byte_array_add(&bitwriter->data, byte, arena);
}


static inline uint32_t bit_width(uint32_t x) {
    assert(x > 0);
    uint32_t n = 0;
    while (x) {
        n++;
        x >>= 1;
    }
    return n;
}


void bitwriter_add_elias_gamma_value(bitwriter_t *bitwriter, uint32_t value, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    assert(value > 0);
    uint32_t numbits = bit_width(value);
    bitwriter_add_value(bitwriter, 0, numbits - 1, arena);
    bitwriter_add_value(bitwriter, value, numbits, arena);
}
