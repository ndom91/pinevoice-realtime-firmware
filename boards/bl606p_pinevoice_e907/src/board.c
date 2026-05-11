/*
 * Copyright (C) 2019-2020 Alibaba Group Holding Limited
 */
#include <board.h>
#include <drv/pin.h>
#include <drv/gpio.h>
#include <drv/dma.h>
#include <drv/wdt.h>
#include <sys_clk.h>
#include "bl606p_glb.h"
#include <bl606p_gpio.h>
#include <sys/app_sys.h>
#include <aos/kernel.h>

#define C906_UART 0

static void board_clock_config(void)
{
    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_UART2);
    GLB_Set_UART_CLK(1, HBN_UART_CLK_XCLK, 0);
}

void board_gpio_init(void)
{
#if 1
    // csi_gpio_t gpio_handler;

    // soc_clk_init();
    // csi_gpio_init(&gpio_handler, 0);

    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO12_GPIO23, 1);
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO36_GPIO45, 1);
    BL_WR_REG(GLB_BASE, GLB_UART_CFG1, 0xffffffff);// SIG7-SIG0
    BL_WR_REG(GLB_BASE, GLB_UART_CFG2, 0x0000ffff);// SIG11-SIG8

    /* pin12 ~ pin 23 and pin36 ~ pin45 must +6 to get actual sig num */
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_2, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART0_TXD); // (20 + 6)%12 = 2
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_3, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART0_RXD); // (21 + 6)%12 = 3

#if C906_UART
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_10, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART2_TXD); // (16 + 6)%12 = 10
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_11, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART2_RXD); // (17 + 6)%12 = 11
#endif
    //GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_6, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART2_TXD); // (12 + 6)%12 = 6
    //GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_11, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_FUN_UART2_RXD);// (11)%12 = 11
    //GLB_UART_Fun_Sel((GLB_UART_SIG_Type)GLB_UART_SIG_FUN_UART2_TXD, (GLB_UART_SIG_FUN_Type)GLB_UART_SIG_6);

    GLB_GPIO_Cfg_Type gpio_cfg;

    gpio_cfg.drive = 0;
    gpio_cfg.smtCtrl = 1;
    gpio_cfg.outputMode = 0;
    gpio_cfg.gpioMode = GPIO_MODE_AF;
    gpio_cfg.pullType = GPIO_PULL_UP;
    gpio_cfg.gpioFun = GPIO_FUN_UART;

    gpio_cfg.gpioPin = GLB_GPIO_PIN_20;
    GLB_GPIO_Init(&gpio_cfg);
    gpio_cfg.gpioPin = GLB_GPIO_PIN_21;
    GLB_GPIO_Init(&gpio_cfg);
#if C906_UART
    gpio_cfg.gpioPin = GLB_GPIO_PIN_16;
    GLB_GPIO_Init(&gpio_cfg);
    gpio_cfg.gpioPin = GLB_GPIO_PIN_17;
    GLB_GPIO_Init(&gpio_cfg);
#endif
#endif
}

void board_pwm_init(void)
{
    /* enabled pwm port 0 1 3*/
    csi_pin_set_mux(GPIO_PIN_3, GPIO3_PWM0_CH3P);
    csi_pin_set_mux(GPIO_PIN_4, GPIO4_PWM0_CH0P);
    csi_pin_set_mux(GPIO_PIN_1, GPIO1_PWM0_CH1P);
}

void board_adc_init(void)
{
;
}

void board_dma_init(void)
{
    // csi_dma_t dma;
    // csi_dma_init(&dma, 0);
}

void board_wdt_init(void)
{
    csi_wdt_t wdt;
    csi_wdt_init(&wdt, 0);
    csi_wdt_stop(&wdt);
}

void board_init(void)
{
    board_clock_config();
    board_gpio_init();
    board_dma_init();
    board_wdt_init();
}

