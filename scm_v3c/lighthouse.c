#include "lighthouse.h"

#include <math.h>
#include <rt_misc.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Memory_map.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"

#define QX3_FINE 0
#define QX3_MID 20
#define HCLOCK_ERROR 924 / 1000
extern char send_packet[127];

static bool lh_packet_ready;

// functions//
void initialize_mote_lighthouse() {
    extern unsigned int ASC[38];  // this is a little sketchy

    int t;
    lh_packet_ready = false;

    // RF Timer rolls over at this value and starts a new cycle
    RFTIMER_REG__MAX_COUNT = 0xFFFFFFFF;

    // Enable RF Timer
    RFTIMER_REG__CONTROL = 0x7;

    // Disable interrupts for the radio FSM
    radio_disable_interrupts();

    // Disable RF timer interrupts
    rftimer_disable_interrupts();

    //--------------------------------------------------------
    // SCM3C Analog Scan Chain Initialization
    //--------------------------------------------------------
    // Init LDO control
    init_ldo_control();

    // Set LDO reference voltages
    set_VDDD_LDO_voltage(63);
    // set_AUX_LDO_voltage(0);
    // set_ALWAYSON_LDO_voltage(0x60);
    set_ALWAYSON_LDO_voltage(120);
    // Select banks for GPIO inputs
    // GPI_control(0,0,0,0);

    // set gpi interrupts
    GPI_control(0, 0, 1, 0);

    // Select banks for GPIO outputs
    GPO_control(0, 6, 0, 0);

    // set optical pin input
    // GPI_enables(0x0008);

    // enable optical pin and external interrupt pins
    GPI_enables(0xFF08);

    // Set all GPIOs as outputs

    // GPO_enables(0xFFFF);

    // Set GPIOs except interrupts as outputs
    GPO_enables(0xD0FF);

    // Set HCLK source as HF_CLOCK
    set_asc_bit(1147);

    // Set initial coarse/fine on HF_CLOCK

    set_sys_clk_secondary_freq(HF_CLOCK_COARSE_LH, HF_CLOCK_FINE_LH);

    // Set RFTimer source as HF_CLOCK
    set_asc_bit(1151);

    // Disable LF_CLOCK
    set_asc_bit(553);

    // Set HCLK divider to 2
    clear_asc_bit(57);
    clear_asc_bit(56);
    clear_asc_bit(55);
    clear_asc_bit(54);
    clear_asc_bit(53);
    set_asc_bit(52);  // inverted
    set_asc_bit(51);
    clear_asc_bit(50);

    // Set RF Timer divider to pass through so that RF Timer is 20 MHz
    set_asc_bit(36);

    // Set 2M RC as source for chip CLK
    set_asc_bit(1156);

    // Enable 32k for cal// could be removed
    set_asc_bit(623);

    // Enable passthrough on chip CLK divider
    set_asc_bit(41);

    // Init counter setup - set all to analog_cfg control
    // ASC[0] is leftmost
    // ASC[0] |= 0x6F800000;
    for (t = 2; t < 9; t++) set_asc_bit(t);

    // Init RX
    // radio_init_rx_MF_lighthouse(); //could be removed

    // Init TX
    radio_init_tx_lighthouse(LO_SUPPLY_LH, LC_CURRENT_LH, PA_SUPPLY_LH, true,
                             true);

    // Set initial IF ADC clock frequency
    set_IF_clock_frequency(IF_COARSE_LH, IF_FINE_LH, 0);  // could be revomed

    // Set initial TX clock frequency
    set_2M_RC_frequency(31, 31, RC2M_COARSE_LH, RC2M_FINE_LH,
                        RC2M_SUPERFINE_LH);  // could be removed

    // Turn on RC 2M for cal
    set_asc_bit(1114);  // could be removed

    // Set initial LO frequency
    LC_monotonic(LC_CODE_LH);

    // Init divider settings
    // radio_init_divider(2000);

    // Program analog scan chain
    analog_scan_chain_write(&ASC[0]);
    analog_scan_chain_load();
    //--------------------------------------------------------
}

