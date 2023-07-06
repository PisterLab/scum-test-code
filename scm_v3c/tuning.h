#ifndef __TUNING_H
#define __TUNING_H

#include <stdbool.h>
#include <stdint.h>

// Minimum tuning code.
#define TUNING_MIN_CODE 0

// Maximum tuning code.
#define TUNING_MAX_CODE 31

// Tuning code.
typedef struct __attribute__((packed)) {
    // Coarse code.
    uint8_t coarse;

    // Mid code.
    uint8_t mid;

    // Fine code.
    uint8_t fine;
} tuning_code_t;

// Sweep range.
typedef struct __attribute__((packed)) {
    // Start code of the sweep.
    uint8_t start;

    // End code of the sweep (inclusive).
    uint8_t end;
} tuning_sweep_range_t;

// Sweep configuration.
typedef struct __attribute__((packed)) {
    // Sweep range for the coarse code.
    tuning_sweep_range_t coarse;

    // Sweep range for the mid code.
    tuning_sweep_range_t mid;

    // Sweep range for the fine code.
    tuning_sweep_range_t fine;
} tuning_sweep_config_t;

// Initialize the tuning code to the minimum value given by the sweep
// configuration.
void tuning_init_for_sweep(tuning_code_t* tuning_code,
                           const tuning_sweep_config_t* sweep_config);

// Validate the sweep configuration.
bool tuning_validate_sweep_config(const tuning_sweep_config_t* sweep_config);

// Increment the tuning code by one fine code.
void tuning_increment_code(tuning_code_t* tuning_code);

// Increment the tuning code by one fine code, rolling over at the range
// boundaries given by the sweep configuration.
void tuning_increment_code_for_sweep(tuning_code_t* tuning_code,
                                     const tuning_sweep_config_t* sweep_config);

// Increment the tuning code by one mid code, rolling over at the range
// boundaries given by the sweep configuration.
void tuning_increment_mid_code_for_sweep(
    tuning_code_t* tuning_code, const tuning_sweep_config_t* sweep_config);

// Check whether the tuning code is at the end of the sweep.
bool tuning_end_of_sweep(const tuning_code_t* tuning_code,
                         const tuning_sweep_config_t* sweep_config);

// Tune the radio to the desired tuning code.
void tuning_tune_radio(const tuning_code_t* tuning_code);

#endif  // __TUNING_H
