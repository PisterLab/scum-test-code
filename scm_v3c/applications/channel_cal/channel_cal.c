/**
\brief This program should find all (or many) of a given SCuM's 802.15.4 channels with the help of an OpenMote
*/

#include <string.h>
#include <stdio.h>
#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
//#include "ble.h"
#include "optical.h"

//=========================== defines =========================================

// RADIO DEFINES
// make sure to set LEN_TX_PKT and LEN_RX_PKT in radio.h
#define OPTICAL_CALIBRATE 1 // 1 if should optical calibrate, 0 if settings are initialized by hand

// CODES FOR DEAD SOLDIER BOARD 8 CHANNEL 11
#define DEFAULT_FIXED_LC_COARSE_TX		23
#define DEFAULT_FIXED_LC_MID_TX			  22
#define DEFAULT_FIXED_LC_FINE_TX		  12

// CODES FOR DEAD SOLDIER BOARD 8 CHANNEL 11
#define DEFAULT_FIXED_LC_COARSE_RX			23
#define DEFAULT_FIXED_LC_MID_RX					16
#define DEFAULT_FIXED_LC_FINE_RX				14

// range of sweep values for channel search - this range can be dramatically reduced if you want to find fewer than all 16 channels
#define SWEEP_COARSE_START_TX 22
#define SWEEP_COARSE_END_TX 35
#define SWEEP_MID_START_TX 1
#define SWEEP_MID_END_TX 31
#define SWEEP_FINE_START_TX 1
#define SWEEP_FINE_END_TX 31

#define SWEEP_COARSE_START_RX 22
#define SWEEP_COARSE_END_RX 25
#define SWEEP_MID_START_RX 0
#define SWEEP_MID_END_RX 31
#define SWEEP_FINE_START_RX 0
#define SWEEP_FINE_END_RX 31

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
	
#define LENGTH_CRC      2
#define LEN_TX_PKT          10+LENGTH_CRC  ///< length of tx packet //annecdotally length 7 packet didn't work, but length 8 did... maybe odd packet lengths don't work????
#define LEN_RX_PKT          16+LENGTH_CRC  ///< length of rx packet


//=========================== variables =======================================

// RADIO VARIABLES
uint8_t tx_packet[LEN_TX_PKT]; // contains the contents of the packet to be transmitted
uint8_t rx_timeout_occurred = 0;

// IF estimate
uint32_t current_IF_estimate = 0;

int rx_count = 0; // count of the number of packets actually received
// need_to_send_ack: set to true if should send ack right after the receive completes.
// Use SEND_ACK to determine whether to send an acknowledgemnets or not.
bool need_to_send_ack = false;

// temporary variables to store which settings to use in TX/RX at a given time
uint8_t fixed_lc_coarse_tx = DEFAULT_FIXED_LC_COARSE_TX;
uint8_t fixed_lc_mid_tx = DEFAULT_FIXED_LC_MID_TX;
uint8_t fixed_lc_fine_tx = DEFAULT_FIXED_LC_FINE_TX;

uint8_t fixed_lc_coarse_rx = DEFAULT_FIXED_LC_COARSE_RX;
uint8_t fixed_lc_mid_rx = DEFAULT_FIXED_LC_MID_RX;
uint8_t fixed_lc_fine_rx = DEFAULT_FIXED_LC_FINE_RX;

// SCuM's channel settings
uint8_t	scum_rx_channel_buffer[128][3]; // columns: coarse, mid, fine
uint8_t	scum_tx_channel_codes[16*4][4]; // row: up to four sets of candidate codes per channel, column: coarse, mid, fine
uint8_t scum_rx_channel_codes[16*5][4]; // row: up to four sets of candidate codes per channel, column: coarse, mid, fine

//uint8_t temp_adjusted_fine_code; // will be set by adjust_tx_fine_with_temp function

// scum channel calibration variables
uint8_t scum_tx_or_rx; // zero for SCuM transmitting, one for SCuM receiving (and ack-ing).

//=========================== prototypes ======================================

void		 radio_delay(void);
void		 onRx(uint8_t *packet, uint8_t packet_len);
void		 onRx_endFrame(void);
void		 test_rf_timer_callback(void);
void 		 increment_RX_code(void);
void		 increment_TX_code(void);

//=========================== main ============================================
	
