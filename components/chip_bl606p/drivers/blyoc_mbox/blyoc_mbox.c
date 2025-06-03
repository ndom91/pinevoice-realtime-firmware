/******************************************************************************
 * @file     blyoc_mbox.c
 * @brief    CSI Source File for mailbox Driver
 * @version  V1.0
 * @date     06. Mar 2019
 ******************************************************************************/
#include "internal.h"
#include <drv/mbox.h>
#include <drv/irq.h>
#include <drv/porting.h>

#include <soc.h>
#include "blyoc_sys/bl_sys.h"
#include "blyoc_sys/blyoc_sys.h"
#include "bl_ipc.h"
#include "blyoc_mbox.h"

#define BL_SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define BL_SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

/**************************mbox dev class****************************/
extern bl_ipc_priv_t ipc_desc[BL_CPUID_MAX];
const uint32_t mbox_data_vector[BL_CPUID_MAX][BL_CPUID_MAX - 1] = {
    {BL_CPUID_M0, BL_CPUID_LP},
    {BL_CPUID_D0, BL_CPUID_LP},
    {BL_CPUID_M0, BL_CPUID_D0}                                    // TODO:可以根据上层协议任意更改
};

static uint32_t blyoc_get_ipc_shareram(uint32_t src, uint32_t dst)
{
    return (BL_IPC_SHARERAM_BASE + (src * 3 * BL_IPC_SHARERAM) + (dst * BL_IPC_SHARERAM));
}

int32_t blyoc_ipc_send(uint32_t srcid, uint32_t dstid, uint8_t *data, uint32_t size)
{
    uint8_t *shareram = NULL;
    //phy_data_t *send_data;

    shareram = (uint8_t *)blyoc_get_ipc_shareram(srcid, dstid);
    if (size > (BL_IPC_SHARERAM - 4)) {
        return -1;
    }

    memcpy(shareram, &size, 4);                                    // memcpy to shareram
    memcpy(shareram + 4, data, size);
    //send_data = (phy_data_t *)(shareram + 4);

    csi_dcache_clean_range(SHM_ALIGN(shareram, SHM_ALIGN_CACHE), SHM_ALIGN_SIZE(size + 4, 64));                    // cash flush
    //printf("blyoc_ipc_send :size %p service_id:%d, command:%d, len:%d, data:0x%08llx, seq:%d\r\n", shareram, send_data->service_id, send_data->command, send_data->len, send_data->data, send_data->seq);
    bl_ipc_trigger(srcid, dstid, BL_IPC_INTRRUPT_SEND_DATA_MASK);   // trigger interrupt

    return size;
}

static int32_t blyoc_ipc_recv(uint32_t srcid, uint32_t dstid, uint8_t *data, uint32_t size)
{
    uint8_t *pui8_shareram = NULL;
    uint32_t ui32_real_len;
    //phy_data_t *recv_data;

    if (size > (BL_IPC_SHARERAM - 4)) {
        return -1;                                                     // user need data len must < fifo len
    }

    pui8_shareram = (uint8_t *)blyoc_get_ipc_shareram(srcid, dstid);      // memcpy to data
    csi_dcache_invalid_range(SHM_ALIGN(pui8_shareram, SHM_ALIGN_CACHE), SHM_ALIGN_SIZE(size + 4, 64));

    ui32_real_len = *((uint32_t *)pui8_shareram);
    if (ui32_real_len > (BL_IPC_SHARERAM - 4)) {
        return -1;
    }

    if (ui32_real_len > size) {
        ui32_real_len = size;                                          // get user need data lth
    }

    csi_dcache_invalid_range(SHM_ALIGN(pui8_shareram, SHM_ALIGN_CACHE), SHM_ALIGN_SIZE(ui32_real_len+4, 64));
    memcpy(data, pui8_shareram + 4, ui32_real_len);

    //recv_data = (phy_data_t *)(data);
    //printf("blyoc_ipc_recv :size %p service_id:%d, command:%d, data:0x%08llx, seq:%d\r\n", pui8_shareram, recv_data->service_id, recv_data->command, recv_data->data, recv_data->seq);
    bl_ipc_trigger(dstid, srcid, BL_IPC_INTRRUPT_FIELD_ACK_MASK);   // trigger interrupt

    return ui32_real_len;
}

uint32_t g_isr_ipc_cnt1     = 0;
uint32_t g_isr_ipc_cnt2     = 0;
uint32_t g_isr_ipc_ackcnt1  = 0;
uint32_t g_isr_ipc_ackcnt2  = 0;
uint32_t g_isr_ipc_datacnt1 = 0;
uint32_t g_isr_ipc_datacnt2 = 0;

