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

#define LENGTH_PACKET       125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_TX_PKT          20+LENGTH_CRC  ///< length of tx packet
#define LEN_RX_PKT          20+LENGTH_CRC  ///< length of rx packet
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
} app_vars_t_rx;

app_vars_t app_vars;
app_vars_t_rx app_vars_rx;
uint8_t					counter;
bool tx_mode;

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
		
		
		counter = 0;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    
    radio_setEndFrameTxCb(cb_endFrame_tx);
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
    
    
    
    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");
		
    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // by calling radio rxEnable
    radio_rxEnable();
    
    // Enable optical SFD interrupt for optical calibration
    optical_enable();
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();
    
		
    // configure 

    while(1){
				int i = 0;
				for (i = 0; i < 10000; i++) {}
			
				printf("SWITCHING TO TX!!\n");
				tx_mode = true;
        send_packet();
				
				printf("ENTERING LOW POWER STATE\n");
				low_power_mode();
				for (i = 0; i < 1000; i++) {
				}
				
				normal_power_mode();
				printf("RETURNING TO HIGH POWER STATE\n");
				
				
				//while (1) {
				printf("SWITCHING TO RX\n");
				tx_mode = false;
				receive_packet();
				//}
				
				while (1) {
				}
    }
		
		while(1) {
			
		}
}

//=========================== public ==========================================

//=========================== private =========================================

void send_packet(void) {
		uint8_t         cfg_coarse;
    uint8_t         cfg_mid;
    uint8_t         cfg_fine;
		uint8_t         j;
		uint8_t         i;
	
		memcpy(&app_vars.packet[0],&payload_identity[0],sizeof(payload_identity)-1);
	
		// loop through all configuration
		for (cfg_coarse=22;cfg_coarse<23;cfg_coarse++){
				for (cfg_mid=16;cfg_mid<17;cfg_mid++){
						for (cfg_fine=22;cfg_fine<24;cfg_fine++){
								// 22 15 15 for golden board
								// 22 20 4 for titan's board
								// 22 15 30 for R4
							// 
								printf(
										"Tx coarse=%d, middle=%d, fine=%d\r\n", 
										cfg_coarse,cfg_mid,cfg_fine
								);
								j = sizeof(payload_identity)-1;
								app_vars.packet[0] = counter++;
								app_vars.packet[1] = cfg_coarse;
								app_vars.packet[2] = cfg_mid;
								app_vars.packet[3] = cfg_fine;
								
								app_vars.packet[j++] = '0' + cfg_coarse/10;
								app_vars.packet[j++] = '0' + cfg_coarse%10;
								app_vars.packet[j++] = '.';
								app_vars.packet[j++] = '0' + cfg_mid/10;
								app_vars.packet[j++] = '0' + cfg_mid%10;
								app_vars.packet[j++] = '.';
								app_vars.packet[j++] = '0' + cfg_fine/10;
								app_vars.packet[j++] = '0' + cfg_fine%10;
								app_vars.packet[j++] = '.';
								
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

void receive_packet(void) {
	  uint8_t         i;
		
		// loop through all configuration
		for (app_vars_rx.cfg_coarse=20;app_vars_rx.cfg_coarse<21;app_vars_rx.cfg_coarse++){
				for (app_vars_rx.cfg_mid=0;app_vars_rx.cfg_mid<1;app_vars_rx.cfg_mid++){
						for (app_vars_rx.cfg_fine=0;app_vars_rx.cfg_fine<1;app_vars_rx.cfg_fine++){
								// 21 31 31 for golden board
								printf(
										"Rx: coarse=%d, middle=%d, fine=%d\r\n", 
										app_vars_rx.cfg_coarse,app_vars_rx.cfg_mid,app_vars_rx.cfg_fine
								);
								for (i=0;i<1;i++) {
										while(app_vars_rx.rxFrameStarted == true);
										radio_rfOff();
										LC_FREQCHANGE(app_vars_rx.cfg_coarse,app_vars_rx.cfg_mid,app_vars_rx.cfg_fine);
										radio_rxEnable();
										radio_rxNow();
										rftimer_setCompareIn(rftimer_readCounter()+TIMER_PERIOD);
										app_vars_rx.changeConfig = false;
										while (app_vars_rx.changeConfig==false);
								}
						}
				}
		}
}

void    cb_endFrame_tx(uint32_t timestamp){
    
    radio_rfOff();
    
    app_vars.sendDone = true;
    
}

void    cb_startFrame_rx(uint32_t timestamp){
    
    app_vars_rx.rxFrameStarted = true;
}

void    cb_endFrame_rx(uint32_t timestamp){
    
    uint8_t i;
    
    radio_getReceivedFrame(
        &(app_vars_rx.packet[0]),
        &app_vars_rx.packet_len,
        sizeof(app_vars_rx.packet),
        &app_vars_rx.rxpk_rssi,
        &app_vars_rx.rxpk_lqi
    );
        
    radio_rfOff();
    
    if(
        app_vars_rx.packet_len == LEN_RX_PKT && (radio_getCrcOk())
    ){
        // Only record IF estimate, LQI, and CDR tau for valid packets
        app_vars_rx.IF_estimate        = radio_getIFestimate();
        app_vars_rx.LQI_chip_errors    = radio_getLQIchipErrors();
        
        //printf(
        //    "pkt received on ch%d %c%c%c%c.%d.%d.%d\r\n",
				printf("Packet num %d. Packet contents 1-3: %d %d %d coarse: %d\tmid: %d\tfine: %d\n",
            app_vars_rx.packet[0],
            app_vars_rx.packet[1],
            app_vars_rx.packet[2],
            app_vars_rx.packet[3],
            app_vars_rx.cfg_coarse,
            app_vars_rx.cfg_mid,
            app_vars_rx.cfg_fine
        );
        
        app_vars_rx.packet_len = 0;
        memset(&app_vars_rx.packet[0],0,LENGTH_PACKET);
    }
    
    radio_rxEnable();
    radio_rxNow();
    
    app_vars_rx.rxFrameStarted = false;
    
}

void    cb_timer(void) {
    if (tx_mode) {
				// Tranmit the packet
				radio_txNow();
		} else {
				// in the case of Rx set the flag
				app_vars_rx.changeConfig = true;
		}
}

