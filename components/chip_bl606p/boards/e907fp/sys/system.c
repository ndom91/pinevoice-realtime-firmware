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
#include <bl606p_glb.h>
#include <bl_reason_code.h>

#ifndef CONFIG_SYSTICK_HZ
#define CONFIG_SYSTICK_HZ 100
#endif

int g_system_clock = IHS_VALUE;
extern int32_t g_top_irqstack;
extern void mm_heap_initialize(void);

extern void blcsi_uart_init(uint8_t id);

extern void Default_IRQHandler(void);
// extern void CORET_IRQHandler(void);
// extern void TIM4_NMIHandler(void);

void (*g_irqvector[48])(void);
void (*g_nmivector)(void);

static void irq_vectors_init(void)
{
    int i;

    for (i = 0; i < 48; i++) {
        g_irqvector[i] = Default_IRQHandler;
    }

    // g_irqvector[CORET_IRQn] = CORET_IRQHandler;
    // g_nmivector = TIM4_NMIHandler;
}

static void _system_init_for_kernel(void)
{
     /* enable mexstatus SPUSHEN and SPSWAPEN */
#if ((CONFIG_CPU_E902 != 1) && (CONFIG_CPU_E902M != 1))
    // uint32_t mexstatus = __get_MEXSTATUS();
    // mexstatus |= (0x3 << 16);
    // __set_MEXSTATUS(mexstatus);
#endif
    irq_vectors_init();

    // csi_coret_config(32000000 / CONFIG_SYSTICK_HZ, CORET_IRQn);    //10ms
    // soc_irq_enable(CORET_IRQn);

}

/* FIXME: Use std driver */
typedef enum {
    MMSYS_REL_VRAM_64_L2_0  = 0,
    MMSYS_REL_VRAM_0_L2_64 = 1,
}MMSYS_VRAM_L2_SRAM_Type;

typedef enum {
    MMSYS_REL_VRAM_0_PFH_192  = 0,
    MMSYS_REL_VRAM_64_PFH_128 = 1,
    MMSYS_REL_VRAM_128_PFH_64 = 2,
    MMSYS_REL_VRAM_192_PFH_0  = 3
}MMSYS_VRAM_PFH_SRAM_Type;

typedef enum {
    MMSYS_REL_VRAM_0_APU_128 = 0,
    MMSYS_REL_VRAM_128_APU_0 = 1,
}MMSYS_VRAM_APU_SRAM_Type;

typedef enum {
    MMSYS_REL_VRAM_0_ISP_64 = 0,
    MMSYS_REL_VRAM_64_ISP_0 = 1,
}MMSYS_VRAM_ISP_SRAM_Type;

typedef struct {
    MMSYS_VRAM_L2_SRAM_Type  l2sram;
    MMSYS_VRAM_PFH_SRAM_Type pfh;
    MMSYS_VRAM_APU_SRAM_Type apu;
    MMSYS_VRAM_ISP_SRAM_Type isp;
}MMSYS_VRAM_Ctrl_Cfg;

/* 0x50 : vram_ctrl */
#define MMSYS_MISC_VRAM_CTRL_OFFSET                             (0x50)
#define MMSYS_MISC_REG_SYSRAM_SET                               MMSYS_MISC_REG_SYSRAM_SET
#define MMSYS_MISC_REG_SYSRAM_SET_POS                           (0U)
#define MMSYS_MISC_REG_SYSRAM_SET_LEN                           (1U)
#define MMSYS_MISC_REG_SYSRAM_SET_MSK                           (((1U<<MMSYS_MISC_REG_SYSRAM_SET_LEN)-1)<<MMSYS_MISC_REG_SYSRAM_SET_POS)
#define MMSYS_MISC_REG_SYSRAM_SET_UMSK                          (~(((1U<<MMSYS_MISC_REG_SYSRAM_SET_LEN)-1)<<MMSYS_MISC_REG_SYSRAM_SET_POS))
#define MMSYS_MISC_REG_PF_SRAM_REL                              MMSYS_MISC_REG_PF_SRAM_REL
#define MMSYS_MISC_REG_PF_SRAM_REL_POS                          (1U)
#define MMSYS_MISC_REG_PF_SRAM_REL_LEN                          (2U)
#define MMSYS_MISC_REG_PF_SRAM_REL_MSK                          (((1U<<MMSYS_MISC_REG_PF_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_PF_SRAM_REL_POS)
#define MMSYS_MISC_REG_PF_SRAM_REL_UMSK                         (~(((1U<<MMSYS_MISC_REG_PF_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_PF_SRAM_REL_POS))
#define MMSYS_MISC_REG_L2_SRAM_REL                              MMSYS_MISC_REG_L2_SRAM_REL
#define MMSYS_MISC_REG_L2_SRAM_REL_POS                          (4U)
#define MMSYS_MISC_REG_L2_SRAM_REL_LEN                          (1U)
#define MMSYS_MISC_REG_L2_SRAM_REL_MSK                          (((1U<<MMSYS_MISC_REG_L2_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_L2_SRAM_REL_POS)
#define MMSYS_MISC_REG_L2_SRAM_REL_UMSK                         (~(((1U<<MMSYS_MISC_REG_L2_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_L2_SRAM_REL_POS))
#define MMSYS_MISC_REG_ISP_SRAM_REL                             MMSYS_MISC_REG_ISP_SRAM_REL
#define MMSYS_MISC_REG_ISP_SRAM_REL_POS                         (6U)
#define MMSYS_MISC_REG_ISP_SRAM_REL_LEN                         (1U)
#define MMSYS_MISC_REG_ISP_SRAM_REL_MSK                         (((1U<<MMSYS_MISC_REG_ISP_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_ISP_SRAM_REL_POS)
#define MMSYS_MISC_REG_ISP_SRAM_REL_UMSK                        (~(((1U<<MMSYS_MISC_REG_ISP_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_ISP_SRAM_REL_POS))
#define MMSYS_MISC_REG_APU_SRAM_REL                             MMSYS_MISC_REG_APU_SRAM_REL
#define MMSYS_MISC_REG_APU_SRAM_REL_POS                         (7U)
#define MMSYS_MISC_REG_APU_SRAM_REL_LEN                         (1U)
#define MMSYS_MISC_REG_APU_SRAM_REL_MSK                         (((1U<<MMSYS_MISC_REG_APU_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_APU_SRAM_REL_POS)
#define MMSYS_MISC_REG_APU_SRAM_REL_UMSK                        (~(((1U<<MMSYS_MISC_REG_APU_SRAM_REL_LEN)-1)<<MMSYS_MISC_REG_APU_SRAM_REL_POS))

