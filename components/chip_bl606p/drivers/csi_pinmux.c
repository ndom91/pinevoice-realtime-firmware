#include <stdint.h>
#include <string.h>
#include <bl606p_glb_gpio.h>
#include <bl606p.h>
#include <drv/gpio.h>
#include <csi_config.h>
#include <drv/irq.h>
#include <stddef.h>
#include <drv/pin.h>
#include <soc.h>

csi_error_t csi_pin_set_mux(pin_name_t pin_name, pin_func_t pin_func)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.drive    = 0;
    cfg.smtCtrl  = 1;
    cfg.gpioPin  = pin_name;
    cfg.gpioFun  = pin_func;
    cfg.gpioMode = 1;
    cfg.pullType = GPIO_MODE_OUTPUT;

    GLB_GPIO_Init(&cfg);

    return CSI_OK;
}

csi_error_t csi_pin_mode(pin_name_t pin_name, csi_gpio_mode_t mode)
{
    csi_error_t ret = CSI_OK;
    GLB_GPIO_Cfg_Type gpioCfg;

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
    gpioCfg.gpioPin = pin_name;
    GLB_GPIO_Init(&gpioCfg);

    return ret;
}


/**
  \brief       get the pin function.
  \param[in]   pin       refs to pin_name_e.
  \return      pin function count
*/
pin_func_t csi_pin_get_mux(pin_name_t pin_name)
{
    uint32_t gpioCfgAddress;
    uint32_t tmpVal;

    gpioCfgAddress = GLB_BASE + GLB_GPIO_CFG0_OFFSET + (pin_name << 2);
    tmpVal = BL_RD_WORD(gpioCfgAddress);
    tmpVal = BL_GET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_FUNC_SEL);
    
    return tmpVal;
}

uint32_t target_pin_to_devidx(pin_name_t pin_name, const csi_pinmap_t *pinmap)
{
    return pin_name;
}

uint32_t target_pin_to_channel(pin_name_t pin_name,const csi_pinmap_t *pinmap)
{
    uint32_t channel = 0;

    if (pin_name == 4) {
        channel = 2;
    }else if (pin_name == 5) {
        channel = 1;
    } else if (pin_name == 6) {
        channel = 4;
    } else if (pin_name == 11) {
        channel = 3;
    } else if (pin_name == 12) {
        channel = 6;
    } else if (pin_name == 13) {
        channel = 7;
    } else if (pin_name == 16) {
        channel = 8;
    } else if (pin_name == 17) {
        channel = 0;
    } else if (pin_name == 18) {
        channel = 9;
    } else if (pin_name == 19) {
        channel = 10;
    } else if (pin_name == 34) {
        channel = 11;
    } else {
        /* not correct pin */
        channel = 0xffff;
    }

    return channel;
}

pin_name_t target_gpio_to_pin(uint8_t gpio_idx, uint8_t channel, const csi_pinmap_t *pinmap)
{
    return gpio_idx;
}
