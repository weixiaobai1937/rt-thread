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
 * 2026-07-23     AisinoChip   SPI1~4 pin groups selectable via Kconfig
 * 2026-07-23     AisinoChip   SPI1~4 DMA channel map presets + custom
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

/* Soft CS pin index: PortA=0, B=16, C=32, D=48, E=64; -1 = PIN_NONE */
#define ACM32_SPI_CS_NONE   (-1)

#ifdef BSP_USING_SPI1
/* SCK: PE12 AF3 (default), PA5 AF0, PB3 AF4 */
#if defined(BSP_SPI1_SCK_PA5)
#define SPI1_SCK_PORT   GPIOA
#define SPI1_SCK_PIN    GPIO_PIN_5
#define SPI1_SCK_AF     GPIO_FUNCTION_0
#elif defined(BSP_SPI1_SCK_PB3)
#define SPI1_SCK_PORT   GPIOB
#define SPI1_SCK_PIN    GPIO_PIN_3
#define SPI1_SCK_AF     GPIO_FUNCTION_4
#else
#define SPI1_SCK_PORT   GPIOE
#define SPI1_SCK_PIN    GPIO_PIN_12
#define SPI1_SCK_AF     GPIO_FUNCTION_3
#endif

/* MOSI: PE11 AF3 (default), PA7 AF0, PB5 AF0 */
#if defined(BSP_SPI1_MOSI_PA7)
#define SPI1_MOSI_PORT  GPIOA
#define SPI1_MOSI_PIN   GPIO_PIN_7
#define SPI1_MOSI_AF    GPIO_FUNCTION_0
#elif defined(BSP_SPI1_MOSI_PB5)
#define SPI1_MOSI_PORT  GPIOB
#define SPI1_MOSI_PIN   GPIO_PIN_5
#define SPI1_MOSI_AF    GPIO_FUNCTION_0
#else
#define SPI1_MOSI_PORT  GPIOE
#define SPI1_MOSI_PIN   GPIO_PIN_11
#define SPI1_MOSI_AF    GPIO_FUNCTION_3
#endif

/* MISO: PE10 AF3 (default), PA6 AF0, PB4 AF4 */
#if defined(BSP_SPI1_MISO_PA6)
#define SPI1_MISO_PORT  GPIOA
#define SPI1_MISO_PIN   GPIO_PIN_6
#define SPI1_MISO_AF    GPIO_FUNCTION_0
#elif defined(BSP_SPI1_MISO_PB4)
#define SPI1_MISO_PORT  GPIOB
#define SPI1_MISO_PIN   GPIO_PIN_4
#define SPI1_MISO_AF    GPIO_FUNCTION_4
#else
#define SPI1_MISO_PORT  GPIOE
#define SPI1_MISO_PIN   GPIO_PIN_10
#define SPI1_MISO_AF    GPIO_FUNCTION_3
#endif

#ifndef SPI1_BUS_CONFIG
#define SPI1_PINS_HINT  "see Kconfig"
#define SPI1_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi1",                                             \
        .Instance = SPI1,                                               \
        .irq_type = SPI1_IRQn,                                          \
        .sck_port  = SPI1_SCK_PORT,  .sck_pin  = SPI1_SCK_PIN,  .sck_af  = SPI1_SCK_AF,  \
        .mosi_port = SPI1_MOSI_PORT, .mosi_pin = SPI1_MOSI_PIN, .mosi_af = SPI1_MOSI_AF, \
        .miso_port = SPI1_MISO_PORT, .miso_pin = SPI1_MISO_PIN, .miso_af = SPI1_MISO_AF, \
    }
#endif

/* CS pin: PE13 (default), PA4, PA15, PE1 */
#if defined(BSP_SPI1_CS_PA4)
#define SPI1_DEFAULT_CS_PIN_INDEX   4   /* PA4 */
#elif defined(BSP_SPI1_CS_PA15)
#define SPI1_DEFAULT_CS_PIN_INDEX   15  /* PA15 */
#elif defined(BSP_SPI1_CS_PE1)
#define SPI1_DEFAULT_CS_PIN_INDEX   65  /* PE1 */
#else
#define SPI1_DEFAULT_CS_PIN_INDEX   77  /* PE13 */
#endif

