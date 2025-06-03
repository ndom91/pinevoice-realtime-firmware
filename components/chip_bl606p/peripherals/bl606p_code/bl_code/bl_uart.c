//FIXME no BL808/BL606p header file including is Allowed here
#include <drv/common.h>
#include <bl606p.h>
#include <bl606p_uart.h>
#include <bl606p_glb.h>

#include "bl_uart.h"
#include "bl_irq.h"

#ifdef BFLB_USE_HAL_DRIVER
void UART0_IRQHandler(void);
void UART1_IRQHandler(void);
void UART2_IRQHandler(void);
void UART3_IRQHandler(void);
#endif

typedef struct hosal_uart_call_fuc_priv_t {
    csi_dev_t         uart_dev;
    uint8_t           uart_id;
    void *priv;
}hosal_uart_call_fuc_priv;

typedef struct bl_uart_notify {
    cb_uart_notify_t rx_cb;
    void            *rx_cb_arg;

    cb_uart_notify_t tx_cb;
    void            *tx_cb_arg;
} bl_uart_notify_t;

//TODO Do in std driver
#define UART_NUMBER_SUPPORTED   4
#define UART_FIFO_TX_CNT        (32)
#define FIFO_TX_SIZE_BURST      (32)
static       uint32_t uartRefClk[BLSTD_UART_ID_MAX];
static const uint32_t uartAddr[BLSTD_UART_ID_MAX]        = {UART0_BASE, UART1_BASE, UART2_BASE, UART3_BASE};
static const uint32_t uartIrqNum[BLSTD_UART_ID_MAX]      = {UART0_IRQn, UART1_IRQn, UART2_IRQn, UART3_IRQn};
static const pFunc    uartIrqFunction[BLSTD_UART_ID_MAX] = {UART0_IRQHandler, UART1_IRQHandler, UART2_IRQHandler, UART3_IRQHandler};
static hosal_uart_call_fuc_priv uart_irq_func[BLSTD_UART_ID_MAX];

static bl_uart_notify_t g_uart_notify_arg[UART_NUMBER_SUPPORTED];

static inline void uart_generic_notify_handler_yoc(void* argv);

void bl_uart_port_enable(uint8_t id)
{
    UART_Enable(id, BLSTD_UART_TXRX);
    return ;
}
void bl_uart_port_disable(uint8_t id)
{
    UART_Disable(id, BLSTD_UART_TXRX);
    return ;
}

/****************************************************************************/ /**
 * @brief  set buad
 *
 * @param  uart_id: uart id type
 * @param  baudRate: baute rate value
 *
 * @return NONE
 *
*******************************************************************************/
static void bl_uart_set_baud(uint8_t uart_id, uint32_t baudRate)
{
    uint32_t fraction = 0;
    uint32_t baudRateDivisor = 0;
    uint32_t uart_base = uartAddr[uart_id];
    uint32_t uartClk = 0;

    /* Disable clock gate when use UART2 */
    if(uart_id == BLSTD_UART2_ID){
        GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_UART2);
    }

    uartClk = uartRefClk[uart_id];                                              // TODO:根据初始化设置的时钟
    /* Cal the baud rate divisor */
    fraction = uartClk * 10 / baudRate % 10;
    baudRateDivisor = uartClk / baudRate;

    if (fraction >= 5) {
        ++baudRateDivisor;
    }

    /* Set the baud rate register value */
    BL_WR_REG(uart_base, UART_BIT_PRD, ((baudRateDivisor - 1) << 0x10) | ((baudRateDivisor - 1) & 0xFFFF));

}

