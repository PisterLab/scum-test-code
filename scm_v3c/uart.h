#ifndef __UART_H
#define __UART_H

#include <stdbool.h>

// UART callback function definitions.
typedef void (*uart_tx_callback_t)(void);
typedef void (*uart_rx_callback_t)(char data);

// Set the UART TX callback.
void uart_set_tx_callback(uart_tx_callback_t callback);

// Set the UART RX callback.
void uart_set_rx_callback(uart_rx_callback_t callback);

// Enable the UART interrupt.
void uart_enable_interrupt(void);

// Disable the UART interrupt.
void uart_disable_interrupt(void);

// Set the clear-to-send line.
void uart_set_cts(bool state);

// Write a character over UART.
void uart_write(char data);

// Read a character over UART.
char uart_read(void);

#endif  // __UART_H
