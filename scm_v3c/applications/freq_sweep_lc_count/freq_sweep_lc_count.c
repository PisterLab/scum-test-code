/**
\brief This program conducts a freqency sweeping test.

After loading this program, SCuM will turn on radio to listen on each 
configuration of coarse[5 bits], middle[5 bits] and fine[5 bits] for 15ms.
Meanwhile, an OpenMote will send NUMPKT_PER_CFG packet on one channel every
4ms. SCuM will print out the pkt it received at each config settings.

This program supposes to be run 16 times on for each channel setting on OpenMote
side.
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE           (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH         (*((unsigned int *) 0x0000FFF8))

#define TIMER_PERIOD        500       ///< 500 = 1ms@500kHz
#define STEPS_PER_CONFIG    32

#define NUM_SAMPLES         10

// measure the LC_count at RX state by default
// uncomment the following macro to change to measure at TX state
//#define FREQ_SWEEP_TX

//=========================== variables =======================================

typedef struct {
    
    volatile    uint8_t         sample_index;
                uint32_t        samples[NUM_SAMPLES];
    
                uint8_t         cfg_coarse;
                uint8_t         cfg_mid;
                uint8_t         cfg_fine;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_timer(void);

uint32_t average_sample(void);
void     update_configuration(void);

//=========================== main ============================================

int main(void) {
    
    uint32_t calc_crc;
    
    uint8_t         i;
    uint8_t         j;
    uint8_t         offset;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();

    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    rftimer_disable_interrupts();
    
    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity..."); 
    
    calc_crc = crc32c(0x0000,CODE_LENGTH);
    
    if(calc_crc == CRC_VALUE){
        printf("CRC OK\r\n");
    } else{
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

#ifdef FREQ_SWEEP_TX
    radio_txEnable();
#else
    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();
#endif
    
    LC_FREQCHANGE(app_vars.cfg_coarse, 15, app_vars.cfg_fine);
    
    // Enable optical SFD interrupt for optical calibration
    optical_enable();
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinished() == 0);

    printf("Cal complete\r\n");
    
    // schedule the first timer
    rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);

    while(1){
        
    }
}

//=========================== public ==========================================

//=========================== private =========================================

uint32_t     average_sample(void){
    uint8_t i;
    uint32_t avg;
    
    avg = 0;
    for (i=0;i<NUM_SAMPLES;i++) {
        avg += app_vars.samples[i];
    }
    avg = avg/NUM_SAMPLES;
    return avg;
}

void     update_configuration(void){
    app_vars.cfg_fine++;
    if (app_vars.cfg_fine==STEPS_PER_CONFIG){
        app_vars.cfg_fine = 0;
        app_vars.cfg_mid++;
        if (app_vars.cfg_mid==STEPS_PER_CONFIG){
            app_vars.cfg_mid = 0;
            app_vars.cfg_coarse++;
            if (app_vars.cfg_coarse==STEPS_PER_CONFIG){
                app_vars.cfg_coarse = 0;
            }
        }
    }
}

void    cb_timer(void) {
    
    uint32_t delay;
    
    uint32_t avg_sample;
    uint32_t count_2M;
    uint32_t count_LC;
    uint32_t count_adc;
    
    rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
    read_counters_3B(&count_2M,&count_LC,&count_adc);
    app_vars.samples[app_vars.sample_index] = count_LC;
    app_vars.sample_index++;
    if (app_vars.sample_index==NUM_SAMPLES) {
        app_vars.sample_index = 0;
        avg_sample = average_sample();
        
        printf(
            "%d.%d.%d.%d\r\n",
            app_vars.cfg_coarse,
            app_vars.cfg_mid,
            app_vars.cfg_fine,
            avg_sample
        );
        
        update_configuration();
#ifdef FREQ_SWEEP_TX
        radio_txEnable();
#else
        radio_rxEnable();
#endif
        LC_FREQCHANGE(app_vars.cfg_coarse, app_vars.cfg_mid, app_vars.cfg_fine);
    }
}

