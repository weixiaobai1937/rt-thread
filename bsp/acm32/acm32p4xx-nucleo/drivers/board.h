/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-25     AisinoChip   first implementation
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtconfig.h>
#include "acm32p4xx_hal_conf.h"

/*-------------------------- UART CONFIG BEGIN --------------------------*/

/* ACM32P4xx Nucleo Board default UART pin mappings */

#if defined(BSP_USING_UART1)
    #define UART1_TX_PORT           GPIOA
    #define UART1_RX_PORT           GPIOA
    #define UART1_TX_PIN            GPIO_PIN_9
    #define UART1_RX_PIN            GPIO_PIN_10
#endif

/* UART2-4 + LPUART1-2 MspInit reference macros */
#if defined(BSP_USING_UART2)
    #define BSP_UART2_PORT          GPIOD
    #define BSP_UART2_PIN_TX        GPIO_PIN_5
    #define BSP_UART2_PIN_RX        GPIO_PIN_6
    #define BSP_UART2_AF            GPIO_FUNCTION_1
#endif

#if defined(BSP_USING_UART3)
    #define BSP_UART3_PORT          GPIOB
    #define BSP_UART3_PIN_TX        GPIO_PIN_10
    #define BSP_UART3_PIN_RX        GPIO_PIN_11
    #define BSP_UART3_AF            GPIO_FUNCTION_1
#endif

#if defined(BSP_USING_UART4)
    #define BSP_UART4_PORT          GPIOC
    #define BSP_UART4_PIN_TX        GPIO_PIN_10
    #define BSP_UART4_PIN_RX        GPIO_PIN_11
    #define BSP_UART4_AF            GPIO_FUNCTION_1
#endif

#if defined(BSP_USING_LPUART1)
    #define BSP_LPUART1_PORT        GPIOA
    #define BSP_LPUART1_PIN_TX      GPIO_PIN_2
    #define BSP_LPUART1_PIN_RX      GPIO_PIN_3
    #define BSP_LPUART1_AF          GPIO_FUNCTION_3
#endif

#if defined(BSP_USING_LPUART2)
    #define BSP_LPUART2_PORT        GPIOB
    #define BSP_LPUART2_PIN_TX      GPIO_PIN_10
    #define BSP_LPUART2_PIN_RX      GPIO_PIN_11
    #define BSP_LPUART2_AF          GPIO_FUNCTION_3
#endif

/*-------------------------- UART CONFIG END --------------------------*/

/* board configuration */

#define SOC_SRAM_START_ADDR     (0x20000000)
#define SOC_SRAM_SIZE           (128)

void rt_hw_board_init(void);

#endif /* __BOARD_H__ */
