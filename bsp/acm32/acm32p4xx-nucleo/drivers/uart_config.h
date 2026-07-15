/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-04     AisinoChip   ACM32P4xx UART config
 *                              USART1-8 + LPUART1/2
 * 2026-07-06     AisinoChip   add DMA fields, refactor for V2 driver
 * 2026-07-15     AisinoChip   UART DMA unit/channel from Kconfig
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DMA unused sentinel */
#define UART_DMA_NONE      ((DMA_Channel_TypeDef *)0)
#define UART_DMA_CH_NONE   ((uint32_t)0xFF)
#define UART_DMA_REQ_NONE  ((uint32_t)0xFF)

struct acm32_uart_config
{
    const char          *name;
    rt_uint8_t          uart_type;       /* 0=USART, 1=LPUART */
    void                *Instance;       /* USART_TypeDef* or LPUART_TypeDef* */
    IRQn_Type           irq_type;

    /* TX DMA (unused: UART_DMA_NONE / UART_DMA_CH_NONE) */
    DMA_Channel_TypeDef *tx_dma_instance;
    uint32_t             tx_dma_channel;
    IRQn_Type            tx_dma_irq;
    uint32_t             tx_dma_reqid;

    /* RX DMA */
    DMA_Channel_TypeDef *rx_dma_instance;
    uint32_t             rx_dma_channel;
    IRQn_Type            rx_dma_irq;
    uint32_t             rx_dma_reqid;
};

/* DMA unit + channel -> Instance / IRQn */
#define ACM32_DMA_CH_INST_(u, c)    DMA##u##_Channel##c
#define ACM32_DMA_CH_INST(u, c)     ACM32_DMA_CH_INST_(u, c)
#define ACM32_DMA_CH_IRQ_(u, c)     DMA##u##_CH##c##_IRQn
#define ACM32_DMA_CH_IRQ(u, c)      ACM32_DMA_CH_IRQ_(u, c)

#define UART_DMA_NONE_FIELDS \
    .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, \
    .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
    .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, \
    .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE

#define UART_DMA_FIELDS(tx_u, tx_c, tx_req, rx_u, rx_c, rx_req) \
    .tx_dma_instance = ACM32_DMA_CH_INST(tx_u, tx_c), \
    .tx_dma_channel  = (tx_c), \
    .tx_dma_irq      = ACM32_DMA_CH_IRQ(tx_u, tx_c), \
    .tx_dma_reqid    = (tx_req), \
    .rx_dma_instance = ACM32_DMA_CH_INST(rx_u, rx_c), \
    .rx_dma_channel  = (rx_c), \
    .rx_dma_irq      = ACM32_DMA_CH_IRQ(rx_u, rx_c), \
    .rx_dma_reqid    = (rx_req)

/* ---- USART1 (console, PA9/PA10, AF1) ---- */
#ifdef BSP_USING_UART1
#ifdef BSP_USING_UART1_DMA
#ifndef BSP_UART1_TX_DMA_UNIT
#define BSP_UART1_TX_DMA_UNIT   1
#endif
#ifndef BSP_UART1_TX_DMA_CH
#define BSP_UART1_TX_DMA_CH     1
#endif
#ifndef BSP_UART1_RX_DMA_UNIT
#define BSP_UART1_RX_DMA_UNIT   1
#endif
#ifndef BSP_UART1_RX_DMA_CH
#define BSP_UART1_RX_DMA_CH     2
#endif
#if (BSP_UART1_TX_DMA_UNIT == 1)
#define UART1_DMA_TX_REQ        DMA1_REQ_USART1_TX
#else
#define UART1_DMA_TX_REQ        DMA2_REQ_USART1_TX
#endif
#if (BSP_UART1_RX_DMA_UNIT == 1)
#define UART1_DMA_RX_REQ        DMA1_REQ_USART1_RX
#else
#define UART1_DMA_RX_REQ        DMA2_REQ_USART1_RX
#endif
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      UART_DMA_FIELDS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, UART1_DMA_TX_REQ, \
                      BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, UART1_DMA_RX_REQ) }
#else
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif
#endif

/* ---- USART2 (PD5/PD6, AF3) ---- */
#ifdef BSP_USING_UART2
#ifdef BSP_USING_UART2_DMA
#ifndef BSP_UART2_TX_DMA_UNIT
#define BSP_UART2_TX_DMA_UNIT   1
#endif
#ifndef BSP_UART2_TX_DMA_CH
#define BSP_UART2_TX_DMA_CH     3
#endif
#ifndef BSP_UART2_RX_DMA_UNIT
#define BSP_UART2_RX_DMA_UNIT   2
#endif
#ifndef BSP_UART2_RX_DMA_CH
#define BSP_UART2_RX_DMA_CH     0
#endif
#if (BSP_UART2_TX_DMA_UNIT == 1)
#define UART2_DMA_TX_REQ        DMA1_REQ_USART2_TX
#else
#define UART2_DMA_TX_REQ        DMA2_REQ_USART2_TX
#endif
#if (BSP_UART2_RX_DMA_UNIT == 1)
#define UART2_DMA_RX_REQ        DMA1_REQ_USART2_RX
#else
#define UART2_DMA_RX_REQ        DMA2_REQ_USART2_RX
#endif
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      UART_DMA_FIELDS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, UART2_DMA_TX_REQ, \
                      BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, UART2_DMA_RX_REQ) }
