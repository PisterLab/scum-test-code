#include <stdio.h>
#include "Memory_Map.h"

unsigned int ASC[38] = {0};
extern char send_packet[127];

unsigned int current_lfsr = 0x12345678;

// coarse1, coarse2, coarse3, fine, superfine dac settings
unsigned int dac_2M_settings[5] = {31, 31, 29, 2, 2};


// Reverse endianness of lower 16 bits
unsigned int flip_lsb8(unsigned int in){
	int out = 0;
	
	out |= (0x01 & in) << 7;
	out |= (0x02 & in) << 5;
	out |= (0x04 & in) << 3;
	out |= (0x08 & in) << 1;
	
	out |= (0x10 & in) >> 1;
	out |= (0x20 & in) >> 3;
	out |= (0x40 & in) >> 5;
	out |= (0x80 & in) >> 7;	
	
	return out;
}



/*
Inputs: 
	scan_bits: Pointer to array of unsigned integers
		containing the scan bits.
Outputs:
	No return value. Writes ASC register to analog config,
	then toggles the clocks to laod the scan chain.
*/
void analog_scan_chain_write(unsigned int* scan_bits) {
	
	int i = 0;
	int j = 0;
	unsigned int asc_reg;
	
	// analog_cfg<357> is resetb for chip shift register, so leave that high
	
	for (i=37; i>=0; i--) {
		
		//printf("\n%d,%lX\n",i,scan_bits[i]);
		
		for (j=0; j<32; j++) {

		// Set scan_in (should be inverted)
		if((scan_bits[i] & (0x00000001 << j)) == 0)
			asc_reg = 0x21;	
		else
			asc_reg = 0x20;

		// Write asc_reg to analog_cfg
		ANALOG_CFG_REG__22 = asc_reg;

		// Lower phi1
		asc_reg &= ~(0x2);
		ANALOG_CFG_REG__22 = asc_reg;

		// Toggle phi2
		asc_reg |= 0x4;
		ANALOG_CFG_REG__22 = asc_reg;
		asc_reg &= ~(0x4);
		ANALOG_CFG_REG__22 = asc_reg;

		// Raise phi1
		asc_reg |= 0x2;
		ANALOG_CFG_REG__22 = asc_reg;
		
		}	
	}
}

/*
	Inputs: 
	Outputs:
*/
void analog_scan_chain_load() {
	
	// Assert load signal (and cfg<357>)
	ANALOG_CFG_REG__22 = 0x0028;

	// Lower load signal
	ANALOG_CFG_REG__22 = 0x0020;

}
/* 
Inputs:
	coarse1/2/3: Integers. Coarse tuning values. Must be between 0 and 31.
	fine/superfine: Integers. Fine and superfine tuning values, respectively.
		Must be between 0 and 31.
Outputs:
	No return value. Sets the 2MHz RC DAC frequency by
	-updating the local dac settings array
	-flipping endianness and setting the appropriate bits in the scanchain array
	-writing it to the scanchain
	-loading the scanchain
*/
void set_2M_RC_frequency(int coarse1, int coarse2, int coarse3, int fine, int superfine) {
    
    unsigned int newval;
    unsigned int newcoarse1, newcoarse2, newcoarse3, newfine, newsuperfine;
    
    /* update our local dac array */
    dac_2M_settings[0] = coarse1;
    dac_2M_settings[1] = coarse2;
    dac_2M_settings[2] = coarse3;
    dac_2M_settings[3] = fine;
    dac_2M_settings[4] = superfine;
    
    // make sure each argument is between 0-31, inclusive
    
    // ASC[34] covers 1088:1119
    newval = ASC[34] & 0x8000001F;
    
    // flip endianness of each
    newcoarse1 = (flip_lsb8(coarse1) >> 3) & 0x1F;
    newcoarse2 = (flip_lsb8(coarse2) >> 3) & 0x1F;
    newcoarse3 = (flip_lsb8(coarse3) >> 3) & 0x1F;
    newfine = (flip_lsb8(fine) >> 3) & 0x1F;
    newsuperfine = (flip_lsb8(superfine) >> 3) & 0x1F;

    newval |= newcoarse1 << 26;
    newval |= newcoarse2 << 21;
    newval |= newcoarse3 << 16;
    newval |= newfine << 11;
    newval |= newsuperfine << 6;
    
    // Enable bit
    newval |= 0x1 << 5;
    
    ASC[34] = newval;

    //write to analog scanchain and load
    //analog_scan_chain_write(&ASC[0]);
    //analog_scan_chain_load();
    
    //print_2MHz_DAC();
}


