import serial
import random
import os
import fcntl
import termios
import sys
import time
import struct
import difflib

# SCM 3 analog scan chain programming script

## Variables for operation to match the matlab scan function
#  LDO enable
lo_enable  = 1; # 1 enables LO LDO, 0 disbales
pa_enable  = 1; # 1 enables PA LDO, 0 disables
div_enable = 1; # 1 enables DIVIDER LDO, 0 disables

#  LO frequency tuning
lo_fine_tune =   15;
lo_mid_tune =    15;
lo_coarse_tune = 15;

#  LO current tuning
lo_current = 127;

#  LDO voltage tuning
lo_panic = 0;
pa_panic = 0;
div_panic = 0;

lo_ldo = 63;
pa_ldo = 63;
div_ldo = 0;

#  Divider settings
sel_12 = 1; # choose 1 for x2 in divider chain, 0 to disable x2
en_backup_2 = 0; # 1 enables the backup div-by-2 prescaler, 0 disables
en_backup_5 = 1; # 1 enables the backup div-by-5 prescaler, 0 disables
pre_dyn_sel = 0; # 0 disbles all
en_64mhz = 0;
en_20mhz = 0;
static_code = 960;
static_en = 1;   # enable the static divider

# Configure voltage for pcb analog and digital LDOs 
vdda_pcb_tune = [0,0,0,0]
vddd_pcb_tune = [0,0,0,1]

# SCM GPIO direction control
# 0,= input to SCM, 1,= output from SCM
# [0:15]

gpio_direction = [1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1]

# Enables for other pcb LDOs
vbat_ldo_enable = 1
gpio_ldo_enable = 1
debug_ldo_enable = 0
vdda_pcb_ldo_enable = 0
vddd_pcb_ldo_enable = 1
ldo_enables = [0,0,0,vbat_ldo_enable,gpio_ldo_enable,debug_ldo_enable,vdda_pcb_ldo_enable,vddd_pcb_ldo_enable]

#------------------------------
# Begin scan chain


ASC = [	1,		#  sel_data     , Recovered data , 0,is for LC and 1,is for Ring 
 		1,		#  sel_counter0_reset, sel signal for counter 0,reset, 0,from ASC[8] and 1,from M0,analog_cfg[0]
            	#  sel signal for counter 0,enable, 0,from ASC[15] and 1,from M0,analog_cfg[7]
		1,		#  sel_counter1_reset, sel signal for counter 1,reset, 0,from ASC[9] and 1,from M0,analog_cfg[1]
            	#  sel signal for counter 1,enable, 0,from ASC[16] and 1,from M0,analog_cfg[8]
		1,		#  sel_counter2_reset, sel signal for counter 2 reset, 0,from ASC[10] and 1,from M0,analog_cfg[2]
            	#  sel signal for counter 2 enable, 0,from ASC[17] and 1,from M0,analog_cfg[9]
		1,		#  sel_counter3_reset, sel signal for counter 3 reset, 0,from ASC[11] and 1,from M0,analog_cfg[3]
            	#  sel signal for counter 3 enable, 0,from ASC[18] and 1,from M0,analog_cfg[10]
		1,		#  sel_counter4_reset, sel signal for counter 4 reset, 0,from ASC[12] and 1,from M0,analog_cfg[4]
            	#  sel signal for counter 4 enable, 0,from ASC[19] and 1,from M0,analog_cfg[11]
		1,		#  sel_counter5_reset, sel signal for counter 5 reset, 0,from ASC[13] and 1,from M0,analog_cfg[5]
            	#  sel signal for counter 5 enable, 0,from ASC[20] and 1,from M0,analog_cfg[12]
		1,		#  counter0_reset, 0,is reset
            	#  && sel signal f or counter 6 reset, 0,from ASC[14] and 1,from M0,analog_cfg[6]
            	#  sel signal for counter 6 enable, 0,from ASC[21] and 1,from M0,analog_cfg[13]
		1,		#  counter1_reset, 0,is reset
		0,		#  counter2_reset, 0,is reset
		0,		#  counter3_reset, 0,is reset
		0,		#  counter4_reset, 0,is reset
		0,		#  counter5_reset, 0,is reset
		0,		#  counter6_reset, 0,is reset
		0,		#  counter0_enable, 1,is enable
		0,		#  counter1_enable, 1,is enable
		0,		#  counter2_enable, 1,is enable
		0,		#  counter3_enable, 1,is enable
		0,		#  counter4_enable, 1,is enable
		0,		#  counter5_enable, 1,is enable
		0,		#  counter6_enable, 1,is enable
		0,
		1,0,	#  sel_mux3in, 00,LF_CLOCK, 01,LF_ext_PAD, 10,HF_CLOCK, 11,1'b0,        //error 10,is external pad
		1,		#  divider_RFTimer_enable, 1,is enable
		1,		#  divider_CortexM0_enable, 1,is enable, HCLK
		1,		#  divider_GFSK_enable, 1,is enable, divider_out_GFSK
		1,		#  divider_ext_GPIO_enable, 1,is enable, EXT_CLK_GPIO
		1,		#  divider_integ_enable, 1,is enable, 
		1,		#  divider_2MHz_enable, 1,is enable,
		1,		#  divider_RFTimer_resetn, 0,is reset
		1,		#  divider_CortexM0_resetn, 0,is reset
		1,		#  divider_GFSK_resetn, 0,is reset
		1,		#  divider_ext_GPIO_resetn, 0,is reset
		1,		#  divider_integ_resetn, 0,is reset
		1,		#  divider_2MHz_resetn, 0,is reset
		1,		#  divider_RFTimer_PT, 1,is passthrough
		1,		#  divider_CortexM0_PT, 1,is passthrough
		1,		#  divider_GFSK_PT, 1,is passthrough
		1,		#  divider_ext_GPIO_PT, 1,is passthrough
		1,		#  divider_integ_PT, 1,is passthrough
		1];		#  divider_2MHz_PT, 1,is passthrough	
