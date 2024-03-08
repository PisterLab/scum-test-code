#include <stdio.h>

#include "Memory_Map.h"
#include "bucket_o_functions.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"

extern unsigned int ASC[38];
unsigned int ASC_FPGA[38] = {0};
extern unsigned int cal_iteration;
extern char recv_packet[130];

extern unsigned int LC_target;
extern unsigned int IF_clk_target;
extern unsigned int LC_code;
extern unsigned int IF_coarse;
extern unsigned int IF_fine;
extern unsigned int cal_iteration;

extern unsigned int HF_CLOCK_fine;
extern unsigned int HF_CLOCK_coarse;
extern unsigned int RC2M_superfine;

unsigned int RX_channel_codes[16] = {0};
unsigned int TX_channel_codes[16] = {0};

// Timer parameters
// Assuming RF timer frequency = 500 kHz
unsigned int packet_interval = 62500;  // 125ms
unsigned int guard_time = 500;
unsigned int radio_startup_time = 70;      // 200us
unsigned int expected_RX_arrival = 25000;  // must be > 30ms
unsigned int ack_turnaround_time = 96;     // 192 us

void LC_FREQCHANGE_ASC(int coarse, int mid, int fine) {
    //	Inputs:
    //		coarse: 5-bit code (0-31) to control the ~15 MHz step frequency
    // DAC 		mid: 5-bit code (0-31) to control the ~800 kHz step
    // frequency DAC 		fine: 5-bit code (0-31) to control the ~100 kHz
    // step
    // frequency DAC
    //  Outputs:
    //		none; need to program ASC

    // mask to ensure that the coarse, mid, and fine are actually 5-bit
    char coarse_m = (char)(coarse & 0x1F);
    char mid_m = (char)(mid & 0x1F);
    char fine_m = (char)(fine & 0x1F);

    unsigned int j;

    // fine_tune 6 946-950 MSB:LSB+dummy
    // mid_tune 6 952-956 MSB:LSB+dummy
    // coarse_tune 6 958-962 MSB:LSB+dummy

    // Set tuning control to ASC
    set_asc_bit(964);

    // Set fine bits
    for (j = 0; j < 5; j++) {
        if ((fine_m >> j) & 0x1)
            set_asc_bit(950 - j);
        else
            clear_asc_bit(950 - j);
    }

    // Set mid bits
    for (j = 0; j < 5; j++) {
        if ((mid_m >> j) & 0x1)
            set_asc_bit(956 - j);
        else
            clear_asc_bit(956 - j);
    }

    // Set coarse bits
    for (j = 0; j < 5; j++) {
        if ((coarse_m >> j) & 0x1)
            set_asc_bit(962 - j);
        else
            clear_asc_bit(962 - j);
    }
}

void LC_monotonic_ASC(int LC_code) {
    // int coarse_divs = 440;
    // int mid_divs = 31; // For full fine code sweeps

    int fine_fix = 0;
    int mid_fix = 0;
    // int coarse_divs = 136;
    int mid_divs =
        25;  // works for Ioana's board, Fil's board, Brad's other board

    // int coarse_divs = 167;
    int coarse_divs = 155;
    // int mid_divs = 27; // works for Brad's board // 25 and 155 worked really
    // well @ low frequency, 27 167 worked great @ high frequency (Brad's board)

    int mid;
    int fine;
    int coarse = (((LC_code / coarse_divs + 19) & 0x000000FF));

    LC_code = LC_code % coarse_divs;
    // mid = ((((LC_code/mid_divs)*4 + mid_fix) & 0x000000FF)); // works for
    // boards (a)
    mid = ((((LC_code / mid_divs) * 3 + mid_fix) & 0x000000FF));
    // mid = ((((LC_code/mid_divs) + mid_fix) & 0x000000FF));
    if (LC_code / mid_divs >= 2) {
        fine_fix = 0;
    };
    fine = (((LC_code % mid_divs + fine_fix) & 0x000000FF));
    if (fine > 15) {
        fine++;
    };

    LC_FREQCHANGE_ASC(coarse, mid, fine);

    // printf("coarse=%d,mid=%d,fine=%d\n",coarse,mid,fine);
}

// Reverses (reflects) bits in a 32-bit word.
unsigned reverse(unsigned x) {
    x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);
    x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);
    x = ((x & 0x0F0F0F0F) << 4) | ((x >> 4) & 0x0F0F0F0F);
    x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
    return x;
}

// Computes 32-bit crc from a starting address over 'length' dwords
unsigned int crc32c(unsigned char* message, unsigned int length) {
    int i, j;
    unsigned int byte, crc;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < length) {
        byte = message[i];          // Get next byte.
        byte = reverse(byte);       // 32-bit reversal.
        for (j = 0; j <= 7; j++) {  // Do eight times.
            if ((int)(crc ^ byte) < 0)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc = crc << 1;
            byte = byte << 1;  // Ready next msg bit.
        }
        i = i + 1;
    }
    return reverse(~crc);
}

unsigned char flipChar(unsigned char b) {
    b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
    b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
    b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
    return b;
}

void GPO_control(unsigned char row1, unsigned char row2, unsigned char row3,
                 unsigned char row4) {
    int j;

    for (j = 0; j <= 3; j++) {
        if ((row1 >> j) & 0x1)
            set_asc_bit(245 + j);
        else
            clear_asc_bit(245 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row2 >> j) & 0x1)
            set_asc_bit(249 + j);
        else
            clear_asc_bit(249 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row3 >> j) & 0x1)
            set_asc_bit(253 + j);
        else
            clear_asc_bit(253 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row4 >> j) & 0x1)
            set_asc_bit(257 + j);
        else
            clear_asc_bit(257 + j);
    }
}

void GPI_control(char row1, char row2, char row3, char row4) {
    int j;

    for (j = 0; j <= 1; j++) {
        if ((row1 >> j) & 0x1)
            set_asc_bit(261 + j);
        else
            clear_asc_bit(261 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row2 >> j) & 0x1)
            set_asc_bit(263 + j);
        else
            clear_asc_bit(263 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row3 >> j) & 0x1)
            set_asc_bit(265 + j);
        else
            clear_asc_bit(265 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row4 >> j) & 0x1)
            set_asc_bit(267 + j);
        else
            clear_asc_bit(267 + j);
    }
}

