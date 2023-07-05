#include "spi.h"

#include "scm3c_hw_interface.h"
#include "Memory_Map.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


typedef struct node_t
{
    int handle;
    spi_pin_config_t config;
    struct node_t* child;
} node_t;

static int handle_count = 0;
static node_t* root = 0;
static node_t* last = 0;

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

node_t* get_node(int handle)
{
    node_t *node = root;

    // Find the device specified by the handle
    while(node != 0)
    {
        if (node->handle == handle)
            break;
        else
            node = node->child;
    }
    return node;
}

int spi_open(spi_pin_config_t *pin_config, spi_mode_t* mode)
{
    node_t* node;


    GPI_enable_set(pin_config->MISO);
    GPO_enable_set(pin_config->CS);
    GPO_enable_set(pin_config->MOSI);
    GPO_enable_set(pin_config->SCLK);
    
    node = malloc(sizeof(node_t));

    spi_digitalWrite(pin_config->MOSI, 0);    // reset low
    spi_digitalWrite(pin_config->SCLK, 0);    // reset low
    spi_digitalWrite(pin_config->CS, 0);    // reset low

    node->handle = handle_count++;
    memcpy(&node->config, pin_config, sizeof(spi_pin_config_t));
    node->child = 0;

    if (root == 0)
    {
        root = node; 
        last = node;
    }
    else 
    {
        last->child = node;
        last = node;
    }
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

int spi_read(int handle, unsigned char* byte)
{
	int bit;
    node_t* node;

    *byte = 0;
    node = get_node(handle);
    if (node == 0)
        return INVALID_HANDLE;

	// sample at falling edge
	for (bit = 7; bit >= 0; bit--)
    {
        spi_digitalWrite(node->config.SCLK, 1);
        spi_digitalWrite(node->config.SCLK, 0);
        *byte |= spi_digitalRead(node->config.MISO) << bit;		
	}

    return 1; // always reads 1 byte.
}

int spi_close(int handle)
{
    node_t *node = root;
    node_t *prev_node = 0;

    // Find the device specified by the handle
    while(node != 0)
    {
        if (node->handle == handle)
            break;
        else
        {
            prev_node = node;
            node = node->child;
        }
    }

    if (node == 0)
    {
        return INVALID_HANDLE;
    }

    if (node == root && node == last)
    { // only one node, which is both root and last node.
        root = 0;
        last = 0;
    }
    else if (node == root)
    { // node is root
        node = node->child;
    }
    else if (node == last)
    { // node is the last node
        last = prev_node;
    }
    else
    { // regular case
        prev_node->child = node->child;
    }

    free(node);
    return 0;
}