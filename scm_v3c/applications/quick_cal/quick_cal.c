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
#include "gpio.h"

//=========================== defines =========================================

#define CRC_VALUE           (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH         (*((unsigned int *) 0x0000FFF8))

#define SYNC_CHANNEL        11
#define NUM_CHANNELS        16
#define SLOTFRAME_LEN       16

#define NUM_SAMPLES         100

#define MAX_PKT_LEN         125+LENGTH_CRC
#define TARGET_PKT_LEN      2+LENGTH_CRC

// timing
#define SLOT_DURATION       500000  ///< 500 = 1ms@500kHz
#define TARGET_PKT_INTERVAL 305     ///< 305 = 610us@500kHz
#define SUB_SLOT_DURATION   400     ///< 305 = 610us@500kHz
#define TXOFFSET            191     ///< measured, 382us
#define WD_DATA_SENDDONE    100     ///>measured,  161us  100 = 200us@500kHz
#define WD_RECEIVING_ACK    300     ///>measured,  412us  250 = 600us@500kHz

// frequency settings
#define SYNC_FREQ_START_RX  23*32*32
#define FREQ_RANGE_RX       32*32
#define SWEEP_STEP_RX       1

#define SYNC_FREQ_START_TX  23*32*32
#define FREQ_RANGE_TX       32*32
#define SWEEP_STEP_TX       3

// calculate frequency settings

#define CONTINOUS_NUM_SAMPLES_TX 7 // size of a sample sequence that increments by 1
#define CONTINOUS_NUM_SAMPLES_RX 7 // size of a sample sequence that increments by 1

//#define NUM_PKT_PER_SLOT    32*32
#define NUM_PKT_PER_SLOT    (SLOT_DURATION/TARGET_PKT_INTERVAL-2)

#define COARSE_MASK         (0x001f<<10)
#define COARSE_OFFSET       10
#define MID_MASK            (0x001f<<5)
#define MID_OFFSET          5
#define FINE_MASK           (0x001f)
#define FINE_OFFSET         0

#define MAGIC_BYTE          0xCF    // Crystal Free

//=========================== variables =======================================

typedef enum{
    T_IDLE              = 0,
    T_TX                = 1,
    T_RX                = 2,
}type_t;

typedef enum{
    S_IDLE              = 0,
    S_LISTEN_FOR_DATA   = 1,
    S_LISTEN_FOR_ACK    = 2,
    S_RECEIVING_DATA    = 3,
    S_RECEIVING_ACK     = 4,
    S_RXPROC            = 6,
    S_DATA_SENDING      = 7,
    S_ACK_SENDING       = 8,
    S_DATA_SENDDONE     = 9,
    S_ACK_SENDDONE      = 10,
    S_DATA_SEND         = 11,
    S_ACK_SEND          = 12,
}state_t;

typedef struct {
    // store the frequency settings for transmitting and receiving each 
    // 16 channels: 
    // - coarse = (freq_setting & (0x001f<<10))>>10
    // - mid    = (freq_setting & (0x001f<< 5))>> 5
    // - fine   =  freq_setting &  0x001f
    
    uint16_t    freq_setting_tx[NUM_CHANNELS];
    uint16_t    freq_setting_rx[NUM_CHANNELS];
    
    uint8_t     sample_index;
    uint16_t    freq_setting_sample[NUM_SAMPLES];
    
    uint16_t    current_freq_setting;
    
    bool        isSync;                // is synchronized?
    uint8_t     currentSlotOffset;
    uint32_t    slotReference;        // the timer read when slot timer fired
    uint32_t    lastCaptureTime;       // time stampe of receiving frame
    
    type_t      type;                  // tx or rx, used in start/end Frame ISR
    state_t     state;                 // radio state
    
    uint8_t     channel_to_calibrate;
    
    bool        freq_setting_rx_done;  // TRUE when all 16 rx settings are found
    bool        freq_setting_tx_done;  // TRUE when all 16 tx settings are found
    
    uint8_t     packet[MAX_PKT_LEN];
    uint8_t     pkt_len;
     int8_t     rxpk_rssi;
    uint8_t     rxpk_lqi;
       bool     rxpk_crc;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void    cb_startFrame_rx(uint32_t timestamp);
void    cb_endFrame_rx(uint32_t timestamp);
void    cb_startFrame_tx(uint32_t timestamp);
void    cb_endFrame_tx(uint32_t timestamp);
void    cb_slot_timer(void);
void    cb_sweep_process_timer(void);
void    cb_timeout_error(void);

void    synchronize(uint32_t capturedTime, uint8_t pkt_channel, uint16_t pkt_seqNum);
void    delay();
uint16_t    find_freq_settings(uint16_t* freq_setting_samples, uint8_t length, uint8_t continous_sample_size);

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
    radio_setStartFrameTxCb(cb_startFrame_tx);
    radio_setEndFrameTxCb(cb_endFrame_tx);
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
    app_vars.current_freq_setting = SYNC_FREQ_START_RX;
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    rftimer_setCompareIn(rftimer_readCounter()+SLOT_DURATION);

    while(1){
        
    }
}

