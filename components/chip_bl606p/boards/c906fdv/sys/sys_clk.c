/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     sys_clk.c
 * @brief    source file for setting system frequency.
 * @version  V1.0
 * @date     14. Oct 2020
 ******************************************************************************/

#include <stdint.h>
#include <soc.h>
#include <sys_clk.h>
#include <drv/io.h>
#include <bl606p_clock.h>

#define  SPARROW_HAPS 0

#ifndef VAD_FREQ_VALUE
#ifdef  SPARROW_HAPS
#define TIMER_FREQ_VALUE                4096000U
#else
#define TIMER_FREQ_VALUE                4096000U
#endif
#endif

#ifndef AUDIO_FREQ_VALUE
#define AUDIO_FREQ_PLL_VALUE            147456000U
#endif

#ifdef SPARROW_HAPS 
#define UART_FREQ_VALUE                 30000000U
#else
#define UART_FREQ_VALUE                 30000000U
#endif

uint32_t g_system_clock = IHS_VALUE;


uint32_t soc_get_pll_clk(void)
{
    return 0;
}

uint32_t soc_get_sys_clk(uint32_t idx)
{
    return g_system_clock;
}
uint32_t soc_get_ahb_clk(uint32_t idx)
{
    return 0;
}
uint32_t soc_get_apb_clk(uint32_t idx)
{
    return 0;
}
uint32_t soc_get_uart_clk(uint32_t idx)
{
    return 0;
}
uint32_t soc_get_audio_clk(uint32_t idx)
{
    return 0;
}
/************************************************/
uint32_t soc_get_uart_freq(uint32_t idx)
{
    return soc_get_uart_clk(idx);
}

uint32_t soc_get_iic_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_spi_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_qspi_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_adc_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_pwm_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_wdt_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_i2s_freq(uint32_t idx)
{
    return soc_get_audio_clk(idx);
}

uint32_t soc_get_timer_freq(uint32_t idx)
{
    return TIMER_FREQ_VALUE;
}

uint32_t soc_get_rtc_freq(uint32_t idx)
{
    return ILS_VALUE;
}

uint32_t soc_get_pll_freq(void)
{
    return soc_get_pll_clk();
}

uint32_t soc_get_cpu_freq(uint32_t idx)
{
    return soc_get_cur_cpu_freq();
}

uint32_t soc_get_sys_freq(uint32_t idx)
{
    return g_system_clock;
    // return soc_get_sys_clk(idx);
}

uint32_t soc_get_ahb_freq(uint32_t idx)
{
    return soc_get_ahb_clk(idx);
}

uint32_t soc_get_apb_freq(uint32_t idx)
{
    return soc_get_apb_clk(idx);
}

uint32_t soc_get_cur_cpu_freq(void)
{
    // return 320*1000*1000;
    return SystemCoreClockGet();
}

uint32_t soc_get_coretim_freq(void)
{
    return 1000*1000;
    // return soc_get_cpu_clk(0);
}



void soc_clock_gate_single(clk_clock_gate_t ip, uint32_t state)
{
}

void soc_soft_reset_single(clk_soft_reset_t ip)
{
}

csi_error_t soc_sysclk_config(system_clk_config_t *config)
{
    return CSI_OK;
}

void soc_set_sys_freq(uint32_t val)
{
}