ASC.extend([0,0,0,0,0,0,0,1][::-1])		#  ASC[49:-1:42]  	divider_RFTimer_Nin, divide value
ASC.extend([0,0,0,0,0,0,1,1][::-1])		#  ASC[57:-1:52]  	divider_CortexM0_Nin, divide value
ASC.extend([0,0,0,0,0,0,0,1][::-1])		#  ASC[65:-1:58]  	divider_GFSK_Nin, divide value
ASC.extend([0,0,0,0,0,0,0,1][::-1])		#  ASC[73:-1:66]  	divider_ext_GPIO_Nin, divide value
ASC.extend([0,0,0,0,0,0,0,1][::-1])		#  ASC[81:-1:74]  	divider_integ_Nin, divide value
ASC.extend([0,0,0,0,0,0,0,1][::-1])		#  ASC[89:-1:82]  	divider_2MHz_Nin, divide value
ASC.extend([0])							#  ASC[90]  		mux_sel_RFTIMER, 0,is divider_out_RFTIMER, 1,is TIMER32k
ASC.extend([0])							#  ASC[91]  		mux_sel_GFSK_CLK, 0,is LC_div_N, 1,is divider_out_GFSK
ASC.extend([0,0][::-1])					#  ASC[93:92]  		mux3_sel_CLK2MHz, 00,RC_2MHz, 01,divider_out_2MHz, 10,LC_2MHz, 11,1'b0
ASC.extend([0,0][::-1])					#  ASC[95:94]  		mux3_sel_CLK1MHz, 00,LC_1MHz_dyn, 01,divider_out_2MHz, 10,LC_1MHz_stat, 11,1'b0
ASC.extend([0,0][::-1])					#  ASC[97:96]		IQ_select, Bob's digital baseband
ASC.extend([0,0][::-1])					#  ASC[99:98] 		op_mode, Bob's digital baseband
ASC.extend([0])							#  ASC[100] 		agc_overload, Bob's digital baseband
ASC.extend([0])							#  ASC[101]			agc_ext_or_int, Bob's digital baseband
ASC.extend([0])							#  ASC[102]			vga_select, Bob's digital baseband
ASC.extend([0])							#  ASC[103]			mf_data_sign, Bob's digital baseband
ASC.extend([0,0,0][::-1])				#  ASC[106:104]		mux sel for 7 counters output to GPIO bank 11
            	                		# 0,analog_rdata[31:0]
                	            		# 1,analog_rdata[63:32]
                    	        		# 2 analog_rdata[95:64]
                        	    		# 3 analog_rdata[127:96]
                       		    		# 4 analog_rdata[159:128]
                            			# 5 analog_rdata[191:160]
                            			# 6 analog_rdata[223:192]
                            			# 7 32'b0

ASC.extend([0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,1][::-1])	# ASC[122:107] 	COUNT_THRESH, Brian's DBB
ASC.extend([0])										# ASC[123] 		FB_EN, Brian's DBB
ASC.extend([0,0,1,0,0,1,1,0][::-1])					# ASC[131:124] 	CLK_DIV_ext, Brian's DBB
ASC.extend([1])										# ASC[132] 		DEMOD_EN, Brian's DBB
ASC.extend([0,0,0,0,0,0,0,0,0,0,0][::-1])			# ASC[143:133] 	INIT_INTEG, Brian's DBB

ASC.extend([0]*16)									# ASC[144:159]
													# ASC[144] sel signal for HCLK divider2 maxdivenb between Memory mapped vs
													# ASC[145] mux in0, 0,ASC[145] 1,analog_cfg[14]
