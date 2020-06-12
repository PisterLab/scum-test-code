/* Fixed-point library. */

#define FIX_P 22               // Integer bits
#define FIX_Q 10               // Fractional bits
#define FIX_F (1 << FIX_Q)     // 2^FIX_Q

typedef int fixed_point_t;

static fixed_point_t fix_init(int f) {
	return f * FIX_F;
}

static int fix_int(fixed_point_t f) {
	return f / FIX_F;
}

static double fix_double(fixed_point_t f) {
  return fix_int(f) + (f & ((1 << FIX_Q) - 1)) * (1 / ((double) (1 << FIX_Q)));
}

static fixed_point_t fix_add(fixed_point_t f, fixed_point_t g) {
	return f + g;
}

static fixed_point_t fix_sub(fixed_point_t f, fixed_point_t g) {
	return f - g;
}

static fixed_point_t fix_mul(fixed_point_t f, fixed_point_t g) {
	return (long long) f * g / FIX_F;
}

static fixed_point_t fix_div(fixed_point_t f, fixed_point_t g) {
	return (long long) f * FIX_F / g;
}
