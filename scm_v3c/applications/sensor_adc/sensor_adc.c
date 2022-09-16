#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "optical.h"
#include "adc.h"

//=========================== defines =========================================

//=========================== variables =======================================

typedef struct {
    adc_config_t adc_config;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t i = 0;

    initialize_mote();

    // Configure the ADC.
    printf("Configuring the ADC...\n");
    app_vars.adc_config.reset_source = ADC_RESET_SOURCE_FSM;
    app_vars.adc_config.convert_source = ADC_CONVERT_SOURCE_FSM;
    app_vars.adc_config.pga_amplify_source = ADC_PGA_AMPLIFY_SOURCE_FSM;
    app_vars.adc_config.pga_gain = 0;
    app_vars.adc_config.settling_time = 0;
    app_vars.adc_config.bandgap_reference_tuning_code = 1;
    app_vars.adc_config.const_gm_tuning_code = 0xFF;
    app_vars.adc_config.vbat_div_4_enabled = true;
    app_vars.adc_config.ldo_enabled = true;
    app_vars.adc_config.input_mux_select = ADC_INPUT_MUX_SELECT_V_BAT_DIV_4;
    app_vars.adc_config.pga_bypass = true;
    adc_config(&app_vars.adc_config);
    adc_enable_interrupt();

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    while (true) {
        // Trigger an ADC read.
        printf("Triggering ADC.\n");
        adc_trigger();
        while (!g_adc_output.valid);
        if (!g_adc_output.valid) {
            printf("ADC output should be valid.\n");
        }
        printf("ADC output: %u\n", g_adc_output.data);

        // Wait a bit.
        for (i = 0; i < 1000000; i++);
    }
}

//=========================== public ==========================================

//=========================== private =========================================
