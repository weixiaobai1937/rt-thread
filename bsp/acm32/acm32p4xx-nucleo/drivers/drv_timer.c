/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx clock_timer driver
 */

#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "drv_tim_utils.h"

#ifdef RT_USING_CLOCK_TIME
#if defined(BSP_USING_TIM1) || defined(BSP_USING_TIM2) || defined(BSP_USING_TIM3) || \
    defined(BSP_USING_TIM6) || defined(BSP_USING_TIM10)

#include "tim_config.h"

/*
 * HAL_TIMER_MSP_Init 位于 drv_tim_utils.c，供 timer/capture/encoder/PWM 共享。
 */

enum
{
#ifdef BSP_USING_TIM1
    TIM1_INDEX,
#endif
#ifdef BSP_USING_TIM2
    TIM2_INDEX,
#endif
#ifdef BSP_USING_TIM3
    TIM3_INDEX,
#endif
#ifdef BSP_USING_TIM6
    TIM6_INDEX,
#endif
#ifdef BSP_USING_TIM10
    TIM10_INDEX,
#endif
};

struct acm32_clock_timer
{
    rt_clock_timer_t    time_device;
    TIM_HandleTypeDef   tim_handle;
    IRQn_Type           tim_irqn;
    char               *name;
};

static struct acm32_clock_timer acm32_clock_timer_obj[] =
{
#ifdef BSP_USING_TIM1
    TIM1_CONFIG,
#endif
#ifdef BSP_USING_TIM2
    TIM2_CONFIG,
#endif
#ifdef BSP_USING_TIM3
    TIM3_CONFIG,
#endif
#ifdef BSP_USING_TIM6
    TIM6_CONFIG,
#endif
#ifdef BSP_USING_TIM10
    TIM10_CONFIG,
#endif
};

static void timer_init(struct rt_clock_timer_device *timer, rt_uint32_t state)
{
    TIM_HandleTypeDef *tim;
    rt_uint32_t timer_clock;

    RT_ASSERT(timer != RT_NULL);
    if (!state)
    {
        return;
    }

    tim = (TIM_HandleTypeDef *)timer->parent.user_data;
    timer_clock = acm32_tim_clock_get(tim->Instance);

    if (timer->freq == 0)
    {
        timer->freq = 1000000; /* default 1MHz count frequency */
    }

    /* 防止 freq > timer_clock 导致 Prescaler 下溢为 0xFFFFFFFF */
    if ((rt_uint32_t)timer->freq > timer_clock)
    {
        timer->freq = (rt_int32_t)timer_clock;
    }

    tim->Init.Period = 1000 - 1;
    tim->Init.Prescaler = (timer_clock / (rt_uint32_t)timer->freq) - 1;
    tim->Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    tim->Init.CounterMode = (timer->info->cntmode == CLOCK_TIMER_CNTMODE_UP) ?
                            TIM_COUNTERMODE_UP : TIM_COUNTERMODE_DOWN;
    tim->Init.RepetitionCounter = 0;
    tim->Init.ARRPreLoadEn = TIM_ARR_PRELOAD_ENABLE;

    HAL_TIMER_MSP_Init(tim);
    HAL_TIMER_Base_Init(tim);
}

static rt_err_t timer_start(rt_clock_timer_t *timer, rt_uint32_t t, rt_clock_timer_mode_t opmode)
{
    TIM_HandleTypeDef *tim;

    RT_ASSERT(timer != RT_NULL);
    tim = (TIM_HandleTypeDef *)timer->parent.user_data;

    tim->Instance->CNT = 0;
    tim->Instance->ARR = (t > 0) ? (t - 1) : 0;

    if (opmode == CLOCK_TIMER_MODE_ONESHOT)
    {
        SET_BIT(tim->Instance->CR1, BIT3); /* OPM */
    }
    else
    {
        CLEAR_BIT(tim->Instance->CR1, BIT3);
    }

    HAL_TIM_ENABLE_IT(tim, TIMER_INT_EN_UPD);
    HAL_TIMER_Base_Start(tim->Instance);

    return RT_EOK;
}

static void timer_stop(rt_clock_timer_t *timer)
{
    TIM_HandleTypeDef *tim;

    RT_ASSERT(timer != RT_NULL);
    tim = (TIM_HandleTypeDef *)timer->parent.user_data;

    HAL_TIM_DISABLE_IT(tim, TIMER_INT_EN_UPD);
    HAL_TIMER_Base_Stop(tim->Instance);
}

