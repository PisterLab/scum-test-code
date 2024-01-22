#ifndef __ADC_MSB_H
#define __ADC_MSB_H

#include <stdbool.h>
#include <stdint.h>

// Maximum ADC sample.
#define ADC_MAX_ADC_SAMPLE 511

// Maximum theoretical ADC sample.
#define ADC_MAX_THEORETICAL_ADC_SAMPLE 1023

// Initialize the ADC MSB disambiguator.
void adc_msb_init(uint16_t adc_sample);

// Disambiguate the given ADC sample.
bool adc_msb_disambiguate(uint16_t adc_sample);

// Get the latest disambiguated ADC sample.
uint16_t adc_msb_get_disambiguated_sample(void);

#endif  // __ADC_MSB_H
