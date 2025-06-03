/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */


/******************************************************************************
 * @file     system.c
 * @brief    CSI Device System Source File
 * @version  V1.0
 * @date     02. Oct 2018
 ******************************************************************************/

#include <csi_config.h>
#include <soc.h>
#include <csi_core.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <drv/dma.h>
#include <drv/etb.h>
#include <drv/spiflash.h>

csi_dma_t g_dma;
#ifdef CONFIG_XIP
static csi_spiflash_t g_spiflash;
#endif

void section_data_copy(void);
void section_ram_code_copy(void);
void section_bss_clear(void);

static void sys_dma_init(void)
{
    csi_dma_init(&g_dma, 0);
}

uint32_t soc_dma_address_remap(uint32_t addr)
{
    uint32_t remap_addr = (addr >> 28) & 0xf;
    addr &= ~0xff000000;
    addr |= 0xd0000000;
    addr |= remap_addr << 24;
    return addr;
}

static void cache_init(void)
{
    uint32_t mxstatus = __get_MXSTATUS();
    mxstatus |= (1 << 22);
    __set_MXSTATUS(mxstatus);
    csi_dcache_enable();
    csi_icache_enable();
}

static void section_init(void)
{
#ifdef CONFIG_XIP
    section_data_copy();
    section_ram_code_copy();
    csi_dcache_clean();
    csi_icache_invalid();
#endif

    section_bss_clear();
}

static void clic_init(void)
{
    int i;

//    CLIC->CLICCFG = 0x4UL;
    /* get interrupt level from info */
    CLIC->CLICCFG = (((CLIC->CLICINFO & CLIC_INFO_CLICINTCTLBITS_Msk) >> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);

    for (i = 0; i < 64; i++) {
        CLIC->CLICINT[i].IP = 0;
        CLIC->CLICINT[i].ATTR = 1; /* use vector interrupt */
    }

    /* config csitimer tick irq priority */
    csi_vic_set_prio(DW_TIMER0_IRQn, 3U);

    /* tspend use positive interrupt */
    CLIC->CLICINT[Machine_Software_IRQn].ATTR = 0x3;
    csi_irq_enable(Machine_Software_IRQn);
}

static void interrupt_init(void)
{
    clic_init();
#ifdef CONFIG_KERNEL_NONE
    __enable_excp_irq();
#endif
}

#ifdef CONFIG_XIP
static void sys_spiflash_init(void)
{
    csi_spiflash_qspi_init(&g_spiflash, 0);
    csi_spiflash_config_data_line(&g_spiflash, SPIFLASH_DATA_4_LINES);
    csi_spiflash_frequence(&g_spiflash, 10000000U);
}
#endif


/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    cache_init();

    section_init();

    interrupt_init();

    // csi_etb_init();

    // sys_dma_init();

    csi_tick_init();

#ifdef CONFIG_XIP
    sys_spiflash_init();
#endif

}

uint32_t soc_get_cpu_id(void)
{
    return 0;
}