ASC.extend([0]*16)									# ASC[160:175] 	Not used
ASC.extend([0]*16)									# ASC[176:191] 	Not used
ASC.extend([0])										# ASC[192] 		sel_FREQ_CONTR_WORD, Brian's DBB
ASC.extend([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0][::-1]) # ASC[208:193] 	SACLEINT_ext, Brian's DBB
ASC.extend([0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0][::-1])	# ASC[224:209] 	EAERLY_DECISION_MARGIN_ext, Brian's DBB
ASC.extend([0,0][::-1])								# ASC[226:225] 	mux_sel_FREQ_CTRL_WORD_SCAN
                        							# 0,ASC[237:227]
                        							# 1,FREQ_CTRL_WORD_SCAN
                        							# 2 analog_cfg[74:64]
                        							# 3 11'b0
ASC.extend([0,0,0,0,0,0,0,0,0,0,0][::-1])			# ASC[237:227] 	FREQ_CTRL_WORD_SCAN
ASC.extend([1])										# ASC[238] 		sel dbb Ring data in, 0,SIG_IN, 1,COMPP_ZCC choose inputs to brians zcc demod
ASC.extend([1])										# ASC[239] 		sel dbb Ring CLK in, 0,ZCC_CLK_Ring, 1,ZCC_CLK
ASC.extend([1])										# ASC[240] 		mux sel for RST_B, 0,ASC[241], 1,analog_cfg[75]
ASC.extend([1])										# ASC[241] 		ASC RST_B
ASC.extend([0])										# ASC[242] 		mux sel ADC reset, 0,adc_reset, 1,adc_reset_gpi
ASC.extend([0])										# ASC[243] 		mux sel ADC convert, 0,adc_convert, 1,adc_convert_gpi
ASC.extend([0])										# ASC[244] 		mux sel ADC pga amplify, 0,adc_pga_amplify, 1,adc_pga_amplify_gpi
ASC.extend([0,1,1,0][::-1])							# ASC[248:245] 	GPO row 1,sel
ASC.extend([0,0,0,0][::-1])							# ASC[252:249] 	GPO row 2 sel
ASC.extend([0,0,0,0][::-1])							# ASC[256:253] 	GPO row 3 sel
ASC.extend([0,0,0,0][::-1])							# ASC[260:257]  GPO row 4 sel
ASC.extend([0,0][::-1])								# ASC[262:261] 	GPI row 1,sel
ASC.extend([0,0][::-1])								# ASC[264:263] 	GPI row 2 sel
ASC.extend([1,1][::-1])								# ASC[266:265] 	GPI row 3 sel
ASC.extend([0,0][::-1])								# ASC[268:267] 	GPI row 4 sel
ASC.extend([0])										# ASC[269] 		mux sel for M0,clk, 0,dbbclk, 1,DATA_CLK_IN
ASC.extend([0])										# ASC[270] 		mux sel for M0,data, 0,dbbdata, 1,DATA_IN

## ASC 271:516 -- LC IF Chain
##----------------------------
## Q channel gain control
agc_gain_mode = [0];                              # 0,= Gain is controlled via scan chain, 1,= controlled by AGC
code_scan = [1,1,1,1,1,1];                      # Gain control vector from scan chain, binary weighted MSB first <5:0>, all ones is max gain
stg3_gm_tune = [1,1,1,1,1,1,1,1,1,1,1,1,1];     # Gain control for AGC driver <1:13>, all ones is max gain, thermometer (not 1-hot)

# Debug control at output of Q mixer
dbg_bias_en_Q = [0];                              # 1,= Turn on bias for source followers
dbg_out_en_Q = [0];                               # 1,= Turn on output pass gate for debug
dbg_input_en_Q = [0];                             # 1,= Turn on input pass gate for debug

# Mixer bias control
mix_bias_In_ndac = [0,0,0,1];                   # Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_i = [0];                                  # 0,= I mixer enabled
mix_bias_Ip_ndac = [0,0,0,1];                   # Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_bias_Qn_ndac = [0,0,0,1];                   # Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_q = [1];                                  # 0,= I mixer enabled
mix_bias_Qp_ndac = [0,0,0,1];                   # Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage

mix_bias_In_pdac = [0,0,0,0];                   # Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Ip_pdac = [0,0,0,0];                   # Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qn_pdac = [0,0,0,0];                   # Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qp_pdac = [0,0,0,0];                   # Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage


IFsection1 = agc_gain_mode 	+ code_scan + stg3_gm_tune + dbg_bias_en_Q + dbg_out_en_Q + dbg_input_en_Q \
							+ mix_bias_In_ndac + mix_off_i + mix_bias_Ip_ndac + mix_bias_Qn_ndac + mix_off_q \
							+ mix_bias_Qp_ndac + mix_bias_In_pdac + mix_bias_Ip_pdac + mix_bias_Qn_pdac + mix_bias_Qp_pdac

## Q channel
tia_cap_on = [0,0,0,0];                         # TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = [1];                               # 1,= pull TIA inputs to ground
tia_enn = [0];                                    # 1,= enable nmos bias in TIA
tia_enp = [1];                                    # 0,= enable pmos bias in TIA

