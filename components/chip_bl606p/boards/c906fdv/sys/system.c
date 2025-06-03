/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. Oct 2018
 ******************************************************************************/

#include <soc.h>
#include <csi_core.h>
#include <drv/tick.h>
#include <drv/porting.h>
#include <drv/irq.h>
#include <drv/dma.h>
#include <bl606p_clock.h>

#if (defined(CONFIG_KERNEL_RHINO) || defined(CONFIG_KERNEL_FREERTOS)) && defined(CONFIG_KERNEL_NONE)
#error "Please check the current system is baremetal or not!!!"
#endif

void section_data_copy(void);
void section_ram_code_copy(void);
void section_bss_clear(void);

int32_t drv_get_cpu_id(void)
{
    return 0;
}

static void section_init(void)
{
    // section_ram_code_copy();
    // csi_dcache_clean();
    // csi_icache_invalid();
}

static void cache_init(void)
{
    /* invalid cache */
    csi_icache_invalid();
    csi_dcache_invalid();
    /* enable cache */
    csi_dcache_enable();
    csi_icache_enable();
}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */

static void interrupt_init(void)
{
    int i;

    for (i = 0; i < 1023; i++) {
        PLIC->PLIC_PRIO[i] = 31;
    }

    for (i = 0; i < 32; i++) {
        PLIC->PLIC_IP[i] = 0;
    }

    for (i = 0; i < 32; i++) {
        PLIC->PLIC_H0_MIE[i] = 0;
        PLIC->PLIC_H0_SIE[i] = 0;
    }

    /* set hart threshold 0, enable all interrupt */
    PLIC->PLIC_H0_MTH = 0;
    PLIC->PLIC_H0_STH = 0;

    for (i = 0; i < 1023; i++) {
        PLIC->PLIC_H0_MCLAIM = i;
        PLIC->PLIC_H0_SCLAIM = i;
    }

    /* set PLIC_PER */
    PLIC->PLIC_PER = 0x1;
}

static void mtimer_init(void)
{
    uint32_t clkSrc = CPU_Get_MTimer_Source_Clock();

    //CPU_Interrupt_Disable(MTIME_IRQn);

    if (clkSrc > 1 * 1000 * 1000) {
        /* Set MTimer clock source 1M */
        CPU_Set_MTimer_CLK(1, clkSrc / 1000 / 1000 - 1);
    } else {
        /* Set MTimer clock source 1k */
        CPU_Set_MTimer_CLK(1, clkSrc / 1000 - 1);
    }

    /* never reset mtimer */
    //CPU_Reset_MTimer();
}

static void mhint_init(void)
{
    uint64_t mhint = __get_MHINT();
    //mhint = 0x0504;
    mhint = (1 << 2) | (1 << 8) | (1 << 10) | (0 << 13);
    __set_MHINT(mhint);
}

static void mie_init(void)
{
    uint32_t mie = __get_MIE();
    mie |= (1 << 11 | 1 << 7 | 1 << 3);
    __set_MIE(mie);
}

void SystemInit(void)
{
    cache_init();
    section_init();
    interrupt_init();
    mtimer_init();
    csi_tick_init();
    mie_init();
    mhint_init();
}
