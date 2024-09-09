#include <stdio.h>
#include <string.h>

#include "ads1299.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "rftimer.h"
#include "gpio.h"
#include "tuning.h"
#include "radio.h"
//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))
#define ISR_TIMER_ID 2

// Start coarse code for the sweep to find 802.15.4 channels.
#define START_COARSE_CODE 20
// End coarse code for the sweep to find 802.15.4 channels.
#define END_COARSE_CODE 23

// Start medium code for the sweep to find 802.15.4 channels.
#define START_MEDIUM_CODE 0
// End medium code for the sweep to find 802.15.4 channels.
#define END_MEDIUM_CODE 31

// Start fine code for the sweep to find 802.15.4 channels.
#define START_FINE_CODE 0
// End fine code for the sweep to find 802.15.4 channels.
#define END_FINE_CODE 31

#define ACTIVE_ADS_CHANNEL 1

// 802.15.4 channel on which to transmit the ADC data.
#define IEEE_802_15_4_TX_CHANNEL 15

//=========================== variables =======================================

typedef struct {
		ads_data_t ads_measurement[100];
		uint8_t gpio_1_state;
		uint8_t gpio_2_state;

} app_vars_t;

/*
	The best code that you could find when running tx_cal_open_loop()
	should be saved here.
	- DL 8/5/2024
*/
static tuning_code_t g_tuning_code = {
	.coarse = 20, 
	.mid = 8,
	.fine = 5
};

#define SINE_LUT_SIZE 4 
uint8_t sine_lut[] = {4, 9, 4, 0};

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

static void self_test_isr(void) {
	gpio_1_toggle();
	gpio_2_toggle();
	
	delay_ticks_asynchronous(100000, ISR_TIMER_ID);
}

void init_self_test_isr(void) {

	// Set initial conditions	
	GPI_enable_clr(1);
	GPI_enable_clr(2);
	radio_delayCPUMilliseconds(1);

	//GPO_enable_set(1);
	//GPO_enable_set(2);
	radio_delayCPUMilliseconds(1);
	
	gpio_1_set();
	gpio_2_clr();
	radio_delayCPUMilliseconds(1);


	rftimer_enable_interrupts();
	rftimer_enable_interrupts_by_id(ISR_TIMER_ID);
	rftimer_set_callback_by_id(self_test_isr, ISR_TIMER_ID);
	
	delay_ticks_asynchronous(100000, ISR_TIMER_ID);
}

void sulu_ads1299_self_test(void)
{
	// Self-test circuit that uses GPIO1 and GPIO2
	// as inputs to an RC high-pass filter that is re-biased to 2.5V
	// Occasional pulses from GPIO1 and GPIO2 should create interesting low-voltage waveforms

	printf("Starting the self-test\n");

	// Set up the GPIO 
	GPO_enable_set(1);
	GPI_enable_clr(1);

	GPO_enable_set(2);
	GPI_enable_clr(2);

	analog_scan_chain_write();
	analog_scan_chain_load();

	// Initial conditions
	gpio_1_clr();
	gpio_2_set();
	radio_delayCPUMilliseconds(5);

	// Begin the loop

	while(1) {
		// Toggle GPIO1
		gpio_1_set();
		gpio_2_clr();
		radio_delayCPUCycles(60);
		gpio_1_clr();
		gpio_2_set();
		radio_delayCPUMilliseconds(10);
	}
}

void tx_cal_open_loop(void) {
	uint8_t packet[16] = {0}; // Initialize all elements to 0
	while(1) {
		for (uint8_t coarse = START_COARSE_CODE; coarse <= END_COARSE_CODE; coarse++) {
			for (uint8_t mid = START_MEDIUM_CODE; mid <= END_MEDIUM_CODE; mid++) {
				for (uint8_t fine = START_FINE_CODE; fine <= END_FINE_CODE; fine++) {
					//printf("Sent - C:%u M:%u F:%u\n", coarse, mid, fine);
					for(uint8_t i = 0; i < 4; i++) {
						g_tuning_code.coarse = coarse;
						g_tuning_code.mid = mid;
						g_tuning_code.fine = fine;

						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						radio_delayCPUCycles(6);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						radio_delayCPUCycles(6);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						tuning_tune_radio(&g_tuning_code);
						radio_delayCPUCycles(6);
						
						
						packet[0] = coarse;
						packet[1] = mid;
						packet[2] = fine;
						packet[3] = 0x55; // 0x55 as a placeholder for additional data
						send_packet_cpu(&packet[0], 18); // 64 + 2 for the CRC
						radio_delayCPUMilliseconds(1); 
					}
					radio_delayCPUCycles(10); // Delay to allow for transmission completion
				}
			}
		}
	}
}

