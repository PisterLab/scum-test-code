#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "gpio.h"
#include "rftimer.h"
#include "scm3c_hw_interface.h"
#include "memory_map.h"
#include "1_wire.h"


// 1wire-search-algorithm, code below taken from;
// https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html
unsigned char ROM_NO[8]; // Rom bytes are put here on search
int LastDiscrepancy;
int LastFamilyDiscrepancy;
int LastDeviceFlag;
unsigned char crc8;

// Routine to configure gpio after mote_init.
// Taken from the mote_init routine just changed the GPI/O_enable values
// GPO are set default, just turn on the RX GPI and turn off the GPO for the RX pin.
void OW_gpio_config(int rx, int tx, int pull_up)
{
	// Set GP0(tx, pull_up) pin set to bank6
	int bank_n = 6;
	if(tx < 4 || pull_up < 4)
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(245 + j);
			} else {
					clear_asc_bit(245 + j);
			}
		}
	}
	else if((tx >= 4 && tx < 8) || (pull_up >= 4 && pull_up < 8))
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(249 + j);
			} else {
					clear_asc_bit(249 + j);
			}
		}
	}		
	else if((tx >= 8 && tx < 12) || (pull_up >= 8 && pull_up < 12))
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(253 + j);
			} else {
					clear_asc_bit(253 + j);
			}
		}
	}
	else if((tx >= 12 && tx < 16) || (pull_up >= 12 && pull_up < 16))
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(257 + j);
			} else {
					clear_asc_bit(257 + j);
			}
		}
	}
	else if(tx >= 16 || pull_up >= 16) // out of bounds defaults to bank0
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(245 + j);
			} else {
					clear_asc_bit(245 + j);
			}
		}		
	}
	
	// Set GPI(rx) pin set to bank0
	bank_n = 0;
	if(rx < 4)
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(261 + j);
			} else {
					clear_asc_bit(261 + j);
			}
		}
	}
	else if(rx >= 4 && rx < 8)
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(263 + j);
			} else {
					clear_asc_bit(263 + j);
			}
		}
	}		
	else if(rx >= 8 && rx < 12)
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(265 + j);
			} else {
					clear_asc_bit(265 + j);
			}
		}
	}
	else if(rx >= 12 && rx < 16)
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(267 + j);
			} else {
					clear_asc_bit(267 + j);
			}
		}
	}
	else if(rx >= 16) // out of bounds defaults to bank0
	{
		for (int j = 0; j <= 3; j++) 
		{
			if ((bank_n >> j) & 0x1) {
					set_asc_bit(261 + j);
			} else {
					clear_asc_bit(261 + j);
			}
		}		
	}	
	
	uint16_t gpi_mask = (0x01 << rx);
	uint16_t gpo_mask = (0x01 << rx);

	// Set GPI enables
	GPI_enables(gpi_mask |= 0x0000); // enable GPIO 1 as RX for 1-wire
	printf("gpi mask = 0x%X\r\n", gpi_mask);
	// Set GPO enables
	GPO_enables(~(gpo_mask &= 0xFFFF)); // GPIO 0 is 1-wire TX, turn off output on RX
	printf("gpo mask = 0x%X\r\n", gpi_mask);
	#ifdef HF_CLOCK
	// Set HCLK source as HF_CLOCK
	set_asc_bit(1147);//****
	// Set RFTimer source as HF_CLOCK
	set_asc_bit(1151);//****
	// Disable LF_CLOCK
	set_asc_bit(553);//*****
	// HF_clock div		
    set_asc_bit(49);
    set_asc_bit(48);
    clear_asc_bit(47);
    set_asc_bit(46);
    clear_asc_bit(45);
    set_asc_bit(44);
    set_asc_bit(43);
    set_asc_bit(42);
	#endif
	#ifdef LF_CLOCK
	// Let HCLK source be LF_CLOCK
	clear_asc_bit(1147);//****
	// Let RFTimer source be LF_CLOCK
	clear_asc_bit(1151);//****
	// Enable LF_CLOCK
	clear_asc_bit(553);//*****
	// LF_clock div	
    set_asc_bit(49);
    set_asc_bit(48);
    set_asc_bit(47);
    clear_asc_bit(46);
    clear_asc_bit(45);
    set_asc_bit(44);
    clear_asc_bit(43);
    clear_asc_bit(42);
	#endif
	
	// scan chain
	analog_scan_chain_write();
	analog_scan_chain_load();
	// Initialize all pins to be low.
	GPIO_REG__OUTPUT &= ~0xFFFF;
	STRONG_PULL_UP_OFF();
}

