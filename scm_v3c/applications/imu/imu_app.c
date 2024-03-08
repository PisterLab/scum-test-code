//------------------------------------------------------------------------------
// u-robot Digital Controller Firmware
//------------------------------------------------------------------------------

#include <math.h>
#include <rt_misc.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "Int_Handlers.h"
#include "Memory_map.h"
#include "bucket_o_functions.h"
#include "lighthouse.h"
#include "rf_global_vars.h"
#include "scm3C_hardware_interface.h"
#include "scm3_hardware_interface.h"
#include "scum_radio_bsp.h"
#include "spi.h"

extern unsigned int current_lfsr;

extern char send_packet[127];
extern unsigned int ASC[38];

// Bootloader will insert length and pre-calculated CRC at these memory
// addresses
#define crc_value (*((unsigned int*)0x0000FFFC))
#define code_length (*((unsigned int*)0x0000FFF8))

// Target radio LO freq = 2.4025G
// Divide ratio is currently 480*2
// Calibration counts for 100ms
unsigned int LC_target = 250187;
unsigned int LC_code = 680;

// HF_CLOCK tuning settings
unsigned int HF_CLOCK_fine = 17;
unsigned int HF_CLOCK_coarse = 3;

// RC 2MHz tuning settings
// This the transmitter chip clock
unsigned int RC2M_coarse = 21;
unsigned int RC2M_fine = 15;
unsigned int RC2M_superfine = 15;

// Receiver clock settings
// The receiver chip clock is derived from this clock
unsigned int IF_clk_target = 1600000;
unsigned int IF_coarse = 22;
unsigned int IF_fine = 18;

unsigned int cal_iteration = 0;
unsigned int run_test_flag = 0;
unsigned int num_packets_to_test = 1;

unsigned short optical_cal_iteration = 0, optical_cal_finished = 0;

unsigned short doing_initial_packet_search;
unsigned short current_RF_channel;
unsigned short do_debug_print = 0;

// Variables for lighthouse RX
unsigned short current_gpio = 0, last_gpio = 0, state = 0, nextstate = 0,
               pulse_type = 0;
unsigned int timestamp_rise, timestamp_fall, pulse_width;
// unsigned int azimuth_delta, elevation_delta;
unsigned int azimuth_t1, elevation_t1, azimuth_t2, elevation_t2;
unsigned short num_data_points = 0, target_num_data_points;
unsigned int azimuth_t1_data[1000], elevation_t1_data[1000],
    azimuth_t2_data[1000], elevation_t2_data[1000];

unsigned int pulse_width_data[1000];

