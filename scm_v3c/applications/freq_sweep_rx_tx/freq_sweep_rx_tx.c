/**
\brief This program conducts a freqency sweeping test where scum will alternate between Tx and Rx.
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))
	
#define OPTICAL_CALIBRATE 	1 // 1 if should optical calibrate, 0 if manual

#define HF_COARSE	3
#define HF_FINE	26
#define LC_CODE 721
#define RC2M_COARSE 25
#define RC2M_FINE 12
#define RC2M_SUPERFINE 16
#define IF_COARSE 22
#define IF_FINE			41

#define MODE 0 // 0 for tx, 1 for rx, 2 for rx then tx
#define SOLAR_MODE					0 // 1 if on solar, 0 if on power supply/usb

#define FIXED_LC_COARSE_RX			22 //22
#define FIXED_LC_MID_RX				22 //25
#define FIXED_LC_FINE_RX				22 //2

#define FIXED_LC_COARSE_TX			22
#define FIXED_LC_MID_TX			  23
#define FIXED_LC_FINE_TX				4

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

typedef enum {
	SWEEP = 0,
	FIXED = 1
} repeat_mode_t;

//=========================== variables =======================================

repeat_mode_t tx_repeat_mode = SWEEP;
repeat_mode_t rx_repeat_mode = SWEEP;

//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);
void		 sweep_send_packet(void);
void		 sweep_receive_packet(void);
void		 repeat_rx_tx(radio_mode_t radio_mode, repeat_mode_t repeat_mode);

//=========================== main ============================================

int main(void) {
    uint32_t calc_crc;
    uint8_t         offset;
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    
		radio_setCallbacks();
		radio_enable_interrupts();
	
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
    
    // Debug output
    //printf("\r\nCode length is %u bytes",code_length); 
    //printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);    
    
    //printf("done\r\n");
    
    if (OPTICAL_CALIBRATE) {
			optical_calibrate();
		} else {
			manual_calibrate(HF_COARSE, HF_FINE, LC_CODE, RC2M_COARSE, RC2M_FINE, RC2M_SUPERFINE, IF_COARSE, IF_FINE);
		}
		
		if (MODE == 0) {
			repeat_rx_tx(TX, tx_repeat_mode);
		} else if(MODE == 1) {
			repeat_rx_tx(RX, rx_repeat_mode);
		} else if(MODE == 2) {
			
		} else {
			printf("Invalid mode\n");
		}
}

//=========================== public ==========================================

//=========================== private =========================================

/* Repeateadly sends or receives packets depending on radio_mode
   Will sweep or be at fixed frequency depending on repeat_mode */
void repeat_rx_tx(radio_mode_t radio_mode, repeat_mode_t repeat_mode) {
	uint8_t         cfg_coarse;
	uint8_t         cfg_mid;
	uint8_t         cfg_fine;
	
	uint8_t         cfg_coarse_start;
	uint8_t         cfg_mid_start;
	uint8_t         cfg_fine_start;
	
	uint8_t         cfg_coarse_stop;
	uint8_t         cfg_mid_stop;
	uint8_t         cfg_fine_stop;
	
	char* radio_mode_string;

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
		
		if (radio_mode == TX) {
			radio_mode_string = "transmit";
		} else {
			radio_mode_string = "receive";
		}
		
		cfg_coarse_stop = cfg_coarse_start + 1;
		cfg_mid_stop = cfg_mid_start + 1;
		cfg_fine_stop = cfg_fine_start + 1;
		
		printf("Fixed %s at c:%u m:%u f:%u\n", radio_mode_string, cfg_coarse_start, cfg_mid_start, cfg_fine_start);
	} else { // sweep mode
		cfg_coarse_start = 22;
		cfg_mid_start = 20;
		cfg_fine_start = 0;
		cfg_coarse_stop = 23;
		cfg_mid_stop = STEPS_PER_CONFIG;
		cfg_fine_stop = STEPS_PER_CONFIG;
	}
	
	while(1){
		// loop through all configuration
		for (cfg_coarse=cfg_coarse_start;cfg_coarse<cfg_coarse_stop;cfg_coarse++){
			for (cfg_mid=cfg_mid_start;cfg_mid<cfg_mid_stop;cfg_mid += 1){
				for (cfg_fine=cfg_fine_start;cfg_fine<cfg_fine_stop;cfg_fine += 1){										
					int i;
					
					if (SOLAR_MODE) {									
						for (i = 0; i < 200000; i++) {}			 // 200000 for scum on solar
					}
					
					if (repeat_mode == SWEEP) {
						printf(
						"coarse=%d, middle=%d, fine=%d\r\n", 
						cfg_coarse,cfg_mid,cfg_fine
						);
					}
					
					for (i=0;i<NUMPKT_PER_CFG;i++) {
						if (radio_mode == RX) {
							receive_packet(cfg_coarse, cfg_mid, cfg_fine);
						} else {
							send_packet(cfg_coarse, cfg_mid, cfg_fine);
						}
					}
				}
			}
		}
	}
}
