/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     wj_adc.c
 * @brief    CSI Source File for ADC Driver
 * @version  V1.0
 * @date     2020-03-05
 ******************************************************************************/

#include <string.h>

#include <drv/adc.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <bl606p_common.h>
#include <bl606p_adc.h>
#include <bl606p_glb.h>
#include <bl_os_hal.h>

#define ADC_SOURCE_FREQ               (40000000)
#define ADC_DEFAULT_DIV               (10)
#define DEFUALT_ADC_OPERATION_TIMEOUT (3000U)
#define OVER_SAMPLE_RATE_1            (1)
#define OVER_SAMPLE_RATE_16           (16)
#define OVER_SAMPLE_RATE_64           (64)
#define OVER_SAMPLE_RATE_128          (128)
#define OVER_SAMPLE_RATE_256          (256)

#define ADC_CLK_SET_DUMMY_WAIT \
    {                          \
        __NOP();               \
        __NOP();               \
        __NOP();               \
        __NOP();               \
        __NOP();               \
        __NOP();               \
        __NOP();               \
        __NOP();               \
    }

typedef struct {
    uint16_t clk_div;
    uint16_t osr;
    uint16_t internal_div;
} adc_config_t;

static uint32_t adc_fifo_val[32];
static ADC_Result_Type adc_parse_val[32];

void adc_generic_notify_handler(void *arg) {

    csi_adc_t *adc = (csi_adc_t *)arg;
    // uint32_t adc_count = ADC_Get_FIFO_Count();
    uint32_t fifo_cnt = 0; //, read_length = 0, timeout_cnt = DEFUALT_ADC_OPERATION_TIMEOUT;

    if (adc->callback == NULL)
       return ;

    if (ADC_GetIntStatus(ADC_INT_POS_SATURATION) == SET && ADC_IntGetMask(ADC_INT_POS_SATURATION) == UNMASK) {
       ADC_IntClr(ADC_INT_POS_SATURATION);
    }

    if (ADC_GetIntStatus(ADC_INT_NEG_SATURATION) == SET && ADC_IntGetMask(ADC_INT_NEG_SATURATION) == UNMASK) {
       ADC_IntClr(ADC_INT_NEG_SATURATION);
    }

    if (ADC_GetIntStatus(ADC_INT_FIFO_UNDERRUN) == SET && ADC_IntGetMask(ADC_INT_FIFO_UNDERRUN) == UNMASK) {
        ADC_IntClr(ADC_INT_FIFO_UNDERRUN);
        /* get fifo cnt */
        fifo_cnt = ADC_Get_FIFO_Count();

        for (uint32_t j = 0; j < fifo_cnt; j++ ) {
            adc_fifo_val[j] = ADC_Read_FIFO();
        }
        if(adc->callback){
            adc->callback(adc, ADC_EVENT_ERROR, adc->arg);
        }
    }

    if (ADC_GetIntStatus(ADC_INT_FIFO_OVERRUN) == SET && ADC_IntGetMask(ADC_INT_FIFO_OVERRUN) == UNMASK) {
        ADC_IntClr(ADC_INT_FIFO_OVERRUN);
        /* get fifo cnt */
        fifo_cnt = ADC_Get_FIFO_Count();

        for (uint32_t j = 0; j < fifo_cnt; j++ ) {
            adc_fifo_val[j] = ADC_Read_FIFO();
        }
        if (adc->callback) {
            adc->callback(adc, ADC_EVENT_ERROR, adc->arg);
        }
    }

    if (ADC_GetIntStatus(ADC_INT_ADC_READY) == SET && ADC_IntGetMask(ADC_INT_ADC_READY) == UNMASK) {
        static uint32_t cnt = 0;
        /* get fifo cnt */
        fifo_cnt = ADC_Get_FIFO_Count();

        for (uint32_t j = 0; j < fifo_cnt; j++ ) {
            adc_fifo_val[j] = ADC_Read_FIFO();
        }

        fifo_cnt = ((adc->num - cnt) < fifo_cnt) ? (adc->num - cnt) : fifo_cnt;

        ADC_Parse_Result(adc_fifo_val, fifo_cnt, (ADC_Result_Type *)adc_parse_val);

        if (!(adc->state.writeable) && !(adc->state.writeable)) {
            for (uint32_t n = 0; n < fifo_cnt; n ++) {
                adc->data[cnt++] = adc_parse_val[n].value;
            }
        }

        if (cnt >= adc->num) {
            if ( adc->num && adc->callback){
                cnt = 0;
                adc->num = 0;
                adc->callback(adc, ADC_EVENT_CONVERT_COMPLETE, adc->arg);
            }
        }
        ADC_IntClr(ADC_INT_ADC_READY);
    }

}

