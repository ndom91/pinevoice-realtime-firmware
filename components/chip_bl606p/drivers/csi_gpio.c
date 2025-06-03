/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     dw_gpio.c
 * @brief
 * @version
 * @date     2020-01-08
 ******************************************************************************/

#include <string.h>
#include <bl606p_glb_gpio.h>
#include <bl606p.h>
#include <drv/gpio.h>
#include <csi_config.h>
#include <utils_list.h>
#include <drv/irq.h>

static void bl_gpio_irqhandler(void *args)
{
    int i;
    uint32_t bitmap = 0;
    csi_dev_t *ptr = (csi_dev_t*)args;
    csi_gpio_t *pgpio;

    pgpio = (csi_gpio_t*)utils_container_of(ptr, csi_gpio_t, dev);
    for (i = GLB_GPIO_PIN_0; i < GLB_GPIO_PIN_MAX; i++) {
        if (SET == GLB_Get_GPIO_IntStatus(i)) {
            GLB_Clr_GPIO_IntStatus(i);
            bitmap += 1 << i;
        }
    }
    pgpio->callback(pgpio, bitmap, pgpio->arg);
}

csi_error_t csi_gpio_init(csi_gpio_t *gpio, uint32_t port_idx)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);

    csi_error_t ret = CSI_OK;

    if(port_idx != 0)
    {
        return CSI_ERROR;
    }

    return ret;
}

void csi_gpio_uninit(csi_gpio_t *gpio)
{
    CSI_PARAM_CHK_NORETVAL(gpio);

    /* release handle */
    memset(gpio, 0, sizeof(csi_gpio_t));
}

static void gpio_direction_set(GLB_GPIO_Cfg_Type *cfg)
{
    uint8_t gpioPin = cfg->gpioPin;
    uint32_t gpioCfgAddress;
    uint32_t tmpVal;

    /* drive strength(drive) = 0  <=>  8.0mA  @ 3.3V */
    /* drive strength(drive) = 1  <=>  9.6mA  @ 3.3V */
    /* drive strength(drive) = 2  <=>  11.2mA @ 3.3V */
    /* drive strength(drive) = 3  <=>  12.8mA @ 3.3V */

    gpioCfgAddress = GLB_BASE + GLB_GPIO_CFG0_OFFSET + (gpioPin << 2);

    /* Disable output anyway*/
    tmpVal = BL_RD_WORD(gpioCfgAddress);
    tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_OE);
    BL_WR_WORD(gpioCfgAddress, tmpVal);

    /* input/output, pull up/down, drive, smt, function */
    tmpVal = BL_RD_WORD(gpioCfgAddress);

    if (cfg->gpioMode == GPIO_MODE_OUTPUT) {
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_IE);
        tmpVal = BL_SET_REG_BIT(tmpVal, GLB_REG_GPIO_0_OE);
    } else {
        tmpVal = BL_SET_REG_BIT(tmpVal, GLB_REG_GPIO_0_IE);
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_OE);
    }

#if 0
    if (cfg->pullType == GPIO_PULL_UP) {
        tmpVal = BL_SET_REG_BIT(tmpVal, GLB_REG_GPIO_0_PU);
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_PD);
    } else if (cfg->pullType == GPIO_PULL_DOWN) {
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_PU);
        tmpVal = BL_SET_REG_BIT(tmpVal, GLB_REG_GPIO_0_PD);
    } else {
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_PU);
        tmpVal = BL_CLR_REG_BIT(tmpVal, GLB_REG_GPIO_0_PD);
    }
#endif

    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_DRV, cfg->drive);
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_SMT, cfg->smtCtrl);
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_FUNC_SEL, cfg->gpioFun);
    tmpVal = BL_SET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_MODE, cfg->outputMode);

    BL_WR_WORD(gpioCfgAddress, tmpVal);

    return ;
}

