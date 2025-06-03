/*
 *  Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/*******************************************************
 * @file    dw_timer.c
 * @brief   source file for timer csi driver
 * @version V1.0
 * @date    23. Sep 2020
 * ******************************************************/

#include <csi_config.h>
#include <drv/timer.h>
#include <drv/irq.h>
#include <bl606p_timer.h>
#include <bl606p_glb.h>
#include <utils_list.h>

#define TIMER_CLK_FREQ    40000000
#define TIMER_PRELOAD_NO  TIMER_COMP_ID_0

/**
  \brief       Timer interrupt handling function
  \param[in]   arg    Callback function member variables
  \return      None
*/
void timer_irq_handler(void *arg)
{
    csi_dev_t *pdev = (csi_dev_t *)arg;
    csi_timer_t *ptimer;
    TIMER_ID_Type time_id;
    TIMER_Chan_Type time_ch;

    ptimer = (csi_timer_t*)utils_container_of(pdev, csi_timer_t, dev);

    if(ptimer->dev.idx / 2 == 0){
        time_id = TIMER0_ID;
    } else {
        time_id = TIMER1_ID;
    }

    if(ptimer->dev.idx % 2 == 0){
        time_ch = TIMER_CH0;
    } else {
        time_ch = TIMER_CH1;
    }

    if (ptimer->callback) {
        ptimer->callback(ptimer, ptimer->arg);
    }

    TIMER_ClearIntStatus(time_id, time_ch, TIMER_PRELOAD_NO);

    return;
}
/**
  \brief       Initialize TIMER Interface. 1. Initializes the resources needed for the TIMER interface 2.registers callback function
  \param[in]   timer    handle timer handle to operate
  \param[in]   idx      timer index
  \return      error code \ref csi_error_t
*/
csi_error_t csi_timer_init(csi_timer_t *timer, uint32_t idx)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (idx > 3) {
        return CSI_ERROR;
    }

    timer->dev.idx = idx;

    return ret;
}
/**
  \brief       De-initialize TIMER Interface. stops operation and releases the software resources used by the interface
  \param[in]   timer    handle timer handle to operate
  \return      None
*/
void csi_timer_uninit(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);
}
/**
  \brief       Start timer
  \param[in]   timer         handle timer handle to operate
  \param[in]   timeout_us    the timeout for timer
  \return      error code \ref csi_error_t
*/
csi_error_t csi_timer_start(csi_timer_t *timer, uint32_t timeout_us)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);
    CSI_PARAM_CHK(timeout_us, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    TIMER_ID_Type time_id;

    TIMER_CFG_Type timerCfg = {
        .clkSrc = TIMER_CLKSRC_XTAL,
        .plTrigSrc = TIMER_PRELOAD_NO + 1,
        .countMode = TIMER_COUNT_PRELOAD,
        .clockDivision = (TIMER_CLK_FREQ / 1000000) - 1,
        .matchVal0 = timeout_us,
        .matchVal1 = 0xFFFFFFFF,
        .matchVal2 = 0xFFFFFFFF,
        .preLoadVal = 0,
    };

    if(timer->dev.idx / 2 == 0){
        time_id = TIMER0_ID;
        timer->dev.reg_base = TIMER0_BASE;
    } else {
        time_id = TIMER1_ID;
        timer->dev.reg_base = TIMER1_BASE;
    }

    if(timer->dev.idx % 2 == 0){
        timerCfg.timerCh = TIMER_CH0;
    } else {
        timerCfg.timerCh = TIMER_CH1;
    }

    TIMER_IntMask(time_id, timerCfg.timerCh, TIMER_INT_ALL, MASK);
    TIMER_Disable(time_id, timerCfg.timerCh);
    TIMER_Init(time_id, &timerCfg);

    TIMER_IntMask(time_id, timerCfg.timerCh, TIMER_PRELOAD_NO, UNMASK);

    TIMER_Enable(time_id, timerCfg.timerCh);

    return ret;
}

/**
  \brief       Stop timer
  \param[in]   timer    handle timer handle to operate
  \return      None
*/
void csi_timer_stop(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);

    TIMER_ID_Type time_id;
    TIMER_Chan_Type time_ch;

    if(timer->dev.idx / 2 == 0){
        time_id = TIMER0_ID;
    } else {
        time_id = TIMER1_ID;
    }

    if(timer->dev.idx % 2 == 0){
        time_ch = TIMER_CH0;
    } else {
        time_ch = TIMER_CH1;
    }

    TIMER_Disable(time_id, time_ch);

    return;
}

