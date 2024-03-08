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
#define TIMER_CB_TX_UPLINK 5
#define TIMER_CB_TX_EB 6
#define TIMER_CB_RX_EB 7

// === LOW-LEVEL RADIO FSM DEFINES === //
#define YES_LISTEN_FOR_ACK 1
#define DO_NOT_LISTEN_FOR_ACK 0

// === CHOOSE BETWEEN THE SCUMS === //
#define SELECTED_SCUM 17  // PROG PORT 27, UART PORT// 17
// #define SELECTED_SCUM 6
// // PROG PORT 37, UART PORT 6 #define SELECTED_SCUM 4 // PROG PORT 4, UART
// PORT LOGIC ANALYZER #define SELECTED_SCUM 77 // TEENSY OPTICAL PROG. PORT, NO
// UART

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
#define TX_LC_FINE 18

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
#define RX_LC_COARSE 25
#define RX_LC_MID 2
#define RX_LC_FINE 13
#define RX_LC_FINE_SYNC RX_LC_FINE + RX_FINE_CODE_FUDGE
#define TX_LC_COARSE 25
#define TX_LC_MID 2
#define TX_LC_FINE 23

// address
#define MY_ADDRESS 0x4EE7
#define MY_ADDRESS_MSB 0x4E
#define MY_ADDRESS_LSB 0xE7

// parent address
#define PREF_PARENT_MSB 0xBB
#define PREF_PARENT_LSB 0xAA

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
void test_rf_timer_callback(void);
void rx_timeout_callback(void);
void join_request_timer_callback(void);
void transmit_EB_timer_callback(void);
void rx_startframe_timeout_callback(void);
void transmit_delay_callback(void);
void receive_delay_callback(void);

void tune_fine_codes(uint32_t IF_estimate);
void tune_fine_rx_sync_code(uint32_t IF_estimate);

uint32_t read_IF_ADC_counter(void);
uint32_t read_2M_counter(void);
void enable_counters(void);

