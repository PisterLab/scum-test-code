#ifndef __RFTIMER_H
#define __RFTIMER_H

#include <stdbool.h>
#include <stdint.h>

//=========================== define ==========================================

#define RFTIMER_MAX_COUNT 0xffffffff

//=========================== typedef =========================================

typedef void (*rftimer_cbt)(void);

//=========================== variables =======================================

//=========================== prototypes ======================================

void rftimer_init(void);
void rftimer_setCompareIn(uint32_t val);
void rftimer_setCompareIn_by_id(uint32_t val, uint8_t id);
void rftimer_set_callback(rftimer_cbt cb);
void rftimer_set_callback_by_id(rftimer_cbt cb, uint8_t id);
uint32_t rftimer_readCounter(void);
void rftimer_enable_interrupts(void);
void rftimer_enable_interrupts_by_id(uint8_t id);
void rftimer_disable_interrupts(void);
void rftimer_disable_interrupts_by_id(uint8_t id);
void rftimer_clear_interrupts(void);
void rftimer_clear_interrupts_by_id(uint8_t id);
void rftimer_set_repeat(bool should_repeat, uint8_t id);
void delay_milliseconds_asynchronous(unsigned int delay_milli, uint8_t id);
void delay_milliseconds_synchronous(unsigned int delay_milli, uint8_t id);

void rftimer_isr(void);

#endif