// Returns all roms in linked list
bus_roms_root_prt_t OWSearch_bus(void)
{
	bus_roms_root_prt_t my_roms_root = malloc(sizeof(rom_list_root_t));
	my_roms_root->next = malloc(sizeof(rom_list_t));
	int cnt = 0;
	int rslt = OWFirst();
	while(rslt)
	{
		cnt++;
		OWStore_rom(ROM_NO, my_roms_root->next);
		rslt = OWNext();
	}
	my_roms_root->item_count = cnt;
	if(cnt == 0)
	{
		free(my_roms_root);
		my_roms_root = NULL;
	}
	return my_roms_root;
}

// Write a given rom to the bus.
void OWWrite_rom(uint64_t device_rom)
{	
	int count = 0;
	while(count < 8)
	{
		uint8_t byte = 0x0;
		byte |= device_rom;
		OWWriteByte(byte);
		device_rom = (device_rom >> 8);
		count++;
	}
}

// Stores the IDs returned from OWsearch into a linked list element.
void OWStore_rom(unsigned char* rom_bytes, bus_roms_list_prt_t base)
{
	uint64_t new_rom = 0;
	for(int i = 7; i >= 0; i--)
	{
		new_rom |= rom_bytes[i];
		if(i > 0)
		{
			new_rom = (new_rom << 8);
		}
	}
	if(base->next == NULL)
	{
		base->rom = new_rom;
		base->next = malloc(sizeof(rom_list_t));
	}
	else
	{
		while(base->next != NULL)
		{
			base = base->next;
		}
		base->rom = new_rom;
		base->next = malloc(sizeof(rom_list_t));
	}
}

// Moves roms from linked list to array and frees the list. 
void OWGet_rom_array(uint64_t* array, bus_roms_root_prt_t root)
{
	int cnt = root->item_count;
	bus_roms_list_prt_t rom_n = root->next;
	free(root);
	for(int i = 0; i < cnt; i++)
	{
		bus_roms_list_prt_t prev_rom = rom_n;
		array[i] = rom_n->rom;
		rom_n = rom_n->next;	
		free(prev_rom);
	}
}
// Reads single byte, use in OWRead_bytes
inline uint8_t OWRead_byte(void)
{
	int count = 8;
	uint8_t data = 0x00;
		while(count > 0)
	{
		data = (data >> 1);
		int new_bit = OWReadBit();
		if(new_bit & 0x01)// if new_bit is a 1
		{
			new_bit = (new_bit << (7)); // set MSb 
			data |= new_bit;
		}
		count--;
	}
	return data;
}

// Read sizeof(buffer) bytes into buffer array.
void OWRead_bytes(uint8_t* buffer, int size)
{
	int n = size;
	int count = 0;
	while(count < n)
	{
		uint8_t new_byte = OWRead_byte();
		buffer[count] = new_byte;
		count++;
	}
}


// CRC bytes, read elements [0 to size] through crc generator. 
// crc8 value should match the crc produced my device if only the data is 
// used to generate the crc value.
// Alternitivly, pass the data and the crc from the device, the function should 
// return 0.
int OWCRC_bytes(uint8_t* byte_array, int size)
{
	crc8 = 0;
	for(int i = 0; i < size - 1; i++)
	{
		docrc8(byte_array[i]);
	}
	return crc8;
}

/*
isolating a device by; get presence responce, send MATCHROM command,
send ROM
*/
int OWIsolate_device(uint64_t device_rom)
{
	int ret = 0;
	ret = OWReset();
	if(ret != 0)
	{
		OWWriteByte(MATCHROM);
		OWWrite_rom(device_rom);
	}
	return ret;
}

// Platform specific function used in 1wire-search-algorithm=================
// Write a bit to the bus
inline void OWWriteBit(uint8_t bit)
{
	switch(bit)
	{
		case 0x00:
			BUS_LOW();
			DELAY_C
			BUS_RELEASE();
			DELAY_D
			break;
		default:
			BUS_LOW();
			DELAY_A
			BUS_RELEASE();
			DELAY_B
			break;
	}
}


// Reads a byte from the bus.
inline uint8_t OWReadBit(void)
{
	BUS_LOW();
	DELAY_A
	BUS_RELEASE();
	DELAY_E
	uint8_t bit = BUS_READ();
	DELAY_F
	return bit;
}

// Return is 1 for presence is detected.
inline int OWReset(void)
{
	BUS_LOW();
	DELAY_H
	BUS_RELEASE();
	DELAY_I
	int state = BUS_READ();
	DELAY_J
	return !state;
}

// Sends a byte 
inline void OWWriteByte(uint8_t byte)
{
	int count = 8;
	while(count > 0)
	{
		uint8_t temp_byte = byte & 0x01;
		if(temp_byte == 1)
		{
			OWWriteBit(0x01);
		}
		else
		{
			OWWriteBit(0x00);
		}
		byte = (byte >> 1);
		count--;
	}
}

//==========================================================================
// 1wire-search-algorithm, code below taken from;
// https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html