# Q Channel comparator offset trim
pctrl = [0,0,0,0,0];                            # <4(MSB):0>, binary weighted, increase from 0,to add cap to either side of comparator
nctrl = [0,0,0,0,0];                            # <4(MSB):0>, binary weighted, increase from 0,to add cap to either side of comparator

# Q channel comparator control
mode_1bit = [1];                                  # 1,= turn on zero crossing mode
adc_comp_en = [1];                                # Clock gate for comparator, 0,= disable the clock

# Q channel stage3 amp control
stg3_amp_en = [0];                                # Bias enable for folded cascode, 1,= bias on
# Control for debug point at input of Q stg3
dbg_out_on_stg3 = [0];                            # 1,= Turn on output pass gate for debug
dbg_input_on_stg3 = [0];                          # 1,= Turn on input pass gate for debug
dbg_bias_en_stg3 = [0];                           # 1,= Turn on bias for source followers

# Q channel stage2 amp control
stg2_amp_en = [0];                                # Bias enable for folded cascode, 1,= bias on
# Control for debug point at input of Q stg2
dbg_out_on_stg2 = [0];                            # 1,= Turn on output pass gate for debug
dbg_input_on_stg2 = [0];                          # 1,= Turn on input pass gate for debug
dbg_bias_en_stg2 = [0];                           # 1,= Turn on bias for source followers

# Q channel stage1,amp control
stg1_amp_en = [0];                                # Bias enable for folded cascode, 1,= bias on
# Control for debug point at input of Q stg1
dbg_out_on_stg1 = [0];                            # 1,= Turn on output pass gate for debug
dbg_input_on_stg1 = [0];                          # 1,= Turn on input pass gate for debug
dbg_bias_en_stg1 = [0];                           # 1,= Turn on bias for source followers

IFsection2 = tia_cap_on + tia_pulldown + tia_enn + tia_enp + pctrl + nctrl + mode_1bit + adc_comp_en \
						+ stg3_amp_en + dbg_out_on_stg3 + dbg_input_on_stg3 + dbg_bias_en_stg3		 \
						+ stg2_amp_en + dbg_out_on_stg2 + dbg_input_on_stg2 + dbg_bias_en_stg2		 \
						+ stg1_amp_en + dbg_out_on_stg1 + dbg_input_on_stg1 + dbg_bias_en_stg1

## Q channel ADC control
vcm_amp_en = 	[0];							# 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = 	[0,0];							# <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = 	[0];							# 1 = enable clock for Vcm SC divider
vref_clk_en = 	[0];							# 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1,1];                          # <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = 	[0];						    # 1 = enable bias for Vref buffer amp
adc_fsm_en = 	[0];							# 1 = enable the adc fsm
adc_dbg_en = 	[0];							# 1 = enable adc debug outputs  (bxp, compp)

IFsection3 = vcm_amp_en + vcm_vdiv_sel + vcm_clk_en + vref_clk_en + vref_vdiv_sel + vref_amp_en + adc_fsm_en + adc_dbg_en;

## Q channel filter
stg3_clk_en = [0];                              # 1 = enable clock for IIR
stg3_C2 = [0,0,1];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [0,0,1];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = [0];                              # 1 = enable clock for IIR
stg2_C2 = [0,0,1];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0,0,1];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = [0];                              # 1 = enable clock for IIR
stg1_C2 = [0,1,0];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0,1,0];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection4 = stg3_clk_en + stg3_C2 + stg3_C1 + stg2_clk_en + stg2_C2 + stg2_C1 + stg1_clk_en + stg1_C2 + stg1_C1;

## I channel ADC control
vcm_amp_en = [1];                               # 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = [0,0];                           # <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = [1];                               # 1 = enable clock for Vcm SC divider
vref_clk_en = [1];                              # 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1,1];                          # <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = [1];                              # 1 = enable bias for Vref buffer amp
adc_fsm_en = [1];                               # 1 = enable the adc fsm
adc_dbg_en = [1];                               # 1 = enable adc debug outputs (bxp, compp)

IFsection5 = vcm_amp_en + vcm_vdiv_sel + vcm_clk_en + vref_clk_en + vref_vdiv_sel + vref_amp_en + adc_fsm_en + adc_dbg_en;

## I channel filter
stg3_clk_en = [1];                              # 1 = enable clock for IIR
stg3_C2 = [0,0,1];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [1,1,1];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = [1];                              # 1 = enable clock for IIR
stg2_C2 = [0,0,0];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0,0,1];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = [1];                              # 1 = enable clock for IIR
stg1_C2 = [0,0,0];                              # C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0,0,0];                              # C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection6 = stg3_clk_en + stg3_C2 + stg3_C1 + stg2_clk_en + stg2_C2 + stg2_C1 + stg1_clk_en + stg1_C2 + stg1_C1;

