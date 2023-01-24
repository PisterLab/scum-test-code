#include "rftimer.h"

#include <stdbool.h>
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

void rftimer_callback(void);

//=========================== main ============================================

int main(void) {
    uint32_t i;

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    initialize_mote();
    crc_check();
    perform_calibration();

    delay_milliseconds_synchronous(1000, 1);
    printf("timer1\n");
    delay_milliseconds_synchronous(1000, 2);
    printf("timer2\n");
    delay_milliseconds_synchronous(1000, 3);
    printf("timer3\n");
    delay_milliseconds_synchronous(1000, 4);
    printf("timer4\n");
    delay_milliseconds_synchronous(1000, 5);
    printf("timer5\n");
    delay_milliseconds_synchronous(1000, 6);
    printf("timer6\n");
    delay_milliseconds_synchronous(1000, 7);
    printf("timer7\n");

    rftimer_set_repeat(true, 1);
    rftimer_set_callback_by_id(rftimer_callback, 1);
    delay_milliseconds_asynchronous(1000, 1);
}

//=========================== public ==========================================

//=========================== private =========================================

void rftimer_callback(void) { printf("callback occured\n"); }
