/**
 * Copyright (c) 2016-2021 Bouffalolab Co., Ltd.
 *
 * Contact information:
 * web site:    https://www.bouffalolab.com/
 */
#include "peripherals_config.h"
#include <hosal_uart.h>

#include <bl_irq.h>
#include <bl_gpio.h>

#include "../bl_code/bl_uart.h"



static void __uart_rx_irq(void *p_arg)
{
    hosal_uart_dev_t *uart = (hosal_uart_dev_t *)p_arg;
    if (uart->rx_cb) {
        uart->rx_cb(uart->p_rxarg);
    }
}

static void __uart_tx_irq(void *p_arg)
{
    hosal_uart_dev_t *uart = (hosal_uart_dev_t *)p_arg;
    if (uart->tx_cb) {
        uart->tx_cb(uart->p_txarg);
    }
}
#if 0
static void __uart_rx_dma_irq(void *p_arg, uint32_t flag)
{
    hosal_uart_dev_t *uart = (hosal_uart_dev_t *)p_arg;

    if (flag != HOSAL_DMA_INT_TRANS_COMPLETE) {
    	blog_error("DMA RX TRANS ERROR\r\n");
    }

    if (uart->rxdma_cb) {
    	uart->rxdma_cb(uart->p_rxdma_arg);
    }
}

static void __uart_tx_dma_irq(void *p_arg, uint32_t flag)
{
    hosal_uart_dev_t *uart = (hosal_uart_dev_t *)p_arg;

    if (flag != HOSAL_DMA_INT_TRANS_COMPLETE) {
    	blog_error("DMA TX TRANS ERROR\r\n");
    }

    if (uart->txdma_cb) {
    	uart->txdma_cb(uart->p_txdma_arg);
    }
}

static int __uart_dma_txcfg(hosal_uart_dev_t *uart, hosal_uart_dma_cfg_t *dma_cfg)
{
	if (dma_cfg->dma_buf == NULL || dma_cfg->dma_buf_size == 0) {
		return -1;
	}
	DMA_Channel_Cfg_Type txchCfg = {
	    (uint32_t)dma_cfg->dma_buf,
		g_uart_addr[uart->port] + UART_FIFO_WDATA_OFFSET,
		dma_cfg->dma_buf_size,
	    DMA_TRNS_M2P,
		DMA_CH0,
	    DMA_TRNS_WIDTH_8BITS,
	    DMA_TRNS_WIDTH_8BITS,
	    DMA_BURST_SIZE_4,
	    DMA_BURST_SIZE_4,
	    DMA_MINC_ENABLE,
	    DMA_PINC_DISABLE,
	    DMA_REQ_NONE,
	    DMA_REQ_UART0_TX,
	};
    UART_FifoCfg_Type fifoCfg =
    {
        .txFifoDmaThreshold     = 0x10,
        .rxFifoDmaThreshold     = 0x10,
        .txFifoDmaEnable        = ENABLE,
        .rxFifoDmaEnable        = DISABLE,
    };

    if (uart->dma_tx_chan >= 0) {
    	DMA_Channel_Update_SrcMemcfg(uart->dma_tx_chan,
    			(uint32_t)dma_cfg->dma_buf, dma_cfg->dma_buf_size);
    	return 0;
    }

	uart->dma_tx_chan = hosal_dma_chan_request(0);
	if (uart->dma_tx_chan < 0) {
		blog_error("dma_tx_chan request failed !\r\n");
		return -1;
	}

	hosal_dma_chan_stop(uart->dma_tx_chan);

    /* FIFO Config*/
	fifoCfg.rxFifoDmaEnable = (uart->dma_rx_chan < 0) ? DISABLE : ENABLE;
    UART_FifoConfig(uart->port, &fifoCfg);

	txchCfg.ch = uart->dma_tx_chan;
	txchCfg.dstPeriph = (uart->port == 0) ? DMA_REQ_UART0_TX : DMA_REQ_UART1_TX;
	DMA_Channel_Init(&txchCfg);
	hosal_dma_irq_callback_set(uart->dma_tx_chan, __uart_tx_dma_irq, (void *)uart);

	return 0;
}

