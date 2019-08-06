#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
#include "../scm3_hardware_interface.h"
#include "../scm3C_hardware_interface.h"

void prog_asc_bit(unsigned int position, unsigned int val){
	/*
	Inputs:
		position: Unsigned integer. The position in the ASC to change.
		val: Unsigned integer. Nonzero if the value should be set to 1, 
			zero if it should be set to 0.
	Outputs:
		No return value. Sets the value in the ASC with the proper 
		masking, etc. given the input boolean value.
	*/
	// unsigned int index = position >> 5;
	if (val != 0) {
		set_asc_bit(position);
	}
	else {
		clear_asc_bit(position);
	}
}


void scan_config_adc(unsigned int sel_reset, unsigned int sel_convert, 
				unsigned int sel_pga_amplify,
				unsigned int pga_gain[], unsigned int adc_settle[], 
				unsigned int bgr_tune[], unsigned int constgm_tune[], 
				unsigned int vbatDiv4_en, unsigned int ldo_en,
				unsigned int input_mux_sel[], unsigned int pga_byp) {
	/*
	Inputs: 
		sel_reset: 0 or 1. Chooses source for ADC reset signal. 
			0 for digital FSM, 1 from GPI.
		sel_convert: 0 or 1. Chooses source for ADC convert signal. 
			0 for digital FSM, 1 from GPI.
		sel_pga_amplify: 0 or 1. Chooses source for PGA amplify signal. 
			0 for digital FSM, 1 from GPI.
		pga_gain: 8-bit binary [0,255]. Gain setting of the PGA.
		adc_settle: 8-bit binary [0,255]. Settle time of the ADC (exact
			relationship unknown).
		bgr_tune: 7-bit binary [0,127]. Value for adjusting the band gap 
			reference. The LSB (RHS) is the panic bit.
		constgm_tune: 8-bit binary [0,255]. Value for adjusting the constant
			gm device.
		vbatDiv4_en: Integer 0 or 1. 1 enables the vbat divide-by-4, 0
			disables.
		ldo_en: Integer 0 or 1. 1 enables the on-chip LDO, 0 disables.
		input_mux_sel: 2-bit binary. Choice of input to the PGA/ADC.
			00: VPTAT
			01: VBAT/4
			10: External pad
			11: Floating
		pga_byp: 0 or 1. 1 bypasses the PGA.
	Outputs:
		None. Configures the scan chain for testing the sensor ADC.
	Notes:
		Everything is MSB -> LSB. Inversions, flips, etc. have been
		handled internally.
	*/
	int i;
	int start_idx;

	// Selecting where the reset comes from
	prog_asc_bit(242, sel_reset);

	// Selecting where the convert signal comes from
	prog_asc_bit(243, sel_convert);
	
	// Selecting where the amplify signal comes from
	prog_asc_bit(244, sel_pga_amplify);

	// PGA gain bits
	start_idx = 766;
	for (i=0; i<6; i++) {
		prog_asc_bit(start_idx+i, pga_gain[i]);
	}
	prog_asc_bit(800, pga_gain[6]);
	prog_asc_bit(773, pga_gain[7]);

	// ADC settling bits
	start_idx = 816;
	for (i=0; i<8; i++) {
		prog_asc_bit(i+start_idx, adc_settle[i]);
	}

	// LDO BGR tuning
	prog_asc_bit(778, bgr_tune[0]);
	start_idx = 784;
	for (i=1; i<7; i++) {
		prog_asc_bit(start_idx, bgr_tune[i]);
		start_idx--;
	}

	// Constant gm tuning
	start_idx = 765;
	for (i=0; i<8; i++) {
		prog_asc_bit(start_idx-i, constgm_tune[i]);
	}

	// Enabling/disabling VBAT/4
	prog_asc_bit(798, vbatDiv4_en);

	// Enabling/disabling on-chip LDO
	prog_asc_bit(801, ldo_en);

	// Input mux selection programming
	prog_asc_bit(1087, input_mux_sel[1]);
	prog_asc_bit(915, input_mux_sel[0]);

	// PGA bypass
	prog_asc_bit(1088, pga_byp);
}

void gpio_loopback_config_adc(void) {
	/*
	Inputs:
		No inputs.
	Outputs:
		No return value. Enables I/O buffers for the GPIOs and sets the 
		appropriate banks for the GPIOs. Does not disable any buffers 
		from their initial setting.

		Untested.
	*/
	unsigned int gpo_mask = get_GPO_enables();
	unsigned int gpi_mask = get_GPI_enables();
	
	gpo_mask |= 0x07FF;
	gpi_mask |= 0x07FF;
	
	GPO_enables(gpo_mask);
	GPI_enables(gpi_mask);

	GPO_control(6,6,6,6);
	GPI_control(0,0,0,0);
}

void gpio_config_adc(unsigned int gpi_control, unsigned int gpo_read) {
	/*
	Inputs:
		gpi_control: 0 or 1. True = control the FSM via GPI
			rather than relying on the taped-out FSM.
		gpo_read: 0 or 1. True = use GPOs to output the sensor
			ADC output bits and the ``done'' signal.
	Outputs:
		None. Enables the necessary GPIO buffers and sets the 
		banks appropriately for the GPIOs to control the FSM via GPI
		and/or read the ADC data from the GPO. 
	Note:
		This disables any potentially conflicting buffers in the affected
		GPIOs, i.e. input and output buffers will not be enabled 
		simultaneously for the GPIOs necessarily set by this function.
		All other GPIOs will be left as-is.
	
		Untested.
	*/
	unsigned int gpi_mask = get_GPI_enables();
	unsigned int gpo_mask = get_GPO_enables();

	if (gpi_control) {
		gpi_mask = gpi_mask |= 0xE000;
		GPI_control(3, 
					get_GPI_control(1), 
					get_GPI_control(2), 
					get_GPI_control(3));
		gpo_mask = gpo_mask &= ~0xE000;
	}

	if (gpo_read) {
		gpo_mask = gpo_mask |= 0x07FF;
		gpi_mask = gpi_mask &= ~0x07FF;
		GPO_control(get_GPO_control(0),
					9,
					9,
					9);
	}
	GPI_enables(gpi_mask);
	GPO_enables(gpo_mask);
}