#define MMSYS_MISC_BASE             ((uint32_t)0x30000000)            /*!< MMSys misc base address */

static void MMSYS_VRAM_Ctrl(MMSYS_VRAM_Ctrl_Cfg *cfg)
{
    uint32_t tmpVal;

    tmpVal=BL_RD_WORD(MMSYS_MISC_BASE + 0x50);
    tmpVal=BL_SET_REG_BITS_VAL(tmpVal, MMSYS_MISC_REG_L2_SRAM_REL,  cfg->l2sram);
    tmpVal=BL_SET_REG_BITS_VAL(tmpVal, MMSYS_MISC_REG_PF_SRAM_REL,  cfg->pfh);
    tmpVal=BL_SET_REG_BITS_VAL(tmpVal, MMSYS_MISC_REG_APU_SRAM_REL, cfg->apu);
    tmpVal=BL_SET_REG_BITS_VAL(tmpVal, MMSYS_MISC_REG_ISP_SRAM_REL, cfg->isp);

    BL_WR_WORD(MMSYS_MISC_BASE + 0x50, tmpVal);

    tmpVal=BL_RD_WORD(MMSYS_MISC_BASE + 0x50);
    tmpVal = BL_SET_REG_BIT(tmpVal, MMSYS_MISC_REG_SYSRAM_SET);
    BL_WR_WORD(MMSYS_MISC_BASE + 0x50, tmpVal);
}

static void l2_sram_vram_config(void)
{
    MMSYS_VRAM_Ctrl_Cfg cfg;
    cfg.l2sram = MMSYS_REL_VRAM_0_L2_64;
    cfg.pfh    = MMSYS_REL_VRAM_192_PFH_0;
    cfg.apu    = MMSYS_REL_VRAM_128_APU_0;
    cfg.isp    = MMSYS_REL_VRAM_64_ISP_0;

    MMSYS_VRAM_Ctrl(&cfg);
}

static void load_section(void)
{
    extern uint32_t __ef_text;

    extern uint32_t __stext;
    extern uint32_t __etext;

    extern uint32_t __srodata;
    extern uint32_t __erodata;

    extern uint32_t __sdata;
    extern uint32_t __edata;

    extern uint32_t __etcm_data;
    extern uint32_t __tcm_start__;
    extern uint32_t __tcm_end__;

    // load text section
    volatile uint32_t *src = &__ef_text;
    volatile uint32_t *dst = &__stext;
    for (; dst < &__etext;) {
        *dst++ = *src++;
    }
 
    // load rodata section
    dst = &__srodata;
    for (; dst < &__erodata;) {
        *dst++ = *src++;
    }

    // load data section
    dst = &__sdata;
    for (; dst < &__edata;) {
        *dst++ = *src++;
    }

    // load OCRAM data section
    src = &__etcm_data;
    dst = &__tcm_start__;
    for (; dst < &__tcm_end__;) {
        *dst++ = *src++;
    }
}

