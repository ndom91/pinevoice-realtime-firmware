/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#ifndef __PCM_INPUT_H__
#define __PCM_INPUT_H__

#include <aos/aos.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define IPC_CMD_PCM_START            0x1
#define KWS_WEAKUP_EVENT             0x2
#define PCM_DATA_EVENT               0x3
#define SESSION_STOP_EVENT           0x4
#define SESSION_START_EVENT          0x5

/**
 * @brief 节点测试命令注册
 * @return void
 */
void cli_reg_cmd_pcminput(void);


#ifdef __cplusplus
}
#endif

#endif
