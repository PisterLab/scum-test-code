#include "ads1299.h"
#include "Memory_Map.h"
#include <stdio.h>

// TODO: move this into the spi_config struct
#define CS_PIN      15
#define CLK_PIN     14
#define DIN_PIN     13 // Used when reading data from the IMU thus a SCuM input
#define DATA_PIN    12 // Used when writing to the IMU thus a SCuM output
#define RST_PIN     11  // need to check IO (output)
#define DRDY_PIN    10  // need to check IO (input)

void digitalWrite(int pin, int high_low) {
	printf("pin: %d high_low =: %d", pin, high_low);
    if (high_low) {
        GPIO_REG__OUTPUT |= (1 << pin);
			printf("%x\n", GPIO_REG__OUTPUT);
				printf("high\n");
		}
    else {
        GPIO_REG__OUTPUT &= ~(1 << pin);
						printf("%x\n", GPIO_REG__OUTPUT);
				printf("low\n");
		}
}

void ADS_initialize() {
    int t;
    int rst_pin = RST_PIN;
//    int drdy_pin = DRDY_PIN;
    int clk_pin = CLK_PIN;
    int cs_pin = CS_PIN;
    int MOSI_pin = DATA_PIN;
    
    // cortex clock 2MHz(0.5us)
    // power up ~32ms
    for (t = 0; t < 65000; t++);
    
    // toggle reset pin
    digitalWrite(rst_pin, 0);    // reset low
	    digitalWrite(12, 0);    // reset low
    digitalWrite(14, 0);    // reset low
    digitalWrite(15, 0);    // reset low

    for (t = 0; t < 10; t++);
//		digitalWrite(MOSI_pin,0);		// test delay time
		printf("run");
/*    digitalWrite(rst_pin, 1);     // reset high
    
    // recommend to wait 18 Tclk
    for (t = 0; t < 20; t++);
    
    digitalWrite(clk_pin, 0);
    digitalWrite(MOSI_pin, 0);
    
    // initialize chip select, and reset
    digitalWrite(cs_pin, 1);
    digitalWrite(rst_pin, 1);*/
}

// System Commands
void ADS_WAKEUP() {
    int t;
    
    digitalWrite(CS_PIN, 0);
    spi_write(_WAKEUP);
    digitalWrite(CS_PIN, 1);
    for (t = 0; t < 10; t++);
    // must wait 4 tCLK before sending another commands
}

// only allow to send WAKEUP after sending STANDBY
void ADS_STANBY() {
    digitalWrite(CS_PIN, 0);
    spi_write(_STANDBY);
    digitalWrite(CS_PIN, 1);
}

// reset all the registers to defaut settings
void ADS_RESET() {
    int t;
    
    digitalWrite(CS_PIN, 0);
    spi_write(_RESET);
    
    // must wait 18 tCLK to execute this command
    for (t = 0; t < 20; t++);
    
    digitalWrite(CS_PIN, 1);
}

// start data conversion
void ADS_START() {
    digitalWrite(CS_PIN, 0);
    spi_write(_START);
    digitalWrite(CS_PIN, 1);
}

// stop data conversion
void ADS_STOP() {
    digitalWrite(CS_PIN, 0);
    spi_write(_STOP);
    digitalWrite(CS_PIN, 1);
}

void ADS_RDATAC() {
    int t;
    
    digitalWrite(CS_PIN, 0);
    spi_write(_RDATAC);
    digitalWrite(CS_PIN, 1);
    
    // must wait 4 tCLK after executing thsi command
    for (t = 0; t < 10; t++);
}

void ADS_SDATAC() {
    int t;
    
    digitalWrite(CS_PIN, 0);
    spi_write(_SDATAC);
    digitalWrite(CS_PIN, 1);
    
    // must wait 4 tCLK after executing thsi command
    for (t = 0; t < 10; t++);
}

