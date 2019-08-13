#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
#include "../scm3_hardware_interface.h"
#include "../scm3C_hardware_interface.h"
#include "./adc_config.h"

// Definition of the GPIO mapping
#define GPIO_REG__PGA_AMPLIFY	0x0040
#define GPIO_REG__ADC_CONVERT	0x0020
#define GPIO_REG__ADC_RESET		0x0010

extern unsigned int ADC_DATA_VALID;

void gpio_loopback_control_adc(unsigned int num_samples, unsigned int cycles_reset,
							unsigned int cycles_to_start, unsigned int cycles_pga,
							unsigned int cycles_after) {
	/*
	Inputs:
		num_samples: Integer. Number of readings you'd like to get from the ADC.
		cycles_reset: Number of cycles to keep the resetb pulled low.
		cycles_to_start: Number of cycles after resetting until continuing with the
			rest of the FSM.
		cycles_pga: Number of cycles for the PGA to settle.
		cycles_after: Number of cycles to wait after each conversion for various other
			processes to finish (if necessary).
	Outputs:
		No return value. Uses the Cortex and GPIO loopback to progress the ADC
		FSM and printf's the value read from the ADC output register.
	Notes:
		This assumes that the GPIO settings have already been established. See
		adc_config/gpio_loopback_config_adc(). This reads from the ADC data register,
		not the GPOs.
	*/
	unsigned int count_samples;
	unsigned int count_cycles;
	
	reset_adc(cycles_reset);
	// Wait until the state machine is initially activated by the ISR
	while (ADC_REG__START != 0x1) {};

	for (count_samples=0; count_samples<num_samples; count_samples++) {
		// Wait some time for any potentiometric sensors to converge
		for (count_cycles=0; count_cycles<cycles_to_start; count_cycles++) {};

		// It should have already gone off the first time, but this is for subsequent 
		// conversions
		ADC_REG__START = 0x1;

		// Set the PGA to amplify mode and wait for it to settle
		GPIO_REG__OUTPUT &= ~GPIO_REG__PGA_AMPLIFY;
		for(count_cycles=0; count_cycles<cycles_pga; count_cycles++) {};

		// Set the ADC to convert and wait for the thing to finish
		GPIO_REG__OUTPUT |= GPIO_REG__ADC_CONVERT;
		while (~ADC_DATA_VALID) {};

		// Unset the convert + amplify signals so the FSM progresses
		GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_CONVERT);
		GPIO_REG__OUTPUT |= GPIO_REG__PGA_AMPLIFY;

		reset_adc(cycles_reset);

		ADC_DATA_VALID = 0;
	}
	printf("%d\n", ADC_REG__DATA);

	// Wait some time for other processes to finish
	for (count_cycles=0; count_cycles<cycles_after; count_cycles++) {};
}

void reset_adc(unsigned int cycles_low) {
	/*
	Inputs:
		cycles_low: The number of cycles to hold the reset GPI low.
	Outputs:
		Resets the ADC by strobing adc_reset_gpi low for ``cycles_low'' number
		of cycles and then bringing it high again.
	*/
	int i;

	// Pull reset low
	GPIO_REG__OUTPUT &= ~GPIO_REG__ADC_RESET;

	// Wait a bit
	for (i=0; i<cycles_low; i++);

	// Pull reset high
	GPIO_REG__OUTPUT |= GPIO_REG__ADC_RESET;
}