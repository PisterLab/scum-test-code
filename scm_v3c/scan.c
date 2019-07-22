#include <stdio.h>
#include <string.h>

int construct_scan() {
	/* START: DO NOT CHANGE THESE */

	// I SWEAR TO ALL THINGS HOLY AND GOOD
	// IF YOU CHANGE ANYTHING IN THIS SECTION
	// I WILL VERSION CONTROL YOU INTO OBLIVION

	/********************************/
	/**** GPIO Direction Control ****/
	/********************************/
	// 0: signal to SCM, 1: signal from SCM
	int gpio_direction[16] = {1,1,1,1, 
							1,1,1,1, 
							1,1,1,1, 
							1,1,1,1};	

	/***********************/
	/**** Miscellaneous ****/
	/***********************/
	// Mux select for 7 counters output to GPIO bank 11
	int mux_sel_gpio11[3] = {0,0,0};// 000: analog_rdata[31:0]
									// 001: analog_rdata[63:32]
									// 010: analog_rdata[95:64]
									// 011: analog_rdata[127:96]
									// 100: analog_rdata[159:128]
									// 101: analog_rdata[191:160]
									// 110: analog_rdata[223:192]
									// 111: 32'b0
	/**************************/
	/**** Counter Settings ****/
	/**************************/
	// counter0: TIMER32k, 0 is reset.
	// counter1: LF_CLOCK, 0 is reset.
	// counter2: HF_CLOCK, 0 is reset.
	// counter3: RC_2MHz, 0 is reset.
	// counter4: LF_ext_GPIO, 0 is reset.
	// counter5: LC_div_N, 0 is reset.
	// counter6: ADC_CLK, 0 is reset.
	int sel_data[1] = {1};	// rrecovered data. 1 for ZCC, 0 for other baseband.
	int sel_counter_resets[7] = {1,1,1,1,1,1,1};	// Select signal for
													// counters [0,...,6]
													// 0: ASC[8,...,14]
													// 1: M0 analog_cfg[0...6]
	int counter_resets[7] = {0,0,0,0,0,0,0};	// 0: reset
	int counter_enables = {0,0,0,0,0,0,0};		// 1: enable

	/**************************/
	/**** Divider Settings ****/
	/**************************/
	// Enables: 1 is enable
	int div_RFTimer_enable[1] = {0};
	int div_CortexM0_enable[1] = {0};
	int div_GFSK_enable[1] = {0};
	int div_ext_GPIO_enable[1] = {0};
	int div_integ_enable[1] = {0}; 
	int div_2MHz_enable[1] = {0};

	// Resets: 1 is reset
	int div_RFTimer_reset[1] = {0};
	int div_CortexM0_reset[1] = {0};
	int div_GFSK_reset[1] = {0};
	int div_ext_GPIO_reset[1] = {0};
	int div_integ_reset[1] = {0};
	int div_2MHz_reset[1] = {0};

	// Passthroughs: 1 is passthrough
	int div_RFTimer_PT[1] = {0};
	int div_CortexM0_PT[1] = {0};
	int div_GFSK_PT[1] = {0};
	int div_ext_GPIO_PT[1] = {0};
	int div_integ_PT[1] = {0};
	int div_2MHz_PT[1] = {0};

	// Divide vlaues: Binary. Don't invert these! It's handled.
	int div_RFTimer_Nin[8] = {0,0,0,0,0,0,0,0};
	int div_CortexM0_Nin[8] = {0,0,0,0,0,0,0,0};
	int div_GFSK_Nin[8] = {0,0,0,0,0,0,0,0};
	int div_ext_GPIO_Nin[8] = {0,0,0,0,0,0,0,0};
	int div_integ_Nin[8] = {0,0,0,0,0,0,0,0};
	int div_2MHz_Nin[8] = {0,0,0,0,0,0,0,0};

	/******************************/
	/**** Clock Mux & Crossbar ****/
	/******************************/
	int mux_sel_RFTimer[1] = {0};			// 0: divider_out_RFTimer; 1: TIMER32k
	int mux_sel_GFSK_clk[1] = {0}; 			// 0 is LC_div_N; 1 is divider_out_GFSK
	int mux3_sel_CLK2MHz[2] = {0,0}; 	// 00: RC_2MHz
										// 01: divider_out_2MHz
										// 10: LC_2MHz
										// 11: 1'b0
	int mux3_sel_CLK1MHz[2] = {0,0};		// 00: LC_1MHz_dyn
								// 01: divider_out_2MHz
								// 10: LC_1MHz_stat
								// 11: 1'b0
	int sel_mux3in[2] = [0,0];	// 00: LF_CLOCK
								// 01: HF_CLOCK
								// 10: LF_ext_PAD
								// 11: 1'b0

	// Crossbar Settings
	// 00: LF_CLOCK
	// 01: HF_CLOCK
	// 02: RC_2MHz
	// 03: TIMER32k
	// 04: LF_ext_PAD
	// 05: LF_ext_GPIO
	// 06: ADC_CLK
	// 07: LC_div_N
	// 08: LC_2MHz
	// 09: LC_1MHz_stat
	// 10: LC_1MHz_dyn
	// 11: 1'b0
	// 12: 1'b0
	// 13: 1'b0
	// 14: 1'b0
	// 15: 1'b0
	int crossbar_HCLK[4] = {0,0,0,0};
	int crossbar_RFTimer[4] = {0,0,0,0};
	int crossbar_TX_chip_clk_to_cortex[4] = {0,0,0,0};
	int crossbar_symbol_clk_ble[4] = {0,0,0,0};
	int crossbar_divider_out_INTEG[4] = {0,0,0,0};
	int crossbar_GFSK_CLK[4] = {0,0,0,0};
	int crossbar_EXT_CLK_GPIO[4] = {0,0,0,0};
	int crossbar_EXT_CLK_GPIO2[4] = {0,0,0,0};
	int crossbar_BLE_PDA[4] = {0,0,0,0};

	int cdr_clk_sel[1] = {0};	// CDR clock choice. 0: LC; 1: ring

	/********************************/
	/**** Bob's Digital Baseband ****/
	/********************************/
	int IQ_select[2] = {0,0};
	int op_mode[2] = {0,0};
	int agc_overload[1] = {0};
	int agc_ext_or_int[1] = {0};
	int vga_select[1] = {0};
	int mf_data_sign[1] = {0};

	/**********************************/
	/**** Brian's Digital Baseband ****/
	/**********************************/
	// Vaguely labeled things called 'Brian's DBB
	int COUNT_THRESH[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 1,1,1,0};
	int FB_EN[1] = {0};
	int CLK_DIV_ext[8] = {0,0,0,0, 0,0,0,0};
	int DEMOD_EN[1] = {0};
	int INIT_INTEG[11] = {0,0,0, 0,0,0,0, 0,0,0,0};
	int sel_FREQ_CTRL_WORD[1] = {0};
	int SACLIENT_ext[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};
	int EARLY_DECISION_MARGIN_ext[16] = {0,0,0,0,0,0,0,0,0,1,0,0,0,1,0,0};
	int mux_sel_FREQ_CTRL_WORD_SCAN[2] = {0,0};	// 00: ASC[237:227]
												// 01: FREQ_CTRL_WORD_SCAN
												// 10: analog_cfg[74:64]
												// 11: 11'b0
	int FREQ_CTRL_WORD_SCAN = {0,0,0, 0,0,0,0, 0,0,0,0};

	// DBB settings
	int sel_dbb_ring_data_in[1] = {1};	// 0: LC osc, Q branch (SIG_IN)
									// 1: LC Osc, I branch (COMPP_ZCC)
	int sel_dbb_ring_clk_in[1] = {1};	// 0: LC osc, Q branch (ZCC_CLK_Ring)
									// 1: LC osc, I branch (ZCC_CLK)
	int sel_resetb[1] = {1};				// 0: ASC[241], 1: analog_cfg[75]
	int resetb[1] = {1};					// Reset ZCC demod

	/*****************************/
	/**** GPIO Mux Selections ****/
	/*****************************/
	int GPO_row1_sel[4] = {0,1,1,0}; 
	int GPO_row2_sel[4] = {0,0,0,0};
	int GPO_row3_sel[4] = {0,0,0,0}; 
	int GPO_row4_sel[4] = {0,0,0,0};
	int GPI_row1_sel[2] = {0,0}; 
	int GPI_row2_sel[2] = {0,0}; 
	int GPI_row3_sel[2] = {1,1}; 
	int GPI_row4_sel[2] = {0,0};

	/****************************/
	/**** M0 Input Selection ****/
	/****************************/
	int cortex_clk_sel[1] = {0};
	int cortex_data_sel[1] = {0};

	/*********************/
	/**** LC IF Chain ****/
	/*********************/
	// Everything is MSB -> LSB
	// Mixer bias control. 
	// Thermometer. Ones go on LHS. All 1s is max bias voltage. 
	int mix_bias_In_ndac[4] = {0,0,0,0};
	int mix_bias_Ip_ndac[4] = {0,0,0,0};
	int mix_bias_Qn_ndac[4] = {0,0,0,0};
	int mix_bias_Qp_ndac[4] = {0,0,0,0};

	// Binary. All 1s is the max bias voltage.
	int mix_bias_In_pdac[4] = {0,0,0,0};
	int mix_bias_Ip_pdac[4] = {0,0,0,0};
	int mix_bias_Qn_pdac[4] = {0,0,0,0};
	int mix_bias_Qp_pdac[4] = {0,0,0,0};
	
	int mix_off_i[1] = {0};
	int mix_off_q[1] = {0};
	
	// Q channel gain control
	int Q_agc_gain_mode[1] = {0};		// 0: scan chain, 1: AGC
	int Q_code_scan[6] = {1,1, 1,1,1,1};
	int Q_stg3_gm_tune[13] = {1, 1,1,1,1, 1,1,1,1, 1,1,1,1};	// Thermometer coding; ones go on LHS

	// Debug control at output of Q mixer
	int Q_dbg_bias_en_Q[1] = {0};
	int Q_dbg_out_en_Q[1] = {0};
	int Q_dbg_input_en_Q[1] = {0};

	// Q channel TIA control
	int Q_tia_cap_on[4] = {0,0,0,0};	// all ones = lowest BW
	int Q_tia_pulldown[1] = {1};				// 1: pull inputs to gnd
	int Q_tia_enn[1] = {0};					// 1: enable NMOS bias
	int Q_tia_enp[1] = {1};					// 1: enable PMOS bias

	// Q channel comparator offset trim. Binary.
	// Increase from 0 to add cap to either side of the comparator.
	int Q_pctrl[5] = {0,0,0,0,0};
	int Q_nctrl[5] = {0,0,0,0,0};

	// Q channel comparator control
	int Q_mode_1bit[1] = {1};		// 1: zero-crossing mode
	int Q_adc_comp_en[1] = {1};	// 0: disable clock for comparator

	// Q channel stage 3/2/3 amp control
	int Q_stg3_amp_en[1] = {0};		// 1: Bias on.
	int Q_dbg_out_on_stg3[1] = {0};	// 1: Turn on output pass gate for debug
	int Q_dbg_input_on_stg3[1] = {0};	// 1: Turn on input pass gate for debug
	int Q_dbg_bias_en_stg3[1] = {0};	// 1: Turn on bias for src followers

	int Q_stg2_amp_en[1] = {0};
	int Q_dbg_out_on_stg2[1] = {0};
	int Q_dbg_input_on_stg2[1] = {0};
	int Q_dbg_bias_en_stg2[1] = {0};	

	int Q_stg1_amp_en[1] = {0};
	int Q_dbg_out_on_stg1[1] = {0};
	int Q_dbg_input_on_stg1[1] = {0};
	int Q_dbg_bias_en_stg1[1] = {0};

	// Q channel ADC control
	int Q_vcm_amp_en[1] = {0};				// 1: Enable bias for Vcm buffer amp
	int Q_vcm_vdiv_sel[2] = {0,0};	// Switch cap divide ratio MSB->LSB;00: ~400mV
	int Q_vcm_clk_en[1] = {0};			// 1: Enable clock for Vcm SC divider
	int Q_vref_amp_en[1] = {0};			// 1: Enable bias for Vref buffer amp
	int Q_vref_vdiv_sel[2] = {1,1};	// Switch cap divide ratio MSB-> LSB;00: ~400mV
	int Q_vref_clk_en[1] = {0};			// 1: Enable clock for Vref SC divider 
	int Q_adc_fsm_en[1] = {0};			// 1: Enable the ADC FSM
	int Q_adc_dbg_en[1] = {0};			// 1: Enable the ADC debug outputs (bxp;compp)

	// Q channel filter
	// clk_en: 1 enables clock for IIR
	// C1/C2: all 1 maximizes cap.
	int Q_stg3_clk_en[1] = {0};
	int Q_stg3_C2[3] = {0,0,1};	
	int Q_stg3_C1[3] = {0,0,1};
	int Q_stg2_clk_en[1] = {0};
	int Q_stg2_C2[3] = {0,0,1};
	int Q_stg2_C1[3] = {0,0,1};
	int Q_stg1_clk_en[1] = {0};
	int Q_stg1_C2[3] = {0,0,1};
	int Q_stg1_C1[3] = {0,0,1};

	// I channel gain control
	int I_agc_gain_mode[1] = {0};
	int I_code_scan[6] = {1,1, 1,1,1,1};
	int I_stg3_gm_tune[13] = {1, 1,1,1,1, 1,1,1,1, 1,1,1,1}; // Thermometer coding;ones go on LHS

	// Debug control at output of I mixer
	int I_dbg_bias_en_I[1] = {0};
	int I_dbg_out_en_I[1] = {0};
	int I_dbg_input_en_I[1] = {0};

	// I channel TIA control
	int I_tia_cap_on[4] = {0,0,0,0};	// All ones = lowest BW
	int I_tia_pulldown[1] = {1};	// 1: pull inputs to gnd
	int I_tia_enn[1] = {0};		// 1: enable NMOS bias
	int I_tia_enp[1] = {1};		// 1: enable PMOS bias

	// I channel comparator offset trim. Binary.
	// Increase from 0 to add cap to either side of the comparator.
	int I_pctrl[5] = {0,0,0,0,0};
	int I_nctrl[5] = {0,0,0,0,0};

	// I channel comparator control
	int I_mode_1bit[1] = {1};		// 1: zero-crossing mode
	int I_adc_comp_en[1] = {1};	// 0: disable clock for comparator

	// I channel stage 3/2/1 amp control
	int I_stg3_amp_en[1] = {0};		// 1: Bias on.
	int I_dbg_out_on_stg3[1] = {0};	// Turn on output pass gate for debug
	int I_dbg_input_on_stg3[1] = {0};	// Turn on input pass gate for debug
	int I_dbg_bias_en_stg3[1] = {0};	// Turn on bias for src followers

	int I_stg2_amp_en[1] = {0};
	int I_dbg_out_on_stg2[1] = {0};
	int I_dbg_input_on_stg2[1] = {0};
	int I_dbg_bias_en_stg2[1] = {0};	

	int I_stg1_amp_en[1] = {0};
	int I_dbg_out_on_stg1[1] = {0};
	int I_dbg_input_on_stg1[1] = {0};
	int I_dbg_bias_en_stg1[1] = {0};

	// I channel ADC control
	int I_vcm_amp_en[1] = {0};			// 1: Enable bias for Vcm buffer amp
	int I_vcm_vdiv_sel[2] = {0,0};	// Switch cap divide ratio MSB->LSB;00: ~400mV
	int I_vcm_clk_en[1] = {0};			// 1: Enable clock for Vcm SC divider
	int I_vref_amp_en[1] = {0};			// 1: Enable bias for Vref buffer amp
	int I_vref_vdiv_sel[2] = {1,1}; // Switch cap divide ratio MSB-> LSB;00: ~400mV
	int I_vref_clk_en[1] = {0};			// 1: Enable clock for Vref SC divider 
	int I_adc_fsm_en[1] = {0};			// 1: Enable the ADC FSM
	int I_adc_dbg_en[1] = {0};			// 1: Enable the ADC debug outputs (bxp;compp)

	// I channel filter
	// clk_en: 1 enables clock for IIR
	// C1/C2: all 1 maximizes cap.
	int I_stg3_clk_en[1] = {0};
	int I_stg3_C2 = {0,0,1};
	int I_stg3_C1 = {0,0,1};
	int I_stg2_clk_en[1] = {0};
	int I_stg2_C2 = {0,0,1};
	int I_stg2_C1 = {0,0,1};
	int I_stg1_clk_en[1] = {0};
	int I_stg1_C2 = {0,0,1};
	int I_stg1_C1 = {0,0,1};

	// Clock generation control
	int adc_dbg_en[1] = {0};			// 1: Enable output of debug phases for ADC clocks (cx;phix)
	int adc_phi_en[1] = {0};			// 1: Enable clocks for ADC (cx;phix)
	int filt_phi_en[1] = {0};		// 1: Enable 4 filter clk phses (0/90/180/270)
	int clk_select[2] = {0,1};	// Mux select for IF clock
								// 00: GND
								// 01: internal RC
								// 10: divided LC
								// 11: External pad
	int filt_dbg_en[1] = {0};		// 1: Enable debug outputs for 4 filter clk phases (0/90/180/270)
	int RC_clk_en[1] = {0};			// 1: Enable RC oscillator
	int RC_coarse[5] = {0,0,0,0,0};	// Binary tuning for RC.
	int RC_fine[5] = {0,0,0,0,0};	// Binary tuning for RC.

	// Miscellaneous IF
	int if_ldo_rdac[7] = {0,0,0,0,0,0,0};
	int if_por_disable[1] = {0};
	int if_scan_reset[1] = {1};

	/**************************/
	/**** Power On Control ****/
	/**************************/
	// 1 turns on LDO via scan chain
	int scan_pon_if[1] = {0};
	int scan_pon_lo[1] = {0}; 
	int scan_pon_pa[1] = {0};
	int scan_pon_div[1] = {0};
	
	// 1 turns on LDO if pon signal from GPIO bank is high
	int gpio_pon_en_if[1] = {0};
	int gpio_pon_en_lo[1] = {0};
	int gpio_pon_en_pa[1] = {0};
	int gpio_pon_en_div[1] = {0};
	
	// 1 turns on the LDO if pon signal from radio FSM is high
	int fsm_pon_en_if[1] = {0};
	int fsm_pon_en_lo[1] = {0};
	int fsm_pon_en_pa[1] = {0};
	int fsm_pon_en_div[1] = {0};

	// 0 forces LDO off, 1 allows control from GPIO, FSM, or scan
	int master_ldo_en_if[1] = {0};
	int master_ldo_en_lo[1] = {0};
	int master_ldo_en_pa[1] = {0};
	int master_ldo_en_div[1] = {0};

	/***************************/
	/**** LC IF Signal Path ****/
	/***************************/
	int TIMER32k_enable[1] = {0};
	int TIMER32k_counter_reset[1] = {0};	// TIMER32k counter. 0 is reset
	int vddd_bgr_tune = {0,1, 1,1,1,1};
	int por_bypass[1] = {0}; // POR bypass in reset logic. 9: Not bypassed.
	int alwayson_ldo_bgr_tune = {0,0, 0,0,0,0};

	// Aux LDO settings
	int mux_select_aux_ldo_enable[1] = {0}; // 0: Controlled by aux_ldo_enable (ASC)
								 // 1: Controlled by analog_cfg[167]
	int aux_ldo_enable[1] = {0};
	int aux_ldo_bgr_tune[7] = {1,0,0, 0,0,0,0};


	// 20Mhz ring
	int ring_20MHz_tune[9] = {0, 0,0,0,0, 0,0,0,0};	// Frequency and current tuning
							// More info not given.
	int ring_20MHz_enable[1] = {0};	// 1: Kick-start 20MHz ring.

	/********************/
	/**** Sensor ADC ****/
	/********************/
	// ADC mux selections: 0 from digital FSM, 1 from GPI
	int mux_sel_sensorADC_reset[1] = {0}; 
	int mux_sel_sensorADC_convert[1] = {0}; 
	int mux_sel_sensorADC_pga_amplify[1] = {0};

	int sensorADC_pga_gain[8] = {0,0,0,0, 0,0,0,0};	// Binary, (real gain setting)-1.
	int sensorADC_settle[8] = {0,0,0,0, 0,0,0,0};	// Bits for adjusting ADC settling time
	int sensorADC_ldo_bgr_tune[7] = {0,0,0, 0,0,0,0};	
	int sensorADC_constgm_tune[8] = {0,0,0,0, 0,0,0,0};	
	int sensorADC_vbatDiv4_en[1] = {0};
	int sensorADC_ldo_en[1] = {0};
	int sensorADC_mux_sel[2] = {0,0};	// 00: VPTAT
									// 01: VBAT/4
									// 10: External pad
									// 11: Floating

	/*********************************/
	/**** LC Tuning + Transmitter ****/
	/*********************************/
	// LO frequency + current tuning: MSB -> LSB
	int lo_fine_tune[6] = {0,0, 0,0,0,0}; 
	int lo_mid_tune[6] = {0,0, 0,0,0,0}; 
	int lo_coarse_tune[6] = {0,0, 0,0,0,0}; 
	int lo_current_tune[8] = {0,0,0,0, 0,0,0,0};
	int lo_tune_select[1] = {1};	// 0: cortex, 1: scan chain
	int polyphase_enable[1] = {1};	// 0: disable, 1: enable

	// LDO voltage tuning
	int lo_panic[1] = {0};
	int pa_panic[1] = {0}; 
	int div_panic[1] = {0};
	int bg_panic[1] = {0};
	int lo_ldo[6] = {0,0, 0,0,0,0};
	int pa_ldo[6] = {0,0, 0,0,0,0}; 
	int div_ldo[6] = {0,0, 0,0,0,0};

	// Test BG for visibility
	int test_bg[6] = {0,0, 0,0,0,0};

	// Modulation settings
	int mod_logic[4] = {0,1,1,0};	// s3: 1 inverts, 0 doesn't invert
							// s2: 0 for cortex, 1 for pad
							// s1/s0:
								// 00: pad
								// 01: cortex
								// 10: vdd
								// 11: gnd
	int mod_15_4_tune[3] = {1,0,1};
	int mod_15_4_tune_d[1] = {0};		// spare dummy bit

	// Divider settings
	int sel_1mhz_2mhz[1] = {1};		// 1: 2MHz, 0: 1MHz
	int pre_dyn_dummy_b[1] = {0};		// Disconnected
	int pre_dyn_b[6] = {0,0, 0,0,0,0};		// Disconnected
	int pre_dyn_en_b[1] = {0};			// Disconnected
	int pre_2_backup_en[1] = {0};		// 1: Enables backup div-by-2 pre-scaler
	int pre_5_backup_en[1] = {1};		// 1: Enables backup div-by-5 pre-scaler
	int pre_dyn_sel[1] = {0};			// 1-4. Determines what pre_dyn will be.
							// pre_dyn 0's indicate disconnected,
							// otherwise it's connected to div-by-5 or div-by-2
	int pre_dyn_dummy[1] = {0};		// Disconnected
	int div_64mhz_enable[1] = {0};		// 1: Enable 64MHz clock source
	int div_20mhz_enable[1] = {0};		// 1: Enable 20MHz clock source
	int div_static_code[16] = {0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0};	// MSB -> LSB (flipping handled internally)
	int div_static_rst_b[1] = {1}; 	// 0: Reset static divider
	int div_static_en[1] = {1};		// 1: Enable static divider
	int dyn_div_N[13] = {0, 0,0,0,0, 0,0,0,0, 0,0,0,0};		// Binary. Divide ratio.
	int div_dynamic_en_b[1] = {1};		// 0: Enable dynamic divider.
	int div_tune_select[1] = {1};		// 0: cortex, 1: scan
	int source_select_2mhz = {0,0};	// 00: static divider
								// 01: dynamic divider
								// 10: aux
								// 11: pad

	// 2MHz oscillator tuning and enable
	int rc_2mhz_tune_coarse_1[5] = {1,1,1,1,1};
	int rc_2mhz_tune_coarse_2[5] = {1,1,1,1,1};
	int rc_2mhz_tune_coarse_3[5] = {0,0,0,1,1};
	int rc_2mhz_tune_fine[5] = {0,1,1,1,1};
	int rc_2mhz_tune_superfine[5] = {0,0,1,1,0};
	int rc_2mhz_enable[1] = {0};

	/*****************************/
	/**** BLE Module Settings ****/
	/*****************************/
	int scan_io_reset[1] = {1};
	int scan_5mhz_select[2] = {0,0};
	int scan_1mhz_select[2] = {0,0};
	int scan_20mhz_select[2] = {0,0};
	int scan_async_bypass[1] = {0};
	int scan_mod_bypass[1] = {1};
	int scan_fine_trim[6] = {1,1,1, 0,0,0};
	int scan_data_in_valid[1] = {1};
	int scan_ble_select[1] = {0};

	// Choosing clock source
	int sel_ble_packetassembler_clk[1] = {0};		// BLE packet assembler clock source
										// 1: analog_cfg_ble[358]
										// 0: BLE_PDA_clk
	int sel_ble_packetdisassembler_clk[1] = {0};	// BLE packet disassembler clock source
										// 1: BLE_PDA_clk
										// analog_cfg_ble[402]
	int sel_ble_cdr_fifo_clk[2] = {0,0};			// BLE CCDR FIFO clock source
										// 00: analog_cfg_ble[476]
										// 01: dbbRingclkin
										// 10: BLE_PDA_clk
										// 11: ADC_CLK_EXT
	int sel_ble_arm_fifo_clk[1] = {0};				// BLE ARM FIFO clock source
										// 1: BLE_PDA_clk
										// 0: analog_cfg_ble[475]
	
	// Disables: 0 is enable
	int div_symbol_clk_ble_dis[1] = {1};
	int div_BLE_PDA_dis[1] = {1};
	int div_EXT_CLK_GPIO2_dis[1] = {1};

	// Resets: 1 is reset
	int div_symbol_clk_ble_reset[1] = {0};
	int div_BLE_PDA_reset[1] = {0};
	int div_EXT_CLK_GPIO2_reset[1] = {0};

	// Passthroughs: 1 is passthrough
	int div_symbol_clk_ble_PT[1] = {0};
	int div_BLE_PDA_PT[1] = {0};
	int div_EXT_CLK_GPIO2_PT[1] = {0};

	// Divide values
	int div_symbol_clk_ble_Nin[8] = {0,0,0,0, 0,0,0,0};
	int div_BLE_PDA_Nin[8] = {0,0,0,0, 0,0,0,0};
	int div_EXT_CLK_GPIO2_Nin[8] = {0,0,0,0, 0,0,0,0};

	/* END: DO NOT CHANGE THESE */

	/* START: Feel free to change things in this section */

	// Example
	div_EXT_CLK_GPIO2_Nin = {1,1,1,1, 1,1,1,1};

	/* END: Feel free to change things in this section */

	/* START: ASC assignment */
	// TODO: Should ASC be pointers to the arrays above? 
	// Is there enough memory to avoid doing that?
	// C is nasty with pointers.


	/* END: ASC assignment */

	return 0;
}