/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <ulog/ulog.h>

#include <yoc/mic.h>
#include <yoc/mic_port.h>

#include "voice_mind.h"
#include "dispatch_process.h"
#include "dispatch_ringbuf.h"
#include "pcm_input.h"

#define TAG "VioceMinD"

#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define FRAME_SIZE ((16000 / 1000) * (16 / 8) * 10) /* 320 */
#define PCM_RINGBUF_LEN      (FRAME_SIZE * 150)

static voice_t g_voice_priv;
extern bool DataInput_Init();
#define DEBUG_DUMP_PCM_DATA 0
#if DEBUG_DUMP_PCM_DATA
volatile uint8_t g_asr_pcm_dump[200*1024];
volatile uint32_t g_asr_pcm_len = 0;
volatile uint8_t g_pcm_debug_flag = 0;
#endif

static void _mic_input_event(void *priv, int evt, void *data, int size)
{
	mic_kws_t      result    = { MIC_WAKEUP_TYPE_NONE, 0, 0, 0, "tian mao jing ling"};

	if (evt == KWS_WEAKUP_EVENT) {
		g_voice_priv.state = VOICE_STATE_BUSY;
		g_voice_priv.event_cb(g_voice_priv.mic, MIC_EVENT_SESSION_START, (void *)&result, sizeof(mic_kws_t));
#if DEBUG_DUMP_PCM_DATA
		g_asr_pcm_len = 0;
#endif
	} else if (evt == PCM_DATA_EVENT) {
        if (g_voice_priv.state == VOICE_STATE_BUSY) {
            dispatch_ringbuffer_create(TYPE_PCM, PCM_RINGBUF_LEN);
            dispatch_ringbuffer_write(TYPE_PCM, (uint8_t *)data, size);
        }
        rec_copy_data(1, data, size);
#if DEBUG_DUMP_PCM_DATA
		if (g_pcm_debug_flag && (g_asr_pcm_len + size < sizeof(g_asr_pcm_dump))) {
            memcpy(&g_asr_pcm_dump[g_asr_pcm_len], data, size);
            aos_log_hexdump("PCM", &g_asr_pcm_dump[g_asr_pcm_len], 32);
            g_asr_pcm_len += size;
        }
#endif
	} else if (evt == SESSION_STOP_EVENT) {
		g_voice_priv.state = VOICE_STATE_IDLE;
		g_voice_priv.event_cb(g_voice_priv.mic, MIC_EVENT_SESSION_STOP, (void *)&result, sizeof(mic_kws_t));
	}
}


static void plugin_task_entry(void *arg)
{
    char *pcm_data = (char *)aos_malloc_check(FRAME_SIZE);
    int data_size = 0;

    printf("DataInput_Init\r\n");
    DataInput_Init(_mic_input_event);

    aos_sem_wait(&g_voice_priv.pcm_sem, AOS_WAIT_FOREVER);
    
    while (g_voice_priv.task_running) {

        if (dispatch_ringbuffer_available_read_size(TYPE_PCM) < FRAME_SIZE) {
            aos_msleep(10);
            continue;

        }
        if ((data_size = voice_get_pcm_data(pcm_data, FRAME_SIZE)) > 0) {
            g_voice_priv.event_cb(g_voice_priv.mic, MIC_EVENT_PCM_DATA, pcm_data, data_size);
        } else {
            aos_msleep(10);
        }
    }

    g_voice_priv.task_exit = 1;
    if (g_voice_priv.kws_data) {
        aos_free(g_voice_priv.kws_data);
    }
    aos_free(pcm_data);
    aos_task_exit(0);
}

static int mic_adaptor_init(mic_t *mic, mic_event_t event)
{
    g_voice_priv.event_cb = event; //aui_mic_imp.c:mic_event_hdl

    g_voice_priv.mic = mic;

    int ret = aos_sem_new(&g_voice_priv.pcm_sem, 0);
    if (ret < 0) {
        return -1;
    }

    return 0;
}

static int mic_adaptor_deinit(mic_t *mic)
{
    aos_check_return_einval(!g_voice_priv.task_running);

    aos_sem_free(&g_voice_priv.pcm_sem);

    return 0;
}

static int mic_adaptor_start(mic_t *mic)
{
    aos_check_return_einval(!g_voice_priv.task_running);

    g_voice_priv.task_running = 1;
    g_voice_priv.task_exit    = 0;

    aos_task_new_ext(&g_voice_priv.plugin_task, "voice_mind", &plugin_task_entry, NULL, 1024 * 8, 11);

    return 0;
}

static int mic_adaptor_stop(mic_t *mic)
{
    aos_check_return_einval(g_voice_priv.task_running);

    g_voice_priv.task_running = 0;

    aos_sem_signal(&g_voice_priv.pcm_sem);

    while (!g_voice_priv.task_exit) {
        aos_msleep(20);
    };

    return 0;
}

static int mic_adaptor_pcm_data_control(mic_t *mic, int enable)
{
    LOGD(TAG, "pcm_control_update enable %d", enable);
    if (enable) {
		g_voice_priv.state = VOICE_STATE_BUSY;
        aos_sem_signal(&g_voice_priv.pcm_sem);
        epm_ipc_msg_send(SESSION_START_EVENT, NULL, 0);
    } else {
		g_voice_priv.state = VOICE_STATE_IDLE;
        epm_ipc_msg_send(SESSION_STOP_EVENT, NULL, 0);
    }

    return 0;
}

static int mic_adaptor_set_push2talk(mic_t *mic, int mode)
{
    printf("%s\r\n", __func__);
    return 0;
}

static int mic_adaptor_wakeup_notify_play_status(mic_t *mic, int play_status, int timeout)
{
    printf("%s\r\n", __func__);
    return 0;
}

static int mic_adaptor_set_wakup_level(mic_t *mic, char *wakeup_word, int level)
{
    printf("%s\r\n", __func__);
    return 0;
}

static int mic_adaptor_start_doa(mic_t *mic)
{
	printf("%s\r\n", __func__);
    return 0;
}

static int mic_adaptor_enable_linear_aec_data(mic_t *mic, int enable)
{
    printf("%s\r\n", __func__);
    return 0;
}

static mic_ops_t voice_ops = {
    .init   = mic_adaptor_init,
    .deinit = mic_adaptor_deinit,

    .start = mic_adaptor_start,
    .stop  = mic_adaptor_stop,

    .pcm_data_control = mic_adaptor_pcm_data_control,

    .set_push2talk      = mic_adaptor_set_push2talk,
    .notify_play_status = mic_adaptor_wakeup_notify_play_status,
    .set_wakeup_level   = mic_adaptor_set_wakup_level,
    .start_doa          = mic_adaptor_start_doa,
    .enable_linear_aec_data = mic_adaptor_enable_linear_aec_data,
};

void aui_mic_register(void)
{
    mic_ops_register(&voice_ops);
}
