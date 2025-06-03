#include "hosal_adc.h"
#include <bl606p_gpio.h>
#include <bl606p_adc.h>
#include <bl606p_glb.h>
#include <bl606p_hbn.h>
#include "aos/kernel.h"

#define ADC_GPIO_NUM    (12)

static int channel_num = 0;
static uint8_t channel_table[ADC_GPIO_NUM] = {0};
static int start_flag = 1;

static int _adc_get_channel_by_gpio(GLB_GPIO_Type pin)
{
    int channel = -1;

    switch (pin) {
        case GLB_GPIO_PIN_17:
            channel = 0;
            break;
        case GLB_GPIO_PIN_5:
            channel = 1;
            break;
        case GLB_GPIO_PIN_4:
            channel = 2;
            break;
        case GLB_GPIO_PIN_11:
            channel = 3;
            break;
        case GLB_GPIO_PIN_6:
            channel = 4;
            break;
        case GLB_GPIO_PIN_40:
            channel = 5;
            break;
        case GLB_GPIO_PIN_12:
            channel = 6;
            break;
        case GLB_GPIO_PIN_13:
            channel = 7;
            break;
        case GLB_GPIO_PIN_16:
            channel = 8;
            break;
        case GLB_GPIO_PIN_18:
            channel = 9;
            break;
        case GLB_GPIO_PIN_19:
            channel = 10;
            break;
        case GLB_GPIO_PIN_34:
            channel = 11;
            break;
        
        default :
            channel = -1;
            break;
    }

    return channel;
}

static int _adc_get_gpio_by_channel(int channel)
{
    int pin = -1;

    switch (channel) {
        case 0:
            pin = GLB_GPIO_PIN_17;
            break;
        case 1:
            pin = GLB_GPIO_PIN_5;
            break;
        case 2:
            pin = GLB_GPIO_PIN_4;
            break;
        case 3:
            pin = GLB_GPIO_PIN_11;
            break;
        case 4:
            pin = GLB_GPIO_PIN_6;
            break;
        case 5:
            pin = GLB_GPIO_PIN_40;
            break;
        case 6:
            pin = GLB_GPIO_PIN_12;
            break;
        case 7:
            pin = GLB_GPIO_PIN_13;
            break;
        case 8:
            pin = GLB_GPIO_PIN_16;
            break;
        case 9:
            pin = GLB_GPIO_PIN_18;
            break;
        case 10:
            pin = GLB_GPIO_PIN_19;
            break;
        case 11:
            pin = GLB_GPIO_PIN_34;
            break;
        
        default :
            pin = -1;
            break;
    }

    return pin;
}

static void _adc_init(hosal_adc_dev_t *adc)
{
    ADC_CFG_Type adcCfg = {
        .v18Sel = ADC_V18_SEL_1P82V,                    /*!< ADC 1.8V select */
        .v11Sel = ADC_V11_SEL_1P1V,                     /*!< ADC 1.1V select */
        .clkDiv = ADC_CLK_DIV_20,                       /*!< Clock divider */
        .gain1 = ADC_PGA_GAIN_1,                        /*!< PGA gain 1 */
        .gain2 = ADC_PGA_GAIN_1,                        /*!< PGA gain 2 */
        .chopMode = ADC_CHOP_MOD_AZ_PGA_ON,             /*!< ADC chop mode select */
        .biasSel = ADC_BIAS_SEL_MAIN_BANDGAP,           /*!< ADC current form main bandgap or aon bandgap */
        .vcm = ADC_PGA_VCM_1P4V,                        /*!< ADC VCM value */
        .vref = ADC_VREF_3P2V,                          /*!< ADC voltage reference */
        .inputMode = ADC_INPUT_SINGLE_END,              /*!< ADC input signal type */
        .resWidth = ADC_DATA_WIDTH_16_WITH_256_AVERAGE, /*!< ADC resolution and oversample rate */
        .offsetCalibEn = 0,                             /*!< Offset calibration enable */
        .offsetCalibVal = 0,                            /*!< Offset calibration value */
    };

    ADC_FIFO_Cfg_Type adcFifoCfg = {
        .fifoThreshold = ADC_FIFO_THRESHOLD_1,
        .dmaEn = DISABLE,
    };
    
    int chan = -1;
    GLB_GPIO_Type pin = adc->config.pin;

    ADC_Disable();
    ADC_Enable();

    ADC_Reset();

    ADC_Init(&adcCfg);
    
    chan = _adc_get_channel_by_gpio(pin);
    //ADC_Channel_Config(chan, ADC_CHAN_GND, 1);
    channel_table[channel_num] = chan; 
    channel_num++;
    ADC_FIFO_Cfg(&adcFifoCfg);

    //ADC_Start();
}

