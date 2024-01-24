#include "sensors.h"

#include <stdlib.h>
#include <string.h>

#include "gpio.h"
#include "sensor_potentiometric.h"
#include "sensor_resistive.h"

// Sensors configuration.
static sensors_config_t g_sensors_config;

// Select a sensor by setting the GPIO select pins.
static inline void sensors_select_sensor(const size_t index) {
    for (size_t i = 0; i < NUM_SENSOR_SELECT_PINS; ++i) {
        if ((index >> i) & 0x1 == 1) {
            gpio_set_high(g_sensors_config.gpios_select[i]);
        } else {
            gpio_set_low(g_sensors_config.gpios_select[i]);
        }
    }
}

void sensors_init(const sensors_config_t* sensors_config) {
    memcpy(&g_sensors_config, sensors_config, sizeof(sensors_config_t));
}

void sensors_measure(sensors_measurements_t* sensors_measurements) {
    for (size_t i = 0; i < g_sensors_config.num_sensors; ++i) {
        // Lower the strobe.
        gpio_set_low(g_sensors_config.gpio_strobe);

        // Select the sensor.
        sensors_select_sensor(i);

        // Measure the sensor.
        switch (g_sensors_config.sensors[i]) {
            case SENSOR_TYPE_POTENTIOMETRIC: {
                sensors_measurements->measurements[i].adc_output =
                    sensor_potentiometric_measure();
                break;
            }
            case SENSOR_TYPE_RESISTIVE: {
                sensor_resistive_init(
                    &g_sensors_config.sensor_configs[i].resistive_config);
                sensor_resistive_measure(
                    &sensors_measurements->measurements[i].time_constant);
                break;
            }
            case SENSOR_TYPE_PH:
            case SENSOR_TYPE_INVALID:
            default: {
                break;
            }
        }

        // Raise the strobe.
        gpio_set_high(g_sensors_config.gpio_strobe);
    }
}