## Clock generation control
adc_dbg_en = [1];                               # 1 = enable output of debug phases for adc clocks (cx,phix)                      
adc_phi_en = [1];                               # 1 = enable clocks for adc (cx,phix)
filt_phi_en = [1];                              # 1 = enable 4 filter clock phases (0,90,180,270)
clk_select = [0,1];                             # <1:0> mux select for IF clock 00=gnd 01=internal RC 10=divided LC 11=external pad
RC_clk_en = [1];                                # 1 = enable RC osc
RC_coarse = [0,0,0,0,0];                        # coarse tuning for RC, binary weighted <4(MSB):0>
filt_dbg_en = [1];                              # 1 = enable debug outputs for 4 filter clock phases (0,90,180,270)
RC_fine = [0,0,0,0,0];                          # fine tuning for RC, binary weighted <4(MSB):0>

IFsection7 = adc_dbg_en + adc_phi_en + filt_phi_en + clk_select + RC_clk_en + RC_coarse + filt_dbg_en + RC_fine;

## I channel
tia_cap_on = [0,0,0,0];                         # TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = [0];                             # 1 = pull TIA inputs to ground
tia_enn = [1];                                  # 1 = enable nmos bias in TIA
tia_enp = [0];                                  # 0 = enable pmos bias in TIA

# I Channel comparator offset trim
pctrl = [1,1,1,1,1];                            # <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator
nctrl = [0,0,0,0,0];                            # <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator

# I channel comparator control
mode_1bit = [0];                                # 1 = turn on zero crossing mode
adc_comp_en = [1];                              # Clock gate for comparator, 0 = disable the clock

# I channel stage3 amp control
stg3_amp_en = [1];                              # Bias enable for folded cascode, 1 = bias on
# Control for debug point at input of I stg3
dbg_out_on_stg3 = [0];                          # 1 = Turn on output pass gate for debug
dbg_input_on_stg3 = [0];                        # 1 = Turn on input pass gate for debug
dbg_bias_en_stg3 = [0];                         # 1 = Turn on bias for source followers

# I channel stage2 amp control
stg2_amp_en = [1];                              # Bias enable for folded cascode, 1 = bias on
# Control for debug point at input of I stg2
dbg_out_on_stg2 = [0];                          # 1 = Turn on output pass gate for debug
dbg_input_on_stg2 = [0];                        # 1 = Turn on input pass gate for debug
dbg_bias_en_stg2 = [0];                         # 1 = Turn on bias for source followers

# I channel stage1 amp control
stg1_amp_en = [1];                              # Bias enable for folded cascode, 1 = bias on
# Control for debug point at input of I stg1
dbg_out_on_stg1 = [1];                          # 1 = Turn on output pass gate for debug
dbg_input_on_stg1 = [0];                        # 1 = Turn on input pass gate for debug
dbg_bias_en_stg1 = [1];                         # 1 = Turn on bias for source followers

IFsection8 = dbg_bias_en_stg1 + dbg_input_on_stg1 + dbg_out_on_stg1 + stg1_amp_en \
    		+ dbg_bias_en_stg2 + dbg_input_on_stg2 + dbg_out_on_stg2 + stg2_amp_en \
    		+ dbg_bias_en_stg3 + dbg_input_on_stg3 + dbg_out_on_stg3 + stg3_amp_en \
    		+ adc_comp_en + mode_1bit + nctrl + pctrl + tia_enp + tia_enn + tia_pulldown + tia_cap_on;

## I channel
# Debug control at output of I mixer
dbg_input_en_I = [0];                           # 1 = Turn on input pass gate for debug
dbg_out_on_I = [0];                             # 1 = Turn on output pass gate for debug
dbg_bias_en_I = [0];                            # 1 = Turn on bias for source followers


# I channel gain control
stg3_gm_tune = [0,0,0,0,0,1,1,1,1,1,1,1,1];     # Gain control for AGC driver <13:1>, all ones is max gain, thermometer (not 1-hot)
code_scan = [1,1,1,1,1,1];                      # Gain control vector from scan chain, binary weighted <0:5(MSB)>, all ones is max gain
# code_scan = zeros(1,6);
agc_gain_mode = [0];                            # 0 = Gain is controlled via scan chain, 1 = controlled by AGC

IFsection9 = dbg_input_en_I + dbg_out_on_I + dbg_bias_en_I + stg3_gm_tune + code_scan + agc_gain_mode;


## LDO control
# if_ldo_rdac = [0 1 0 1 0 1 1];                  # Resistor for setting bandgap ref voltage <0:6(MSB)>
if_ldo_rdac = [0,0,0,0,0,0,0];                    # Resistor for setting bandgap ref voltage <0:6(MSB)>
por_disable = [0];                                # 1 = disable ability for POR to reset IF blocks
scan_reset = [1];                                 # 0 = force reset from scan chain

IFsection10 = if_ldo_rdac + por_disable + scan_reset;


## Compile into one vector
IF_scan = IFsection1 + IFsection2 + IFsection3 + IFsection4 + IFsection5 + IFsection6 + IFsection7 + IFsection8 + IFsection9 + IFsection10;