void ATTR_TCM_SECTION c906_sys_clock_init(uint8_t flag)
{
#if 1
    if (flag) {
        HBN_Set_Ldo11_Aon_Vout(HBN_LDO_LEVEL_1P25V);
        HBN_Set_Ldo11_Rt_Vout(HBN_LDO_LEVEL_1P25V);
        HBN_Set_Ldo11_Rtc_Vout(HBN_LDO_LEVEL_1P25V);
        GLB_Config_CPU_PLL(GLB_XTAL_40M,cpuPllCfg_480M);
    } else {
        GLB_Config_CPU_PLL(GLB_XTAL_40M,cpuPllCfg_380M);
    }

    GLB_Set_DSP_System_CLK(GLB_DSP_SYS_CLK_CPUPLL_400M);
    GLB_Set_DSP_System_CLK_Div(0, 1);
#else
    HBN_Set_Ldo11_Aon_Vout(HBN_LDO_LEVEL_1P30V);
    HBN_Set_Ldo11_Rt_Vout(HBN_LDO_LEVEL_1P30V);
    HBN_Set_Ldo11_Rtc_Vout(HBN_LDO_LEVEL_1P30V);
    AON_Set_DCDC11_Top_Vout(AON_DCDC_LEVEL_1P130V);
    /*set cpupll 520M*/
    GLB_Config_CPU_PLL(GLB_XTAL_40M, cpuPllCfg_520M);
    /* set 906 clock source is cpupull */
    GLB_Set_DSP_System_CLK(GLB_DSP_SYS_CLK_CPUPLL_400M);
    /* set 906 bclock*/
    GLB_Set_DSP_System_CLK_Div(0, 1);
    /* EMI clock set source cpupll */
    GLB_Set_EMI_CLK(1, 3, 1);
    /*set psram source is cpupll and 2 div,if 906 clk is 480/520M, than psram clk is 240/260M*/
    //GLB_Set_PSram_CLK(1, 1, 0, 1);
#endif
}
/**
  * @brief  initialize system map
  * @param  None
  * @return None
  */
//static void systemmap_config(void)
//{
//   csi_sysmap_config_region(0, 0x20000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
//   csi_sysmap_config_region(1, 0x40000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
//   csi_sysmap_config_region(2, 0x50000000, SYSMAP_SYSMAPCFG_SO_Msk);
//   csi_sysmap_config_region(3, 0x50700000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
//   csi_sysmap_config_region(4, 0x60000000, SYSMAP_SYSMAPCFG_SO_Msk);
//   csi_sysmap_config_region(5, 0x80000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
//   csi_sysmap_config_region(6, 0x90000000, SYSMAP_SYSMAPCFG_B_Msk | SYSMAP_SYSMAPCFG_C_Msk);
//   csi_sysmap_config_region(7, 0xf0000000, SYSMAP_SYSMAPCFG_SO_Msk);
//}

/**
  * @brief  initialize the system
  *         Initialize the psr and vbr.
  * @param  None
  * @return None
  */
void SystemInit(void)
{
    int i;
    csi_dcache_clean();
    csi_icache_invalid();
#if ((CONFIG_CPU_E902 != 1) && (CONFIG_CPU_E902M != 1))
    //systemmap_config();
#endif

    /* enable mstatus FS */
#if (__riscv_flen)
    uint32_t mstatus = __get_MSTATUS();
    mstatus |= (1 << 13);
    __set_MSTATUS(mstatus);
#endif

    /* get interrupt level from info */
    CLIC->CLICCFG = (((CLIC->CLICINFO & CLIC_INFO_CLICINTCTLBITS_Msk) >> CLIC_INFO_CLICINTCTLBITS_Pos) << CLIC_CLICCFG_NLBIT_Pos);

    for (i = 0; i < 64; i++) {
        CLIC->CLICINT[i].IP = 0;
        CLIC->CLICINT[i].ATTR = 1; /* use vector interrupt */
    }

    /* tspend use positive interrupt */
    CLIC->CLICINT[Machine_Software_IRQn].ATTR = 0x3;

#if ((CONFIG_CPU_E902 != 1) && (CONFIG_CPU_E902M != 1))
    csi_dcache_enable();
#endif
    csi_icache_enable();

    load_section();

    csi_irq_enable(Machine_Software_IRQn);

    _system_init_for_kernel();
    csi_tick_init();

    void bl_sys_psram_init(void);

#if 0
    int start_addr = (int)(int64_t)(SystemInit);
    if (start_addr & 0x58000000) {
        bl_sys_psram_init();
    }
#endif
    extern void c906_sys_clock_init(uint8_t flag);
    c906_sys_clock_init(1);
    // blcsi_uart_init(0);
    bl_sys_init();
}

void sys_ram_init(void)
{
    void l2_sram_vram_config(void);
    l2_sram_vram_config();
}

int _pre_main(void)
{
    extern int pre_main(void);
    return pre_main();
}
