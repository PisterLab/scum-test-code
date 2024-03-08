#include <stdio.h>

#include "Memory_Map.h"

// RF emulator is a TEST module used instead of RFcontroller to verify DMA
// RF_DATA_REQ and RF_DATA_STORE modes.

// Set a memory location with a value and ask to the DMA to read from that
// location

void RFemulator_init() {
    *(unsigned int*)(0x02000100) = 0xCAFEDECA;
    *(unsigned int*)(AHB_RF_BASE + 0x04) = 0x200007d8;
    printf("RF emulator initialized\n");
}
