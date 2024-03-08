/*
------------------------------------------------------------------------------

v7: june 27
-- changed GPO ports to allow jumpering big board
-- perform 1000 adc reads and calculate result internally


*/
#include <rt_misc.h>
#include <stdio.h>

#include "Int_Handlers.h"
#include "Memory_map.h"
#include "global_vars.h"

#define GPIO_REG__PGA_AMPLIFY 0x0040
#define GPIO_REG__ADC_CONVERT 0x0020
#define GPIO_REG__ADC_RESET 0x0010
/*
#define GPIO_REG__PGA_AMPLIFY	0x0800
#define GPIO_REG__ADC_CONVERT	0x0400
#define GPIO_REG__ADC_RESET	  0x0200
*/
#define NUM_ADC_READS 10000

extern unsigned int adc_data;
extern unsigned int ADC_DATA_VALID;

void reset16(void);
void reset1(void);

//////////////////////////////////////////////////////////////////
// Main Function
//////////////////////////////////////////////////////////////////

int main(void) {
    int t = 0;
    int val1 = 0;
    int ii = 0;
    // int adc_data[NUM_ADC_READS];
    unsigned long sumtotal = 0;
    int net_adc_val = 0;

    ADC_DATA_VALID = 0;
    adc_triggered = 1;
    reset1();

    while (1) {
        if (adc_triggered == 1) {
            sumtotal = 0;

            for (ii = 0; ii < NUM_ADC_READS; ii++) {
                for (t = 0; t < 100; t++) {
                };  // slowdown for potentiometric sensors

                // activate state machine so ADC result will be copied into
                // buffer
                ADC_REG__START = 0x1;

                // set PGA to amplify mode and wait for amp to settle
                GPIO_REG__OUTPUT &= (~GPIO_REG__PGA_AMPLIFY);
                val1 = GPIO_REG__OUTPUT;

                for (t = 0; t < 100; t++) {
                };  // count to 100 ends up being about 50us at 5MHz clock

                // set convert high; look for SAR output
                GPIO_REG__OUTPUT |= GPIO_REG__ADC_CONVERT;
                val1 = GPIO_REG__OUTPUT;

                while (ADC_DATA_VALID != 1) {
                    val1 = GPIO_REG__OUTPUT;
                }
                // adc_data[ii] = ADC_REG__DATA;
                sumtotal = sumtotal + ADC_REG__DATA;

                GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_CONVERT);
                GPIO_REG__OUTPUT |= GPIO_REG__PGA_AMPLIFY;
                val1 = GPIO_REG__OUTPUT;

                reset1();

                ADC_DATA_VALID = 0;
            }  // end for loop

            net_adc_val = sumtotal / NUM_ADC_READS;

            printf("%d\n", net_adc_val);

            for (t = 0; t < 25000; t++) {};

        }  // end if(adc_triggered==1)

    }  // end while(1)

    return 0;  // never get here
}

void reset1(void) {
    int val1;
    int t;
    // strobe reset low then high
    GPIO_REG__OUTPUT &= (~GPIO_REG__ADC_RESET);
    val1 = GPIO_REG__OUTPUT;

    for (t = 0; t < (50); t++);

    GPIO_REG__OUTPUT |= GPIO_REG__ADC_RESET;
    val1 = GPIO_REG__OUTPUT;
}