uint8_t destination_address(uint8_t msb, uint8_t lsb);

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
    rftimer_set_callback_by_id(receive_delay_callback, TIMER_CB_TX_RX_DELAY);
    rftimer_set_callback_by_id(rx_timeout_callback, TIMER_CB_RX_TIMEOUT);
    rftimer_set_callback_by_id(rx_startframe_timeout_callback,
                               TIMER_CB_SF_TIMEOUT);
    rftimer_set_callback_by_id(join_request_timer_callback, TIMER_CB_TX_UPLINK);
    rftimer_set_callback_by_id(transmit_EB_timer_callback, TIMER_CB_TX_EB);
    rftimer_set_callback_by_id(test_rf_timer_callback, TIMER_CB_RX_EB);

    scumpong_vars.sync_state = DESYNCHED;
    rftimer_enable_interrupts();
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_RDY_WAIT);
    rftimer_enable_interrupts_by_id(TIMER_CB_RX_TIMEOUT);
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_UPLINK);
    rftimer_enable_interrupts_by_id(TIMER_CB_SF_TIMEOUT);
    rftimer_enable_interrupts_by_id(TIMER_CB_TX_EB);
    rftimer_enable_interrupts_by_id(TIMER_CB_RX_EB);

    // get calibrated IF settings:
    app_vars.IF_coarse_clk_setting = scm3c_hw_interface_get_IF_coarse();
    app_vars.IF_fine_clk_setting = scm3c_hw_interface_get_IF_fine();

    // prepare radio to listen on initial channel
    radio_rfOn();
    LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                  channel_vars.rx_fine);

    // begin to listen
    radio_rxEnable();
    radio_rxNow();

    // set timeout/reset timer
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 6000,
                               TIMER_CB_SF_TIMEOUT);

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

    // TODO - move this into main? or leave it in ISR?
    if (radio_getCrcOk()) {
        // app_vars.capture_current_time = rftimer_readCounter();
        if ((destination_address(
                 app_vars.packet[DESTINATION_ADDRESS_MSB_INDEX],
                 app_vars.packet[DESTINATION_ADDRESS_LSB_INDEX]) ==
             PACKET_EB) &&
            (app_vars.packet[SOURCE_ADDRESS_MSB_INDEX] == PREF_PARENT_MSB) &&
            (app_vars.packet[SOURCE_ADDRESS_LSB_INDEX] == PREF_PARENT_LSB)) {
            app_vars.current_count = rftimer_readCounter();

            // store IF counter result:
            app_vars.ADC_counter = read_IF_ADC_counter();

            gpio_15_clr();
            gpio_14_clr();
            gpio_13_clr();
            gpio_12_clr();
            app_vars.IF_estimate = radio_getIFestimate();
            scumpong_vars.last_EB_received = 1;

            // app_vars.printflag = 1;

            if (scumpong_vars.sync_state == DESYNCHED) {
                app_vars.sync_counter++;
                tune_fine_codes(app_vars.IF_estimate);
            } else {
                radio_rfOff();
            }

            // extract timing information for next EB from packet:
            temp_storage_1 = ((uint32_t)(app_vars.packet[4]));
            temp_storage_2 = ((uint32_t)(app_vars.packet[5]));
            time_sync_vars.rx_EB_timer_from_packet =
                (((((temp_storage_1 << 8) | (temp_storage_2)) + 1) *
                  ((uint32_t)(40000))) /
                 32768) *
                100;

            // extract timing information for join request from packet:
            temp_storage_1 = ((uint32_t)(app_vars.packet[6]));
            temp_storage_2 = ((uint32_t)(app_vars.packet[7]));
            time_sync_vars.tx_join_req_timer_from_parent =
                (((((temp_storage_1 << 8) | (temp_storage_2)) + 1) *
                  ((uint32_t)(5000))) /
                 32768) *
                100;
            if (scumpong_vars.sync_state == DESYNCHED) {
                time_sync_vars.rx_EB_timer =
                    time_sync_vars.rx_EB_timer - RX_EB_BACKOFF;
                // === for some reason, the timer gets messed up for very long
                // EB periods, reasons unknown
                // rftimer_setCompareIn_by_id(app_vars.current_count +
                // time_sync_vars.rx_EB_timer + RX_EB_BACKOFF, TIMER_CB_RX_EB );
                // if (SELECTED_SCUM==4) {
                //		time_sync_vars.rx_EB_timer = 1996650;
                //}
                rftimer_setCompareIn_by_id(app_vars.current_count +
                                               time_sync_vars.rx_EB_timer -
                                               RX_EB_BACKOFF,
                                           TIMER_CB_RX_EB);
                // time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer +
                // RX_EB_BACKOFF; time_sync_vars.rx_EB_timer =
                // time_sync_vars.rx_EB_timer + RX_EB_BACKOFF;
                app_vars.dont_pll = 0;
            } else if ((scumpong_vars.sync_state == SYNCHED) ||
                       (scumpong_vars.sync_state == JOINED)) {
                // update the rx EB timer :D
                // over-the-air "PLL" goes here.

                if (((app_vars.current_count -
                      time_sync_vars.rx_EB_start_reception_time) >
                     (RX_EB_GUARD_TIME_TARGET + RX_EB_PACKET_DURATION) + 500)) {
                    if (app_vars.dont_pll == 0) {
                        time_sync_vars.rx_EB_timer =
                            time_sync_vars.rx_EB_timer + 200;
                    } else {
                        time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer;
                    }
                    // tune_fine_rx_sync_code(app_vars.IF_estimate);
                    tune_fine_codes(app_vars.IF_estimate);
                } else if (((app_vars.current_count -
                             time_sync_vars.rx_EB_start_reception_time) <
                            (RX_EB_GUARD_TIME_TARGET + RX_EB_PACKET_DURATION) -
                                200)) {
                    if (app_vars.dont_pll == 0) {
                        time_sync_vars.rx_EB_timer =
                            time_sync_vars.rx_EB_timer - 200;
                    } else {
                        time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer;
                    }
                    // tune_fine_rx_sync_code(app_vars.IF_estimate);
                    tune_fine_codes(app_vars.IF_estimate);
                } else if ((app_vars.current_count -
                            time_sync_vars.rx_EB_start_reception_time) >
                           (RX_EB_GUARD_TIME_TARGET + RX_EB_PACKET_DURATION) +
                               200) {
                    if (app_vars.dont_pll == 0) {
                        time_sync_vars.rx_EB_timer =
                            time_sync_vars.rx_EB_timer + 50;
                    } else {
                        time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer;
                    }
                    // tune_fine_rx_sync_code(app_vars.IF_estimate);
                    tune_fine_codes(app_vars.IF_estimate);
                } else if ((app_vars.current_count -
                            time_sync_vars.rx_EB_start_reception_time) <
                           (RX_EB_GUARD_TIME_TARGET + RX_EB_PACKET_DURATION)) {
                    // time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer -
                    // 50;
                    time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer;
                    app_vars.dont_pll = 1;
                    // tune_fine_rx_sync_code(app_vars.IF_estimate);
                    tune_fine_codes(app_vars.IF_estimate);
                } else {
                    time_sync_vars.rx_EB_timer = time_sync_vars.rx_EB_timer;
                    tune_fine_codes(app_vars.IF_estimate);
                }

                // rftimer_setCompareIn_by_id(app_vars.current_count +
                // time_sync_vars.rx_EB_timer , TIMER_CB_RX_EB );
                rftimer_setCompareIn_by_id(app_vars.current_count +
                                               time_sync_vars.rx_EB_timer -
                                               RX_IF_BACKOFF,
                                           TIMER_CB_RX_EB);
                app_vars.IF_on_early = 1;
                time_sync_vars.i_was_early_by =
                    app_vars.current_count -
                    time_sync_vars.rx_EB_start_reception_time;
                // rftimer_setCompareIn_by_id(app_vars.current_count +
                // time_sync_vars.rx_EB_timer - RX_EB_BACKOFF, TIMER_CB_RX_EB );
                // rftimer_setCompareIn_by_id(app_vars.current_count +
                // time_sync_vars.rx_EB_timer + RX_EB_BACKOFF, TIMER_CB_RX_EB );
            }

            // get parent address:
            temp_storage_1 = ((uint16_t)(app_vars.packet[0]));
            temp_storage_2 = ((uint16_t)(app_vars.packet[1]));
            scumpong_vars.parent_address =
                (temp_storage_1 << 8) | (temp_storage_2);
            if (scumpong_vars.sync_state == DESYNCHED) {
                // if (((app_vars.current_count - app_vars.previous_count) >
                // time_sync_vars.rx_EB_timer_from_packet*1.5)) {
                if (((app_vars.current_count - app_vars.previous_count) >
                     TIME_INTERVAL * 1.5)) {
                    // missed an EB during sync procedure.
                    if (time_sync_vars.rx_EB_timer == 0) {
                        // time_sync_vars.rx_EB_timer =
                        // time_sync_vars.rx_EB_timer_from_packet - 2000;
                        time_sync_vars.rx_EB_timer =
                            time_sync_vars.rx_EB_timer_from_packet;
                    }
                } else {
                    // packet rate seems correct
                    // use time rate
                    time_sync_vars.rx_EB_timer =
                        app_vars.current_count -
                        app_vars.previous_count;  // maintain a "count"
                }
            }
        } else if (destination_address(
                       app_vars.packet[DESTINATION_ADDRESS_MSB_INDEX],
                       app_vars.packet[DESTINATION_ADDRESS_LSB_INDEX]) ==
                   PACKET_FOR_ME) {  // indicates a packet unicast to me
            if ((scumpong_vars.sync_state == SYNCHED) ||
                (scumpong_vars.sync_state == JOINED)) {  //
                gpio_15_clr();
                gpio_14_clr();
                gpio_13_clr();
                gpio_12_clr();
            }
            if (((app_vars.packet[4] == 0x00) &&
                 (app_vars.packet[5] ==
                  0x44))) {  // indicates either join request ack. (from parent)
                             // or join request (from new child)

                if (((app_vars.packet[SOURCE_ADDRESS_MSB_INDEX] ==
                      PREF_PARENT_MSB) &&
                     (app_vars.packet[SOURCE_ADDRESS_LSB_INDEX] ==
                      PREF_PARENT_LSB))) {  // parent responded to my join
                                            // request
                    temp_storage_1 = ((uint32_t)(app_vars.packet[6]));
                    temp_storage_2 = ((uint32_t)(app_vars.packet[7]));
                    time_sync_vars.tx_dedicated_slot_from_parent =
                        (((((temp_storage_1 << 8) | (temp_storage_2)) + 1) *
                          ((uint32_t)(5000))) /
                         32768) *
                        100;  // extract dedicated slot TX time
                    temp_storage_1 = ((uint32_t)(app_vars.packet[9]));
                    temp_storage_2 = ((uint32_t)(app_vars.packet[10]));
                    time_sync_vars.tx_EB_timer_from_parent =
                        (((((temp_storage_1 << 8) | (temp_storage_2)) + 1) *
                          ((uint32_t)(40000))) /
                         32768) *
                        100;  // extract dedicated EB TX time
                    scumpong_vars.sync_state = JOINED;
                    // printf("tx EB at: %d\r\n",
                    // time_sync_vars.tx_EB_timer_from_parent);
                    radio_rfOff();  // turn radio off immediately
                    printf("JOINED, tx EB at: %d, ded. slot at %d \r\n",
                           time_sync_vars.tx_EB_timer_from_parent,
                           time_sync_vars.tx_dedicated_slot_from_parent);
                } else {  // set everything up so that the child knows when to
                          // speak:
                    // store child address:

                    scumpong_vars.child_address_MSB =
                        app_vars.packet[SOURCE_ADDRESS_MSB_INDEX];
                    scumpong_vars.child_address_LSB =
                        app_vars.packet[SOURCE_ADDRESS_LSB_INDEX];
                    scumpong_vars.ack_join_request = 1;
                }
            }
        }

        if (app_vars.sync_counter == NUM_RX_EBS_FOR_SYNC) {
            scumpong_vars.sync_state = SYNCHED;
            app_vars.sync_counter++;  // disable so that more advanced states
                                      // are not treated as SYNCHED again
        }

        // response state machine
        if (scumpong_vars.sync_state ==
            SYNCHED) {      // if synched - send a join request
            radio_rfOff();  // do not listen forever
            if ((ONLY_LISTEN == 0) &&
                (EB_ONLY == 0)) {  // debug mode - do not send join requests
                rftimer_setCompareIn_by_id(
                    app_vars.current_count +
                        time_sync_vars.tx_join_req_timer_from_parent,
                    TIMER_CB_TX_UPLINK);  // send a join request
            }
            if (EB_ONLY == 1) {  // debug mode - only send EBs
                scumpong_vars.sync_state = JOINED;
                time_sync_vars.tx_EB_timer_eb_only =
                    time_sync_vars.rx_EB_timer - EB_TIMER_SKEW;
                time_sync_vars.tx_EB_timer_from_parent =
                    time_sync_vars.rx_EB_timer;
            }
            // disable the weird hack interrupt
            rftimer_disable_interrupts_by_id(TIMER_CB_SF_TIMEOUT);
        } else if (scumpong_vars.sync_state ==
                   JOINED) {  // if join request was acknowledged, I am in the
                              // network, send in my dedicated slot and wait for
                              // response
            radio_rfOff();    // do not listen forever
            scumpong_vars.desync_risk = 0;
            if (scumpong_vars.ack_join_request == 1) {
                rftimer_setCompareIn_by_id(
                    rftimer_readCounter() + 500,
                    TIMER_CB_TX_UPLINK);  // acknowledge the child join request.
            } else {
                if (EB_ONLY == 0) {  // only send in dedicated slot if we are
                                     // not in EB only debug mode
                    rftimer_setCompareIn_by_id(
                        app_vars.current_count +
                            time_sync_vars.tx_dedicated_slot_from_parent,
                        TIMER_CB_TX_UPLINK);  // send a packet in the dedicated
                                              // slot
                }
            }
            // rftimer_setCompareIn_by_id(app_vars.current_count +
            // time_sync_vars.tx_EB_timer_from_parent - RX_IF_BACKOFF,
            // TIMER_CB_TX_EB ); // send an EB if (EB_ONLY == 1) {
            //		time_sync_vars.tx_EB_timer_from_parent =
            // time_sync_vars.rx_EB_timer - EB_TIMER_SKEW;
            // }
            if (EB_ONLY == 1) {
                rftimer_setCompareIn_by_id(
                    app_vars.current_count +
                        time_sync_vars.tx_EB_timer_eb_only - RX_IF_BACKOFF,
                    TIMER_CB_TX_EB);  // send an EB
            } else {
                rftimer_setCompareIn_by_id(
                    app_vars.current_count +
                        time_sync_vars.tx_EB_timer_from_parent,
                    TIMER_CB_TX_EB);  // send an EB
            }
        } else {  // in DESYNCH, keep listening
            // keep listening
            radio_rxEnable();
            radio_rxNow();
        }

        // debug print: TODO move debug prints into main...
        /*if (scumpong_vars.sync_state == DESYNCHED) {
                        printf("my count: %d, RX EB packet: %d, join req. time
        %d, IF: %d \r\n", app_vars.current_count - app_vars.previous_count,
        time_sync_vars.rx_EB_timer_from_packet,
        time_sync_vars.tx_join_req_timer_from_parent, app_vars.IF_estimate);
        }
        if ((scumpong_vars.sync_state == SYNCHED)||(scumpong_vars.sync_state ==
        JOINED)) { printf("my count: %d, will listen at: %d, IF: %d \r\n",
        app_vars.current_count - app_vars.previous_count,
        time_sync_vars.rx_EB_timer, app_vars.IF_estimate); printf("time between
        rf on and rx end: %d\r\n",
        app_vars.current_count-time_sync_vars.rx_EB_start_reception_time);
        }*/

        // debug prints:
        app_vars.previous_count = app_vars.current_count;

        app_vars.source_address =
            (app_vars.packet[0] << 8) + app_vars.packet[1];
        app_vars.destin_address =
            (app_vars.packet[2] << 8) + app_vars.packet[3];
        app_vars.pack_type = (app_vars.packet[4] << 8) + app_vars.packet[5];
        app_vars.printflag = 1;

        // modify IF ADC clock, if necessary
        /*if(app_vars.ADC_counter > (5685+5)) {
app_vars.IF_fine_clk_setting += 1;
}
if(app_vars.ADC_counter < (5685-5)) {
app_vars.IF_fine_clk_setting -= 1;
}

        if(app_vars.IF_fine_clk_setting <= 5) {
                        app_vars.IF_fine_clk_setting += 10;
                        app_vars.IF_coarse_clk_setting -= 1;
        }

        set_IF_clock_frequency(app_vars.IF_coarse_clk_setting,
app_vars.IF_fine_clk_setting, 0);
scm3c_hw_interface_set_IF_coarse(app_vars.IF_coarse_clk_setting);
scm3c_hw_interface_set_IF_fine(app_vars.IF_fine_clk_setting);

analog_scan_chain_write();
analog_scan_chain_load();*/

    } else {
        // CRC miss or packet not for me :)
        radio_rxEnable();
        radio_rxNow();
    }
    memset(app_vars.packet, 0, sizeof(app_vars.packet));
}

