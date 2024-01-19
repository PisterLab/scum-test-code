#ifndef __SENSOR_POTENTIOMETRIC_H
#define __SENSOR_POTENTIOMETRIC_H

#include <stdint.h>

// Measure a potentiometric sensor by returning the ADC output.
uint16_t sensor_potentiometric_measure(void);

#endif  // __SENSOR_POTENTIOMETRIC_H
