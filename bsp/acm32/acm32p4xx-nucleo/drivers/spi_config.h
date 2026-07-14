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
#endif /* BSP_USING_SPI1 */

/* 默认软 CS：PE13 在 pin 表中的 index（Task 2 扩展后：PortE base=64, pin13 → 77） */
#define SPI1_DEFAULT_CS_PIN_INDEX   77

#ifdef __cplusplus
}
#endif

#endif /* __SPI_CONFIG_H__ */
