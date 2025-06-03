#ifndef _PORT_API___
#define _port_API___

#include <bl606p_common.h>
#include <misc.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include <bl606p_spi.h>
#include <hosal_spi_flash.h>

#include "hosal_pwm.h"

#define SOFT_SPI 0
#define CHECK_TE_ENABLE         (0)     // 使能检测TE脚
#define SPI_DMA_ENABLE          (1)     // 使能SPI DMA
#define DEFAULT_PWM_FRRQ 100000
#define DEFAULT_BRIGHTNESS 4000

extern uint8_t spi_cs_gpio;
extern  uint8_t spi_clk_gpio;
extern  uint8_t spi_mosi_gpio;
extern  uint8_t spi_dc_gpio;
extern  uint8_t spi_reset_gpio;
extern  uint8_t spi_led_gpio;

#define CS_HIGH GLB_GPIO_Write(spi_cs_gpio, 1)
#define CS_LOW GLB_GPIO_Write(spi_cs_gpio, 0)
#define MOSI_HIGE GLB_GPIO_Write(spi_mosi_gpio, 1)
#define MOSI_LOW GLB_GPIO_Write(spi_mosi_gpio, 0)
#define CLK_HIGH GLB_GPIO_Write(spi_clk_gpio, 1)
#define CLK_LOW GLB_GPIO_Write(spi_clk_gpio, 0)
#define DC_HIGH GLB_GPIO_Write(spi_dc_gpio, 1)
#define DC_LOW GLB_GPIO_Write(spi_dc_gpio, 0)

// #define LED_HIGH GLB_GPIO_Write(spi_led_gpio, 1)
// #define LED_LOW GLB_GPIO_Write(spi_led_gpio, 0)
 #define LED_HIGH spi_set_brightness(led_brightness);
 #define LED_LOW  spi_set_brightness(0);

#define SPI_NUM SPI0_ID
#define MAX_SEQ_CMD_LENGTH 32
#define SPI_SCREEN_TIMNEOUT 100 // ms

void spi_screen_set_address(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end);
void spi_screen_write_len(uint8_t *byte, uint32_t len);
void spi_screen_reset();
void spi_screen_init();
void spi_screen_write_reg(uint8_t byte);
void spi_screen_write_data(uint8_t byte);
void spi_screen_write_byte(uint8_t byte);
int spi_screen_is_ready(void);
void spi_screen_register_tc_cb(void (*cb)(void *arg), void *arg);
void spi_set_brightness(uint32_t brightness);

#endif