/****************************************************************************/ /**
 * @brief   set uart check bit
 *
 * @param  uart_id: uart id type
 * @param  parity:  uart check bit cfg
 *
 * @return NONE
 *
*******************************************************************************/
static void bl_uart_set_parity_bit(uint8_t uart_id, UART_Parity_Type parity)
{
    uint32_t tmpValTxCfg = 0;
    uint32_t tmpValRxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    /* Configure parity type */
    tmpValTxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);
    tmpValRxCfg = BL_RD_REG(uart_base, UART_URX_CONFIG);

    switch (parity) {
        case BLSTD_UART_PARITY_NONE:
            tmpValTxCfg = BL_CLR_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValRxCfg = BL_CLR_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            break;

        case BLSTD_UART_PARITY_ODD:
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_SEL);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_SEL);
            break;

        case BLSTD_UART_PARITY_EVEN:
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValTxCfg = BL_CLR_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_SEL);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            tmpValRxCfg = BL_CLR_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_SEL);
            break;

        default:
          break;
    }

      /* Write back */
      BL_WR_REG(uart_base, UART_UTX_CONFIG, tmpValTxCfg);
      BL_WR_REG(uart_base, UART_URX_CONFIG, tmpValRxCfg);

}

/****************************************************************************/ /**
 * @brief  set uart data bit
 *
 * @param  uart_id: uart id value
 * @param  data_bits: uart data bit value
 *
 * @return NONE
 *
*******************************************************************************/
static void bl_uart_set_data_bit(uint8_t uart_id, UART_DataBits_Type data_bits)
{
    uint32_t tmpValTxCfg = 0;
    uint32_t tmpValRxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    tmpValTxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);
    tmpValRxCfg = BL_RD_REG(uart_base, UART_URX_CONFIG);

    /* Configure data bits */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_D, (data_bits + 4));
    tmpValRxCfg = BL_SET_REG_BITS_VAL(tmpValRxCfg, UART_CR_URX_BIT_CNT_D, (data_bits + 4));

    /* Write back */
    BL_WR_REG(uart_base, UART_UTX_CONFIG, tmpValTxCfg);
    BL_WR_REG(uart_base, UART_URX_CONFIG, tmpValRxCfg);

}

/****************************************************************************/ /**
 * @brief  set uart stop bit
 *
 * @param  uart_id: uart id value
 * @param  data_bits: uart stop bit value
 *
 * @return SUCCESS
 *
*******************************************************************************/
static void bl_uart_set_stop_bit(uint8_t uart_id, UART_StopBits_Type stop_bits)
{
    uint32_t tmpValTxCfg = 0;
    uint32_t tmpValRxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    tmpValTxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);
    tmpValRxCfg = BL_RD_REG(uart_base, UART_URX_CONFIG);

    /* Configure tx stop bits */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_P, stop_bits);

    /* Write back */
    BL_WR_REG(uart_base, UART_UTX_CONFIG, tmpValTxCfg);
    BL_WR_REG(uart_base, UART_URX_CONFIG, tmpValRxCfg);

}

/****************************************************************************/ /**
 * @brief  set uart cts flow
 *
 * @param  uart_id: uart id value
 * @param  ctsFlowControl: disable/enable
 *
 * @return NONE
 *
*******************************************************************************/
static void bl_uart_set_tx_cts_flow(uint8_t uart_id, BL_Fun_Type ctsFlowControl)
{
    uint32_t tmpValTxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    /* read  uart_utx_config reg value */
    tmpValTxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);

    /* Configure tx cts flow control function */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_CTS_EN, ctsFlowControl);

    /* Write back */
    BL_WR_REG(uart_base, UART_UTX_CONFIG, tmpValTxCfg);
}

/****************************************************************************/ /**
 * @brief  set uart rx deglitch
 *
 * @param  uart_id: uart id value
 * @param  data_bits: uart stop bit value
 *
 * @return SUCCESS
 *
*******************************************************************************/
static void bl_uart_set_rx_deg_litch(uint8_t uart_id, BL_Fun_Type rxDeglitch)
{
    uint32_t tmpValRxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    /* read  uart_utx_config reg value */
    tmpValRxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);

    /* Configure tx cts flow control function */
    tmpValRxCfg = BL_SET_REG_BITS_VAL(tmpValRxCfg, UART_CR_URX_DEG_EN, rxDeglitch);

    /* Write back */
    BL_WR_REG(uart_base, UART_URX_CONFIG, tmpValRxCfg);
}

