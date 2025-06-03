#define IPC_SER_TEST_AP_FLAG
#ifdef IPC_SER_TEST_AP_FLAG

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <aos/kernel.h>
#include <aos/list.h>
#include <ipc.h>


#define IPC_BUF_LEN (4096*2)

typedef struct {
    ipc_t *ipc;
    char data[IPC_BUF_LEN] __attribute__ ((aligned(64)));
} ipc_test_t;

static ipc_test_t g_test[2];

#define TAG "AP"
static char *s[] = {
    "00000000",
    "11111111",
    "22222222",
    "33333333",
    "44444444",
    "55555555",
    "66666666",
    "77777777",
    "88888888",
    "99999999",
};
/*ipc 同步调用示例*/

int ipc_sync(void)
{
    message_t msg;

    /* 设置message的command为103 */
    msg.command = 103;
    /* 设置同步标志 */
    msg.flag    = MESSAGE_SYNC;
    /* 设置message的service_id为20 */
    msg.service_id = 20;
    msg.resp_data = NULL;
    msg.resp_len = 0;

    /* 设置一次发送数据量 */
    int snd_len = 4096;
    int offset = 0;
    char *send = (char *)s;


    while (offset < strlen(s)) {
        /* 设置message的请求data指针 */
        msg.req_data    = send + offset;

        /* 设置message的请求data的长度 */
        snd_len = 4096 < (strlen(s) - offset)? 4096 : (strlen(s) - offset);
        msg.req_len     = snd_len;
        /* 发送message*/
        ipc_message_send(g_test[0].ipc, &msg, AOS_WAIT_FOREVER);
        /* 发送成功后将文件偏移发送数据量*/
        offset += snd_len;
    }

    printf("ipc sync done\n");
    return 0;
}

// async message demo


/*ipc 异步调用示例*/
int ipc_async(void)
{
    message_t msg;
    memset(&msg, 0, sizeof(message_t));

    /* 设置message的service_id为20 */
    msg.service_id = 20;
    /* 设置message的command为104 */
    msg.command = 104;
    /* 设置异步标志*/
    msg.flag   = MESSAGE_ASYNC | SHM_CACHE;

    msg.service_id = 20;

    for (int i = 0; i < 10; i++) {
        msg.req_data    = s[i%10];
        msg.req_len     = strlen(s[i%10]) + 1;

        /* 发送 message */
        ipc_message_send(g_test[0].ipc, &msg, AOS_WAIT_FOREVER);
    }

    printf("ipc async done\n");

    return 0;
}

static void cli_ipc_process(ipc_t *ipc, message_t *msg, void *priv)
{
    switch (msg->command) {

        case 104: {
#if 1
            /* 异步cmd处理 */
            static int offset = 0;
            /* 比较异步发送的数据是否正确*/
            int ret = memcmp(s[offset % 10], msg->req_data, msg->req_len);

            offset ++;

            if (ret != 0) {
                printf("ipc async err!!!\n");
            }

            if (offset == 100) {
                printf("ipc async ok!!!\n");
                offset = 0;
            }
#endif
            printf("ipc async ok, msg->req_len %d %s\r\n", msg->req_len, (char *)msg->req_data);
            break;
        }

        case 103: {
#if 0
            char *music = (char *)s;
            static int music_len = 0;

            /* 比较同步发送的数据是否正确*/
            int ret = memcmp(music + music_len, msg->req_data, msg->req_len);
            music_len += msg->req_len;

            if (ret != 0) {
                printf("ipc sync err!!!\n");
            }
            if (music_len == strlen(s)) {
                /* 若文件发送完毕则打印成功*/
                printf("music recv ok, total:(%d)\n", music_len);
                music_len = 0;
            }
            /* 回复同步message的ack*/
            ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
#endif
            printf("ipc sync ok!!!\n");
        }
        break;
        default :
            break;
    }
}

int ipc_server_init(void)
{
    ipc_test_t *i = &g_test[0];
    /* 获取ipc （cpu_id:1）, 目的地址的cpu id号*/
    i->ipc = ipc_get(1);

    /* 添加ipc服务（service_id：20）*/
    ipc_add_service(i->ipc, 20, cli_ipc_process, i);

    return 0;
}

#endif
