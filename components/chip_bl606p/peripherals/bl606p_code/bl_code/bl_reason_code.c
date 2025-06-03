#include <stdio.h>
#include <stdbool.h>
#include <bl606p_glb.h>
#include <bl606p_pds.h>
#include "bl_reason_code.h"

#define MFG_CONFIG_REG    (0x2000F100)
#define MFG_CONFIG_VAL    ("0mfg")

#define REASON_WDT        (0x77646F67) // watchdog reboot wdog
#define REASON_SOFTWARE   (0x736F6674) // software        soft
#define REASON_POWEROFF   (0x0) // software        soft

#define RST_REASON (*((volatile uint32_t *)0x20010000)) // use 4 Bytes

static BL_RST_REASON_E s_rst_reason = BL_RST_POWER_OFF;

static char *RST_REASON_ARRAY[] = {
    "BL_RST_POWER_OFF",
    "BL_RST_HARDWARE_WATCHDOG",
    "BL_RST_FATAL_EXCEPTION",
    "BL_RST_SOFTWARE_WATCHDOG",
    "BL_RST_SOFTWARE"
};
extern volatile bool sys_log_all_enable;//XXX in debug.c

BL_Err_Type ATTR_CLOCK_SECTION PDS_Power_Down_MM_System(void)
{
    uint32_t tmpVal = 0;

    /* mm_pwr_off=0, [1]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_SET_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_PWR_OFF);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* wait > 30us */
    arch_delay_us(45);

    /* mm_iso_en=0, [5]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_SET_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_ISO_EN);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_gate_clk=0, [17]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_SET_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_GATE_CLK);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_stby=0, [13]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_SET_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_MEM_STBY);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_reset=0, [9]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_SET_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_PDS_RST);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    return SUCCESS;
}

BL_RST_REASON_E bl_sys_rstinfo_get(void)
{
    BL_RST_REASON_E ret = s_rst_reason;

    s_rst_reason = REASON_POWEROFF;

    return ret;
}

BL_RST_REASON_E bl_sys_rstinfo_get_ext(void)
{
    return s_rst_reason;
}

int bl_sys_rstinfo_set(BL_RST_REASON_E val)
{
    if (val == BL_RST_SOFTWARE_WATCHDOG) {
        RST_REASON = REASON_WDT;
    } else if (val == BL_RST_SOFTWARE) {
        RST_REASON = REASON_SOFTWARE;
    }

    return 0;
}

void bl_sys_rstinfo_init(void)
{
    if (RST_REASON == REASON_WDT) {
        s_rst_reason = BL_RST_SOFTWARE_WATCHDOG;
    } else if (RST_REASON == REASON_SOFTWARE) {
        s_rst_reason = BL_RST_SOFTWARE;
    } else {
        s_rst_reason = BL_RST_POWER_OFF;
    }

    bl_sys_rstinfo_set(BL_RST_SOFTWARE_WATCHDOG);
}

int bl_sys_rstinfo_getsting(char *info)
{
    memcpy(info, (char *)RST_REASON_ARRAY[s_rst_reason], strlen(RST_REASON_ARRAY[s_rst_reason]));
    *(info + strlen(RST_REASON_ARRAY[s_rst_reason])) = '\0';
    return 0;
}

int bl_sys_logall_enable(void)
{
    sys_log_all_enable = true;
    return 0;
}

int bl_sys_logall_disable(void)
{
    sys_log_all_enable = false;
    return 0;
}

int bl_sys_reset_por(void)
{
    uint32_t irq_flag = 0U;

    bl_sys_rstinfo_set(BL_RST_SOFTWARE);
    
    irq_flag = csi_irq_save();
    
    GLB_SW_POR_Reset();
    while (1) {
        /*empty dead loop*/
    }
    (void)irq_flag;

    return 0;
}

void bl_sys_mfg_config(char *outbuf, int len, int argc, char **argv)
{
#if 1
    union _reg_t {
        uint8_t byte[4];
        uint32_t word;
    } mfg = {
        .byte = MFG_CONFIG_VAL,
    };

    *(volatile uint32_t*)(MFG_CONFIG_REG) = mfg.word;
    
    bl_sys_reset_por();
#else
    puts("WARN: bl_sys_mfg_config is NOT implemented\r\n");
#endif
}

void __attribute__((section(".tcm_code"))) bl_sys_reset_system(void)
{
    uint32_t irq_flag = 0U;
    
    bl_sys_rstinfo_set(BL_RST_SOFTWARE);

    irq_flag = csi_irq_save();

    PDS_Power_Down_MM_System();   
    *((volatile uint32_t *)(uintptr_t)(0x20000540)) |= 0x16F0F10; 

    *((volatile uint32_t *)(uintptr_t)(0x20000544)) |= 0x3FFF3304;
    GLB_SW_System_Reset();
 
    while (1) {
        /*empty dead loop*/
    }
    (void)irq_flag;
}


int bl_sys_em_config(void)
{
#if 0
    extern uint8_t __LD_CONFIG_EM_SEL;
    volatile uint32_t em_size;

    em_size = (uint32_t)&__LD_CONFIG_EM_SEL;

    switch (em_size) {
        case 0 * 1024:
        {
            GLB_Set_EM_Sel(GLB_EM_0KB);
        }
        break;
        case 8 * 1024:
        {
            GLB_Set_EM_Sel(GLB_EM_8KB);
        }
        break;
        case 16 * 1024:
        {
            GLB_Set_EM_Sel(GLB_EM_16KB);
        }
        break;
        default:
        {
            /*nothing here*/
        }
    }

    return 0;
#else
    puts("WARN: bl_sys_em_config is NOT implemented\r\n");
    return 0;
#endif
}

int bl_sys_early_init(void)
{
#if 0
    extern BL_Err_Type HBN_Aon_Pad_IeSmt_Cfg(uint8_t padCfg);
    HBN_Aon_Pad_IeSmt_Cfg(1);

    extern void freertos_risc_v_trap_handler(void); //freertos_riscv_ram/portable/GCC/RISC-V/portASM.S
    write_csr(mtvec, &freertos_risc_v_trap_handler);
    
    /* reset here for use wtd first then init hwtimer later*/
    GLB_AHB_Slave1_Reset(BL_AHB_SLAVE1_TMR);
    /*debuger may NOT ready don't print anything*/
    return 0;
#else
    puts("WARN: bl_sys_early_init is NOT implemented\r\n");
    return 0;
#endif
}

int bl_sys_init(void)
{
    //bl_sys_em_config();
    bl_sys_rstinfo_get();
    bl_sys_rstinfo_init();
    return 0;
}

#ifdef AOS_COMP_CLI
#include <aos/cli.h>
void cli_reg_cmd_mfg(void)
{
    static const struct cli_command cmd_info = { "mfg", "mfg", bl_sys_mfg_config};

    aos_cli_register_command(&cmd_info);
}
#endif
