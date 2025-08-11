#ifndef BITREADER_H_
#define BITREADER_H_

#include "arena.h"
#include "byte_array.h"


typedef struct bitreader_t {
    byte_array_view_t data;
    uint32_t bit;
    uint32_t cached_byte;
    uint32_t index;
} bitreader_t;


// Make a new bitreader
bitreader_t bitreader_make(byte_array_view_t data);

// Read a bit from the stream
uint32_t bitreader_get_bit(bitreader_t *bitreader);

// Read a value from the stream
uint8_t bitreader_get_value(bitreader_t *bitreader, uint32_t numbits);

// Read an aligned byte from the stream
uint8_t bitreader_get_aligned_byte(bitreader_t *bitreader);

// Read an elias gamma value from the stream
uint8_t bitreader_get_elias_gamma_value(bitreader_t *bitreader);

// Read a hybrid (fixed+elias) value from the stream
uint16_t bitreader_get_hybrid_value(bitreader_t *bitreader, uint32_t fixed_bits);


#endif // ifndef BITREADER_H_