static int _adc_check_gpio_valid(GLB_GPIO_Type pin)
{
    int i;

    GLB_GPIO_Type gpio_arr[ADC_GPIO_NUM] =
    {
        GLB_GPIO_PIN_17, GLB_GPIO_PIN_5, GLB_GPIO_PIN_4,
        GLB_GPIO_PIN_11, GLB_GPIO_PIN_6, GLB_GPIO_PIN_40,
        GLB_GPIO_PIN_12, GLB_GPIO_PIN_13, GLB_GPIO_PIN_16,
        GLB_GPIO_PIN_18, GLB_GPIO_PIN_19, GLB_GPIO_PIN_34
    };

    for (i = 0; i < ADC_GPIO_NUM; i++) {
        if (pin == gpio_arr[i]) {
            return 0;
        }
    }

    printf("gpio %d can not used as adc\r\n", pin);

    return -1;
}


static void _adc_gpio_init(GLB_GPIO_Type pin)
{
    GLB_GPIO_Func_Init(GPIO_FUN_ANALOG, &pin, 1);
}

int hosal_adc_init(hosal_adc_dev_t *adc)
{
    int ret = -1;

    if (NULL == adc) {
        printf("error!\r\n");
    }
    
    GLB_GPIO_Type adc_pin = adc->config.pin;
    
    ret = _adc_check_gpio_valid(adc_pin);
    if (ret) {
        printf("error\r\n");
        return ret;
    }

    _adc_get_channel_by_gpio(adc_pin);
    _adc_gpio_init(adc_pin);
    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_GPIP);
    GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_XCLK, 0);
    _adc_init(adc);

    start_flag = 1;

    return 0;
}


int hosal_adc_add_channel(hosal_adc_dev_t *adc, uint32_t channel)
{
    int pin = -1;
    int i = 0, flag = 0;

    ADC_Chan_Type pos_chlist_single[ADC_GPIO_NUM];
    ADC_Chan_Type neg_chlist_single[ADC_GPIO_NUM];
    
    if (NULL == adc) {
        printf("error!\r\n");
        return -1;
    }
    

    if(channel == ADC_CHAN5) {
        HBN_Set_IO4041_As_Xtal_32K_IO(false);
    }

    pin = _adc_get_gpio_by_channel(channel);
    if (pin < 0) {
        printf("not support channel\r\n");
        return -1;
    }
   
    
    for (i = 0; i < channel_num; i++) {
        if (channel_table[i] == channel) {
            flag = 1;
        }
    }
    
    if (!flag) {
        channel_table[channel_num] = channel; 
        _adc_gpio_init(pin);
        channel_num++;
    }
    
    for (i = 0; i < channel_num; i++) {
        pos_chlist_single[i] = channel_table[i]; 
        printf("channel:%d\r\n", channel_table[i]);
        neg_chlist_single[i] = ADC_CHAN_GND;
    }
    printf("num:%d\r\n", channel_num);
    
    ADC_Scan_Channel_Config(pos_chlist_single, neg_chlist_single, channel_num, 1);
    if (start_flag) {
        ADC_Start();
        start_flag = 0;
    }
    return 0;
}

int hosal_adc_value_get(hosal_adc_dev_t *adc, uint32_t channel, uint32_t timeout)
{
    
    if (NULL == adc) {
        printf("error\r\n");
        return -1;
    }
    ADC_Result_Type result[ADC_GPIO_NUM];
    uint32_t reg_val = 0;
    int val = -1;
    int i = 0;
    
    //memset(result, 0, ADC_GPIO_NUM);
    for (i = 0; i < ADC_GPIO_NUM; i++) {
        result[i].posChan = 255;
        ADC_Get_FIFO_Count();

        //printf("data_num:%u\r\n", ret);
        if (ADC_Get_FIFO_Count() == 0) {
            printf("111\r\n");
            return -1;
        }
        ADC_FIFO_Clear();
        aos_msleep(1);
        reg_val = ADC_Read_FIFO();
        
        if (reg_val) {
            ADC_Parse_Result(&reg_val, 1, result);
        }
        
        //printf("ii:%d, result[i].posChan:%u\r\n",i, result[0].posChan);
        if (channel == result[0].posChan) {
            val = (int)(result[0].volt * 1000);
            //printf("valdddd:%d\r\n", val);
            return val;
        }
    }
    return -1;
}

int hosal_adc_finalize(hosal_adc_dev_t *adc)
{
    channel_num = 0;
    memset(channel_table, 0 , ADC_GPIO_NUM);
    ADC_Stop();
    return 0;
}