// initialize tx
// args: LO supply, LC supply current, PA supply voltage, lo cortex control,
// divider cortex control
void radio_init_tx_lighthouse(uint8_t lo_supply_v, uint8_t lc_supply_c,
                              uint8_t pa_supply_v, bool lo_cortex_ctrl,
                              bool div_cortex_ctrl) {
    // Set up 15.4 modulation source
    // ----
    // For FPGA, the TX modulation comes in at the external pad so need to set
    // the mod_logic mux to route this signal for modulation mod_logic<3:0> =
    // ASC<996:999> The two LSBs change the mux from cortex mod source to pad
    // The other bits are used for inverting the modulation bitstream
    // With these settings, the TX should start at +500 kHz above the channel
    // frequency A '1' data bit then causes the TX to decrease in frequency by 1
    // MHz (this generates proper 15.4 output) If for some reason you wanted to
    // start 500 kHz below the channel and step up by 1 MHz for a '1', then need
    // to change the settings here In the IC version, comment these two lines
    // out (they switch modulation source to the pad)
    // set_asc_bit(997);
    // set_asc_bit(996);
    // set_asc_bit(998);
    // set_asc_bit(999);
    // ----

    // Set 15.4 modulation tone spacing
    // ----
    // The correct tone spacing is 1 MHz.  This requires adjusting the cap DAC
    // in the TX The settings below are probably close enough
    // mod_15_4_tune<2:0> = ASC<1002:1000>
    set_asc_bit(1000);
    set_asc_bit(1001);
    set_asc_bit(1002);

    // set dummy bit to 1
    set_asc_bit(1003);
    // ----

    // If you need to adjust the tone spacing, turn on the LO and PA, and
    // uncomment the lines below one at a time to force the transmitter to each
    // of its two output tones Then adjust mod_15_4_tune until the spacing is
    // close to 1 MHz Note, I haven't tested this
    // -----------------
    // Force TX to output the 'high' FSK tone
    // set_asc_bit(999);
    // clear_asc_bit(998);

    // Force TX to output the 'low' FSK tone
    // clear_asc_bit(999);
    // set_asc_bit(998);
    // -----------------

    // Need to set analog_cfg<183> to 1 to select 15.4 for chips out
    ANALOG_CFG_REG__11 = 0x0080;

    // codes
    // LC: 100
    // PA_Supply: 63
    // LO_supply:64,0

    // Set current in LC tank
    set_LC_current(lc_supply_c);

    // Set LDO voltages for PA and LO
    set_PA_supply(pa_supply_v);
    set_LO_supply(lo_supply_v, 0);

    // LO cortex control:964
    if (lo_cortex_ctrl) {
        // Ensure cortex control of LO
        clear_asc_bit(964);
    }
    // divider cortex control: 1081
    if (div_cortex_ctrl) {
        // Ensure cortex control of divider
        clear_asc_bit(1081);
    }
}

