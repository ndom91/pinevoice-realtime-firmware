/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <aos/aos.h>
#include <aos/kernel.h>
#include <aos/kv.h>
#include <ulog/ulog.h>

#include <yoc/mic.h>
#include <yoc/mic_port.h>

#include "voice_mind.h"
#include "dispatch_process.h"
#include "dispatch_ringbuf.h"
#include "pcm_input.h"

#define TAG "VioceMinD"

#define MIN(x, y) ((x) > (y) ? (y) : (x))

#define PCM_RATE 16000
#define PCM_CHANNELS 3
#define PCM_FRAME_MS 10
#define PCM_SAMPLE_BYTES 2
#define PCM_MONO_FRAME_SIZE ((PCM_RATE / 1000) * PCM_SAMPLE_BYTES * PCM_FRAME_MS)
#define PCM_INPUT_FRAME_SIZE (PCM_MONO_FRAME_SIZE * PCM_CHANNELS)
#define PCM_RINGBUF_LEN      (PCM_INPUT_FRAME_SIZE * 150)

// How the capture stream is turned into the mono frame the realtime client
// sends. The right answer depends on what the driver actually delivers, which
// is why this is tunable from the console instead of compiled in:
//
//   kv set    va_mic_mode ch0   (mono | ch0 | ch1 | avg | diff)
//   kv setint va_mic_gain 4     (linear multiplier, 1..64)
//   reboot
//
// "mono" reads a single 320-byte frame and passes it through untouched — the
// behaviour from before the de-interleaving change, and a known-usable
// baseline. Every other mode reads a 3-channel frame and picks or combines
// the two microphone channels. "diff" exists to test whether the pair is
// wired anti-phase, in which case "avg" cancels the voice instead of
// reinforcing it.
#define MIC_KV_MODE_KEY "va_mic_mode"
#define MIC_KV_GAIN_KEY "va_mic_gain"
#define MIC_KV_DGAIN_KEY "va_mic_dgain"

// Codec digital capture gain, in dB. The microphones are PDM, so the board's
// analog gain never reaches them and this is the only stage that changes the
// captured level. It cannot be applied from board_audio_init(): bringing the
// ADC up for capture resets the register, so it is written from the capture
// task once PCM is flowing, and re-asserted whenever the value changes.
#define MIC_DGAIN_DEFAULT 40
#define MIC_DGAIN_MIN     0
#define MIC_DGAIN_MAX     60

extern int board_audio_in_set_digital_gain(int id, int gain);

// Defaults to unity because capture level is now corrected where it belongs,
// in the codec's digital stage (AUIDO_IN_GAIN_MIC_DIGITAL). This multiply is
// kept as a tuning knob, but it scales the noise floor along with the signal
// and so buys headroom rather than signal-to-noise: prefer the codec gain.
#define MIC_GAIN_DEFAULT 1
#define MIC_GAIN_MAX     64
// Re-read KV roughly every two seconds so a console change applies without a
// reboot, while staying far off the 10 ms frame path.
#define MIC_RELOAD_FRAMES 200

typedef enum {
    MIC_MODE_MONO = 0,
    MIC_MODE_CH0,
    MIC_MODE_CH1,
    MIC_MODE_AVG,
    MIC_MODE_DIFF,
} mic_mode_t;

// Measured on hardware: reading the stream flat as mono yields clean speech at
// realtime, while the de-interleaving modes deliver ~40% of realtime because
// they consume three frames per frame emitted. The driver hands us a single
// channel, so PCM_CHANNELS does not describe this source and the other modes
// are kept only as diagnostics.
#define MIC_MODE_DEFAULT MIC_MODE_MONO

static mic_mode_t g_mic_mode = MIC_MODE_DEFAULT;
static int g_mic_gain = MIC_GAIN_DEFAULT;
static int g_mic_dgain = MIC_DGAIN_DEFAULT;
// The codec register is only writable once capture is up, so reloads before
// that point record the value and leave applying it to the capture task.
static bool g_mic_capture_live;

static const char *mic_mode_name(mic_mode_t mode)
{
    switch (mode) {
    case MIC_MODE_MONO: return "mono";
    case MIC_MODE_CH0:  return "ch0";
    case MIC_MODE_CH1:  return "ch1";
    case MIC_MODE_DIFF: return "diff";
    case MIC_MODE_AVG:
    default:            return "avg";
    }
}

static mic_mode_t mic_mode_parse(const char *name)
{
    if (strcmp(name, "mono") == 0) return MIC_MODE_MONO;
    if (strcmp(name, "ch0") == 0)  return MIC_MODE_CH0;
    if (strcmp(name, "ch1") == 0)  return MIC_MODE_CH1;
    if (strcmp(name, "diff") == 0) return MIC_MODE_DIFF;
    if (strcmp(name, "avg") == 0)  return MIC_MODE_AVG;
    return MIC_MODE_DEFAULT;
}

