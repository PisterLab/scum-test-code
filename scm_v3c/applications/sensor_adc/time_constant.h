#ifndef __TIME_CONSTANT_H
#define __TIME_CONSTANT_H

#include <stdbool.h>
#include <stdint.h>

#include "fixed_point.h"

// Initialize the time constant analysis. Initialization should be performed
// prior to each run.
void time_constant_init(uint32_t time_constant_sampling_period_ms);

// Add an ADC sample to the time constant analysis. Return whether the sample
// was added successfully.
bool time_constant_add_sample(uint16_t adc_sample);

// Return whether there are sufficient ADC samples.
bool time_constant_has_sufficient_samples(void);

// Estimate the time constant in seconds.
fixed_point_t time_constant_estimate(void);

#endif  // __TIME_CONSTANT_H
