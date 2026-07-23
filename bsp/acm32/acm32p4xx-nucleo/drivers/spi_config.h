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
/* Default pin group if rtconfig.h has no choice yet */
#if !defined(BSP_SPI1_PINS_PE) && !defined(BSP_SPI1_PINS_PA) && !defined(BSP_SPI1_PINS_PB)
#define BSP_SPI1_PINS_PE
#endif

#ifndef SPI1_BUS_CONFIG
#if defined(BSP_SPI1_PINS_PA)
/* PA5 SCK AF0, PA7 MOSI AF0, PA6 MISO AF0 */
#define SPI1_PINS_HINT  "PA5/SCK PA7/MOSI PA6/MISO"
#define SPI1_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi1",                                             \
        .Instance = SPI1,                                               \
        .irq_type = SPI1_IRQn,                                          \
        .sck_port = GPIOA,  .sck_pin = GPIO_PIN_5,  .sck_af = GPIO_FUNCTION_0, \
        .mosi_port = GPIOA, .mosi_pin = GPIO_PIN_7,  .mosi_af = GPIO_FUNCTION_0, \
        .miso_port = GPIOA, .miso_pin = GPIO_PIN_6,  .miso_af = GPIO_FUNCTION_0, \
    }
#elif defined(BSP_SPI1_PINS_PB)
/* PB3 SCK AF4, PB5 MOSI AF0, PB4 MISO AF4 */
#define SPI1_PINS_HINT  "PB3/SCK PB5/MOSI PB4/MISO"
#define SPI1_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi1",                                             \
        .Instance = SPI1,                                               \
        .irq_type = SPI1_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_3,  .sck_af = GPIO_FUNCTION_4, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_5,  .mosi_af = GPIO_FUNCTION_0, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_4,  .miso_af = GPIO_FUNCTION_4, \
    }
#else /* BSP_SPI1_PINS_PE */
/* PE12 SCK AF3, PE11 MOSI AF3, PE10 MISO AF3 */
#define SPI1_PINS_HINT  "PE12/SCK PE11/MOSI PE10/MISO"
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
#endif /* SPI1_BUS_CONFIG */

#ifdef BSP_SPI1_CS_PIN
#define SPI1_DEFAULT_CS_PIN_INDEX   BSP_SPI1_CS_PIN
#else
#if defined(BSP_SPI1_PINS_PA)
#define SPI1_DEFAULT_CS_PIN_INDEX   4   /* PA4 */
#elif defined(BSP_SPI1_PINS_PB)
#define SPI1_DEFAULT_CS_PIN_INDEX   15  /* PA15 */
#else
#define SPI1_DEFAULT_CS_PIN_INDEX   77  /* PE13 */
#endif
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
#if !defined(BSP_SPI2_PINS_PB13) && !defined(BSP_SPI2_PINS_PC) && !defined(BSP_SPI2_PINS_PB10)
#define BSP_SPI2_PINS_PB13
#endif

#ifndef SPI2_BUS_CONFIG
#if defined(BSP_SPI2_PINS_PC)
/* PC7 SCK AF6, PC9 MOSI AF6, PC8 MISO AF6 */
#define SPI2_PINS_HINT  "PC7/SCK PC9/MOSI PC8/MISO"
#define SPI2_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi2",                                             \
        .Instance = SPI2,                                               \
        .irq_type = SPI2_IRQn,                                          \
        .sck_port = GPIOC,  .sck_pin = GPIO_PIN_7,  .sck_af = GPIO_FUNCTION_6, \
        .mosi_port = GPIOC, .mosi_pin = GPIO_PIN_9,  .mosi_af = GPIO_FUNCTION_6, \
        .miso_port = GPIOC, .miso_pin = GPIO_PIN_8,  .miso_af = GPIO_FUNCTION_6, \
    }
#elif defined(BSP_SPI2_PINS_PB10)
/* PB10 SCK AF3, PB15 MOSI AF3, PB14 MISO AF3 */
#define SPI2_PINS_HINT  "PB10/SCK PB15/MOSI PB14/MISO"
#define SPI2_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi2",                                             \
        .Instance = SPI2,                                               \
        .irq_type = SPI2_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_10, .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_15, .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_14, .miso_af = GPIO_FUNCTION_3, \
    }
