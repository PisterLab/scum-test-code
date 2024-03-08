//------------------------------------------------------------------------------
// u-robot Digital Controller Firmware
//------------------------------------------------------------------------------

#include <math.h>
#include <rt_misc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Int_Handlers.h"
#include "Memory_map.h"
#include "bucket_o_functions.h"
#include "rf_global_vars.h"
#include "scm3B_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"

extern char send_packet[127];
extern unsigned int ASC[38];
extern unsigned int ASC_FPGA[38];

// Bootloader will insert length and pre-calculated CRC at these memory
// addresses
#define crc_value (*((unsigned int*)0x0000FFFC))
#define code_length (*((unsigned int*)0x0000FFF8))

// LC freq = 2.405G
unsigned int LC_target = 2001200;  // 2002083;//2004000;//2004000; //801900;
unsigned int LC_code = 360;

unsigned int HF_CLOCK_fine = 15;
unsigned int HF_CLOCK_coarse = 5;
unsigned int RC2M_superfine = 16;

// MF IF clock settings
unsigned int IF_clk_target = 1600000;
unsigned int IF_coarse = 22;
unsigned int IF_fine = 17;

unsigned int cal_iteration = 0;
unsigned int run_test_flag = 0;
unsigned int num_packets_to_test = 1;

unsigned short optical_cal_iteration = 0, optical_cal_finished = 0;

unsigned short doing_initial_packet_search;
unsigned short current_RF_channel;
unsigned short do_debug_print = 0;
//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
    int t, t2;
    unsigned int calc_crc;

    // Check CRC
    printf("\n-------------------\n");
    printf("Validating program integrity...");

    calc_crc = crc32c(0x0000, code_length);

    if (calc_crc == crc_value) {
        printf("CRC OK\n");
    } else {
        printf(
            "\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\n");
        while (1);
    }
    // printf("\nCode length is %u bytes",code_length);
    // printf("\nCRC calculated by SCM is: 0x%X",calc_crc);

    printf("Initializing...");

    // Set up mote configuration
    initialize_mote();

    printf("done\n");
    printf("Calibrating frequencies...");

    // Turn on LC + divider for initial frequency calibration
    radio_enable_LO();

    // Enable optical SFD interrupt for optical calibration
    ISER = 0x0800;

    // Wait for optical cal to finish
    while (optical_cal_finished == 0);
    optical_cal_finished = 0;

    current_RF_channel = 13;

    printf("Listening for packets on ch %d (LC_code=%d)\n", current_RF_channel,
           RX_channel_codes[current_RF_channel - 11]);

    // First listen continuously for rx packet
    doing_initial_packet_search = 1;

    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // Begin listening
    setFrequencyRX(current_RF_channel);
    radio_rxEnable();
    radio_rxNow();

    // Wait awhile
    for (t2 = 0; t2 < 100; t2++) {
        // Delay
        for (t = 0; t < 100000; t++);

        if (doing_initial_packet_search == 0) {
            printf("Locked to incoming packet rate...\n");
            break;
        }
    }

    // If no packet received, then stop RX so can reprogram
    if (doing_initial_packet_search == 1) {
        radio_rfOff();
        radio_disable_interrupts();
        printf("RX Stopped - Lock Failed\n");
    }

    while (1) {
        int t;

        for (t = 0; t < 1000; t++);
    }
}