void radio_init_rx_MF_lighthouse() {
    extern unsigned int ASC[38];  // this is a little sketchy
    // int j;
    unsigned int mask1, mask2;
    unsigned int tau_shift, e_k_shift, correlation_threshold;

    // IF uses ASC<271:500>, mask off outside that range
    mask1 = 0xFFFC0000;
    mask2 = 0x000007FF;
    ASC[8] &= mask1;
    ASC[15] &= mask2;

    // A large number of bits in the radio scan chain have no need to be changed
    // These values were exported from Matlab during radio testing
    // Same settings as used for 122418 ADC data captures
    ASC[8] |= (0x4050FFE0 & ~mask1);   // 256-287
    ASC[9] = 0x00422188;               // 288-319
    ASC[10] = 0x88040031;              // 320-351
    ASC[11] = 0x113B4081;              // 352-383
    ASC[12] = 0x027E8102;              // 384-415
    ASC[13] = 0x03ED4844;              // 416-447
    ASC[14] = 0x60010000;              // 448-479
    ASC[15] |= (0xFFE02E03 & ~mask2);  // 480-511

    // Set clock mux to internal RC oscillator
    clear_asc_bit(424);
    set_asc_bit(425);

    // Set gain for I and Q (63 is the max)
    set_IF_gain_ASC(63, 63);

    // Set gm for stg3 ADC drivers
    // Sets the transconductance for the third amplifier which drives the ADC
    // (7 was experimentally found to be about the best choice)
    set_IF_stg3gm_ASC(7, 7);  //(I, Q)

    // Set comparator trims
    // These allow you to trim the comparator offset for both I and Q channels
    // Shouldn't make much of a difference in matched filter mode, but can for
    // zero-crossing demod Only way to observe effect of trim is to adjust and
    // look for increase/decrease in packet error rate
    set_IF_comparator_trim_I(0, 0);  //(p,n)
    set_IF_comparator_trim_Q(0, 0);  //(p,n)

    // Setup baseband

    // Choose matched filter demod
    // ASC<0:1> = [0 0]
    clear_asc_bit(0);
    clear_asc_bit(1);

    // IQ source select
    // '0' = from radio
    // '1' = from GPIO
    clear_asc_bit(96);

    // Automatic Gain Control Setup

    // Set gain control signals to come from ASC
    clear_asc_bit(271);
    clear_asc_bit(491);

    // ASC<100> = envelope detector
    // '0' to choose envelope detector,
    // '1' chooses original scm3 overload detector
    clear_asc_bit(100);

    // VGA gain select mux {102=MSB, 101=LSB}
    // Chooses the source of gain control signals connected to analog
    // 00 = AGC FSM
    // 01 or 10 = analog_cfg
    // 11 = GPIN
    clear_asc_bit(101);
    clear_asc_bit(102);

    // Activate TIA only mode
    // '1' = only control gain of TIA
    // '0' = control gain of TIA and stage1/2
    set_asc_bit(97);

    // Memory mapped config registers
    // analog_cfg[239:224]	AGC		{gain_imbalance_select 1,
    // gain_offset 3, vga_ctrl_Q_analogcfg 6, vga_ctrl_I_analogcfg 6}
    // ANALOG_CFG_REG__14 analog_cfg[255:240]	AGC {envelope_threshold 4,
    // wait_time 12} ANALOG_CFG_REG__15 gain_imbalance_select '0' = subtract
    // 'gain_offset' from Q channel '1' = subtract 'gain_offset' from I channel
    // envelope_threshold = the max-min value of signal that will cause gain
    // reduction wait_time = how long FSM waits for settling before making
    // another adjustment
    ANALOG_CFG_REG__14 = 0x0000;
    ANALOG_CFG_REG__15 = 0xA00F;

    // Matched Filter/Clock & Data Recovery
    // Choose output polarity of demod
    // If RX LO is 2.5MHz below the channel, use ASC<103>=1
    // This bit just inverts the output data bits
    set_asc_bit(103);

    // CDR feedback parameters
    // Determined experimentally - unlikely to need to ever change
    tau_shift = 11;
    e_k_shift = 2;
    ANALOG_CFG_REG__3 = (tau_shift << 11) | (e_k_shift << 7);

    // Threshold used for packet detection
    // This number corresponds to the Hamming distance threshold for determining
    // if incoming 15.4 chip stream is a packet
    correlation_threshold = 5;
    ANALOG_CFG_REG__9 = correlation_threshold;

    // Mux select bits to choose internal demod or external clk/data from gpio
    // '0' = on chip, '1' = external from GPIO
    clear_asc_bit(269);
    clear_asc_bit(270);

    // Set LDO reference voltage
    // Best performance was found with LDO at max voltage (0)
    // Some performance can be traded for power by turning this voltage down
    set_IF_LDO_voltage(0);

    // Set RST_B to analog_cfg[75]
    // This chooses whether the reset for the digital blocks is connected to a
    // memory mapped register or a scan bit
    set_asc_bit(240);

    // Mixer and polyphase control settings can be driven from either ASC or
    // memory mapped I/O Mixers and polyphase should both be enabled for RX and
    // both disabled for TX
    // --
    // For polyphase (1=enabled),
    // 	mux select signal ASC<746>=0 gives control to ASC<971>
    //	mux select signal ASC<746>=1 gives control to analog_cfg<256> (bit 0 of
    // ANALOG_CFG_REG__16)
    // --
    // For mixers (0=enabled), both I and Q should be enabled for matched filter
    // mode 	mux select signals ASC<744>=0 and ASC<745>=0 give control to
    // ASC<298> and ASC<307>
    //	mux select signals ASC<744>=1 and ASC<745>=1 give control to
    // analog_cfg<257> analog_cfg<258> (bits 1 and 2 in ANALOG_CFG_REG__16)

    // Set mixer and polyphase control signals to memory mapped I/O
    set_asc_bit(744);
    set_asc_bit(745);
    set_asc_bit(746);

    // Enable both polyphase and mixers via memory mapped IO (...001 = 0x1)
    // To disable both you would invert these values (...110 = 0x6)
    ANALOG_CFG_REG__16 = 0x1;
}

