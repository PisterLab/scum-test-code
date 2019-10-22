/**
\brief This program shows a pingpong communication between SCuM and OpenMote.

After loading this program, SCuM will listen on channel CHANNEL, once it hears
one valid frame sent by OpenMote it will lock on the frequency and keep 
listening.

After locked, SCuM will send back a frame every time it receives one frame from
OpenMote. 

The target image running on OpenMote can be found at: 
(https://openwsn-builder.paris.inria.fr/job/Firmware/board=openmote-cc2538,
    label=master,project=bsp_radio,toolchain=armgcc/)
*/

#include <string.h>

#include "scm3c_hardware_interface.h"
#include "memory_map.h"
#include "rftimer.h"
#include "radio.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))

#define LENGTH_PACKET   125+LENGTH_CRC ///< maximum length is 127 bytes
#define LEN_TX_PKT      30+LENGTH_CRC  ///< length of tx packet
#define LEN_RX_PKT      20+LENGTH_CRC  ///< length of rx packet
#define CHANNEL         11             ///< 11=2.405GHz
#define TIMER_PERIOD    1000           ///< 500 = 1ms@500kHz
#define ID              0x99           ///< byte sent in the packets

//=========================== variables =======================================

typedef struct {
            uint8_t         packet[LENGTH_PACKET];
            uint8_t         packet_len;
            int8_t          rxpk_rssi;
            uint8_t         rxpk_lqi;
    
   volatile bool            rxpk_crc;
   volatile bool            doing_initial_packet_search;
    
   volatile uint32_t        IF_estimate;
   volatile uint32_t        LQI_chip_errors;
   volatile uint32_t        cdr_tau_value;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

void     cb_startFrame_tx(uint32_t timestamp);
void     cb_endFrame_tx(uint32_t timestamp);
void     cb_startFrame_rx(uint32_t timestamp);
void     cb_endFrame_rx(uint32_t timestamp);
void     cb_timer(void);

//=========================== main ============================================

int main(void) {
    
    int t,t2,x;
    uint32_t calc_crc;
    
    memset(&app_vars,0,sizeof(app_vars_t));
    
    radio_init();
    rftimer_init();
    
    radio_setStartFrameTxCb(cb_startFrame_tx);
    radio_setEndFrameTxCb(cb_endFrame_tx);
    radio_setStartFrameRxCb(cb_startFrame_rx);
    radio_setEndFrameRxCb(cb_endFrame_rx);
    
    rftimer_set_callback(cb_timer);
    
    // Disable interrupts for the radio and rftimer
    radio_disable_interrupts();
    rftimer_disable_interrupts();
    
    printf("Initializing...");
        
    // Set up mote configuration
    // This function handles all the analog scan chain setup
    initialize_mote();
    
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
    
    // After bootloading the next thing that happens is frequency calibration using optical
    printf("Calibrating frequencies...\r\n");
    
    // Initial frequency calibration will tune the frequencies for HCLK, the RX/TX chip clocks, and the LO
    // For the LO, calibration for RX channel 11, so turn on AUX, IF, and LO LDOs
    // Aux is inverted (0 = on)
    // Memory-mapped LDO control
    // ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
    // For MUX signals, '1' = FSM control, '0' = memory mapped control
    // For EN signals, '1' = turn on LDO
    ANALOG_CFG_REG__10 = 0x18;
    
    // Enable polyphase and mixers via memory-mapped I/O (for receive mode)
    ANALOG_CFG_REG__16 = 0x1;
    
    // Enable optical SFD interrupt for optical calibration
    ISER = 0x0800;
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    //skip building a channel table for now; hardcode LC values

    printf("Listening for packets on ch %d \r\n",CHANNEL);

    // First listen continuously for rx packet
    app_vars.doing_initial_packet_search = true;
    
    // Enable interrupts for the radio FSM
    radio_enable_interrupts();

    // Begin listening
    radio_setFrequency(CHANNEL, FREQ_RX);
    radio_rxEnable();
    radio_rxNow();
    
    // Wait awhile
    for (t2=0; t2<100; t2++){
        
        // Delay
        for(t=0; t<100000; t++);
        
        if(app_vars.doing_initial_packet_search == false) {
            printf("Locked to incoming packet rate...\r\n");
            break;
        }
    }
    
    // If no packet received, then stop RX so can reprogram
    if(app_vars.doing_initial_packet_search == true) {
        radio_rfOff();
        radio_disable_interrupts();
        printf("RX Stopped - Lock Failed\n");
    }

    while(1) {
        for(t=0; t<1000000; t++);
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void    cb_startFrame_tx(uint32_t timestamp){
    
}

void    cb_endFrame_tx(uint32_t timestamp){
    
    // turn to listen
    
    radio_rfOff();
    
//    radio_frequency_housekeeping(
//        app_vars.IF_estimate,
//        app_vars.LQI_chip_errors,
//        app_vars.cdr_tau_value
//    );
    
    radio_setFrequency(CHANNEL, FREQ_RX);
    radio_rxEnable();
    radio_rxNow();
    
}

void    cb_startFrame_rx(uint32_t timestamp){
    
}

void    cb_endFrame_rx(uint32_t timestamp){
    
    radio_getReceivedFrame(
        &(app_vars.packet[0]),
        &app_vars.packet_len,
        sizeof(app_vars.packet),
        &app_vars.rxpk_rssi,
        &app_vars.rxpk_lqi
    );
    
    // Check if the packet length is as expected (20 payload bytes + 2 for CRC)    
    // In this demo code, it is assumed the OpenMote is sending packets with 20B payloads
    if(app_vars.packet_len != LEN_RX_PKT){
        
        // Keep listening
        radio_setFrequency(CHANNEL, FREQ_RX);
        radio_rxEnable();
        radio_rxNow();
    } else {
        if (radio_getCrcOk()==false){
            // Length was right but CRC was wrong
            
            // Keep listening
            radio_setFrequency(CHANNEL, FREQ_RX);
            radio_rxEnable();
            radio_rxNow();
    
            // Packet has good CRC value and is the correct length
        } else {
            
            // If this is the first valid packet received, start timers for next expected packet
            if(app_vars.doing_initial_packet_search == true){
                
                // Clear this flag to note we are no longer doing continuous RX listen
                app_vars.doing_initial_packet_search = false;
                
            }
            
            // Only record IF estimate, LQI, and CDR tau for valid packets
            app_vars.IF_estimate        = radio_getIFestimate();
            app_vars.LQI_chip_errors    = radio_getLQIchipErrors();
            
            // Read the value of tau debug at end of packet
            // Do this later in the ISR to make sure this register has settled before trying to read it
            // (the register is on the adc clock domain)
            app_vars.cdr_tau_value = radio_get_cdr_tau_value();
            
            // Prepare ack - for this demo code the contents are arbitrary
            // The OpenMote receiver is looking for 30B packets - still on channel 11
            // Data is stored in app_vars.packet[]
            
            app_vars.packet[0] = 't';
            app_vars.packet[1] = 'e';
            app_vars.packet[2] = 's';
            app_vars.packet[3] = 't';
            
            radio_loadPacket(app_vars.packet, LEN_TX_PKT);
            
            radio_rfOff();
            
            // Setup LO for transmit
            radio_setFrequency(CHANNEL, FREQ_TX);
            
            // Turn on RF for TX
            radio_txEnable();
            
            // send frame after 2ms
            rftimer_setCompareIn(rftimer_readCounter()+ TIMER_PERIOD);
        }
    }
}

void    cb_timer(void) {
    
    // Tranmit the packet
    radio_txNow();
}

