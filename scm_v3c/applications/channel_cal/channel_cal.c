// This application finds the SCuM's tuning codes for all (or many) 802.15.4
// channels with the help of an OpenMote.

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "802_15_4.h"
#include "memory_map.h"
#include "optical.h"
#include "radio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"
#include "tuning.h"

// Maximum number of TX tuning codes per channel.
#define MAX_NUM_TX_TUNING_CODES_PER_CHANNEL 4

// Maximum number of RX tuning codes.
#define MAX_NUM_RX_TUNING_CODES 80

// Minimum number of IF counts to accept a RX tuning code.
#define MIN_NUM_IF_COUNTS 475

// Maximum number of IF counts to accept a RX tuning code.
#define MAX_NUM_IF_COUNTS 525

// Minimum RX tuning fine code to proceed to the next 802.15.4 channel.
#define MIN_RX_TUNING_FINE_CODE 9

// Channel calibration state enum.
typedef enum {
    CHANNEL_CAL_STATE_INVALID = -1,
    CHANNEL_CAL_STATE_TX = 0,
    CHANNEL_CAL_STATE_RX = 1,
    CHANNEL_CAL_STATE_RX_ACK = 2,
    CHANNEL_CAL_STATE_DONE = 3,
} channel_cal_state_e;

// Channel calibration TX command.
typedef enum {
    CHANNEL_CAL_COMMAND_NONE = 0x00,
    CHANNEL_CAL_COMMAND_CHANGE_CHANNEL = 0xFF,
} channel_cal_command_e;

// Channel calibration TX packet for acknowledgements.
typedef struct __attribute__((packed)) {
    // Sequence number.
    uint8_t sequence_number;

    // Reserved.
    uint8_t reserved1;

    // Reserved.
    uint8_t reserved2;

    // Reserved.
    uint8_t reserved3;

    // Command for the OpenMote.
    channel_cal_command_e command : 8;

    // Reserved.
    uint8_t reserved4;

    // Tuning code.
    tuning_code_t tuning_code;

    // Reserved.
    uint8_t reserved5;

    // Reserved.
    uint8_t reserved6;

    // Reserved.
    uint8_t reserved7;
} channel_cal_tx_packet_t;

// Channel calibration RX packet.
typedef struct __attribute__((packed)) {
    // Sequence number.
    uint8_t sequence_number;

    // Channel.
    uint8_t channel;

    // Transmit tuning codes.
    tuning_code_t tx_tuning_codes[MAX_NUM_TX_TUNING_CODES_PER_CHANNEL];
} channel_cal_rx_packet_t;

// Channel calibration RX tuning code.
typedef struct __attribute__((packed)) {
    // Channel.
    uint8_t channel;

    tuning_code_t tuning_code;
} channel_cal_rx_tuning_code_t;

// TX tuning code.
static tuning_code_t g_tx_tuning_code;

// RX tuning code.
static tuning_code_t g_rx_tuning_code;

// TX sweep configuration. The sweep range can be reduced if we do not want to
// find all 802.15.4 channels.
static const tuning_sweep_config_t g_tx_sweep_config = {
    .coarse =
        {
            .start = 24,
            .end = 26,
        },
    .mid =
        {
            .start = TUNING_MIN_CODE,
            .end = TUNING_MAX_CODE,
        },
    .fine =
        {
            .start = TUNING_MIN_CODE,
            .end = TUNING_MAX_CODE,
        },
};

// RX sweep configuration. The sweep range can be reduced if we do not want to
// find all 802.15.4 channels.
static const tuning_sweep_config_t g_rx_sweep_config = {
    .coarse =
        {
            .start = 24,
            .end = 26,
        },
    .mid =
        {
            .start = TUNING_MIN_CODE,
            .end = TUNING_MAX_CODE,
        },
    .fine =
        {
            .start = TUNING_MIN_CODE,
            .end = TUNING_MAX_CODE,
        },
};

// TX packet buffer.
static channel_cal_tx_packet_t g_tx_packet;

// Number of packets received.
static uint32_t g_rx_num_packets_received = 0;

