#ifndef _PORT_API___
#define _port_API___

#include <bl606p_common.h>
#include <misc.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include <bl606p_spi.h>
#include <hosal_spi_flash.h>

#define SOFT_SPI 0

static uint8_t spi_cs_gpio = GLB_GPIO_PIN_28;
static uint8_t spi_clk_gpio = GLB_GPIO_PIN_27;
static uint8_t spi_mosi_gpio = GLB_GPIO_PIN_26;
static uint8_t spi_dc_gpio = GLB_GPIO_PIN_25;
static uint8_t spi_reset_gpio = GLB_GPIO_PIN_12;
static uint8_t spi_led_gpio = GLB_GPIO_PIN_24;

#define CS_HIGH GLB_GPIO_Write(spi_cs_gpio, 1)
#define CS_LOW GLB_GPIO_Write(spi_cs_gpio, 0)
#define MOSI_HIGE GLB_GPIO_Write(spi_mosi_gpio, 1)
#define MOSI_LOW GLB_GPIO_Write(spi_mosi_gpio, 0)
#define CLK_HIGH GLB_GPIO_Write(spi_clk_gpio, 1)
#define CLK_LOW GLB_GPIO_Write(spi_clk_gpio, 0)
#define DC_HIGH GLB_GPIO_Write(spi_dc_gpio,1)
#define DC_LOW GLB_GPIO_Write(spi_dc_gpio,0)

#define LED_HIGH GLB_GPIO_Write(GLB_GPIO_PIN_24,1)
#define LED_LOW GLB_GPIO_Write(GLB_GPIO_PIN_24,0)


#define SPI_NUM SPI0_ID
#define MAX_SEQ_CMD_LENGTH 32
#define SPI_SCREEN_TIMNEOUT 100    //ms

void spi_screen_set_address(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end);
void spi_screen_reset();
void spi_screen_init();
void spi_screen_write_reg(uint8_t byte);
void spi_screen_write_data(uint8_t byte);
void spi_screen_write_byte(uint8_t byte);

#endif