// Enable output drivers for GPIO based on 'mask'
// '1' = output enabled, so GPO_enables(0xFFFF) enables all output drivers
// GPO enables are active low on-chip
void GPO_enables(unsigned int mask) {
    // out_en<0:15> =
    // ASC<1131>,ASC<1133>,ASC<1135>,ASC<1137>,ASC<1140>,ASC<1142>,ASC<1144>,ASC<1146>,...
    // ASC<1115>,ASC<1117>,ASC<1119>,ASC<1121>,ASC<1124>,ASC<1126>,ASC<1128>,ASC<1130>
    unsigned short asc_locations[16] = {1131, 1133, 1135, 1137, 1140, 1142,
                                        1144, 1146, 1115, 1117, 1119, 1121,
                                        1124, 1126, 1128, 1130};
    unsigned int j;

    for (j = 0; j <= 15; j++) {
        if ((mask >> j) & 0x1)
            clear_asc_bit(asc_locations[j]);
        else
            set_asc_bit(asc_locations[j]);
    }
}

// Enable input path for GPIO based on 'mask'
// '1' = input enabled, so GPI_enables(0xFFFF) enables all inputs
// GPI enables are active high on-chip
void GPI_enables(unsigned int mask) {
    // in_en<0:15> =
    // ASC<1132>,ASC<1134>,ASC<1136>,ASC<1138>,ASC<1139>,ASC<1141>,ASC<1143>,ASC<1145>,...
    // ASC<1116>,ASC<1118>,ASC<1120>,ASC<1122>,ASC<1123>,ASC<1125>,ASC<1127>,ASC<1129>
    unsigned short asc_locations[16] = {1132, 1134, 1136, 1138, 1139, 1141,
                                        1143, 1145, 1116, 1118, 1120, 1122,
                                        1123, 1125, 1127, 1129};
    unsigned int j;

    for (j = 0; j <= 15; j++) {
        if ((mask >> j) & 0x1)
            set_asc_bit(asc_locations[j]);
        else
            clear_asc_bit(asc_locations[j]);
    }
}

void set_asc_bit_FPGA(unsigned int position) {
    unsigned int index;

    index = position >> 5;

    ASC_FPGA[index] |= 0x80000000 >> (position - (index << 5));

    // Possibly more efficient
    // ASC[position/32] |= 1 << (position%32);
}

void clear_asc_bit_FPGA(unsigned int position) {
    unsigned int index;

    index = position >> 5;

    ASC_FPGA[index] &= ~(0x80000000 >> (position - (index << 5)));

    // Possibly more efficient
    // ASC[position/32] &= ~(1 << (position%32));
}

void GPO_control_FPGA(unsigned char row1, unsigned char row2,
                      unsigned char row3, unsigned char row4) {
    int j;

    for (j = 0; j <= 3; j++) {
        if ((row1 >> j) & 0x1)
            set_asc_bit_FPGA(245 + j);
        else
            clear_asc_bit_FPGA(245 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row2 >> j) & 0x1)
            set_asc_bit_FPGA(249 + j);
        else
            clear_asc_bit_FPGA(249 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row3 >> j) & 0x1)
            set_asc_bit_FPGA(253 + j);
        else
            clear_asc_bit_FPGA(253 + j);
    }

    for (j = 0; j <= 3; j++) {
        if ((row4 >> j) & 0x1)
            set_asc_bit_FPGA(257 + j);
        else
            clear_asc_bit_FPGA(257 + j);
    }
}

void GPI_control_FPGA(char row1, char row2, char row3, char row4) {
    int j;

    for (j = 0; j <= 1; j++) {
        if ((row1 >> j) & 0x1)
            set_asc_bit_FPGA(261 + j);
        else
            clear_asc_bit_FPGA(261 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row2 >> j) & 0x1)
            set_asc_bit_FPGA(263 + j);
        else
            clear_asc_bit_FPGA(263 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row3 >> j) & 0x1)
            set_asc_bit_FPGA(265 + j);
        else
            clear_asc_bit_FPGA(265 + j);
    }

    for (j = 0; j <= 1; j++) {
        if ((row4 >> j) & 0x1)
            set_asc_bit_FPGA(267 + j);
        else
            clear_asc_bit_FPGA(267 + j);
    }
}

// Configure how radio and AUX LDOs are turned on and off
void init_ldo_control(void) {
    // Analog scan chain setup for radio LDOs
    clear_asc_bit(501);  // = scan_pon_if
    clear_asc_bit(502);  // = scan_pon_lo
    clear_asc_bit(503);  // = scan_pon_pa
    set_asc_bit(504);    // = gpio_pon_en_if
    clear_asc_bit(505);  // = fsm_pon_en_if
    set_asc_bit(506);    // = gpio_pon_en_lo
    clear_asc_bit(507);  // = fsm_pon_en_lo
    clear_asc_bit(508);  // = gpio_pon_en_pa
    clear_asc_bit(509);  // = fsm_pon_en_pa
    set_asc_bit(510);    // = master_ldo_en_if
    set_asc_bit(511);    // = master_ldo_en_lo
    set_asc_bit(512);    // = master_ldo_en_pa
    clear_asc_bit(513);  // = scan_pon_div
    set_asc_bit(514);    // = gpio_pon_en_div
    clear_asc_bit(515);  // = fsm_pon_en_div
    set_asc_bit(516);    // = master_ldo_en_div

    // AUX LDO Control:
    // ASC<914> chooses whether ASC<916> or analog_cfg<167> controls LDO
    // 0 = ASC<916> has control
    // 1 = analog_cfg<167> has control
    // set_asc_bit(914);

    // Examples of controlling AUX LDO:

    // Turn on aux ldo from analog_cfg<167>
    // ANALOG_CFG_REG__10 = 0x0080;

    // Aux LDO  = off via ASC
    // clear_asc_bit(914);
    // set_asc_bit(916);

    // Aux LDO  = on via ASC
    // clear_asc_bit(914);
    // clear_asc_bit(916);

    // Memory-mapped LDO control
    // ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX |
    // IF_MUX | LO_MUX For MUX signals, '1' = FSM control, '0' = memory mapped
    // control For EN signals, '1' = turn on LDO

    // Some examples:

    // Assert all PON_XX signals for the radio via memory mapped register
    // ANALOG_CFG_REG__10 = 0x00F8;

    // Turn off all PON_XX signals for the radio via memory mapped register
    // ANALOG_CFG_REG__10 = 0x0000;

    // Turn on only LO via memory mapped register
    // ANALOG_CFG_REG__10 = 0x0008;

    // Give FSM control of all radio PON signals
    // ANALOG_CFG_REG__10 = 0x0007;

    // Debug visibility
    // PON signals are available on GPIO<4-7> on bank 5
    // GPIO<4> = PON_LO
    // GPIO<5> = PON_PA
    // GPIO<6> = PON_IF
    // GPIO<7> = PON_DIV
    // GPO_control(0,5,0,0);
    // Enable the output direction
}