void test_rf_timer_callback(void) {
    uint8_t i;

    if (app_vars.IF_on_early == 1) {
        rftimer_setCompareIn_by_id(rftimer_readCounter() + RX_IF_BACKOFF,
                                   TIMER_CB_RX_EB);
        ANALOG_CFG_REG__10 = 0x0010;
        ANALOG_CFG_REG__16 = 0x1;
        app_vars.IF_on_early = 0;
    } else {
        time_sync_vars.rx_EB_start_reception_time = rftimer_readCounter();
        gpio_15_set();

        if ((scumpong_vars.sync_state == SYNCHED) ||
            (scumpong_vars.sync_state == JOINED)) {
            LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                          channel_vars.rx_fine_sync);
            // LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
            // channel_vars.rx_fine);

            // start a timeout timer
            rftimer_setCompareIn_by_id(rftimer_readCounter() + RX_TIMEOUT_TIME,
                                       TIMER_CB_RX_TIMEOUT);  // 5000 = 10ms
            scumpong_vars.last_EB_received = 0;
        }
        radio_rxEnable();
        radio_rxNow();
    }
}

void join_request_timer_callback(void) {
    // transmit a join req. packet

    uint16_t packet_timer_32k;

    LC_FREQCHANGE(channel_vars.tx_coarse, channel_vars.tx_mid,
                  channel_vars.tx_fine);

    if (scumpong_vars.ack_join_request == 1) {
        // prepare packet
        app_vars.tx_packet[0] = (uint8_t)(MY_ADDRESS >> 8);
        app_vars.tx_packet[1] = (uint8_t)(MY_ADDRESS & 0xFF);  // my address
        app_vars.tx_packet[2] = scumpong_vars.child_address_MSB;
        app_vars.tx_packet[3] = scumpong_vars.child_address_LSB;
        app_vars.tx_packet[4] = 0x00;
        app_vars.tx_packet[5] =
            0x44;  // indicates a response to a join request.
        app_vars.tx_packet[6] = 0x4F;
        app_vars.tx_packet[7] =
            0xF9;  // give child its dedicated slot (hard coded for now...)
        app_vars.tx_packet[8] = 0x15;
        packet_timer_32k =
            (time_sync_vars.tx_EB_timer_from_parent / 4000000) * 32768;
        app_vars.tx_packet[9] = 0x3F;
        app_vars.tx_packet[10] = 0x98;

        scumpong_vars.ack_join_request = 0;

        radio_loadPacket(app_vars.tx_packet, TX_PACKET_LEN);
        radio_txEnable();
        rftimer_setCompareIn_by_id(rftimer_readCounter() + 1000, 1);

        rftimer_setCompareIn_by_id(
            app_vars.current_count +
                time_sync_vars.tx_dedicated_slot_from_parent,
            TIMER_CB_TX_UPLINK);  // send a packet in the dedicated slot

        // indicate that I DO want to listen for an ACK for this downlink
        // packet:
        scumpong_vars.listen_for_ack = DO_NOT_LISTEN_FOR_ACK;
    }

    else {
        // prepare packet:
        app_vars.tx_packet[0] = (uint8_t)(MY_ADDRESS >> 8);
        app_vars.tx_packet[1] = (uint8_t)(MY_ADDRESS & 0xFF);  // my address
        app_vars.tx_packet[2] = (uint8_t)(scumpong_vars.parent_address >> 8);
        app_vars.tx_packet[3] = (uint8_t)((scumpong_vars.parent_address) &
                                          0xFF);  // destination address
        app_vars.tx_packet[4] = 0x00;
        app_vars.tx_packet[5] = 0x44;

        radio_loadPacket(app_vars.tx_packet, TX_PACKET_LEN);
        radio_txEnable();
        rftimer_setCompareIn_by_id(rftimer_readCounter() + 1000, 1);

        // indicate that I DO want to listen for an ACK for this uplink packet
        scumpong_vars.listen_for_ack = YES_LISTEN_FOR_ACK;
    }
}
void transmit_EB_timer_callback(void) {
    // transmit an EB packet

    time_sync_vars.current_downlink_time = rftimer_readCounter() + 5000;

    LC_FREQCHANGE(channel_vars.tx_coarse, channel_vars.tx_mid,
                  channel_vars.tx_fine);
    // prepare packet:
    app_vars.tx_packet[0] = (uint8_t)(MY_ADDRESS >> 8);
    app_vars.tx_packet[1] = (uint8_t)(MY_ADDRESS & 0xFF);
    app_vars.tx_packet[2] = 0xFF;
    app_vars.tx_packet[3] = 0xFF;
    app_vars.tx_packet[4] =
        (uint8_t)((time_sync_vars.tx_EB_timer_from_parent >> 8) & 0x00FF);
    app_vars.tx_packet[5] =
        (uint8_t)(time_sync_vars.tx_EB_timer_from_parent & 0x00FF);
    app_vars.tx_packet[6] = 0x66;
    app_vars.tx_packet[7] = 0x66;
    app_vars.tx_packet[8] = 0x12;

    radio_loadPacket(app_vars.tx_packet, TX_PACKET_LEN);
    radio_txEnable();
    rftimer_setCompareIn_by_id(time_sync_vars.current_downlink_time,
                               TIMER_CB_TX_RDY_WAIT);
    // indicate that I do not want to listen for an ACK because this is an EB
    scumpong_vars.listen_for_ack = DO_NOT_LISTEN_FOR_ACK;

    // set up timer to listen for join request:
    if (EB_ONLY == 0) {
        scumpong_vars.future_listen_in_EB = 1;
        rftimer_setCompareIn_by_id(
            time_sync_vars.current_downlink_time +
                (((((0x66 << 8) | (0x66)) + 1) * ((uint32_t)(5000))) / 32768) *
                    100,
            TIMER_CB_TX_RX_DELAY);
    }
}

