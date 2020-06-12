/**
\brief This program has multiple uses: rx sweep/fixed, tx sweep/fixed, and rx then tx (or any sort of combination)
*/

#include <string.h>

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
#define CLOCK_RATIO_VS_TEMP_SLOPE -30.715 // dummy value
#define CLOCK_RATIO_VS_TEMP_OFFSET 1915.142 // y intercept // dummy value
#define FINE_CODE_VS_TEMP_SLOPE 1.194 // dummy value
#define FINE_CODE_VS_TEMP_OFFSET -12.439 // y intercept // dummy value
#define TEMP_MEASURE_DURATION_MILLISECONDS 100 // duration for which we should measure the 2MHz and 32kHz clocks in order to measure the temperature

// RADIO DEFINES
#define OPTICAL_CALIBRATE 1 // 1 if should optical calibrate, 0 if manual
#define MODE 8 // 0 for tx, 1 for rx, 2 for rx then tx, ... and more (see switch statement below)
#define SOLAR_MODE 0 // 1 if on solar, 0 if on power supply/usb
#define SOLAR_DELAY 5000 // for loop iteration count for delay while on solar between radio periods (5000 = ~3 seconds at 500KHz clock, which is low_power_mode)
#define SEND_OPTICAL 0 // 1 if you want to send it 0 if you don't. You do need to have the correct channel
#define SWEEP_TX 1 // 1 if sweep, 0 if fixed
#define SWEEP_RX 1 // 1 if sweep, 0 if fixed
#define SEND_ACK 0 // 1 if we should send an ack after packet rx and 0 otherwise
#define NUM_ACK 1 // number of acknowledgments to send upon receiving a packet

// fixed rx/tx coarse, mid, fine settings used if SWEEP_RX and SWEEP_TX is 0
#define FIXED_LC_COARSE_TX			22
#define FIXED_LC_MID_TX			 		22
#define FIXED_LC_FINE_TX				9

#define FIXED_LC_COARSE_RX			22
#define FIXED_LC_MID_RX				  22
#define FIXED_LC_FINE_RX				19

// if SWEEP_TX = 0 or SWEEP_RX = 0 then these values define the LC range to sweep. used for both sweeping Rx and Tx
#define SWEEP_COARSE_START 22
#define SWEEP_COARSE_END 23
#define SWEEP_MID_START 15
#define SWEEP_MID_END 32
#define SWEEP_FINE_START 0
#define SWEEP_FINE_END 32

// 1.82V
#define HF_COARSE 3
#define HF_FINE 24
#define RC2M_COARSE 22
#define RC2M_FINE 17
#define RC2M_SUPERFINE 15
#define IF_COARSE 22
#define IF_FINE 31

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

//=========================== variables =======================================

char tx_packet[LEN_TX_PKT]; // contains the contents of the packet to be transmitted
unsigned packet_counter = 0; // number of times we have transmitted or attempted to receive
int rx_count = 0; // count of the number of packets actually received
// need_to_send_ack: set to true if should send ack right after the receive completes.
// Use SEND_ACK to determine whether to send an acknowledgemnets or not.
bool need_to_send_ack = false;
double temp;

uint32_t count_2M;
uint32_t count_32k;

//=========================== prototypes ======================================

void		 repeat_rx_tx(radio_mode_t radio_mode, uint8_t should_sweep, int total_packets);
void		 radio_delay(void);
void		 onRx(uint8_t *packet, uint8_t packet_len);
void		 temp_radio_adjust_loop(void);
void		 radio_temp_adjust_callback(void);
void     read_temp_callback(void);
void		 delay_milliseconds_test_loop(void);

//=========================== main ============================================
	