// put this in senddone (in inthandlers.h)////
//  set radio frequency for rx
// run radio rx enable
// call radio rx_now
// if you hear a packet, rx_done runs and thats where things could be found
// (acks)
void send_lh_packet(unsigned int sync_time, unsigned int laser_time,
                    lh_id_t lighthouse, angle_type_t angle_type) {
    int i;
    if (USE_RADIO == 0) {
        return;
    }
    // turn on radio (radio_txenable)
    radio_txEnable();
    // enable radio interrupts (radio_enable_interrupts) (do this somewhere;
    // only needs to be done once) place data into a "send_packet" global
    // variable array
    send_packet[0] = (lighthouse << 1) | angle_type;
    send_packet[1] = sync_time & 0xFF;
    send_packet[2] = (sync_time >> 8) & 0xFF;
    send_packet[3] = (sync_time >> 16) & 0xFF;
    send_packet[4] = (sync_time >> 24) & 0xFF;

    send_packet[5] = laser_time & 0xFF;
    send_packet[6] = (laser_time >> 8) & 0xFF;
    send_packet[7] = (laser_time >> 16) & 0xFF;
    send_packet[8] = (laser_time >> 24) & 0xFF;
    for (i = 0; i < 1000; i++) {}
    // call "radio_loadpacket", which puts array into hardware fifo (takes time)
    radio_loadPacket(9);

    // set radio frequency (radio_setfrequency). This needs to be figured out
    // LC_FREQCHANGE(22&0x1F, 21&0x1F, 4&0x1F); //for no pa
    // LC_FREQCHANGE(23&0x1F, 2&0x1F, 6&0x1F); //for pa

    LC_FREQCHANGE(23 & 0x1F, QX3_MID & 0x1F, QX3_FINE & 0x1F);  // qx3
    for (i = 0; i < 200; i++) {}
    // transmit packet (radio_txnow) (wait 50 us between tx enable and tx_now)
    radio_txNow();
}
pulse_type_t classify_pulse(unsigned int timestamp_rise,
                            unsigned int timestamp_fall) {
    pulse_type_t pulse_type;
    unsigned int pulse_width;

    pulse_width = (timestamp_fall - timestamp_rise) * HCLOCK_ERROR;
    pulse_type = INVALID;

    // Identify what kind of pulse this was

    if (pulse_width < 585 + WIDTH_BIAS && pulse_width > 100 + WIDTH_BIAS)
        pulse_type = LASER;  // Laser sweep (THIS NEEDS TUNING)
    if (pulse_width < 675 + WIDTH_BIAS && pulse_width > 585 + WIDTH_BIAS)
        pulse_type = AZ;  // Azimuth sync, data=0, skip = 0
    if (pulse_width >= 675 + WIDTH_BIAS && pulse_width < 781 + WIDTH_BIAS)
        pulse_type = EL;  // Elevation sync, data=0, skip = 0
    if (pulse_width >= 781 + WIDTH_BIAS && pulse_width < 885 + WIDTH_BIAS)
        pulse_type = AZ;  // Azimuth sync, data=1, skip = 0
    if (pulse_width >= 885 + WIDTH_BIAS && pulse_width < 989 + WIDTH_BIAS)
        pulse_type = EL;  // Elevation sync, data=1, skip = 0
    if (pulse_width >= 989 + WIDTH_BIAS && pulse_width < 1083 + WIDTH_BIAS)
        pulse_type = AZ_SKIP;  // Azimuth sync, data=0, skip = 1
    if (pulse_width >= 1083 + WIDTH_BIAS && pulse_width < 1200 + WIDTH_BIAS)
        pulse_type = EL_SKIP;  // elevation sync, data=0, skip = 1
    if (pulse_width >= 1200 + WIDTH_BIAS && pulse_width < 1300 + WIDTH_BIAS)
        pulse_type = AZ_SKIP;  // Azimuth sync, data=1, skip = 1
    if (pulse_width >= 1300 + WIDTH_BIAS && pulse_width < 1400 + WIDTH_BIAS)
        pulse_type = EL_SKIP;  // Elevation sync, data=1, skip = 1

    return pulse_type;
}

