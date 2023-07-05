#include <stdint.h>
#include "scm3c_hw_interface.h"

#ifndef __SPI_LIB
#define __SPI_LIB

// error return values
#define INVALID_HANDLE -1 // handle is not valid
#define INVALID_IOCTL_REQ -2 // spi_ioctl request is not valid

// spi_ioctl requests
#define SPI_CS 0 // chip select

typedef struct spi_pin_config_t{
    uint8_t MISO; 
    uint8_t MOSI;
    uint8_t SCLK;
    uint8_t CS;
} spi_pin_config_t;

typedef struct spi_mode_t{
    uint32_t data_rate; // TODO: not used yet.
    uint32_t buffer_size; // TODO: not used yet.
} spi_mode_t;

/**
 * Reads a byte to the SPI peripheral specified by handle.
 * @param handle The SPI peripheral handle returned from spi_open().
 * @param byte The pointer to a buffer of a byte.
 * @return The number of bytes read, which is always 1 when successful.
 *      If operation fails, a negative value is returned indicating the error type.
*/
int spi_read(int handle, unsigned char* byte);

/**
 * Writes a byte to the SPI peripheral specified by handle.
 * @param handle The SPI peripheral handle returned from spi_open().
 * @param write_byte The value of the byte to be written into the SPI peripheral.
 * @return The number of bytes written, which is always 1 when successful.
 *      If operation fails, a negative value is returned indicating the error type.
*/
int spi_write(int handle, const unsigned char write_byte);

/**
 * Closes a SPI peripheral.
 * @param handle The SPI peripheral handle returned from spi_open().
 * @return Non-negative value when successful.
 *      If operation fails, a negative value is returned indicating the error type.
*/
int spi_close(int handle);

/**
 * Opens a SPI peripheral.
 * @param pin_config A pointer to spi_pin_config_t specifying the four pins of SPI.
 * @param mode A pointer to spi_mote_t specifying how the peripheral is used.
 * @return Non-negative handle representing the SPI peripheral.
 *      If operation fails, a negative value is returned indicating the error type.
*/
int spi_open(spi_pin_config_t *pin_config, spi_mode_t* mode);

/**
 * IO control for SPI.
 * @param handle The SPI peripheral handle returned from spi_open().
 * @param request A device-dependent request code.
 * @param arg Optional argument for requests. 
 * @return Non-negative value when successful.
 *      If operation fails, a negative value is returned indicating the error type.
*/
int spi_ioctl(int handle, int request, int32_t arg);

#endif
