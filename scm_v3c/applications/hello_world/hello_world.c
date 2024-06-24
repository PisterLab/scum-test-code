#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "optical.h"
#include "scm3c_hw_interface.h"

// Number of for loop cycles between Hello World messages.
// 700000 for loop cycles roughly correspond to 1 second.
#define NUM_CYCLES_BETWEEN_TX 1000000

// TX counter.
static uint32_t g_tx_counter = 0;

int main(void) {
    initialize_mote();
    crc_check();
    perform_calibration();

    while (true) {
        printf("Hello World! %u\n", g_tx_counter);
        ++g_tx_counter;

        for (size_t i = 0; i < NUM_CYCLES_BETWEEN_TX; ++i);
    }
}
