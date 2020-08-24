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

#define TIMER_PERIOD        500000             ///< 500 = 1ms@500kHz

#define BLE_CALIBRATE_LC    true
#define BLE_SWEEP_FINE      true
    
#define CHANNEL             37
#define PKT_LENGTH          64

#define TXPOWER             0xD5

const static uint8_t ble_device_addr[6] = { 
    0xaa, 0xbb, 0xcc, 0xcc, 0xbb, 0xaa
};

const static uint8_t ble_uuid[16]       = {

    0xa2, 0x4e, 0x71, 0x12, 0xa0, 0x3f, 
    0x46, 0x23, 0xbb, 0x56, 0xae, 0x67,
    0xbd, 0x65, 0x3c, 0x73
};

//=========================== variables =======================================

typedef struct {
    uint8_t         tx_coarse;
    uint8_t         tx_mid;
    uint8_t         tx_fine;
    
    bool            txNext;
    
    uint8_t         packet[PDU_LENGTH+CRC_LENGTH];
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void    cb_timer(void);
void    transmit_ble_packet(void);
void    prepare_packet(void);
void    prepare_ibeacon_packet(void);

//=========================== main ============================================

int main(void) {

    uint32_t calc_crc;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    ble_init_tx();

    rftimer_set_callback(cb_timer);

    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();

    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity...");

    calc_crc = crc32c(0x0000,CODE_LENGTH);

    if (calc_crc == CRC_VALUE){
        printf("CRC OK\r\n");
    } else {
        printf("\r\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\r\n");
        while(1);
    }

    // Debug output
    // printf("\r\nCode length is %u bytes",code_length);
    // printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);

    // printf("done\r\n");

    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");

    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO

#if BLE_CALIBRATE_LC
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
    
#endif

    // Enable optical SFD interrupt for optical calibration
    optical_enable();

    // Wait for optical cal to finish
    while(!optical_getCalibrationFinished());

    printf("Cal complete\r\n");

    // Disable static divider to save power
    divProgram(480, 0, 0);

    // Configure coarse, mid, and fine codes for TX.
#if BLE_CALIBRATE_LC
    app_vars.tx_coarse  = optical_getLCCoarse();
    app_vars.tx_mid     = optical_getLCMid();
    app_vars.tx_fine    = optical_getLCFine();
#else
    // CHANGE THESE VALUES AFTER LC CALIBRATION.
    app_vars.tx_coarse  = 24;
    app_vars.tx_mid     = 8;
    app_vars.tx_fine    = 15;
#endif

    ble_set_channel(CHANNEL);
    prepare_ibeacon_packet();
//    prepare_packet();
    
    ble_load_packt(&app_vars.packet[0]);

    while (1) {
        transmit_ble_packet();
        rftimer_setCompareIn(rftimer_readCounter() + TIMER_PERIOD);
        app_vars.txNext = false;
        while (!app_vars.txNext);
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void cb_timer(void) {
    app_vars.txNext = true;
}

void prepare_packet(void) {
    
    uint8_t i;
    
    memset(app_vars.packet, 0, sizeof(app_vars.packet));
    
    // adv head (to be explained)
    i = 0;
    app_vars.packet[i++] = flipChar(0x02);
    app_vars.packet[i++] = flipChar(0x25);
    
    // adv address: 0x00, 0x02, 0x72, 0x32, 0x80, 0xC6
    
    app_vars.packet[i++] = flipChar(0xC6);
    app_vars.packet[i++] = flipChar(0x80);
    app_vars.packet[i++] = flipChar(0x32);
    app_vars.packet[i++] = flipChar(0x72);
    app_vars.packet[i++] = flipChar(0x02);
    app_vars.packet[i++] = flipChar(0x00);
    
    // name: 6 bytes long: 1 byte gap code 0x10, 5 bytes data
    
    app_vars.packet[i++] = flipChar(0x06);
    app_vars.packet[i++] = flipChar(0x08);
    app_vars.packet[i++] = flipChar('S');
    app_vars.packet[i++] = flipChar('C');
    app_vars.packet[i++] = flipChar('U');
    app_vars.packet[i++] = flipChar('M');
    app_vars.packet[i++] = flipChar('3');
}

void prepare_ibeacon_packet(void) {
    
    uint8_t i;
    uint8_t j;
    
    memset(app_vars.packet, 0, sizeof(app_vars.packet));
    
    // adv head (to be explained)
    i = 0;
    app_vars.packet[i++] = flipChar(0x42);
    app_vars.packet[i++] = flipChar(0x21);
    
    // adv address: 0x00, 0x02, 0x72, 0x32, 0x80, 0xC6
    
    for (j=6; j>0; j--) {
        app_vars.packet[i++] = flipChar(ble_device_addr[j-1]);
    }
    
    app_vars.packet[i++] = flipChar(0x1a);
    app_vars.packet[i++] = flipChar(0xff);
    app_vars.packet[i++] = flipChar(0x4c);
    app_vars.packet[i++] = flipChar(0x00);
    
    app_vars.packet[i++] = flipChar(0x02);
    app_vars.packet[i++] = flipChar(0x15);
    for (j=16; j>0; j--) {
        app_vars.packet[i++] = flipChar(ble_uuid[j-1]);
    }
    
    // major
    app_vars.packet[i++] = flipChar(0x00);
    app_vars.packet[i++] = flipChar(0xff);
    // minor
    app_vars.packet[i++] = flipChar(0x00);
    app_vars.packet[i++] = flipChar(0x0f);
    // power level
    app_vars.packet[i++] = flipChar(TXPOWER);
}

void transmit_ble_packet(void) {
    int t, tx_fine;

#if BLE_SWEEP_FINE
    for (tx_fine = 0; tx_fine < 32; ++tx_fine) {
        LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, tx_fine);
        printf("Transmitting on %u %u %u\r\n", app_vars.tx_coarse, app_vars.tx_mid, tx_fine);

        // Wait for frequency to settle.
        for (t = 0; t < 5000; ++t);

        ble_transmit();
    }
#else
    LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);
    printf("Transmitting on %u %u %u\r\n", app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);

    // Wait for frequency to settle.
    for (t = 0; t < 5000; ++t);

    ble_transmit();
#endif
}