int main(void) {
    uint32_t calc_crc;
		int i,j;
	
	  uint8_t cfg_coarse_rx;
		uint8_t cfg_mid_rx;
		uint8_t cfg_fine_rx;
	
		uint8_t cfg_coarse_tx;
		uint8_t	cfg_mid_tx;
		uint8_t	cfg_fine_tx;
    
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
		
		radio_setRxCb(onRx);
		
    if (OPTICAL_CALIBRATE) {
			//optical_calibrate();
			perform_calibration();
		}

		scum_tx_or_rx = 0;

		radio_delay();
		//receive_packet(SWEEP_COARSE_START_RX, SWEEP_MID_START_RX, SWEEP_FINE_START_RX, 1);
		LC_FREQCHANGE(SWEEP_COARSE_START_RX, SWEEP_MID_START_RX, SWEEP_FINE_START_RX);
		receive_packet_length(1, true);
	
		fixed_lc_coarse_tx = SWEEP_COARSE_START_TX;
		fixed_lc_mid_tx = SWEEP_MID_START_TX;
		fixed_lc_fine_tx = SWEEP_FINE_START_TX;
	
		while (1) {
				if (scum_tx_or_rx == 0) { // TX mode
					
						// disable timeout interrupt:
						//rftimer_disable_interrupts(7);
						rftimer_disable_interrupts_by_id(7);
								
						radio_delay();
						// format packet to contain the transmit code
						tx_packet[0] = 1;
						tx_packet[6] = fixed_lc_coarse_tx;
						tx_packet[7] = fixed_lc_mid_tx;
						tx_packet[8] = fixed_lc_fine_tx;
						
						// send a packet
						//send_packet(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx, tx_packet);
						LC_FREQCHANGE(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx);
						send_packet(tx_packet, LEN_TX_PKT);
															
						// increment TX code
						fixed_lc_fine_tx++;
						if (fixed_lc_fine_tx > SWEEP_FINE_END_TX) {
								fixed_lc_mid_tx++;
								if (fixed_lc_mid_tx > SWEEP_MID_END_TX) {
										fixed_lc_coarse_tx++;
								}
						}
						
						if ((fixed_lc_fine_tx>=SWEEP_FINE_END_TX)&&(fixed_lc_mid_tx>=SWEEP_MID_END_TX)&&(fixed_lc_coarse_tx>=SWEEP_COARSE_END_TX)) { // done sweeping, switch to RX
								scum_tx_or_rx = 1;
								fixed_lc_coarse_rx = SWEEP_COARSE_START_RX;
								fixed_lc_mid_rx = SWEEP_MID_START_RX;
								fixed_lc_fine_rx = SWEEP_FINE_START_RX;
							
								// set up the timeout timer:
								//rftimer_set_callback(test_rf_timer_callback, 7);
								rftimer_set_callback_by_id(test_rf_timer_callback, 7);
								//rftimer_setCompareIn(rftimer_readCounter()+20*500, 7);
								rftimer_setCompareIn_by_id(rftimer_readCounter()+20*500, 7);
						}
						
						if (fixed_lc_fine_tx > SWEEP_FINE_END_TX) {
								fixed_lc_fine_tx = SWEEP_FINE_START_TX;
						}
						if (fixed_lc_mid_tx > SWEEP_MID_END_TX) {
								fixed_lc_mid_tx = SWEEP_MID_START_TX;
						}
				}
				
				else if (scum_tx_or_rx == 1) { // RX+ack mode
						radio_delay();
					
						if (need_to_send_ack == 1){
								if (tx_packet[4] == 0x00) {
										for (i=0;i<2;i++) {
												radio_delay();
												//send_packet(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx, tx_packet);
												LC_FREQCHANGE(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx);
												send_packet(tx_packet, LEN_TX_PKT);										}
								}
								else if (tx_packet[4] == 0xFF) {
										for (i=0;i<3;i++) {
												radio_delay();
												//send_packet(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx, tx_packet);
												LC_FREQCHANGE(fixed_lc_coarse_tx, fixed_lc_mid_tx, fixed_lc_fine_tx);
												send_packet(tx_packet, LEN_TX_PKT);
												radio_delay();
												radio_delay();
												radio_delay();
										}
								}
								need_to_send_ack = 0;
								//rftimer_setCompareIn(rftimer_readCounter()+500*50, 7); // reset timeout timer
								rftimer_setCompareIn_by_id(rftimer_readCounter()+500*50, 7);
						}
						else if (need_to_send_ack == 0) {
								radio_delay();
								//receive_packet(fixed_lc_coarse_rx, fixed_lc_mid_rx, fixed_lc_fine_rx, LEN_RX_PKT);
								LC_FREQCHANGE(fixed_lc_coarse_rx, fixed_lc_mid_rx, fixed_lc_fine_rx);
								receive_packet_length(LEN_RX_PKT, true);
						}

						if ((fixed_lc_fine_rx==SWEEP_FINE_START_RX)&&(fixed_lc_mid_rx==SWEEP_MID_START_RX)&&(fixed_lc_coarse_rx>SWEEP_COARSE_END_RX)) { // switch back to TX mode
								scum_tx_or_rx = 2;
								fixed_lc_coarse_tx = SWEEP_COARSE_START_TX;
								fixed_lc_mid_tx = SWEEP_MID_START_TX;
								fixed_lc_fine_tx = SWEEP_FINE_START_TX;
								//rftimer_disable_interrupts(7);
								rftimer_disable_interrupts_by_id(7);
						}
				}
				
				if (scum_tx_or_rx == 2) {
						// disable timer
						//rftimer_disable_interrupts(7);
						rftimer_disable_interrupts_by_id(7);
					
						// print results of search
						printf("TX packet results:\r\n");
						for(i=0;i<64;i++) {
								printf("%d %d %d, ch. %d\r\n",scum_tx_channel_codes[i][0], scum_tx_channel_codes[i][1], scum_tx_channel_codes[i][2], scum_tx_channel_codes[i][3]);
						}
						printf("RX packet results:\r\n");
						for(i=0;i<80;i++) {
								printf("%d %d %d, ch. %d\r\n",scum_rx_channel_codes[i][0], scum_rx_channel_codes[i][1], scum_rx_channel_codes[i][2],scum_rx_channel_codes[i][3]);
						}
						scum_tx_or_rx = 3;
				}
				
		};

		printf("why am I here\r\n");
		
		while (1) {};
}

