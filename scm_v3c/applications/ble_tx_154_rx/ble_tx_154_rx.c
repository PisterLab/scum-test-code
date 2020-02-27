/**
\brief This program lets SCuM receive 15.4 packets and broadcast them as a BLE packet.
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

#define LENGTH_PACKET       125 + LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_RX_PKT          20 + LENGTH_CRC  ///< length of rx packet

#define TIMER_PERIOD        200000           ///< 500 = 1ms@500kHz
#define BLE_TX_PERIOD       1

#define BLE_CALIBRATE_LC    false
#define BLE_SWEEP_FINE      false
#define BLE_NUM_REPEAT      1                // Number of times to repeat packet when not sweeping

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

                uint8_t         rx_coarse;
                uint8_t         rx_mid;
                uint8_t         rx_fine;

                uint8_t         tx_coarse;
                uint8_t         tx_mid;
                uint8_t         tx_fine;

                uint16_t        rx_iteration;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);
void     receive_154_packet(void);
void     transmit_ble_packet(void);

//=========================== main ============================================

int main(void) {

    uint32_t calc_crc;

    uint8_t         i;
    uint8_t         j;
    uint8_t         offset;
    int             t;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    ble_init_tx();

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

    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();

#if BLE_CALIBRATE_LC
    optical_enableLCCalibration();

    // Turn on LDOs for calibration
    radio_txEnable();

    // Turn on LO, DIV, PA, and IF
    ANALOG_CFG_REG__10 = 0x78;

    // For TX, LC target freq = 2.402G - 0.25M = 2.40175 GHz.
    optical_setLCTarget(250182);
#endif

    // Enable optical SFD interrupt for optical calibration
    optical_enable();

    // Wait for optical cal to finish
    while(!optical_getCalibrationFinished());

    printf("Cal complete\r\n");

    // Disable static divider to save power
	divProgram(480, 0, 0);

    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // Configure coarse, mid, and fine codes for RX.
    app_vars.rx_coarse = 23;
    app_vars.rx_mid = 15;
    app_vars.rx_fine = 15;

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

    while (1) {
        receive_154_packet();
        rftimer_setCompareIn(rftimer_readCounter() + TIMER_PERIOD);
        app_vars.changeConfig = false;
        while (!app_vars.changeConfig);

        ++app_vars.rx_iteration;
        if (app_vars.rx_iteration % BLE_TX_PERIOD == 0) {
            transmit_ble_packet();
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

    if (
        app_vars.packet_len == LEN_RX_PKT && (radio_getCrcOk())
    ) {
        // Only record IF estimate, LQI, and CDR tau for valid packets
        app_vars.IF_estimate        = radio_getIFestimate();
        app_vars.LQI_chip_errors    = radio_getLQIchipErrors();

        printf(
            "pkt received on ch%d %d%d%d%d.%d.%d.%d\r\n",
            app_vars.packet[4],
            app_vars.packet[0],
            app_vars.packet[1],
            app_vars.packet[2],
            app_vars.packet[3],
            app_vars.rx_coarse,
            app_vars.rx_mid,
            app_vars.rx_fine
        );

        ble_set_data(app_vars.packet);
        ble_set_data_tx_en(true);

        app_vars.packet_len = 0;
        memset(&app_vars.packet[0], 0, LENGTH_PACKET);
    }

    radio_rxEnable();
    radio_rxNow();

    app_vars.rxFrameStarted = false;
}

void    cb_timer(void) {
    app_vars.changeConfig = true;
}

void    receive_154_packet(void) {
    printf("Receiving on %u %u %u\n", app_vars.rx_coarse, app_vars.rx_mid, app_vars.rx_fine);
    while (app_vars.rxFrameStarted);
    radio_rfOff();
    LC_FREQCHANGE(app_vars.rx_coarse, app_vars.rx_mid, app_vars.rx_fine);
    radio_rxEnable();
    radio_rxNow();
}

void    transmit_ble_packet(void) {
    int i, t, tx_fine;

    ble_gen_packet();

#if BLE_SWEEP_FINE
    for (tx_fine = 0; tx_fine < 32; ++tx_fine) {
        LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, tx_fine);
        printf("Transmitting on %u %u %u\n", app_vars.tx_coarse, app_vars.tx_mid, tx_fine);

        // Wait for frequency to settle.
        for (t = 0; t < 5000; ++t);

        ble_transmit();
    }
#else
    for (i = 0; i < BLE_NUM_REPEAT; ++i) {
        LC_FREQCHANGE(app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);
        printf("Transmitting on %u %u %u\n", app_vars.tx_coarse, app_vars.tx_mid, app_vars.tx_fine);

        // Wait for frequency to settle.
        for (t = 0; t < 5000; ++t);

        ble_transmit();
    }
#endif
}