// TX channel tuning codes.
static tuning_code_t
    g_tx_channel_tuning_codes[NUM_802_15_4_CHANNELS]
                             [MAX_NUM_TX_TUNING_CODES_PER_CHANNEL];

// RX channel tuning codes.
static channel_cal_rx_tuning_code_t
    g_rx_channel_tuning_codes[MAX_NUM_RX_TUNING_CODES];

// Channel calibration state.
static channel_cal_state_e g_channel_cal_state = CHANNEL_CAL_STATE_INVALID;

// Add some delay.
// TODO(fil): Change this function to use an RF timer compare register.
static inline void radio_delay(void) {
    for (uint8_t i = 0; i < 5; ++i) {
    }
}

static void radio_rx_callback(const uint8_t* packet, const uint8_t packet_len) {
    const uint32_t if_estimate = read_IF_estimate();
    radio_rfOff();

    const channel_cal_rx_packet_t* rx_packet =
        (const channel_cal_rx_packet_t*)packet;
    const uint8_t channel = rx_packet->channel;
    memcpy(g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL],
           rx_packet->tx_tuning_codes,
           MAX_NUM_TX_TUNING_CODES_PER_CHANNEL * sizeof(tuning_code_t));

    // Print the received transmit tuning codes.
    printf("Channel %u RX: (%u, %u, %u)\n", channel, g_rx_tuning_code.coarse,
           g_rx_tuning_code.mid, g_rx_tuning_code.fine);
    printf("IF estimate: %u\n", if_estimate);
    printf("Channel %u TX:", channel);
    for (uint8_t i = 0; i < MAX_NUM_TX_TUNING_CODES_PER_CHANNEL; ++i) {
        printf(" (%u, %u, %u)", rx_packet->tx_tuning_codes[i].coarse,
               rx_packet->tx_tuning_codes[i].mid,
               rx_packet->tx_tuning_codes[i].fine);
    }
    printf("\n");

    // Use the second set of tuning codes, if there are multiple tuning codes
    // that the OpenMote reecived from SCuM.
    if (g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL][1].coarse !=
            0 &&
        g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL][1].mid != 0 &&
        g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL][1].fine !=
            0) {
        g_tx_tuning_code =
            g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL][1];
    } else {
        g_tx_tuning_code =
            g_tx_channel_tuning_codes[channel - MIN_802_15_4_CHANNEL][0];
    }

    // Prepare the acknowledgement packet.
    memset(&g_tx_packet, 0, sizeof(channel_cal_tx_packet_t));

    if (if_estimate >= MIN_NUM_IF_COUNTS && if_estimate <= MAX_NUM_IF_COUNTS) {
        g_rx_channel_tuning_codes[g_rx_num_packets_received].channel = channel;
        g_rx_channel_tuning_codes[g_rx_num_packets_received].tuning_code =
            g_rx_tuning_code;
        ++g_rx_num_packets_received;

        if (g_rx_tuning_code.fine <= MIN_RX_TUNING_FINE_CODE) {
            g_tx_packet.command = CHANNEL_CAL_COMMAND_CHANGE_CHANNEL;
        }

        tuning_increment_mid_code_for_sweep(&g_rx_tuning_code,
                                            &g_rx_sweep_config);
    } else {
        tuning_increment_code_for_sweep(&g_rx_tuning_code, &g_rx_sweep_config);
    }

    radio_delay();
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 50 * 500, 7);
    g_channel_cal_state = CHANNEL_CAL_STATE_RX_ACK;
}

static void rf_timer_rx_callback(void) {
    tuning_increment_code_for_sweep(&g_rx_tuning_code, &g_rx_sweep_config);
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 20 * 500, 7);
}

