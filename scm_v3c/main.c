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
#include "test_code.h"
#include "./sensor_adc/adc_test.h"
#include "zappy2.h"


extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory addresses	
#define crc_value         (*((unsigned int *) 0x0000FFFC))
#define code_length       (*((unsigned int *) 0x0000FFF8))

// Target LC freq = 2.405G
// Divide ratio is currently 480
unsigned int LC_target = 501042; 
unsigned int LC_code = 975;

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

unsigned short optical_cal_iteration = 0;
unsigned short optical_cal_finished = 0;

unsigned short doing_initial_packet_search;
unsigned short current_RF_channel;
unsigned short do_debug_print = 0;

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
	int t;
	int i;
	int r=0;
	int periodCounts=49500, toggles =400;
	unsigned int calc_crc;
	char out_string[10];
	// Set up mote configuration
	printf("Initializing...");
	initialize_mote();
	
	// Check CRC
	printf("\n-------------------\n");
	printf("Validating program integrity..."); 
	
	calc_crc = crc32c(0x0000,code_length);

	if(calc_crc == crc_value){
		printf("CRC OK\n");
	}
	else {
		printf("\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\n");
		while(1);
	}
	
	if (0) {
		printf("Calibrating frequencies...\n");
		
		ANALOG_CFG_REG__10 = 0x78;
		// Enable optical SFD interrupt for optical calibration
		ISER = 0x0800;
		
		// Wait for optical cal to finish
		while(optical_cal_finished == 0) {}
		optical_cal_finished = 0;

		printf("Cal complete\n");
	}
	
	
//testZappy2(1000);
//while(1)
//	{

//		if(!(~(0xFDFF | (~(GPIO_REG__INPUT) & ~(0xFDFF))))&&r!=1)//checks if GPIO9 is low and that you are in state one
//		{
//			//printf("%x\n", GPIO_REG__INPUT);
//			printf("state1\n");
//			sara_start(toggles,periodCounts);
//			r=1;
//			while(!(~(0xFDFF | (~(GPIO_REG__INPUT) & ~(0xFDFF)))))//waits until GPIO9 goes to zero
//			{
//					 for(t=0;t<10000;t++);
//			}
//		}

//		if(!(~(0xFDFF | (~(GPIO_REG__INPUT) & ~(0xFDFF))))&&r==1)//checks if GPIO9 is low and you are in state 2
//		{
//			GPIO_REG__OUTPUT=GPIO_REG__OUTPUT & 0xFFCF;//should set GPIO4 and 5 to zero.
//			i=0;
//			while(i<12)  { 
//					if(i<12)
//					{
//							GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
//							for(t=0;t<periodCounts;t++);
//					}
//					i=i+1;
//			}
//			r=0;
//			printf("state2\n");
//			while(!(~(0xFDFF | (~(GPIO_REG__INPUT) & ~(0xFDFF)))))//waits until GPIO9 goes to zero
//			{
//					 for(t=0;t<10000;t++);
//			}
//		}
//	}

printf("I am alive :D\n");
while(1)
{
	for(t=0;t<periodCounts;t++);
	GPIO_REG__OUTPUT=0x0000;
	//printf("I am alive :D\n");
	for(t=0;t<periodCounts;t++);
	GPIO_REG__OUTPUT=0xFFFF;
}
}
