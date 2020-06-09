/**
\brief This program conducts continuously calibration over temperature changes.

After loading this program, SCuM will do the following:
- sweep frequency settings to get the RX setting
- sweep frequency settings to get the TX setting
- using the TX and RX setting to transmit frame to OpenMote and listen for ACK

Temperature will change during the last phase.
This program allows SCuM to continuously calibrate its frequency according to
the frequency offset for TX in ACK and the IF counts for RX.

This calibration only applies on on signel channel, e.g. channel 11.

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

#define RX_TIMEOUT          500 // 500 = 1ms@500kHz

#define SWEEP_START         ((23<<10) | (31<<5) | (31<<5))
#define SWEEP_END           ((24<<10) | (31<<5) | (31<<5))

#define SETTING_SIZE        100
#define BEACON_PERIOD       10      // seconds
#define SECOND_IN_TICKS     500000  // @500kHz

#define MAX_PKT_SIZE        127
#define TARGET_PKT_SIZE     5
#define FREQ_OFFSET_THRESHOLD   5   // target frequency offset is 2

//=========================== variables =======================================

typedef enum {
    SWEEP_RX        = 0,
    SWEEP_RX_DONE   = 1,
    SWEEP_TX        = 2
} state_t;

typedef struct {
    
    // setting for tx
    uint16_t tx_setting_target;
    uint16_t tx_settings_list[SETTING_SIZE];
      int8_t tx_settings_freq_offset_list[SETTING_SIZE];
    uint16_t tx_list_index;
    
    // setting for rx
    uint16_t rx_setting_target;
    uint16_t rx_settings_list[SETTING_SIZE];
    uint16_t rx_list_index;
    
    uint16_t current_setting;
    
    // stats
    state_t state;
    uint8_t rx_done;
    uint8_t tx_done;
    
    uint8_t beacon_stops_in;    // in seconds
    
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void    cb_timer(void);
void    cb_startFrame_tx(uint32_t timestamp);
void    cb_endFrame_tx(uint32_t timestamp);
void    cb_startFrame_rx(uint32_t timestamp);
void    cb_endFrame_rx(uint32_t timestamp);

void    getFrequencyTx(uint16_t setting_start, uint16_t setting_end);
void    getFrequencyRx(uint16_t setting_start, uint16_t setting_end);
void    contiuously_calibration_start(void);

//=========================== main ============================================

int main(void) {
    
    uint32_t calc_crc;
    
    uint8_t         i;
    uint8_t         j;
    uint8_t         offset;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    app_vars.beacon_stops_in = BEACON_PERIOD;
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();

    rftimer_set_callback(cb_timer);
    radio_setStartFrameTxCb(cb_startFrame_tx);
    radio_setEndFrameTxCb(cb_endFrame_tx);
    radio_setStartFrameRxCb(cb_startFrame_rx);
    radio_setEndFrameRxCb(cb_endFrame_rx);
    
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

    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();
    
    // Enable optical SFD interrupt for optical calibration
    optical_enable();
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    app_vars.state = SWEEP_RX;

    while(1){
        
        // obtain frequency setting for RX
        getFrequencyRx(SWEEP_START, SWEEP_END);
        
        // obtain frequency setting for TX
        getFrequencyTx(SWEEP_START, SWEEP_END);
        
        // start contiuously calibration
        contiuously_calibration_start();
    }
}

//=========================== public ==========================================

//=========================== callback ========================================

void    cb_timer(void) {
    
    switch(app_vars.state){
        case SWEEP_RX:
            app_vars.rx_done = 1;
        break;
        case SWEEP_RX_DONE:
            app_vars.state = SWEEP_TX;
        break;
        case SWEEP_TX:
            app_vars.rx_done = 1;   // rx for ack
        break;
    }
}

void    cb_startFrame_rx(uint32_t timestamp) {
    
}

void    cb_endFrame_rx(uint32_t timestamp) {
    
    uint8_t pkt[MAX_PKT_SIZE];
    uint8_t pkt_len;
     int8_t rssi;
    uint8_t lqi;
    
    uint16_t temperature;
    
    // disable timeout interrupt
    rftimer_disable_interrupts();
    
    radio_getReceivedFrame(
        &(pkt[0]),
        &pkt_len,
        sizeof(pkt),
        &rssi,
        &lqi
    );
        
    if (radio_getCrcOk() && pkt_len == TARGET_PKT_SIZE) {
        temperature = (pkt[0] << 8) | (pkt[1]);
        
        if (app_vars.state == SWEEP_RX) {
        
            app_vars.rx_settings_list[app_vars.rx_list_index++] = \
                app_vars.current_setting;
            app_vars.beacon_stops_in = BEACON_PERIOD - pkt[2];
        } else {
            
            app_vars.tx_settings_list[app_vars.tx_list_index] = \
                app_vars.current_setting;
            app_vars.tx_settings_freq_offset_list[app_vars.tx_list_index] = \
                (int8_t)(pkt[2]);
            app_vars.tx_list_index++;
        }
    }
    
    app_vars.rx_done = 1;
}

void    cb_startFrame_tx(uint32_t timestamp) {
    
}

void    cb_endFrame_tx(uint32_t timestamp) {
    
    app_vars.tx_done = 1;
}

//=========================== helper ==========================================

void delay(void) {
    uint16_t i;
    for (i=0;i<0x1fff;i++);
}

void    getFrequencyRx(
    uint16_t setting_start, 
    uint16_t setting_end
) {
    
    uint8_t i;
    
    // make sure we are at SWEEP_RX state
    
    while (app_vars.state != SWEEP_RX);
    
    // sweep settings to find the ones for RX
    
    for (
        app_vars.current_setting=setting_start; 
        app_vars.current_setting<setting_end; 
        app_vars.current_setting++
    ) {
        radio_rfOff();
        
        delay();
        
        LC_FREQCHANGE(
            (app_vars.current_setting>>10) & 0x001F,
            (app_vars.current_setting>>5)  & 0x001F,
            (app_vars.current_setting)     & 0x001F
        );
        radio_rxEnable();
        radio_rxNow();
        rftimer_setCompareIn(rftimer_readCounter()+RX_TIMEOUT);
        while(app_vars.rx_done == 0);
    }
    
    // update state and schedule next state
    
    app_vars.state = SWEEP_RX_DONE;
    rftimer_setCompareIn(
        rftimer_readCounter()+app_vars.beacon_stops_in*SECOND_IN_TICKS
    );
    
    // choose the median setting in the rx_settings_list as 
    //      target rx frequency setting
    
    i = 0;
    while (app_vars.rx_settings_list[i] != 0) {
        i++;
    }
    app_vars.rx_setting_target = app_vars.rx_settings_list[i/2];
}

void    getFrequencyTx(
    uint16_t setting_start, 
    uint16_t setting_end
) {
    
    uint8_t i;
     int8_t diff;
    uint8_t pkt[TARGET_PKT_SIZE];
    
    while (app_vars.state != SWEEP_TX);
    
    // sweep settings to find the ones for TX
    
    for (
        app_vars.current_setting=setting_start; 
        app_vars.current_setting<setting_end; 
        app_vars.current_setting++
    ) {
        
        // transmit probe frame
        
        radio_rfOff();
        radio_loadPacket(pkt, TARGET_PKT_SIZE);
        
        delay();
        
        LC_FREQCHANGE(
            (app_vars.current_setting>>10) & 0x001F,
            (app_vars.current_setting>>5)  & 0x001F,
            (app_vars.current_setting)     & 0x001F
        );
        radio_txEnable();
        radio_txNow();
        while(app_vars.tx_done == 0);
        
        // listen for ack
        
        radio_rfOff();
        
        delay();
        
        LC_FREQCHANGE(
            (app_vars.rx_setting_target>>10) & 0x001F,
            (app_vars.rx_setting_target>>5)  & 0x001F,
            (app_vars.rx_setting_target)     & 0x001F
        );
        
        radio_rxEnable();
        radio_rxNow();
        rftimer_setCompareIn(rftimer_readCounter()+RX_TIMEOUT);
        while(app_vars.rx_done == 0);
    }
    
    // choose the first setting in the tx_settings_list within the threshold
    //      as the target tx frequency setting
    
    i = 0;
    diff = (int8_t)(app_vars.tx_settings_freq_offset_list[i]-2);
    while (
        diff >=  FREQ_OFFSET_THRESHOLD ||
        diff <= -FREQ_OFFSET_THRESHOLD
    ) {
        i++;
    }
    app_vars.tx_setting_target = app_vars.tx_settings_list[i];
}

void    contiuously_calibration_start(void) {
    
}

