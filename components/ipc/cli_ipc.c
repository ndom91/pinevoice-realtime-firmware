/*
 * Copyright (C) 2017-2022 Bouffalolab Group Holding Limited
 * Change Logs:
 *   Date        Author       Notes
 *   2019-03-25  MSP          the first version
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

#include <ipc.h>

#include <aos/cli.h>
#include <aos/debug.h>

#include <csi_core.h>

// #include <drv/tick.h>

// #include "app_sys.h"
#define MSP_CPUID_E907                      (1)
#define MSP_CPUID_C906                      (0)

#if defined(CONFIG_CPU_E907)
#define MSP_DEST_CPUID                           MSP_CPUID_C906
#else if defined(CONFIG_CPU_C906)
#define MSP_DEST_CPUID                           MSP_CPUID_E907
#endif

#define MSP_DATA_IPC_SERIVCE_ID             (99)

#define MSP_IPC_CMD_DATA_CPU1_TO_CPU0       (100)
#define MSP_IPC_CMD_DATA_CPU1_TO_CPU0_ACK   (101)
#define MSP_IPC_CMD_DATA_CPU0_TO_CPU1       (110)
#define MSP_IPC_CMD_DATA_CPU0_TO_CPU1_ACK   (111)

#define msp_cache_flush  csi_dcache_clean_range         //(uint32_t*addr, uint32_t len)
#define msp_cache_remove csi_dcache_invalid_range       //(uint32_t*addr, uint32_t len)

typedef struct {
    uint64_t    addr;
    uint64_t    len;
} msg_t;

typedef struct {
    ipc_t *         ipc;
    aos_sem_t       sem;
    uint64_t        recv_loopstop;
    uint64_t        send_loopstop;

#if defined(CONFIG_CPU_E907)
    uint64_t        cpu1_local_seq;                 // CPU1->CPU0 E907->C906
    uint64_t        cpu1_local_seq_ack_mirro;       // CPU0->CPU1 C906->E907
    uint64_t        cpu0_local_seq_mirro;           // CPU0->CPU1 C906->E907
    uint64_t        cpu0_local_seq_ack;             // CPU1->CPU0 E907->C906
#else if defined(CONFIG_CPU_C906)
    uint64_t        cpu1_local_seq_mirro;           // CPU1->CPU0 E907->C906
    uint64_t        cpu1_local_seq_ack;             // CPU0->CPU1 C906->E907
    uint64_t        cpu0_local_seq;                 // CPU0->CPU1 C906->E907
    uint64_t        cpu0_local_seq_ack_mirro;       // CPU1->CPU0 E907->C906
#endif
} msp_snd_t;

msp_snd_t *g_msp_snd = NULL;

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
    msp_snd_t *snd = (msp_snd_t *)priv;

    switch (msg->command) {
#if defined(CONFIG_CPU_E907)
        case MSP_IPC_CMD_DATA_CPU1_TO_CPU0_ACK:
            {
                msg_t *m = (msg_t *)msg->req_data;

                msp_cache_remove((size_t *)m->addr, 64*2);

                if (sizeof(snd->cpu1_local_seq_ack_mirro) != m->len) {
                    printf("ERR: recv msg(from c906 %d) addr:%p, len:%lld\r\n",
                        MSP_IPC_CMD_DATA_CPU1_TO_CPU0_ACK, (void *)m->addr, m->len);
                    break;
                }

                memcpy(&(snd->cpu1_local_seq_ack_mirro), (void *)m->addr, sizeof(snd->cpu1_local_seq_ack_mirro));

                // printf("recv msg(from c906 %d) addr:%p, len:%lld, cpu1_local_seq_ack_mirro = %lld\r\n",
                //    MSP_IPC_CMD_DATA_CPU1_TO_CPU0_ACK, (void *)m->addr, m->len, snd->cpu1_local_seq_ack_mirro);
            } break;
        case MSP_IPC_CMD_DATA_CPU0_TO_CPU1:
            {
                msg_t *m = (msg_t *)msg->req_data;

                msp_cache_remove((size_t *)m->addr, 64*2);

                if (sizeof(snd->cpu0_local_seq_mirro) != m->len) {
                    printf("ERR: recv msg(from c906 %d) addr:%p, len:%lld\r\n",
                        MSP_IPC_CMD_DATA_CPU0_TO_CPU1, (void *)m->addr, m->len);
                    break;
                }

                memcpy(&(snd->cpu0_local_seq_mirro), (void *)m->addr, sizeof(snd->cpu0_local_seq_mirro));

                // printf("recv msg(from c906 %d) addr:%p, len:%lld, cpu0_local_seq_mirro:%lld\r\n",
                //    MSP_IPC_CMD_DATA_CPU0_TO_CPU1, (void *)m->addr, m->len, snd->cpu0_local_seq_mirro);
            } break;
#else if defined(CONFIG_CPU_C906)
        case MSP_IPC_CMD_DATA_CPU1_TO_CPU0:
            {
                msg_t *m = (msg_t *)msg->req_data;

                msp_cache_remove((size_t *)m->addr, 64*2);

                if (sizeof(snd->cpu1_local_seq_mirro) != m->len) {
                    printf("ERR: recv msg(from e907 %d) addr:0x%08x, len:%d\r\n",
                        MSP_IPC_CMD_DATA_CPU1_TO_CPU0, (void *)m->addr, m->len);
                    break;
                }

                memcpy(&(snd->cpu1_local_seq_mirro), (void *)m->addr, sizeof(snd->cpu1_local_seq_mirro));

                // printf("recv msg(from e907 %d) addr:0x%08x, len:%d, cpu1_local_seq_mirro:%lld\r\n",
                //     MSP_IPC_CMD_DATA_CPU1_TO_CPU0, (void *)m->addr, m->len, snd->cpu1_local_seq_mirro);
            } break;
        case MSP_IPC_CMD_DATA_CPU0_TO_CPU1_ACK:
            {
                msg_t *m = (msg_t *)msg->req_data;

                msp_cache_remove((size_t *)m->addr, 64*2);

                if (sizeof(snd->cpu0_local_seq_ack_mirro) != m->len) {
                    printf("ERR: recv msg(from e907 %d) addr:0x%08x, len:%d\r\n",
                        MSP_IPC_CMD_DATA_CPU0_TO_CPU1_ACK, (void *)m->addr, m->len);
                    break;
                }

                memcpy(&(snd->cpu0_local_seq_ack_mirro), (void *)m->addr, sizeof(snd->cpu0_local_seq_ack_mirro));

                // printf("recv msg(from e907 %d) addr:0x%08x, len:%d, cpu0_local_seq_ack_mirro:%lld\r\n",
                //     MSP_IPC_CMD_DATA_CPU0_TO_CPU1_ACK, (void *)m->addr, m->len, snd->cpu0_local_seq_ack_mirro);
            } break;
#endif
        default:
            break;
    }

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(snd->ipc, msg, AOS_WAIT_FOREVER);
    }

    aos_sem_signal(&(snd->sem));
}

int msp_ipc_msg_send(int cmd, void *data, int len)
{
    message_t       send_msg;
    msg_t m;

    memset(&send_msg, 0, sizeof(message_t));

    m.addr = (uint64_t)data;
    m.len =  (uint64_t)len;

    send_msg.command    = cmd;
    send_msg.flag       = MESSAGE_SYNC;
    send_msg.service_id = MSP_DATA_IPC_SERIVCE_ID;
    send_msg.req_data   = (uint64_t)&m;
    send_msg.req_len    = sizeof(m);

	ipc_message_send(g_msp_snd->ipc, &send_msg, AOS_WAIT_FOREVER);

	return len;
}

// memory
void *msp_malloc_align(int size, int align_bytes)
{
    void *base_ptr = NULL;
    void *mem_ptr = NULL;

    base_ptr = aos_malloc(size + align_bytes);//alloc alignbytes, rather than align_bytes-1, because we need to store offset

    mem_ptr = (void *)((int)((int *)base_ptr + align_bytes -1) & ~(align_bytes-1));
    if(mem_ptr == base_ptr) {//base_ptr already align_bytes align
        mem_ptr = base_ptr + align_bytes;//force move it to one more  alignbytes
    }
    *((int *)mem_ptr-1) = mem_ptr - base_ptr;
    // printf("offset is %d base prt %x mem_ptr %x\n",*((int *)mem_ptr -1),base_ptr,mem_ptr);
    return mem_ptr;
}

void msp_free_align(void *ptr)
{
    void *base_addr = NULL;
    // printf("%x %x\n",ptr,*((int *)ptr - 1));
    base_addr = (void *)(ptr- *((int *)ptr-1));
    // printf("ptr %x base_addr %x\n",ptr,base_addr);
    aos_free(base_addr);
}

void *msp_ipc_open(void)
{
    aos_status_t st;
    msp_snd_t *snd = NULL;

    snd = msp_malloc_align(64, sizeof(msp_snd_t)*2*2);
    memset(snd, 0, sizeof(sizeof(msp_snd_t)*2*2));

    /* creat queue */
    aos_sem_create(&(snd->sem), 500, 0);

    /* get ipc hdr */
    snd->ipc = ipc_get(MSP_DEST_CPUID);// current cpu is C906, send to E907

    ipc_add_service(snd->ipc, MSP_DATA_IPC_SERIVCE_ID, _msg_process, snd);

    return (void *)snd;
}

