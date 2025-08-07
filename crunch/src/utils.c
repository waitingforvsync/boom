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


