#include "ieee_802_15_4.h"

#include <stdbool.h>
#include <stdint.h>

bool ieee_802_15_4_validate_channel(const uint8_t channel) {
    return channel >= IEEE_802_15_4_MIN_CHANNEL &&
           channel <= IEEE_802_15_4_MAX_CHANNEL;
}
