#include "Memory_Map.h"
#include <stdio.h>
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "radio.h"
#include "bucket_o_functions.h"

extern unsigned short current_RF_channel;

void timer_isr() {
    
    // The below explains what each timer interrupt does:    
    
// COMPARE0 @ t=0
// --------------------
// Set the LO frequency for packet reception on ch11
// setFrequencyRX(channel);
 
// Disable TX interrupts (turn them on only if RX is successful)
// RFTIMER_REG__COMPARE3_CONTROL = 0x0;
// RFTIMER_REG__COMPARE4_CONTROL = 0x0;

// Enable RX watchdog in case packet is never heard
// RFTIMER_REG__COMPARE2_CONTROL = 0x3;

    
// COMPARE1 (t1) = Turn on the analog part of the RX 
// --------------------
// t0 = (expected - guard_time - radio_startup_time);
// rxEnable();


// COMPARE2 (t2) = Time to start listening for packet 
// --------------------
// t1 = (expected_arrival - guard_time);
// rxNow(); 


// COMPARE3 (t3) = RX watchdog 
// If SFD was never heard, then turn the radio back off
// --------------------
// t2 = (t1 + 2 * guard_time);
// radio_rfOff();


// COMPARE4 (t4) = Turn on transmitter 
// --------------------
// t3 = (expected_RX_arrival + ack_turnaround_time - radio_startup_time);
// txEnable();


// COMPARE5 (t5) = Transmit packet 
// --------------------
// t4 = (expected_RX_arrival + ack_turnaround_time - radio_startup_time);
// txNow();



    unsigned int interrupt = RFTIMER_REG__INT;
    
    if (interrupt & 0x00000001){ //printf("COMPARE0 MATCH\r\n");
        
        // Setup LO for reception
        setFrequencyRX(current_RF_channel);

        // Enable RX watchdog
        RFTIMER_REG__COMPARE3_CONTROL = 0x3;
        
        // Disable TX interrupts (turn them on only if RX is successful)
        RFTIMER_REG__COMPARE4_CONTROL = 0x0;
        RFTIMER_REG__COMPARE5_CONTROL = 0x0;
        
        // Toggle GPIO7 to indicate start of packet interval (t=0)
        GPIO_REG__OUTPUT |= 0x80;
        GPIO_REG__OUTPUT |= 0x80;
        GPIO_REG__OUTPUT &= ~(0x80);
        
    }
    if (interrupt & 0x00000002){// printf("COMPARE1 MATCH\r\n");
        
    // Raise GPIO1 to indicate RX is on
        GPIO_REG__OUTPUT |= 0x2;

        // Turn on the analog part of RX
        radio_rxEnable();
        
    }
    if (interrupt & 0x00000004){// printf("COMPARE2 MATCH\r\n");
                
        // Start listening for packet
        radio_rxNow();
                
    }
    // Watchdog has expired - no packet received
    if (interrupt & 0x00000008){// printf("COMPARE3 MATCH\r\n");
        
        // Lower GPIO1 to indicate RX is off
        GPIO_REG__OUTPUT &= ~(0x2);
        
        // Turn off the radio
        radio_rfOff();
        
    }
    // Turn on transmitter to allow frequency to settle
    if (interrupt & 0x00000010){// printf("COMPARE4 MATCH\r\n");
        
        // Raise GPIO3 to indicate TX is on
        GPIO_REG__OUTPUT |= 0x8;
        
        // Setup LO for transmit
        setFrequencyTX(current_RF_channel);
        
        // Turn on RF for TX
        radio_txEnable();
        
    }
    // Transmit now
    if (interrupt & 0x00000020){// printf("COMPARE5 MATCH\r\n");    
        
        // Tranmit the packet
        radio_txNow();
        
    }
    if (interrupt & 0x00000040) printf("COMPARE6 MATCH\r\n");
    if (interrupt & 0x00000080) printf("COMPARE7 MATCH\r\n");
    if (interrupt & 0x00000100) printf("CAPTURE0 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE0);
    if (interrupt & 0x00000200) printf("CAPTURE1 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE1);
    if (interrupt & 0x00000400) printf("CAPTURE2 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE2);
    if (interrupt & 0x00000800) printf("CAPTURE3 TRIGGERED AT: 0x%x\r\n", RFTIMER_REG__CAPTURE3);
    if (interrupt & 0x00001000) printf("CAPTURE0 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE0);
    if (interrupt & 0x00002000) printf("CAPTURE1 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE1);
    if (interrupt & 0x00004000) printf("CAPTURE2 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE2);
    if (interrupt & 0x00008000) printf("CAPTURE3 OVERFLOW AT: 0x%x\r\n", RFTIMER_REG__CAPTURE3);
    
    RFTIMER_REG__INT_CLEAR = interrupt;
}