void analog_scan_chain_write_3B_fromFPGA(unsigned int* scan_bits) {
    int i = 0;
    int j = 0;
    unsigned int asc_reg;

    for (i = 37; i >= 0; i--) {
        // printf("\n%d,%lX\n",i,scan_bits[i]);

        for (j = 0; j < 32; j++) {
            // Set scan_in (should be inverted)
            if ((scan_bits[i] & (0x00000001 << j)) == 0)
                asc_reg = 0x1;
            else
                asc_reg = 0x0;

            // Write asc_reg to analog_cfg
            ANALOG_CFG_REG__5 = asc_reg;

            // Lower phi1
            asc_reg &= ~(0x2);
            ANALOG_CFG_REG__5 = asc_reg;

            // Toggle phi2
            asc_reg |= 0x4;
            ANALOG_CFG_REG__5 = asc_reg;
            asc_reg &= ~(0x4);
            ANALOG_CFG_REG__5 = asc_reg;

            // Raise phi1
            asc_reg |= 0x2;
            ANALOG_CFG_REG__5 = asc_reg;
        }
    }
}

void analog_scan_chain_load_3B_fromFPGA() {
    // Assert load signal (and cfg<357>)
    ANALOG_CFG_REG__5 = 0x0028;

    // Lower load signal
    ANALOG_CFG_REG__5 = 0x0020;
}

// SRAM Verification Test
// BW 2-25-18
// Adapted from March C- algorithm, Eqn (2) in [1]
// [1] Van De Goor, Ad J. "Using march tests to test SRAMs." IEEE Design & Test
// of Computers 10.1 (1993): 8-14. Only works for DMEM since you must be able to
// read and write
unsigned int sram_test(unsigned int* baseAddress, unsigned int num_dwords) {
    int i;
    int j;

    unsigned int* addr;
    unsigned int num_errors = 0;
    addr = baseAddress;

    printf("\n\nStarting SRAM test from 0x%X to 0x%X...\n", baseAddress,
           baseAddress + num_dwords);
    printf("This takes awhile...\n");

    // Write 0 to all bits, in any address order
    // Outer loop selects 32-bit dword, inner loop does single bit
    for (i = 0; i < num_dwords; i++) {
        for (j = 0; j < 32; j++) {
            addr[i] &= ~(1UL << j);
        }
    }

    // (r0,w1) (incr addr)
    for (i = 0; i < num_dwords; i++) {
        for (j = 0; j < 32; j++) {
            if ((addr[i] & (1UL << j)) != 0x0) {
                printf("\nERROR 1 @ address %X bit %d -- Value is %X", i, j,
                       addr[i]);
                num_errors++;
            }
            addr[i] |= 1UL << j;
        }
    }

    // (r1,w0) (incr addr)
    for (i = 0; i < num_dwords; i++) {
        for (j = 0; j < 32; j++) {
            if ((addr[i] & (1UL << j)) != 1UL << j) {
                printf("\nERROR 2 @ address %X bit %d -- Value is %X", i, j,
                       addr[i]);
                num_errors++;
            }
            addr[i] &= ~(1UL << j);
        }
    }

    // (r0,w1) (decr addr)
    for (i = (num_dwords - 1); i >= 0; i--) {
        for (j = 31; j >= 0; j--) {
            if ((addr[i] & (1UL << j)) != 0x0) {
                printf("\nERROR 3 @ address %X bit %d -- Value is %X", i, j,
                       addr[i]);
                num_errors++;
            }
            addr[i] |= 1UL << j;
        }
    }

    // (r1,w0) (decr addr)
    for (i = (num_dwords - 1); i >= 0; i--) {
        for (j = 31; j >= 0; j--) {
            if ((addr[i] & (1UL << j)) != 1UL << j) {
                printf("\nERROR 4 @ address %X bit %d -- Value is %X", i, j,
                       addr[i]);
                num_errors++;
            }
            addr[i] &= ~(1UL << j);
        }
    }

    // r0 (any order)
    for (i = 0; i < num_dwords; i++) {
        for (j = 0; j < 32; j++) {
            if ((addr[i] & (1UL << j)) != 0x0) {
                printf("\nERROR 5 @ address %X bit %d -- Value is %X", i, j,
                       addr[i]);
                num_errors++;
            }
        }
    }

    printf("\nSRAM Test Complete -- %d Errors\n", num_errors);

    return num_errors;
}

// Change the reference voltage for the IF LDO
// 0 <= code <= 127
void set_IF_LDO_voltage(int code) {
    unsigned int j;

    // ASC<492:498> = if_ldo_rdac<0:6> (<0:6(MSB)>)

    for (j = 0; j <= 6; j++) {
        if ((code >> j) & 0x1)
            set_asc_bit(492 + j);
        else
            clear_asc_bit(492 + j);
    }
}

// Change the reference voltage for the VDDD LDO
// 0 <= code <= 127
void set_VDDD_LDO_voltage(int code) {
    unsigned int j;

    // ASC(791:1:797) (LSB:MSB)

    for (j = 0; j <= 4; j++) {
        if ((code >> j) & 0x1)
            set_asc_bit(797 - j);
        else
            clear_asc_bit(797 - j);
    }

    // Two MSBs are inverted
    for (j = 5; j <= 6; j++) {
        if ((code >> j) & 0x1)
            clear_asc_bit(797 - j);
        else
            set_asc_bit(797 - j);
    }
}

// Change the reference voltage for the AUX LDO
// 0 <= code <= 127
void set_AUX_LDO_voltage(int code) {
    unsigned int j;

    // ASC(923:-1:917) (MSB:LSB)

    for (j = 0; j <= 4; j++) {
        if ((code >> j) & 0x1)
            set_asc_bit(917 + j);
        else
            clear_asc_bit(917 + j);
    }

    // Two MSBs are inverted
    for (j = 5; j <= 6; j++) {
        if ((code >> j) & 0x1)
            clear_asc_bit(917 + j);
        else
            set_asc_bit(917 + j);
    }
}

