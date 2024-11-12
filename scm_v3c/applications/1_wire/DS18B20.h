#ifndef __DS18B20_H
#define __DS18B20_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "memory_map.h"

//=========================== defines =========================================
// ***  Congfig. defines 
// DS18B20 resolution - valid values are only {9,10,11,12}
#define RESOLUTION 12
// End Congfig. defines ***

#define READ_SCRATCH 0xBE
#define WRITE_SCRATCH 0x4E
#define COPY_SCRATCH 0x48
#define RECALL_E2 0xB8
#define READ_POWER_SUPPLY 0xB4
#define CONVERT_T 0x44
#define T_CONV_9 94
#define T_CONV_10 188
#define T_CONV_11 375
#define T_CONV_12 750
//=========================== prototypes ======================================
// DS18B20
void init_DS18B20(uint64_t device_rom, uint8_t T_h, uint8_t T_l, uint8_t config);
uint16_t get_temp(uint64_t device_rom, int use_strong_pull_up);
#endif
