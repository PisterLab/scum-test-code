#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Memory_Map.h"
#include "gpio.h"
#include "memory_map.h"
#include "optical.h"
#include "radio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"

//=========================== defines =========================================

#define TIME_INTERVAL 2000000

// #define RX_PACKET_LEN 9+2 // 2 for CRC
#define RX_PACKET_LEN 125 + 2
#define TX_PACKET_LEN 18 + 2

// === NETWORK STATES === //
#define DESYNCHED 0  // INITIAL STATE, BUSY LISTEN FOR PACKETS
#define SYNCHED 1    // RECEIVED "NUM_RX_EBS_FOR_SYNC" PACKETS IN A ROW
#define JOINED \
    2  // SENT A JOIN REQ. AND RECEIVED A VALID ACK WITH SCHEDULE INFO!

// === TIME VARIABLES === //
#define RX_EB_BACKOFF 0
// #define RX_EB_BACKOFF					1400
// #define RX_EB_BACKOFF						2000
// #define LC_DELAY							6000
#define RX_TIMEOUT_TIME 10000

// === FREQUENCY VARIABLES === //
#define NUMBER_OF_STORED_IF_COUNTS 4

// === PACKET INDEX INFORMATION === //
#define SOURCE_ADDRESS_MSB_INDEX 0
#define SOURCE_ADDRESS_LSB_INDEX 1
#define DESTINATION_ADDRESS_MSB_INDEX 2
#define DESTINATION_ADDRESS_LSB_INDEX 3
#define PACKET_TYPE_MSB_INDEX 4
#define PACKET_TYPE_LSB_INDEX 5
#define EB_RATE_MSB_INDEX 4
#define EB_RATE_LSB_INDEX 5

// === PACKET TYPES === //
// by destination address:
#define PACKET_EB 2
#define PACKET_FOR_ME 1
#define PACKET_NOT_FOR_ME 0
// by packet purpose/contents:
#define JOIN_REQUEST 4
#define KEEP_ALIVE 5
#define PINGU 6

// === MISC. === //
#define NUM_RX_EBS_FOR_SYNC 4
#define NUM_RX_EBS_FOR_AVG 4
// #define RX_FINE_CODE_FUDGE		2
#define RX_FINE_CODE_FUDGE 3

// === TIME INFORMATION === //
#define RX_EB_GUARD_TIME_TARGET 4000
#define RX_EB_PACKET_DURATION 320
#define RX_IF_BACKOFF 5000
// #define RX_IF_BACKOFF
// 0

// === TIMER COMPARE REG. CALLBACKS === // USE OF 0 IS DEDICATED TO LOW-LEVEL
// RADIO DELAYS, USE AT YOUR OWN RISK
#define TIMER_CB_TX_RDY_WAIT 1
#define TIMER_CB_TX_RX_DELAY 2
#define TIMER_CB_SF_TIMEOUT 3
#define TIMER_CB_RX_TIMEOUT 4
#define TIMER_CB_TX_BEACON 5
#define TIMER_CB_TX_EB 6
#define TIMER_CB_RX_EB 7

// === LOW-LEVEL RADIO FSM DEFINES === //
#define YES_LISTEN_FOR_ACK 1
#define DO_NOT_LISTEN_FOR_ACK 0

// === CHOOSE BETWEEN THE SCUMS === //
// #define SELECTED_SCUM						17 //
// PROG PORT 27, UART PORT// 17
#define SELECTED_SCUM 6  // PROG PORT 37, UART PORT 6
// #define SELECTED_SCUM						4 //
// PROG PORT 4, UART PORT LOGIC ANALYZER #define SELECTED_SCUM 77 // TEENSY
// OPTICAL PROG. PORT, NO UART

// === DEBUG FLAGS === //
#define EB_ONLY 1

#if SELECTED_SCUM == 17
// LC CODES, CHANNEL 17
#define RX_LC_COARSE 25
#define RX_LC_MID 5
#define RX_LC_FINE 15
#define RX_LC_FINE_SYNC RX_LC_FINE + RX_FINE_CODE_FUDGE
#define TX_LC_COARSE 24
#define TX_LC_MID 24
#define TX_LC_FINE 22

// address
#define MY_ADDRESS 0xCAFE

