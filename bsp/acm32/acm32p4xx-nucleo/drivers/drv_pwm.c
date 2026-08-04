/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx PWM driver
 */

#include <board.h>
#include <rtdevice.h>
#include "drv_tim_utils.h"

#if defined(RT_USING_PWM) && (defined(BSP_USING_PWM1) || defined(BSP_USING_PWM2) || \
                              defined(BSP_USING_PWM3) || defined(BSP_USING_PWM10))

extern HAL_StatusTypeDef HAL_TIMER_OC_Stop(TIM_TypeDef *TIMx, uint32_t Channel);

#define DBG_TAG "drv.pwm"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define PWM_CH1_MASK  (1U << 0)
#define PWM_CH2_MASK  (1U << 1)
#define PWM_CH3_MASK  (1U << 2)
#define PWM_CH4_MASK  (1U << 3)

#define PWM_MAX_PERIOD    65535
#define PWM_MIN_PERIOD    2
#define PWM_MIN_PULSE     1

enum
{
#ifdef BSP_USING_PWM1
    PWM1_INDEX,
#endif
#ifdef BSP_USING_PWM2
    PWM2_INDEX,
#endif
#ifdef BSP_USING_PWM3
    PWM3_INDEX,
#endif
#ifdef BSP_USING_PWM10
    PWM10_INDEX,
#endif
};

struct acm32_pwm
{
    struct rt_device_pwm pwm_device;
    TIM_HandleTypeDef    tim_handle;
    rt_uint8_t           channel_mask;
    char                *name;
};

static struct acm32_pwm acm32_pwm_obj[] =
{
#ifdef BSP_USING_PWM1
    {.tim_handle.Instance = TIM1, .name = "pwm1", .channel_mask = 0},
#endif
#ifdef BSP_USING_PWM2
    {.tim_handle.Instance = TIM2, .name = "pwm2", .channel_mask = 0},
#endif
#ifdef BSP_USING_PWM3
    {.tim_handle.Instance = TIM3, .name = "pwm3", .channel_mask = 0},
#endif
#ifdef BSP_USING_PWM10
    {.tim_handle.Instance = TIM10, .name = "pwm10", .channel_mask = 0},
#endif
};

static void pwm_channel_mask_init(void)
{
#ifdef BSP_USING_PWM1_CH1
    acm32_pwm_obj[PWM1_INDEX].channel_mask |= PWM_CH1_MASK;
#endif
#ifdef BSP_USING_PWM1_CH2
    acm32_pwm_obj[PWM1_INDEX].channel_mask |= PWM_CH2_MASK;
#endif
#ifdef BSP_USING_PWM1_CH3
    acm32_pwm_obj[PWM1_INDEX].channel_mask |= PWM_CH3_MASK;
#endif
#ifdef BSP_USING_PWM1_CH4
    acm32_pwm_obj[PWM1_INDEX].channel_mask |= PWM_CH4_MASK;
#endif
#ifdef BSP_USING_PWM2_CH1
    acm32_pwm_obj[PWM2_INDEX].channel_mask |= PWM_CH1_MASK;
#endif
#ifdef BSP_USING_PWM2_CH2
    acm32_pwm_obj[PWM2_INDEX].channel_mask |= PWM_CH2_MASK;
#endif
#ifdef BSP_USING_PWM2_CH3
    acm32_pwm_obj[PWM2_INDEX].channel_mask |= PWM_CH3_MASK;
#endif
#ifdef BSP_USING_PWM2_CH4
    acm32_pwm_obj[PWM2_INDEX].channel_mask |= PWM_CH4_MASK;
#endif
#ifdef BSP_USING_PWM3_CH1
    acm32_pwm_obj[PWM3_INDEX].channel_mask |= PWM_CH1_MASK;
#endif
#ifdef BSP_USING_PWM3_CH2
    acm32_pwm_obj[PWM3_INDEX].channel_mask |= PWM_CH2_MASK;
#endif
#ifdef BSP_USING_PWM3_CH3
    acm32_pwm_obj[PWM3_INDEX].channel_mask |= PWM_CH3_MASK;
#endif
#ifdef BSP_USING_PWM3_CH4
    acm32_pwm_obj[PWM3_INDEX].channel_mask |= PWM_CH4_MASK;
#endif
#ifdef BSP_USING_PWM10_CH1
    acm32_pwm_obj[PWM10_INDEX].channel_mask |= PWM_CH1_MASK;
#endif
}

