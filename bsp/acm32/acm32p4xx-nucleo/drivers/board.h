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

/** After configuring corresponding UART or UART DMA, you can use it.
  *
  * STEP 1, define macro define related to the serial port opening based on the serial port number
  *                 such as     #define BSP_USING_UART1
  *
  * STEP 2, according to the corresponding pin of serial port, modify the related serial port information
  *                 such as     #define UART1_TX_PORT            GPIOA
  *                             #define UART1_RX_PORT            GPIOA
  *                             #define UART1_TX_PIN        GPIO_PIN_9
  *                             #define UART1_RX_PIN        GPIO_PIN_10
  *
  * STEP 3, if you want using SERIAL DMA, you must open it in the RT-Thread Settings.
  *                 RT-Thread Setting -> Components -> Device Drivers -> Serial Device Drivers -> Enable Serial DMA Mode
  *
  * STEP 4, according to serial port number to define serial port tx/rx DMA function in the board.h file
  *                 such as     #define BSP_UART1_RX_USING_DMA
  *
  */

/* ACM32P4xx Nucleo Board: UART1 TX=PA9, RX=PA10 */
#if defined(BSP_USING_UART1)
    #define UART1_TX_PORT           GPIOA
    #define UART1_RX_PORT           GPIOA
    #define UART1_TX_PIN            GPIO_PIN_9
    #define UART1_RX_PIN            GPIO_PIN_10

    #if defined(BSP_UART1_RX_USING_DMA)
        #define UART1_RX_DMA_INSTANCE   DMA1_Channel0
        #define UART1_RX_DMA_IRQ        DMA1_CH0_IRQn
        #define UART1_RX_DMA_REQUEST    DMA_REQUEST_USART1_RX
    #endif /* BSP_UART1_RX_USING_DMA */

    #if defined(BSP_UART1_TX_USING_DMA)
        #define UART1_TX_DMA_INSTANCE   DMA1_Channel1
        #define UART1_TX_DMA_IRQ        DMA1_CH1_IRQn
        #define UART1_TX_DMA_REQUEST    DMA_REQUEST_USART1_TX
    #endif /* BSP_UART1_TX_USING_DMA */

#endif /* BSP_USING_UART1 */

/*-------------------------- UART CONFIG END --------------------------*/

/* board configuration */

#define SOC_SRAM_START_ADDR     (0x20000000)
#define SOC_SRAM_SIZE           (128)

void rt_hw_board_init(void);

#endif /* __BOARD_H__ */
