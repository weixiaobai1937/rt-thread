/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx LPTIMER driver
 *
 * This driver initializes LPTIM1 hardware for low-power wakeup.
 * It registers as an INIT_DEVICE but does NOT register a standard
 * RT-Thread timer device (rt_lptimer).
 * To use LPTIM as a userspace timer, enable RT_USING_PM and use the
 * rt_lptimer software API.
 */

#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_LPTIMER

#define DBG_TAG "drv.lptimer"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static LPTIM_HandleTypeDef hlptim1;

static void HAL_LPTIM_AutoReloadMatchCallback(LPTIM_HandleTypeDef *hlptim)
{
    RT_UNUSED(hlptim);
    /* Placeholder for PM wakeup or user callback. */
    /* The LPTIM ARR match flag is auto-cleared by HAL_LPTIM_IRQHandler. */
}

void LPTIM1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_LPTIM_IRQHandler(&hlptim1);
    rt_interrupt_leave();
}

void HAL_LPTIM_MspInit(LPTIM_HandleTypeDef *hlptim)
{
    RT_UNUSED(hlptim);
    __HAL_RCC_LPTIM1_CLK_ENABLE();
    HAL_NVIC_SetPriority(LPTIM1_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(LPTIM1_IRQn);
}

static int rt_hw_lptimer_init(void)
{
    hlptim1.Instance = LPTIM1;
    hlptim1.Init.ClockSource = LPTIM_CLOCKSOURCE_INTERNAL_PCLK;
    hlptim1.Init.ClockPrescaler = LPTIM_CLOCKPRESCALER_DIV128;
    hlptim1.Init.CounterSource = LPTIM_COUNTERSOURCE_INTERNAL;
    hlptim1.Init.TriggerSource = LPTIM_TRIGSOURCE_SOFTWARE;
    hlptim1.Init.UpdateMode = LPTIM_UPDATE_IMMEDIATE;
    hlptim1.Init.WaveformPolarity = LPTIM_WAVEFORMPOLARITY_HIGH;
    hlptim1.Init.TriggerPolarity = LPTIM_TRIGPOLARITY_RISING;
    hlptim1.Init.TriggerFilter = LPTIM_TRIGFILTER_DISABLE;
    hlptim1.Init.CounterPolarity = LPTIM_COUNTERPOLARITY_RISING;
    hlptim1.Init.CounterFilter = LPTIM_COUNTERFILTER_DISABLE;
    hlptim1.Init.Input1Source = LPTIM_INPUT1SOURCE_0;
    hlptim1.Init.Input2Source = LPTIM_INPUT2SOURCE_0;

    if (HAL_LPTIM_Init(&hlptim1) != HAL_OK)
    {
        LOG_E("LPTIM1 init failed");
        return -RT_ERROR;
    }

    if (HAL_LPTIM_RegisterCallback(&hlptim1, LPTIM_CALLBACKID_AUTORELOAD_MATCH,
                                   HAL_LPTIM_AutoReloadMatchCallback) != HAL_OK)
    {
        LOG_E("LPTIM1 register callback failed");
        HAL_LPTIM_DeInit(&hlptim1);
        NVIC_DisableIRQ(LPTIM1_IRQn);
        return -RT_ERROR;
    }

    if (HAL_LPTIM_ConfigCountValue(&hlptim1, 0, 0xFFFF, 0) != HAL_OK)
    {
        LOG_E("LPTIM1 config count failed");
        HAL_LPTIM_DeInit(&hlptim1);
        NVIC_DisableIRQ(LPTIM1_IRQn);
        return -RT_ERROR;
    }

    if (HAL_LPTIM_EnableIT(&hlptim1, LPTIM_IT_ARRM) != HAL_OK)
    {
        LOG_E("LPTIM1 enable IT failed");
        HAL_LPTIM_DeInit(&hlptim1);
        NVIC_DisableIRQ(LPTIM1_IRQn);
        return -RT_ERROR;
    }

    if (HAL_LPTIM_Timeout_Start(&hlptim1) != HAL_OK)
    {
        LOG_E("LPTIM1 start failed");
        HAL_LPTIM_DeInit(&hlptim1);
        NVIC_DisableIRQ(LPTIM1_IRQn);
        return -RT_ERROR;
    }

    LOG_I("LPTIM1 init ok, pclk=%luHz, prescaler=128, period=0xFFFF",
          HAL_RCC_GetPCLK1Freq());

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_lptimer_init);

#endif /* BSP_USING_LPTIMER */