csi_error_t csi_adc_init(csi_adc_t *adc, uint32_t idx)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    adc_config_t *cfg;

    adc->priv = (void*)bl_os_malloc(sizeof(adc_config_t));
    if (NULL == adc->priv) {
        return CSI_ERROR;
    }

    if (idx != 0) {
        return CSI_ERROR;
    }

    cfg = (adc_config_t*)(adc->priv);
    memset(cfg, 0, sizeof(adc_config_t));

    // csi_irq_disable((uint32_t)adc->dev.irq_num);
    ADC_IntMask(ADC_INT_ALL, MASK);

    ADC_FIFO_Cfg_Type adcFifoCfg = {
        .fifoThreshold = ADC_FIFO_THRESHOLD_1,
        .dmaEn = DISABLE,
    };

    ADC_CFG_Type adcCfg = {
        .v18Sel = ADC_V18_SEL_1P82V,
        .v11Sel = ADC_V11_SEL_1P1V,
        .clkDiv = ADC_CLK_DIV_4,
        .gain1 = ADC_PGA_GAIN_1,
        .gain2 = ADC_PGA_GAIN_1,
        .chopMode = ADC_CHOP_MOD_AZ_PGA_ON,
        .biasSel = ADC_BIAS_SEL_MAIN_BANDGAP,
        .vcm = ADC_PGA_VCM_1P2V,
        .vref = ADC_VREF_3P2V,
        .inputMode = ADC_INPUT_SINGLE_END,
        .resWidth = ADC_DATA_WIDTH_12,
        .offsetCalibEn = 0,
        .offsetCalibVal = 0,
    };

    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_GPIP);
    GLB_Set_ADC_CLK(ENABLE, GLB_ADC_CLK_XCLK, ADC_DEFAULT_DIV - 1);

    /* default over-sample rate as 1 */
    cfg->clk_div = ADC_DEFAULT_DIV;
    cfg->osr = 1;
    cfg->internal_div = 4;

    ADC_Disable();
    ADC_Enable();
    ADC_Reset();
    ADC_Init(&adcCfg);
    //ADC_Channel_Config(ADC_CHAN6, ADC_CHAN_GND, 1);
    ADC_FIFO_Cfg(&adcFifoCfg);

    return ret;
}

void csi_adc_uninit(csi_adc_t *adc)
{
    CSI_PARAM_CHK_NORETVAL(adc);

    ADC_Disable();
    memset(adc, 0, sizeof(csi_adc_t));
}

csi_error_t csi_adc_set_buffer(csi_adc_t *adc, uint32_t *data, uint32_t num)
{
    csi_error_t ret = CSI_OK;
    CSI_PARAM_CHK(adc, CSI_ERROR);
    CSI_PARAM_CHK(data, CSI_ERROR);
    CSI_PARAM_CHK(num, CSI_ERROR);

    if (num == 0U) {
        ret = CSI_ERROR;
    }else{
        adc->data = data;
        adc->num = num;
        ret = CSI_OK;
    }

    return ret;
}

csi_error_t csi_adc_start(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    csi_error_t   ret = CSI_OK;

    ADC_Start();

    return ret;
}

csi_error_t csi_adc_start_async(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    csi_error_t   ret = CSI_OK;

    do{
        if ((adc->data == NULL) || (adc->num == 0U)) {
            ret = CSI_ERROR;
            break;
        }

        // /* rx buffer not full */
        // if (adc->state.readable == 0U) {
        //     ret = CSI_BUSY;
        //     break;
        // }

        // /* last conversion existed */
        // if (adc->state.writeable == 0U) {
        //     ret = CSI_ERROR;
        //     break;
        // }

        if (adc->start) {
            adc->state.writeable = 0U;
            adc->state.readable  = 0U;
            adc->start(adc);
        }
    }while(0);

    return ret;
}

csi_error_t csi_adc_stop(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    csi_error_t   ret = CSI_OK;

    uint32_t tmpVal;
    ADC_Stop();

    tmpVal = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, AON_GPADC_CONT_CONV_EN, 0);
    BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, tmpVal);

    tmpVal = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
    tmpVal = BL_CLR_REG_BIT(tmpVal, AON_GPADC_SCAN_EN);
    BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, tmpVal);

    ADC_Reset();

    return ret;
}

