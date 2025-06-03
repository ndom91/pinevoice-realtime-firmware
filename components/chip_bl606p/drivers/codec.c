#include <drv/codec.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>

#include <soc.h>
#include <bl_os_hal.h>
#include <blyoc_codec/blyoc_codec.h>
#include <blyoc_audio/blyoc_audio.h>
#include <blyoc_audio/bl_audio_output.h>
#include <blyoc_audio/bl_audio_input.h>
#ifdef AOS_COMP_ULOG
#include <ulog/ulog.h>
#else
#define LOGD(...)
#endif

#if 0
#if TG6210A_CONFIG_DEBUG
#define user_log(M, ...) do {  printf("[%9ld]======[%s:%d] " M "",  \
                                 bl_os_clock_gettime_ms(),          \
                                 __func__,                       \
                                 __LINE__,                       \
                                 ##__VA_ARGS__);                 \
                               } while(0==1)
#else
#define user_log(M, ...)
#endif
#else
#define TAG                "codec"
#define user_log(M, ...)
#endif

/*                            function                              */
csi_error_t csi_codec_init(csi_codec_t *codec, uint32_t idx);

csi_error_t csi_codec_input_open(csi_codec_t *codec, csi_codec_input_t *ch, uint32_t ch_idx);
csi_error_t csi_codec_input_config(csi_codec_input_t *ch, csi_codec_input_config_t *config);
csi_error_t csi_codec_input_attach_callback(csi_codec_input_t *ch, void *callback, void *arg);
csi_error_t csi_codec_input_link_dma(csi_codec_input_t *ch, csi_dma_ch_t *dma);
csi_error_t csi_codec_input_start(csi_codec_input_t *ch);
uint32_t csi_codec_input_read(csi_codec_input_t *ch, void *data, uint32_t size);
uint32_t csi_codec_input_read_async(csi_codec_input_t *ch, void *data, uint32_t size);
void csi_codec_input_detach_callback(csi_codec_input_t *ch);
void csi_codec_input_stop(csi_codec_input_t *ch);
void csi_codec_input_close(csi_codec_input_t *ch);

uint32_t csi_codec_input_buffer_avail(csi_codec_input_t *ch);
uint32_t csi_codec_input_buffer_remain(csi_codec_input_t *ch);
csi_error_t csi_codec_input_buffer_reset(csi_codec_input_t *ch);
csi_error_t csi_codec_input_mute(csi_codec_input_t *ch, bool enable);
csi_error_t csi_codec_input_mix_gain(csi_codec_input_t *ch, uint32_t val);
csi_error_t csi_codec_input_digital_gain(csi_codec_input_t *ch, uint32_t val);
csi_error_t csi_codec_input_analog_gain(csi_codec_input_t *ch, uint32_t val);

csi_error_t csi_codec_output_open(csi_codec_t *codec, csi_codec_output_t *ch, uint32_t ch_idx);
csi_error_t csi_codec_output_config(csi_codec_output_t *ch, csi_codec_output_config_t *config);
csi_error_t csi_codec_output_attach_callback(csi_codec_output_t *ch, void *callback, void *arg);
csi_error_t csi_codec_output_start(csi_codec_output_t *ch);
csi_error_t csi_codec_output_link_dma(csi_codec_output_t *ch, csi_dma_ch_t *dma);
uint32_t csi_codec_output_write(csi_codec_output_t *ch, const void *data, uint32_t size);
uint32_t csi_codec_output_write_async(csi_codec_output_t *ch, const void *data, uint32_t size);
void csi_codec_output_stop(csi_codec_output_t *ch);
void csi_codec_output_detach_callback(csi_codec_output_t *ch);
void csi_codec_output_close(csi_codec_output_t *ch);


csi_error_t csi_codec_output_pause(csi_codec_output_t *ch);
csi_error_t csi_codec_output_resume(csi_codec_output_t *ch);
uint32_t csi_codec_output_buffer_avail(csi_codec_output_t *ch);
uint32_t csi_codec_output_buffer_remain(csi_codec_output_t *ch);
csi_error_t csi_codec_output_buffer_reset(csi_codec_output_t *ch);
csi_error_t csi_codec_output_mute(csi_codec_output_t *ch, bool enable);
csi_error_t csi_codec_output_digital_gain(csi_codec_output_t *ch, uint32_t val);
csi_error_t csi_codec_output_analog_gain(csi_codec_output_t *ch, uint32_t val);
csi_error_t csi_codec_output_mix_gain(csi_codec_output_t *ch, uint32_t val);
csi_error_t csi_codec_output_get_state(csi_codec_output_t *ch, csi_state_t *state);