static rt_err_t timer_ctrl(rt_clock_timer_t *timer, rt_uint32_t cmd, void *arg)
{
    TIM_HandleTypeDef *tim;
    rt_err_t result = RT_EOK;

    RT_ASSERT(timer != RT_NULL);
    RT_ASSERT(arg != RT_NULL);

    tim = (TIM_HandleTypeDef *)timer->parent.user_data;

    switch (cmd)
    {
    case CLOCK_TIMER_CTRL_FREQ_SET:
    {
        rt_uint32_t freq = *((rt_uint32_t *)arg);
        rt_uint32_t timer_clock = acm32_tim_clock_get(tim->Instance);
        rt_uint32_t psc;

        if (freq == 0)
        {
            return -RT_EINVAL;
        }
        if (freq > timer_clock)
        {
            return -RT_EINVAL;
        }

        psc = timer_clock / freq;
        tim->Instance->PSC = psc - 1;
        tim->Instance->EGR |= TIM_EVENTSOURCE_UPDATE;
        timer->freq = (rt_int32_t)(timer_clock / psc);
    }
    break;
    default:
        result = -RT_ENOSYS;
        break;
    }

    return result;
}

static rt_uint32_t timer_counter_get(rt_clock_timer_t *timer)
{
    RT_ASSERT(timer != RT_NULL);
    return ((TIM_HandleTypeDef *)timer->parent.user_data)->Instance->CNT;
}

static const struct rt_clock_timer_info _info = TIM_DEV_INFO_CONFIG;
#ifdef BSP_USING_TIM2
static const struct rt_clock_timer_info _info32 = TIM2_DEV_INFO_CONFIG;
#endif

static const struct rt_clock_timer_ops _ops =
{
    .init = timer_init,
    .start = timer_start,
    .stop = timer_stop,
    .count_get = timer_counter_get,
    .control = timer_ctrl,
};

#ifdef BSP_USING_TIM1
void TIM1_BRK_UP_TRG_COM_IRQHandler(void)
{
    rt_interrupt_enter();
    if (TIM1->SR & TIMER_SR_UIF)
    {
        rt_clock_timer_isr(&acm32_clock_timer_obj[TIM1_INDEX].time_device);
    }
    TIM1->SR = TIMER_SR_UIF;
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_TIM2
void TIM2_IRQHandler(void)
{
    rt_interrupt_enter();
    if (TIM2->SR & TIMER_SR_UIF)
    {
        rt_clock_timer_isr(&acm32_clock_timer_obj[TIM2_INDEX].time_device);
    }
    TIM2->SR = TIMER_SR_UIF;
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_TIM3
void TIM3_IRQHandler(void)
{
    rt_interrupt_enter();
    if (TIM3->SR & TIMER_SR_UIF)
    {
        rt_clock_timer_isr(&acm32_clock_timer_obj[TIM3_INDEX].time_device);
    }
    TIM3->SR = TIMER_SR_UIF;
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_TIM6
void TIM6_IRQHandler(void)
{
    rt_interrupt_enter();
    if (TIM6->SR & TIMER_SR_UIF)
    {
        rt_clock_timer_isr(&acm32_clock_timer_obj[TIM6_INDEX].time_device);
    }
    TIM6->SR = TIMER_SR_UIF;
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_TIM10
void TIM10_IRQHandler(void)
{
    rt_interrupt_enter();
    if (TIM10->SR & TIMER_SR_UIF)
    {
        rt_clock_timer_isr(&acm32_clock_timer_obj[TIM10_INDEX].time_device);
    }
    TIM10->SR = TIMER_SR_UIF;
    rt_interrupt_leave();
}
#endif

static int acm32_clock_timer_init(void)
{
    int i;
    int result = RT_EOK;
    const struct rt_clock_timer_info *info;

    for (i = 0; i < (int)(sizeof(acm32_clock_timer_obj) / sizeof(acm32_clock_timer_obj[0])); i++)
    {
        info = &_info;
#ifdef BSP_USING_TIM2
        if (acm32_clock_timer_obj[i].tim_handle.Instance == TIM2)
        {
            info = &_info32;
        }
#endif
        acm32_clock_timer_obj[i].time_device.info = info;
        acm32_clock_timer_obj[i].time_device.ops  = &_ops;
        result = rt_clock_timer_register(&acm32_clock_timer_obj[i].time_device,
                                         acm32_clock_timer_obj[i].name,
                                         &acm32_clock_timer_obj[i].tim_handle);
        if (result != RT_EOK)
        {
            break;
        }
    }

    return result;
}
INIT_BOARD_EXPORT(acm32_clock_timer_init);

#endif /* BSP_USING_TIMx */
#endif /* RT_USING_CLOCK_TIME */
