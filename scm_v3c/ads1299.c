#include "ads1299.h"
#include "Memory_Map.h"
#include <stdio.h>

// TODO: move this into the spi_config struct
// SPI bus definitions
#define CS_PIN      11
#define CLK_PIN     14
#define DIN_PIN     13 // Used when reading data from the IMU thus a SCuM input
#define DATA_PIN    12 // Used when writing to the IMU thus a SCuM output
// ADS pin definitions
#define RST_PIN     15  // need to check IO (output)
#define DRDY_PIN    3  // need to check IO (input)
#define ADS_DVDD 	7	// ADS1299 is powered by SCuM's GPIO pin 7, supplies 1.8V


void digitalWrite(int pin, int high_low) {
	printf("wrote pin: %d high_low =: %d\r\n", pin, high_low);
    if (high_low) {
        GPIO_REG__OUTPUT |= (1 << pin);
	}
    else {
        GPIO_REG__OUTPUT &= ~(1 << pin);
	}
}

uint8_t digitalRead(int pin) {
	uint8_t i = 0;
	int clk_pin = CLK_PIN;
	i = (GPIO_REG__INPUT&(1 << pin)) >> pin;
	// printf("i = %d\n", i);
	return i;
}

void ADS_initialize() {
    int t;
    int rst_pin = RST_PIN;
//    int drdy_pin = DRDY_PIN;
    int clk_pin = CLK_PIN;
    int cs_pin = CS_PIN;
    int MOSI_pin = DATA_PIN;

	// Enable power to the ADS1299
	digitalWrite(ADS_DVDD, 1);
    
    // cortex clock 2MHz(0.5us)
    // power up ~32ms
    for (t = 0; t < 65000; t++);
    
    // toggle reset pin
    digitalWrite(rst_pin, 0);    // reset low
	digitalWrite(DATA_PIN, 0);    // reset low
    digitalWrite(CLK_PIN, 0);    // reset low
    digitalWrite(CS_PIN, 0);    // reset low

    for (t = 0; t < 10; t++);
//		digitalWrite(MOSI_pin,0);		// test delay time
	printf("Initialized ADS1299\r\n");
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

void ADS_WREG(unsigned char addr, unsigned char val) {
	unsigned char opcode1 = addr + 0x40;

	digitalWrite(CS_PIN, 0);
	spi_write(opcode1);
	spi_write(0x00);
	spi_write(val);
	digitalWrite(CS_PIN, 1);
}

void ADS_RREGS(unsigned char addr, unsigned char NregminusOne) {
    unsigned char opcode1 = addr + 0x20;
    unsigned char read_reg;
	unsigned char i;

    digitalWrite(CS_PIN, 0);
    spi_write(opcode1);
	spi_write(NregminusOne);
	for (i = 0; i <= NregminusOne; i++) {
		read_reg = spi_read();
		printf("%x\n", read_reg);
	}
    digitalWrite(CS_PIN, 1);
}

void read_ads_register(ads_data_t* ads_measurement) {
	unsigned char read_reg;
	int nchan = 8;
	int i, j;
	int32_t read_24bit;

//	ADS_START();
	while (digitalRead(DRDY_PIN));
	// while (digitalRead(DRDY_PIN) == 0x00);
	// while (digitalRead(DRDY_PIN) == 0x01);

	digitalWrite(CS_PIN, 0);
	read_24bit = 0;
	for (i = 0; i < 3; i++) {
		read_reg = spi_read();
		read_24bit = (read_24bit << 8) | read_reg;
		// printf("%x", read_reg);
	}
	// printf("\n");
	ads_measurement->config = read_24bit;
	// printf("config = %x\n", read_24bit);
	for (j = 0; j < nchan; j++) {
		read_24bit = 0;
		for (i = 0; i < 3; i++) {
			read_reg = spi_read();
			read_24bit = (read_24bit << 8) | read_reg;
			// printf("%x", read_reg);
		}
		// printf("\n");
		if (read_24bit >> 23) {
			read_24bit |= 0xFF000000;
		}
		else {
			read_24bit &= 0x00FFFFFF;
		}
		ads_measurement->channel[j] = read_24bit;
		// printf("channel[%d] = %x\n", j, read_24bit);
	}
	digitalWrite(CS_PIN, 1);

	// printf("%x\n", ads_measurement->config);
	// for (j = 0; j < nchan; j++) {
	// 	printf("%x\n", ads_measurement->channel[j]);
	// }
}

