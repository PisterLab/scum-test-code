#ifndef __SENSOR_RESISTIVE_H
#define __SENSOR_RESISTIVE_H

#include <stdint.h>

#include "fixed_point.h"
#include "gpio.h"
#include "sensor_capacitor.h"

// Resistive sensor configuration struct.
typedef struct {
    // RF timer ID.
    uint8_t rftimer_id;

    // Sampling period in milliseconds.
    uint16_t sampling_period_ms;

    // GPIO excitation pin.
    gpio_e gpio_excitation;

    // Sensor capacitor configuration.
    sensor_capacitor_config_t sensor_capacitor_config;
} sensor_resistive_config_t;

// Resistive sensor time constant struct.
typedef struct {
    // Time constant scaled by the scaling factor.
    fixed_point_t time_constant;

    // Scaling factor of the time constant.
    fixed_point_t scaling_factor;
} sensor_resistive_time_constant_t;

// Initialize measuring a resistive sensor.
void sensor_resistive_init(
    const sensor_resistive_config_t* sensor_resistive_config);

// Measure a resistive sensor by returning the time constant. The time constant
// is outputted as an output argument struct with a scaling factor.
void sensor_resistive_measure(sensor_resistive_time_constant_t* time_constant);

// RF timer callback for measuring a resistive sensor.
void sensor_resistive_rftimer_callback(void);

#endif  // __SENSOR_RESISTIVE_H
