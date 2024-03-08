#include <string.h>

#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

//=========================== variables =======================================

typedef struct {
    uint8_t count;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t i;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    initialize_mote();
    crc_check();
    perform_calibration();

    while (1) {
        printf("Hello World! %d\n", app_vars.count);
        app_vars.count += 1;

        for (i = 0; i < 1000000; i++);
    }
}

//=========================== public ==========================================

//=========================== private =========================================
