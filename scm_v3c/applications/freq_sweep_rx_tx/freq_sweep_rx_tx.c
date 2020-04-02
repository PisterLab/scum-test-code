/**
\brief This program has multiple uses: rx sweep/fixed, tx sweep/fixed, and rx then tx (or any sort of combination)
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"
#include "wireless_config.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))

//hard code here ############################################
//#define HF_COARSE 3
//#define HF_FINE 25
//#define LC_CODE 721
//#define RC2M_COARSE 22
//#define RC2M_FINE 16
//#define RC2M_SUPERFINE 14
//#define IF_COARSE 22
//#define IF_FINE 24



// hard code 03/31/2020 from rx 
#define HF_COARSE 3
#define HF_FINE 22
#define LC_CODE 721
#define RC2M_COARSE 22
#define RC2M_FINE 18
#define RC2M_SUPERFINE 14
#define IF_COARSE 22
#define IF_FINE 7
//hard code 04/01/2020 from tx
//#define HF_COARSE 3
//#define HF_FINE 23
//#define LC_CODE 721
//#define RC2M_COARSE 24
//#define RC2M_FINE 13
//#define RC2M_SUPERFINE 15
//#define IF_COARSE 22
//#define IF_FINE 29
//#define HF_COARSE 3
//#define HF_FINE 22
//#define LC_CODE 721
//#define RC2M_COARSE 22
//#define RC2M_FINE 15
//#define RC2M_SUPERFINE 15
//#define IF_COARSE 22
//#define IF_FINE 27

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32
#define TX_PACKET_LENGTH 4

// fixed rx/tx coarse, mid, fine settings used if OPTICAL_CALIBRATE is 0
#define FIXED_LC_COARSE_RX			22
#define FIXED_LC_MID_RX				24
#define FIXED_LC_FINE_RX				13

#define FIXED_LC_COARSE_TX			22
#define FIXED_LC_MID_TX			  29
#define FIXED_LC_FINE_TX				6


typedef enum {
	SWEEP = 1,
	FIXED = 0
} repeat_mode_t;

//=========================== variables =======================================

repeat_mode_t tx_repeat_mode = SWEEP;//change this to FIXED for solar 
repeat_mode_t rx_repeat_mode = SWEEP;
uint8_t tx_packet[TX_PACKET_LENGTH];

//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);
void		 sweep_send_packet(void);
void		 sweep_receive_packet(void);
void		 repeat_rx_tx(radio_mode_t radio_mode, repeat_mode_t repeat_mode, int total_packets);
void		 onRx(uint8_t *packet, uint8_t packet_len);

//=========================== main ============================================
	uint8_t HF_CLOCK_fine;
	uint8_t HF_CLOCK_coarse;
	uint8_t RC2M_coarse;
	uint8_t RC2M_fine;
	uint8_t RC2M_superfine;
	uint8_t IF_clk_target;
	uint8_t IF_coarse;
	uint8_t IF_fine;
int main(void) {
    uint32_t calc_crc;
    uint8_t         offset;



   

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
			manual_calibrate(HF_COARSE, HF_FINE, LC_CODE, RC2M_COARSE, RC2M_FINE, RC2M_SUPERFINE, IF_COARSE, IF_FINE);
		}
		
		//low_power_mode();
		
		switch (MODE) {
			case 0: // tx
				repeat_rx_tx(TX, tx_repeat_mode, -1);
				break;
			case 1: //rx
				repeat_rx_tx(RX, rx_repeat_mode, -1);
				break;
			case 2: //tx then rx
				repeat_rx_tx(TX, tx_repeat_mode, 1);// number means to send one packet. if you change to negative infinity. usually want to try for two
				repeat_rx_tx(RX, rx_repeat_mode, 1);
				
				low_power_mode();
				while(1);
				break;
			case 3: //idle
				while(1) {
					printf("idle\n");
				}
				break;
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
void repeat_rx_tx(radio_mode_t radio_mode, repeat_mode_t repeat_mode, int total_packets) {
	uint8_t         cfg_coarse;
	uint8_t         cfg_mid;
	uint8_t         cfg_fine;
	
	uint8_t         cfg_coarse_start;
	uint8_t         cfg_mid_start;
	uint8_t         cfg_fine_start;
	
	uint8_t         cfg_coarse_stop;
	uint8_t         cfg_mid_stop;
	uint8_t         cfg_fine_stop;
	uint8_t         t1 =22;
	uint8_t         t2=29;
	uint8_t         t3=6;
	
	int packet_counter = 0;
	
	char* radio_mode_string;
	
	if (radio_mode == TX) {
		radio_mode_string = "transmit";
	} else {
		radio_mode_string = "receive";
	}

	if (repeat_mode == FIXED) { // fixed frequency mode
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
		cfg_coarse_start = 22;
		cfg_mid_start = 22;
		cfg_fine_start = 0;
		cfg_coarse_stop = 23;
		cfg_mid_stop = STEPS_PER_CONFIG;
		cfg_fine_stop = STEPS_PER_CONFIG;
		
		printf("Sweeping %s\n", radio_mode_string);
	}
	
	while(1){
		// loop through all configuration
		for (cfg_coarse=cfg_coarse_start;cfg_coarse<cfg_coarse_stop;cfg_coarse++){
			for (cfg_mid=cfg_mid_start;cfg_mid<cfg_mid_stop;cfg_mid += 1){
				for (cfg_fine=cfg_fine_start;cfg_fine<cfg_fine_stop;cfg_fine += 1){										
					int i;
					
					if (SOLAR_MODE) {
						low_power_mode();
						for (i = 0; i < 5000; i++) {}
						normal_power_mode();
						
						//for (i = 0; i < 900000; i++) {}
						printf("event\n");
					}
					
					if (repeat_mode == SWEEP) {
						printf( "coarse=%d, middle=%d, fine=%d\r\n", cfg_coarse, cfg_mid, cfg_fine);
					}
					
					for (i=0;i<NUMPKT_PER_CFG;i++) {
						if (radio_mode == RX) {
							receive_packet(cfg_coarse, cfg_mid, cfg_fine);
						} else if (!SEND_OPTICAL) {
							tx_packet[0] = (uint8_t) packet_counter;
							tx_packet[1] = cfg_coarse;
							tx_packet[2] = cfg_mid;
							tx_packet[3] = cfg_fine;
							
							send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet, TX_PACKET_LENGTH);
						}
						else {
							HF_CLOCK_coarse     = scm3c_hw_interface_get_HF_CLOCK_coarse();
							HF_CLOCK_fine       = scm3c_hw_interface_get_HF_CLOCK_fine();
							RC2M_coarse         = scm3c_hw_interface_get_RC2M_coarse();
							RC2M_fine           = scm3c_hw_interface_get_RC2M_fine();
							RC2M_superfine      = scm3c_hw_interface_get_RC2M_superfine();
							IF_clk_target       = scm3c_hw_interface_get_IF_clk_target();
							IF_coarse           = scm3c_hw_interface_get_IF_coarse();
							IF_fine             = scm3c_hw_interface_get_IF_fine();
							tx_packet[0] = (uint8_t)HF_CLOCK_fine;
							tx_packet[1] = t1;
							tx_packet[2] = t2;
							tx_packet[3] = t3;
//							tx_packet[1] = HF_CLOCK_coarse;
//							tx_packet[2] = HF_CLOCK_fine;
//							tx_packet[3] = RC2M_coarse;
//							tx_packet[1] = RC2M_superfine;
//							tx_packet[2] = IF_coarse;
//							tx_packet[3] = IF_fine;
							
							
							send_packet(cfg_coarse, cfg_mid, cfg_fine, tx_packet,TX_PACKET_LENGTH);//hardcoded TX_PACKET_LENGTH to 7
						}
						
						
						packet_counter += 1;
						if (packet_counter == total_packets) {
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
