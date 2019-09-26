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

// Target LC freq = 2.4025G
// Divide ratio is currently 480
// Calibration counts for 100ms
unsigned int LC_target = 500521; 
//unsigned int LC_code = 989; //Board#4

//unsigned int LC_code = 793; //Board#5 tx ch11
//unsigned int LC_code = 796; //Board#5 rx ch11
//unsigned int LC_code = 741; //Board#5 rx ch11
unsigned int LC_code = 548; //Board#5 rx ch11


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

// Variables for lighthouse RX
unsigned short current_gpio = 0, last_gpio = 0, state = 0, nextstate = 0, pulse_type = 0;
unsigned int timestamp_rise, timestamp_fall, pulse_width;
//unsigned int azimuth_delta, elevation_delta;
unsigned int azimuth_t1, elevation_t1, azimuth_t2, elevation_t2;
unsigned short num_data_points = 0, target_num_data_points;
unsigned int azimuth_t1_data[1000], elevation_t1_data[1000], azimuth_t2_data[1000], elevation_t2_data[1000];

unsigned int pulse_width_data[1000];




//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	int t,t2;
	unsigned int calc_crc;

	unsigned int rdata_lsb, rdata_msb, count_LC, count_32k, count_2M;
	
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
	// Debug output
	//printf("\nCode length is %u bytes",code_length); 
	//printf("\nCRC calculated by SCM is: 0x%X",calc_crc);	
	
	//printf("done\n");
	printf("Calibrating frequencies...\n");
	
	// For initial calibration, turn on AUX, DIV, IF, LO
	// Aux is inverted (0 = on)
	// Memory-mapped LDO control
	// ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
	// For MUX signals, '1' = FSM control, '0' = memory mapped control
	// For EN signals, '1' = turn on LDO
	ANALOG_CFG_REG__10 = 0x58;
	
	// Enable optical SFD interrupt for optical calibration
	ISER = 0x0800;
	
	// Wait for optical cal to finish
	while(optical_cal_finished == 0);
	optical_cal_finished = 0;

	printf("Cal complete\n");
	
	radio_rfOff();
	
	// Disable all interrupts
	ICER = 0xFFFF;
	
	
	// Begin SCM Lighthouse RX Demo
	// ----------------------------
	// This is meant to be a barebones proof of concept for how to process lighthouse signals on SCM.
	// BW 7-27-19

	// Algorithm
	// ----
	// Continuously poll GPIO<3> which is connected to the output of optical receiver.
	// Look for rising and falling edges and measure the pulse widths between the two.
	// An FSM is used to look for a sequence of Az-Sync | Az-Sweep | El-Sync | El-Sweep
	// If those four pulses are found in order, 
	// the times between rising edges for the sync->laser pulses are printed,
	// Azimuth printed first, elevation second.
	// If a full valid measurement isn't acquired, don't print anything.
	
		
	// Hardware Setup - these were all already done above in initialize_mote()
	// -----
	// The first four GPIO outputs must be set to Bank 0 (to get optical debug signals)
	// The first four GPIO inputs must be set to Bank 0 (to get Cortex GP inputs)
	// Both input and output directions for those banks must be enabled
	// Need to lower always on supply to keep from glitching
	// Set RFTimer source to HF_CLOCK (20MHz) and its divider to divide by 2
	// Set HCLK source to HF_CLOCK (20MHz) and its divider to divide by 2
	// This results in both HCLK (the Cortex) and RFTimer running at 10 MHz


	// Possible improvements and alternative implementation options:
	// ----
	// 1)
	// Rather than polling GPIO<3>, it would probably be preferable to use interrupts,
	// but SCM doesn't have an easy way to only trigger an ISR on rise/fall edges.
	// If the signal is still high when the ISR finishes running, it will execute again.
	// This might be a problem for the long duration of the sync pulses.
	// One possibility might be to plug into two interrupts, one active low and one active high,
	// then ping-ping back and forth between the two enabling/disabling as needed.
	// These issues are probably mitigated if the sync pulses are replaced with RF.
	// Note that using interrupts will require physically connecting the optical RX output
	// to a different GPIO pin that serves as an interrupt.
	
	
	// 2)
	// There is another counter connected to HF_CLOCK that could be used instead of RFTimer
	// If you need RFTimer to do radio timing, then may be forced to use this other counter.
	// It is not as easy to read from but should work; see optical_cal section for how to read/reset
	
	
	// 3)
	// The timing difference between the two rising edges of az/el sync pulses could be used
	// to make a better estimate of time between sync and sweep rising edges.
	// The 10 MHz SCM clock is not all that accurate - probably +/- 1500ppm after calibration.
	// Since you know how many ticks there should be in an ideal 10 MHz clock between the two
	// sync pulses, you should be able to estimate the error in the SCM 10 MHz clock.
	// Ex: there should be 83,333 10MHz ticks in 8.333ms; if instead the average number of 
	// ticks you were getting between sync pulse rising edges was 83,210 then you know that 
	// the SCM 10 MHz clock is too slow by about 1500ppm, and can apply a correction to the
	// measurements between sync and sweep.
	
	// 4)
	// You may be able to run HCLK/RFTimer faster than 10 MHz if desired.  Probably need to raise
	// the supply voltage for VDDD in order to do so.  Simply trying to go to 20 MHz was not 
	// working when tried with the board used to develop this demo.
	
	// 5)
	// Note that some issues were seen depending on what was configurtion was happening in 
	// initialize_mote().  Random glitches can occur in the output of the optical RX depending on 
	// other mote settings.  The settings used in this demo seem to work, but if the radio is needed
	// then more configuration must happen inside initialize_mote().  If adding those changes causes
	// the lighthouse RX to stop working then try disabling those new changes one by one to isolate
	// the cause.
	
	// Reset RF Timer count register
	RFTIMER_REG__COUNTER = 0x0;
	
	// Number of data points to gather before printing
	target_num_data_points = 120;
	
		
	// Loop forever looking for pulses
	// If this gets interrupted every once in awhile by an ISR, it should still work (untested)
	// but you will likely miss some lighthouse pulses.
	while(1) {	
		
		// Read GPIO<3> (optical_data_raw - this is the digital output of the optical receiver)
		// The optical_data_raw signal is not synchronized to HCLK domain so could possibly see glitching problems
		last_gpio = current_gpio;
		current_gpio = (0x8 & GPIO_REG__INPUT) >> 3;
		
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
			pulse_type_t pulse_type;
			// Save when this event happened
			timestamp_fall = RFTIMER_REG__COUNTER;
			
			// Calculate how wide this pulse was
			//pulse_width = timestamp_fall - timestamp_rise;
			pulse_width = timestamp_fall - timestamp_rise;
				
			//*** 1. classify pulse based on fall and rise time *** 
			pulse_type = classify_pulse(timestamp_rise,timestamp_fall);
			//printf("Pulse type: %d\n",(int)pulse_type);
			

			//*** 2. update state machine based on pulse type and timestamp rise time of pulse ***
			update_state(pulse_type,timestamp_rise);
	
			
			
			
			
		}
	}
}