// Change the reference voltage for the always-on LDO
// 0 <= code <= 127
void set_ALWAYSON_LDO_voltage(int code) {
    unsigned int j;

    // ASC(924:929) (MSB:LSB)

    for (j = 0; j <= 4; j++) {
        if ((code >> j) & 0x1)
            set_asc_bit(929 - j);
        else
            clear_asc_bit(929 - j);
    }

    // MSB of normal DAC is inverted
    if ((code >> 5) & 0x1)
        clear_asc_bit(924);
    else
        set_asc_bit(924);

    // Panic bit was added for 3B (inverted)
    if ((code >> 6) & 0x1)
        clear_asc_bit(557);
    else
        set_asc_bit(557);
}

// Must set IF clock frequency AFTER calling this function

void set_zcc_demod_threshold(unsigned int thresh) {
    int j;

    // counter threshold 122:107 MSB:LSB
    for (j = 0; j <= 15; j++) {
        if ((thresh >> j) & 0x1)
            set_asc_bit(107 + j);
        else
            clear_asc_bit(107 + j);
    }
}

// Set the divider value for ZCC demod
// Should be equal to (IF_clock_rate / 2 MHz)
void set_IF_ZCC_clkdiv(unsigned int div_value) {
    int j;

    // CLK_DIV = ASC<131:124> MSB:LSB
    for (j = 0; j <= 7; j++) {
        if ((div_value >> j) & 0x1)
            set_asc_bit(124 + j);
        else
            clear_asc_bit(124 + j);
    }
}

// Set the early decision value for ZCC demod
void set_IF_ZCC_early(unsigned int early_value) {
    int j;

    // ASC<224:209> MSB:LSB
    for (j = 0; j <= 15; j++) {
        if ((early_value >> j) & 0x1)
            set_asc_bit(209 + j);
        else
            clear_asc_bit(209 + j);
    }
}

// Untested function
void set_IF_stg3gm_ASC(unsigned int Igm, unsigned int Qgm) {
    int j;

    // Set all bits to zero
    for (j = 0; j < 13; j++) {
        clear_asc_bit(472 + j);
        clear_asc_bit(278 + j);
    }

    // 472:484 = I stg3 gm 13:1
    for (j = 0; j <= Igm; j++) {
        set_asc_bit(484 - j);
    }

    // 278:290 = Q stg3 gm 1:13
    for (j = 0; j <= Qgm; j++) {
        clear_asc_bit(278 + j);
    }
}

// Adjust the comparator offset trim for I channel
// Valid input range 0-31
void set_IF_comparator_trim_I(unsigned int ptrim, unsigned int ntrim) {
    int j;

    // I comparator N side = 452:456 LSB:MSB
    for (j = 0; j <= 4; j++) {
        if ((ntrim >> j) & 0x1)
            set_asc_bit(452 + j);
        else
            clear_asc_bit(452 + j);
    }

    // I comparator P side = 457:461 LSB:MSB
    for (j = 0; j <= 4; j++) {
        if ((ptrim >> j) & 0x1)
            set_asc_bit(457 + j);
        else
            clear_asc_bit(457 + j);
    }
}

// Adjust the comparator offset trim for Q channel
// Valid input range 0-31
void set_IF_comparator_trim_Q(unsigned int ptrim, unsigned int ntrim) {
    int j;

    // I comparator N side = 340:344 MSB:LSB
    for (j = 0; j <= 4; j++) {
        if ((ntrim >> j) & 0x1)
            set_asc_bit(344 - j);
        else
            clear_asc_bit(344 - +j);
    }

    // I comparator P side = 335:339 MSB:LSB
    for (j = 0; j <= 4; j++) {
        if ((ptrim >> j) & 0x1)
            set_asc_bit(339 - j);
        else
            clear_asc_bit(339 - j);
    }
}

// Untested function
void set_IF_gain_ASC(unsigned int Igain, unsigned int Qgain) {
    int j;

    // 485:490 = I code 0:5
    for (j = 0; j <= 4; j++) {
        if ((Igain >> j) & 0x1)
            set_asc_bit(485 + j);
        else
            clear_asc_bit(485 + j);
    }

    // 272:277 = Q code 5:0
    for (j = 0; j <= 5; j++) {
        if ((Qgain >> j) & 0x1)
            set_asc_bit(277 - j);
        else
            clear_asc_bit(277 - j);
    }
}

