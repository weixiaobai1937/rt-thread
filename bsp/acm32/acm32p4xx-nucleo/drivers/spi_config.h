/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-14     AisinoChip   ACM32P4xx-Nucleo SPI1 config
 */

#ifndef __SPI_CONFIG_H__
#define __SPI_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_SPI1
#ifndef SPI1_BUS_CONFIG
#define SPI1_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi1",                                             \
        .Instance = SPI1,                                               \
        .irq_type = SPI1_IRQn,                                          \
        .sck_port = GPIOE,  .sck_pin = GPIO_PIN_12, .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOE, .mosi_pin = GPIO_PIN_11, .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOE, .miso_pin = GPIO_PIN_10, .miso_af = GPIO_FUNCTION_3, \
    }
#endif

#ifdef BSP_USING_SPI1_DMA
#ifndef SPI1_DMA_TX_CONFIG
#define SPI1_DMA_TX_CONFIG \
    { .Instance = DMA2_Channel1, .Channel = 1, \
      .irq = DMA2_CH1_IRQn, .reqid = DMA2_REQ_SPI1_TX }
#endif
#ifndef SPI1_DMA_RX_CONFIG
#define SPI1_DMA_RX_CONFIG \
    { .Instance = DMA2_Channel3, .Channel = 3, \
      .irq = DMA2_CH3_IRQn, .reqid = DMA2_REQ_SPI1_RX }
#endif
#endif /* BSP_USING_SPI1_DMA */
#endif /* BSP_USING_SPI1 */

#ifdef BSP_USING_SPI2
#ifndef SPI2_BUS_CONFIG
#define SPI2_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi2",                                             \
        .Instance = SPI2,                                               \
        .irq_type = SPI2_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_13, .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_15, .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_14, .miso_af = GPIO_FUNCTION_3, \
    }
#endif

#ifdef BSP_USING_SPI2_DMA
#ifndef SPI2_DMA_TX_CONFIG
#define SPI2_DMA_TX_CONFIG \
    { .Instance = DMA1_Channel0, .Channel = 0, \
      .irq = DMA1_CH0_IRQn, .reqid = DMA1_REQ_SPI2_TX }
#endif
#ifndef SPI2_DMA_RX_CONFIG
#define SPI2_DMA_RX_CONFIG \
    { .Instance = DMA1_Channel1, .Channel = 1, \
      .irq = DMA1_CH1_IRQn, .reqid = DMA1_REQ_SPI2_RX }
#endif
#endif /* BSP_USING_SPI2_DMA */
#endif /* BSP_USING_SPI2 */

#define SPI_DMA_MIN_SIZE    32

/* 默认软 CS：PE13 在 pin 表中的 index（Task 2 扩展后：PortE base=64, pin13 → 77） */
#define SPI1_DEFAULT_CS_PIN_INDEX   77

/* PB12: PortB base=16, pin12 → 28 */
#define SPI2_DEFAULT_CS_PIN_INDEX   28

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CONFIG_H__ */
