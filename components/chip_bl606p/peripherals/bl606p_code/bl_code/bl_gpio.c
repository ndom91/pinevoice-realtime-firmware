#include <stdint.h>

#include <bl606p_common.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include "bl_gpio.h"
#include "bl_irq.h"

/***************************************************************************
 * function  : bl_pin_set_func is used for set cpu's pin function
 * parameter :
 *          pin      : pin num
 *          pullType : GPIO_PULL_NONE   or  GPIO_PULL_DOWN  or GPIO_PULL_UP
 *          gpioMode : GPIO_MODE_OUTPUT or GPIO_MODE_INPUT or GPIO_MODE_AF or GPIO_MODE_ANALOG
 *          pin_func : GLB_GPIO_FUNC_Type enumeration
 ************************************************************************* */
int bl_pin_set_func(uint8_t pin, uint8_t pullType, uint8_t gpioMode, uint8_t pin_func)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.drive    = 0;
    cfg.smtCtrl  = 1;
    cfg.gpioPin  = pin;
    cfg.gpioFun  = pin_func;                                            // all the function number of GPIO is the same, we use def from GPIO0 here
    cfg.gpioMode = gpioMode;
    cfg.pullType = pullType;
    cfg.outputMode = 0;

    GLB_GPIO_Init(&cfg);
    if (40 == pin) {
        *((volatile uint32_t *)0x2000f038) &=~(1<<27);
    }
    if (41 == pin) {
        *((volatile uint32_t *)0x2000f038) &=~(1<<28);
    }

    return 0;
}

int bl_gpio_enable_output(uint8_t pin, uint8_t pullup, uint8_t pulldown)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.drive=0;
    cfg.smtCtrl=1;
    cfg.gpioPin = pin;
    cfg.gpioFun = GPIO_FUN_GPIO;//all the function number of GPIO is the same, we use def from GPIO0 here
    cfg.gpioMode = GPIO_MODE_OUTPUT;
    cfg.pullType = GPIO_PULL_NONE;
    cfg.outputMode = 0;
    if (pullup) {
        cfg.pullType = GPIO_PULL_UP;
    }
    if (pulldown) {
        cfg.pullType = GPIO_PULL_DOWN;
    }
    GLB_GPIO_Init(&cfg);

    if (40 == pin) {
        *((volatile uint32_t *)0x2000f038) &=~(1<<27);
    }
    if (41 == pin) {
        *((volatile uint32_t *)0x2000f038) &=~(1<<28);
    }

    return 0;
}

int bl_gpio_enable_input(uint8_t pin, uint8_t pullup, uint8_t pulldown)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.drive=0;
    cfg.smtCtrl=1;
    cfg.gpioPin = pin;
    cfg.gpioFun = GPIO_FUN_GPIO;//all the function number of GPIO is the same, we use def from GPIO0 here
    cfg.gpioMode = GPIO_MODE_INPUT;
    cfg.pullType = GPIO_PULL_NONE;
    cfg.outputMode = 0;
    if (pullup) {
        cfg.pullType = GPIO_PULL_UP;
    }
    if (pulldown) {
        cfg.pullType = GPIO_PULL_DOWN;
    }
    GLB_GPIO_Init(&cfg);

    return 0;
}

int bl_gpio_output_set(uint8_t pin, uint8_t value)
{
    GLB_GPIO_Write((GLB_GPIO_Type)pin, value ? 1 : 0);
    return 0;
}

int bl_gpio_input_get(uint8_t pin, uint8_t *value)
{
    *value = GLB_GPIO_Read((GLB_GPIO_Type)pin);
    return 0;
}

int bl_gpio_input_get_value(uint8_t pin)
{
    return GLB_GPIO_Read((GLB_GPIO_Type)pin);
}

void bl_gpio_intmask(uint8_t gpioPin, uint8_t mask)
{
    GLB_GPIO_IntMask(gpioPin, mask ? MASK : UNMASK);
}

void bl_set_gpio_intmod(uint8_t gpioPin, uint8_t intCtrlMod, uint8_t intTrgMod)
{
//    GLB_Set_GPIO_IntMod(gpioPin, intCtrlMod, intTrgMod);                  // todo: mcu代码暂时没有实现这个函数
}

int bl_gpio_int_clear(uint8_t gpioPin, uint8_t intClear)
{
#if 0
    uint32_t tmpVal;
    if (gpioPin < 32) {
        /*GPIO0 ~ GPIO31*/
        tmpVal = BL_RD_REG(GLB_BASE, GLB_GPIO_INT_CLR1);
        if(intClear==SET){
            tmpVal = tmpVal|(1<<gpioPin);
        }else{
            tmpVal = tmpVal&~(1<<gpioPin);
        }
        BL_WR_REG(GLB_BASE,GLB_GPIO_INT_CLR1,tmpVal);
    }
    return 0;
#endif
    uint32_t tmpVal;

    tmpVal = GLB_Clr_GPIO_IntStatus(gpioPin);
    if (SUCCESS == tmpVal) {
        return 0;
    } else {
        return -1;
    }
}

int bl_gpio_check_is_interrupt(int gpioPin)
{
    int reg_val = 0;

    reg_val = GLB_Get_GPIO_IntStatus((GLB_GPIO_Type)gpioPin);
    if (reg_val == SET) {
        return 0;
    } else {
        return -1;
    }
}

static int exec_gpio_handler(gpio_ctx_t *pstnode)
{
    bl_gpio_intmask(pstnode->gpioPin, 1);

    if (pstnode->gpio_handler) {
        pstnode->gpio_handler(pstnode);
        return 0;
    }

    return -1;
}

static void gpio_interrupt_entry(gpio_ctx_t *pstnode)
{
    int ret;

    while (pstnode) {
        ret = bl_gpio_check_is_interrupt(pstnode->gpioPin);
        if (ret == 0) {
            exec_gpio_handler(pstnode);
        }

        pstnode = pstnode->next;
    }
    return;
}

void bl_gpio_register(gpio_ctx_t *pstnode)
{
    bl_gpio_intmask(pstnode->gpioPin, 1);
    bl_set_gpio_intmod(pstnode->gpioPin, pstnode->intCtrlMod, pstnode->intTrgMod);
    bl_irq_register_with_ctx(GPIO_INT0_IRQn, gpio_interrupt_entry, pstnode);
    bl_gpio_intmask(pstnode->gpioPin, 0);
    bl_irq_enable(GPIO_INT0_IRQn);
}

void bl_gpio_set_for_pdm(int pdm_in, int pdm_clk)
{
    GLB_GPIO_Cfg_Type gpio_cfg;
    gpio_cfg.drive = 2;
    gpio_cfg.smtCtrl = 1;
    gpio_cfg.gpioMode = GPIO_MODE_AF;
    gpio_cfg.pullType = GPIO_PULL_UP;
    gpio_cfg.gpioFun = GPIO_FUN_PDM;

    gpio_cfg.gpioPin = pdm_in;
    GLB_GPIO_Init(&gpio_cfg);

    gpio_cfg.gpioPin = pdm_clk;
    GLB_GPIO_Init(&gpio_cfg);
}
