#include <rt_misc.h>
#include <stdio.h>
#include <time.h>

#include "memory_map.h"

#pragma import(__use_no_semihosting)

struct __FILE {
    unsigned char* ptr;
};

FILE __stdout = {(unsigned char*)APB_UART_BASE};
FILE __stdin = {(unsigned char*)APB_UART_BASE};

int ferror(FILE* f) { return 0; }

int uart_out(int ch) {
    unsigned char* UARTPtr;
    UARTPtr = (unsigned char*)APB_UART_BASE;
    *UARTPtr = (char)ch;
    return (ch);
}

int uart_in() {
    char ch;
    unsigned char* UARTPtr;
    UARTPtr = (unsigned char*)APB_UART_BASE;
    ch = *UARTPtr;
    uart_out(ch);
    return ((int)ch);
}

int fputc(int ch, FILE* f) { return (uart_out(ch)); }

int fgetc(FILE* f) { return (uart_in()); }

void _ttywrch(int ch) { fputc(ch, &__stdout); }

void _sys_exit(void) {
    printf("\r\nTEST DONE\r\n");
    while (1);
}
