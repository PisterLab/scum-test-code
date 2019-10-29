#ifndef __RFTIMER_H
#define __RFTIMER_H

#include <stdint.h>

//=========================== define ==========================================

#define RFTIMER_MAX_COUNT   0xffffffff

//=========================== typedef =========================================

typedef void  (*rftimer_cbt)(void);

//=========================== variables =======================================

//=========================== prototypes ======================================

void     rftimer_init(void);
void     rftimer_setCompareIn(uint32_t val);
void     rftimer_set_callback(rftimer_cbt cb);
uint32_t rftimer_readCounter(void);
void     rftimer_enable_interrupts(void);
void     rftimer_disable_interrupts(void);

void     rftimer_isr(void);

#endif