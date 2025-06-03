/*
 * Copyright (C) 2017-2022 Bouffalolab Group Holding Limited
 * Change Logs:
 *   Date        Author       Notes
 *   2019-03-25  MSP          the first version
 */
#include "multicore_driver.h"
#include <ipc.h>
#include <csi_core.h>
#include <aos/kernel.h>
#include <stdio.h>
#include <k_api.h>
#include "internal.h"

#define CONFIG_HEATBEAT_TIMEOUT_S 5

#define SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

typedef struct {
    uint64_t addr;
    int len;
} msg_t;

typedef struct {
    ipc_t              *ipc;
    mipc_process_func_t cb;
    void *arg;
#if defined CPU_M0
    time_t heatbeat_time;
    aos_timer_t timer;
    uint8_t heatbeat_reset;
#endif
} mipc_manager_t;

#if defined CPU_M0
static void _heatbeat_check(void *timer, void *arg)
{
    mipc_manager_t *handle = (mipc_manager_t *)arg;
    
    static uint8_t check_cnt = 0;
    static time_t last_heatbeat = 0;

    if (last_heatbeat && (last_heatbeat == handle->heatbeat_time)) {
        check_cnt++;
        printf("heatbeat check_cnt:%ld\r\n", check_cnt);
    } else {
        check_cnt = 0;
    }
    if (check_cnt >= CONFIG_HEATBEAT_TIMEOUT_S) {
       
        aos_event_set(&handle->ipc->evt, CHANNEL_READ_EVENT, AOS_EVENT_OR);
        CPSR_ALLOC();
        RHINO_CRITICAL_ENTER();
        blyoc_ipc_uninit(blyoc_cpuid_get());
        bootc906_start();
        blyoc_ipc_init(blyoc_cpuid_get());
        int ret = ipc_send_enable(&handle->ipc, 0);
        printf("heatbeat err %ld, ret: %d reboot c906\r\n", handle->heatbeat_time, ret);
        RHINO_CRITICAL_EXIT();
        
        check_cnt = 0;
        handle->heatbeat_time = 0;
        handle->heatbeat_reset = 1;
    }
    last_heatbeat = handle->heatbeat_time;
}
#endif

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
    mipc_manager_t *handle = (mipc_manager_t *)priv;
    
    switch (msg->command) {
    case MIPC_CTL_CMD: {
    	if (handle->cb) {
    		handle->cb(handle->arg,
    				(const void *)msg->req_data, msg->req_len,
					(void *)msg->resp_data, msg->resp_len);
    	}
    } break;
    case MIPC_HEARTBEAT: {
#if defined CPU_M0
        handle->heatbeat_time = *(time_t *)msg->req_data;

        if (sizeof(time_t) != msg->req_len) {
            printf("recv size error %d\r\n", msg->req_len);
        }

        struct tm *tm = localtime(&handle->heatbeat_time);
        //printf("heatbeat:%lld %02d:%02d:%02d\r\n", handle->heatbeat_time, tm->tm_hour, tm->tm_min, tm->tm_sec);
        
        *(int *)msg->resp_data = 0;
        
        if (handle->heatbeat_reset) {
            if (handle->heatbeat_time > 1) {
                return;
            }
            printf("CHANNEL_WRITE_EVENT enable\r\n");
            ipc_send_enable(&handle->ipc, 1);
            handle->heatbeat_reset = 0;
        }
#endif
    } break;
    default :
        break;
    }

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
    }
}

int multicore_server_init(mipc_process_func_t cb, void *arg)
{
    mipc_manager_t *handle = aos_calloc(sizeof(mipc_manager_t), 1);
#if defined CPU_M0
    handle->ipc = ipc_get(0);
    aos_timer_new(&handle->timer, _heatbeat_check, handle, 1000, 1);
#elif defined CPU_D0
    handle->ipc = ipc_get(1);
#endif
    handle->cb  = cb;
    handle->arg = arg;
    ipc_add_service(handle->ipc, MIPC_SERIVCE_ID, _msg_process, handle);

    return 0;
}
