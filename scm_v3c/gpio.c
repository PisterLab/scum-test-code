#include "memory_map.h"
#include "optical.h" 

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

void gpio_init(void){
    GPIO_REG__OUTPUT    &= ~0xFFFF; // all PINS low at initial
}

// frame
void gpio_0_set(void){
    GPIO_REG__OUTPUT    |=  0x0001;
}
void gpio_0_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0001;
}
void gpio_0_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0001;
}

// frame
void gpio_1_set(void){
    GPIO_REG__OUTPUT    |=  0x0002;
}
void gpio_1_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0002;
}
void gpio_1_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0002;
}

// isr
void gpio_2_set(void){
    GPIO_REG__OUTPUT    |=  0x0004;
}
void gpio_2_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0004;
}
void gpio_2_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0004;
}

// slot
void gpio_3_set(void){
    GPIO_REG__OUTPUT    |=  0x0008;
}
void gpio_3_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0008;
}
void gpio_3_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0008;
}

// fsm
void gpio_4_set(void){
    GPIO_REG__OUTPUT    |=  0x0010;
}
void gpio_4_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0010;
}
void gpio_4_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0010;
}

// task
void gpio_5_set(void){
    GPIO_REG__OUTPUT    |=  0x0020;
}
void gpio_5_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0020;
}
void gpio_5_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0020;
}

// radio
void gpio_6_set(void){
    GPIO_REG__OUTPUT    |=  0x0040;
}
void gpio_6_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0040;
}
void gpio_6_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0040;
}

void gpio_7_set(void){
    GPIO_REG__OUTPUT    |=  0x0080;
}
void gpio_7_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0080;
}
void gpio_7_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0080;
}

void gpio_8_set(void){
    GPIO_REG__OUTPUT    |=  0x0100;
}
void gpio_8_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0100;
}
void gpio_8_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0100;
}

void gpio_9_set(void){
    GPIO_REG__OUTPUT    |=  0x0200;
}
void gpio_9_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0200;
}
void gpio_9_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0200;
}

void gpio_10_set(void){
    GPIO_REG__OUTPUT    |=  0x0400;
}
void gpio_10_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0400;
}
void gpio_10_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0400;
}

void gpio_11_set(void){
    GPIO_REG__OUTPUT    |=  0x0800;
}
void gpio_11_clr(void){
    GPIO_REG__OUTPUT    &= ~0x0800;
}
void gpio_11_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x0800;
}

void gpio_12_set(void){
    GPIO_REG__OUTPUT    |=  0x1000;
}
void gpio_12_clr(void){
    GPIO_REG__OUTPUT    &= ~0x1000;
}
void gpio_12_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x1000;
}

void gpio_13_set(void){
    GPIO_REG__OUTPUT    |=  0x2000;
}
void gpio_13_clr(void){
    GPIO_REG__OUTPUT    &= ~0x2000;
}
void gpio_13_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x2000;
}

void gpio_14_set(void){
    GPIO_REG__OUTPUT    |=  0x4000;
}
void gpio_14_clr(void){
    GPIO_REG__OUTPUT    &= ~0x4000;
}
void gpio_14_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x4000;
}

void gpio_15_set(void){
    GPIO_REG__OUTPUT    |=  0x8000;
}
void gpio_15_clr(void){
    GPIO_REG__OUTPUT    &= ~0x8000;
}
void gpio_15_toggle(void){
    GPIO_REG__OUTPUT    ^=  0x8000;
}

// ISRs for external interrupts
void ext_gpio3_activehigh_debounced_isr(){
    printf("External Interrupt GPIO3 triggered\r\n");
}
void ext_gpio8_activehigh_isr(){
    // Trigger the interrupt for calibration
    optical_sfd_isr();
}
void ext_gpio9_activelow_isr(){
    printf("External Interrupt GPIO9 triggered\r\n");
}
void ext_gpio10_activelow_isr(){
    printf("External Interrupt GPIO10 triggered\r\n");
}
