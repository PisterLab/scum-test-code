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
#include "bucket_o_functions.h"
#include <math.h>
#include "scum_radio_bsp.h"
#include "lighthouse.h"

extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory addresses	
#define crc_value         (*((unsigned int *) 0x0000FFFC))
#define code_length       (*((unsigned int *) 0x0000FFF8))

// Target radio LO freq = 2.4025G
// Divide ratio is currently 480*2
// Calibration counts for 100ms
unsigned int LC_target = 250187;
unsigned int LC_code = 680;

// HF_CLOCK tuning settings
unsigned int HF_CLOCK_fine = 17;
unsigned int HF_CLOCK_coarse = 3;

// RC 2MHz tuning settings
// This the transmitter chip clock
unsigned int RC2M_coarse = 21;
unsigned int RC2M_fine = 15;
unsigned int RC2M_superfine = 15;

// Receiver clock settings
// The receiver chip clock is derived from this clock
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

// Variables for lighthouse RX	
unsigned short current_gpio = 0, last_gpio = 0, state = 0, nextstate = 0, pulse_type = 0;	
unsigned int timestamp_rise, timestamp_fall, pulse_width;	
//unsigned int azimuth_delta, elevation_delta;	
unsigned int azimuth_t1, elevation_t1, azimuth_t2, elevation_t2;	
unsigned short num_data_points = 0, target_num_data_points;	
unsigned int azimuth_t1_data[1000], elevation_t1_data[1000], azimuth_t2_data[1000], elevation_t2_data[1000];	

unsigned int pulse_width_data[1000];	

void test_LC_sweep_tx(void) {
	/*
	Inputs:
	Outputs:
	Note:
		Initialization of the scan settings should already have 
		been performed.
	*/

	int coarse, mid, fine;
	unsigned char iterations = 3;
	unsigned int i;

	// Enable the TX. NB: Requires 50us for frequency settling
	// transient.
	radio_txEnable();
	
	while (1) {
		for (coarse=22; coarse<25; coarse++) {
			for (mid=0; mid<32; mid++) {
				for (fine=0; fine<32; fine++) {
					// Construct the packet 
					// with payload {coarse, mid, fine} in 
					// separate bytes
					send_packet[0] = coarse & 0x1F;
					send_packet[1] = mid & 0x1F;
					send_packet[2] = fine & 0x1F;
					send_packet[4] = 53;
					send_packet[5] = 53;
					radio_loadPacket(5);

					// Set the LC frequency
					//LC_FREQCHANGE(22&0x1F, 21&0x1F, 4&0x1F);
					LC_FREQCHANGE(coarse&0x1F, mid&0x1F, fine&0x1F);
					// TODO: Wait for at least 50us
					for (i=0; i<5000; i++) {}

					// Send bits out the radio thrice for redundancy
					for(i=0; i<iterations; i++) {
						radio_txNow();
					}
				}
			}
		}
	}
}

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	int t,t2,x;
	unsigned int calc_crc;

	unsigned int rdata_lsb, rdata_msb, count_LC, count_32k, count_2M;
	
	printf("Initializing...");
		
	// Set up mote configuration
	// This function handles all the analog scan chain setup
	initialize_mote_lighthouse();
		
	// Check CRC to ensure there were no errors during optical programming
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
	
	// Debug output
	//printf("\nCode length is %u bytes",code_length); 
	//printf("\nCRC calculated by SCM is: 0x%X",calc_crc);	
	
	//printf("done\n");
	
	// After bootloading the next thing that happens is frequency calibration using optical
	printf("Calibrating frequencies...\n");
	
	// Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
	// For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
	// Aux is inverted (0 = on)
	// Memory-mapped LDO control
	// ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
	// For MUX signals, '1' = FSM control, '0' = memory mapped control
	// For EN signals, '1' = turn on LDO
	// Turn on LO, DIV, PA
	ANALOG_CFG_REG__10 = 0x68;
	
	// Turn off polyphase and disable mixer
	ANALOG_CFG_REG__16 = 0x6;
	
	// Enable optical SFD interrupt for optical calibration
	ISER = 0x0800;
	
	// Wait for optical cal to finish
	while(optical_cal_finished == 0);
	optical_cal_finished = 0;

	printf("Cal complete\n");
	
	// Disable divider to save power
	//divProgram(480, 0, 0);
	
		// Reset RF Timer count register	
	RFTIMER_REG__COUNTER = 0x0;	

//test_LC_sweep_tx();
	// Number of data points to gather before printing		
	target_num_data_points = 120;
	
	//test_LC_sweep_tx();
	// The optical_data_raw signal is not synchronized to HCLK domain so could possibly see glitching problems	
		last_gpio = current_gpio;	
		current_gpio = (0x8 & GPIO_REG__INPUT) >> 3;	
		//debounce_gpio(current_gpio,&deb_gpio,&trans_time);	
		//current_gpio = deb_gpio;	
		//if(state != nextstate) printf("%d\n",state);	

		// Update to next FSM state	
		state = nextstate;	
		while(1) {		

		// Read GPIO<3> (optical_data_raw - this is the digital output of the optical receiver)	
		// The optical_data_raw signal is not synchronized to HCLK domain so could possibly see glitching problems	
		last_gpio = current_gpio;	
		current_gpio = (0x8 & GPIO_REG__INPUT) >> 3;	
		//debounce_gpio(current_gpio,&deb_gpio,&trans_time);	
		//current_gpio = deb_gpio;	
		//if(state != nextstate) printf("%d\n",state);	

		// Update to next FSM state	
		state = nextstate;	

		// Detect rising edge	
		if(last_gpio == 0 && current_gpio == 1){	

			// Reset RF Timer count register at rising edge of first sync pulse	
			//if(state == 0) RFTIMER_REG__COUNTER = 0x0;	

			// Save when this event happened	
			timestamp_rise = RFTIMER_REG__COUNTER;	

		}	

		// Detect falling edge	
		else if(last_gpio == 1 && current_gpio == 0){	

			// Save when this event happened	
			timestamp_fall = RFTIMER_REG__COUNTER;	

			// Calculate how wide this pulse was	
			//pulse_width = timestamp_fall - timestamp_rise;	
			pulse_width = timestamp_fall - timestamp_rise;	
			//printf("Pulse Type, Width:%d, %d\n",classify_pulse(timestamp_rise, timestamp_fall),pulse_width);	

			// Need to determine what kind of pulse this was	
			// Laser sweep pulses will have widths of only a few us	
			// Sync pulses have a width corresponding to	
			// 62.5 us - azimuth   - data=0 (625 ticks of 10MHz clock)	
			// 72.9 us - elevation - data=0 (729 ticks)	
			// 83.3 us - azimuth   - data=1 (833 ticks)	
			// 93.8 us - elevation - data=0 (938 ticks)	
			// A second lighthouse can be distinguished by differences in these pulse widths	
			update_state(classify_pulse(timestamp_rise, timestamp_fall),timestamp_rise);	
		}	
	}
}