void transmit_delay_callback(void) { radio_txNow(); }

void receive_delay_callback(void) {
    LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                  channel_vars.rx_fine);
    radio_rxEnable();
    radio_rxNow();
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 20000,
                               TIMER_CB_RX_TIMEOUT);

    if (scumpong_vars.future_listen_in_EB == 1) {
        rftimer_setCompareIn_by_id(
            time_sync_vars.current_downlink_time +
                (((((0x66 << 8) | (0x66)) + 1) * ((uint32_t)(5000))) / 32768) *
                    100,
            TIMER_CB_TX_RX_DELAY);
        scumpong_vars.future_listen_in_EB = 0;
        scumpong_vars.in_case_of_EB_miss = 1;
    }
}

void rx_timeout_callback(void) {
    radio_rfOff();
    if (scumpong_vars.in_case_of_EB_miss == 1) {
        scumpong_vars.in_case_of_EB_miss = 0;
    } else if ((scumpong_vars.last_EB_received == 0) &&
               (scumpong_vars.in_case_of_EB_miss ==
                0)) {  // missed an EB, have not yet listened for join req.
        app_vars.current_count = rftimer_readCounter();
        // if (scumpong_vars.desync_risk == 0) {
        // rftimer_setCompareIn_by_id(app_vars.current_count +
        // time_sync_vars.rx_EB_timer - RX_EB_BACKOFF - RX_TIMEOUT_TIME -
        // RX_IF_BACKOFF - RX_EB_GUARD_TIME_TARGET, TIMER_CB_RX_EB);
        rftimer_setCompareIn_by_id(
            app_vars.current_count + time_sync_vars.rx_EB_timer -
                RX_EB_BACKOFF - RX_IF_BACKOFF - RX_EB_GUARD_TIME_TARGET - 2500,
            TIMER_CB_RX_EB);
        app_vars.IF_on_early = 1;
        if (EB_ONLY == 0) {
            rftimer_setCompareIn_by_id(
                app_vars.current_count +
                    time_sync_vars.tx_EB_timer_from_parent - RX_EB_BACKOFF -
                    RX_TIMEOUT_TIME + time_sync_vars.i_was_early_by +
                    RX_EB_PACKET_DURATION,
                TIMER_CB_TX_EB);  // still send an EB
        } else {
            // rftimer_setCompareIn_by_id(app_vars.current_count +
            // time_sync_vars.tx_EB_timer_eb_only - RX_EB_BACKOFF -
            // RX_TIMEOUT_TIME - RX_EB_GUARD_TIME_TARGET -
            // RX_EB_PACKET_DURATION, TIMER_CB_TX_EB ); // still send an EB
            rftimer_setCompareIn_by_id(app_vars.current_count +
                                           time_sync_vars.tx_EB_timer_eb_only -
                                           RX_EB_BACKOFF - RX_TIMEOUT_TIME -
                                           RX_EB_PACKET_DURATION - 1000,
                                       TIMER_CB_TX_EB);  // still send an EB
        }
        time_sync_vars.rx_EB_timer =
            time_sync_vars.rx_EB_timer - RX_EB_BACKOFF - 2500;
        //}
        // else if (scumpong_vars.desync_risk == 1) {
        //		rftimer_setCompareIn_by_id(app_vars.current_count +
        // time_sync_vars.rx_EB_timer, TIMER_CB_RX_EB);
        //		rftimer_setCompareIn_by_id(app_vars.current_count +
        // time_sync_vars.tx_EB_timer_from_parent, TIMER_CB_TX_EB ); // still
        // send an EB 		time_sync_vars.rx_EB_timer =
        // time_sync_vars.rx_EB_timer;
        //}

        LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                      channel_vars.rx_fine);
        channel_vars.rx_fine_sync = channel_vars.rx_fine + RX_FINE_CODE_FUDGE;
        // channel_vars.rx_fine_sync = channel_vars.rx_fine;

        app_vars.previous_count = 0;
        scumpong_vars.desync_risk = 1;
    }
}

