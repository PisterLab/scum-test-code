#include <stdio.h>
#include <time.h>
#include <rt_misc.h>
#include <stdlib.h>
#include <math.h>
#include "../Memory_map.h"
#include "../scm3C_hardware_interface.h"
#include "../scm3_hardware_interface.h"
#include "../scum_radio_bsp.h"

// Packet to be sent over TX
extern char send_packet[127];

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
		for (coarse=0; coarse<32; coarse++) {
			for (mid=0; mid<32; mid++) {
				for (fine=0; fine<32; fine++) {
					// Construct the packet 
					// with payload {coarse, mid, fine} in 
					// separate bytes
					send_packet[0] = coarse & 0xFF;
					send_packet[1] = mid & 0xFF;
					send_packet[2] = fine & 0xFF;
					radio_loadPacket(3);

					// Set the LC frequency
					LC_FREQCHANGE(coarse&0x1F, mid&0x1F, fine&0x1F);

					// TODO: Wait for at least 50us
					for (i=0; i<250; i++) {}

					// Send bits out the radio thrice for redundancy
					for(i=0; i<iterations; i++) {
						radio_txNow();
					}
				}
			}
		}
	}
}

void test_LC_sweep_rx(void) {
	/*
	Inputs:
	Outputs:
	Note:
		Initialization should already be set.
	*/

	int coarse, mid, fine;

	// unsigned int code;

	for (coarse=0; coarse<32; coarse++) {
		for (mid=0; mid<32; mid++) {
			for (fine=0; fine<32; fine++) {
				// Set the LC frequency
				LC_FREQCHANGE(coarse&0x1F, mid&0x1F, fine&0x1F);

				// TODO: Listen for a packet for the openMote
				// TODO: Upon reception, print the RSSI, LQI, and {coarse/mid/fine}
			}
		}
	}
}

void test_LC_sweep_uart(unsigned char PA_on, unsigned int iterations) {
	/*
	Inputs:
		PA_on: 0 = PA is off; anything else = PA is on.
		iterations: Number of times to take a reading for each
			DAC setting.
	Outputs:
	*/
	int coarse;
	int mid;
	int fine;

	unsigned int code;			// The combined code fed into the tuning DAC
	unsigned int counter_out;	// The output of the counter. The frequency 
								// will be calculated off-chip for this.

	unsigned int i;

	// Sending the number of iterations over UART to SCM
	// in conjunction with what test_LC_sweep() expects
	printf("%d",iterations);

	// Iterating through coarse and mid while fine is all 1 or all 0
	for (coarse=0; coarse<32; coarse++){
		for (mid=0; mid<32; mid++){
			for (fine=0; fine<32; fine+=31) {
				// Change the frequency
				LC_FREQCHANGE(coarse&0x1F, mid&0x1F, fine&0x1F);

				// Measure the frequency with the divider on
				code = fine&0x1F + ((mid&0x1F) << 5) + ((coarse&0x1F) << 10);
				
				// TODO: Where to get the counter output?
				// counter_out = 
				for (i=0; i<iterations; i++) {
					printf("%X:%d\n", code, counter_out);
				}
			}
		}
	}
}