void radio_init_rx_MF() {
    // int j;
    unsigned int mask1, mask2;
    unsigned int tau_shift, e_k_shift, correlation_threshold;

    // IF uses ASC<271:500>, mask off outside that range
    mask1 = 0xFFFC0000;
    mask2 = 0x000007FF;
    ASC[8] &= mask1;
    ASC[15] &= mask2;

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

    // Set gain for I and Q
    set_IF_gain_ASC(43, 43);

    // Set gm for stg3 ADC drivers
    set_IF_stg3gm_ASC(7, 7);  //(I, Q)

    // Set comparator trims
    set_IF_comparator_trim_I(0, 5);   //(p,n)
    set_IF_comparator_trim_Q(15, 0);  //(p,n)

    // Setup baseband

    // Choose MF demod
    // ASC<0:1> = [0 0]
    clear_asc_bit(0);
    clear_asc_bit(1);

    // IQ source select
    // '0' = from radio
    // '1' = from GPIO
    clear_asc_bit(96);

    // AGC Setup

    // ASC<100> = envelope detector
    // '0' to choose envelope detector,
    // '1' chooses original scm3 overload detector
    clear_asc_bit(100);

    // VGA gain select mux {102=MSB, 101=LSB}
    // Chooses the source of gain control signals connected to analog
    // 00 = AGC FSM
    // 01 or 10 = analog_cfg
    // 11 = GPIN
    set_asc_bit(101);
    set_asc_bit(102);

    // Activate TIA only mode
    // '1' = only control gain of TIA
    // '0' = control gain of TIA and stage1/2
    set_asc_bit(97);

    // Memory mapped config registers
    // analog_cfg[239:224]	AGC		{gain_imbalance_select 1,
    // gain_offset 3, vga_ctrl_Q_analogcfg 6, vga_ctrl_I_analogcfg 6}
    // ANALOG_CFG_REG__14 analog_cfg[255:240]	AGC
    // {envelope_threshold 4, wait_time 12}		ANALOG_CFG_REG__15
    // gain_imbalance_select
    // '0' = subtract 'gain_offset' from Q channel
    // '1' = subtract 'gain_offset' from I channel
    // envelope_threshold = the max-min value of signal that will cause gain
    // reduction wait_time = how long FSM waits for settling before making
    // another adjustment
    ANALOG_CFG_REG__14 = 0x0000;
    ANALOG_CFG_REG__15 = 0xA00F;

    // MF/CDR
    // Choose output polarity of demod
    set_asc_bit(103);

    // CDR feedback parameters
    tau_shift = 11;
    e_k_shift = 2;
    ANALOG_CFG_REG__3 = (tau_shift << 11) | (e_k_shift << 7);

    // Threshold used for packet detection
    correlation_threshold = 14;
    ANALOG_CFG_REG__9 = correlation_threshold;

    // Mux select bits to choose internal demod or external clk/data from gpio
    // '0' = on chip, '1' = external from GPIO
    clear_asc_bit(269);
    clear_asc_bit(270);

    // Set LDO reference voltage
    set_IF_LDO_voltage(0);

    // Set RST_B to analog_cfg[75]
    set_asc_bit(240);

    // Set RST_B = 1 (it is active low)
    // ANALOG_CFG_REG__4 = 0x2800;

    // Enable the polyphase filter
    // The polyphase is required for RX and is ideally off in TX
    // Changing it requires programming the ASC in between TX and RX modes
    // So for now just leaving it always enabled
    // Note this may trash the TX efficiency
    // And will also significantly affect the frequency change between TX and RX
    // ASC[30] |= 0x00100000;
    set_asc_bit(971);
}

// Must set IF clock frequency AFTER calling this function
void radio_init_rx_ZCC() {
    // int j;
    unsigned int mask1, mask2;
    unsigned int correlation_threshold;

    // IF uses ASC<271:500>, mask off outside that range
    mask1 = 0xFFFE0000;
    mask2 = 0x000007FF;
    ASC[8] &= mask1;
    ASC[15] &= mask2;

    // Set ASC bits for the analog RX blocks
    //	ASC[8] |= (0x0000FFF0	 & mask1);   //256-287
    //	ASC[9] = 0x00423188;   //288-319
    //	ASC[10] = 0x88020000;   //320-351
    //	ASC[11] = 0x000C0000;   //352-383
    //	ASC[12] = 0x00188102;   //384-415
    //	ASC[13] = 0x01603C44;   //416-447
    //	ASC[14] = 0x70010001;   //448-479
    //	ASC[15] |= (0xFFE02800 & mask2);   //480-511

    ASC[8] |= (0x0000FFF0 & ~mask1);   // 256-287
    ASC[9] = 0x00422188;               // 288-319
    ASC[10] = 0x88040071;              // 320-351
    ASC[11] = 0x100C4081;              // 352-383
    ASC[12] = 0x00188102;              // 384-415
    ASC[13] = 0x017FC844;              // 416-447
    ASC[14] = 0x70010001;              // 448-479
    ASC[15] |= (0xFFE00800 & ~mask2);  // 480-511

    // Set zcc demod parameters

    // Set clk/data mux to zcc
    // ASC<0:1> = [1 1]
    set_asc_bit(0);
    set_asc_bit(1);

    // Set counter threshold 122:107 MSB:LSB
    // for 76MHz, use 13
    set_zcc_demod_threshold(14);
    // for 64MHz, use 24?
    // set_zcc_demod_threshold();

    // Set clock divider value for zcc
    // The IF clock divided by this value must equal 2 MHz for 802.15.4
    set_IF_ZCC_clkdiv(38);

    // Set early decision margin to a large number to essentially disable it
    set_IF_ZCC_early(80);

    // Mux select bits to choose I branch as input to zcc demod
    set_asc_bit(238);
    set_asc_bit(239);

    // Mux select bits to choose internal demod or external clk/data from gpio
    // '0' = on chip, '1' = external from GPIO
    clear_asc_bit(269);
    clear_asc_bit(270);

    // Enable ZCC demod
    set_asc_bit(132);

    // Threshold used for packet detection
    correlation_threshold = 5;
    ANALOG_CFG_REG__9 = correlation_threshold;

    // Trim comparator offset
    set_IF_comparator_trim_I(0, 10);

    // Set LDO reference voltage
    set_IF_LDO_voltage(0);

    // Set RST_B to analog_cfg[75]
    set_asc_bit(240);

    // Set RST_B to ASC<241>
    // clear_asc_bit(240);

    // check to see what onchip zcc does when taken out of reset
    // set_asc_bit(241);

    // Leave baseband held in reset until RX activated
    // RST_B = 0 (it is active low)
    ANALOG_CFG_REG__4 = 0x2000;
    ANALOG_CFG_REG__4 = 0x2800;
}

void radio_init_tx() {
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
    set_asc_bit(997);
    set_asc_bit(996);
    set_asc_bit(998);
    set_asc_bit(999);

    // Make sure the BLE modulation mux is not also modulating the BLE DAC at
    // the same time Bit 1013 sets the BLE mod dac to cortex, since we are using
    // the pad for 15.4 here In the IC version, comment this line out (ie, leave
    // the ble mod source as the pad since 15.4 will use the cortex)
    set_asc_bit(1013);
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

    // Set current in LC tank
    set_LC_current(100);

    // Set LDO voltages for PA and LO
    set_PA_supply(63);
    set_LO_supply(64, 0);
}