/****************************************************************************/ /**
 * @brief  set uart lin_mode
 *
 * @param  uart_id: uart id value
 * @param  tx_cfg_val: uart tx line mode
 * @param  tx_cfg_val: uart rx line mode
 *
 * @return SUCCESS
 *
*******************************************************************************/
static void bl_uart_set_lin_mode_cfg(uint8_t uart_id, BL_Fun_Type tx_cfg_val, BL_Fun_Type rx_cfg_val)
{
    uint32_t tmpValRxCfg = 0;
    uint32_t tmpValTxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    /* read  uart_utx_config reg value */
    tmpValTxCfg = BL_RD_REG(uart_base, UART_UTX_CONFIG);
    tmpValRxCfg = BL_RD_REG(uart_base, UART_URX_CONFIG);

    /* Configure tx lin mode function */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_LIN_EN, tx_cfg_val);
    /* Configure rx lin mode function */
    tmpValRxCfg = BL_SET_REG_BITS_VAL(tmpValRxCfg, UART_CR_URX_LIN_EN, rx_cfg_val);

    /* Write back */
    BL_WR_REG(uart_base, UART_UTX_CONFIG, tmpValTxCfg);
    BL_WR_REG(uart_base, UART_URX_CONFIG, tmpValRxCfg);
}

/****************************************************************************/ /**
 * @brief  set uart rx deglitch
 *
 * @param  uart_id: uart id value
 * @param  data_bits: uart stop bit value
 *
 * @return SUCCESS
 *
*******************************************************************************/
static void bl_uart_set_sw_mode_cfg(uint8_t uart_id, BL_Fun_Type tx_cfg_val, BL_Fun_Type rx_cfg_val)
{
    uint32_t tmpValTxCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    tmpValTxCfg = BL_RD_REG(uart_base, UART_SW_MODE);
    /* Configure rx rts output SW control mode */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_URX_RTS_SW_MODE, rx_cfg_val);
    /* Configure tx output SW control mode */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_TXD_SW_MODE, tx_cfg_val);

    BL_WR_REG(uart_base, UART_SW_MODE, tmpValTxCfg);
}

/****************************************************************************/ /**
 * @brief  set uart lsb or msb
 *
 * @param  uart_id: uart id value
 * @param  byteBitInverse: uart lsb or msb bit value
 *
 * @return NONE
 *
*******************************************************************************/
static void bl_uart_set_lsb_msb_bit(uint8_t uart_id, UART_ByteBitInverse_Type byteBitInverse)
{
    uint32_t tmpValCfg = 0;
    uint32_t uart_base = uartAddr[uart_id];

    /* Configure LSB-first or MSB-first */
    tmpValCfg = BL_RD_REG(uart_base, UART_DATA_CONFIG);

    switch (byteBitInverse) {
        case UART_LSB_FIRST:
            tmpValCfg = BL_CLR_REG_BIT(tmpValCfg, UART_CR_UART_BIT_INV);
            break;
        case UART_MSB_FIRST:
            tmpValCfg = BL_SET_REG_BIT(tmpValCfg, UART_CR_UART_BIT_INV);
            break;
        default:
            break;
    }

    BL_WR_REG(uart_base, UART_DATA_CONFIG, tmpValCfg);
}

static int uart_signal_get(uint8_t pin)
{
    //TODO no magic number is allowed here
    if (pin >= 12 && pin <=23) {
        return (pin + 6) % 12;
    } else if (pin >= 36 && pin <=45) {
        return (pin + 6) % 12;
    }

    return (pin + 6) % 12;
}

static int uart_func_get(uint8_t id, GLB_UART_SIG_FUN_Type uartfunc)
{
    switch (id) {
        case 0:
            return uartfunc;
        case 1:
            return (GLB_UART_SIG_FUN_UART1_RTS - GLB_UART_SIG_FUN_UART0_RTS) * 1 + uartfunc;
        case 2:
            return (GLB_UART_SIG_FUN_UART1_RTS - GLB_UART_SIG_FUN_UART0_RTS) * 1 + uartfunc;
        default:
            /*empty here*/
            //TODO should assert here?
            return uartfunc;
    }
}

