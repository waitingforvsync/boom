#include "utils.h"
#include <assert.h>


uint32_t get_bit_width(uint32_t x) {
    assert(x > 0);
    uint32_t n = 0;
    while (x) {
        n++;
        x >>= 1;
    }
    return n;
}


uint32_t get_elias_gamma_cost(uint32_t value) {
    assert(value > 0 && value <= 256);  // 256 will be read as 0
    return get_bit_width(value) * 2 - 1;
}


uint32_t get_hybrid_cost(uint32_t value, uint32_t num_fixed_bits) {
    return get_elias_gamma_cost((value >> num_fixed_bits) + 1) + num_fixed_bits;
}