/**
  \brief       Get timer remaining value
  \param[in]   timer    handle timer handle to operate
  \return      the remaining value
*/
uint32_t csi_timer_get_remaining_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, 0U);

    TIMER_ID_Type time_id;
    TIMER_Chan_Type time_ch;
    uint32_t counter;
    uint32_t compare;

    if(timer->dev.idx / 2 == 0){
        time_id = TIMER0_ID;
    } else {
        time_id = TIMER1_ID;
    }

    if(timer->dev.idx % 2 == 0){
        time_ch = TIMER_CH0;
    } else {
        time_ch = TIMER_CH1;
    }

    counter = TIMER_GetCounterValue(time_id, time_ch);
    compare = TIMER_GetCompValue(time_id, time_ch, TIMER_PRELOAD_NO) + 2;

    if (counter > compare) {
        return 0;
    }

    return (compare - counter);
}

/**
  \brief       Get timer load value
  \param[in]   timer    handle timer handle to operate
  \return      the load value
*/
uint32_t csi_timer_get_load_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, 0U);

    TIMER_ID_Type time_id;
    TIMER_Chan_Type time_ch;

    if(timer->dev.idx / 2 == 0){
        time_id = TIMER0_ID;
    } else {
        time_id = TIMER1_ID;
    }

    if(timer->dev.idx % 2 == 0){
        time_ch = TIMER_CH0;
    } else {
        time_ch = TIMER_CH1;
    }

    return (TIMER_GetCompValue(time_id, time_ch, TIMER_PRELOAD_NO) + 2);
}

/**
  \brief       get time run status
  \param[in]   timer    handle timer handle to operate
  \return      the run status
*/
bool csi_timer_is_running(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, 0U);

    uint32_t tmpVal;

    tmpVal = BL_RD_REG(timer->dev.reg_base, TIMER_TCER);

    if(timer->dev.idx % 2 == 0){
        tmpVal &= 0x01 << 1;
    } else {
        tmpVal &= 0x01 << 2;
    }

    return (tmpVal) ? true : false;
}

/**
  \brief       Attach the callback handler to timer
  \param[in]   timer       operate handle.
  \param[in]   callback    callback function
  \param[in]   arg         callback's param
  \return      error code \ref csi_error_t
*/
csi_error_t csi_timer_attach_callback(csi_timer_t *timer, void *callback, void *arg)
{
    CSI_PARAM_CHK(timer, CSI_ERROR);

    uint32_t irq_idx = 0;

    if(timer->dev.idx >= 2){
        return CSI_UNSUPPORTED;
    }

    switch (timer->dev.idx){
        case 0:
            irq_idx = TIMER0_CH0_IRQn;
            break;
        case 1:
            irq_idx = TIMER0_CH1_IRQn;
            break;
        case 2:
            irq_idx = TIMER1_CH0_IRQn;
            break;
        case 3:
            irq_idx = TIMER1_CH1_IRQn;
            break;
        default:
            break;
    }

    timer->callback = callback;
    timer->arg = arg;
    csi_irq_attach(irq_idx, &timer_irq_handler, &timer->dev);
    csi_irq_enable(irq_idx);

    return CSI_OK;
}

/**
  \brief       Detach the callback handler
  \param[in]   timer    operate handle.
*/
void csi_timer_detach_callback(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);
    uint32_t irq_idx = 0;

    if(timer->dev.idx >= 2){
        return;
    }

    switch (timer->dev.idx){
        case 0:
            irq_idx = TIMER0_CH0_IRQn;
            break;
        case 1:
            irq_idx = TIMER0_CH1_IRQn;
            break;
        case 2:
            irq_idx = TIMER1_CH0_IRQn;
            break;
        case 3:
            irq_idx = TIMER1_CH1_IRQn;
            break;
        default:
            break;
    }

    timer->callback = NULL;
    timer->arg = NULL;
    csi_irq_disable(irq_idx);
    csi_irq_detach(irq_idx);

    return;
}
