#include <stdio.h>
#include <string.h>

#include "ads1299.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "rftimer.h"
#include "gpio.h"
#include "tuning.h"
#include "channel_cal.h"
//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))
#define PWM_TIMER_ID 2

// Start coarse code for the sweep to find 802.15.4 channels.
#define START_COARSE_CODE 20
// End coarse code for the sweep to find 802.15.4 channels.
#define END_COARSE_CODE 27
// 802.15.4 channel on which to transmit the ADC data.
#define IEEE_802_15_4_TX_CHANNEL 17

//=========================== variables =======================================

typedef struct {
    ads_data_t ads_measurement[100];
    uint8_t pwm_counter;
    uint8_t dac_setpoint;
    uint8_t sine_idx;
} app_vars_t;

static tuning_code_t g_tuning_code;

#define SINE_LUT_SIZE 4 
uint8_t sine_lut[] = {4, 9, 4, 0};

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

void pwm_isr(void) {
  app_vars.pwm_counter++;
  
  if(app_vars.pwm_counter >= 9 ) {
    gpio_1_set();
    app_vars.pwm_counter = 0;
    app_vars.sine_idx = (app_vars.sine_idx + 1) % SINE_LUT_SIZE;
    app_vars.dac_setpoint = sine_lut[app_vars.sine_idx];
    delay_ticks_asynchronous(100, PWM_TIMER_ID);
  }
  else if(app_vars.pwm_counter>=app_vars.dac_setpoint) {
    gpio_1_clr();
    delay_ticks_asynchronous(100, PWM_TIMER_ID);
  }
  else {
    delay_ticks_asynchronous(100, PWM_TIMER_ID);
  }
}

void init_pwm_dac(uint16_t freq, uint8_t duty_cycle)
{
  // Interrupt driven PWM DAC
  // 10ms chunks of time = 100 Hz
  app_vars.pwm_counter = 0;
  app_vars.sine_idx = 0;
  app_vars.dac_setpoint = sine_lut[app_vars.sine_idx];

  // Set up the RF timer interrupt routine
  rftimer_enable_interrupts();
  rftimer_enable_interrupts_by_id(PWM_TIMER_ID);
  //rftimer_set_repeat(true, PWM_TIMER_ID); // Have to manually do repeat for ticks-based
  rftimer_set_callback_by_id(pwm_isr, PWM_TIMER_ID);

  // Set up the GPIO 
  gpio_1_set();
  GPO_enable_set(1);
  GPI_enable_clr(1);
  // Kick off the PWM DAC
  delay_ticks_asynchronous(100, PWM_TIMER_ID);
}

int main(void) {
    uint32_t i, j;
    unsigned char print_reg;
    uint32_t Nsample = 2;
    uint8_t rreg;
    uint32_t adc_data[2];

    /***    Initialize SCuM     ****/
    initialize_mote();

    printf("Initializing the channel calibration.\n");
    if (!channel_cal_init(START_COARSE_CODE, END_COARSE_CODE)) {
        return 0;
    }

    crc_check();
    perform_calibration();

    GPI_enable_clr(4);
    GPO_enable_set(4);

    /***    Initialize Radio     ****/
    printf("RUN CH CAL\n");
    // Run the channel calibration.
    if (!channel_cal_run()) {
        printf("CH CAL FAIL\n");
        return 0;
    }
    // Get the tuning code for the 802.15.4 channel we want to use.
    while(1) {
      if (!channel_cal_get_tx_tuning_code(IEEE_802_15_4_TX_CHANNEL, &g_tuning_code)) {
          printf("No TX tuning code found for channel %u.\n", IEEE_802_15_4_TX_CHANNEL);
          
      }
    }
    printf("Transmitting on channel %u: (%u, %u, %u).\n",
           IEEE_802_15_4_TX_CHANNEL, g_tuning_code.coarse,
           g_tuning_code.mid, g_tuning_code.fine);

    // Bring ADS's CSB pin high so it has a chance to throw away HCLK garbage
    memset(&app_vars, 0, sizeof(app_vars_t));
    delay_milliseconds_synchronous(100, 1);
    
    GPI_enable_clr(4);
    GPO_enable_set(4);
    gpio_4_set();

    delay_milliseconds_synchronous(1000, 1);
		
		gpio_4_clr();

    printf("Power up ADS1299 NOW!\r\n");
    //delay_milliseconds_synchronous(1, 1000);

    /***    Initialize ADS1299     ****/
    ADS_initialize();
    for(i=0; i<1000; i++) {
      ADS_RESET();
      ADS_SDATAC();
    }
    
    print_reg = ADS_RREG(0x00);
    printf("ID: %x\r\n", print_reg);        // should return 3E
    print_reg = ADS_RREG(0x05);
    printf("channel1: %x\r\n", print_reg);  // print the config off the ADS
    ADS_WREG(0x05, 0x00);                   // enable channel 1 (0x60 = 0110 0000 where 110 is gain 24)
    print_reg = ADS_RREG(0x05);             // confirm channel 1 is enabled
    printf("channel1: %x\r\n", print_reg);
    ADS_WREG(0x03, 0xE0);                   // change the config on ADS
    print_reg = ADS_RREG(0x03);             // confirm the config on ADS
    printf("CONFIG3: %x\n", print_reg);

    for (rreg = 0x00; rreg < 0x18; rreg = rreg + 0x01) {
        print_reg = ADS_RREG(rreg);
        printf("%x\r\n", print_reg);
    }

    ADS_RREGS(0x00, 0x17);


    /***    Kickoff the PWM DAC interrupt routine     ****/
    init_pwm_dac(100, 50);

    /***    Read ADS data     ****/
    ADS_START();
    ADS_RDATAC();

    for (i = 0; i < 10; i++)
        ;

    while(1) {
      
      for (i = 0; i < Nsample; i++) {
          ADS_POLL_MEASUREMENTS(&app_vars.ads_measurement[i]);
      }
      
      for (i = 0; i < Nsample; i++) {
          for (j = 0; j < 1; j++) {
              adc_data[j] = app_vars.ads_measurement[i].channel[j];
              //printf("0x%x\r\n", app_vars.ads_measurement[i].channel[j]);
          }
      }
      send_packet(&adc_data, sizeof(adc_data));
      delay_milliseconds_synchronous(10, 1);
    }
    printf("exit\n");
}
//=========================== public ==========================================

//=========================== private =========================================
