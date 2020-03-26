/**
\brief This program conducts a freqency sweeping test.

After loading this program, SCuM will turn on radio to listen on each 
configuration of coarse[5 bits], middle[5 bits] and fine[5 bits] for 15ms.
Meanwhile, an OpenMote will send NUMPKT_PER_CFG packet on one channel every
4ms. SCuM will print out the pkt it received at each config settings.

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

#define CRC_VALUE           (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH         (*((unsigned int *) 0x0000FFF8))
    
#define LENGTH_PACKET       125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_RX_PKT          20+LENGTH_CRC  ///< length of rx packet

#define TIMER_PERIOD        3000          ///< 500 = 1ms@500kHz //default = 7500 = 15 ms

#define NUMPKT_PER_CFG      1
#define STEPS_PER_CONFIG    32

#define OPTICAL_CALIBRATE 	0 // 1 if should optical calibrate, 0 if manual
#define SOLAR_MODE					1 // 1 if on solar, 0 if on power supply/usb
#define SHOULD_SWEEP				0 // set to 1 to sweep, set to 0 to work at fixed LC frequency (with settigns defined right below)
#define FIXED_LC_COARSE			22 //22
#define FIXED_LC_MID				23 //25
#define FIXED_LC_FINE				23 //2

//=========================== variables =======================================

typedef struct {
                uint8_t         packet[LENGTH_PACKET];
                uint8_t         packet_len;
                int8_t          rxpk_rssi;
                uint8_t         rxpk_lqi;
    
    volatile    bool            rxpk_crc;
    // a flag to mark when to change configure
    volatile    bool            changeConfig;
    // a flag to avoid change configure during receiving frame
    volatile    bool            rxFrameStarted; 
    
    volatile    uint32_t        IF_estimate;
    volatile    uint32_t        LQI_chip_errors;
    volatile    uint32_t        cdr_tau_value;
    
                uint8_t         cfg_coarse;
                uint8_t         cfg_mid;
                uint8_t         cfg_fine;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
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
	
		int HF_coarse = 3;
		int HF_fine = 26;
		int LC_code = 721;
		int RC2M_coarse = 22;
		int RC2M_fine = 17;
		int RC2M_superfine = 14;
		int IF_coarse = 22;
		int IF_fine = 31;
		
    memset(&app_vars,0,sizeof(app_vars_t));

    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();

    radio_setStartFrameRxCb(cb_startFrame_rx);
    radio_setEndFrameRxCb(cb_endFrame_rx);
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
    
    // Debug output
    //printf("\r\nCode length is %u bytes",code_length); 
    //printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);    
    
    //printf("done\r\n");
    
		if (OPTICAL_CALIBRATE) {
			optical_calibrate();
		} else {
			manual_calibrate(HF_coarse, HF_fine, LC_code, RC2M_coarse, RC2M_fine, RC2M_superfine, IF_coarse, IF_fine);
		}

    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    // configure 
		
		if (SHOULD_SWEEP == 0) { // fixed frequency mode
			cfg_coarse_start = FIXED_LC_COARSE;
			cfg_mid_start = FIXED_LC_MID;
			cfg_fine_start = FIXED_LC_FINE;
			cfg_coarse_stop = cfg_coarse_start + 1;
			cfg_mid_stop = cfg_mid_start + 1;
			cfg_fine_stop = cfg_fine_start + 1;
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
        for (app_vars.cfg_coarse=cfg_coarse_start;app_vars.cfg_coarse<cfg_coarse_stop;app_vars.cfg_coarse++){
						if (SHOULD_SWEEP) {
							printf("coarse=%d\r\n", app_vars.cfg_coarse);
						}
						
            for (app_vars.cfg_mid=cfg_mid_start;app_vars.cfg_mid<cfg_mid_stop;app_vars.cfg_mid += 1){
                for (app_vars.cfg_fine=cfg_fine_start;app_vars.cfg_fine<cfg_fine_stop;app_vars.cfg_fine += 1){
										int q;
									
										if (SOLAR_MODE) {
											//for (q = 0; q < 200000; q++) {}
											low_power_mode();
											for (q = 0; q < 2000; q++) {}
											normal_power_mode();
										}
										//radio_rfOff();
										//printf("radio off\n");
										//for (q = 0; q < 1000000; q++) {}			 // 200000 for scum on solar
										//for (q = 0; q < 50000; q++) {}
											
										
										
										//printf("low power\n");
										
										//printf("normal\n");
											
										// golden board: 21 30 29
									// titan's board: 23 3 31
										if (SHOULD_SWEEP && 1) {
											printf(
													"coarse=%d, middle=%d, fine=%d\r\n", 
													app_vars.cfg_coarse,app_vars.cfg_mid,app_vars.cfg_fine
											);
										}
                    for (i=0;i<NUMPKT_PER_CFG;i++) {
												//int h = 0;
                        //while(app_vars.rxFrameStarted == true) {
												//	printf(".");
												//}
												app_vars.rxFrameStarted = false;
                        LC_FREQCHANGE(app_vars.cfg_coarse,app_vars.cfg_mid,app_vars.cfg_fine);
												//printf("radio on\n");
                        radio_rxEnable();
                        radio_rxNow();
                        rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
                        app_vars.changeConfig = false;
                        while (app_vars.changeConfig==false);
                    }
                }
            }
        }
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_startFrame_rx(uint32_t timestamp){
    
    app_vars.rxFrameStarted = true;
}

void    cb_endFrame_rx(uint32_t timestamp){
    
    uint8_t i;
    
    radio_getReceivedFrame(
        &(app_vars.packet[0]),
        &app_vars.packet_len,
        sizeof(app_vars.packet),
        &app_vars.rxpk_rssi,
        &app_vars.rxpk_lqi
    );
        
    radio_rfOff();
    
    if(
        app_vars.packet_len == LEN_RX_PKT && (radio_getCrcOk())
    ){
        // Only record IF estimate, LQI, and CDR tau for valid packets
        app_vars.IF_estimate        = radio_getIFestimate();
        app_vars.LQI_chip_errors    = radio_getLQIchipErrors();
        
        //printf(
        //    "pkt received on ch%d %c%c%c%c.%d.%d.%d\r\n",
				printf("Packet num %d. Packet contents 1-3: %d %d %d coarse: %d\tmid: %d\tfine: %d\n",
            app_vars.packet[0],
            app_vars.packet[1],
            app_vars.packet[2],
            app_vars.packet[3],
            app_vars.cfg_coarse,
            app_vars.cfg_mid,
            app_vars.cfg_fine
        );
        
        app_vars.packet_len = 0;
        memset(&app_vars.packet[0],0,LENGTH_PACKET);
    }
    
    //radio_rxEnable();
    //radio_rxNow();
    
    app_vars.rxFrameStarted = false;
    //printf("end frame\n");
}

void    cb_timer(void) {
		int q;
    
    app_vars.changeConfig = true;
	
		radio_rfOff();
	
		//printf("timer end\n");
		
}

