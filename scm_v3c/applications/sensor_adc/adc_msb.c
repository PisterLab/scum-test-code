#include "adc_msb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// Previous ADC sample.
uint16_t g_adc_msb_previous_sample = 0;

void adc_msb_init(const uint16_t adc_sample) {
    g_adc_msb_previous_sample = adc_sample;
}

bool adc_msb_disambiguate(const uint16_t adc_sample) {
    // Add multiples of 512 to minimize the difference between consecutive ADC
    // samples.
    if (adc_sample < g_adc_msb_previous_sample) {
        const size_t num_multiples =
            (g_adc_msb_previous_sample - adc_sample) / (ADC_MAX_ADC_SAMPLE + 1);
        if (g_adc_msb_previous_sample -
                (adc_sample + num_multiples * (ADC_MAX_ADC_SAMPLE + 1)) <
            (adc_sample + (num_multiples + 1) * (ADC_MAX_ADC_SAMPLE + 1) -
             g_adc_msb_previous_sample)) {
            g_adc_msb_previous_sample =
                adc_sample + num_multiples * (ADC_MAX_ADC_SAMPLE + 1);
        } else {
            g_adc_msb_previous_sample =
                adc_sample + (num_multiples + 1) * (ADC_MAX_ADC_SAMPLE + 1);
        }

        // Limit the ADC sample to 10 bits.
        if (g_adc_msb_previous_sample > ADC_MAX_THEORETICAL_ADC_SAMPLE) {
            g_adc_msb_previous_sample -= (ADC_MAX_ADC_SAMPLE + 1);
        }
    } else {
        g_adc_msb_previous_sample = adc_sample;
    }
    return true;
}

uint16_t adc_msb_get_disambiguated_sample(void) {
    return g_adc_msb_previous_sample;
}
