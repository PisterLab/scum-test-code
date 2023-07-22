#include "spi.h"

#include "scm3c_hw_interface.h"
#include "Memory_Map.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define SPI_MAX_DEVICES 10


typedef struct node_t
{
    int handle;
    spi_pin_config_t config;
} node_t;

static node_t spi_nodes[SPI_MAX_DEVICES];
static uint32_t spi_node_bmap = 0;


static node_t* get_node(int handle)
{
    return &spi_nodes[handle];
}

void spi_digitalWrite(int pin, int high_low) {
    if (high_low) {
        GPIO_REG__OUTPUT |= (1 << pin);
	}
    else {
        GPIO_REG__OUTPUT &= ~(1 << pin);
	}
}

uint8_t spi_digitalRead(int pin) {
	uint8_t i = 0;
	i = (GPIO_REG__INPUT&(1 << pin)) >> pin;
	return i;
}

int spi_open(spi_pin_config_t *pin_config, spi_mode_t* mode)
{
    node_t* node;
    uint8_t new_handle;

    GPI_enable_set(pin_config->MISO);
    GPO_enable_clr(pin_config->MISO);
    GPO_enable_set(pin_config->CS);
    GPI_enable_clr(pin_config->CS);
    GPO_enable_set(pin_config->MOSI);
    GPI_enable_clr(pin_config->MOSI);
    GPO_enable_set(pin_config->SCLK);
    GPI_enable_clr(pin_config->SCLK);
    
    analog_scan_chain_write();
    analog_scan_chain_load();


    spi_digitalWrite(pin_config->MOSI, 0);    // reset low
    spi_digitalWrite(pin_config->SCLK, 0);    // reset low
    spi_digitalWrite(pin_config->CS, 0);    // reset low

    // Find first available handle in the bitmap
    
    for(new_handle = 0; new_handle < SPI_MAX_DEVICES; new_handle++){
        if((spi_node_bmap & (1 << new_handle)) == 0){
            node = &spi_nodes[new_handle];
            spi_node_bmap |= (1 << new_handle);
            node->handle = new_handle;
            break;
        }
    }

    if(new_handle == SPI_MAX_DEVICES){
        printf("ERROR: No more SPI devices available\r\n");
        return -1;
    }


    node->config = *pin_config;
    
    return node->handle;
}

int spi_ioctl(int handle, int request, int32_t arg)
{
    node_t* node;
    
    node = get_node(handle);
    if (node == 0)
        return INVALID_HANDLE;

    switch (request)
    {
        case SPI_CS:
            spi_digitalWrite(node->config.CS, arg);
            break;      
        default:
            return INVALID_IOCTL_REQ;
    }

    return 0;
}

int spi_write(int handle, const unsigned char write_byte)
{
    int bit;
    node_t* node;

    node = get_node(handle);
    if (node == 0)
        return INVALID_HANDLE;

	// clock low at the beginning
	spi_digitalWrite(node->config.SCLK, 0);

	// sample at falling edge
    
	for (bit = 7; bit >= 0; bit--)
    {
        spi_digitalWrite(node->config.SCLK, 1);
        spi_digitalWrite(node->config.MOSI, (write_byte & (1<<bit)) != 0);
        spi_digitalWrite(node->config.SCLK, 0);
	}

    // set data out to 0
	spi_digitalWrite(node->config.MOSI, 0);

    return 1; // always writes 1 byte.
}

int spi_read(int handle, uint8_t* byte)
{
	int8_t bit;
    node_t* node;

    *byte = 0;
    node = get_node(handle);
    if (node == 0)
        return INVALID_HANDLE;

	
    __disable_irq();
	for (bit = 7; bit >= 0; bit--)
    {
        spi_digitalWrite(node->config.SCLK, 1);
        //*byte = 0x1E;
        //spi_digitalRead(node->config.MISO);
        *byte |= spi_digitalRead(node->config.MISO) << bit;
        __asm("nop");
        __asm("nop");
        __asm("nop");
        __asm("nop");

        spi_digitalWrite(node->config.SCLK, 0);
    
	}
    __enable_irq();


    // delay few clock cycles
    for(bit = 0; bit < 10; bit++) 
    {
        __asm("nop");
    }
    return 1; // always reads 1 byte.
}

int spi_close(int handle)
{
    node_t* node;

    node = get_node(handle);
    if (node == 0)
        return INVALID_HANDLE;

    spi_node_bmap &= ~(1 << handle);

    return 0;
}