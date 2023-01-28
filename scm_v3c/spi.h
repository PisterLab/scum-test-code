#include <stdint.h>

// ADS SPI Command Definition Byte Assignments (Datasheet, p35)
#define _WAKEUP 0x02 // Wake-up from standby mode
#define _STANDBY 0x04 // Enter Standby mode
#define _RESET 0x06 // Reset the device registers to default
#define _START 0x08 // Start and restart (synchronize) conversions
#define _STOP 0x0A // Stop conversion
#define _RDATAC 0x10 // Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC 0x11 // Stop Read Data Continuous mode
#define _RDATA 0x12 // Read data by command; supports multiple read back


typedef union int16_buff_t{
	int16_t value;
	uint8_t bytes[2];
} int16_buff_t;

typedef struct int24_buff_t{
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


unsigned int read_gyro_x();
unsigned int read_gyro_y();
unsigned int read_gyro_z();

void clock_toggle(unsigned char cycle);

void spi_write(unsigned char writeByte);

unsigned char spi_read();

void spi_chip_select();

void spi_chip_deselect();

void initialize_imu(void);

unsigned int read_acc_y();

unsigned int read_acc_z();

void test_imu_life();

unsigned char read_imu_register(unsigned char reg);

void write_imu_register(unsigned char reg, unsigned char data);

void read_all_imu_data(imu_data_t* imu_measurement);

void log_imu_data(imu_data_t* imu_measurement);