static uint32_t hal_channel_from_rt_ch(uint32_t rt_ch)
{
    switch (rt_ch)
    {
    case 1:
        return TIM_CHANNEL_1;
    case 2:
        return TIM_CHANNEL_2;
    case 3:
        return TIM_CHANNEL_3;
    case 4:
        return TIM_CHANNEL_4;
    default:
        LOG_E("invalid PWM channel %u, valid: 1-4", rt_ch);
        return 0xFFFFFFFF;  /* invalid channel sentinel */
    }
}

static rt_err_t drv_pwm_set(TIM_HandleTypeDef *htim, struct rt_pwm_configuration *cfg)
{
    rt_uint64_t timer_clk;
    rt_uint32_t period, pulse, hal_ch;
    uint32_t psc;

    if (cfg->period == 0)
    {
        return -RT_EINVAL;
    }

    timer_clk = acm32_tim_clock_get(htim->Instance);
    timer_clk /= 1000000UL;

    period = (rt_uint64_t)cfg->period * timer_clk / 1000ULL;
    psc = (period + PWM_MAX_PERIOD - 1) / PWM_MAX_PERIOD;
    if (psc > 0x10000U)
    {
        return -RT_EINVAL;
    }
    period = period / psc;

    if (period < PWM_MIN_PERIOD)
    {
        period = PWM_MIN_PERIOD;
    }

    pulse = (rt_uint64_t)cfg->pulse * timer_clk / psc / 1000ULL;
    if (pulse < PWM_MIN_PULSE)
    {
        pulse = PWM_MIN_PULSE;
    }
    else if (pulse > period)
    {
        pulse = period;
    }

    hal_ch = hal_channel_from_rt_ch(cfg->channel);
    if (hal_ch == 0xFFFFFFFF)
        return -RT_EINVAL;

    htim->Init.Prescaler         = psc - 1;
    htim->Init.Period            = period - 1;
    htim->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim->Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim->Init.ARRPreLoadEn      = TIM_ARR_PRELOAD_ENABLE;
    htim->Init.RepetitionCounter = 0;

    if (HAL_TIMER_Base_Init(htim) != HAL_OK)
    {
        LOG_E("HAL_TIMER_Base_Init failed for ch%u", cfg->channel);
        return -RT_ERROR;
    }

    {
        TIM_OC_InitTypeDef oc_cfg = {0};

        oc_cfg.OCMode       = OUTPUT_MODE_PWM1;
        oc_cfg.Pulse        = pulse - 1;
        oc_cfg.OCPolarity   = OUTPUT_POL_ACTIVE_HIGH;
        oc_cfg.OCNPolarity  = OUTPUT_POL_ACTIVE_HIGH;
        oc_cfg.OCFastMode   = OUTPUT_FAST_MODE_DISABLE;
        oc_cfg.OCIdleState  = OUTPUT_DISABLE_IDLE_STATE;
        oc_cfg.OCNIdleState = OUTPUT_DISABLE_IDLE_STATE;

        if (HAL_TIMER_Output_Config(htim->Instance, &oc_cfg, hal_ch) != HAL_OK)
        {
            LOG_E("HAL_TIMER_Output_Config failed for ch%u", cfg->channel);
            return -RT_ERROR;
        }
    }

    htim->Instance->CNT = 0;
    htim->Instance->EGR |= TIM_EVENTSOURCE_UPDATE;

    return RT_EOK;
}