## Power on control module
# scan_pon_XX turns on LDO via scan chain
# gpio_pon_en_XX will turn on LDO if power on signal from GPIO bank is high
# fsm_pon_en_XX will turn on LDO if power on signal from radio FSM is high
# master_ldo_en_XX = 0 forces LDO off; =1 allows control from gpio, fsm, or scan-chain
scan_pon_if = [0];
scan_pon_lo = [lo_enable];
scan_pon_pa = [pa_enable];
gpio_pon_en_if = [0];
fsm_pon_en_if = [0];
gpio_pon_en_lo = [0];
fsm_pon_en_lo = [0];
gpio_pon_en_pa = [0];
fsm_pon_en_pa = [0];
master_ldo_en_if = [0];
master_ldo_en_lo = [lo_enable];
master_ldo_en_pa = [1];
scan_pon_div = [div_enable];
gpio_pon_en_div = [0];
fsm_pon_en_div = [0];
master_ldo_en_div = [div_enable];
power_control =   scan_pon_if + scan_pon_lo + scan_pon_pa	\
    			+ gpio_pon_en_if + fsm_pon_en_if 			\
    			+ gpio_pon_en_lo + fsm_pon_en_lo			\
    			+ gpio_pon_en_pa + fsm_pon_en_pa			\
    			+ master_ldo_en_if + master_ldo_en_lo + master_ldo_en_pa \
    			+ scan_pon_div + gpio_pon_en_div + fsm_pon_en_div + master_ldo_en_div;

# Entire LC IF sigpath
ASC.extend(IF_scan + power_control);

# DB section A
##----------------------------
ASC.extend([0]*429)
ASC[623] = 1; #32 kHz oscillator enable
# TO BE COMPLETED AT A LATER DATE

## ASC 946:1088 -- LC tuning and transmitter
##----------------------------

#ASC(979) = 0; # ASC[979] unusued
#ASC(987) = 0; # ASC[987]unused
#ASC(1011) = 0; #unused

#  LC LO frequency tuning and RX/TX
#[int(x) for x in list('{0:0b}'.format(8))]

ASC[915] = 1; #safety
ASC[916] = 1; # AUX LDO enable
ASC[917] = 1; #safety

ASC[802] = 0;

fine_code =   [int(lo_fine_tune)   for lo_fine_tune   in list('{0:05b}'.format(lo_fine_tune))];
mid_code =    [int(lo_mid_tune)    for lo_mid_tune    in list('{0:05b}'.format(lo_mid_tune))];
coarse_code = [int(lo_coarse_tune) for lo_coarse_tune in list('{0:05b}'.format(lo_coarse_tune))];
fine_tune = fine_code[::-1] + [0];
mid_tune = mid_code[::-1] + [0];
coarse_tune = coarse_code[::-1] + [0];

lo_tune_select = [1]; 	# 0 for cortex, 1 for scan chain
polyphase_enable = [1]; # 0 disables, 1 enables
lo_current_tune = [int(lo_current) for lo_current in list('{0:08b}'.format(lo_current))][::-1];

#  test BG for visibility:
test_bg = [0,0,0,0,0,0,1];

#  LDO settings:
pa_ldo_rdac = [pa_panic] + [int(pa_ldo) for pa_ldo in list('{0:06b}'.format(8))][::-1];
lo_ldo_rdac = [lo_panic] + [int(lo_ldo) for lo_ldo in list('{0:06b}'.format(8))][::-1];
div_ldo_rdac = [div_panic] + [int(div_ldo) for div_ldo in list('{0:06b}'.format(8))][::-1];

#  modulation settings
mod_logic = [0,1,1,0]; # msb -> lsb, s3:0 to not invert, 1 to invert
                       #             s2:0 cortex, 1 pad
                       #             s0,s1: choose between pad, cortex,
                       #             vdd, gnd
mod_15_4_tune = [1,0,1]; #msb -> lsb
mod_15_4_tune_d = [0]; # spare dummy bit

#  divider settings
sel_1mhz_2mhz = [sel_12]; # 0 is for 2mhz, 1 is for 1mhz
pre_dyn_dummy_b = [0]; # disconnected
pre_dyn_b = [0,0,0,0,0,0]; # disconnected
pre_dyn_en_b = [0]; # disconnected
pre_2_backup_en = [en_backup_2]; # enable the backup div-by-2 pre-scaler
pre_2_backup_en_b = [1-en_backup_2]; # disable the div-by-2 pre-scaler
pre_5_backup_en = [en_backup_5]; # enable the backup div-by-5 pre-scaler
pre_5_backup_en_b = [1-en_backup_5]; # disable the backup div-by-5 pre-scaler
pre_dyn = [1,1,1,0,0,0];
if pre_dyn_sel == 0:
    pre_dyn = [1,1,1,0,0,0]; # by-5, by-2, by-5, disconnected, disconnected, disconnected
elif pre_dyn_sel == 1:
    pre_dyn = [0,1,1,0,0,0];
elif pre_dyn_sel == 2:
    pre_dyn = [1,0,1,0,0,0];