// This function takes the current gpio state as input returns a debounced
// version of of the current gpio state. It keeps track of the previous gpio
// states in order to add hysteresis to the system. The return value includes
// the current gpio state and the time that the first transition ocurred, which
// should help glitches from disrupting legitamate pulses. This is called
void debounce_gpio(unsigned short gpio, unsigned short* gpio_out,
                   unsigned int* trans_out) {
    // keep track of number of times this gpio state has been measured since
    // most recent transistion
    static int count;
    static gpio_tran_t deb_gpio;  // current debounced state

    static unsigned int tran_time;
    static unsigned short target_state;
    // two states: debouncing and not debouncing
    static enum state {
        NOT_DEBOUNCING = 0,
        DEBOUNCING = 1
    } state = NOT_DEBOUNCING;

    switch (state) {
        case NOT_DEBOUNCING: {
            // if not debouncing, compare current gpio state to previous
            // debounced gpio state
            if (gpio != deb_gpio.gpio) {
                // record start time of this transition
                tran_time = RFTIMER_REG__COUNTER;
                // if different, initiate debounce procedure
                state = DEBOUNCING;
                target_state = gpio;

                // increment counter for averaging
                count++;
            }
            // otherwise just break without changing curr_state
            break;
        }
        case DEBOUNCING: {
            // if debouncing, compare current gpio state to target transition
            // state
            if (gpio == target_state) {
                // if same as target transition state, increment counter
                count++;
            } else {
                // if different from target transition state, decrement counter
                count--;
            }

            // if count is high enough
            if (count >= DEB_THRESH) {
                deb_gpio.timestamp_tran = tran_time;
                deb_gpio.gpio = target_state;
                state = NOT_DEBOUNCING;
                count = 0;

            } else if (count == 0) {
                count = 0;
                state = NOT_DEBOUNCING;
            }
            break;
        }
    }
    *gpio_out = deb_gpio.gpio;
    *trans_out = deb_gpio.timestamp_tran;
}

// keeps track of the current state and will print out pulse train information
// when it's done.
void update_state(pulse_type_t pulse_type, unsigned int timestamp_rise) {
    if (pulse_type == INVALID) {
        return;
    }
    // FSM which searches for the four pulse sequence
    // An output will only be printed if four pulses are found and the sync
    // pulse widths are within the bounds listed above.
    update_state_azimuth(pulse_type, timestamp_rise);
    update_state_elevation(pulse_type, timestamp_rise);
}