//=========================== public ==========================================

//=========================== private =========================================


// TODO: change this function to use an RF Timer compare register
void radio_delay(void) {
	uint16_t j;
	for (j = 0; j < 5; j++) {}
}

void onRx(uint8_t *packet, uint8_t packet_len) {
	
		uint8_t 	i;
		uint16_t 	j;
		uint8_t		k;
		uint8_t current_channel;

		// get IF estimate
		current_IF_estimate = read_IF_estimate();
	
		// turn off radio
		radio_rfOff();
		
		// store the RX codes in the buffer
		scum_rx_channel_buffer[rx_count][0] = fixed_lc_coarse_rx;
		scum_rx_channel_buffer[rx_count][1] = fixed_lc_mid_rx;
		scum_rx_channel_buffer[rx_count][2] = fixed_lc_fine_rx;
	
		current_channel = packet[1];
		fixed_lc_coarse_tx = packet[2];
		fixed_lc_mid_tx = packet[3];
		fixed_lc_fine_tx = packet[4];
		
		// default to NOT requesting a channel change on openmote
		tx_packet[4] = 0x00;
	
		for (k=0;k<3;k++) {
				scum_tx_channel_codes[(current_channel-11)*4][k] = packet[k+2];
				scum_tx_channel_codes[(current_channel-11)*4+1][k] = packet[k+5];
				scum_tx_channel_codes[(current_channel-11)*4+2][k] = packet[k+8];
				scum_tx_channel_codes[(current_channel-11)*4+3][k] = packet[k+11];
		}
		scum_tx_channel_codes[(current_channel-11)*4][3] = current_channel;
		scum_tx_channel_codes[(current_channel-11)*4+1][3] = current_channel;
		scum_tx_channel_codes[(current_channel-11)*4+2][3] = current_channel;
		scum_tx_channel_codes[(current_channel-11)*4+3][3] = current_channel;
		
		if (packet[5] == 0) {
				fixed_lc_coarse_tx = packet[2];
				fixed_lc_mid_tx = packet[3];
				fixed_lc_fine_tx = packet[4];
		}
		else {
				fixed_lc_coarse_tx = packet[5];
				fixed_lc_mid_tx = packet[6];
				fixed_lc_fine_tx = packet[7];
		}
			
		// print the codes for debug purposes:
		for (k=0;k<12;k++) {
				printf("%d ", scum_tx_channel_codes[(current_channel-11)*4+k/4][k%4]);
		}
		printf("\r\n");
		printf("IF estimate: %d\r\n",current_IF_estimate);
		
		if ((current_IF_estimate < 525)&&(current_IF_estimate > 475)) { // a good fit for a receive code
				printf("rx chan found: %d %d %d\r\n", fixed_lc_coarse_rx, fixed_lc_mid_rx, fixed_lc_fine_rx);
			
				// store in the RX channel table
				scum_rx_channel_codes[rx_count][0] = fixed_lc_coarse_rx;
				scum_rx_channel_codes[rx_count][1] = fixed_lc_mid_rx;
				scum_rx_channel_codes[rx_count][2] = fixed_lc_fine_rx;
				scum_rx_channel_codes[rx_count][3] = current_channel;
			
				// increment channel table counter
				rx_count += 1;
			
				// check if this channel is done
				if (fixed_lc_fine_rx <= 9) {
						// modify TX ACK packet to include a "change channel" request on the openmote - 0xFF in the 4th bit
						tx_packet[4] = 0xFF;
						printf("finished with this channel\r\n");
				}
				else {
						tx_packet[4] = 0x00;
						printf("not finished with this channel\r\n");
				}
			
				// increment mid code
				fixed_lc_fine_rx = SWEEP_FINE_START_RX;
				fixed_lc_mid_rx++;
				if (fixed_lc_mid_rx > SWEEP_MID_END_RX) {
						fixed_lc_mid_rx = SWEEP_MID_START_RX;
						fixed_lc_coarse_rx++;
				}
				
		}
		else if (fixed_lc_fine_rx >= 30) { // received OK packets at/near end of fine range, store it anyways:
				printf("rx chan found: %d %d %d\r\n", fixed_lc_coarse_rx, fixed_lc_mid_rx, fixed_lc_fine_rx);
				
				scum_rx_channel_codes[rx_count][0] = fixed_lc_coarse_rx;
				scum_rx_channel_codes[rx_count][1] = fixed_lc_mid_rx;
				scum_rx_channel_codes[rx_count][2] = fixed_lc_fine_rx;
				
				// increment channel table counter
				rx_count += 1;
			
				// increment normally
				fixed_lc_fine_rx++;
				if (fixed_lc_fine_rx > SWEEP_FINE_END_RX) {
						fixed_lc_fine_rx = SWEEP_FINE_START_RX;
						fixed_lc_mid_rx++;
				}
				if (fixed_lc_mid_rx > SWEEP_MID_END_RX) {
						fixed_lc_mid_rx = SWEEP_MID_START_RX;
						fixed_lc_coarse_rx++;
				}
		}
		
		else { // continue sweeping through codes
				// increment RC code
				fixed_lc_fine_rx++;
				if (fixed_lc_fine_rx > SWEEP_FINE_END_RX) {
						fixed_lc_fine_rx = SWEEP_FINE_START_RX;
						fixed_lc_mid_rx++;
				}
				if (fixed_lc_mid_rx > SWEEP_MID_END_RX) {
						fixed_lc_mid_rx = SWEEP_MID_START_RX;
						fixed_lc_coarse_rx++;
				}
		}

		radio_delay();

		need_to_send_ack = true;
}