#ifdef BSP_USING_SPI1_DMA
/* Resolve TX/RX unit+ch from Kconfig map (preset) or custom ints */
#if defined(BSP_SPI1_DMA_MAP_DMA1)
#undef BSP_SPI1_TX_DMA_UNIT
#undef BSP_SPI1_TX_DMA_CH
#undef BSP_SPI1_RX_DMA_UNIT
#undef BSP_SPI1_RX_DMA_CH
#define BSP_SPI1_TX_DMA_UNIT    1
#define BSP_SPI1_TX_DMA_CH      0
#define BSP_SPI1_RX_DMA_UNIT    1
#define BSP_SPI1_RX_DMA_CH      1
#elif defined(BSP_SPI1_DMA_MAP_CUSTOM)
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
#else /* BSP_SPI1_DMA_MAP_DEFAULT or legacy */
#undef BSP_SPI1_TX_DMA_UNIT
#undef BSP_SPI1_TX_DMA_CH
#undef BSP_SPI1_RX_DMA_UNIT
#undef BSP_SPI1_RX_DMA_CH
#define BSP_SPI1_TX_DMA_UNIT    2
#define BSP_SPI1_TX_DMA_CH      1
#define BSP_SPI1_RX_DMA_UNIT    2
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
/* SCK: PB13 AF3 (default), PC7 AF6, PF8 AF3, PA9 AF3 */
#if defined(BSP_SPI2_SCK_PC7)
#define SPI2_SCK_PORT   GPIOC
#define SPI2_SCK_PIN    GPIO_PIN_7
#define SPI2_SCK_AF     GPIO_FUNCTION_6
#elif defined(BSP_SPI2_SCK_PF8)
#define SPI2_SCK_PORT   GPIOF
#define SPI2_SCK_PIN    GPIO_PIN_8
#define SPI2_SCK_AF     GPIO_FUNCTION_3
#elif defined(BSP_SPI2_SCK_PA9)
#define SPI2_SCK_PORT   GPIOA
#define SPI2_SCK_PIN    GPIO_PIN_9
#define SPI2_SCK_AF     GPIO_FUNCTION_3
#else
#define SPI2_SCK_PORT   GPIOB
#define SPI2_SCK_PIN    GPIO_PIN_13
#define SPI2_SCK_AF     GPIO_FUNCTION_3
#endif

/* MOSI: PB15 AF3 (default), PC9 AF6, PF0 AF3, PC1 AF3 */
#if defined(BSP_SPI2_MOSI_PC9)
#define SPI2_MOSI_PORT  GPIOC
#define SPI2_MOSI_PIN   GPIO_PIN_9
#define SPI2_MOSI_AF    GPIO_FUNCTION_6
#elif defined(BSP_SPI2_MOSI_PF0)
#define SPI2_MOSI_PORT  GPIOF
#define SPI2_MOSI_PIN   GPIO_PIN_0
#define SPI2_MOSI_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI2_MOSI_PC1)
#define SPI2_MOSI_PORT  GPIOC
#define SPI2_MOSI_PIN   GPIO_PIN_1
#define SPI2_MOSI_AF    GPIO_FUNCTION_3
#else
#define SPI2_MOSI_PORT  GPIOB
#define SPI2_MOSI_PIN   GPIO_PIN_15
#define SPI2_MOSI_AF    GPIO_FUNCTION_3
#endif

/* MISO: PB14 AF3 (default), PC8 AF6, PF4 AF3, PC2 AF3 */
#if defined(BSP_SPI2_MISO_PC8)
#define SPI2_MISO_PORT  GPIOC
#define SPI2_MISO_PIN   GPIO_PIN_8
#define SPI2_MISO_AF    GPIO_FUNCTION_6
#elif defined(BSP_SPI2_MISO_PF4)
#define SPI2_MISO_PORT  GPIOF
#define SPI2_MISO_PIN   GPIO_PIN_4
#define SPI2_MISO_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI2_MISO_PC2)
#define SPI2_MISO_PORT  GPIOC
#define SPI2_MISO_PIN   GPIO_PIN_2
#define SPI2_MISO_AF    GPIO_FUNCTION_3
#else
#define SPI2_MISO_PORT  GPIOB
#define SPI2_MISO_PIN   GPIO_PIN_14
#define SPI2_MISO_AF    GPIO_FUNCTION_3
#endif

