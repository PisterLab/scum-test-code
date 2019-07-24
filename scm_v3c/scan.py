def construct_scan(
	#############
	### NOTES ###
	#############
	# All things are MSB -> LSB unless otherwise noted. Any flips, etc. have
	# been handled internally so everyone can keep their sanity.
	# Everything _looks_ like it's off-by-one, but it isn't; there's
	# the zeroth element appended at the beginning!

	##############################
	### GPIO Direction Control ###
	##############################
	gpio_direction = [1]*16, # 0: signal to SCM, 1: signal from SCM

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

	cdr_clk_sel=0,	# CDR clock choice. 0: LC, 1: ring

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
	vddd_bgr_tune=[0]+[1]*6,
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

	# Divide values
	div_symbol_clk_ble_Nin=[0]*8,
	div_BLE_PDA_Nin=[0]*8,
	div_EXT_CLK_GPIO2_Nin=[0]*8):
	"""
	Inputs:
		See comments above.
	Outputs:
		Returns the constructed scan-chain bits with the appropriate inversions
		and reversals to be fed into the chip.
	"""

	ASC = [0]*1199

	ASC[0] 		= sel_data
	ASC[1:8] 	= sel_counter_resets
	ASC[8:14]	= counter_resets[1:]
	ASC[14:21]	= counter_enables
	ASC[21:23]	= sel_mux3in

	ASC[23] = div_RFTimer_enable
	ASC[24] = div_CortexM0_enable
	ASC[25] = div_GFSK_enable
	ASC[26] = div_ext_GPIO_enable
	ASC[27] = div_integ_enable
	ASC[28] = div_2MHz_enable
	ASC[29] = div_RFTimer_reset
	ASC[30] = div_CortexM0_reset
	ASC[31] = div_GFSK_reset
	ASC[32] = div_ext_GPIO_reset
	ASC[33] = div_integ_reset
	ASC[34] = div_2MHz_reset
	ASC[35] = div_RFTimer_PT
	ASC[36] = div_CortexM0_PT
	ASC[37] = div_GFSK_PT
	ASC[38] = div_ext_GPIO_PT
	ASC[39] = div_integ_PT
	ASC[40] = div_2MHz_PT

	div_RFTimer_Nin_inv 	= [int(1-x) for x in div_RFTimer_Nin]
	div_CortexM0_dec = int(''.join(map(str,div_CortexM0_Nin)), 2)

	if div_CortexM0_dec >=4:
		div_CortexM0_Nin_inv = [0]*8
	else:
		div_CortexM0_Nin_inv = [int(1-x) for x in div_CortexM0_Nin]

	div_GFSK_Nin_inv 		= [int(1-x) for x in div_GFSK_Nin]
	div_ext_GPIO_Nin_inv 	= [int(1-x) for x in div_ext_GPIO_Nin]
	div_integ_Nin_inv 		= [int(1-x) for x in div_integ_Nin]
	div_2MHz_Nin_inv 		= [int(1-x) for x in div_2MHz_Nin]

	ASC[41:49] = div_RFTimer_Nin_inv[::-1]
	ASC[49:57] = div_CortexM0_Nin_inv[::-1]
	ASC[57:65] = div_GFSK_Nin_inv[::-1]
	ASC[65:73] = div_ext_GPIO_Nin_inv[::-1]
	ASC[73:81] = div_integ_Nin_inv[::-1]
	ASC[81:89] = div_2MHz_Nin_inv[::-1]

	ASC[89] = mux_sel_RFTimer
	ASC[90] = mux_sel_GFSK_clk
	ASC[91:93] = mux3_sel_CLK2MHz
	ASC[93:95] = mux3_sel_CLK1MHz

	ASC[95:97] = IQ_select
	ASC[97:99] = op_mode
	ASC[99] = agc_overload
	ASC[100] = agc_ext_or_int
	ASC[101] = vga_select
	ASC[102] = mf_data_sign

	ASC[103:106] = mux_sel_gpio11

	ASC[106:122] 	= COUNT_THRESH
	ASC[122] 		= FB_EN
	ASC[123:131] 	= CLK_DIV_ext
	ASC[131] 		= DEMOD_EN
	ASC[132:143] 	= INIT_INTEG
	ASC[191] 		= sel_FREQ_CTRL_WORD
	ASC[192:208]	= SACLIENT_ext
	ASC[208:224] 	= EARLY_DECISION_MARGIN_ext
	ASC[224:226] 	= mux_sel_FREQ_CTRL_WORD_SCAN
	ASC[226:237]	= FREQ_CTRL_WORD_SCAN

	ASC[237]		= sel_dbb_ring_data_in
	ASC[238]		= sel_dbb_ring_clk_in
	ASC[239]		= sel_resetb
	ASC[240]		= resetb

	ASC[241] = mux_sel_sensorADC_reset
	ASC[242] = mux_sel_sensorADC_convert
	ASC[243] = mux_sel_sensorADC_pga_amplify

	ASC[244:248] = GPO_row1_sel
	ASC[248:252] = GPO_row2_sel
	ASC[252:256] = GPO_row3_sel
	ASC[256:260] = GPO_row4_sel
	ASC[260:262] = GPI_row1_sel
	ASC[262:264] = GPI_row2_sel
	ASC[264:266] = GPI_row3_sel
	ASC[266:268] = GPI_row4_sel

	ASC[268] = cortex_clk_sel
	ASC[269] = cortex_data_sel

	ASC[270] 		= Q_agc_gain_mode
	ASC[271:277] 	= Q_code_scan
	ASC[277:290] 	= Q_stg3_gm_tune
	ASC[290] 		= Q_dbg_bias_en_Q
	ASC[291] 		= Q_dbg_out_en_Q
	ASC[292] 		= Q_dbg_input_en_Q
	
	mix_bias_In_ndac_flip = [int(1-x) for x in mix_bias_In_ndac]
	mix_bias_Ip_ndac_flip = [int(1-x) for x in mix_bias_Ip_ndac]
	mix_bias_Qn_ndac_flip = [int(1-x) for x in mix_bias_Qn_ndac]
	mix_bias_Qp_ndac_flip = [int(1-x) for x in mix_bias_Qp_ndac]

	mix_bias_In_pdac_flip = [int(1-x) for x in mix_bias_In_pdac]
	mix_bias_Ip_pdac_flip = [int(1-x) for x in mix_bias_Ip_pdac]
	mix_bias_Qn_pdac_flip = [int(1-x) for x in mix_bias_Qn_pdac]
	mix_bias_Qp_pdac_flip = [int(1-x) for x in mix_bias_Qp_pdac]

	ASC[293:297] = mix_bias_In_ndac_flip
	ASC[297] = mix_off_i
	ASC[298:302] = mix_bias_Ip_ndac_flip
	ASC[302:306] = mix_bias_Qn_ndac_flip
	ASC[306] = mix_off_q
	ASC[307:311] = mix_bias_Qp_ndac_flip
	ASC[311:315] = mix_bias_In_pdac_flip
	ASC[315:319] = mix_bias_Ip_pdac_flip
	ASC[319:323] = mix_bias_Qn_pdac_flip
	ASC[323:327] = mix_bias_Qp_pdac_flip
	
	ASC[327:331] = Q_tia_cap_on
	ASC[331] = Q_tia_pulldown
	ASC[332] = Q_tia_enn
	ASC[333] = Q_tia_enp
	ASC[334:339] = Q_pctrl
	ASC[339:344] = Q_nctrl
	ASC[344] = Q_mode_1bit
	ASC[345] = Q_adc_comp_en
	ASC[346] = Q_stg3_amp_en
	ASC[347] = Q_dbg_out_on_stg3
	ASC[348] = Q_dbg_input_on_stg3
	ASC[349] = Q_dbg_bias_en_stg3
	ASC[350] = Q_stg2_amp_en
	ASC[351] = Q_dbg_out_on_stg2
	ASC[352] = Q_dbg_input_on_stg2
	ASC[353] = Q_dbg_bias_en_stg2
	ASC[354] = Q_stg1_amp_en
	ASC[355] = Q_dbg_out_on_stg1
	ASC[356] = Q_dbg_input_on_stg1
	ASC[357] = Q_dbg_bias_en_stg1

	ASC[358] = Q_vcm_amp_en
	ASC[359:361] = Q_vcm_vdiv_sel[::-1]
	ASC[361] = Q_vcm_clk_en
	ASC[362] = Q_vref_clk_en
	ASC[363:365] = Q_vref_vdiv_sel
	ASC[365] = Q_vref_amp_en
	ASC[366] = Q_adc_fsm_en
	ASC[367] = Q_adc_dbg_en

	ASC[368] = Q_stg3_clk_en
	ASC[369:372] = Q_stg3_C2
	ASC[372:375] = Q_stg3_C1
	ASC[375] = Q_stg2_clk_en
	ASC[376:379] = Q_stg2_C2
	ASC[379:382] = Q_stg2_C1
	ASC[382] = Q_stg1_clk_en
	ASC[383:386] = Q_stg1_C2
	ASC[386:389] = Q_stg1_C1
	
	ASC[389] = I_vcm_amp_en
	ASC[390:392] = I_vcm_vdiv_sel[::-1]
	ASC[392] = I_vcm_clk_en
	ASC[393] = I_vref_clk_en
	ASC[394:396] = I_vref_vdiv_sel
	ASC[396] = I_vref_amp_en
	ASC[397] = I_adc_fsm_en
	ASC[398] = I_adc_dbg_en

	ASC[399] = I_stg3_clk_en
	ASC[400:403] = I_stg3_C2
	ASC[403:406] = I_stg3_C1
	ASC[406] = I_stg2_clk_en
	ASC[407:410] = I_stg2_C2
	ASC[410:413] = I_stg2_C1
	ASC[413] = I_stg1_clk_en
	ASC[414:417] = I_stg1_C2
	ASC[417:420] = I_stg1_C1

	ASC[420] = adc_dbg_en
	ASC[421] =	adc_phi_en
	ASC[422] = filt_phi_en
	ASC[423:425] = clk_select
	ASC[425] = RC_clk_en
	ASC[426:431] = RC_coarse
	ASC[431] = filt_dbg_en
	ASC[432:437] = RC_fine
	
	ASC[437] = I_dbg_bias_en_stg1
	ASC[438] = I_dbg_input_on_stg1
	ASC[439] = I_dbg_out_on_stg1
	ASC[440] = I_stg1_amp_en
	ASC[441] = I_dbg_bias_en_stg2
	ASC[442] = I_dbg_input_on_stg2
	ASC[443] = I_dbg_out_on_stg2
	ASC[444] = I_stg2_amp_en
	ASC[445] = I_dbg_bias_en_stg3
	ASC[446] = I_dbg_input_on_stg3
	ASC[447] = I_dbg_out_on_stg3
	ASC[448] = I_stg3_amp_en

	ASC[449] = I_adc_comp_en
	ASC[450] = I_mode_1bit
	ASC[451:456] = I_nctrl[::-1]
	ASC[456:461] = I_pctrl[::-1]
	ASC[461] = I_tia_enp
	ASC[462] = I_tia_enn
	ASC[463] = I_tia_pulldown
	ASC[464:468] = I_tia_cap_on

	ASC[468] = I_dbg_input_en_I
	ASC[469] = I_dbg_out_en_I
	ASC[470] = I_dbg_bias_en_I
	ASC[471:484] = I_stg3_gm_tune[::-1]
	ASC[484:490] = I_code_scan[::-1]
	ASC[490] = I_agc_gain_mode

	ASC[491:498] = if_ldo_rdac[::-1]
	ASC[498] = if_por_disable
	ASC[499] = if_scan_reset

	ASC[500] = scan_pon_if
	ASC[501] = scan_pon_lo
	ASC[502] = scan_pon_pa
	ASC[503] = gpio_pon_en_if
	ASC[504] = fsm_pon_en_if
	ASC[505] = gpio_pon_en_lo
	ASC[506] = fsm_pon_en_lo
	ASC[507] = gpio_pon_en_pa
	ASC[508] = fsm_pon_en_pa
	ASC[509] = master_ldo_en_if
	ASC[510] = master_ldo_en_lo
	ASC[511] = master_ldo_en_pa
	ASC[512] = scan_pon_div
	ASC[513] = gpio_pon_en_div
	ASC[514] = fsm_pon_en_div
	ASC[515] = master_ldo_en_div

	ASC[622] = TIMER32k_enable
	ASC[757:765] = sensorADC_constgm_tune[::-1]
	ASC[765:771] = sensorADC_pga_gain[0:6]
	ASC[772] = sensorADC_pga_gain[7]
	ASC[777] = sensorADC_ldo_bgr_tune[0]
	ASC[778:784] = sensorADC_ldo_bgr_tune[1:][::-1]

	ASC[791:798] = vddd_bgr_tune
	ASC[798] = por_bypass
	ASC[799] = sensorADC_pga_gain[6]
	ASC[797] = sensorADC_vbatDiv4_en
	ASC[800] = sensorADC_ldo_en
	ASC[815:823] = sensorADC_settle
	ASC[913] = mux_select_aux_ldo_enable
	ASC[914] = sensorADC_mux_sel[0]
	ASC[915] = aux_ldo_enable
	ASC[916:923] = aux_ldo_bgr_tune[::-1]
	ASC[923:929] = alwayson_ldo_bgr_tune
	ASC[931:940] = ring_20MHz_tune
	ASC[940] = ring_20MHz_enable
	
	ASC[1086] = sensorADC_mux_sel[1]
	ASC[1087] = sensorADC_pga_bypas

	ASC[945:951] = lo_fine_tune
	ASC[951:957] = lo_mid_tune
	ASC[957:963] = lo_coarse_tune
	ASC[963] = lo_tune_select
	ASC[964:970] = test_bg
	ASC[970] = polyphase_enable
	ASC[971] = pa_panic
	ASC[972:978] = pa_ldo
	
	ASC[979] = lo_panic
	ASC[980:986] = lo_ldo
	ASC[987:995] = lo_current_tune
	ASC[995:999] = mod_logic
	ASC[999:1002] = mod_15_4_tune[::-1]
	ASC[1002] = mod_15_4_tune_d
	ASC[1003] = div_panic
	ASC[1004:1010] = div_ldo
	
	if pre_dyn_sel == 0:
		pre_dyn = [1,1,1,0,0,0]
	elif pre_dyn_sel == 1:
		pre_dyn = [0,1,1,0,0,0]
	elif pre_dyn_sel == 2:
		pre_dyn = [1,0,1,0,0,0]
	elif pre_dyn_sel == 3:
		pre_dyn = [1,1,0,0,0,0]
	else:
		pre_dyn = [1,1,1,0,0,0]

	div_static_code_flip = div_static_code[::-1]

	div_static_select_inv = div_static_code_flip[10:16] + \
						div_static_code_flip[4:10] + \
						[div_static_en, div_static_rst_b] + \
						div_static_code_flip[0:4]

	div_static_select = [int(1-x) for x in div_static_select_inv]

	div_dynamic_select = dyn_div_N[::-1] + [div_dynamic_en_b]
	
	ASC[1011] = sel_1mhz_2mhz
	ASC[1012] = pre_dyn_dummy_b
	ASC[1013:1019] = pre_dyn_b
	ASC[1019] = pre_dyn_en_b
	ASC[1020] = 1-pre_2_backup_en
	ASC[1021] = pre_2_backup_en
	ASC[1022] = pre_5_backup_en
	ASC[1023] = 1-pre_5_backup_en
	ASC[1024:1030] = pre_dyn
	ASC[1030] = pre_dyn_dummy
	ASC[1031] = div_64mhz_enable
	ASC[1032] = div_20mhz_enable
	ASC[1033] = scan_io_reset
	ASC[1034:1036] = scan_5mhz_select
	ASC[1036:1038] = scan_1mhz_select
	ASC[1038:1040] = scan_20mhz_select
	ASC[1040] = scan_async_bypass
	ASC[1041] = scan_mod_bypass
	ASC[1042:1048] = scan_fine_trim
	ASC[1048:1066] = div_static_select
	ASC[1066:1080] = div_dynamic_select
	ASC[1080] = div_tune_select
	ASC[1081] = bg_panic
	ASC[1082:1084] = source_select_2mhz
	ASC[1084] = scan_data_in_valid
	ASC[1085] = scan_ble_select
	ASC[1088:1093] = rc_2mhz_tune_coarse_1[::-1]
	ASC[1093:1098] = rc_2mhz_tune_coarse_2[::-1] 
	ASC[1098:1103] = rc_2mhz_tune_coarse_3[::-1]
	ASC[1103:1108] = rc_2mhz_tune_fine[::-1]
	ASC[1108:1113] = rc_2mhz_tune_superfine[::-1]
	ASC[1113] = rc_2mhz_enable

	ASC[1150:1154] = crossbar_RFTimer[::-1]
	ASC[1154:1158] = crossbar_TX_chip_clk_to_cortex[::-1]
	ASC[1158:1162] = crossbar_symbol_clk_ble[::-1]
	ASC[1162:1166] = crossbar_divider_out_INTEG[::-1]
	ASC[1166:1170] = crossbar_GFSK_CLK[::-1]
	ASC[1170:1174] = crossbar_EXT_CLK_GPIO[::-1]
	ASC[1174:1178] = crossbar_EXT_CLK_GPIO2[::-1]
	ASC[1178:1182] = crossbar_BLE_PDA[::-1]

	ASC[516] = div_symbol_clk_ble_dis
	ASC[517] = div_BLE_PDA_dis
	ASC[518] = div_EXT_CLK_GPIO2_dis
	ASC[519] = div_symbol_clk_ble_reset
	ASC[520] = div_BLE_PDA_reset
	ASC[521] = div_EXT_CLK_GPIO2_reset

	ASC[522] = div_symbol_clk_ble_PT
	ASC[523] = div_BLE_PDA_PT
	ASC[524] = div_EXT_CLK_GPIO2_PT

	ASC[525:533] = div_symbol_clk_ble_Nin[::-1]
	ASC[533:541] = div_BLE_PDA_Nin[::-1]
	ASC[541:549] = div_EXT_CLK_GPIO2_Nin[::-1]

	ASC[549] = sel_ble_packetassembler_clk
	ASC[550] = sel_ble_packetdisassembler_clk
	ASC[551] 	= counter_resets[0]
	ASC[553:555] = sel_ble_cdr_fifo_clk[::-1]
	ASC[555] = sel_ble_arm_fifo_clk

	out_mask = gpio_direction
	in_mask = [int(1-x) for x in gpio_direction]

	ASC[1130] = 1-out_mask[0]
	ASC[1131] = in_mask[0]
	ASC[1132] = 1-out_mask[1]
	ASC[1133] = in_mask[1]
	ASC[1134] = 1-out_mask[2]
	ASC[1135] = in_mask[2]
	ASC[1136] = 1-out_mask[3]
	ASC[1137] = in_mask[3]
	ASC[1138] = in_mask[4]
	ASC[1139] = 1-out_mask[4]
	ASC[1140] = in_mask[5]
	ASC[1141] = 1-out_mask[5]
	ASC[1142] = in_mask[6]
	ASC[1143] = 1-out_mask[6]
	ASC[1144] = in_mask[7]
	ASC[1145] = 1-out_mask[7]
	ASC[1114] = 1-out_mask[8]
	ASC[1115] = in_mask[8]
	ASC[1116] = 1-out_mask[9]
	ASC[1117] = in_mask[9]
	ASC[1118] = 1-out_mask[10]
	ASC[1119] = in_mask[10]
	ASC[1120] = 1-out_mask[11]
	ASC[1121] = in_mask[11]
	ASC[1122] = in_mask[12]
	ASC[1123] = 1-out_mask[12]
	ASC[1124] = in_mask[13]
	ASC[1125] = 1-out_mask[13]
	ASC[1126] = in_mask[14]
	ASC[1127] = 1-out_mask[14]
	ASC[1128] = in_mask[15]
	ASC[1129] = 1-out_mask[15]

	ASC = [cdr_clk_sel] + ASC

	return ASC

