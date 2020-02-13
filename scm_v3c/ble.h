#ifndef __BLE_H
#define __BLE_H

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
#define NAME_GAP_CODE				  0x10

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

#define ADVA_LENGTH           6    // Advertiser address is 6 bytes long.
#define PDU_LENGTH            39   // 2 byte PDU header + 37 bytes PDU.
#define CRC_LENGTH            3    // CRC is 3 bytes long.

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void ble_init(void);
void ble_init_tx(void);
void ble_init_rx(void);
void ble_gen_packet(void);
void ble_gen_test_packet(void);
void ble_set_AdvA(uint8_t *AdvA);
void ble_set_channel(uint8_t channel);
void ble_transmit(void);

#endif
