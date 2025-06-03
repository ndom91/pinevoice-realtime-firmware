/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     uart.c
 * @brief    CSI Source File for uart Driver
 * @version  V2.01
 * @date     2020-04-09
 ******************************************************************************/
#if 0
#include <drv/uart.h>
#include <drv/dma.h>
#include <drv/irq.h>
#include <drv/gpio.h>
#include <drv/pin.h>
#include <drv/porting.h>
#include <soc.h>
#include <dw_uart_ll.h>
#include <drv/tick.h>

#include <bl606p.h>
#include <bl606p_glb.h>
#include <bl606p_uart.h>
#include <blyoc_uart/blyoc_uart.h>

void blyoc_puts(char *str);

uint8_t blcsi_uart_getchar(uint8_t id);
int32_t blcsi_uart_send(uint8_t id, uint8_t *buf, uint32_t len);
int blcsi_uart_receive();
void blcsi_get_state();

static uint8_t g_csi_uart_init = 0;
csi_error_t csi_uart_init(csi_uart_t *uart, uint32_t idx)
{
    csi_error_t ret = CSI_OK;

    uart->rx_size = 0U;
    uart->tx_size = 0U;
    uart->rx_data = NULL;
    uart->tx_data = NULL;
    uart->tx_dma  = NULL;
    uart->rx_dma  = NULL;

    uart->dev.idx = idx;
    blcsi_uart_init(idx);
    g_csi_uart_init = 1;

    return ret;
}

void csi_uart_uninit(csi_uart_t *uart)
{
    uart->rx_size = 0U;
    uart->tx_size = 0U;
    uart->rx_data = NULL;
    uart->tx_data = NULL;
    uart->tx_dma  = NULL;
    uart->rx_dma  = NULL;

#ifdef CPU_D0
    uart->dev.idx = CPU0_CONSOLE_UART_IDX;
    blcsi_uart_uninit(CPU0_CONSOLE_UART_IDX);
#endif
#ifdef CPU_M0
    uart->dev.idx = CPU1_CONSOLE_UART_IDX;
    blcsi_uart_uninit(CPU1_CONSOLE_UART_IDX);
#endif
    g_csi_uart_init = 0;
}

ATTRIBUTE_DATA csi_error_t csi_uart_baud(csi_uart_t *uart, uint32_t baud)
{
    csi_error_t ret = CSI_OK;

//    blcsi_uart_baud(uart->dev.idx, baud);

    return ret;
}

csi_error_t csi_uart_format(csi_uart_t *uart,  csi_uart_data_bits_t data_bits,
                            csi_uart_parity_t parity, csi_uart_stop_bits_t stop_bits)
{
    csi_error_t ret = CSI_OK;
    return ret;
}

csi_error_t csi_uart_flowctrl(csi_uart_t *uart,  csi_uart_flowctrl_t flowctrl)
{
    return 0;
}

void csi_uart_putc(csi_uart_t *uart, uint8_t ch)
{
	uint8_t id = uart->dev.idx;

    if (g_csi_uart_init) {
        blcsi_uart_send(id, &ch, 1);
    }
}

ATTRIBUTE_DATA uint8_t csi_uart_getc(csi_uart_t *uart)
{
	uint8_t id = uart->dev.idx;
    uint8_t ch = 0;
    /* add bl code here */

    ch = blcsi_uart_getchar(id);
    return ch;
}

int32_t csi_uart_receive(csi_uart_t *uart, void *data, uint32_t size, uint32_t timeout)
{
    uint8_t id = uart->dev.idx;

    return UART_ReceiveData(id, data, size);

    //return size;
}

csi_error_t csi_uart_receive_async(csi_uart_t *uart, void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;

    blyoc_puts("csi_uart_receive_async\r\n");

    return ret;
}

int32_t csi_uart_send(csi_uart_t *uart, const void *data, uint32_t size, uint32_t timeout)
{
    int32_t trans_num = 0;
    uint8_t id = uart->dev.idx;
    /* add bl code here */

    trans_num = blcsi_uart_send(id, (uint8_t *)data, size);
    return trans_num;
}

csi_error_t csi_uart_send_async(csi_uart_t *uart, const void *data, uint32_t size)
{
    csi_error_t ret = CSI_OK;
    uint8_t id = uart->dev.idx;

    //blyoc_puts("csi_uart_send_async\r\n");

    blcsi_uart_send(id, (uint8_t *)data, size);

    if (uart->callback) {
        uart->callback(uart, UART_EVENT_SEND_COMPLETE, uart->arg);
    }

    return ret;
}

void uart_generic_notify_handler(void *arg);

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

void bl606_uart_handler(void *arg)
{
    uart_generic_notify_handler(arg);
}

csi_error_t csi_uart_attach_callback(csi_uart_t *uart, void  *callback, void *arg)
{
    CSI_PARAM_CHK(uart, CSI_ERROR);
    uint8_t id = uart->dev.idx;
    IRQn_Type irqn;

    //dw_uart_regs_t *uart_base;
    //uart_base = (dw_uart_regs_t *)HANDLE_REG_BASE(uart);

    uart->callback = callback;
    uart->arg = arg;
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
#ifdef CPU_M0
    csi_irq_attach(irqn, &uart_generic_notify_handler, &uart->dev);
    csi_irq_enable(irqn);
#endif
#ifdef CPU_D0
    void all_e907_irq_entry(void *arg);
    csi_irq_attach(WL_ALL_IRQn, all_e907_irq_entry, &uart->dev);
    csi_irq_enable(WL_ALL_IRQn);
    csi_vic_set_pending_irq(WL_ALL_IRQn);
    //GLB_DSP_Set_MCU_IntMask(GLB_MCU_ALL_INT_UART1_IRQ, UNMASK);
#endif

    //dw_uart_enable_recv_irq(uart_base);
    int bl_uart_int_rx_enable(uint8_t id);
    bl_uart_int_rx_enable(id);

    return CSI_OK;

    //blyoc_puts("csi_uart_attach_callback\r\n");
}

void csi_uart_detach_callback(csi_uart_t *uart)
{
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
#endif