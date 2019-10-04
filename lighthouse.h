typedef enum pulse_type_t{AZ=0,AZ_SKIP=1,EL=2,EL_SKIP=3,LASER=4,INVALID = 5} pulse_type_t;
//contains the current gpio state and the time that it first transitioned
typedef struct gpio_tran_t {
	unsigned short gpio;
	unsigned int timestamp_tran;
}gpio_tran_t;
#define DEBUG_STATE 0
#define DEB_THRESH 1
//functions
pulse_type_t classify_pulse(unsigned int timestamp_rise, unsigned int timestamp_fall);
void update_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void azimuth_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void update_state_azimuth(pulse_type_t pulse_type, unsigned int timestamp_rise);
unsigned int sync_pulse_width_compensate(unsigned int pulse_width);
void update_state_elevation(pulse_type_t pulse_type, unsigned int timestamp_rise);
gpio_tran_t debounce_gpio(unsigned short gpio);