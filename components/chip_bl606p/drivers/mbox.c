
#include <drv/mbox.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>
#include <soc.h>
#include <bl606p.h>
#include <blyoc_mbox/blyoc_mbox.h>


typedef struct {
    int             addr;
    size_t          size;
    int             block_size;
    int32_t         used_flag;
    int             inited;
} ipc_mem_t;

/**
  \brief       Initialize mbox Interface.
               Initializes the resources needed for the mbox interface.
  \param[in]   mbox  Operate handle.
  \param[in]   idx  The device idx.
  \return      Error code \ref csi_error_t.
*/
csi_error_t csi_mbox_init(csi_mbox_t *mbox, uint32_t idx)
{
    if (blyoc_mbox_init(mbox, idx)) {
        return CSI_ERROR;
    }

    return CSI_OK;
}

/**
  \brief       Uninitialize mbox interface. stops operation and releases the software resources used by the interface.
  \param[in]   mbox  Operate handle.
*/
void csi_mbox_uninit(csi_mbox_t *mbox)
{
    blyoc_mbox_uninit(mbox);
}

/**
  \brief       Start sending data to mbox transmitter.
  \param[in]   mbox  Operate handle.
  \param[in]   channel_id  Index of channel.
  \param[in]   data  Pointer to buffer with data to send to mbox transmitter.
  \param[in]   size  Number of data items to send.
  \return      sent Number of data or error code.
*/
int32_t csi_mbox_send(csi_mbox_t *mbox, uint32_t channel_id, const void *data, uint32_t size)
{
    return blyoc_mbox_send(mbox, channel_id, data, size);
}

/**
  \brief       Start receiving data from mbox receiver.
  \param[in]   mbox  Operate handle.
  \param[in]   channel_id  Index of channel.
  \param[out]  data  Pointer to buffer with data to receive from mailbox.
  \param[in]   size  Number of data items to receive.
  \return      received  Number or error code.
*/
int32_t csi_mbox_receive(csi_mbox_t *mbox, uint32_t channel_id, void *data, uint32_t size)
{
    return blyoc_mbox_receive(mbox, channel_id, data, size);
}

/**
  \brief       Attach callback to the mbox.
  \param[in]   mbox  Operate handle.
  \param[in]   cb  Event callback function.
  \param[in]   arg  User private param  for event callback.
  \return      Error code \ref csi_error_t.
*/
csi_error_t csi_mbox_attach_callback(csi_mbox_t *mbox, void *callback, void *arg)
{
    mbox->callback = callback;
    mbox->arg = arg;

    return CSI_OK;
}

/**
  \brief       Detach callback from the mbox
  \param[in]   mbox  Operate handle.
*/
void csi_mbox_detach_callback(csi_mbox_t *mbox)
{
    mbox->callback = NULL;
    mbox->arg = NULL;
}

#if 0
static ipc_mem_t g_ipc_mem;

#define IPC_MEM_START (BL_IPC_SHARERAM_BASE)
#if defined CPU_D0
    #define IPC_MEM_ADDR (IPC_MEM_START)
    #define IPC_MEM_SIZE (12 * 1024)
    #define IPC_MEM_BLOCK_SIZE (IPC_MEM_SIZE/2)
#endif

#if defined CPU_M0
    #define IPC_MEM_ADDR (IPC_MEM_START + 0x3000)
    #define IPC_MEM_SIZE (12 * 1024)
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
    return 0;
}
#endif

