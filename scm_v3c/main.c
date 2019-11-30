//------------------------------------------------------------------------------
// u-robot Digital Controller Firmware
//------------------------------------------------------------------------------

#include <stdio.h>
#include <time.h>
#include <rt_misc.h>
#include <stdlib.h>
#include "Memory_map.h"
#include "Int_Handlers.h"
#include "rf_global_vars.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include <math.h>
#include "scum_radio_bsp.h"

// # defines for calibration
# define		EXECUTE_COARSE_ISM_SEARCH		0x0; // if this is set to 1, run the search for coarse codes throughout the ISM band
# define		EXECUTE_MONOTONIC						0x0; // if this is set to 1, run the LC_monotonic generating function in the optical ISR
# define		COARSE_CODE_SEARCH_INIT			18;  // initial guess for low end of the ISM band

extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory addresses	
#define crc_value         (*((unsigned int *) 0x0000FFFC))
#define code_length       (*((unsigned int *) 0x0000FFF8))

// Target LC freq = 2.405G
// Divide ratio is currently 480*2
unsigned int LC_target = 501042; 
unsigned int LC_code = 5;

// HF_CLOCK tuning settings
unsigned int HF_CLOCK_fine = 17;
unsigned int HF_CLOCK_coarse = 3;

// RC 2MHz tuning settings
unsigned int RC2M_coarse = 21;
unsigned int RC2M_fine = 15;
unsigned int RC2M_superfine = 15;

// Receiver clock settings
unsigned int IF_clk_target = 1600000;
unsigned int IF_coarse = 22;
unsigned int IF_fine = 18;

unsigned int cal_iteration = 0;
unsigned int run_test_flag = 0;
unsigned int num_packets_to_test = 1;

unsigned short optical_cal_iteration = 0, optical_cal_finished = 0;

unsigned short doing_initial_packet_search;
unsigned short current_RF_channel;
unsigned short do_debug_print = 0;

// loop and state variables for coarse code search
unsigned int coarse_code_search_state;
unsigned int coarse_code;
unsigned int coarse_search_index;
unsigned int ism_coarse_codes[16];

// loop variables for building LC monotonic
unsigned int count_LC_glob;
unsigned int count_32k_glob;
unsigned int previous_count_LC; // stores the count in the previous iteration for comparison purposes
unsigned int mon_build_complete; // signals when the LC monotomic function has been built (mid0 code only)
unsigned int mon_build_complete_coarse; // signals when the LC monotonic function has been built (coarse0 code only)
unsigned int pass; // 
unsigned int mid0; // correct mid0 value
unsigned int coarse0;


//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	int t,t2;
	int i1, i2, i3;
	int sweep_loop;
	int count_valid;

	unsigned int calc_crc;

	unsigned int rdata_lsb, rdata_msb, count_LC, count_32k, count_2M;
	
	unsigned char packetBLE[16];
	unsigned char *byte_addr = &packetBLE[0];
	
	unsigned char spi_out;
	unsigned char spi_in;
	
	unsigned int x_accel;
	unsigned int y_accel;
	unsigned int z_accel;
	
	unsigned int execute_coarse_ISM_search;
	unsigned int execute_monotonic;
	
	printf("Initializing...");
		
	// Set up mote configuration
	initialize_mote();
	
	// Check CRC
	printf("\n-------------------\n");
	printf("Validating program integrity..."); 
	
	calc_crc = crc32c(0x0000,code_length);

	if(calc_crc == crc_value){
		printf("CRC OK\n");
	}
	else{
		printf("\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\n");
		while(1);
	}
	
	//printf("done\n");
	printf("Calibrating frequencies...\n");
	
	// Memory-mapped LDO control
	// ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
	// For MUX signals, '1' = FSM control, '0' = memory mapped control
	// For EN signals, '1' = turn on LDO
	ANALOG_CFG_REG__10 = 0x78;
	
	// ---------------------------------------------------------------------------------------------------- //
	// -------------------  INITIALIZATION FOR OPTICAL SFD BASED FREQUENCY CALIBRATION  ------------------- //
	
	// -------- Set up initial variables to find the list of coarse codes necessary for scouring the ISM band
	execute_coarse_ISM_search = EXECUTE_COARSE_ISM_SEARCH;
	if (execute_coarse_ISM_search) {
		coarse_code = COARSE_CODE_SEARCH_INIT;
		coarse_search_index = 0;
		coarse_code_search_state = 1;
	}
	else {
		coarse_code_search_state = 0;
	}
	
	// -------- Set up initial variables for the LC monotonic building
	execute_monotonic = EXECUTE_MONOTONIC;
	if (execute_monotonic) {
		mon_build_complete = 0;
		pass = 1;
		mid0 = 23;
		coarse0 = 142;
	}
	else {
		mon_build_complete = 1;
	}
	// --------------------------------------  END OF INITIALIZATION -------------------------------------- //
	// ---------------------------------------------------------------------------------------------------- //

	/*
	// Enable optical SFD interrupt for optical calibration
	ISER = 0x0800;
	
	// Wait for optical cal to finish
	
	// Set up initial variables for monotonic building
	while(optical_cal_finished == 0);
	optical_cal_finished = 0;
	// FINISHED WITH OPTICAL CALIBRATION
	*/

	printf("Cal complete\n");
			
	x_accel = 0; y_accel = 0; z_accel = 0;
	write_imu_register(0x06,0x00);
	for(t=0; t<5000; t++);
		
while(1) {
		
	//initialize_mote();
	
	
	x_accel = read_acc_x();
	y_accel = read_acc_y();
	z_accel = read_acc_z();
				
			
	/*
	// Memory-mapped LDO control
	// ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
	// For MUX signals, '1' = FSM control, '0' = memory mapped control
	// For EN signals, '1' = turn on LDO
	ANALOG_CFG_REG__10 = 0x00;
	for(t=0; t<1000; t++);
	ANALOG_CFG_REG__10 = 0x08;
	for(t=0; t<500; t++);
	ANALOG_CFG_REG__10 = 0x28;
	for(t=0; t<4000; t++);
	ANALOG_CFG_REG__10 = 0x00;
	for(t=0; t<10000; t++);
	ANALOG_CFG_REG__10 = 0x08;
	for(t=0; t<500; t++);
	ANALOG_CFG_REG__10 = 0x18;
	for(t=0; t<2500; t++);
	ANALOG_CFG_REG__10 = 0x00;
	
	
	for(t=0;t<39;t++){
		ASC[t] &= 0x00000000;
	}
	
	// Program analog scan chain
	analog_scan_chain_write(&ASC[0]);
	analog_scan_chain_load();*/
	
	
	}
}
