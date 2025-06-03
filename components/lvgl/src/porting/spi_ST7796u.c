/*
 *by tongxiaohua@20220706
 *ST7796 SCREEN DRIVER test
 */
#include <bl606p_common.h>
#include <misc.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include <bl606p_spi.h>
#include "port_api.h"
#include "bl606p_dma.h"

#define LLI_BUFF_SIZE 2048
#define MAX_SEQ_CMD_LENGTH 32
#define BMP_HEAD 54

uint8_t spi_cs_gpio = GLB_GPIO_PIN_28;
uint8_t spi_clk_gpio = GLB_GPIO_PIN_27;
uint8_t spi_mosi_gpio = GLB_GPIO_PIN_26;
uint8_t spi_dc_gpio = GLB_GPIO_PIN_25;
uint8_t spi_reset_gpio = GLB_GPIO_PIN_24;
uint8_t spi_led_gpio = GLB_GPIO_PIN_12;
uint8_t spi_te_gpio = GLB_GPIO_PIN_0;
uint32_t led_brightness = DEFAULT_BRIGHTNESS;
hosal_pwm_dev_t pwd_led;
#if SPI_DMA_ENABLE && 0
SemaphoreHandle_t dma_lock;
#else
static void (*s_lvgl_flush_cb)(void *arg) = NULL;
static void *s_lvgl_flush_arg = NULL;
#endif

/*
 *init seq,
 *-1,cmd start
 *-2,sleep,ms
 *-3,end
 */

// static int spi_screen_init_seq[] = {

//     -1, 0xF0, 0xC3,
//     -1, 0xF0, 0x96,
//     -1, 0x36, 0x48,
//     -1, 0xB4, 0x01,
//     -1, 0xB7, 0xC6,
//     -1, 0xE8, 0x40,0x8A,0x00,0x00,0x29,0x19,0xa5,0x33,
//     -1, 0xC2, 0xA7,
//     -1, 0xC5, 0x18,
//     -1, 0xE0, 0xf0,0x0e,0x19,0x10,0x10,0x0c,0x42,0x54,0x4f,0x08,0x0f,0x0b,0x21,0x23,
//     -1, 0xe1, 0xf0,0x0e,0x19,0x10,0x10,0x1a,0x41,0x44,0x52,0x0d,0x1c,0x1e,0x20,0x23,
//     -1, 0x3A, 0x05,
//     -1, 0x21,
//     -1, 0x35,0x00,
//     -1, 0x11,
//     -2, 120,
//     -1, 0x29,
//     -1, 0x2c,
//     -3};
static int spi_screen_init_seq[] = {

    -1, 0xF0, 0xC3,
    -1, 0xF0, 0x96,
    -1, 0x36, 0x48,
    //-1, 0x3A, 0x66,
    -1, 0x3A, 0x55,
    //-1, 0xB1,0x80,0x10,
    -1, 0xB4, 0x00, // 01
    -1, 0xB6, 0x80, 0x00, 0x27,
    -1, 0xB7, 0xC6,
    -1, 0xE8, 0x40, 0x84, 0x1D, 0x21, 0x28, 0x13, 0x3f, 0x33,
    -1, 0xC0, 0xA0, 0x07,
    -1, 0xC1, 0x09, // 13
    -1, 0xC2, 0xA5, // a7
    -1, 0xC5, 0x08, // 09
    -1, 0xE0, 0xF0, 0x17, 0x1B, 0x0B, 0x0A, 0x08, 0x37, 0x44, 0x4B, 0x3C, 0x17, 0x16, 0x2E, 0x35,
    -1, 0xE1, 0xF0, 0x1A, 0x1D, 0x08, 0x06, 0x10, 0x37, 0x33, 0x4A, 0x32, 0x10, 0x11, 0x29, 0x31,
    -1, 0xF0, 0x3C,
    -1, 0x35, 0x00,
    -1, 0x21,
    -1, 0x11,
    -2, 120,
    -1, 0x29,
    -1, 0x2c,
    -3};
