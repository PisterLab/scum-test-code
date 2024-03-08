#include "spi.h"

#include "Memory_Map.h"

#define CS_PIN 15
#define CLK_PIN 14
#define DIN_PIN 13   // Used when reading data from the IMU thus a SCuM input
#define DATA_PIN 12  // Used when writing to the IMU thus a SCuM output

void spi_write(unsigned char writeByte) {
    int j;
    int t = 0;
    int clk_pin = CLK_PIN;
    int data_pin = DATA_PIN;

    for (j = 7; j >= 0; j--) {
        if ((writeByte & (0x01 << j)) != 0) {
            GPIO_REG__OUTPUT &= ~(1 << clk_pin);  // clock low
            GPIO_REG__OUTPUT |= 1 << data_pin;    // write a 1
            GPIO_REG__OUTPUT |= 1 << clk_pin;     // clock high
        } else {
            GPIO_REG__OUTPUT &= ~(1 << clk_pin);   // clock low
            GPIO_REG__OUTPUT &= ~(1 << data_pin);  // write a 0
            GPIO_REG__OUTPUT |= (1 << clk_pin);    // clock high
        }
    }

    GPIO_REG__OUTPUT &= ~(1 << data_pin);  // set data out to 0
}

unsigned char spi_read() {
    unsigned char readByte;
    int j;
    int t = 0;
    int clk_pin = CLK_PIN;
    int din_pin = DIN_PIN;
    readByte = 0;

    GPIO_REG__OUTPUT &= ~(1 << clk_pin);  // clock low

    for (j = 7; j >= 0; j--) {
        GPIO_REG__OUTPUT |= (1 << clk_pin);  // clock high
        readByte |= ((GPIO_REG__INPUT & (1 << din_pin)) >> din_pin) << j;
        GPIO_REG__OUTPUT &= ~(1 << clk_pin);  // clock low
    }

    return readByte;
}

void spi_chip_select() {
    int t = 0;
    int dout_pin = DATA_PIN;
    int cs_pin = CS_PIN;
    // drop chip select low to select the chip
    GPIO_REG__OUTPUT &= ~(1 << cs_pin);
    GPIO_REG__OUTPUT &= ~(1 << dout_pin);
    for (t = 0; t < 50; t++);
}

void spi_chip_deselect() {
    int cs_pin = CS_PIN;
    // hold chip select high to deselect the chip
    GPIO_REG__OUTPUT |= (1 << cs_pin);
}

void initialize_imu(void) {
    int i;

    write_imu_register(0x06, 0x41);
    for (i = 0; i < 50000; i++);
    write_imu_register(0x06, 0x01);
    for (i = 0; i < 50000; i++);
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

    spi_chip_select();       // drop chip select
    spi_write(reg);          // write the selected register to the port
    read_byte = spi_read();  // clock out the bits and read them
    spi_chip_deselect();     // raise chip select

    return read_byte;
}

void write_imu_register(unsigned char reg, unsigned char data) {
    reg &= 0x7F;  // guarantee that the function input is valid (not necessarily
                  // a valid, and readable, register)

    spi_chip_select();    // drop chip select
    spi_write(reg);       // write the selected register to the port
    spi_write(data);      // write the desired register contents
    spi_chip_deselect();  // raise chip select
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
    printf(
        "AX: %3d %3d, AY: %3d %3d, AZ: %3d %3d, GX: %3d %3d, GY: %3d %3d, GZ: "
        "%3d %3d\n",
        imu_measurement->acc_x.bytes[0], imu_measurement->acc_x.bytes[1],
        imu_measurement->acc_y.bytes[0], imu_measurement->acc_y.bytes[1],
        imu_measurement->acc_z.bytes[0], imu_measurement->acc_z.bytes[1],
        imu_measurement->gyro_x.bytes[0], imu_measurement->gyro_x.bytes[1],
        imu_measurement->gyro_y.bytes[0], imu_measurement->gyro_y.bytes[1],
        imu_measurement->gyro_z.bytes[0], imu_measurement->gyro_z.bytes[1]);
}
