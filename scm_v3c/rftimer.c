#include <string.h>
#include <stdio.h>
#include "memory_map.h"
#include "scm3c_hw_interface.h"
#include "radio.h"
#include "rftimer.h"

// ========================== definition ======================================

#define MINIMUM_COMPAREVALE_ADVANCE  5

// ========================== variable ========================================

typedef struct {
    rftimer_cbt      rftimer_cb;
    rftimer_cbt      rftimer_action_cb;
    uint32_t         last_compare_value;
    uint8_t          noNeedClearFlag;
} rftimer_vars_t;

rftimer_vars_t rftimer_vars;

// todo: this is a very hacky solution. Need to find a better way to do this
extern unsigned int count_2M;
extern unsigned int count_32k;

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
    
    RFTIMER_REG__COMPARE0           = val & RFTIMER_MAX_COUNT;
}

uint32_t rftimer_readCounter(void){
    return RFTIMER_REG__COUNTER;
}

void rftimer_enable_interrupts(void){
    // enable compare interrupt (this also cancels any pending interrupts)
    RFTIMER_REG__COMPARE0_CONTROL   = RFTIMER_COMPARE_ENABLE |   \
                                      RFTIMER_COMPARE_INTERRUPT_ENABLE;
    ISER = 0x80;
}

void rftimer_disable_interrupts(void){
    RFTIMER_REG__COMPARE0_CONTROL = 0x0;
    ICER = 0x80;
}

/* Delays the chip for a period of time in milliseconds based off the rate
 * of the 500kHz RF TIMER. Internally, this function uses RFTIMER COMPARE 7.
 *
 * @param callback - the callback to call after the delay has completed
 * @param delay_milli - the delay in milliseconds
*/
void delay_milliseconds(rftimer_cbt callback, unsigned int delay_milli) {
	// RF TIMER is derived from HF timer (20MHz) through a divide ratio of 40, thus it is 500kHz.
	// the count defined by RFTIMER_REG__MAX_COUNT and RFTIMER_REG__COMPARE1 indicate how many
	// counts the timer should go through before triggering an interrupt. This is the basis for
	// the following calculation. For example a count of 0x0000C350 corresponds to 100ms.
	unsigned int rf_timer_count = delay_milli * 500; // same as (delay_milli * 500000) / 1000;

  rftimer_set_callback(callback);
	rftimer_enable_interrupts();

	RFTIMER_REG__MAX_COUNT = rf_timer_count;
	RFTIMER_REG__COMPARE7 = rf_timer_count;
	RFTIMER_REG__COMPARE7_CONTROL = 0x03;

	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;

	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
}

// ========================== interrupt =======================================

void rftimer_isr(void) {
    
    unsigned int interrupt = RFTIMER_REG__INT;
    
    if (interrupt & 0x00000001){
#ifdef ENABLE_PRINTF
        printf("COMPARE0 MATCH\r\n");
#endif
    }
    
    if (interrupt & 0x00000002){
#ifdef ENABLE_PRINTF
        printf("COMPARE1 MATCH\r\n");
#endif			
			// This interrupt is used for measuring temperature.
			// We are just going to read in the counts of the 2MHz and the 32kHz
			// clocks which will be used for making the temperature estimate.
			read_count_2M_32K(&count_2M, &count_32k);
    }
    
    if (interrupt & 0x00000004){
#ifdef ENABLE_PRINTF
        printf("COMPARE2 MATCH\r\n");
#endif
			
			printf("do 2M and 32K count measurement here!\n");
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
			// this interrupt is used by the delay_milliseconds function. We are going to do nothing
			// for this one since we are just delaying
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
		
		// disable the interrupt (I believes this make using the RF timer a "one time interrupt" rather than a
		// repeating interrupt that keeps happening at an interval, but not sure)
		ICER = 0x0080;
    
    RFTIMER_REG__INT_CLEAR = interrupt;

    if (rftimer_vars.rftimer_cb != NULL) {
        rftimer_vars.rftimer_cb();
    }
}