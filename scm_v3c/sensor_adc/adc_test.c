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

extern unsigned int ADC_DATA_VALID;

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
	printf("About to pull reset low...%X\n",GPIO_REG__OUTPUT);
	// Pull reset low
	GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_RESET);
	printf("Reset low %X\n", GPIO_REG__OUTPUT);
	// Wait a bit
	for (i=0; i<cycles_low; i++) {}

	// Pull reset high
	GPIO_REG__OUTPUT |= GPIO_REG__ADC_RESET;
}

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

	// for (count_samples=0; count_samples<num_samples; count_samples++) {
	while (1) {
		// It should have already gone off the first time, but this is for subsequent 
		// conversions
		// printf("Starting\n");
		ADC_REG__START = 0x1;

		// printf("Resetting ADC...%X\n", GPIO_REG__OUTPUT);
		// Unset the convert + amplify signals
		// Reset the ADC
		reset_adc(cycles_reset);
		// printf("ADC reset %X\n", GPIO_REG__OUTPUT);

		ADC_DATA_VALID = 0;

		// Wait some time for any potentiometric sensors to converge
		// printf("Waiting some cycles to start...\n");
		for (count_cycles=0; count_cycles<cycles_to_start; count_cycles++) {}


		// Set the PGA to amplify mode and wait for it to settle
		GPIO_REG__OUTPUT &= ~GPIO_REG__PGA_AMPLIFY;
		// printf("Waiting some cycles for the PGA to settle %X\n",GPIO_REG__OUTPUT);
		for(count_cycles=0; count_cycles<cycles_pga; count_cycles++) {}

		// Set the ADC to convert and wait for the thing to finish
		// The ISR will print out the ADC reading value
		GPIO_REG__OUTPUT |= GPIO_REG__ADC_CONVERT;
		// printf("Waiting on the ADC data to be valid %X\n", GPIO_REG__OUTPUT);

		for (count_cycles=0; count_cycles<1000; count_cycles++) {}
		printf("%d\n",ADC_REG__DATA);
		// while (~ADC_DATA_VALID) {}
		
	}

	// Wait some time for other processes to finish
	for (count_cycles=0; count_cycles<cycles_after; count_cycles++) {}
}

