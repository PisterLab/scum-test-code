#include "temperature.h"
#include "scm3c_hw_interface.h"
#include "Memory_map.h"
#include "rftimer.h"

extern unsigned int count_2M;
extern unsigned int count_32k;

/* Performs a temperature measurement (in this case will set count_2M and count_32k
 * variables defined in freq_sweep_rx_tx.c). Is performed asynchronously.
 */
void measure_2M_32k_counters(unsigned int measurement_time_milliseconds) {
	reset_counters();
	enable_counters();
	
	delay_milliseconds_synchronous(measurement_time_milliseconds);
	
	// This function leverages the fact that the counters are reset upon calling the delay function
	// so we know by the point the delay is done our count will make sense based on when the
	// call occurred.
	read_count_2M_32K(&count_2M, &count_32k);
}

//void measure_2M_32k_counters() {		
//	// Enable RF timer interrupt
//	ISER = 0x0080;

//	RFTIMER_REG__CONTROL = 0x7;
//	RFTIMER_REG__MAX_COUNT = 0x0000C350;
//	RFTIMER_REG__COMPARE2 = 0x0000C350;
//	RFTIMER_REG__COMPARE2_CONTROL = 0x03;

//	// Reset all counters
//	ANALOG_CFG_REG__0 = 0x0000;

//	// Enable all counters
//	ANALOG_CFG_REG__0 = 0x3FFF;
//}