void tx_endframe_callback(uint32_t timestamp) {
    radio_rfOff();
    LC_FREQCHANGE(channel_vars.rx_coarse, channel_vars.rx_mid,
                  channel_vars.rx_fine);

    // in case of ack - delay for a little bit between TX and RX, otherwise the
    // radio is unhappy
    if (scumpong_vars.listen_for_ack == YES_LISTEN_FOR_ACK) {
        rftimer_setCompareIn_by_id(rftimer_readCounter() + 500,
                                   TIMER_CB_TX_RX_DELAY);
    }
    memset(app_vars.packet, 0, sizeof(app_vars.packet));
    memset(app_vars.tx_packet, 0, sizeof(app_vars.tx_packet));
}

void rx_startframe_timeout_callback(void) {
    uint16_t i;
    radio_rfOff();
    for (i = 0; i < 0xFF; i++);
    if (scumpong_vars.sync_state == DESYNCHED) {
        radio_rxEnable();
        radio_rxNow();
    }
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 100000,
                               TIMER_CB_SF_TIMEOUT);
}

// tidying functions
uint8_t destination_address(uint8_t msb, uint8_t lsb) {
    uint8_t destination;

    if ((msb == 0xFF) && (lsb == 0xFF)) {
        destination = PACKET_EB;
        return destination;
    } else if ((msb == (uint8_t)(MY_ADDRESS >> 8)) &&
               (lsb == (uint8_t)(MY_ADDRESS & 0xFF))) {
        destination = PACKET_FOR_ME;
        return destination;
    } else {
        destination = PACKET_NOT_FOR_ME;
        return destination;
    }
}

