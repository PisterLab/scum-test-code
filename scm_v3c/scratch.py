test_dict = dict(
	gpio_direction = [1]*16, # 0: signal to SCM, 1: signal from SCM
	#############
	### NOTES ###
	#############
	# All things are MSB -> LSB unless otherwise noted. Any flips, etc. have
	# been handled internally so everyone can keep their sanity.

	##############################
	### GPIO Direction Control ###
	##############################

	#####################
	### Miscellaneous ###
	#####################
	# Mux select for 7 counters output to GPIO bank 11
	mux_sel_gpio11=[0]*3,	# 000: analog_rdata[31:0]
							# 001: analog_rdata[63:32]
							# 010: analog_rdata[95:64]
							# 011: analog_rdata[127:96]
							# 100: analog_rdata[159:128]
							# 101: analog_rdata[191:160]
							# 110: analog_rdata[223:192]
							# 111: 32'b0

	########################
	### Counter Settings ###
	########################
	# counter0: TIMER32k, 0 is reset.
	# counter1: LF_CLOCK, 0 is reset.
	# counter2: HF_CLOCK, 0 is reset.
	# counter3: RC_2MHz, 0 is reset.
	# counter4: LF_ext_GPIO, 0 is reset.
	# counter5: LC_div_N, 0 is reset.
	# counter6: ADC_CLK, 0 is reset.
	sel_data=1,					# Recovered data. 1 for ZCC, 0 for other baseband.
	sel_counter_resets=[1]*7,	# Select signal for counters [0,...,6]. 
								# 0 for ASC[8,...,14].
								# 1 for M0 analog_cfg[0,...,6] 
	counter_resets=[0]*7, 		# 0 is reset.
	counter_enables=[0]*7,		# 1 is enable.
	
	########################
	### Divider Settings ###
	########################
	# Enables: 1 is enable.
	div_RFTimer_enable=0, div_CortexM0_enable=0, div_GFSK_enable=0,
	div_ext_GPIO_enable=0, div_integ_enable=0, div_2MHz_enable=0,

	# Resets: 1 is reset.
	div_RFTimer_reset=0, div_CortexM0_reset=0, div_GFSK_reset=0,
	div_ext_GPIO_reset=0, div_integ_reset=0, div_2MHz_reset=0,

	# Passthroughs: 1 is passthrough.
	div_RFTimer_PT=0, div_CortexM0_PT=0, div_GFSK_PT=0,
	div_ext_GPIO_PT=0, div_integ_PT=0, div_2MHz_PT=0,

	# Divide values: MSB -> LSB. Don't invert these! It's already handled.
	div_RFTimer_Nin=[0]*8, div_CortexM0_Nin=[0]*8, div_GFSK_Nin=[0]*8,
	div_ext_GPIO_Nin=[0]*8, div_integ_Nin=[0]*8, div_2MHz_Nin=[0]*8,

	############################
	### Clock Mux & Crossbar ###
	############################
	mux_sel_RFTimer=0,			# 0 for divider_out_RFTimer, 1 for TIMER32k.
	mux_sel_GFSK_clk=0, 		# 0 is LC_div_N, 1 is divider_out_GFSK.	
	mux3_sel_CLK2MHz=[0]*2, 	# 00: RC_2MHz
								# 01: divider_out_2MHz
								# 10: LC_2MHz
								# 11: 1'b0
	mux3_sel_CLK1MHz=[0]*2,		# 00: LC_1MHz_dyn
								# 01: divider_out_2MHz
								# 10: LC_1MHz_stat
								# 11: 1'b0
	sel_mux3in=[0,0],	# 00: LF_CLOCK
						# 01: HF_CLOCK
						# 10: LF_ext_PAD
						# 11: 1'b0

	# Crossbar Settings
	# 00: LF_CLOCK
	# 01: HF_CLOCK
	# 02: RC_2MHz
	# 03: TIMER32k
	# 04: LF_ext_PAD
	# 05: LF_ext_GPIO
	# 06: ADC_CLK
	# 07: LC_div_N
	# 08: LC_2MHz
	# 09: LC_1MHz_stat
	# 10: LC_1MHz_dyn
	# 11: 1'b0
	# 12: 1'b0
	# 13: 1'b0
	# 14: 1'b0
	# 15: 1'b0
	crossbar_HCLK=[0]*4,	
	crossbar_RFTimer=[0]*4,	
	crossbar_TX_chip_clk_to_cortex=[0]*4,
	crossbar_symbol_clk_ble=[0]*4,
	crossbar_divider_out_INTEG=[0]*4,
	crossbar_GFSK_CLK=[0]*4,
	crossbar_EXT_CLK_GPIO=[0]*4,
	crossbar_EXT_CLK_GPIO2=[0]*4,
	crossbar_BLE_PDA=[0]*4,

	##############################
	### Bob's Digital Baseband ###
	##############################
	IQ_select=[0]*2, 
	op_mode=[0]*2, 
	agc_overload=0,
	agc_ext_or_int=0, 
	vga_select=0, 
	mf_data_sign=0,

	################################
	### Brian's Digital Baseband ###
	################################
	# Vaguely labeled things called 'Brian's DBB
	COUNT_THRESH=[0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0],
	FB_EN=0, 
	CLK_DIV_ext=[0]*8,
	DEMOD_EN=0, 
	INIT_INTEG=[0]*11,
	sel_FREQ_CTRL_WORD=0, 
	SACLIENT_ext=[0]*16,
	EARLY_DECISION_MARGIN_ext=[0]*9 + [1,0,0,0,1,0,0], 
	mux_sel_FREQ_CTRL_WORD_SCAN=[0]*2, 	# 00: ASC[237:227]
										# 01: FREQ_CTRL_WORD_SCAN
										# 10: analog_cfg[74:64]
										# 11: 11'b0
	FREQ_CTRL_WORD_SCAN=[0]*11,

	# DBB settings
	sel_dbb_ring_data_in=1,	# 0: LC osc, Q branch (SIG_IN)
							# 1: LC Osc, I branch (COMPP_ZCC)
	sel_dbb_ring_clk_in=1,	# 0: LC osc, Q branch (ZCC_CLK_Ring)
							# 1: LC osc, I branch (ZCC_CLK)
	sel_resetb=1,			# 0: ASC[241], 1: analog_cfg[75]
	resetb=1,				# Reset ZCC demod.

	###########################
	### GPIO Mux Selections ###
	###########################
	GPO_row1_sel=[0,1,1,0], 
	GPO_row2_sel=[0]*4, 
	GPO_row3_sel=[0]*4, 
	GPO_row4_sel=[0]*4,
	GPI_row1_sel=[0]*2, 
	GPI_row2_sel=[0]*2, 
	GPI_row3_sel=[1]*2, 
	GPI_row4_sel=[0]*2,

	##########################
	### M0 Input Selection ###
	##########################
	cortex_clk_sel=0,	# 0: dbbclk, 1: DATA_CLK_IN
	cortex_data_sel=0,	# 0: dbbdata, 1: DATA_IN

	###################
	### LC IF Chain ###
	###################
	# Everything is MSB -> LSB
	# Mixer bias control. 
	# Thermometer, ones go on LHS. All 1s is max bias voltage. 
	mix_bias_In_ndac=[0]*4, 
	mix_bias_Ip_ndac=[0]*4, 
	mix_bias_Qn_ndac=[0]*4,
	mix_bias_Qp_ndac=[0]*4,

	# Binary. All 1s is the max bias voltage.
	mix_bias_In_pdac=[0]*4,
	mix_bias_Ip_pdac=[0]*4,
	mix_bias_Qn_pdac=[0]*4,
	mix_bias_Qp_pdac=[0]*4,
	
	mix_off_i=0,
	mix_off_q=0,
	
	# Q channel gain control
	Q_agc_gain_mode=0, 		# 0: scan chain, 1: AGC
	Q_code_scan=[1]*6, 
	Q_stg3_gm_tune=[1]*13, 	# Thermometer coding, ones go on LHS

	# Debug control at output of Q mixer
	Q_dbg_bias_en_Q=0,
	Q_dbg_out_en_Q=0,
	Q_dbg_input_en_Q=0,

	# Q channel TIA control
	Q_tia_cap_on=[0]*4, # all ones = lowest BW
	Q_tia_pulldown=1,	# 1: pull inputs to gnd
	Q_tia_enn=0,		# 1: enable NMOS bias
	Q_tia_enp=1,		# 1: enable PMOS bias

	# Q channel comparator offset trim. Binary.
	# Increase from 0 to add cap to either side of the comparator.
	Q_pctrl=[0]*5,
	Q_nctrl=[0]*5,

	# Q channel comparator control
	Q_mode_1bit=1,		# 1: zero-crossing mode
	Q_adc_comp_en=1,	# 0: disable clock for comparator

	# Q channel stage 3/2/3 amp control
	Q_stg3_amp_en=0,		# 1: Bias on.
	Q_dbg_out_on_stg3=0,	# 1: Turn on output pass gate for debug
	Q_dbg_input_on_stg3=0,	# 1: Turn on input pass gate for debug
	Q_dbg_bias_en_stg3=0,	# 1: Turn on bias for src followers

	Q_stg2_amp_en=0,
	Q_dbg_out_on_stg2=0,
	Q_dbg_input_on_stg2=0,
	Q_dbg_bias_en_stg2=0,	

	Q_stg1_amp_en=0,
	Q_dbg_out_on_stg1=0,
	Q_dbg_input_on_stg1=0,
	Q_dbg_bias_en_stg1=0,

	# Q channel ADC control
	Q_vcm_amp_en=0, 		# 1: Enable bias for Vcm buffer amp
	Q_vcm_vdiv_sel=[0]*2, 	# Switch cap divide ratio MSB->LSB, 00: ~400mV
	Q_vcm_clk_en=0,			# 1: Enable clock for Vcm SC divider
	Q_vref_amp_en=0, 		# 1: Enable bias for Vref buffer amp
	Q_vref_vdiv_sel=[1]*2,	# Switch cap divide ratio MSB-> LSB, 00: ~400mV
	Q_vref_clk_en=0,		# 1: Enable clock for Vref SC divider 
	Q_adc_fsm_en=0, 		# 1: Enable the ADC FSM
	Q_adc_dbg_en=0,			# 1: Enable the ADC debug outputs (bxp, compp)

	# Q channel filter
	# clk_en: 1 enables clock for IIR
	# C1/C2: all 1 maximizes cap.
	Q_stg3_clk_en=0,
	Q_stg3_C2=[0,0,1],	
	Q_stg3_C1=[0,0,1],
	Q_stg2_clk_en=0,
	Q_stg2_C2=[0,0,1],
	Q_stg2_C1=[0,0,1],
	Q_stg1_clk_en=0,
	Q_stg1_C2=[0,0,1],
	Q_stg1_C1=[0,0,1],

	# I channel gain control
	I_agc_gain_mode=0, 
	I_code_scan=[1]*6, 
	I_stg3_gm_tune=[1]*13,  # Thermometer coding, ones go on LHS

	# Debug control at output of I mixer
	I_dbg_bias_en_I=0,
	I_dbg_out_en_I=0,
	I_dbg_input_en_I=0,

	# I channel TIA control
	I_tia_cap_on=[0]*4,	# All ones = lowest BW
	I_tia_pulldown=1,	# 1: pull inputs to gnd
	I_tia_enn=0,		# 1: enable NMOS bias
	I_tia_enp=1,		# 1: enable PMOS bias

	# I channel comparator offset trim. Binary.
	# Increase from 0 to add cap to either side of the comparator.
	I_pctrl=[0]*5,
	I_nctrl=[0]*5,

	# I channel comparator control
	I_mode_1bit=1,		# 1: zero-crossing mode
	I_adc_comp_en=1,	# 0: disable clock for comparator

	# I channel stage 3/2/1 amp control
	I_stg3_amp_en=0,		# 1: Bias on.
	I_dbg_out_on_stg3=0,	# Turn on output pass gate for debug
	I_dbg_input_on_stg3=0,	# Turn on input pass gate for debug
	I_dbg_bias_en_stg3=0,	# Turn on bias for src followers

	I_stg2_amp_en=0,
	I_dbg_out_on_stg2=0,
	I_dbg_input_on_stg2=0,
	I_dbg_bias_en_stg2=0,	

	I_stg1_amp_en=0,
	I_dbg_out_on_stg1=0,
	I_dbg_input_on_stg1=0,
	I_dbg_bias_en_stg1=0,

	# I channel ADC control
	I_vcm_amp_en=0, 		# 1: Enable bias for Vcm buffer amp
	I_vcm_vdiv_sel=[0]*2, 	# Switch cap divide ratio MSB->LSB, 00: ~400mV
	I_vcm_clk_en=0,			# 1: Enable clock for Vcm SC divider
	I_vref_amp_en=0, 		# 1: Enable bias for Vref buffer amp
	I_vref_vdiv_sel=[1]*2,  # Switch cap divide ratio MSB-> LSB, 00: ~400mV
	I_vref_clk_en=0,		# 1: Enable clock for Vref SC divider 
	I_adc_fsm_en=0, 		# 1: Enable the ADC FSM
	I_adc_dbg_en=0,			# 1: Enable the ADC debug outputs (bxp, compp)

	# I channel filter
	# clk_en: 1 enables clock for IIR
	# C1/C2: all 1 maximizes cap.
	I_stg3_clk_en=0,
	I_stg3_C2=[0,0,1],
	I_stg3_C1=[0,0,1],
	I_stg2_clk_en=0,
	I_stg2_C2=[0,0,1],
	I_stg2_C1=[0,0,1],
	I_stg1_clk_en=0,
	I_stg1_C2=[0,0,1],
	I_stg1_C1=[0,0,1],

	# Clock generation control
	adc_dbg_en=0,		# 1: Enable output of debug phases for ADC clocks (cx, phix)
	adc_phi_en=0,		# 1: Enable clocks for ADC (cx, phix)
	filt_phi_en=0,		# 1: Enable 4 filter clk phses (0/90/180/270)
	clk_select=[0,1],	# Mux select for IF clock
						# 00: GND
						# 01: internal RC
						# 10: divided LC
						# 11: External pad
	filt_dbg_en=0,		# 1: Enable debug outputs for 4 filter clk phases (0/90/180/270)
	RC_clk_en=0,		# 1: Enable RC oscillator
	RC_coarse=[0]*5,	# Binary tuning for RC.
	RC_fine=[0]*5,		# Binary tuning for RC.

	# Miscellaneous IF
	if_ldo_rdac = [0]*7,
	if_por_disable=0,
	if_scan_reset=1,

	########################
	### Power On Control ###
	########################
	# 1 turns on LDO via scan chain
	scan_pon_if=0,
	scan_pon_lo=0, 
	scan_pon_pa=0,
	scan_pon_div=0,
	
	# 1 turns on LDO if pon signal from GPIO bank is high
	gpio_pon_en_if=0,
	gpio_pon_en_lo=0,
	gpio_pon_en_pa=0,
	gpio_pon_en_div=0,
	
	# 1 turns on the LDO if pon signal from radio FSM is high
	fsm_pon_en_if=0,
	fsm_pon_en_lo=0,
	fsm_pon_en_pa=0,
	fsm_pon_en_div=0,

	# 0 forces LDO off, 1 allows control from GPIO, FSM, or scan
	master_ldo_en_if=0,
	master_ldo_en_lo=0,
	master_ldo_en_pa=0,
	master_ldo_en_div=0,

	#########################
	### LC IF Signal Path ###
	#########################
	TIMER32k_enable=0,
	TIMER32k_counter_reset=0,	# TIMER32k counter. 0 is reset
	vddd_bgr_tune=[0]+[1]*7,
	por_bypass=0, # POR bypass in reset logic. 9: Not bypassed.
	alwayson_ldo_bgr_tune=[0]*6,

	# Aux LDO settings
	mux_select_aux_ldo_enable=0, # 0: Controlled by aux_ldo_enable (ASC)
								 # 1: Controlled by analog_cfg[167]
	aux_ldo_enable=0,
	aux_ldo_bgr_tune=[1]+[0]*6,


	# 20Mhz ring
	ring_20MHz_tune=[0]*9,	# Frequency and current tuning.
							# More info not given.
	ring_20MHz_enable=0,	# 1: Kick-start 20MHz ring.

	##################
	### Sensor ADC ###
	##################
	# ADC mux selections: 0 from digital FSM, 1 from GPI
	mux_sel_sensorADC_reset=0, 
	mux_sel_sensorADC_convert=0, 
	mux_sel_sensorADC_pga_amplify=0,

	sensorADC_pga_gain=[0]*8,		# Binary, (real gain setting)-1.
	sensorADC_settle=[0]*8,			# Bits for adjusting ADC settling time
	sensorADC_ldo_bgr_tune=[0]*7,	
	sensorADC_constgm_tune=[0]*8,	
	sensorADC_vbatDiv4_en=0,		
	sensorADC_ldo_en=0,				
	sensorADC_mux_sel=[0,0],	# 00: VPTAT
								# 01: VBAT/4
								# 10: External pad
								# 11: Floating

	###############################
	### LC Tuning + Transmitter ###
	###############################
	# LO frequency + current tuning: MSB -> LSB
	lo_fine_tune=[0]*6, 
	lo_mid_tune=[0]*6, 
	lo_coarse_tune=[0]*6, 
	lo_current_tune=[0]*8,
	lo_tune_select=1,	# 0: cortex, 1: scan chain
	polyphase_enable=1,	# 0: disable, 1: enable

	# LDO voltage tuning
	lo_panic=0,
	pa_panic=0, 
	div_panic=0,
	bg_panic=0,
	lo_ldo=[0]*6,
	pa_ldo=[0]*6, 
	div_ldo=[0]*6,

	# Test BG for visibility
	test_bg=[0]*6,

	# Modulation settings
	mod_logic=[0,1,1,0],	# s3: 1 inverts, 0 doesn't invert
							# s2: 0 for cortex, 1 for pad
							# s1/s0:
								# 00: pad
								# 01: cortex
								# 10: vdd
								# 11: gnd
	mod_15_4_tune=[1,0,1],
	mod_15_4_tune_d=0,		# spare dummy bit

	# Divider settings
	sel_1mhz_2mhz=1,		# 1: 2MHz, 0: 1MHz
	pre_dyn_dummy_b=0,		# Disconnected
	pre_dyn_b=[0]*6,		# Disconnected
	pre_dyn_en_b=0,			# Disconnected
	pre_2_backup_en=0,		# 1: Enables backup div-by-2 pre-scaler
	pre_5_backup_en=1,		# 1: Enables backup div-by-5 pre-scaler
	pre_dyn_sel=0,			# 1-4. Determines what pre_dyn will be.
							# pre_dyn 0's indicate disconnected,
							# otherwise it's connected to div-by-5 or div-by-2
	pre_dyn_dummy=0,		# Disconnected
	div_64mhz_enable=0,		# 1: Enable 64MHz clock source
	div_20mhz_enable=0,		# 1: Enable 20MHz clock source
	div_static_code=[0]*16,	# MSB -> LSB (flipping handled internally)
	div_static_rst_b=1, 	# 0: Reset static divider
	div_static_en=1,		# 1: Enable static divider
	dyn_div_N=[0]*13,		# Binary. Divide ratio.
	div_dynamic_en_b=1,		# 0: Enable dynamic divider.
	div_tune_select=1,		# 0: cortex, 1: scan
	source_select_2mhz=[0]*2,	# 00: static divider
								# 01: dynamic divider
								# 10: aux
								# 11: pad

	# 2MHz oscillator tuning and enable
	rc_2mhz_tune_coarse_1=[1]*5,
	rc_2mhz_tune_coarse_2=[1]*5,
	rc_2mhz_tune_coarse_3=[0,0,0,1,1],
	rc_2mhz_tune_fine=[0,1,1,1,1],
	rc_2mhz_tune_superfine=[0,0,1,1,0],
	rc_2mhz_enable=0,

	###########################
	### BLE Module Settings ###
	###########################
	scan_io_reset=1,
	scan_5mhz_select=[0,0],
	scan_1mhz_select=[0,0],
	scan_20mhz_select=[0,0],
	scan_async_bypass=0,
	scan_mod_bypass=1,
	scan_fine_trim=[1]*3+[0]*3,
	scan_data_in_valid=1,
	scan_ble_select=0,

	# Choosing clock source
	sel_ble_packetassembler_clk=0,		# BLE packet assembler clock source
										# 1: analog_cfg_ble[358]
										# 0: BLE_PDA_clk
	sel_ble_packetdisassembler_clk=0,	# BLE packet disassembler clock source
										# 1: BLE_PDA_clk
										# analog_cfg_ble[402]
	sel_ble_cdr_fifo_clk=[0,0],			# BLE CCDR FIFO clock source
										# 00: analog_cfg_ble[476]
										# 01: dbbRingclkin
										# 10: BLE_PDA_clk
										# 11: ADC_CLK_EXT
	sel_ble_arm_fifo_clk=0,				# BLE ARM FIFO clock source
										# 1: BLE_PDA_clk
										# 0: analog_cfg_ble[475]
	
	# Disables: 0 is enable
	div_symbol_clk_ble_dis=1,
	div_BLE_PDA_dis=1,
	div_EXT_CLK_GPIO2_dis=1,

	# Resets: 1 is reset
	div_symbol_clk_ble_reset=0,
	div_BLE_PDA_reset=0,
	div_EXT_CLK_GPIO2_reset=0,

	# Passthroughs: 1 is passthrough
	div_symbol_clk_ble_PT=0,
	div_BLE_PDA_PT=0,
	div_EXT_CLK_GPIO2_PT=0,

	# Divide values.
	div_symbol_clk_ble_Nin=[0]*8,
	div_BLE_PDA_Nin=[0]*8,
	div_EXT_CLK_GPIO2_Nin=[0]*8)