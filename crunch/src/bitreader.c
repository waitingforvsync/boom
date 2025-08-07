#include "bitreader.h"


bitreader_t bitreader_make(byte_array_view_t data) {
    return (bitreader_t) {
        .data = data,
        .bit = 0,
        .cached_byte = 0,
        .index = 0
    };
}


uint32_t bitreader_get_bit(bitreader_t *bitreader) {
    assert(bitreader);
    assert(bitreader->data.data);
    if (bitreader->bit == 0) {
        bitreader->cached_byte = byte_array_view_get(bitreader->data, bitreader->index++);
    }

    uint32_t value = !!(bitreader->cached_byte & (1 << bitreader->bit));
    bitreader->bit = (bitreader->bit + 1) & 0x07;
    return value;
}


uint8_t bitreader_get_value(bitreader_t *bitreader, uint32_t numbits) {
    assert(bitreader);
    assert(bitreader->data.data);
    assert(numbits <= 8);
    uint8_t value = 0;
    for (uint32_t n = 0; n < numbits; n++) {
        value = (value << 1) | bitreader_get_bit(bitreader);
    }
    return value;
}


uint8_t bitreader_get_aligned_byte(bitreader_t *bitreader) {
    assert(bitreader);
    assert(bitreader->data.data);
    return byte_array_view_get(bitreader->data, bitreader->index++);
}


uint8_t bitreader_get_elias_gamma_value(bitreader_t *bitreader) {
    assert(bitreader);
    assert(bitreader->data.data);
    uint8_t numbits = 0;
    while (!bitreader_get_bit(bitreader)) {
        numbits++;
    }
    uint8_t value = 1;
    for (uint32_t n = 0; n < numbits; n++) {
        value = (value << 1) | bitreader_get_bit(bitreader);
    }
    return value;
}


uint16_t bitreader_get_hybrid_value(bitreader_t *bitreader, uint32_t fixed_bits) {
    assert(bitreader);
    assert(bitreader->data.data);
    uint16_t value = (bitreader_get_elias_gamma_value(bitreader) - 1) & 0xFF;
    return (value << fixed_bits) | bitreader_get_value(bitreader, fixed_bits);
}
