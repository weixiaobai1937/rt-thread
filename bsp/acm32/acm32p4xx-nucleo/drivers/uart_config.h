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
 * 2026-07-23     AisinoChip   pin groups + DMA map presets (like SPI)
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

    /* TX/RX pins (Kconfig pin group) */
    GPIO_TypeDef        *tx_port;
    rt_uint32_t          tx_pin;
    rt_uint32_t          tx_af;
    GPIO_TypeDef        *rx_port;
    rt_uint32_t          rx_pin;
    rt_uint32_t          rx_af;

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

#define UART_PIN_FIELDS(tp, tpin, taf, rp, rpin, raf) \
    .tx_port = (tp), .tx_pin = (tpin), .tx_af = (taf), \
    .rx_port = (rp), .rx_pin = (rpin), .rx_af = (raf)

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

/* ---- USART1 ---- */
#ifdef BSP_USING_UART1

/* --- UART1 TX --- */
#if defined(BSP_UART1_TX_PA9)
  #define UART1_TX_PORT     GPIOA
  #define UART1_TX_PIN      GPIO_PIN_9
  #define UART1_TX_AF       GPIO_FUNCTION_1
  #define UART1_TX_HINT     "PA9"
#elif defined(BSP_UART1_TX_PB6)
  #define UART1_TX_PORT     GPIOB
  #define UART1_TX_PIN      GPIO_PIN_6
  #define UART1_TX_AF       GPIO_FUNCTION_1
  #define UART1_TX_HINT     "PB6"
#elif defined(BSP_UART1_TX_PA14)
  #define UART1_TX_PORT     GPIOA
  #define UART1_TX_PIN      GPIO_PIN_14
  #define UART1_TX_AF       GPIO_FUNCTION_1
  #define UART1_TX_HINT     "PA14"
#else
  #define UART1_TX_PORT     GPIOA
  #define UART1_TX_PIN      GPIO_PIN_9
  #define UART1_TX_AF       GPIO_FUNCTION_1
  #define UART1_TX_HINT     "PA9"
#endif

/* --- UART1 RX --- */
#if defined(BSP_UART1_RX_PA10)
  #define UART1_RX_PORT     GPIOA
  #define UART1_RX_PIN      GPIO_PIN_10
  #define UART1_RX_AF       GPIO_FUNCTION_1
  #define UART1_RX_HINT     "PA10"
#elif defined(BSP_UART1_RX_PB7)
  #define UART1_RX_PORT     GPIOB
  #define UART1_RX_PIN      GPIO_PIN_7
  #define UART1_RX_AF       GPIO_FUNCTION_1
  #define UART1_RX_HINT     "PB7"
#elif defined(BSP_UART1_RX_PA13)
  #define UART1_RX_PORT     GPIOA
  #define UART1_RX_PIN      GPIO_PIN_13
  #define UART1_RX_AF       GPIO_FUNCTION_1
  #define UART1_RX_HINT     "PA13"
#else
  #define UART1_RX_PORT     GPIOA
  #define UART1_RX_PIN      GPIO_PIN_10
  #define UART1_RX_AF       GPIO_FUNCTION_1
  #define UART1_RX_HINT     "PA10"
#endif

#define UART1_PINS_HINT UART1_TX_HINT "/TX " UART1_RX_HINT "/RX"
#define UART1_PIN_CFG   UART_PIN_FIELDS(UART1_TX_PORT, UART1_TX_PIN, UART1_TX_AF, \
                                        UART1_RX_PORT, UART1_RX_PIN, UART1_RX_AF)

#ifdef BSP_USING_UART1_DMA
#if defined(BSP_UART1_DMA_MAP_DMA2)
#undef BSP_UART1_TX_DMA_UNIT
#undef BSP_UART1_TX_DMA_CH
#undef BSP_UART1_RX_DMA_UNIT
#undef BSP_UART1_RX_DMA_CH
#define BSP_UART1_TX_DMA_UNIT   2
#define BSP_UART1_TX_DMA_CH     1
#define BSP_UART1_RX_DMA_UNIT   2
#define BSP_UART1_RX_DMA_CH     2
#elif defined(BSP_UART1_DMA_MAP_CUSTOM)
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
#else /* DEFAULT or legacy */
#undef BSP_UART1_TX_DMA_UNIT
#undef BSP_UART1_TX_DMA_CH
#undef BSP_UART1_RX_DMA_UNIT
#undef BSP_UART1_RX_DMA_CH
#define BSP_UART1_TX_DMA_UNIT   1
#define BSP_UART1_TX_DMA_CH     1
#define BSP_UART1_RX_DMA_UNIT   1
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
      UART1_PIN_CFG, \
      UART_DMA_FIELDS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, UART1_DMA_TX_REQ, \
                      BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, UART1_DMA_RX_REQ) }
