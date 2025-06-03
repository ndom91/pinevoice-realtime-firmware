
#include <stdio.h>
#include <stdbool.h>

#include <bl606p_glb.h>
#include <bl606p_pds.h>
#include "blyoc_sys.h"

void blyoc_enable_cpu0(void)
{
    PDS_Power_On_MM_System();
}

void blyoc_boot_cpu0(uint32_t start_addr)
{
    GLB_Halt_CPU(GLB_CORE_ID_D0);
    GLB_Set_CPU_Reset_Address(GLB_CORE_ID_D0, start_addr);
    GLB_Release_CPU(GLB_CORE_ID_D0);
}

uint32_t blyoc_cpuid_get(void)
{
    uint32_t ui32_value;

    ui32_value = BLYOC_GET_REG32(BLYOC_REG_CORE_ID_BASE);
    switch (ui32_value) {
        case BLYOC_REG_CORE_ID_D0:
            ui32_value = BLYOC_CPUID_D0;
            break;
        case BLYOC_REG_CORE_ID_M0:
            ui32_value = BLYOC_CPUID_M0;
            break;
        case BLYOC_REG_CORE_ID_LP:
            ui32_value = BLYOC_CPUID_LP;
            break;
        default:
            ui32_value = 0xFF;
            break;
    }

    return ui32_value;
}

uint32_t soc_get_cpu_id(void)
{
    return blyoc_cpuid_get();
}
