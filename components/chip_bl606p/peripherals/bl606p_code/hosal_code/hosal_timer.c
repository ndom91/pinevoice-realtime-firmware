#include <bl606p_timer.h>
#include <bl606p_common.h>
#include <bl606p.h>
#include <bl_irq.h>
#include <hosal_timer.h>
#include <drv/common.h>
#include "peripherals_config.h"

typedef struct hosal_timer_call_fuc_priv_t {
    csi_dev_t         timer_dev;
    hosal_timer_dev_t *timer;
    void *priv;
} hosal_timer_call_fuc_priv;

static hosal_timer_call_fuc_priv ctx;

static void timer_process(void *ctx)
{
    hosal_timer_call_fuc_priv *priv = (hosal_timer_call_fuc_priv *)ctx;

	hosal_timer_dev_t *tim= priv->timer;
    void *arg;
	hosal_timer_cb_t handle;
    
    handle = tim->config.cb;
    arg = tim->config.arg;

    if (tim->port == 0) {
        TIMER_IntMask(TIMER0_ID, TIMER_CH0, TIMER_INT_ALL, MASK);
        TIMER_ClearIntStatus(TIMER0_ID, TIMER_CH0, TIMER_COMP_ID_0);
        if (tim->config.reload_mode == TIMER_RELOAD_ONCE) {
            TIMER_Disable(TIMER0_ID, TIMER_CH0);
        }
    } else if (tim->port == 1) {
        TIMER_IntMask(TIMER0_ID, TIMER_CH1, TIMER_INT_ALL, MASK);
        TIMER_ClearIntStatus(TIMER0_ID, TIMER_CH1, TIMER_COMP_ID_0);
        if (tim->config.reload_mode == TIMER_RELOAD_ONCE) {
            TIMER_Disable(TIMER0_ID, TIMER_CH1);
        }
    }

    if (handle) {
        handle(arg);
    }

    if (tim->port == 0) {
        TIMER_IntMask(TIMER0_ID, TIMER_CH0, TIMER_INT_COMP_0, UNMASK);
    } else if (tim->port == 1) {
        TIMER_IntMask(TIMER0_ID, TIMER_CH1, TIMER_INT_COMP_0, UNMASK);
    }
}

int hosal_timer_init(hosal_timer_dev_t *tim)
{
    TIMER_CFG_Type timer_cfg =
    {
        TIMER_CH1,
        TIMER_CLKSRC_XTAL,
        TIMER_PRELOAD_TRIG_COMP0,
        TIMER_COUNT_PRELOAD,
        39,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0xFFFFFFFF,
        0,
    };
    
    ctx.timer = tim;
    
    if (tim->port == 0) {
        timer_cfg.timerCh = 0;
    } else if (tim->port == 1) {
        timer_cfg.timerCh = 1;
    } else {
        blog_error("timer channel %d not exists\r\n", tim->port);
		return -1;
    }
	timer_cfg.matchVal0 = tim->config.period;
    TIMER_IntMask(TIMER0_ID, timer_cfg.timerCh, TIMER_INT_ALL, MASK);
    TIMER_Disable(TIMER0_ID, timer_cfg.timerCh);
    TIMER_Init(TIMER0_ID, &timer_cfg);

    /* Clear interrupt status*/
    TIMER_ClearIntStatus(TIMER0_ID, timer_cfg.timerCh, TIMER_COMP_ID_0);
    TIMER_ClearIntStatus(TIMER0_ID, timer_cfg.timerCh, TIMER_COMP_ID_1);
    TIMER_ClearIntStatus(TIMER0_ID, timer_cfg.timerCh, TIMER_COMP_ID_2);

    /* Enable timer match interrupt */
    TIMER_IntMask(TIMER0_ID, timer_cfg.timerCh, TIMER_INT_COMP_0, UNMASK);
    TIMER_IntMask(TIMER0_ID, timer_cfg.timerCh, TIMER_INT_COMP_1, MASK);
    TIMER_IntMask(TIMER0_ID, timer_cfg.timerCh, TIMER_INT_COMP_2, MASK);
   
    if (tim->port == 0) {
		bl_irq_register_with_ctx_yoc(TIMER0_CH0_IRQn, timer_process, &ctx);
    } else {
	    bl_irq_register_with_ctx_yoc(TIMER0_CH1_IRQn, timer_process, &ctx);
    }

    return 0;
}


int hosal_timer_start(hosal_timer_dev_t *tim)
{
    if (tim->port == 0) {
		bl_irq_enable(TIMER0_CH0_IRQn);
		TIMER_Enable(TIMER0_ID, TIMER_CH0);
    } else if (tim->port == 1) {
		bl_irq_enable(TIMER0_CH1_IRQn);
		TIMER_Enable(TIMER0_ID, TIMER_CH1);
    } else {
        blog_error("timer channel %d not exists\r\n", tim->port);
		return -1;
    }
    return 0;
}

void hosal_timer_stop(hosal_timer_dev_t *tim)
{
    if (tim->port == 0) {
		bl_irq_disable(TIMER0_CH0_IRQn);
		TIMER_Disable(TIMER0_ID, TIMER_CH0);
    } else if (tim->port == 1) {
		bl_irq_disable(TIMER0_CH1_IRQn);
		TIMER_Disable(TIMER0_ID, TIMER_CH1);
    } else {
        blog_info("timer channel %d not exists\r\n", tim->port);
		return;
    }
}

int hosal_timer_finalize(hosal_timer_dev_t *tim)
{
    if (tim->port == 0) {
		bl_irq_disable(TIMER0_CH0_IRQn);
        bl_irq_unregister_with_ctx_yoc(TIMER0_CH0_IRQn);
        //bl_irq_unregister(TIMER0_CH0_IRQn, timer_process);
        TIMER_IntMask(TIMER0_ID, TIMER_CH0, TIMER_INT_ALL, MASK);
		TIMER_Disable(TIMER0_ID, TIMER_CH0);
    } else if (tim->port == 1) {
		bl_irq_disable(TIMER0_CH1_IRQn);
        bl_irq_unregister_with_ctx_yoc(TIMER0_CH1_IRQn);
        //bl_irq_unregister(TIMER0_CH1_IRQn, timer_process);
        TIMER_IntMask(TIMER0_ID, TIMER_CH1, TIMER_INT_ALL, MASK);
		TIMER_Disable(TIMER0_ID, TIMER_CH1);
    } else {
        blog_error("timer channel %d not exists\r\n", tim->port);
		return -1;
    }
    return 0;
}
