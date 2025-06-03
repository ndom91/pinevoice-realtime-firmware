/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     cache.c
 * @brief    source file for setting cache.
 * @version  V1.0
 * @date     18. July 2018
 ******************************************************************************/

#include <stdint.h>
#include <drv/porting.h>
#include <csi_core.h>

void soc_dcache_clean_invalid_range(unsigned long addr, uint32_t size)
{
    csi_dcache_clean_invalid_range((uint32_t *)addr, size);
}

void soc_dcache_clean_invalid_all(void)
{
    csi_dcache_clean_invalid();
}

void soc_dcache_invalid_range(unsigned long addr, uint32_t size)
{
    csi_dcache_invalid_range((uint32_t *)addr, size);
}