/* Initializes the 2MHz DAC with values set in the dac_2M_settings array. */
void initialize_2M_DAC(void) {
    set_2M_RC_frequency(dac_2M_settings[0], dac_2M_settings[1], dac_2M_settings[2], dac_2M_settings[3], dac_2M_settings[4]);
    // printf("Initialized 2MHz DAC\n");
    // print_2MHz_DAC();
}

/*
Inputs:
	count_2M: Pointer unsigned integer. Location to store 
		2MHz counter value.
	count_LC: Pointer to unsigned integer. Location to store 
		LC divider counter value (read via counter4).
	count_32k: Pointer to unsigned integer. Location to store 
		32kHz counter value.
Outputs:
	No return value. Reads all the values from the counters when the
	function is called.
	1. Disables all counters
	2. Reads the counters' values and stores them in the locations specified
		by the input arguments.
	3. Resets all the counters
	4. Re-enables all the counters
*/
void read_counters(unsigned int* count_2M, unsigned int* count_LC, unsigned int* count_32k){

	unsigned int rdata_lsb, rdata_msb;//, count_LC, count_32k;
	
	// Disable all counters
	ANALOG_CFG_REG__0 = 0x007F;
		
	// Read 2M counter
	rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
	rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
	*count_2M = rdata_lsb + (rdata_msb << 16);
		
	// Read LC_div counter (via counter4)
	rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x200000);
	rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x240000);
	*count_LC = rdata_lsb + (rdata_msb << 16);
		
	// Read 32k counter
	rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x000000);
	rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x040000);
	*count_32k = rdata_lsb + (rdata_msb << 16);
	
	// Reset all counters
	ANALOG_CFG_REG__0 = 0x0000;		
	
	// Enable all counters
	ANALOG_CFG_REG__0 = 0x3FFF;	
	
	//printf("LC_count=%X\n",count_LC);
	//printf("2M_count=%X\n",count_2M);
	//printf("32k_count=%X\n\n",count_32k);

}

/*
Inputs:
	current_lfsr: Pointer to array of unsigned integers. TODO: What is this?
Outputs:
	No return value. TODO: What does this actually do?
*/
void update_PN31_byte(unsigned int* current_lfsr){
	int i;
	
	for(i=0; i<8; i++){
		int newbit = (((*current_lfsr >> 30) ^ (*current_lfsr >> 27)) & 1);
		*current_lfsr = ((*current_lfsr << 1) | newbit);	
	}
}

/*
Inputs:
	num_bytes: Unsigned integer. 
Outputs:
	No return value. TOOD: What does this actually do?
*/
void TX_load_PN_data(unsigned int num_bytes){
	int i;
	for(i=0; i<num_bytes; i++){
		send_packet[i] = (char)(current_lfsr & 0xFF);
	}
	
	RFCONTROLLER_REG__TX_PACK_LEN = num_bytes + 2;
	RFCONTROLLER_REG__CONTROL = 0x1; // "lod"	
}

/*
Inputs:
	num_bytes: Unsigned integer. 
Outputs:
	No return value. TOOD: What does this actually do?
*/
void TX_load_counter_data(unsigned int num_bytes){
	int i;
	for(i=0; i<num_bytes; i++){
		send_packet[i] = (char)(0x30 + i);
	}
	
	RFCONTROLLER_REG__TX_PACK_LEN = num_bytes + 2;	
	RFCONTROLLER_REG__CONTROL = 0x1; // "lod"
}


/*
Inputs:
	position: Unsigned integer. The index within the ASC
		to modify. Note that the ASC is 32-bit aligned.
Outputs:
	No return value. TOOD: What does this actually do?
*/
void set_asc_bit(unsigned int position){

	unsigned int index;
	
	index = position >> 5;
	
	ASC[index] |= 0x80000000 >> (position - (index << 5));
	
	// Possibly more efficient
	//ASC[position/32] |= 1 << (position%32);

}