unsigned char ADS_RREG(unsigned char addr) {
    unsigned char opcode1 = addr + 0x20;
    unsigned char read_reg;
    
    digitalWrite(CS_PIN, 0);
    spi_write(opcode1);
    spi_write(0x00);
    read_reg = spi_read();
    digitalWrite(CS_PIN, 1);
    
    return read_reg;
}
/*
void clock_toggle(unsigned char cycle) {
	int j;
	int clk_pin = CLK_PIN;

	for (j = 0; j < cycle; j++) {
		GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
		GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
	}
}
*/
/*
void spi_write(unsigned char writeByte) {
	int j;
	int t=0;
	int clk_pin = CLK_PIN;
	int data_pin = DATA_PIN;

	// sample at falling edge
	for (j=7;j>=0;j--) {
		if ((writeByte&(0x01<<j)) != 0) {
			GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
			GPIO_REG__OUTPUT |= 1 << data_pin; // write a 1
			GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
		}
		else {
			GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
			GPIO_REG__OUTPUT &= ~(1 << data_pin); // write a 0
			GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
		}
	}
	
	// sample at rising edge
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
}*/
/*
unsigned char spi_read() {
	unsigned char readByte;
	int j;
	int t = 0;
	int clk_pin = CLK_PIN;
	int din_pin = DIN_PIN;
	readByte=0;

	// sample at falling edge
	GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
	
	for (j=7;j>=0;j--) {
		GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
		readByte |= ((GPIO_REG__INPUT&(1 << din_pin))>>din_pin)<<j;		
		GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high		
	}
	
	// sample at rising edge
	GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low
	
	for (j=7;j>=0;j--) {
		GPIO_REG__OUTPUT |= (1 << clk_pin); // clock high
		readByte |= ((GPIO_REG__INPUT&(1 << din_pin))>>din_pin)<<j;		
		GPIO_REG__OUTPUT &= ~(1 << clk_pin); // clock low		
	}
	
	return readByte;
}*/
/*
void spi_chip_select() {
	int t = 0;
	int dout_pin = DATA_PIN;
	int cs_pin = CS_PIN;
	// drop chip select low to select the chip
	GPIO_REG__OUTPUT &= ~(1 << cs_pin);
	GPIO_REG__OUTPUT &= ~(1 << dout_pin);
	for(t=0; t<50; t++);
}

void spi_chip_deselect() {
	int cs_pin = CS_PIN;
	// hold chip select high to deselect the chip
	GPIO_REG__OUTPUT |= (1 << cs_pin);
}*/
/*
void initialize_imu(void) {
	int i;
	
	write_imu_register(0x06,0x41);
	for(i=0; i<50000; i++);
	write_imu_register(0x06,0x01);
	for(i=0; i<50000; i++);
}

void initialize_ads(void) {
	start_select();
	reset_select();
}

void start_select() {
	int start_pin = START_PIN;
	GPIO_REG__OUTPUT |= (1 << start_pin);
}
void reset_select() {
	int reset_pin = RST_PIN;
	GPIO_REG__OUTPUT |= (1 << reset_pin);
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

void test_imu_life() {
	int i = 0;
	unsigned char read_byte;
	unsigned char write_byte = 0x00;

	read_byte = read_imu_register(write_byte);	

	if (read_byte == 0xEA) {
		printf("My IMU is alive!!!\n");
	}
	else {
		printf("My IMU is not working :( \n");	}
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


void read_ads_register(ads_data_t* ads_measurement) {
	unsigned char reg;
	
	unsigned char status1;
	unsigned char status2;
	unsigned char status3;
	unsigned char electrode1_1;
	unsigned char electrode1_2;
	unsigned char electrode1_3;
	unsigned char electrode2_1;
	unsigned char electrode2_2;
	unsigned char electrode2_3;
	unsigned char electrode3_1;
	unsigned char electrode3_2;
	unsigned char electrode3_3;
	unsigned char electrode4_1;
	unsigned char electrode4_2;
	unsigned char electrode4_3;
	unsigned char electrode5_1;
	unsigned char electrode5_2;
	unsigned char electrode5_3;
	unsigned char electrode6_1;
	unsigned char electrode6_2;
	unsigned char electrode6_3;
	unsigned char electrode7_1;
	unsigned char electrode7_2;
	unsigned char electrode7_3;
	unsigned char electrode8_1;
	unsigned char electrode8_2;
	unsigned char electrode8_3;
	
	unsigned int electrode1_val;
	
//	reg &= 0x12;
//	reg |= 0x12;						
//	reg = 0x12;
	
	spi_chip_select();      // drop chip select
	reg = 0x11;
	spi_write(reg);
//	clock_toggle(4);		// wait for 4 cycle
//	reg = 0x20;
//	spi_write(reg);         // write the selected register to the port
//	reg = 0x01;
//	spi_write(reg);
//	status1 = spi_read(); // clock out the bits and read them
//	status2 = spi_read(); // clock out the bits and read them
//	status3 = spi_read(); // clock out the bits and read them
//	printf("Results: %x %x\n",status1, status2);
	spi_chip_deselect();
	electrode1_1 = spi_read(); // clock out the bits and read them
	electrode1_2 = spi_read(); // clock out the bits and read them
	electrode1_3 = spi_read(); // clock out the bits and read them
	electrode2_1 = spi_read(); // clock out the bits and read them
	electrode2_2 = spi_read(); // clock out the bits and read them
	electrode2_3 = spi_read(); // clock out the bits and read them
	electrode3_1 = spi_read(); // clock out the bits and read them
	electrode3_2 = spi_read(); // clock out the bits and read them
	electrode3_3 = spi_read(); // clock out the bits and read them
	electrode4_1 = spi_read(); // clock out the bits and read them
	electrode4_2 = spi_read(); // clock out the bits and read them
	electrode4_3 = spi_read(); // clock out the bits and read them
	electrode5_1 = spi_read(); // clock out the bits and read them
	electrode5_2 = spi_read(); // clock out the bits and read them
	electrode5_3 = spi_read(); // clock out the bits and read them
	electrode6_1 = spi_read(); // clock out the bits and read them
	electrode6_2 = spi_read(); // clock out the bits and read them
	electrode6_3 = spi_read(); // clock out the bits and read them
	electrode7_1 = spi_read(); // clock out the bits and read them
	electrode7_2 = spi_read(); // clock out the bits and read them
	electrode7_3 = spi_read(); // clock out the bits and read them
	electrode8_1 = spi_read(); // clock out the bits and read them
	electrode8_2 = spi_read(); // clock out the bits and read them
	electrode8_3 = spi_read(); // clock out the bits and read them
	spi_chip_deselect();    // raise chip select
	
	ads_measurement->electrode1.bytes[0] = electrode1_1;
	ads_measurement->electrode1.bytes[1] = electrode1_2;
	ads_measurement->electrode1.bytes[2] = electrode1_3;
	
	electrode1_val = electrode1_1<<16;
	electrode1_val |= electrode1_2<<8;
	electrode1_val |= electrode1_3;
	
	ads_measurement->electrode1.value = electrode1_val;
}

void log_ads_data(ads_data_t* ads_measurement) {
	printf("Results: %x %x %x\n",
		ads_measurement->electrode1.bytes[0],
		ads_measurement->electrode1.bytes[1],
		ads_measurement->electrode1.bytes[2]);
}
*/