static void mic_capture_reload(void)
{
    char name[16];
    mic_mode_t mode = MIC_MODE_DEFAULT;
    int gain = MIC_GAIN_DEFAULT;

    memset(name, 0, sizeof(name));
    if (aos_kv_getstring(MIC_KV_MODE_KEY, name, sizeof(name)) >= 0 && name[0] != '\0') {
        mode = mic_mode_parse(name);
    }
    // An out-of-range multiplier means an unusable KV entry, not a request to
    // mute or to clip everything, so fall back rather than honour it.
    if (aos_kv_getint(MIC_KV_GAIN_KEY, &gain) != 0 || gain < 1 || gain > MIC_GAIN_MAX) {
        gain = MIC_GAIN_DEFAULT;
    }

    int dgain = MIC_DGAIN_DEFAULT;
    if (aos_kv_getint(MIC_KV_DGAIN_KEY, &dgain) != 0
        || dgain < MIC_DGAIN_MIN || dgain > MIC_DGAIN_MAX) {
        dgain = MIC_DGAIN_DEFAULT;
    }

    if (dgain != g_mic_dgain) {
        g_mic_dgain = dgain;
        if (g_mic_capture_live) {
            board_audio_in_set_digital_gain(0, dgain);
        }
        LOGI(TAG, "capture dgain=%ddb", dgain);
    }

    if (mode == g_mic_mode && gain == g_mic_gain) {
        return;
    }
    g_mic_mode = mode;
    g_mic_gain = gain;
    LOGI(TAG, "capture mode=%s gain=x%d", mic_mode_name(mode), gain);
}

// Called once PCM is flowing. Writing the codec gain any earlier is lost when
// the ADC is brought up for capture.
static void mic_capture_apply_dgain(void)
{
    g_mic_capture_live = true;
    board_audio_in_set_digital_gain(0, g_mic_dgain);
    LOGI(TAG, "capture dgain=%ddb applied", g_mic_dgain);
}

static inline int16_t mic_scale(int32_t value, int gain)
{
    value *= gain;
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return (int16_t)value;
}

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
		// TODO: gamiee, g_voice_priv.state = VOICE_STATE_BUSY;
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
    int16_t *pcm_data = (int16_t *)aos_malloc_check(PCM_INPUT_FRAME_SIZE);
    int16_t *mono_data = (int16_t *)aos_malloc_check(PCM_MONO_FRAME_SIZE);
    int data_size = 0;
    int frames_since_reload = 0;

    mic_capture_reload();

    printf("DataInput_Init\r\n");
    DataInput_Init(_mic_input_event);

    aos_sem_wait(&g_voice_priv.pcm_sem, AOS_WAIT_FOREVER);

    mic_capture_apply_dgain();

    while (g_voice_priv.task_running) {

        if (++frames_since_reload >= MIC_RELOAD_FRAMES) {
            frames_since_reload = 0;
            mic_capture_reload();
        }

        // "mono" consumes one frame per frame emitted; the de-interleaving
        // modes consume one frame per channel. Reading the wrong amount here
        // is what starves the stream to a fraction of realtime.
        mic_mode_t mode = g_mic_mode;
        int gain = g_mic_gain;
        int read_size = (mode == MIC_MODE_MONO) ? PCM_MONO_FRAME_SIZE : PCM_INPUT_FRAME_SIZE;

        if (dispatch_ringbuffer_available_read_size(TYPE_PCM) < read_size) {
            aos_msleep(10);
            continue;

        }
        if ((data_size = voice_get_pcm_data(pcm_data, read_size)) == read_size) {
            for (int i = 0; i < PCM_MONO_FRAME_SIZE / PCM_SAMPLE_BYTES; i++) {
                int32_t sample;

                switch (mode) {
                case MIC_MODE_MONO:
                    sample = pcm_data[i];
                    break;
                case MIC_MODE_CH0:
                    sample = pcm_data[i * PCM_CHANNELS];
                    break;
                case MIC_MODE_CH1:
                    sample = pcm_data[i * PCM_CHANNELS + 1];
                    break;
                case MIC_MODE_DIFF:
                    sample = ((int32_t)pcm_data[i * PCM_CHANNELS]
                              - pcm_data[i * PCM_CHANNELS + 1]) / 2;
                    break;
                case MIC_MODE_AVG:
                default:
                    sample = ((int32_t)pcm_data[i * PCM_CHANNELS]
                              + pcm_data[i * PCM_CHANNELS + 1]) / 2;
                    break;
                }
                mono_data[i] = mic_scale(sample, gain);
            }
            g_voice_priv.event_cb(
                g_voice_priv.mic, MIC_EVENT_PCM_DATA, mono_data, PCM_MONO_FRAME_SIZE
            );
        } else {
            aos_msleep(10);
        }
    }

    g_voice_priv.task_exit = 1;
    if (g_voice_priv.kws_data) {
        aos_free(g_voice_priv.kws_data);
    }
    aos_free(pcm_data);
    aos_free(mono_data);
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