static void uart_clk_sel(uint8_t id,  uint32_t uartClk)
{
    //Enable UART clock
    GLB_Set_UART_CLK(1, 0, 0);

    uartRefClk[id] = uartClk;
}

static void uart_pin_sel(uint8_t id, uint8_t tx_pin, uint8_t rx_pin, uint8_t cts_pin, uint8_t rts_pin)
{
    GLB_GPIO_Cfg_Type gpio_cfg;
    uint8_t uart_func = GLB_UART_SIG_0, uart_sig;

    //FIXME SWAP set is NOT put here
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO12_GPIO23, 1);
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO24_GPIO35, 1);
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO36_GPIO45, 1);

    //common GPIO cfg
    gpio_cfg.drive = 0;
    gpio_cfg.smtCtrl = 1;
    gpio_cfg.gpioMode = GPIO_MODE_AF;
    gpio_cfg.pullType = GPIO_PULL_UP;
    gpio_cfg.gpioFun = GPIO_FUN_UART;
    //cfg for UART Tx
    if (tx_pin != GLB_GPIO_PIN_MAX) {
        gpio_cfg.gpioPin = tx_pin;
//        uart_func = uart_func_get(id, GLB_UART_SIG_FUN_UART0_TXD);
        if (id == 0) {
            uart_func = GLB_UART_SIG_FUN_UART0_TXD;
        } else if (id == 1) {
            uart_func = GLB_UART_SIG_FUN_UART1_TXD;
        }
//        uart_sig = uart_signal_get(gpio_cfg.gpioPin);
        uart_sig = (gpio_cfg.gpioPin+6) % 12;
        GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_sig, (GLB_UART_SIG_FUN_Type)uart_func);
        GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_func, (GLB_UART_SIG_FUN_Type)uart_sig);
        GLB_GPIO_Init(&gpio_cfg);
    }
    //cfg for UART Rx
    if (rx_pin != GLB_GPIO_PIN_MAX) {
        gpio_cfg.gpioPin = rx_pin;
//        uart_func = uart_func_get(id, GLB_UART_SIG_FUN_UART0_RXD);
        if (id == 0) {
            uart_func = GLB_UART_SIG_FUN_UART0_RXD;
        } else if (id == 1) {
            uart_func = GLB_UART_SIG_FUN_UART1_RXD;
        }
//        uart_sig = uart_signal_get(gpio_cfg.gpioPin);
        uart_sig = (gpio_cfg.gpioPin+6) % 12;
        GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_sig, (GLB_UART_SIG_FUN_Type)uart_func);
        GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_func, (GLB_UART_SIG_FUN_Type)uart_sig);
        GLB_GPIO_Init(&gpio_cfg);
    }
}

int bl_uart_init(uint8_t id, uint8_t tx_pin, uint8_t rx_pin, uint8_t cts_pin, uint8_t rts_pin, uint32_t baudrate)
{
    UART_CFG_Type uart_dbg_cfg = {
    //   32 * 1000 * 1000, /*UART clock*/
      80*1000*1000,     /*UART clock from XTAL*/
      2000000,          /* baudrate  */
      UART_DATABITS_8,  /* data bits  */
      UART_STOPBITS_1,  /* stop bits */
      BLSTD_UART_PARITY_NONE, /* parity  */
      DISABLE,          /* Disable auto flow control */
      DISABLE,          /* Disable rx input de-glitch function */
      DISABLE,          /* Disable RTS output SW control mode */
      DISABLE,          /* Disable tx output SW control mode */
      DISABLE,          /* Disable tx lin mode */
      DISABLE,          /* Disable rx lin mode */
      0,                /* Tx break bit count for lin mode */
      UART_LSB_FIRST,   /* UART each data byte is send out LSB-first */
    };

    UART_FifoCfg_Type fifoCfg = {
      16,      /* TX FIFO threshold */
      16,      /* RX FIFO threshold */
      DISABLE, /* Disable tx dma req/ack interface */
      DISABLE  /* Disable rx dma req/ack interface */
    };
    uart_dbg_cfg.baudRate = baudrate;

    /* uart gpio pin select */
    uart_pin_sel(id, tx_pin, rx_pin, cts_pin, rts_pin);

    /* uart refclk select */
    uart_clk_sel(id, uart_dbg_cfg.uartClk);

    /* disable all interrupt */
    UART_IntMask(id, UART_INT_ALL, MASK);

    /* disable uart before config */
    UART_Disable(id, BLSTD_UART_TXRX);

    /* uart init with default configuration */
    UART_Init(id, &uart_dbg_cfg);

    /* UART fifo configuration */
    UART_FifoConfig(id, &fifoCfg);

    /* Enable tx free run mode */
    UART_TxFreeRun(id, ENABLE);

    /* Set rx time-out value */
    UART_SetRxTimeoutValue(id, 80);

    /* enable uart */
    UART_AutoBaudDetection(id, 0);
    UART_Enable(id, BLSTD_UART_TXRX);

    return 0;
}

