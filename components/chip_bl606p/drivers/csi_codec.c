#include <drv/codec.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>
#include <xcodec.h>

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

csi_error_t csi_codec_init(csi_codec_t *codec, uint32_t idx)
{
    return xcodec_init(codec, idx);
}

csi_error_t csi_codec_input_open(csi_codec_t *codec, csi_codec_input_t *ch, uint32_t ch_idx)
{
    return xcodec_input_open(codec, ch, ch_idx);
}

csi_error_t csi_codec_input_config(csi_codec_input_t *ch, csi_codec_input_config_t *config)
{
    return xcodec_input_config(ch, config);
}

csi_error_t csi_codec_input_attach_callback(csi_codec_input_t *ch, void *callback, void *arg)
{
    return xcodec_input_attach_callback(ch, callback, arg);
}

csi_error_t csi_codec_input_link_dma(csi_codec_input_t *ch, csi_dma_ch_t *dma)
{
    xcodec_dma_ch_t dma_desc;

    if (NULL == dma) {
        return xcodec_input_link_dma(ch, NULL);
    }
    dma_desc.ctrl_id = dma->ctrl_id;
    dma_desc.ch_id = dma->ch_id;
    return xcodec_input_link_dma(ch, &dma_desc);
}

csi_error_t csi_codec_input_start(csi_codec_input_t *ch)
{
    return xcodec_input_start(ch);
}

uint32_t csi_codec_input_read(csi_codec_input_t *ch, void *data, uint32_t size)
{
    return xcodec_input_read(ch, data, size);
}

uint32_t csi_codec_input_read_async(csi_codec_input_t *ch, void *data, uint32_t size)
{
    return xcodec_input_read_async(ch, data, size);
}

void csi_codec_input_stop(csi_codec_input_t *ch)
{
    xcodec_input_stop(ch);
}

void csi_codec_input_detach_callback(csi_codec_input_t *ch)
{
    xcodec_input_detach_callback(ch);
}

void csi_codec_input_close(csi_codec_input_t *ch)
{
    xcodec_input_close(ch);
}

uint32_t csi_codec_input_buffer_avail(csi_codec_input_t *ch)
{
    return xcodec_input_buffer_avail(ch);
}
uint32_t csi_codec_input_buffer_remain(csi_codec_input_t *ch)
{
    return xcodec_input_buffer_remain(ch);
}

csi_error_t csi_codec_input_buffer_reset(csi_codec_input_t *ch)
{
    // bl606p unnecessary
    return xcodec_input_buffer_reset(ch);
}

csi_error_t csi_codec_input_mute(csi_codec_input_t *ch, bool enable)
{
    return xcodec_input_mute(ch, enable);
}

csi_error_t csi_codec_input_mix_gain(csi_codec_input_t *ch, uint32_t val)
{
    return xcodec_input_mix_gain(ch, val);
}
csi_error_t csi_codec_input_digital_gain(csi_codec_input_t *ch, uint32_t val)
{
    return xcodec_input_digital_gain(ch, val);
}
csi_error_t csi_codec_input_analog_gain(csi_codec_input_t *ch, uint32_t val)
{
    return xcodec_input_analog_gain(ch, val);
}

csi_error_t csi_codec_output_open(csi_codec_t *codec, csi_codec_output_t *ch, uint32_t ch_idx)
{
    return xcodec_output_open(codec, ch, ch_idx);
}

csi_error_t csi_codec_output_config(csi_codec_output_t *ch, csi_codec_output_config_t *config)
{
    return xcodec_output_config(ch, config);
}

csi_error_t csi_codec_output_attach_callback(csi_codec_output_t *ch, void *callback, void *arg)
{
    return xcodec_output_attach_callback(ch, callback, arg);
}

csi_error_t csi_codec_output_link_dma(csi_codec_output_t *ch, csi_dma_ch_t *dma)
{
    xcodec_dma_ch_t dma_desc;

    if (NULL == dma) {
        return xcodec_output_link_dma(ch, NULL);
    }
    dma_desc.ctrl_id = dma->ctrl_id;
    dma_desc.ch_id = dma->ch_id;
    return xcodec_output_link_dma(ch, &dma_desc);
}

csi_error_t csi_codec_output_start(csi_codec_output_t *ch)
{
    return xcodec_output_start(ch);
}

uint32_t csi_codec_output_write(csi_codec_output_t *ch, const void *data, uint32_t size)
{
    return xcodec_output_write(ch, data, size);
}

uint32_t csi_codec_output_write_async(csi_codec_output_t *ch, const void *data, uint32_t size)
{

    return xcodec_output_write_async(ch, data, size);
}

void csi_codec_output_stop(csi_codec_output_t *ch)
{
    xcodec_output_stop(ch);
}

void csi_codec_output_detach_callback(csi_codec_output_t *ch)
{
    xcodec_output_detach_callback(ch);
}

void csi_codec_output_close(csi_codec_output_t *ch)
{
    xcodec_output_close(ch);
}

csi_error_t csi_codec_output_pause(csi_codec_output_t *ch)
{
    return xcodec_output_pause(ch);
}

csi_error_t csi_codec_output_resume(csi_codec_output_t *ch)
{
    return xcodec_output_resume(ch);
}

uint32_t csi_codec_output_buffer_avail(csi_codec_output_t *ch)
{
    return xcodec_output_buffer_avail(ch);
}

uint32_t csi_codec_output_buffer_remain(csi_codec_output_t *ch)
{
    return xcodec_output_buffer_remain(ch);
}

csi_error_t csi_codec_output_buffer_reset(csi_codec_output_t *ch)
{
    return xcodec_output_buffer_reset(ch);
}

csi_error_t csi_codec_output_mute(csi_codec_output_t *ch, bool enable)
{
    return xcodec_output_mute(ch, enable);
}

csi_error_t csi_codec_output_digital_gain(csi_codec_output_t *ch, uint32_t val)
{
    return xcodec_output_digital_gain(ch, val);
}

csi_error_t csi_codec_output_analog_gain(csi_codec_output_t *ch, uint32_t val)
{
    return xcodec_output_analog_gain(ch, val);
}

csi_error_t csi_codec_output_mix_gain(csi_codec_output_t *ch, uint32_t val)
{
    return xcodec_output_mix_gain(ch, val);
}

csi_error_t csi_codec_output_get_state(csi_codec_output_t *ch, csi_state_t *state)
{
    return xcodec_output_get_state(ch, state);
}

void csi_codec_uninit(csi_codec_t *codec)
{
    xcodec_uninit(codec);
}