/*
Inputs:
	position: Unsigned integer. The index within the ASC
		to modify. Note that the ASC is 32-bit aligned.
Outputs:
	No return value. TOOD: What does this actually do?
*/

unsigned int get_asc_bit(unsigned int position) {
	/*
	Inputs:
		position: The position in the ASC to get the bit value.
	Outputs:
		The bit (the LSB of unsigned int form) at the particular
		position in the analog scan chain.
	*/
	unsigned int word_index = position >> 5;
	unsigned int ASC_word_rev = reverse(ASC[word_index]);
	unsigned int ASC_bit = (ASC_word_rev >> (position - (word_index<<5))) 
							& 0x1;
	return ASC_bit;
}

void clear_asc_bit(unsigned int position){

	unsigned int index;
	
	index = position >> 5;
	
	ASC[index] &= ~(0x80000000 >> (position - (index << 5)));
	
	// Possibly more efficient
  //ASC[position/32] &= ~(1 << (position%32));                


}

unsigned int get_GPI_enables(void) {
	/*
	Inputs:
		None.
	Outputs:
		Returns the mask for the GPI enables in unsigned
		int form. Note that this is intended to be 2 bytes long.
	*/
	unsigned int gpi_enables = 0x0000;
	// LSB -> MSB
	unsigned short asc_locations[16] = {1132,1134,1136,1138,1139,1141,1143,1145,1116,1118,1120,1122,1123,1125,1127,1129};
	int i;
	for (i=0; i<16; i++) {
		gpi_enables |= (get_asc_bit(asc_locations[i]) << i);
	}
	return gpi_enables;
}

unsigned int get_GPO_enables(void) {
	/*
	Inputs:
		None.
	Outputs:
		Returns the mask for the GPO enables in unsigned int form.
		Note that this is intended to be 2 bytes long.
	*/
	unsigned int gpo_enables = 0x0000;
	// LSB -> MSB
	unsigned short asc_locations[16] = {1131,1133,1135,1137,1140,1142,1144,1146,1115,1117,1119,1121,1124,1126,1128,1130};
	int i;
	for (i=0; i<16; i++) {
		gpo_enables |= ((0x1-get_asc_bit(asc_locations[i])) << i);
	}
	return gpo_enables;
}

unsigned char get_GPI_control(unsigned short rowNum) {
	/*
	Inputs:
		rowNum: Integer [0,3]. Determines which grouping of GPIs you'd 
			like to get the bank for.
	Outputs:
		Returns (in unsigned character form) the bank number associated
		a specific grouping of GPIOs. e.g. if you have the GPI setting such
		that D_OUT<0> is adc_reset_gpi, get_GPI_control(0) will return 
		3. A return value of 0xFF indicates that the row number was invalid.
	Notes:
		Untested.
	*/
	int start_idx;
	int i;
	unsigned char row_value;
	switch (rowNum) {
		case 0:
			start_idx = 261;
			break;
		case 1:
			start_idx = 263;
			break;
		case 2:
			start_idx = 265;
			break;
		case 3:
			start_idx = 267;
			break;
		default:
			// Do nothing
			return 0xFF;
	}
	for(i=0; i<2; i++) {
		row_value |= (get_asc_bit(start_idx+i) << i);
	}
	return row_value;
}

unsigned char get_GPO_control(unsigned short rowNum) {
	/*
	Inputs:
		rowNum: Integer [0,3]. Determines which grouping of GPIs you'd 
			like to get the bank for.
	Outputs:
		Returns (in unsigned character form) the bank number associated
		a specific grouping of GPIOs. e.g. if you have the GPI setting such
		that D_OUT<0> is ADC_CLK, get_GPO_control(0) will return 2. A return
		value of 0xFF indicates that the row number was invalid.
	Notes:
		Untested.
	*/
	int start_idx;
	int i;
	unsigned char row_value;
	switch (rowNum) {
		case 0:
			start_idx = 245;
			break;
		case 1:
			start_idx = 249;
			break;
		case 2:
			start_idx = 253;
			break;
		case 3:
			start_idx = 257;
			break;
		default:
			// Do nothing
			return 0xFF;
	}
	for(i=0; i<4; i++) {
		row_value |= (get_asc_bit(start_idx+i) << i);
	}
	return row_value;
}
