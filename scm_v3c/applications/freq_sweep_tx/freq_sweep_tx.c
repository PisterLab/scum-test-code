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

    uint8_t         cfg_coarse;
    uint8_t         cfg_mid;
    uint8_t         cfg_fine;
    
    uint8_t         i;
    uint8_t         j;
    uint8_t         offset;
		uint8_t					counter;
	
		/*
		// manual calibration codes for usb power
		int HF_coarse = 3; 
		int HF_fine = 21;
		int LC_code = 721; 
		int RC2M_coarse = 24;
		int RC2M_fine = 17; 
		int RC2M_superfine = 16;
		int IF_coarse = 22; 
		int IF_fine = 13; 
		*/
		
		// manual calibration codes for keithley 1.86V
		int HF_coarse = 3;
		int HF_fine = 26;
		int LC_code = 721;
		int RC2M_coarse = 25;
		int RC2M_fine = 14;
		int RC2M_superfine = 16;
		int IF_coarse = 22;
		int IF_fine = 39;
		
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
    
    // Debug output
    //printf("\r\nCode length is %u bytes",code_length); 
    //printf("\r\nCRC calculated by SCM is: 0x%X",calc_crc);    
    
    //printf("done\r\n");
    
    
    
    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
    // After bootloading the next thing that happens is frequency calibration using optical
    //printf("Calibrating frequencies...\r\n");
		
    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    //radio_rxEnable();
    //ANALOG_CFG_REG__10 = 0x0018;
		//RFCONTROLLER_REG__CONTROL = RF_RESET;
    // Enable optical SFD interrupt for optical calibration
    //optical_enable();
    
    // Wait for optical cal to finish
    //while(optical_getCalibrationFinshed() == 0);
		
		/*
		HF coarse: 3
		HF fine: 31
		LC code: 712 
		RC2M_coarse: 35
		RC2M_fine: 15
		RC2M_superfine: 15
		RC2M_coarse: 35
		RC2M_fine: 15 
		RC2M_superfine: 15
		IF_coarse: 22
		IF_fine: 32
		IF_coarse: 22 
		IF_fine: 32 

		scm3c_hw_interface_set_HF_CLOCK_coarse(HF_CLOCK_coarse);
    scm3c_hw_interface_set_HF_CLOCK_fine(HF_CLOCK_fine); //21
		LC_monotonic(optical_vars.LC_code);
		set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);
		scm3c_hw_interface_set_RC2M_coarse(RC2M_coarse);
		scm3c_hw_interface_set_RC2M_fine(RC2M_fine);
		scm3c_hw_interface_set_RC2M_superfine(RC2M_superfine);
		set_IF_clock_frequency(IF_coarse, IF_fine, 0);
		scm3c_hw_interface_set_IF_coarse(IF_coarse);
		scm3c_hw_interface_set_IF_fine(IF_fine);
		analog_scan_chain_write();
		analog_scan_chain_load();
		*/
		
		/*
		scm3c_hw_interface_set_HF_CLOCK_coarse(3);
    scm3c_hw_interface_set_HF_CLOCK_fine(31); //21
		LC_monotonic(712);
		set_2M_RC_frequency(31, 31, 35, 15, 15);
		scm3c_hw_interface_set_RC2M_coarse(35);
		scm3c_hw_interface_set_RC2M_fine(15);
		scm3c_hw_interface_set_RC2M_superfine(15);
		set_IF_clock_frequency(22, 32, 0);
		scm3c_hw_interface_set_IF_coarse(22);
		scm3c_hw_interface_set_IF_fine(32);
		analog_scan_chain_write();
		analog_scan_chain_load();
		*/
		
		
		printf("using manually set optical calibration settings\n");
		
		scm3c_hw_interface_set_HF_CLOCK_coarse(HF_coarse);
    scm3c_hw_interface_set_HF_CLOCK_fine(HF_fine); //21
		LC_monotonic(LC_code);
		set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);
		scm3c_hw_interface_set_RC2M_coarse(RC2M_coarse);
		scm3c_hw_interface_set_RC2M_fine(RC2M_fine);
		scm3c_hw_interface_set_RC2M_superfine(RC2M_superfine);
		set_IF_clock_frequency(IF_coarse, IF_fine, 0);
		scm3c_hw_interface_set_IF_coarse(IF_coarse);
		scm3c_hw_interface_set_IF_fine(IF_fine);
		analog_scan_chain_write();
		analog_scan_chain_load();
		

    printf("Cal complete\r\n");
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
    // configure 
		

    while(1){
        
        memcpy(&app_vars.packet[0],&payload_identity[0],sizeof(payload_identity)-1);
        
        // loop through all configuration
        for (cfg_coarse=22;cfg_coarse<23;cfg_coarse++){
            for (cfg_mid=17;cfg_mid<18;cfg_mid++){
                for (cfg_fine=0;cfg_fine<1;cfg_fine += 5){
									// titan: 22 20 4 on usb power; 22 13 25 on keithley 1.5V; 22 21 10 on keithley 1.86V // on solar 22 17 0
										int q;
										for (q = 0; q < 400000; q++) {}
										
										//cfg_coarse = 22;
										//cfg_mid = 15;
										//cfg_fine = 14;
                    printf(
                        "coarse=%d, middle=%d, fine=%d\r\n", 
                        cfg_coarse,cfg_mid,cfg_fine
                    );
                    j = sizeof(payload_identity)-1;
										app_vars.packet[0] = counter++;
										app_vars.packet[1] = cfg_coarse;
										app_vars.packet[2] = cfg_mid;
										app_vars.packet[3] = cfg_fine;
										
                    //app_vars.packet[j++] = '0' + cfg_coarse/10;
                    //app_vars.packet[j++] = '0' + cfg_coarse%10;
                    //app_vars.packet[j++] = '.';
                    //app_vars.packet[j++] = '0' + cfg_mid/10;
                    //app_vars.packet[j++] = '0' + cfg_mid%10;
                    //app_vars.packet[j++] = '.';
                    //app_vars.packet[j++] = '0' + cfg_fine/10;
                    //app_vars.packet[j++] = '0' + cfg_fine%10;
                    //app_vars.packet[j++] = '.';
                    
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

