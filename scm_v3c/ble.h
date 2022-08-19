#ifndef __BLE_H
#define __BLE_H

#include <stdbool.h>
#include <stdint.h>

//=========================== define ==========================================

// BLE packet assembly globals.
#define BPREAMBLE             0x55
// Split BACCESS_ADDRESS into bytes to avoid big-/little-endianness issue.
#define BACCESS_ADDRESS1      0x6B
#define BACCESS_ADDRESS2      0x7D
#define BACCESS_ADDRESS3      0x91
#define BACCESS_ADDRESS4      0x71

#define PDU_HEADER1           0x40
#define PDU_HEADER2           0xA4 // PDU is 37 bytes long (6 bytes advertiser address + 31 bytes data).

// Short name.
#define NAME_LENGTH           5
#define NAME_HEADER           0x60 // Name is 6 bytes long (1 byte GAP code + 5 bytes data).
#define NAME_GAP_CODE         0x10

// LC frequency codes (coarse+mid+fine).
#define LC_FREQCODES_LENGTH   2
#define LC_FREQCODES_HEADER   0xC0 // LC freq codes are 3 bytes long (1 byte GAP code + 2 bytes data/15 bits).
#define LC_FREQCODES_GAP_CODE 0x03 // Custom GAP code for LC freq codes (0xC0 LSB first).

// Counters (2M and 32kHz).
#define COUNTERS_LENGTH       8
#define COUNTERS_HEADER       0x90 // Counters are 9 bytes long (1 byte GAP code + 4 bytes 2M counter + 4 bytes 32k counter).
#define COUNTERS_GAP_CODE     0x43 // Custom GAP code for counters (0xC2 LSB first).

// Temperature.
#define TEMP_LENGTH           2
#define TEMP_HEADER           0xC0 // Temperature is 3 bytes long (1 byte GAP code + 2 bytes data).
#define TEMP_GAP_CODE         0x83 // Custom GAP code for temperature (0xC1 LSB first).

// Custom data.
#define CUSTOM_DATA_LENGTH    4
#define CUSTOM_DATA_HEADER    0xA0 // Custom data is 5 bytes long (1 byte GAP code + 4 bytes data).
#define CUSTOM_DATA_GAP_CODE  0xC3 // Custom GAP code for custom data (0xC3 LSB first).

#define ADVA_LENGTH           6    // Advertiser address is 6 bytes long.
#define PDU_LENGTH            0x23 // 2 byte PDU header + 37 bytes PDU.
#define CRC_LENGTH            3    // CRC is 3 bytes long.

#define MAX_DATA_LENGTH       31   // Maximum data length is 31 bytes.

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void ble_init(void);
void ble_init_tx(void);
void ble_init_rx(void);
void ble_transmit(void);
void ble_append_crc(uint8_t* pdu_crc, uint16_t pdu_lenght);
void ble_whitening(uint8_t* pkt, uint16_t lenght);
void ble_prepare_packt(uint8_t* pdu, uint8_t pdu_length);

void ble_load_tx_arb_fifo(void);
void ble_txNow_tx_arb_fifo(void);

void ble_gen_packet(void);
void ble_gen_test_packet(void);
void ble_set_AdvA(uint8_t *AdvA);
void ble_set_channel(uint8_t channel);
void ble_set_name_tx_en(bool name_tx_en);
void ble_set_name(char *name);
void ble_set_lc_freq_codes_tx_en(bool lc_freq_codes_tx_en);
void ble_set_lc_freq_codes(uint16_t lc_freq_codes);
void ble_set_counters_tx_en(bool counters_tx_en);
void ble_set_count_2M(uint32_t count_2M);
void ble_set_count_32k(uint32_t count_32k);
void ble_set_temp_tx_en(bool temp_tx_en);
void ble_set_temp(double temp);
void ble_set_data_tx_en(bool data_tx_en);
void ble_set_data(uint8_t *data);

#endif
