unsigned reverse(unsigned x);
unsigned int crc32c(unsigned char *message, unsigned int length);
unsigned char flipChar(unsigned char b);
void init_ldo_control(void);
unsigned int sram_test(unsigned int * baseAddress, unsigned int num_dwords);
void radio_init_rx_MF(void);
void radio_init_rx_ZCC(void);
void radio_init_tx(void);
void radio_init_divider(unsigned int div_value);
void radio_disable_all(void);
void GPO_control(unsigned char row1, unsigned char row2, unsigned char row3, unsigned char row4);
void GPI_control(char row1, char row2, char row3, char row4);
unsigned int read_IF_estimate(void);
unsigned int read_LQI(void);
unsigned int read_RSSI(void);
void set_IF_clock_frequency(int coarse, int fine, int high_range);
void GPO_enables(unsigned int mask);
void GPI_enables(unsigned int mask);
void set_IF_LDO_voltage(int code);
void set_VDDD_LDO_voltage(int code);
void set_AUX_LDO_voltage(int code);
void set_ALWAYSON_LDO_voltage(int code);
void LC_monotonic_ASC(int LC_code, int mid_divs, int coarse_divs, int coarse_offset);
void LC_FREQCHANGE_ASC(int coarse, int mid, int fine);
void LC_FREQCHANGE(int coarse, int mid, int fine);
void radio_enable_PA(void);
void radio_enable_LO(void);
void radio_enable_RX(void);
void read_counters_3B(unsigned int* count_2M, unsigned int* count_LC, unsigned int* count_adc, unsigned int* count_32k);
void do_fake_cal(void);
void packet_test_loop(unsigned int num_packets);
void set_IF_stg3gm_ASC(unsigned int Igm, unsigned int Qgm);
void set_IF_gain_ASC(unsigned int Igain, unsigned int Qgain);
void set_zcc_demod_threshold(unsigned int thresh);
void set_IF_comparator_trim_I(unsigned int ptrim, unsigned int ntrim);
void set_IF_comparator_trim_Q(unsigned int ptrim, unsigned int ntrim);
void set_IF_ZCC_clkdiv(unsigned int div_value);
void set_IF_ZCC_early(unsigned int early_value);
void initialize_mote(void);
void set_sys_clk_secondary_freq(unsigned int coarse, unsigned int fine);
unsigned int build_RX_channel_table(unsigned int channel_11_LC_code);
void build_TX_channel_table(unsigned int channel_11_LC_code,unsigned int count_LC_RX_ch11);
void build_channel_table(unsigned int channel_11_LC_code);
unsigned int estimate_temperature_2M_32k(void);

void set_LC_current(unsigned int current);
void set_PA_supply(unsigned int code);
void set_LO_supply(unsigned int code, unsigned char panic);
void set_DIV_supply(unsigned int code, unsigned char panic);
void prescaler(int code);
void divProgram(unsigned int div_ratio, unsigned int reset, unsigned int enable);

void find_mid_divs(int mid0);

void radio_init_tx_BLE(void);
void gen_test_ble_packet(unsigned char *packet);
void load_tx_arb_fifo(unsigned char *packet);
void transmit_tx_arb_fifo(void);

void gen_ble_packet(unsigned char *packet, unsigned char *AdvA, unsigned char channel);
void LC_monotonic(int LC_code, int mid_divs, int coarse_divs, int coarse_offset);

// SPI functions
void spi_write(unsigned char writeByte);
unsigned char spi_read(void);
void spi_chip_select(void);
void spi_chip_deselect(void);

// Accelerometer read functions
unsigned int read_acc_x(void);
unsigned int read_acc_y(void);
unsigned int read_acc_z(void);
void test_imu_life(void);

// IMU-specific read/write functions
unsigned char read_imu_register(unsigned char reg);
void write_imu_register(unsigned char reg, unsigned char data);

