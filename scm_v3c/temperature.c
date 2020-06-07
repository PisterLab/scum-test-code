#include "temperature.h"
#include "scm3c_hw_interface.h"
#include "Memory_map.h"

// VARIABLES
double temp = 20; // current temperature. Will be set after interrupt triggered
uint8_t temp_iteration = 0;
uint8_t num_temp_iterations = 5;
uint32_t cumulative_count_2M = 0;
uint32_t cumulative_count_32k = 0;

// FUNCTIONS
void measure_temperature() {
	// RFCONTROLLER_REG__INT_CONFIG = 0x3FF;   // Enable all interrupts and pulses to radio timer
	// RFCONTROLLER_REG__ERROR_CONFIG = 0x1F;  // Enable all errors
	
	temp_iteration = 0;
	cumulative_count_2M = 0;
	cumulative_count_32k = 0;
	
	// Enable RF timer interrupt
	ISER = 0x0080;

	RFTIMER_REG__CONTROL = 0x7;
	RFTIMER_REG__MAX_COUNT = 0x0000C350;
	RFTIMER_REG__COMPARE1 = 0x0000C350;
	RFTIMER_REG__COMPARE1_CONTROL = 0x03;

	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;

	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
}

void measure_2M_32k_counters() {		
	// Enable RF timer interrupt
	ISER = 0x0080;

	RFTIMER_REG__CONTROL = 0x7;
	RFTIMER_REG__MAX_COUNT = 0x0000C350;
	RFTIMER_REG__COMPARE2 = 0x0000C350;
	RFTIMER_REG__COMPARE2_CONTROL = 0x03;

	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;

	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;
}