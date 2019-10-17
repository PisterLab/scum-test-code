#include "Memory_Map.h"
#include <stdio.h>
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "radio.h"
#include "bucket_o_functions.h"

extern unsigned int LC_target;
extern unsigned int LC_code;
extern unsigned int IF_clk_target;
extern unsigned int IF_coarse;
extern unsigned int IF_fine;
extern unsigned int cal_iteration;
extern unsigned int ASC[38];

extern unsigned int HF_CLOCK_fine;
extern unsigned int HF_CLOCK_coarse;
extern unsigned int RC2M_superfine;
extern unsigned int RC2M_fine;
extern unsigned int RC2M_coarse;

unsigned int num_32k_ticks_in_100ms;
unsigned int num_2MRC_ticks_in_100ms;
unsigned int num_IFclk_ticks_in_100ms;
unsigned int num_LC_ch11_ticks_in_100ms;
unsigned int num_HFclock_ticks_in_100ms;

extern unsigned int RX_channel_codes[16];
extern unsigned int TX_channel_codes[16];
extern unsigned short optical_cal_iteration,optical_cal_finished;

// Timer parameters 
extern unsigned int packet_interval;

// This interrupt goes off every time 32 new bits of data have been shifted into the optical register
// Do not recommend trying to do any CPU intensive actions while trying to receive optical data
// ex, printf will mess up the received data values
void optical_32_isr(){
    //printf("Optical 32-bit interrupt triggered\r\n");
    
    //unsigned int LSBs, MSBs, optical_shiftreg;
    //int t;
    
    // 32-bit register is analog_rdata[335:304]
    //LSBs = ANALOG_CFG_REG__19; //16 LSBs
    //MSBs = ANALOG_CFG_REG__20; //16 MSBs
    //optical_shiftreg = (MSBs << 16) + LSBs;    
    
    // Toggle GPIO 0
    //GPIO_REG__OUTPUT ^= 0x1;
    
}

// This interrupt goes off when the optical register holds the value {221, 176, 231, 47}
// This interrupt can also be used to synchronize to the start of an optical data transfer
// Need to make sure a new bit has been clocked in prior to returning from this ISR, or else it will immediately execute again
void optical_sfd_isr(){
    
    int t;
    unsigned int rdata_lsb, rdata_msb; 
    unsigned int count_LC, count_32k, count_2M, count_HFclock, count_IF;
        
    // Disable all counters
    ANALOG_CFG_REG__0 = 0x007F;
    
    // Keep track of how many calibration iterations have been completed
    optical_cal_iteration++;
        
    // Read 32k counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x000000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x040000);
    count_32k = rdata_lsb + (rdata_msb << 16);

    // Read HF_CLOCK counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x100000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x140000);
    count_HFclock = rdata_lsb + (rdata_msb << 16);

    // Read 2M counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
    count_2M = rdata_lsb + (rdata_msb << 16);
        
    // Read LC_div counter (via counter4)
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
    count_LC = rdata_lsb + (rdata_msb << 16);
    
    // Read IF ADC_CLK counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x300000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x340000);
    count_IF = rdata_lsb + (rdata_msb << 16);
    
    // Reset all counters
    ANALOG_CFG_REG__0 = 0x0000;        
    
    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;    
        
    // Don't make updates on the first two executions of this ISR
    if(optical_cal_iteration > 2){
        
        // Do correction on HF CLOCK
        // Fine DAC step size is about 6000 counts
        if(count_HFclock < 1997000) HF_CLOCK_fine--;
        if(count_HFclock > 2003000) HF_CLOCK_fine++;
        set_sys_clk_secondary_freq(HF_CLOCK_coarse, HF_CLOCK_fine);
        
        // Do correction on LC
        if(count_LC > (LC_target + 0)) LC_code -= 1;
        if(count_LC < (LC_target - 0))    LC_code += 1;
        LC_monotonic(LC_code);
            
        // Do correction on 2M RC
        // Coarse step ~1100 counts, fine ~150 counts, superfine ~25
        // Too fast
        if(count_2M > (200600)) RC2M_coarse += 1;
        else if(count_2M > (200080)) RC2M_fine += 1;
        else if(count_2M > (200015))    RC2M_superfine += 1;
        
        // Too slow
        if(count_2M < (199400))    RC2M_coarse -= 1;
        else if(count_2M < (199920)) RC2M_fine -= 1;
        else if(count_2M < (199985))    RC2M_superfine -= 1;
        set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);

        // Do correction on IF RC clock
        // Fine DAC step size is ~2800 counts
        if(count_IF > (1600000+1400)) IF_fine += 1;
        if(count_IF < (1600000-1400))    IF_fine -= 1;
        set_IF_clock_frequency(IF_coarse, IF_fine, 0);
        
        analog_scan_chain_write(&ASC[0]);
        analog_scan_chain_load();    
    }
    
    // Debugging output
    printf("HF=%d-%d   2M=%d-%d,%d,%d   LC=%d-%d   IF=%d-%d\r\n",count_HFclock,HF_CLOCK_fine,count_2M,RC2M_coarse,RC2M_fine,RC2M_superfine,count_LC,LC_code,count_IF,IF_fine); 
     
    if(optical_cal_iteration == 25){
        // Disable this ISR
        ICER = 0x0800;
        optical_cal_iteration = 0;
        optical_cal_finished = 1;
        
        // Store the last count values
        num_32k_ticks_in_100ms = count_32k;
        num_2MRC_ticks_in_100ms = count_2M;
        num_IFclk_ticks_in_100ms = count_IF;
        num_LC_ch11_ticks_in_100ms = count_LC;
        num_HFclock_ticks_in_100ms = count_HFclock;
        
        // Update the expected packet rate based on the measured HF clock frequency
        // Have an estimate of how many 20MHz clock ticks there are in 100ms
        // But need to know how many 20MHz/40 = 500kHz ticks there are in 125ms (if doing 8 Hz packet rate)
        // (125 / 100) / 40 = 1/32
        packet_interval = num_HFclock_ticks_in_100ms >> 5;
    
        // Debug prints
        //printf("LC_code=%d\r\n", LC_code);
        //printf("IF_fine=%d\r\n", IF_fine);
        
        printf("done\r\n");
                
        // This was an earlier attempt to build out a complete table of LC_code for TX/RX on each channel
        // It doesn't really work well yet so leave it commented
        //printf("Building channel table...");
        
        //build_channel_table(LC_code);
        
        //printf("done\r\n");
        
        //radio_disable_all();
        
        // Halt all counters
        ANALOG_CFG_REG__0 = 0x0000;
    }
}