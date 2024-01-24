#ifndef __SENSORS_H
#define __SENSORS_H

#include <stdlib.h>

#include "gpio.h"
#include "sensor.h"

// Number of GPIO sensor select pins.
#define NUM_SENSOR_SELECT_PINS 3

// Maximum number of sensors.
#define MAX_NUM_SENSORS (1 << NUM_SENSOR_SELECT_PINS)

// Sensors configuration struct.
typedef struct {
    // GPIO strobe pin.
    gpio_e gpio_strobe;

    // GPIO select pins, ordered from LSB to MSB.
    gpio_e gpios_select[NUM_SENSOR_SELECT_PINS];

    // Number of sensors.
    size_t num_sensors;

    // Sensor types.
    sensor_type_e sensors[MAX_NUM_SENSORS];

    // Sensor configs.
    sensor_config_t sensor_configs[MAX_NUM_SENSORS];
} sensors_config_t;

// Sensors measurements struct.
typedef struct {
    sensor_measurement_t measurements[MAX_NUM_SENSORS];
} sensors_measurements_t;

// Initiailize the sensors.
void sensors_init(const sensors_config_t* sensors_config);

// Measure all sensors.
void sensors_measure(sensors_measurements_t* sensors_measurements);

#endif  // __SENSORS_H
