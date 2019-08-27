// Functions written by Brad, originally for 3B
void analog_scan_chain_write(unsigned int* scan_bits);
void analog_scan_chain_load(void);
void initialize_2M_DAC(void);
void set_2M_RC_frequency(int coarse1, int coarse2, int coarse3, int fine, int superfine);
void read_counters(unsigned int* count_2M, unsigned int* count_LC, unsigned int* count_32k);
unsigned int flip_lsb8(unsigned int in);
void update_PN31_byte(unsigned int* current_lfsr);
void TX_load_PN_data(unsigned int num_bytes);
void TX_load_counter_data(unsigned int num_bytes);
void set_asc_bit(unsigned int position);
void clear_asc_bit(unsigned int position);

// Functions written by Fil, mostly for 3
void enable_polyphase_ASC(void);
void disable_polyphase_ASC(void);
void disable_div_power_ASC(void);
void enable_div_power_ASC(void);
void ext_clk_ble_ASC(void);
void int_clk_ble_ASC(void);
void enable_1mhz_ble_ASC(void);
void disable_1mhz_ble_ASC(void);
void set_LC_current(unsigned int current);
void set_PA_supply(unsigned int code);
void set_LO_supply(unsigned int code, unsigned char panic);
void set_DIV_supply(unsigned int code, unsigned char panic);
void prescaler(int code);
void LC_monotonic(int LC_code);
void LC_FREQCHANGE(int coarse, int mid, int fine);
void divProgram(unsigned int div_ratio, unsigned int reset, unsigned int enable);