void test_LC_sweep_tx(void) {
    /*
    Inputs:
    Outputs:
    Note:
            Initialization of the scan settings should already have
            been performed.
    */
    int gpo_state = 0;
    int coarse, mid, fine;
    unsigned int iterations = 3;
    unsigned int i;
    // store measurement
    imu_data_t imu_measurement;

    // test_imu_life();
    imu_measurement.acc_x.value = 200;
    imu_measurement.acc_y.value = -200;
    imu_measurement.acc_z.value = 1000;
    imu_measurement.gyro_x.value = -1000;
    imu_measurement.gyro_y.value = -1234;
    imu_measurement.gyro_z.value = 1111;

    // Enable the TX. NB: Requires 50us for frequency settling
    // transient.
    radio_txEnable();

    while (1) {
        for (coarse = 20; coarse < 27; coarse++) {
            for (mid = 0; mid < 32; mid++) {
                for (fine = 0; fine < 32; fine++) {
                    imu_measurement.acc_x.value = coarse & 0x1F;
                    imu_measurement.acc_y.value = mid & 0x1F;
                    imu_measurement.acc_z.value = fine & 0x1F;
                    // Construct the packet
                    // with payload {coarse, mid, fine} in
                    // separate bytes

                    // place packet type code in first byte of packet
                    send_packet[0] = IMU_CODE;

                    // place imu acc x data into the rest of the packet (lsb
                    // first)
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

                    // Set the LC frequency
                    // LC_FREQCHANGE(22&0x1F, 21&0x1F, 4&0x1F);
                    LC_FREQCHANGE(coarse & 0x1F, mid & 0x1F, fine & 0x1F);
                    // TODO: Wait for at least 50us
                    GPIO_REG__OUTPUT = ~GPIO_REG__OUTPUT;
                    // gpo toggle on all gpos
                    for (i = 0; i < 5000; i++) {}

                    // send packet
                    radio_txNow();

                    // Send bits out the radio thrice for redundancy
                    for (i = 0; i < iterations; i++) {
                        for (i = 0; i < 5000; i++) {}
                        // send packet
                        radio_txNow();
                        // send_imu_packet(imu_measurement);
                    }
                }
            }
        }
    }
}

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
    unsigned int calc_crc;
    int i;

    printf("Initializing...");

    // Set up mote configuration
    // This function handles all the analog scan chain setup
    // Do not call any other scm3C or scm3 hardware interface functions, they
    // will mess up the analog scanchain functions This should be independent of
    // all the random global variable settings
    initialize_mote_lighthouse();

    // Check CRC to ensure there were no errors during optical programming
    printf("\n-------------------\n");
    printf("Validating program integrity...");

    calc_crc = crc32c(0x0000, code_length);

    if (calc_crc == crc_value) {
        printf("CRC OK\n");
    } else {
        printf(
            "\nProgramming Error - CRC DOES NOT MATCH - Halting Execution\n");
        while (1) {
            // GPIO_REG__OUTPUT = ~GPIO_REG__OUTPUT;
        }
    }

    // After bootloading the next thing that happens is frequency calibration
    // using optical
    printf("Calibrating frequencies...\n");

    // Initial frequency calibration will tune the frequencies for HCLK, the
    // RX/TX chip clocks, and the LO For the LO, calibration for RX channel 11,
    // so turn on AUX, IF, and LO LDOs Aux is inverted (0 = on) Memory-mapped
    // LDO control ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN
    // | PA_MUX | IF_MUX | LO_MUX For MUX signals, '1' = FSM control, '0' =
    // memory mapped control For EN signals, '1' = turn on LDO Turn on LO, DIV,
    // PA
    ANALOG_CFG_REG__10 = 0x78;

    // Turn off polyphase and disable mixer
    ANALOG_CFG_REG__16 = 0x6;

    // Enable optical SFD interrupt for optical calibration
    ISER = 0x0800;

    // Wait for optical cal to finish
    while (optical_cal_finished == 0);
    optical_cal_finished = 0;

    ICER = 0xFFFF;
    printf("Cal complete\n");

    // run frequency cal sweep

    // test_LC_sweep_tx();

    // enable gpio 8 and gpio 9 interrupts (interrupts 1 and 2) (gpio 9 might
    // not work)
    ISER = GPIO9_LOW_INT | GPIO8_HIGH_INT;

    // Reset RF Timer count register
    RFTIMER_REG__COUNTER = 0x0;

    // Number of data points to gather before printing
    target_num_data_points = 120;

    /*
    while(1){
            imu_data_t imu_measurement;
            for( i = 0; i < 100000; i++);
            //current_gpio = GPIO_REG__INPUT;
            //imu_measurement.acc_x.value = current_gpio;
            //send_imu_packet(imu_measurement);
            test_imu_life();
    }*/

    // The optical_data_raw signal is not synchronized to HCLK domain so could
    // possibly see glitching problems
    last_gpio = current_gpio;
    current_gpio = (0x8 & GPIO_REG__INPUT) >> 3;

    test_imu_life();

    write_imu_register(0x06, 0x41);
    for (i = 0; i < 50000; i++);
    write_imu_register(0x06, 0x01);
    for (i = 0; i < 50000; i++);

    // start localization loop
    while (1) {
        // poll imu

        // store measurement
        imu_data_t imu_measurement;

        // test_imu_life();
        // GPIO_REG__OUTPUT = ~GPIO_REG__OUTPUT;

        imu_measurement.acc_x.value = read_acc_x();
        imu_measurement.acc_y.value = read_acc_y();
        imu_measurement.acc_z.value = read_acc_z();
        imu_measurement.gyro_x.value = read_gyro_x();
        imu_measurement.gyro_y.value = read_gyro_y();
        imu_measurement.gyro_z.value = read_gyro_z();

        // send measurement
        for (i = 0; i < 25000; i++) {}
        send_imu_packet(imu_measurement);

        // use interrupts intesad of polling
        /*
        // Read GPIO<3> (optical_data_raw - this is the digital output of the
        optical receiver)
        // The optical_data_raw signal is not synchronized to HCLK domain so
        could possibly see glitching problems last_gpio = current_gpio;
        current_gpio = (0x8 & GPIO_REG__INPUT) >> 3;

        // Detect rising edge
        if(last_gpio == 0 && current_gpio == 1){

                // Save when this event happened
                timestamp_rise = RFTIMER_REG__COUNTER;

        }

        // Detect falling edge
        else if(last_gpio == 1 && current_gpio == 0){

                // Save when this event happened
                timestamp_fall = RFTIMER_REG__COUNTER;

                update_state(classify_pulse(timestamp_rise,
        timestamp_fall),timestamp_rise);
        }	*/
    }
}