// parent address
#define PERMITTED_PARENT 0x1234
#define PREF_PARENT_MSB 0x12
#define PREF_PARENT_LSB 0x34

// modes
#define ONLY_LISTEN 1

// time nonsense
#define EB_TIMER_SKEW 20000

#elif SELECTED_SCUM == 6

// LC CODES, CHANNEL 17
#define RX_LC_COARSE 24
#define RX_LC_MID 20
#define RX_LC_FINE 14
#define RX_LC_FINE_SYNC RX_LC_FINE + RX_FINE_CODE_FUDGE
#define TX_LC_COARSE 24
#define TX_LC_MID 20
#define TX_LC_FINE 25

// address
#define MY_ADDRESS 0x4EE7
#define MY_ADDRESS_MSB 0x4E
#define MY_ADDRESS_LSB 0xE7

// parent address
#define PREF_PARENT_MSB 0xCA
#define PREF_PARENT_LSB 0xFE

// modes
#define ONLY_LISTEN 0

// time nonsense
#define EB_TIMER_SKEW 20000

#elif SELECTED_SCUM == 4
// LC CODES, CHANNEL 17
#define RX_LC_COARSE 26
#define RX_LC_MID 9
#define RX_LC_FINE 16
#define RX_LC_FINE_SYNC RX_LC_FINE + RX_FINE_CODE_FUDGE
#define TX_LC_COARSE 26
#define TX_LC_MID 12
#define TX_LC_FINE 19

// address
#define MY_ADDRESS 0xFA73

// parent address
#define PREF_PARENT_MSB 0x12
#define PREF_PARENT_LSB 0x34

// modes
#define ONLY_LISTEN 0

// time nonsense
#define EB_TIMER_SKEW 15000

#elif SELECTED_SCUM == 77
// LC CODES, CHANNEL 17
#define RX_LC_COARSE 25
#define RX_LC_MID 6
#define RX_LC_FINE 13
#define RX_LC_FINE_SYNC RX_LC_FINE + RX_FINE_CODE_FUDGE
#define TX_LC_COARSE 25
#define TX_LC_MID 5
#define TX_LC_FINE 16

// address
#define MY_ADDRESS 0xBBAA

// parent address
#define PREF_PARENT_MSB 0x12
#define PREF_PARENT_LSB 0x34

// modes
#define ONLY_LISTEN 0

// time nonsense
#define EB_TIMER_SKEW 15000
#endif

//=========================== variables =======================================

typedef struct {
    uint8_t dummy;
    uint32_t current_count;
    uint32_t previous_count;
    uint8_t rxpk_done;
    uint8_t sync_counter;

    uint8_t packet[RX_PACKET_LEN];
    uint8_t tx_packet[TX_PACKET_LEN];
    uint8_t packet_len;
    int8_t rxpk_rssi;
    uint8_t rxpk_lqi;

    uint32_t capture_current_time;

    uint8_t printflag;
    uint16_t source_address;
    uint16_t destin_address;
    uint16_t pack_type;

    volatile bool rxpk_crc;
    volatile bool changeConfig;
    volatile bool rxFrameStarted;

    volatile uint32_t IF_estimate;
    volatile uint32_t LQI_chip_errors;
    volatile uint32_t cdr_tau_value;

    uint8_t IF_on_early;

    uint32_t ADC_counter;
    uint32_t chip_clk_counter;
    uint32_t IF_fine_clk_setting;
    uint32_t IF_coarse_clk_setting;

    uint8_t dont_pll;

} app_vars_t;

typedef struct {
    uint8_t sync_state;
    uint8_t listen_for_ack;
    uint8_t last_EB_received;

    uint16_t parent_address;

    uint8_t child_address_MSB;
    uint8_t child_address_LSB;

    uint8_t future_listen_in_EB;

    uint8_t ack_join_request;
    uint8_t ack_child_packet;

    uint8_t in_case_of_EB_miss;

    uint8_t desync_risk;

} scumpong_vars_t;

