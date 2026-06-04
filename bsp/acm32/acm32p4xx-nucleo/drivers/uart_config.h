/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   first implementation for ACM32P4xx
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_UART1

#ifndef UART1_CONFIG
#define UART1_CONFIG                                                \
    {                                                               \
        .name       = "uart1",                                      \
        .Instance   = USART1,                                       \
        .irq_type   = USART1_IRQn,                                  \
        .tx_port    = UART1_TX_PORT,                                \
        .rx_port    = UART1_RX_PORT,                                \
        .tx_pin     = UART1_TX_PIN,                                 \
        .rx_pin     = UART1_RX_PIN,                                 \
    }
#endif /* UART1_CONFIG */

#endif /* BSP_USING_UART1 */

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
