#ifndef __FIXED_POINT_H
#define __FIXED_POINT_H

#include <stdint.h>
#include <stdlib.h>

// Number of integer bits.
#define FIXED_POINT_P 50

// Number of short integer bits.
#define FIXED_POINT_SHORT_P 18

// Number of fractional bits.
#define FIXED_POINT_Q 14

// Conversion factor to and from fixed point integers.
#define FIXED_POINT_F (1 << FIXED_POINT_Q)

// Fixed point number type.
typedef int64_t fixed_point_t;

// Fixed point short number type.
typedef int32_t fixed_point_short_t;

// Initialize a fixed point number from an integer.
static inline fixed_point_t fixed_point_init(const int32_t f) {
    return f << FIXED_POINT_Q;
}

// Convert from a fixed point number to an integer.
static inline int32_t fixed_point_integer(const fixed_point_t f) {
    return f >> FIXED_POINT_Q;
}

// Add two fixed point numbers.
static inline fixed_point_t fixed_point_add(const fixed_point_t f,
                                            const fixed_point_t g) {
    return f + g;
}

// Subtract two fixed point numbers.
static inline fixed_point_t fixed_point_subtract(const fixed_point_t f,
                                                 const fixed_point_t g) {
    return f - g;
}

// Multiply two fixed point numbers.
static inline fixed_point_t fixed_point_multiply(const fixed_point_t f,
                                                 const fixed_point_t g) {
    const fixed_point_t f_abs = f > 0 ? f : -f;
    const fixed_point_t g_abs = g > 0 ? g : -g;
    int64_t product_abs_without_shift = 0;
    for (size_t i = 0; i < sizeof(fixed_point_t) * 8; ++i) {
        if ((g_abs >> i) & 0x1 == 1) {
            product_abs_without_shift += (int64_t)f_abs << i;
        }
    }
    const fixed_point_t product_abs =
        product_abs_without_shift >> FIXED_POINT_Q;
    return (f < 0 ^ g < 0) ? -product_abs : product_abs;
}

// Divide two fixed point numbers.
static inline fixed_point_t fixed_point_divide(const fixed_point_t f,
                                               const fixed_point_t g) {
    return ((int64_t)f << FIXED_POINT_Q) / g;
}

#endif  // __FIXED_POINT_H
