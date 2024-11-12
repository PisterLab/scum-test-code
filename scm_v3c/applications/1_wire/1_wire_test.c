#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "gpio.h"
#include "rftimer.h"
#include "1_wire.h"
#include "DS18B20.h"

//=========================== defines =========================================
// SCuM
#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

#define DS18B20 
//=========================== variables =======================================
// SCuM
typedef struct {
    uint8_t count;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================
// 1-Wire

void silence_callback(void);
//=========================== main ============================================

int main(void) {	
	// ===================init the mote============================================
	memset(&app_vars, 0, sizeof(app_vars_t)); 
	printf("Initializing...\r\n");
	initialize_mote();
	crc_check();
	perform_calibration();
	printf("Initialization Complete.\r\n");
	rftimer_set_callback_by_id(silence_callback, 1);
	
// Start -- 1-wire init.===========================================================
	/*
	GPIO 0, 1 are configured for bus using "OW_gpio_config();"
	Declare List root.
	Assign OWSearch_bus()'s output to the root. Returns NULL on fail.
	Declare array with List root member "item_count" elements.
	Pass both the root and the array to OWGet_rom_array tranfer() moves the list into an array.
	*/
	OW_gpio_config(RX_PIN, TX_PIN, STRONG_PULL_UP_PIN); // config for GPIO_0 = TX, GPIO_1 = RX, and  GPIO_2 = pull-up
	bus_roms_root_prt_t rom_list_root; //Declaring List root.
	if((rom_list_root = OWSearch_bus()) == NULL)// Try to get the ROMs of all devices
	{
		printf("No ROMs Found.\r\n");
	}
	else //OWSearch_bus() found >= 1 ROM
	{
		uint64_t ROMS[rom_list_root->item_count]; // Declare array with root member "item_count" elements.
		OWGet_rom_array(ROMS, rom_list_root); // Store ROMs in array, frees linked list
// END -- 1-wire init.=============================================================	
		for(int i = 0; i < sizeof(ROMS) / sizeof(uint64_t); i++)// Print all ROMs
		{
			printf("0x%llX\r\n", ROMS[i]);
		}

	/*
	After 1-wire init. success all roms discovered on bus are stored in ROMS[].
	Individual devices can be isolated using OWIsolate_device(ROMS[n]);
	*/
		#ifdef DS18B20
		for(int i = 0; i < sizeof(ROMS) / sizeof(uint64_t); i++)
		{
			if((ROMS[i] & 0x28) == 0x28)
			{
				init_DS18B20(ROMS[i], 100, 10, RESOLUTION); // resolution is defined in DS18B20.h
			}
		}
		int count = 1;
		while(1)
		{
			printf("Sample #%d\r\n", count);
			for(int i = 0; i < sizeof(ROMS) / sizeof(uint64_t); i++)
			{
				if((ROMS[i] & 0x28) == 0x28)
				{
					OWIsolate_device(ROMS[i]);
					printf("Sensor %d temp = %dC\r\n", i, get_temp(ROMS[i], USE_STRONG_PULL));
				}
			}
			count++;
			delay_milliseconds_synchronous(1000	, 1);
		}
		#endif
	}
}

void silence_callback(void)
{
	//SHHHHH!!!!
}