#else /* BSP_SPI2_PINS_PB13 */
/* PB13 SCK AF3, PB15 MOSI AF3, PB14 MISO AF3 */
#define SPI2_PINS_HINT  "PB13/SCK PB15/MOSI PB14/MISO"
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
#endif /* SPI2_BUS_CONFIG */

#ifdef BSP_SPI2_CS_PIN
#define SPI2_DEFAULT_CS_PIN_INDEX   BSP_SPI2_CS_PIN
#else
#if defined(BSP_SPI2_PINS_PC)
#define SPI2_DEFAULT_CS_PIN_INDEX   38  /* PC6 */
#elif defined(BSP_SPI2_PINS_PB10)
#define SPI2_DEFAULT_CS_PIN_INDEX   25  /* PB9 */
#else
#define SPI2_DEFAULT_CS_PIN_INDEX   28  /* PB12 */
#endif
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
#if !defined(BSP_SPI3_PINS_PC) && !defined(BSP_SPI3_PINS_PB)
#define BSP_SPI3_PINS_PC
#endif

#ifndef SPI3_BUS_CONFIG
#if defined(BSP_SPI3_PINS_PB)
/* PB3 SCK AF3, PB5 MOSI AF3, PB4 MISO AF3 */
#define SPI3_PINS_HINT  "PB3/SCK PB5/MOSI PB4/MISO"
#define SPI3_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi3",                                             \
        .Instance = SPI3,                                               \
        .irq_type = SPI3_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_3,  .sck_af = GPIO_FUNCTION_3, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_5,  .mosi_af = GPIO_FUNCTION_3, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_4,  .miso_af = GPIO_FUNCTION_3, \
    }
#else /* BSP_SPI3_PINS_PC */
/* PC10 SCK AF2, PC12 MOSI AF2, PC11 MISO AF2 */
#define SPI3_PINS_HINT  "PC10/SCK PC12/MOSI PC11/MISO"
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
#endif /* SPI3_BUS_CONFIG */

#ifdef BSP_SPI3_CS_PIN
#define SPI3_DEFAULT_CS_PIN_INDEX   BSP_SPI3_CS_PIN
#else
#if defined(BSP_SPI3_PINS_PB)
#define SPI3_DEFAULT_CS_PIN_INDEX   15  /* PA15 */
#else
#define SPI3_DEFAULT_CS_PIN_INDEX   41  /* PC9 */
#endif
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
#if !defined(BSP_SPI4_PINS_PE) && !defined(BSP_SPI4_PINS_PB)
#define BSP_SPI4_PINS_PE
#endif

#ifndef SPI4_BUS_CONFIG
#if defined(BSP_SPI4_PINS_PB)
/* PB2 SCK AF1, PB1 MOSI AF0, PB6 MISO AF3 */
#define SPI4_PINS_HINT  "PB2/SCK PB1/MOSI PB6/MISO"
#define SPI4_BUS_CONFIG                                                 \
    {                                                                   \
        .bus_name = "spi4",                                             \
        .Instance = SPI4,                                               \
        .irq_type = SPI4_IRQn,                                          \
        .sck_port = GPIOB,  .sck_pin = GPIO_PIN_2,  .sck_af = GPIO_FUNCTION_1, \
        .mosi_port = GPIOB, .mosi_pin = GPIO_PIN_1,  .mosi_af = GPIO_FUNCTION_0, \
        .miso_port = GPIOB, .miso_pin = GPIO_PIN_6,  .miso_af = GPIO_FUNCTION_3, \
    }
#else /* BSP_SPI4_PINS_PE */
/* PE2 SCK AF4, PE6 MOSI AF4, PE5 MISO AF4 */
#define SPI4_PINS_HINT  "PE2/SCK PE6/MOSI PE5/MISO"
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
#endif /* SPI4_BUS_CONFIG */

#ifdef BSP_SPI4_CS_PIN
#define SPI4_DEFAULT_CS_PIN_INDEX   BSP_SPI4_CS_PIN
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