csi_error_t csi_gpio_dir(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_dir_t dir)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(pin_mask, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    uint32_t offset = 0;
    GLB_GPIO_Cfg_Type gpioCfg;

    if(pin_mask == 0)
    {
        return CSI_ERROR;
    }

    gpioCfg.gpioFun = GPIO_FUN_GPIO;
    gpioCfg.drive = 1;
    gpioCfg.smtCtrl = 1;
    gpioCfg.outputMode = 0;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            switch (dir) {
                case GPIO_DIRECTION_INPUT:
                    gpioCfg.gpioMode = GPIO_MODE_INPUT;
                    gpioCfg.pullType = GPIO_PULL_NONE;
                    gpioCfg.gpioPin = offset;
                    //GLB_GPIO_Init(&gpioCfg);
                    gpio_direction_set(&gpioCfg);
                    break;

                case GPIO_DIRECTION_OUTPUT:
                    gpioCfg.gpioMode = GPIO_MODE_OUTPUT;
                    gpioCfg.pullType = GPIO_PULL_NONE;
                    gpioCfg.gpioPin = offset;
                    gpio_direction_set(&gpioCfg);
                    //GLB_GPIO_Init(&gpioCfg);
                    break;

                default:
                    ret = CSI_UNSUPPORTED;
                    break;
            }
        }
        pin_mask >>= 1U;
        offset++;
    }

    return ret;
}

csi_error_t csi_gpio_mode(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_mode_t mode)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    uint8_t offset = 0U;
    GLB_GPIO_Cfg_Type gpioCfg;

    if(pin_mask == 0)
    {
        return CSI_ERROR;
    }

    switch (mode) {
        case GPIO_MODE_PULLNONE:
            gpioCfg.gpioMode = GPIO_MODE_INPUT;
            gpioCfg.pullType = GPIO_PULL_NONE;
            break;
        case GPIO_MODE_PULLUP:
            gpioCfg.gpioMode = GPIO_MODE_INPUT;
            gpioCfg.pullType = GPIO_PULL_UP;
            break;
        case GPIO_MODE_PULLDOWN:
            gpioCfg.gpioMode = GPIO_MODE_INPUT;
            gpioCfg.pullType = GPIO_PULL_DOWN;
            break;
        case GPIO_MODE_OPEN_DRAIN:
            return CSI_UNSUPPORTED;
            //break;
        case GPIO_MODE_PUSH_PULL:
            gpioCfg.gpioMode = GPIO_MODE_OUTPUT;
            //gpioCfg.pullType = GPIO_PULL_NONE;
            gpioCfg.pullType = GPIO_PULL_UP;
            break;
        default:
            return CSI_UNSUPPORTED;
    }

    gpioCfg.gpioFun = GPIO_FUN_GPIO;
    gpioCfg.drive = 1;
    gpioCfg.smtCtrl = 1;
    gpioCfg.outputMode = 0;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            gpioCfg.gpioPin = offset;
            GLB_GPIO_Init(&gpioCfg);
        }
        pin_mask = (pin_mask >> 1U);
        offset++;
    }

    return ret;
}

csi_error_t csi_gpio_irq_mode(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_irq_mode_t mode)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);

    csi_error_t    ret = CSI_OK;
    GLB_GPIO_INT_Cfg_Type intCfg;
    uint8_t offset = 0;

    if(pin_mask == 0)
    {
        return CSI_ERROR;
    }

    switch (mode) {
        case GPIO_IRQ_MODE_RISING_EDGE:
            intCfg.trig = GLB_GPIO_INT_TRIG_SYNC_RISING_EDGE;
            break;
        case GPIO_IRQ_MODE_FALLING_EDGE:
            intCfg.trig = GLB_GPIO_INT_TRIG_SYNC_FALLING_EDGE;
            break;
        case GPIO_IRQ_MODE_BOTH_EDGE:
            intCfg.trig = GLB_GPIO_INT_TRIG_SYNC_FALLING_RISING_EDGE;
            break;
        case GPIO_IRQ_MODE_LOW_LEVEL:
            intCfg.trig = GLB_GPIO_INT_TRIG_SYNC_LOW_LEVEL;
            break;
        case GPIO_IRQ_MODE_HIGH_LEVEL:
            intCfg.trig = GLB_GPIO_INT_TRIG_SYNC_HIGH_LEVEL;
            break;
        default:
            return CSI_UNSUPPORTED;
    }

    intCfg.intMask = MASK;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            intCfg.gpioPin = offset;
            GLB_GPIO_Int_Init(&intCfg);
        }

        pin_mask >>= 1U;
        offset++;
    }

    return ret;
}