csi_error_t csi_adc_stop_async(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    if (adc->stop) {
        adc->stop(adc);
        adc->state.readable  = 1U;
        adc->state.writeable = 1U;
        adc->data = NULL;
        adc->num = 0U;
    } else {
        ret = CSI_ERROR;
    }

    return ret;
}

csi_error_t csi_adc_channel_enable(csi_adc_t *adc, uint8_t ch_id, bool is_enable)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if(ch_id == ADC_CHAN5)
    {
        HBN_Set_IO4041_As_Xtal_32K_IO(false);
    }

    // uint32_t regCmd;
    // uint32_t regCfg1;

    uint32_t tmpVal;

    CHECK_PARAM(IS_AON_ADC_CHAN_TYPE(ch_id));

    // regCmd = BL_RD_REG(AON_BASE, AON_GPADC_REG_CMD);
    // regCmd = BL_SET_REG_BITS_VAL(regCmd, AON_GPADC_POS_SEL, ch_id);
    // regCmd = BL_SET_REG_BITS_VAL(regCmd, AON_GPADC_NEG_SEL, ADC_CHAN_GND);
    // BL_WR_REG(AON_BASE, AON_GPADC_REG_CMD, regCmd);

    // regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
    // regCfg1 = BL_CLR_REG_BIT(regCfg1, AON_GPADC_SCAN_EN);
    // BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);

    tmpVal = BL_RD_REG(AON_BASE, AON_GPADC_REG_SCN_POS1);
    tmpVal = tmpVal & (~(0x1F));
    tmpVal |= (ch_id );
    tmpVal = tmpVal & (~(0x1F << (5)));
    tmpVal |= (ch_id << (5));
    BL_WR_REG(AON_BASE, AON_GPADC_REG_SCN_POS1, tmpVal);

    tmpVal = BL_RD_REG(AON_BASE, AON_GPADC_REG_SCN_NEG1);
    tmpVal = tmpVal & (~(0x1F));
    tmpVal |= (ADC_CHAN_GND );
    tmpVal = tmpVal & (~(0x1F << (5)));
    tmpVal |= (ADC_CHAN_GND << (5));
    BL_WR_REG(AON_BASE, AON_GPADC_REG_SCN_NEG1, tmpVal);

    /* Scan mode */
    tmpVal = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, AON_GPADC_SCAN_LENGTH, 1);
    tmpVal = BL_SET_REG_BIT(tmpVal, AON_GPADC_CLK_ANA_INV);
    tmpVal = BL_SET_REG_BIT(tmpVal, AON_GPADC_SCAN_EN);
    BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, tmpVal);

    return ret;
}

csi_error_t csi_adc_channel_sampling_time(csi_adc_t *adc, uint8_t ch_id, uint16_t clock_num)
{
    return CSI_ERROR;
}

csi_error_t csi_adc_sampling_time(csi_adc_t *adc, uint16_t clock_num)
{
    // FIXME:
    return CSI_OK;
}

/* div support 1, 16, 64, 128, 256
 * 1->12bit
 * 16->14bit
 * 64->14bit
 * 126->16bit
 * 256->16bit*/
uint32_t csi_adc_freq_div(csi_adc_t *adc, uint32_t div)
{
    uint32_t freq;
    uint32_t regCfg1;
    adc_config_t *cfg;

    if (div != OVER_SAMPLE_RATE_1 && div != OVER_SAMPLE_RATE_16 && div != OVER_SAMPLE_RATE_64 &&
        div != OVER_SAMPLE_RATE_128 && div != OVER_SAMPLE_RATE_256) {
        return 0;
    }

    cfg = (adc_config_t*)(adc->priv);

    switch (div) {
        case OVER_SAMPLE_RATE_1:
            regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
            regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_RES_SEL, 0);
            BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
            ADC_CLK_SET_DUMMY_WAIT;
            cfg->osr = OVER_SAMPLE_RATE_1;
            break;
        case OVER_SAMPLE_RATE_16:
            regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
            regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_RES_SEL, 1);
            BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
            ADC_CLK_SET_DUMMY_WAIT;
            cfg->osr = OVER_SAMPLE_RATE_16;
            break;
        case OVER_SAMPLE_RATE_64:
            regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
            regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_RES_SEL, 2);
            BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
            ADC_CLK_SET_DUMMY_WAIT;
            cfg->osr = OVER_SAMPLE_RATE_64;
            break;
        case OVER_SAMPLE_RATE_128:
            regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
            regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_RES_SEL, 3);
            BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
            ADC_CLK_SET_DUMMY_WAIT;
            cfg->osr = OVER_SAMPLE_RATE_128;
            break;
        case OVER_SAMPLE_RATE_256:
            regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
            regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_RES_SEL, 4);
            BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
            ADC_CLK_SET_DUMMY_WAIT;
            cfg->osr = OVER_SAMPLE_RATE_256;
            break;
        default:
            break;
    }

    freq = ADC_SOURCE_FREQ / (cfg->clk_div * cfg->internal_div * cfg->osr);

    return freq;
}