void radio_init_divider(unsigned int div_value) {
    // int j;

    // Set divider LDO value to max
    set_DIV_supply(0, 0);

    // Disable /5 prescaler
    clear_asc_bit(1023);  // en
    set_asc_bit(1024);    // enb

    // Enable /2 prescaler
    set_asc_bit(1022);    // en
    clear_asc_bit(1021);  // enb

    // pre_dyn 2 0 1 = eb 0 1 2
    // eb2 turns on the other /2 (active low)
    // eb1 and eb0 turn on the other /5 thing --leave these high
    // pre_dyn<5:0> = asc<1030:1025>
    set_asc_bit(1025);
    set_asc_bit(1026);  // set this low to turn on the other /2; must disable
                        // all other dividers
    set_asc_bit(1027);

    // Activate 8MHz/20MHz output
    set_asc_bit(1033);

    // Enable static divider
    // set_asc_bit(1061);

    // Set sel12 = 1 (choose whether x2 is active)
    // set_asc_bit(1012);

    // Set divider controls to ASC
    // set_asc_bit(1081);

    // Release divider reset
    // set_asc_bit(1062);

    // Setting static divider N value
    // div_static_select starts at ASC(1049) and is 18 bits long
    // [static_code(11:16) static_code(5:10) static_en rstb static_code(1:4)]
    // The code is also inverted
    /*
    div_value = ~div_value & 0xFFFF;

    for(j=0; j<=5; j++){
            if((div_value >> (j+10)) & 0x1)
                    set_asc_bit(1049+j);
            else
                    clear_asc_bit(1049+j);
    }

    for(j=0; j<=5; j++){
            if((div_value >> (j+4)) & 0x1)
                    set_asc_bit(1054+j);
            else
                    clear_asc_bit(1054+j);
    }

            for(j=0; j<=3; j++){
            if((div_value >> j) & 0x1)
                    set_asc_bit(1063+j);
            else
                    clear_asc_bit(1063+j);
    }
    */
}

void read_counters_3B(unsigned int* count_2M, unsigned int* count_LC,
                      unsigned int* count_adc) {
    unsigned int rdata_lsb, rdata_msb;  //, count_LC, count_32k;

    // Disable all counters
    ANALOG_CFG_REG__0 = 0x007F;

    // Read 2M counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
    *count_2M = rdata_lsb + (rdata_msb << 16);

    // Read LC_div counter (via counter4)
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
    *count_LC = rdata_lsb + (rdata_msb << 16);

    // Read adc counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x300000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x340000);
    *count_adc = rdata_lsb + (rdata_msb << 16);

    // Reset all counters
    ANALOG_CFG_REG__0 = 0x0000;

    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;

    // printf("LC_count=%X\n",*count_LC);
    // printf("2M_count=%X\n",*count_2M);
    // printf("adc_count=%X\n\n",*count_adc);
}

void radio_enable_PA() {
    // Turn on PA via memory mapped register
    // LO should have already been enabled to allow it to settle
    // AUX LDO also needs to be turned on since the chip clock divider is on aux
    // supply
    ANALOG_CFG_REG__10 = 0x8028;
}

void radio_enable_LO() {
    // Turn on only LO via memory mapped register
    ANALOG_CFG_REG__10 = 0x0008;
}

void radio_enable_RX() {
    // Turn on LO, IF, and AUX LDOs via memory mapped register
    ANALOG_CFG_REG__10 = 0x0098;

    // Deassert reset on baseband
    // RST_B = 1 (it is active low)
    // analog_cfg[75]; note that analog_cfg[78:76] = sample_pt for MF CDR = 2
    ANALOG_CFG_REG__4 = 0x2800;
}

void radio_disable_all() {
    // Turn off all LDOs, including AUX
    ANALOG_CFG_REG__10 = 0x0000;

    // Put baseband back in reset until RX activated
    // RST_B = 0 (it is active low)
    // ANALOG_CFG_REG__4 = 0x2000;
}

// read IF estimate
unsigned int read_IF_estimate() {
    // Check valid flag
    if (ANALOG_CFG_REG__16 & 0x400)
        return ANALOG_CFG_REG__16 & 0x3FF;
    else
        return 0;
}

// Read Link Quality Indicator
unsigned int read_LQI() { return ANALOG_CFG_REG__21 & 0xFF; }

// Read RSSI - the gain control settings

unsigned int read_RSSI() { return ANALOG_CFG_REG__15 & 0xF; }

// set IF clock frequency
void set_IF_clock_frequency(int coarse, int fine, int high_range) {
    // Coarse and fine frequency tune, binary weighted
    // ASC<427:431> = RC_coarse<4:0> (<4(MSB):0>)
    // ASC<433:437> = RC_fine<4:0>   (<4(MSB):0>)

    unsigned int j;

    for (j = 0; j <= 4; j++) {
        if ((coarse >> j) & 0x1)
            set_asc_bit(431 - j);
        else
            clear_asc_bit(431 - j);
    }

    for (j = 0; j <= 4; j++) {
        if ((fine >> j) & 0x1)
            set_asc_bit(437 - j);
        else
            clear_asc_bit(437 - j);
    }

    // Switch between high and low speed ranges for IF RC:
    //'1' = high range
    // ASC<726> = RC_high_speed_mode
    if (high_range == 1)
        set_asc_bit(726);
    else
        clear_asc_bit(726);
}

// Set frequency for TI 20M oscillator
void set_sys_clk_secondary_freq(unsigned int coarse, unsigned int fine) {
    // coarse 0:4 = 860 861 875b 876b 877b
    // fine 0:4 870 871 872 873 874b

    int j;

    for (j = 0; j <= 3; j++) {
        if ((fine >> j) & 0x1)
            set_asc_bit(870 + j);
        else
            clear_asc_bit(870 + j);
    }
    if ((fine >> 4) & 0x1)
        clear_asc_bit(874);
    else
        set_asc_bit(874);

    for (j = 0; j <= 1; j++) {
        if ((coarse >> j) & 0x1)
            set_asc_bit(860 + j);
        else
            clear_asc_bit(860 + j);
    }
    for (j = 2; j <= 4; j++) {
        if ((coarse >> j) & 0x1)
            clear_asc_bit(873 + j);
        else
            set_asc_bit(873 + j);
    }
}

void do_fake_cal() {
    // Turn on LO, divider, IF
    radio_enable_LO();

    // Keep track of how many iterations of calibration have been done
    cal_iteration = 0;

    // This is the target count value to set the receiver's LO to the correct
    // frequency The count value is RF_freq / 3000 LC_target = 825000;

    // Reset and disable counters used for frequency calibration
    ANALOG_CFG_REG__0 = 0x0;

    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;

    // Set compare interrupt 0 to go off when count=1
    RFTIMER_REG__COMPARE0_CONTROL = 0x03;
    //--------------------------------------------------------
}

