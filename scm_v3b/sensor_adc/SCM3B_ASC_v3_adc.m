% v1, BW: initial version
% 
% v2, 2018 nov 27, DB: replaced ASC(1:90) and added "Freq selection crossbar section
%
% v3, 2018 dec 05, DB: cleaned up sensor ADC and power control sections.
% Had to add '%d' to x2= line in "For printing out ASC configuration"
% section.

close all
clear all
clc

% SCM GPIO direction control
% 0 = input to SCM, 1 = output from SCM
% [0:15]
gpio_direction = [0 0 0 0  1 1 1 1  1 1 1 1  1 1 1 1];

%%-----------
% Begin ASC
ASC=zeros(1,1200); % initialize analog scanchain

%% internal section, uses ASD 0:270. copied from scum-3 tapeout google doc, "scan registers" tab.

% zeroth ASC bit is shifted in at the end to prevent more off-by-one errors
% than there will be in this file already


ASC(1)=1;	%  sel_data     , Recovered data, 1 is for ZCC and 0 is for other baseband
ASC(2)=1;	%  sel_counter0_reset, sel signal for counter 0 reset, 0 from ASC[8] and 1 from M0 analog_cfg[0]
%  sel signal for counter 0 enable, 0 from ASC[15] and 1 from M0 analog_cfg[7]
ASC(3)=1;	%  sel_counter1_reset, sel signal for counter 1 reset, 0 from ASC[9] and 1 from M0 analog_cfg[1]
%  sel signal for counter 1 enable, 0 from ASC[16] and 1 from M0 analog_cfg[8]
ASC(4)=1;	%  sel_counter2_reset, sel signal for counter 2 reset, 0 from ASC[10] and 1 from M0 analog_cfg[2]
%  sel signal for counter 2 enable, 0 from ASC[17] and 1 from M0 analog_cfg[9]
ASC(5)=1;	%  sel_counter3_reset, sel signal for counter 3 reset, 0 from ASC[11] and 1 from M0 analog_cfg[3]
%  sel signal for counter 3 enable, 0 from ASC[18] and 1 from M0 analog_cfg[10]
ASC(6)=1;	%  sel_counter4_reset, sel signal for counter 4 reset, 0 from ASC[12] and 1 from M0 analog_cfg[4]
%  sel signal for counter 4 enable, 0 from ASC[19] and 1 from M0 analog_cfg[11]
ASC(7)=1;	%  sel_counter5_reset, sel signal for counter 5 reset, 0 from ASC[13] and 1 from M0 analog_cfg[5]
%  sel signal for counter 5 enable, 0 from ASC[20] and 1 from M0 analog_cfg[12]
ASC(8)=1;	%  sel_counter6_reset, sel signal for counter 6 reset, 0 from ASC[14] and 1 from M0 analog_cfg[6]
%  sel signal for counter 6 enable, 0 from ASC[21] and 1 from M0 analog_cfg[13]
ASC(552)=0;	%  TIMER32K (counter0) counter, 0 is reset
ASC(9)=0;	%  LF_CLOCK (counter1) counter, 0 is reset
ASC(10)=0;	%  HF_CLOCK (counter2) counter, 0 is reset
ASC(11)=0;	%  RC_2MHz (counter3) counter, 0 is reset
ASC(12)=0;	%  LF_ext_GPIO (counter4) counter, 0 is reset
ASC(13)=0;	%  LC_div_N (counter5) counter, 0 is reset
ASC(14)=0;	%  ADC_CLK (counter6) counter, 0 is reset
ASC(15)=0;	%  counter0_enable, 1 is enable
ASC(16)=0;	%  counter1_enable, 1 is enable
ASC(17)=0;	%  counter2_enable, 1 is enable
ASC(18)=0;	%  counter3_enable, 1 is enable
ASC(19)=0;	%  counter4_enable, 1 is enable
ASC(20)=0;	%  counter5_enable, 1 is enable
ASC(21)=0;	%  counter6_enable, 1 is enable
ASC(22:23)=[0 0];	%  sel_mux3in, 00 LF_CLOCK, 01 HF_CLOCK, 10 LF_ext_PAD, 11 1'b0
ASC(24)=0;	%  divider_RFTimer_enable, 0 is enable
ASC(25)=0;	%  divider_CortexM0_enable, 0 is enable, HCLK
ASC(26)=0;	%  divider_GFSK_enable, 0 is enable, divider_out_GFSK
ASC(27)=0;	%  divider_ext_GPIO_enable, 0 is enable, EXT_CLK_GPIO
ASC(28)=0;	%  divider_integ_enable, 0 is enable,
ASC(29)=0;	%  divider_2MHz_enable, 0 is enable,
ASC(30)=0;	%  divider_RFTimer_resetn, 1 is reset
ASC(31)=0;	%  divider_CortexM0_resetn, 1 is reset
ASC(32)=0;	%  divider_GFSK_reset, 1 is reset
ASC(33)=0;	%  divider_ext_GPIO_resetn, 1 is reset
ASC(34)=0;	%  divider_integ_resetn, 1 is reset
ASC(35)=0;	%  divider_2MHz_resetn, 1 is reset
ASC(36)=0;  %  divider_RFTimer_PT, 1 is passthrough
ASC(37)=0;	%  divider_CortexM0_PT, 1 is passthrough
ASC(38)=0;	%  divider_GFSK_PT, 1 is passthrough
ASC(39)=0;	%  divider_ext_GPIO_PT, 1 is passthrough
ASC(40)=0;	%  divider_integ_PT, 1 is passthrough
ASC(41)=0;	%  divider_2MHz_PT, 1 is passthrough