#else
#define UART1_CONFIG \
    { .name = "uart1", .uart_type = 0, .Instance = USART1, .irq_type = USART1_IRQn, \
      UART1_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif
#endif /* BSP_USING_UART1 */

/* ---- USART2 ---- */
#ifdef BSP_USING_UART2

/* --- UART2 TX --- */
#if defined(BSP_UART2_TX_PD5)
  #define UART2_TX_PORT     GPIOD
  #define UART2_TX_PIN      GPIO_PIN_5
  #define UART2_TX_AF       GPIO_FUNCTION_3
  #define UART2_TX_HINT     "PD5"
#elif defined(BSP_UART2_TX_PA2)
  #define UART2_TX_PORT     GPIOA
  #define UART2_TX_PIN      GPIO_PIN_2
  #define UART2_TX_AF       GPIO_FUNCTION_1
  #define UART2_TX_HINT     "PA2"
#elif defined(BSP_UART2_TX_PC2)
  #define UART2_TX_PORT     GPIOC
  #define UART2_TX_PIN      GPIO_PIN_2
  #define UART2_TX_AF       GPIO_FUNCTION_2
  #define UART2_TX_HINT     "PC2"
#else
  #define UART2_TX_PORT     GPIOD
  #define UART2_TX_PIN      GPIO_PIN_5
  #define UART2_TX_AF       GPIO_FUNCTION_3
  #define UART2_TX_HINT     "PD5"
#endif

/* --- UART2 RX --- */
#if defined(BSP_UART2_RX_PD6)
  #define UART2_RX_PORT     GPIOD
  #define UART2_RX_PIN      GPIO_PIN_6
  #define UART2_RX_AF       GPIO_FUNCTION_3
  #define UART2_RX_HINT     "PD6"
#elif defined(BSP_UART2_RX_PA3)
  #define UART2_RX_PORT     GPIOA
  #define UART2_RX_PIN      GPIO_PIN_3
  #define UART2_RX_AF       GPIO_FUNCTION_1
  #define UART2_RX_HINT     "PA3"
#elif defined(BSP_UART2_RX_PC1)
  #define UART2_RX_PORT     GPIOC
  #define UART2_RX_PIN      GPIO_PIN_1
  #define UART2_RX_AF       GPIO_FUNCTION_2
  #define UART2_RX_HINT     "PC1"
#else
  #define UART2_RX_PORT     GPIOD
  #define UART2_RX_PIN      GPIO_PIN_6
  #define UART2_RX_AF       GPIO_FUNCTION_3
  #define UART2_RX_HINT     "PD6"
#endif

#define UART2_PINS_HINT UART2_TX_HINT "/TX " UART2_RX_HINT "/RX"
#define UART2_PIN_CFG   UART_PIN_FIELDS(UART2_TX_PORT, UART2_TX_PIN, UART2_TX_AF, \
                                        UART2_RX_PORT, UART2_RX_PIN, UART2_RX_AF)

#ifdef BSP_USING_UART2_DMA
#if defined(BSP_UART2_DMA_MAP_DMA1)
#undef BSP_UART2_TX_DMA_UNIT
#undef BSP_UART2_TX_DMA_CH
#undef BSP_UART2_RX_DMA_UNIT
#undef BSP_UART2_RX_DMA_CH
#define BSP_UART2_TX_DMA_UNIT   1
#define BSP_UART2_TX_DMA_CH     0
#define BSP_UART2_RX_DMA_UNIT   1
#define BSP_UART2_RX_DMA_CH     1
#elif defined(BSP_UART2_DMA_MAP_CUSTOM)
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
#else /* DEFAULT or legacy */
#undef BSP_UART2_TX_DMA_UNIT
#undef BSP_UART2_TX_DMA_CH
#undef BSP_UART2_RX_DMA_UNIT
#undef BSP_UART2_RX_DMA_CH
#define BSP_UART2_TX_DMA_UNIT   1
#define BSP_UART2_TX_DMA_CH     3
#define BSP_UART2_RX_DMA_UNIT   2
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
      UART2_PIN_CFG, \
      UART_DMA_FIELDS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, UART2_DMA_TX_REQ, \
                      BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, UART2_DMA_RX_REQ) }
