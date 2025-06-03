#if 0
#include <bl606p_common.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include <bl606p_uart.h>

#include <stdint.h>
#include <drv/uart.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include "blyoc_uart.h"

#define UART_NUMBER_SUPPORTED       (3)
#define UART_FIFO_MAX_LEN           (128)
#define UART_DEFAULT_RTO_TIMEOUT    (100)

static const uint32_t uartAddr[4] = {UART0_BASE, UART1_BASE, UART2_BASE, UART3_BASE};

void blyoc_puts(char *str)
{
    if (str) {
#if defined CPU_M0
        blcsi_uart_send(CPU1_CONSOLE_UART_IDX, (uint8_t *)str, strlen((const char *)str));
#endif
#if defined CPU_D0
        blcsi_uart_send(CPU0_CONSOLE_UART_IDX, (uint8_t *)str, strlen((const char *)str));
#endif
    }
}

static void uart_gpio_demo(int id, uint8_t tx_pin, uint8_t rx_pin)
{
    GLB_GPIO_Cfg_Type gpio_cfg;
    uint8_t uart_func = 0;
    uint8_t uart_sig;

    gpio_cfg.drive = 0;
    gpio_cfg.smtCtrl = 1;
    gpio_cfg.gpioMode = GPIO_MODE_AF;
    gpio_cfg.pullType = GPIO_PULL_UP;
    gpio_cfg.gpioFun = GPIO_FUN_UART;

    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO12_GPIO23, 1);
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO24_GPIO35, 1);
    GLB_UART_Sig_Swap_Set(GLB_UART_SIG_SWAP_GRP_GPIO36_GPIO45, 1);

    gpio_cfg.gpioPin=tx_pin;
    if (id == 0) {
    	uart_func = GLB_UART_SIG_FUN_UART0_TXD;
    } else if (id == 1) {
    	uart_func = GLB_UART_SIG_FUN_UART1_TXD;
    }
    uart_sig = (gpio_cfg.gpioPin+6) % 12;
    /*link to one uart sig*/
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_sig, (GLB_UART_SIG_FUN_Type)uart_func);
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_func, (GLB_UART_SIG_FUN_Type)uart_sig);
    GLB_GPIO_Init(&gpio_cfg);

    gpio_cfg.gpioPin=rx_pin;
    if (id == 0) {
    	uart_func = GLB_UART_SIG_FUN_UART0_RXD;
    } else if (id == 1) {
    	uart_func = GLB_UART_SIG_FUN_UART1_RXD;
    }
    uart_sig = (gpio_cfg.gpioPin+6) % 12;
    /*link to one uart sig*/
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_sig, (GLB_UART_SIG_FUN_Type)uart_func);
    GLB_UART_Fun_Sel((GLB_UART_SIG_Type)uart_func, (GLB_UART_SIG_FUN_Type)uart_sig);
    GLB_GPIO_Init(&gpio_cfg);

    GLB_Set_UART_CLK(1, 0, 0);
}

