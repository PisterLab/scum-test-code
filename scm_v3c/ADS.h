#include <stdint.h>

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
/*
typedef union int16_buff_t{
	int16_t value;
	uint8_t bytes[2];
} int16_buff_t;

typedef union int24_buff_t{
	int32_t value;
	uint8_t bytes[3];
} int24_buff_t;

typedef struct imu_data_t{
	int16_buff_t acc_x;
	int16_buff_t acc_y;
	int16_buff_t acc_z;
	int16_buff_t gyro_x;
	int16_buff_t gyro_y;
	int16_buff_t gyro_z;
} imu_data_t;

typedef struct ads_data_t{
	int24_buff_t electrode1;
} ads_data_t;
*/
void digitalWrite(int pin, int high_low);

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
//void clock_toggle(unsigned char cycle);
//void spi_write(unsigned char writeByte);
//unsigned char spi_read();

//void spi_chip_select();
//void spi_chip_deselect();

/*
unsigned int read_gyro_x();
unsigned int read_gyro_y();
unsigned int read_gyro_z();

void spi_write(unsigned char writeByte);

unsigned char spi_read();

void spi_chip_select();

void spi_chip_deselect();

void start_select();

void reset_select();

void initialize_imu(void);

unsigned int read_acc_y();

unsigned int read_acc_z();

void test_imu_life();

unsigned char read_imu_register(unsigned char reg);

void read_ads_register(ads_data_t* ads_measurement);

void write_imu_register(unsigned char reg, unsigned char data);

void write_ads_register(unsigned char reg, unsigned char data);

void read_all_imu_data(imu_data_t* imu_measurement);

void log_imu_data(imu_data_t* imu_measurement);

void log_ads_data(ads_data_t* ads_measurement);
*/
