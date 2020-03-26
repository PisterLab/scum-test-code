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


//=========================== variables =======================================


//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);
void		 send_packet(void);
void		 receive_packet(void);

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
		
    sweep_send_packet();
}

//=========================== public ==========================================

//=========================== private =========================================