int configure_ads1299(void) {
	uint8_t wreg_val = 0x0;
	uint8_t rreg;
	uint8_t print_reg = ads_rreg(ADS_REG_ID);
	printf("ID: %x\r\n", print_reg);        // should return 3E


	// REG_CHnSet
	
	print_reg = ads_rreg(ADS_REG_CH1SET);
	printf("channel1: %x\r\n", print_reg);  // print the config off the ADS
	// Power-down
	//This bit determines the channel power mode for the
	//corresponding channel.
	//0 : Normal operation
	//1 : Channel power-down.
	//When powering down a channel, TI recommends that the
	//channel be set to input short by setting the appropriate
	//MUXn[2:0] = 001 of the CHnSET register.
	wreg_val = 0x0 << 7; // 0 - normal operation, 1 - channel power-down
	// PGA gain
	// These bits determine the PGA gain setting.
	// 000 : 1
	// 001 : 2
	// 010 : 4
	// 011 : 6
	// 100 : 8
	// 101 : 12
	// 110 : 24
	// 111 : Do not use
	wreg_val |= 0x0 << 4; // 0 - gain 1, 1 - gain 2, 2 - gain 4, 3 - gain 6, 4 - gain 8, 5 - gain 12, 6 - gain 24
	// SRB2 connection
	// This bit determines the SRB2 connection for the corresponding
	// channel.
	// 0 : Open
	// 1 : Closed
	wreg_val |= 0x0 << 3; // 0 - open, 1 - closed
	// These bits determine the channel input selection.
	// 000 : Normal electrode input
	// 001 : Input shorted (for offset or noise measurements)
	// 010 : Used in conjunction with BIAS_MEAS bit for BIAS
	// measurements.
	// 011 : MVDD for supply measurement
	// 100 : Temperature sensor
	// 101 : Test signal
	// 110 : BIAS_DRP (positive electrode is the driver)
	// 111 : BIAS_DRN (negative electrode is the driver)
	wreg_val |= 0x0;

	ads_wreg(ADS_REG_CH1SET, wreg_val);                
	print_reg = ads_rreg(ADS_REG_CH1SET);             // confirm channel 1 is enabled
	if(print_reg != wreg_val) {
		printf("ERROR: REG_CH1SET FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
		return 0;
	}

	uint8_t ch234_set = wreg_val |  0x1; // This should power down channels 2, 3, and 4
	ads_wreg(ADS_REG_CH2SET, ch234_set);               
	ads_wreg(ADS_REG_CH3SET, ch234_set);                   // enable channel 3 (0x60 = 0110 0000 where 110 is gain 24)
	ads_wreg(ADS_REG_CH4SET, ch234_set);                   // enable channel 4 (0x60 = 0110 0000 where 110 is gain 24)

	// REG_CONFIG2
	wreg_val = 0x6 << 5;  // RESERVED
	wreg_val |= 0x1 << 4; // 0 - test driven exterally, 1 - test driven internally
	wreg_val |= 0x0 << 3; // RESERVED
	wreg_val |= 0x0 << 2; // 0 - CAL_AMP 1 × –(VREFP – VREFN) / 2400,  1 - 2× –(VREFP – VREFN) / 2400
	// Test signal frequency
	// These bits determine the calibration signal frequency.
	// 00 : Pulsed at fCLK / 221
	// 01 : Pulsed at fCLK / 220
	// 10 : Do not use
	// 11 : At dc
	wreg_val |= 0x0 << 0; // 0 - CAL_FREQ fCLK / 2^21, 1 - fCLK / 2^20
	ads_wreg(ADS_REG_CONFIG2, wreg_val);                   // change the config on ADS
	print_reg = ads_rreg(ADS_REG_CONFIG2);             // confirm the config on ADS
	if(print_reg != wreg_val) {
		printf("ERROR: REG_CONFIG2 FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
		return 0;
	}

	// REG_CONFIG3
	wreg_val = 0xE0;
	ads_wreg(ADS_REG_CONFIG3, wreg_val);                   // change the config on ADS
	print_reg = ads_rreg(ADS_REG_CONFIG3);             // confirm the config on ADS
	if(print_reg != wreg_val) {
		printf("ERROR: REG_CONFIG3 FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
		return 0;
	}

	printf("--ADS1299 Comm. OK!--\r\n");

	// Print out all the registers
	for (rreg = 0x00; rreg < 0x18; rreg = rreg + 0x01) {
		print_reg = ads_rreg(rreg);
		printf("ADS_REG: %x = %x\r\n", rreg, print_reg);
	}
	return 1;
}

int main(void) {
	uint32_t i, j;
	unsigned char print_reg;
	uint32_t Nsample = 8;

	int32_t adc_data[32];


	/***    Initialize SCuM     ****/
	initialize_mote();

	// Set the banks to use the ARM core's GPIOs for all of our available
	// pins
	
	printf("Initializing the channel calibration.\n");
	//if (!channel_cal_init(START_COARSE_CODE, END_COARSE_CODE)) {
	//    return 0;
	//}

	crc_check();
	perform_calibration();


	GPO_control(6, 6, 6, 6); 
	GPI_control(0, 0, 0, 0);

	GPI_enable_clr(5); // /ADS_RESET
	GPO_enable_set(5); 
	// On Rev. 3 we use GPIO0 as the debug output
	GPI_enable_clr(0);
	GPO_enable_set(0);


	analog_scan_chain_write();
	analog_scan_chain_load();
	

	gpio_0_set();
	gpio_0_clr();


	printf("Starting the open loop tuning.\n");
	// Start the open loop tuning
	//tx_cal_open_loop();
	
	tuning_tune_radio(&g_tuning_code);
	
	printf("Transmitting on channel %u: (%u, %u, %u).\n",
			IEEE_802_15_4_TX_CHANNEL, g_tuning_code.coarse,
			g_tuning_code.mid, g_tuning_code.fine);
	tuning_tune_radio(&g_tuning_code);
	tuning_tune_radio(&g_tuning_code);
	tuning_tune_radio(&g_tuning_code);
	tuning_tune_radio(&g_tuning_code);
	tuning_tune_radio(&g_tuning_code);
	tuning_tune_radio(&g_tuning_code);


	// Program analog scan chain
	analog_scan_chain_write();
	analog_scan_chain_load();


	printf("Power up ADS1299 NOW!\r\n");
	radio_delayCPUMilliseconds(5);

	/***    Initialize ADS1299     ****/
	ads_init();
	for(i=0; i<100; i++) {
		ads_reset();
		ads_sdatac();
	}

	/***    Configure ADS1299     ***/
	uint8_t ads_init_success = configure_ads1299();
	if(!ads_init_success) {
		printf("ERROR: ADS1299 INIT FAILED\r\n");
		return 0;
	}

	/***    Self Test     ***/
	init_self_test_isr(); // Asynchronously stimulate GPIO1 and GPIO2

	// DEBUG MARKER
	gpio_0_set();
	gpio_0_clr();

	/***    Read ADS data     ****/
	ads_start();
	ads_rdatac();
	uint8_t packet_size = (Nsample*4) + 2;
	while(1) {
		
		// DEBUG MARKER
		gpio_0_set();
		gpio_0_clr();
		for (i = 0; i < (Nsample+1); i++) {
			ads_poll_measurements(&app_vars.ads_measurement[i]);
		}
		
		for (i = 1; i < (Nsample+1); i++) {
			adc_data[i] = app_vars.ads_measurement[i].channel[ACTIVE_ADS_CHANNEL - 1];
			__asm("nop");
			__asm("nop");
			__asm("nop");
			//printf("%d\r\n", adc_data[i]);
			__asm("nop");
			__asm("nop");
			__asm("nop");
		}
		//delay_milliseconds_synchronous(10, 1);
		__asm("nop");
		__asm("nop");
		__asm("nop");
		send_packet(&adc_data[1], packet_size);
		__asm("nop");
		__asm("nop");
		__asm("nop");

		//for(i = 0; i < 500; i++) __asm("nop");
		//radio_delayCPUMilliseconds(10);
		//delay_milliseconds_synchronous(10, 1);
	}
	printf("exit\n");
}
//=========================== public ==========================================

//=========================== private =========================================