uint8_t packet_type(uint8_t msb, uint8_t lsb) { uint8_t type; }

void set_channel(uint8_t channel, uint8_t TX_or_RX) {}

void tune_fine_codes(uint32_t IF_estimate) {
    if (IF_estimate > 525) {
        // increase channel codes
        channel_vars.rx_fine++;
        channel_vars.tx_fine++;
        channel_vars.rx_fine_sync++;
        // channel_vars.rx_fine_desync++;
    } else if (IF_estimate < 480) {
        // decrease channel codes
        channel_vars.rx_fine--;
        channel_vars.tx_fine--;
        channel_vars.rx_fine_sync--;
        // channel_vars.rx_fine_desync--;
    }
}

void tune_fine_rx_sync_code(uint32_t IF_estimate) {
    if (IF_estimate > 530) {
        channel_vars.rx_fine_sync++;
    } else if (IF_estimate <= 475) {
        channel_vars.rx_fine_sync--;
    }
}

uint32_t read_IF_ADC_counter() {
    uint32_t count_IF;
    uint16_t rdata_lsb;
    uint16_t rdata_msb;

    // Read IF ADC_CLK counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x300000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x340000);
    count_IF = rdata_lsb + (rdata_msb << 16);

    // reset counters
    ANALOG_CFG_REG__0 = 0x0000;

    // enable counter
    // ANALOG_CFG_REG__0 = 0x3FFF;

    // return
    return count_IF;
}
uint32_t read_2M_counter() {
    uint32_t count_2M;
    uint16_t rdata_lsb;
    uint16_t rdata_msb;

    // Read 2M counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
    count_2M = rdata_lsb + (rdata_msb << 16);

    // reset counter
    ANALOG_CFG_REG__0 = 0x0008;

    // enable counter
    ANALOG_CFG_REG__0 = 0x3FFF;

    // return
    return count_2M;
}

void enable_counters() {
    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;
}