void update_state_elevation(pulse_type_t pulse_type,
                            unsigned int timestamp_rise) {
    static unsigned int elevation_a_sync;
    static unsigned int elevation_b_sync;

    static unsigned int elevation_a_laser;
    static unsigned int elevation_b_laser;

    static int state = 0;

    int nextstate;

    // TODO: keep track of last delta time measurement for filtering
    static unsigned int last_delta_a;
    static unsigned int last_delta_b;

    if (pulse_type == INVALID) {
        return;
    }
    switch (state) {
        // Search for an azimuth sync pulse, assume its A
        case 0: {
            if (pulse_type == EL || pulse_type == EL_SKIP) {
                if (pulse_type == EL) {
                    elevation_a_sync = timestamp_rise;
                    nextstate = 1;

                } else {
                    // go to elevation b state
                    nextstate = 2;
                    // printf("state transition: %d to %d\n",state,nextstate);
                }
            } else {
                if (DEBUG_STATE) {
                    printf("el state fail. State %d, Pulse Type: %d \n", state,
                           pulse_type);
                }
                nextstate = 0;
            }

            break;
        }

        // Waiting for another consecutive elevation skip sync from B, this
        // should be a skip sync pulse
        case 1: {
            if (pulse_type == EL_SKIP) {
                nextstate = 3;
            } else if (pulse_type == EL) {
                // return to this state (skip 0)
                nextstate = 1;
                elevation_a_sync = timestamp_rise;
            } else {
                nextstate = 0;
                if (DEBUG_STATE) {
                    printf("el state fail. State %d, Pulse Type: %d \n", state,
                           pulse_type);
                }
            }
            break;
        }

        // Azimuth B sync state
        case 2: {
            if (pulse_type == EL) {
                // the last pulse was an azimuth sync from lighthouse B
                elevation_b_sync = timestamp_rise;
                // go to azimuth b laser detect
                nextstate = 4;
                // printf("state transition: %d to %d\n",state,nextstate);
            } else if (pulse_type == EL_SKIP) {
                // return to this state (skipping 0)
                nextstate = 2;
            } else {
                nextstate = 0;
                // printf("state fail. State %d, Pulse Type: %d
                // \n",state,pulse_type);
            }
            break;
        }

        // Elevation A laser sweep
        case 3: {
            if (pulse_type == LASER) {
                // lighthouse a laser
                elevation_a_laser = timestamp_rise;

                nextstate = 0;
                if (last_delta_a > 0 &&
                    abs(((int)(elevation_a_laser - elevation_a_sync)) -
                        (int)last_delta_a) < 4630) {
                    // printf("el A: %d,
                    // %d\n",elevation_a_sync,elevation_a_laser);
                    send_lh_packet(elevation_a_sync, elevation_a_laser, A,
                                   ELEVATION);
                }
                last_delta_a = elevation_a_laser - elevation_a_sync;
            } else if (pulse_type == EL) {
                // skip straight to state 1
                nextstate = 1;
                elevation_a_sync = timestamp_rise;
            } else if (pulse_type == EL_SKIP) {
                // skip straight to state 2
                nextstate = 2;
            } else {
                nextstate = 0;
                if (DEBUG_STATE) {
                    printf("el state fail. State: %d, Pulse Type: %d \n", state,
                           pulse_type);
                }
            }
            break;
        }

        // elevation B laser sweep
        case 4: {
            if (pulse_type == LASER) {
                // lighthouse b laser
                elevation_b_laser = timestamp_rise;
                // go to azimuth b laser detect
                nextstate = 0;
                if (last_delta_b > 0 &&
                    abs(((int)(elevation_b_laser - elevation_b_sync)) -
                        (int)last_delta_b) < 4630) {
                    // printf("el B: %d,
                    // %d\n",elevation_b_sync,elevation_b_laser);
                    send_lh_packet(elevation_b_sync, elevation_b_laser, B,
                                   ELEVATION);
                }
                last_delta_b = elevation_b_laser - elevation_b_sync;
            } else if (pulse_type == EL) {
                // skip straight to state 1
                nextstate = 1;
                elevation_a_sync = timestamp_rise;
            } else if (pulse_type == EL_SKIP) {
                // skip straight to state 2
                nextstate = 2;
            } else {
                nextstate = 0;
                if (DEBUG_STATE) {
                    printf("el state fail. State: %d, Pulse Type: %d \n", state,
                           pulse_type);
                }
            }
            break;
        }
    }

    state = nextstate;
}