static void cmd_cli_ipcsloopstop(char *wbuf, int wbuf_len, int argc, char **argv)
{
    msp_snd_t *snd = g_msp_snd;

    if (NULL == snd) {
        return;
    }
    snd->send_loopstop = 1;
}
static void cmd_cli_ipcrloopstop(char *wbuf, int wbuf_len, int argc, char **argv)
{
    msp_snd_t *snd = g_msp_snd;

    if (NULL == snd) {
        return;
    }
    snd->recv_loopstop = 1;
}

void sendloop_task_entry(void *arg)
{
    msp_snd_t *snd = (msp_snd_t *)arg;

    printf("sendloop_task_entry\r\n");

#if defined(CONFIG_CPU_E907)
    // first send
    snd->cpu1_local_seq += 1;
    // printf("first send snd->cpu1_local_seq:%lld addr:%p\r\n", snd->cpu1_local_seq, &(snd->cpu1_local_seq));
    msp_cache_flush((size_t *)snd, 64*2);
    msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU1_TO_CPU0, &snd->cpu1_local_seq, sizeof(snd->cpu1_local_seq));

    while (1) {
        if (snd->cpu1_local_seq_ack_mirro == snd->cpu1_local_seq) {
            snd->cpu1_local_seq += 1;
            // printf("judge send snd->cpu1_local_seq:%lld addr:%p\r\n", snd->cpu1_local_seq, &(snd->cpu1_local_seq));
            msp_cache_flush((size_t *)snd, 64*2);
            msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU1_TO_CPU0, &snd->cpu1_local_seq, sizeof(snd->cpu1_local_seq));
        }
        if (snd->cpu1_local_seq >= 1*1000*1000*100) {
            break;
        }
        if (snd->send_loopstop) {
            break;
        }
        // aos_msleep(500);
    }