static rt_err_t drv_pwm_enable(TIM_HandleTypeDef *htim, struct rt_pwm_configuration *cfg, rt_bool_t enable)
{
    uint32_t hal_ch = hal_channel_from_rt_ch(cfg->channel);

    if (enable)
    {
        if (cfg->complementary)
        {
            HAL_TIMER_OCxN_Start(htim->Instance, hal_ch);
        }
        else
        {
            HAL_TIMER_OC_Start(htim->Instance, hal_ch);
        }
        HAL_TIMER_Base_Start(htim->Instance);
    }
    else
    {
        if (cfg->complementary)
        {
            /* No OCxN stop API in this HAL; clear the CCER output-enable bit. */
            htim->Instance->CCER &= ~(1U << (hal_ch * 4U + 2U));
        }
        else
        {
            HAL_TIMER_OC_Stop(htim->Instance, hal_ch);
        }
        /* 检查是否所有通道均已禁用，若是则停止定时器基计数器以省电 */
        if ((htim->Instance->CCER & 0x5555U) == 0)
        {
            HAL_TIMER_Base_Stop(htim->Instance);
        }
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_get(TIM_HandleTypeDef *htim, struct rt_pwm_configuration *cfg)
{
    rt_uint64_t timer_clk;
    rt_uint32_t hal_ch;
    rt_uint32_t ccr_val;

    timer_clk = acm32_tim_clock_get(htim->Instance);
    timer_clk /= 1000000UL;

    hal_ch = hal_channel_from_rt_ch(cfg->channel);

    switch (hal_ch)
    {
    case TIM_CHANNEL_1:
        ccr_val = htim->Instance->CCR1;
        break;
    case TIM_CHANNEL_2:
        ccr_val = htim->Instance->CCR2;
        break;
    case TIM_CHANNEL_3:
        ccr_val = htim->Instance->CCR3;
        break;
    case TIM_CHANNEL_4:
        ccr_val = htim->Instance->CCR4;
        break;
    default:
        ccr_val = 0;
        break;
    }

    cfg->period = (rt_uint64_t)(htim->Instance->ARR + 1) * (htim->Instance->PSC + 1) * 1000UL / timer_clk;
    cfg->pulse  = (rt_uint64_t)(ccr_val + 1) * (htim->Instance->PSC + 1) * 1000UL / timer_clk;

    return RT_EOK;
}

static rt_err_t acm32_pwm_control(struct rt_device_pwm *device, int cmd, void *arg)
{
    struct rt_pwm_configuration *cfg = (struct rt_pwm_configuration *)arg;
    TIM_HandleTypeDef *htim;
    struct acm32_pwm *pwm;
    rt_uint8_t ch_bit;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(cfg != RT_NULL);

    htim = (TIM_HandleTypeDef *)device->parent.user_data;
    pwm = rt_container_of(device, struct acm32_pwm, pwm_device);

    if (cfg->channel < 1 || cfg->channel > 4)
        return -RT_EINVAL;

    ch_bit = (rt_uint8_t)(1U << (cfg->channel - 1));
    if ((pwm->channel_mask & ch_bit) == 0)
    {
        LOG_E("%s channel %u not enabled in Kconfig", pwm->name, cfg->channel);
        return -RT_EINVAL;
    }

    switch (cmd)
    {
    case PWM_CMD_ENABLE:
        return drv_pwm_enable(htim, cfg, RT_TRUE);
    case PWM_CMD_DISABLE:
        return drv_pwm_enable(htim, cfg, RT_FALSE);
    case PWM_CMD_SET:
        return drv_pwm_set(htim, cfg);
    case PWM_CMD_GET:
        return drv_pwm_get(htim, cfg);
    default:
        return -RT_EINVAL;
    }
}

static const struct rt_pwm_ops acm_pwm_ops =
{
    .control = acm32_pwm_control,
};

static int rt_hw_pwm_init(void)
{
    rt_uint32_t i;
    rt_err_t result = RT_EOK;

    pwm_channel_mask_init();

    for (i = 0; i < sizeof(acm32_pwm_obj) / sizeof(acm32_pwm_obj[0]); i++)
    {
        if (acm32_pwm_obj[i].channel_mask == 0)
        {
            continue;
        }

        HAL_TIMER_MSP_Init(&acm32_pwm_obj[i].tim_handle);

        result = rt_device_pwm_register(&acm32_pwm_obj[i].pwm_device,
                                        acm32_pwm_obj[i].name,
                                        &acm_pwm_ops,
                                        &acm32_pwm_obj[i].tim_handle);
        if (result != RT_EOK)
        {
            LOG_E("%s register failed", acm32_pwm_obj[i].name);
            break;
        }
        LOG_D("%s registered", acm32_pwm_obj[i].name);
    }

    return result;
}
INIT_DEVICE_EXPORT(rt_hw_pwm_init);

#endif /* RT_USING_PWM && BSP_USING_PWMx */