#else
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif
#endif

/* ---- USART3 (PB10/PB11, AF1) ---- */
#ifdef BSP_USING_UART3
#ifdef BSP_USING_UART3_DMA
#ifndef BSP_UART3_TX_DMA_UNIT
#define BSP_UART3_TX_DMA_UNIT   2
#endif
#ifndef BSP_UART3_TX_DMA_CH
#define BSP_UART3_TX_DMA_CH     2
#endif
#ifndef BSP_UART3_RX_DMA_UNIT
#define BSP_UART3_RX_DMA_UNIT   2
#endif
#ifndef BSP_UART3_RX_DMA_CH
#define BSP_UART3_RX_DMA_CH     3
#endif
#if (BSP_UART3_TX_DMA_UNIT == 1)
#define UART3_DMA_TX_REQ        DMA1_REQ_USART3_TX
#else
#define UART3_DMA_TX_REQ        DMA2_REQ_USART3_TX
#endif
#if (BSP_UART3_RX_DMA_UNIT == 1)
#define UART3_DMA_RX_REQ        DMA1_REQ_USART3_RX
#else
#define UART3_DMA_RX_REQ        DMA2_REQ_USART3_RX
#endif
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      UART_DMA_FIELDS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, UART3_DMA_TX_REQ, \
                      BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, UART3_DMA_RX_REQ) }
#else
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif
#endif

/* ---- USART4 (PC10/PC11, AF4) ---- */
#ifdef BSP_USING_UART4
#ifdef BSP_USING_UART4_DMA
#ifndef BSP_UART4_TX_DMA_UNIT
#define BSP_UART4_TX_DMA_UNIT   2
#endif
#ifndef BSP_UART4_TX_DMA_CH
#define BSP_UART4_TX_DMA_CH     1
#endif
#ifndef BSP_UART4_RX_DMA_UNIT
#define BSP_UART4_RX_DMA_UNIT   1
#endif
#ifndef BSP_UART4_RX_DMA_CH
#define BSP_UART4_RX_DMA_CH     0
#endif
#if (BSP_UART4_TX_DMA_UNIT == 1)
#define UART4_DMA_TX_REQ        DMA1_REQ_USART4_TX
#else
#define UART4_DMA_TX_REQ        DMA2_REQ_USART4_TX
#endif
#if (BSP_UART4_RX_DMA_UNIT == 1)
#define UART4_DMA_RX_REQ        DMA1_REQ_USART4_RX
#else
#define UART4_DMA_RX_REQ        DMA2_REQ_USART4_RX
#endif
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      UART_DMA_FIELDS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, UART4_DMA_TX_REQ, \
                      BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, UART4_DMA_RX_REQ) }
#else
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif
#endif

/* ---- USART5-8 (placeholder, adjust per board) ---- */
#ifdef BSP_USING_UART5
#define UART5_CONFIG \
    { .name = "uart5", .uart_type = 0, .Instance = USART5, .irq_type = USART5_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART6
#define UART6_CONFIG \
    { .name = "uart6", .uart_type = 0, .Instance = USART6, .irq_type = USART6_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART7
#define UART7_CONFIG \
    { .name = "uart7", .uart_type = 0, .Instance = USART7, .irq_type = USART7_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART8
#define UART8_CONFIG \
    { .name = "uart8", .uart_type = 0, .Instance = USART8, .irq_type = USART8_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

/* ---- LPUART1 (PA2/PA3, AF3) ---- */
#ifdef BSP_USING_LPUART1
#define LPUART1_CONFIG \
    { .name = "lpuart1", .uart_type = 1, .Instance = LPUART1, .irq_type = LPUART1_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

/* ---- LPUART2 (PB10/PB11, AF3) ---- */
#ifdef BSP_USING_LPUART2
#define LPUART2_CONFIG \
    { .name = "lpuart2", .uart_type = 1, .Instance = LPUART2, .irq_type = LPUART2_IRQn, \
      UART_DMA_NONE_FIELDS }
#endif

/* Cross-check UART/SPI DMA unit+channel uniqueness */
#include "dma_channel_check.h"

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
