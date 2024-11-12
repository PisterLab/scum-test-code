#include <stdlib.h>
#include <stdio.h>

#include "DS18B20.h"
#include "1_wire.h"
#include "rftimer.h"

// DS18B20 =======================================================================
// Isolates then configures a DS18B20's alarms and resolution registers.
// Write all changes to EEPROM
// config = 9 | 10 | 11 | 12, for resolution bits
void init_DS18B20(uint64_t device_rom, uint8_t T_h, uint8_t T_l, uint8_t resolution)
{
	int config; // resolution sets config
	switch(RESOLUTION)
	{
		case 9:
			config = 0x00;
			break;
		case 10:
			config = 0x20;
			break;
		case 11:
			config = 0x40;
			break;
		case 12:
			config = 0x60;
			break;
		default:
			config = 0x00;
			break;
	}

	uint8_t scratch_mem[9] = {0}; // Read the scratch
	int cnt = 0;
	do
	{
		// Write the config registers.
		OWIsolate_device(device_rom);
		OWWriteByte(WRITE_SCRATCH);
		OWWriteByte(T_h); // T_high alarm
		OWWriteByte(T_l); // T_low alarm
		OWWriteByte(config); // resolution config.
		
		// Copy scatch memory into device EEPROM 
		OWIsolate_device(device_rom);
		OWWriteByte(COPY_SCRATCH); // Copies to EEPROM
		STRONG_PULL_UP_ON();
		delay_milliseconds_synchronous(15, 1);
		STRONG_PULL_UP_OFF();

		// Read scatch memory for CRC check.
		OWIsolate_device(device_rom);
		OWWriteByte(READ_SCRATCH);
		OWRead_bytes(scratch_mem, sizeof(scratch_mem));
		cnt++;
		if(cnt > 10)
		{
			printf("\r\nCRC fail\r\n");
		}
	}while(((OWCRC_bytes(scratch_mem, sizeof(scratch_mem) + 1)) != 0) && (cnt <= 10));
}

// Returns temperature data from a given ROM
uint16_t get_temp(uint64_t device_rom, int use_strong_pull_up)
{
	int delay; // Resolution selects delay
	switch(RESOLUTION)
	{
		case 9:
			delay = T_CONV_9;
			break;
		case 10:
			delay = T_CONV_10;
			break;
		case 11:
			delay = T_CONV_11;
			break;
		case 12:
			delay = T_CONV_12;
			break;
		default:
			delay = T_CONV_9;
			break;
	}
	uint8_t scratch_bytes[9] = {0};
	uint16_t temp_val = 0x00;
	int cnt = 0;
	do{
		OWIsolate_device(device_rom);
		if(use_strong_pull_up > 0)
		{
			
			OWWriteByte(CONVERT_T);// take a temperature reading
			STRONG_PULL_UP_ON();
			delay_milliseconds_synchronous(delay + 10, 1); // Let the sensor think
			STRONG_PULL_UP_OFF();
			NOP_500();
		}
		else
		{
			OWWriteByte(CONVERT_T);// take a temperature reading
			delay_milliseconds_synchronous(delay, 1); // Let the sensor think
		}
		

		OWIsolate_device(device_rom);
		OWWriteByte(READ_SCRATCH);
		OWRead_bytes(scratch_bytes, sizeof(scratch_bytes));
		temp_val |= scratch_bytes[1]; // MSB
		temp_val = (temp_val << 8);
		temp_val |= scratch_bytes[0]; // LSB
		temp_val = (temp_val >> 4); // Truncate the decimal for now.
		cnt++;
		if(cnt > 10)
		{
			printf("\r\nCRC fail\r\n");
		}
		//temp_val = (int)((temp_val * (9/5)) + 32); // convert to Freedom units ;)
	}while(((OWCRC_bytes(scratch_bytes, sizeof(scratch_bytes) + 1)) != 0) && (cnt <= 10));
	return temp_val;
}