// keeps track of the current state and will print out pulse train information
// when it's done.
void update_state_azimuth(pulse_type_t pulse_type,
                          unsigned int timestamp_rise) {
    static unsigned int azimuth_a_sync;
    static unsigned int azimuth_b_sync;

    static unsigned int azimuth_a_laser;
    static unsigned int azimuth_b_laser;

    static int state = 0;

    int nextstate;

    // TODO: keep track of last delta time measurement for filtering
    static unsigned int last_delta_a;
    static unsigned int last_delta_b;

    if (pulse_type == INVALID) {
        return;
    }
    // FSM which searches for the four pulse sequence
    // An output will only be printed if four pulses are found and the sync
    // pulse widths are within the bounds listed above.
    switch (state) {
        // Search for an azimuth A sync pulse, we don't know if it's A or B yet
        case 0: {
            if (pulse_type == AZ || pulse_type == AZ_SKIP) {
                if (pulse_type == AZ) {
                    azimuth_a_sync = timestamp_rise;
                    nextstate = 1;
                    // printf("state transition: %d to %d\n",state,nextstate);
                } else {
                    // go to azimuth b state
                    nextstate = 2;
                    // printf("state transition: %d to %d\n",state,nextstate);
                }
            } else
                nextstate = 0;

            break;
        }

        // Waiting for another consecutive azimuth skip sync from B, this should
        // be a skip sync pulse
        case 1: {
            if (pulse_type == AZ_SKIP) {
                // lighthouse A sweep pulse

                nextstate = 3;
                // printf("state transition: %d to %d\n",state,nextstate);
            } else if (pulse_type == AZ) {
                // staty in this state
                nextstate = 1;
                azimuth_a_sync = timestamp_rise;
            } else {
                nextstate = 0;
                // printf("state fail. State %d, Pulse Type: %d
                // \n",state,pulse_type);
            }
            break;
        }

        // Azimuth B sync state
        case 2: {
            if (pulse_type == AZ) {
                // the last pulse was an azimuth sync from lighthouse B
                azimuth_b_sync = timestamp_rise;
                // go to azimuth b laser detect
                nextstate = 4;
                // printf("state transition: %d to %d\n",state,nextstate);
            } else if (pulse_type == AZ_SKIP) {
                // stay in this state
                nextstate = 2;
            } else {
                nextstate = 0;
                // printf("state fail. State %d, Pulse Type: %d
                // \n",state,pulse_type);
            }
            break;
        }

        // Azimuth A laser sweep
        case 3: {
            if (pulse_type == LASER) {
                // lighthouse a laser
                azimuth_a_laser = timestamp_rise;
                // go to azimuth b laser detect
                nextstate = 0;
                // filter out pulses that have changed by more than 10 degrees
                // (4630 ticks)
                if (last_delta_a > 0 &&
                    abs(((int)(azimuth_a_laser - azimuth_a_sync)) -
                        (int)last_delta_a) < 4630) {
                    // printf("az A: %d, %d\n",azimuth_a_sync,azimuth_a_laser);
                    send_lh_packet(azimuth_a_sync, azimuth_a_laser, A, AZIMUTH);
                }
                last_delta_a = azimuth_a_laser - azimuth_a_sync;
            } else if (pulse_type == AZ) {
                // go to first pulse AZ state
                nextstate = 1;
                azimuth_a_sync = timestamp_rise;
            } else if (pulse_type == AZ_SKIP) {
                // go to az b first pulse state
                nextstate = 2;
            } else {
                nextstate = 0;
                // printf("state fail. State %d, Pulse Type: %d
                // \n",state,pulse_type);
            }
            break;
        }

        // Azimuth B laser sweep
        case 4: {
            if (pulse_type == LASER) {
                // lighthouse b laser
                azimuth_b_laser = timestamp_rise;
                // go to azimuth b laser detect
                nextstate = 0;

                // filter out pulses that have changed by more than 10 degrees
                // (4630 ticks)
                if (last_delta_b > 0 &&
                    abs(((int)(azimuth_b_laser - azimuth_b_sync)) -
                        (int)last_delta_b) < 4630) {
                    // printf("az B: %d, %d,
                    // %d\n",azimuth_b_sync,azimuth_b_laser,azimuth_b_laser-azimuth_b_sync
                    // );
                    send_lh_packet(azimuth_b_sync, azimuth_b_laser, B, AZIMUTH);
                }
                last_delta_b = azimuth_b_laser - azimuth_b_sync;
            } else if (pulse_type == AZ) {
                // go to first pulse AZ state
                nextstate = 1;
                azimuth_a_sync = timestamp_rise;
            } else if (pulse_type == AZ_SKIP) {
                // go to az b first pulse state
                nextstate = 2;
            } else {
                nextstate = 0;
                // printf("state fail. State %d, Pulse Type: %d
                // \n",state,pulse_type);
            }
            break;
        }
    }

    state = nextstate;
}

unsigned int sync_pulse_width_compensate(unsigned int pulse_width) {
    static unsigned int sync_widths[60];
    unsigned int avg;  // average sync pulse width in 10 MHz ticks
    int i;
    unsigned int sum = 0;

    static unsigned int sync_count = 0;

    // if it's a sync pulse, add to average pulse width
    if (pulse_width > 585 && pulse_width < 700) {
        sync_widths[sync_count % 60] = pulse_width;
        sync_count++;
    }

    for (i = 0; i < 60; i++) {
        sum += sync_widths[i];
    }

    avg = sum / 60;

    printf("avg: %d\n", avg);
    return avg;
}

