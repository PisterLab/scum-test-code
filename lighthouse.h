typedef enum pulse_type_t{AZ=0,AZ_SKIP=1,EL=2,EL_SKIP=3,LASER=4,INVALID = 5} pulse_type_t;
typedef enum lh_id_t {A = 0, B = 1} lh_id_t;
typedef enum angle_type_t {AZIMUTH = 0, ELEVATION = 1} angle_type_t;
//contains the current gpio state and the time that it first transitioned
typedef struct gpio_tran_t {
	unsigned short gpio;
	unsigned int timestamp_tran;
}gpio_tran_t;
#define DEBUG_STATE 0
#define DEB_THRESH 2
#define WIDTH_BIAS 0
#define USE_RADIO 1

//defines for lighthouse localization scum configuration
#define HF_CLOCK_FINE_LH  		17
#define HF_CLOCK_COARSE_LH 		3
#define IF_COARSE_LH 					22
#define IF_FINE_LH  					18

	// RC 2MHz tuning settings
// This the transmitter chip clock
#define RC2M_COARSE_LH 			21
#define RC2M_FINE_LH 				15
#define RC2M_SUPERFINE_LH 	15
#define LC_CODE_LH 					680


//functions
pulse_type_t classify_pulse(unsigned int timestamp_rise, unsigned int timestamp_fall);
void update_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void azimuth_state(pulse_type_t pulse_type, unsigned int timestamp_rise);
void update_state_azimuth(pulse_type_t pulse_type, unsigned int timestamp_rise);
unsigned int sync_pulse_width_compensate(unsigned int pulse_width);
void update_state_elevation(pulse_type_t pulse_type, unsigned int timestamp_rise);
void debounce_gpio(unsigned short gpio, unsigned short * deb_gpio, unsigned int * trans_out);
