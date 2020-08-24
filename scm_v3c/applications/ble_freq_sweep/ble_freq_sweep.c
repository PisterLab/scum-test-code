/**
\brief This program lets SCuM transmit BLE packets over a range of 
    frequency settings.
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "ble.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE           (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH         (*((unsigned int *) 0x0000FFF8))

#define CHANNEL             37      // ble channel
#define PKT_LENGTH          64      // pdu length

#define TXPOWER             0xC5    // used for ibeacon pkt

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32
#define TIMER_PERIOD        500  // 500 = 1ms@500kHz

// only this coarse settings are swept, 
// channel 37 and 0 are known within the setting scope of coarse=24
#define CFG_COARSE          24

const static uint8_t ble_device_addr[6] = {
    0xaa, 0xbb, 0xcc, 0xcc, 0xbb, 0xaa
};

//const static uint8_t ble_uuid[16]       = {

//    0xa2, 0x4e, 0x71, 0x12, 0xa0, 0x3f, 
//    0x46, 0x23, 0xbb, 0x56, 0xae, 0x67,
//    0xbd, 0x65, 0x3c, 0x73
//};

const static uint8_t ble_uuid[16]       = {

    0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 
    0x77, 0x88, 0x99, 0xaa, 0xbb, 0xcc,
    0xdd, 0xee, 0xff
};

//=========================== variables =======================================

typedef struct {
    uint8_t         tx_coarse;
    uint8_t         tx_mid;
    uint8_t         tx_fine;
    
    bool            sendDone;
    
    uint8_t         pdu[PDU_LENGTH+CRC_LENGTH]; // protocol data unit
    uint8_t         pdu_len;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void    cb_endFrame_tx(uint32_t timestamp);
void    cb_timer(void);

uint8_t prepare_pdu(void);
uint8_t prepare_freq_setting_pdu(uint8_t coarse, uint8_t mid, uint8_t fine);
void    delay_tx(void);
void    delay_lc_setup(void);

//=========================== main ============================================

int main(void) {

    uint32_t calc_crc;

    uint8_t cfg_coarse;
    uint8_t cfg_mid;
    uint8_t cfg_fine;
    
    uint8_t i;
    uint8_t j;
    uint8_t offset;
    
    uint32_t t;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    ble_init_tx();

    radio_setEndFrameTxCb(cb_endFrame_tx);
    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();

    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity...");

    calc_crc = crc32c(0x0000,CODE_LENGTH);

    if (calc_crc == CRC_VALUE) {
        printf("CRC OK\r\n");
    } else {
        printf("\r\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\r\n");
        while(1);
    }

    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");

    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO

    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();

    // Enable optical SFD interrupt for optical calibration
    optical_enable();

    // Wait for optical cal to finish
    while(!optical_getCalibrationFinished());

    printf("Cal complete\r\n");
    
    // Enable interrupts for the radio FSM (Not working for ble)
    radio_enable_interrupts();

    radio_rfOff();
    
    ble_set_channel(CHANNEL);
    
    while (1) {
        
        // loop through all configuration
        
        // customize coarse, mid, fine values to change the sweeping range
        for (cfg_coarse=24;cfg_coarse<25;cfg_coarse++) {
            for (cfg_mid=6;cfg_mid<10;cfg_mid++) {
                for (cfg_fine=0;cfg_fine<STEPS_PER_CONFIG;cfg_fine++) {
                    
//                    printf(
//                        "coarse=%d, middle=%d, fine=%d\r\n", 
//                        cfg_coarse,cfg_mid,cfg_fine
//                    );
                    
                    for (i=0;i<NUMPKT_PER_CFG;i++) {
                        
                        app_vars.pdu_len = prepare_pdu();
//                        app_vars.pdu_len = prepare_freq_setting_pdu(cfg_coarse, cfg_mid, cfg_fine);
                        ble_prepare_packt(&app_vars.pdu[0], app_vars.pdu_len);
                        
                        LC_FREQCHANGE(cfg_coarse, cfg_mid, cfg_fine);
                        
                        delay_lc_setup();
                        
                        ble_load_tx_arb_fifo();
                        radio_txEnable();
                        
                        delay_tx();
                        
                        ble_txNow_tx_arb_fifo();
                        
                        // need to make sure the tx is done before 
                        // starting a new transmission
                        
                        rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
                        app_vars.sendDone = false;
                        while (app_vars.sendDone==false);
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

//==== callback

void cb_timer(void) {
    app_vars.sendDone = true;
}

void    cb_endFrame_tx(uint32_t timestamp){
    
    printf("this is end of tx \r\n");
    
}

//==== delay

// 0x07ff roughly corresponds to 2.8ms
#define TX_DELAY 0x07ff

void delay_tx(void) {
    uint16_t i;
    for (i=0;i<TX_DELAY;i++);
}

#define LC_SETUP_DELAY 0x02ff

void delay_lc_setup(void) {
    uint16_t i;
    for (i=0;i<LC_SETUP_DELAY;i++);
}

//==== pdu related

uint8_t prepare_freq_setting_pdu(uint8_t coarse, uint8_t mid, uint8_t fine) {
    
    uint8_t i;
    uint8_t j;
    
    uint8_t field_len;
    
    memset(app_vars.pdu, 0, sizeof(app_vars.pdu));
    
    // adv head (to be explained)
    i = 0;
    field_len = 0;
    
    app_vars.pdu[i++] = flipChar(0x42);
    i++;    // skip the length field, fill it at last
    
    // adv address
    
    for (j=6; j>0; j--) {
        app_vars.pdu[i++] = flipChar(ble_device_addr[j-1]);
    }
    
    field_len += 6;
    
    app_vars.pdu[i++] = flipChar(0x04);
    app_vars.pdu[i++] = flipChar(0xC0);
    app_vars.pdu[i++] = flipChar(coarse);
    app_vars.pdu[i++] = flipChar(mid);
    app_vars.pdu[i++] = flipChar(fine);
    
    field_len += 5;
    
    app_vars.pdu[1] = flipChar(field_len);
    
    // the pdu length = field_len plus 2 bytes header
    return (field_len+2);
}

uint8_t prepare_pdu(void) {
    
    uint8_t i;
    uint8_t j;
    
    uint8_t field_len;
    
    memset(app_vars.pdu, 0, sizeof(app_vars.pdu));
    
    // adv head (to be explained)
    i = 0;
    field_len = 0;
    
    app_vars.pdu[i++] = flipChar(0x42);
    i++;    // skip the length field, fill it at last
    
    // adv address
    
    for (j=6; j>0; j--) {
        app_vars.pdu[i++] = flipChar(ble_device_addr[j-1]);
    }
    
    field_len += 6;    
    
    app_vars.pdu[i++] = flipChar(0x1a);
    app_vars.pdu[i++] = flipChar(0xff);
    app_vars.pdu[i++] = flipChar(0x4c);
    app_vars.pdu[i++] = flipChar(0x00);
    
    field_len += 4;
    
    app_vars.pdu[i++] = flipChar(0x02);
    app_vars.pdu[i++] = flipChar(0x15);
    for (j=16; j>0; j--) {
        app_vars.pdu[i++] = flipChar(ble_uuid[j-1]);
    }
    
    // major
    app_vars.pdu[i++] = flipChar(0x00);
    app_vars.pdu[i++] = flipChar(0xff);
    // minor
    app_vars.pdu[i++] = flipChar(0x00);
    app_vars.pdu[i++] = flipChar(0x0f);
    // power level
    app_vars.pdu[i++] = flipChar(TXPOWER);
    
    field_len += 23;
    
    app_vars.pdu[1] = flipChar(field_len);
    
    // the pdu length = field_len plus 2 bytes header
    return (field_len+2);
}
