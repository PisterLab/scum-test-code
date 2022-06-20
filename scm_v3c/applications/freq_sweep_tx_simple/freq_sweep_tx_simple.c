#include <string.h>

#include "scm3c_hw_interface.h"
#include "optical.h"
#include "radio.h"

//=========================== defines =========================================

#define TX_PACKET_LEN 8+2 // 2 for CRC

//=========================== variables =======================================

typedef struct {
    uint8_t dummy;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================
void fill_tx_packet(uint8_t *packet, uint8_t packet_len, repeat_rx_tx_state_t state);

//=========================== main ============================================

int main(void) {
    repeat_rx_tx_params_t repeat_params;
    uint8_t txPacket[TX_PACKET_LEN];
    
    memset(&app_vars,0,sizeof(app_vars_t));

    initialize_mote();
    crc_check();
    perform_calibration();
  
    repeat_params.packet_count = -1;
    repeat_params.pkt_len = TX_PACKET_LEN;
    repeat_params.radio_mode = TX_MODE;
    repeat_params.repeat_mode = SWEEP;
    repeat_params.fill_tx_packet = fill_tx_packet;
    repeat_params.sweep_lc_coarse_start = 18;
    repeat_params.sweep_lc_coarse_end = 26;
    repeat_params.sweep_lc_mid_start = 0;
    repeat_params.sweep_lc_mid_end = 31;
    repeat_params.sweep_lc_fine_start = 0;
    repeat_params.sweep_lc_fine_end = 31;
    repeat_params.fixed_lc_coarse = 24;
    repeat_params.fixed_lc_mid = 5;
    repeat_params.fixed_lc_fine = 9;
    
    repeat_rx_tx(repeat_params);
}

//=========================== private =========================================
void fill_tx_packet(uint8_t *packet, uint8_t packet_len, repeat_rx_tx_state_t state) {
    sprintf(packet, "%d %d %d", state.cfg_coarse, state.cfg_mid, state.cfg_fine);
}
