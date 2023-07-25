#include <stdio.h>
#include <string.h>

#include "ads1299.h"
#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "rftimer.h"
#include "gpio.h"
#include "tuning.h"
#include "radio.h"
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

static tuning_code_t g_tuning_code = {
  .coarse = 23, 
  .mid = 18,
  .fine = 2
};

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
    uint32_t Nsample = 9;
    uint8_t rreg;
    int32_t adc_data[32];
    uint8_t wreg_val = 0x0;

    /***    Initialize SCuM     ****/
    initialize_mote();

    printf("Initializing the channel calibration.\n");
    if (!channel_cal_init(START_COARSE_CODE, END_COARSE_CODE)) {
        return 0;
    }

    crc_check();
    perform_calibration();


    /***    Initialize Radio     ****/
    /*
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
    */
    
    tuning_tune_radio(&g_tuning_code);
    printf("Transmitting on channel %u: (%u, %u, %u).\n",
           IEEE_802_15_4_TX_CHANNEL, g_tuning_code.coarse,
           g_tuning_code.mid, g_tuning_code.fine);
    tuning_tune_radio(&g_tuning_code);
    tuning_tune_radio(&g_tuning_code);
    tuning_tune_radio(&g_tuning_code);
    tuning_tune_radio(&g_tuning_code);
    tuning_tune_radio(&g_tuning_code);
    tuning_tune_radio(&g_tuning_code);

    GPO_control(6, 6, 6, 6);  // 0 in 3rd arg connects clk_3wb to GPO8 for 3WB cal

    GPI_enable_clr(4);
    GPO_enable_set(4);

    // High-Z for rev. 1 boards
    GPO_enable_clr(7);
    GPI_enable_clr(7);

    // Program analog scan chain
    analog_scan_chain_write();
    analog_scan_chain_load();

    gpio_7_set();

    // Bring ADS's CSB pin high so it has a chance to throw away HCLK garbage
    memset(&app_vars, 0, sizeof(app_vars_t));
    delay_milliseconds_synchronous(100, 1);

    gpio_4_set();

    delay_milliseconds_synchronous(1000, 1);
		
		gpio_4_clr();

    printf("Power up ADS1299 NOW!\r\n");
    //delay_milliseconds_synchronous(1, 1000);

    /***    Initialize ADS1299     ****/
    ads_init();
    for(i=0; i<100; i++) {
      ads_reset();
      ads_sdatac();
    }
    
    print_reg = ads_rreg(ADS_REG_ID);
    printf("ID: %x\r\n", print_reg);        // should return 3E


    // REG_CHnSet
    
    print_reg = ads_rreg(ADS_REG_CH1SET);
    printf("channel1: %x\r\n", print_reg);  // print the config off the ADS
    // Power-down
    //This bit determines the channel power mode for the
    //corresponding channel.
    //0 : Normal operation
    //1 : Channel power-down.
    //When powering down a channel, TI recommends that the
    //channel be set to input short by setting the appropriate
    //MUXn[2:0] = 001 of the CHnSET register.
    wreg_val = 0x0 << 7; // 0 - normal operation, 1 - channel power-down
    // PGA gain
    // These bits determine the PGA gain setting.
    // 000 : 1
    // 001 : 2
    // 010 : 4
    // 011 : 6
    // 100 : 8
    // 101 : 12
    // 110 : 24
    // 111 : Do not use
    wreg_val |= 0x0 << 4; // 0 - gain 1, 1 - gain 2, 2 - gain 4, 3 - gain 6, 4 - gain 8, 5 - gain 12, 6 - gain 24
    // SRB2 connection
    // This bit determines the SRB2 connection for the corresponding
    // channel.
    // 0 : Open
    // 1 : Closed
    wreg_val |= 0x0 << 3; // 0 - open, 1 - closed
    // These bits determine the channel input selection.
    // 000 : Normal electrode input
    // 001 : Input shorted (for offset or noise measurements)
    // 010 : Used in conjunction with BIAS_MEAS bit for BIAS
    // measurements.
    // 011 : MVDD for supply measurement
    // 100 : Temperature sensor
    // 101 : Test signal
    // 110 : BIAS_DRP (positive electrode is the driver)
    // 111 : BIAS_DRN (negative electrode is the driver)
    wreg_val |= 0x1;

    ads_wreg(ADS_REG_CH1SET, wreg_val);                   // enable channel 1 (0x60 = 0110 0000 where 110 is gain 24)
    print_reg = ads_rreg(ADS_REG_CH1SET);             // confirm channel 1 is enabled
    if(print_reg != wreg_val) {
      printf("ERROR: REG_CH1SET FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
      return 0;
    }

    ads_wreg(ADS_REG_CH2SET, wreg_val | 5);                   // enable channel 2 (0x60 = 0110 0000 where 110 is gain 24)
    ads_wreg(ADS_REG_CH3SET, wreg_val);                   // enable channel 3 (0x60 = 0110 0000 where 110 is gain 24)
    ads_wreg(ADS_REG_CH4SET, wreg_val);                   // enable channel 4 (0x60 = 0110 0000 where 110 is gain 24)

    // REG_CONFIG2
    wreg_val = 0x6 << 5;  // RESERVED
    wreg_val |= 0x1 << 4; // 0 - test driven exterally, 1 - test driven internally
    wreg_val |= 0x0 << 3; // RESERVED
    wreg_val |= 0x0 << 2; // 0 - CAL_AMP 1 × –(VREFP – VREFN) / 2400,  1 - 2× –(VREFP – VREFN) / 2400
    // Test signal frequency
    // These bits determine the calibration signal frequency.
    // 00 : Pulsed at fCLK / 221
    // 01 : Pulsed at fCLK / 220
    // 10 : Do not use
    // 11 : At dc
    wreg_val |= 0x0 << 0; // 0 - CAL_FREQ fCLK / 2^21, 1 - fCLK / 2^20
    ads_wreg(ADS_REG_CONFIG2, wreg_val);                   // change the config on ADS
    print_reg = ads_rreg(ADS_REG_CONFIG2);             // confirm the config on ADS
    if(print_reg != wreg_val) {
      printf("ERROR: REG_CONFIG2 FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
      return 0;
    }

    // REG_CONFIG3
    wreg_val = 0xE0;
    ads_wreg(ADS_REG_CONFIG3, wreg_val);                   // change the config on ADS
    print_reg = ads_rreg(ADS_REG_CONFIG3);             // confirm the config on ADS
    if(print_reg != wreg_val) {
      printf("ERROR: REG_CONFIG3 FAILED CONFIG (%x)\r\n", print_reg);  // print the config off the ADS
      return 0;
    }

    printf("--ADS1299 Comm. OK!--\r\n");
  
    for (rreg = 0x00; rreg < 0x18; rreg = rreg + 0x01) {
        print_reg = ads_rreg(rreg);
        printf("ADS_REG: %x = %x\r\n", rreg, print_reg);
    }


    // Read all the registers
    //ads_rregs(ADS_REG_ID, ADS_REG_CONFIG4);


    /***    Kickoff the PWM DAC interrupt routine     ****/
    //init_pwm_dac(100, 50);

    /***    Read ADS data     ****/
    ads_start();
    ads_rdatac();

    for (i = 0; i < 10; i++)
        ;

    while(1) {
      
      for (i = 0; i < Nsample; i++) {
          ads_poll_measurements(&app_vars.ads_measurement[i]);
      }
      
      for (i = 1; i < Nsample; i++) {
          adc_data[i] = app_vars.ads_measurement[i].channel[1];
          __asm("nop");
          __asm("nop");
          __asm("nop");

          //printf("%d\r\n", adc_data[i]);
          __asm("nop");
          __asm("nop");
          __asm("nop");
      }
      //delay_milliseconds_synchronous(10, 1);
      __asm("nop");
      __asm("nop");
      __asm("nop");
      send_packet(&adc_data[1], 34);
      __asm("nop");
      __asm("nop");
      __asm("nop");

      for(i = 0; i < 500; i++) __asm("nop");
      //delay_milliseconds_synchronous(10, 1);
    }
    printf("exit\n");
}
//=========================== public ==========================================

//=========================== private =========================================

