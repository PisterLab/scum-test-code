#include "imu.h"
#include "spi.h"

#include "Memory_Map.h"

int spi_handle;

void initialize_imu(void) {
	int i;
    spi_pin_config_t config;
    spi_mode_t mode;

    config.CS = 15;
    config.SCLK = 14;
    config.MOSI = 12;
    config.MISO = 13;
    
    spi_handle = open(&config, &mode);
	
	write_imu_register(0x06,0x41);
	for(i=0; i<50000; i++);
	write_imu_register(0x06,0x01);
	for(i=0; i<50000; i++);
}

unsigned int read_acc_x() {
    unsigned int acc_x;
    unsigned char read_byte;
    unsigned char write_byte = 0x2D;

    acc_x = (read_imu_register(write_byte)) << 8;
    write_byte = 0x2E;
    acc_x |= read_imu_register(write_byte);

    return acc_x;
}

unsigned int read_acc_y() {
    unsigned int acc_y;
    unsigned char read_byte;
    unsigned char write_byte = 0x2F;

    acc_y = (read_imu_register(write_byte)) << 8;
    write_byte = 0x30;
    acc_y |= read_imu_register(write_byte);

    return acc_y;
}

unsigned int read_acc_z() {
    unsigned int acc_z;
    unsigned char read_byte;
    unsigned char write_byte = 0x31;

    acc_z = (read_imu_register(write_byte)) << 8;
    write_byte = 0x32;
    acc_z |= read_imu_register(write_byte);

    return acc_z;
}

unsigned int read_gyro_x() {
    unsigned int gyro_x;
    unsigned char read_byte;
    unsigned char write_byte = 0x33;

    gyro_x = (read_imu_register(write_byte)) << 8;
    write_byte = 0x34;
    gyro_x |= read_imu_register(write_byte);

    return gyro_x;
}

unsigned int read_gyro_y() {
    unsigned int gyro_y;
    unsigned char read_byte;
    unsigned char write_byte = 0x35;

    gyro_y = (read_imu_register(write_byte)) << 8;
    write_byte = 0x36;
    gyro_y |= read_imu_register(write_byte);

    return gyro_y;
}

unsigned int read_gyro_z() {
    unsigned int gyro_z;
    unsigned char read_byte;
    unsigned char write_byte = 0x37;

    gyro_z = (read_imu_register(write_byte)) << 8;
    write_byte = 0x38;
    gyro_z |= read_imu_register(write_byte);

    return gyro_z;
}

void test_imu_life() {
    int i = 0;
    unsigned char read_byte;
    unsigned char write_byte = 0x00;

    read_byte = read_imu_register(write_byte);

    if (read_byte == 0xEA) {
        printf("My IMU is alive!!!\n");
    } else {
        printf("My IMU is not working :( \n");
    }
}

unsigned char read_imu_register(unsigned char reg) {
    unsigned char read_byte;
    reg &= 0x7F;
    reg |= 0x80;  // guarantee that the function input is a valid input (not
                  // necessarily a valid, and readable, register)

    ioctl(spi_handle, SPI_CS, 0);  // drop chip select
    write(spi_handle, reg);        // write the selected register to the port
    read(spi_handle, &read_byte);  // clock out the bits and read them
    ioctl(spi_handle, SPI_CS, 1);  // raise chip select

    return read_byte;
}

void write_imu_register(unsigned char reg, unsigned char data) {
    reg &= 0x7F;  // guarantee that the function input is valid (not necessarily
                  // a valid, and readable, register)

    ioctl(spi_handle, SPI_CS, 0);  // drop chip select
    write(spi_handle, reg);        // write the selected register to the port
    write(spi_handle, data);       // clock out the bits and read them
    ioctl(spi_handle, SPI_CS, 1);  // raise chip select
}

void read_all_imu_data(imu_data_t* imu_measurement) {
    imu_measurement->acc_x.value = read_acc_x();
    imu_measurement->acc_y.value = read_acc_y();
    imu_measurement->acc_z.value = read_acc_z();
    imu_measurement->gyro_x.value = read_gyro_x();
    imu_measurement->gyro_y.value = read_gyro_y();
    imu_measurement->gyro_z.value = read_gyro_z();
}

void log_imu_data(imu_data_t* imu_measurement) {
	printf("AX: %3d %3d, AY: %3d %3d, AZ: %3d %3d, GX: %3d %3d, GY: %3d %3d, GZ: %3d %3d\n",
		imu_measurement->acc_x.bytes[0],
		imu_measurement->acc_x.bytes[1],
		imu_measurement->acc_y.bytes[0],
		imu_measurement->acc_y.bytes[1],
		imu_measurement->acc_z.bytes[0],
		imu_measurement->acc_z.bytes[1],
		imu_measurement->gyro_x.bytes[0],
		imu_measurement->gyro_x.bytes[1],
		imu_measurement->gyro_y.bytes[0],
		imu_measurement->gyro_y.bytes[1],
		imu_measurement->gyro_z.bytes[0],
		imu_measurement->gyro_z.bytes[1]);
}
