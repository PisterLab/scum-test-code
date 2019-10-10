#include <stdio.h>
#include "Memory_Map.h"
#include "scm3_hardware_interface.h"
#include "scm3C_hardware_interface.h"
#include "bucket_o_functions.h"

extern unsigned int ASC[38];
extern unsigned int cal_iteration;
extern char recv_packet[130];

extern unsigned int RX_channel_codes[16]; 
extern unsigned int TX_channel_codes[16];
extern unsigned short current_RF_channel;

extern char send_packet[127];

// These coefficients are used for filtering frequency feedback information
// These are no necessarily the ideal values to use; situationally dependent
unsigned char FIR_coeff[11] = {4,16,37,64,87,96,87,64,37,16,4};
unsigned int IF_estimate_history[11] = {500,500,500,500,500,500,500,500,500,500};
signed short cdr_tau_history[11] = {0};

extern unsigned int LQI_chip_errors;
extern unsigned int IF_estimate;
extern signed short cdr_tau_value;

extern unsigned int IF_fine;
extern unsigned int IF_coarse;

// How many packets must be received before adjusting RX clock rates
// Should be at least as long as the FIR filters
unsigned short frequency_update_rate = 15; 
unsigned short frequency_update_cooldown_timer;

extern unsigned int packet_interval;
extern unsigned int expected_RX_arrival;
extern signed int SFD_timestamp;

//If the RX watchdog expires, there will be no ack transmitted so no need to setup for TX


// SCM has separate setFrequency functions for RX and TX because of the way the radio is built
// The LO needs to be set to a different frequency for TX vs RX
void setFrequencyRX(unsigned int channel){
    
    // Set LO code for RX channel
    LC_monotonic(RX_channel_codes[channel-11]);
    
}

void setFrequencyTX(unsigned int channel){

    // Set LO code for TX channel
    LC_monotonic(TX_channel_codes[channel-11]);

}

void radio_loadPacket(unsigned int len){

    int i;
    
    RFCONTROLLER_REG__TX_DATA_ADDR = &send_packet[0];
            
    // Set length field (should include +2 for CRC in length)
    RFCONTROLLER_REG__TX_PACK_LEN = len + 2;

    // Load packet into radio
    RFCONTROLLER_REG__CONTROL = 0x1;
}

// Turn on the radio for transmit
// This should be done at least ~50 us before txNow()
void radio_txEnable(){
    
    // Turn off polyphase and disable mixer
    ANALOG_CFG_REG__16 = 0x6;
    
    // Turn on LO, PA, and AUX LDOs
    ANALOG_CFG_REG__10 = 0x0028;
}

// Begin modulating the radio output for TX
// Note that you need some delay before txNow() to allow txLoad() to finish loading the packet
void radio_txNow(){
    
    RFCONTROLLER_REG__CONTROL = 0x2;
}

// Turn on the radio for receive
// This should be done at least ~50 us before rxNow()
void radio_rxEnable(){
    
    // Turn on LO, IF, and AUX LDOs via memory mapped register
    ANALOG_CFG_REG__10 = 0x0018;
    
    // Enable polyphase and mixers via memory-mapped I/O
    ANALOG_CFG_REG__16 = 0x1;
    
    // Where packet will be stored in memory
    DMA_REG__RF_RX_ADDR = &recv_packet[0];
    
    // Reset radio FSM
    RFCONTROLLER_REG__CONTROL = 0x10;
    
}

// Radio will begin searching for start of packet
void radio_rxNow(){
    
    // Reset digital baseband
    ANALOG_CFG_REG__4 = 0x2000;
    ANALOG_CFG_REG__4 = 0x2800;

    // Start RX FSM
    RFCONTROLLER_REG__CONTROL = 0x4;
}

void radio_rfOff(){
    
    // Hold digital baseband in reset
    ANALOG_CFG_REG__4 = 0x2000;

    // Turn off LDOs
    ANALOG_CFG_REG__10 = 0x0000;
}