int main(void) {
    initialize_mote();

    radio_setRxCb(radio_rx_callback);

    analog_scan_chain_write();
    analog_scan_chain_load();

    crc_check();
    perform_calibration();

    if (!tuning_validate_sweep_config(&g_tx_sweep_config)) {
        printf("Invalid TX sweep configuration.\n");
        return EXIT_FAILURE;
    }
    if (!tuning_validate_sweep_config(&g_rx_sweep_config)) {
        printf("Invalid RX sweep configuration.\n");
        return EXIT_FAILURE;
    }

    tuning_init_for_sweep(&g_tx_tuning_code, &g_tx_sweep_config);
    tuning_init_for_sweep(&g_rx_tuning_code, &g_rx_sweep_config);
    rftimer_set_callback_by_id(rf_timer_rx_callback, 7);
    rftimer_enable_interrupts();
    g_channel_cal_state = CHANNEL_CAL_STATE_TX;
    while (true) {
        radio_delay();
        switch (g_channel_cal_state) {
            case CHANNEL_CAL_STATE_TX: {
                memset(&g_tx_packet, 0, sizeof(channel_cal_tx_packet_t));
                g_tx_packet.sequence_number = 1;
                g_tx_packet.tuning_code = g_tx_tuning_code;

                tuning_tune_radio(&g_tx_tuning_code);
                send_packet(&g_tx_packet, sizeof(channel_cal_tx_packet_t));
                tuning_increment_code_for_sweep(&g_tx_tuning_code,
                                                &g_tx_sweep_config);

                if (tuning_end_of_sweep(&g_tx_tuning_code,
                                        &g_tx_sweep_config)) {
                    rftimer_enable_interrupts_by_id(7);
                    rftimer_setCompareIn_by_id(rftimer_readCounter() + 20 * 500,
                                               7);
                    g_channel_cal_state = CHANNEL_CAL_STATE_RX;
                }
                break;
            }
            case CHANNEL_CAL_STATE_RX: {
                tuning_tune_radio(&g_rx_tuning_code);
                radio_delay();
                receive_packet(/*timeout=*/true);

                if (tuning_end_of_sweep(&g_rx_tuning_code,
                                        &g_rx_sweep_config)) {
                    rftimer_disable_interrupts_by_id(7);
                    rftimer_disable_interrupts();
                    g_channel_cal_state = CHANNEL_CAL_STATE_DONE;
                }
                break;
            }
            case CHANNEL_CAL_STATE_RX_ACK: {
                if (g_tx_packet.command == CHANNEL_CAL_COMMAND_CHANGE_CHANNEL) {
                    for (uint8_t i = 0; i < 3; ++i) {
                        radio_delay();
                        tuning_tune_radio(&g_tx_tuning_code);
                        send_packet(&g_tx_packet,
                                    sizeof(channel_cal_tx_packet_t));
                        radio_delay();
                        radio_delay();
                        radio_delay();
                    }
                } else {
                    for (uint8_t i = 0; i < 2; ++i) {
                        radio_delay();
                        tuning_tune_radio(&g_tx_tuning_code);
                        send_packet(&g_tx_packet,
                                    sizeof(channel_cal_tx_packet_t));
                    }
                }

                g_channel_cal_state = CHANNEL_CAL_STATE_RX;
                break;
            }
            case CHANNEL_CAL_STATE_DONE: {
                // Print the channel calibration results.
                printf("TX packet results:\n");
                for (uint8_t i = 0; i < NUM_802_15_4_CHANNELS; ++i) {
                    for (uint8_t j = 0; j < MAX_NUM_TX_TUNING_CODES_PER_CHANNEL;
                         ++j) {
                        printf("Channel %u: (%u, %u, %u)\n",
                               i + MIN_802_15_4_CHANNEL,
                               g_tx_channel_tuning_codes[i][j].coarse,
                               g_tx_channel_tuning_codes[i][j].mid,
                               g_tx_channel_tuning_codes[i][j].fine);
                    }
                }
                printf("RX packet results:\n");
                for (uint8_t i = 0; i < MAX_NUM_RX_TUNING_CODES; ++i) {
                    printf("Channel %u: (%u, %u, %u)\n",
                           g_rx_channel_tuning_codes[i].channel,
                           g_rx_channel_tuning_codes[i].tuning_code.coarse,
                           g_rx_channel_tuning_codes[i].tuning_code.mid,
                           g_rx_channel_tuning_codes[i].tuning_code.fine);
                }
                return EXIT_SUCCESS;
            }
            case CHANNEL_CAL_STATE_INVALID:
            default: {
                return EXIT_FAILURE;
            }
        }
    }
}
