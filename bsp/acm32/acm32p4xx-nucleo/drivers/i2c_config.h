/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx I2C pin config
 * 2026-07-24     AisinoChip   per-signal pin choices (Kconfig)
 */

#ifndef __I2C_CONFIG_H__
#define __I2C_CONFIG_H__

#include "board.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BSP_USING_I2C1
#if !defined(BSP_I2C1_SCL_PB6) && !defined(BSP_I2C1_SCL_PA13) && !defined(BSP_I2C1_SCL_PF11)
#define BSP_I2C1_SCL_PB6
#endif
#if !defined(BSP_I2C1_SDA_PB7) && !defined(BSP_I2C1_SDA_PA14) && !defined(BSP_I2C1_SDA_PF12)
#define BSP_I2C1_SDA_PB7
#endif

#if defined(BSP_I2C1_SCL_PB6)
  #define I2C1_SCL_PORT    GPIOB
  #define I2C1_SCL_PIN     GPIO_PIN_6
  #define I2C1_SCL_AF      GPIO_FUNCTION_7
#elif defined(BSP_I2C1_SCL_PA13)
  #define I2C1_SCL_PORT    GPIOA
  #define I2C1_SCL_PIN     GPIO_PIN_13
  #define I2C1_SCL_AF      GPIO_FUNCTION_4
#elif defined(BSP_I2C1_SCL_PF11)
  #define I2C1_SCL_PORT    GPIOF
  #define I2C1_SCL_PIN     GPIO_PIN_11
  #define I2C1_SCL_AF      GPIO_FUNCTION_4
#else
  #define I2C1_SCL_PORT    GPIOB
  #define I2C1_SCL_PIN     GPIO_PIN_6
  #define I2C1_SCL_AF      GPIO_FUNCTION_7
#endif

#if defined(BSP_I2C1_SDA_PB7)
  #define I2C1_SDA_PORT    GPIOB
  #define I2C1_SDA_PIN     GPIO_PIN_7
  #define I2C1_SDA_AF      GPIO_FUNCTION_7
#elif defined(BSP_I2C1_SDA_PA14)
  #define I2C1_SDA_PORT    GPIOA
  #define I2C1_SDA_PIN     GPIO_PIN_14
  #define I2C1_SDA_AF      GPIO_FUNCTION_4
#elif defined(BSP_I2C1_SDA_PF12)
  #define I2C1_SDA_PORT    GPIOF
  #define I2C1_SDA_PIN     GPIO_PIN_12
  #define I2C1_SDA_AF      GPIO_FUNCTION_4
#else
  #define I2C1_SDA_PORT    GPIOB
  #define I2C1_SDA_PIN     GPIO_PIN_7
  #define I2C1_SDA_AF      GPIO_FUNCTION_7
#endif

#ifndef I2C1_BUS_CONFIG
#define I2C1_BUS_CONFIG                                                 \
    {                                                                   \
        .name = "i2c1",                                                 \
        .Instance = I2C1,                                               \
        .irq_type = I2C1_IRQn,                                          \
        .clock_speed = CLOCK_SPEED_STANDARD,                            \
        .scl_port = I2C1_SCL_PORT, .scl_pin = I2C1_SCL_PIN, .scl_af = I2C1_SCL_AF, \
        .sda_port = I2C1_SDA_PORT, .sda_pin = I2C1_SDA_PIN, .sda_af = I2C1_SDA_AF, \
    }
#endif /* I2C1_BUS_CONFIG */
#endif /* BSP_USING_I2C1 */

#ifdef BSP_USING_I2C2
#if !defined(BSP_I2C2_SCL_PE1) && !defined(BSP_I2C2_SCL_PB10) && !defined(BSP_I2C2_SCL_PG10)
#define BSP_I2C2_SCL_PE1
#endif
#if !defined(BSP_I2C2_SDA_PE0) && !defined(BSP_I2C2_SDA_PB11) && !defined(BSP_I2C2_SDA_PG11)
#define BSP_I2C2_SDA_PE0
#endif

#if defined(BSP_I2C2_SCL_PE1)
  #define I2C2_SCL_PORT    GPIOE
  #define I2C2_SCL_PIN     GPIO_PIN_1
  #define I2C2_SCL_AF      GPIO_FUNCTION_7
#elif defined(BSP_I2C2_SCL_PB10)
  #define I2C2_SCL_PORT    GPIOB
  #define I2C2_SCL_PIN     GPIO_PIN_10
  #define I2C2_SCL_AF      GPIO_FUNCTION_0
#elif defined(BSP_I2C2_SCL_PG10)
  #define I2C2_SCL_PORT    GPIOG
  #define I2C2_SCL_PIN     GPIO_PIN_10
  #define I2C2_SCL_AF      GPIO_FUNCTION_0
#else
  #define I2C2_SCL_PORT    GPIOE
  #define I2C2_SCL_PIN     GPIO_PIN_1
  #define I2C2_SCL_AF      GPIO_FUNCTION_7
#endif

#if defined(BSP_I2C2_SDA_PE0)
  #define I2C2_SDA_PORT    GPIOE
  #define I2C2_SDA_PIN     GPIO_PIN_0
  #define I2C2_SDA_AF      GPIO_FUNCTION_7
#elif defined(BSP_I2C2_SDA_PB11)
  #define I2C2_SDA_PORT    GPIOB
  #define I2C2_SDA_PIN     GPIO_PIN_11
  #define I2C2_SDA_AF      GPIO_FUNCTION_0
#elif defined(BSP_I2C2_SDA_PG11)
  #define I2C2_SDA_PORT    GPIOG
  #define I2C2_SDA_PIN     GPIO_PIN_11
  #define I2C2_SDA_AF      GPIO_FUNCTION_0
#else
  #define I2C2_SDA_PORT    GPIOE
  #define I2C2_SDA_PIN     GPIO_PIN_0
  #define I2C2_SDA_AF      GPIO_FUNCTION_7
#endif

#ifndef I2C2_BUS_CONFIG
#define I2C2_BUS_CONFIG                                                 \
    {                                                                   \
        .name = "i2c2",                                                 \
        .Instance = I2C2,                                               \
        .irq_type = I2C2_IRQn,                                          \
        .clock_speed = CLOCK_SPEED_STANDARD,                            \
        .scl_port = I2C2_SCL_PORT, .scl_pin = I2C2_SCL_PIN, .scl_af = I2C2_SCL_AF, \
        .sda_port = I2C2_SDA_PORT, .sda_pin = I2C2_SDA_PIN, .sda_af = I2C2_SDA_AF, \
    }
#endif /* I2C2_BUS_CONFIG */
#endif /* BSP_USING_I2C2 */

#ifdef __cplusplus
}
#endif

#endif /* __I2C_CONFIG_H__ */
