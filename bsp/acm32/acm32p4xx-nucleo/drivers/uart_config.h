/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   ACM32P4xx UART config
 *                              USART1-8 + LPUART1/2
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

struct acm32_uart_config
{
    const char          *name;
    rt_uint8_t          uart_type;
    void                *Instance;
    IRQn_Type           irq_type;
    GPIO_TypeDef        *tx_port;
    GPIO_TypeDef        *rx_port;
    rt_uint32_t         tx_pin;
    rt_uint32_t         rx_pin;
    rt_uint32_t         tx_af;
    rt_uint32_t         rx_af;
};

/* ---- USART1 (console, PA9/PA10, AF1) ---- */
#ifdef BSP_USING_UART1
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      .tx_port = GPIOA, .rx_port = GPIOA, .tx_pin = GPIO_PIN_9, .rx_pin = GPIO_PIN_10, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

/* ---- USART2 (PD5/PD6, AF1) ---- */
#ifdef BSP_USING_UART2
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      .tx_port = GPIOD, .rx_port = GPIOD, .tx_pin = GPIO_PIN_5, .rx_pin = GPIO_PIN_6, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

/* ---- USART3 (PB10/PB11, AF1) ---- */
#ifdef BSP_USING_UART3
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      .tx_port = GPIOB, .rx_port = GPIOB, .tx_pin = GPIO_PIN_10, .rx_pin = GPIO_PIN_11, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

/* ---- USART4 (PC10/PC11, AF1) ---- */
#ifdef BSP_USING_UART4
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      .tx_port = GPIOC, .rx_port = GPIOC, .tx_pin = GPIO_PIN_10, .rx_pin = GPIO_PIN_11, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

/* ---- USART5-8 (placeholder pins, adjust per board) ---- */
#ifdef BSP_USING_UART5
#define UART5_CONFIG \
    { .name = "uart5", .uart_type = 0, .Instance = USART5, .irq_type = USART5_IRQn, \
      .tx_port = GPIOA, .rx_port = GPIOA, .tx_pin = GPIO_PIN_0, .rx_pin = GPIO_PIN_1, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

#ifdef BSP_USING_UART6
#define UART6_CONFIG \
    { .name = "uart6", .uart_type = 0, .Instance = USART6, .irq_type = USART6_IRQn, \
      .tx_port = GPIOB, .rx_port = GPIOB, .tx_pin = GPIO_PIN_0, .rx_pin = GPIO_PIN_1, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

#ifdef BSP_USING_UART7
#define UART7_CONFIG \
    { .name = "uart7", .uart_type = 0, .Instance = USART7, .irq_type = USART7_IRQn, \
      .tx_port = GPIOC, .rx_port = GPIOC, .tx_pin = GPIO_PIN_0, .rx_pin = GPIO_PIN_1, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

#ifdef BSP_USING_UART8
#define UART8_CONFIG \
    { .name = "uart8", .uart_type = 0, .Instance = USART8, .irq_type = USART8_IRQn, \
      .tx_port = GPIOD, .rx_port = GPIOD, .tx_pin = GPIO_PIN_0, .rx_pin = GPIO_PIN_1, \
      .tx_af = GPIO_FUNCTION_1, .rx_af = GPIO_FUNCTION_1, }
#endif

/* ---- LPUART1 (PA2/PA3, AF3) ---- */
#ifdef BSP_USING_LPUART1
#define LPUART1_CONFIG \
    { .name = "lpuart1", .uart_type = 1, .Instance = LPUART1, .irq_type = LPUART1_IRQn, \
      .tx_port = GPIOA, .rx_port = GPIOA, .tx_pin = GPIO_PIN_2, .rx_pin = GPIO_PIN_3, \
      .tx_af = GPIO_FUNCTION_3, .rx_af = GPIO_FUNCTION_3, }
#endif

/* ---- LPUART2 (PB10/PB11, AF3) ---- */
#ifdef BSP_USING_LPUART2
#define LPUART2_CONFIG \
    { .name = "lpuart2", .uart_type = 1, .Instance = LPUART2, .irq_type = LPUART2_IRQn, \
      .tx_port = GPIOB, .rx_port = GPIOB, .tx_pin = GPIO_PIN_10, .rx_pin = GPIO_PIN_11, \
      .tx_af = GPIO_FUNCTION_3, .rx_af = GPIO_FUNCTION_3, }
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
