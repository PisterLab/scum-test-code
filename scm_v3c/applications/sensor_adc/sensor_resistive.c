#include "sensor_resistive.h"

#include <stdint.h>
#include <string.h>

#include "adc.h"
#include "fixed_point.h"
#include "gpio.h"
#include "rftimer.h"
#include "sensor_capacitor.h"
#include "sensor_gpio.h"
#include "time_constant.h"

// Resistive sensor measurement state.
typedef enum {
    SENSOR_RESISTIVE_MEASUREMENT_STATE_INVALID = -1,
    SENSOR_RESISTIVE_MEASUREMENT_STATE_IDLE = 0,
    SENSOR_RESISTIVE_MEASUREMENT_STATE_TRIGGERED = 1,
    SENSOR_RESISTIVE_MEASUREMENT_STATE_MEASURING = 2,
    SENSOR_RESISTIVE_MEASUREMENT_STATE_DONE = 3,
} sensor_resistive_measurement_state_e;

// Resistive sensor RF timer ID.
static uint8_t g_sensor_resistive_rftimer_id = 0;

// Resistive sensor sampling period in milliseconds.
static uint16_t g_sensor_resistive_sampling_period_ms = 0;

// Resistive sensor GPIO excitation pin.
static gpio_e g_sensor_resistive_gpio_excitation = GPIO_INVALID;

// Resistive sensor measurement state.
static sensor_resistive_measurement_state_e
    g_sensor_resistive_measurement_state =
        SENSOR_RESISTIVE_MEASUREMENT_STATE_IDLE;

// Run the resistive sensor measurement.
static inline void sensor_resistive_measure_run(void) {
    while (true) {
        switch (g_sensor_resistive_measurement_state) {
            case SENSOR_RESISTIVE_MEASUREMENT_STATE_TRIGGERED: {
                time_constant_init(g_sensor_resistive_sampling_period_ms);
                sensor_gpio_excite(g_sensor_resistive_gpio_excitation);
                adc_trigger();
                delay_milliseconds_asynchronous(
                    g_sensor_resistive_sampling_period_ms,
                    g_sensor_resistive_rftimer_id);
                g_sensor_resistive_measurement_state =
                    SENSOR_RESISTIVE_MEASUREMENT_STATE_MEASURING;
                break;
            }
            case SENSOR_RESISTIVE_MEASUREMENT_STATE_MEASURING: {
                if (adc_output_valid()) {
                    const uint16_t adc_output = adc_peek_output();
                    adc_output_reset_valid();
                    time_constant_add_sample(adc_output);
                    if (time_constant_has_sufficient_samples()) {
                        g_sensor_resistive_measurement_state =
                            SENSOR_RESISTIVE_MEASUREMENT_STATE_DONE;
                        return;
                    }
                }
                break;
            }
            case SENSOR_RESISTIVE_MEASUREMENT_STATE_IDLE:
            case SENSOR_RESISTIVE_MEASUREMENT_STATE_DONE:
            case SENSOR_RESISTIVE_MEASUREMENT_STATE_INVALID:
            default: {
                break;
            }
        }
    }
}

void sensor_resistive_init(
    const sensor_resistive_config_t* sensor_resistive_config) {
    g_sensor_resistive_rftimer_id = sensor_resistive_config->rftimer_id;
    g_sensor_resistive_sampling_period_ms =
        sensor_resistive_config->sampling_period_ms;
    g_sensor_resistive_gpio_excitation =
        sensor_resistive_config->gpio_excitation;
    const sensor_gpio_config_t sensor_gpio_config = {
        .rftimer_id = g_sensor_resistive_rftimer_id,
    };
    sensor_gpio_init(&sensor_gpio_config);
    sensor_capacitor_init(&sensor_resistive_config->sensor_capacitor_config);
    // TODO(titan): Only the first capacitor mask is used.
    sensor_capacitor_set_next_mask();
}

void sensor_resistive_measure(sensor_resistive_time_constant_t* time_constant) {
    g_sensor_resistive_measurement_state =
        SENSOR_RESISTIVE_MEASUREMENT_STATE_TRIGGERED;
    sensor_resistive_measure_run();

    // Estimate the time constant.
    // printf("Received sufficient samples. Estimating the time constant
    // now.\n");
    const fixed_point_t estimated_time_constant = time_constant_estimate();
    const fixed_point_t scaling_factor = fixed_point_init(1);

    // Output the estimated time constant.
    memset(time_constant, 0, sizeof(sensor_resistive_time_constant_t));
    time_constant->time_constant = estimated_time_constant;
    time_constant->scaling_factor = scaling_factor;
}

void sensor_resistive_rftimer_callback(void) {
    switch (g_sensor_resistive_measurement_state) {
        case SENSOR_RESISTIVE_MEASUREMENT_STATE_IDLE: {
            g_sensor_resistive_measurement_state =
                SENSOR_RESISTIVE_MEASUREMENT_STATE_TRIGGERED;
            break;
        }
        case SENSOR_RESISTIVE_MEASUREMENT_STATE_MEASURING: {
            // Trigger an asynchronous ADC read.
            adc_trigger();
            delay_milliseconds_asynchronous(
                g_sensor_resistive_sampling_period_ms,
                g_sensor_resistive_rftimer_id);
            break;
        }
        case SENSOR_RESISTIVE_MEASUREMENT_STATE_TRIGGERED:
        case SENSOR_RESISTIVE_MEASUREMENT_STATE_DONE:
        case SENSOR_RESISTIVE_MEASUREMENT_STATE_INVALID:
        default: {
            break;
        }
    }
}