elif pre_dyn_sel == 3:
    pre_dyn = [1,1,0,0,0,0];

pre_dyn_dummy = [0]; # disconnected
div_64mhz_enable = [en_64mhz]; # enable the 64mhz clock source
div_20mhz_enable = [en_20mhz]; # enable the 20mhz clock source
div_static_code = static_code; # blaze it
div_static_code_bin = [int(div_static_code) for div_static_code in list('{0:016b}'.format(div_static_code))];
div_static_rst_b = [1]; # reset (active low) of the static divider
div_static_en = [static_en]; # static divider enable
div_static_select = div_static_code_bin[10:16] + \
						div_static_code_bin[4:10] + \
						div_static_en + \
						div_static_rst_b + \
						div_static_code_bin[0:4];
div_static_select[:] = [1-x for x in div_static_select]
                    
dyn_div_N = 420; # joke's not funny anymore
div_dynamic_select = [int(dyn_div_N) for dyn_div_N in list('{0:013b}'.format(dyn_div_N))]
div_dynamic_en_b = [1]; # disable the dynamic divider
div_dynamic_select = div_dynamic_select + div_dynamic_en_b;
div_tune_select = [1]; # 0 for cortex, 1 for scan
source_select_2mhz = [0,0]; # 00 static, 01 dynamic, 10 from aux, 11 pad

# BLE module settings - I have forgotten what most of these do
scan_io_reset = [1];
scan_5mhz_select = [0,0]; # msb -> lsb
scan_1mhz_select = [0,0]; # msb -> lsb
scan_20mhz_select = [0,0]; # msb -> lsb
scan_async_bypass = [0];
scan_mod_bypass = [1];
scan_fine_trim = [1,1,1,0,0,0]; # msb -> lsb # note: test 0 0 0 1 1 1 later.
scan_data_in_valid = [1];
scan_ble_select = [0];

ASC.extend(fine_tune + mid_tune + coarse_tune + lo_tune_select + test_bg[1:] + polyphase_enable + pa_ldo_rdac);
ASC.extend([0]);
ASC.extend(lo_ldo_rdac);
ASC.extend([0])
ASC.extend(lo_current_tune + mod_logic + mod_15_4_tune[::-1] + mod_15_4_tune_d + div_ldo_rdac)
ASC.extend([0])

ASC.extend(		  sel_1mhz_2mhz + \
                  pre_dyn_dummy_b + \
                  pre_dyn_b + \
                  pre_dyn_en_b + \
                  pre_2_backup_en_b + \
                  pre_2_backup_en + \
                  pre_5_backup_en + \
                  pre_5_backup_en_b + \
                  pre_dyn + \
                  pre_dyn_dummy + \
                  div_64mhz_enable + \
                  div_20mhz_enable + \
                  scan_io_reset + \
                  scan_5mhz_select + \
                  scan_1mhz_select + \
                  scan_20mhz_select + \
                  scan_async_bypass + \
                  scan_mod_bypass + \
                  scan_fine_trim + \
                  div_static_select + \
                  div_dynamic_select + \
                  div_tune_select + \
                  [test_bg[0]] + \
                  source_select_2mhz + \
                  scan_data_in_valid + \
                  scan_ble_select )
                  
ASC.extend([0,0]) #unused

#  2 MHz oscillator tuning and enable
ASC.extend([1,1,1,1,1]); # ASC[1089:1093] coarse 1
ASC.extend([1,1,1,1,1]); # ASC[1094:1098] coarse 2
ASC.extend([0,0,0,1,1]); # ASC[1099:1103] coarse 3
ASC.extend([0,1,1,1,1]); # ASC[1104:1108] fine
ASC.extend([0,0,1,1,0]); # ASC[1109:1113] superfine
ASC.extend([0]); # 2MHz RC enable

## GPIO Direction Control

# 1 = enabled, 0 = disabled
# <0:15>
#out_mask = gpio_direction;
out_mask = [1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1];
#out_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0]; 
#in_mask = -gpio_direction + 1;
in_mask = [1,1,1,1, 1,1,1,1, 1,1,1,1, 1,1,1,1];
#in_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0];



# Turning on loopback of gp<1> to conenct gpo<1> to GPIO_PON
##--------------------------
#in_mask[1] = 1;
##--------------------------

# Map to ASC values, on-chip mapping is:
# out_en<0:15> = ASC<1131>,ASC<1133>,ASC<1135>,ASC<1137>,ASC<1140>,ASC<1142>,ASC<1144>,ASC<1146>,ASC<1115>,ASC<1117>,ASC<1119>,ASC<1121>,ASC<1124>,ASC<1126>,ASC<1128>,ASC<1130>				
# in_en<0:15> = ASC<1132>,ASC<1134>,ASC<1136>,ASC<1138>,ASC<1139>,ASC<1141>,ASC<1143>,ASC<1145>,ASC<1116>,ASC<1118>,ASC<1120>,ASC<1122>,ASC<1123>,ASC<1125>,ASC<1127>,ASC<1129>	
ASC.extend([1-out_mask[8]])
ASC.extend([in_mask[8]])
ASC.extend([1-out_mask[9]])
ASC.extend([in_mask[9]])
ASC.extend([1-out_mask[10]])
ASC.extend([in_mask[10]])
ASC.extend([1-out_mask[11]])
ASC.extend([in_mask[11]])

