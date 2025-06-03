/*
 * Copyright (C) 2017-2020 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     devices.c
 * @brief    source file for the devices
 * @version  V1.0
 * @date     2019-12-18
******************************************************************************/

#include <stdio.h>
#include <csi_config.h>
#include <soc.h>
#include <drv/uart.h>
#include <drv/timer.h>
#include <drv/dma.h>
#include <drv/iic.h>
#include <drv/gpio.h>
#include <drv/irq.h>
#include <drv/pin.h>
#include <drv/i2s.h>

const csi_perip_info_t g_soc_info[] = {
    {DW_UART_BASE,             DW_UART_IRQn,             0,    DEV_DW_UART_TAG},
    // {DW_DMA_BASE,              DW_DMA_IRQn,              0,    DEV_DW_DMA_TAG},
    // {DW_TIMER0_BASE,           DW_TIMER0_IRQn,           0,    DEV_DW_TIMER_TAG},
    // {DW_TIMER1_BASE,           DW_TIMER1_IRQn,           1,    DEV_DW_TIMER_TAG},
    // {DW_TIMER2_BASE,           DW_TIMER2_IRQn,           2,    DEV_DW_TIMER_TAG},
    // {DW_TIMER3_BASE,           DW_TIMER3_IRQn,           3,    DEV_DW_TIMER_TAG},
    // {DW_GPIO_BASE,             DW_GPIO_IRQn,             0,    DEV_DW_GPIO_TAG},
    // {WJ_VAD_BASE,              WJ_VAD_TRIG_IRQn,         0,    DEV_WJ_VAD_TAG},
    // {WJ_I2S0_BASE,             WJ_I2S0_IRQn,             0,    DEV_WJ_I2S_TAG},
    // {WJ_I2S1_BASE,             WJ_I2S1_IRQn,             1,    DEV_WJ_I2S_TAG},
    // {WJ_I2S2_BASE,             WJ_I2S2_IRQn,             2,    DEV_WJ_I2S_TAG},
    // {WJ_I2S3_BASE,             WJ_I2S_IN_IRQn,           3,    DEV_WJ_I2S_TAG},
    // {WJ_TDM_BASE,              WJ_TDM_IRQn,              0,    DEV_WJ_TDM_TAG},
    // {WJ_SPDIF0_BASE,           WJ_SPDIF0_IRQn,           0,    DEV_WJ_SPDIF_TAG},
    // {WJ_SPDIF1_BASE,           WJ_SPDIF1_IRQn,           1,    DEV_WJ_SPDIF_TAG},
    // {DW_IIC0_BASE,             DW_IIC0_IRQn,             0,    DEV_DW_IIC_TAG},
    // {DW_IIC1_BASE,             DW_IIC1_IRQn,             1,    DEV_DW_IIC_TAG},
    // {DW_WDT_BASE,              DW_WDT_IRQn,              0,    DEV_DW_WDT_TAG},
    // {DW_BUS_MONITOR_BASE,      WJ_BUS_MONITOR_IRQn,      0,    DEV_WJ_PMU_TAG},
    // {WJ_MBOX_BASE,             WJ_MAILBOX_IRQn,          0,    DEV_WJ_MBOX_TAG},
    {0, 0, 0, 0}
};

#if 0
const uint8_t g_dma_chnum[] = {16};

/* DMA handshake number */
/* The member of uart_tx_hs_num is the handshake number for ETB */
const uint16_t uart_tx_hs_num[] = {19};
const uint16_t uart_rx_hs_num[] = {18};
const uint16_t iic_tx_hs_num[]  = {21, 23};
const uint16_t iic_rx_hs_num[]  = {20, 22};
const uint16_t i2s_tx_hs_num[]  = {9, 11, 13, 36, 37, 38, 39};
const uint16_t i2s_rx_hs_num[]  = {8, 10, 12, 14, 15, 16, 17};
const uint16_t spdif_tx_hs_num[]  = {25, 27};
const uint16_t spdif_rx_hs_num[]  = {24, 26};
const uint16_t tdm_rx_hs_num[]  = {28, 29, 30, 31, 32, 33, 34, 35};
const uint16_t vad_rx_hs_num[]  = {0, 1, 2, 3, 4, 5, 6, 7};

const csi_dma_ch_desc_t uart0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t iic1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s2_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t i2s3_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t spdif0_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t spdif1_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t tdm_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};