% Divider codes are all inverted, so 00000000 is 11111111 (maximum divide ratio)
% Exception is CortexM0_Nin aka HCLK, 00000000 is 00000100 (divide by 4)
ASC(49:-1:42)=[0 0 0 0 0 0 0 0];	%  divider_RFTimer_Nin, divide value
ASC(57:-1:50)=[0 0 0 0 0 0 0 0];	%  divider_CortexM0_Nin, divide value
ASC(65:-1:58)=[0 0 0 0 0 0 0 0];	%  divider_GFSK_Nin, divide value
ASC(73:-1:66)=[0 0 0 0 0 0 0 0];	%  divider_ext_GPIO_Nin, divide value
ASC(81:-1:74)=[0 0 0 0 0 0 0 0];	%  divider_integ_Nin, divide value
ASC(89:-1:82)=[0 0 0 0 0 0 0 0];	%  divider_2MHz_Nin, divide value

ASC(90)=0;	%  mux_sel_RFTIMER, 0 is divider_out_RFTIMER, 1 is TIMER32k
ASC(91)=0;	%  mux_sel_GFSK_CLK, 0 is LC_div_N, 1 is divider_out_GFSK
ASC(93:-1:92)=[0 0];	%  mux3_sel_CLK2MHz, 00 RC_2MHz, 01 divider_out_2MHz, 10 LC_2MHz, 11 1'b0
ASC(95:-1:94)=[0 0];	%  mux3_sel_CLK1MHz, 00 LC_1MHz_dyn, 01 divider_out_2MHz, 10 LC_1MHz_stat, 11 1'b0
ASC(97:-1:96)=[0 0];	%  IQ_select, Bob's digital baseband
ASC(99:-1:98)=[0 0];	%  op_mode, Bob's digital baseband
ASC(100)=0;	%  agc_overload, Bob's digital baseband
ASC(101)=0;	%  agc_ext_or_int, Bob's digital baseband
ASC(102)=0;	%  vga_select, Bob's digital baseband
ASC(103)=0;	%  mf_data_sign, Bob's digital baseband

ASC(106:-1:104)=[0 0 0];	%  mux sel for 7 counters output to GPIO bank 11
                            % 0 analog_rdata[31:0]
                            % 1 analog_rdata[63:32]
                            % 2 analog_rdata[95:64]
                            % 3 analog_rdata[127:96]
                            % 4 analog_rdata[159:128]
                            % 5 analog_rdata[191:160]
                            % 6 analog_rdata[223:192]
                            % 7 32'b0
                            
ASC(122:-1:107)=[0 0 0 0 0 0 0 0 0 0 0 0 1 1 1 0];	%  COUNT_THRESH, Brian's DBB. Count midpoint between high and low ?f
ASC(123)=0;	%  FB_EN, Brian's DBB. 
ASC(131:-1:124)=[0 0 1 0 0 1 0 1];	%  CLK_DIV_ext, Brian's DBB. Receiver clock divide ratio to get to 2MHz
ASC(132)=1;	%  DEMOD_EN, Brian's DBB
ASC(143:-1:133)=[0 0 0 0 0 0 0 0 0 0 0];	%  INIT_INTEG, Brian's DBB
ASC(159:-1:144)=0;	
%  ASC(144) sel signal for HCLK divider2 maxdivenb between Memory mapped vs
%	ASC(145) mux in0, 0 ASC[145] 1 analog_cfg[14]
ASC(175:-1:160)=0;	%  Not used
ASC(191:-1:176)=0;	%  Not used
ASC(192)=0;	%  sel_FREQ_CONTR_WORD, Brian's DBB
ASC(208:-1:193)=[0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0];	%  SACLEINT_ext, Brian's DBB
ASC(224:-1:209)=[0 0 0 0 0 0 0 0 0 1 0 0 0 1 0 0];	%  EAERLY_DECISION_MARGIN_ext, Brian's DBB
ASC(226:-1:225)=[0 0];	%  mux_sel_FREQ_CTRL_WORD_SCAN
                        % 0 ASC[237:227]
                        % 1 FREQ_CTRL_WORD_SCAN
                        % 2 analog_cfg[74:64]
                        % 3 11'b0
ASC(237:-1:227)=[0 0 0 0 0 0 0 0 0 0 0];	%  FREQ_CTRL_WORD_SCAN
ASC(238)=1;	%  sel dbb Ring data in, 0 SIG_IN (LC osc, Q branch), 1 COMPP_ZCC (LC osc, I branch) //choose inputs to brians zcc demod
ASC(239)=1;	%  sel dbb Ring CLK in, 0 ZCC_CLK_Ring (LC osc, Q branch), 1 ZCC_CLK (LC osc, I branch)
ASC(240)=1;	%  mux sel for RST_B, 0 ASC[241], 1 analog_cfg[75]
ASC(241)=1;	%  ASC RST_B. Reset ZCC demod.

