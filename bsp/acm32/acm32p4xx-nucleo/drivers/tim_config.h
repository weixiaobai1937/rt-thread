/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx TIM config for clock_timer
 */

#ifndef __TIM_CONFIG_H__
#define __TIM_CONFIG_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TIM_DEV_INFO_CONFIG
#define TIM_DEV_INFO_CONFIG                     \
    {                                           \
        .maxfreq = 1000000,                     \
        .minfreq = 2000,                        \
        .maxcnt  = 0xFFFF,                      \
        .cntmode = CLOCK_TIMER_CNTMODE_UP,      \
    }
#endif

#ifndef TIM2_DEV_INFO_CONFIG
#define TIM2_DEV_INFO_CONFIG                    \
    {                                           \
        .maxfreq = 1000000,                     \
        .minfreq = 2000,                        \
        .maxcnt  = 0xFFFFFFFF,                  \
        .cntmode = CLOCK_TIMER_CNTMODE_UP,      \
    }
#endif

#ifdef BSP_USING_TIM1
#ifndef TIM1_CONFIG
#define TIM1_CONFIG                             \
    {                                           \
       .tim_handle.Instance = TIM1,             \
       .tim_irqn            = TIM1_BRK_UP_TRG_COM_IRQn, \
       .name                = "timer1",         \
    }
#endif
#endif

#ifdef BSP_USING_TIM2
#ifndef TIM2_CONFIG
#define TIM2_CONFIG                             \
    {                                           \
       .tim_handle.Instance = TIM2,             \
       .tim_irqn            = TIM2_IRQn,        \
       .name                = "timer2",         \
    }
#endif
#endif

#ifdef BSP_USING_TIM3
#ifndef TIM3_CONFIG
#define TIM3_CONFIG                             \
    {                                           \
       .tim_handle.Instance = TIM3,             \
       .tim_irqn            = TIM3_IRQn,        \
       .name                = "timer3",         \
    }
#endif
#endif

#ifdef BSP_USING_TIM6
#ifndef TIM6_CONFIG
#define TIM6_CONFIG                             \
    {                                           \
       .tim_handle.Instance = TIM6,             \
       .tim_irqn            = TIM6_IRQn,        \
       .name                = "timer6",         \
    }
#endif
#endif

#ifdef BSP_USING_TIM10
#ifndef TIM10_CONFIG
#define TIM10_CONFIG                            \
    {                                           \
       .tim_handle.Instance = TIM10,            \
       .tim_irqn            = TIM10_IRQn,       \
       .name                = "timer10",        \
    }
#endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* __TIM_CONFIG_H__ */
