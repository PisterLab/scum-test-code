#include "temperature.h"
#include "scm3c_hw_interface.h"
#include "Memory_map.h"
#include "rftimer.h"

void measure_temperature(rftimer_cbt callback, unsigned int measurement_time_milliseconds) {
	// RF TIMER is derived from HF timer (20MHz) through a divide ratio of 40, thus it is 500kHz.
	// the count defined by RFTIMER_REG__MAX_COUNT and RFTIMER_REG__COMPARE1 indicate how many
	// counts the timer should go through before triggering an interrupt. This is the basis for
	// the following calculation. For example a count of 0x0000C350 corresponds to 100ms.
	unsigned int rf_timer_count = (measurement_time_milliseconds * 500000) / 1000;
		
	// RFCONTROLLER_REG__INT_CONFIG = 0x3FF;   // Enable all interrupts and pulses to radio timer
	// RFCONTROLLER_REG__ERROR_CONFIG = 0x1F;  // Enable all errors

  rftimer_set_callback(callback);
		
	// Enable RF timer interrupt
	ISER = 0x0080;

	RFTIMER_REG__CONTROL = 0x7;
	RFTIMER_REG__MAX_COUNT = rf_timer_count;
	RFTIMER_REG__COMPARE1 = rf_timer_count;
	RFTIMER_REG__COMPARE1_CONTROL = 0x03;

	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;

	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
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