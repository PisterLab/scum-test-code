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

// Number of capacitors.
#define GPIO_NUM_CAPACITORS 2

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
static const uint8_t g_gpio_excitation = 0;

// GPIO 1 and GPIO 3 are the control pins for two different capacitors.
static const uint8_t g_gpio_capacitors[GPIO_NUM_CAPACITORS] = {1, 3};

// Empty callback for the RF timer.
static void rftimer_empty_callback(void) {}

// Callback for the RF timer.
static void rftimer_callback(void) {
    // Trigger an ADC read.
    adc_trigger();
    delay_milliseconds_asynchronous(/*delay_milli=*/10, 7);
}

// Set the GPIO input and output enables in the analog scan chain.
static inline void gpio_set_enable(void) {
    GPI_enables(g_gpio_input_enable);
    GPO_enables(g_gpio_output_enable);
    analog_scan_chain_write();
    analog_scan_chain_load();
}

// Set the GPIO to high.
static inline void gpio_set_high(const uint8_t gpio) {
    GPIO_REG__OUTPUT &= ~(1 << gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    gpio_set_enable();
    GPIO_REG__OUTPUT |= (1 << gpio);
}

// Set the GPIO to low.
static inline void gpio_set_low(const uint8_t gpio) {
    GPIO_REG__OUTPUT &= ~(1 << gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    gpio_set_enable();
    GPIO_REG__OUTPUT &= ~(1 << gpio);
}

// Set the GPIO to high-Z.
static inline void gpio_set_high_z(const uint8_t gpio) {
    // High-Z mode is achieved by enabling the GPIO as an input pin and
    // outputting a 1 to the GPIO.
    g_gpio_input_enable |= (1 << gpio);
    g_gpio_output_enable &= ~(1 << gpio);
    gpio_set_enable();
    GPIO_REG__OUTPUT |= (1 << gpio);
}

// Set the GPIO high for some time before setting it to high-Z.
static inline void gpio_excite(const uint8_t gpio) {
    gpio_set_high(gpio);
    delay_milliseconds_synchronous(/*delay_milli=*/1000, 6);
    gpio_set_high_z(gpio);
}

// Set the GPIOs for the capacitors. Active capacitors are pulled low while
// inactive capacitors are set to high-Z.
static inline void gpio_set_capacitors(const uint8_t capacitor_mask) {
    printf("Capacitor mask: 0x%x\n");
    for (size_t i = 0; i < GPIO_NUM_CAPACITORS; ++i) {
        if ((capacitor_mask >> i) & 0x1 == 1) {
            gpio_set_low(g_gpio_capacitors[i]);
        } else {
            gpio_set_high_z(g_gpio_capacitors[i]);
        }
    }
}

int main(void) {
    initialize_mote();

    // Configure the RF timer.
    rftimer_set_callback_by_id(rftimer_empty_callback, 6);
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

    GPO_control(6, 6, 6, 6);
    GPI_enables(g_gpio_input_enable);
    GPO_enables(g_gpio_output_enable);

    analog_scan_chain_write();
    analog_scan_chain_load();

    time_constant_init();
    gpio_set_capacitors(0x1);
    gpio_excite(g_gpio_excitation);
    delay_milliseconds_asynchronous(TIME_CONSTANT_SAMPLING_PERIOD_MS, 7);
    while (true) {
        if (g_adc_output.valid) {
            time_constant_add_sample(g_adc_output.data);
            g_adc_output.valid = false;
            if (time_constant_has_sufficient_samples()) {
                delay_milliseconds_asynchronous(
                    TIME_CONSTANT_MEASUREMENT_PERIOD_MS, 7);
                printf("Received sufficient samples.\n");
                const fixed_point_t estimated_time_constant =
                    time_constant_estimate();
                printf("Estimated time constant: %lld / %lld\n",
                       estimated_time_constant, fixed_point_init(1));
                time_constant_init();
                gpio_excite(g_gpio_excitation);
            }
        }
    }
}