int32_t bl_uart_config(uint8_t uart_id, uart_cfg_type cfg_set, bl_uart_config_t* cfg)
{
    /* disable uart before config */
    bl_uart_port_disable(uart_id);

    switch (cfg_set) {
        case bl_uart_set_baud_cmd:
            bl_uart_set_baud(uart_id, cfg->baud_rate);
            break;
        case bl_uart_set_data_bit_cmd:
            bl_uart_set_data_bit(uart_id, (UART_DataBits_Type)cfg->data_width);
            break;
        case bl_uart_set_parity_bit_cmd:
            bl_uart_set_parity_bit(uart_id, (UART_Parity_Type)cfg->parity);
            break;
        case bl_uart_set_stop_bit_cmd:
            bl_uart_set_stop_bit(uart_id, (UART_StopBits_Type)cfg->stop_bits);
            break;
        case bl_uart_set_bit_inverse_cmd:
            bl_uart_set_lsb_msb_bit(uart_id, (UART_ByteBitInverse_Type)cfg->bitInverse);
            break;
        default:
            break;
    }

    /* enable uart */
    bl_uart_port_enable(uart_id);

    return 0;
}
/****************************************************************************/ /**
 * @brief  uart get tx fifo unoccupied count value
 *
 * @param  uartId: UART ID type
 *
 * @return Tx fifo unoccupied count value
 *
*******************************************************************************/
uint32_t bl_uart_get_txfifo_count(uint8_t uartId)
{
    return UART_GetTxFifoCount(uartId);
}

/****************************************************************************/ /**
 * @brief  uart get rx fifo occupied count value
 *
 * @param  uartId: UART ID type
 *
 * @return Rx fifo occupied count value
 *
*******************************************************************************/
uint32_t bl_uart_get_rxfifo_count(uint8_t uartId)
{
    return UART_GetRxFifoCount(uartId);
}

/*This function is NOT thread safe*/
int bl_uart_buff_send(uint8_t id, uint8_t data)
{
#if 1
    uint32_t UARTx = uartAddr[id];

    /* Wait for FIFO */
    while (UART_GetTxFifoCount(id) == 0) {
    }

    BL_WR_BYTE(UARTx + UART_FIFO_WDATA_OFFSET, data);

    return 0;
#else
    int i_ret = 0;

    i_ret = UART_SendData((UART_ID_Type)id, (uint8_t *)data, 1);
    if (i_ret !=0 ) {
        return -1;
    }

    return i_ret;
#endif
}

int bl_uart_buff_recv(uint8_t id)
{
#if 1
    int ret;
    uint32_t UARTx = uartAddr[id];

    /* Receive data */
    if (UART_GetRxFifoCount(id) > 0) {
        ret  = BL_RD_BYTE(UARTx + UART_FIFO_RDATA_OFFSET);
    } else {
        ret = -1;
    }

    return ret;
#else
    int i_ret = 0;
    uint8_t data;
    i_ret = UART_ReceiveData((UART_ID_Type)id, &data, 1);
    if (i_ret != 1) {
        return -1;
    } else {
        return data;
    }

#endif
}

