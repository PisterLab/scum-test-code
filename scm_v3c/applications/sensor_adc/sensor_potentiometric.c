#include "sensor_potentiometric.h"

#include <stdint.h>

#include "adc.h"

uint16_t sensor_potentiometric_measure(void) { return adc_read_output(); }
