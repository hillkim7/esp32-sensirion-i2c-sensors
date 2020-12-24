/*
 * Wrapper module for source code compatibility with esp-open-rtos.
 */

#ifndef __ESP8266_WRAPPER_H__
#define __ESP8266_WRAPPER_H__

#ifdef ESP_PLATFORM  // ESP32 (ESP-IDF)

#ifdef __cplusplus
extern "C" {
#endif

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "driver/gpio.h"
//#include "driver/uart.h"
#include "driver/spi_common.h"

/*
 * esp-open-rtos SDK function wrapper 
 */

uint32_t sdk_system_get_time ();


/*
 * esp-open-rtos I2C interface wrapper
 */

#define I2C_FREQ_80K     80000
#define I2C_FREQ_100K   100000
#define I2C_FREQ_400K   400000
#define I2C_FREQ_500K   500000
#define I2C_FREQ_600K   600000
#define I2C_FREQ_800K   800000
#define I2C_FREQ_1000K 1000000
#define I2C_FREQ_1300K 1300000
  
void i2c_init (int bus, gpio_num_t scl, gpio_num_t sda, uint32_t freq);

int i2c_slave_write (uint8_t bus, uint8_t addr, const uint8_t *reg, 
                     uint8_t *data, uint32_t len);

int i2c_slave_read (uint8_t bus, uint8_t addr, const uint8_t *reg, 
                    uint8_t *data, uint32_t len);

/*
 * esp-open-rtos SPI interface wrapper
 */

bool spi_bus_init (spi_host_device_t host, 
                   uint8_t sclk , uint8_t miso, uint8_t mosi);

bool spi_device_init (uint8_t bus, uint8_t cs);

size_t spi_transfer_pf(uint8_t bus, uint8_t cs, 
                       const uint8_t *mosi, uint8_t *miso, uint16_t len);

/*
 * freertos api wrapper
 */

#define QueueHandle_t xQueueHandle

#ifdef __cplusplus
}
#endif

#endif  // ESP_PLATFORM

#endif  // __ESP8266_WRAPPER_H__
