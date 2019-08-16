void reset_adc(unsigned int cycles_low);
void onchip_control_adc_shot(void);
void loopback_control_adc_shot(unsigned int cycles_reset,
	unsigned int cycles_to_start, unsigned int cycles_pga);
void onchip_control_adc_continuous(void);
void loopback_control_adc_continuous(unsigned int cycles_reset,
	unsigned int cycles_to_start, unsigned int cycles_pga);
void halt_adc_continuous(void);
