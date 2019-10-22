#ifndef __RADIO_H
#define __RADIO_H

#include <stdbool.h>
#include <stdint.h>

//=========================== define ==========================================

#define LENGTH_CRC      2

typedef enum {
   FREQ_TX                        = 0x01,
   FREQ_RX                        = 0x02,
} radio_freq_t;

//=========================== typedef =========================================

typedef void  (*radio_capture_cbt)(uint32_t timestamp);

//=========================== variables =======================================

//=========================== prototypes ======================================

//==== admin
void radio_init(void);
void radio_setStartFrameTxCb(radio_capture_cbt cb);
void radio_setEndFrameTxCb(radio_capture_cbt cb);
void radio_setStartFrameRxCb(radio_capture_cbt cb);
void radio_setEndFrameRxCb(radio_capture_cbt cb);
void radio_setErrorCb(radio_capture_cbt cb);
void radio_rfOff(void);
void radio_enable_interrupts(void);
void radio_disable_interrupts(void);

//==== get/set
bool        radio_getCrcOk(void);
uint32_t    radio_getIFestimate(void);
uint32_t    radio_getLQIchipErrors(void);
int16_t     radio_get_cdr_tau_value(void);

//==== frequency
void radio_frequency_housekeeping(
    uint32_t IF_estimate,
    uint32_t LQI_chip_errors,
    int16_t cdr_tau_value
);
void radio_setFrequency(uint8_t frequency, radio_freq_t tx_or_rx);
void radio_build_channel_table(uint32_t channel_11_LC_code);

//==== tx
void radio_loadPacket(uint8_t* packet, uint16_t len);
void radio_txEnable(void);
void radio_txNow(void);

//==== rx
void radio_rxEnable(void);
void radio_rxNow(void);
void radio_getReceivedFrame(
    uint8_t* pBufRead,
    uint8_t* pLenRead,
    uint8_t  maxBufLen,
     int8_t* pRssi,
    uint8_t* pLqi
);

//==== interrupts
void radio_isr(void);

#endif