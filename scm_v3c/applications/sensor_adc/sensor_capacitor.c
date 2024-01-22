#include "sensor_capacitor.h"

#include <stdlib.h>
#include <string.h>

#include "sensor_gpio.h"

// Sensor capacitor configuration.
static sensor_capacitor_config_t g_sensor_capacitor_config;

// Current capacitor mask index.
static size_t g_sensor_capacitor_mask_index = 0;

void sensor_capacitor_init(
    const sensor_capacitor_config_t* sensor_capacitor_config) {
    memcpy(&g_sensor_capacitor_config, sensor_capacitor_config,
           sizeof(sensor_capacitor_config_t));
}

void sensor_capacitor_set_next_mask(void) {
    const sensor_capacitor_mask_e capacitor_mask =
        g_sensor_capacitor_config
            .capacitor_masks[g_sensor_capacitor_mask_index];
    sensor_capacitor_set_gpios(capacitor_mask);
    g_sensor_capacitor_mask_index =
        (g_sensor_capacitor_mask_index + 1) %
        g_sensor_capacitor_config.num_capacitor_masks;
}

void sensor_capacitor_set_gpios(const sensor_capacitor_mask_e capacitor_mask) {
    for (size_t i = 0; i < g_sensor_capacitor_config.num_capacitors; ++i) {
        if ((capacitor_mask >> i) & 0x1 == 1) {
            sensor_gpio_set_low(g_sensor_capacitor_config.gpios[i]);
        } else {
            sensor_gpio_set_high_z(g_sensor_capacitor_config.gpios[i]);
        }
    }
}
