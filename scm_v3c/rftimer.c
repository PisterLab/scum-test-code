#include <string.h>
#include <stdio.h>
#include "memory_map.h"
#include "scm3c_hw_interface.h"
#include "radio.h"
#include "rftimer.h"
#include "gpio.h"

// ========================== definition ======================================

#define MINIMUM_COMPAREVALE_ADVANCE     5
#define LARGEST_INTERVAL                0xffff

// ========================== variable ========================================

typedef struct {
    rftimer_cbt      rftimer_cb;
    rftimer_cbt      rftimer_action_cb;
    uint32_t         last_compare_value;
    uint8_t          noNeedClearFlag;
} rftimer_vars_t;

rftimer_vars_t rftimer_vars;

// ========================== prototype =======================================

// ========================== public ==========================================

void rftimer_init(void){
    
    memset(&rftimer_vars, 0, sizeof(rftimer_vars_t));
    
    // set period of radiotimer
    RFTIMER_REG__MAX_COUNT          = RFTIMER_MAX_COUNT;
    // enable timer and interrupt
    RFTIMER_REG__CONTROL            = 0x07;
}

void rftimer_set_callback(rftimer_cbt cb){
    rftimer_vars.rftimer_cb = cb;
}

void rftimer_setCompareIn(uint32_t val){
    
    rftimer_enable_interrupts();
    
    // A timer scheudled in the past
    
    //Note: this is a hack!!
    //  since the timer counter overflow after hitting the MAX_COUNT,
    //      theoritically, there is no way to find a timer is scheduled 
    //      in the past or in the future. However, if we know the LARGEST_INTERVAL
    //      between each two adjacent timers:
    //      if the val - current count < largest interval:
    //          it's a timer scheduled in the future
    //      else:
    //          it's a timer scheduled in the past
    //          manually trigger an interrupt
    //      LARGEST_INTERVAL is application defined value.
    
    if ((val & RFTIMER_MAX_COUNT)-RFTIMER_REG__COUNTER<LARGEST_INTERVAL){
        
    } else {
        // seems doesn't work?
        RFTIMER_REG__INT = 0x0001;
    }
    
    RFTIMER_REG__COMPARE0           = val & RFTIMER_MAX_COUNT;
}

uint32_t rftimer_readCounter(void){
    return RFTIMER_REG__COUNTER;
}

void rftimer_enable_interrupts(void){
    
    // clear pending bit first
    rftimer_clear_interrupts();
    
    // enable compare interrupt (this also cancels any pending interrupts)
    RFTIMER_REG__COMPARE0_CONTROL   = RFTIMER_COMPARE_ENABLE |   \
                                      RFTIMER_COMPARE_INTERRUPT_ENABLE;
    ISER = 0x80;
}

void rftimer_disable_interrupts(void){
    // disable compare interrupt
    RFTIMER_REG__COMPARE0_CONTROL = 0x0000;
    ICER = 0x80;
}

void rftimer_clear_interrupts(void){
    RFTIMER_REG__INT_CLEAR = 0x0001;
}

// ========================== interrupt =======================================

void rftimer_isr(void) {
    
    uint16_t interrupt;
        
    gpio_2_set();
    
    interrupt = RFTIMER_REG__INT;
    
    if (interrupt & 0x00000001){
#ifdef ENABLE_PRINTF
        printf("COMPARE0 MATCH\r\n");
#endif
        if (rftimer_vars.rftimer_cb!=NULL) {
            rftimer_vars.rftimer_cb();
        }
    }
    
    if (interrupt & 0x00000002){
#ifdef ENABLE_PRINTF
        printf("COMPARE1 MATCH\r\n");
#endif
    }
    
    if (interrupt & 0x00000004){
#ifdef ENABLE_PRINTF
        printf("COMPARE2 MATCH\r\n");
#endif
    }
    
    // Watchdog has expired - no packet received
    if (interrupt & 0x00000008){
#ifdef ENABLE_PRINTF
        printf("COMPARE3 MATCH\r\n");
#endif
    }
    
    // Turn on transmitter to allow frequency to settle
    if (interrupt & 0x00000010){
#ifdef ENABLE_PRINTF
        printf("COMPARE4 MATCH\r\n");
#endif
    }
    
    // Transmit now
    if (interrupt & 0x00000020){
#ifdef ENABLE_PRINTF
        printf("COMPARE5 MATCH\r\n");
#endif
    }
    
    if (interrupt & 0x00000040) {
#ifdef ENABLE_PRINTF
        printf("COMPARE6 MATCH\r\n");
#endif
    }
    
    if (interrupt & 0x00000080) {
#ifdef ENABLE_PRINTF
        printf("COMPARE7 MATCH\r\n");
#endif
    }
    
    if (interrupt & 0x00000100) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE0 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE0);
#endif
    }
    
    if (interrupt & 0x00000200) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE1 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE1);
#endif
    }
    
    if (interrupt & 0x00000400) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE2 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE2);
#endif
    }
    
    if (interrupt & 0x00000800) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE3 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE3);
#endif
    }
    
    if (interrupt & 0x00001000) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE0 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE0);
#endif
    }
    
    if (interrupt & 0x00002000) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE1 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE1);
#endif
    }
    
    if (interrupt & 0x00004000) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE2 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE2);
#endif
    }
    
    if (interrupt & 0x00008000) {
#ifdef ENABLE_PRINTF
        printf("CAPTURE3 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE3);
#endif
    }
    
    RFTIMER_REG__INT_CLEAR = interrupt;
    
    gpio_2_clr();
}