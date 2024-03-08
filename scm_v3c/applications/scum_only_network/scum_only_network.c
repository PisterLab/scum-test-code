#include <string.h>

#include "gpio.h"
#include "memory_map.h"
#include "optical.h"
#include "radio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

#define RFTIMER_CAL_DURATION 50000  // 50000 = 100ms, approximately

#define TX_PACKET_LEN 8 + 2  // 2 for CRC
#define RX_PACKET_LEN 8 + 2  // 2 for CRC

#define TX_RATE 100000  // rate of packet transmitting, approximately 5 Hz.
#define RX_RATE \
    200000  // rate that receiver is reset to improve scum-scum reliability.

// #define SELECTED_SCUM				27
#define SELECTED_SCUM 37

#if SELECTED_SCUM == 27
#define SCUM_TX 1
#elif SELECTED_SCUM == 37
#define SCUM_RX 1
#endif

//=========================== variables =======================================

typedef struct {
    uint8_t count;
    uint8_t tx_packet[TX_PACKET_LEN];
    uint8_t rx_packet[RX_PACKET_LEN];
    uint8_t packet_counter;
} app_vars_t;

app_vars_t app_vars;

typedef struct {
    uint8_t cal_finished;
    uint8_t cal_iteration;
} cal_vars_t;

app_vars_t app_vars;
cal_vars_t cal_vars;

//=========================== prototypes ======================================
void timer_cb(void);

//=========================== main ============================================

int main(void) {
    uint32_t i;

    memset(&app_vars, 0, sizeof(app_vars_t));
    memset(&cal_vars, 0, sizeof(cal_vars_t));

    printf("Initializing...");

    initialize_mote();
    crc_check();
    perform_calibration();

#if SCUM_TX == 1
    radio_txEnable();
    LC_FREQCHANGE(24, 22, 19);
#elif SCUM_RX == 1
    radio_rxEnable();
    LC_FREQCHANGE(24, 19, 24);
#endif

    rftimer_init();
    rftimer_set_callback_by_id(timer_cb, 1);
    rftimer_enable_interrupts_by_id(1);
    rftimer_enable_interrupts();
    rftimer_setCompareIn_by_id(rftimer_readCounter() + RFTIMER_CAL_DURATION, 1);

    cal_vars.cal_finished = 1;
    while (!cal_vars.cal_finished) {}

    while (1) {
        // printf("Hello World! %d\n", app_vars.count);
        app_vars.count += 1;
#if SCUM_TX == 1
        app_vars.packet_counter++;
        app_vars.tx_packet[0] = app_vars.packet_counter;
        send_packet(app_vars.tx_packet, TX_PACKET_LEN);
#elif SCUM_RX == 1
        receive_packet_length(RX_PACKET_LEN, false);
#endif
    }
}

//=========================== public ==========================================

//=========================== private =========================================

void timer_cb(void) {
    uint32_t rdata_lsb, rdata_msb;
    uint32_t count_LC, count_32k, count_2M, count_HFclock, count_IF;

    uint32_t RC2M_coarse;
    uint32_t RC2M_fine;
    uint32_t RC2M_superfine;
    uint32_t IF_clk_target;
    uint32_t IF_coarse;
    uint32_t IF_fine;

    rftimer_setCompareIn_by_id(rftimer_readCounter() + RFTIMER_CAL_DURATION,
                               1);  // reset timer

    // Disable all counters
    ANALOG_CFG_REG__0 = 0x007F;

    RC2M_coarse = scm3c_hw_interface_get_RC2M_coarse();
    RC2M_fine = scm3c_hw_interface_get_RC2M_fine();
    RC2M_superfine = scm3c_hw_interface_get_RC2M_superfine();
    IF_clk_target = scm3c_hw_interface_get_IF_clk_target();
    IF_coarse = scm3c_hw_interface_get_IF_coarse();
    IF_fine = scm3c_hw_interface_get_IF_fine();

    // Read HF_CLOCK counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x100000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x140000);
    count_HFclock = rdata_lsb + (rdata_msb << 16);

    // Read 2M counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
    count_2M = rdata_lsb + (rdata_msb << 16);

    // Read LC_div counter (via counter4)
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
    count_LC = rdata_lsb + (rdata_msb << 16);

    // Read IF ADC_CLK counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x300000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x340000);
    count_IF = rdata_lsb + (rdata_msb << 16);

    // Reset all counters
    ANALOG_CFG_REG__0 = 0x0000;

    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;

    cal_vars.cal_iteration++;

    if (cal_vars.cal_iteration > 2) {
        // execute calibration on 2M clock

        // TOO FAST:
        if (count_2M > (count_LC / 8 + 600)) {
            RC2M_coarse += 1;
        } else {
            if (count_2M > (count_LC / 8 + 80)) {
                RC2M_fine += 1;
            } else {
                if (count_2M > (count_LC / 8 + 15)) {
                    RC2M_superfine += 1;
                }
            }
        }

        // TOO SLOW:
        if (count_2M < (count_LC / 8 - 600)) {
            RC2M_coarse -= 1;
        } else {
            if (count_2M < (count_LC / 8 - 80)) {
                RC2M_fine -= 1;
            } else {
                if (count_2M < (count_LC / 8 - 15)) {
                    RC2M_superfine -= 1;
                }
            }
        }
        set_2M_RC_frequency(31, 31, RC2M_coarse, RC2M_fine, RC2M_superfine);
        scm3c_hw_interface_set_RC2M_coarse(RC2M_coarse);
        scm3c_hw_interface_set_RC2M_fine(RC2M_fine);
        scm3c_hw_interface_set_RC2M_superfine(RC2M_superfine);

        // Do correction on IF RC clock
        // Fine DAC step size is ~2800 counts
        if (count_IF > (count_LC + 1400)) {
            IF_fine += 1;
        }
        if (count_IF < (count_LC - 1400)) {
            IF_fine -= 1;
        }

        set_IF_clock_frequency(IF_coarse, IF_fine, 0);
        scm3c_hw_interface_set_IF_coarse(IF_coarse);
        scm3c_hw_interface_set_IF_fine(IF_fine);

        analog_scan_chain_write();
        analog_scan_chain_load();

        // print debug outputs:
        printf("HF=%d   2M=%d-%d,%d,%d   LC=%d   IF=%d-%d\r\n", count_HFclock,
               count_2M, RC2M_coarse, RC2M_fine, RC2M_superfine, count_LC,
               count_IF, IF_fine);
    }

    if (cal_vars.cal_iteration == 25) {
        rftimer_disable_interrupts_by_id(1);
        cal_vars.cal_iteration = 0;
        cal_vars.cal_finished = 1;

        // halt all counters:
        ANALOG_CFG_REG__0 = 0x0000;
    }
}
