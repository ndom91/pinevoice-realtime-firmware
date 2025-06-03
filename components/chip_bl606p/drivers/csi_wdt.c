#include <drv/wdt.h>
#include <drv/irq.h>
#include <bl606p_timer.h>
#include <bl606p.h>
#include <utils_list.h>
#include <bl606p_glb.h>

#define TIMER_CLK_FREQ    (32768)

static void bl_wdt_irq_handler(void *arg)
{
    csi_dev_t *pdev = (csi_dev_t *)arg;
    csi_wdt_t *pwdt;
    uint32_t tmpVal = 0;

    pwdt = (csi_wdt_t*)utils_container_of(pdev, csi_wdt_t, dev);

    tmpVal = BL_RD_REG(pwdt->dev.reg_base, TIMER_WICR);
    WDT_ENABLE_ACCESS(pwdt->dev.reg_base);
    BL_WR_REG(pwdt->dev.reg_base, TIMER_WICR, BL_SET_REG_BIT(tmpVal, TIMER_WICLR));
    WDT_ResetCounterValue(pwdt->dev.idx);

    if (pwdt->callback) {
          pwdt->callback(pwdt, pwdt->arg);
    }

    return;
}

/**
  \brief       Initialize WDT Interface. Initializes the resources needed for the WDT interface
  \param[in]   wdt    wdt handle to operate
  \param[in]   idx    wdt index
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_init(csi_wdt_t *wdt, uint32_t idx)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if(idx > 0)
    {
        return CSI_ERROR;
    }

    wdt->dev.idx = idx;
    wdt->priv = NULL;
    if(idx == 0){
        wdt->dev.reg_base = TIMER0_BASE;
    }else if(idx == 1){
        wdt->dev.reg_base = TIMER1_BASE;
    }

    GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_TIMER);
    WDT_Disable(wdt->dev.idx);
    WDT_Set_Clock(wdt->dev.idx, TIMER_CLKSRC_32K, 0);
    WDT_ClearResetStatus(wdt->dev.idx);
    WDT_IntMask(wdt->dev.idx, WDT_INT, MASK);
    WDT_ResetCounterValue(wdt->dev.idx);
    WDT_CompValueEffectImmediately(wdt->dev.idx, ENABLE);

    return ret;
}
/**
  \brief       De-initialize WDT Interface. stops operation and releases the software resources used by the interface
  \param[in]   wdt    handle to operate
  \return      None
*/
void csi_wdt_uninit(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK_NORETVAL(wdt);

    WDT_Disable(wdt->dev.idx);

    return;
}
/**
  \brief       Set the WDT value
  \param[in]   wdt    handle to operate
  \param[in]   ms     the timeout value(ms)
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_set_timeout(csi_wdt_t *wdt, uint32_t ms)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    uint32_t clk_div;
    uint64_t clk_cycles;

    clk_cycles = ((uint64_t)ms * TIMER_CLK_FREQ + 500) / 1000;
    /* get the best frequency division value */
    if(clk_cycles > 0xFFFF){
        clk_div = (clk_cycles + 0xFFFF - 1) / 0xFFFF;
    } else {
        clk_div = 1;
    }

    /* Check the frequency range */
    if(clk_div - 1 > 0xFF) {
        wdt->priv = NULL;
        return CSI_ERROR;
    }

    /* record the dividing frequency value */
    wdt->priv = (void *)(uintptr_t)clk_div;

    clk_cycles = (clk_cycles + (clk_div / 2)) / clk_div;

    /* set div */
    WDT_Set_Clock(wdt->dev.idx, TIMER_CLKSRC_32K, clk_div - 1);

    WDT_SetCompValue(wdt->dev.idx, (uint32_t)clk_cycles);

    return ret;
}
/**
  \brief       Start the WDT
  \param[in]   wdt    handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_start(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    WDT_Enable(wdt->dev.idx);

    return ret;
}
/**
  \brief       Stop the WDT
  \param[in]   wdt    handle to operate
  \return      None
*/
void csi_wdt_stop(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK_NORETVAL(wdt);

    WDT_Disable(wdt->dev.idx);
}
/**
  \brief       Feed the WDT
  \param[in]   wdt    handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_feed(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    WDT_ResetCounterValue(wdt->dev.idx);

    return ret;
}
/**
  \brief       Get the remaining time to timeout
  \param[in]   wdt    handle to operate
  \return      tne remaining time of wdt(ms)
*/
uint32_t csi_wdt_get_remaining_time(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, 0U);
    uint32_t compare_val = 0;
    uint32_t current_val = 0;
    uint64_t clk_cycles;

    compare_val = WDT_GetMatchValue(wdt->dev.idx);
    current_val = WDT_GetCounterValue(wdt->dev.idx);

    if(current_val > compare_val){
        clk_cycles = 0;
    }else{
        if(wdt->priv == NULL){
            clk_cycles = 0;
        }else{
            clk_cycles = compare_val - current_val;
            clk_cycles *= (uintptr_t)(wdt->priv);
            clk_cycles = (clk_cycles * 1000 + 500) / TIMER_CLK_FREQ;
        }
    }
    return (uint32_t)clk_cycles;
}

/**
  \brief       Check wdt is running
  \param[in]   wdt    handle wdt handle to operate
  \return      true->running, false->stopped
*/
bool csi_wdt_is_running(csi_wdt_t *wdt)
{
    CSI_PARAM_CHK(wdt, false);

    uint32_t tmpVal;
    uint32_t bitval;

    tmpVal = BL_RD_REG(wdt->dev.reg_base, TIMER_WMER);
    bitval = BL_GET_REG_BITS_VAL(tmpVal, TIMER_WE);

    return bitval;
}

/**
  \brief       Attach the callback handler to wdt
  \param[in]   wdt         operate handle
  \param[in]   callback    callback function
  \param[in]   arg         callback's param
  \return      error code \ref csi_error_t
*/
csi_error_t csi_wdt_attach_callback(csi_wdt_t *wdt, void *callback, void *arg)
{
    uint32_t irq_idx = 0;
    CSI_PARAM_CHK(wdt, CSI_ERROR);

    wdt->callback = callback;
    wdt->arg = arg;

    if (wdt->dev.idx == 0) {
        irq_idx = TIMER0_WDT_IRQn;
    } else if (wdt->dev.idx == 1) {
        irq_idx = TIMER1_WDT_IRQn;
    }

    csi_irq_attach(irq_idx, &bl_wdt_irq_handler, &wdt->dev);
    csi_irq_enable(irq_idx);

    WDT_IntMask(wdt->dev.idx, WDT_INT, UNMASK);

    return CSI_OK;
}

/**
  \brief       Detach the callback handler
  \param[in]   wdt    operate handle
  \return      None
*/
void csi_wdt_detach_callback(csi_wdt_t *wdt)
{
    uint32_t irq_idx = 0;
    CSI_PARAM_CHK_NORETVAL(wdt);

    wdt->callback = NULL;
    wdt->arg = NULL;

    if (wdt->dev.idx == 0) {
        irq_idx = TIMER0_WDT_IRQn;
    } else if (wdt->dev.idx == 1) {
        irq_idx = TIMER1_WDT_IRQn;
    }

    csi_irq_disable(irq_idx);
    csi_irq_detach(irq_idx);
    WDT_IntMask(wdt->dev.idx, WDT_INT, MASK);

    return;
}