void test_rf_timer_callback(void) {

		// timeout indicator
		rx_timeout_occurred = 1;
	
		// increment receive code
		increment_RX_code();
		
		// restart timer
		//rftimer_enable_interrupts(7);
		
		rftimer_enable_interrupts_by_id(7);
		rftimer_enable_interrupts();
		//rftimer_setCompareIn(rftimer_readCounter() + 500*20, 7);
		rftimer_setCompareIn_by_id(rftimer_readCounter() + 500*20, 7);
	
		if ((fixed_lc_coarse_rx>=SWEEP_COARSE_END_RX)&&(fixed_lc_mid_rx>=SWEEP_MID_END_RX)&&(fixed_lc_fine_rx>=SWEEP_FINE_END_RX)) {
				scum_tx_or_rx = 2;
				fixed_lc_coarse_tx = SWEEP_COARSE_START_TX;
				fixed_lc_mid_tx = SWEEP_MID_START_TX;
				fixed_lc_fine_tx = SWEEP_FINE_START_TX;
		}
		
		//printf("timed out on %d %d %d\n",fixed_lc_coarse_rx, fixed_lc_mid_rx, fixed_lc_fine_rx);
}

void increment_RX_code(void) {
		if (fixed_lc_fine_rx >= SWEEP_FINE_END_RX) {
				fixed_lc_fine_rx = SWEEP_FINE_START_RX;

				if (fixed_lc_mid_rx == SWEEP_MID_END_RX) {
						fixed_lc_mid_rx = SWEEP_MID_START_RX;
						fixed_lc_coarse_rx++;
				}
				else {
						fixed_lc_mid_rx++;
				}
		}
		else {
				fixed_lc_fine_rx = fixed_lc_fine_rx+1;
		}
}

void increment_TX_code(void) {
		fixed_lc_fine_tx++;
		
		if (fixed_lc_fine_tx > SWEEP_FINE_END_TX) {
				fixed_lc_fine_tx = SWEEP_FINE_START_TX;
			
				fixed_lc_mid_tx++;
		
				if (fixed_lc_mid_tx > SWEEP_MID_END_TX) {
						fixed_lc_mid_tx = SWEEP_MID_START_TX;
				}
		}
}