% Sensor ADC control inputs: 0 from digital state machine, 1 from GPI
ASC(242)=1;	%  mux sel ADC reset, 0 adc_reset, 1 adc_reset_gpi
ASC(243)=1;	%  mux sel ADC convert, 0 adc_convert, 1 adc_convert_gpi
ASC(244)=1;	%  mux sel ADC pga amplify, 0 adc_pga_amplify, 1 adc_pga_amplify_gpi

% GPIO output mux select
ASC(248:-1:245)=[1 0 0 1];
ASC(252:-1:249)=[0 0 0 0];
ASC(256:-1:253)=[0 0 0 0];
ASC(260:-1:257)=[0 0 0 0];
%    ASC(248:-1:245)=dec2bin(6,4)-48;	%  GPO row 1 sel
%    ASC(252:-1:249)=dec2bin(9,4)-48;	%  GPO row 2 sel
%    ASC(256:-1:253)=dec2bin(9,4)-48;	%  GPO row 3 sel
%    ASC(260:-1:257)=dec2bin(9,4)-48;	%  GPO row 4 sel

% GPIO input mux select
ASC(262:-1:261)=[1 1];	%  GPI row 1 sel
ASC(264:-1:263)=[0 0];	%  GPI row 2 sel
ASC(266:-1:265)=[0 0];	%  GPI row 3 sel
ASC(268:-1:267)=[0 0];	%  GPI row 4 sel

% Select whether data/clk going to radio FSM come from pads or internal
ASC(269)=0;	%  mux sel for M0 clk, 0 dbbclk, 1 DATA_CLK_IN
ASC(270)=0;	%  mux sel for M0 data, 0 dbbdata, 1 DATA_IN

%% ASC 271:516 -- LC IF Chain
%%----------------------------
%% Q channel gain control
agc_gain_mode = 0;                              % 0 = Gain is controlled via scan chain, 1 = controlled by AGC

code_scan_I = (de2bi(63,6));
code_scan_Q = fliplr((de2bi(63,6)));

stg3_gm_tune_I = fliplr([1 1 1 1 1 1 0 0 0 0 0 0 0]);     % Gain control for AGC driver <1:13>, all ones is max gain, thermometer (not 1-hot)
stg3_gm_tune_Q = [1 1 1 1 1 1 0 0 0 0 0 0 0];     % Gain control for AGC driver <1:13>, all ones is max gain, thermometer (not 1-hot)

% Debug control at output of Q mixer
dbg_bias_en_Q = 0;                              % 1 = Turn on bias for source followers
dbg_out_en_Q = 0;                               % 1 = Turn on output pass gate for debug
dbg_input_en_Q = 0;                             % 1 = Turn on input pass gate for debug