#else if defined(CONFIG_CPU_C906)
    // first send
    snd->cpu0_local_seq += 1;
    // printf("first send snd->cpu0_local_seq:%d addr:0x%08x\r\n", snd->cpu0_local_seq, &(snd->cpu0_local_seq));
    msp_cache_flush((size_t *)snd, 64*2);
    msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU0_TO_CPU1, &snd->cpu0_local_seq, sizeof(snd->cpu0_local_seq));

    while (1) {
        // judge send
        if (snd->cpu0_local_seq_ack_mirro == snd->cpu0_local_seq) {
            snd->cpu0_local_seq += 1;
            // printf("judge send snd->cpu0_local_seq:%d addr:0x%08x\r\n", snd->cpu0_local_seq, &(snd->cpu0_local_seq));
            msp_cache_flush((size_t *)snd, 64*2);
            msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU0_TO_CPU1, &snd->cpu0_local_seq, sizeof(snd->cpu0_local_seq));
        }
        if (snd->cpu0_local_seq >= 1*1000*1000*100) {
            break;
        }
        if (snd->send_loopstop) {
            break;
        }
        // aos_msleep(500);
    }
#endif

    aos_task_exit(0);
}

void recvloop_task_entry(void *arg)
{
    msp_snd_t *snd = (msp_snd_t *)arg;

    printf("recvloop_task_entry\r\n");

    while (1) {
        if (snd->recv_loopstop) {
            break;
        }
        aos_sem_wait(&(snd->sem), AOS_WAIT_FOREVER);
#if defined(CONFIG_CPU_E907)
        if ((snd->cpu0_local_seq_ack + 1) == snd->cpu0_local_seq_mirro) {
            snd->cpu0_local_seq_ack += 1;
            // printf("triger send cpu0_local_seq_ack:%d, addr:%p\r\n", snd->cpu0_local_seq_ack, &(snd->cpu0_local_seq_ack));
            msp_cache_flush((size_t *)snd, 64*2);
            msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU0_TO_CPU1_ACK, &(snd->cpu0_local_seq_ack), sizeof(snd->cpu0_local_seq_ack));
        }
#else if defined(CONFIG_CPU_C906)
        if ((snd->cpu1_local_seq_ack + 1) == snd->cpu1_local_seq_mirro) {
            snd->cpu1_local_seq_ack += 1;
            // printf("triger send cpu1_local_seq_ack:%d, addr:0x%08x\r\n", snd->cpu1_local_seq_ack, &(snd->cpu1_local_seq_ack));
            msp_cache_flush((size_t *)snd, 64*2);
            msp_ipc_msg_send(MSP_IPC_CMD_DATA_CPU1_TO_CPU0_ACK, &snd->cpu1_local_seq_ack, sizeof(snd->cpu1_local_seq_ack));
        }
#endif
    }

    aos_task_exit(0);
}