int main(void) {
    uint32_t calc_crc;
    uint8_t         offset;
		int i,j;
    
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
				repeat_rx_tx(TX, SWEEP_TX, -1);
				break;
			case 1: //rx indefinite
				repeat_rx_tx(RX, SWEEP_RX, -1);
				break;

			case 2: //single tx then single rx then low power
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
				while(1)
				{
					low_power_mode();
					sara_start(300,300);
					//(200,200); //second argument affects rate of GPIO 4 and 5 and 6. GPIO 6 is clock. Set to (300, 250) for 96 Hz to test motors
					//GPIO_REG__OUTPUT=0x0000;
					
					for(i=0;i<100;i++);
					sara_release(300);
					for(i=0;i<100;i++);
				}
				break;
			case 7: // temperature compensated transmit loop 
				temp_radio_adjust_loop();
				while (1) {} // since this is interrupt based we need some loop to stall in while we wait for interrupts
				break;
			case 8: // test of RF TIMER delay milliseconds function
				delay_milliseconds_test_loop();
				while (1) {} // since this is interrupt based we need some loop to stall in while we wait for interrupts
			default:
				printf("Invalid mode\n");
				break;
		}
}

//=========================== public ==========================================

//=========================== private =========================================

/* Repeateadly sends or receives packets depending on radio_mode
   Will sweep or be at fixed frequency depending on repeat_mode
	 total_packets indicates the number of packets to send/receive, -1 if infinite*/
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
	
	char* radio_mode_string;
	
	if (radio_mode == TX) {
		radio_mode_string = "transmit";
	} else {
		radio_mode_string = "receive";
	}

	if (!should_sweep) { // fixed frequency mode
		if (radio_mode == TX) {
			cfg_coarse_start = FIXED_LC_COARSE_TX;
			cfg_mid_start = FIXED_LC_MID_TX;
			cfg_fine_start = FIXED_LC_FINE_TX;
		} else {
			cfg_coarse_start = FIXED_LC_COARSE_RX;
			cfg_mid_start = FIXED_LC_MID_RX;
			cfg_fine_start = FIXED_LC_FINE_RX;
		}
				
		cfg_coarse_stop = cfg_coarse_start + 1;
		cfg_mid_stop = cfg_mid_start + 1;
		cfg_fine_stop = cfg_fine_start + 1;
		
		printf("Fixed %s at c:%u m:%u f:%u\n", radio_mode_string, cfg_coarse_start, cfg_mid_start, cfg_fine_start);
	} else { // sweep mode
		cfg_coarse_start = SWEEP_COARSE_START;
		cfg_coarse_stop = SWEEP_COARSE_END;
		cfg_mid_start = SWEEP_MID_START;
		cfg_mid_stop = SWEEP_MID_END;
		cfg_fine_start = SWEEP_FINE_START;
		cfg_fine_stop = SWEEP_FINE_END;
		
		printf("Sweeping %s\n", radio_mode_string);
	}
	
	while(1){
		// loop through all configuration
		for (cfg_coarse=cfg_coarse_start;cfg_coarse<cfg_coarse_stop;cfg_coarse+=1){
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
									
									send_ack(FIXED_LC_COARSE_TX, FIXED_LC_MID_TX, FIXED_LC_FINE_TX);
								}
								
								need_to_send_ack = false;
							}
						}
						else { // TX mode
							if (!SEND_OPTICAL) {
								tx_packet[0] = (uint8_t) packet_counter;
								tx_packet[1] = cfg_coarse;
								tx_packet[2] = cfg_mid;
								tx_packet[3] = cfg_fine;
								
								send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet);
							} else {
								HF_CLOCK_coarse     = scm3c_hw_interface_get_HF_CLOCK_coarse();
								HF_CLOCK_fine       = scm3c_hw_interface_get_HF_CLOCK_fine();
								RC2M_coarse         = scm3c_hw_interface_get_RC2M_coarse();
								RC2M_fine           = scm3c_hw_interface_get_RC2M_fine();
								RC2M_superfine      = scm3c_hw_interface_get_RC2M_superfine();
								IF_coarse           = scm3c_hw_interface_get_IF_coarse();
								IF_fine             = scm3c_hw_interface_get_IF_fine();
								
								tx_packet[0] = (uint8_t) packet_counter;
								tx_packet[1] = (uint8_t) 0;
								tx_packet[2] = (uint8_t)HF_CLOCK_coarse;
								tx_packet[3] = (uint8_t)HF_CLOCK_fine;
//								tx_packet[4] = (uint8_t)RC2M_coarse;
//								tx_packet[5] = (uint8_t)RC2M_fine;
//								tx_packet[6] = (uint8_t)RC2M_superfine;
//								tx_packet[7] = (uint8_t)IF_coarse;
//								tx_packet[8] = (uint8_t)IF_fine;
//								tx_packet[9] = (uint8_t) 0;
//								tx_packet[10] = cfg_coarse;
//								tx_packet[11] = cfg_mid;
//								tx_packet[12] = cfg_fine;
										
								send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet);
							}
						}

						// stop after send or received a certain number of times
						packet_counter += 1;
						
						if (total_packets > 0) {
							//printf("packet %d out of %d\n", packet_counter, total_packets);
						}
						
						if (packet_counter == total_packets) {
							//printf("stopping %s\n", radio_mode_string);
							return;
						}
					}
				} 
			}
		}
	}
}