void radio_frequency_housekeeping(){
    
    signed int sum = 0;
    int jj;
    unsigned int IF_est_filtered;
    signed int chip_rate_error_ppm, chip_rate_error_ppm_filtered;
    unsigned short packet_len;
    signed int timing_correction;
    
    packet_len = recv_packet[0];
                
    
    // Update the expected number of RF timer counts that corresponds to the packet rate
    timing_correction = (expected_RX_arrival - SFD_timestamp);
    packet_interval -= timing_correction;
    
    // The value at which the RF timer rolls over
    RFTIMER_REG__MAX_COUNT = packet_interval - timing_correction;
    
    //printf("%d - %d\r\n", SFD_timestamp, RFTIMER_REG__MAX_COUNT);
    
    
    // When updating LO and IF clock frequncies, must wait long enough for the changes to propagate before changing again
    // Need to receive as many packets as there are taps in the FIR filter
    frequency_update_cooldown_timer++;
    
    // FIR filter for cdr tau slope
    sum = 0;
    
    // A tau value of 0 indicates there is no rate mistmatch between the TX and RX chip clocks
    // The cdr_tau_value corresponds to the number of samples that were added or dropped by the CDR
    // Each sample point is 1/16MHz = 62.5ns
    // Need to estimate ppm error for each packet, then FIR those values to make tuning decisions
    // error_in_ppm = 1e6 * (#adjustments * 62.5ns) / (packet length (bytes) * 64 chips/byte * 500ns/chip)
    // Which can be simplified to (#adjustments * 15625) / (packet length * 8)
                
    chip_rate_error_ppm = (cdr_tau_value * 15625) / (packet_len * 8);
    
    // Shift old samples
    for (jj=9; jj>=0; jj--){
        cdr_tau_history[jj+1] = cdr_tau_history[jj];        
    }
    
    // New sample
    cdr_tau_history[0] = chip_rate_error_ppm;
    
    // Do FIR convolution
    for (jj=0; jj<=10; jj++){
        sum = sum + cdr_tau_history[jj] * FIR_coeff[jj];        
    }
    
    // Divide by 512 (sum of the coefficients) to scale output
    chip_rate_error_ppm_filtered = sum / 512;
    
    //printf("%d -- %d\r\n",cdr_tau_value,chip_rate_error_ppm_filtered);
    
    // The IF clock frequency steps are about 2000ppm, so make an adjustment only if the error is larger than 1000ppm
    // Must wait long enough between changes for FIR to settle (at least 10 packets)
    // Need to add some handling here in case the IF_fine code will rollover with this change (0 <= IF_fine <= 31)
    if(frequency_update_cooldown_timer == frequency_update_rate){
        if(chip_rate_error_ppm_filtered > 1000)
            set_IF_clock_frequency(IF_coarse, IF_fine++, 0);
        if(chip_rate_error_ppm_filtered < -1000)
            set_IF_clock_frequency(IF_coarse, IF_fine--, 0);
    }
    
    
    // FIR filter for IF estimate
    sum = 0;
                
    // The IF estimate reports how many zero crossings (both pos and neg) there were in a 100us period
    // The IF should on average be 2.5 MHz, which means the IF estimate will return ~500 when there is no IF error
    // Each tick is roughly 5 kHz of error
    
    // Only make adjustments when the chip error rate is <10% (this value was picked as an arbitrary choice)
    // While packets can be received at higher chip error rates, the average IF estimate tends to be less accurate
    // Estimated chip_error_rate = LQI_chip_errors/256 (assuming the packet length was at least 8 Bytes)
    if(LQI_chip_errors < 25){
    
        // Shift old samples
        for (jj=9; jj>=0; jj--){
            IF_estimate_history[jj+1] = IF_estimate_history[jj];        
        }
        
        // New sample
        IF_estimate_history[0] = IF_estimate;

        // Do FIR convolution
        for (jj=0; jj<=10; jj++){
            sum = sum + IF_estimate_history[jj] * FIR_coeff[jj];        
        }
        
        // Divide by 512 (sum of the coefficients) to scale output
        IF_est_filtered = sum / 512;
        
        //printf("%d - %d, %d\r\n",IF_estimate,IF_est_filtered,LQI_chip_errors);
        
        // The LO frequency steps are about ~80-100 kHz, so make an adjustment only if the error is larger than that
        // These hysteresis bounds (+/- X) have not been optimized
        // Must wait long enough between changes for FIR to settle (at least as many packets as there are taps in the FIR)
        // For now, assume that TX/RX should both be updated, even though the IF information is only from the RX code
        if(frequency_update_cooldown_timer == frequency_update_rate){
            if(IF_est_filtered > 520){
                RX_channel_codes[current_RF_channel - 11]++; 
                TX_channel_codes[current_RF_channel - 11]++; 
            }
            if(IF_est_filtered < 480){
                RX_channel_codes[current_RF_channel - 11]--; 
                TX_channel_codes[current_RF_channel - 11]--; 
            }
            
            //printf("--%d - %d\r\n",IF_estimate,IF_est_filtered);

            frequency_update_cooldown_timer = 0;
        }
    }
}
        


void radio_enable_interrupts(){
    
    // Enable radio interrupts in NVIC
    ISER = 0x40;
    
    // Enable all interrupts and pulses to radio timer
    //RFCONTROLLER_REG__INT_CONFIG = 0x3FF;   
        
    // Enable TX_SEND_DONE, RX_SFD_DONE, RX_DONE
    RFCONTROLLER_REG__INT_CONFIG = 0x1C;
    
    // Enable all errors
    //RFCONTROLLER_REG__ERROR_CONFIG = 0x1F;  
    
    // Enable only the RX CRC error
    RFCONTROLLER_REG__ERROR_CONFIG = 0x8;    //0x10; x10 is wrong? 
}


void radio_disable_interrupts(){

    // Clear radio interrupts in NVIC
    ICER = 0x40;
}


void rftimer_enable_interrupts(){
    // Enable RF timer interrupts in NVIC
    ISER = 0x80;
}


void rftimer_disable_interrupts(){
    // Enable RF timer interrupts in NVIC
    ICER = 0x80;
}