int bl_uart_buffs_send(uint8_t id, uint8_t *data, int len)
{
    int ret;

    ret = UART_SendData((UART_ID_Type)id, (uint8_t *)data, (uint32_t)len);
    if (ret !=0 ) {
        return -1;
    }

    return len;
}

int bl_uart_buffs_recv(uint8_t id, uint8_t *data, int len)
{
    return UART_ReceiveData((UART_ID_Type)id, data, len);
}

int bl_uart_irq_rx_enable(uint8_t id)
{
    bl_uart_port_disable(id);

    UART_SetRxTimeoutValue((UART_ID_Type)id, 24);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, UNMASK);

    bl_uart_port_enable(id);

    return 0;
}

int bl_uart_irq_rx_disable(uint8_t id)
{
    bl_uart_port_disable(id);

    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, MASK);

    bl_uart_port_enable(id);

    return 0;
}

int bl_uart_irq_tx_enable(uint8_t id)
{
    bl_uart_port_disable(id);

    UART_IntMask((UART_ID_Type)id, UART_INT_TX_END, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FER, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FIFO_REQ, UNMASK);

    bl_uart_port_enable(id);

    return 0;
}

int bl_uart_irq_tx_disable(uint8_t id)
{
    bl_uart_port_disable(id);

    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FER, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_END, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FIFO_REQ, MASK);

    bl_uart_port_enable(id);

    return 0;
}

int bl_uart_flush(uint8_t id)
{
    /* Wait for FIFO */
    while (UART_FIFO_TX_CNT != UART_GetTxFifoCount(id)) {
    }

    return 0;
}

void bl_uart_setbaud(uint8_t id, uint32_t baud)
{
    //FIXME
    puts("uart is NOT implemented\r\n");
    while (1) {
    }

    bl_uart_set_baud(id, baud);
}
#if 1
int bl_uart_irq_enable(uint8_t id)
{
    if (id > BLSTD_UART_ID_MAX) {
        return -1;
    }

    bl_uart_irq_rx_enable(id);
    bl_uart_irq_tx_enable(id);
    bl_irq_register(uartIrqNum[id], (void *)&uartIrqFunction[id]); // io sdk
    bl_irq_enable(uartIrqNum[id]);

    return 0;
}

int bl_uart_irq_disable(uint8_t id)
{
    if (id > BLSTD_UART_ID_MAX) {
        return -1;
    }

    bl_uart_irq_rx_enable(id);
    bl_uart_irq_tx_enable(id);
    bl_irq_unregister(uartIrqNum[id], (void *)&uartIrqFunction[id]);
    bl_irq_disable(uartIrqNum[id]);

    return 0;
}
#else
int bl_uart_irq_enable(uint8_t id)
{
    if (id > BLSTD_UART_ID_MAX) {
        return -1;
    }

    bl_uart_irq_rx_enable(id);
//    bl_uart_irq_tx_enable(id);
    bl_irq_register_with_ctx_yoc(uartIrqNum[id], (void *)uart_generic_notify_handler_yoc, &uart_irq_func[id]);
    bl_irq_enable_yoc(uartIrqNum[id]);

    return 0;
}

int bl_uart_irq_disable(uint8_t id)
{
    if (id > BLSTD_UART_ID_MAX) {
        return -1;
    }

    bl_uart_irq_rx_disable(id);
    bl_uart_irq_tx_disable(id);
    bl_irq_unregister(uartIrqNum[id], (void *)&uartIrqFunction[id]);
    bl_irq_disable_yoc(uartIrqNum[id]);

    return 0;
}
#endif
int bl_uart_irq_rx_notify_register(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }

    g_uart_notify_arg[id].rx_cb = cb;
    g_uart_notify_arg[id].rx_cb_arg = arg;

    return 0;
}

int bl_uart_irq_tx_notify_register(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }

    g_uart_notify_arg[id].tx_cb = cb;
    g_uart_notify_arg[id].tx_cb_arg = arg;

    return 0;
}

