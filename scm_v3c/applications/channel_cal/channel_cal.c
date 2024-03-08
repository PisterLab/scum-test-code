#include "channel_cal.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "ieee_802_15_4.h"
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

// Channel calibration TX packet.
typedef struct __attribute__((packed)) {
    // Sequence number.
    uint8_t sequence_number;

    // Channel.
    uint8_t channel;

    // Reserved.
    uint8_t reserved1;

    // Reserved.
    uint8_t reserved2;

    // Command for the OpenMote.
    channel_cal_command_e command : 8;

    // Reserved.
    uint8_t reserved3;

    // Tuning code.
    tuning_code_t tuning_code;

    // Reserved.
    uint8_t reserved4;

    // CRC.
    uint16_t crc;
} channel_cal_tx_packet_t;

// Channel calibration RX packet.
typedef struct __attribute__((packed)) {
    // Sequence number.
    uint8_t sequence_number;

    // Channel.
    uint8_t channel;

    // TX tuning codes.
    tuning_code_t tx_tuning_codes[MAX_NUM_TX_TUNING_CODES_PER_CHANNEL];

    // Reserved.
    uint8_t reserved1;

    // Reserved.
    uint8_t reserved2;

    // CRC.
    uint16_t crc;
} channel_cal_rx_packet_t;

// Channel calibration RX tuning code.
typedef struct __attribute__((packed)) {
    // Channel.
    uint8_t channel;

    tuning_code_t tuning_code;
} channel_cal_rx_tuning_code_t;

// TX tuning code.
static tuning_code_t g_channel_cal_tx_tuning_code;

// RX tuning code.
static tuning_code_t g_channel_cal_rx_tuning_code;

// TX sweep configuration.
static tuning_sweep_config_t g_channel_cal_tx_sweep_config;

// RX sweep configuration.
static tuning_sweep_config_t g_channel_cal_rx_sweep_config;

// TX packet buffer.
static channel_cal_tx_packet_t g_channel_cal_tx_packet;

// Number of packets received.
static uint32_t g_channel_cal_num_rx_packets = 0;

// TX channel tuning codes.
static tuning_code_t
    g_channel_cal_tx_channel_tuning_codes[IEEE_802_15_4_NUM_CHANNELS]
                                         [MAX_NUM_TX_TUNING_CODES_PER_CHANNEL];

// RX channel tuning codes.
static channel_cal_rx_tuning_code_t
    g_channel_cal_rx_channel_tuning_codes[MAX_NUM_RX_TUNING_CODES];

// Channel calibration state.
static channel_cal_state_e g_channel_cal_state = CHANNEL_CAL_STATE_INVALID;

// Add some delay for the radio.
// TODO(fil): Change this function to use an RF timer compare register.
static inline void channel_cal_radio_delay(void) {
    for (uint8_t i = 0; i < 5; ++i) {}
}

// Radio RX callback function.
static void radio_rx_callback(const uint8_t* packet,
                              const uint8_t packet_length) {
    const channel_cal_rx_packet_t* rx_packet =
        (const channel_cal_rx_packet_t*)packet;
    const uint8_t channel = rx_packet->channel;
    if (!ieee_802_15_4_validate_channel(channel)) {
        printf("Received an unexpected packet from channel %u.\n", channel);
        return;
    }

    const uint32_t if_estimate = read_IF_estimate();
    radio_rfOff();

    // Print the received transmit tuning codes.
    printf("Channel %u RX: (%u, %u, %u)\n", channel,
           g_channel_cal_rx_tuning_code.coarse,
           g_channel_cal_rx_tuning_code.mid, g_channel_cal_rx_tuning_code.fine);
    printf("IF estimate: %u\n", if_estimate);
    printf("Channel %u TX:", channel);
    for (uint8_t i = 0; i < MAX_NUM_TX_TUNING_CODES_PER_CHANNEL; ++i) {
        printf(" (%u, %u, %u)", rx_packet->tx_tuning_codes[i].coarse,
               rx_packet->tx_tuning_codes[i].mid,
               rx_packet->tx_tuning_codes[i].fine);
    }
    printf("\n");

    // Copy the received transmit tuning codes.
    memcpy(&g_channel_cal_tx_channel_tuning_codes[channel -
                                                  IEEE_802_15_4_MIN_CHANNEL],
           rx_packet->tx_tuning_codes,
           MAX_NUM_TX_TUNING_CODES_PER_CHANNEL * sizeof(tuning_code_t));

    if (!channel_cal_get_tx_tuning_code(channel,
                                        &g_channel_cal_tx_tuning_code)) {
        printf("No TX tuning code found for channel %u.\n", channel);
    }
    memset(&g_channel_cal_tx_packet, 0, sizeof(channel_cal_tx_packet_t));

    if (if_estimate >= MIN_NUM_IF_COUNTS && if_estimate <= MAX_NUM_IF_COUNTS) {
        g_channel_cal_rx_channel_tuning_codes[g_channel_cal_num_rx_packets]
            .channel = channel;
        g_channel_cal_rx_channel_tuning_codes[g_channel_cal_num_rx_packets]
            .tuning_code = g_channel_cal_rx_tuning_code;
        ++g_channel_cal_num_rx_packets;

        if (g_channel_cal_rx_tuning_code.fine <= MIN_RX_TUNING_FINE_CODE) {
            g_channel_cal_tx_packet.channel = channel;
            g_channel_cal_tx_packet.command =
                CHANNEL_CAL_COMMAND_CHANGE_CHANNEL;
        }

        tuning_increment_mid_code_for_sweep(&g_channel_cal_rx_tuning_code,
                                            &g_channel_cal_rx_sweep_config);
    } else {
        tuning_increment_code_for_sweep(&g_channel_cal_rx_tuning_code,
                                        &g_channel_cal_rx_sweep_config);
    }

    channel_cal_radio_delay();
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 50 * 500, 7);
    g_channel_cal_state = CHANNEL_CAL_STATE_RX_ACK;
}

