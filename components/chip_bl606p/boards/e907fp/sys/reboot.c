/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     reboot.c
 * @brief    source file for the reboot
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#include <soc.h>
//#include <drv/wdt.h>
#include <bl606p_glb.h>
#include <bl_reason_code.h>
#include <drv/porting.h>

boot_reason_t soc_get_boot_reason(void)
{
    BL_RST_REASON_E ret;
    ret = bl_sys_rstinfo_get();
    
    if (ret == BL_RST_POWER_OFF) {
       return BOOTREASON_POWER; 
    } else if (ret == BL_RST_SOFTWARE_WATCHDOG) {
        return BOOTREASON_WDT;
    } else if (ret == BL_RST_SOFTWARE) {
        return BOOTREASON_SOFT;
    } else {
        return BOOTREASON_OTHER;
    }
}

void __attribute__((section(".tcm_code"))) drv_reboot(void)
{
    bl_sys_reset_por();
    //bl_sys_reset_system();
}