csi_error_t csi_gpio_irq_enable(csi_gpio_t *gpio, uint32_t pin_mask, bool enable)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(pin_mask, CSI_ERROR);
    uint8_t offset = 0;

    if(pin_mask == 0)
    {
        return CSI_ERROR;
    }

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            GLB_GPIO_IntMask((GLB_GPIO_Type)offset, enable ? UNMASK : MASK);
        }

        pin_mask >>= 1U;
        offset++;
    }

    return CSI_OK;
}

csi_error_t csi_gpio_debounce(csi_gpio_t *gpio, uint32_t pin_mask, bool enable)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(pin_mask, CSI_ERROR);

    if(pin_mask == 0)
    {
        return CSI_ERROR;
    }

    return CSI_UNSUPPORTED;
}

void csi_gpio_toggle(csi_gpio_t *gpio, uint32_t pin_mask)
{
    CSI_PARAM_CHK_NORETVAL(gpio);
    CSI_PARAM_CHK_NORETVAL(pin_mask);

    uint32_t gpioCfgAddress;
    uint32_t tmpVal;
    uint8_t value;
    uint8_t offset = 0;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            gpioCfgAddress = GLB_BASE + GLB_GPIO_CFG0_OFFSET + (offset << 2);
            tmpVal = BL_RD_WORD(gpioCfgAddress);
            value = BL_GET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_O);
            GLB_GPIO_Write((GLB_GPIO_Type)offset, value ? 0 : 1);
        }

        pin_mask >>= 1U;
        offset++;
    }
}

void  csi_gpio_write(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_pin_state_t value)
{
    CSI_PARAM_CHK_NORETVAL(gpio);
    CSI_PARAM_CHK_NORETVAL(pin_mask);

    uint32_t offset = 0;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            GLB_GPIO_Write((GLB_GPIO_Type)offset, value ? 1 : 0);
        }

        pin_mask >>= 1U;
        offset++;
    }
}

uint32_t csi_gpio_read(csi_gpio_t *gpio, uint32_t pin_mask)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(pin_mask, CSI_ERROR);
    uint32_t bitmap = 0;

    uint32_t offset = 0;

    while (pin_mask) {
        if (pin_mask & 0x01U) {
            if (GLB_GPIO_Read((GLB_GPIO_Type)offset)) {
                bitmap += (1 << offset);
            }
        }

        pin_mask >>= 1U;
        offset++;
    }

    return bitmap;
}

csi_error_t  csi_gpio_attach_callback(csi_gpio_t *gpio, void *callback, void *arg)
{
    CSI_PARAM_CHK(gpio, CSI_ERROR);
    CSI_PARAM_CHK(callback, CSI_ERROR);

    gpio->callback = callback;
    gpio->arg      = arg;

    csi_irq_attach(GPIO_INT0_IRQn, bl_gpio_irqhandler, &gpio->dev);
    csi_irq_enable(GPIO_INT0_IRQn);

    return CSI_OK;
}

void csi_gpio_detach_callback(csi_gpio_t *gpio)
{
    CSI_PARAM_CHK_NORETVAL(gpio);

    gpio->callback = NULL;
    gpio->arg      = NULL;
}