void radio_delay(void) {
	uint16_t j;
	
	if (SOLAR_MODE) {
		low_power_mode();
		for (j = 0; j < 5000; j++) {}
		normal_power_mode();
	}
}

void onRx(uint8_t *packet, uint8_t packet_len) {
	uint8_t i;
	uint16_t j;
	
	rx_count += 1;
	printf("received a total of %d packets\n", rx_count);
	
	if (SEND_ACK)
		need_to_send_ack = true;
	
	//printf("packet first item: %d\n", packet[0]); //there are 20 or 22 packets and they are uint8_t
	if (packet[1]==1)
	{
		//printf("Got message to actuate gripper!!!\n");
		//sara(100, 2,1);
	}
}

/* Loop that will get a temperature estimate, which will then trigger a calculation of the
 * proper fine code to use based on that temperature.
 */
void temp_radio_adjust_loop(void) {
	measure_temperature(radio_temp_adjust_callback, TEMP_MEASURE_DURATION_MILLISECONDS);
}

/* Once called count_2M and count_32k will already be set
	with the the count that accumulated from the start of the RF TIMER running (from the start of
	the call to measure_temperature). The duration of the timer is specified in the call to 
	measure_temperature. This function will set the temep variable based on the hard coded
	linear model relating the ratio of the 2M/32kHz clocks and a temperature estimate. */
void read_temp_callback(void) {
	double ratio;
	
	// calculate the ratio between the 2M and the 32kHZz clocks
	ratio = fix_double(fix_div(fix_init(count_2M), fix_init(count_32k)));
	
	// using our linear model that we fit based on fixed point temperature measurement
	// we can determine an estimate for temperature
	temp = CLOCK_RATIO_VS_TEMP_SLOPE * ratio + CLOCK_RATIO_VS_TEMP_OFFSET;
	
	printf("2M: %u, 32kHz: %u, Ratio: %f, Temp: %d\n", count_2M, count_32k, ratio, (int) temp);
}

/* Setting this function as the callback to read_temperature will update the temp variable through
	an estimated temperature set by calling read_temp_callback function. Then this function will ajust
	the LC based on the linear model relating fine codes and temperature. Relies on FIXED_LC_COARSE_TX
	and FIXED_LC_MID_TX to represent the coarse and mid codes at room temperature. Will then determine
	the correct fine code based on a hardcoded linear model. */
void radio_temp_adjust_callback(void) {
	uint8_t LC_fine_code;
	
	// process the clock count data that has just been obtained to come up with a temperature estimate
	read_temp_callback();
	
	// based on the temp value, come up with an estimate for a fine code based on the linear model
	LC_fine_code = FINE_CODE_VS_TEMP_SLOPE * temp + FINE_CODE_VS_TEMP_OFFSET;
	
	// set the packet contents and transmit a packet at the determined LC code
	tx_packet[1] = packet_counter++;
	tx_packet[2] = (uint8_t) FIXED_LC_COARSE_TX;
	tx_packet[3] = (uint8_t) FIXED_LC_MID_TX;
	tx_packet[4] = (uint8_t) LC_fine_code;
	tx_packet[5] = (uint8_t) temp;
	send_packet(FIXED_LC_COARSE_TX, FIXED_LC_MID_TX, LC_fine_code, tx_packet);
	
	// wait for a certain period of time before starting process of reading in tempterature and transmitting
	delay_milliseconds(temp_radio_adjust_loop, 3000);
}

/* A function used for testing the delay milliseconds functionality. */
void delay_milliseconds_test_loop(void) {
	printf("delay over!\n");
	delay_milliseconds(delay_milliseconds_test_loop, 3000);
}
