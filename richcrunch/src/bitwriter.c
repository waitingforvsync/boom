#include "bitwriter.h"
#include "utils.h"
#include <assert.h>


bitwriter_t bitwriter_make(uint32_t initial_capacity, arena_t *arena) {
    assert(arena);
    return (bitwriter_t) {
        .data = byte_array_make(initial_capacity, arena),
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
//    assert(numbits <= 8 || (numbits == 9 && value == 256));
    for (uint32_t n = numbits; n-- > 0;) {
        bitwriter_add_bit(bitwriter, value & (1 << n), arena);
    }
}


void bitwriter_add_aligned_byte(bitwriter_t *bitwriter, uint32_t byte, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    byte_array_add(&bitwriter->data, byte, arena);
}


void bitwriter_add_elias_gamma_value(bitwriter_t *bitwriter, uint32_t value, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    assert(value > 0 && value <= 256);  // 256 will be read as 0
    uint32_t numbits = get_bit_width(value);
    bitwriter_add_value(bitwriter, 0, numbits - 1, arena);
    bitwriter_add_value(bitwriter, value, numbits, arena);
}


void bitwriter_add_hybrid_value(bitwriter_t *bitwriter, uint32_t value, uint32_t fixed_bits, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    bitwriter_add_elias_gamma_value(bitwriter, (value >> fixed_bits) + 1, arena);
    bitwriter_add_value(bitwriter, value, fixed_bits, arena);
}


void bitwriter_add_huffman_code(bitwriter_t *bitwriter, uint16_array_view_t huffman_codes, uint32_t value, arena_t *arena) {
    assert(bitwriter);
    assert(bitwriter->data.data);
    assert(huffman_codes.data);
    uint16_t huffman_code = uint16_array_view_get(huffman_codes, value);
    assert(huffman_code != 0);
    bitwriter_add_value(bitwriter, huffman_code, get_bit_width(huffman_code) - 1, arena);
}
