#ifndef UTILS_H_
#define UTILS_H_

#include <stdint.h>


uint32_t get_bit_width(uint32_t x);

uint32_t get_elias_gamma_cost(uint32_t value);

uint32_t get_hybrid_cost(uint32_t value, uint32_t num_fixed_bits);


#endif // ifndef UTILS_H_
