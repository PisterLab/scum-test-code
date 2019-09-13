#include <stdio.h>
#include <stdlib.h>
#include "Memory_Map.h"
#include "scm3_hardware_interface.h"
#include "scm3C_hardware_interface.h"
void sara_start(unsigned int toggles)
{	
	unsigned int i=1;
	unsigned int t;
	unsigned int x=1;
	

	i=1;
	x=1;
	GPIO_REG__OUTPUT = 0x0010;

	while(x<2*toggles+1)  { //LOOP
		if(i<11)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
		}
		else
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFF8F); // toggles all first three bits
				i=1;
				x++;
		}
		i=i+1;

	}	
	
	while(i<11)  { //LOOP
		if(i<11)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
		}
		i=i+1;
	}	
	
}
void GPIO3_interrupt_enable(void)
{
	ISER=0x0002;
}
void GPIO9_interrupt_enable(void)
{
	ISER=0x2000;
}
void GPIO9_interrupt_disable(void)
{
	ICPR=0x2000;
	ICER=0x2000;
}
void int_to_bin_digit(unsigned int in, int count, int* out)
{
    /* assert: count <= sizeof(int)*CHAR_BIT */
    unsigned int mask = 1U << (count-1);
    int i;
    for (i = 0; i < count; i++) {
        out[i] = (in & mask) ? 1 : 0;
        in <<= 1;
    }
}
