#include <bl606p_glb.h>
#include <bl606p_pds.h>
#include <bl606p_gpio.h>

#include "../../chip_bl606p.h"
#include "bl_sys.h"

#include <stdint.h>
#include <bl606p_pds.h>
#include <bl606p_glb.h>
#include <bl606p_glb_gpio.h>
#include <mm_misc_reg.h>
#include <core_rv32.h>
#include "bl606p_psram.h"

/****************************************************************************/ /**
 * @brief  power on mm system
 *
 * @param  None
 *
 * @return SUCCESS or ERROR
 *
*******************************************************************************/
uint32_t ATTR_CLOCK_SECTION pds_power_on_mm_system(void)
{
    uint32_t tmpVal = 0;

    /* mm_pwr_off=0, [1]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_CLR_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_PWR_OFF);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* wait > 30us */
    arch_delay_us(45);

    /* mm_iso_en=0, [5]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_CLR_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_ISO_EN);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_gate_clk=0, [17]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_CLR_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_GATE_CLK);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_stby=0, [13]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_CLR_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_MEM_STBY);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    /* mm_reset=0, [9]=0 */
    tmpVal = BL_RD_REG(PDS_BASE, PDS_CTL2);
    tmpVal = BL_CLR_REG_BIT(tmpVal, PDS_CR_PDS_FORCE_MM_PDS_RST);
    BL_WR_REG(PDS_BASE, PDS_CTL2, tmpVal);

    return 0;
}

void bl_enable_cpu0(void)
{
    pds_power_on_mm_system();
}

void bl_boot_cpu0(uint32_t start_addr)
{
    GLB_Halt_CPU(GLB_CORE_ID_D0);
    GLB_Set_CPU_Reset_Address(GLB_CORE_ID_D0, start_addr);
    GLB_Release_CPU(GLB_CORE_ID_D0);
}

uint32_t bl_cpuid_get(void)
{
    uint32_t ui32_value;

    ui32_value = BL_GET_REG32(BL_REG_CORE_ID_BASE);
    switch (ui32_value) {
        case BL_REG_CORE_ID_D0:
            ui32_value = BL_CPUID_D0;
            break;
        case BL_REG_CORE_ID_M0:
            ui32_value = BL_CPUID_M0;
            break;
        case BL_REG_CORE_ID_LP:
            ui32_value = BL_CPUID_LP;
            break;
        default:
            ui32_value = 0xFF;
            break;
    }

    return ui32_value;
}

static void _init_psram_gpio(void)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.pullType = GPIO_PULL_NONE;
    cfg.drive = 0;
    cfg.smtCtrl = 1;

    for (uint8_t i = 0; i < 12; i++) {
        cfg.gpioPin = 52 + i;
        //	cfg.gpioFun = GPIO_FUN_UART;
        cfg.gpioMode = GPIO_MODE_INPUT;

        GLB_GPIO_Init(&cfg);
    }
}

static void _psram_winbond_init(int8_t burst_len, uint8_t is_fixLatency, uint8_t latency)
{
	PSRAM_Ctrl_Cfg_Type psramCtrlCfg = {
	    .vendor = PSRAM_CTRL_VENDOR_WINBOND,
	    .ioMode = PSRAM_CTRL_X8_MODE,
	    .size = PSRAM_SIZE_4MB,
	};

	PSRAM_Winbond_Cfg_Type winbondCfg = {
	    .rst = DISABLE,
	    .clockType = PSRAM_CLOCK_DIFF, //
	    .inputPowerDownMode = DISABLE,
	    .linear_dis = ENABLE,
	    .hybridSleepMode = DISABLE,
	    .PASR = PSRAM_PARTIAL_REFRESH_FULL,
	    .disDeepPowerDownMode = ENABLE,
	    .fixedLatency = DISABLE,
	    .brustLen = PSRAM_WINBOND_BURST_LENGTH_64_BYTES,
	    .brustType = PSRAM_WRAPPED_BURST,
	    .latency = PSRAM_WINBOND_3_CLOCKS_LATENCY,
	    .driveStrength = PSRAM_WINBOND_DRIVE_STRENGTH_35_OHMS_FOR_4M_115_OHMS_FOR_8M,
	};

    winbondCfg.brustLen = burst_len;
    winbondCfg.fixedLatency = is_fixLatency;
    winbondCfg.latency = latency;

    //    dump_psram_config();
    PSram_Ctrl_Init(PSRAM0_ID, &psramCtrlCfg);
    //MSG("0x20052010=0x%x\r\n", *((uint32_t *)0x20052010));
    //MSG("0x20052014=0x%x\r\n", *((uint32_t *)0x20052014));
    //MSG("0x20052114=0x%x\r\n", *((uint32_t *)0x20052114));
    PSram_Ctrl_Winbond_Reset(PSRAM0_ID);
    //    dump_psram_config();
    //MSG("sw reset\r\n");
    //	MSG("0x%x\r\n",*((uint32_t*)0x20000620));
    //MSG("0x20052010=0x%x\r\n", *((uint32_t *)0x20052010));
    //MSG("0x20052014=0x%x\r\n", *((uint32_t *)0x20052014));
    PSram_Ctrl_Winbond_Write_Reg(PSRAM0_ID, PSRAM_WINBOND_REG_CR0, &winbondCfg);
    //	MSG("w latency 1\r\n");
    //MSG("w cr0\r\n");
    //MSG("0x20052010=0x%x\r\n", *((uint32_t *)0x20052010));
    //MSG("0x20052014=0x%x\r\n", *((uint32_t *)0x20052014));
    //dump_psram_config();
}

void boot_c906(int boot_addr)
{
    GLB_GPIO_Cfg_Type gpio_init;

    PDS_Power_On_MM_System();

    //l2_sram_vram_config();

    /* Enable C906 JTAG */
    gpio_init.gpioFun = 27; // C906 JTAG
    gpio_init.gpioMode = GPIO_MODE_AF;
    gpio_init.pullType = GPIO_PULL_UP;
    gpio_init.drive=1;
    gpio_init.smtCtrl=1;

    gpio_init.gpioPin = 0;
    GLB_GPIO_Init(&gpio_init);

    gpio_init.gpioPin = 1;
    GLB_GPIO_Init(&gpio_init);

    gpio_init.gpioPin = 2;
    GLB_GPIO_Init(&gpio_init);

    gpio_init.gpioPin = 3;
    GLB_GPIO_Init(&gpio_init);

    bl_boot_cpu0(boot_addr);
}

void bl_sys_psram_init(void)
{
	_init_psram_gpio();
    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_PSRAM0_CTRL | GLB_AHB_CLOCK_PSRAM1_CTRL);
    GLB_Set_PSram_CLK(1, 1, GLB_PSRAM_EMI_WIFIPLL_320M, 0);
    csi_dcache_enable();
    _psram_winbond_init(0, 0, PSRAM_WINBOND_6_CLOCKS_LATENCY);
}