void packet_test_loop(unsigned int num_packets) {
    unsigned int packet_count = 0;
    int t;

    ICER = 0x81;

    radio_enable_LO();
    for (t = 0; t < 1000; t++);

    while (packet_count < num_packets) {
        // Turn on radio
        // radio_enable_LO();

        // Wait - how long between radio turn on and packet arrival
        // 50 ~ 50 us
        // for(t=0;t<1000;t++);

        // Reset baseband
        ANALOG_CFG_REG__4 = 0x2000;
        ANALOG_CFG_REG__4 = 0x2800;

        // Stop and restart RX mode
        RFCONTROLLER_REG__CONTROL = 0x10;
        DMA_REG__RF_RX_ADDR = &recv_packet[0];
        RFCONTROLLER_REG__CONTROL = 0x4;

        // Request a packet be sent by strobing GPIO-0
        // (repeat a few times to make sure it is a wide enough pulse)
        // repeat 3x = resulting pulse is 2us wide
        GPIO_REG__OUTPUT = 0x0001;
        GPIO_REG__OUTPUT = 0x0001;
        GPIO_REG__OUTPUT = 0x0001;
        GPIO_REG__OUTPUT = 0x0000;

        // Wait long enough for packet to have arrived
        for (t = 0; t < 15000; t++);

        // Turn off radio
        // radio_disable_all();

        // Increment packet counter
        packet_count++;

        // Wait
        // for(t=0;t<1000;t++);
    }
    radio_disable_all();
    ISER = 0x81;
}

void initialize_mote() {
    int t;

    // Start of new RX
    RFTIMER_REG__COMPARE0 = 1;

    // Turn on the RX
    RFTIMER_REG__COMPARE1 =
        expected_RX_arrival - guard_time - radio_startup_time;

    // Time to start listening for packet
    RFTIMER_REG__COMPARE2 = expected_RX_arrival - guard_time;

    // RX watchdog - packet never arrived
    RFTIMER_REG__COMPARE3 = expected_RX_arrival + guard_time;

    // RF Timer rolls over at this value and starts a new cycle
    RFTIMER_REG__MAX_COUNT = packet_interval;

    // Enable RF Timer
    RFTIMER_REG__CONTROL = 0x7;

    // Disable interrupts for the radio FSM
    radio_disable_interrupts();

    // Disable RF timer interrupts
    rftimer_disable_interrupts();

    //--------------------------------------------------------
    // SCM3B Analog Scan Chain Initialization
    //--------------------------------------------------------
    // Init LDO control - RX enabled by GPIO_PON
    init_ldo_control();

    // Set LDO reference voltages
    // set_VDDD_LDO_voltage(0);
    // set_AUX_LDO_voltage(0);
    // set_ALWAYSON_LDO_voltage(0);

    // Init GPIO - for access to all clocks
    // Select input banks for GPIO_PON and interrupts
    GPI_control(1, 0, 1, 0);

    // Setup access to clocks for cal
    // Need to keep first row set to bank 0 for optical bootload to keep working
    GPO_control(0, 10, 8, 10);

    // Only GPIO_PON is an input
    GPI_enables(0x0002);
    GPO_enables(0xFFFD);

    // Disable LF_CLOCK
    set_asc_bit(553);  // LF_CLOCK
    // set_asc_bit(830); //HF_CLOCK

    // Set initial coarse/fine on HF_CLOCK
    // coarse 0:4 = 860 861 875b 876b 877b
    // fine 0:4 870 871 872 873 874b
    set_sys_clk_secondary_freq(HF_CLOCK_coarse,
                               HF_CLOCK_fine);  // Close to 20MHz at room temp

    // Need to set crossbar so HF_CLOCK comes out divider_out_integ (gpio5,
    // bank10) This is connected to JA9 on FPGA which is HF_CLOCK
    set_asc_bit(1163);

    // Set passthrough on divide_out_integ divider
    set_asc_bit(40);

    // Set HCLK source as HF_CLOCK
    set_asc_bit(1147);

    // Set RFTimer source as HF_CLOCK
    set_asc_bit(1151);

    // HF_CLOCK will be trimmed to 20MHz, so set RFTimer div value to 40 to get
    // 500kHz (inverted, so 1101 0111)
    set_asc_bit(49);
    set_asc_bit(48);
    clear_asc_bit(47);
    set_asc_bit(46);
    clear_asc_bit(45);
    set_asc_bit(44);
    set_asc_bit(43);
    set_asc_bit(42);

    // Set 2M RC as source for chip CLK
    set_asc_bit(1156);

    // Route IF ADC_CLK out of BLE_PDA_clk for intitial optical calibration
    // (Can't use ADC_CLK and optical signals at the same time since they are
    // all in the first group of four) This pin is shared with Q_LC[2], so that
    // pin was connected to counter 4 in FPGA Comment these two lines out in IC
    // version
    set_asc_bit(1180);
    set_asc_bit(1181);

    // Then set the divider for BLE_PDA_clk to passthrough
    // Comment this line out in IC version
    set_asc_bit(524);

    // Enable 32k for cal
    set_asc_bit(623);

    // Enable passthrough on chip CLK divider
    set_asc_bit(41);

    // Init counter setup - set all to analog_cfg control
    // ASC[0] is leftmost
    // ASC[0] |= 0xFF800000;
    for (t = 2; t < 22; t++) set_asc_bit(t);

    // Init RX
    radio_init_rx_MF();

    // Init TX
    radio_init_tx();

    // Set initial IF ADC clock frequency
    set_IF_clock_frequency(IF_coarse, IF_fine, 0);

    // Set initial TX clock frequency
    set_2M_RC_frequency(31, 31, 21, 17, RC2M_superfine);

    // Turn on RC 2M for cal
    set_asc_bit(1114);

    // Set initial LO frequency
    LC_monotonic_ASC(LC_code);

    // Init divider settings
    radio_init_divider(2000);

    // Program analog scan chain on SCM3B
    analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
    analog_scan_chain_load_3B_fromFPGA();
    //--------------------------------------------------------

    //--------------------------------------------------------
    // FPGA Analog Scan Chain Initialization
    //--------------------------------------------------------

    // Copy settings from 3B ASC
    for (t = 0; t <= 38; t++) {
        ASC_FPGA[t] = ASC[t];
    }

    // Set up GPIO settings for FPGA
    // There are no direction controls on FPGA; hard-wired
    // Connect all GPOs to cortex GP outputs
    GPO_control_FPGA(6, 5, 10, 10);
    // GPO_control_FPGA(0,0,0,0);
    // GPI_control_FPGA(2,3,3,3);

    // Program analog scan chain on FPGA
    analog_scan_chain_write(&ASC_FPGA[0]);
    analog_scan_chain_load();

    // GPIO_PON is connected to the LO turn-on signal

    // printf("Initialization Complete\n");
    //--------------------------------------------------------

    // radio_enable_LO();
}