typedef struct {
    uint32_t rx_EB_timer;
    uint32_t rx_EB_timer_from_packet;
    uint32_t rx_EB_next_channel;
    uint8_t rx_EB_index;
    uint32_t rx_EB_timer_history[NUM_RX_EBS_FOR_AVG];
    uint8_t rx_EBs_to_average;  // only less than NUM_RX_EBS_FOR_AVG if it's at
                                // the beginning of a desync period
    uint32_t rx_EB_start_reception_time;

    uint32_t tx_join_req_timer_from_parent;
    uint32_t tx_dedicated_slot_from_parent;
    uint32_t tx_EB_timer_from_parent;

    uint32_t current_downlink_time;

    uint32_t i_was_early_by;

    uint32_t tx_EB_timer_eb_only;

} time_sync_vars_t;

typedef struct {
    uint8_t if_estimate_index;
    uint16_t if_estimate_buffer[NUMBER_OF_STORED_IF_COUNTS];
} freq_update_vars_t;

typedef struct {
    uint8_t tx_coarse;
    uint8_t tx_mid;
    uint8_t tx_fine;
    uint8_t rx_coarse;
    uint8_t rx_mid;
    uint8_t rx_fine;
    uint8_t rx_fine_sync;
    uint8_t rx_fine_desync;
} channel_vars_t;

time_sync_vars_t time_sync_vars;
scumpong_vars_t scumpong_vars;
app_vars_t app_vars;
freq_update_vars_t freq_update_vars;
channel_vars_t channel_vars;

//=========================== prototypes ======================================
void radio_startframe_cb(uint32_t timestamp);
void radio_rx_cb(uint32_t timestamp);
void tx_endframe_callback(uint32_t timestamp);
void tx_beacon_callback(void);
void transmit_delay_callback(void);

//=========================== main ============================================

int main(void) {
    uint8_t i;

    // reset global variables
    memset(&app_vars, 0, sizeof(app_vars_t));
    memset(&scumpong_vars, 0, sizeof(scumpong_vars_t));
    memset(&time_sync_vars, 0, sizeof(time_sync_vars_t));
    memset(&channel_vars, 0, sizeof(channel_vars_t));

    initialize_mote();
    crc_check();
    perform_calibration();

    // initialize channel settings, from initial calibration:
    channel_vars.rx_coarse = RX_LC_COARSE;
    channel_vars.rx_mid = RX_LC_MID;
    channel_vars.rx_fine = RX_LC_FINE;
    channel_vars.tx_coarse = TX_LC_COARSE;
    channel_vars.tx_mid = TX_LC_MID;
    channel_vars.tx_fine = TX_LC_FINE;
    channel_vars.rx_fine_sync = RX_LC_FINE + RX_FINE_CODE_FUDGE;
    channel_vars.rx_fine_desync = RX_LC_FINE;

    // initialize RADIO start and endframe callbacks TODO: make this a private
    // function
    radio_setStartFrameRxCb(radio_startframe_cb);
    radio_setEndFrameRxCb(radio_rx_cb);
    radio_setEndFrameTxCb(tx_endframe_callback);

    // initialize RFTIMER compare register callbacks and interrupts TODO: make
    // this a private function
    rftimer_init();
    rftimer_set_callback_by_id(transmit_delay_callback, TIMER_CB_TX_RDY_WAIT);
    // rftimer_set_callback_by_id(receive_delay_callback,
    // TIMER_CB_TX_RX_DELAY); rftimer_set_callback_by_id(rx_timeout_callback,
    // TIMER_CB_RX_TIMEOUT);
    // rftimer_set_callback_by_id(rx_startframe_timeout_callback,
    // TIMER_CB_SF_TIMEOUT);
    rftimer_set_callback_by_id(tx_beacon_callback, TIMER_CB_TX_BEACON);
    // rftimer_set_callback_by_id(transmit_EB_timer_callback,
    // TIMER_CB_TX_EB); rftimer_set_callback_by_id(test_rf_timer_callback,
    // TIMER_CB_RX_EB);

    scumpong_vars.sync_state = DESYNCHED;
    rftimer_enable_interrupts();
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_RDY_WAIT);
    rftimer_enable_interrupts_by_id(TIMER_CB_RX_TIMEOUT);
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_BEACON);
    rftimer_enable_interrupts_by_id(TIMER_CB_SF_TIMEOUT);
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_EB);
    rftimer_enable_interrupts_by_id(TIMER_CB_RX_EB);

    // get calibrated IF settings:
    app_vars.IF_coarse_clk_setting = scm3c_hw_interface_get_IF_coarse();
    app_vars.IF_fine_clk_setting = scm3c_hw_interface_get_IF_fine();

