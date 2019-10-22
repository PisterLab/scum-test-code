#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
// #include "../Int_Handlers.h"
#include "../scm3_hardware_interface.h"
#include "../scm3C_hardware_interface.h"
#include "./adc_config.h"

/*
2019.
This file carries functions which have self-contained tests for the ADC, i.e.
they don't require any signals from the Teensy or UART to trigger. 

For those which rely on a UART trigger, all that's required is the 
scan settings are set appropriately (see adc_config.c)
*/

// Definition of the GPIO mapping
#define GPIO_REG__PGA_AMPLIFY	0x0004
#define GPIO_REG__ADC_CONVERT	0x0002
#define GPIO_REG__ADC_RESET		0x0001

unsigned short ADC_DATA_VALID;
unsigned short ADC_CONTINUOUS = 0;	// 0 if a continuous run is not occurring. Otherwise, 
									// a continuous run is occurring.
unsigned short ADC_STOP = 0;		// 0 if a continuous run is not being stopped. Otherwise,
									// used to stop continuous ADC conversions.

void reset_adc(unsigned int cycles_low) {
	/*
	Inputs:
		cycles_low: The number of cycles to hold the reset GPI low.
	Outputs:
		Resets the ADC by strobing adc_reset_gpi low for ``cycles_low'' number
		of cycles and then bringing it high again.
	*/
	int i;
	GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_CONVERT);
	GPIO_REG__OUTPUT |= GPIO_REG__PGA_AMPLIFY;

	// Pull reset low
	GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_RESET);

	// Wait a bit
	for (i=0; i<cycles_low; i++) {}

	// Pull reset high
	GPIO_REG__OUTPUT |= GPIO_REG__ADC_RESET;
}

void onchip_control_adc_shot(void) {
	/*
	Inputs:
		No inputs.
	Outputs:
		No return value. Triggers a single ADC reading where the 
		ADC is controlled by the on-chip FSM.
	Notes:
		In hardware, the reset is functionally tied directly to
		the Cortex M0's soft reset. It is guaranteed that the
		comparator will suffer from serious hysteresis and have 
		problems with the FSM.
	*/
	ADC_REG__START = 0x1;
}

void onchip_fix_control_adc_shot(unsigned int cycles_reset) {
	/*
	Inputs:
		No inputs.
	Outputs:
		No return value. Triggers a single ADC reading where the ADC
		reset is controlled via GPIO loopback and all other signals are 
		conrolled via on-chip FSM.
	Notes:
		Untested.
	*/
	// Trigger a standard ADC reading
	int i;
	ADC_DATA_VALID = 0;
	ADC_REG__START = 0x1;

	// Pull reset low, wait, then pull reset high
	GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_RESET);
	for (i=0; i<cycles_reset; i++) {}
	GPIO_REG__OUTPUT |= GPIO_REG__ADC_RESET;
}

void loopback_control_adc_shot(unsigned int cycles_reset,
							unsigned int cycles_to_start, unsigned int cycles_pga){
	/*
	Inputs:
		cycles_reset: Number of cycles to keep the resetb pulled low.
		cycles_to_start: Number of cycles after resetting until continuing with the
			rest of the FSM.
		cycles_pga: Number of cycles for the PGA to settle.
	Outputs:
		No return value. Uses the Cortex and GPIO loopback to progress the ADC
		FSM and printf's the value read from the ADC output register.
	Notes:
		This assumes that the GPIO settings have already been established. See
		adc_config/gpio_loopback_config_adc(). This printf's from the ADC data register,
		not the GPOs.
	*/
	unsigned int count_cycles;

	// Unset the convert + amplify signals + reset the ADC
	reset_adc(cycles_reset);

	ADC_DATA_VALID = 0;
	ADC_REG__START = 0x1;

	// Wait some time for any potentiometric sensors to converge
	for (count_cycles=0; count_cycles<cycles_to_start; count_cycles++) {}

	// Set the PGA to amplify mode and wait for it to settle
	GPIO_REG__OUTPUT &= ~GPIO_REG__PGA_AMPLIFY;
	for(count_cycles=0; count_cycles<cycles_pga; count_cycles++) {}

	// Set the ADC to convert and wait for the thing to finish
	// The ISR will print out the ADC reading value
	GPIO_REG__OUTPUT |= GPIO_REG__ADC_CONVERT;

	// while (~ADC_DATA_VALID) {
	// 	for (count_cycles=0;count_cycles<2;count_cycles++) {}
	// }

	// Unset the convert + amplify signals + reset the ADC
	// reset_adc(cycles_reset);

}

void onchip_control_adc_continuous(void) {
	/*
	Inputs:
		No inputs.
	Outputs:
		No return value. Repeatedly triggers ADC readings where the ADC is 
		controlled by the on-chip FSM. Halts when ADC_STOP is nonzero.
	Notes:
		Untested. Known issue where including the loop for while(~ADC_DATA_VALID)
		prevents the ADC ISR from executing. Removing it and replacing it 
		with a counter loop allows the interrupt to occur normally.
	*/
	// Flagging that nonstop conversions have started
	ADC_CONTINUOUS = 1;

	// Keep going until the flag is raised
	while (~ADC_STOP) {
		ADC_DATA_VALID = 0;
		onchip_control_adc_shot();
		while (~ADC_DATA_VALID) {};
	}

	// Reset start and stop flags 
	ADC_STOP = 0;
	ADC_CONTINUOUS = 0;
}

void loopback_control_adc_continuous(unsigned int cycles_reset,
							unsigned int cycles_to_start, unsigned int cycles_pga) {
	/*
	Inputs:
		cycles_reset: Number of cycles to keep the resetb pulled low.
		cycles_to_start: Number of cycles after resetting until continuing with the
			rest of the FSM.
		cycles_pga: Number of cycles for the PGA to settle.
	Outputs:
		No return value. Uses the Cortex and GPIO loopback to progress the ADC
		FSM and printf's the value read from the ADC output register.
	Notes:
		This assumes that the GPIO settings have already been established. See
		adc_config/gpio_loopback_config_adc(). This printf's from the ADC data register,
		not the GPOs.
	Notes:
		Untested.
	*/
	ADC_CONTINUOUS = 1;
	while (~ADC_STOP) {
		ADC_DATA_VALID = 0;
		loopback_control_adc_shot(cycles_reset, cycles_to_start, cycles_pga);
	}
	ADC_STOP = 0;
	ADC_CONTINUOUS = 0;
}

void halt_adc_continuous(void) {
	/*
	Inputs:
		No inputs.
	Outputs:
		No return value. If a continuous ADC run has been started, halt it.
		Otherwise, do nothing.
	Notes:
		Untested.
	*/
	if (ADC_CONTINUOUS) {ADC_STOP = 1;}
	ADC_DATA_VALID = 0;
	return;
}
