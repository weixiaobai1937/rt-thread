/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx FDCAN driver (CAN classic mode)
 */

#ifndef __DRV_CAN_H__
#define __DRV_CAN_H__

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_CAN) && (defined(BSP_USING_FDCAN1) || defined(BSP_USING_FDCAN2))

#ifdef __cplusplus
extern "C" {
#endif

/* FDCAN clock = AHB1 bus clock, configurable per BSP */
#ifndef FDCAN_CLOCK_HZ
#define FDCAN_CLOCK_HZ          60000000U
#endif

#ifdef BSP_USING_FDCAN1
#ifdef BSP_FDCAN1_TX_PD1
#define FDCAN1_TX_PORT          GPIOD
#define FDCAN1_TX_PIN           GPIO_PIN_1
#elif defined(BSP_FDCAN1_TX_PB9)
#define FDCAN1_TX_PORT          GPIOB
#define FDCAN1_TX_PIN           GPIO_PIN_9
#elif defined(BSP_FDCAN1_TX_PA12)
#define FDCAN1_TX_PORT          GPIOA
#define FDCAN1_TX_PIN           GPIO_PIN_12
#else
#define FDCAN1_TX_PORT          GPIOD
#define FDCAN1_TX_PIN           GPIO_PIN_1
#endif

#ifdef BSP_FDCAN1_RX_PD0
#define FDCAN1_RX_PORT          GPIOD
#define FDCAN1_RX_PIN           GPIO_PIN_0
#elif defined(BSP_FDCAN1_RX_PB8)
#define FDCAN1_RX_PORT          GPIOB
#define FDCAN1_RX_PIN           GPIO_PIN_8
#elif defined(BSP_FDCAN1_RX_PA11)
#define FDCAN1_RX_PORT          GPIOA
#define FDCAN1_RX_PIN           GPIO_PIN_11
#else
#define FDCAN1_RX_PORT          GPIOD
#define FDCAN1_RX_PIN           GPIO_PIN_0
#endif

#define FDCAN1_AF               GPIO_FUNCTION_4
#endif /* BSP_USING_FDCAN1 */

#ifdef BSP_USING_FDCAN2
#ifdef BSP_FDCAN2_TX_PE6
#define FDCAN2_TX_PORT          GPIOE
#define FDCAN2_TX_PIN           GPIO_PIN_6
#elif defined(BSP_FDCAN2_TX_PD12)
#define FDCAN2_TX_PORT          GPIOD
#define FDCAN2_TX_PIN           GPIO_PIN_12
#else
#define FDCAN2_TX_PORT          GPIOE
#define FDCAN2_TX_PIN           GPIO_PIN_6
#endif

#ifdef BSP_FDCAN2_RX_PE5
#define FDCAN2_RX_PORT          GPIOE
#define FDCAN2_RX_PIN           GPIO_PIN_5
#elif defined(BSP_FDCAN2_RX_PD11)
#define FDCAN2_RX_PORT          GPIOD
#define FDCAN2_RX_PIN           GPIO_PIN_11
#else
#define FDCAN2_RX_PORT          GPIOE
#define FDCAN2_RX_PIN           GPIO_PIN_5
#endif

#define FDCAN2_AF               GPIO_FUNCTION_1
#endif /* BSP_USING_FDCAN2 */

typedef struct
{
    const char *name;
    FDCAN_HandleTypeDef fdcanHandle;
    FDCAN_RxHeaderTypeDef RxHeader;
    FDCAN_TxHeaderTypeDef TxHeader;
    FDCAN_NewFilterTypeDef FilterConfig;
    struct rt_can_device device;
} acm32_can_t;

#ifdef __cplusplus
}
#endif

#endif /* RT_USING_CAN */
#endif /* __DRV_CAN_H__ */
