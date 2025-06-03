/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */
/******************************************************************************
 * @file     tick.c
 * @brief    Source File for tick
 * @version  V1.0
 * @date     7. April 2020
 ******************************************************************************/

#include <soc.h>
#include <csi_config.h>
#include <sys_clk.h>
#include <drv/common.h>
#include <drv/irq.h>
#include <drv/tick.h>
#include <drv/porting.h>
#include <drv/timer.h>
#include <csi_core.h>

#define __WEAK         __attribute__((weak))

#ifndef CONFIG_TICK_TIMER_IDX
#define CONFIG_TICK_TIMER_IDX   0U
#endif

#ifdef CONFIG_TICK_USE_CORETIME
static csi_dev_t tick_dev;
#else
static csi_timer_t tick_timer;
#endif

static volatile uint32_t csi_tick = 0U;
static volatile uint32_t last_time_ms = 0U;
static volatile uint64_t last_time_us = 0U;

void csi_tick_increase(void)
{
    csi_tick++;
}


static void tick_irq_handler(csi_timer_t *timer_handle, void *arg)
{
    csi_tick_increase();
    csi_coret_config((soc_get_coretim_freq()/ CONFIG_SYSTICK_HZ), CORET_IRQn);
#if defined(CONFIG_KERNEL_RHINO)
    extern void krhino_tick_proc(void);
    krhino_tick_proc();
#elif defined(CONFIG_KERNEL_FREERTOS)
    xPortSysTickHandler();
#elif defined(CONFIG_KERNEL_UCOS)
    OSTimeTick();
#endif
}

csi_error_t csi_tick_init(void)
{
#ifdef CONFIG_TICK_USE_CORETIME
    csi_tick = 0U;
    tick_dev.irq_num = (uint8_t)CORET_IRQn;

    csi_vic_set_prio(CORET_IRQn, 0U);

    csi_irq_attach((uint32_t)tick_dev.irq_num, &tick_irq_handler, &tick_dev);
    csi_coret_config((soc_get_coretim_freq()/ CONFIG_SYSTICK_HZ), CORET_IRQn);
    csi_irq_enable((uint32_t)tick_dev.irq_num);

    return CSI_OK;
#else
    csi_error_t ret;

    csi_tick = 0U;
    ret = csi_timer_init(&tick_timer, CONFIG_TICK_TIMER_IDX);

    if (ret == CSI_OK) {
        ret = csi_timer_attach_callback(&tick_timer, tick_irq_handler, NULL);

        if (ret == CSI_OK) {
            ret = csi_timer_start(&tick_timer, (1000000U / CONFIG_SYSTICK_HZ));
        }
    }

    return ret;
#endif
}

void csi_tick_uninit(void)
{
#ifdef CONFIG_TICK_USE_CORETIME
    csi_irq_disable((uint32_t)tick_dev.irq_num);
    csi_irq_detach((uint32_t)tick_dev.irq_num);
#else
    csi_timer_stop(&tick_timer);
    csi_timer_uninit(&tick_timer);
#endif
}

uint32_t csi_tick_get(void)
{
    return csi_tick;
}

uint32_t csi_tick_get_ms(void)
{
    uint32_t time;
    time = ((((uint64_t)csi_coret_get_valueh() << 32U) | csi_coret_get_value())) / (soc_get_coretim_freq() / 1000U);
    return time;
}

uint64_t csi_tick_get_us(void)
{
    uint64_t time;
    time = ((((uint64_t)csi_coret_get_valueh() << 32U) | csi_coret_get_value()) * 1000U) / (soc_get_coretim_freq() / 1000U);
    return time;
}


#ifdef CONFIG_TICK_USE_CORETIME
static void _500usdelay(void)
{
    uint32_t load = csi_coret_get_load();
    uint32_t start = csi_coret_get_value();
    uint32_t cur;
    uint32_t cnt = (soc_get_coretim_freq() / 1000U / 2U);

    while (1) {
        cur = csi_coret_get_value();

        if (start > cur) {
            if ((start - cur) >= cnt) {
                break;
            }
        } else {
            if (((load - cur) + start) > cnt) {
                break;
            }
        }
    }
}
#else
static void _mdelay(void)
{
    uint32_t load = csi_timer_get_load_value(&tick_timer);
    uint32_t start_r = csi_timer_get_remaining_value(&tick_timer);
    uint32_t cur_r;
    uint32_t cnt   = (soc_get_timer_freq(CONFIG_TICK_TIMER_IDX) / 1000U);

    while (1) {
        cur_r = csi_timer_get_remaining_value(&tick_timer);

        if (start_r > cur_r) {
            if ((start_r - cur_r) >= cnt) {
                break;
            }
        } else {
            if (((load - cur_r) + start_r) >= cnt) {
                break;
            }
        }
    }
}
#endif

__WEAK void mdelay(uint32_t ms)
{
    while (ms) {
        ms--;
#ifdef CONFIG_TICK_USE_CORETIME
        _500usdelay();
        _500usdelay();
#else
        _mdelay();
#endif
    }
}


static void _10udelay(void)
{
#ifdef CONFIG_TICK_USE_CORETIME
    uint32_t load  = csi_coret_get_load();
    uint32_t start = csi_coret_get_value();
    uint32_t cnt   = (soc_get_coretim_freq() / 1000U / 100U);

    while (1) {
        uint32_t cur = csi_coret_get_value();

        if (start > cur) {
            if ((start - cur) >= cnt) {
                break;
            }
        } else {
            if (((load - cur) + start) > cnt) {
                break;
            }
        }
    }
#else
    uint32_t load = csi_timer_get_load_value(&tick_timer);
    uint32_t start_r = csi_timer_get_remaining_value(&tick_timer);
    uint32_t cur_r;
    uint32_t cnt   = (soc_get_timer_freq(CONFIG_TICK_TIMER_IDX) / 10U);

    while (1) {
        cur_r = csi_timer_get_remaining_value(&tick_timer);

        if (start_r > cur_r) {
            if ((start_r - cur_r) >= cnt) {
                break;
            }
        } else {
            if (((load - cur_r) + start_r) >= cnt) {
                break;
            }
        }
    }
#endif
}

/**
 * Ps: At least delay over 10us
*/
void udelay(uint32_t us)
{
    us /= 10U;

    while (us) {
        us--;
        _10udelay();
    }
}

