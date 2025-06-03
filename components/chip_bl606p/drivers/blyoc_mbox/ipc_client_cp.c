#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <ipc.h>
#include <string.h>

#define TAG "CP"
#define IPC_BUF_LEN (512)

static char test_buf[IPC_BUF_LEN] __attribute__ ((aligned(64)));
static ipc_t *ipc;

int ipc_sync(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    /* 设置message的service_id为20 */
    msg.service_id = 20;
    /* 设置message的command为104 */
    msg.command = 104;
    /* 设置异步标志*/
    msg.flag   = MESSAGE_ASYNC;

    msg.service_id = 20;

    memset(test_buf, 0x55, IPC_BUF_LEN);

    for (int i = 0; i < 100; i++) {
        msg.req_data    = test_buf;
        msg.req_len     = IPC_BUF_LEN;

        /* 发送 message */
        ipc_message_send(ipc, &msg, AOS_WAIT_FOREVER);
    }

    printf("ipc async done\n");

    return 0;
}

static void cli_ipc_process(ipc_t *ipc, message_t *msg, void *priv)
{
    uint8_t *recv_data;
    uint8_t err = 0;
    switch (msg->command) {

        case 104: {
            recv_data = (uint8_t *)msg->req_data;
            for (int i = 0; i < msg->req_len; i++) {
                if (recv_data[i] != 0x55) {
                    err = 1;
                    break;
                }
            }
            if (err) {
                printf("ipc async error!!!!!!!!!!!!!\r\n");
            } else {
                printf("ipc async ok, msg->req_len %d\r\n", msg->req_len);
            }
            break;
        }
        default :
            break;
    }
}

int ipc_cli_init(uint8_t cpu_id)
{
    /* 获取ipc （cpu_id:0）*/
    ipc = ipc_get(cpu_id);
    
    /* 添加ipc服务（service_id：20）*/
    ipc_add_service(ipc, 20, cli_ipc_process, NULL);

    return 0;
}

