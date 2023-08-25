#ifndef __FIXED_POINT_H
#define __FIXED_POINT_H

#include <stdint.h>

// Number of integer bits.
#define FIXED_POINT_P 16

// Number of fractional bits.
#define FIXED_POINT_Q 16

// Conversion factor to and from fixed point integers.
#define FIXED_POINT_F (1 << FIXED_POINT_Q)

// Fixed point number type.
typedef int32_t fixed_point_t;

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
    if (g > 0) {
        const fixed_point_t g_upper_bits = g & 0xFFFF8000;
        const fixed_point_t g_lower_bits = g & 0x7FFF;
        return (fixed_point_t)(((int64_t)f * (int64_t)g_upper_bits) >>
                               FIXED_POINT_Q) +
               (fixed_point_t)(((int64_t)f * (int64_t)g_lower_bits) >>
                               FIXED_POINT_Q);
    }
    const fixed_point_t g_upper_bits = (-g) & 0xFFFF8000;
    const fixed_point_t g_lower_bits = (-g) & 0x7FFF;
    return -(
        (fixed_point_t)(((int64_t)f * (int64_t)g_upper_bits) >> FIXED_POINT_Q) +
        (fixed_point_t)(((int64_t)f * (int64_t)g_lower_bits) >> FIXED_POINT_Q));
}

// Divide two fixed point numbers.
static inline fixed_point_t fixed_point_divide(const fixed_point_t f,
                                               const fixed_point_t g) {
    return ((int64_t)f << FIXED_POINT_Q) / g;
}

#endif  // __FIXED_POINT_H
