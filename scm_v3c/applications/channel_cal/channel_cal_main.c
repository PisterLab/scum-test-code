// This application finds the SCuM's tuning codes for all (or many) 802.15.4
// channels with the help of an OpenMote.

#include <stdio.h>
#include <stdlib.h>

#include "channel_cal.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

// Start coarse code for the sweep to find 802.15.4 channels.
#define START_COARSE_CODE 23

// End coarse code for the sweep to find 802.15.4 channels.
#define END_COARSE_CODE 24

int main(void) {
    initialize_mote();

    // Initialize the channel calibration.
    printf("Initializing channel calibration.\n");
    if (!channel_cal_init(START_COARSE_CODE, END_COARSE_CODE)) {
        return EXIT_FAILURE;
    }

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    printf("Running channel calibration.\n");
    if (!channel_cal_run()) {
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
