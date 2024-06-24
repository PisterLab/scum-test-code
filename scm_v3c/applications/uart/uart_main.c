#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "optical.h"
#include "scm3c_hw_interface.h"
#include "uart.h"

// Number of for loop cycles between TX messages.
// 700000 for loop cycles roughly correspond to 1 second.
#define NUM_CYCLES_BETWEEN_TX 1000000

// TX counter.
uint32_t g_tx_counter = 0;

// UART RX callback function.
void uart_rx_callback(const char data) { printf("%c", data); }

int main(void) {
    initialize_mote();

    // Configure the UART.
    uart_set_rx_callback(uart_rx_callback);
    uart_enable_interrupt();

    crc_check();
    perform_calibration();

    while (true) {
        printf("Hello World! %u\n", g_tx_counter);
        ++g_tx_counter;

        for (size_t i = 0; i < NUM_CYCLES_BETWEEN_TX; ++i) {}
    }
}
