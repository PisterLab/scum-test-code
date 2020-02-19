/**
\brief This program lets SCuM transmit BLE packets.
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "ble.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE           (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH         (*((unsigned int *) 0x0000FFF8))

#define TIMER_PERIOD        50000             ///< 500 = 1ms@500kHz

#define BLE_CALIBRATE_LC    false
#define BLE_SWEEP_FINE      true

//=========================== variables =======================================

typedef struct {
                uint8_t         tx_coarse;
                uint8_t         tx_mid;
                uint8_t         tx_fine;

                bool            txNext;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_timer(void);

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

    //printf("done\r\n");

    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");

    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO

#if BLE_CALIBRATE_LC
    optical_enableLCCalibration();

    // Turn on LO, DIV, PA, and IF
    ANALOG_CFG_REG__10 = 0x78;

    // Turn off polyphase and disable mixer
    ANALOG_CFG_REG__16 = 0x6;

    // For TX, LC target freq = 2.402G - 0.25M = 2.40175 GHz.
    optical_setLCTarget(250020);
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
    app_vars.tx_coarse = optical_getLCCoarse();
    app_vars.tx_mid = optical_getLCMid();
    app_vars.tx_fine = optical_getLCFine();
#else
    // CHANGE THESE VALUES AFTER LC CALIBRATION.
    app_vars.tx_coarse = 23;
    app_vars.tx_mid = 11;
    app_vars.tx_fine = 23;
#endif

    ble_gen_packet();

    while (1) {
        rftimer_setCompareIn(rftimer_readCounter() + TIMER_PERIOD);
        app_vars.txNext = false;
        while (!app_vars.txNext);
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_timer(void) {
    int tx_fine, t;
    app_vars.txNext = true;

#if BLE_SWEEP_FINE
    for (tx_fine = 0; tx_fine < 32; ++tx_fine) {
        LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, tx_fine);
        printf("Transmitting on %u %u %u\n", app_vars.tx_coarse, app_vars.tx_mid, tx_fine);

        // Wait for frequency to settle.
        for (t = 0; t < 5000; ++t);

        ble_transmit();
    }
#else
    LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);
    printf("Transmitting on %u %u %u\n", app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);

    // Wait for frequency to settle.
    for (t = 0; t < 5000; ++t);

    ble_transmit();
#endif
}