static void plyoc_interrupt_handler(uint32_t ui32_rawstatus, uint8_t ui8_cpuid, csi_mbox_t *mbox)
{
    // callback
            // ----H ----------------------------------L----
            // 0000 0000   0000 0000   0000 0000   0000 0000
            // default       cpu2         cpu1        cpu0
    pbl_ipc_priv_t pblyoc_mbox;
    int i_value = 0;

    pblyoc_mbox = (pbl_ipc_priv_t)mbox->priv;

#if defined(CONFIG_CPU_E907)
    ui32_rawstatus = (ui32_rawstatus >> 0);
#else if defined(CONFIG_CPU_C906)
    ui32_rawstatus = (ui32_rawstatus >> 8);
#endif
    //for (i_value = 0; i_value < BL_CPUID_MAX; i_value++)             // TODO: 遍历三次处理相应的中断
    g_isr_ipc_cnt1++;
    {
    	if (ui32_rawstatus & BLYOC_IPC_INTRRUPT_FIELD_ACK_MASK) {
            g_isr_ipc_ackcnt1++;
            if(NULL != mbox->callback){                                // TODO:表明数据发送完成，可以区处理该地址的数据，并清除相应的中断位
                g_isr_ipc_ackcnt2++;
                mbox->callback(mbox, MBOX_EVENT_SEND_COMPLETE, pblyoc_mbox->ui32_channel_id, pblyoc_mbox->ui32_bufflen, mbox->arg);
            }
        }

        if (ui32_rawstatus & BLYOC_IPC_INTRRUPT_SEND_DATA_MASK) {
            g_isr_ipc_datacnt1++;
            if(NULL != mbox->callback){                                // TODO:应答报，释放一个应答事件，表明处理完成可以发送下一包数据，并清除应答状态位
                g_isr_ipc_datacnt2++;
                mbox->callback(mbox, MBOX_EVENT_RECEIVED, pblyoc_mbox->ui32_channel_id, BL_IPC_SHARERAM - 4, mbox->arg);
            }
        }

        //ui32_rawstatus = (ui32_rawstatus >> 8);
    }
    g_isr_ipc_cnt2++;
}

static int blyoc_ipc_irqhander(void *arg)
{
    csi_mbox_t *mbox = (csi_mbox_t *)arg;
    uint32_t ui32_rawstatus;
    uint32_t ui32_cpuid;
    pbl_ipc_priv_t pblyoc_mbox;

    if (NULL == mbox) {
        return -1;
    }

    pblyoc_mbox = (pbl_ipc_priv_t)mbox->priv;
    ui32_cpuid = pblyoc_mbox->ui32_local_id;

    ui32_rawstatus = bl_ipc_get_isr_status(ui32_cpuid);                // get raw status

    plyoc_interrupt_handler(ui32_rawstatus, ui32_cpuid, mbox);

    bl_ipc_clear_pend(ui32_cpuid, ui32_rawstatus);                     // clear interrupt status

    return 0;
}

int blyoc_mbox_init(csi_mbox_t *mbox, uint32_t idx)
{
    uint32_t  ui32_cpuid = 0;

    if (NULL == mbox) {
        return -1;
    }

    ui32_cpuid = blyoc_cpuid_get();
    mbox->priv = &ipc_desc[ui32_cpuid];
    mbox->dev.irq_num = ipc_desc[ui32_cpuid].bl_ipc_irq_num;

    csi_irq_attach(mbox->dev.irq_num, &blyoc_ipc_irqhander, (void *)mbox);
    csi_irq_enable(mbox->dev.irq_num);

    blyoc_ipc_init(ui32_cpuid);

    return 0;
}

int32_t blyoc_mbox_send(csi_mbox_t *mbox, uint32_t ui32_channel_id, const void *data, uint32_t size)
{
    uint32_t send_id, recv_id;
    int32_t real_len;
    bl_ipc_priv_t *pblyoc_mbox;

    if (BL_CPUID_MAX - 1 < ui32_channel_id) {
        return -1;
    }

    pblyoc_mbox = (bl_ipc_priv_t *)mbox->priv;
    pblyoc_mbox->ui32_channel_id = ui32_channel_id;
    pblyoc_mbox->pui8_send_buf = (uint8_t *)data;

    send_id = bl_cpuid_get();
    recv_id = mbox_data_vector[send_id][ui32_channel_id];                  // accord channel id  to get recv_id锛�
    real_len = blyoc_ipc_send(send_id, recv_id, (uint8_t *)data, size);     // miss alagin  ?触发一个中断

    pblyoc_mbox->ui32_bufflen = real_len;

    return real_len;
}

int32_t blyoc_mbox_receive(csi_mbox_t *mbox, uint32_t ui32_channel_id, void *data, uint32_t size)
{
    uint32_t recv_id, send_id;
    int32_t real_len;
    bl_ipc_priv_t *pblyoc_mbox;

    if (BL_CPUID_MAX - 1 < ui32_channel_id) {
        return -1;
    }

    pblyoc_mbox = (bl_ipc_priv_t *)mbox->priv;
    pblyoc_mbox->ui32_channel_id = ui32_channel_id;
    pblyoc_mbox->pui8_recv_buf = data;

    recv_id = bl_cpuid_get();
    send_id = mbox_data_vector[recv_id][ui32_channel_id];                  // accord channel id  to get recv_id
    real_len = blyoc_ipc_recv(send_id, recv_id, (uint8_t *)data, size);
    pblyoc_mbox->ui32_bufflen = real_len;

    return real_len;
}

void blyoc_mbox_uninit(csi_mbox_t *mbox)
{
    int i32_cpuid;

    if (NULL == mbox) {
        return;
    }

    i32_cpuid = bl_cpuid_get();
    blyoc_ipc_uninit(i32_cpuid);
    csi_irq_disable(mbox->dev.irq_num);
}

