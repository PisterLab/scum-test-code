/**
\brief This program has multiple uses: rx sweep/fixed, tx sweep/fixed, and rx then tx (or any sort of combination)
*/

#include <string.h>
#include <stdio.h>
#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"
#include "zappy2.h"
#include "temperature.h"
#include "fixed-point.h"

//=========================== defines =========================================

// TEMPERATURE DEFINES
// LOG_TEMPERATURE_AFTER_OPTICAL_CALIBRATION variable exists in optical.c
// and indicates whether you want to conduct a measurements of the 32kHz and 2MHz clocks
// after optical calibration has completed for the purpose of measuring temperature
// the optical programmer (if programmed to do so) will continue to send optical
// calibration pulses as a reference.
#define CLOCK_RATIO_VS_TEMP_SLOPE -19.293 //-19.667 //-30.715 // dummy value
#define CLOCK_RATIO_VS_TEMP_OFFSET 1298.134 // 1317.511 //1915.142 // y intercept // dummy value
#define CLOCK_RATIO_VS_FINE_CODE_SLOPE -10.567 //-24.33
#define CLOCK_RATIO_VS_FINE_CODE_OFFEST 710.411 // 1592.204 // y intercept // dummy value
#define TEMP_MEASURE_DURATION_MILLISECONDS 100 // duration for which we should measure the 2MHz and 32kHz clocks in order to measure the temperature

// RADIO DEFINES
// make sure to set LEN_TX_PKT and LEN_RX_PKT in radio.h
#define OPTICAL_CALIBRATE 1 // 1 if should optical calibrate, 0 if manual

#define MODE 1 // 0 for tx, 1 for rx, 2 for rx then tx, ... and more (see switch statement below)
#define SOLAR_MODE 1 // 1 if on solar, 0 if on power supply/usb (this enables/disables the SOLAR_DELAY delay)
//NEED TO UNCOMMENT IN TX? radio_delay
#define SOLAR_DELAY 25000 // for loop iteration count for delay while on solar between radio periods (5000 = ~3 seconds at 500KHz clock, which is low_power_mode)
#define SWEEP_TX 1 // 1 if sweep, 0 if fixed
#define SWEEP_RX 1 // 1 if sweep, 0 if fixed
#define SEND_ACK 1 // 1 if we should send an ack after packet rx and 0 otherwise
#define NUM_ACK 10 // number of acknowledgments to send upon receiving a packet

// fixed rx/tx coarse, mid, fine settings used if SWEEP_RX and SWEEP_TX is 0.
// these default values are used to set a variable that will represent the fixed LC values
// the reason these are used to set variables is that these FIXED values can later change
// in the case of events like a temperature chage, which could trigger a change in the
// "fixed" value we operate it. In other words by fixed we just mean that we aren't sweeping;
// the LC values that we transmit or receive at may change (for example compensated due to 
// temperature changes), but we just won't sweep the LC.

#define DEFAULT_FIXED_LC_COARSE_TX		22
#define DEFAULT_FIXED_LC_MID_TX			 20
#define DEFAULT_FIXED_LC_FINE_TX		14

#define DEFAULT_FIXED_LC_COARSE_RX			21
#define DEFAULT_FIXED_LC_MID_RX				  17
#define DEFAULT_FIXED_LC_FINE_RX				22

// if SWEEP_TX = 0 or SWEEP_RX = 0 then these values define the LC range to sweep. used for both sweeping Rx and Tx
#define SWEEP_COARSE_START_TX 20
#define SWEEP_COARSE_END_TX 21
#define SWEEP_MID_START_TX 27
#define SWEEP_MID_END_TX 28
#define SWEEP_FINE_START_TX 3
#define SWEEP_FINE_END_TX 9

#define SWEEP_COARSE_START_RX 19
#define SWEEP_COARSE_END_RX 23
#define SWEEP_MID_START_RX 0
#define SWEEP_MID_END_RX 31
#define SWEEP_FINE_START_RX 0
#define SWEEP_FINE_END_RX 31

// SARA 30.56C 1.778V
//#define HF_COARSE 3
//#define HF_FINE 18
//#define RC2M_COARSE 14
//#define RC2M_FINE 17
//#define RC2M_SUPERFINE 13
//#define IF_COARSE 22
//#define IF_FINE 20

