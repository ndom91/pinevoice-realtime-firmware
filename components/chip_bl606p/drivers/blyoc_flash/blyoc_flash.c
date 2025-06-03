/**
 * @file hal_flash.c
 * @brief
 *
 * Copyright (c) 2021 Bouffalolab team
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 */
#include "bl606p_glb.h"
#include "bl606p_clock.h"
#include "bl606p_xip_sflash.h"
#include "bl606p_sf_ctrl.h"
#include "bl606p_sf_cfg.h"
#include "bl_irq.h"
#include "aos/kernel.h"

static SPI_Flash_Cfg_Type g_flash_cfg;
#define FLASH_START_OFFSET      0
#define FLASH_START_ADDR        0
#define WAIT_FOREVER 0xFFFFFFFF

static aos_mutex_t g_flash_lock = NULL;

static void flash_lock_create(void)
{
	aos_mutex_new(&g_flash_lock);
}

static void flash_lock_delete(void)
{
	aos_mutex_free(&g_flash_lock);
}

static void flash_lock(void)
{
    if (g_flash_lock) {
        aos_mutex_lock(&g_flash_lock, WAIT_FOREVER);
    }
}

static void flash_unlock(void)
{
    if (g_flash_lock) {
        aos_mutex_unlock(&g_flash_lock);
    }
}

#if CONFIG_RUN_IN_FLASH
#define FLASH_LOCK_CREATE()
#define FLASH_LOCK()    GLOBAL_IRQ_SAVE()
#define FLASH_UNLOCK()  GLOBAL_IRQ_RESTORE()
#else
#define FLASH_LOCK_CREATE()    flash_lock_create()
#define FLASH_LOCK()           flash_lock()
#define FLASH_UNLOCK()         flash_unlock()
#endif

/**
 * @brief flash read data
 *
 * @param addr
 * @param data
 * @param len
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_read(uint32_t startaddr, void *data, uint32_t len)
{
    BL_Err_Type stat;
    uint8_t isAesEnable=0;

    if (startaddr < FLASH_START_OFFSET) {
    	return -1;
    }

    // printf("=================== bl_flash_read 0x%08lx, 0x%08lx, realaddr:0x%08lx\r\n", startaddr, startaddr+len, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR);
    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    stat = XIP_SFlash_Read_Need_Lock(&g_flash_cfg, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR, data, len, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();
//    memcpy(data, (const void *)(startaddr), len);

    return stat;
}

/**
 * @brief flash write data
 *
 * @param addr
 * @param data
 * @param len
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_write(uint32_t startaddr, void *data, uint32_t len)
{
    BL_Err_Type stat;
    uint8_t isAesEnable=0;

    if (startaddr < FLASH_START_OFFSET) {
    	return -1;
    }

    // printf("=================== bl_flash_write 0x%08lx, 0x%08lx, realaddr:0x%08lx\r\n", startaddr, startaddr+len, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR);
    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    stat = XIP_SFlash_Write_Need_Lock(&g_flash_cfg, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR, data, len, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();

    return stat;
}

/**
 * @brief flash erase
 *
 * @param startaddr
 * @param endaddr
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_erase(uint32_t startaddr, uint32_t len)
{
    BL_Err_Type stat;
    uint8_t isAesEnable=0;
    //int ms;

    printf("=================== bl_flash_erase 0x%08lx, 0x%08lx, realaddr:0x%08lx\r\n", startaddr, startaddr+len, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR);
    if (startaddr < FLASH_START_OFFSET) {
    	return -1;
    }

    //ms = csi_tick_get_ms();
    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    stat = XIP_SFlash_Erase_Need_Lock(&g_flash_cfg, startaddr - FLASH_START_OFFSET + FLASH_START_ADDR, len-1, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();
    //ms = csi_tick_get_ms() - ms;
    //printf("%s %d\r\n", __func__, ms);

    return stat;
}


/**
 * @brief multi flash adapter
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_init(void)
{
    uint8_t isAesEnable=0;
    FLASH_LOCK_CREATE();
    /* Get flash config identify */
    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    SF_Cfg_Flash_Identify_Ext(1, 0x80, 0, &g_flash_cfg, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();

    return 0;
}

