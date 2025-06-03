#include "../../chip_bl606p.h"
#include "blyoc_sys/bl_sys.h"
#include "bl_ipc.h"

bl_ipc_priv_t ipc_desc[BL_CPUID_MAX] = {
    {
        .bl_ipc_base    = BL_IPC_BASE_D0,
        .bl_ipc_irq_num = BL_IPC_D0_IRQn,
    },
    {
        .bl_ipc_base    = BL_IPC_BASE_M0,
        .bl_ipc_irq_num = BL_IPC_M0_IRQn,
    },
    {
        .bl_ipc_base    = BL_IPC_BASE_LP,
        .bl_ipc_irq_num = BL_IPC_LP_IRQn,
    }
};

void bl_ipc_mask_enable(uint32_t  ui32_cpuid)
{
    ipc_desc[ui32_cpuid].bl_ipc_base->int_umaskset = BL_INTRRUPT_UMASK_FLAG;

    return;
}

void bl_ipc_mask_disable(uint32_t  ui32_cpuid)
{
    ipc_desc[ui32_cpuid].bl_ipc_base->int_umaskclear = BL_INTRRUPT_UMASK_FLAG;

    return;
}

void bl_ipc_clear_pend(uint32_t  ui32_cpuid, uint32_t ui32_rawstatus)
{
    ipc_desc[ui32_cpuid].bl_ipc_base->int_clear = ui32_rawstatus;

    return;
}

uint32_t bl_ipc_get_isr_status(uint32_t  ui32_cpuid)
{

    uint32_t ui32_rawstatus = 0;

    ui32_rawstatus =  ipc_desc[ui32_cpuid].bl_ipc_base->int_rawstatus;

    return ui32_rawstatus;
}

int bl_ipc_trigger(int i32_src_id, int i32_dest_id, uint32_t ui32_value)
{
    if (ui32_value > 0xFF) {
        return -1;
    }

    ipc_desc[i32_dest_id].bl_ipc_base->trigger_int = (ui32_value << (i32_src_id * 8));

    return 0;
}

uint32_t bl_ipc_get_base(uint32_t ui32_cpuid)
{
    if (ui32_cpuid > BL_CPUID_MAX) {
        return -1;
    }

    return (uint32_t)&ipc_desc[ui32_cpuid];
}

uint32_t blyoc_ipc_init(uint32_t ui32_cpuid)
{

    if (ui32_cpuid > BL_CPUID_MAX) {
        return -1;
    }

    ipc_desc[ui32_cpuid].ui32_local_id = ui32_cpuid;

    bl_ipc_clear_pend(ui32_cpuid, BL_INTRRUPT_UMASK_FLAG); 
    bl_ipc_mask_enable(ui32_cpuid);

    return 0;
}


void blyoc_ipc_uninit(uint32_t ui32_cpuid)
{
    if (ui32_cpuid > BL_CPUID_MAX) {
        return ;
    }

    ipc_desc[ui32_cpuid].ui32_local_id = ui32_cpuid;

    bl_ipc_mask_disable(ui32_cpuid);

    return ;
}


