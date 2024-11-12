#ifndef __1_WIRE_H
#define __1_WIRE_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

//=========================== defines =========================================
// ***  Congfig. defines
// Define the HCLK source.
#define HF_CLOCK // HF_CLOCK or LF_CLOCK
// Enable strong pull-up 
#define USE_STRONG_PULL 1 // 1 = true, 0 = false
// GPIO pins are specified here.
#define TX_PIN 0
#define RX_PIN 1
#define STRONG_PULL_UP_PIN 7
// End Congfig. defines ***

// ROM commands.
#define READROM 0x33
#define SKIPROM 0xCC
#define MATCHROM 0x55
#define SEARCHROM 0xF0
//===================================
// 1-wire search rom, code taken from;
// https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html
#define FALSE 0
#define TRUE 1
//===================================

// Function Macros
#define BUS_LOW() GPIO_REG__OUTPUT |= (1 << TX_PIN); // Pulls bus LOW
#define BUS_RELEASE() GPIO_REG__OUTPUT &= ~(1 << TX_PIN); // Release bus to Pull-up
#define BUS_READ() (GPIO_REG__INPUT &= (1 << RX_PIN)) ? 1 : 0; // Read the logic level of the bus.
#define STRONG_PULL_UP_OFF() GPIO_REG__OUTPUT |= (1 << STRONG_PULL_UP_PIN);
#define STRONG_PULL_UP_ON() GPIO_REG__OUTPUT &= ~(1 << STRONG_PULL_UP_PIN);
// NOP Macros for us timing
#define NOP_5() __asm("NOP\n\t" "NOP\n\t" "NOP\n\t" "NOP\n\t" "NOP\n\t"); //~1us
#define NOP_10() {NOP_5(); NOP_5();} // 2us
#define NOP_50() {NOP_10(); NOP_10(); NOP_10(); NOP_10(); NOP_10();} // 10us
#define NOP_100() {NOP_50(); NOP_50()} // 20 us
#define NOP_500() {NOP_100(); NOP_100(); NOP_100(); NOP_100(); NOP_100();} // 100us

#ifdef HF_CLOCK
//Delay Macros for 1-wire timing HF_clock
#define DELAY_A {NOP_10(); NOP_10(); NOP_10();}//6us
#define DELAY_B {NOP_100(); NOP_100(); NOP_100(); NOP_10(); NOP_10();}//64
#define DELAY_C {NOP_100(); NOP_100(); NOP_100();}//60
#define DELAY_D {NOP_50();}//10
#define DELAY_E {NOP_10(); NOP_10(); NOP_10(); NOP_10(); NOP_5();}//9
#define DELAY_F {NOP_100(); NOP_100(); NOP_50();}//55
#define DELAY_G {}//0 
#define DELAY_H {NOP_500(); NOP_500(); NOP_500(); NOP_500(); NOP_500(); NOP_100(); NOP_100(); NOP_100(); NOP_100();}//480
#define DELAY_I {NOP_100(); NOP_100(); NOP_100(); NOP_50();}//70
#define DELAY_J {NOP_500(); NOP_500(); NOP_500(); NOP_500(); NOP_50();}//410
#endif
#ifdef LF_CLOCK
// Delay Macros for 1-wire timing LF_clock LF = HF * 2/3
#define DELAY_A { NOP_10(); NOP_10();}//6us * 0.7 = 4.2 = 4
#define DELAY_B {NOP_100(); NOP_100(); NOP_10(); NOP_10(); NOP_5();}//64 * 0.7 = 44.8 = 45
#define DELAY_C {NOP_100(); NOP_100(); NOP_10();}//60 * 0.7 = 42
#define DELAY_D {NOP_10(); NOP_10(); NOP_10(); NOP_5();}//10 * 0.7 = 7
#define DELAY_E {NOP_10(); NOP_10(); NOP_10();}//9 * 0.7 = 6.3 = 6
#define DELAY_F {NOP_100(); NOP_50(); NOP_10(); NOP_10(); NOP_10(); NOP_10(); NOP_5();}//55 * 0.7 = 38.5 = 39
#define DELAY_G {}//0 
#define DELAY_H {NOP_500(); NOP_500(); NOP_500(); NOP_100(); NOP_50(); NOP_10(); NOP_10(); NOP_10();}//480 * 0.7 = 336
#define DELAY_I {NOP_100(); NOP_100(); NOP_10(); NOP_10(); NOP_10(); NOP_10(); NOP_5();}//70 * 0.7 = 49
#define DELAY_J {NOP_500(); NOP_500(); NOP_100(); NOP_100(); NOP_100(); NOP_100(); NOP_10(); NOP_10(); NOP_10(); NOP_5();}//410 * 0.7 = 287
#endif
//=========================== variables =======================================
// ROMs found on the bus are stored in a linked list structure. Where the root contains the 
// pointed to the first node(next) and the total number of nodes(item_count). Each node stores
// an individual ROM(rom) and the pointer to the next(next).
typedef struct rom_list_root{
	uint32_t item_count;
	struct rom_list* next;
} rom_list_root_t,	*bus_roms_root_prt_t;

typedef struct rom_list{
	uint64_t rom;
	struct rom_list* next;
} rom_list_t,	*bus_roms_list_prt_t;
	
//=========================== prototypes ======================================
// User function
// Configures SCuM I/O for bus, set up banks, leaves all pins except RX at outputs.
void OW_gpio_config(int rx, int tx, int pull_up);
// Read all roms on bus, returns base to linked list.
bus_roms_root_prt_t OWSearch_bus(void);
// Create an array of ROMs from linked list.
void OWGet_rom_array(uint64_t* array, bus_roms_root_prt_t root);
 // Write a rom to bus.
void OWWrite_rom(uint64_t device_rom);
// Isolate a device by reseting, geting presence and writing the ROM.
int OWIsolate_device(uint64_t device_rom); 
// Reads a byte from bus.
uint8_t OWRead_byte(void);
// Reads n bytes from bus.
void OWRead_bytes(uint8_t* buffer, int size);
// Returns the CRC8 of a series of bytes.
int OWCRC_bytes(uint8_t* byte_array, int size); 
	
// Driver
void OWStore_rom(unsigned char* rom_bytes, bus_roms_list_prt_t base);
void OWWriteBit(uint8_t bit);
uint8_t OWReadBit(void);
int OWReset(void);
void OWWriteByte(uint8_t byte);
//===================================
// 1-wire search rom, code taken from;
// https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html
int OWFirst(void);
int OWNext(void);
int OWSearch(void);
unsigned char docrc8(unsigned char value);
#endif