/**
 * @brief flash deinit
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_deinit(void)
{
    SF_Ctrl_Disable();

    return 0;
}

/**
 * @brief flash write register with cmd
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_write_reg_withcmd(uint8_t writeRegCmd, uint8_t *regValue, uint8_t regLen)
{
    uint8_t isAesEnable=0;
    uint32_t offset;

    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    XIP_SFlash_State_Save(&g_flash_cfg, &offset, 0, 0);
    SFlash_Write_Reg_With_Cmd(&g_flash_cfg, writeRegCmd, regValue, regLen);
    XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();

    return 0;
}

/**
 * @brief flash release from power down
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_release_power_down(void)
{
    uint8_t isAesEnable=0;
    uint32_t offset;

    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    XIP_SFlash_State_Save(&g_flash_cfg, &offset, 0, 0);
    SFlash_Release_Powerdown(&g_flash_cfg);
    XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();

    return 0;
}

/**
 * @brief flash set power down
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_power_down(void)
{
    uint8_t isAesEnable=0;
    uint32_t offset;

    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    XIP_SFlash_State_Save(&g_flash_cfg, &offset, 0, 0);
    SFlash_Powerdown();
    XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();

    return 0;
}

/**
 * @brief flash set clock
 *
 * @return current flash clock
 */
int ATTR_TCM_SECTION bl_flash_set_clock(uint32_t clock)
{
    uint8_t isAesEnable=0;
    uint32_t offset;
    SF_Ctrl_Cfg_Type sfCtrlCfg={
        .owner=SF_CTRL_OWNER_SAHB,
        .en32bAddr=DISABLE,
        .clkDelay=1,
        .clkInvert=1,
        .rxClkInvert=0,
        .doDelay=0,
        .diDelay=0,
        .oeDelay=0,
    };

    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    XIP_SFlash_State_Save(&g_flash_cfg, &offset, 0, 0);
    if (clock == 20000000) {
        GLB_Set_SF_CLK(1, GLB_SFLASH_CLK_80M_MUXPLL, 3);
        sfCtrlCfg.clkDelay = 0;
        sfCtrlCfg.rxClkInvert = 1;
        SFlash_Init(&sfCtrlCfg, NULL);
    } else if (clock == 40000000) {
        GLB_Set_SF_CLK(1, GLB_SFLASH_CLK_80M_MUXPLL, 1);
        sfCtrlCfg.clkDelay = 1;
        sfCtrlCfg.rxClkInvert = 0;
        SFlash_Init(&sfCtrlCfg, NULL);
    } else if (clock == 80000000) {
        GLB_Set_SF_CLK(1, GLB_SFLASH_CLK_80M_MUXPLL, 0);
        sfCtrlCfg.clkDelay = 1;
        sfCtrlCfg.rxClkInvert = 1;
        SFlash_Init(&sfCtrlCfg, NULL);
    } else {
        XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
        XIP_SFlash_Opt_Exit(isAesEnable);
        clock = Clock_Peripheral_Clock_Get(BL_PERIPHERAL_CLOCK_FLASH);
        goto _ret;
    }
    XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);

_ret:
    FLASH_UNLOCK();
    return clock;
}

/**
 * @brief flash set io mode
 *
 * @return BL_Err_Type
 */
int ATTR_TCM_SECTION bl_flash_set_iomode(uint8_t io_mode)
{
    uint8_t isAesEnable=0;
    uint32_t offset;
    SF_Ctrl_Cmds_Cfg cfg;
    SF_Ctrl_IO_Type io = (SF_Ctrl_IO_Type)io_mode;

    cfg.ackLatency = 1;
    cfg.cmdsCoreEn = 1;
    cfg.cmdsEn = 1;
    cfg.cmdsWrapMode = 2;
    cfg.cmdsWrapLen = 2;

    FLASH_LOCK();
    XIP_SFlash_Opt_Enter(&isAesEnable);
    XIP_SFlash_State_Save(&g_flash_cfg, &offset, 0, 0);
    if (io == SF_CTRL_QIO_MODE) {
        g_flash_cfg.ioMode = io;
    } else {
        g_flash_cfg.ioMode = (io|0x10);
        cfg.cmdsEn = 1;
        cfg.cmdsWrapMode = 1;
        cfg.cmdsWrapLen = 9;
    }
    SF_Ctrl_Cmds_Set(&cfg, 0);
    XIP_SFlash_State_Restore(&g_flash_cfg, offset, 0, 0);
    XIP_SFlash_Opt_Exit(isAesEnable);
    FLASH_UNLOCK();
    return 0;
}