ASC.extend([in_mask[12]])
ASC.extend([1-out_mask[12]])
ASC.extend([in_mask[13]])
ASC.extend([1-out_mask[13]])
ASC.extend([in_mask[14]])
ASC.extend([1-out_mask[14]])
ASC.extend([in_mask[15]])
ASC.extend([1-out_mask[15]])

ASC.extend([1-out_mask[0]])
ASC.extend([in_mask[0]])
ASC.extend([1-out_mask[1]])
ASC.extend([in_mask[1]])
ASC.extend([1-out_mask[2]])
ASC.extend([in_mask[2]])
ASC.extend([1-out_mask[3]])
ASC.extend([in_mask[3]])

ASC.extend([in_mask[4]])
ASC.extend([1-out_mask[4]])
ASC.extend([in_mask[5]])
ASC.extend([1-out_mask[5]])
ASC.extend([in_mask[6]])
ASC.extend([1-out_mask[6]])
ASC.extend([in_mask[7]])
ASC.extend([1-out_mask[7]])

# remaining bits are floating
ASC.extend([0]*53)

ASC[:] = [int(1-x) for x in ASC] # invert the scan chain :)

#ASC = [0,0,0,0]*300;

#  BEGIN SERIAL SECTION
# ---------------------------------------------------
#com_port = '/dev/cu.usbmodem3954801';
com_port = '/dev/cu.usbmodem4943141';
#com_port = '/dev/cu.usbmodem4002061';
#com_port = '/dev/cu.usbmodem4702721';

# Open COM port to teensy to bit-bang scan chain
ser = serial.Serial(
    port=com_port,
    baudrate=19200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

## Transmit pcb config to microcontroller
ser.write(b'boardconfig\n');

vdd_tune = vdda_pcb_tune + vddd_pcb_tune;
ser.write((str(int(''.join(map(str,vdd_tune)),2))+'\n').encode())
ser.write((str(int(''.join(map(str,gpio_direction)),2))+'\n').encode())
ser.write((str(int(''.join(map(str,ldo_enables)),2))+'\n').encode())

# ASC configuration
time.sleep(0.1)

ser.write(b'clockon\n');

# Convert array to string for uart
ASC_string = ''.join(map(str,ASC));

ASC_test = [0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1, \
			0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,0,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,0,0,1,1,0,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,0,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, \
			0,1,1,1,1,1,1,0,1,1,1,1,0,1,1,1,0,0,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,0,0,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,0,1,1,0,1,1,1,0,1,1,0,1,1,0,1,1,0,1,0,1,1,0,0,0,0,0,0,0,0,1,1,0,0,0, \
			0,0,1,1,1,1,1,0,0,1,1,1,1,1,1,0,0,0,1,0,0,1,1,1,1,1,0,1,1,1,1,1,0,1,0,0,1,1,1,0,1,1,1,0,0,1,1,1,1,1,1,0,0,0,0,0,1,0, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,1,0,0,1,1,1,1,1,1,1,0,0,0,1,1,0,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,0,0,0,0,0,0,1,1,1,1,0,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,1,1,1, \
			1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,0,0,0,0,1,1,0,0,0,0,1,1,0,0,0,0,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,0,0,0,0,0,0,0,1,0,0,1,0,1,0,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,0,1,0,0,0,1,1,1,1,1,1,0,1,1,1,1,1,1,1,0,0, \
			0,0,1,1,1,0,0,0,0,0,0,0,1,1,1,1,0,1,1,0,0,0,0,1,1,0,1,1,0,1,0,0,1,1,1,1,0,1,1,1,1,0,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1, \
			0,0,1,0,0,0,0,1,1,0,0,1,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,0,1,0,1,0,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, \
			1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1];

ASC_test_string = ''.join(map(str,ASC_test))

# Send string to uC and program into IC
ser.write(b'ascwrite\n');
print(ser.readline());
#ser.write((ASC_test_string[::-1]).encode()); # fliplr to feed bits in the correct direction
ser.write((ASC_string[::-1]).encode());
print(ser.readline());

# Execute the load command to latch values inside chip
ser.write(b'ascload\n');
print(ser.readline());

# Read back the scan chain contents
ser.write(b'ascread\n');
x = ser.readline();
scan_out = x[::-1];
print(ASC_string)
#print(ASC_test_string)
print(scan_out.decode())

# Compare what was written to what was read back
if int(ASC_string) == int(scan_out.decode()):
    print('Read matches Write')
else:
    print('Read/Write Comparison Incorrect')

ser.close();