static unsigned char dscrc_table[] = {
 0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
 157,195, 33,127,252,162, 64, 30, 95, 1,227,189, 62, 96,130,220,
 35,125,159,193, 66, 28,254,160,225,191, 93, 3,128,222, 60, 98,
 190,224, 2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
 70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89, 7,
 219,133,103, 57,186,228, 6, 88, 25, 71,165,251,120, 38,196,154,
 101, 59,217,135, 4, 90,184,230,167,249, 27, 69,198,152,122, 36,
 248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91, 5,231,185,
 140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
 17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
 175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
 50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
 202,148,118, 40,171,245, 23, 73, 8, 86,180,234,105, 55,213,139,
 87, 9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
 233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
 116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53};
//--------------------------------------------------------------------------
// Calculate the CRC8 of the byte value provided with the current 
// global 'crc8' value. 
// Returns current global crc8 value
//
unsigned char docrc8(unsigned char value)
{
 // See Application Note 27
 
 // TEST BUILD
 crc8 = dscrc_table[crc8 ^ value];
 return crc8;
}

// Find the 'first' devices on the 1-Wire bus
// Return TRUE : device found, ROM number in ROM_NO buffer
// FALSE : no device present
//
int OWFirst(void)
{
 // reset the search state
 LastDiscrepancy = 0;
 LastDeviceFlag = FALSE;
 LastFamilyDiscrepancy = 0;
 return OWSearch();
}
//--------------------------------------------------------------------------
// Find the 'next' devices on the 1-Wire bus
// Return TRUE : device found, ROM number in ROM_NO buffer
// FALSE : device not found, end of search
//
int OWNext(void)
{
 // leave the search state alone
 return OWSearch();
}
//--------------------------------------------------------------------------
// Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
// search state.
// Return TRUE : device found, ROM number in ROM_NO buffer
// FALSE : device not found, end of search
//
int OWSearch(void)
{
	int id_bit_number;
	int last_zero, rom_byte_number, search_result;
	int id_bit, cmp_id_bit;
	unsigned char rom_byte_mask, search_direction;
	// initialize for search
	id_bit_number = 1;
	last_zero = 0;
	rom_byte_number = 0;
	rom_byte_mask = 1;
	search_result = 0;
	crc8 = 0;
	// if the last call was not the last one
	if(!LastDeviceFlag)
	{
		// 1-Wire reset
		if(!OWReset())
		{
			// reset the search
			LastDiscrepancy = 0;
			LastDeviceFlag = FALSE;
			LastFamilyDiscrepancy = 0;
			return FALSE;
		}
		// issue the search command 
		OWWriteByte(SEARCHROM); 
		// loop to do the search
		do
		{
			// read a bit and its complement
			id_bit = OWReadBit();
			cmp_id_bit = OWReadBit();
			// check for no devices on 1-wire
			if((id_bit == 1) && (cmp_id_bit == 1))
			{
				break;
			}
			else
			{
				// all devices coupled have 0 or 1
				if(id_bit != cmp_id_bit)
				{
					search_direction = id_bit; // bit write value for search
				}
				else
				{
					// if this discrepancy if before the Last Discrepancy
					// on a previous next then pick the same as last time
					if (id_bit_number < LastDiscrepancy)
					{
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);\
					}
					else
					{
						// if equal to last pick 1, if not then pick 0
						search_direction = (id_bit_number == LastDiscrepancy);
						// if 0 was picked then record its position in LastZero
					}

					if(search_direction == 0)
					{
						last_zero = id_bit_number;
						// check for Last discrepancy in family
						if(last_zero < 9)
						{
							LastFamilyDiscrepancy = last_zero;
						}
					}
				}
				// set or clear the bit in the ROM byte rom_byte_number
				// with mask rom_byte_mask
				if(search_direction == 1)
				{
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				}
				else
				{
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				}
				// serial number search direction write bit
				OWWriteBit(search_direction);
				// increment the byte counter id_bit_number
				// and shift the mask rom_byte_mask
				id_bit_number++;
				rom_byte_mask <<= 1;
				// if the mask is 0 then go to new SerialNum byte rom_byte_number and reset mask
				if(rom_byte_mask == 0)
				{
					docrc8(ROM_NO[rom_byte_number]); // accumulate the CRC
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		}
		while(rom_byte_number < 8); // loop until through all ROM bytes 0-7
		// if the search was successful then
		{
			if(!((id_bit_number < 65) || (crc8 != 0)))
			{
				// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
				LastDiscrepancy = last_zero;
				// check for last device
				if(LastDiscrepancy == 0)
				{
					LastDeviceFlag = TRUE;
				}
				search_result = TRUE;
			}
		}
	}

	// if no device found then reset counters so next 'search' will be like a first
	if(!search_result || !ROM_NO[0])
	{
		LastDiscrepancy = 0;
		LastDeviceFlag = FALSE;
		LastFamilyDiscrepancy = 0;
		search_result = FALSE;
	}
	return search_result;
}
