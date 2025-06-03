/*
 * Copyright (C) 2022 Alibaba Group Holding Limited
 */

#include <stdint.h>

#include <devices/drv_snd_bl606p.h>
#include <board.h>

#include "eq_config.h"

#define PDM_CHANNEL                     0
#define PDM_IN_PIN                      0
#define PDM_CLK_PIN                     2
#define PA_PIN                          16

/* 默认采集增益 */
static int g_audio_in_gain[]  = { AUIDO_IN_GAIN_MIC, AUIDO_IN_GAIN_MIC, AUIDO_IN_GAIN_REF };
static int g_audio_out_gain[] = { AUIDO_OUT_GAIN, AUIDO_OUT_GAIN };

/*采集增益配置*/
extern int mic_analog_gain_set(int ch, float gaindb);

int board_audio_in_set_gain(int id, int gain)
{
    if (id < 0 || id >= sizeof(g_audio_in_gain) / sizeof(int)) {
        return -1;
    }
    mic_analog_gain_set(id, gain);
    g_audio_in_gain[id] = gain;
    return 0;
}

int board_audio_in_get_gain(int id)
{
    if (id < 0 || id >= sizeof(g_audio_in_gain) / sizeof(int)) {
        return -1;
    }
    return g_audio_in_gain[id];
}

/*播放增益配置*/
extern int spk_analog_gain_set(int ch, float gain);
int board_audio_out_set_gain(int id, int gain)
{
    // if (id < 0 || id >= sizeof(g_audio_out_gain) / sizeof(int)) {
    //     return -1;
    // }

    spk_analog_gain_set(0, gain);
    g_audio_out_gain[0] = gain;
    return 0;
}

int board_audio_out_get_gain(int id)
{
    // if (id < 0 || id >= sizeof(g_audio_out_gain) / sizeof(int)) {
    //     return -1;
    // }

    return g_audio_out_gain[0];
}

#ifdef CONFIG_SMART_AUDIO
extern audio_vol_config_t g_vol_config;

audio_vol_config_t *board_audio_out_get_vol_config()
{
    return &g_vol_config;
}

/*EQ参数配置*/
extern int auo_eq_set(uint32_t *eq_filt_coef, int bytes);
extern int auo_eq_show(void);

int board_eq_set_param(void *data, int byte)
{
    if (data == NULL) {
        /*设置默认参数*/
        auo_eq_set((uint32_t *)eq_filt_coef, sizeof(eq_filt_coef));
        return 0;
    }
    return auo_eq_set((uint32_t *)data, byte);
}

int board_eq_show_param(void)
{
    return auo_eq_show();
}

int board_eq_get_type(void)
{
    return EQ_TYPE_HARDWARE;
}

uint8_t *board_eq_get_param(size_t *byte)
{
    /*硬件EQ无需实现*/
    return NULL;
}

extern int auo_drc_set(uint32_t *drc_filt_coef, int bytes);
int board_drc_set_param(void *data, int byte)
{
    if (data == NULL) {
        /*设置默认参数*/
        auo_drc_set((uint32_t *)drc_filt_coef, sizeof(drc_filt_coef));
        return 0;
    }

    return auo_drc_set((uint32_t *)data, byte);
}
#endif /*CONFIG_SMART_AUDIO*/

/*PA控制引脚*/
int board_audio_get_pa_mute_pin(void)
{
    return AUIDO_PA_MUTE_PIN;
}

void board_audio_init(void)
{
    snd_bl606p_config_t snd_config;
    snd_config.audio_in_gain_list[0] = board_audio_in_get_gain(0);
    snd_config.audio_in_gain_list[1] = board_audio_in_get_gain(1);
    snd_config.audio_in_gain_list[2] = board_audio_in_get_gain(2);
    snd_config.audio_out_gain_list[0] = board_audio_out_get_gain(0);
    snd_config.audio_out_gain_list[1] = board_audio_out_get_gain(1);
    snd_config.mic_type = INPUT_MIC_TYPE_DIGITAL;
    snd_config.pdm_channel = PDM_CHANNEL;
    snd_config.pdm_in_pin = PDM_IN_PIN;
    snd_config.pdm_clk_pin = PDM_CLK_PIN;
    snd_config.pa_pin = PA_PIN;

    snd_card_bl606p_register(&snd_config);
}
