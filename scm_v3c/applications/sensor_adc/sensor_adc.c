#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc.h"
#include "fixed_point.h"
#include "gpio.h"
#include "memory_map.h"
#include "optical.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"
#include "time_constant.h"

// RF timer ID.
#define RFTIMER_ID 7

// Number of capacitors.
#define GPIO_NUM_CAPACITORS 2

// Number of capacitor configurations.
#define CAPACITOR_NUM_CONFIGURATIONS 3

// GPIO excitation duration in milliseconds.
#define GPIO_EXCITATION_DURATION_MS 1000  // ms

// ADC state.
typedef enum {
    ADC_STATE_INVALID = -1,
    ADC_STATE_IDLE = 0,
    ADC_STATE_TRIGGERED = 1,
    ADC_STATE_MEASURING = 2,
    ADC_STATE_PROCESSING = 3,
} adc_state_e;

// Capacitor mask enumeration.
typedef enum {
    CAPACITOR_MASK_NONE = 0,
    CAPACITOR_MASK_1 = 1 << 0,
    CAPACITOR_MASK_2 = 1 << 1,
} capacitor_mask_e;

// ADC state.
static adc_state_e g_adc_state = ADC_STATE_IDLE;

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

// GPIO configuration.
static uint16_t g_gpio_input_enable = 0x0000;
static uint16_t g_gpio_output_enable = 0x0000;

// GPIO 0 is the excitation pin.
static const gpio_e g_gpio_excitation = GPIO_0;

// GPIO 1 and GPIO 3 are the control pins for two different capacitors.
static const gpio_e g_gpio_capacitors[GPIO_NUM_CAPACITORS] = {GPIO_1, GPIO_3};

// Current capacitor configuration.
static size_t g_capacitor_configuration_index = 0;

// Capacitor configurations.
static const capacitor_mask_e
    g_capacitor_configurations[CAPACITOR_NUM_CONFIGURATIONS] = {
        CAPACITOR_MASK_1, CAPACITOR_MASK_2,
        CAPACITOR_MASK_1 | CAPACITOR_MASK_2};

// Callback for the RF timer.
static void rftimer_callback(void) {
    switch (g_adc_state) {
        case ADC_STATE_IDLE: {
            g_adc_state = ADC_STATE_TRIGGERED;
            break;
        }
        case ADC_STATE_MEASURING: {
            // Trigger an asynchronous ADC read.
            adc_trigger();
            delay_milliseconds_asynchronous(
                /*delay_milli=*/TIME_CONSTANT_SAMPLING_PERIOD_MS, RFTIMER_ID);
            break;
        }
        case ADC_STATE_TRIGGERED:
        case ADC_STATE_PROCESSING:
        case ADC_STATE_INVALID:
        default: {
            break;
        }
    }
}

// Set the GPIO input and output enables in the analog scan chain.
static inline void sensor_gpio_set_enable(void) {
    GPI_enables(g_gpio_input_enable);
    GPO_enables(g_gpio_output_enable);
    analog_scan_chain_write();
    analog_scan_chain_load();
}

// Set the GPIO to high.
static inline void sensor_gpio_set_high(const gpio_e gpio) {
    gpio_set_low(gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_high(gpio);
}

// Set the GPIO to low.
static inline void sensor_gpio_set_low(const gpio_e gpio) {
    gpio_set_low(gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_low(gpio);
}

// Set the GPIO to high-Z.
static inline void sensor_gpio_set_high_z(const gpio_e gpio) {
    // High-Z mode is achieved by enabling the GPIO as an input pin and
    // outputting a 1 to the GPIO.
    g_gpio_input_enable |= (1 << gpio);
    g_gpio_output_enable &= ~(1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_high(gpio);
}

// Set the GPIO high for some time before setting it to high-Z.
static inline void gpio_excite(const gpio_e gpio) {
    sensor_gpio_set_high(gpio);
    delay_milliseconds_synchronous(/*delay_milli=*/GPIO_EXCITATION_DURATION_MS,
                                   7);
    sensor_gpio_set_high_z(gpio);
}

// Set the GPIOs for the capacitors. Active capacitors are pulled low while
// inactive capacitors are set to high-Z.
static inline void capacitor_set_gpios(const capacitor_mask_e capacitor_mask) {
    for (size_t i = 0; i < GPIO_NUM_CAPACITORS; ++i) {
        if ((capacitor_mask >> i) & 0x1 == 1) {
            sensor_gpio_set_low(g_gpio_capacitors[i]);
        } else {
            sensor_gpio_set_high_z(g_gpio_capacitors[i]);
        }
    }
}

// Set the next capacitor configuration.
static inline void capacitor_set_next_configuration(void) {
    const capacitor_mask_e capacitor_configuration =
        g_capacitor_configurations[g_capacitor_configuration_index];
    printf("Capacitor configuration: 0x%x\n", capacitor_configuration);
    capacitor_set_gpios(capacitor_configuration);
    g_capacitor_configuration_index =
        (g_capacitor_configuration_index + 1) % CAPACITOR_NUM_CONFIGURATIONS;
}

int main(void) {
    initialize_mote();

    // Configure the RF timer.
    rftimer_set_callback_by_id(rftimer_callback, RFTIMER_ID);
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

    GPO_control(6, 6, 6, 6);
    GPI_enables(g_gpio_input_enable);
    GPO_enables(g_gpio_output_enable);

    analog_scan_chain_write();
    analog_scan_chain_load();

    g_adc_state = ADC_STATE_TRIGGERED;
    while (true) {
        switch (g_adc_state) {
            case ADC_STATE_TRIGGERED: {
                time_constant_init();
                capacitor_set_next_configuration();
                gpio_excite(g_gpio_excitation);
                delay_milliseconds_asynchronous(
                    TIME_CONSTANT_SAMPLING_PERIOD_MS, RFTIMER_ID);
                g_adc_state = ADC_STATE_MEASURING;
                break;
            }
            case ADC_STATE_MEASURING: {
                if (adc_output_valid()) {
                    uint16_t adc_output = adc_peek_output();
                    adc_output_reset_valid();
                    time_constant_add_sample(adc_output);
                    if (time_constant_has_sufficient_samples()) {
                        g_adc_state = ADC_STATE_PROCESSING;
                        printf("Received sufficient samples.\n");
                        const fixed_point_t estimated_time_constant =
                            time_constant_estimate();
                        printf("Estimated time constant: %lld / %lld\n",
                               estimated_time_constant, fixed_point_init(1));

                        delay_milliseconds_asynchronous(
                            TIME_CONSTANT_MEASUREMENT_PERIOD_MS, RFTIMER_ID);
                        g_adc_state = ADC_STATE_IDLE;
                    }
                }
                break;
            }
            case ADC_STATE_IDLE:
            case ADC_STATE_PROCESSING:
            case ADC_STATE_INVALID:
            default: {
                break;
            }
        }
    }
}