#else
#define UART2_CONFIG \
    { .name = "uart2", .uart_type = 0, .Instance = USART2, .irq_type = USART2_IRQn, \
      UART2_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif
#endif /* BSP_USING_UART2 */

/* ---- USART3 ---- */
#ifdef BSP_USING_UART3

/* --- UART3 TX --- */
#if defined(BSP_UART3_TX_PB10)
  #define UART3_TX_PORT     GPIOB
  #define UART3_TX_PIN      GPIO_PIN_10
  #define UART3_TX_AF       GPIO_FUNCTION_1
  #define UART3_TX_HINT     "PB10"
#elif defined(BSP_UART3_TX_PD8)
  #define UART3_TX_PORT     GPIOD
  #define UART3_TX_PIN      GPIO_PIN_8
  #define UART3_TX_AF       GPIO_FUNCTION_3
  #define UART3_TX_HINT     "PD8"
#elif defined(BSP_UART3_TX_PC10)
  #define UART3_TX_PORT     GPIOC
  #define UART3_TX_PIN      GPIO_PIN_10
  #define UART3_TX_AF       GPIO_FUNCTION_4
  #define UART3_TX_HINT     "PC10"
#else
  #define UART3_TX_PORT     GPIOB
  #define UART3_TX_PIN      GPIO_PIN_10
  #define UART3_TX_AF       GPIO_FUNCTION_1
  #define UART3_TX_HINT     "PB10"
#endif

/* --- UART3 RX --- */
#if defined(BSP_UART3_RX_PB11)
  #define UART3_RX_PORT     GPIOB
  #define UART3_RX_PIN      GPIO_PIN_11
  #define UART3_RX_AF       GPIO_FUNCTION_1
  #define UART3_RX_HINT     "PB11"
#elif defined(BSP_UART3_RX_PD9)
  #define UART3_RX_PORT     GPIOD
  #define UART3_RX_PIN      GPIO_PIN_9
  #define UART3_RX_AF       GPIO_FUNCTION_3
  #define UART3_RX_HINT     "PD9"
#elif defined(BSP_UART3_RX_PC11)
  #define UART3_RX_PORT     GPIOC
  #define UART3_RX_PIN      GPIO_PIN_11
  #define UART3_RX_AF       GPIO_FUNCTION_4
  #define UART3_RX_HINT     "PC11"
#else
  #define UART3_RX_PORT     GPIOB
  #define UART3_RX_PIN      GPIO_PIN_11
  #define UART3_RX_AF       GPIO_FUNCTION_1
  #define UART3_RX_HINT     "PB11"
#endif

#define UART3_PINS_HINT UART3_TX_HINT "/TX " UART3_RX_HINT "/RX"
#define UART3_PIN_CFG   UART_PIN_FIELDS(UART3_TX_PORT, UART3_TX_PIN, UART3_TX_AF, \
                                        UART3_RX_PORT, UART3_RX_PIN, UART3_RX_AF)

#ifdef BSP_USING_UART3_DMA
#if defined(BSP_UART3_DMA_MAP_DMA1)
#undef BSP_UART3_TX_DMA_UNIT
#undef BSP_UART3_TX_DMA_CH
#undef BSP_UART3_RX_DMA_UNIT
#undef BSP_UART3_RX_DMA_CH
#define BSP_UART3_TX_DMA_UNIT   1
#define BSP_UART3_TX_DMA_CH     2
#define BSP_UART3_RX_DMA_UNIT   1
#define BSP_UART3_RX_DMA_CH     3
#elif defined(BSP_UART3_DMA_MAP_CUSTOM)
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
#else
#undef BSP_UART3_TX_DMA_UNIT
#undef BSP_UART3_TX_DMA_CH
#undef BSP_UART3_RX_DMA_UNIT
#undef BSP_UART3_RX_DMA_CH
#define BSP_UART3_TX_DMA_UNIT   2
#define BSP_UART3_TX_DMA_CH     2
#define BSP_UART3_RX_DMA_UNIT   2
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
      UART3_PIN_CFG, \
      UART_DMA_FIELDS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, UART3_DMA_TX_REQ, \
                      BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, UART3_DMA_RX_REQ) }
