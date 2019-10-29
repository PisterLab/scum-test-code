/**
\brief This program conducts a freqency sweeping test.

After loading this program, SCuM will send 10 packet on each configuration 
of coarse[5 bits], middle[5 bits] and fine[5 bits]. Meanwhile, an OpenMote
listens on one Channel and print out the freq_offset if it receives a frame.

This program supposes to be run 16 times on for each channel setting on OpenMote
side.
*/

#include <string.h>

#include "scm3c_hardware_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
    
#define LC_CODE_RX      700 //Board Q3: tested at Inria A102 room (Oct, 16 2019)
#define LC_CODE_TX      707 //Board Q3: tested at Inria A102 room (Oct, 16 2019)

#define LENGTH_PACKET   125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_TX_PKT      30+LENGTH_CRC  ///< length of tx packet
#define LEN_RX_PKT      20+LENGTH_CRC  ///< length of rx packet
#define CHANNEL         11             ///< 11=2.405GHz
#define TIMER_PERIOD    1000           ///< 500 = 1ms@500kHz
#define ID              0x99           ///< byte sent in the packets

//=========================== variables =======================================

typedef struct {
    uint8_t         packet[LENGTH_PACKET];
    uint8_t         packet_len;
    bool            sendDone;
} app_vars_t;

app_vars_t app_vars;

extern unsigned int RX_channel_codes[16];
extern unsigned int TX_channel_codes[16];

//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_timer(void);

//=========================== main ============================================

int main(void) {
    
    int t,t2,x;
    uint32_t calc_crc;

    uint8_t         cfg_coarse;
    uint8_t         cfg_middle;
    uint8_t         cfg_fine;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    radio_init();
    rftimer_init();
    
    radio_setEndFrameTxCb(cb_endFrame_tx);
    
    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    
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
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    //skip building a channel table for now; hardcode LC values
    RX_channel_codes[0] = LC_CODE_RX; 
    TX_channel_codes[0] = LC_CODE_TX;
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    // configure 

    while(1){
        
        // initialize configuration
        cfg_coarse  = 0;
        cfg_middle  = 0;
        cfg_fine    = 0;
        
        // loop through all configuration
        for (cfg_coarse=0;cfg_coarse<32;cfg_coarse++){
            for (cfg_middle=0;cfg_middle<32;cfg_middle++){
                for (cfg_fine=0;cfg_fine<32;cfg_fine++){
                    if (app_vars.sendDone==true){
                        // add new frequency configuration function to radio (TBD)
                        radio_setFrequency(cfg_coarse,cfg_middle,cfg_fine,FREQ_TX);
                        radio_txEnable();
                        rftimer_setCompareIn(rftime_readCounter()+TIMER_PERIOD);
                        app_vars.sendDone = false;
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_endFrame_tx(uint32_t timestamp){
    
    radio_rfOff();
    
    app_vars.sendDone = true;
    
}

void    cb_timer(void) {
    
    // Tranmit the packet
    radio_txNow();
}