#if SELECTED_SCUM == 6
    // prepare radio to listen on initial channel
    radio_rfOn();
    LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                  channel_vars.rx_fine);

    // begin to listen
    radio_rxEnable();
    radio_rxNow();
#elif SELECTED_SCUM == 17
    // set timeout/reset timer
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 125000,
                               TIMER_CB_TX_BEACON);
#endif

    while (1) {
        app_vars.rxpk_done = 0;
        while (app_vars.rxpk_done == 0) {
            // fake sleep mode
            if (app_vars.printflag == 1) {
                printf("fine: %d, my count: %d, guard time: %d, IF: %d \r\n",
                       channel_vars.rx_fine_sync, time_sync_vars.rx_EB_timer,
                       app_vars.current_count -
                           time_sync_vars.rx_EB_start_reception_time,
                       app_vars.IF_estimate);
                printf("IF ADC clock count: %d, and setting: %d\r\n",
                       app_vars.ADC_counter, app_vars.IF_fine_clk_setting);
                app_vars.printflag = 0;
            }
        }
    }
}

//=========================== private =========================================

void radio_startframe_cb(uint32_t timestamp) {
    // start+enable the IF ADC counter

    // reset all counters:
    ANALOG_CFG_REG__0 = 0x0000;

    // enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;

    // raise GPIO flag
    gpio_14_set();
}

void radio_rx_cb(uint32_t timestamp) {
    uint8_t i;
    uint32_t temp_storage_1;
    uint32_t temp_storage_2;

    // stop+disable counters:
    ANALOG_CFG_REG__0 = 0x007F;

    radio_getReceivedFrame(&(app_vars.packet[0]), &app_vars.packet_len,
                           sizeof(app_vars.packet), &app_vars.rxpk_rssi,
                           &app_vars.rxpk_lqi);

    app_vars.IF_estimate = radio_getIFestimate();

    // TODO - move this into main? or leave it in ISR?
    if (radio_getCrcOk()) {
        printf("CRC OK, ");
    } else {
        // CRC miss or packet not for me :)
        printf("CRC miss, ");
    }
    memset(app_vars.packet, 0, sizeof(app_vars.packet));

    printf("IF: %d\r\n", app_vars.IF_estimate);

    // first attempt - continuously receive :)
    radio_rxEnable();
    radio_rxNow();
}

void tx_beacon_callback(void) {
    // transmit a beacon packet to be received by another SCuM

    uint16_t packet_timer_32k;

    app_vars.current_count = rftimer_readCounter();

    LC_FREQCHANGE(channel_vars.tx_coarse, channel_vars.tx_mid,
                  channel_vars.tx_fine);

    app_vars.tx_packet[0] = (uint8_t)(MY_ADDRESS >> 8);
    app_vars.tx_packet[1] = (uint8_t)(MY_ADDRESS & 0xFF);  // my address
    app_vars.tx_packet[2] = scumpong_vars.child_address_MSB;
    app_vars.tx_packet[3] = scumpong_vars.child_address_LSB;
    app_vars.tx_packet[4] = 0x00;
    app_vars.tx_packet[5] = 0x44;  // indicates a response to a join request.
    app_vars.tx_packet[6] = 0x4F;
    app_vars.tx_packet[7] =
        0xF9;  // give child its dedicated slot (hard coded for now...)
    app_vars.tx_packet[8] = 0x15;
    packet_timer_32k =
        (time_sync_vars.tx_EB_timer_from_parent / 4000000) * 32768;
    app_vars.tx_packet[9] = 0x3F;
    app_vars.tx_packet[10] = 0x98;

    // scumpong_vars.ack_join_request = 0;

    radio_loadPacket(app_vars.tx_packet, TX_PACKET_LEN);
    radio_txEnable();
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 1000, 1);

    rftimer_setCompareIn_by_id(
        app_vars.current_count + 125000,
        TIMER_CB_TX_BEACON);  // send a packet in the dedicated slot
}

void transmit_delay_callback(void) {
    radio_txNow();
    printf("sent a packet\r\n");
}

void tx_endframe_callback(uint32_t timestamp) {
    radio_rfOff();
    memset(app_vars.packet, 0, sizeof(app_vars.packet));
    memset(app_vars.tx_packet, 0, sizeof(app_vars.tx_packet));
}