#include <drv/uart.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>
#include <soc.h>
#include <drv/tick.h>
#include <bl606p_common.h>
#include <bl606p_uart.h>
#include <bl606p_glb.h>

static const uint32_t uartAddr[4] = {UART0_BASE, UART1_BASE, UART2_BASE, UART3_BASE};

void uart_generic_notify_handler(void *arg)
{
    csi_uart_t *uart = (csi_uart_t *)arg;
    uint8_t id = uart->dev.idx;

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
    }

    /* Tx fifo ready interrupt,auto-cleared when data is pushed */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_UTX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_UTX_FRDY_MASK)){
        /* Transmit data request interrupt */
    }

    /* Rx fifo ready interrupt,auto-cleared when data is popped */
    if(BL_IS_REG_BIT_SET(tmpVal,UART_URX_FRDY_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_FRDY_MASK)){
        /*Receive Data ready*/
        if (UART_GetRxFifoCount(id)) {
            if ((uart->rx_data == NULL) || (uart->rx_size == 0U)) {
                if (uart->callback) {
                    uart->callback(uart, UART_EVENT_RECEIVE_FIFO_READABLE, uart->arg);
                } else {
                    // do {
                        uint8_t data;
                        while(UART_GetRxFifoCount(id)) {
                            UART_ReceiveData(id, &data, 1);
                        }
                }
            }
        }
        else
        {
            uart->rx_size -= UART_ReceiveData(id,uart->rx_data,uart->rx_size);
            if (uart->callback) {
                uart->callback(uart, UART_EVENT_RECEIVE_COMPLETE, uart->arg);
            }
        }
    }

    /* Rx time-out interrupt */
    if (BL_IS_REG_BIT_SET(tmpVal,UART_URX_RTO_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_RTO_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x10);

        /*Receive Data ready*/
        if (UART_GetRxFifoCount(id)) {
            if ((uart->rx_data == NULL) || (uart->rx_size == 0U)) {
                if (uart->callback) {
                    uart->callback(uart, UART_EVENT_RECEIVE_FIFO_READABLE, uart->arg);
                } else {
                    // do {
                        uint8_t data;
                        while(UART_GetRxFifoCount(id)) {
                            UART_ReceiveData(id, &data, 1);
                        }
                }
            }
            else
            {
                uart->rx_size -= UART_ReceiveData(id,uart->rx_data,uart->rx_size);
                if (uart->callback) {
                    uart->callback(uart, UART_EVENT_RECEIVE_COMPLETE, uart->arg);
                }
            }
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

csi_error_t csi_uart_init(csi_uart_t *uart, uint32_t idx)
{
    csi_error_t ret = CSI_OK;
    IRQn_Type irqn;

    if (idx > 3) {
        return CSI_ERROR;
    }

    uart->rx_size = 0U;
    uart->tx_size = 0U;
    uart->rx_data = NULL;
    uart->tx_data = NULL;
    uart->tx_dma  = NULL;
    uart->rx_dma  = NULL;

    uart->dev.idx = idx;

    if (idx == 0) {
        irqn = UART0_IRQn;
    } else if (idx == 1) {
        irqn = UART1_IRQn;
    } else if (idx == 2) {
        irqn = UART2_IRQn;
    } else {
        irqn = UART3_IRQn;
    }
    uart->dev.irq_num = irqn;

    UART_CFG_Type uart_cfg = {
      40 * 1000 * 1000, /*UART clock*/
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

    UART_FifoCfg_Type fifocfg = {
      15,      /* tx fifo threshold */
      15,      /* rx fifo threshold */
      DISABLE, /* disable tx dma req/ack interface */
      DISABLE  /* disable rx dma req/ack interface */
    };

    /* disable all interrupt */
    UART_IntMask(idx, UART_INT_ALL, MASK);

    /* disable uart before config */
    UART_Disable(idx, BLSTD_UART_TXRX);

    /* uart init with default configuration */
    UART_Init(idx, &uart_cfg);

    /* UART fifo configuration */
    UART_FifoConfig(idx, &fifocfg);

    /* Enable tx free run mode */
    UART_TxFreeRun(idx, ENABLE);

    /* Set rx time-out value */
    UART_SetRxTimeoutValue(idx, 80);

    UART_TxFifoClear(idx);
    UART_RxFifoClear(idx);

    /* enable uart */
    UART_Enable(idx, BLSTD_UART_TXRX);
    return ret;
}

void csi_uart_uninit(csi_uart_t *uart)
{
    uint8_t id = uart->dev.idx;

    uart->rx_size = 0U;
    uart->tx_size = 0U;
    uart->rx_data = NULL;
    uart->tx_data = NULL;
    uart->tx_dma  = NULL;
    uart->rx_dma  = NULL;

    /* disable all interrupt */
    UART_IntMask(id, UART_INT_ALL, MASK);

    /* disable uart before config */
    UART_Disable(id, BLSTD_UART_TXRX);
}

csi_error_t csi_uart_baud(csi_uart_t *uart, uint32_t baud)
{
    csi_error_t ret = CSI_OK;

    uint32_t fraction = 0;
    uint32_t baudRateDivisor = 0;

    uint8_t id = uart->dev.idx;

    uint32_t UARTx = uartAddr[id];

    /* Cal the baud rate divisor */
    fraction = (40 * 1000 * 1000) * 10 / baud % 10;
    baudRateDivisor = (40 * 1000 * 1000) / baud;

    if (fraction >= 5) {
        ++baudRateDivisor;
    }

    /* disable uart before config */
    UART_Disable(id, BLSTD_UART_TXRX);

    /* Set the baud rate register value */
    BL_WR_REG(UARTx, UART_BIT_PRD, ((baudRateDivisor - 1) << 0x10) | ((baudRateDivisor - 1) & 0xFFFF));

    /* enable uart */
    UART_Enable(id, BLSTD_UART_TXRX);
    return ret;
}

csi_error_t csi_uart_format(csi_uart_t *uart,  csi_uart_data_bits_t data_bits,
                            csi_uart_parity_t parity, csi_uart_stop_bits_t stop_bits)
{
    csi_error_t ret = CSI_OK;
    uint32_t tmpValTxCfg = 0;
    uint32_t tmpValRxCfg = 0;
    uint32_t UARTx;

    uint8_t id = uart->dev.idx;

    UARTx = uartAddr[id];

    UART_Disable(id, BLSTD_UART_TXRX);

    /* Configure parity type */
    tmpValTxCfg = BL_RD_REG(UARTx, UART_UTX_CONFIG);
    tmpValRxCfg = BL_RD_REG(UARTx, UART_URX_CONFIG);

    switch (parity) {
        case UART_PARITY_NONE:
            tmpValTxCfg = BL_CLR_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValRxCfg = BL_CLR_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            break;

        case UART_PARITY_ODD:
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_SEL);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_SEL);
            break;

        case UART_PARITY_EVEN:
            tmpValTxCfg = BL_SET_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_EN);
            tmpValTxCfg = BL_CLR_REG_BIT(tmpValTxCfg, UART_CR_UTX_PRT_SEL);
            tmpValRxCfg = BL_SET_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_EN);
            tmpValRxCfg = BL_CLR_REG_BIT(tmpValRxCfg, UART_CR_URX_PRT_SEL);
            break;

        default:
            break;
    }

    /* Configure data bits */
    tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_D, (data_bits + 4));
    tmpValRxCfg = BL_SET_REG_BITS_VAL(tmpValRxCfg, UART_CR_URX_BIT_CNT_D, (data_bits + 4));

    /* Configure tx stop bits */
    if(stop_bits == UART_STOP_BITS_1)
        tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_P, 1);
    else if(stop_bits == UART_STOP_BITS_1_5)
        tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_P, 2);
    else if(stop_bits == UART_STOP_BITS_2)
        tmpValTxCfg = BL_SET_REG_BITS_VAL(tmpValTxCfg, UART_CR_UTX_BIT_CNT_P, 3);

    /* Write back */
    BL_WR_REG(UARTx, UART_UTX_CONFIG, tmpValTxCfg);
    BL_WR_REG(UARTx, UART_URX_CONFIG, tmpValRxCfg);

    /* enable uart */
    UART_Enable(id, BLSTD_UART_TXRX);
    return ret;
}

