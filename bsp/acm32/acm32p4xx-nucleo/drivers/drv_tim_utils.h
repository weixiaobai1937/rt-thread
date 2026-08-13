/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-27     AisinoChip   Declaration of the shared timer clock get function
 */

#ifndef __DRV_TIM_UTILS_H__
#define __DRV_TIM_UTILS_H__

#include "board.h"

/**
 * @brief Get the input clock frequency of the TIM peripheral
 *
 * Determines the APB1/APB2 bus from the TIM instance address, then doubles
 * the clock when HCLK!=PCLK. Applies to all drivers using the TIM peripheral,
 * such as PWM, Timer and InputCapture.
 *
 * @param instance TIM peripheral instance pointer (e.g. TIM1, TIM2, TIM3, TIM6, TIM10)
 * @return Timer input clock frequency (Hz)
 */
rt_uint32_t acm32_tim_clock_get(TIM_TypeDef *instance);

#endif /* __DRV_TIM_UTILS_H__ */