// SARA 38.5C 1.778V
//#define HF_COARSE 3
//#define HF_FINE 19
//#define RC2M_COARSE 15
//#define RC2M_FINE 14
//#define RC2M_SUPERFINE 17
//#define IF_COARSE 22
//#define IF_FINE 18

// SARA 1.81V 39.5C
#define HF_COARSE 3
#define HF_FINE 19
#define RC2M_COARSE 15
#define RC2M_FINE 13
#define RC2M_SUPERFINE 16
#define IF_COARSE 22
#define IF_FINE 20

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

typedef enum {
	PREDEFINED     = 0x01,
	LC_CODES		   = 0x02,
	LC_CODES_ASCII = 0x03,
	OPTICAL_VALS   = 0x04,
	TEMP					 = 0x05,
	COUNT_2M_32K	 = 0x06,
	COMPRESSED_CLOCK = 0x07,
	CUSTOM = 0x08
} tx_packet_content_source_t;

//=========================== variables =======================================

// RADIO VARIABLES
// Defines what/when data is set for the repeat_rx_tx function (see this function for details on values).
// note that the value here is simlpy a default (fallback valuee) and in many of this program's "modes" 
// this value is additionally set
tx_packet_content_source_t tx_packet_data_source = LC_CODES;

uint8_t tx_packet[LEN_TX_PKT]; // contains the contents of the packet to be transmitted
int rx_count = 0; // count of the number of packets actually received
// need_to_send_ack: set to true if should send ack right after the receive completes.
// Use SEND_ACK to determine whether to send an acknowledgemnets or not.
bool need_to_send_ack = false;

uint8_t fixed_lc_coarse_tx = DEFAULT_FIXED_LC_COARSE_TX;
uint8_t fixed_lc_mid_tx = DEFAULT_FIXED_LC_MID_TX;
uint8_t fixed_lc_fine_tx = DEFAULT_FIXED_LC_FINE_TX;

uint8_t fixed_lc_coarse_rx = DEFAULT_FIXED_LC_COARSE_RX;
uint8_t fixed_lc_mid_rx = DEFAULT_FIXED_LC_MID_RX;
uint8_t fixed_lc_fine_rx = DEFAULT_FIXED_LC_FINE_RX;

// TEMPERATURE VARIABLES
double temp;
uint32_t count_2M;
uint32_t count_32k;

// HACK SOLUTION TO GET ACK TO SEND RX CODES NEED TO FIX LATER
uint8_t custom_tx_packet[LEN_TX_PKT];

uint8_t temp_adjusted_fine_code; // will be set by adjust_tx_fine_with_temp function

//=========================== prototypes ======================================

void		 repeat_rx_tx(radio_mode_t radio_mode, uint8_t should_sweep, int total_packets);
void		 radio_delay(void);
void		 onRx(uint8_t *packet, uint8_t packet_len);
void		 adjust_tx_fine_with_temp(void);
void		 delay_milliseconds_test_loop(void);

//=========================== main ============================================
	