// static int spi_screen_init_seq[] = {
//     -1,0xF0,0xC3,
//     -1,0xF0,0x96,
//     -1,0x36,0x48,
//     -1,0x3A,0x55,
//     -1,0xB4,0x01,
//     -1,0xB1,0x80,0x10,
//     -1,0xB5,0x1F,0x39,0x00,0x20,
//     -1,0xB6,0x8A,0x07,0x27,
//     -1,0xB7,0xC6,
//     -1,0xB9,0x02,0xE0,
//     -1,0xC0,0x80,0x05,
//     -1,0xC1,0x15,
//     -1,0xC2,0xA7,
//     -1,0xC5,0x15,
//     -1,0xE8,0x40,0x84,0x1D,0x21,0x28,0x13,0x3f,0x33,
//     -1,0xE0,0xF0,0x03,0x09,0x02,0x01,0x01,0x33,0x44,0x49,0x3A,0x16,0x18,0x2F,0x34,
//     -1,0xE1,0xF0,0x0F,0x15,0x0D,0x0D,0x28,0x32,0x33,0x49,0x03,0x0D,0x0E,0x28,0x2F,
//     -1,0xF0,0x3C,
//     -1,0xF0,0x69,
//     -1,0x35,0x00,
//     -1,0x21,
//     -1,0x11,
//     -2,120,
//     -1,0x29,
//     -3
// };
BL_Err_Type hard_spi_screen_reset()
{
    // GLB_GPIO_Write(spi_reset_gpio, 1);
    // arch_delay_us(1000 * 200);
    GLB_GPIO_Write(spi_reset_gpio, 0);
    // arch_delay_us(1000 * 400);
    arch_delay_us(1000 * 100);
    GLB_GPIO_Write(spi_reset_gpio, 1);
    // arch_delay_us(1000 * 400);
    arch_delay_us(1000 * 100);
    // arch_delay_us(1000 * 120);

    // LED_LOW;
    // CS_LOW;
    // LED_HIGH;
}

void spi_set_brightness(uint32_t brightness)
{
    // 把0-10000落到3100-5000的范围，因为经测试30%~50%的占空比才能有变化
    brightness = brightness / 5 + 3300;

    hosal_pwm_config_t pct = {spi_led_gpio, brightness, DEFAULT_PWM_FRRQ};

    led_brightness = brightness;

    hosal_pwm_para_chg(&pwd_led, pct);
}

/**
 * @brief spi screen gpio init
 *
 * @return None
 */
