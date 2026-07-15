/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-14     AisinoChip   ACM32P4xx-Nucleo SPI1 config
 * 2026-07-15     AisinoChip   SPI DMA unit/channel from Kconfig
 * 2026-07-15     AisinoChip   SPI3/SPI4 multi-instance
 */

#ifndef __SPI_CONFIG_H__
#define __SPI_CONFIG_H__

#include "board.h"
#include "acm32p4xx_hal_conf.h"

#ifdef __cplusplus
extern "C" {
#endif

/* DMA unit + channel -> Instance / IRQn (completion still uses SPI batch IRQ) */
#define ACM32_DMA_CH_INST_(u, c)    DMA##u##_Channel##c
#define ACM32_DMA_CH_INST(u, c)     ACM32_DMA_CH_INST_(u, c)
#define ACM32_DMA_CH_IRQ_(u, c)     DMA##u##_CH##c##_IRQn
#define ACM32_DMA_CH_IRQ(u, c)      ACM32_DMA_CH_IRQ_(u, c)

#define ACM32_SPI_DMA_CFG(unit, ch, req)                    \
    {                                                       \
        .Instance = ACM32_DMA_CH_INST(unit, ch),            \
        .Channel  = (ch),                                   \
        .irq      = ACM32_DMA_CH_IRQ(unit, ch),             \
        .reqid    = (req),                                  \
    }

/* ReqID is the same numeric value on DMA1 and DMA2 for SPI1/2 */
#define ACM32_SPI_DMA_REQ(unit, name)   DMA##unit##_REQ_##name

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
#ifndef BSP_SPI1_TX_DMA_UNIT
#define BSP_SPI1_TX_DMA_UNIT    2
#endif
#ifndef BSP_SPI1_TX_DMA_CH
#define BSP_SPI1_TX_DMA_CH      1
#endif
#ifndef BSP_SPI1_RX_DMA_UNIT
#define BSP_SPI1_RX_DMA_UNIT    2
#endif
#ifndef BSP_SPI1_RX_DMA_CH
#define BSP_SPI1_RX_DMA_CH      3
#endif

#if (BSP_SPI1_TX_DMA_UNIT == 1)
#define SPI1_DMA_TX_REQ     DMA1_REQ_SPI1_TX
#else
#define SPI1_DMA_TX_REQ     DMA2_REQ_SPI1_TX
#endif
#if (BSP_SPI1_RX_DMA_UNIT == 1)
#define SPI1_DMA_RX_REQ     DMA1_REQ_SPI1_RX
#else
#define SPI1_DMA_RX_REQ     DMA2_REQ_SPI1_RX
#endif

#ifndef SPI1_DMA_TX_CONFIG
#define SPI1_DMA_TX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, SPI1_DMA_TX_REQ)
#endif
#ifndef SPI1_DMA_RX_CONFIG
#define SPI1_DMA_RX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH, SPI1_DMA_RX_REQ)
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
#ifndef BSP_SPI2_TX_DMA_UNIT
#define BSP_SPI2_TX_DMA_UNIT    1
#endif
#ifndef BSP_SPI2_TX_DMA_CH
#define BSP_SPI2_TX_DMA_CH      0
#endif
#ifndef BSP_SPI2_RX_DMA_UNIT
#define BSP_SPI2_RX_DMA_UNIT    1
#endif
#ifndef BSP_SPI2_RX_DMA_CH
#define BSP_SPI2_RX_DMA_CH      1
#endif

#if (BSP_SPI2_TX_DMA_UNIT == 1)
#define SPI2_DMA_TX_REQ     DMA1_REQ_SPI2_TX
#else
#define SPI2_DMA_TX_REQ     DMA2_REQ_SPI2_TX
#endif
#if (BSP_SPI2_RX_DMA_UNIT == 1)
#define SPI2_DMA_RX_REQ     DMA1_REQ_SPI2_RX
#else
#define SPI2_DMA_RX_REQ     DMA2_REQ_SPI2_RX
#endif

#ifndef SPI2_DMA_TX_CONFIG
#define SPI2_DMA_TX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, SPI2_DMA_TX_REQ)
#endif
#ifndef SPI2_DMA_RX_CONFIG
#define SPI2_DMA_RX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH, SPI2_DMA_RX_REQ)
#endif
#endif /* BSP_USING_SPI2_DMA */
#endif /* BSP_USING_SPI2 */

#ifdef BSP_USING_SPI3
#ifndef SPI3_BUS_CONFIG
#define SPI3_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi3",                                             \
        .Instance = SPI3,                                               \
        .irq_type = SPI3_IRQn,                                          \
        .sck_port = GPIOC,  .sck_pin = GPIO_PIN_10, .sck_af = GPIO_FUNCTION_2, \
        .mosi_port = GPIOC, .mosi_pin = GPIO_PIN_12, .mosi_af = GPIO_FUNCTION_2, \
        .miso_port = GPIOC, .miso_pin = GPIO_PIN_11, .miso_af = GPIO_FUNCTION_2, \
    }
