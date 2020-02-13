unsigned int read_gyro_x();
unsigned int read_gyro_y();
unsigned int read_gyro_z();

void spi_write(unsigned char writeByte);

unsigned char spi_read();

void spi_chip_select();

void spi_chip_deselect();

unsigned int read_acc_y();

unsigned int read_acc_z();

void test_imu_life();

unsigned char read_imu_register(unsigned char reg);

void write_imu_register(unsigned char reg, unsigned char data);
