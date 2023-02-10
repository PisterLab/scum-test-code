#ifndef __IEEE_802_15_4_H
#define __IEEE_802_15_4_H

#include <stdbool.h>
#include <stdint.h>

// Number of 802.15.4 channels.
#define IEEE_802_15_4_NUM_CHANNELS 16

// Minimum 802.15.4 channel.
#define IEEE_802_15_4_MIN_CHANNEL 11

// Maximum 802.15.4 channel.
#define IEEE_802_15_4_MAX_CHANNEL 26

// Validate the channel.
bool ieee_802_15_4_validate_channel(uint8_t channel);

#endif  // __IEEE_802_15_4_H
