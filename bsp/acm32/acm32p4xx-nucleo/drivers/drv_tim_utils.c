/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-27     AisinoChip   提取共享定时器时钟获取函数
 */

#include "board.h"
#include <rtthread.h>

/* 根据 TIM 实例判断 APB1/APB2 总线时钟，HCLK!=PCLK 时自动倍频 */
rt_uint32_t acm32_tim_clock_get(TIM_TypeDef *instance)
{
    rt_uint32_t pclk;
    uint32_t base = (uint32_t)instance;

    switch (base)
    {
    /* APB2 总线定时器 */
    case TIM1_BASE_ADDR:
    case TIM10_BASE_ADDR:
        pclk = HAL_RCC_GetPCLK2Freq();
        break;
    /* APB1 总线定时器 */
    case TIM2_BASE_ADDR:
    case TIM3_BASE_ADDR:
    case TIM6_BASE_ADDR:
    default:
        pclk = HAL_RCC_GetPCLK1Freq();
        break;
    }

    /* 当 HCLK != PCLK 时，定时器时钟自动倍频（Cortex-M 手册规定） */
    if (HAL_RCC_GetHCLKFreq() != pclk)
    {
        pclk <<= 1;
    }

    return pclk;
}