#endif

#ifdef BSP_USING_SPI3_DMA
#ifndef BSP_SPI3_TX_DMA_UNIT
#define BSP_SPI3_TX_DMA_UNIT    2
#endif
#ifndef BSP_SPI3_TX_DMA_CH
#define BSP_SPI3_TX_DMA_CH      0
#endif
#ifndef BSP_SPI3_RX_DMA_UNIT
#define BSP_SPI3_RX_DMA_UNIT    2
#endif
#ifndef BSP_SPI3_RX_DMA_CH
#define BSP_SPI3_RX_DMA_CH      2
#endif

#if (BSP_SPI3_TX_DMA_UNIT == 1)
#define SPI3_DMA_TX_REQ     DMA1_REQ_SPI3_TX
#else
#define SPI3_DMA_TX_REQ     DMA2_REQ_SPI3_TX
#endif
#if (BSP_SPI3_RX_DMA_UNIT == 1)
#define SPI3_DMA_RX_REQ     DMA1_REQ_SPI3_RX
#else
#define SPI3_DMA_RX_REQ     DMA2_REQ_SPI3_RX
#endif

#ifndef SPI3_DMA_TX_CONFIG
#define SPI3_DMA_TX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, SPI3_DMA_TX_REQ)
#endif
#ifndef SPI3_DMA_RX_CONFIG
#define SPI3_DMA_RX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH, SPI3_DMA_RX_REQ)
#endif
#endif /* BSP_USING_SPI3_DMA */
#endif /* BSP_USING_SPI3 */

#ifdef BSP_USING_SPI4
#ifndef SPI4_BUS_CONFIG
#define SPI4_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi4",                                             \
        .Instance = SPI4,                                               \
        .irq_type = SPI4_IRQn,                                          \
        .sck_port = GPIOE,  .sck_pin = GPIO_PIN_2,  .sck_af = GPIO_FUNCTION_4, \
        .mosi_port = GPIOE, .mosi_pin = GPIO_PIN_6,  .mosi_af = GPIO_FUNCTION_4, \
        .miso_port = GPIOE, .miso_pin = GPIO_PIN_5,  .miso_af = GPIO_FUNCTION_4, \
    }
#endif

#ifdef BSP_USING_SPI4_DMA
#ifndef BSP_SPI4_TX_DMA_UNIT
#define BSP_SPI4_TX_DMA_UNIT    1
#endif
#ifndef BSP_SPI4_TX_DMA_CH
#define BSP_SPI4_TX_DMA_CH      2
#endif
#ifndef BSP_SPI4_RX_DMA_UNIT
#define BSP_SPI4_RX_DMA_UNIT    1
#endif
#ifndef BSP_SPI4_RX_DMA_CH
#define BSP_SPI4_RX_DMA_CH      3
#endif

#if (BSP_SPI4_TX_DMA_UNIT == 1)
#define SPI4_DMA_TX_REQ     DMA1_REQ_SPI4_TX
#else
#define SPI4_DMA_TX_REQ     DMA2_REQ_SPI4_TX
#endif
#if (BSP_SPI4_RX_DMA_UNIT == 1)
#define SPI4_DMA_RX_REQ     DMA1_REQ_SPI4_RX
#else
#define SPI4_DMA_RX_REQ     DMA2_REQ_SPI4_RX
#endif

#ifndef SPI4_DMA_TX_CONFIG
#define SPI4_DMA_TX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, SPI4_DMA_TX_REQ)
#endif
#ifndef SPI4_DMA_RX_CONFIG
#define SPI4_DMA_RX_CONFIG \
    ACM32_SPI_DMA_CFG(BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH, SPI4_DMA_RX_REQ)
#endif
#endif /* BSP_USING_SPI4_DMA */
#endif /* BSP_USING_SPI4 */

#define SPI_DMA_MIN_SIZE    32

/* Soft CS pin index: PortA=0,B=16,C=32,D=48,E=64 */
#define SPI1_DEFAULT_CS_PIN_INDEX   77  /* PE13 */
#define SPI2_DEFAULT_CS_PIN_INDEX   28  /* PB12 */
#define SPI3_DEFAULT_CS_PIN_INDEX   41  /* PC9  */
#define SPI4_DEFAULT_CS_PIN_INDEX   68  /* PE4  */

/* Cross-check UART/SPI DMA unit+channel uniqueness */
#include "dma_channel_check.h"

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CONFIG_H__ */