#ifndef SPI2_BUS_CONFIG
#define SPI2_PINS_HINT  "see Kconfig"
#define SPI2_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi2",                                             \
        .Instance = SPI2,                                               \
        .irq_type = SPI2_IRQn,                                          \
        .sck_port  = SPI2_SCK_PORT,  .sck_pin  = SPI2_SCK_PIN,  .sck_af  = SPI2_SCK_AF,  \
        .mosi_port = SPI2_MOSI_PORT, .mosi_pin = SPI2_MOSI_PIN, .mosi_af = SPI2_MOSI_AF, \
        .miso_port = SPI2_MISO_PORT, .miso_pin = SPI2_MISO_PIN, .miso_af = SPI2_MISO_AF, \
    }
#endif

/* CS pin: PB12 (default), PC6, PF6, PB9 */
#if defined(BSP_SPI2_CS_PC6)
#define SPI2_DEFAULT_CS_PIN_INDEX   38  /* PC6 */
#elif defined(BSP_SPI2_CS_PF6)
#define SPI2_DEFAULT_CS_PIN_INDEX   102 /* PF6 */
#elif defined(BSP_SPI2_CS_PB9)
#define SPI2_DEFAULT_CS_PIN_INDEX   25  /* PB9 */
#else
#define SPI2_DEFAULT_CS_PIN_INDEX   28  /* PB12 */
#endif

#ifdef BSP_USING_SPI2_DMA
#if defined(BSP_SPI2_DMA_MAP_DMA2)
#undef BSP_SPI2_TX_DMA_UNIT
#undef BSP_SPI2_TX_DMA_CH
#undef BSP_SPI2_RX_DMA_UNIT
#undef BSP_SPI2_RX_DMA_CH
#define BSP_SPI2_TX_DMA_UNIT    2
#define BSP_SPI2_TX_DMA_CH      0
#define BSP_SPI2_RX_DMA_UNIT    2
#define BSP_SPI2_RX_DMA_CH      1
#elif defined(BSP_SPI2_DMA_MAP_CUSTOM)
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
#else /* DEFAULT or legacy */
#undef BSP_SPI2_TX_DMA_UNIT
#undef BSP_SPI2_TX_DMA_CH
#undef BSP_SPI2_RX_DMA_UNIT
#undef BSP_SPI2_RX_DMA_CH
#define BSP_SPI2_TX_DMA_UNIT    1
#define BSP_SPI2_TX_DMA_CH      0
#define BSP_SPI2_RX_DMA_UNIT    1
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
/* SCK: PC10 AF2 (default), PB3 AF3, PG0 AF3 */
#if defined(BSP_SPI3_SCK_PB3)
#define SPI3_SCK_PORT   GPIOB
#define SPI3_SCK_PIN    GPIO_PIN_3
#define SPI3_SCK_AF     GPIO_FUNCTION_3
#elif defined(BSP_SPI3_SCK_PG0)
#define SPI3_SCK_PORT   GPIOG
#define SPI3_SCK_PIN    GPIO_PIN_0
#define SPI3_SCK_AF     GPIO_FUNCTION_3
#else
#define SPI3_SCK_PORT   GPIOC
#define SPI3_SCK_PIN    GPIO_PIN_10
#define SPI3_SCK_AF     GPIO_FUNCTION_2
#endif

/* MOSI: PC12 AF2 (default), PB5 AF3, PG2 AF3, PD6 AF2 */
#if defined(BSP_SPI3_MOSI_PB5)
#define SPI3_MOSI_PORT  GPIOB
#define SPI3_MOSI_PIN   GPIO_PIN_5
#define SPI3_MOSI_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI3_MOSI_PG2)
#define SPI3_MOSI_PORT  GPIOG
#define SPI3_MOSI_PIN   GPIO_PIN_2
#define SPI3_MOSI_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI3_MOSI_PD6)
#define SPI3_MOSI_PORT  GPIOD
#define SPI3_MOSI_PIN   GPIO_PIN_6
#define SPI3_MOSI_AF    GPIO_FUNCTION_2
#else
#define SPI3_MOSI_PORT  GPIOC
#define SPI3_MOSI_PIN   GPIO_PIN_12
#define SPI3_MOSI_AF    GPIO_FUNCTION_2
#endif