// callback that is called by gpio ints
// level parameter is what level the interrupt is at (high or low).
void lh_int_cb(int level) {
    // Keep track of static duration level state
    static int state = 0;
    static bool debounced = false;
    static uint8_t debounce_count_high = 0;
    static uint8_t debounce_count_low = 0;
    static uint32_t timestamp_rise = 0;
    static uint32_t timestamp_fall = 0;

    // detect edge transitions and disable level interrupts to mimic edge
    // behavior

    // check for rising edge
    if (level == 1 && state == 0) {
        if (debounce_count_high == 0) {
            // capture rising edge
            timestamp_rise = RFTIMER_REG__COUNTER;
        }

        // increment debounce count
        debounce_count_high++;

        if (debounce_count_high >= DEB_THRESH) {
            // reset debounce count
            debounce_count_high = 0;
            // if level high, disable high interrupt and enable low interrupt
            state = 1;

            // disable gpio8 active high interrupt
            ICER = GPIO8_HIGH_INT;

            // enable active low interrupt
            ISER = GPIO9_LOW_INT;
            // clear interrupt
            ICPR = GPIO9_LOW_INT;
//
#if DEBUG_INT == 1
            send_lh_packet(2, 2, A, AZIMUTH);
#endif
        }
    }
    // if level low with falling edge, disable low interrupt and enable high
    // interrupt
    else if (level == 0 && state == 1) {
        // capture edge on first edge
        if (debounce_count_low == 0) {
            // capture falling edge
            timestamp_fall = RFTIMER_REG__COUNTER;
        }

        // increment debounce counter
        debounce_count_low++;

        if (debounce_count_low >= DEB_THRESH) {
            // reset debounce count
            debounce_count_low = 0;

            // if level high, disable high interrupt and enable low interrupt
            state = 0;

            // disable gpio9 active low interrupt
            ICER = GPIO9_LOW_INT;

            // enable active high interrupt
            ISER = GPIO8_HIGH_INT;

            // clear interrupt
            ICPR = GPIO8_HIGH_INT;

#if DEBUG_INT == 1
            send_lh_packet(1, 1, A, AZIMUTH);
#endif

            update_state(classify_pulse(timestamp_rise, timestamp_fall),
                         timestamp_rise);
            // send_lh_packet(timestamp_rise,timestamp_fall,A,AZIMUTH);
        }
    }
}

// This function sends an IMU packet, which is different than a lighthouse
// packet by length and the first byte will be 'i' (105)
void send_imu_packet(imu_data_t imu_measurement) {
    int i;
    // enable radio
    radio_txEnable();

    // place packet type code in first byte of packet
    send_packet[0] = IMU_CODE;

    // place imu acc x data into the rest of the packet (lsb first)
    send_packet[1] = imu_measurement.acc_x.bytes[0];
    send_packet[2] = imu_measurement.acc_x.bytes[1];

    // place acceleration y data into packet
    send_packet[3] = imu_measurement.acc_y.bytes[0];
    send_packet[4] = imu_measurement.acc_y.bytes[1];

    // place acceleration z data into packet
    send_packet[5] = imu_measurement.acc_z.bytes[0];
    send_packet[6] = imu_measurement.acc_z.bytes[1];

    // place gyro x data into packet
    send_packet[7] = imu_measurement.gyro_x.bytes[0];
    send_packet[8] = imu_measurement.gyro_x.bytes[1];

    // place gyro y data into packet
    send_packet[9] = imu_measurement.gyro_y.bytes[0];
    send_packet[10] = imu_measurement.gyro_y.bytes[1];

    // place gyro z data into packet
    send_packet[11] = imu_measurement.gyro_z.bytes[0];
    send_packet[12] = imu_measurement.gyro_z.bytes[1];

    // load packet
    radio_loadPacket(13);

    // set lo frequency
    // original: LC_FREQCHANGE(23&0x1F, 2&0x1F, 6&0x1F); //for pa
    // LC_FREQCHANGE(23&0x1F, 21&0x1F, 8&0x1F);
    LC_FREQCHANGE(23 & 0x1F, QX3_MID & 0x1F, QX3_FINE & 0x1F);
    // wait for 1000 loop cycles

    for (i = 0; i < 1000; i++) {}

    // send packet
    radio_txNow();
}