const csi_dma_ch_desc_t vad_dma_ch_list[] = {
    {0, 0}, {0, 1}, {0, 2},  {0, 3},  {0, 4},  {0, 5},  {0, 6},  {0, 7},
    {0, 8}, {0, 9}, {0, 10},  {0, 11},  {0, 12},  {0, 13},  {0, 14},  {0, 15},
    {0xff, 0xff}
};


const csi_dma_ch_spt_list_t dma_spt_list[] = {
    {DEV_DW_UART_TAG,   0, uart0_dma_ch_list},
    {DEV_DW_IIC_TAG,    0, iic0_dma_ch_list},
    {DEV_DW_IIC_TAG,    1, iic1_dma_ch_list},
    {DEV_WJ_I2S_TAG,    0, i2s0_dma_ch_list},
    {DEV_WJ_I2S_TAG,    1, i2s1_dma_ch_list},
    {DEV_WJ_I2S_TAG,    2, i2s2_dma_ch_list},
    {DEV_WJ_I2S_TAG,    3, i2s3_dma_ch_list},
    {DEV_WJ_SPDIF_TAG,  0, spdif0_dma_ch_list},
    {DEV_WJ_SPDIF_TAG,  1, spdif1_dma_ch_list},
    {DEV_WJ_TDM_TAG,    0, tdm_dma_ch_list},
    {DEV_WJ_VAD_TAG,    0, vad_dma_ch_list},
    {0xFFFFU,         0xFFU,         NULL},
};

const csi_pinmap_t gpio_pinmap[] = {
    {PA0,           0,     0,           PIN_FUNC_GPIO },
    {PA1,           0,     1,           PIN_FUNC_GPIO },
    {PA2,           0,     2,           PIN_FUNC_GPIO },
    {PA3,           0,     3,           PIN_FUNC_GPIO },
    {PA4,           0,     4,           PIN_FUNC_GPIO },
    {PA5,           0,     5,           PIN_FUNC_GPIO },
    {PA6,           0,     6,           PIN_FUNC_GPIO },
    {PA7,           0,     7,           PIN_FUNC_GPIO },
    {PA8,           0,     8,           PIN_FUNC_GPIO },
    {PA9,           0,     9,           PIN_FUNC_GPIO },
    {PA10,          0,    10,           PIN_FUNC_GPIO },
    {PA11,          0,    11,           PIN_FUNC_GPIO },
    {PA12,          0,    12,           PIN_FUNC_GPIO },
    {PA13,          0,    13,           PIN_FUNC_GPIO },
    {PA14,          0,    14,           PIN_FUNC_GPIO },
    {PA15,          0,    15,           PIN_FUNC_GPIO },
    {PA16,          0,    16,           PIN_FUNC_GPIO },
    {PA17,          0,    17,           PIN_FUNC_GPIO },
    {PA18,          0,    18,           PIN_FUNC_GPIO },
    {PA19,          0,    19,           PIN_FUNC_GPIO },
    {PA20,          0,    20,           PIN_FUNC_GPIO },
    {PA21,          0,    21,           PIN_FUNC_GPIO },
    {PA22,          0,    22,           PIN_FUNC_GPIO },
    {PA23,          0,    23,           PIN_FUNC_GPIO },
    {PA24,          0,    24,           PIN_FUNC_GPIO },
    {PA25,          0,    25,           PIN_FUNC_GPIO },
    {PA26,          0,    26,           PIN_FUNC_GPIO },
    {PA27,          0,    27,           PIN_FUNC_GPIO },
    {PA28,          0,    28,           PIN_FUNC_GPIO },
    {PA29,          0,    29,           PIN_FUNC_GPIO },
    {PA30,          0,    30,           PIN_FUNC_GPIO },
    {0xFFFFFFFFU,   0xFFU, 0xFFU,       0xFFFFFFFFU   },
};

const csi_pinmap_t uart_pinmap[] = {
    {PA4,             0,     PIN_UART_RX,    PA4_UART_RX  },
    {PA5,             0,     PIN_UART_TX,    PA5_UART_TX  },
    {PA6,             0,     PIN_UART_RX,    PA6_UART_RX  },
    {PA7,             0,     PIN_UART_TX,    PA7_UART_TX  },
    {PA12,            0,     PIN_UART_RX,    PA12_UART_TX },
    {PA13,            0,     PIN_UART_TX,    PA13_UART_RX },
    {PA23,            0,     PIN_UART_RX,    PA23_UART_RX },
    {PA24,            0,     PIN_UART_TX,    PA24_UART_TX },
    {0xFFFFFFFFU, 0xFFU, 0xFFU,      0xFFFFFFFFU  },
};

