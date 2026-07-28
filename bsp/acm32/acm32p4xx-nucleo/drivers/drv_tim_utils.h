/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-27     AisinoChip   共享定时器时钟获取函数声明
 */

#ifndef __DRV_TIM_UTILS_H__
#define __DRV_TIM_UTILS_H__

#include "board.h"

/**
 * @brief 获取 TIM 外设的输入时钟频率
 *
 * 根据 TIM 实例地址判断 APB1/APB2 总线，再检查 HCLK!=PCLK 时倍频。
 * 适用于 PWM、Timer、InputCapture 等所有使用 TIM 外设的驱动。
 *
 * @param instance TIM 外设实例指针（如 TIM1、TIM2、TIM3、TIM6、TIM10）
 * @return 定时器输入时钟频率（Hz）
 */
rt_uint32_t acm32_tim_clock_get(TIM_TypeDef *instance);

#endif /* __DRV_TIM_UTILS_H__ */
