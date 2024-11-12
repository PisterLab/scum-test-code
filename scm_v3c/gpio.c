#include "gpio.h"

#include <stdio.h>

#include "memory_map.h"
#include "optical.h"

void gpio_set_high(const gpio_e gpio) { GPIO_REG__OUTPUT |= (1 << gpio); }

void gpio_set_low(const gpio_e gpio) { GPIO_REG__OUTPUT &= ~(1 << gpio); }

void gpio_toggle(const gpio_e gpio) { GPIO_REG__OUTPUT ^= (1 << gpio); }

int gpio_read(const gpio_e gpio) { return (GPIO_REG__INPUT &= (1 << gpio)) ? 1 : 0; } 

void gpio_init(void) {
    // Initialize all pins to be low.
    GPIO_REG__OUTPUT &= ~0xFFFF;
}

void gpio_0_set(void) { gpio_set_high(GPIO_0); }
void gpio_0_clr(void) { gpio_set_low(GPIO_0); }
void gpio_0_toggle(void) { gpio_toggle(GPIO_0); }
int gpio_0_read(void)	{	return gpio_read(GPIO_0);	}

void gpio_1_set(void) { gpio_set_high(GPIO_1); }
void gpio_1_clr(void) { gpio_set_low(GPIO_1); }
void gpio_1_toggle(void) { gpio_toggle(GPIO_1); }
int gpio_1_read(void)	{	return gpio_read(GPIO_1);	}

void gpio_2_set(void) { gpio_set_high(GPIO_2); }
void gpio_2_clr(void) { gpio_set_low(GPIO_2); }
void gpio_2_toggle(void) { gpio_toggle(GPIO_2); }
int gpio_2_read(void)	{	return gpio_read(GPIO_2);	}

void gpio_3_set(void) { gpio_set_high(GPIO_3); }
void gpio_3_clr(void) { gpio_set_low(GPIO_3); }
void gpio_3_toggle(void) { gpio_toggle(GPIO_3); }
int gpio_3_read(void)	{	return gpio_read(GPIO_3);	}

void gpio_4_set(void) { gpio_set_high(GPIO_4); }
void gpio_4_clr(void) { gpio_set_low(GPIO_4); }
void gpio_4_toggle(void) { gpio_toggle(GPIO_4); }
int gpio_4_read(void)	{	return gpio_read(GPIO_4);	}

void gpio_5_set(void) { gpio_set_high(GPIO_5); }
void gpio_5_clr(void) { gpio_set_low(GPIO_5); }
void gpio_5_toggle(void) { gpio_toggle(GPIO_5); }
int gpio_5_read(void)	{	return gpio_read(GPIO_5);	}

void gpio_6_set(void) { gpio_set_high(GPIO_6); }
void gpio_6_clr(void) { gpio_set_low(GPIO_6); }
void gpio_6_toggle(void) { gpio_toggle(GPIO_6); }
int gpio_6_read(void)	{	return gpio_read(GPIO_6);	}

void gpio_7_set(void) { gpio_set_high(GPIO_7); }
void gpio_7_clr(void) { gpio_set_low(GPIO_7); }
void gpio_7_toggle(void) { gpio_toggle(GPIO_7); }
int gpio_7_read(void)	{	return gpio_read(GPIO_7);	}

void gpio_8_set(void) { gpio_set_high(GPIO_8); }
void gpio_8_clr(void) { gpio_set_low(GPIO_8); }
void gpio_8_toggle(void) { gpio_toggle(GPIO_8); }
int gpio_8_read(void)	{	return gpio_read(GPIO_8);	}

void gpio_9_set(void) { gpio_set_high(GPIO_9); }
void gpio_9_clr(void) { gpio_set_low(GPIO_9); }
void gpio_9_toggle(void) { gpio_toggle(GPIO_9); }
int gpio_9_read(void)	{	return gpio_read(GPIO_9);	}

void gpio_10_set(void) { gpio_set_high(GPIO_10); }
void gpio_10_clr(void) { gpio_set_low(GPIO_10); }
void gpio_10_toggle(void) { gpio_toggle(GPIO_10); }
int gpio_10_read(void)	{	return gpio_read(GPIO_10);	}

void gpio_11_set(void) { gpio_set_high(GPIO_11); }
void gpio_11_clr(void) { gpio_set_low(GPIO_11); }
void gpio_11_toggle(void) { gpio_toggle(GPIO_11); }
int gpio_11_read(void)	{	return gpio_read(GPIO_11);	}

void gpio_12_set(void) { gpio_set_high(GPIO_12); }
void gpio_12_clr(void) { gpio_set_low(GPIO_12); }
void gpio_12_toggle(void) { gpio_toggle(GPIO_12); }
int gpio_12_read(void)	{	return gpio_read(GPIO_12);	}

void gpio_13_set(void) { gpio_set_high(GPIO_13); }
void gpio_13_clr(void) { gpio_set_low(GPIO_13); }
void gpio_13_toggle(void) { gpio_toggle(GPIO_13); }
int gpio_13_read(void)	{	return gpio_read(GPIO_13);	}

void gpio_14_set(void) { gpio_set_high(GPIO_14); }
void gpio_14_clr(void) { gpio_set_low(GPIO_14); }
void gpio_14_toggle(void) { gpio_toggle(GPIO_14); }
int gpio_14_read(void)	{	return gpio_read(GPIO_14);	}

void gpio_15_set(void) { gpio_set_high(GPIO_15); }
void gpio_15_clr(void) { gpio_set_low(GPIO_15); }
void gpio_15_toggle(void) { gpio_toggle(GPIO_15); }
int gpio_15_read(void)	{	return gpio_read(GPIO_15);	}

// ISRs for external interrupts.
void ext_gpio3_activehigh_debounced_isr() {
    printf("External Interrupt GPIO3 triggered\r\n");
}
void ext_gpio8_activehigh_isr() {
    // Trigger the interrupt for calibration.
    optical_sfd_isr();
}
void ext_gpio9_activelow_isr() {
    printf("External Interrupt GPIO9 triggered\r\n");
}
void ext_gpio10_activelow_isr() {
    printf("External Interrupt GPIO10 triggered\r\n");
}