int32_t csi_adc_read(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    uint32_t timestart;
    uint32_t val;
    ADC_Result_Type result;

    timestart = csi_tick_get_ms();
    while(ADC_Get_FIFO_Count() < 2) {
        if((csi_tick_get_ms() - timestart) > DEFUALT_ADC_OPERATION_TIMEOUT) {
            return CSI_TIMEOUT;
        }
    }

    ADC_Read_FIFO();
    val = ADC_Read_FIFO();
    if (val) {
        ADC_Parse_Result(&val, 1, &result);
        // printf("result p:%d val:%d\r\n", (uint32_t)result.posChan, (uint32_t)(result.value));
        // printf("result volt:%d\r\n", (uint32_t)(result.volt));
    }

    return (uint32_t)(result.value);
}

csi_error_t csi_adc_get_state(csi_adc_t *adc, csi_state_t *state)
{
    return CSI_ERROR;
}

uint32_t csi_adc_get_freq(csi_adc_t *adc)
{
    CSI_PARAM_CHK(adc, 0U);
    uint32_t freq;
    adc_config_t *cfg;

    cfg = (adc_config_t*)(adc->priv);
    if (0 == cfg->clk_div || 0 == cfg->osr || 0 == cfg->internal_div) {
        return CSI_ERROR;
    }

   // printf("freq:%ld clk:%d cycles:%d internal:%d\r\n", ADC_SOURCE_FREQ, cfg->clk_div, cfg->cycles, cfg->internal_div);

    freq = ADC_SOURCE_FREQ / (cfg->clk_div * cfg->internal_div * cfg->osr);

    return freq;
}

csi_error_t csi_adc_attach_callback(csi_adc_t *adc, void *callback, void *arg)
{
    CSI_PARAM_CHK(adc, CSI_ERROR);
    CSI_PARAM_CHK(callback, CSI_ERROR);

    adc->callback = callback;
    adc->arg      = arg;
    adc->start    = csi_adc_start;
    adc->stop     = csi_adc_stop;
    adc->dev.irq_num = GPADC_DMA_IRQn;

    ADC_IntMask(ADC_INT_ALL, MASK);

    csi_irq_attach((uint32_t)adc->dev.irq_num, &adc_generic_notify_handler, &adc->dev);
    csi_irq_enable((uint32_t)adc->dev.irq_num);

    ADC_IntMask(ADC_INT_ADC_READY, UNMASK);
    ADC_IntMask(ADC_INT_FIFO_OVERRUN, UNMASK);
    return CSI_OK;
}

csi_error_t csi_adc_continue_mode(csi_adc_t *adc, bool is_enable)
{
    uint32_t regCfg1;
    regCfg1 = BL_RD_REG(AON_BASE, AON_GPADC_REG_CONFIG1);
    regCfg1 = BL_SET_REG_BITS_VAL(regCfg1, AON_GPADC_CONT_CONV_EN, is_enable);
    BL_WR_REG(AON_BASE, AON_GPADC_REG_CONFIG1, regCfg1);
    return CSI_OK;
}

void csi_adc_detach_callback(csi_adc_t *adc)
{
    CSI_PARAM_CHK_NORETVAL(adc);

    csi_irq_disable((uint32_t)adc->dev.irq_num);

    adc->callback  = NULL;
    adc->arg       = NULL;
    adc->start     = NULL;
    adc->stop      = NULL;
}

csi_error_t csi_adc_link_dma(csi_adc_t *adc, csi_dma_ch_t *dma)
{
    return CSI_ERROR;
}
