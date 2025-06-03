/*
 * Copyright (C) 2017-2022 Bouffalolab Group Holding Limited
 * Change Logs:
 *   Date        Author       Notes
 *   2019-03-25  MSP          the first version
 */

#ifndef __MULTICORE_DRIVER_H__
#define __MULTICORE_DRIVER_H__

#include <aos/aos.h>

#ifdef __cplusplus
extern "C" {
#endif

#define WAIT_FOREVER 0xFFFFFFFF

#define MIPC_SERIVCE_ID  0x22

#define MIPC_CTL_CMD     0x1
#define MIPC_HEARTBEAT   0x2

typedef void (*mipc_process_func_t)(void *arg, const void *data, uint32_t size, void *reslut, uint32_t reslut_size);

int multicore_server_init(mipc_process_func_t cb, void *arg);

void *multicore_cli_init(void);
int multicore_cli_send(void *handle, const void *data, uint32_t size, void *reslut, uint32_t reslut_size);

int multicore_heartbeat_init(void *multicore_handle);

#ifdef __cplusplus
}
#endif

#endif
