#ifndef LIGHTHOUSE_HEADER
#define LIGHTHOUSE_HEADER

#include <stdbool.h>
#include <stdint.h>

#include "spi.h"

typedef enum pulse_type_t {
    AZ = 0,
    AZ_SKIP = 1,
    EL = 2,
    EL_SKIP = 3,
    LASER = 4,
    INVALID = 5
} pulse_type_t;
typedef enum lh_id_t { A = 0, B = 1 } lh_id_t;
typedef enum angle_type_t { AZIMUTH = 0, ELEVATION = 1 } angle_type_t;
// contains the current gpio state and the time that it first transitioned
typedef struct gpio_tran_t {
    unsigned short gpio;
    unsigned int timestamp_tran;
} gpio_tran_t;

#define DEBUG_STATE 0
#define DEB_THRESH 2
#define WIDTH_BIAS 0
#define USE_RADIO 1
#define DEBUG_INT 0
#define IMU_CODE 105  // code that tells code that its an imu packet

// defines for lighthouse localization scum configuration
#define HF_CLOCK_FINE_LH 17
#define HF_CLOCK_COARSE_LH 3
#define IF_COARSE_LH 22
#define IF_FINE_LH 18

// RC 2MHz tuning settings
// This the transmitter chip clock
#define RC2M_COARSE_LH 21
#define RC2M_FINE_LH 15
#define RC2M_SUPERFINE_LH 15
#define LC_CODE_LH 680

// interrupt defines
#define GPIO8_HIGH_INT 0x1000
#define GPIO9_LOW_INT 0x2000
#define GPIO10_LOW_INT 0x4000

// functions
pulse_type_t classify_pulse(unsigned int timestamp_rise,
                            unsigned int timestamp_fall);
void update_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void azimuth_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void update_state_azimuth(pulse_type_t pulse_type, unsigned int timestamp_rise);
unsigned int sync_pulse_width_compensate(unsigned int pulse_width);
void update_state_elevation(pulse_type_t pulse_type,
                            unsigned int timestamp_rise);
void debounce_gpio(unsigned short gpio, unsigned short* deb_gpio,
                   unsigned int* trans_out);
void initialize_mote_lighthouse(void);
void radio_init_tx_lighthouse(uint8_t lo_supply_v, uint8_t lc_supply_c,
                              uint8_t pa_supply_v, bool lo_cortex_ctrl,
                              bool div_cortex_ctrl);
void radio_init_rx_MF_lighthouse(void);
void send_lh_packet(unsigned int sync_time, unsigned int laser_time,
                    lh_id_t lighthouse, angle_type_t angle_type);
void lh_int_cb(int level);
void send_imu_packet(imu_data_t imu_measurement);
// defines for initialization

#define LC_CURRENT_LH 100
#define PA_SUPPLY_LH 63
#define LO_SUPPLY_LH 64

#endif