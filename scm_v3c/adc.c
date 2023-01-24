#include "adc.h"

#include <stdint.h>
#include <stdio.h>

#include "memory_map.h"
#include "scm3c_hw_interface.h"

// ADC analog scan chain bit enum.
typedef enum {
    ADC_INVALD_ASC_BIT = -1,
    ADC_RESET_SOURCE_ASC_BIT = 242,
    ADC_CONVERT_SOURCE_ASC_BIT = 243,
    ADC_PGA_AMPLIFY_SOURCE_ASC_BIT = 244,
    ADC_CONST_GM_DEVICE_TUNING_CODE_0_ASC_BIT = 765,
    ADC_CONST_GM_DEVICE_TUNING_CODE_1_ASC_BIT = 764,
    ADC_CONST_GM_DEVICE_TUNING_CODE_2_ASC_BIT = 763,
    ADC_CONST_GM_DEVICE_TUNING_CODE_3_ASC_BIT = 762,
    ADC_CONST_GM_DEVICE_TUNING_CODE_4_ASC_BIT = 761,
    ADC_CONST_GM_DEVICE_TUNING_CODE_5_ASC_BIT = 760,
    ADC_CONST_GM_DEVICE_TUNING_CODE_6_ASC_BIT = 759,
    ADC_CONST_GM_DEVICE_TUNING_CODE_7_ASC_BIT = 758,
    ADC_PGA_GAIN_0_ASC_BIT = 766,
    ADC_PGA_GAIN_1_ASC_BIT = 767,
    ADC_PGA_GAIN_2_ASC_BIT = 768,
    ADC_PGA_GAIN_3_ASC_BIT = 769,
    ADC_PGA_GAIN_4_ASC_BIT = 770,
    ADC_PGA_GAIN_5_ASC_BIT = 771,
    ADC_PGA_GAIN_6_ASC_BIT = 800,
    ADC_PGA_GAIN_7_ASC_BIT = 773,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_0_ASC_BIT = 778,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_1_ASC_BIT = 784,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_2_ASC_BIT = 783,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_3_ASC_BIT = 782,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_4_ASC_BIT = 781,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_5_ASC_BIT = 780,
    ADC_BANDGAP_REFERENCE_TUNING_CODE_6_ASC_BIT = 779,
    ADC_VBAT_DIV_4_ENABLED_ASC_BIT = 798,
    ADC_LDO_ENABLED_ASC_BIT = 801,
    ADC_SETTLING_TIME_0_ASC_BIT = 816,
    ADC_SETTLING_TIME_1_ASC_BIT = 817,
    ADC_SETTLING_TIME_2_ASC_BIT = 818,
    ADC_SETTLING_TIME_3_ASC_BIT = 819,
    ADC_SETTLING_TIME_4_ASC_BIT = 820,
    ADC_SETTLING_TIME_5_ASC_BIT = 821,
    ADC_SETTLING_TIME_6_ASC_BIT = 822,
    ADC_SETTLING_TIME_7_ASC_BIT = 823,
    ADC_INPUT_MUX_SELECT_0_ASC_BIT = 915,
    ADC_INPUT_MUX_SELECT_1_ASC_BIT = 1087,
    ADC_PGA_BYPASS_ASC_BIT = 1088,
} adc_asc_bit_t;

// ADC output.
adc_output_t g_adc_output;

static void adc_set_asc_bit(adc_asc_bit_t asc_bit, uint8_t value) {
    if ((value & 0x1) == 1) {
        set_asc_bit((unsigned int)asc_bit);
    } else {
        clear_asc_bit((unsigned int)asc_bit);
    }
}

static void adc_set_pga_gain_asc_bits(uint8_t pga_gain) {
    adc_set_asc_bit(ADC_PGA_GAIN_0_ASC_BIT, (pga_gain >> 7) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_1_ASC_BIT, (pga_gain >> 6) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_2_ASC_BIT, (pga_gain >> 5) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_3_ASC_BIT, (pga_gain >> 4) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_4_ASC_BIT, (pga_gain >> 3) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_5_ASC_BIT, (pga_gain >> 2) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_6_ASC_BIT, (pga_gain >> 1) & 0x1);
    adc_set_asc_bit(ADC_PGA_GAIN_7_ASC_BIT, pga_gain & 0x1);
}

static void adc_set_settling_time_asc_bits(uint8_t settling_time) {
    adc_set_asc_bit(ADC_SETTLING_TIME_0_ASC_BIT, (settling_time >> 7) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_1_ASC_BIT, (settling_time >> 6) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_2_ASC_BIT, (settling_time >> 5) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_3_ASC_BIT, (settling_time >> 4) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_4_ASC_BIT, (settling_time >> 3) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_5_ASC_BIT, (settling_time >> 2) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_6_ASC_BIT, (settling_time >> 1) & 0x1);
    adc_set_asc_bit(ADC_SETTLING_TIME_7_ASC_BIT, settling_time & 0x1);
}

