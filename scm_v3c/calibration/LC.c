#include <stdio.h>
#include <stdlib.h>
#include "../Memory_Map.h"
#include "../scm3_hardware_interface.h"
#include "../scm3C_hardware_interface.h"


void test_LC_sweep(unsigned char PA_on, unsigned int iterations) {
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
				counter_out = 
				for (i=0; i<iterations; i++) {
					printf("%X:%d\n", code, counter_out);
				}
			}
		}
	}
}