//=========================== public ==========================================

//=========================== private =========================================

//====  delay
void    delay(void){
    uint16_t i;
    // works,           0x1f, 0x0f
    // doesn't work     when comment out
    for (i=0;i<0x001f;i++);
}

//==== sync

void synchronize(uint32_t capturedTime, uint8_t pkt_channel, uint16_t pkt_seqNum){

    uint32_t slot_boudary;

    // synchronize currentslotoffset
    app_vars.currentSlotOffset = pkt_channel-SYNC_CHANNEL;

    // synchronize to slot boudary
    rftimer_disable_interrupts();
    slot_boudary = capturedTime - pkt_seqNum*TARGET_PKT_INTERVAL - TXOFFSET;
    rftimer_set_callback(cb_slot_timer);
    rftimer_setCompareIn(slot_boudary+SLOT_DURATION);

    app_vars.isSync = true;
}

//==== calcuate the freqency setting

/**
    Algorithm to find frequency setting among frequency samples.
 
    1. find the a sequence of samples that increments by 1 in the samples set.
    
    2. If the sequency length reach to CONTINOUS_NUM_SAMPLES
            return the median sample value of this sequence
        else:
            keep looking for such sequency length
    3. If not find sequency length larger than CONTINOUS_NUM_SAMPLES
            return the median of whole sample set
*/

uint16_t find_freq_settings(uint16_t* samples, uint8_t length, uint8_t continous_sample_size){
    uint8_t i;
    uint16_t previous_sample;
    uint8_t num_continous_samples;
    
    num_continous_samples = 0;
    
    previous_sample = 0;
    
    for (i=0;i<length;i++){
        
        if (samples[i]!=0){
            if (previous_sample==0){
                previous_sample = samples[i];
            } else {
                if (samples[i]-previous_sample==1){
                    num_continous_samples++;
                } else {
                    if (num_continous_samples>=continous_sample_size){
                        return samples[i-num_continous_samples/2];
                    }
                    num_continous_samples = 0;
                }
                previous_sample = app_vars.freq_setting_sample[i];
            }
        } else {
            
            // no more freq setting samples
            
            // take the median freq_settings
            return samples[i/2];
        }
    }
}

//==== isr

//---- startFrame
void    cb_startFrame_tx(uint32_t timestamp){
    
    app_vars.lastCaptureTime = timestamp;
}

void    cb_startFrame_rx(uint32_t timestamp){
    
    app_vars.lastCaptureTime = timestamp;
    
    switch(app_vars.state){
        case S_LISTEN_FOR_DATA:
            app_vars.state = S_RECEIVING_DATA;
        
            // set watch dog
            rftimer_disable_interrupts();
            rftimer_set_callback(cb_timeout_error);
            rftimer_setCompareIn(rftimer_readCounter()+WD_DATA_SENDDONE);
        
        break;
        case S_LISTEN_FOR_ACK:
            app_vars.state = S_RECEIVING_ACK;
        
            // set watch dog
            rftimer_disable_interrupts();
            rftimer_set_callback(cb_timeout_error);
            rftimer_setCompareIn(rftimer_readCounter()+WD_DATA_SENDDONE);
        break;
        default:
            // something goes wrong
            printf("unexpected state=%d at startFrame_rx!!\r\n");
        break;
    }
}

