#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "fixed_point.h"
#include "memory_map.h"
#include "optical.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"
#include "time_constant.h"

// ADC configuration.
static const adc_config_t g_adc_config = {
    .reset_source = ADC_RESET_SOURCE_FSM,
    .convert_source = ADC_CONVERT_SOURCE_FSM,
    .pga_amplify_source = ADC_PGA_AMPLIFY_SOURCE_FSM,
    .pga_gain = 0,
    .settling_time = 0,
    .bandgap_reference_tuning_code = 1,
    .const_gm_tuning_code = 0xFF,
    .vbat_div_4_enabled = false,
    .ldo_enabled = true,
    .input_mux_select = ADC_INPUT_MUX_SELECT_EXTERNAL_SIGNAL,
    .pga_bypass = true,
};

static void rftimer_callback(void) {
    // Trigger an ADC read.
    adc_trigger();
    delay_milliseconds_asynchronous(/*delay_milli=*/10, 7);
}

int main(void) {
    initialize_mote();

    // Configure the RF timer.
    rftimer_set_callback_by_id(rftimer_callback, 7);
    rftimer_enable_interrupts();
    rftimer_enable_interrupts_by_id(7);

    // Configure the ADC.
    printf("Configuring the ADC.\n");
    adc_config(&g_adc_config);
    adc_enable_interrupt();

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    // TODO(titan): Set up GPIOs to drive the switch.

    time_constant_init();
    delay_milliseconds_asynchronous(TIME_CONSTANT_SAMPLING_PERIOD_MS, 7);
    while (true) {
        if (g_adc_output.valid) {
            time_constant_add_sample(g_adc_output.data);
            g_adc_output.valid = false;
            if (time_constant_has_sufficient_samples()) {
                delay_milliseconds_asynchronous(TIME_CONSTANT_MEASUREMENT_PERIOD_MS, 7);
                printf("Received sufficient samples.\n");
                const fixed_point_t estimated_time_constant = time_constant_estimate();
                printf("Estimated time constant: %lld / %lld\n",
                       estimated_time_constant, fixed_point_init(1));
                time_constant_init();
            }
        }
    }
}