static void cmd_cli_ipcsendloop(char *wbuf, int wbuf_len, int argc, char **argv)
{
    aos_task_t task;

    if (NULL == g_msp_snd) {
        printf("error, recv loop have not runing.\r\n");
        return;
    }

    aos_task_new_ext(&task, "ipcsloop", sendloop_task_entry, g_msp_snd,
                     4096, AOS_DEFAULT_APP_PRI + 5);
}

static void cmd_cli_ipcrecvloop(char *wbuf, int wbuf_len, int argc, char **argv)
{
    aos_task_t task;

    if (NULL != g_msp_snd) {
        printf("error, recv loop have already runing.\r\n");
        return;
    }

    g_msp_snd = msp_ipc_open();

    aos_task_new_ext(&task, "ipcrloop", recvloop_task_entry, g_msp_snd,
                    4096, AOS_DEFAULT_APP_PRI + 4);
}
#if 0
void debug_printf_ipc_count(void)
{
    extern uint64_t ipc_send_data_mask0_flag;
    extern uint64_t ipc_send_data_mask1_flag;
    extern uint64_t ipc_send_data_mask2_flag;
    extern uint64_t ipc_send_data_mask3_flag;
    extern uint64_t ipc_send_data_mask_flag;
    extern uint64_t ipc_field_ack_mask0_isr;
    extern uint64_t ipc_field_ack_mask1_isr;
    extern uint64_t ipc_field_ack_mask2_isr;
    extern uint64_t ipc_field_ack_mask3_isr;
    extern uint64_t ipc_send_data_mask0_isr;
    extern uint64_t ipc_send_data_mask1_isr;
    extern uint64_t ipc_send_data_mask2_isr;
    extern uint64_t ipc_send_data_mask3_isr;
    extern uint64_t ipc_field_ack_mask_flag;
    extern uint64_t ipc_field_ack_mask0_flag;
    extern uint64_t ipc_field_ack_mask1_flag;
    extern uint64_t ipc_field_ack_mask2_flag;
    extern uint64_t ipc_field_ack_mask3_flag;
    extern uint64_t g_all_e907_per_isr_count;

#if defined(CONFIG_CPU_E907)
    printf("E907-E907 send    :%lld-%lld-%lld-%lld,total:%lld\r\n",
        ipc_send_data_mask0_flag,
        ipc_send_data_mask1_flag,
        ipc_send_data_mask2_flag,
        ipc_send_data_mask3_flag,
        ipc_send_data_mask_flag);
    printf("E907-E906 phy ack :%lld-%lld-%lld-%lld\r\n",
        ipc_field_ack_mask0_isr,
        ipc_field_ack_mask1_isr,
        ipc_field_ack_mask2_isr,
        ipc_field_ack_mask3_isr);
    printf("E907-C906 send    :%lld-%lld-%lld-%lld\r\n",
        ipc_send_data_mask0_isr,
        ipc_send_data_mask1_isr,
        ipc_send_data_mask2_isr,
        ipc_send_data_mask3_isr);
    printf("E907-C907 phy  ack:%lld-%lld-%lld-%lld,total:%lld\r\n",
        ipc_field_ack_mask0_flag,
        ipc_field_ack_mask1_flag,
        ipc_field_ack_mask2_flag,
        ipc_field_ack_mask3_flag,
        ipc_field_ack_mask_flag);
    printf("g_all_e907_per_isr_count = %lld\r\n", g_all_e907_per_isr_count);
#else
    printf("E906-E906 send    :%d-%d-%d-%d,total:%d\r\n",
        ipc_send_data_mask0_flag,
        ipc_send_data_mask1_flag,
        ipc_send_data_mask2_flag,
        ipc_send_data_mask3_flag,
        ipc_send_data_mask_flag);
    printf("E906-E907 phy ack :%d-%d-%d-%d\r\n",
        ipc_field_ack_mask0_isr,
        ipc_field_ack_mask1_isr,
        ipc_field_ack_mask2_isr,
        ipc_field_ack_mask3_isr);
    printf("E906-C907 send    :%d-%d-%d-%d\r\n",
        ipc_send_data_mask0_isr,
        ipc_send_data_mask1_isr,
        ipc_send_data_mask2_isr,
        ipc_send_data_mask3_isr);
    printf("E906-C906 phy  ack:%d-%d-%d-%d,total:%d\r\n",
        ipc_field_ack_mask0_flag,
        ipc_field_ack_mask1_flag,
        ipc_field_ack_mask2_flag,
        ipc_field_ack_mask3_flag,
        ipc_field_ack_mask_flag);
    printf("g_all_e907_per_isr_count = %d\r\n", g_all_e907_per_isr_count);
#endif
}
#endif

