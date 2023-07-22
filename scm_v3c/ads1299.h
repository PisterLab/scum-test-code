#include <stdint.h>
#include "spi.h"

//SPI Command Definition Byte Assignments (Datasheet, p35)
#define ADS_CMD_WAKEUP 0x02 // Wake-up from standby mode
#define ADS_CMD_STANDBY 0x04 // Enter Standby mode
#define ADS_CMD_RESET 0x06 // Reset the device registers to default
#define ADS_CMD_START 0x08 // Start and restart (synchronize) conversions
#define ADS_CMD_STOP 0x0A // Stop conversion
#define ADS_CMD_RDATAC 0x10 // Enable Read Data Continuous mode (default mode at power-up)
#define ADS_CMD_SDATAC 0x11 // Stop Read Data Continuous mode
#define ADS_CMD_RDATA 0x12 // Read data by command; supports multiple read back


//Register Addresses
#define ADS_REG_ID 0x00
#define ADS_REG_CONFIG1 0x01
#define ADS_REG_CONFIG2 0x02
#define ADS_REG_CONFIG3 0x03
#define ADS_REG_LOFF 0x04
#define ADS_REG_CH1SET 0x05
#define ADS_REG_CH2SET 0x06
#define ADS_REG_CH3SET 0x07
#define ADS_REG_CH4SET 0x08
#define ADS_REG_CH5SET 0x09
#define ADS_REG_CH6SET 0x0A
#define ADS_REG_CH7SET 0x0B
#define ADS_REG_CH8SET 0x0C
#define ADS_REG_BIAS_SENSP 0x0D
#define ADS_REG_BIAS_SENSN 0x0E
#define ADS_REG_LOFF_SENSP 0x0F
#define ADS_REG_LOFF_SENSN 0x10
#define ADS_REG_LOFF_FLIP 0x11
#define ADS_REG_LOFF_STATP 0x12
#define ADS_REG_LOFF_STATN 0x13
#define ADS_REG_GPIO 0x14
#define ADS_REG_MISC1 0x15
#define ADS_REG_MISC2 0x16
#define ADS_REG_CONFIG4 0x17

typedef struct ads_data_t{
	uint32_t config;
	int32_t channel[8];
} ads_data_t;


void ads_init(void);

// System Commands
void ads_wakeup(void);
void ads_standby(void);
void ads_reset(void);
void ads_start(void);
void ads_stop(void);

/* Read data continuous mode*/
void ads_rdatac(void);

/* Stop read data continuous mode */
void ads_sdatac(void);
uint8_t ads_rreg(uint8_t addr);
void ads_wreg(uint8_t addr, uint8_t val);
void ads_rregs(uint8_t addr, uint8_t NregminusOne);
void ads_poll_measurements(ads_data_t* ads_measurement);