#include <drv/mbox.h>
#include <semaphore.h>

#include <pthread.h>

static sem_t blyoc_sem_mbox_rx;
static sem_t blyoc_sem_mbox_tx;

static void mailbox_event_cb_fun(csi_mbox_t *mbox, csi_mbox_event_t event, uint32_t channel_id, uint32_t received_len, void *arg)
{
    if (event == MBOX_EVENT_SEND_COMPLETE) {
        sem_post(&blyoc_sem_mbox_tx);
    } else if (event == MBOX_EVENT_RECEIVED) {
        sem_post(&blyoc_sem_mbox_rx);
    } else {
        printf("ERR->Error event handle\n");
    }
}

void blyoc_test_mbox_recv(void *data, uint32_t size)
{
    csi_mbox_t test_mbox2;

    sem_init(&blyoc_sem_mbox_rx, 0, 0);
    csi_mbox_init(&test_mbox2, 0);
    csi_mbox_attach_callback(&test_mbox2, mailbox_event_cb_fun, data);

    sem_wait(&blyoc_sem_mbox_rx);
    csi_mbox_receive(&test_mbox2, 0, data, size);

    csi_mbox_detach_callback(&test_mbox2);
    csi_mbox_uninit(&test_mbox2);
    sem_destroy(&blyoc_sem_mbox_rx);

    printf(" mbox-recv successfully\n");

    return;
}

void blyoc_test_mbox_send(void *data, uint32_t size)
{
    csi_mbox_t test_mbox1;

    sem_init(&blyoc_sem_mbox_tx, 0, 0);
    csi_mbox_init(&test_mbox1, 0);
    csi_mbox_attach_callback(&test_mbox1, mailbox_event_cb_fun, NULL);

    csi_mbox_send(&test_mbox1, 0, data, size);
    sem_wait(&blyoc_sem_mbox_tx);

    printf(" send ok\n");

    csi_mbox_detach_callback(&test_mbox1);
    csi_mbox_uninit(&test_mbox1);
    sem_destroy(&blyoc_sem_mbox_tx);

    printf(" mbox-send successfully\n");

    return;
}


void test_ipc_recv()
{
    long long  time;
    uint32_t mbox_recv_data[16];

    printf("recv start\n ");

    blyoc_test_mbox_recv(mbox_recv_data, sizeof(mbox_recv_data));          // 接收ipc的数据地址

    time = bl_os_clock_gettime_ms();
    printf("tset recv is ok %d %d %lld\r\n", mbox_recv_data[0], mbox_recv_data[1], time);
}

void test_ipc_send()
{
    long long  time;
    uint32_t mbox_send_data[16];

    mbox_send_data[0] = 0x1;
    mbox_send_data[1] = 0x3;

    printf("send start\n ");

    blyoc_test_mbox_send(mbox_send_data, sizeof(mbox_send_data));

    time = bl_os_clock_gettime_ms();
    printf("tset send is ok %d %d %lld\r\n", mbox_send_data[0], mbox_send_data[1], time);
}
#if 0
void test_ipc_send_task()
{
    pthread_t tset_ipc_test;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    pthread_create(&tset_ipc_test, &attr, test_ipc_send, NULL);

    pthread_join(tset_ipc_test, NULL);
}

void test_ipc_recv_task()
{
    pthread_t tset_ipc_test;
    pthread_attr_t attr;

    pthread_attr_init(&attr);

    pthread_create(&tset_ipc_test, &attr, test_ipc_recv, NULL);

    pthread_join(tset_ipc_test, NULL);
}

#endif



