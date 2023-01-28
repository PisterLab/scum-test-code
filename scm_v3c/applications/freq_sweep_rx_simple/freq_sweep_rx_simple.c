#include <string.h>

#include "optical.h"
#include "radio.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define RX_PACKET_LEN 125 + 2  // 2 for CRC

//=========================== variables =======================================

typedef struct {
    uint8_t dummy;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================
void radio_rx_cb(uint8_t* packet, uint8_t packet_len);

//=========================== main ============================================

int main(void) {
    repeat_rx_tx_params_t repeat_params;

    memset(&app_vars, 0, sizeof(app_vars_t));

    initialize_mote();
    crc_check();
    perform_calibration();

    radio_setRxCb(radio_rx_cb);

    repeat_params.packet_count = -1;
    repeat_params.pkt_len = RX_PACKET_LEN;
    repeat_params.radio_mode = RX_MODE;
    repeat_params.repeat_mode = SWEEP;
    repeat_params.sweep_lc_coarse_start = 20;
    repeat_params.sweep_lc_coarse_end = 25;
    repeat_params.sweep_lc_mid_start = 0;
    repeat_params.sweep_lc_mid_end = 31;
    repeat_params.sweep_lc_fine_start = 0;
    repeat_params.sweep_lc_fine_end = 31;
    repeat_params.fixed_lc_coarse = 24;
    repeat_params.fixed_lc_mid = 21;
    repeat_params.fixed_lc_fine = 3;

    repeat_rx_tx(repeat_params);
}

//=========================== private =========================================
void radio_rx_cb(uint8_t* packet, uint8_t packet_len) {
    uint8_t i;

    // Log the packet
    printf("rx_simple: Received Packet. Contents: ");

    for (i = 0; i < packet_len - LENGTH_CRC; i++) {
        printf("%d ", packet[i]);
    }
    printf("\n");
}
