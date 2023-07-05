#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpio.h"
#include "memory_map.h"
#include "optical.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

// ========================== define for sine freq ============================

#define freq 10
#define PI 3.142857
#define sample 10
#define repeat 10

//=========================== variables =======================================

typedef struct {
    uint8_t count;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t i, j;
    //		double period;
    //		int interval;
    //		double delay;
    int high, low;
    //		double t;
    int sine[10] = {5, 8, 10, 10, 8, 5, 2, 0, 0, 2};
    //		int sine[10] = {1, 1, 1, 1, 8, 5, 2, 0, 0 ,2};

    memset(&app_vars, 0, sizeof(app_vars_t));

    printf("Initializing...");

    initialize_mote();
    crc_check();
    perform_calibration();

    gpio_init();
    /*
    period = 1.0 / freq;
    interval = 1000.0 * period / sample;
    printf("%f\n", period);
    printf("%d", interval);
    t = 2.0 / 1000 * PI * freq * interval * 5;
    delay = sin(t) * 10;
    high = delay;
    low = interval - high;
    printf("%g %d %g", delay, sample, t);
    */
    while (1) {
        for (i = 0; i < sample; i++) {
            for (j = 0; j < repeat; j++) {
                high = sine[i];
                //					printf("%d", high);
                low = 10 - high;
                //				printf("%g %d %g", delay, sample,
                //period);
                if (high) {
                    gpio_1_set();
                    delay_milliseconds_synchronous(high, 1);
                }
                if (low) {
                    gpio_1_clr();
                    delay_milliseconds_synchronous(low, 1);
                }
            }
        }
    }

/*	
while (1) {
      gpio_1_set();
      delay_milliseconds_synchronous(1,
       1); gpio_1_clr(); delay_milliseconds_synchronous(1, 1);
                            }
    */
    /*    while(1){
            printf("Hello World! %d\n", app_vars.count);
            app_vars.count += 1;

            for (i = 0; i < 1000000; i++);
        }
    */
}

//=========================== public ==========================================

//=========================== private =========================================
