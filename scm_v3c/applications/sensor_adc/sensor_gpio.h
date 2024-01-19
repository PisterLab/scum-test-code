#ifndef __SENSOR_GPIO_H
#define __SENSOR_GPIO_H

#include <stdint.h>

#include "gpio.h"

// Sensor GPIO configuration struct.
typedef struct {
    // RF timer ID.
    uint8_t rftimer_id;
} sensor_gpio_config_t;

// Initialize the sensor GPIOs.
void sensor_gpio_init(const sensor_gpio_config_t* sensor_gpio_config);

// Set the GPIO to high.
void sensor_gpio_set_high(gpio_e gpio);

// Set the GPIO to low.
void sensor_gpio_set_low(gpio_e gpio);

// Set the GPIO to high-Z.
void sensor_gpio_set_high_z(gpio_e gpio);

// Set the GPIO high for some time before setting it to high-Z.
void sensor_gpio_excite(gpio_e gpio);

#endif  // __SENSOR_GPIO_H
