/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     uart.c
 * @brief    CSI Source File for uart Driver
 * @version  V2.01
 * @date     2020-04-09
 ******************************************************************************/

#include <drv/spiflash.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>
#include <soc.h>
#include <drv/tick.h>
#include <bl606p.h>
#include <blyoc_flash/blyoc_flash.h>
#include "bl606p_common.h"

#define FLASH_START_OFFSET      0

/**
  \brief       Initialize EFLASH Interface. 1. Initializes the resources needed for the EFLASH interface 2.registers event callback function
  \param[in]   eflash  eflash handle to operate.
  \param[in]   idx  device id
  \param[in]   arg  User can define it by himself as callback's param
  \return      error code
*/
ATTR_TCM_SECTION csi_error_t csi_spiflash_spi_init(csi_spiflash_t *spiflash, uint32_t idx, void *spi_cs_callback)
{
    //LOGD("flash", "%s\r\n", __func__);
    if(idx != 0)
    {
        return CSI_ERROR;
    }

    bl_flash_init();

    return CSI_OK;
}

/**
  \brief       De-initialize EFLASH Interface. stops operation and releases the software resources used by the interface
  \param[in]   eflash  eflash handle to operate.
  \return      error code
*/
ATTR_TCM_SECTION void csi_spiflash_spi_uninit(csi_spiflash_t *spiflash)
{
    bl_flash_deinit();
}

/**
  \brief       Read data from Flash.
  \param[in]   eflash  eflash handle to operate.
  \param[in]   offset  Data address.
  \param[out]  data  Pointer to a buffer storing the data read from Flash.
  \param[in]   size   Number of data items to read.
  \return      error code
*/
ATTR_TCM_SECTION int32_t csi_spiflash_read(csi_spiflash_t *spiflash, uint32_t offset, void *data, uint32_t size)
{
    offset += FLASH_START_OFFSET;
    //printf("%s addr 0x%08x, len %ld\r\n", __func__, offset, size);
    bl_flash_read(offset, data, size);
    return size;
}

/**
  \brief       Program data to Flash.
  \param[in]   eflash  eflash handle to operate.
  \param[in]   offset  Data address.
  \param[in]   data  Pointer to a buffer containing the data to be programmed to Flash.
  \param[in]   size   Number of data items to program.
  \return      error code
*/
ATTR_TCM_SECTION int32_t csi_spiflash_program(csi_spiflash_t *spiflash, uint32_t offset, const void *data, uint32_t size)
{
    offset += FLASH_START_OFFSET;
    //printf("%s addr 0x%08x, len %ld\r\n", __func__, offset, size);
    bl_flash_write(offset, (void *)data, size);
    return size;
}

/**
  \brief       Erase Flash Sector.
  \param[in]   eflash  eflash handle to operate.
  \param[in]   offset  flash address, flash address need sector size aligned
  \param[in]   size  erase size
  \return      error code
*/
ATTR_TCM_SECTION csi_error_t csi_spiflash_erase(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    offset += FLASH_START_OFFSET;
    //printf("%s addr 0x%08x, len %ld\r\n", __func__, offset, size);
    bl_flash_erase(offset, size);
    return CSI_OK;
}

ATTR_TCM_SECTION csi_error_t csi_spiflash_get_flash_info(csi_spiflash_t *spiflash, csi_spiflash_info_t *flash_info)
{
    // FIXME:
    flash_info->flash_id = 1;
    flash_info->flash_name = (char *)"FLASH0";
    flash_info->xip_addr = FLASH_START_OFFSET;
    flash_info->flash_size = 0x1000000;
    flash_info->sector_size = 4096;
    return CSI_OK;
}

/**
  \brief       Write status register
  \param[in]   spiflash  SPIFLASH handle to operate
  \param[in]   cmd       Cmd code
  \param[out]  data      Data buf to save flash status register
  \param[in]   size      Register length in byte
  \return      Error code
*/
ATTR_TCM_SECTION csi_error_t csi_spiflash_write_reg(csi_spiflash_t *spiflash, uint8_t cmd_code, uint8_t *data, uint32_t size)
{
    bl_flash_write_reg_withcmd(cmd_code, data, size);
    return CSI_OK;
}

/**
  \brief       Enable spiflash write protection
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   offset      Protect flash offset,offset need protect block size aligned
  \param[in]   size        Lock size(byte)
  \return      Error code
*/
csi_error_t csi_spiflash_lock(csi_spiflash_t *spiflash, uint32_t offset, uint32_t size)
{
    return CSI_ERROR;
}

/**
  \brief       Set QSPI data line
  \param[in]   spiflash    SPIFLASH handle to operate
  \param[in]   line        SPIFLASH data line mode
  \return      Error code
*/
ATTR_TCM_SECTION csi_error_t csi_spiflash_config_data_line(csi_spiflash_t *spiflash, csi_spiflash_data_line_t line)
{
    uint8_t io_mode = 4;

    if (line == SPIFLASH_DATA_4_LINES) {
        io_mode = 4;
    } else if (line == SPIFLASH_DATA_2_LINES) {
        io_mode = 1;
    } else if (line == SPIFLASH_DATA_1_LINE) {
        io_mode = 0;
    } else {
        return CSI_ERROR;
    }

    bl_flash_set_iomode(io_mode);
    return CSI_OK;
}

/**
  \brief       Set QSPI frequence
  \param[in]   spiflash SPIFLASH handle to operate
  \param[in]   hz       SPIFLASH frequence
  \return      The actual config frequency
*/
ATTR_TCM_SECTION uint32_t csi_spiflash_frequence(csi_spiflash_t *spiflash, uint32_t hz)
{
    return bl_flash_set_clock(hz);
}

/**
  \brief       Flash power down.
  \param[in]   spiflash SPIFLASH handle to operate.
  \return      error code
*/
ATTR_TCM_SECTION csi_error_t csi_spiflash_release_power_down(csi_spiflash_t *spiflash)
{
    bl_flash_release_power_down();
    return CSI_OK;
}

/**
  \brief       Flash power release.
  \param[in]   spiflash SPIFLASH handle to operate.
  \return      none
*/
ATTR_TCM_SECTION void csi_spiflash_power_down(csi_spiflash_t *spiflash)
{
    bl_flash_power_down();
}