int main(void) {
    uint32_t calc_crc;
    uint8_t         offset;
		int i,j;
		short counter;
		int gripper_result;
    
    printf("Initializing...");
	
    // Check CRC to ensure there were no errors during optical programming
    printf("\r\n-------------------\r\n");
    printf("Validating program integrity..."); 
    
    calc_crc = crc32c(0x0000,CODE_LENGTH);
    
    if(calc_crc == CRC_VALUE){
        printf("CRC OK\r\n");
    } else{
        printf("\r\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\r\n");
        while(1);
    }
		
		// Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
		
		radio_setCallbacks(onRx);

    if (OPTICAL_CALIBRATE) {
			optical_calibrate();
		} else {
			manual_calibrate(HF_COARSE, HF_FINE, RC2M_COARSE, RC2M_FINE, RC2M_SUPERFINE, IF_COARSE, IF_FINE);
		}

		switch (MODE) {
			case 0: // tx indefinite
				tx_packet_data_source = LC_CODES;
				repeat_rx_tx(TX, SWEEP_TX, -1);
				break;
			case 1: //rx indefinite
				repeat_rx_tx(RX, SWEEP_RX, -1);
				break;
			case 2: //single tx then single rx then low power
				tx_packet_data_source = LC_CODES;
				printf("going into switching mode!\n");
				
				for (i = 0; i < 100; i++) {
					repeat_rx_tx(TX, SWEEP_TX, 1);// number means to send one packet. if you change to negative infinity. usually want to try for two
					//for (j = 0; j < 1000000; j++) {}
					repeat_rx_tx(RX, SWEEP_RX, 1);
				}
			
				//printf("entering low power state indefinitely. Power cycle before reprogramming.\n");
				printf("done!\n");
				
				//low_power_mode();
				while(1);
				break;
			case 3: //tx then rx NONSOLAR
				tx_packet_data_source = LC_CODES;
				repeat_rx_tx(TX, SWEEP_TX, 1);
			
				for (i = 0; i < 100000; i++){}
			
				repeat_rx_tx(RX, SWEEP_RX, 1);
				break;
			case 4: // idle normal power used for doing nothing while letting optical interrupts happen for tmperature mode
				while (1) {}
				break;
			case 5: // idle low power
				low_power_mode();
				while (1) {}
				break;
			case 6: //turn on go to low power and after you are done closing send packet
				low_power_mode();
				while(1)
				//for(j=0;j<10;j++)
				{	
					sara_start(1500,60);
					//(200,200); //second argument affects rate of GPIO 4 and 5 and 6. GPIO 6 is clock. Set to (300, 250) for 96 Hz to test motors
					//GPIO_REG__OUTPUT=0x0000;
					
					for(i=0;i<100;i++);
					sara_release(300);
					for(i=0;i<100;i++);
					printf("toggle!\n");
				}
				while(1)
				{}
				break;
			case 7: // temperature compensated transmit loop
				while (1) {
					temp = get_2MHz_32k_ratio_temp_estimate(TEMP_MEASURE_DURATION_MILLISECONDS, CLOCK_RATIO_VS_TEMP_SLOPE, CLOCK_RATIO_VS_TEMP_OFFSET);
					adjust_tx_fine_with_temp();
				
					tx_packet_data_source = TEMP;
										
					// will not sweep since that is exactly what we are trying to get rid of by compensating with temperature
					repeat_rx_tx(TX, 0, 1); // send 1 packet(s)
				}
				break;
			case 8: // test of RF TIMER delay milliseconds function
				delay_milliseconds_test_loop();
				while (1) {} // since this is interrupt based we need some loop to stall in while we wait for interrupts
				break;
			case 9: // sprintf transmit test
				sprintf(tx_packet, "2MHz: %d 32kHz: %d", 280000, 50000);
							
				tx_packet_data_source = LC_CODES;
				repeat_rx_tx(TX, SWEEP_TX, -1);
			
				while (1) {}
				break;
			case 10: // take one measurement of the clocks for temperature measurement and then continuously transmit the result for a certain period, then repeat
				// note: since this is a sweep, we need to set the sweep coarse and mid code start and end fixed values to be such that the coarse and mid
				// are always fixed. The fine code sweep should be 0 to 32
				while (1) {
					// Get the counts for 2MHz and 32kHz clocks
					read_counters_duration(TEMP_MEASURE_DURATION_MILLISECONDS);
					
					count_2M = scm3c_hw_interface_get_count_2M();
					count_32k = scm3c_hw_interface_get_count_32k();
					
					// Now continuously transimt the clock counts.
					printf("2M: %u, 32kHz: %u\n", count_2M, count_32k);
		
					tx_packet_data_source = COMPRESSED_CLOCK;
					repeat_rx_tx(TX, 1, 32); // in this case we will always sweep since with different temps we don't know which LC codes will work, so set to 1 not 0
				}
				
				break;
			case 11: // continuously measure and log 2MHz and 32kHz
				while (1) {
					// Get the counts for 2MHz and 32kHz clocks
					read_counters_duration(TEMP_MEASURE_DURATION_MILLISECONDS);
					
					count_2M = scm3c_hw_interface_get_count_2M();
					count_32k = scm3c_hw_interface_get_count_32k();
					
					printf("2M: %u, 32kHz: %u\n", count_2M, count_32k);
				}
				break;
			case 12: // continuously measure and print temperature
				while (1) {
					// Get the counts for 2MHz and 32kHz clocks
					temp = get_2MHz_32k_ratio_temp_estimate(TEMP_MEASURE_DURATION_MILLISECONDS, CLOCK_RATIO_VS_TEMP_SLOPE, CLOCK_RATIO_VS_TEMP_OFFSET);
					
					printf("2M: %u, 32kHz: %u, Temp: %d\n", count_2M, count_32k, (int) temp);					
				}
				break;
			case 13: // measure divider current draw
				while (1) {
					printf("switching\n");
					ANALOG_CFG_REG__10 = 0x0058;
					delay_milliseconds_synchronous(2000);
					printf("switching\n");
					ANALOG_CFG_REG__10 = 0x0018;
					delay_milliseconds_synchronous(2000);
				}
				break;
			case 14: // contact sensor development// Alex wrote this code 
				
				printf("GPIO ON\n");
				//enable GPIOs
				GPO_enables(0xFFFF);
				GPI_enables(0xFFFF);
				GPI_control(0,0,0,0);//sets GPI to cortex registers
				GPO_control(6,6,6,6); //GPIO 0 -15 //sets GP0 to cortex registers
				// Program analog scan chain
				analog_scan_chain_write();
				analog_scan_chain_load();
				
				//set gpio to 3.3V 
				GPIO_REG__OUTPUT=0xFFFF;
//				if((GPIO_REG__INPUT | 0xFDFF) == 0xFFFF)
//				{
//					printf("GPIO9 high\n");
//				}
				//disable GPIOs
				GPO_enables(0x0000);
				
				// Program analog scan chain
				analog_scan_chain_write();
				analog_scan_chain_load();
				
				printf("GPIO OFF\n");
				//check til GPI turns to zero;
				counter=0; 
				while(1){
					  gripper_result = ~(GPIO_REG__INPUT | 0xFDFF)==0xffff0200;
						printf("%x\n", GPIO_REG__INPUT);
						if(gripper_result){
							counter=counter+1;
						}	
						else {
							counter=0;
							
						}
						if(counter>40){
							printf("Gripper Closed %d %d \n", counter, gripper_result);
						}
				}
				break;
			case 15: // SARA final code with rx until packet is received, then send acks, then turn off radio, then operate SARA then loop
				while (1) {
					printf("Attempting to receive a packet\n");
					repeat_rx_tx(RX, SWEEP_RX, 1); // keep looping until we receive a single packet. Sends NUM_ACK acks if SEND_ACK set to 1
					
					// if we reach this point it means that we have received a packet and have (optionally) sent acks.
					printf("packet received. starting SARA toggle!\n");
					// now trigger SARA. ALEX CHECK THE PARAMETERS HERE
					low_power_mode();
					sara_start(1500,60);
					//(200,200); //second argument affects rate of GPIO 4 and 5 and 6. GPIO 6 is clock. Set to (300, 250) for 96 Hz to test motors
					//GPIO_REG__OUTPUT=0x0000;
					
					for(i=0;i<100;i++);
					sara_release(300);
					for(i=0;i<100;i++);
					printf("toggle!\n");
					normal_power_mode();
				}
			default:
				printf("Invalid mode\n");
				break;
		}
}

