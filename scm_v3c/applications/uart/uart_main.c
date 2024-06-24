#include <stdbool.h>
#include <stdio.h>

#include "optical.h"
#include "scm3c_hw_interface.h"
#include "uart.h"

// UART RX callback function.
void uart_rx_callback(const char data) { printf("%c", data); }

int main(void) {
    initialize_mote();

    // Configure the UART.
    uart_set_rx_callback(uart_rx_callback);
    uart_enable_interrupt();

    crc_check();
    perform_calibration();

    printf("Waiting for UART data...\n");
    while (true) {
        __wfi();
    }
}
