#include "Memory_Map.h"
#include "spi.h"

void spi_write(unsigned char writeByte) {
	int j;
	int t=0;
	int clk_pin = 14;
	int data_pin = 12;
	
	for (j=7;j>=0;j--) {
		if ((writeByte&(0x01<<j)) != 0) {
			GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
			GPIO_REG__OUTPUT |= 1 << data_pin; // write a 1
			GPIO_REG__OUTPUT |= 1 << clk_pin; // clock high
		}
		else {
			GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
			GPIO_REG__OUTPUT &= ~(1 << data_pin); // write a 0
			GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
		}
	}
	
	GPIO_REG__OUTPUT &= ~(1 << data_pin); // set data out to 0
}

unsigned char spi_read() {
	unsigned char readByte;
	int j;
	int t = 0;
	int clk_pin = 14;
	int din_pin = 13;
	readByte=0;

	
	GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
	
	for (j=7;j>=0;j--) {
		GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
		readByte |= ((GPIO_REG__INPUT&(1 << din_pin))>>din_pin)<<j;		
		GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low		
	}
	
	return readByte;
}

void spi_chip_select() {
	int t = 0;
	int dout_pin = 12;
	int cs_pin = 15;
	// drop chip select low to select the chip
	GPIO_REG__OUTPUT &= ~(1 << cs_pin);
	GPIO_REG__OUTPUT &= ~(1 << dout_pin);
	for(t=0; t<50; t++);
}

void spi_chip_deselect() {
	int cs_pin = 15;
	// hold chip select high to deselect the chip
	GPIO_REG__OUTPUT |= (1 << cs_pin);
}

unsigned int read_acc_x() {
	unsigned int acc_x;
	unsigned char read_byte;
	unsigned char write_byte = 0x2D;
	
	acc_x = (read_imu_register(write_byte))<<8;	
	write_byte = 0x2E;
	acc_x |= read_imu_register(write_byte);
	
	return acc_x;
	
}

unsigned int read_acc_y() {
	unsigned int acc_y;
	unsigned char read_byte;
	unsigned char write_byte = 0x2F;
	
	acc_y = (read_imu_register(write_byte))<<8;	
	write_byte = 0x30;
	acc_y |= read_imu_register(write_byte);
	
	return acc_y;
}

unsigned int read_acc_z() {
	unsigned int acc_z;
	unsigned char read_byte;
	unsigned char write_byte = 0x31;
	
	acc_z = (read_imu_register(write_byte))<<8;
	write_byte = 0x32;
	acc_z |= read_imu_register(write_byte);
	
	return acc_z;
}


unsigned int read_gyro_x() {
	unsigned int gyro_x;
	unsigned char read_byte;
	unsigned char write_byte = 0x33;
	
	gyro_x = (read_imu_register(write_byte))<<8;
	write_byte = 0x34;
	gyro_x |= read_imu_register(write_byte);
	
	return gyro_x;
}

unsigned int read_gyro_y() {
	unsigned int gyro_y;
	unsigned char read_byte;
	unsigned char write_byte = 0x35;
	
	gyro_y = (read_imu_register(write_byte))<<8;
	write_byte = 0x36;
	gyro_y |= read_imu_register(write_byte);
	
	return gyro_y;
}

unsigned int read_gyro_z() {
	unsigned int gyro_z;
	unsigned char read_byte;
	unsigned char write_byte = 0x37;
	
	gyro_z = (read_imu_register(write_byte))<<8;
	write_byte = 0x38;
	gyro_z |= read_imu_register(write_byte);
	
	return gyro_z;
}

imu_data_t test_imu_life() {
	imu_data_t imu_measurement;
	int i = 0;
	unsigned char read_byte;
	unsigned char write_byte = 0x00;
	
	imu_measurement.acc_x.value = 66;
	read_byte = read_imu_register(write_byte);	

	if (read_byte == 0xEA) {
		printf("My IMU is alive!!!\n");

		imu_measurement.acc_x.value = 11;
	}
	else {
		printf("My IMU is not working :( \n");
		imu_measurement.acc_x.value = 22;
	}
	
	return imu_measurement;
}

unsigned char read_imu_register(unsigned char reg) {
	unsigned char read_byte;
	reg &= 0x7F;
	reg |= 0x80;						// guarantee that the function input is a valid input (not necessarily a valid, and readable, register)
	
	spi_chip_select();      // drop chip select
	spi_write(reg);         // write the selected register to the port
	read_byte = spi_read(); // clock out the bits and read them
	spi_chip_deselect();    // raise chip select
	
	return read_byte;
}

void write_imu_register(unsigned char reg, unsigned char data) {
	reg &= 0x7F;						// guarantee that the function input is valid (not necessarily a valid, and readable, register)
	
	spi_chip_select();			// drop chip select
	spi_write(reg);					// write the selected register to the port
	spi_write(data);				// write the desired register contents
	spi_chip_deselect();		// raise chip select
	
}