static void adc_set_bandgap_refernce_tuning_code_asc_bits(
    uint8_t bandgap_reference_tuning_code) {
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_0_ASC_BIT,
                    (bandgap_reference_tuning_code >> 6) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_1_ASC_BIT,
                    (bandgap_reference_tuning_code >> 5) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_2_ASC_BIT,
                    (bandgap_reference_tuning_code >> 4) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_3_ASC_BIT,
                    (bandgap_reference_tuning_code >> 3) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_4_ASC_BIT,
                    (bandgap_reference_tuning_code >> 2) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_5_ASC_BIT,
                    (bandgap_reference_tuning_code >> 1) & 0x1);
    adc_set_asc_bit(ADC_BANDGAP_REFERENCE_TUNING_CODE_6_ASC_BIT,
                    bandgap_reference_tuning_code & 0x1);
}

static void adc_set_const_gm_tuning_code_asc_bits(
    uint8_t const_gm_tuning_code) {
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_0_ASC_BIT,
                    (const_gm_tuning_code >> 7) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_1_ASC_BIT,
                    (const_gm_tuning_code >> 6) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_2_ASC_BIT,
                    (const_gm_tuning_code >> 5) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_3_ASC_BIT,
                    (const_gm_tuning_code >> 4) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_4_ASC_BIT,
                    (const_gm_tuning_code >> 3) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_5_ASC_BIT,
                    (const_gm_tuning_code >> 2) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_6_ASC_BIT,
                    (const_gm_tuning_code >> 1) & 0x1);
    adc_set_asc_bit(ADC_CONST_GM_DEVICE_TUNING_CODE_7_ASC_BIT,
                    const_gm_tuning_code & 0x1);
}

void adc_config(const adc_config_t* adc_config) {
    // Set the ASC bit for the ADC reset signal source.
    adc_set_asc_bit(ADC_RESET_SOURCE_ASC_BIT,
                    (uint8_t)adc_config->reset_source);

    // Set the ASC bit for the ADC convert signal source.
    adc_set_asc_bit(ADC_CONVERT_SOURCE_ASC_BIT,
                    (uint8_t)adc_config->convert_source);

    // Set the ASC bit for the PGA amplify signal source.
    adc_set_asc_bit(ADC_PGA_AMPLIFY_SOURCE_ASC_BIT,
                    (uint8_t)adc_config->pga_amplify_source);

    // Set the ASC bits for the PGA gain.
    adc_set_pga_gain_asc_bits(adc_config->pga_gain);

    // Set the ASC bits for the ADC settling time.
    adc_set_settling_time_asc_bits(adc_config->settling_time);

    // Set the ASC bits for the bandgap reference tuning code.
    adc_set_bandgap_refernce_tuning_code_asc_bits(
        adc_config->bandgap_reference_tuning_code);

    // Set the ASC bits for the const gm device tuning code.
    adc_set_const_gm_tuning_code_asc_bits(adc_config->const_gm_tuning_code);

    // Set the ASC bit for enabling the VBAT / 4 input.
    adc_set_asc_bit(ADC_VBAT_DIV_4_ENABLED_ASC_BIT,
                    adc_config->vbat_div_4_enabled);

    // Set the ASC bit for enabling the on-chip LDO.
    adc_set_asc_bit(ADC_LDO_ENABLED_ASC_BIT, adc_config->ldo_enabled);

    // Set the ASC bits for the ADC input mux select.
    adc_set_asc_bit(ADC_INPUT_MUX_SELECT_0_ASC_BIT,
                    (adc_config->input_mux_select >> 1) & 0x1);
    adc_set_asc_bit(ADC_INPUT_MUX_SELECT_1_ASC_BIT,
                    adc_config->input_mux_select & 0x1);

    // Set the ASC bit for bypassing the PGA.
    adc_set_asc_bit(ADC_PGA_BYPASS_ASC_BIT, adc_config->pga_bypass);
}

void adc_trigger(void) {
    g_adc_output.valid = false;
    ADC_REG__START = 0x1;
}

void adc_enable_interrupt(void) {
    ISER |= (0x1 << 3);
    printf("ADC interrupt enabled: 0x%x.\n", ISER);
}

void adc_disable_interrupt(void) {
    ISER &= ~(0x1 << 3);
    printf("ADC interrupt disabled: 0x%x.\n", ISER);
}

void adc_isr(void) {
    printf("ADC conversion complete.\n");
    g_adc_output.data = ADC_REG__DATA;
    g_adc_output.valid = true;
}