static void cmd_cli_ipcdump (char *wbuf, int wbuf_len, int argc, char **argv)
{
#if 0
    extern uint32_t g_count_csi_mbox_send;
    extern uint32_t g_count_csi_mbox_send_done;
    extern uint32_t g_count_csi_mbox_receive;
    extern uint32_t g_count_csi_mbox_receive_done;

    extern uint32_t g_count_channel_put_message;
    extern uint32_t g_count_channel_put_message_done;
    extern uint32_t g_count_channel_get_message;
    extern uint32_t g_count_channel_get_message_done;

    extern uint32_t g_count_ipc_message_send1;
    extern uint32_t g_count_ipc_message_send2;
    extern uint32_t g_count_ipc_message_send3;
    extern uint32_t g_count_ipc_message_send_done;
    extern uint32_t g_count_ipc_message_ack;
    extern uint32_t g_count_ipc_message_ack_done;

    debug_printf_ipc_count();

    printf("g_count_ipc_message_send1         = %d\r\n", g_count_ipc_message_send1);
    printf("g_count_ipc_message_send2         = %d\r\n", g_count_ipc_message_send2);
    printf("g_count_ipc_message_send3         = %d\r\n", g_count_ipc_message_send3);
    printf("g_count_ipc_message_send_done     = %d\r\n", g_count_ipc_message_send_done);
    printf("g_count_ipc_message_ack           = %d\r\n", g_count_ipc_message_ack);
    printf("g_count_ipc_message_ack_done      = %d\r\n", g_count_ipc_message_ack_done);
    printf("g_count_channel_put_message       = %d\r\n", g_count_channel_put_message);
    printf("g_count_channel_put_message_done  = %d\r\n", g_count_channel_put_message_done);
    printf("g_count_channel_get_message       = %d\r\n", g_count_channel_get_message);
    printf("g_count_channel_get_message_done  = %d\r\n", g_count_channel_get_message_done);
    printf("g_count_csi_mbox_send             = %d\r\n", g_count_csi_mbox_send);
    printf("g_count_csi_mbox_send_done        = %d\r\n", g_count_csi_mbox_send_done);
    printf("g_count_csi_mbox_receive          = %d\r\n", g_count_csi_mbox_receive);
    printf("g_count_csi_mbox_receive_done     = %d\r\n", g_count_csi_mbox_receive_done);
#endif

    if (NULL != g_msp_snd) {
#if defined(CONFIG_CPU_E907)
        printf("cpu1_local_seq            = %lld\r\n", g_msp_snd->cpu1_local_seq);
        printf("cpu1_local_seq_ack_mirro  = %lld\r\n", g_msp_snd->cpu1_local_seq_ack_mirro);
        printf("cpu0_local_seq_mirro      = %lld\r\n", g_msp_snd->cpu0_local_seq_mirro);
        printf("cpu0_local_seq_ack        = %lld\r\n", g_msp_snd->cpu0_local_seq_ack);
#else if defined(CONFIG_CPU_C906)
        printf("cpu1_local_seq_mirro      = %d\r\n", g_msp_snd->cpu1_local_seq_mirro);
        printf("cpu1_local_seq_ack        = %d\r\n", g_msp_snd->cpu1_local_seq_ack);
        printf("cpu0_local_seq            = %d\r\n", g_msp_snd->cpu0_local_seq);
        printf("cpu0_local_seq_ack_mirro  = %d\r\n", g_msp_snd->cpu0_local_seq_ack_mirro);
#endif
    }
}

static const struct cli_command cmd_info[] = {
    { "ipcdump", "ipc dump", cmd_cli_ipcdump },
    { "ipcslstop", "ipc loop stop", cmd_cli_ipcsloopstop },
    { "ipcrlstop", "ipc loop stop", cmd_cli_ipcrloopstop },
    { "ipcsloop", "ipc send loop", cmd_cli_ipcsendloop },
    { "ipcrloop", "ipc recv loop", cmd_cli_ipcrecvloop },
};

/************************************************************************
    test way1
        E907:   ipcrloop
        C906:   ipcrloop
                ipcsend
************************************************************************/
void cli_reg_cmd_ipc(void)
{
    aos_cli_register_commands(cmd_info, sizeof(cmd_info)/sizeof(struct cli_command));
}