void csi_codec_uninit(csi_codec_t *codec);

#define CSI_DMA_OUTPUT_NODE_NUM        (16)
#define CSI_DMA_OUTPUT_PER_NODE_SIZE   (1024)
#define CSI_DMA_INPUT_NODE_NUM         (11)
#define CSI_DMA_INPUT_PER_NODE_SIZE    (1920)
//#define CSI_DMA_INPUT_PER_NODE_SIZE    (1536)

static aui_ch_t rx_context;

csi_error_t csi_codec_init(csi_codec_t *codec, uint32_t idx)
{
    if(idx != 0)
    {
        return CSI_ERROR;
    }
    uint32_t pa_pin = codec->output_chs->pa_pin;
    audio_poweron(pa_pin);

    return CSI_OK;
}
csi_error_t csi_codec_input_open(csi_codec_t *codec, csi_codec_input_t *ch, uint32_t ch_idx)
{
    int ret = -1;

    if(ch_idx != 0)
    {
        return CSI_ERROR;
    }

    rx_context.ch_idx = ch_idx;
    rx_context.ringbuffer = (blyoc_ringbuf_t *)ch->ring_buf;
    rx_context.dma = bl_os_malloc(sizeof(aui_dma_t));
    memset(rx_context.dma, 0, sizeof(aui_dma_t));

    rx_context.dma->id = DMA0_ID;
    rx_context.dma->ch = DMA_CH1;
    rx_context.dma->maxcount = CSI_DMA_INPUT_NODE_NUM;
    rx_context.dma->per_node_size = CSI_DMA_INPUT_PER_NODE_SIZE;

    codec->input_chs      = ch;
    ch->codec             = codec;
    ch->ch_idx            = ch_idx;
    ch->state.error       = 0U;
    ch->state.readable    = 0U;
    ch->state.writeable   = 0U;

    ret = aui_init(&rx_context);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

csi_error_t csi_codec_input_config(csi_codec_input_t *ch, csi_codec_input_config_t *config)
{
    int ret = -1;
    aui_cfg_t cfg;

    cfg.sample_rate = config->sample_rate;
    cfg.bit_width = config->bit_width;
    cfg.buffer = config->buffer;
    cfg.buffer_size = config->buffer_size;
    cfg.sound_channel_num = config->sound_channel_num;

    ch->ring_buf->buffer  = config->buffer;
    ch->ring_buf->size    = config->buffer_size;
    ch->period            = config->period;
    ch->sound_channel_num = config->sound_channel_num;

    ret = aui_channel_config(&rx_context, &cfg);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

void csi_rx_user_cb(aui_ch_t *context, audio_codec_event_t event, void *arg)
{
    static uint32_t r_node_num = 0;
    csi_codec_input_t *ch = (csi_codec_input_t *)arg;

    if (event == BL_EVENT_NODE_READ_COMPLETE) {
        r_node_num++;

        if (r_node_num * CSI_DMA_INPUT_PER_NODE_SIZE >= ch->period) {
            if (ch->callback) {
                ch->callback(ch, CODEC_EVENT_PERIOD_READ_COMPLETE, ch->arg);
            }
            r_node_num = 0;
        }
    } else if (event == BL_EVENT_READ_BUFFER_FULL) {
        if (ch->callback) {
            ch->callback(ch, CODEC_EVENT_READ_BUFFER_FULL, ch->arg);
        }
    } else {
        printf("rx event not support event = %d\r\n", event);
    }
}

csi_error_t csi_codec_input_attach_callback(csi_codec_input_t *ch, void *callback, void *arg)
{
    ch->callback = callback;
    ch->arg = arg;

    aui_attach_callback(&rx_context, &csi_rx_user_cb, ch);

    return CSI_OK;
}


csi_error_t csi_codec_input_link_dma(csi_codec_input_t *ch, csi_dma_ch_t *dma)
{
    int ret = -1;

    ret = aui_rx_dma_link(&rx_context, dma);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

csi_error_t csi_codec_input_start(csi_codec_input_t *ch)
{

#if 1
    int ret = -1;

    ret = aui_start(&rx_context);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
#endif
    return CSI_OK;
}


uint32_t csi_codec_input_read(csi_codec_input_t *ch, void *data, uint32_t size)
{
    return aui_read(&rx_context, data, size);
}

uint32_t csi_codec_input_read_async(csi_codec_input_t *ch, void *data, uint32_t size)
{
    return aui_read(&rx_context, data, size);
}


void csi_codec_input_stop(csi_codec_input_t *ch)
{
    aui_stop(&rx_context);
}

void csi_codec_input_detach_callback(csi_codec_input_t *ch)
{
    ch->callback = NULL;
    ch->arg = NULL;

    return ;
}

void csi_codec_input_close(csi_codec_input_t *ch)
{
    if (rx_context.dma) {
        bl_os_free(rx_context.dma);
        rx_context.dma = NULL;
    }
    return ;
}

uint32_t csi_codec_input_buffer_avail(csi_codec_input_t *ch)
{
    return aui_buffer_avail(&rx_context);
}
uint32_t csi_codec_input_buffer_remain(csi_codec_input_t *ch)
{
    return aui_buffer_remain(&rx_context);
}

csi_error_t csi_codec_input_buffer_reset(csi_codec_input_t *ch)
{
    // bl606p unnecessary
    return CSI_OK;
}

csi_error_t csi_codec_input_mute(csi_codec_input_t *ch, bool enable)
{
    //blyoc_audio_input_set_mute(ch, enable);
    //aui_mute(&rx_context, enable);
    mic_mute(0, enable);

    return CSI_OK;
}

csi_error_t csi_codec_input_mix_gain(csi_codec_input_t *ch, uint32_t val)
{
    //blyoc_audio_input_set_mix_gain(ch, 0, val);
    //bl606p not support

    return CSI_OK;
}
csi_error_t csi_codec_input_digital_gain(csi_codec_input_t *ch, uint32_t val)
{
    //blyoc_audio_input_digital_gain(ch, 1, val);
    //aui_digital_gain(&rx_context, val);// db
    //aui_digital_gain(&rx_context, 0);// float db

    return CSI_OK;
}
csi_error_t csi_codec_input_analog_gain(csi_codec_input_t *ch, uint32_t val)
{
    //blyoc_audio_input_analog_gain(ch, val);
    //aui_analog_gain(&rx_context, val);// db

    return CSI_OK;
}


static auo_ch_t tx_context;

csi_error_t csi_codec_output_open(csi_codec_t *codec, csi_codec_output_t *ch, uint32_t ch_idx)
{
    int ret = -1;

    tx_context.ch_idx = ch_idx;
    tx_context.ringbuffer = (blyoc_ringbuf_t *)ch->ring_buf;
    tx_context.dma = bl_os_malloc(sizeof(auo_dma_t));
    memset(tx_context.dma, 0, sizeof(auo_dma_t));

    tx_context.dma->id = DMA0_ID;
    tx_context.dma->ch = DMA_CH0;
    tx_context.dma->maxcount = CSI_DMA_OUTPUT_NODE_NUM;
    tx_context.dma->per_node_size = CSI_DMA_OUTPUT_PER_NODE_SIZE;

    codec->output_chs     = ch;
    ch->codec             = codec;
    ch->ch_idx            = ch_idx;
    ch->state.error       = 0U;
    ch->state.readable    = 0U;
    ch->state.writeable   = 0U;

    ret = auo_init(&tx_context);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

csi_error_t csi_codec_output_config(csi_codec_output_t *ch, csi_codec_output_config_t *config)
{
    int ret = -1;
    auo_cfg_t cfg;

    if (config->sample_rate%8000) {
        return CSI_ERROR;
    }
    cfg.sample_rate = config->sample_rate;
    cfg.bit_width = config->bit_width;
    cfg.buffer = config->buffer;
    cfg.buffer_size = config->buffer_size;
    cfg.sound_channel_num = config->sound_channel_num;

    ch->ring_buf->buffer  = config->buffer;
    ch->ring_buf->size    = config->buffer_size;
    ch->period            = config->period;
    ch->sound_channel_num = config->sound_channel_num;

    ret = auo_channel_config(&tx_context, &cfg);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

void csi_tx_user_cb(auo_ch_t *tx_context, audio_codec_event_t event, void *arg)
{
    static uint32_t w_node_num = 0;
    csi_codec_output_t *ch = (csi_codec_output_t *)arg;

    if (event == BL_EVENT_NODE_WRITE_COMPLETE) {
        w_node_num++;

        if (w_node_num * CSI_DMA_OUTPUT_PER_NODE_SIZE >= ch->period) {
            if (ch->callback) {
                ch->callback(ch, CODEC_EVENT_PERIOD_WRITE_COMPLETE, ch->arg);
            }
            w_node_num = 0;
        }
    } else if (event == BL_EVENT_WRITE_BUFFER_EMPTY) {
        if (ch->callback) {
            ch->callback(ch, CODEC_EVENT_WRITE_BUFFER_EMPTY, ch->arg);
        }
    } else {
        printf("event not support\r\n");
    }
}

csi_error_t csi_codec_output_attach_callback(csi_codec_output_t *ch, void *callback, void *arg)
{
    ch->callback = callback;
    ch->arg = arg;

    auo_attach_callback(&tx_context, &csi_tx_user_cb, ch);

    return CSI_OK;
}

csi_error_t csi_codec_output_link_dma(csi_codec_output_t *ch, csi_dma_ch_t *dma)
{
    int ret = -1;

    ret = auo_tx_dma_link(&tx_context, dma);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
}

csi_error_t csi_codec_output_start(csi_codec_output_t *ch)
{
#if 0
    int ret = -1;

    ret = auo_start(&tx_context);

    if (ret) {
        return CSI_ERROR;
    } else {
        return CSI_OK;
    }
#endif
    return CSI_OK;
}

uint32_t csi_codec_output_write(csi_codec_output_t *ch, const void *data, uint32_t size)
{
    if (NULL == tx_context.dma) {
        user_log("codec have close ?\r\n");
        return 0;
    }
    //printf("----***********************************11111111111\r\n");
    //return 0;
    return auo_write(&tx_context, data, size);
}

uint32_t csi_codec_output_write_async(csi_codec_output_t *ch, const void *data, uint32_t size)
{
    if (NULL == tx_context.dma) {
        user_log("codec have close ?\r\n");
        return 0;
    }
    //printf("----***********************************\r\n");
    //return 0;
    return auo_write(&tx_context, data, size);
}

void csi_codec_output_stop(csi_codec_output_t *ch)
{
    auo_stop(&tx_context);
}

void csi_codec_output_detach_callback(csi_codec_output_t *ch)
{
    ch->callback = NULL;
    ch->arg = NULL;
}

void csi_codec_output_close(csi_codec_output_t *ch)
{
    //blyoc_audio_output_close(ch);
    if (tx_context.dma) {
        bl_os_free(tx_context.dma);
        tx_context.dma = NULL;
    }
}


csi_error_t csi_codec_output_pause(csi_codec_output_t *ch)
{
    auo_pause(&tx_context);

    return CSI_OK;
}

csi_error_t csi_codec_output_resume(csi_codec_output_t *ch)
{
    auo_resume(&tx_context);

    return CSI_OK;
}

uint32_t csi_codec_output_buffer_avail(csi_codec_output_t *ch)
{
    return auo_buffer_avail(&tx_context);
}

uint32_t csi_codec_output_buffer_remain(csi_codec_output_t *ch)
{
    return auo_buffer_remain(&tx_context);
}

csi_error_t csi_codec_output_buffer_reset(csi_codec_output_t *ch)
{
    //auo_buffer_reset(&tx_context);
    return CSI_OK;
}

csi_error_t csi_codec_output_mute(csi_codec_output_t *ch, bool enable)
{
    //blyoc_audio_output_set_mute(ch, enable ? 0 : 1);

    //auo_mute(&tx_context, enable);
    spk_mute(0, 1);// (ch, en)
    return CSI_OK;
}

csi_error_t csi_codec_output_digital_gain(csi_codec_output_t *ch, uint32_t val)
{
    //blyoc_audio_output_digital_gain(ch, 1, val);
    //auo_digtal_gain(&tx_context, val*2);
    spk_digital_gain_set(0, val);//(int ch, float gaindb)
    LOGD(TAG, "csi_codec_output_digital_gain = %ld\r\n", val);
    return CSI_OK;
}

csi_error_t csi_codec_output_analog_gain(csi_codec_output_t *ch, uint32_t val)
{
    //blyoc_audio_output_analog_gain(ch, val);
    return CSI_OK;
}

csi_error_t csi_codec_output_mix_gain(csi_codec_output_t *ch, uint32_t val)
{
    // bl606p not support

    return CSI_OK;
}

csi_error_t csi_codec_output_get_state(csi_codec_output_t *ch, csi_state_t *state)
{
    // bl606p always ok

    return CSI_OK;
}


void csi_codec_uninit(csi_codec_t *codec)
{
}

