#include <drv/pwm.h>
#include <drv/irq.h>
#include <bl606p_pwm.h>
#include <bl606p_glb.h>


#define PWM_CHANNAL_MAX 4
#define PWM_XCLK_CLK 40000000
#define PWM_DUTY_MAX 10000

uint16_t last_period_us = 0;

/**
  \brief       Initialize PWM Interface. Initializes the resources needed for the PWM interface
  \param[in]   pwm    pwm handle to operate
  \param[in]   idx    pwm idx
  \return      error code \ref csi_error_t
*/
csi_error_t csi_pwm_init(csi_pwm_t *pwm, uint32_t idx)
{
    CSI_PARAM_CHK(pwm, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if(idx > 1)
    {
        return CSI_ERROR;
    }
    (pwm->dev).idx = idx;

    return ret;
}

/**
  \brief       De-initialize PWM Interface. stops operation and releases the software resources used by the interface
  \param[in]   pwm    pwm handle to operate
  \return      None
*/
void csi_pwm_uninit(csi_pwm_t *pwm)
{
    CSI_PARAM_CHK_NORETVAL(pwm);

    PWMx_Disable((pwm->dev).idx);
    memset(pwm, 0, sizeof(csi_pwm_t));

    uint64_t pin_pos_0 = 0, pin_pos_1 = 0;
    GLB_GPIO_Cfg_Type cfg;
    GLB_GPIO_Type pin;

    cfg.drive = 1;
    cfg.smtCtrl = 1;
    cfg.gpioMode = GPIO_MODE_INPUT;
    cfg.pullType = GPIO_PULL_DOWN;

    for (pin = GLB_GPIO_PIN_0; pin < GLB_GPIO_PIN_MAX; pin++) {
        cfg.gpioFun = GLB_GPIO_Get_Fun(pin);
        if (cfg.gpioFun == GPIO_FUN_PWM0) {
            cfg.gpioFun = GPIO_FUN_GPIO;
            pin_pos_0 = (pin_pos_0 | (1 << pin));
        } else if (cfg.gpioFun == GPIO_FUN_PWM1) {
            cfg.gpioFun = GPIO_FUN_GPIO;
            pin_pos_1 = (pin_pos_1 | (1 << pin));
        } else {
            continue;
        }
        cfg.gpioPin = pin;
        GLB_GPIO_Init(&cfg);
    }

    GLB_AHB_MCU_Software_Reset(GLB_AHB_MCU_SW_PWM);

    for (pin = GLB_GPIO_PIN_0; pin < GLB_GPIO_PIN_MAX; pin++) {
        if (pin_pos_0 & (1 << pin)) {
            cfg.gpioFun = GPIO_FUN_PWM0;
        } else if (pin_pos_1 & (1 << pin)) {
            cfg.gpioFun = GPIO_FUN_PWM1;
        } else {
            continue;
        }
        cfg.gpioPin = pin;
        GLB_GPIO_Init(&cfg);
    }

    last_period_us = 0;
    return;
}

/**
  \brief       Config pwm out mode
  \param[in]   pwm               pwm handle to operate
  \param[in]   channel           channel num
  \param[in]   period_us         the PWM period in us
  \param[in]   pulse_width_us    the PMW pulse width in us
  \param[in]   polarity          the PWM polarity \ref csi_pwm_polarity_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_pwm_out_config(csi_pwm_t *pwm,
                               uint32_t  channel,
                               uint32_t period_us,
                               uint32_t pulse_width_us,
                               csi_pwm_polarity_t polarity)
{
    CSI_PARAM_CHK(pwm, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    uint16_t range;

    PWMx_CFG_Type pwmxCfg = {
        .clk = PWM_CLK_XCLK,
        .stopMode = PWM_STOP_GRACEFUL,
        .clkDiv = 40,
        .period = 0,
        .intPulseCnt = 0,
        .extPol = PWM_BREAK_Polarity_HIGH,
        .stpRept = DISABLE,
        .adcSrc = PWM_TRIGADC_SOURCE_NONE,
    };

    PWM_CHx_CFG_Type chxCfg = {
        .modP = PWM_MODE_ENABLE,
        .modN = PWM_MODE_ENABLE,
        .polP = PWM_POL_ACTIVE_HIGH,
        .polN = PWM_POL_ACTIVE_HIGH,
        .idlP = PWM_IDLE_STATE_INACTIVE,
        .idlN = PWM_IDLE_STATE_INACTIVE,
        .brkP = PWM_BREAK_STATE_INACTIVE,
        .brkN = PWM_BREAK_STATE_INACTIVE,
        .thresholdL = 0,
        .thresholdH = 0,
        .dtg = 0,
    };

    if((period_us == 0) || (pulse_width_us > period_us))
    {
        return CSI_ERROR;
    }

    range = period_us >> 16;

    if(last_period_us != period_us)
    {
        if(range > 1500)
        {
            return CSI_ERROR;
        }

        pwmxCfg.clkDiv = (range + 1) * 40;
        pwmxCfg.period = (period_us + (range + 1)/2)/(range + 1);
        PWMx_Disable((pwm->dev).idx);
        PWMx_Init((pwm->dev).idx, &pwmxCfg);
    }

    chxCfg.thresholdL = 0;
    chxCfg.thresholdH = (pulse_width_us + (range + 1)/2)/(range + 1);
    chxCfg.polP = (polarity ? PWM_POL_ACTIVE_LOW:PWM_POL_ACTIVE_HIGH);
    PWM_Channelx_Pwm_Mode_Set((pwm->dev).idx, channel, PWM_MODE_DISABLE, PWM_MODE_DISABLE);
    PWM_Channelx_Init((pwm->dev).idx, channel, &chxCfg);
    if(last_period_us != period_us)
    {
        last_period_us = period_us;
        PWMx_Enable((pwm->dev).idx);
    }
    return ret;
}

/**
  \brief       Start generate pwm signal
  \param[in]   pwm        pwm handle to operate
  \param[in]   channel    channel num
  \return      error code \ref csi_error_t
*/
csi_error_t csi_pwm_out_start(csi_pwm_t *pwm, uint32_t channel)
{
    CSI_PARAM_CHK(pwm, CSI_ERROR);

    PWM_Channelx_Pwm_Mode_Set((pwm->dev).idx, channel, PWM_MODE_ENABLE, PWM_MODE_ENABLE);

    return CSI_OK;
}

/**
  \brief       Stop generate pwm signal
  \param[in]   pwm        pwm handle to operate
  \param[in]   channel    channel num
  \return      None
*/
void csi_pwm_out_stop(csi_pwm_t *pwm, uint32_t channel)
{
    CSI_PARAM_CHK_NORETVAL(pwm);

    PWM_Channelx_Pwm_Mode_Set((pwm->dev).idx, channel, PWM_MODE_DISABLE, PWM_MODE_DISABLE);

    return;
}

csi_error_t csi_pwm_capture_config(csi_pwm_t *pwm, uint32_t channel, csi_pwm_capture_polarity_t polarity, uint32_t count)
{
    return CSI_ERROR;
}

csi_error_t csi_pwm_capture_start(csi_pwm_t *pwm, uint32_t channel)
{
    return CSI_ERROR;
}

void csi_pwm_capture_stop(csi_pwm_t *pwm, uint32_t channel)
{
    return;
}

csi_error_t csi_pwm_attach_callback(csi_pwm_t *pwm, void *callback, void *arg)
{
    return CSI_ERROR;
}

void csi_pwm_detach_callback(csi_pwm_t *pwm)
{
    return;
}
