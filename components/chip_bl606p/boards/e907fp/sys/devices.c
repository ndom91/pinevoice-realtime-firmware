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
    // {DW_WDT_BASE,              DW_WDT_IRQn,              0,    DEV_DW_WDT_TAG},
    // {DW_BUS_MONITOR_BASE,      WJ_BUS_MONITOR_IRQn,      0,    DEV_WJ_PMU_TAG},
    // {WJ_MBOX_BASE,             WJ_MAILBOX_IRQn,          0,    DEV_WJ_MBOX_TAG},
    {0, 0, 0, 0}
};


const csi_pinmap_t adc_pinmap[] = {
    {GPIO_PIN_17, 0, 0, GPIO17_ADC_CH0},    {GPIO_PIN_5, 0, 1, GPIO5_ADC_CH1},
    {GPIO_PIN_4, 0, 2, GPIO4_ADC_CH2},      {GPIO_PIN_11, 0, 3, GPIO11_ADC_CH3},
    {GPIO_PIN_40, 0, 5, GPIO40_ADC_CH5},    {GPIO_PIN_12, 0, 6, GPIO12_ADC_CH6},
    {GPIO_PIN_16, 0, 8, GPIO16_ADC_CH8},    {GPIO_PIN_18, 0, 9, GPIO18_ADC_CH9},
    {GPIO_PIN_19, 0, 10, GPIO19_ADC_CH10},  {GPIO_PIN_34, 0, 11, GPIO34_ADC_CH11},
    {0xFFU, 0xFFU, 0xFFU, 0xFFU}
};

const csi_pinmap_t pwm_pinmap[] = {
    {GPIO_PIN_0, 0, 0, GPIO0_PWM0_CH0P},    {GPIO_PIN_0, 1, 0, GPIO0_PWM1_CH0P},
    {GPIO_PIN_1, 0, 1, GPIO1_PWM0_CH1P},    {GPIO_PIN_1, 1, 1, GPIO1_PWM1_CH1P},
    {GPIO_PIN_2, 0, 2, GPIO2_PWM0_CH2P},    {GPIO_PIN_2, 1, 2, GPIO2_PWM1_CH2P},
    {GPIO_PIN_3, 0, 3, GPIO3_PWM0_CH3P},    {GPIO_PIN_3, 1, 3, GPIO3_PWM1_CH3P},
    {GPIO_PIN_4, 0, 0, GPIO4_PWM0_CH0P},    {GPIO_PIN_4, 1, 0, GPIO4_PWM1_CH0P},
    {GPIO_PIN_5, 0, 1, GPIO5_PWM0_CH1P},    {GPIO_PIN_5, 1, 1, GPIO5_PWM1_CH1P},
    {GPIO_PIN_11, 0, 3, GPIO11_PWM0_CH3P},    {GPIO_PIN_11, 1, 3, GPIO11_PWM1_CH3P},
    {GPIO_PIN_12, 0, 0, GPIO12_PWM0_CH0P},    {GPIO_PIN_12, 1, 0, GPIO12_PWM1_CH0P},
    {GPIO_PIN_16, 0, 0, GPIO16_PWM0_CH0P},    {GPIO_PIN_16, 1, 0, GPIO16_PWM1_CH0P},
    {GPIO_PIN_17, 0, 1, GPIO17_PWM0_CH1P},    {GPIO_PIN_17, 1, 1, GPIO17_PWM1_CH1P},
    {GPIO_PIN_18, 0, 2, GPIO18_PWM0_CH2P},    {GPIO_PIN_18, 1, 2, GPIO18_PWM1_CH2P},
    {GPIO_PIN_19, 0, 3, GPIO19_PWM0_CH3P},    {GPIO_PIN_19, 1, 3, GPIO19_PWM1_CH3P},
    {0xFFU, 0xFFU, 0xFFU, 0xFFU}
};

const csi_pinmap_t iic_pinmap[] = {
    {0xFFU, 0xFFU, 0xFFU, 0xFFU },
};

const csi_pinmap_t uart_pinmap[] = {
    {0xFFU, 0xFFU, 0xFFU, 0xFFU  },
};

const csi_pinmap_t spi_pinmap[] = {
    {0xFFU, 0xFFU, 0xFFU, 0xFFU  },
};