int bl_uart_irq_rx_notify_unregister(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }
    g_uart_notify_arg[id].rx_cb = NULL;
    g_uart_notify_arg[id].rx_cb_arg = NULL;

    return 0;
}

int bl_uart_irq_tx_notify_unregister(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }
    g_uart_notify_arg[id].tx_cb = NULL;
    g_uart_notify_arg[id].tx_cb_arg = NULL;

    return 0;
}

static inline void uart_generic_notify_handler(uint8_t id)
{
    cb_uart_notify_t cb;
    void *arg;
    uint32_t tmpVal = 0;
    uint32_t maskVal = 0;
    uint32_t UARTx = uartAddr[id];

    tmpVal = BL_RD_REG(UARTx,UART_INT_STS);
    maskVal = BL_RD_REG(UARTx,UART_INT_MASK);


    /* Length of uart tx data transfer arrived interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_END_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_END_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x1);
    }

    /* Length of uart rx data transfer arrived interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_END_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_END_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x2);

        /*Receive Data ready*/
        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Tx fifo ready interrupt,auto-cleared when data is pushed */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_FRDY_MASK)){
        /* Transmit data request interrupt */
        cb = g_uart_notify_arg[id].tx_cb;
        arg = g_uart_notify_arg[id].tx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx fifo ready interrupt,auto-cleared when data is popped */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_FRDY_MASK)){
        /*Receive Data ready*/

        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx time-out interrupt */
    if (BL_IS_REG_BIT_SET(tmpVal,UART_URX_RTO_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_RTO_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x10);

        /*Receive Data ready*/
        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx parity check error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_PCE_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_PCE_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x20);
    }

    /* Tx fifo overflow/underflow error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_FER_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_FER_MASK)){
    }

    /* Rx fifo overflow/underflow error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_FER_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_FER_MASK)){
    }

    return;
}

static inline void uart_generic_notify_handler_yoc(void* argv)
{
    uint8_t id = ((hosal_uart_call_fuc_priv*)argv)->uart_id;
//    printf("irq\r\n");
    cb_uart_notify_t cb;
    void *arg;
    uint32_t tmpVal = 0;
    uint32_t maskVal = 0;
    uint32_t UARTx = uartAddr[id];

    tmpVal = BL_RD_REG(UARTx,UART_INT_STS);
    maskVal = BL_RD_REG(UARTx,UART_INT_MASK);


    /* Length of uart tx data transfer arrived interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_END_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_END_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x1);
    }

    /* Length of uart rx data transfer arrived interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_END_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_END_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x2);

        /*Receive Data ready*/
        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Tx fifo ready interrupt,auto-cleared when data is pushed */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_FRDY_MASK)){
        /* Transmit data request interrupt */
        cb = g_uart_notify_arg[id].tx_cb;
        arg = g_uart_notify_arg[id].tx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx fifo ready interrupt,auto-cleared when data is popped */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_FRDY_MASK)){
        /*Receive Data ready*/

        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx time-out interrupt */
    if (BL_IS_REG_BIT_SET(tmpVal,UART_URX_RTO_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_RTO_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x10);

        /*Receive Data ready*/
        cb = g_uart_notify_arg[id].rx_cb;
        arg = g_uart_notify_arg[id].rx_cb_arg;

        if (cb) {
            /*notify up layer*/
            cb(arg);
        }
    }

    /* Rx parity check error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_PCE_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_PCE_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x20);
    }

    /* Tx fifo overflow/underflow error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_FER_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_FER_MASK)){
    }

    /* Rx fifo overflow/underflow error interrupt */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_FER_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_FER_MASK)){
    }

    return;
}

#ifdef BFLB_USE_HAL_DRIVER
void UART0_IRQHandler(void)
{
    uart_generic_notify_handler(0);
}

void UART1_IRQHandler(void)
{
    uart_generic_notify_handler(1);
}

void UART2_IRQHandler(void)
{
    uart_generic_notify_handler(2);
}

void UART3_IRQHandler(void)
{
    uart_generic_notify_handler(3);
}
#endif

