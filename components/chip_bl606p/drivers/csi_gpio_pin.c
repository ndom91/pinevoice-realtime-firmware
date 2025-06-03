#include <string.h>
#include <bl606p_glb_gpio.h>
#include <bl606p.h>
#include <drv/gpio.h>
#include <csi_config.h>
#include <utils_list.h>
#include <drv/irq.h>
#include <drv/gpio_pin.h>

typedef struct bl_csi_gpio_ctx {
    struct bl_csi_gpio_ctx *next;
    void *handle;
    void *arg;
    int pin;
    csi_gpio_pin_t *priv;
} bl_csi_gpio_ctx_t;

typedef struct {
    csi_dev_t         gpio_dev;
    bl_csi_gpio_ctx_t *node;
    void *priv;
}blcsi_gpio_call_fuc_priv;

static bl_csi_gpio_ctx_t *gpio_head = NULL;
static blcsi_gpio_call_fuc_priv gpio_irq_func;
  
static void bl_gpio_pin_irqhandler(void *args)
{
    bl_csi_gpio_ctx_t *node = ((blcsi_gpio_call_fuc_priv *)args)->node;
    while (node) {
        csi_gpio_pin_t *pin = node->priv;   
        if (SET == GLB_Get_GPIO_IntStatus(pin->pin_idx)) {
            GLB_Clr_GPIO_IntStatus(pin->pin_idx);
            if (pin->callback) {
                pin->callback(pin, pin->arg);
            }
        }
        node = node->next;
    }
    return;
}

csi_error_t csi_gpio_pin_init(csi_gpio_pin_t *pin, pin_name_t pin_name)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    csi_gpio_t *pgpio;

    pgpio = malloc(sizeof(csi_gpio_t));
    if (!pgpio) {
        return CSI_ERROR;
    }

    pin->gpio = pgpio;
    pin->pin_idx = pin_name;

    return ret;
}

void csi_gpio_pin_uninit(csi_gpio_pin_t *pin)
{
    CSI_PARAM_CHK_NORETVAL(pin);

    free(pin->gpio);
    /* release handle */
    memset(pin, 0, sizeof(csi_gpio_pin_t));
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

csi_error_t csi_gpio_pin_dir(csi_gpio_pin_t *pin, csi_gpio_dir_t dir)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);

    csi_error_t ret = CSI_OK;
    GLB_GPIO_Cfg_Type gpioCfg;

    gpioCfg.gpioFun = GPIO_FUN_GPIO;
    gpioCfg.drive = 1;
    gpioCfg.smtCtrl = 1;
    gpioCfg.outputMode = 0;

    switch (dir) {
        case GPIO_DIRECTION_INPUT:
            gpioCfg.gpioMode = GPIO_MODE_INPUT;
            gpioCfg.pullType = GPIO_PULL_NONE;
            gpioCfg.gpioPin = pin->pin_idx;
            //GLB_GPIO_Init(&gpioCfg);
            gpio_direction_set(&gpioCfg);
            break;

        case GPIO_DIRECTION_OUTPUT:
            gpioCfg.gpioMode = GPIO_MODE_OUTPUT;
            gpioCfg.pullType = GPIO_PULL_NONE;
            gpioCfg.gpioPin = pin->pin_idx;
            gpio_direction_set(&gpioCfg);
            //GLB_GPIO_Init(&gpioCfg);
            break;

        default:
            ret = CSI_UNSUPPORTED;
            break;
    }

    return ret;
}

csi_error_t csi_gpio_pin_mode(csi_gpio_pin_t *pin, csi_gpio_mode_t mode)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);

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
    
    gpioCfg.gpioPin = pin->pin_idx;
    GLB_GPIO_Init(&gpioCfg);

    return ret;
}

