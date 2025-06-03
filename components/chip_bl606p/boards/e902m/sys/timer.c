/*
 *  Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/*******************************************************
 * @file    dw_timer.c
 * @brief   source file for timer csi driver
 * @version V1.0
 * @date    23. Sep 2020
 * ******************************************************/

// #include <csi_config.h>
#include <drv/timer.h>
#include <drv/irq.h>
// #include "dw_timer_ll.h"

#define DW_TIMER_GET_RELOAD_VAL(_tim_, _frq_)      ((_tim_ < 25000U) ? ((_frq_ * _tim_) / 1000U) : (_frq_ * (_tim_ / 1000U)))

/**
  \brief       Timer interrupt handling function
  \param[in]   arg    Callback function member variables
  \return      None
*/
void dw_timer_irq_handler(void *arg)
{

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

}
/**
  \brief       Get timer remaining value
  \param[in]   timer    handle timer handle to operate
  \return      the remaining value
*/
uint32_t csi_timer_get_remaining_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, 0U);

    return 0;
}
/**
  \brief       Get timer load value
  \param[in]   timer    handle timer handle to operate
  \return      the load value
*/
uint32_t csi_timer_get_load_value(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, 0U);

    return 0;
}
/**
  \brief       Check timer is running
  \param[in]   timer    handle timer handle to operate
  \return      true->running, false->stopped
*/
bool csi_timer_is_running(csi_timer_t *timer)
{
    CSI_PARAM_CHK(timer, false);

    return false;
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

    return CSI_OK;
}

/**
  \brief       Detach the callback handler
  \param[in]   timer    operate handle.
*/
void csi_timer_detach_callback(csi_timer_t *timer)
{
    CSI_PARAM_CHK_NORETVAL(timer);

}

#ifdef CONFIG_PM
csi_error_t dw_timer_pm_action(csi_dev_t *dev, csi_pm_dev_action_t action)
{
    CSI_PARAM_CHK(dev, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    return ret;
}

csi_error_t csi_timer_enable_pm(csi_timer_t *timer)
{
    return 0;
}

void csi_timer_disable_pm(csi_timer_t *timer)
{

}
#endif