static int __uart_dma_rxcfg(hosal_uart_dev_t *uart, hosal_uart_dma_cfg_t *dma_cfg)
{
	if (dma_cfg->dma_buf == NULL || dma_cfg->dma_buf_size == 0) {
		return -1;
	}

	DMA_Channel_Cfg_Type rxchCfg = {
		g_uart_addr[uart->port] + UART_FIFO_RDATA_OFFSET,
		(uint32_t)dma_cfg->dma_buf,
		dma_cfg->dma_buf_size,
	    DMA_TRNS_P2M,
		DMA_CH0,
	    DMA_TRNS_WIDTH_8BITS,
	    DMA_TRNS_WIDTH_8BITS,
	    DMA_BURST_SIZE_16,
	    DMA_BURST_SIZE_16,
	    DMA_PINC_DISABLE,
	    DMA_MINC_ENABLE,
	    DMA_REQ_UART0_RX,
	    DMA_REQ_NONE,
	};
    UART_FifoCfg_Type fifoCfg =
    {
        .txFifoDmaThreshold     = 0x10,
        .rxFifoDmaThreshold     = 0x10,
        .txFifoDmaEnable        = DISABLE,
        .rxFifoDmaEnable        = ENABLE,
    };

    if (uart->dma_rx_chan >= 0) {
    	DMA_Channel_Update_DstMemcfg(uart->dma_rx_chan,
    			(uint32_t)dma_cfg->dma_buf, dma_cfg->dma_buf_size);
    	return 0;
    }

	uart->dma_rx_chan = hosal_dma_chan_request(0);
	if (uart->dma_rx_chan < 0) {
		blog_error("dma_rx_chan request failed !\r\n");
		return -1;
	}

	hosal_dma_chan_stop(uart->dma_rx_chan);

    /* FIFO Config*/
	fifoCfg.txFifoDmaEnable = (uart->dma_tx_chan < 0) ? DISABLE : ENABLE;
    UART_FifoConfig(uart->port, &fifoCfg);

	rxchCfg.ch = uart->dma_rx_chan;
	rxchCfg.srcPeriph = (uart->port == 0) ? DMA_REQ_UART0_RX : DMA_REQ_UART1_RX;

	DMA_Channel_Init(&rxchCfg);
	hosal_dma_irq_callback_set(uart->dma_rx_chan, __uart_rx_dma_irq, (void *)uart);

	return 0;
}
#endif
static void __uart_config_set(hosal_uart_dev_t *uart, int ctl, const hosal_uart_config_t *cfg)
{
    bl_uart_config_t cfg_set = {0};

    cfg_set.uart_id = (uint8_t)cfg->uart_id;

    switch (ctl) {
        case HOSAL_UART_BAUD_SET:
            cfg_set.baud_rate = cfg->baud_rate;
            bl_uart_config(cfg_set.uart_id, bl_uart_set_baud_cmd, &cfg_set);
            break;
        case HOSAL_UART_DATA_WIDTH_SET:
            cfg_set.data_width = cfg->data_width;
            bl_uart_config(cfg_set.uart_id, bl_uart_set_data_bit_cmd, &cfg_set);
            break;
        case HOSAL_UART_STOP_BITS_SET:
            cfg_set.stop_bits = cfg->stop_bits;
            bl_uart_config(cfg_set.uart_id, bl_uart_set_stop_bit_cmd, &cfg_set);
            break;
        case HOSAL_UART_FLOWMODE_SET:

            break;
        case HOSAL_UART_PARITY_SET:
            cfg_set.parity = cfg->parity;
            bl_uart_config(cfg_set.uart_id, bl_uart_set_parity_bit_cmd, &cfg_set);
            break;
        case HOSAL_UART_MODE_SET:
            if (cfg->mode == HOSAL_UART_MODE_INT) {
                bl_uart_irq_disable(uart->port);
                bl_uart_irq_tx_notify_register(uart->port, __uart_tx_irq, uart);
                bl_uart_irq_rx_notify_register(uart->port, __uart_rx_irq, uart);
                bl_uart_irq_enable(uart->port);
            } else {
                bl_uart_irq_disable(uart->port);
            }
            break;
        default:
            break;
    }

}
int hosal_uart_abr_get(hosal_uart_dev_t *uart, uint8_t mode)
{
    return -1;
}
int hosal_uart_init(hosal_uart_dev_t *uart)
{
    //static uint8_t uart_clk_init = 0;
    //const uint8_t uart_div = 3;
    hosal_uart_config_t *cfg = &uart->config;

    /* enable clk */

#if 0
    if (0 == uart_clk_init) {
        GLB_Set_UART_CLK(1, HBN_UART_CLK_160M, uart_div);
        uart_clk_init = 1;
    }
#endif

    uart->dma_rx_chan = -1;
    uart->dma_tx_chan = -1;
    uart->port = cfg->uart_id;

    if (uart->port == 0 || uart->port == 3) {
        printf("not support uart0 and uart3\r\n");
        return -1;
    }

    bl_uart_init(uart->port, uart->config.tx_pin, uart->config.rx_pin,
                             bl_glb_gpio_pin_max, bl_glb_gpio_pin_max, uart->config.baud_rate);

    if (cfg->mode == HOSAL_UART_MODE_INT) {
        bl_uart_irq_disable(uart->port);
    	bl_uart_irq_tx_notify_register(uart->port, __uart_tx_irq, uart);
    	bl_uart_irq_rx_notify_register(uart->port, __uart_rx_irq, uart);
    	bl_uart_irq_enable(uart->port);
    } else {
    	bl_uart_irq_disable(uart->port);
    }

    /* Enable uart */
    bl_uart_port_enable(uart->port);

    return 0;
}

