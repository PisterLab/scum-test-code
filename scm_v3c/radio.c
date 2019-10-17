#include "Memory_Map.h"
#include <stdio.h>
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"
#include "bucket_o_functions.h"

extern char send_packet[127];
extern char recv_packet[130];

unsigned int chips[100];
unsigned int chip_index = 0;
int raw_chips;
int jj;
unsigned int acfg3_val;

extern unsigned int LC_target;
extern unsigned int LC_code;
extern unsigned int IF_clk_target;
extern unsigned int IF_coarse;
extern unsigned int IF_fine;
extern unsigned int cal_iteration;
extern unsigned int ASC[38];

unsigned int num_packets_received;
unsigned int num_crc_errors;
unsigned int wrong_lengths;
unsigned int LQI_chip_errors;
unsigned int num_valid_packets_received;
unsigned int IF_estimate;

unsigned int current_thresh = 0;
extern unsigned int run_test_flag;
extern unsigned int num_packets_to_test;
signed short cdr_tau_value;

extern unsigned int HF_CLOCK_fine;
extern unsigned int HF_CLOCK_coarse;
extern unsigned int RC2M_superfine;
extern unsigned int RC2M_fine;
extern unsigned int RC2M_coarse;

extern unsigned int RX_channel_codes[16];
extern unsigned int TX_channel_codes[16];
extern unsigned short optical_cal_iteration,optical_cal_finished;

signed int SFD_timestamp = 0;
signed int SFD_timestamp_n_1 = 0;
signed int timing_correction = 0;
extern unsigned short doing_initial_packet_search;

// Timer parameters 
extern unsigned int packet_interval;
extern unsigned int radio_startup_time;
extern unsigned int expected_RX_arrival;
extern unsigned int ack_turnaround_time;
extern unsigned int guard_time;
extern unsigned short current_RF_channel;

extern unsigned short do_debug_print;

void radio_isr() {
    
    unsigned int interrupt = RFCONTROLLER_REG__INT;
    unsigned int error     = RFCONTROLLER_REG__ERROR;
    int t;
    
  //if (interrupt & 0x00000001) printf("TX LOAD DONE\r\n");
    //if (interrupt & 0x00000002) printf("TX SFD DONE\r\n");
    if (interrupt & 0x00000004){ //printf("TX SEND DONE\r\n");
        
        // Lower GPIO3 to indicate TX is off
        GPIO_REG__OUTPUT &= ~(0x8);
        
        // Packet sent; turn transmitter off
        radio_rfOff();
                
        // Apply frequency corrections
        radio_frequency_housekeeping();
        
        // Debug outputs
        printf("TX DONE\r\n");
        printf("IF=%d, LQI=%d, CDR=%d, len=%d, SFD=%d, LC=%d\r\n",IF_estimate,LQI_chip_errors,cdr_tau_value,recv_packet[0],packet_interval,LC_code);
        
    }
    if (interrupt & 0x00000008){// printf("RX SFD DONE\r\n");
        SFD_timestamp_n_1 = SFD_timestamp;
        SFD_timestamp = RFTIMER_REG__COUNTER;
        
        // Toggle GPIO5 to indicate a SFD was found
        // Write twice to make the pulse wider for easier capture on scope
        GPIO_REG__OUTPUT |= 0x20;
        GPIO_REG__OUTPUT |= 0x20;
        GPIO_REG__OUTPUT &= ~(0x20);
        
        // Disable watchdog if a SFD has been found
        RFTIMER_REG__COMPARE3_CONTROL = 0x0;
        
        if(doing_initial_packet_search==1){
            // Sync next RX turn-on to expected arrival time
            RFTIMER_REG__COUNTER = expected_RX_arrival;
        }
        
    }
    if (interrupt & 0x00000010) {
        
        //int i;
        unsigned int RX_DONE_timestamp;
        char num_bytes_rec = recv_packet[0];
        
        // Debug printing of packet contents
        //char *current_byte_rec = recv_packet+1;
        //printf("RX DONE\r\n");
        //printf("RX'd %d: \r\n", num_bytes_rec);
        //for (i=0; i < num_bytes_rec; i++) {
        //    printf("%c",current_byte_rec[i]);
        //}
        //printf("\r\n");
        
        // Set GPIO1 low to indicate RX is now done/off
        GPIO_REG__OUTPUT &= ~(0x2);
        
        // Note when the packet reception was complete
        RX_DONE_timestamp = RFTIMER_REG__COUNTER;
        
        // Keep track of how many packets have been received
        num_packets_received++;    
        
        // Check if the packet length is as expected (20 payload bytes + 2 for CRC)    
        // In this demo code, it is assumed the OpenMote is sending packets with 20B payloads
        if(num_bytes_rec != 22){
            wrong_lengths++;
            
            // If packet was a failure, turn the radio off
            if(doing_initial_packet_search == 0){
                
                radio_rfOff();
            }
            else{
                
                // Keep listening if doing initial search
                radio_rxEnable();
                radio_rxNow();
            }
            
        }
        
        // Length was right but CRC was wrong
        else if (error == 0x00000008){
            
            // Keep track of number of CRC errors
            num_crc_errors++;
            
            // Clear error flag
            RFCONTROLLER_REG__ERROR_CLEAR = error;

            // If packet was a failure, turn the radio off
            if(doing_initial_packet_search == 0){
                
                radio_rfOff();
            }
            else{
                
                // Keep listening if doing initial search
                radio_rxEnable();
                radio_rxNow();
            }
        
        // Packet has good CRC value and is the correct length
        } else {
            
            // If this is the first valid packet received, start timers for next expected packet
            if(doing_initial_packet_search == 1){
                
                // Clear this flag to note we are no longer doing continuous RX listen
                doing_initial_packet_search = 0;
            
                // Enable compare interrupts for RX
                RFTIMER_REG__COMPARE0_CONTROL = 0x3;
                RFTIMER_REG__COMPARE1_CONTROL = 0x3;
                RFTIMER_REG__COMPARE2_CONTROL = 0x3;
                
                // Enable timer ISRs in the NVIC
                rftimer_enable_interrupts();
                
                // Shut radio off until next packet
                radio_rfOff();
            }    
            // Already locked onto packet rate
            else{
                
                // Only record IF estimate, LQI, and CDR tau for valid packets
                IF_estimate = read_IF_estimate();
                LQI_chip_errors    = ANALOG_CFG_REG__21 & 0xFF; //read_LQI();    
                
                // Read the value of tau debug at end of packet
                // Do this later in the ISR to make sure this register has settled before trying to read it
                // (the register is on the adc clock domain)
                cdr_tau_value = ANALOG_CFG_REG__25;
                
                // Keep track of how many received packets were valid
                num_valid_packets_received++;
                
                // Prepare ack - for this demo code the contents are arbitrary
                // The OpenMote receiver is looking for 30B packets - still on channel 11
                // Data is stored in send_packet[]
                send_packet[0] = 't';
                send_packet[1] = 'e';
                send_packet[2] = 's';
                send_packet[3] = 't';
                radio_loadPacket(30);
        
                // Transmit packet at this time
                RFTIMER_REG__COMPARE5 = RX_DONE_timestamp + ack_turnaround_time;    
                
                // Turn on transmitter at this time
                RFTIMER_REG__COMPARE4 = RFTIMER_REG__COMPARE5 - radio_startup_time;
                    
                // Enable TX timers
                RFTIMER_REG__COMPARE4_CONTROL = 0x3;
                RFTIMER_REG__COMPARE5_CONTROL = 0x3;
                
                // Turn radio off until next packet
                radio_rfOff();
            }
        
        }
        
            // Debug print output        
            //printf("IF=%d, LQI=%d, CDR=%d, len=%d, SFD=%d, LC=%d, numcrc=%d\r\n",IF_estimate,LQI_chip_errors,cdr_tau_value,recv_packet[0],packet_interval,LC_code,num_crc_errors);
            
            // Uncomment below to enter continuous RX loop
            //setFrequencyRX(current_RF_channel);
            //radio_rxEnable();
            //radio_rxNow();
            //rftimer_disable_interrupts();
    }
    
    //if (error != 0) {
    //    if (error & 0x00000001) printf("TX OVERFLOW ERROR\r\n");
    //    if (error & 0x00000002) printf("TX CUTOFF ERROR\r\n");
    //    if (error & 0x00000004) printf("RX OVERFLOW ERROR\r\n");
    //    if (error & 0x00000008) printf("RX CRC ERROR\r\n");
    //    if (error & 0x00000010) printf("RX CUTOFF ERROR\r\n");
        RFCONTROLLER_REG__ERROR_CLEAR = error;
    //}
    
    RFCONTROLLER_REG__INT_CLEAR = interrupt;
}