static void uart_init_demo(uint8_t uart_id)
{
    static UART_CFG_Type uartCfg = {
        80*1000*1000,                                        /* UART clock */
        2000000,                                             /* UART Baudrate */
        UART_DATABITS_8,                                     /* UART data bits length */
        UART_STOPBITS_1,                                     /* UART data stop bits length */
        BLSTD_UART_PARITY_NONE,                              /* UART no parity */
        DISABLE,                                             /* Disable auto flow control */
        DISABLE,                                             /* Disable rx input de-glitch function */
        DISABLE,                                             /* Disable RTS output SW control mode */
        DISABLE,                                             /* Disable tx output SW control mode */
        DISABLE,                                             /* Disable tx lin mode */
        DISABLE,                                             /* Disable rx lin mode */
        0,                                                   /* Tx break bit count for lin mode */
        UART_LSB_FIRST                                       /* UART each data byte is send out LSB-first */
    };

    /* Disable all interrupt */
    UART_IntMask(uart_id, UART_INT_ALL,MASK);

    /* Disable uart before config */
    UART_Disable(uart_id, BLSTD_UART_TXRX);

    /* UART init */
    UART_Init(uart_id, &uartCfg);

    /* Enable tx free run mode */
    UART_TxFreeRun(uart_id, ENABLE);

    /* Enable uart */
    UART_Enable(uart_id, BLSTD_UART_TXRX);
}
static int __bl_sendchar(uint8_t id, uint8_t data);
static int blyoc_uart_gpio_init(uint8_t id, uint8_t tx_pin, uint8_t rx_pin, int baudrate)
{
    UART_CFG_Type uart_dbg_cfg = {
      40 * 1000 * 1000, /*UART clock*/
      baudrate,          /* baudrate  */
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
      16,      /* tx fifo threshold */
      16,      /* rx fifo threshold */
      DISABLE, /* disable tx dma req/ack interface */
      DISABLE  /* disable rx dma req/ack interface */
    };

    UART_IntMask(1, UART_INT_ALL, MASK);

    // uart_gpio_demo(id, tx_pin, rx_pin);
    // uart_init_demo(id);

    /* disable all interrupt */
    UART_IntMask(id, UART_INT_ALL, MASK);

    /* disable uart before config */
    UART_Disable(id, BLSTD_UART_TXRX);

    UART_Init(id, &uart_dbg_cfg);

    /* uart fifo configuration */
    UART_FifoConfig(id, &fifocfg);

    /* Enable tx free run mode */
    UART_TxFreeRun(id, ENABLE);

    /* Set rx time-out value */
    UART_SetRxTimeoutValue(id, 80);

    /* enable uart */
    UART_Enable(id, BLSTD_UART_TXRX);

#if defined CPU_M0
    blyoc_puts("(CPU_ID==M0)\r\n");
#endif
#if defined CPU_D0
    blyoc_puts("(CPU_ID==D0)\r\n");
#endif

    //blyoc_enable_cpu0();
    //blyoc_boot_cpu0(0x80000000);

    return 0;
}

void blcsi_uart_init(uint8_t id)
{
    if (0 == id) {
        blyoc_uart_gpio_init(0, 20, 21, 2000000);
    } else {
        blyoc_uart_gpio_init(id, 24, 25, 2000000);
    }
}

uint8_t blcsi_uart_getchar(uint8_t id)
{
    uint8_t ch;

    // blyoc_puts("blcsi_uart_getchar\r\n");

    if (UART_ReceiveData(id, &ch, 1)) {
        return ch;
    }
    return 0;
}

static int __bl_sendchar(uint8_t id, uint8_t data)
{

//    UART_SendData(id, &data,1);
    uint32_t UARTx = uartAddr[id];

    /* Wait for FIFO */
    while (UART_GetTxFifoCount(id) == 0) {
    }

    BL_WR_BYTE(UARTx + UART_FIFO_WDATA_OFFSET, data);

    return 0;
}

int32_t blcsi_uart_send(uint8_t id, uint8_t *buf, uint32_t len)
{
    int i;
    for (i = 0; i < len ; i++) {
        __bl_sendchar(id, buf[i]);
    }

    return len;
}

int blcsi_uart_receive()
{
    blyoc_puts("blcsi_uart_receive\r\n");
    return 0;
}

void blcsi_get_state()
{
    blyoc_puts("blcsi_get_state\r\n");
}


int bl_uart_int_rx_enable(uint8_t id)
{
    UART_SetRxTimeoutValue((UART_ID_Type)id, 24);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, UNMASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, UNMASK);
    return 0;
}

int bl_uart_int_rx_disable(uint8_t id)
{
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_FIFO_REQ, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RX_END, MASK);
    UART_IntMask((UART_ID_Type)id, UART_INT_RTO, MASK);
    return 0;
}

int bl_uart_int_tx_enable(uint8_t id)
{
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FIFO_REQ, UNMASK);
    return 0;
}

int bl_uart_int_tx_disable(uint8_t id)
{
    UART_IntMask((UART_ID_Type)id, UART_INT_TX_FIFO_REQ, MASK);
    return 0;
}

int bl_uart_data_send(uint8_t id, uint8_t data)
{
    UART_SendData((UART_ID_Type)id, &data, 1);
    return 0;
}

int bl_uart_datas_send(uint8_t id, uint8_t *data, int len)
{
    UART_SendData((UART_ID_Type)id, data, len);
    return 0;
}

static bl_uart_notify_t g_uart_notify_arg[2] = {
};