#else
#define UART3_CONFIG \
    { .name = "uart3", .uart_type = 0, .Instance = USART3, .irq_type = USART3_IRQn, \
      UART3_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif
#endif /* BSP_USING_UART3 */

/* ---- USART4 ---- */
#ifdef BSP_USING_UART4

/* --- UART4 TX --- */
#if defined(BSP_UART4_TX_PC10)
  #define UART4_TX_PORT     GPIOC
  #define UART4_TX_PIN      GPIO_PIN_10
  #define UART4_TX_AF       GPIO_FUNCTION_4
  #define UART4_TX_HINT     "PC10"
#elif defined(BSP_UART4_TX_PD1)
  #define UART4_TX_PORT     GPIOD
  #define UART4_TX_PIN      GPIO_PIN_1
  #define UART4_TX_AF       GPIO_FUNCTION_3
  #define UART4_TX_HINT     "PD1"
#elif defined(BSP_UART4_TX_PA0)
  #define UART4_TX_PORT     GPIOA
  #define UART4_TX_PIN      GPIO_PIN_0
  #define UART4_TX_AF       GPIO_FUNCTION_3
  #define UART4_TX_HINT     "PA0"
#elif defined(BSP_UART4_TX_PB9)
  #define UART4_TX_PORT     GPIOB
  #define UART4_TX_PIN      GPIO_PIN_9
  #define UART4_TX_AF       GPIO_FUNCTION_1
  #define UART4_TX_HINT     "PB9"
#elif defined(BSP_UART4_TX_PE9)
  #define UART4_TX_PORT     GPIOE
  #define UART4_TX_PIN      GPIO_PIN_9
  #define UART4_TX_AF       GPIO_FUNCTION_6
  #define UART4_TX_HINT     "PE9"
#else
  #define UART4_TX_PORT     GPIOC
  #define UART4_TX_PIN      GPIO_PIN_10
  #define UART4_TX_AF       GPIO_FUNCTION_4
  #define UART4_TX_HINT     "PC10"
#endif

/* --- UART4 RX --- */
#if defined(BSP_UART4_RX_PC11)
  #define UART4_RX_PORT     GPIOC
  #define UART4_RX_PIN      GPIO_PIN_11
  #define UART4_RX_AF       GPIO_FUNCTION_4
  #define UART4_RX_HINT     "PC11"
#elif defined(BSP_UART4_RX_PD0)
  #define UART4_RX_PORT     GPIOD
  #define UART4_RX_PIN      GPIO_PIN_0
  #define UART4_RX_AF       GPIO_FUNCTION_3
  #define UART4_RX_HINT     "PD0"
#elif defined(BSP_UART4_RX_PA1)
  #define UART4_RX_PORT     GPIOA
  #define UART4_RX_PIN      GPIO_PIN_1
  #define UART4_RX_AF       GPIO_FUNCTION_3
  #define UART4_RX_HINT     "PA1"
#elif defined(BSP_UART4_RX_PB8)
  #define UART4_RX_PORT     GPIOB
  #define UART4_RX_PIN      GPIO_PIN_8
  #define UART4_RX_AF       GPIO_FUNCTION_1
  #define UART4_RX_HINT     "PB8"
#elif defined(BSP_UART4_RX_PE10)
  #define UART4_RX_PORT     GPIOE
  #define UART4_RX_PIN      GPIO_PIN_10
  #define UART4_RX_AF       GPIO_FUNCTION_6
  #define UART4_RX_HINT     "PE10"
#else
  #define UART4_RX_PORT     GPIOC
  #define UART4_RX_PIN      GPIO_PIN_11
  #define UART4_RX_AF       GPIO_FUNCTION_4
  #define UART4_RX_HINT     "PC11"
#endif

#define UART4_PINS_HINT UART4_TX_HINT "/TX " UART4_RX_HINT "/RX"
#define UART4_PIN_CFG   UART_PIN_FIELDS(UART4_TX_PORT, UART4_TX_PIN, UART4_TX_AF, \
                                        UART4_RX_PORT, UART4_RX_PIN, UART4_RX_AF)

#ifdef BSP_USING_UART4_DMA
#if defined(BSP_UART4_DMA_MAP_DMA1)
#undef BSP_UART4_TX_DMA_UNIT
#undef BSP_UART4_TX_DMA_CH
#undef BSP_UART4_RX_DMA_UNIT
#undef BSP_UART4_RX_DMA_CH
#define BSP_UART4_TX_DMA_UNIT   1
#define BSP_UART4_TX_DMA_CH     2
#define BSP_UART4_RX_DMA_UNIT   1
#define BSP_UART4_RX_DMA_CH     3
#elif defined(BSP_UART4_DMA_MAP_CUSTOM)
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
#else
#undef BSP_UART4_TX_DMA_UNIT
#undef BSP_UART4_TX_DMA_CH
#undef BSP_UART4_RX_DMA_UNIT
#undef BSP_UART4_RX_DMA_CH
#define BSP_UART4_TX_DMA_UNIT   2
#define BSP_UART4_TX_DMA_CH     1
#define BSP_UART4_RX_DMA_UNIT   1
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
      UART4_PIN_CFG, \
      UART_DMA_FIELDS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, UART4_DMA_TX_REQ, \
                      BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, UART4_DMA_RX_REQ) }
