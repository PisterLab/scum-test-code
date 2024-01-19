#ifndef __SENSOR_CAPACITOR_H
#define __SENSOR_CAPACITOR_H

#include <stdlib.h>

#include "gpio.h"

// Maximum number of capacitors in the capacitive DAC.
#define MAX_NUM_SENSOR_CAPACITORS 4

// Maximum number of capacitor masks.
#define MAX_NUM_SENSOR_CAPACITOR_MASKS (1 << MAX_NUM_SENSOR_CAPACITORS)

// Capacitor mask enumeration.
typedef enum {
    CAPACITOR_MASK_NONE = 0,
    CAPACITOR_MASK_1 = 1 << 0,
    CAPACITOR_MASK_2 = 1 << 1,
    CAPACITOR_MASK_3 = 1 << 2,
    CAPACITOR_MASK_4 = 1 << 3,
} sensor_capacitor_mask_e;

// Sensor capacitor configuration struct.
typedef struct {
    // Number of capacitors.
    size_t num_capacitors;

    // GPIOs corresponding to the capacitors.
    gpio_e gpios[MAX_NUM_SENSOR_CAPACITORS];

    // Number of capacitor masks.
    size_t num_capacitor_masks;

    // Capacitor masks.
    sensor_capacitor_mask_e capacitor_masks[MAX_NUM_SENSOR_CAPACITOR_MASKS];
} sensor_capacitor_config_t;

// Initialize the sensor capacitors.
void sensor_capacitor_init(
    const sensor_capacitor_config_t* sensor_capacitor_config);

// Configure the capacitors according to the next capacitor mask.
void sensor_capacitor_set_next_mask(void);

// Set the GPIOs for the capacitors. Active capacitors are pulled low while
// inactive capacitors are set to high-Z.
void sensor_capacitor_set_gpios(sensor_capacitor_mask_e capacitor_mask);

#endif  // __SENSOR_CAPACITOR_H