% Mixer bias control
mix_bias_In_ndac = [0 0 0 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_i = 0;                                  % 0 = I mixer enabled
mix_bias_Ip_ndac = [0 0 0 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_bias_Qn_ndac = [0 0 0 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage
mix_off_q = 0;                                 % 0 = I mixer enabled
mix_bias_Qp_ndac = [0 0 0 1];                   % Mixer gate bias control, thermometer, <3:0>, all 1s = lowest bias voltage

mix_bias_In_pdac = [1 0 0 0];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Ip_pdac = [1 0 0 0];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qn_pdac = [1 0 0 0];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage
mix_bias_Qp_pdac = [1 0 0 0];                   % Mixer gate bias control, binary weighted, <4(MSB):1>, all 0s = highest bias voltage

IFsection1 = [agc_gain_mode code_scan_Q stg3_gm_tune_Q dbg_bias_en_Q dbg_out_en_Q dbg_input_en_Q ...
    mix_bias_In_ndac mix_off_i mix_bias_Ip_ndac mix_bias_Qn_ndac mix_off_q ...
    mix_bias_Qp_ndac mix_bias_In_pdac mix_bias_Ip_pdac mix_bias_Qn_pdac mix_bias_Qp_pdac];

%% Q channel
tia_cap_on = [0 0 0 0];                         % TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = 0;                               % 1 = pull TIA inputs to ground
tia_enn = 1;                                    % 1 = enable nmos bias in TIA
tia_enp = 0;                                    % 0 = enable pmos bias in TIA

% Q Channel comparator offset trim
pctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator
nctrl = [0 0 0 0 0];                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator

% Q channel comparator control
mode_1bit = 0;                                  % 1 = turn on zero crossing mode
adc_comp_en = 0;                                % Clock gate for comparator, 0 = disable the clock

% Q channel stage3 amp control
stg3_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg3
dbg_out_on_stg3 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg3 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg3 = 0;                           % 1 = Turn on bias for source followers

% Q channel stage2 amp control
stg2_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg2
dbg_out_on_stg2 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg2 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg2 = 0;                           % 1 = Turn on bias for source followers

% Q channel stage1 amp control
stg1_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of Q stg1
dbg_out_on_stg1 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg1 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg1 = 0;                           % 1 = Turn on bias for source followers

IFsection2 = [tia_cap_on tia_pulldown tia_enn tia_enp pctrl nctrl mode_1bit adc_comp_en ...
    stg3_amp_en dbg_out_on_stg3 dbg_input_on_stg3 dbg_bias_en_stg3 ...
    stg2_amp_en dbg_out_on_stg2 dbg_input_on_stg2 dbg_bias_en_stg2 ...
    stg1_amp_en dbg_out_on_stg1 dbg_input_on_stg1 dbg_bias_en_stg1];

%% Q channel ADC control
vcm_amp_en = 0;                                 % 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = [0 0];                           % <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = 0;                                 % 1 = enable clock for Vcm SC divider
vref_clk_en = 0;                                % 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1 1];                          % <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = 0;                                % 1 = enable bias for Vref buffer amp
adc_fsm_en = 0;                                 % 1 = enable the adc fsm
adc_dbg_en = 0;                                 % 1 = enable adc debug outputs  (bxp, compp)

IFsection3 = [vcm_amp_en vcm_vdiv_sel vcm_clk_en vref_clk_en vref_vdiv_sel vref_amp_en adc_fsm_en adc_dbg_en];

%% Q channel filter
stg3_clk_en = 0;                                % 1 = enable clock for IIR
stg3_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = 0;                                % 1 = enable clock for IIR
stg2_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = 0;                                % 1 = enable clock for IIR
stg1_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection4 = [stg3_clk_en stg3_C2 stg3_C1 stg2_clk_en stg2_C2 stg2_C1 stg1_clk_en stg1_C2 stg1_C1];

%% I channel ADC control
vcm_amp_en = 0;                                 % 1 = enable bias for Vcm buffer amp
vcm_vdiv_sel = [0 0];                           % <0:1> switch cap divider ratio, 00 = ~400mV
vcm_clk_en = 0;                                 % 1 = enable clock for Vcm SC divider
vref_clk_en = 0;                                % 1 = enable clock for Vref SC divider
vref_vdiv_sel = [1 1];                          % <1:0> switch cap divider ratio, 11 = ~50mV, 10 = ~100mV
vref_amp_en = 0;                                % 1 = enable bias for Vref buffer amp
adc_fsm_en = 0;                                 % 1 = enable the adc fsm
adc_dbg_en = 0;                                 % 1 = enable adc debug outputs (bxp, compp)

IFsection5 = [vcm_amp_en vcm_vdiv_sel vcm_clk_en vref_clk_en vref_vdiv_sel vref_amp_en adc_fsm_en adc_dbg_en];

%% I channel filter
stg3_clk_en = 0;                                % 1 = enable clock for IIR
stg3_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg3_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg2_clk_en = 0;                                % 1 = enable clock for IIR
stg2_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg2_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

stg1_clk_en = 0;                                % 1 = enable clock for IIR
stg1_C2 = [0 0 0];                              % C2 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap
stg1_C1 = [0 0 0];                              % C1 control for IIR <3(MSB):1>, binary weighted, all 1s = most cap

IFsection6 = [stg3_clk_en stg3_C2 stg3_C1 stg2_clk_en stg2_C2 stg2_C1 stg1_clk_en stg1_C2 stg1_C1];

%% Clock generation control
adc_dbg_en = 0;                                 % 1 = enable output of debug phases for adc clocks (cx,phix)                      
adc_phi_en = 0;                                 % 1 = enable clocks for adc (cx,phix)
filt_phi_en = 0;                                % 1 = enable 4 filter clock phases (0,90,180,270)
clk_select = [0 1];                             % <1:0> mux select for IF clock 00=gnd 01=internal RC 10=divided LC 11=external pad
RC_clk_en = 0;                                  % 1 = enable RC osc

coarse = 31;
fine = 18;
RC_coarse = de2bi(coarse,5,'left-msb');                        % coarse tuning for RC, binary weighted <4(MSB):0>
filt_dbg_en = 0;                                % 1 = enable debug outputs for 4 filter clock phases (0,90,180,270)
RC_fine = de2bi(fine,5,'left-msb');                          % fine tuning for RC, binary weighted <4(MSB):0>

% 1 = enable high speed mode for IF RC osillator
ASC(726)=0; 

IFsection7 = [adc_dbg_en adc_phi_en filt_phi_en clk_select RC_clk_en RC_coarse filt_dbg_en RC_fine];

%% I channel
tia_cap_on = [0 0 0 0];                         % TIA bandwidth <4(MSB):1>, binary weighted, all ones = lowest bandwidth                       
tia_pulldown = 0;                               % 1 = pull TIA inputs to ground
tia_enn = 1;                                    % 1 = enable nmos bias in TIA
tia_enp = 0;                                    % 0 = enable pmos bias in TIA

% I Channel comparator offset trim
pctrl = fliplr([0 0 0 0 0]);                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator
nctrl = fliplr([0 0 0 0 0]);                            % <4(MSB):0>, binary weighted, increase from 0 to add cap to either side of comparator

% I channel comparator control
mode_1bit = 0;                                  % 1 = turn on zero crossing mode
adc_comp_en = 0;                                % Clock gate for comparator, 0 = disable the clock

% I channel stage3 amp control
stg3_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg3
dbg_out_on_stg3 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg3 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg3 = 0;                           % 1 = Turn on bias for source followers

% I channel stage2 amp control
stg2_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg2
dbg_out_on_stg2 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg2 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg2 = 0;                           % 1 = Turn on bias for source followers

% I channel stage1 amp control
stg1_amp_en = 0;                                % Bias enable for folded cascode, 1 = bias on
% Control for debug point at input of I stg1
dbg_out_on_stg1 = 0;                            % 1 = Turn on output pass gate for debug
dbg_input_on_stg1 = 0;                          % 1 = Turn on input pass gate for debug
dbg_bias_en_stg1 = 0;                           % 1 = Turn on bias for source followers

IFsection8 = [dbg_bias_en_stg1 dbg_input_on_stg1 dbg_out_on_stg1 stg1_amp_en ...
    dbg_bias_en_stg2 dbg_input_on_stg2 dbg_out_on_stg2 stg2_amp_en ...
    dbg_bias_en_stg3 dbg_input_on_stg3 dbg_out_on_stg3 stg3_amp_en ...
    adc_comp_en mode_1bit nctrl pctrl tia_enp tia_enn tia_pulldown tia_cap_on];


%% I channel
% Debug control at output of I mixer
dbg_input_en_I = 0;                             % 1 = Turn on input pass gate for debug
dbg_out_on_I = 0;                               % 1 = Turn on output pass gate for debug
dbg_bias_en_I = 0;                              % 1 = Turn on bias for source followers

% I channel gain control
agc_gain_mode = 0;                              % 0 = Gain is controlled via scan chain, 1 = controlled by AGC

IFsection9 = [dbg_input_en_I dbg_out_on_I dbg_bias_en_I stg3_gm_tune_I code_scan_I agc_gain_mode];


%% LDO control
if_ldo_rdac = [0 0 0 0 0 0 0];                  % Resistor for setting bandgap ref voltage <0:6(MSB)>
por_disable = 0;                                % 1 = disable ability for POR to reset IF blocks
scan_reset = 1;                                 % 0 = force reset from scan chain

IFsection10 = [if_ldo_rdac por_disable scan_reset];


%% Compile into one vector
IF_scan = [IFsection1 IFsection2 IFsection3 IFsection4 IFsection5 IFsection6 IFsection7 IFsection8 IFsection9 IFsection10];

%% Power on control module
% scan_pon_XX turns on LDO via scan chain
% gpio_pon_en_XX will turn on LDO if power on signal from GPIO bank is high
% fsm_pon_en_XX will turn on LDO if power on signal from radio FSM is high
% master_ldo_en_XX = 0 forces LDO off; =1 allows control from gpio, fsm, or scan-chain
scan_pon_if = 0;
scan_pon_lo = 1;
scan_pon_pa = 0;
gpio_pon_en_if = 0;
fsm_pon_en_if = 0;
gpio_pon_en_lo = 1;
fsm_pon_en_lo = 1;
gpio_pon_en_pa = 0;
fsm_pon_en_pa = 0;
master_ldo_en_if = 0;
master_ldo_en_lo = 1;
master_ldo_en_pa = 0;
scan_pon_div = 0;
gpio_pon_en_div = 0;
fsm_pon_en_div = 0;
master_ldo_en_div = 0;
power_control = [scan_pon_if scan_pon_lo scan_pon_pa ...
    gpio_pon_en_if fsm_pon_en_if ...
    gpio_pon_en_lo fsm_pon_en_lo ...
    gpio_pon_en_pa fsm_pon_en_pa ...
    master_ldo_en_if master_ldo_en_lo master_ldo_en_pa ...
    scan_pon_div gpio_pon_en_div fsm_pon_en_div master_ldo_en_div];


% Entire LC IF sigpath
ASC(271:516) = [IF_scan power_control];
%%----------------------------

%% Power and oscillator controls

ASC(623)=1; 		% 32k timer enable

% tune BGR voltage serving as VDDD LDO reference
ASC(791:1:797)=[1 1 0 0 0 0 0];

ASC(799)=0; % POR bypass in reset logic; low=not bypassed

% Mux for choosing turn on signal for aux ldo
% JIRA-101 Added inversion here so that the aux LDO defaults to enabled at cold start
% assign aux_ldo_enable_needs_levelshifted = ~(ASC_aux_ldo_enable_mux_select? analog_cfg[167] : ASC_aux_ldo_enable);
ASC(914)=0; % ASC_aux_ldo_enable_mux_select. 0 = controlled by ASC<916>, 1 = controlled by analog_cfg<167>
ASC(916)=0; % ASC_aux_ldo_enable

% adjust BGR reference voltage used by aux digital LDO
ASC(923:-1:917) = [1 1 0 0 0 0 0];

% adjust BGR reference voltage used by always on/analog scan LDO
ASC(924:929) = [0 0 0 0 0 0];  

% tune current & freq of primary system clock (20MHz ring)
ASC(932:940) = [0 0 0 0 0 0 0 0 0];
ASC(941)=1; % kickstart primary system clock (20MHz ring)

%% ************************************************************************
% sensor adc section
PGA_GAIN=dec2bin(1,8)-48; % PGA gain setting; 0 min gain setting
ASC(771:-1:766)=PGA_GAIN(1:6);
ASC(772)=0; % floating
ASC(800)=PGA_GAIN(7);
ASC(773)=PGA_GAIN(8);

ASC(1088)=1; % PGA_BYPASS set to 1 to disable PGA and switch t-gates to circumvent PGA

LDO_tune=dec2bin(1,7)-48; % temp sensor LDO reference voltage adjustment
ASC(778)=LDO_tune(1);
ASC(779:784)=LDO_tune(7:-1:2); 

ASC(765:-1:758)=dec2bin(2^7,8)-48;	% temp sensor LDO IREF constant-gm resistor; adjusts comparator and PGA amp bias current

ASC(798)=0; % enable VBATT/4 voltage divider
ASC(801)=1; % enable temp sensor LDO

MUX_SEL=[1 0]; % PGA/ADC input mux. 0=VPTAT, 1=VBAT/4, 2=external pad, 3=n/a, floating;
ASC(1087)=MUX_SEL(2); % LSB of PGA input mux
ASC(915)=MUX_SEL(1); % MSB of PGA input mux

ASC(823:-1:816)=dec2bin(2^7,8)-48; % ADC_SETTLE; adjust replica dac that controls time taken for ADC capdac to settle

%% ************************************************************************

%%  LC LO Parameters
%% -----------------------------------------------
lo_fine_tune = 31;
lo_mid_tune = 16;
lo_coarse_tune = 24;
lo_current = 127;
pa_panic = 0;
pa_ldo = 0;
lo_panic = 1;
lo_ldo = 0;
div_panic = 0;
div_ldo = 0;
sel_12 = 1;
en_backup_2 = 0;
en_backup_5 = 1;
pre_dyn_sel = 0;
en_64mhz = 0;
en_20mhz = 0;
static_code = 244;
static_en = 1;
bg_panic = 0;
bg = 0;
lo_tune_select = 1; %0 for cortex, 1 for scan chain
polyphase_enable = 0; % 0 disables, 1 enables


%  LC LO frequency tuning and RX/TX
fine_code =   lo_fine_tune;
mid_code =    lo_mid_tune;
coarse_code = lo_coarse_tune;
fine_tune = [fliplr(de2bi(fine_code,5)) 0];
mid_tune = [fliplr(de2bi(mid_code,5)) 0];
coarse_tune = [fliplr(de2bi(coarse_code,5)) 0];
lo_current_tune = fliplr(de2bi(lo_current,8));

%  test BG for visibility:
%test_bg = [1 0 1 1 1 1 1]; % panic bit -> msb -> lsb
test_bg = [bg_panic fliplr(de2bi(bg,6))];

%  LDO settings:
pa_ldo_rdac = [pa_panic fliplr(de2bi(pa_ldo,6))];
lo_ldo_rdac = [lo_panic fliplr(de2bi(lo_ldo,6))];
div_ldo_rdac = [div_panic fliplr(de2bi(div_ldo,6))];

%  modulation settings
mod_logic = [0 0 0 0]; % msb -> lsb, s3:0 to not invert, 1 to invert
                       %             s2:0 cortex, 1 pad
                       %             s0,s1: choose between pad, cortex,
                       %             vdd, gnd
mod_15_4_tune = [1 0 0]; %msb -> lsb
mod_15_4_tune_d = 1; % spare dummy bit

%  divider settings
sel_1mhz_2mhz = sel_12; % 0 is for 2mhz, 1 is for 1mhz
pre_dyn_dummy_b = 0; % disconnected
pre_dyn_b = [0 0 0 0 0 0]; % disconnected
pre_dyn_en_b = 0; % disconnected
pre_2_backup_en = en_backup_2; % enable the backup div-by-2 pre-scaler
pre_2_backup_en_b = 1-pre_2_backup_en; % disable the div-by-2 pre-scaler
pre_5_backup_en = en_backup_5; % enable the backup div-by-5 pre-scaler
pre_5_backup_en_b = 1-pre_5_backup_en; % disable the backup div-by-5 pre-scaler
pre_dyn = [1 1 1 0 0 0];
if(pre_dyn_sel == 0)
    pre_dyn = [1 1 1 0 0 0]; % by-5, by-2, by-5, disconnected, disconnected, disconnected
elseif(pre_dyn_sel == 1)
    pre_dyn = [0 1 1 0 0 0];
elseif(pre_dyn_sel == 2)
    pre_dyn = [1 0 1 0 0 0];
elseif(pre_dyn_sel == 3)
    pre_dyn = [1 1 0 0 0 0];
end
pre_dyn_dummy = 0; % disconnected
div_64mhz_enable = en_64mhz; % enable the 64mhz clock source
div_20mhz_enable = en_20mhz; % enable the 20mhz clock source
div_static_code = static_code; % blaze it
div_static_code_bin = de2bi(div_static_code,16); % in lsb->msb order
div_static_rst_b = 1; % reset (active low) of the static divider
div_static_en = static_en; % static divider enable
div_static_select = [   div_static_code_bin(11:16), ...
                        div_static_code_bin(5:10), ...
                        div_static_en, ...
                        div_static_rst_b, ...
                        div_static_code_bin(1:4)    ];
div_static_select = 1 - div_static_select;
                    
dyn_div_N = 420; % joke's not funny anymore
div_dynamic_select = de2bi(dyn_div_N,13);
div_dynamic_en_b = 1; % disable the dynamic divider
div_dynamic_select = [div_dynamic_select div_dynamic_en_b];
div_tune_select = 1; % 0 for cortex, 1 for scan
source_select_2mhz = [0 0]; % 00 static, 01 dynamic, 10 from aux, 11 pad

% BLE module settings - I have forgotten what most of these do
scan_io_reset = 1;
scan_5mhz_select = [0 1]; % msb -> lsb
scan_1mhz_select = [0 0]; % msb -> lsb
scan_20mhz_select = [0 1]; % msb -> lsb
scan_async_bypass = 1;
scan_mod_bypass = 1;
scan_fine_trim = [0 0 0 0 0 0]; % msb -> lsb
scan_data_in_valid = 1;
scan_ble_select = 0;

ASC(946:978) = [fine_tune mid_tune ...
                coarse_tune ...
                lo_tune_select ...
                test_bg(2:end) ...
                polyphase_enable ...
                pa_ldo_rdac];
ASC(979) = 0;                               % unused
ASC(980:986) = [lo_ldo_rdac];
ASC(987) = 0;                               % unused
ASC(988:1010) = [lo_current_tune ...
                mod_logic ...
                fliplr(mod_15_4_tune) ...
                mod_15_4_tune_d ...
                div_ldo_rdac ...
                ];
ASC(1011) = 0;
ASC(1012:1086) = [sel_1mhz_2mhz ...
                  pre_dyn_dummy_b ...
                  pre_dyn_b ...
                  pre_dyn_en_b ...
                  pre_2_backup_en_b ...
                  pre_2_backup_en ...
                  pre_5_backup_en ...
                  pre_5_backup_en_b ...
                  pre_dyn ...
                  pre_dyn_dummy ...
                  div_64mhz_enable ...
                  div_20mhz_enable ...
                  scan_io_reset ...
                  scan_5mhz_select ...
                  scan_1mhz_select ...
                  scan_20mhz_select ...
                  scan_async_bypass ...
                  scan_mod_bypass ...
                  scan_fine_trim ...
                  div_static_select ...
                  div_dynamic_select ...
                  div_tune_select ...
                  test_bg(1) ...
                  source_select_2mhz ...
                  scan_data_in_valid ...
                  scan_ble_select
                  ];

% TX chip clock - 2M RC
ASC(1089:1093) = [1 1 1 1 1]; % coarse 1    lsb:msb
ASC(1094:1098) = [1 1 1 1 1]; % coarse 2
ASC(1099:1103) = [1 1 1 1 1]; % coarse 3
ASC(1104:1108) = [0 1 1 0 1]; % fine
ASC(1109:1113) = [0 0 0 0 1]; % superfine
ASC(1114) = 1; % 2MHz RC enable

%% Freq selection crossbar

% crossbar selections are like so:
% 	.in0(LF_CLOCK),
% 	.in1(HF_CLOCK),
% 	.in2(RC_2MHz),
% 	.in3(TIMER32k),
% 	.in4(LF_ext_PAD),
% 	.in5(LF_ext_GPIO),
% 	.in6(ADC_CLK),
% 	.in7(LC_div_N),
% 	.in8(LC_2MHz),
% 	.in9(LC_1MHz_stat),
% 	.in10(LC_1MHz_dyn),
% 	.in11(1'b0),
% 	.in12(1'b0),
% 	.in13(1'b0),
% 	.in14(1'b0),
% 	.in15(1'b0),

ASC(1150:-1:1147) = [0 0 0 1]; % crossbar_HCLK
% if surface bypass pad is tied high, ASC[1147] is inverted
% assign cross_HCLK_sel_LSB = ADC_CLK_Ring ? ~ASC_crossbar[1147] : ASC_crossbar[1147];

ASC(1154:-1:1151) = [0 0 0 0]; % crossbar_RFTimer
ASC(1158:-1:1155) = [0 0 0 0]; % crossbar_TX_chip_clk_to_cortex
ASC(1162:-1:1159) = [0 0 0 0]; % crossbar_symbol_clk_ble
ASC(1166:-1:1163) = [0 0 0 0]; % crossbar_divider_out_INTEG
ASC(1170:-1:1167) = [0 0 0 0]; % crossbar_GFSK_CLK
ASC(1174:-1:1171) = [0 0 0 0]; % crossbar_EXT_CLK_GPIO
ASC(1178:-1:1175) = [0 0 0 0]; % crossbar_EXT_CLK_GPIO2
ASC(1182:-1:1179) = [0 0 0 0]; % crossbar_BLE_PDA

ASC(550) = 0; % control whether BLE packet assembler is clocked via divider or via analog_cfg
% assign ble_packetassembler_clock = ASC_divider_ctrl[550] ? analog_cfg_ble[358] : BLE_PDA_clk;
ASC(551) = 0; % control whether BLE packet disassembler is clocked via divider or via analog_cfg
% assign ble_pdatop1_io_clk_PDA = ASC_divider_ctrl[551] ? BLE_PDA_clk : analog_cfg_ble[402];
ASC(555:-1:554) = [0 0]; % control whether BLE CDR FIFO CLK is clocked via divider or via analog_cfg
% 	.in0(analog_cfg_ble[476]),
% 	.in1(dbbRingclkin),
% 	.in2(BLE_PDA_clk),
% 	.in3(ADC_CLK_EXT),
ASC(556) = 0; % control whether BLE ARM FIFO CLK is clocked via divider or via analog_cfg
% assign ble_pdatop1_io_clk_ARM = ASC_divider_ctrl[556] ? BLE_PDA_clk : analog_cfg_ble[475];

ASC(517)=0;	%  divider_symbol_clk_ble, 0 is enable
ASC(518)=0;	%  divider_BLE_PDA_clk, 0 is enable
ASC(519)=0;	%  divider_EXT_CLK_GPIO2, 0 is enable

ASC(520)=0;	%  divider_symbol_clk_ble, 1 is reset
ASC(521)=0;	%  divider_BLE_PDA_clk, 1 is reset
ASC(522)=0;	%  divider_EXT_CLK_GPIO2, 1 is reset

ASC(523)=0; %  divider_symbol_clk_ble, 1 is passthrough
ASC(524)=0;	%  divider_BLE_PDA_clk, 1 is passthrough
ASC(525)=0;	%  divider_EXT_CLK_GPIO2, 1 is passthrough

ASC(533:-1:526)=[0 0 0 0 0 0 0 0];	%  divider_symbol_clk_ble, divide value
ASC(541:-1:534)=[0 0 0 0 0 0 0 0];	%  divider_BLE_PDA_clk, divide value
ASC(549:-1:542)=[0 0 0 0 0 0 0 0];	%  divider_EXT_CLK_GPIO2, divide value

ASC(552)=0;	%  TIMER32K counter, 0 is reset


%% GPIO Direction Control
% 1 = enabled, 0 = disabled
% <0:15>
out_mask = gpio_direction;
% out_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0]; 
in_mask = -gpio_direction + 1;
% in_mask = [0 0 0 0  0 0 0 0  0 0 0 0  0 0 0 0];

% Map to ASC values, on-chip mapping is:
% out_en<0:15> = ASC<1131>,ASC<1133>,ASC<1135>,ASC<1137>,ASC<1140>,ASC<1142>,ASC<1144>,ASC<1146>,ASC<1115>,ASC<1117>,ASC<1119>,ASC<1121>,ASC<1124>,ASC<1126>,ASC<1128>,ASC<1130>				
% in_en<0:15> = ASC<1132>,ASC<1134>,ASC<1136>,ASC<1138>,ASC<1139>,ASC<1141>,ASC<1143>,ASC<1145>,ASC<1116>,ASC<1118>,ASC<1120>,ASC<1122>,ASC<1123>,ASC<1125>,ASC<1127>,ASC<1129>	
ASC(1131:2:1137) = 1 - out_mask(1:4);
ASC(1140:2:1146) = 1 - out_mask(5:8);
ASC(1115:2:1121) = 1 - out_mask(9:12);
ASC(1124:2:1130) = 1 - out_mask(13:16);
ASC(1132:2:1138) = in_mask(1:4);
ASC(1139:2:1145) = in_mask(5:8);
ASC(1116:2:1122) = in_mask(9:12);
ASC(1123:2:1129) = in_mask(13:16);


%% leftover bits
%ASC(1147:1199)=zeros(size(ASC(1147:1199))); % floating


%% shift in zeroth bit
sel_clk=1;              % select signal for recovered clock from clock recovery module, 0 is for LC , 1 is for Ring
ASC(1:1200) = [sel_clk, ASC(1:1199)];

% 
% %% For printing out ASC configuration
% for iii = 1:37  
%      x1 = (sprintf('%d-%d',(iii-1)*32,(iii-1)*32+31));
%     x2 = dec2hex(bin2dec(num2str(ASC((iii-1)*32+1:(iii-1)*32+32),'%d')),8);
%     disp(sprintf('ASC[%d] = 0x%s;   //%s',iii-1,x2,x1))
% end
% 
% x2 = dec2hex(bin2dec(num2str(ASC(1184:1200))),8);
% disp(sprintf('ASC[37] = 0x%s;',x2))
% 


%% Invert all bits due to inverter on chip
ASC=1-ASC;

%% Send to uC

% Open COM port 
s = serial('COM7','BaudRate',19200);         % Machine specific com port
s.OutputBufferSize = 2000; 
s.InputBufferSize = 2000; 
fopen(s);

%% ASC configuration
pause(0.1)

% Convert array to string for uart
ASC_string = sprintf('%d', ASC);

% Send string to uC and program into IC
fprintf(s,'ascwrite');
disp(fgets(s))
fprintf(s,fliplr(ASC_string)); % fliplr to feed bits in the correct direction
disp(fgets(s))

% Execute the load command to latch values inside chip
fprintf(s,'ascload');
disp(fgets(s));

% Read back the scan chain contents
fprintf(s,'ascread');
x = (fgets(s));
x = fliplr(x(1:1200));

% Compare what was written to what was read back
if(strcmp(ASC_string,x))
    disp('Read matches Write')
else
    disp('Read/Write Comparison Incorrect')
    sum(xor(double(x)-48,ASC))
end
fclose(s);

%%

s = serial('COM7','BaudRate',19200);         % Machine specific com port
s.OutputBufferSize = 2000; 
s.InputBufferSize = 2000; 
fopen(s);
fprintf(s,'togglehardreset');

fclose(s);

%%

s = serial('COM7','BaudRate',19200);         % Machine specific com port
s.OutputBufferSize = 2000; 
s.InputBufferSize = 2000; 
fopen(s);

fprintf(s,'sample_sensor_adc_debug');
adc_result = fgets(s);
watchdog = fgets(s);
disp([  adc_result(1:end-2) ' ' watchdog(1:end-2)]);

fclose(s);