int hosal_uart_receive(hosal_uart_dev_t *uart, void *data, uint32_t expect_size)
{
    //int ch;
    uint32_t counter = 0;

    counter = bl_uart_buffs_recv(uart->port, data, expect_size);
    #if 0
    while (counter < expect_size) {
        if ((ch = bl_uart_buff_recv(uart->port)) < 0) {
            break;
        }
        ((uint8_t*)data)[counter] = ch;
        counter++;
    }
    #endif
    return counter;
}

int hosal_uart_send(hosal_uart_dev_t *uart, const void *data, uint32_t size)
{
    uint32_t i = 0;

    while (i < size) {
        bl_uart_buff_send(uart->port, ((uint8_t*)data)[i]);
        i++;
    }
    return i;
}

int hosal_uart_ioctl(hosal_uart_dev_t *uart, int ctl, void *p_arg)
{
#if 0
	hosal_uart_dma_cfg_t *dma_cfg;
#endif

    switch (ctl) {
    case HOSAL_UART_BAUD_SET:
        uart->config.baud_rate = (uint32_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_BAUD_SET, &uart->config);
        break;
    case HOSAL_UART_BAUD_GET:
        if (p_arg) {
            *(uint32_t *)p_arg = uart->config.baud_rate;
        }
        break;
    case HOSAL_UART_DATA_WIDTH_SET:
        uart->config.data_width = (hosal_uart_data_width_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_DATA_WIDTH_SET, &uart->config);
        break;
    case HOSAL_UART_DATA_WIDTH_GET:
        if (p_arg) {
            *(hosal_uart_data_width_t *)p_arg = uart->config.data_width;
        }
        break;
    case HOSAL_UART_STOP_BITS_SET:
        uart->config.stop_bits = (hosal_uart_stop_bits_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_STOP_BITS_SET, &uart->config);
        break;
    case HOSAL_UART_STOP_BITS_GET:
        if (p_arg) {
            *(hosal_uart_stop_bits_t *)p_arg = uart->config.stop_bits;
        }
        break;
    case HOSAL_UART_FLOWMODE_SET:
        uart->config.flow_control = (hosal_uart_flow_control_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_FLOWMODE_SET, &uart->config);
        break;
    case HOSAL_UART_FLOWSTAT_GET:
        if (p_arg) {
            *(hosal_uart_flow_control_t *)p_arg = uart->config.flow_control;
        }
        break;
    case HOSAL_UART_PARITY_SET:
        uart->config.parity = (hosal_uart_parity_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_PARITY_SET, &uart->config);
        break;
    case HOSAL_UART_PARITY_GET:
        if (p_arg) {
            *(hosal_uart_parity_t *)p_arg = uart->config.parity;
        }
        break;
    case HOSAL_UART_MODE_SET:
        uart->config.mode = (hosal_uart_mode_t)p_arg;
        __uart_config_set(uart, HOSAL_UART_MODE_SET, &uart->config);
        break;
    case HOSAL_UART_MODE_GET:
        if (p_arg) {
            *(hosal_uart_mode_t *)p_arg = uart->config.mode;
        }
        break;
    case HOSAL_UART_FREE_TXFIFO_GET:
        if (p_arg) {
            *(uint32_t *)p_arg = bl_uart_get_txfifo_count(uart->port);
        }
        break;
    case HOSAL_UART_FREE_RXFIFO_GET:
        if (p_arg) {
            *(uint32_t *)p_arg = bl_uart_get_rxfifo_count(uart->port);
        }
        break;
    case HOSAL_UART_FLUSH:
        bl_uart_flush(uart->port);
        break;
    case HOSAL_UART_TX_TRIGGER_ON:
    	bl_uart_irq_tx_enable(uart->port);
    	break;
    case HOSAL_UART_TX_TRIGGER_OFF:
    	bl_uart_irq_tx_disable(uart->port);
    	break;
#if 0
    case HOSAL_UART_DMA_TX_START:
    	dma_cfg = (hosal_uart_dma_cfg_t *)p_arg;
    	if (__uart_dma_txcfg(uart, dma_cfg) != 0) {
    		return -1;
    	}
    	hosal_dma_chan_start(uart->dma_tx_chan);
    	break;
    case HOSAL_UART_DMA_RX_START:
    	dma_cfg = (hosal_uart_dma_cfg_t *)p_arg;
    	if (__uart_dma_rxcfg(uart, dma_cfg) != 0) {
    		return -1;
    	}
    	hosal_dma_chan_start(uart->dma_rx_chan);
    	break;
#endif
    default :
        return -1;
    }
    return 0;
}

