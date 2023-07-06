#include <stdio.h>
#include <string.h>

#include "ads1299.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "rftimer.h"
#include "gpio.h"
//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

//=========================== variables =======================================

typedef struct {
    ads_data_t ads_measurement[100];
    uint8_t pwm_counter;
    uint8_t dac_setpoint;
    uint8_t sine_idx;
} app_vars_t;

uint8_t sine_lut[] = {4, 9, 0, 4};

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

void pwm_isr(void) {
  app_vars.pwm_counter++;
  if(app_vars.pwm_counter==app_vars.dac_setpoint) {
    gpio_1_clr();
    delay_milliseconds_asynchronous(1, 1);
  }
  else if(app_vars.pwm_counter==10) {
    gpio_1_set();
    delay_milliseconds_asynchronous(1, 1);
    app_vars.pwm_counter = 0;
    app_vars.sine_idx++;
    app_vars.dac_setpoint = sine_lut[app_vars.sine_idx];
  }
  else {
    delay_milliseconds_asynchronous(1, 1);
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
  rftimer_enable_interrupts_by_id(1);
  rftimer_set_repeat(true, 1);
  rftimer_set_callback_by_id(pwm_isr, 1);

  // Set up the GPIO 
  gpio_1_set();
  GPO_enable_set(1);
  GPI_enable_clr(1);
  // Kick off the PWM DAC
  delay_milliseconds_asynchronous(1, 100);
}

int main(void) {
    uint32_t i, j;
    unsigned char print_reg;
    uint32_t Nsample = 100;
    uint8_t rreg;

    memset(&app_vars, 0, sizeof(app_vars_t));

    /***    Initialize SCuM     ****/
    initialize_mote();
    crc_check();
    perform_calibration();

    printf("Power up ADS1299 NOW!\r\n");
    //delay_milliseconds_synchronous(1, 1000);

    /***    Initialize ADS1299     ****/
    ADS_initialize();
    ADS_RESET();
    ADS_SDATAC();
    print_reg = ADS_RREG(0x00);
    printf("ID: %x\r\n", print_reg);        // should return 3E
    print_reg = ADS_RREG(0x05);
    printf("channel1: %x\r\n", print_reg);  // print the config off the ADS
    ADS_WREG(0x05, 0x60);                   // enable channel 1
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
      
      printf("read ads data\n");
      for (i = 0; i < Nsample; i++) {
          ADS_POLL_MEASUREMENTS(&app_vars.ads_measurement[i]);
      }
      printf("print ads data\n");
      for (i = 0; i < Nsample; i++) {
          // printf("%x\n",app_vars.ads_measurement[i].config);
          printf("printf sample %d", i);
          for (j = 0; j < 1; j++) {
              printf("0x%x\r\n", app_vars.ads_measurement[i].channel[j]);
          }
      }
      delay_milliseconds_synchronous(1, 1);
    }
    printf("exit\n");
}
//=========================== public ==========================================

//=========================== private =========================================
