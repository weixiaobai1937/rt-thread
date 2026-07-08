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
 */

#ifndef __UART_CONFIG_H__
#define __UART_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DMA 未使用时的哨兵值 */
#define UART_DMA_NONE      ((DMA_Channel_TypeDef *)0)
#define UART_DMA_CH_NONE   ((uint32_t)0xFF)
#define UART_DMA_REQ_NONE  ((uint32_t)0xFF)

/* DMA 接收循环缓冲区大小 */
#define UART_DMA_RX_BUF_SIZE  256

struct acm32_uart_config
{
    const char          *name;
    rt_uint8_t          uart_type;       /* 0=USART, 1=LPUART */
    void                *Instance;       /* USART_TypeDef* or LPUART_TypeDef* */
    IRQn_Type           irq_type;

    /* TX DMA（不使用填 UART_DMA_NONE / UART_DMA_CH_NONE） */
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

/*
 * 每个 UART 的配置通过宏展开，设计原则：
 * - 非 DMA 模式（默认）：DMA 字段填 UART_DMA_NONE / UART_DMA_CH_NONE
 * - DMA 模式：定义 BSP_USING_UARTx_DMA 后自动替换
 *
 * 引脚在 MspInit 中根据 Instance 匹配处理，不再在配置表中硬编码引脚号。
 */

/* ---- USART1 (console, PA9/PA10, AF1) ---- */
#ifdef BSP_USING_UART1
#ifdef BSP_USING_UART1_DMA
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      .tx_dma_instance = DMA1_Channel1, .tx_dma_channel = 1, .tx_dma_irq = DMA1_CH1_IRQn, .tx_dma_reqid = DMA1_REQ_USART1_TX, \
      .rx_dma_instance = DMA1_Channel2, .rx_dma_channel = 2, .rx_dma_irq = DMA1_CH2_IRQn, .rx_dma_reqid = DMA1_REQ_USART1_RX }
#else
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif
#endif

/* ---- USART2 (PD5/PD6, AF1) ---- */
#ifdef BSP_USING_UART2
#ifndef BSP_USING_UART2_DMA
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#else
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      .tx_dma_instance = DMA1_Channel3, .tx_dma_channel = 3, .tx_dma_irq = DMA1_CH3_IRQn, .tx_dma_reqid = DMA1_REQ_USART2_TX, \
      .rx_dma_instance = DMA1_Channel4, .rx_dma_channel = 4, .rx_dma_irq = DMA1_CH4_IRQn, .rx_dma_reqid = DMA1_REQ_USART2_RX }
#endif
#endif

/* ---- USART3 (PB10/PB11, AF1) ---- */
#ifdef BSP_USING_UART3
#ifndef BSP_USING_UART3_DMA
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#else
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      .tx_dma_instance = DMA1_Channel5, .tx_dma_channel = 5, .tx_dma_irq = DMA1_CH5_IRQn, .tx_dma_reqid = DMA1_REQ_USART3_TX, \
      .rx_dma_instance = DMA1_Channel6, .rx_dma_channel = 6, .rx_dma_irq = DMA1_CH6_IRQn, .rx_dma_reqid = DMA1_REQ_USART3_RX }
#endif
#endif

/* ---- USART4 (PC10/PC11, AF1) ---- */
#ifdef BSP_USING_UART4
#ifndef BSP_USING_UART4_DMA
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#else
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      .tx_dma_instance = DMA1_Channel7, .tx_dma_channel = 7, .tx_dma_irq = DMA1_CH7_IRQn, .tx_dma_reqid = DMA1_REQ_USART4_TX, \
      .rx_dma_instance = DMA2_Channel1, .rx_dma_channel = 1, .rx_dma_irq = DMA2_CH1_IRQn, .rx_dma_reqid = DMA2_REQ_USART4_RX }
#endif
#endif

/* ---- USART5-8 (placeholder, adjust per board) ---- */
#ifdef BSP_USING_UART5
#define UART5_CONFIG \
    { .name = "uart5", .uart_type = 0, .Instance = USART5, .irq_type = USART5_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

#ifdef BSP_USING_UART6
#define UART6_CONFIG \
    { .name = "uart6", .uart_type = 0, .Instance = USART6, .irq_type = USART6_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

#ifdef BSP_USING_UART7
#define UART7_CONFIG \
    { .name = "uart7", .uart_type = 0, .Instance = USART7, .irq_type = USART7_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

#ifdef BSP_USING_UART8
#define UART8_CONFIG \
    { .name = "uart8", .uart_type = 0, .Instance = USART8, .irq_type = USART8_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

/* ---- LPUART1 (PA2/PA3, AF3) ---- */
#ifdef BSP_USING_LPUART1
#define LPUART1_CONFIG \
    { .name = "lpuart1", .uart_type = 1, .Instance = LPUART1, .irq_type = LPUART1_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

/* ---- LPUART2 (PB10/PB11, AF3) ---- */
#ifdef BSP_USING_LPUART2
#define LPUART2_CONFIG \
    { .name = "lpuart2", .uart_type = 1, .Instance = LPUART2, .irq_type = LPUART2_IRQn, \
      .tx_dma_instance = UART_DMA_NONE, .tx_dma_channel = UART_DMA_CH_NONE, .tx_dma_irq = (IRQn_Type)0, .tx_dma_reqid = UART_DMA_REQ_NONE, \
      .rx_dma_instance = UART_DMA_NONE, .rx_dma_channel = UART_DMA_CH_NONE, .rx_dma_irq = (IRQn_Type)0, .rx_dma_reqid = UART_DMA_REQ_NONE }
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