//---- endFrame
void    cb_endFrame_tx(uint32_t timestamp){
    
    uint8_t     setting_index;
    
    radio_rfOff();

    setting_index = app_vars.channel_to_calibrate-SYNC_CHANNEL;

    LC_FREQCHANGE(
        (app_vars.freq_setting_rx[setting_index] & COARSE_MASK) >> COARSE_OFFSET,
        (app_vars.freq_setting_rx[setting_index] &    MID_MASK) >>    MID_OFFSET,
        (app_vars.freq_setting_rx[setting_index] &   FINE_MASK) >>   FINE_OFFSET
    );
    radio_rxEnable();
    radio_rxNow();

    // set watch dog
    rftimer_disable_interrupts();
    rftimer_set_callback(cb_timeout_error);
    rftimer_setCompareIn(rftimer_readCounter()+WD_RECEIVING_ACK);

    app_vars.state = S_LISTEN_FOR_ACK;
}

void    cb_endFrame_rx(uint32_t timestamp){
    
    bool        isValidFrame;
    uint8_t     pkt_channel;
    uint16_t    pkt_seqNum;
    
    radio_rfOff();
    
    gpio_7_toggle();
    
    memset(app_vars.packet,0,MAX_PKT_LEN);

    // get packet from radio
    radio_getReceivedFrame(
        app_vars.packet,
        &app_vars.pkt_len,
        sizeof(app_vars.packet),
        &app_vars.rxpk_rssi,
        &app_vars.rxpk_lqi
    );

    // check the frame is valid or not

    isValidFrame = true;

    if (app_vars.pkt_len == TARGET_PKT_LEN){

        pkt_channel = SYNC_CHANNEL+((app_vars.packet[0] & 0xf0)>>4);
        pkt_seqNum  = ((uint16_t)(app_vars.packet[0] & 0x0f))<<8 |
                       (uint16_t)(app_vars.packet[1]);
        
        if (pkt_seqNum>=NUM_PKT_PER_SLOT){
            isValidFrame = false;
        }
        
        if (app_vars.isSync){
            if ((pkt_channel - SYNC_CHANNEL)!= app_vars.currentSlotOffset){
                isValidFrame = false;
            }
        } else {
            if (pkt_channel != SYNC_CHANNEL){
                isValidFrame = false;
            }
        }
    } else {
        
        isValidFrame = false;
    }
    
    if (isValidFrame){
        
        gpio_8_toggle();
        
        if (app_vars.isSync){
            
            switch(app_vars.state){
            case S_RECEIVING_DATA:
                
                rftimer_disable_interrupts();
            
                if (app_vars.freq_setting_rx[app_vars.currentSlotOffset]==0) {
                    
                    rftimer_set_callback(cb_sweep_process_timer);
                    rftimer_setCompareIn(rftimer_readCounter()+SUB_SLOT_DURATION);
                } else {
                    rftimer_set_callback(cb_slot_timer);
                    rftimer_setCompareIn(app_vars.slotReference+SLOT_DURATION);
                }
            
                gpio_1_toggle();
                
                if (
                    app_vars.currentSlotOffset==0   &&
                    app_vars.freq_setting_rx[0] != 0
                ) {
                    // resynchronize
                    synchronize(app_vars.lastCaptureTime, pkt_channel, pkt_seqNum);
                    
                } else {
                
                    // doing calibration on rx channel if haven't yet
                    
                    if (app_vars.freq_setting_rx[app_vars.currentSlotOffset]==0){
                    
                        app_vars.freq_setting_sample[app_vars.sample_index++] = \
                            app_vars.current_freq_setting;
                    }
                }
                
                app_vars.state = S_IDLE;
                
            break;
            case S_RECEIVING_ACK:
                
                rftimer_disable_interrupts();
                if (app_vars.freq_setting_tx[app_vars.channel_to_calibrate-SYNC_CHANNEL]==0) {
                    
                    rftimer_set_callback(cb_sweep_process_timer);
                    rftimer_setCompareIn(rftimer_readCounter()+SUB_SLOT_DURATION);
                } else {
                    rftimer_set_callback(cb_slot_timer);
                    rftimer_setCompareIn(app_vars.slotReference+SLOT_DURATION);
                }
                
                // doing calibration on tx channel
                
                app_vars.freq_setting_sample[app_vars.sample_index++] = \
                    app_vars.current_freq_setting;
                
                app_vars.state = S_IDLE;
                
            break;
            default:
                printf("Unexpected state=%d at endFrame ISR!\r\n",app_vars.state);
            break;
            }
        } else {
                
            printf(
                "channel=%d, seqNum=%d coarse=%d, mid=%d, fine=%d\r\n", 
                pkt_channel, pkt_seqNum, 
                (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
            );
            
            // synchronize to the network
            synchronize(app_vars.lastCaptureTime, pkt_channel, pkt_seqNum);
            
            app_vars.state = S_IDLE;
            
        }
    }
}

//==== timer

//---- slot timer
void    cb_slot_timer(void) {
    
    uint8_t temp;
    bool    tx_ch_11_to_25_done;
    uint8_t i;
    
    gpio_3_toggle();
    
    app_vars.slotReference = rftimer_readCounter();
    rftimer_setCompareIn(app_vars.slotReference+SLOT_DURATION);
    
    app_vars.currentSlotOffset = \
        (app_vars.currentSlotOffset+1) % SLOTFRAME_LEN;
    
    app_vars.state = S_IDLE;
    
    if (app_vars.isSync){
        
        // sync'ed
        
        if (app_vars.currentSlotOffset==0){
            
            // sync'ed
            // slot 0 
            
            if (app_vars.freq_setting_rx_done){
                
                // sync'ed
                // slot 0
                // freq_setting_RX_done is TRUE
                
                // re-sync
                app_vars.type = T_RX;
                
                app_vars.current_freq_setting = app_vars.freq_setting_rx[0];
                LC_FREQCHANGE(
                    (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                    (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                    (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                );
                radio_rxEnable();
                radio_rxNow();
                
                app_vars.state = S_LISTEN_FOR_DATA;
                
            } else {
                
                // sync'ed
                // slot 0
                // freq_setting_RX_done is FALSE
                
                if (app_vars.freq_setting_rx[0] != 0){
                
                    // channel rx_11 found already
                    
                    // re-sync
                    app_vars.current_freq_setting = app_vars.freq_setting_rx[0];
                    LC_FREQCHANGE(
                        (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                        (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                        (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                    );
                    radio_rxEnable();
                    radio_rxNow();
                    
                    app_vars.state = S_LISTEN_FOR_DATA;
                    
                } else {
                    
                    app_vars.type = T_RX; 
            
                    app_vars.channel_to_calibrate = app_vars.currentSlotOffset + SYNC_CHANNEL;
                    
                    // channel rx_11 not found, setup freq sweep process
                    
                    app_vars.current_freq_setting = SYNC_FREQ_START_RX;
                    
                    cb_sweep_process_timer();
                }
            }
        } else {
            
            // sync'ed
            // slot 1 - 15
            
            if (app_vars.freq_setting_rx_done){
                
                //--- debugging
                
//                app_vars.type = T_RX;
//                app_vars.current_freq_setting = app_vars.freq_setting_rx[app_vars.currentSlotOffset];
//                LC_FREQCHANGE(
//                    (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
//                    (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
//                    (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
//                );
//                radio_rxEnable();
//                radio_rxNow();
//                
//                app_vars.state = S_LISTEN_FOR_DATA;
//                
//                return;
                
                //--- debugging
                
                // sync'ed
                // slot 1 - 15
                // freq_setting_RX_done is TRUE
                
                app_vars.type = T_TX;
                
                if (app_vars.freq_setting_tx_done){
                    
                    // sync'ed
                    // slot 1 - 15
                    // freq_setting_RX_done is TRUE
                    // freq_setting_TX_done is TRUE
                    
                    app_vars.current_freq_setting = app_vars.freq_setting_tx[app_vars.currentSlotOffset-1];
                    LC_FREQCHANGE(
                        (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                        (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                        (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                    );
                    app_vars.pkt_len = TARGET_PKT_LEN;
                    app_vars.packet[0] = app_vars.currentSlotOffset << 4;
                    app_vars.packet[1] = MAGIC_BYTE;
                    radio_loadPacket(app_vars.packet, app_vars.pkt_len);
                    radio_txEnable();
                    delay();
                    radio_txNow();
                    
                } else {
                    
                    // sync'ed
                    // slot 1 - 15
                    // freq_setting_RX_done is TRUE
                    // freq_setting_TX_done is FALSE
                    
                    if (app_vars.currentSlotOffset==1){
                        
                        // sync'ed
                        // slot 1
                        // freq_setting_RX_done is TRUE
                        // freq_setting_TX_done is FALSE
                        
                        // check whether channel 11-25 settings are recorded
                        
                        tx_ch_11_to_25_done = true;
                        for (i=0;i<15;i++){
                            if (app_vars.freq_setting_tx[i]==0){
                                tx_ch_11_to_25_done = false;
                                break;
                            }
                        }
                        
                        if (tx_ch_11_to_25_done){
                            // calculate for channel 26
                            app_vars.channel_to_calibrate = 26;
                            
                            // start with tx freq_setting of channel 25
                            app_vars.current_freq_setting = app_vars.freq_setting_tx[14];
                        } else {
                            app_vars.channel_to_calibrate = app_vars.currentSlotOffset-1 + SYNC_CHANNEL;
                            app_vars.current_freq_setting = SYNC_FREQ_START_TX;
                        }
                    } else {
                        
                        // sync'ed
                        // slot 2 - 15
                        // freq_setting_RX_done is TRUE
                        // freq_setting_TX_done is FALSE
                        
                        if (app_vars.freq_setting_tx[app_vars.currentSlotOffset-2]==0){
                            
                            printf("current slot=%d: previous TX frequency is unknonw \r\n",app_vars.currentSlotOffset);
                            
                            return;
                            
                        } else {
                            
                            app_vars.channel_to_calibrate = app_vars.currentSlotOffset-1 + SYNC_CHANNEL;
                            app_vars.current_freq_setting = app_vars.freq_setting_tx[app_vars.currentSlotOffset-2];
                        }
                    }
                    
                    cb_sweep_process_timer();
                }
            } else {
                
                // sync'ed
                // slot 1 - 15
                // freq_setting_RX_done is FALSE
                
                app_vars.type = T_RX;
                
                if (app_vars.freq_setting_rx[app_vars.currentSlotOffset-1]==0){
                    // The previous frequency is unknown
                    
                    // Don't performance frequency sweep
                    
                    // something goes wrong
                    
                    printf("current slot=%d: previous RX frequency is unknonw \r\n",app_vars.currentSlotOffset);
                    
                    return;
                    
                } else {
                    
                    if (app_vars.freq_setting_rx[app_vars.currentSlotOffset]==0){
                        
                        // doing freq_sweep for target channel
                        app_vars.channel_to_calibrate   = app_vars.currentSlotOffset + SYNC_CHANNEL;
                        app_vars.current_freq_setting   = app_vars.freq_setting_rx[app_vars.currentSlotOffset-1];
                        cb_sweep_process_timer();
                        
                    } else {
                        // channel rx_(11+currentSlotOffset) found already
                        
                        app_vars.current_freq_setting = app_vars.freq_setting_rx[app_vars.currentSlotOffset];
                        LC_FREQCHANGE(
                            (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                            (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                            (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                        );
                        radio_rxEnable();
                        radio_rxNow();
                        
                        app_vars.state = S_LISTEN_FOR_DATA;
                    }
                }
            }
        }
    } else {
        
        // de-sync
        
        app_vars.type = T_RX;
        
        app_vars.current_freq_setting   = SYNC_FREQ_START_RX;
        cb_sweep_process_timer();
    }
}

//---- subslot timer
void    cb_sweep_process_timer(void) {
    
    uint8_t     i;
    uint32_t    start_frequency;
    
    gpio_4_toggle();
    
    // reschedule sub calc_process
    rftimer_disable_interrupts();
    rftimer_set_callback(cb_sweep_process_timer);
    rftimer_setCompareIn(rftimer_readCounter() + SUB_SLOT_DURATION);
    
    switch(app_vars.type){
        
        case T_TX:
            
            // swept 2 coarse setting already?
        
            if (app_vars.isSync){
                if (app_vars.currentSlotOffset == 1){
                    // slot 1
                    if (app_vars.channel_to_calibrate == 26){
                        start_frequency = app_vars.freq_setting_rx[15];
                    } else {
                        start_frequency = SYNC_FREQ_START_TX;
                    }
                } else {
                        // slot 0
                    if (app_vars.currentSlotOffset == 0){
                        printf("TX freq_sweep_done abnormally on slot 0!\r\n");
                    } else {
                        // slot 2-15
                        start_frequency = app_vars.freq_setting_tx[app_vars.currentSlotOffset-2];
                    }
                }
            } else {
                printf("trying to calibrate tx freq_setting when de-sync!!\r\n");
            }
        
            // check if 2 coarse freq_sweep is done
            
            if (app_vars.current_freq_setting>start_frequency+FREQ_RANGE_TX){
                
                if (app_vars.isSync) {
                    
                    // freq_sweep is done, calculate the freq_setting
                    
                    radio_rfOff();
                    
                    rftimer_disable_interrupts();
                    rftimer_set_callback(cb_slot_timer);
                    rftimer_setCompareIn(app_vars.slotReference + SLOT_DURATION);
                    
                    gpio_5_toggle();
                    
                    // check if there is freq_setting sample recorded
                    
                    if (app_vars.freq_setting_sample[0]!=0){
                        
                        app_vars.freq_setting_tx[app_vars.channel_to_calibrate-SYNC_CHANNEL] = \
                            find_freq_settings(
                                    app_vars.freq_setting_sample, 
                                    NUM_SAMPLES,
                                    CONTINOUS_NUM_SAMPLES_TX
                                );
                        
                        // found at least one setting sample
                        
                        app_vars.sample_index = 0;
                        memset(
                            app_vars.freq_setting_sample,
                            0,
                            sizeof(app_vars.freq_setting_sample)
                        );
                        
                        // check whether all rx freq_settings are recorded
                        app_vars.freq_setting_tx_done = true;
                        for (i=0;i<SLOTFRAME_LEN;i++){
                            if (app_vars.freq_setting_tx[i]==0){
                                app_vars.freq_setting_tx_done = false;
                                break;
                            }
                        }
                        
                        printf("tx%d c=%d m=%d f=%d\r\n",
                            app_vars.channel_to_calibrate, 
                            (app_vars.freq_setting_tx[app_vars.channel_to_calibrate-SYNC_CHANNEL] & COARSE_MASK) >> COARSE_OFFSET,
                            (app_vars.freq_setting_tx[app_vars.channel_to_calibrate-SYNC_CHANNEL] &    MID_MASK) >>    MID_OFFSET,
                            (app_vars.freq_setting_tx[app_vars.channel_to_calibrate-SYNC_CHANNEL] &   FINE_MASK) >>   FINE_OFFSET
                            
                        );
                    } else {
                        printf("tx=%d no sample found??\r\n", app_vars.channel_to_calibrate);
                    }
                } else {
                    printf("trying to calibrate tx freq_setting when de-sync!!\r\n");
                }
            } else {
                
                switch(app_vars.state){
                    case S_DATA_SEND:
                    case S_LISTEN_FOR_ACK:
                    case S_RECEIVING_ACK:
                        gpio_7_toggle();
                    break;
                    case S_IDLE:
                        app_vars.current_freq_setting += SWEEP_STEP_TX;
                        LC_FREQCHANGE(
                            (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                            (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                            (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                        );
                        app_vars.pkt_len   = TARGET_PKT_LEN;
                        app_vars.packet[0] = app_vars.currentSlotOffset << 4;
                        app_vars.packet[1] = MAGIC_BYTE;
                        radio_loadPacket(app_vars.packet, app_vars.pkt_len);
                        radio_txEnable();
                        delay();
                        radio_txNow();
                        
                        app_vars.state = S_DATA_SEND;
                    break;
                    default:
                        printf("Unexpected state at subslot: state=%d\r\n", app_vars.state);
                    break;
                }
            }
        break;
        case T_RX:
            
            // determine the start_frequency according to slotoffset
        
            if (app_vars.isSync){
                if (app_vars.currentSlotOffset==0){
                    start_frequency = SYNC_FREQ_START_RX;
                } else {
                    start_frequency = app_vars.freq_setting_rx[app_vars.currentSlotOffset-1];
                }
            } else {
                start_frequency = SYNC_FREQ_START_RX;
            }
            
            // check if 2 coarse freq_sweep is done
            
            if (app_vars.current_freq_setting>start_frequency+FREQ_RANGE_RX){
                
                if (app_vars.isSync) {
                    
                    // freq_sweep is done, calculate the freq_setting
                    
                    radio_rfOff();
                    
                    rftimer_disable_interrupts();
                    rftimer_set_callback(cb_slot_timer);
                    rftimer_setCompareIn(app_vars.slotReference + SLOT_DURATION);
                    
                    gpio_5_toggle();
                    
                    // check if there is freq_setting sample recorded
                    
                    if (app_vars.freq_setting_sample[0]!=0){
                        
                        // found at least one setting sample
                        
                        app_vars.freq_setting_rx[app_vars.currentSlotOffset] = \
                            find_freq_settings(
                                app_vars.freq_setting_sample, 
                                NUM_SAMPLES,
                                CONTINOUS_NUM_SAMPLES_RX
                            );
                        
                        
                        // reset 
                        app_vars.sample_index = 0;
                        memset(
                            app_vars.freq_setting_sample,
                            0,
                            sizeof(app_vars.freq_setting_sample)
                        );
                        
                        
                        // check whether all rx freq_settings are recorded
                        app_vars.freq_setting_rx_done = true;
                        for (i=0;i<SLOTFRAME_LEN;i++){
                            if (app_vars.freq_setting_rx[i]==0){
                                app_vars.freq_setting_rx_done = false;
                                break;
                            }
                        }
                        
                        printf("rx%d c=%d m=%d f=%d\r\n",
                            app_vars.currentSlotOffset+SYNC_CHANNEL, 
                            (app_vars.freq_setting_rx[app_vars.currentSlotOffset] & COARSE_MASK) >> COARSE_OFFSET,
                            (app_vars.freq_setting_rx[app_vars.currentSlotOffset] &    MID_MASK) >>    MID_OFFSET,
                            (app_vars.freq_setting_rx[app_vars.currentSlotOffset] &   FINE_MASK) >>   FINE_OFFSET
                            
                        );
                    } else {
                        printf("rx_%d, no sample found??\r\n", app_vars.currentSlotOffset+SYNC_CHANNEL);
                    }
                    
                } else {
                    
                    // not sync'ed
                    
                    // continous sweeping the frequency
                    
                    app_vars.current_freq_setting = SYNC_FREQ_START_RX;
                }
            } else {
                
                switch(app_vars.state){
                    case S_IDLE:
                    case S_LISTEN_FOR_DATA:
                        app_vars.current_freq_setting += SWEEP_STEP_RX;
                        LC_FREQCHANGE(
                            (app_vars.current_freq_setting & COARSE_MASK) >> COARSE_OFFSET,
                            (app_vars.current_freq_setting &    MID_MASK) >>    MID_OFFSET,
                            (app_vars.current_freq_setting &   FINE_MASK) >>   FINE_OFFSET
                        );
                        radio_rxEnable();
                        radio_rxNow();
                        
                        app_vars.state = S_LISTEN_FOR_DATA;
                    break;
                    case S_RECEIVING_DATA:
                        gpio_7_toggle();
                    break;
                    default:
                        // something wrong
                        printf("wrong state in calc_process!\r\n");
                    break;
                }
            }
        break;
        default:
            // wrong type
        break;
    }
}

void cb_timeout_error(void){
    
    gpio_5_toggle();
    
    // timeout happens
    
    rftimer_disable_interrupts();
    
    // turn off radio
    radio_rfOff();
    
    switch(app_vars.state){
        case S_RECEIVING_ACK:
        case S_LISTEN_FOR_ACK:
            // calibrate for tx channel
            if (app_vars.currentSlotOffset==0){
                printf("Timeout for waiting ack on slot 0!\r\n");
            } else {
                if (
                    (
                        app_vars.channel_to_calibrate==26 && 
                        app_vars.freq_setting_tx[15]==0
                    ) || 
                    (
                        app_vars.channel_to_calibrate!=26 && 
                        app_vars.freq_setting_tx[app_vars.currentSlotOffset-1]==0
                    )
                ){
                    // schedule next sub slot
                    rftimer_set_callback(cb_sweep_process_timer);
                    rftimer_setCompareIn(rftimer_readCounter()+SUB_SLOT_DURATION);
                } else {
                    // schedule next slot
                    rftimer_set_callback(cb_slot_timer);
                    rftimer_setCompareIn(app_vars.slotReference+SLOT_DURATION);
                }
            }
            app_vars.type = T_TX;
        break;
        case S_RECEIVING_DATA:
            // calibrate for rx channel
            if (app_vars.freq_setting_rx[app_vars.currentSlotOffset]==0) {
                
                // schedule next sub slot
                rftimer_set_callback(cb_sweep_process_timer);
                rftimer_setCompareIn(rftimer_readCounter()+SUB_SLOT_DURATION);
            } else {
                // schedule next slot
                rftimer_set_callback(cb_slot_timer);
                rftimer_setCompareIn(app_vars.slotReference+SLOT_DURATION);
            }
        break;
        default:
            printf("Unexpected state=%d at timeout ISR!\r\n",app_vars.state);
        break;
    }
    
    // change state
    app_vars.state = S_IDLE;
}
