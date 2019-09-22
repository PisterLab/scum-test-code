unsigned reverse(unsigned x);

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

unsigned int get_asc_bit(unsigned int position);
unsigned int get_GPI_enables(void);
unsigned int get_GPO_enables(void);
unsigned char get_GPI_control(unsigned short rowNum);
unsigned char get_GPO_control(unsigned short rowNum);