//=========================== public ==========================================

//=========================== private =========================================

/* Repeateadly sends or receives packets depending on radio_mode
   Will sweep or be at fixed frequency depending on repeat_mode
	 total_packets indicates the number of packets to send/receive, -1 if infinite.
	 If doing tx, need to set tx_packet_data_source variable before use. */
void repeat_rx_tx(radio_mode_t radio_mode, uint8_t should_sweep, int total_packets) {
	uint8_t         cfg_coarse;
	uint8_t         cfg_mid;
	uint8_t         cfg_fine;
	
	uint8_t         cfg_coarse_start;
	uint8_t         cfg_mid_start;
	uint8_t         cfg_fine_start;
	
	uint8_t         cfg_coarse_stop;
	uint8_t         cfg_mid_stop;
	uint8_t         cfg_fine_stop;
	
	uint8_t HF_CLOCK_fine;
	uint8_t HF_CLOCK_coarse;
	uint8_t RC2M_coarse;
	uint8_t RC2M_fine;
	uint8_t RC2M_superfine;
	uint8_t IF_clk_target;
	uint8_t IF_coarse;
	uint8_t IF_fine;
	
	uint8_t i;
	
	tx_packet_content_source_t saved_tx_packet_data_source;
	
	unsigned packet_counter = 0; // number of times we have transmitted or attempted to receive
	
	char* radio_mode_string;
	
	if (radio_mode == TX) {
		radio_mode_string = "transmit";
	} else {
		radio_mode_string = "receive";
	}

	if (!should_sweep) { // fixed frequency mode
		if (radio_mode == TX) {
			cfg_coarse_start = fixed_lc_coarse_tx;
			cfg_mid_start = fixed_lc_mid_tx;
			cfg_fine_start = fixed_lc_fine_tx;
		} else {
			cfg_coarse_start = fixed_lc_coarse_rx;
			cfg_mid_start = fixed_lc_mid_rx;
			cfg_fine_start = fixed_lc_fine_rx;
		}
				
		cfg_coarse_stop = cfg_coarse_start + 1;
		cfg_mid_stop = cfg_mid_start + 1;
		cfg_fine_stop = cfg_fine_start + 1;
		
		printf("Fixed %s at c:%u m:%u f:%u\n", radio_mode_string, cfg_coarse_start, cfg_mid_start, cfg_fine_start);
	} else { // sweep mode
			if (radio_mode == TX) {
				cfg_coarse_start = SWEEP_COARSE_START_TX;
				cfg_coarse_stop = SWEEP_COARSE_END_TX;
				cfg_mid_start = SWEEP_MID_START_TX;
				cfg_mid_stop = SWEEP_MID_END_TX;
				cfg_fine_start = SWEEP_FINE_START_TX;
				cfg_fine_stop = SWEEP_FINE_END_TX;
			} else {
				cfg_coarse_start = SWEEP_COARSE_START_RX;
				cfg_coarse_stop = SWEEP_COARSE_END_RX;
				cfg_mid_start = SWEEP_MID_START_RX;
				cfg_mid_stop = SWEEP_MID_END_RX;
				cfg_fine_start = SWEEP_FINE_START_RX;
				cfg_fine_stop = SWEEP_FINE_END_RX;
			}
		
		printf("Sweeping %s\n", radio_mode_string);
	}
	
	while(1){
		//printf("looping...\n");
		// loop through all configuration
		for (cfg_coarse=cfg_coarse_start;cfg_coarse<cfg_coarse_stop;cfg_coarse += 1){
			for (cfg_mid=cfg_mid_start;cfg_mid<cfg_mid_stop;cfg_mid += 1){
				for (cfg_fine=cfg_fine_start;cfg_fine<cfg_fine_stop;cfg_fine += 1){										
					int i;
					uint8_t k;
					uint8_t j;
					
					radio_delay();
					
					if (should_sweep) {
						printf( "coarse=%d, middle=%d, fine=%d\r\n", cfg_coarse, cfg_mid, cfg_fine);
					}
					
					for (i=0;i<NUMPKT_PER_CFG;i++) {
						if (radio_mode == RX) {
							receive_packet(cfg_coarse, cfg_mid, cfg_fine);
							
							if (need_to_send_ack) {
								for (j = 0; j < NUM_ACK; j++) {
									radio_delay();
									
									printf("sending ack %d out of %d\n", j + 1, NUM_ACK);
									//send_ack(FIXED_LC_COARSE_TX, FIXED_LC_MID_TX, FIXED_LC_FINE_TX, cfg_coarse, cfg_mid, cfg_fine, j);
									//send_ack(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx, cfg_coarse, cfg_mid, cfg_fine, j);
									
									// send the acks by sweeping through another call to repeat_rx_tx
									saved_tx_packet_data_source = tx_packet_data_source;
									tx_packet_data_source = CUSTOM;
									
									sprintf(custom_tx_packet, "%d %d %d", cfg_coarse, cfg_mid, cfg_fine);
									
									repeat_rx_tx(TX, SWEEP_TX, NUM_ACK);
									
									tx_packet_data_source = saved_tx_packet_data_source;
									//tx_packet_data_source = LC_CODES;
								}
								
								need_to_send_ack = false;
							}
						}
						else { // TX mode
							tx_packet[0] = (uint8_t) packet_counter;
							
							switch (tx_packet_data_source) { // defines how to set packet contents
								case PREDEFINED: // packet content set prior
									break;
								case LC_CODES: // packet content will be LC coarse mid fine 
//									tx_packet[1] = cfg_coarse;
//									tx_packet[2] = cfg_mid;
//									tx_packet[3] = cfg_fine;
								
								  sprintf(tx_packet, "%d %d %d", cfg_coarse, cfg_mid, cfg_fine);
								
									break;
								case OPTICAL_VALS: // packet content will be optical settings
									HF_CLOCK_coarse     = scm3c_hw_interface_get_HF_CLOCK_coarse();
									HF_CLOCK_fine       = scm3c_hw_interface_get_HF_CLOCK_fine();
									RC2M_coarse         = scm3c_hw_interface_get_RC2M_coarse();
									RC2M_fine           = scm3c_hw_interface_get_RC2M_fine();
									RC2M_superfine      = scm3c_hw_interface_get_RC2M_superfine();
									IF_coarse           = scm3c_hw_interface_get_IF_coarse();
									IF_fine             = scm3c_hw_interface_get_IF_fine();
									
									tx_packet[1] = (uint8_t) 0;
									tx_packet[2] = (uint8_t)HF_CLOCK_coarse;
									tx_packet[3] = (uint8_t)HF_CLOCK_fine;
									tx_packet[4] = (uint8_t)RC2M_coarse;
									tx_packet[5] = (uint8_t)RC2M_fine;
									tx_packet[6] = (uint8_t)RC2M_superfine;
									tx_packet[7] = (uint8_t)IF_coarse;
									tx_packet[8] = (uint8_t)IF_fine;
									tx_packet[9] = (uint8_t) 0;
									tx_packet[10] = cfg_coarse;
									tx_packet[11] = cfg_mid;
									tx_packet[12] = cfg_fine;
								
									break;
								case TEMP:
									// format: FF TT.TT
									//sprintf(tx_packet, "%2d %2.2f", cfg_fine, temp);
									
										// had to do it this weird way since the normal double formatting was making the packets stop sending after a while.... I have no clue why
										sprintf(tx_packet, "%02d %d.%02d", cfg_fine, (uint8_t) temp, (uint8_t) ((temp - (uint8_t) temp) * 100));
									
//									tx_packet[1] = (uint8_t) cfg_coarse;
//									tx_packet[2] = (uint8_t) cfg_mid;
//									tx_packet[3] = (uint8_t) cfg_fine;
//									tx_packet[4] = (uint8_t) (int) temp; // two digits left of decimal place
//									tx_packet[5] = (uint8_t) ((temp - (uint8_t)(temp))*100); // two digits right of decimal place
//									tx_packet[6] = (uint8_t) 0;
//									tx_packet[7] = (uint8_t) 0;
//									tx_packet[8] = (uint8_t) 0;
//									tx_packet[9] = (uint8_t) 0;
									
//									tx_packet[0] = 0;
//									tx_packet[1] = cfg_fine;
//									tx_packet[2] = 2;
//									tx_packet[3] = 3;
//									tx_packet[4] = 4;
//									tx_packet[5] = 5;
//									tx_packet[6] = 6;
//									tx_packet[7] = 7;
//									tx_packet[8] = 8;
//									tx_packet[9] = 9;
								
									break;
								case COUNT_2M_32K:
									sprintf(tx_packet, "C:%d M:%d F:%d 2MHz:%d 32kHz:%d", cfg_coarse, cfg_mid, cfg_fine, count_2M, count_32k);
								
									break;
								case LC_CODES_ASCII:
									sprintf(tx_packet, "C:%d M:%d F:%d", cfg_coarse, cfg_mid, cfg_fine);
								
									break;
								case COMPRESSED_CLOCK: // for when on solar and packet length is compressed
									// since we are on solar and the sweep of 32 fine codes will take a while (probably 1.5 minutes)
									// and the temperature properly is changing at a faster rate, we can just take a new temp measurement
									// every time we transmit
									read_counters_duration(TEMP_MEASURE_DURATION_MILLISECONDS);
									count_2M = scm3c_hw_interface_get_count_2M();
									count_32k = scm3c_hw_interface_get_count_32k();
									printf("2M: %u, 32kHz: %u\n", count_2M, count_32k);
								
									// mode for sending the 2MHz clocks and the 32kHz clock as well as fine code
									// send in a compressed format taking the form of:
									// 0: packet counter 1: fine 2: 2MHz[5-4] 3: 2MHz[3-2] 4: 2MHz[1-0] 5: 32kHz[3-2] 6: 23kHz[1-0]
									// where the indices indicate the bits taken from each value (0 is LSB).
									// example: if 2MHz is 179746, then 2: 17, 3: 97, 4: 46
									// The point of this is to make the packet as small as possible to send this information so that
									// we can build a figure that shows the relationship between fine code and clock ratio
//									tx_packet[1] = (uint8_t) cfg_fine;
//									tx_packet[2] = (uint8_t) (count_2M / 100000); // highest digit
//									tx_packet[3] = (uint8_t) ((count_2M / 10000) - (10*tx_packet[2])); // 2nd highest digit
//									tx_packet[4] = (uint8_t) (((count_2M - ((count_2M / 10000) * 10000))) / 100); // middle 2 digits
//									tx_packet[5] = (uint8_t) (count_2M % 100); // lower 2 digits
//									tx_packet[6] = (uint8_t) (count_32k / 100); // upper 2 digits
//									tx_packet[7] = (uint8_t) (count_32k % 100); // lower 2 digits
								
									//FF2222223333; length = 12
									sprintf(tx_packet, "%2d%d%d", cfg_fine, count_2M, count_32k);
									
//									tx_packet[0] = 0;
//									tx_packet[1] = 1;
//									tx_packet[2] = 2;
//									tx_packet[3] = 3;
//									tx_packet[4] = 4;
//									tx_packet[5] = 5;
//									tx_packet[6] = 6;
//									tx_packet[7] = 7;
//									tx_packet[8] = 8;
//									tx_packet[9] = 9;
									

									break;
								case CUSTOM:
									memcpy(&tx_packet[0],custom_tx_packet,LEN_TX_PKT);
								  break;
								default:
									printf("ERROR: unset tx packet content source");
								
									break;
							}
							
							send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet);
						}

						// stop after send or received a certain number of times
						packet_counter += 1;
						
						if (total_packets > 0) {
							//printf("packet %d out of %d\n", packet_counter, total_packets);
						}
						
						if (SOLAR_MODE == 1 & should_sweep == 0) {
//							printf("radio event\n");
						}
						
						//printf("packet counter: %d, rx_count: %d\n", packet_counter, rx_count);
						
						if ((radio_mode == TX && packet_counter == total_packets) || (radio_mode == RX && rx_count == total_packets)) {
							//printf("stopping as we have received/transmitted %d packets\n", packet_counter);
							rx_count = 0;
							return;
						}
					}
				} 
			}
		}
	}
}