/* MISO: PC11 AF2 (default), PB4 AF3, PG6 AF3 */
#if defined(BSP_SPI3_MISO_PB4)
#define SPI3_MISO_PORT  GPIOB
#define SPI3_MISO_PIN   GPIO_PIN_4
#define SPI3_MISO_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI3_MISO_PG6)
#define SPI3_MISO_PORT  GPIOG
#define SPI3_MISO_PIN   GPIO_PIN_6
#define SPI3_MISO_AF    GPIO_FUNCTION_3
#else
#define SPI3_MISO_PORT  GPIOC
#define SPI3_MISO_PIN   GPIO_PIN_11
#define SPI3_MISO_AF    GPIO_FUNCTION_2
#endif

#ifndef SPI3_BUS_CONFIG
#define SPI3_PINS_HINT  "see Kconfig"
#define SPI3_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi3",                                             \
        .Instance = SPI3,                                               \
        .irq_type = SPI3_IRQn,                                          \
        .sck_port  = SPI3_SCK_PORT,  .sck_pin  = SPI3_SCK_PIN,  .sck_af  = SPI3_SCK_AF,  \
        .mosi_port = SPI3_MOSI_PORT, .mosi_pin = SPI3_MOSI_PIN, .mosi_af = SPI3_MOSI_AF, \
        .miso_port = SPI3_MISO_PORT, .miso_pin = SPI3_MISO_PIN, .miso_af = SPI3_MISO_AF, \
    }
#endif

/* CS pin: PA4 (default), PA15, PG4, PC9 */
#if defined(BSP_SPI3_CS_PA15)
#define SPI3_DEFAULT_CS_PIN_INDEX   15  /* PA15 */
#elif defined(BSP_SPI3_CS_PG4)
#define SPI3_DEFAULT_CS_PIN_INDEX   100 /* PG4 */
#elif defined(BSP_SPI3_CS_PC9)
#define SPI3_DEFAULT_CS_PIN_INDEX   41  /* PC9 */
#else
#define SPI3_DEFAULT_CS_PIN_INDEX   4   /* PA4 */
#endif

#ifdef BSP_USING_SPI3_DMA
#if defined(BSP_SPI3_DMA_MAP_DMA1)
#undef BSP_SPI3_TX_DMA_UNIT
#undef BSP_SPI3_TX_DMA_CH
#undef BSP_SPI3_RX_DMA_UNIT
#undef BSP_SPI3_RX_DMA_CH
#define BSP_SPI3_TX_DMA_UNIT    1
#define BSP_SPI3_TX_DMA_CH      2
#define BSP_SPI3_RX_DMA_UNIT    1
#define BSP_SPI3_RX_DMA_CH      3
#elif defined(BSP_SPI3_DMA_MAP_CUSTOM)
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
#else /* DEFAULT or legacy */
#undef BSP_SPI3_TX_DMA_UNIT
#undef BSP_SPI3_TX_DMA_CH
#undef BSP_SPI3_RX_DMA_UNIT
#undef BSP_SPI3_RX_DMA_CH
#define BSP_SPI3_TX_DMA_UNIT    2
#define BSP_SPI3_TX_DMA_CH      0
#define BSP_SPI3_RX_DMA_UNIT    2
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
/* SCK: PE2 AF4 (default), PB2 AF1, PB9 AF3, PE12 AF4 */
#if defined(BSP_SPI4_SCK_PB2)
#define SPI4_SCK_PORT   GPIOB
#define SPI4_SCK_PIN    GPIO_PIN_2
#define SPI4_SCK_AF     GPIO_FUNCTION_1
#elif defined(BSP_SPI4_SCK_PB9)
#define SPI4_SCK_PORT   GPIOB
#define SPI4_SCK_PIN    GPIO_PIN_9
#define SPI4_SCK_AF     GPIO_FUNCTION_3
#elif defined(BSP_SPI4_SCK_PE12)
#define SPI4_SCK_PORT   GPIOE
#define SPI4_SCK_PIN    GPIO_PIN_12
#define SPI4_SCK_AF     GPIO_FUNCTION_4
#else
#define SPI4_SCK_PORT   GPIOE
#define SPI4_SCK_PIN    GPIO_PIN_2
#define SPI4_SCK_AF     GPIO_FUNCTION_4
#endif

