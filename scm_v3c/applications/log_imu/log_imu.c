/**
\brief This program calibrates, then repeatedly logs IMU measurements over serial.
Pin setup (See configuration in spi.c):
Chip select: GPO15
Clock:       GPO14
Data in:     GPI13
Data out:    GPO12
*/

#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "optical.h"
#include "spi.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))

//=========================== variables =======================================

typedef struct {
    imu_data_t imu_measurement;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t calc_crc;
	  uint32_t i;

    memset(&app_vars,0,sizeof(app_vars_t));
    
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
    
    // Enable optical SFD interrupt for optical calibration
    optical_enable();
    
    // Wait for optical cal to finish
    while(optical_getCalibrationFinshed() == 0);

    printf("Cal complete\r\n");
    
    // Configure GPIO
    // Set GPI enables
    // Hex entry 1: 0x2 = 2 = 0b0010 = GPI 13 on for IMU
    GPI_enables(0x2000);

    // Set GPO enables
    // Hex entry 1: 0xD = 14 = 0b1101 = GPO 12,14,15 on for IMU
    GPO_enables(0xD000);
    
    // Program analog scan chain (update GPIO configs)
    analog_scan_chain_write();
    analog_scan_chain_load();
    
    initialize_imu();
    test_imu_life();
    
    while(1){
        read_all_imu_data(&app_vars.imu_measurement);
        log_imu_data(&app_vars.imu_measurement);

        for (i = 0; i < 100000; i++);			
    }
}

//=========================== public ==========================================

//=========================== private =========================================