int bl_uart_int_rx_notify_register(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }

    g_uart_notify_arg[id].rx_cb = cb;
    g_uart_notify_arg[id].rx_cb_arg = arg;

    return 0;
}

int bl_uart_int_tx_notify_register(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }

    g_uart_notify_arg[id].tx_cb = cb;
    g_uart_notify_arg[id].tx_cb_arg = arg;

    return 0;
}

int bl_uart_int_rx_notify_unregister(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }
    g_uart_notify_arg[id].rx_cb = NULL;
    g_uart_notify_arg[id].rx_cb_arg = NULL;

    return 0;
}

int bl_uart_int_tx_notify_unregister(uint8_t id, cb_uart_notify_t cb, void *arg)
{
    if (!(id < UART_NUMBER_SUPPORTED)) {
        /*UART ID overflow*/
        return -1;
    }
    g_uart_notify_arg[id].tx_cb = NULL;
    g_uart_notify_arg[id].tx_cb_arg = NULL;

    return 0;
}

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
    }

    /* Rx time-out interrupt */
    //if (BL_IS_REG_BIT_SET(tmpVal,UART_URX_RTO_INT) && !BL_IS_REG_BIT_SET(maskVal,UART_CR_URX_RTO_MASK)){
        BL_WR_REG(UARTx,UART_INT_CLEAR,0x10);

        /*Receive Data ready*/
        uint8_t UART_GetRxFifoCount(UART_ID_Type uartId);
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
                        //dw_uart_getchar(uart_base);
                    // } while (--rxfifo_num);
                }
            }
        }
        //
    //}

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

void blcsi_uart_baud(UART_ID_Type uartId, uint32_t baud)
{
    uint32_t fraction = 0;
    uint32_t baud_ratedivisor = 0;
    uint32_t uart_base = uartAddr[uartId];
    uint32_t uart_clk = 0;

    /* Disable clock gate when use UART2 */
    if(uartId == BLSTD_UART2_ID){
        GLB_PER_Clock_UnGate(GLB_AHB_CLOCK_UART2);
    }
#if defined CPU_D0
    uart_clk = 50 * 1000 * 1000;
#endif
#if defined CPU_M0
    uart_clk = 32 * 1000 * 1000;
#endif

    /* Cal the baud rate divisor */
    fraction = uart_clk * 10 / baud % 10;
    baud_ratedivisor = uart_clk / baud;

    if (fraction >= 5) {
        ++baud_ratedivisor;
    }

    /* Set the baud rate register value */
    BL_WR_REG(uart_base, UART_BIT_PRD, ((baud_ratedivisor - 1) << 0x10) | ((baud_ratedivisor - 1) & 0xFFFF));
}

static void gpio_uninit_uart(uint8_t id, uint8_t tx_pin, uint8_t rx_pin, uint8_t cts_pin, uint8_t rts_pin)
{
    GLB_GPIO_Cfg_Type cfg;

    cfg.pullType = GPIO_PULL_NONE;
    cfg.drive = 0;
    cfg.smtCtrl = 1;

    cfg.gpioPin = tx_pin;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.gpioMode = GPIO_MODE_OUTPUT;
    GLB_GPIO_Init(&cfg);

    cfg.gpioPin = rx_pin;
    cfg.gpioFun = GPIO_FUN_GPIO;
    cfg.gpioMode = GPIO_MODE_OUTPUT;
    GLB_GPIO_Init(&cfg);

}

static int blyoc_uart_uninit(uint8_t id, uint8_t tx_pin, uint8_t rx_pin, int baudrate)
{
    /* gpio uninit uart */
    gpio_uninit_uart(id, tx_pin, rx_pin, 0xFF, 0xFF);

    /* disable all interrupt */
    UART_IntMask(id, UART_INT_ALL, MASK);

    /* disable uart before config */
    UART_Disable(id, BLSTD_UART_TXRX);

    return 0;
}

void blcsi_uart_uninit(uint8_t id)
{
	if(id > 1){
		return;
	}

    if (0 == id) {
    	blyoc_uart_uninit(id, 16, 17, 2000000);
    } else {
    	blyoc_uart_uninit(id, 39, 8, 2000000);
    }
}
#endif