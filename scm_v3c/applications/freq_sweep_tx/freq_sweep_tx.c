/**
\brief This program conducts a freqency sweeping test.

After loading this program, SCuM will send 10 packet on each configuration 
of coarse[5 bits], middle[5 bits] and fine[5 bits]. Meanwhile, an OpenMote
listens on one Channel and print out the freq_offset if it receives a frame.

This program supposes to be run 16 times on for each channel setting on OpenMote
side.
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

#define LENGTH_PACKET       125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_TX_PKT          20+LENGTH_CRC  ///< length of tx packet
#define CHANNEL             11             ///< 11=2.405GHz
#define TIMER_PERIOD        2000           ///< 500 = 1ms@500kHz

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

//=========================== variables =======================================

static const uint8_t payload_identity[] = "test.";

typedef struct {
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
    volatile    bool            sendDone;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_endFrame_tx(uint32_t timestamp);
void     cb_timer(void);

//=========================== main ============================================

int main(void) {
    
    uint32_t calc_crc;

		uint8_t         cfg_coarse_start;
    uint8_t         cfg_mid_start;
    uint8_t         cfg_fine_start;
		uint8_t         cfg_coarse_stop;
    uint8_t         cfg_mid_stop;
    uint8_t         cfg_fine_stop;
    
    uint8_t         i;
    uint8_t         j;
    uint8_t         offset;
		uint8_t					counter;
		
		counter = 0;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    
    radio_setEndFrameTxCb(cb_endFrame_tx);
    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();
    
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
		
		// manual_calibrate(3,21,721,24,17,16,22,13); // manual calibration codes for USB power
		// manual_calibrate(3,26,721,25,14,16,22,39); // manual calibration codes for keithley 1.86V
    optical_calibrate();
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    // configure freq sweep
        
		if (1) { // fixed frequency mode
			cfg_coarse_start = 22;
			cfg_mid_start = 16; //on 14 off 15 --14
			cfg_fine_start = 0; //on 5 off 6 -- 20
			cfg_coarse_stop = cfg_coarse_start + 1;
			cfg_mid_stop = cfg_mid_start + 1;
			cfg_fine_stop = cfg_fine_start + 1;
		} else { // sweep mode
			cfg_coarse_start = 21;
			cfg_mid_start = 0;
			cfg_fine_start = 0;
			cfg_coarse_stop = 23;
			cfg_mid_stop = STEPS_PER_CONFIG;
			cfg_fine_stop = STEPS_PER_CONFIG;
		}
		
    while(1){
				uint8_t         cfg_coarse;
				uint8_t         cfg_mid;
				uint8_t         cfg_fine;
        //memcpy(&app_vars.packet[0],&payload_identity[0],sizeof(payload_identity)-1);
        
        // loop through all configuration
        for (cfg_coarse=cfg_coarse_start;cfg_coarse<cfg_coarse_stop;cfg_coarse++){
            for (cfg_mid=cfg_mid_start;cfg_mid<cfg_mid_stop;cfg_mid += 1){
                for (cfg_fine=cfg_fine_start;cfg_fine<cfg_fine_stop;cfg_fine += 10){
									// titan: 22 20 4 on usb power; 22 13 25 on keithley 1.5V; 22 21 10 on keithley 1.86V // on solar 22 17 0
										int q;
										//for (q = 0; q < 100000; q++) {}			 // 400000 for scum on solar
                    
										printf(
											"coarse=%d, middle=%d, fine=%d\r\n", 
											cfg_coarse,cfg_mid,cfg_fine
                    );
                    j = sizeof(payload_identity)-1;
										app_vars.packet[0] = counter++;
										app_vars.packet[1] = cfg_coarse;
										app_vars.packet[2] = cfg_mid;
										app_vars.packet[3] = cfg_fine;
                    
                    for (i=0;i<NUMPKT_PER_CFG;i++) {
                        
                        radio_loadPacket(app_vars.packet, LEN_TX_PKT);
                        LC_FREQCHANGE(cfg_coarse,cfg_mid,cfg_fine);
                        radio_txEnable();
                        rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
                        app_vars.sendDone = false;
											
                        while (app_vars.sendDone==false);
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_endFrame_tx(uint32_t timestamp){
    
    radio_rfOff();
    
    app_vars.sendDone = true;
    
}

void    cb_timer(void) {
    
    // Tranmit the packet
    radio_txNow();
}

