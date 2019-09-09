#include <stdio.h>
#include <stdlib.h>
#include "Memory_Map.h"
#include "scm3_hardware_interface.h"
#include "scm3C_hardware_interface.h"
void sara_start(void)
{	
	unsigned int i=1;
	unsigned int t;
	unsigned int x=1;

	i=1;
	while(1)
	{
		for(t=0;t<1000;t++);
		if (i==1)
		{
				printf("made it :)");
				i=i+1;
		}
		GPIO_REG__OUTPUT = 0x0000;
		
		for(t=0;t<1000;t++);
		GPIO_REG__OUTPUT = 0xFFFF;
		
	}

//	GPIO_REG__OUTPUT = 0x0000;
//	while(1)  { //LOOP
//		//for (y=0;y<1000;y++)
//		GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFFE); //toggles only bit one
//		if(i<11)
//		{
//				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFFE); //toggles only bit one
//		}
//		else
//		{
//				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFF8); // toggles all first three bits
//				i=1;
//		}
//		i=i+1;
//		for(t=0;t<0;t++);
//		GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFFE);
//		for(t=0;t<0;t++);
//		if(x==50000)
//		{
//			GPIO_REG__OUTPUT=0x0000;
//		
//			for(t=0;t<100000;t++)
//			{
//				GPIO_REG__OUTPUT = ~(GPIO_REG__OUTPUT ^ 0xFFFE); //toggles only bit one
//			}
//			x=0;
//			GPIO_REG__OUTPUT = 0x0003;
//		}
//		x=x+1;
	//}	
}