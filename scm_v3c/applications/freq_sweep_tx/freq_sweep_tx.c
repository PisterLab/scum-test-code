/**
\brief This program conducts a freqency sweeping test.

After loading this program, SCuM will send 10 packet on each configuration
of coarse[5 bits], middle[5 bits] and fine[5 bits]. Meanwhile, an OpenMote
listens on one Channel and print out the freq_offset if it receives a frame.

This program supposes to be run 16 times on for each channel setting on OpenMote
side.
*/

#include <string.h>

#include "memory_map.h"
#include "optical.h"
#include "radio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

#define LENGTH_PACKET 125 + LENGTH_CRC  ///< maximum length is 127 bytes
#define LEN_TX_PKT 20 + LENGTH_CRC      ///< length of tx packet
#define CHANNEL 11                      ///< 11=2.405GHz
#define TIMER_PERIOD 2000               ///< 500 = 1ms@500kHz

#define NUMPKT_PER_CFG 3
#define STEPS_PER_CONFIG 32

//=========================== variables =======================================

static const uint8_t payload_identity[] = "test.";

typedef struct {
    uint8_t packet[LENGTH_PACKET];
    uint8_t packet_len;
    volatile bool sendDone;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void cb_endFrame_tx(uint32_t timestamp);
void cb_timer(void);

//=========================== main ============================================

int main(void) {
    uint32_t calc_crc;

    uint8_t cfg_coarse;
    uint8_t cfg_mid;
    uint8_t cfg_fine;

    uint8_t i;
    uint8_t j;
    uint8_t offset;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();

    radio_setEndFrameTxCb(cb_endFrame_tx);
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
        while (1)
            ;
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
    while (optical_getCalibrationFinshed() == 0)
        ;

    printf("Cal complete\r\n");

    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // configure

    while (1) {
        memcpy(&app_vars.packet[0], &payload_identity[0],
               sizeof(payload_identity) - 1);

        // loop through all configuration
        for (cfg_coarse = 0; cfg_coarse < STEPS_PER_CONFIG; cfg_coarse++) {
            for (cfg_mid = 0; cfg_mid < STEPS_PER_CONFIG; cfg_mid++) {
                for (cfg_fine = 0; cfg_fine < STEPS_PER_CONFIG; cfg_fine++) {
                    //                    printf(
                    //                        "coarse=%d, middle=%d,
                    //                        fine=%d\r\n",
                    //                        cfg_coarse,cfg_mid,cfg_fine
                    //                    );
                    j = sizeof(payload_identity) - 1;
                    app_vars.packet[j++] = '0' + cfg_coarse / 10;
                    app_vars.packet[j++] = '0' + cfg_coarse % 10;
                    app_vars.packet[j++] = '.';
                    app_vars.packet[j++] = '0' + cfg_mid / 10;
                    app_vars.packet[j++] = '0' + cfg_mid % 10;
                    app_vars.packet[j++] = '.';
                    app_vars.packet[j++] = '0' + cfg_fine / 10;
                    app_vars.packet[j++] = '0' + cfg_fine % 10;
                    app_vars.packet[j++] = '.';

                    for (i = 0; i < NUMPKT_PER_CFG; i++) {
                        radio_loadPacket(app_vars.packet, LEN_TX_PKT);
                        LC_FREQCHANGE(cfg_coarse, cfg_mid, cfg_fine);
                        radio_txEnable();
                        rftimer_setCompareIn(rftimer_readCounter() +
                                             TIMER_PERIOD);
                        app_vars.sendDone = false;
                        while (app_vars.sendDone == false)
                            ;
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void cb_endFrame_tx(uint32_t timestamp) {
    radio_rfOff();

    app_vars.sendDone = true;
}

void cb_timer(void) {
    // Tranmit the packet
    radio_txNow();
}
