#include <stdio.h>
#include <stdlib.h>
#include "Memory_Map.h"
//#include "scm3_hardware_interface.h"
//#include "scm3c_hardware_interface.h"
#include "scm3c_hw_interface.h"
void sara_start(unsigned int toggles,unsigned int periodCounts)
{	
	unsigned int i=1;
	unsigned int t;
	unsigned int x=1;
	

	i=1;
	x=1;
	//GPIO_REG__OUTPUT = 0x0010;//starts the first toggle
	GPIO_REG__OUTPUT=GPIO_REG__OUTPUT | 0x0010;
	GPIO_REG__OUTPUT=GPIO_REG__OUTPUT & 0xFFDF;
	//I consider it a toggle when any of the two signals generate a one

	while(x<toggles)  { //LOOP
		if(i<16)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
				for(t=0;t<periodCounts;t++);
		}
		else
		{		
				if ((GPIO_REG__OUTPUT | 0xFFEF)==0xFFFF)//if GPIO4 is high toggle GPIO5
				{
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFF9F); // toggles GPIO6 and GPIO5
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFAF); //toggles GPIO6 & 4
						for(t=0;t<periodCounts;t++);
				}
				else
				{
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFAF); // toggles GPIO6 and GPIO5
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFF9F); //toggles GPIO6 & 4
						for(t=0;t<periodCounts;t++);
				}
				
				i=0;
				x++;
		}
		i=i+1;

	}	
	
	while(i<11)  { 
		if(i<11)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
				for(t=0;t<periodCounts;t++);
		}
		i=i+1;
	}	
	//I'm going to set GPIO 4, and 5 stay high.
	GPIO_REG__OUTPUT = GPIO_REG__OUTPUT | 0x0030; 
	i=0;
	while(i<12)  { 
	if(i<12)
	{
			GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
			for(t=0;t<periodCounts;t++);
	}
	i=i+1;
	}	
}

void sara_start2(unsigned int toggles,unsigned int periodCounts)
{	
	unsigned int i=1;
	unsigned int t;
	unsigned int x=1;
	

	i=1;
	x=1;
	GPIO_REG__OUTPUT=GPIO_REG__OUTPUT | 0x0100;
	GPIO_REG__OUTPUT=GPIO_REG__OUTPUT & 0xFDFF;
	//GPIO_REG__OUTPUT = 0x0100;//starts the first toggle
	//I consider it a toggle when any of the two signals generate a one

	while(x<toggles)  { //LOOP
		if(i<16)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
				for(t=0;t<periodCounts;t++);
		}
		else
		{		
				if ((GPIO_REG__OUTPUT | 0xFDFF)==0xFFFF)//if GPIO9 is high toggle GPIO8
				{
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFEBF); // toggles GPIO6 and GPIO8
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFDBF); //toggles GPIO6 & 9
						for(t=0;t<periodCounts;t++);
				}
				else
				{
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFDBF); // toggles GPIO6 and GPIO9
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
						i++;
						for(t=0;t<periodCounts;t++);
						GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFEBF); //toggles GPIO6 & 8
						for(t=0;t<periodCounts;t++);
				}
				
				i=0;
				x++;
		}
		i=i+1;

	}	
	
	while(i<11)  { 
		if(i<11)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
				for(t=0;t<periodCounts;t++);
		}
		i=i+1;
	}	
	//I'm going to set GPIO 8, and 9 stay high.
	GPIO_REG__OUTPUT = GPIO_REG__OUTPUT | 0x0300; 
	i=0;
	while(i<12)  { 
	if(i<12)
	{
			GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
			for(t=0;t<periodCounts;t++);
	}
	i=i+1;
	}	
}


void sara_release(unsigned int periodCounts)
{
	int i, t;
	GPIO_REG__OUTPUT = 0x0000; 
	i=0;
	while(i<12)  { 
	if(i<12)
	{
			GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
			for(t=0;t<periodCounts;t++);
	}
	i=i+1;
	}	
}
	
void testZappy2(unsigned int periodCounts)
{
	unsigned int i=0, t=0;
	GPIO_REG__OUTPUT = 0x0010;//starts the first toggle
	//I consider it a toggle when any of the two signals generate a one
	while(1)  { //LOOP
		if(i<41)
		{
				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFBF); //toggles only clock GPIO 6
				for(t=0;t<periodCounts;t++);
		}
		else
		{		
			GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFF8F); // toggles GPIO 4, 5, and 6.
			for(t=0;t<periodCounts;t++);
			i=0;
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