// This ISR goes off when the raw chip shift register interrupt goes high
// It reads the current 32 bits and then prints them out after N cycles
void rawchips_32_isr() {
    
    unsigned int jj;
    unsigned int rdata_lsb, rdata_msb;
    
    // Read 32bit val
    rdata_lsb = ANALOG_CFG_REG__17;
    rdata_msb = ANALOG_CFG_REG__18;
    chips[chip_index] = rdata_lsb + (rdata_msb << 16);    
        
    chip_index++;
    
    //printf("x1\r\n");
    
    // Clear the interrupt
    //ANALOG_CFG_REG__0 = 1;
    //ANALOG_CFG_REG__0 = 0;
    acfg3_val |= 0x20;
    ANALOG_CFG_REG__3 = acfg3_val;
    acfg3_val &= ~(0x20);
    ANALOG_CFG_REG__3 = acfg3_val;

    if(chip_index == 10){    
        for(jj=1;jj<10;jj++){
            printf("%X\r\n",chips[jj]);
        }

        ICER = 0x0100;
        ISER = 0x0200;
        chip_index = 0;
        
        // Wait for print to complete
        for(jj=0;jj<10000;jj++);
        
        // Execute soft reset
        *(unsigned int*)(0xE000ED0C) = 0x05FA0004;
    }
}

// With HCLK = 5MHz, data rate of 1.25MHz tested OK
// For faster data rate, will need to raise the HCLK frequency
// This ISR goes off when the input register matches the target value
void rawchips_startval_isr() {
    
    unsigned int rdata_lsb, rdata_msb;
    
    // Clear all interrupts
    acfg3_val |= 0x60;
    ANALOG_CFG_REG__3 = acfg3_val;
    acfg3_val &= ~(0x60);
    ANALOG_CFG_REG__3 = acfg3_val;
        
    // Enable the interrupt for the 32bit 
    ISER = 0x0200;
    ICER = 0x0100;
    ICPR = 0x0200;
    
    // Read 32bit val
    rdata_lsb = ANALOG_CFG_REG__17;
    rdata_msb = ANALOG_CFG_REG__18;
    chips[chip_index] = rdata_lsb + (rdata_msb << 16);
    chip_index++;

    //printf("x2\r\n");

}
