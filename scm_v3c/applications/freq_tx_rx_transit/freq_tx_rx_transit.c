/**
\brief This program conducts an experiment to test the minimal turn over time to
turn radio from tx to rx or rx to tx.
*/

#include <string.h>

#include "gpio.h"
#include "memory_map.h"
#include "optical.h"
#include "radio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

#define LENGTH_PACKET 125 + LENGTH_CRC  ///< maximum length is 127 bytes
#define LEN_TX_PKT 90 + LENGTH_CRC      ///< length of tx packet
#define CHANNEL 11                      ///< 11=2.405GHz
#define TIMER_PERIOD 3500               ///< 3500 =  7ms@500kHz
#define TIMER_TIMEOUT 2000              ///< 2000 =  4ms@500kHz

#define NUMPKT_PER_CFG 1
#define STEPS_PER_CONFIG 32

typedef enum {
    IDLE = 0,
    RX = 1,
    TIMEOUT = 2,
} state_t;

//=========================== variables =======================================

typedef struct {
    int8_t rxpk_rssi;
    uint8_t rxpk_lqi;

    uint8_t packet[LENGTH_PACKET];
    uint8_t packet_len;
    volatile bool sendDone;
    volatile bool receiveDone;
    state_t state;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_endFrame_tx(uint32_t timestamp);
void cb_endFrame_rx(uint32_t timestamp);
void cb_timer(void);

//=========================== main ============================================

int main(void) {
    uint32_t calc_crc;

    uint8_t cfg_coarse;
    uint8_t cfg_mid;
    uint8_t cfg_fine;

    uint8_t i;
    uint16_t j;
    uint16_t k;
    uint8_t offset;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();

    radio_setEndFrameTxCb(cb_endFrame_tx);
    radio_setEndFrameRxCb(cb_endFrame_rx);
    rftimer_set_callback(cb_timer);

    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();

    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity...");

    calc_crc = crc32c(0x0000, CODE_LENGTH);

    if (calc_crc == CRC_VALUE) {
        printf("CRC OK\r\n");
    } else {
        printf(
            "\r\nProgramming Error - CRC DOES NOT MATCH - Halting "
            "Execution\r\n");
        while (1);
    }

    // Debug output
    // printf("\r\nCode length is %u bytes",code_length);
    // printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);

    // printf("done\r\n");

    // After bootloading the next thing that happens is frequency calibration
    // using optical
    printf("Calibrating frequencies...\r\n");

    // Initial frequency calibration will tune the frequencies for HCLK, the
    // RX/TX chip clocks, and the LO

    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO
    // LDOs by calling radio rxEnable
    radio_rxEnable();

    // Enable optical SFD interrupt for optical calibration
    optical_enable();

    // Wait for optical cal to finish
    while (optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");

    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // configure

    while (1) {
        memset(&app_vars.packet[0], 0, LENGTH_PACKET);

        gpio_1_toggle();

        // loop through all configuration
        //        for (cfg_coarse=23;cfg_coarse<25;cfg_coarse++){
        //            for (cfg_mid=0;cfg_mid<STEPS_PER_CONFIG;cfg_mid++){
        //                for (cfg_fine=0;cfg_fine<STEPS_PER_CONFIG;cfg_fine++){
        //                    printf(
        //                        "coarse=%d, middle=%d, fine=%d\r\n",
        //                        cfg_coarse,cfg_mid,cfg_fine
        //                    );
        for (j = 0; j < 0xffff; j++);
        app_vars.packet[0] = 0;
        app_vars.packet[1] = 0xcf;
        radio_rfOff();

        for (i = 0; i < NUMPKT_PER_CFG; i++) {
            for (k = 0; k < 0x1ff; k++);
            radio_loadPacket(app_vars.packet, LEN_TX_PKT);
            LC_FREQCHANGE(23, 27, 26);
            radio_txEnable();
            app_vars.sendDone = false;
            app_vars.receiveDone = false;
            for (k = 0; k < 0x1ff; k++);
            radio_txNow();
            while (app_vars.sendDone == false);
            while (app_vars.receiveDone == false);
        }
        //                }
        //            }
        //        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void cb_endFrame_rx(uint32_t timestamp) {
    app_vars.receiveDone = true;

    radio_rfOff();

    rftimer_disable_interrupts();

    radio_getReceivedFrame(&(app_vars.packet[0]), &app_vars.packet_len,
                           sizeof(app_vars.packet), &app_vars.rxpk_rssi,
                           &app_vars.rxpk_lqi);

    if (app_vars.packet_len == LEN_TX_PKT && (radio_getCrcOk())) {
        gpio_3_toggle();

        app_vars.packet_len = 0;
        memset(&app_vars.packet[0], 0, LENGTH_PACKET);
    }
}

void cb_endFrame_tx(uint32_t timestamp) {
    radio_rfOff();

    app_vars.sendDone = true;

    app_vars.state = RX;

    rftimer_setCompareIn(rftimer_readCounter() + TIMER_PERIOD);
}

void cb_timer(void) {
    uint16_t k;

    if (app_vars.state == RX) {
        LC_FREQCHANGE(24, 8, 21);
        radio_rxEnable();
        for (k = 0; k < 0x1ff; k++);
        radio_rxNow();

        app_vars.state = TIMEOUT;

        rftimer_setCompareIn(rftimer_readCounter() + TIMER_TIMEOUT);
    } else {
        if (app_vars.state == TIMEOUT) {
            radio_rfOff();

            app_vars.receiveDone = true;

            app_vars.state = IDLE;
        } else {
            printf("something wrong!\r\n");
        }
    }
}