#else
#define UART4_CONFIG \
    { .name = "uart4", .uart_type = 0, .Instance = USART4, .irq_type = USART4_IRQn, \
      UART4_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif
#endif /* BSP_USING_UART4 */

/* ---- USART5-8 (placeholder pins; extend Kconfig when needed) ---- */
#ifdef BSP_USING_UART5
#define UART5_CONFIG \
    { .name = "uart5", .uart_type = 0, .Instance = USART5, .irq_type = USART5_IRQn, \
      UART_PIN_FIELDS(GPIOA, GPIO_PIN_0, GPIO_FUNCTION_1, GPIOA, GPIO_PIN_1, GPIO_FUNCTION_1), \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART6
#define UART6_CONFIG \
    { .name = "uart6", .uart_type = 0, .Instance = USART6, .irq_type = USART6_IRQn, \
      UART_PIN_FIELDS(GPIOB, GPIO_PIN_0, GPIO_FUNCTION_1, GPIOB, GPIO_PIN_1, GPIO_FUNCTION_1), \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART7
#define UART7_CONFIG \
    { .name = "uart7", .uart_type = 0, .Instance = USART7, .irq_type = USART7_IRQn, \
      UART_PIN_FIELDS(GPIOC, GPIO_PIN_0, GPIO_FUNCTION_1, GPIOC, GPIO_PIN_1, GPIO_FUNCTION_1), \
      UART_DMA_NONE_FIELDS }
#endif

#ifdef BSP_USING_UART8
#define UART8_CONFIG \
    { .name = "uart8", .uart_type = 0, .Instance = USART8, .irq_type = USART8_IRQn, \
      UART_PIN_FIELDS(GPIOD, GPIO_PIN_0, GPIO_FUNCTION_1, GPIOD, GPIO_PIN_1, GPIO_FUNCTION_1), \
      UART_DMA_NONE_FIELDS }
#endif

/* ---- LPUART1 ---- */
#ifdef BSP_USING_LPUART1

/* --- LPUART1 TX --- */
#if defined(BSP_LPUART1_TX_PA2)
  #define LPUART1_TX_PORT   GPIOA
  #define LPUART1_TX_PIN    GPIO_PIN_2
  #define LPUART1_TX_AF     GPIO_FUNCTION_3
  #define LPUART1_TX_HINT   "PA2"
#elif defined(BSP_LPUART1_TX_PA9)
  #define LPUART1_TX_PORT   GPIOA
  #define LPUART1_TX_PIN    GPIO_PIN_9
  #define LPUART1_TX_AF     GPIO_FUNCTION_0
  #define LPUART1_TX_HINT   "PA9"
#elif defined(BSP_LPUART1_TX_PB6)
  #define LPUART1_TX_PORT   GPIOB
  #define LPUART1_TX_PIN    GPIO_PIN_6
  #define LPUART1_TX_AF     GPIO_FUNCTION_0
  #define LPUART1_TX_HINT   "PB6"
#else
  #define LPUART1_TX_PORT   GPIOA
  #define LPUART1_TX_PIN    GPIO_PIN_2
  #define LPUART1_TX_AF     GPIO_FUNCTION_3
  #define LPUART1_TX_HINT   "PA2"
#endif

/* --- LPUART1 RX --- */
#if defined(BSP_LPUART1_RX_PA3)
  #define LPUART1_RX_PORT   GPIOA
  #define LPUART1_RX_PIN    GPIO_PIN_3
  #define LPUART1_RX_AF     GPIO_FUNCTION_3
  #define LPUART1_RX_HINT   "PA3"
#elif defined(BSP_LPUART1_RX_PA10)
  #define LPUART1_RX_PORT   GPIOA
  #define LPUART1_RX_PIN    GPIO_PIN_10
  #define LPUART1_RX_AF     GPIO_FUNCTION_0
  #define LPUART1_RX_HINT   "PA10"
#elif defined(BSP_LPUART1_RX_PB7)
  #define LPUART1_RX_PORT   GPIOB
  #define LPUART1_RX_PIN    GPIO_PIN_7
  #define LPUART1_RX_AF     GPIO_FUNCTION_0
  #define LPUART1_RX_HINT   "PB7"
#else
  #define LPUART1_RX_PORT   GPIOA
  #define LPUART1_RX_PIN    GPIO_PIN_3
  #define LPUART1_RX_AF     GPIO_FUNCTION_3
  #define LPUART1_RX_HINT   "PA3"
#endif

#define LPUART1_PINS_HINT LPUART1_TX_HINT "/TX " LPUART1_RX_HINT "/RX"
#define LPUART1_PIN_CFG   UART_PIN_FIELDS(LPUART1_TX_PORT, LPUART1_TX_PIN, LPUART1_TX_AF, \
                                          LPUART1_RX_PORT, LPUART1_RX_PIN, LPUART1_RX_AF)

#define LPUART1_CONFIG \
    { .name = "lpuart1", .uart_type = 1, .Instance = LPUART1, .irq_type = LPUART1_IRQn, \
      LPUART1_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif

/* ---- LPUART2 ---- */
#ifdef BSP_USING_LPUART2

/* --- LPUART2 TX --- */
#if defined(BSP_LPUART2_TX_PB10)
  #define LPUART2_TX_PORT   GPIOB
  #define LPUART2_TX_PIN    GPIO_PIN_10
  #define LPUART2_TX_AF     GPIO_FUNCTION_3
  #define LPUART2_TX_HINT   "PB10"
#elif defined(BSP_LPUART2_TX_PC0)
  #define LPUART2_TX_PORT   GPIOC
  #define LPUART2_TX_PIN    GPIO_PIN_0
  #define LPUART2_TX_AF     GPIO_FUNCTION_0
  #define LPUART2_TX_HINT   "PC0"
#elif defined(BSP_LPUART2_TX_PE3)
  #define LPUART2_TX_PORT   GPIOE
  #define LPUART2_TX_PIN    GPIO_PIN_3
  #define LPUART2_TX_AF     GPIO_FUNCTION_0
  #define LPUART2_TX_HINT   "PE3"
#else
  #define LPUART2_TX_PORT   GPIOB
  #define LPUART2_TX_PIN    GPIO_PIN_10
  #define LPUART2_TX_AF     GPIO_FUNCTION_3
  #define LPUART2_TX_HINT   "PB10"
#endif

/* --- LPUART2 RX --- */
#if defined(BSP_LPUART2_RX_PB11)
  #define LPUART2_RX_PORT   GPIOB
  #define LPUART2_RX_PIN    GPIO_PIN_11
  #define LPUART2_RX_AF     GPIO_FUNCTION_3
  #define LPUART2_RX_HINT   "PB11"
#elif defined(BSP_LPUART2_RX_PC1)
  #define LPUART2_RX_PORT   GPIOC
  #define LPUART2_RX_PIN    GPIO_PIN_1
  #define LPUART2_RX_AF     GPIO_FUNCTION_0
  #define LPUART2_RX_HINT   "PC1"
#elif defined(BSP_LPUART2_RX_PE4)
  #define LPUART2_RX_PORT   GPIOE
  #define LPUART2_RX_PIN    GPIO_PIN_4
  #define LPUART2_RX_AF     GPIO_FUNCTION_0
  #define LPUART2_RX_HINT   "PE4"
#else
  #define LPUART2_RX_PORT   GPIOB
  #define LPUART2_RX_PIN    GPIO_PIN_11
  #define LPUART2_RX_AF     GPIO_FUNCTION_3
  #define LPUART2_RX_HINT   "PB11"
#endif

#define LPUART2_PINS_HINT LPUART2_TX_HINT "/TX " LPUART2_RX_HINT "/RX"
#define LPUART2_PIN_CFG   UART_PIN_FIELDS(LPUART2_TX_PORT, LPUART2_TX_PIN, LPUART2_TX_AF, \
                                          LPUART2_RX_PORT, LPUART2_RX_PIN, LPUART2_RX_AF)

#define LPUART2_CONFIG \
    { .name = "lpuart2", .uart_type = 1, .Instance = LPUART2, .irq_type = LPUART2_IRQn, \
      LPUART2_PIN_CFG, UART_DMA_NONE_FIELDS }
#endif

#ifdef __cplusplus
}
#endif

#endif /* __UART_CONFIG_H__ */
