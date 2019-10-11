//------------------------------------------------------------------------------
// u-robot Digital Controller Firmware
//------------------------------------------------------------------------------

#include <stdio.h>
#include <time.h>
#include <rt_misc.h>
#include <stdlib.h>
#include "Memory_map.h"
#include "Int_Handlers.h"
#include "rf_global_vars.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "bucket_o_functions.h"
#include <math.h>
#include "scum_radio_bsp.h"

extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory addresses    
#define crc_value         (*((unsigned int *) 0x0000FFFC))
#define code_length       (*((unsigned int *) 0x0000FFF8))

// Target radio LO freq = 2.4025G
// Divide ratio is currently 480*2
// Calibration counts for 100ms
unsigned int LC_target = 250260;
unsigned int LC_code = 698;

// HF_CLOCK tuning settings
unsigned int HF_CLOCK_fine = 17;
unsigned int HF_CLOCK_coarse = 3;

// RC 2MHz tuning settings
// This the transmitter chip clock
unsigned int RC2M_coarse = 21;
unsigned int RC2M_fine = 15;
unsigned int RC2M_superfine = 15;

// Receiver clock settings
// The receiver chip clock is derived from this clock
unsigned int IF_clk_target = 1600000;
unsigned int IF_coarse = 22;
unsigned int IF_fine = 18;

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
    
    int t,t2,x;
    unsigned int calc_crc;

    unsigned int rdata_lsb, rdata_msb, count_LC, count_32k, count_2M;
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
        
    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity..."); 
    
    calc_crc = crc32c(0x0000,code_length);

    if(calc_crc == crc_value){
        printf("CRC OK\r\n");
    }
    else{
        printf("\r\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\r\n");
        while(1);
    }
    
    // Debug output
    //printf("\r\nCode length is %u bytes",code_length); 
    //printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);    
    
    //printf("done\r\n");
    
    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");
    
    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // Aux is inverted (0 = on)
    // Memory-mapped LDO control
    // ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
    // For MUX signals, '1' = FSM control, '0' = memory mapped control
    // For EN signals, '1' = turn on LDO
    ANALOG_CFG_REG__10 = 0x18;
    
    // Enable polyphase and mixers via memory-mapped I/O (for receive mode)
    ANALOG_CFG_REG__16 = 0x1;
    
    // Enable optical SFD interrupt for optical calibration
    ISER = 0x0800;
    
    // Wait for optical cal to finish
    while(optical_cal_finished == 0);
    optical_cal_finished = 0;

    printf("Cal complete\r\n");

    //skip building a channel table for now; hardcode LC values
    LC_code = 698; //Board Q3: tested at Inria A102 room (Oct, 11 2019)
    RX_channel_codes[0] = LC_code; 
    LC_code = 713; //Board Q3: tested at Inria A102 room (Oct, 11 2019)
    TX_channel_codes[0] = LC_code;

    current_RF_channel = 11;

    printf("Listening for packets on ch %d (LC_code=%d)\r\n",current_RF_channel,RX_channel_codes[current_RF_channel-11]);

    // First listen continuously for rx packet
    doing_initial_packet_search = 1;
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // Begin listening
    setFrequencyRX(current_RF_channel);
    radio_rxEnable();
    radio_rxNow();    
    
    // Wait awhile
    for (t2=0; t2<100; t2++){
        
        // Delay
        for(t=0; t<100000; t++);
        
        if(doing_initial_packet_search == 0) {
            printf("Locked to incoming packet rate...\r\n");
            break;
        }
    }
    
    // If no packet received, then stop RX so can reprogram
    if(doing_initial_packet_search == 1) {
        radio_rfOff();
        radio_disable_interrupts();
        printf("RX Stopped - Lock Failed\n");
    }

    while(1) {
        for(t=0; t<1000000; t++);
    }
}

