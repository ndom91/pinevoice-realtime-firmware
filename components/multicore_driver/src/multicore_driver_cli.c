/*
 * Copyright (C) 2017-2022 Bouffalolab Group Holding Limited
 * Change Logs:
 *   Date        Author       Notes
 *   2019-03-25  MSP          the first version
 */

/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include "multicore_driver.h"
#include <ipc.h>
#include <csi_core.h>
#include <string.h>

#define SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

typedef struct {
    uint64_t addr;
    int len;
} msg_t;

typedef struct {
    ipc_t              *ipc;
    aos_mutex_t        lock;
} mipc_manager_t;

/* Global FS lock init */
static void mipc_lock_create(aos_mutex_t *lock)
{
    aos_mutex_new(lock);
}

/* Global FS lock destroy */
static void mipc_lock_destory(aos_mutex_t *lock)
{
    if (lock) {
        aos_mutex_free(lock);
    }
}

static void mipc_lock(aos_mutex_t *lock)
{
    if (lock) {
        aos_mutex_lock(lock, WAIT_FOREVER);
    }
}

static void mipc_unlock(aos_mutex_t *lock)
{
    if (lock) {
    	aos_mutex_unlock(lock);
    }
}

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
    msg_t *m = (msg_t *)msg->req_data;

    if (m && m->addr) {
        csi_dcache_invalid_range((size_t *)m->addr, m->len);
    }

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
    }
}

int multicore_cli_ipc_send(void *handle, int cmd, void *req_data, int req_size,
                       void *resp_data, int resp_size)
{
    int res = 0;
    mipc_manager_t *hdl = (mipc_manager_t *)handle;

    message_t send_msg;

    mipc_lock(&hdl->lock);
    memset(&send_msg, 0, sizeof(message_t));

    send_msg.command    = cmd;
    send_msg.flag       = MESSAGE_SYNC;
    send_msg.service_id = MIPC_SERIVCE_ID;
    send_msg.req_data   = (uint64_t)req_data;
    send_msg.req_len    = req_size;
    send_msg.resp_data  = (uint64_t)resp_data;
    send_msg.resp_len   = resp_size;

	ipc_message_send(hdl->ipc, &send_msg, AOS_WAIT_FOREVER);
	mipc_unlock(&hdl->lock);
    return res;
}

int multicore_cli_send(void *handle, const void *data, uint32_t size, void *reslut, uint32_t reslut_size)
{
	multicore_cli_ipc_send(handle, MIPC_CTL_CMD, data, size, reslut, reslut_size);
	return 0;
}

void *multicore_cli_init(void)
{
    mipc_manager_t *handle = aos_malloc(sizeof(mipc_manager_t));
    mipc_lock_create(&handle->lock);

#if defined CPU_M0
    handle->ipc = ipc_get(0);
#elif defined CPU_D0
    handle->ipc = ipc_get(1);
#endif
    ipc_add_service(handle->ipc, MIPC_SERIVCE_ID, _msg_process, handle);

    return handle;
}