static void hard_spi_screen_gpio_init(void)
{
    GLB_GPIO_Type gpioPins[3] = {spi_clk_gpio, spi_mosi_gpio, spi_cs_gpio};

    GLB_GPIO_Cfg_Type reset_gpioCfg = {
        /* reset */
        .gpioPin = spi_reset_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .outputMode = GPIO_OUTPUT_VALUE_MODE,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type dc_gpioCfg = {
        /* dc */
        .gpioPin = spi_dc_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .outputMode = GPIO_OUTPUT_VALUE_MODE,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type te_gpioCfg = {
        /* te */
        .gpioPin = spi_te_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_INPUT,
        .pullType = GPIO_PULL_UP,
        .drive = 0,
        .smtCtrl = 1};

    GLB_GPIO_Func_Init(GPIO_FUN_SPI0, (GLB_GPIO_Type *)gpioPins, sizeof(gpioPins) / sizeof(gpioPins[0]));
    GLB_GPIO_Init(&reset_gpioCfg);
    GLB_GPIO_Init(&dc_gpioCfg);
    GLB_GPIO_Init(&te_gpioCfg);

    // pwm led
    pwd_led.port = 0;
    pwd_led.config.pin = spi_led_gpio;
    pwd_led.config.duty_cycle = led_brightness;
    pwd_led.config.freq = DEFAULT_PWM_FRRQ;
    pwd_led.priv = NULL;

    hosal_pwm_init(&pwd_led);

    // GLB_Set_MCU_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);
}

/**
 * @brief spi screen init
 *
 * @return BL_Err_Type
 */
BL_Err_Type hard_spi_screen_init(void)
{
    SPI_CFG_Type spiCfg = {
        DISABLE,                      /* Disable de-glitch function */
        SPI_SLAVE_PIN_4,              /* SPI 4-pin mode(CS is enabled) */
        ENABLE,                       /* Disable master continuous transfer mode */
        SPI_BYTE_INVERSE_BYTE0_FIRST, /* The byte 0 is sent first in SPI transfer */
        SPI_BIT_INVERSE_MSB_FIRST,    /* MSB is sent first in SPI transfer */
        SPI_CLK_PHASE_INVERSE_1,      /* SPI clock phase */
        SPI_CLK_POLARITY_HIGH,        /* SPI clock plarity */
        // SPI_CLK_POLARITY_LOW,
        SPI_FRAME_SIZE_8 /* SPI frame size 8-bit(also the valid width for each fifo entry) */
    };

    // SPI_ClockCfg_Type clockCfg = {
    //     2, /* Length of start condition */
    //     2, /* Length of stop condition */
    //     2, /* Length of data phase 0,affecting clock */
    //     2, /* Length of data phase 1,affecting clock */
    //     2  /* Length of interval between frame */
    // };

    SPI_FifoCfg_Type fifocfg = {
        7,
        0,
        ENABLE,
        DISABLE};
    /* gpio and pad init */
    hard_spi_screen_gpio_init();
    GLB_Swap_MCU_SPI_0_MOSI_With_MISO(ENABLE);
    GLB_Set_MCU_SPI_0_ACT_MOD_Sel(GLB_SPI_PAD_ACT_AS_MASTER);

    /* SPI interrupt config */
    // SPI_Int_Callback_Install(SPI_USE_ID, SPI_INT_END, SPI_Int_Callback);
    SPI_IntMask(SPI_NUM, SPI_INT_ALL, MASK);
    // bl_irq_enable(SPI_USE_IRQ);
    // CPU_Interrupt_Enable(SPI_USE_IRQ);
    // System_NVIC_SetPriority(SPI_USE_IRQ, 4, 1);

    /* SPI config */
    SPI_Init(SPI_NUM, &spiCfg);
    /* Set SPI clock */
    SPI_SetClock(SPI_NUM, 80000000);
    // SPI_ClockConfig(SPI_NUM, &clockCfg);

    /*fifo & dma*/
    SPI_FifoConfig(SPI_NUM, &fifocfg);

    /* Enable spi master mode */
    SPI_Enable(SPI_NUM, SPI_WORK_MODE_MASTER);

    hard_spi_screen_reset();

#if SPI_DMA_ENABLE && 0
    dma_lock = xSemaphoreCreateBinary();
    if (dma_lock == NULL)
    {
        printf("dma lock create failed!\r\n");
        return ERROR;
    }
#endif
    return SUCCESS;
}

#if SPI_DMA_ENABLE
static int lli_list_init(DMA_LLI_Ctrl_Type **pptxlli, DMA_LLI_Ctrl_Type **pprxlli, uint8_t *ptx_data, uint8_t *prx_data, uint32_t length)
{
    uint32_t i = 0;
    uint32_t count;
    uint32_t remainder;
    struct DMA_Control_Reg_t dmactrl;

    count = length / LLI_BUFF_SIZE;
    remainder = length % LLI_BUFF_SIZE;

    if (remainder != 0)
    {
        count = count + 1;
    }

    dmactrl.SBSize = DMA_BURST_SIZE_8;
    dmactrl.DBSize = DMA_BURST_SIZE_8;
    dmactrl.SWidth = DMA_TRNS_WIDTH_8BITS;
    dmactrl.DWidth = DMA_TRNS_WIDTH_8BITS;
    dmactrl.dst_min_mode = DISABLE;
    dmactrl.dst_add_mode = DISABLE;
    dmactrl.fix_cnt = 0;
    dmactrl.Prot = 0;
    dmactrl.reserved_20 = 0;
    dmactrl.I = 1;

    if (*pptxlli)
    {
        vPortFree(*pptxlli);
        *pptxlli = NULL;
    }
    *pptxlli = pvPortMalloc(sizeof(DMA_LLI_Ctrl_Type) * count);
    if (*pptxlli == NULL)
    {
        printf("malloc lli failed. \r\n");

        return -1;
    }

    for (i = 0; i < count; i++)
    {
        if (remainder == 0)
        {
            dmactrl.TransferSize = LLI_BUFF_SIZE;
        }
        else
        {
            if (i == count - 1)
            {
                dmactrl.TransferSize = remainder;
            }
            else
            {
                dmactrl.TransferSize = LLI_BUFF_SIZE;
            }
        }

        if (i == count - 1)
        {
            dmactrl.I = 1;
        }
        else
        {
            dmactrl.I = 0;
        }

        if (NULL != ptx_data)
        {
            dmactrl.SI = DMA_MINC_ENABLE;
            dmactrl.DI = DMA_MINC_DISABLE;
            (*pptxlli)[i].srcDmaAddr = (uint32_t)(ptx_data + i * LLI_BUFF_SIZE);
            (*pptxlli)[i].destDmaAddr = (uint32_t)(SPI0_BASE + SPI_FIFO_WDATA_OFFSET);
            (*pptxlli)[i].dmaCtrl = dmactrl;

            if (i != 0)
            {
                (*pptxlli)[i - 1].nextLLI = (uint32_t) & (*pptxlli)[i];
            }
            (*pptxlli)[i].nextLLI = 0;
        }
    }

    if (NULL != pptxlli)
    {
        csi_dcache_clean_range(pptxlli, count * sizeof(DMA_LLI_Ctrl_Type));
    }

    if (NULL != ptx_data)
    {
        csi_dcache_clean_range(ptx_data, length);
    }

    return count;
}

static void dma_spi_int_handler_tx(void *arg)
{
    // printf("!!![dma_log]!!!once DMA send:%d\r\n",__LINE__);
#if 0
    xSemaphoreGiveFromISR(dma_lock);
#else
    if (s_lvgl_flush_cb)
        s_lvgl_flush_cb(s_lvgl_flush_arg);
#endif
}

int dma_spi_send_data(int port, uint8_t *data, int len)
{
#if 1
    DMA_LLI_Cfg_Type txllicfg = {
        .dir = DMA_TRNS_M2P,
        .srcPeriph = DMA_REQ_NONE,
        .dstPeriph = DMA_REQ_SPI0_TX};
    static DMA_LLI_Ctrl_Type *ptxlli = NULL;
    int dma_id = DMA0_ID;
    int tx_dma_ch = DMA_CH6;

    csi_dcache_clean_invalid();
#if 0
    xSemaphoreTake(dma_lock, -1);
#else
    while (DMA_Channel_Is_Busy(dma_id, tx_dma_ch))
    {
        vTaskDelay(1);
    }
#endif

    DMA_Channel_Disable(dma_id, tx_dma_ch);
    DMA_IntMask(dma_id, tx_dma_ch, DMA_INT_ALL, MASK);

    if (lli_list_init(&ptxlli, NULL, data, NULL, len) < 0)
    {
        printf("init lli failed. \r\n");
        return -1;
    }

    msp_dma_irq_function(dma_id, tx_dma_ch, &dma_spi_int_handler_tx, NULL);
    DMA_IntMask(dma_id, tx_dma_ch, DMA_INT_ALL, UNMASK);
    DMA_LLI_Init(dma_id, tx_dma_ch, (DMA_LLI_Cfg_Type *)&txllicfg);
    DMA_LLI_Update(dma_id, tx_dma_ch, ptxlli);
    DMA_Channel_Enable(dma_id, tx_dma_ch);
    csi_dcache_clean_invalid();
#else
    int i = 0;
    int count = 0;
    int need_len = 0;

    if (len % LLI_BUFF_SIZE == 0)
    {
        count = len / LLI_BUFF_SIZE;
    }
    else
    {
        count = len / LLI_BUFF_SIZE + 1;
    }

    for (i = 0; i < count - 1; i++)
    {
        dma_spi_send_data_fragment(port, &data[i * LLI_BUFF_SIZE], LLI_BUFF_SIZE);
    }

    if (len % LLI_BUFF_SIZE == 0)
    {
        dma_spi_send_data_fragment(port, &data[i * LLI_BUFF_SIZE], LLI_BUFF_SIZE);
    }
    else
    {
        dma_spi_send_data_fragment(port, &data[i * LLI_BUFF_SIZE], len % LLI_BUFF_SIZE);
    }

#endif
    return 0;
}
#endif

BL_Err_Type spi_screen_deinit()
{
    hosal_pwm_stop(&pwd_led);
    hosal_pwm_finalize(&pwd_led);
    SPI_Disable(SPI_NUM, SPI_WORK_MODE_MASTER);
    SPI_DeInit(SPI_NUM);

    return SUCCESS;
}

void spi_screen_write_reg(uint8_t byte)
{
#if SOFT_SPI
    soft_spi_write_reg(byte);
#elif SPI_DMA_ENABLE
    BL_Err_Type err = SUCCESS;
    DC_LOW;
    if (dma_spi_send_data(SPI_NUM, &byte, 1) < 0)
    {
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#else
    BL_Err_Type err = SUCCESS;
    DC_LOW;
    if ((err = SPI_SendData(SPI_NUM, &byte, 1, SPI_TIMEOUT_ENABLE)) != SUCCESS)
    {
        int j = 0;
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#endif
}

void spi_screen_write_data(uint8_t byte)
{
#if SOFT_SPI
    soft_spi_write_data(byte);
#elif SPI_DMA_ENABLE
    BL_Err_Type err = SUCCESS;
    DC_HIGH;
    if (dma_spi_send_data(SPI_NUM, &byte, 1) < 0)
    {
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#else
    BL_Err_Type err = SUCCESS;
    DC_HIGH;
    if ((err = SPI_SendData(SPI_NUM, &byte, 1, SPI_TIMEOUT_ENABLE)) != SUCCESS)
    {
        int j = 0;
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#endif
}

void spi_screen_write_byte(uint8_t byte)
{
#if SOFT_SPI
    soft_spi_write_byte(byte);
#elif SPI_DMA_ENABLE
    BL_Err_Type err = SUCCESS;
    if (dma_spi_send_data(SPI_NUM, &byte, 1) < 0)
    {
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#else
    BL_Err_Type err = SUCCESS;
    if ((err = SPI_SendData(SPI_NUM, &byte, 1, SPI_TIMEOUT_ENABLE)) != SUCCESS)
    {
        int j = 0;
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#endif
}

void spi_screen_register_tc_cb(void (*cb)(void *arg), void *arg)
{
    s_lvgl_flush_cb = cb;
    s_lvgl_flush_arg = arg;
}

void spi_screen_write_len(uint8_t *byte, uint32_t len)
{
    csi_dcache_clean_range(byte, len);
#if SOFT_SPI
    uint32_t i = 0;
    for (i = 0; i < len; i++)
    {
        soft_spi_write_byte(byte + i);
    }
#elif SPI_DMA_ENABLE
    BL_Err_Type err = SUCCESS;
    if (dma_spi_send_data(SPI_NUM, byte, len) < 0)
    {
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#else
    BL_Err_Type err = SUCCESS;
    if ((err = SPI_SendData(SPI_NUM, byte, len, SPI_TIMEOUT_ENABLE)) != SUCCESS)
    {
        int j = 0;
        printf("send data :[%x]faild !err = %d\r\n", byte, err);
    }
#endif
}

void spi_screen_write_data16(uint16_t data)
{
    spi_screen_write_data(data >> 8);
    spi_screen_write_data(data);
}

int spi_screen_is_ready(void)
{
#if CHECK_TE_ENABLE && SPI_DMA_ENABLE
    if (DMA_Channel_Is_Busy(DMA0_ID, DMA_CH6))
    {
        return 0;
    }
    while (!GLB_GPIO_Read(spi_te_gpio))
        ;
#elif CHECK_TE_ENABLE && !SPI_DMA_ENABLE
    while (!GLB_GPIO_Read(spi_te_gpio))
        ;
#elif !CHECK_TE_ENABLE && SPI_DMA_ENABLE
    if (DMA_Channel_Is_Busy(DMA0_ID, DMA_CH6))
    {
        return 0;
    }
#endif
    while (SPI_GetTxFifoCount(SPI0_ID) < 32)
        ;
    while (SPI_GetBusyStatus(SPI0_ID) != 0)
        ;
    return 1;
}

static void SetAddress(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    uint8_t data[4] = {0};
    spi_screen_write_reg(0x2a);
    spi_screen_write_data16(x_start);
    spi_screen_write_data16(x_end);
    spi_screen_write_reg(0x2b);
    spi_screen_write_data16(y_start);
    spi_screen_write_data16(y_end);
    spi_screen_write_reg(0x2c);
}

void hard_spi_lcd_test(void)
{

    uint32_t i, j, k = 0;
    SetAddress(0, 0, 319, 319);
    DC_HIGH;
    for (i = 0; i < 320; i++)
    {
        for (j = 0; j < 320; j++)
        {
            spi_screen_write_byte(0x00);
            spi_screen_write_byte(0x00);
            spi_screen_write_byte(0xff);
        }
    }
}

#define GUI_IMG_PX_SIZE_ALPHA_BYTE 4
#define GUI_565_BYTE 2

static int _RGB888ToRGB565(unsigned int *n888Color, unsigned char *n565Color)
{
    unsigned char cAlpha = (*n888Color & 0xff000000) >> 24;
    unsigned char cRed = (*n888Color & 0x00ff0000) >> 19;
    unsigned char cGreen = (*n888Color & 0x0000ff00) >> 10;
    unsigned char cBlue = (*n888Color & 0x000000ff) >> 3;

    n565Color[0] = (cBlue << 3) + (cGreen >> 3);
    n565Color[1] = (cGreen << 5) + cRed;
    // n565Color[2] = cAlpha;
}

void soft_spi_screen_reset();
void soft_spi_screen_init();
void soft_spi_write_reg(uint8_t byte);
void soft_spi_write_data(uint8_t byte);
void screen_init();
void spi_lcd_test(void);
void screen_led_on(void);

void screen_led_on(void){
    hosal_pwm_start(&pwd_led);
}

void screen_init()
{

    uint8_t send_buf[MAX_SEQ_CMD_LENGTH] = {0};
    int i = 0, send_buf_len = 0;
    int x = 0, y = 0;

    printf("start init:%d\r\n", bl_os_clock_gettime_ms());
    spi_screen_init();
    printf("end init:%d\r\n", bl_os_clock_gettime_ms());

    for (i = 0; i < sizeof(spi_screen_init_seq) / sizeof(spi_screen_init_seq[0]); i++){
        if (spi_screen_init_seq[i] != -1 && spi_screen_init_seq[i] != -2 && spi_screen_init_seq[i] != -3){
            send_buf[send_buf_len++] = spi_screen_init_seq[i];
        }
        else if (spi_screen_init_seq[i] == -2){
            arch_delay_us(1000 * spi_screen_init_seq[i + 1]);
            i++;
        }else{
            // printf("start or end!\r\n");
            if (send_buf_len > 0){
                int j = 0;
                // printf("send cmd:%02x\r\n", send_buf[j]);
                spi_screen_write_reg(send_buf[j]);
                DC_HIGH;
                for (j = 1; j < send_buf_len; j++)
                {
                    // printf("send byte:%02x\r\n", send_buf[j]);
                    spi_screen_write_byte(send_buf[j]);
                }
            }
            memset(send_buf, 0, sizeof(send_buf));
            send_buf_len = 0;
        }
    }
    printf("screen_init success!%d\r\n", bl_os_clock_gettime_ms());

    //screen_led_on
}

// api
void spi_screen_set_address(uint16_t x_start, uint16_t y_start, uint16_t x_end, uint16_t y_end)
{
    SetAddress(x_start, y_start, x_end, y_end);
}
void spi_screen_reset()
{
#if SOFT_SPI
    soft_spi_screen_reset();
#else
    hard_spi_screen_reset();
#endif
}
void spi_screen_init()
{
#if SOFT_SPI
    soft_spi_screen_init();
#else
    hard_spi_screen_init();
#endif
}

///////////////////SOFT SPI////////////////////////////////////////////////////////////

void soft_spi_screen_reset()
{
    GLB_GPIO_Write(spi_reset_gpio, 1);
    arch_delay_us(1000 * 50);
    GLB_GPIO_Write(spi_reset_gpio, 0);
    arch_delay_us(1000 * 50);
    GLB_GPIO_Write(spi_reset_gpio, 1);
    arch_delay_us(1000 * 50);
}

void soft_spi_screen_init()
{
    GLB_GPIO_Cfg_Type cs_gpioCfg = {

        .gpioPin = spi_cs_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type clk_gpioCfg = {

        .gpioPin = spi_clk_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type mosi_gpioCfg = {

        .gpioPin = spi_mosi_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type dc_gpioCfg = {

        .gpioPin = spi_dc_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type reset_gpioCfg = {

        .gpioPin = spi_reset_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};
    GLB_GPIO_Cfg_Type led_gpioCfg = {

        .gpioPin = spi_led_gpio,
        .gpioFun = 11,
        .gpioMode = GPIO_MODE_OUTPUT,
        .pullType = GPIO_PULL_NONE,
        .drive = 3,
        .smtCtrl = 1};

    GLB_GPIO_Init(&cs_gpioCfg);
    GLB_GPIO_Init(&clk_gpioCfg);
    GLB_GPIO_Init(&mosi_gpioCfg);
    GLB_GPIO_Init(&dc_gpioCfg);
    GLB_GPIO_Init(&reset_gpioCfg);
    GLB_GPIO_Init(&led_gpioCfg);

    soft_spi_screen_reset();

    CS_HIGH;
    CLK_HIGH;
    // LED_HIGH;
}

/*CPOL=0,CPHA=0*/
void soft_spi_write_byte(uint8_t byte)
{
    uint8_t i = 0;
    CS_LOW;
    for (i = 0; i < 8; i++)
    {

        if (byte & 0x80)
            MOSI_HIGE;
        else
            MOSI_LOW;
        byte <<= 1;
        CLK_LOW;
        // arch_delay_us(1);
        CLK_HIGH;
    }
    CS_HIGH;
}

void soft_spi_write_reg(uint8_t byte)
{
    CS_LOW;
    DC_LOW;
    soft_spi_write_byte(byte);
    CS_HIGH;
}

void soft_spi_write_data(uint8_t byte)
{
    CS_LOW;
    DC_HIGH;
    soft_spi_write_byte(byte);
    CS_HIGH;
}

void soft_spi_write_data16(uint16_t data)
{
    soft_spi_write_data(data >> 8);
    soft_spi_write_data(data);
}

// static void SetAddress(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
// {

//     soft_spi_write_reg(0x2a);
//     soft_spi_write_data16(x_start);
//     soft_spi_write_data16(x_end);
//     soft_spi_write_reg(0x2b);
//     soft_spi_write_data16(y_start);
//     soft_spi_write_data16(y_end);
//     soft_spi_write_reg(0x2c);

// }

void soft_spi_lcd_test(void)
{

    uint32_t i, j, k = 0;
    SetAddress(0, 0, 319, 319);
    CS_LOW;
    DC_HIGH;
    for (i = 0; i < 320; i++)
    {
        for (j = 0; j < 320; j++)
        {
            soft_spi_write_byte(0x00);
            soft_spi_write_byte(0x00);
            soft_spi_write_byte(0xff);
        }
    }
    CS_HIGH;
}