def program_scan(
		scan_settings, com_port='COM10', 
		vdda_pcb_tune=[0]*4, vddd_pcb_tune=[0]*4,
		gpio_direction=[1]*16,
		vbat_ldo_enable=0, gpio_ldo_enable=0,  debug_ldo_enable=0,
		vdda_pcb_ldo_enable=0, vddd_pcb_ldo_enable=0):
	"""
	Inputs:
		scan_settings: Dictionary of parameters for 'construct_scan'. Keys 
			should be parameter name, and the value should be the intended 
			value. See documentation for 'construct_scan' for list of 
			parameters and default values.
		com_port: String. Name of the port through which to open the serial
			connection.
		vdda_pcb_tune: List of 4 zeros or ones (integer). TODO: Unknown 
			encoding scheme.
		vddd_pcb_tune: List of 4 zeros or ones (integer). TODO: Unknown 
			encoding scheme.
		gpio_direction: List of 16 zeros or ones (integer). The leftmost 
			element is associated with GPIO00, and the rightmost element is
			associated with GPIO15. 0 indicates a signal to SCM, and 1 
			indicates a signal from SCM.
		vbat_ldo_enable: Integer 0 or 1, with 1 enabling the PCB LDO for VBAT 
			and 0 disabling it.
		gpio_ldo_enable: Integer 0 or 1, with 1 enabling the PCB LDO for
			the GPIOs and 0 disabling it.
		debug_ldo_enable: Integer 0 or 1, with 1 enabling the PCB LDO for
			the debug and 0 disabling it.
		vdda_pcb_ldo_enable: Integer 0 or 1, with 1 enabling the PCB LDO for
			VDDA and 0 disabling it.
		vddd_pcb_ldo_enable: Integer 0 or 1, with 1 enabling the PCB LDO for 
			VDDD and 0 disabling it.
	Outputs:
		No return value. Programs scan with the parameters in 'scan_settings'
		and transmits the PCB configuration using COM port 'com_port'. 
	"""
	ser = serial.Serial(
		port=com_port,
		baudrate=19200,
		parity=serial.PARITY_NONE,
		stopbits=serial.STOPBITS_ONE,
		bytesize=serial.EIGHTBITS)

	# Transmit PCB config to microcontroller
	ldo_enables = [0]*3 + [vbat_ldo_enable, gpio_ldo_enable, debug_ldo_enable, \
						vdda_pcb_ldo_enable, vddd_pcb_ldo_enable]
	vdd_tune = vdda_pcb_tune = vddd_pcb_tune

	ser.write(b'boardconfig\n')
	ser.write((str(int(''.join(map(str,vdd_tune)),2))+'\n').encode())
	ser.write((str(int(''.join(map(str,gpio_direction)),2))+'\n').encode())
	ser.write((str(int(''.join(map(str,ldo_enables)),2))+'\n').encode())

	# Pause and turn on the clock
	time.sleep(0.1)
	ser.write(b'clockon\n')

	# Send ASC string to uC and program to chip
	ASC = construct_scan(**scan_settings)
	ASC_str = ''.join(map(str,ASC))
	ser.write(b'ascwrite\n')
	print(ser.readline())
	ser.write(ASC_str.encode())
	print(ser.readline())
	
	# Execute the load command to latch values in the chip
	ser.write(b'ascload\n')
	print(ser.readline())

	# Read back the scan chain contents
	ser.write('ascread\n')
	scan_out_rev = ser.readline()
	scan_out = scan_out_rev[::-1]
	print("Scan In: {}".format(ASC_str))
	print("Scan Out: {}".format(scan_out.decode()))

	# Check if scan in matches scan out
	if int(ASC_string) == int(scan_out.decode()):
		print("Read Matches Write")
	else:
		print("Read/Write Comparison Incorrect")

	# Due diligence of closing the serial
	ser.close()
	return

#################################################
#################################################
################## Main Method ################## 
#################################################
#################################################

if __name__ == "__main__":
	bleh = construct_scan()