unsigned int build_RX_channel_table(unsigned int channel_11_LC_code) {
    unsigned int rdata_lsb, rdata_msb;
    int t, ii = 0;
    unsigned int count_LC[16] = {0};
    unsigned int count_targets[17] = {0};

    RX_channel_codes[0] = channel_11_LC_code;

    // for(ii=0; ii<16; ii++){
    while (ii < 16) {
        LC_monotonic_ASC(RX_channel_codes[ii]);
        analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
        analog_scan_chain_load_3B_fromFPGA();

        // Reset all counters
        ANALOG_CFG_REG__0 = 0x0000;

        // Enable all counters
        ANALOG_CFG_REG__0 = 0x3FFF;

        // Count for some arbitrary amount of time
        for (t = 1; t < 16000; t++);

        // Disable all counters
        ANALOG_CFG_REG__0 = 0x007F;

        // Read count result
        rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
        rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
        count_LC[ii] = rdata_lsb + (rdata_msb << 16);

        count_targets[ii + 1] = ((961 + (ii + 1) * 2) * count_LC[0]) / 961;

        // Adjust LC_code to match new target
        if (ii > 0) {
            if (count_LC[ii] < (count_targets[ii] - 20)) {
                RX_channel_codes[ii]++;
            } else {
                RX_channel_codes[ii + 1] = RX_channel_codes[ii] + 40;
                ii++;
            }
        }

        if (ii == 0) {
            RX_channel_codes[ii + 1] = RX_channel_codes[ii] + 40;
            ii++;
        }
    }

    // for(ii=0; ii<16; ii++){
    //	printf("\nRX ch=%d,  count_LC=%d,  count_targets=%d,
    // RX_channel_codes=%d",ii+11,count_LC[ii],count_targets[ii],RX_channel_codes[ii]);
    // }

    return count_LC[0];
}

void build_TX_channel_table(unsigned int channel_11_LC_code,
                            unsigned int count_LC_RX_ch11) {
    unsigned int rdata_lsb, rdata_msb;
    int t, ii = 0;
    unsigned int count_LC[16] = {0};
    unsigned int count_targets[17] = {0};

    unsigned short nums[16] = {802, 904, 929, 269, 949, 434, 369, 578,
                               455, 970, 139, 297, 587, 109, 373, 159};
    unsigned short dens[16] = {801, 901, 924, 267, 940, 429, 364, 569,
                               447, 951, 136, 290, 572, 106, 362, 154};

    // Need to adjust here for shift from PA
    TX_channel_codes[0] = channel_11_LC_code;

    // for(ii=0; ii<16; ii++){
    while (ii < 16) {
        LC_monotonic_ASC(TX_channel_codes[ii]);
        analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
        analog_scan_chain_load_3B_fromFPGA();

        // Reset all counters
        ANALOG_CFG_REG__0 = 0x0000;

        // Enable all counters
        ANALOG_CFG_REG__0 = 0x3FFF;

        // Count for some arbitrary amount of time
        for (t = 1; t < 16000; t++);

        // Disable all counters
        ANALOG_CFG_REG__0 = 0x007F;

        // Read count result
        rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
        rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
        count_LC[ii] = rdata_lsb + (rdata_msb << 16);

        // Until figure out why modulation spacing is only 800kHz, only set
        // 400khz above RF channel
        count_targets[ii] = (nums[ii] * count_LC_RX_ch11) / dens[ii];
        // count_targets[ii] = ((24054 + ii*50) * count_LC_RX_ch11) / 24025;
        // count_targets[ii] = ((24055 + ii*50) * count_LC_RX_ch11) / 24025;

        if (count_LC[ii] < (count_targets[ii] - 5)) {
            TX_channel_codes[ii]++;
        } else {
            TX_channel_codes[ii + 1] = TX_channel_codes[ii] + 40;
            ii++;
        }
    }

    // for(ii=0; ii<16; ii++){
    //	printf("\nTX ch=%d,  count_LC=%d,  count_targets=%d,
    // TX_channel_codes=%d",ii+11,count_LC[ii],count_targets[ii],TX_channel_codes[ii]);
    // }
}

void build_channel_table(unsigned int channel_11_LC_code) {
    unsigned int count_LC_RX_ch11;

    // Make sure in RX mode first

    count_LC_RX_ch11 = build_RX_channel_table(channel_11_LC_code);

    // printf("--\n");

    // Switch over to TX mode

    // Turn polyphase off for TX
    clear_asc_bit(971);

    // Hi-Z mixer wells for TX
    set_asc_bit(298);
    set_asc_bit(307);

    // Analog scan chain setup for radio LDOs for RX
    clear_asc_bit(504);  // = gpio_pon_en_if
    set_asc_bit(506);    // = gpio_pon_en_lo
    set_asc_bit(508);    // = gpio_pon_en_pa

    build_TX_channel_table(channel_11_LC_code, count_LC_RX_ch11);

    radio_disable_all();
}

unsigned int estimate_temperature_2M_32k() {
    unsigned int rdata_lsb, rdata_msb;
    unsigned int count_2M, count_32k;
    int t;

    // Reset all counters
    ANALOG_CFG_REG__0 = 0x0000;

    // Enable all counters
    ANALOG_CFG_REG__0 = 0x3FFF;

    // Count for some arbitrary amount of time
    for (t = 1; t < 50000; t++);

    // Disable all counters
    ANALOG_CFG_REG__0 = 0x007F;

    // Read 2M counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x180000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x1C0000);
    count_2M = rdata_lsb + (rdata_msb << 16);

    // Read 32k counter
    rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x000000);
    rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x040000);
    count_32k = rdata_lsb + (rdata_msb << 16);

    // printf("%d - %d - %d\n",count_2M,count_32k,(count_2M << 13) / count_32k);

    return (count_2M << 13) / count_32k;
}
