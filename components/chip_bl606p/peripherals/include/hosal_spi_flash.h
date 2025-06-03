#ifndef __HOSAL_SPI_FLASH__H__
#define __HOSAL_SPI_FLASH__H__

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup hosal_spi_flash
 *  spi hal API.
 *
 *  @{
 */

#include <stdint.h>
#include "peripherals_config.h"

void hosal_spi_flash_set_gpio(uint8_t clk, uint8_t cs, uint8_t mosi, uint8_t miso);
BL_Err_Type hosal_spi_flash_init(void);
BL_Err_Type hosal_spi_flash_read_jedec_id(uint8_t *data);
BL_Err_Type hosal_spi_flash_read(uint32_t addr, uint8_t *data, uint32_t len);
BL_Err_Type hosal_spi_flash_write(uint32_t addr, uint8_t *data, uint32_t len);
BL_Err_Type hosal_spi_flash_erase(uint32_t startaddr, uint32_t len);

#ifdef __cplusplus
}
#endif

#endif /* HOSAL_SPI_FLASH_H */