// RF timer callback function.
static void rf_timer_callback(void) {
    tuning_increment_code_for_sweep(&g_channel_cal_rx_tuning_code,
                                    &g_channel_cal_rx_sweep_config);
    rftimer_setCompareIn_by_id(rftimer_readCounter() + 20 * 500, 7);
}

bool channel_cal_init(const uint8_t start_coarse_code,
                      const uint8_t end_coarse_code) {
    if (start_coarse_code > end_coarse_code) {
        printf(
            "Start coarse code %u should be smaller than or equal to the end "
            "coarse code %u.\n",
            start_coarse_code, end_coarse_code);
        return false;
    }
    if (start_coarse_code < TUNING_MIN_CODE) {
        printf("Start coarse code %u is out of range [%u, %u].\n",
               start_coarse_code, TUNING_MIN_CODE, TUNING_MAX_CODE);
        return false;
    }
    if (end_coarse_code > TUNING_MAX_CODE) {
        printf("End coarse code %u is out of range [%u, %u].\n",
               end_coarse_code, TUNING_MIN_CODE, TUNING_MAX_CODE);
        return false;
    }

    // Set the TX sweep configuration.
    g_channel_cal_tx_sweep_config = (tuning_sweep_config_t){
        .coarse =
            {
                .start = start_coarse_code,
                .end = end_coarse_code,
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

    // Set the RX sweep configuration.
    g_channel_cal_rx_sweep_config = (tuning_sweep_config_t){
        .coarse =
            {
                .start = start_coarse_code,
                .end = end_coarse_code,
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

    // Validate the sweep configurations.
    if (!tuning_validate_sweep_config(&g_channel_cal_tx_sweep_config)) {
        printf("Invalid TX sweep configuration.\n");
        return false;
    }
    if (!tuning_validate_sweep_config(&g_channel_cal_rx_sweep_config)) {
        printf("Invalid RX sweep configuration.\n");
        return false;
    }

    tuning_init_for_sweep(&g_channel_cal_tx_tuning_code,
                          &g_channel_cal_tx_sweep_config);
    tuning_init_for_sweep(&g_channel_cal_rx_tuning_code,
                          &g_channel_cal_rx_sweep_config);
    radio_setRxCb(radio_rx_callback);
    rftimer_set_callback_by_id(rf_timer_callback, 7);
    return true;
}

bool channel_cal_run(void) {
    rftimer_enable_interrupts();
    g_channel_cal_state = CHANNEL_CAL_STATE_TX;
    printf("Starting TX channel calibration.\n");

    while (true) {
        channel_cal_radio_delay();
        switch (g_channel_cal_state) {
            case CHANNEL_CAL_STATE_TX: {
                memset(&g_channel_cal_tx_packet, 0,
                       sizeof(channel_cal_tx_packet_t));
                g_channel_cal_tx_packet.tuning_code =
                    g_channel_cal_tx_tuning_code;

                tuning_tune_radio(&g_channel_cal_tx_tuning_code);
                send_packet(&g_channel_cal_tx_packet,
                            sizeof(channel_cal_tx_packet_t));
                tuning_increment_code_for_sweep(&g_channel_cal_tx_tuning_code,
                                                &g_channel_cal_tx_sweep_config);

                if (tuning_end_of_sweep(&g_channel_cal_tx_tuning_code,
                                        &g_channel_cal_tx_sweep_config)) {
                    rftimer_enable_interrupts_by_id(7);
                    rftimer_setCompareIn_by_id(
                        rftimer_readCounter() +
                            IEEE_802_15_4_NUM_CHANNELS * 1000 * 500,
                        7);
                    g_channel_cal_state = CHANNEL_CAL_STATE_RX;
                    printf("Starting RX channel calibration.\n");
                }
                break;
            }
            case CHANNEL_CAL_STATE_RX: {
                tuning_tune_radio(&g_channel_cal_rx_tuning_code);
                channel_cal_radio_delay();
                receive_packet(/*timeout=*/true);

                if (tuning_end_of_sweep(&g_channel_cal_rx_tuning_code,
                                        &g_channel_cal_rx_sweep_config)) {
                    rftimer_disable_interrupts_by_id(7);
                    rftimer_disable_interrupts();
                    g_channel_cal_state = CHANNEL_CAL_STATE_DONE;
                }
                break;
            }
            case CHANNEL_CAL_STATE_RX_ACK: {
                if (g_channel_cal_tx_packet.command ==
                    CHANNEL_CAL_COMMAND_CHANGE_CHANNEL) {
                    for (uint8_t i = 0; i < 3; ++i) {
                        channel_cal_radio_delay();
                        tuning_tune_radio(&g_channel_cal_tx_tuning_code);
                        send_packet(&g_channel_cal_tx_packet,
                                    sizeof(channel_cal_tx_packet_t));
                        channel_cal_radio_delay();
                        channel_cal_radio_delay();
                        channel_cal_radio_delay();
                    }
                } else {
                    for (uint8_t i = 0; i < 2; ++i) {
                        channel_cal_radio_delay();
                        tuning_tune_radio(&g_channel_cal_tx_tuning_code);
                        send_packet(&g_channel_cal_tx_packet,
                                    sizeof(channel_cal_tx_packet_t));
                    }
                }

                g_channel_cal_state = CHANNEL_CAL_STATE_RX;
                break;
            }
            case CHANNEL_CAL_STATE_DONE: {
                // Print the channel calibration results.
                printf("TX packet results:\n");
                for (uint8_t i = 0; i < IEEE_802_15_4_NUM_CHANNELS; ++i) {
                    for (uint8_t j = 0; j < MAX_NUM_TX_TUNING_CODES_PER_CHANNEL;
                         ++j) {
                        printf(
                            "Channel %u: (%u, %u, %u)\n",
                            i + IEEE_802_15_4_MIN_CHANNEL,
                            g_channel_cal_tx_channel_tuning_codes[i][j].coarse,
                            g_channel_cal_tx_channel_tuning_codes[i][j].mid,
                            g_channel_cal_tx_channel_tuning_codes[i][j].fine);
                    }
                }
                printf("RX packet results:\n");
                for (uint8_t i = 0; i < MAX_NUM_RX_TUNING_CODES; ++i) {
                    printf("Channel %u: (%u, %u, %u)\n",
                           g_channel_cal_rx_channel_tuning_codes[i].channel,
                           g_channel_cal_rx_channel_tuning_codes[i]
                               .tuning_code.coarse,
                           g_channel_cal_rx_channel_tuning_codes[i]
                               .tuning_code.mid,
                           g_channel_cal_rx_channel_tuning_codes[i]
                               .tuning_code.fine);
                }
                return true;
            }
            case CHANNEL_CAL_STATE_INVALID:
            default: {
                return false;
            }
        }
    }
}

bool channel_cal_get_tx_tuning_code(const uint8_t channel,
                                    tuning_code_t* tuning_code) {
    // Validate the channel.
    if (!ieee_802_15_4_validate_channel(channel)) {
        printf("Invalid 802.15.4 channel: %u.\n", channel);
        return false;
    }

    // Use the second set of tuning codes if there are multiple possible tuning
    // codes.
    if (g_channel_cal_tx_channel_tuning_codes[channel -
                                              IEEE_802_15_4_MIN_CHANNEL][1]
                .coarse != 0 &&
        g_channel_cal_tx_channel_tuning_codes[channel -
                                              IEEE_802_15_4_MIN_CHANNEL][1]
                .mid != 0 &&
        g_channel_cal_tx_channel_tuning_codes[channel -
                                              IEEE_802_15_4_MIN_CHANNEL][1]
                .fine != 0) {
        *tuning_code =
            g_channel_cal_tx_channel_tuning_codes[channel -
                                                  IEEE_802_15_4_MIN_CHANNEL][1];
    } else {
        *tuning_code =
            g_channel_cal_tx_channel_tuning_codes[channel -
                                                  IEEE_802_15_4_MIN_CHANNEL][0];
    }
    return tuning_code->coarse != 0 && tuning_code->mid != 0 &&
           tuning_code->fine != 0;
}

bool channel_cal_get_rx_tuning_code(const uint8_t channel,
                                    tuning_code_t* tuning_code) {
    // Validate the channel.
    if (!ieee_802_15_4_validate_channel(channel)) {
        printf("Invalid 802.15.4 channel: %u.\n", channel);
        return false;
    }

    // Use the second set of tuning codes if there are multiple possible tuning
    // codes.
    for (uint8_t i = 0; i < MAX_NUM_RX_TUNING_CODES; ++i) {
        if (g_channel_cal_rx_channel_tuning_codes[i].channel == channel) {
            if (i != MAX_NUM_RX_TUNING_CODES - 1 &&
                g_channel_cal_rx_channel_tuning_codes[i + 1].channel ==
                    channel) {
                *tuning_code =
                    g_channel_cal_rx_channel_tuning_codes[i + 1].tuning_code;
                return true;
            }
            *tuning_code = g_channel_cal_rx_channel_tuning_codes[i].tuning_code;
            return true;
        }
    }
    return false;
}
