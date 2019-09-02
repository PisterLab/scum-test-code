#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
#include "../scm3C_hardware_interface.h"

/*
2019.
Function suffixes:
	_init: Anything that goes in scm3C_hardware_interface.c/initialize_mote()
	_main: Anything that goes in main.c/main()
*/

void gpo_toggle_init(void){
	/*
	Inputs:
		None.
	Outputs:
		No return value. Enables all the GPO buffers, disables all the GPI 
		buffers, and sets all the GPOs to bank 6 (CORTEX_GPO).
	Notes:
		Untested. This should be run before gpo_toggle_main().
	*/
	// Enable all output buffers
	GPO_enables(0xFFFF);

	// Disable all input buffers
	GPI_enables(0x0000);

	// Set output buffer bank to CORTEX_GPO
	GPO_control(6,6,6,6);
}

void gpo_toggle_main(void){
	/*
	Inputs:
		No input values.
	Outputs:
		No return value. Checks GPO buffer functionality by pulling all GPOs
		high and low ad infinitum. This will require a probe of the hardware.
	Notes:
		Untested. gpo_toggle_init() should be run in scm3c_hardware_interface/
		initialize_mote() before calling this function.
	*/
	unsigned int i;
	while (1) {
		// Write all 1 to the GPOs
		GPIO_REG__OUTPUT = 0xFFFF;

		// Wait a lil bit
		for(i=0; i<1; i++) {}

		// Write all 0 to the GPOs
		GPIO_REG__OUTPUT = 0x0000;
	}
}

void gpio_loopback_init(void){
	/*
	Inputs:
		None.
	Outputs:
		No return value. Enables all the GPO and GPI buffers and sets GPIOs to 
		be cortex write and cortex read for GPIO loopback. This does not 
		require a probe of the hardware.
	Notes:
		Untested. This should be run before gpio_loopback_main().
	*/
	// Enable all buffers
	GPO_enables(0xFFFF);
	GPI_enables(0xFFFF);

	// Set banks for loopback
	GPO_control(6,6,6,6);
	GPI_control(0,0,0,0);
}


unsigned char gpio_loopback_main(void){
	/*
	Inputs:
		None.
	Outputs:
		Checks
		- GPO buffer functionality by incrementing GPO<15:0> up to 2^16 - 1.
		- GPI buffer functionality by reading GPI values from the memory-
			mapped register and comparing it against the written GPO value. 
			If it isn't the same, printf's an error message and returns 0. 
			Otherwise, printf's an "ok" message and returns 1.
	Notes:
		Untested. gpio_loopback_init() should be run in 
		scm3c_hardware_interface/initialize_mote() before calling this 
		function. UART connection is necessary for full utility.
	*/
	unsigned int i;
	unsigned int j;
	for(i=0; i<0xFFFF; i++) {
		// Write the value to the GPOs
		GPIO_REG__OUTPUT = i;

		// Wait a little bit
		for (j=0; j<1; j++) {}

		// Read from the GPIs
		if (GPIO_REG__INPUT != i) {
			printf("FAIL gpio_loopback_main: GPO %X != GPI %X\n", i, GPIO_REG__INPUT);
			return 0;
		}
	}
	printf("PASS gpio_loopback_main\n");
	return 1;
}
