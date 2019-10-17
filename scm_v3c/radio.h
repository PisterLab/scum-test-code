#ifndef __RADIO_H
#define __RADIO_H

#include <stdint.h>

//=========================== define ==========================================

typedef enum {
   FREQ_TX                        = 0x01,
   FREQ_RX                        = 0x02,
} radio_freq_t;

//=========================== typedef =========================================

typedef void  (*radio_capture_cbt)(uint32_t timestamp);

//=========================== variables =======================================

//=========================== prototypes ======================================

void setFrequencyRX(unsigned int channel);
void setFrequencyTX(unsigned int channel);
void radio_loadPacket(unsigned int len);
void radio_txEnable(void);
void radio_txNow(void);
void radio_rxEnable(void);
void radio_rxNow(void);
void radio_rfOff(void);
void radio_enable_interrupts(void);
void radio_disable_interrupts(void);
void rftimer_enable_interrupts(void);
void rftimer_disable_interrupts(void);
void radio_frequency_housekeeping(void);

void radio_setFrequency(uint8_t frequency, radio_freq_t tx_or_rx);

#endif