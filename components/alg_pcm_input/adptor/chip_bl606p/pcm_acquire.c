/*
 * Copyright (C) 2019-2022 Alibaba Group Holding Limited
 */
#include <ulog/ulog.h>
#include <alsa/pcm.h>

#include "../../pcm_input_internal.h"

#define TAG "AACQ"

extern long long msp_now_ms(void);

/* Debug: If equal to 1,pcm simulation mode, at startup time at ignore alsa capture data */
extern int g_pcminput_ignore_alsa;

static msp_pcm_t *pcmC0_    = NULL;
static msp_pcm_t *capture_init(const char *devname, unsigned int sample_rate /*16000*/, int chn_num,
                               int bit_format /*16bit*/, msp_pcm_uframes_t peroid_size)
{
    msp_pcm_hw_params_t *params;
    msp_pcm_t           *pcm = NULL;

    int err = 0;

    err = msp_pcm_open(&pcm, devname, MSP_PCM_STREAM_CAPTURE, 0);

    if (err < 0) {
        LOGE(TAG, "msp_pcm_open %s error", devname);
        return NULL;
    }

    msp_pcm_hw_params_alloca(&params);
    err = msp_pcm_hw_params_any(pcm, params);

    if (err < 0) {
        LOGE(TAG, "Broken configuration for this PCM: no configurations available");
        msp_pcm_close(pcm);
        return NULL;
    }

    err = msp_pcm_hw_params_set_access(pcm, params, MSP_PCM_ACCESS_RW_NONINTERLEAVED);

    if (err < 0) {
        LOGE(TAG, "Access type not available");
        msp_pcm_close(pcm);
        return NULL;
    }

    err = msp_pcm_hw_params_set_format(pcm, params, bit_format);

    if (err < 0) {
        LOGE(TAG, "Sample bit_format non available");
        msp_pcm_close(pcm);
        return NULL;
    }

    err = msp_pcm_hw_params_set_channels(pcm, params, chn_num);

    if (err < 0) {
        LOGE(TAG, "Channels count non available");
        msp_pcm_close(pcm);
        return NULL;
    }

    msp_pcm_hw_params_set_rate_near(pcm, params, &sample_rate, 0);

    msp_pcm_uframes_t val_peroid_size = peroid_size;
    msp_pcm_hw_params_set_period_size_near(pcm, params, &val_peroid_size, 0);

    msp_pcm_uframes_t val_buffer_frames = val_peroid_size * 23; /*buffer保存16个frame*/
    msp_pcm_hw_params_set_buffer_size_near(pcm, params, &val_buffer_frames);

    err = msp_pcm_hw_params(pcm, params);

    if (err < 0) {
        LOGE(TAG, "msp_pcm_hw_params error");
        msp_pcm_close(pcm);
        return NULL;
    }

    return pcm;
}

static int bl606p_voice_pcm_acquire_init(int bit_format, int sample_rate, int frame_ms, int chn_num)
{
    /* Single frame single channel sample count */
    msp_pcm_uframes_t peroid_size = frame_ms * (sample_rate / 1000);

    pcmC0_ = capture_init("pcmC0", sample_rate, chn_num, bit_format, peroid_size);

    if (pcmC0_ == NULL) {
        return -1;
    }

    ssize_t capture_byte = msp_pcm_frames_to_bytes(pcmC0_, peroid_size);

    return (int)capture_byte;
}

/**
 * @brief  capture audio data from alsa
 *
 * @param  [out] data : 3-channel interleaved audio (mic1,mic2,ref)
 * @param  [int] len : data buffer byte length
 * @return <0 failed, >=0 byte length of capture data
 */
static int bl606p_voice_pcm_acquire(void *data, int len)
{
    int rlen = 0;
    int ret = 0;

    if (pcmC0_ == NULL) {
        return 0;
    }

#if 0
    ret = msp_pcm_wait(pcmC0_, MSP_WAIT_FOREVER);
#else
    while (msp_pcm_avail(pcmC0_) < msp_pcm_bytes_to_frames(pcmC0_, len)) {
        ret = msp_pcm_wait(pcmC0_, MSP_WAIT_FOREVER);//
    }
#endif

    if (ret < 0) {
        // msp_pcm_recover(g_pcm, ret, 1);
        static long long last_time = 0;

        long long now = msp_now_ms();
        if (now - last_time > 3000) {
            LOGW(TAG, "pcm read XRUN\r\n");
            last_time = now;
        }
        return 0;
    }

    rlen = msp_pcm_readn(pcmC0_, (void**)data, msp_pcm_bytes_to_frames(pcmC0_, len));
    rlen = msp_pcm_frames_to_bytes(pcmC0_, rlen);

    /* pcm push hook, overwrite capture data */
    if (g_pcminput_ignore_alsa == 0) {
        voice_pcm_http_rewrite(data, rlen);
    } else {
        /* no alsa data to alg when dev startup */
        int hook_ret = voice_pcm_http_rewrite(data, rlen);

        if (hook_ret <= 0) {
            rlen = hook_ret;
        }
    }
    return rlen;
}

pcm_acquire_ops_t g_pcm_acquire_ops = {
    .init    = bl606p_voice_pcm_acquire_init,
    .acquire = bl606p_voice_pcm_acquire,
};
