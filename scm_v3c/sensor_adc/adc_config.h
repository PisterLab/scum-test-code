void prog_asc_bit(unsigned int position, unsigned int val);
void scan_config_adc(unsigned int sel_reset, unsigned int sel_convert, 
	unsigned int sel_pga_amplify,
	unsigned int pga_gain[], unsigned int adc_settle[], 
	unsigned int bgr_tune[], unsigned int constgm_tune[], 
	unsigned int vbatDiv4_en, unsigned int ldo_en,
	unsigned int input_mux_sel[], unsigned int pga_byp);
void gpio_loopback_config_adc(void);
void gpio_onchip_config_adc(unsigned int gpi_control, unsigned int gpo_read);
