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

#define CHANNEL             0       // ble channel

#define TXPOWER             0xC5    // used for ibeacon pkt

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32
#define TIMER_PERIOD        5000  // 500 = 1ms@500kHz

// only this coarse settings are swept, 
// channel 37 and 0 are known within the setting scope of coarse=24
#define CFG_COARSE          24

const static uint8_t ble_device_addr[6] = {
    0xaa, 0xbb, 0xcc, 0xcc, 0xbb, 0xaa
};

const static uint8_t ble_uuid[16]       = {

    0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf,
    0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf, 0xcf
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

uint8_t prepare_pdu_nordic_aoa_beacon(void);
uint8_t prepare_pdu_ibeacon(void);
uint8_t prepare_pdu_cte_inline(void);
uint8_t prepare_freq_setting_pdu(uint8_t coarse, uint8_t mid, uint8_t fine);
void    delay_tx(void);
void    delay_lc_setup(void);

//=========================== main ============================================

int main(void) {

    uint32_t calc_crc;

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
    
    optical_enableLCCalibration();

    // Turn on LO, DIV, PA, and IF
    ANALOG_CFG_REG__10 = 0x78;

    // Turn off polyphase and disable mixer
    ANALOG_CFG_REG__16 = 0x6;

#if CHANNEL==37
    // For TX, LC target freq = 2.402G - 0.25M = 2.40175 GHz.
    optical_setLCTarget(250182);
#elif CHANNEL==0
    
    // For TX, LC target freq = 2.404G - 0.25M = 2.40375 GHz.
    optical_setLCTarget(250390);
#endif

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
        for (cfg_mid=8;cfg_mid<11;cfg_mid++) {
            for (cfg_fine=0;cfg_fine<STEPS_PER_CONFIG;cfg_fine+=2) {
                
                printf(
                    "coarse=%d, middle=%d, fine=%d\r\n", 
                    CFG_COARSE,cfg_mid,cfg_fine
                );
                
                for (i=0;i<NUMPKT_PER_CFG;i++) {
                    
                    radio_rfOff();
                    
                    app_vars.pdu_len = prepare_freq_setting_pdu(CFG_COARSE, cfg_mid, cfg_fine);
                    ble_prepare_packt(&app_vars.pdu[0], app_vars.pdu_len);
                    
                    LC_FREQCHANGE(CFG_COARSE, cfg_mid, cfg_fine);
                    
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
    
    app_vars.pdu[i++] = flipChar(0x20);
    app_vars.pdu[i++] = flipChar(0x03);
    app_vars.pdu[i++] = flipChar(coarse);
    app_vars.pdu[i++] = flipChar(mid);
    app_vars.pdu[i++] = flipChar(fine);
    
    field_len += 5;
    
    return field_len;
}

uint8_t prepare_pdu_cte_inline(void) {
    
    uint8_t i;
    uint8_t j;
    
    uint8_t field_len;
    
    memset(app_vars.pdu, 0, sizeof(app_vars.pdu));
    
    // adv head (to be explained)
    i = 0;
    field_len = 0;
    
    app_vars.pdu[i++] = flipChar(0x20);
    app_vars.pdu[i++] = flipChar(0x02);
    
    app_vars.pdu[i++] = flipChar(0x03);
    app_vars.pdu[i++] = flipChar(0xdd);
    app_vars.pdu[i++] = flipChar(0xff);
    
    field_len += 3;
    
    // the pdu length = field_len plus 2 bytes header
    return (field_len+2);
}

uint8_t prepare_pdu_ibeacon(void) {
    
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

uint8_t prepare_pdu_nordic_aoa_beacon(void){

    uint8_t i;
    uint8_t field_len;
    
    memset(app_vars.pdu, 0, sizeof(app_vars.pdu));
    
    i         = 0;
    
    app_vars.pdu[i++] = flipChar(0x46);
    app_vars.pdu[i++] = flipChar(0x06);
    field_len = 6;
    app_vars.pdu[i++] = flipChar(0x01);
    app_vars.pdu[i++] = flipChar(0x02);
    app_vars.pdu[i++] = flipChar(0x03);
    app_vars.pdu[i++] = flipChar(0x04);
    app_vars.pdu[i++] = flipChar(0x05);
    app_vars.pdu[i++] = flipChar(0xc6);
    
    // the pdu length = field_len plus 2 bytes header
    return (field_len+2);
}