csi_error_t csi_uart_flowctrl(csi_uart_t *uart,  csi_uart_flowctrl_t flowctrl)
{
    return 0;
}

void csi_uart_putc(csi_uart_t *uart, uint8_t ch)
{
	uint8_t id = uart->dev.idx;
    UART_SendData(id, &ch, 1);
}

uint8_t csi_uart_getc(csi_uart_t *uart)
{
	uint8_t id = uart->dev.idx;
    uint8_t ch = 0;
    /* add bl code here */
    while(UART_ReceiveData(id, &ch, 1) == 0);
    return ch;
}

int32_t csi_uart_receive(csi_uart_t *uart, void *data, uint32_t size, uint32_t timeout)
{
    uint32_t len = 0;
    uint8_t id = uart->dev.idx;

    uint32_t start_time = csi_tick_get_ms();

    while(len < size)
    {
        len += UART_ReceiveData(id, data, 32);
        if((csi_tick_get_ms() - start_time) >= timeout)
        {
            break;
        }
    }
    return len;
}

csi_error_t csi_uart_receive_async(csi_uart_t *uart, void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;

    uart->rx_data = (uint8_t*)data;
    uart->rx_size = size;
    return ret;
}

int32_t csi_uart_send(csi_uart_t *uart, const void *data, uint32_t size, uint32_t timeout)
{
    uint8_t id = uart->dev.idx;
    /* add bl code here */

    UART_SendData(id, (uint8_t *)data, size);
    return size;
}