//csi_error_t csi_gpio_irq_mode(csi_gpio_t *gpio, uint32_t pin_mask, csi_gpio_irq_mode_t mode)
csi_error_t csi_gpio_pin_irq_mode(csi_gpio_pin_t *pin, csi_gpio_irq_mode_t mode)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);

    csi_error_t    ret = CSI_OK;
    GLB_GPIO_INT_Cfg_Type intCfg;

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

    intCfg.gpioPin = pin->pin_idx;
    GLB_GPIO_Int_Init(&intCfg);

    return ret;
}

csi_error_t csi_gpio_pin_irq_enable(csi_gpio_pin_t *pin, bool enable)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);

    GLB_GPIO_IntMask((GLB_GPIO_Type)pin->pin_idx, enable ? UNMASK : MASK);

    return CSI_OK;
}

csi_error_t csi_gpio_pin_debonce(csi_gpio_pin_t *pin, bool enable)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);

    return CSI_UNSUPPORTED;
}

void csi_gpio_pin_toggle(csi_gpio_pin_t *pin)
{
    CSI_PARAM_CHK_NORETVAL(pin);

    uint32_t gpioCfgAddress;
    uint32_t tmpVal;
    uint8_t value;

    gpioCfgAddress = GLB_BASE + GLB_GPIO_CFG0_OFFSET + ((GLB_GPIO_Type)pin->pin_idx << 2);
    tmpVal = BL_RD_WORD(gpioCfgAddress);
    value = BL_GET_REG_BITS_VAL(tmpVal, GLB_REG_GPIO_0_O);
    GLB_GPIO_Write((GLB_GPIO_Type)pin->pin_idx, value ? 0 : 1);
}

void csi_gpio_pin_write(csi_gpio_pin_t *pin, csi_gpio_pin_state_t value)
{
    CSI_PARAM_CHK_NORETVAL(pin);

    GLB_GPIO_Write((GLB_GPIO_Type)pin->pin_idx, value ? 1 : 0);
}

csi_gpio_pin_state_t csi_gpio_pin_read(csi_gpio_pin_t *pin)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    csi_gpio_pin_state_t state;

    state = GLB_GPIO_Read((GLB_GPIO_Type)pin->pin_idx); 

    return state;
}

csi_error_t csi_gpio_pin_attach_callback(csi_gpio_pin_t *pin, void *callback, void *arg)
{
    CSI_PARAM_CHK(pin, CSI_ERROR);
    CSI_PARAM_CHK(callback, CSI_ERROR);

    pin->callback = callback;
    pin->arg      = arg;
    (pin->gpio)->priv = pin;

    bl_csi_gpio_ctx_t *node = NULL;
    bl_csi_gpio_ctx_t *node_f = NULL;
    //GLB_GPIO_INT_Cfg_Type intCfg;

    node = (bl_csi_gpio_ctx_t *)aos_malloc(sizeof(bl_csi_gpio_ctx_t));
    if (!node) {
        printf("irq ctx malloc failed \r\n");
        return -1;
    }
    node->handle     = callback;
    node->arg        = arg;
    node->pin        = pin->pin_idx;
    node->priv       = pin;
    if (!gpio_head) {
        gpio_head = node;
        node->next = NULL;
    } else {
        for (node_f = gpio_head; node_f != NULL; node_f = node_f->next) {
            if (node_f->pin == node->pin) {
#if 0
                memcpy(node_f, node, sizeof(bl_csi_gpio_ctx_t));// will crash beacause next field is NULL
#endif
                node_f->handle = node->handle;
                node_f->arg    = node->arg;
                aos_free(node);
                break;
            }
        }
        if (node_f == NULL) {
            node->next = gpio_head;
            gpio_head = node;
        }
    }
    gpio_irq_func.node = gpio_head;

    csi_irq_attach(GPIO_INT0_IRQn, &bl_gpio_pin_irqhandler, &gpio_irq_func);
    //csi_irq_attach(GPIO_INT0_IRQn, &bl_gpio_pin_irqhandler, pin);
    csi_irq_enable(GPIO_INT0_IRQn);

    return CSI_OK;
}