const csi_pinmap_t iic_pinmap[] = {
    {PA6,            0,     PIN_IIC_SCL,    PA6_IIC0_SCL   },
    {PA7,            0,     PIN_IIC_SDA,    PA7_IIC0_SDA   },
    {PA7,            1,     PIN_IIC_SDA,    PA7_IIC1_SDA   },
    {PA6,            1,     PIN_IIC_SCL,    PA6_IIC1_SCL   },
    {PA12,           1,     PIN_IIC_SCL,    PA12_IIC1_SCL  },
    {PA13,           1,     PIN_IIC_SDA,    PA13_IIC1_SDA  },
    {PA16,           1,     PIN_IIC_SCL,    PA16_IIC1_SCL  },
    {PA17,           1,     PIN_IIC_SDA,    PA17_IIC1_SDA  },
    {PA20,           1,     PIN_IIC_SCL,    PA20_IIC1_SCL  },
    {PA21,           1,     PIN_IIC_SDA,    PA21_IIC1_SDA  },
    {PA21,           1,     PIN_IIC_SCL,    PA21_IIC1_SCL  },
    {PA20,           1,     PIN_IIC_SDA,    PA20_IIC1_SDA  },
    {PA23,           1,     PIN_IIC_SCL,    PA23_IIC1_SCL  },
    {PA24,           1,     PIN_IIC_SDA,    PA24_IIC1_SDA  },
    {PA29,           1,     PIN_IIC_SCL,    PA29_IIC0_SCL  },
    {PA30,           1,     PIN_IIC_SDA,    PA30_IIC0_SDA  },
    {0xFFFFFFFFU, 0xFFU,       0xFFU,   0xFFFFFFFFU },
};

const csi_pinmap_t i2s_pinmap[] = {
    {PA9,            0,     PIN_I2S_WSCLK,  PA9_I2S0_WSCLK  },
    {PA9,            1,     PIN_I2S_WSCLK,  PA9_I2S1_WSCLK  },
    {PA10,           0,     PIN_I2S_SCLK,   PA10_I2S0_SCLK  },
    {PA10,           1,     PIN_I2S_SCLK,   PA10_I2S1_SCLK  },
    {PA11,           0,     PIN_I2S_SDA,    PA11_I2S0_SDA   },
    {PA11,           1,     PIN_I2S_SDI,    PA11_I2S1_DIN   },
    {PA12,           0,     PIN_I2S_MCLK,   PA12_I2S0_MCLK  },
    {PA12,           1,     PIN_I2S_SDO,    PA12_I2S1_DOUT  },
    {PA13,           1,     PIN_I2S_MCLK,   PA13_I2S1_MCLK  },
    {PA14,           1,     PIN_I2S_WSCLK,  PA14_I2S1_WSCLK },
    {PA14,           0,     PIN_I2S_WSCLK,  PA14_I2S0_WSCLK },
    {PA15,           1,     PIN_I2S_SCLK,   PA15_I2S1_SCLK  },
    {PA16,           1,     PIN_I2S_SDI,    PA16_I2S1_SDI   },
    {PA17,           1,     PIN_I2S_SDO,    PA17_I2S1_SDO   },
    {PA18,           2,     PIN_I2S_WSCLK,  PA18_I2S2_WSCLK },
    {PA19,           2,     PIN_I2S_SCLK,   PA19_I2S2_SCLK  },
    {PA20,           2,     PIN_I2S_MCLK,   PA20_I2S2_MCLK  },
    {PA21,           2,     PIN_I2S_SDI,    PA21_I2S2_SDI   },
    {PA22,           2,     PIN_I2S_SDO,    PA22_I2S2_SDO   },
    {PA27,           0,     PIN_I2S_WSCLK,  PA27_I2S0_WSCLK },
    {PA28,           0,     PIN_I2S_SCLK,   PA28_I2S0_SCLK  },
    {PA29,           0,     PIN_I2S_SDA,    PA29_I2S0_SDA   },
    {PA30,           0,     PIN_I2S_MCLK,   PA30_I2S0_MCLK  },
    {0xFFFFFFFFU, 0xFFU, 0xFFU, 0xFFFFFFFFU  },
};
#endif