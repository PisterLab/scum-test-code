#ifndef __GPIO_H
#define __GPIO_H

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void gpio_init(void);

// frame
void gpio_1_set(void);
void gpio_1_clr(void);
void gpio_1_toggle(void);

// isr
void gpio_2_set(void);
void gpio_2_clr(void);
void gpio_2_toggle(void);

// slot
void gpio_3_set(void);
void gpio_3_clr(void);
void gpio_3_toggle(void);

// fsm
void gpio_4_set(void);
void gpio_4_clr(void);
void gpio_4_toggle(void);

// task
void gpio_5_set(void);
void gpio_5_clr(void);
void gpio_5_toggle(void);

// radio
void gpio_6_set(void);
void gpio_6_clr(void);
void gpio_6_toggle(void);

void gpio_7_set(void);
void gpio_7_clr(void);
void gpio_7_toggle(void);

void gpio_8_set(void);
void gpio_8_clr(void);
void gpio_8_toggle(void);

#endif
