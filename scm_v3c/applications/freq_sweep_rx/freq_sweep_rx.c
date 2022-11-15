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
    
#define LENGTH_PACKET       125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_RX_PKT          20+LENGTH_CRC  ///< length of rx packet

#define TIMER_PERIOD        7500           ///< 500 = 1ms@500kHz

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

//=========================== variables =======================================

typedef struct {
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
                int8_t          rxpk_rssi;
                uint8_t         rxpk_lqi;
    
    volatile    bool            rxpk_crc;
    // a flag to mark when to change configure
    volatile    bool            changeConfig;
    // a flag to avoid change configure during receiving frame
    volatile    bool            rxFrameStarted; 
    
    volatile    uint32_t        IF_estimate;
    volatile    uint32_t        LQI_chip_errors;
    volatile    uint32_t        cdr_tau_value;
    
                uint8_t         cfg_coarse;
                uint8_t         cfg_mid;
                uint8_t         cfg_fine;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);

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

    radio_setStartFrameRxCb(cb_startFrame_rx);
    radio_setEndFrameRxCb(cb_endFrame_rx);
    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
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

    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();
    
    // Enable optical SFD interrupt for optical calibration
    optical_enable();
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    // configure 

    while(1){
        
        // loop through all configuration
        for (app_vars.cfg_coarse=0;app_vars.cfg_coarse<STEPS_PER_CONFIG;app_vars.cfg_coarse++){
            for (app_vars.cfg_mid=0;app_vars.cfg_mid<STEPS_PER_CONFIG;app_vars.cfg_mid++){
                for (app_vars.cfg_fine=0;app_vars.cfg_fine<STEPS_PER_CONFIG;app_vars.cfg_fine++){
//                    printf(
//                        "coarse=%d, middle=%d, fine=%d\r\n", 
//                        app_vars.cfg_coarse,app_vars.cfg_mid,app_vars.cfg_fine
//                    );
                    for (i=0;i<NUMPKT_PER_CFG;i++) {
                        while(app_vars.rxFrameStarted == true);
                        radio_rfOff();
                        LC_FREQCHANGE(app_vars.cfg_coarse,app_vars.cfg_mid,app_vars.cfg_fine);
                        radio_rxEnable();
                        radio_rxNow();
                        rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
                        app_vars.changeConfig = false;
                        while (app_vars.changeConfig==false);
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_startFrame_rx(uint32_t timestamp){
    
    app_vars.rxFrameStarted = true;
}

void    cb_endFrame_rx(uint32_t timestamp){
    
    uint8_t i;
    
    radio_getReceivedFrame(
        &(app_vars.packet[0]),
        &app_vars.packet_len,
        sizeof(app_vars.packet),
        &app_vars.rxpk_rssi,
        &app_vars.rxpk_lqi
    );
        
    radio_rfOff();
    
    if(
        radio_getCrcOk()
    ){
        // Only record IF estimate, LQI, and CDR tau for valid packets
        app_vars.IF_estimate        = radio_getIFestimate();
        app_vars.LQI_chip_errors    = radio_getLQIchipErrors();
        
        printf(
            "pkt received on ch%d %c%c%c%c.%d.%d.%d\r\n",
            app_vars.packet[4],
            app_vars.packet[0],
            app_vars.packet[1],
            app_vars.packet[2],
            app_vars.packet[3],
            app_vars.cfg_coarse,
            app_vars.cfg_mid,
            app_vars.cfg_fine
        );
        
        app_vars.packet_len = 0;
        memset(&app_vars.packet[0],0,LENGTH_PACKET);
    }
    
    radio_rxEnable();
    radio_rxNow();
    
    app_vars.rxFrameStarted = false;
    
}

void    cb_timer(void) {
    
    app_vars.changeConfig = true;
}