/* MOSI: PE6 AF4 (default), PB1 AF0, PB15 AF6, PE14 AF4, PB14 AF5 */
#if defined(BSP_SPI4_MOSI_PB1)
#define SPI4_MOSI_PORT  GPIOB
#define SPI4_MOSI_PIN   GPIO_PIN_1
#define SPI4_MOSI_AF    GPIO_FUNCTION_0
#elif defined(BSP_SPI4_MOSI_PB15)
#define SPI4_MOSI_PORT  GPIOB
#define SPI4_MOSI_PIN   GPIO_PIN_15
#define SPI4_MOSI_AF    GPIO_FUNCTION_6
#elif defined(BSP_SPI4_MOSI_PE14)
#define SPI4_MOSI_PORT  GPIOE
#define SPI4_MOSI_PIN   GPIO_PIN_14
#define SPI4_MOSI_AF    GPIO_FUNCTION_4
#elif defined(BSP_SPI4_MOSI_PB14)
#define SPI4_MOSI_PORT  GPIOB
#define SPI4_MOSI_PIN   GPIO_PIN_14
#define SPI4_MOSI_AF    GPIO_FUNCTION_5
#else
#define SPI4_MOSI_PORT  GPIOE
#define SPI4_MOSI_PIN   GPIO_PIN_6
#define SPI4_MOSI_AF    GPIO_FUNCTION_4
#endif

/* MISO: PE5 AF4 (default), PB6 AF3, PE13 AF4 */
#if defined(BSP_SPI4_MISO_PB6)
#define SPI4_MISO_PORT  GPIOB
#define SPI4_MISO_PIN   GPIO_PIN_6
#define SPI4_MISO_AF    GPIO_FUNCTION_3
#elif defined(BSP_SPI4_MISO_PE13)
#define SPI4_MISO_PORT  GPIOE
#define SPI4_MISO_PIN   GPIO_PIN_13
#define SPI4_MISO_AF    GPIO_FUNCTION_4
#else
#define SPI4_MISO_PORT  GPIOE
#define SPI4_MISO_PIN   GPIO_PIN_5
#define SPI4_MISO_AF    GPIO_FUNCTION_4
#endif

#ifndef SPI4_BUS_CONFIG
#define SPI4_PINS_HINT  "see Kconfig"
#define SPI4_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi4",                                             \
        .Instance = SPI4,                                               \
        .irq_type = SPI4_IRQn,                                          \
        .sck_port  = SPI4_SCK_PORT,  .sck_pin  = SPI4_SCK_PIN,  .sck_af  = SPI4_SCK_AF,  \
        .mosi_port = SPI4_MOSI_PORT, .mosi_pin = SPI4_MOSI_PIN, .mosi_af = SPI4_MOSI_AF, \
        .miso_port = SPI4_MISO_PORT, .miso_pin = SPI4_MISO_PIN, .miso_af = SPI4_MISO_AF, \
    }
#endif

/* CS pin: PE4 (default), PA11, PB12 */
#if defined(BSP_SPI4_CS_PA11)
#define SPI4_DEFAULT_CS_PIN_INDEX   11  /* PA11 */
#elif defined(BSP_SPI4_CS_PB12)
#define SPI4_DEFAULT_CS_PIN_INDEX   28  /* PB12 */
#else
#define SPI4_DEFAULT_CS_PIN_INDEX   68  /* PE4 */
#endif

#ifdef BSP_USING_SPI4_DMA
#if defined(BSP_SPI4_DMA_MAP_DMA2)
#undef BSP_SPI4_TX_DMA_UNIT
#undef BSP_SPI4_TX_DMA_CH
#undef BSP_SPI4_RX_DMA_UNIT
#undef BSP_SPI4_RX_DMA_CH
#define BSP_SPI4_TX_DMA_UNIT    2
#define BSP_SPI4_TX_DMA_CH      2
#define BSP_SPI4_RX_DMA_UNIT    2
#define BSP_SPI4_RX_DMA_CH      0
#elif defined(BSP_SPI4_DMA_MAP_CUSTOM)
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
#else /* DEFAULT or legacy */
#undef BSP_SPI4_TX_DMA_UNIT
#undef BSP_SPI4_TX_DMA_CH
#undef BSP_SPI4_RX_DMA_UNIT
#undef BSP_SPI4_RX_DMA_CH
#define BSP_SPI4_TX_DMA_UNIT    1
#define BSP_SPI4_TX_DMA_CH      2
#define BSP_SPI4_RX_DMA_UNIT    1
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

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CONFIG_H__ */
