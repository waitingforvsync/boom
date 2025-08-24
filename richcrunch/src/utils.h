#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>
#include <assert.h>


// Return the minimum of two int32s
static inline int32_t min_int32(int32_t a, int32_t b) {
    return (a < b) ? a : b;
}

// Return the minimum of two uint32s
static inline uint32_t min_uint32(uint32_t a, uint32_t b) {
    return (a < b) ? a : b;
}

// Return the maximum of two int32s
static inline int32_t max_int32(int32_t a, int32_t b) {
    return (a > b) ? a : b;
}

// Return the maximum of two uint32s
static inline uint32_t max_uint32(uint32_t a, uint32_t b) {
    return (a > b) ? a : b;
}

// Returns the minimum number of bits required to represent the argument
uint32_t get_bit_width(uint32_t x);


static inline uint32_t get_elias_gamma_cost(uint32_t value) {
    assert(value > 0 && value <= 256);  // 256 will be read as 0
    return get_bit_width(value) * 2 - 1;
}

static inline uint32_t get_hybrid_cost(uint32_t value, uint32_t num_fixed_bits) {
    return get_elias_gamma_cost((value >> num_fixed_bits) + 1) + num_fixed_bits;
}




#endif // ifndef UTILS_H_