int hosal_uart_callback_set(hosal_uart_dev_t *uart,
                          int callback_type,
                          hosal_uart_callback_t pfn_callback,
                          void *arg)
{
    if (callback_type == HOSAL_UART_TX_CALLBACK) {
        uart->tx_cb = pfn_callback;
        uart->p_txarg = arg;
    } else if (callback_type == HOSAL_UART_RX_CALLBACK) {
        uart->rx_cb = pfn_callback;
        uart->p_rxarg = arg;
    }
#if 0
    else if (callback_type == HOSAL_UART_TX_DMA_CALLBACK) {
        uart->txdma_cb = pfn_callback;
        uart->p_txdma_arg = arg;
    } else if (callback_type == HOSAL_UART_RX_DMA_CALLBACK) {
        uart->rxdma_cb = pfn_callback;
        uart->p_rxdma_arg = arg;
    }
#endif
    return 0;
}

int hosal_uart_finalize(hosal_uart_dev_t *uart)
{
    bl_uart_irq_disable(uart->port);
    bl_uart_port_disable(uart->port);
#if 0
    if (uart->dma_rx_chan > 0) {
    	hosal_dma_chan_release(uart->dma_rx_chan);
    }
    if (uart->dma_tx_chan > 0) {
    	hosal_dma_chan_release(uart->dma_tx_chan);
    }
#endif
    return 0;
}
