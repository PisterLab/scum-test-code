#include "tuning.h"

#include <stdbool.h>
#include <stdint.h>

#include "scm3c_hw_interface.h"

void tuning_init_for_sweep(tuning_code_t* tuning_code,
                           const tuning_sweep_config_t* sweep_config) {
    tuning_code->coarse = sweep_config->coarse.start;
    tuning_code->mid = sweep_config->mid.start;
    tuning_code->fine = sweep_config->fine.start;
}

bool tuning_validate_sweep_config(const tuning_sweep_config_t* sweep_config) {
    if (sweep_config->coarse.start > TUNING_MAX_CODE ||
        sweep_config->coarse.end > TUNING_MAX_CODE) {
        return false;
    }
    if (sweep_config->mid.start > TUNING_MAX_CODE ||
        sweep_config->mid.end > TUNING_MAX_CODE) {
        return false;
    }
    if (sweep_config->fine.start > TUNING_MAX_CODE ||
        sweep_config->fine.end > TUNING_MAX_CODE) {
        return false;
    }
    if (sweep_config->coarse.start > sweep_config->coarse.end ||
        sweep_config->mid.start > sweep_config->mid.end ||
        sweep_config->fine.start > sweep_config->fine.end) {
        return false;
    }
    return true;
}

void tuning_increment_code(tuning_code_t* tuning_code) {
    tuning_sweep_config_t sweep_config = {
        .coarse =
            {
                .start = TUNING_MIN_CODE,
                .end = TUNING_MAX_CODE,
            },
        .mid =
            {
                .start = TUNING_MIN_CODE,
                .end = TUNING_MAX_CODE,
            },
        .fine =
            {
                .start = TUNING_MIN_CODE,
                .end = TUNING_MAX_CODE,
            },
    };
    tuning_increment_code_for_sweep(tuning_code, &sweep_config);
}

void tuning_increment_code_for_sweep(
    tuning_code_t* tuning_code, const tuning_sweep_config_t* sweep_config) {
    ++tuning_code->fine;
    if (tuning_code->fine > sweep_config->fine.end) {
        tuning_increment_mid_code_for_sweep(tuning_code, sweep_config);
    }
}

void tuning_increment_mid_code_for_sweep(
    tuning_code_t* tuning_code, const tuning_sweep_config_t* sweep_config) {
    tuning_code->fine = sweep_config->fine.start;
    ++tuning_code->mid;
    if (tuning_code->mid > sweep_config->mid.end) {
        tuning_code->mid = sweep_config->mid.start;
        ++tuning_code->coarse;
    }
}

bool tuning_end_of_sweep(const tuning_code_t* tuning_code,
                         const tuning_sweep_config_t* sweep_config) {
    return tuning_code->coarse > sweep_config->coarse.end ||
           (tuning_code->coarse == sweep_config->coarse.end &&
            (tuning_code->mid > sweep_config->mid.end ||
             (tuning_code->mid == sweep_config->mid.end &&
              tuning_code->fine >= sweep_config->fine.end)));
}

void tuning_tune_radio(const tuning_code_t* tuning_code) {
    LC_FREQCHANGE(tuning_code->coarse, tuning_code->mid, tuning_code->fine);
}
