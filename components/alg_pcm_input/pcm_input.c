/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <aos/kernel.h>
#include <ulog/ulog.h>
#include <ipc.h>
#include <csi_core.h>
#include "pcm_input_internal.h"
#include "pcm_input.h"
#include <msp/record.h>

#define TAG "data_input"

#define IPC_CMD_PCM_DATA_SEND        0x11
#define MIC_DATA_IPC_SERIVCE_ID      0xEE
#define PCM_PRIOD_MS 20
#define PCM_RATE     16000
#define PCM_CHN      3
#define PCM_FRAME    16
#define PCM_PRIOD_SIZE ((PCM_RATE / 1000) * PCM_PRIOD_MS * (16 / 8 * PCM_CHN))
#define RINBUF_SIZE  (PCM_FRAME * PCM_PRIOD_SIZE)
typedef void (*mic_input_event_t)(void *priv, int evt, void *data, int size);

#define SHM_ALIGN(addr, align) ((void *)(((uint32_t)(addr) + align - 1U) & (~(uint32_t)(align - 1U))))
#define SHM_ALIGN_SIZE(size, align) (((uint32_t)(size) + align - 1U) & (~(align - 1U)))

static struct pcminput g_pcminput;

typedef struct {
    uint64_t addr;
    int len;
} msg_t;

struct pcminput {
	int format;
	int sample_rate;
	int frame_ms;
	int chn_num;
	int audio_state;
	rec_hdl_t hdl_mic;
	ipc_t *audio_ipc;
    aos_mutex_t mutex;
	mic_input_event_t pfunc_event;
	void *priv;
	uint8_t pcm_data[(PCM_RATE / 1000) * PCM_PRIOD_MS * (16 / 8 * PCM_CHN)] __attribute__((__aligned__(64)));
};

int voice_pcm_acquire(void *data, int len);
int voice_pcm_acquire_init(int bit_format, int sample_rate, int frame_ms, int chn_num);

static void _msg_process(ipc_t *ipc, message_t *msg, void *priv)
{
	struct pcminput *self = (struct pcminput *)priv;

    switch (msg->command) {
        case IPC_CMD_PCM_START: {
            self->audio_state = 1;
        } break;
        case KWS_WEAKUP_EVENT: {
        	if (self->pfunc_event) {
        		self->pfunc_event(self->priv, KWS_WEAKUP_EVENT, (void *)NULL, 0);
        	}
        } break;
        case PCM_DATA_EVENT: {
        	if (self->pfunc_event) {
        		self->pfunc_event(self->priv, PCM_DATA_EVENT, (void *)msg->req_data, msg->req_len);
        	}
        } break;
        default:
            break;
    }

    if (msg->flag & MESSAGE_SYNC) {
        ipc_message_ack(ipc, msg, AOS_WAIT_FOREVER);
    }
}

static void ipc_wait(struct pcminput *self)
{
    //message_t       send_msg;

    //memset(&send_msg, 0, sizeof(message_t));

    //send_msg.command    = IPC_CMD_PCM_START;
    //send_msg.flag       = 1;
    //send_msg.service_id = MIC_DATA_IPC_SERIVCE_ID;
    //send_msg.req_data   = (uint64_t)NULL;
    //send_msg.req_len    = 0;
	//while (!self->audio_state) {
	//    ipc_message_send(self->audio_ipc, &send_msg, 10);
	//    aos_msleep(10);
	//}
    self->audio_state = 1;
}

static int mic_data_ready(void *arg, void *data, size_t rlen)
{
    struct pcminput *self = (struct pcminput *)arg;
    int      capture_byte = 0;

    if (rlen <= 0) {
        return 0;
    }

    memcpy(self->pcm_data, data, rlen);

    if (self->audio_state) {
        epm_ipc_msg_send(IPC_CMD_PCM_DATA_SEND, self->pcm_data, rlen);
    }
    rec_copy_data(0, self->pcm_data, rlen);
    //self->Send(0, output);
    return 0;

}

int epm_ipc_msg_send(int event, void *data, int len)
{
    message_t       send_msg;

    memset(&send_msg, 0, sizeof(message_t));

    send_msg.command    = event;
    send_msg.flag       = MESSAGE_SYNC;
    send_msg.service_id = MIC_DATA_IPC_SERIVCE_ID;
    send_msg.req_data   = (uint64_t)data;
    send_msg.req_len    = len;

    aos_mutex_lock(&g_pcminput.mutex, AOS_WAIT_FOREVER);
	ipc_message_send(g_pcminput.audio_ipc, &send_msg, 10000);
    aos_mutex_unlock(&g_pcminput.mutex);
	return 0;
}

bool DataInput_Init(mic_input_event_t event_cb, void *priv)
{
    struct pcminput *this = &g_pcminput;
    char buf1[64];

    printf("DataInput_Init this->pcm_data:%p\r\n", this->pcm_data);

    this->chn_num = PCM_CHN;
    this->format  = PCM_FRAME;
    this->frame_ms = PCM_PRIOD_MS;
    this->sample_rate = PCM_RATE;
    this->pfunc_event = event_cb;
    this->priv = priv;

    aos_mutex_new(&this->mutex);

    this->audio_ipc = ipc_get(0);
    ipc_add_service(this->audio_ipc, MIC_DATA_IPC_SERIVCE_ID, _msg_process, this);
    ipc_wait(this);
    snprintf(buf1, sizeof(buf1), "mic://format=%u&sample=%u&chan=%u&frame_ms=%u",
                PCM_FRAME, PCM_RATE, PCM_CHN, PCM_PRIOD_MS);

    int frame_size = PCM_RATE/1000*(PCM_FRAME/8)*PCM_CHN*PCM_PRIOD_MS;

    this->hdl_mic = record_register(buf1, "null://");

    record_set_data_ready_cb(this->hdl_mic, mic_data_ready, (void *)this);
    record_set_chunk_size(this->hdl_mic, frame_size);
    record_start(this->hdl_mic);
    return true;
}

