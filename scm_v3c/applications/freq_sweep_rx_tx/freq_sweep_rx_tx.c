/**
\brief This program has multiple uses: rx sweep/fixed, tx sweep/fixed, and rx then tx (or any sort of combination)
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================
	
#define OPTICAL_CALIBRATE 1// 1 if should optical calibrate, 0 if manual
#define MODE 0 // 0 for tx, 1 for rx, 2 for rx then tx, ... and more
#define SOLAR_MODE 1// 1 if on solar, 0 if on power supply/usb
#define SEND_OPTICAL 0 // 1 if you want to send it 0 if you don't. You do need to have the correct channel
#define SWEEP_TX 1// 1 if sweep, 0 if fixed
#define SWEEP_RX 1 // 1 if sweep, 0 if fixed

// fixed rx/tx coarse, mid, fine settings used if SWEEP_RX and SWEEP_TX is 0
#define FIXED_LC_COARSE_TX			23
#define FIXED_LC_MID_TX			  0
#define FIXED_LC_FINE_TX			6

#define FIXED_LC_COARSE_RX			22
#define FIXED_LC_MID_RX				23

#define FIXED_LC_FINE_RX				13

// if SWEEP_TX = 0 or SWEEP_RX = 0 then these values define the LC range to sweep. used for both sweeping Rx and Tx
#define SWEEP_COARSE_START 23
#define SWEEP_COARSE_END 24
#define SWEEP_MID_START 0
#define SWEEP_MID_END 2
#define SWEEP_FINE_START 0
#define SWEEP_FINE_END 32

// fixed optical calibration settings to use if OPTICAL_CALIBRATE is 0
//3  | 24 | 22 | 16 | 16 | 22 | 26 | 0  | 22 | 26 | 11 
//#define HF_COARSE 3
//#define HF_FINE 24
//#define RC2M_COARSE 22
//#define RC2M_FINE 15
//#define RC2M_SUPERFINE 15
//#define IF_COARSE 22
//#define IF_FINE 21
//#define HF_COARSE 3
//#define HF_FINE 24
//#define RC2M_COARSE 22
//#define RC2M_FINE 15
//#define RC2M_SUPERFINE 16
//#define IF_COARSE 22
//#define IF_FINE 21
//#define HF_COARSE 3
//#define HF_FINE 31
//#define RC2M_COARSE 22
//#define RC2M_FINE 18
//#define RC2M_SUPERFINE 14
//#define IF_COARSE 22
//#define IF_FINE 41

// 1.82V
#define HF_COARSE 3
#define HF_FINE 24
#define RC2M_COARSE 22
#define RC2M_FINE 17
#define RC2M_SUPERFINE 15
#define IF_COARSE 22
#define IF_FINE 31

//1.85V
//#define HF_COARSE 3
//#define HF_FINE 23
//#define RC2M_COARSE 22
//#define RC2M_FINE 17
//#define RC2M_SUPERFINE 15
//#define IF_COARSE 22
//#define IF_FINE 41

//1.89V
//#define HF_COARSE 3
//#define HF_FINE 24
//#define RC2M_COARSE 22
//#define RC2M_FINE 17
//#define RC2M_SUPERFINE 15
//#define IF_COARSE 22
//#define IF_FINE 41

// 1.

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

//=========================== variables =======================================

char tx_packet[LEN_TX_PKT];

//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);
void		 sweep_send_packet(void);
void		 sweep_receive_packet(void);
void		 repeat_rx_tx(radio_mode_t radio_mode, uint8_t should_sweep, int total_packets);
void		 onRx(uint8_t *packet, uint8_t packet_len);

//=========================== main ============================================
	
int main(void) {
    uint32_t calc_crc;
    uint8_t         offset;
		int i;
    
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
				repeat_rx_tx(TX, SWEEP_TX, 1);// number means to send one packet. if you change to negative infinity. usually want to try for two
				repeat_rx_tx(RX, SWEEP_RX, 1);
			
				printf("entering low power state indefinitely. Power cycle before reprogramming.\n");
				
				low_power_mode();
				while(1);
				break;
			case 3: //tx then rx NONSOLAR
				repeat_rx_tx(TX, SWEEP_TX, 1);
			
				for (i = 0; i < 100000; i++){}
			
				repeat_rx_tx(RX, SWEEP_RX, 1);
				break;
			case 4: // idle
				while(1) {
					printf("idle\n");
				}
				break;
			case 5: // idle low power, but radio on
				printf("low power idle radio on\n");
				radio_rxEnable();
				low_power_mode();
				while (1) {
					for (i = 0; i < 10000; i++){
					}
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
	
	unsigned packet_counter = 0;
	
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
					
					if (SOLAR_MODE) {
						low_power_mode();
						for (i = 0; i < 5000; i++) {}
						normal_power_mode();
						
						//for (i = 0; i < 900000; i++) {}
						//printf("rx/tx send/receive\n");
					}
					
					if (should_sweep) {
						printf( "coarse=%d, middle=%d, fine=%d\r\n", cfg_coarse, cfg_mid, cfg_fine);
					}
					
					for (i=0;i<NUMPKT_PER_CFG;i++) {
						if (radio_mode == RX) {
							receive_packet(cfg_coarse, cfg_mid, cfg_fine);
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
								tx_packet[4] = (uint8_t)RC2M_coarse;
								tx_packet[5] = (uint8_t)RC2M_fine;
								tx_packet[6] = (uint8_t)RC2M_superfine;
								tx_packet[7] = (uint8_t)IF_coarse;
								tx_packet[8] = (uint8_t)IF_fine;
								tx_packet[9] = (uint8_t) 0;
								tx_packet[10] = cfg_coarse;
								tx_packet[11] = cfg_mid;
								tx_packet[12] = cfg_fine;
								tx_packet[13] = (uint8_t) 0;	
								tx_packet[14] = (uint8_t) 0;
								tx_packet[15] = (uint8_t) 0;
								tx_packet[16] = (uint8_t) 0;
								tx_packet[17] = (uint8_t) 0;
								tx_packet[18] = (uint8_t) 0;
								tx_packet[19] = (uint8_t) 0;
								tx_packet[20] = (uint8_t) 0;
								tx_packet[21] = (uint8_t) 0;
							
								send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet);
							}
						}

						// stop after send or received a certain number of times
						packet_counter += 1;
						if (packet_counter == total_packets) {
							printf("stopping %s\n", radio_mode_string);
							return;
						}
					}
				} 
			}
		}
	}
}

void onRx(uint8_t *packet, uint8_t packet_len) {
	//printf("packet first item: %d\n", packet[0]); //there are 20 or 22 packets and they are uint8_t
	if (packet[1]==23)
	{
		//sara(100, 2,1);
	}
}
