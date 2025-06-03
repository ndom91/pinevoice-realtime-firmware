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

#if defined CPU_D0
//extern uint8_t g_cpu_m0_ready;
static void heartbeat_func(void *timer, void *arg)
{
    int res;
    static time_t rawtime;
    void *handle = arg;

    time(&rawtime);
    //printf("heartbeat %ld ...\r\n", rawtime);
    multicore_cli_ipc_send(handle, MIPC_HEARTBEAT, &rawtime, sizeof(time_t), &res, sizeof(res));
}

int multicore_heartbeat_init(void *multicore_handle)
{
    static aos_timer_t timer;
    aos_timer_new(&timer, heartbeat_func, multicore_handle, 1000, 1);
    return 0;
}
#endif
