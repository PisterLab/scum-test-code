#ifndef __CHANNEL_CAL_H
#define __CHANNEL_CAL_H

#include <stdbool.h>
#include <stdint.h>

#include "tuning.h"

// Initialize the channel calibration. The sweep range can be reduced if not all
// 802.15.4 channels need to be found.
bool channel_cal_init(uint8_t start_coarse_code, uint8_t end_coarse_code);

// Run the channel calibration.
bool channel_cal_run(void);

// Get the TX tuning code for the channel.
bool channel_cal_get_tx_tuning_code(uint8_t channel,
                                    tuning_code_t* tuning_code);

// Get the RX tuning code for the channel.
bool channel_cal_get_rx_tuning_code(uint8_t channel,
                                    tuning_code_t* tuning_code);

#endif  // __CHANNEL_CAL_H
