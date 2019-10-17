#ifndef __RADIO_H
#define __RADIO_H

//=========================== define ==========================================

//=========================== typedef =========================================

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

#endif