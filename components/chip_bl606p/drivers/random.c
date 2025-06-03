/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     uart.c
 * @brief    CSI Source File for uart Driver
 * @version  V2.01
 * @date     2020-04-09
 ******************************************************************************/

#include "stdio.h"
#include "bl_sec.h"
#include "drv/rng.h"
/**
  \brief       Get data from the TNG engine
  \param[out]  Data  Pointer to buffer with data get from TNG
  \param[in]   Num   Number of data items,uinit in uint32
  \return      Error code \ref csi_error_t
*/
csi_error_t csi_rng_get_multi_word(uint32_t *data, uint32_t num)
{
    for (int i = 0; i < num; i++) {
        *data++ = bl_rand();
    }
    return CSI_OK;
}

/**
  \brief       Get data from the TNG engine
  \return      Error code \ref csi_error_t
*/
csi_error_t csi_rng_get_single_word(uint32_t* data)
{
    *data = bl_rand();
    return CSI_OK;
}


