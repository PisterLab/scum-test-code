#include <stdint.h>
#include "spi.h"

//SPI Command Definition Byte Assignments (Datasheet, p35)
#define _WAKEUP 0x02 // Wake-up from standby mode
#define _STANDBY 0x04 // Enter Standby mode
#define _RESET 0x06 // Reset the device registers to default
#define _START 0x08 // Start and restart (synchronize) conversions
#define _STOP 0x0A // Stop conversion
#define _RDATAC 0x10 // Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC 0x11 // Stop Read Data Continuous mode
#define _RDATA 0x12 // Read data by command; supports multiple read back


//Register Addresses
#define ID 0x00
#define CONFIG1 0x01
#define CONFIG2 0x02
#define CONFIG3 0x03
#define LOFF 0x04
#define CH1SET 0x05
#define CH2SET 0x06
#define CH3SET 0x07
#define CH4SET 0x08
#define CH5SET 0x09
#define CH6SET 0x0A
#define CH7SET 0x0B
#define CH8SET 0x0C
#define BIAS_SENSP 0x0D
#define BIAS_SENSN 0x0E
#define LOFF_SENSP 0x0F
#define LOFF_SENSN 0x10
#define LOFF_FLIP 0x11
#define LOFF_STATP 0x12
#define LOFF_STATN 0x13
#define GPIO 0x14
#define MISC1 0x15
#define MISC2 0x16
#define CONFIG4 0x17

typedef struct ads_data_t{
	uint32_t config;
	int32_t channel[8];
} ads_data_t;


void ADS_initialize();

// System Commands
void ADS_WAKEUP();
void ADS_STANDBY();
void ADS_RESET();
void ADS_START();
void ADS_STOP();
void ADS_RDATAC();
void ADS_SDATAC();
unsigned char ADS_RREG(unsigned char addr);
void ADS_WREG(unsigned char addr, unsigned char val);
void ADS_RREGS(unsigned char addr, unsigned char NregminusOne);
void read_ads_register(ads_data_t* ads_measurement);