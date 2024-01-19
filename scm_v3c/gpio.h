#ifndef __GPIO_H
#define __GPIO_H

// GPIO enumeration.
typedef enum {
    GPIO_INVALID = -1,
    GPIO_0 = 0,
    GPIO_1 = 1,
    GPIO_2 = 2,
    GPIO_3 = 3,
    GPIO_4 = 4,
    GPIO_5 = 5,
    GPIO_6 = 6,
    GPIO_7 = 7,
    GPIO_8 = 8,
    GPIO_9 = 9,
    GPIO_10 = 10,
    GPIO_11 = 11,
    GPIO_12 = 12,
    GPIO_13 = 13,
    GPIO_14 = 14,
    GPIO_15 = 15,
} gpio_e;

// Set the GPIO to high.
void gpio_set_high(gpio_e gpio);

// Set the GPIO to low.
void gpio_set_low(gpio_e gpio);

// Toggle the GPIO.
void gpio_toggle(gpio_e gpio);

void gpio_init(void);

void gpio_0_set(void);
void gpio_0_clr(void);
void gpio_0_toggle(void);

// Frame.
void gpio_1_set(void);
void gpio_1_clr(void);
void gpio_1_toggle(void);

// ISR.
void gpio_2_set(void);
void gpio_2_clr(void);
void gpio_2_toggle(void);

// Slot.
void gpio_3_set(void);
void gpio_3_clr(void);
void gpio_3_toggle(void);

// FSM.
void gpio_4_set(void);
void gpio_4_clr(void);
void gpio_4_toggle(void);

// Task.
void gpio_5_set(void);
void gpio_5_clr(void);
void gpio_5_toggle(void);

// Radio.
void gpio_6_set(void);
void gpio_6_clr(void);
void gpio_6_toggle(void);

void gpio_7_set(void);
void gpio_7_clr(void);
void gpio_7_toggle(void);

void gpio_8_set(void);
void gpio_8_clr(void);
void gpio_8_toggle(void);

void gpio_9_set(void);
void gpio_9_clr(void);
void gpio_9_toggle(void);

void gpio_10_set(void);
void gpio_10_clr(void);
void gpio_10_toggle(void);

void gpio_11_set(void);
void gpio_11_clr(void);
void gpio_11_toggle(void);

void gpio_12_set(void);
void gpio_12_clr(void);
void gpio_12_toggle(void);

void gpio_13_set(void);
void gpio_13_clr(void);
void gpio_13_toggle(void);

void gpio_14_set(void);
void gpio_14_clr(void);
void gpio_14_toggle(void);

void gpio_15_set(void);
void gpio_15_clr(void);
void gpio_15_toggle(void);

#endif  // __GPIO_H