#define MUTE_STATUS_OD_VALID      (1)   // MUTE_STATUS_OD H <--- MUTE LED OFF
#define MUTE_STATUS_OD_PIN        (18)
#define MUTE_SET_LV_PIN           (17)
void mutekey_task_entry(void *arg)
{
    int mute_state = MUTE_STATUS_OD_VALID;
    uint32_t   value = 0;
    int ret;
    int press_tmp = 0;

    /* init gpio */
    // hal_gpio_init(&gpio);
    bl_gpio_enable_input(MUTE_STATUS_OD_PIN, 0, 0);
    bl_gpio_output_set(MUTE_SET_LV_PIN, 1);
    bl_gpio_enable_output(MUTE_SET_LV_PIN, 1, 0);// pullup

    /* get kv */
    ret = aos_kv_getint("MUTE_STATE", &mute_state);
    if (ret != 0) {
        printf("msp kv mute state:%d(first init)\r\n", mute_state);
        aos_kv_setint("MUTE_STATE", mute_state);
    }

    if (MUTE_STATUS_OD_VALID != mute_state) {
        printf("msp kv mute set mute\r\n");
        bl_gpio_output_set(MUTE_SET_LV_PIN, 0);
        aos_msleep(5);
        bl_gpio_output_set(MUTE_SET_LV_PIN, 1);
    }

    /* config */
    while (1) {
        value = bl_gpio_input_get_value(MUTE_STATUS_OD_PIN);
        if (value != mute_state) {
            press_tmp++;
            if (press_tmp >= 10) {
                mute_state = value;
                if (app_sys_get_boot_reason() != BOOT_REASON_FACTORY_MODE) {
                    aos_kv_setint("MUTE_STATE", mute_state);
                } 
                printf("msp kv mute state:%d\r\n", mute_state);
            }
        } else {
            press_tmp = 0;
        }

        aos_msleep(20);
    }

    aos_task_exit(0);
}

void trv03_mute_config(void)
{
    static aos_task_t mute_task_handle;

    aos_task_new_ext(&mute_task_handle, "mute_task", mutekey_task_entry,
            NULL, 4096, 32);
}

void board_app_init(void)
{
    trv03_mute_config();
}

static uint32_t g_pa_delay_1 = 5;
static uint32_t g_pa_delay_2 = 90;     /* this is time set according to the pa manual */

void msp_codec_pa_init_pre(void)
{
//#if CONFIG_ENABLE_BOTTOM_PA
    // pullup
#if CONFIG_CODEC_USE_I2S
    //msp_i2s_device_init();
#endif
    msp_gpio_output_config(CONFIG_AUDIO_PA_PIN, 1);
    //bl_gpio_enable_output(pa_pin, 1, 0);
    MSP_LOGD("msp_codec_pa_init_pre\r\n");
//#endif
}

void msp_codec_pa_init(void)
{
// #if CONFIG_ENABLE_BOTTOM_PA
//     // init
//     bl_gpio_enable_output(CONFIG_AUDIO_PA_PIN, 1, 0);
// #endif
//#if CONFIG_ENABLE_BOTTOM_PA
    // gpio set 0 ---> disable
    //msp_msleep(1000);
    msp_gpio_output_set(CONFIG_AUDIO_PA_PIN, 0);
    msp_msleep(1);
    MSP_LOGD("msp_codec_pa_init\r\n");
//#endif
}

//void msp_codec_pa_after_opendac(void){}

void msp_codec_pa_after_opendac(void)
{
    static int init = 0;
//#if CONFIG_ENABLE_BOTTOM_PA
    // enable
    // enable, gpio set 1
    //user_log("msp_codec_pa_after_opendac delay1 ---> %d ms\r\n", g_pa_delay_1);
    msp_msleep(g_pa_delay_1);
    if (0 == init) {
        //msp_msleep(1000);
    }
    msp_gpio_output_set(CONFIG_AUDIO_PA_PIN, 1);
    //user_log("msp_codec_pa_after_opendac delay2 ---> %d ms\r\n", g_pa_delay_2);
    if (0 == init) {
        //msp_msleep(1000);
        init = 1;
    }
    msp_msleep(g_pa_delay_2);
    MSP_LOGD("msp_codec_pa_after_opendac\r\n");
//#endif
}

void msp_codec_pa_before_closedac(void)
{
//#if CONFIG_ENABLE_BOTTOM_PA
    //msp_msleep(50);
    // disable, gpio set 0
    msp_gpio_output_set(CONFIG_AUDIO_PA_PIN, 0);
    msp_msleep(20);
    MSP_LOGD("msp_codec_pa_before_closedac\r\n");
//#endif
}
