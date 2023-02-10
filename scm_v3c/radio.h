#ifndef __RADIO_H
#define __RADIO_H

#include <stdbool.h>
#include <stdint.h>

//=========================== define ==========================================

#define LENGTH_CRC 2

//=========================== typedef =======================
typedef enum {
    FREQ_TX = 0x01,
    FREQ_RX = 0x02,
} radio_freq_t;

typedef enum {
    TX_MODE = 0x01,
    RX_MODE = 0x02,
} radio_mode_t;

typedef enum {
    SWEEP = 0x01,
    FIXED = 0x02,
} repeat_mode_t;

typedef struct {
    uint8_t cfg_coarse;
    uint8_t cfg_mid;
    uint8_t cfg_fine;
} repeat_rx_tx_state_t;

typedef void (*radio_capture_cbt)(uint32_t timestamp);
typedef void (*radio_rx_cbt)(uint8_t* packet, uint8_t packet_len);
typedef void (*fill_tx_packet_t)(uint8_t* packet, uint8_t packet_len,
                                 repeat_rx_tx_state_t repeat_rx_tx_state);

typedef struct {
    radio_mode_t radio_mode;
    int16_t packet_count;  // Should be -1 if repeat indefinitely
    repeat_mode_t repeat_mode;
    uint8_t pkt_len;  // Should include extra 2 bytes for CRC
    uint8_t* txPacket;
    fill_tx_packet_t fill_tx_packet;
    uint8_t fixed_lc_coarse;
    uint8_t fixed_lc_mid;
    uint8_t fixed_lc_fine;
    uint8_t sweep_lc_coarse_start;
    uint8_t sweep_lc_mid_start;
    uint8_t sweep_lc_fine_start;
    uint8_t sweep_lc_coarse_end;
    uint8_t sweep_lc_mid_end;
    uint8_t sweep_lc_fine_end;
} repeat_rx_tx_params_t;

//=========================== variables =======================================

//=========================== prototypes ======================================

//==== admin
void send_packet(void* packet, uint8_t pkt_len);
void receive_packet(bool timeout);
void receive_packet_length(uint8_t pkt_len, bool timeout);
void cb_startFrame_tx_radio(uint32_t timestamp);
void cb_endFrame_tx_radio(uint32_t timestamp);
void cb_startFrame_rx_radio(uint32_t timestamp);
void cb_endFrame_rx_radio(uint32_t timestamp);
void default_radio_rx_cb(uint8_t* packet, uint8_t packet_len);
void cb_timer_radio(void);
void radio_setRxCb(radio_rx_cbt radio_rx_cb);
void repeat_rx_tx(repeat_rx_tx_params_t repeat_rx_tx_params);

void radio_init(void);
void radio_setStartFrameTxCb(radio_capture_cbt cb);
void radio_setEndFrameTxCb(radio_capture_cbt cb);
void radio_setStartFrameRxCb(radio_capture_cbt cb);
void radio_setEndFrameRxCb(radio_capture_cbt cb);
void radio_setErrorCb(radio_capture_cbt cb);
void radio_rfOn(void);
void radio_rfOff(void);
void radio_enable_interrupts(void);
void radio_disable_interrupts(void);
void radio_reset(void);

//==== get/set
bool radio_getCrcOk(void);
uint32_t radio_getIFestimate(void);
uint32_t radio_getLQIchipErrors(void);
int16_t radio_get_cdr_tau_value(void);

//==== frequency
void radio_frequency_housekeeping(uint32_t IF_estimate,
                                  uint32_t LQI_chip_errors,
                                  int16_t cdr_tau_value);
void radio_setFrequency(uint8_t frequency, radio_freq_t tx_or_rx);
void radio_build_channel_table(uint32_t channel_11_LC_code);

//==== tx
void radio_loadPacket(void* packet, uint16_t len);
void radio_txEnable(void);
void radio_txNow(void);

//==== rx
void radio_rxEnable(void);
void radio_rxNow(void);
void radio_getReceivedFrame(uint8_t* pBufRead, uint8_t* pLenRead,
                            uint8_t maxBufLen, int8_t* pRssi, uint8_t* pLqi);

//==== interrupts
void radio_isr(void);

#endif
