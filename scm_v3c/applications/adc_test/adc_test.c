#include <string.h>

#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "optical.h"

//=========================== defines =========================================

#define CRC_VALUE         (*((unsigned int *) 0x0000FFFC))
#define CODE_LENGTH       (*((unsigned int *) 0x0000FFF8))

//=========================== variables =======================================

typedef struct {
    uint8_t count;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t i;

    memset(&app_vars,0,sizeof(app_vars_t));
    
    printf("Initializing...");
        
    initialize_mote();
    crc_check();
    perform_calibration();
	
    
    while(1){
        app_vars.count += 1;
				// Request single-shot measurement from ADC
				ADC_REG__START = 0x1;
				
				// ADC will interrupt when measurement complete
			  // See adc_isr() in adc.c
        
        for (i = 0; i < 1000000; i++);
    }
}

//=========================== public ==========================================

//=========================== private =========================================
