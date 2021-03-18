#ifndef __RFTIMER_H
#define __RFTIMER_H

#include <stdint.h>
#include <stdbool.h>

//=========================== define ==========================================

#define RFTIMER_MAX_COUNT   0xffffffff

//=========================== typedef =========================================

typedef void  (*rftimer_cbt)(void);

typedef struct {
    rftimer_cbt      rftimer_cbs[8]; // one callback for each COMPARE value (0-7)
    rftimer_cbt      rftimer_action_cb;
    uint32_t         last_compare_value;
    uint8_t          noNeedClearFlag;
} rftimer_vars_t;

//=========================== variables =======================================



//=========================== prototypes ======================================

void     rftimer_init(void);
void     rftimer_setCompareIn(uint32_t val, uint8_t id);
void		 rftimer_set_callback(rftimer_cbt cb, uint8_t id);
uint32_t rftimer_readCounter(void);
void     rftimer_enable_interrupts(uint8_t id);
void     rftimer_disable_interrupts(uint8_t id);
void		 rftimer_set_repeat(bool should_repeat, uint8_t id);
void		 delay_milliseconds_asynchronous(unsigned int delay_milli, uint8_t id);
void		 delay_milliseconds_synchronous(unsigned int delay_milli, uint8_t id);

void     rftimer_isr(void);
void		 handle_interrupt(uint8_t id);

#endif