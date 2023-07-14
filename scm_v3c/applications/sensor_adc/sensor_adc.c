#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "adc.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

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

int main(void) {
    initialize_mote();

    // Configure the ADC.
    printf("Configuring the ADC.\n");
    adc_config(&g_adc_config);
    adc_enable_interrupt();

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    while (true) {
        // Trigger an ADC read.
        printf("Triggering ADC.\n");
        adc_trigger();
        while (!g_adc_output.valid) {
        }
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        printf("ADC output: %u\n", g_adc_output.data);

        // Wait for around 1 second.
        for (uint32_t i = 0; i < 700000; ++i) {
        }
    }
}
