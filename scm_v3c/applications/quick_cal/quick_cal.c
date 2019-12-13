/**
\brief This program helps SCuM to find the frequency settings
for each 16 channel with a quick_cal box.

\author Tengfei Chang <tengfei.chang@inria.fr> Dec 2019
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

#define SYNC_CHANNEL        11
#define NUM_CHANNELS        16
#define SLOTFRAME_LEN       16

#define MAX_PKT_LEN         125+LENGTH_CRC
#define PKT_TX_LEN          2+LENGTH_CRC

// timing
#define SLOT_DURATION       500000  ///< 500 = 1ms@500kHz
#define SUB_SLOT_DURATION   400     ///< 305 = 610us@500kHz

// frequency settings
#define SYNC_FREQ_START     22*32*32
#define FREQ_RANGE          2*32*32
#define SWEEP_STEP          2
#define COARSE_MASK         (0x001f<<10)
#define COARSE_OFFSET       10
#define MID_MASK            (0x001f<<5)
#define MID_OFFSET          5
#define FINE_MASK           (0x001f)
#define FINE_OFFSET         0

//=========================== variables =======================================

typedef enum{
    T_IDLE              = 0,
    T_TX                = 1,
    T_RX                = 2,
}type_t;

typedef enum{
    S_IDLE              = 0,
    S_LISTENING         = 1,
    S_RECEIVING         = 2,
    S_RXPROC            = 3,
    S_DATA_SENDING      = 4,
    S_ACK_SENDING       = 5,
    S_DATA_SENDDONE     = 6,
    S_ACK_SENDDONE      = 7,
    S_DATA_SEND         = 8,
    S_ACK_SEND          = 9,
}state_t;

typedef struct {
    // store the frequency settings for transmitting and receiving each 
    // 16 channels: 
    // - coarse = (freq_setting & (0x001f<<10))>>10
    // - mid    = (freq_setting & (0x001f<< 5))>> 5
    // - fine   =  freq_setting &  0x001f
    
    uint16_t    freq_setting_tx[NUM_CHANNELS];
    uint16_t    freq_setting_rx[NUM_CHANNELS];
    
    uint16_t    freq_setting_index;
    uint16_t    current_freq_setting;
    
    bool        isSync;                // is synchronized?
    uint8_t     currentSlotOffset;
    uint32_t    slotRerference;        // the timer read when slot timer fired
    
    type_t      type;                  // tx or rx, used in start/end Frame ISR
    state_t     state;                 // radio state
    
    uint8_t     channel_to_calc;
    
    bool        freq_setting_rx_done;  // TRUE when all 16 rx settings are found
    bool        freq_setting_tx_done;  // TRUE when all 16 tx settings are found
    
    uint8_t     packet[MAX_PKT_LEN];
    uint8_t     pkt_len;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void    cb_startFrame(uint32_t timestamp);
void    cb_endFrame(uint32_t timestamp);
void    cb_slot_timer(void);
void    cb_calc_process_timer(void);

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

    radio_setStartFrameRxCb(cb_startFrame);
    radio_setEndFrameRxCb(cb_endFrame);
    rftimer_set_callback(cb_slot_timer);
    
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
    
    app_vars.currentSlotOffset = SLOTFRAME_LEN - 1;
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    app_vars.current_freq_setting = SYNC_FREQ_START;
    rftimer_setCompareIn(rftimer_readCounter()+SLOT_DURATION);

    while(1){
        
    }
}

//=========================== public ==========================================

//=========================== private =========================================


void    cb_startFrame(uint32_t timestamp){
    
}

void    cb_endFrame(uint32_t timestamp){
    
}

void    cb_slot_timer(void) {
    
    app_vars.slotRerference = rftimer_readCounter();
    rftimer_setCompareIn(app_vars.slotRerference+SLOT_DURATION);
    
    app_vars.currentSlotOffset = \
        (app_vars.currentSlotOffset+1) % SLOTFRAME_LEN;
    
    if (app_vars.isSync){
        
        if (app_vars.freq_setting_rx_done){
            
            app_vars.type = T_TX;
            
            app_vars.packet[0] = 0xCA;
            app_vars.packet[1] = 0xFE;
            
            if (app_vars.currentSlotOffset==0){
                
                // skip slot 0 as all motes are listening on ch11 except root
                
                // no way to calculate channel rx_11
                
                return;
                
            } else {
                
                
                app_vars.channel_to_calc = \
                    app_vars.currentSlotOffset-1 + SYNC_CHANNEL;
                
                app_vars.freq_setting_index   = 0;
                cb_calc_process_timer();
            }
            
        } else {
            
            // sweep frequency setting to find one Rx channel
             
            app_vars.type = T_RX; 
            
            app_vars.channel_to_calc = \
                app_vars.currentSlotOffset + SYNC_CHANNEL;
            
            if (app_vars.currentSlotOffset==0){
                
                app_vars.current_freq_setting = SYNC_FREQ_START;
            } else {
                
                if (app_vars.freq_setting_rx[app_vars.currentSlotOffset-1]==0){
                    // the previous frequency is unknown
                    
                    // don't performance frequency sweep
                    
                    // something goes wrong
                    
                    return;
                    
                } else {
                    
                    app_vars.current_freq_setting = 
                        app_vars.freq_setting_rx[app_vars.currentSlotOffset-1];
                }
            }
            
            app_vars.freq_setting_index   = 0;
            cb_calc_process_timer();
        }
    } else {
        
        app_vars.type = T_RX;
        
        app_vars.freq_setting_index   = 0;
        app_vars.current_freq_setting = SYNC_FREQ_START;
        
        cb_calc_process_timer();
    }
}

void    cb_calc_process_timer(void) {
    
    app_vars.freq_setting_index += 1;
    
    if (app_vars.freq_setting_index>FREQ_RANGE/SWEEP_STEP){
        rftimer_disable_interrupts();
        rftimer_set_callback(cb_slot_timer);
        rftimer_setCompareIn(app_vars.slotRerference + SLOT_DURATION);
    } else {
        rftimer_disable_interrupts();
        rftimer_set_callback(cb_calc_process_timer);
        rftimer_setCompareIn(
            app_vars.slotRerference +     \
            app_vars.freq_setting_index*SUB_SLOT_DURATION
        );
    }
    
    radio_rfOff();
    app_vars.current_freq_setting += SWEEP_STEP;
    LC_FREQCHANGE(
        (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
        (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
        (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
    );
    
    switch(app_vars.type){
        
    case T_TX:
        
        if (app_vars.current_freq_setting>SYNC_FREQ_START+FREQ_RANGE){
            if (app_vars.isSync) {
                // freq_sweep is done, calculate the freq_setting
                // todo
                
            } else {
                // something goes wrong
            }
        } else {
            app_vars.pkt_len = PKT_TX_LEN;
            radio_loadPacket(app_vars.packet, app_vars.pkt_len);
            radio_txEnable();
            radio_txNow();
        }
    
    break;
    case T_RX:

    
        if (app_vars.current_freq_setting>SYNC_FREQ_START+FREQ_RANGE){
            if (app_vars.isSync) {
                // freq_sweep is done, calculate the freq_setting
                // todo
                
            } else {
                // doesn't receive a valid frame during one slot to sync
                
                // wait next slot to repeat
            }
        } else {
            radio_rxEnable();
            radio_rxNow();
        }

    break;
    default:
        // wrong state
    break;
    }
}