// TODO: change this function to use delay_milliseconds to use RF TIMER, which is independent of 
// HCLK so that the timing is more more consistent regardless of clock speed (won't matter whether
// you are in lower or normal power state).
void radio_delay(void) {
	uint16_t j;
	
	if (SOLAR_MODE) {
		//low_power_mode();
		for (j = 0; j < SOLAR_DELAY; j++) {}
		//normal_power_mode();
	}
}

void onRx(uint8_t *packet, uint8_t packet_len) {
	uint8_t i;
	uint16_t j;
	
	rx_count += 1;
	//printf("received a total of %d packets\n", rx_count);
	
	if (SEND_ACK)
		need_to_send_ack = true;
	
	//printf("packet first item: %d\n", packet[0]); //there are 20 or 22 packets and they are uint8_t
//	if (packet[1]==1) // THIS IS OUTDATED CODE
//	{
		//printf("Got message to actuate gripper!!!\n");
		//sara(100, 2,1);
//	}
}

/* This function will update the fixed fine code based on a linear model relating the 2MHz and 32kHz clock ratios
 * and fine codes that properly transmit.
 */
void adjust_tx_fine_with_temp(void) {
	// calculate the ratio between the 2M and the 32kHZz clocks
	double ratio = fix_double(fix_div(fix_init(count_2M), fix_init(count_32k)));
	
	fixed_lc_fine_tx = CLOCK_RATIO_VS_FINE_CODE_SLOPE * ratio + CLOCK_RATIO_VS_FINE_CODE_OFFEST;
}

/* A function used for testing the delay milliseconds functionality. */
void delay_milliseconds_test_loop(void) {
	printf("delay over!\n");
	delay_milliseconds(delay_milliseconds_test_loop, 3000);
}
