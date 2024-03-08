#include <stdio.h>

#include "Memory_Map.h"

// this function is necessary because this portion of memory is not directly
// managed by the processor, so a manual handling is carryed on.

void data_memory_init() {
    int i;
    unsigned int address;

    address = AHB_DATAMEM_BASE;

    for (i = 0; i < 256; i++) {
        *(unsigned int*)address = 0x20000000;
        address = address + 0x4;
    }

    printf("Data memory initialized\n\n");
}

// Print all the content of the data memory
void data_memory_stamp() {
    short int i;
    unsigned int rfcurr = 0;

    printf("Data Memory Content");

    for (i = 0; i < 512; i++) {
        unsigned int value = *(unsigned int*)rfcurr;
        printf("%x : %d \n", rfcurr, value);
        rfcurr = rfcurr + 0x4;
    }
}
