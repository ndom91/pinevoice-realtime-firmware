#include "bl_ipc.h"
#if 1
typedef struct {
    int             addr;
    size_t          size;
    int             block_size;
    int32_t         used_flag;
    int             inited;
} ipc_mem_t;
static ipc_mem_t g_ipc_mem;

#define IPC_MEM_START (0x40001000)//(BL_IPC_SHARERAM_BASE + BL_IPC_SHARERAM * 9)
#if defined CPU_D0
    #define IPC_MEM_ADDR (IPC_MEM_START)
    #define IPC_MEM_SIZE (6 * 1024)                                    // (0x4000 - 0x1000) / 2
    #define IPC_MEM_BLOCK_SIZE (IPC_MEM_SIZE/1)
#endif

#if defined CPU_M0
    #define IPC_MEM_ADDR (IPC_MEM_START + 0x1800)
    #define IPC_MEM_SIZE (6 * 1024)
    #define IPC_MEM_BLOCK_SIZE (IPC_MEM_SIZE/1)
#endif

//#if defined CPU_LP
//    #define IPC_MEM_ADDR (IPC_MEM_START + 0x6000)
//    #define IPC_MEM_SIZE (12 * 1024)
//    #define IPC_MEM_BLOCK_SIZE (IPC_MEM_SIZE/1)
//#endif

void drv_ipc_mem_init(void)
{
    if (g_ipc_mem.inited == 1) {
        return;
    }

    g_ipc_mem.addr          = IPC_MEM_ADDR;
    g_ipc_mem.size          = IPC_MEM_SIZE;
    g_ipc_mem.block_size    = IPC_MEM_BLOCK_SIZE;
    g_ipc_mem.used_flag     = 0;
    g_ipc_mem.inited        = 1;

}

void *drv_ipc_mem_alloc(int *len)
{
    int cnt = g_ipc_mem.size / g_ipc_mem.block_size;

    for (int i = 0; i < cnt; i++) {
        if ((g_ipc_mem.used_flag & (1 << i)) == 0) {
            g_ipc_mem.used_flag |= (1 << i);

            *len = g_ipc_mem.block_size;
            return ((char *)g_ipc_mem.addr + i * g_ipc_mem.block_size);
        }
    }

    *len = 0;
    return NULL;
}

void drv_ipc_mem_free(void *p)
{
    int addr = (int)p;
    int offset = addr - g_ipc_mem.addr;

    if (offset % g_ipc_mem.block_size == 0) {
        int i = offset / g_ipc_mem.block_size;

        g_ipc_mem.used_flag &= ~(1 << i);
    }

}

int drv_ipc_mem_use_cache(void)
{
    return 1;
}
#endif
