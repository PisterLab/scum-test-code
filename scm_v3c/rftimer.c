#include <string.h>
#include <stdio.h>
#include "memory_map.h"
#include "scm3c_hw_interface.h"
#include "radio.h"
#include "rftimer.h"
#include <stdbool.h>

// ========================== definition ======================================

#define MINIMUM_COMPAREVALE_ADVANCE  5
#define NUM_INTERRUPTS 8


// ========================== variable ========================================

rftimer_vars_t rftimer_vars;

bool delay_completed[NUM_INTERRUPTS]; // flag indicating whether the delay has completed. For use by delay_milliseoncds_synchronous method
bool is_repeating[NUM_INTERRUPTS]; // flag indicating whethere each COMPARE will repeat at a fixed rate
unsigned int timer_durations[NUM_INTERRUPTS]; // indicates length each COMPARE interrupt was set to run for. Used for repeating delay.

unsigned int* RF_TIMER_REG_ADDRESSES[] = {RFTIMER_REG__COMPARE0_ADDR,
																					RFTIMER_REG__COMPARE1_ADDR,
																					RFTIMER_REG__COMPARE2_ADDR,
																					RFTIMER_REG__COMPARE3_ADDR,
																					RFTIMER_REG__COMPARE4_ADDR,
																					RFTIMER_REG__COMPARE5_ADDR,
																					RFTIMER_REG__COMPARE6_ADDR,
																					RFTIMER_REG__COMPARE7_ADDR};

unsigned int* RF_TIMER_REG_CONTROL_ADDRESES[] ={RFTIMER_REG__COMPARE0_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE1_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE2_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE3_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE4_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE5_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE6_CONTROL_ADDR,
																								RFTIMER_REG__COMPARE7_CONTROL_ADDR};

// ========================== prototype =======================================

// ========================== public ==========================================

void rftimer_init(void){
    
    memset(&rftimer_vars, 0, sizeof(rftimer_vars_t));
    
    // set period of radiotimer
    RFTIMER_REG__MAX_COUNT          = RFTIMER_MAX_COUNT;
    // enable timer and interrupt
    RFTIMER_REG__CONTROL            = 0x07;
}

void rftimer_set_callback(rftimer_cbt cb, uint8_t id) {
    rftimer_vars.rftimer_cbs[id] = cb;
}

void rftimer_setCompareIn(uint32_t val, uint8_t id){
    rftimer_enable_interrupts(id);
	
		*RF_TIMER_REG_ADDRESSES[id] = val & RFTIMER_MAX_COUNT;
    
    //RFTIMER_REG__COMPARE0           = val & RFTIMER_MAX_COUNT;
}

uint32_t rftimer_readCounter(void){
    return RFTIMER_REG__COUNTER;
}

void rftimer_enable_interrupts(uint8_t id){
    // enable compare interrupt (this also cancels any pending interrupts)
    *RF_TIMER_REG_CONTROL_ADDRESES[id]   = RFTIMER_COMPARE_ENABLE |   \
																					 RFTIMER_COMPARE_INTERRUPT_ENABLE;
    ISER = 0x80;
}

void rftimer_disable_interrupts(uint8_t id){
    *RF_TIMER_REG_CONTROL_ADDRESES[id] = 0x0;
    ICER = 0x80;
}

// Sets a flag indicating whether to make the desired interrupt repeat right after finishing
void rftimer_set_repeat(bool should_repeat, uint8_t id) {
	is_repeating[id] = should_repeat;
}

/* Delays the chip for a period of time in milliseconds based off the rate
 * of the 500kHz RF TIMER. Internally, this function uses RFTIMER COMPARE 7.
 * This is an asynchronous delay, so the program can continue executation and
 * eventually the interrupt will be called indicating the end of the delay.
 * You will need to set callback if desired.
 *
 * @param delay_milli - the delay in milliseconds
*/
void delay_milliseconds_asynchronous(unsigned int delay_milli, uint8_t id) {
	// RF TIMER is derived from HF timer (20MHz) through a divide ratio of 40, thus it is 500kHz.
	// the count defined by RFTIMER_REG__MAX_COUNT and RFTIMER_REG__COMPARE1 indicate how many
	// counts the timer should go through before triggering an interrupt. This is the basis for
	// the following calculation. For example a count of 0x0000C350 corresponds to 100ms.
	unsigned int rf_timer_count = delay_milli * 500; // same as (delay_milli * 500000) / 1000;
	rftimer_enable_interrupts(id);
	timer_durations[id] = delay_milli;
	
	rftimer_setCompareIn(rftimer_readCounter()+rf_timer_count, id);
}

/* Performs a delay that will not return until the delay has completed.
 * For additional documentation see the delay_milliseconds function.
 */
void delay_milliseconds_synchronous(unsigned int delay_milli, uint8_t id) {
	delay_completed[id] = false;
	
	delay_milliseconds_asynchronous(delay_milli, id);
	
	// do nothing until delay has finished
	while (delay_completed[id] == false) {}
}

// ========================== interrupt =======================================

void rftimer_isr(void) {
    
	unsigned int interrupt = RFTIMER_REG__INT;
	int i = 0;
	
	int interrupt_id = 1;
	for (i = 0; i < 8; i++) {
		if (interrupt & interrupt_id) {
			#ifdef ENABLE_PRINTF
				printf("COMPARE%d MATCH\r\n", i);
			#endif
			
			handle_interrupt(i);
		}
		
		interrupt_id = interrupt_id << 1;
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
	
	// Austin: disable the interrupt (I believes this make using the RF timer a "one time interrupt" rather than a
	// repeating interrupt that keeps happening at an interval, but not sure)
	//ICER = 0x0080; // COMMENTED THIS OUT DURING RFTIMER CODE UPDATE ALLOWING USE OF ALL TIMERS, BUT MIGHT BE NEEDED!!!!
	
	RFTIMER_REG__INT_CLEAR = interrupt;
}

void handle_interrupt(uint8_t id) {
	delay_completed[id] = true; // used for delay synchronous function
	
	if (is_repeating[id]) {
		delay_milliseconds_asynchronous(timer_durations[id], id);
	}
	
	if (rftimer_vars.rftimer_cbs[id] != NULL) {
		rftimer_vars.rftimer_cbs[id]();
  } else {
		printf("interrupt %d called, but had no callback defined.\n", id);
	}
}