csi_error_t csi_uart_send_async(csi_uart_t *uart, const void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    uint8_t id = uart->dev.idx;

    UART_SendData(id, (uint8_t *)data, size);

    if (uart->callback) {
        uart->callback(uart, UART_EVENT_SEND_COMPLETE, uart->arg);
    }

    return ret;
}

void all_e907_irq_entry(void *arg)
{
	const uint8_t e907_uart_irq[] = {
			GLB_MCU_ALL_INT_UART_IRQ,
			GLB_MCU_ALL_INT_UART1_IRQ,
			GLB_MCU_ALL_INT_UART2_IRQ,
	};
	csi_uart_t *uart = (csi_uart_t *)arg;
	uint8_t id = uart->dev.idx;

	if (GLB_DSP_Get_MCU_IntStatus(e907_uart_irq[id])) {
		uart_generic_notify_handler(uart);
	}
	for (int i = 0; i < 64; i++) {
		if (GLB_DSP_Get_MCU_IntStatus(i)) {
			GLB_DSP_Clr_MCU_IntStatus(i);
		}
	}
}

csi_error_t csi_uart_attach_callback(csi_uart_t *uart, void  *callback, void *arg)
{
    CSI_PARAM_CHK(uart, CSI_ERROR);
    uint8_t id = uart->dev.idx;
    IRQn_Type irqn;

    uart->callback = callback;
    uart->arg = arg;
    irqn = uart->dev.irq_num;
    csi_irq_attach(irqn, uart_generic_notify_handler, &uart->dev);
    csi_irq_enable(irqn);

    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, UNMASK);
    // UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, UNMASK);
    return CSI_OK;
}

void csi_uart_detach_callback(csi_uart_t *uart)
{
    uint8_t id = uart->dev.idx;
    IRQn_Type irqn;

    uart->callback = NULL;
    uart->arg = NULL;
    //uart->send = dw_uart_send_intr;
    //uart->receive = dw_uart_receive_intr;
    if (id == 0) {
        irqn = UART0_IRQn;
    } else if (id == 1) {
        irqn = UART1_IRQn;
    } else if (id == 2) {
        irqn = UART2_IRQn;
    } else {
        irqn = UART3_IRQn;
    }
    uart->dev.irq_num = irqn;

    csi_irq_detach(irqn);
    csi_irq_disable(irqn);

    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, MASK);
}

csi_error_t csi_uart_get_state(csi_uart_t *uart, csi_state_t *state)
{
    /* add bl code here */
    //ch = blcsi_get_state();
    return CSI_OK;
}

csi_error_t csi_uart_link_dma(csi_uart_t *uart, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    csi_error_t ret = CSI_OK;
    return ret;
}
