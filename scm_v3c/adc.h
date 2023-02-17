#ifndef __ADC_H
#define __ADC_H

#include <stdbool.h>
#include <stdint.h>

// ADC reset signal source enum.
typedef enum {
    ADC_RESET_SOURCE_INVALID = -1,
    ADC_RESET_SOURCE_FSM = 0,
    ADC_RESET_SOURCE_GPI = 1,
} adc_reset_source_e;

// ADC convert signal source enum.
typedef enum {
    ADC_CONVERT_SOURCE_INVALID = -1,
    ADC_CONVERT_SOURCE_FSM = 0,
    ADC_CONVERT_SOURCE_GPI = 1,
} adc_convert_source_e;

// PGA amplify signal source enum.
typedef enum {
    ADC_PGA_AMPLIFY_SOURCE_INVALID = -1,
    ADC_PGA_AMPLIFY_SOURCE_FSM = 0,
    ADC_PGA_AMPLIFY_SOURCE_GPI = 1,
} adc_pga_amplify_source_e;

// ADC input mux select enum.
typedef enum {
    ADC_INPUT_MUX_SELECT_INVALID = -1,
    ADC_INPUT_MUX_SELECT_V_PTAT = 0,
    ADC_INPUT_MUX_SELECT_V_BAT_DIV_4 = 1,
    ADC_INPUT_MUX_SELECT_EXTERNAL_SIGNAL = 2,
    ADC_INPUT_MUX_SELECT_FLOATING = 3,
} adc_input_mux_select_e;

// ADC config.
typedef struct __attribute__((packed)) {
    // ADC reset signal source.
    adc_reset_source_e reset_source;

    // ADC convert signal source.
    adc_convert_source_e convert_source;

    // PGA amplify signal source.
    adc_pga_amplify_source_e pga_amplify_source;

    // 8-bit PGA gain. The actual gain is pga_gain + 1 for a range from 1 to
    // 256.
    uint8_t pga_gain;

    // 8-bit ADC settling time. 0x00 gives the ADC roughly 1.72 us to retrieve
    // all 10 bits, and 0xFF gives the ADC roughly 0.35 us to retrieve all 10
    // bits.
    uint8_t settling_time;

    // 7-bit bandgap reference tuning code to control the reference voltage to
    // the LDO. The MSB is the panic bit and sets the reference voltage to the
    // maximum, which is around 1.2 V. 0x00 sets the reference voltage to the
    // minimum, which is around 0.8 V.
    uint8_t bandgap_reference_tuning_code;

    // 8-bit const gm device tuning code to control the reference current used
    // in the ADC's comparator and the PGA. Increasing the tuning code decreases
    // the reference current.
    uint8_t const_gm_tuning_code;

    // If true, the VBAT / 4 input to the mux is enabled. Note that this does
    // not choose VBAT / 4 as the output of the mux.
    bool vbat_div_4_enabled;

    // If true, the on-chip LDO is enabled.
    bool ldo_enabled;

    // 2-bit ADC input mux select.
    adc_input_mux_select_e input_mux_select;

    // If true, the PGA is bypassed.
    bool pga_bypass;
} adc_config_t;

// ADC output.
typedef struct __attribute__((packed)) {
    // 10-bit ADC output.
    uint16_t data;

    // If true, ADC conversion has finished and the output is valid.
    bool valid;
} adc_output_t;

// ADC output.
extern adc_output_t g_adc_output;

// Configure the ADC according to the given ADC configuration.
void adc_config(const adc_config_t* adc_config);

// Trigger an ADC read.
void adc_trigger(void);

// Enable the ADC interrupt.
void adc_enable_interrupt(void);

// Disable the ADC interrupt.
void adc_disable_interrupt(void);

#endif  // __ADC_H
