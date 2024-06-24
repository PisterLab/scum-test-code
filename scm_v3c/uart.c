#include "uart.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory_map.h"

// Flow control constants.
// XOFF, XON, and XONXOFF_ESCAPE are transmitted as two bytes: {XONXOFF_ESCAPE,
// data ^ XONXOFF_MASK}.
enum {
    XOFF = 0x13,
    XON = 0x11,
    XONXOFF_ESCAPE = 0x12,
    XONXOFF_MASK = 0x10,
};

// UART TX callback function.
static uart_tx_callback_t g_uart_tx_callback;

// UART RX callback function.
static uart_rx_callback_t g_uart_rx_callback;

// UART XON/XOFF escaping. If true, the current data character is being escaped
// and has to be transmitted after the escape character.
static bool g_uart_xon_xoff_escaping = false;

// UART XON/XOFF escaped character.
static char g_uart_xon_xoff_escaped_char = 0;

// UART TX interrupt service routine.
void uart_tx_isr(void) {
    if (g_uart_xon_xoff_escaping) {
        UART_REG__RX_DATA = g_uart_xon_xoff_escaped_char ^ XONXOFF_MASK;
        g_uart_xon_xoff_escaping = false;
    }

    if (g_uart_tx_callback != NULL) {
        g_uart_tx_callback();
    }
}

// UART RX interrupt service routine.
void uart_rx_isr(void) {
    if (g_uart_rx_callback != NULL) {
        g_uart_rx_callback(uart_read());
    }
}

void uart_set_tx_callback(const uart_tx_callback_t callback) {
    g_uart_tx_callback = callback;
}

void uart_set_rx_callback(const uart_rx_callback_t callback) {
    g_uart_rx_callback = callback;
}

void uart_enable_interrupt(void) {
    ISER |= (0x1 << 0);
    printf("UART interrupt enabled: 0x%x.\n", ISER);
}

void uart_disable_interrupt(void) {
    ICER |= (0x1 << 0);
    printf("UART interrupt disabled: 0x%x.\n", ICER);
}

void uart_set_cts(const bool state) {
    if (state) {
        UART_REG__TX_DATA = XON;
    } else {
        UART_REG__TX_DATA = XOFF;
    }
}

void uart_write(const char data) {
    if (data == XOFF || data == XON || data == XONXOFF_MASK) {
        g_uart_xon_xoff_escaping = true;
        g_uart_xon_xoff_escaped_char = data;
        UART_REG__TX_DATA = XONXOFF_ESCAPE;
    } else {
        UART_REG__TX_DATA = data;
    }

    // There is no TX done interrupt, so call the interrupt handler directly.
    uart_tx_isr();
}

char uart_read(void) { return UART_REG__RX_DATA; }
