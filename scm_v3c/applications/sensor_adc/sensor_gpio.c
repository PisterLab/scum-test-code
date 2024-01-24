#include "sensor_gpio.h"

#include <stdint.h>
#include <string.h>

#include "rftimer.h"
#include "scm3c_hw_interface.h"

// GPIO excitation duration in milliseconds.
#define GPIO_EXCITATION_DURATION_MS 1000  // ms

// Sensor GPIO configuration.
static sensor_gpio_config_t g_sensor_gpio_config;

// GPIO enable configurations.
static uint16_t g_gpio_input_enable = 0x0000;
static uint16_t g_gpio_output_enable = 0xFFFF;

// Set the GPIO input and output enables in the analog scan chain.
static inline void sensor_gpio_set_enable(void) {
    GPI_enables(g_gpio_input_enable);
    GPO_enables(g_gpio_output_enable);
    analog_scan_chain_write();
    analog_scan_chain_load();
}

void sensor_gpio_init(const sensor_gpio_config_t* sensor_gpio_config) {
    memcpy(&g_sensor_gpio_config, sensor_gpio_config,
           sizeof(sensor_gpio_config_t));
    sensor_gpio_set_enable();
}

void sensor_gpio_set_high(const gpio_e gpio) {
    gpio_set_low(gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_high(gpio);
}

void sensor_gpio_set_low(const gpio_e gpio) {
    gpio_set_low(gpio);
    g_gpio_input_enable &= ~(1 << gpio);
    g_gpio_output_enable |= (1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_low(gpio);
}

void sensor_gpio_set_high_z(const gpio_e gpio) {
    // High-Z mode is achieved by enabling the GPIO as an input pin and
    // outputting a 1 to the GPIO.
    g_gpio_input_enable |= (1 << gpio);
    g_gpio_output_enable &= ~(1 << gpio);
    sensor_gpio_set_enable();
    gpio_set_high(gpio);
}

void sensor_gpio_excite(const gpio_e gpio) {
    sensor_gpio_set_high(gpio);
    delay_milliseconds_synchronous(/*delay_milli=*/GPIO_EXCITATION_DURATION_MS,
                                   g_sensor_gpio_config.rftimer_id);
    sensor_gpio_set_high_z(gpio);
}
