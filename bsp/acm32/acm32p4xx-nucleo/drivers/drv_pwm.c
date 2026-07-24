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

static void pwm_pin_gpio_clk_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
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
        return TIM_CHANNEL_1;
    }
}

static void pwm_config_gpio(TIM_TypeDef *instance, rt_uint32_t channel)
{
    GPIO_InitTypeDef gpio = {0};
    uint32_t base = (uint32_t)instance;

    gpio.Mode  = GPIO_MODE_AF_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;

    switch (base)
    {
    case TIM1_BASE_ADDR:
        switch (channel)
        {
#ifdef BSP_USING_PWM1_CH1
        case 1:
#ifdef BSP_PWM1_CH1_PE9
            pwm_pin_gpio_clk_enable(GPIOE);
            gpio.Pin = GPIO_PIN_9;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOE, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_8;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH2
        case 2:
#ifdef BSP_PWM1_CH2_PE11
            pwm_pin_gpio_clk_enable(GPIOE);
            gpio.Pin = GPIO_PIN_11;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOE, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_9;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH3
        case 3:
#ifdef BSP_PWM1_CH3_PE13
            pwm_pin_gpio_clk_enable(GPIOE);
            gpio.Pin = GPIO_PIN_13;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOE, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_10;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH4
        case 4:
#ifdef BSP_PWM1_CH4_PE14
            pwm_pin_gpio_clk_enable(GPIOE);
            gpio.Pin = GPIO_PIN_14;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOE, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_11;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#endif
            break;
#endif
        default:
            break;
        }
        break;
    case TIM2_BASE_ADDR:
        switch (channel)
        {
#ifdef BSP_USING_PWM2_CH1
        case 1:
#ifdef BSP_PWM2_CH1_PA0
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_0;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_5;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH2
        case 2:
#ifdef BSP_PWM2_CH2_PA1
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_1;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_3;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOB, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH3
        case 3:
#ifdef BSP_PWM2_CH3_PA2
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_2;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_10;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOB, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH4
        case 4:
#ifdef BSP_PWM2_CH4_PA3
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_3;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOA, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_11;
            gpio.Alternate = GPIO_AF2;
            HAL_GPIO_Init(GPIOB, &gpio);
#endif
            break;
#endif
        default:
            break;
        }
        break;
    case TIM3_BASE_ADDR:
        switch (channel)
        {
#ifdef BSP_USING_PWM3_CH1
        case 1:
#ifdef BSP_PWM3_CH1_PA6
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_6;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOA, &gpio);
#elif defined(BSP_PWM3_CH1_PB4)
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_4;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOB, &gpio);
#elif defined(BSP_PWM3_CH1_PC6)
            pwm_pin_gpio_clk_enable(GPIOC);
            gpio.Pin = GPIO_PIN_6;
            gpio.Alternate = GPIO_AF0;
            HAL_GPIO_Init(GPIOC, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH2
        case 2:
#ifdef BSP_PWM3_CH2_PA7
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_7;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOA, &gpio);
#elif defined(BSP_PWM3_CH2_PB5)
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_5;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOB, &gpio);
#elif defined(BSP_PWM3_CH2_PC7)
            pwm_pin_gpio_clk_enable(GPIOC);
            gpio.Pin = GPIO_PIN_7;
            gpio.Alternate = GPIO_AF0;
            HAL_GPIO_Init(GPIOC, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH3
        case 3:
#ifdef BSP_PWM3_CH3_PB0
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_0;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOB, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOC);
            gpio.Pin = GPIO_PIN_8;
            gpio.Alternate = GPIO_AF0;
            HAL_GPIO_Init(GPIOC, &gpio);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH4
        case 4:
#ifdef BSP_PWM3_CH4_PB1
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_1;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOB, &gpio);
#else
            pwm_pin_gpio_clk_enable(GPIOC);
            gpio.Pin = GPIO_PIN_9;
            gpio.Alternate = GPIO_AF0;
            HAL_GPIO_Init(GPIOC, &gpio);
#endif
            break;
#endif
        default:
            break;
        }
        break;
    case TIM10_BASE_ADDR:
        switch (channel)
        {
#ifdef BSP_USING_PWM10_CH1
        case 1:
#ifdef BSP_PWM10_CH1_PF7
            pwm_pin_gpio_clk_enable(GPIOF);
            gpio.Pin = GPIO_PIN_7;
            gpio.Alternate = GPIO_AF0;
            HAL_GPIO_Init(GPIOF, &gpio);
#elif defined(BSP_PWM10_CH1_PA4)
            pwm_pin_gpio_clk_enable(GPIOA);
            gpio.Pin = GPIO_PIN_4;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOA, &gpio);
#elif defined(BSP_PWM10_CH1_PB8)
            pwm_pin_gpio_clk_enable(GPIOB);
            gpio.Pin = GPIO_PIN_8;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOB, &gpio);
#elif defined(BSP_PWM10_CH1_PE3)
            pwm_pin_gpio_clk_enable(GPIOE);
            gpio.Pin = GPIO_PIN_3;
            gpio.Alternate = GPIO_AF1;
            HAL_GPIO_Init(GPIOE, &gpio);
#endif
            break;
#endif
        default:
            break;
        }
        break;
    default:
        break;
    }
}

static rt_uint32_t pwm_timer_clock_get(TIM_TypeDef *instance)
{
    rt_uint32_t pclk;
    uint32_t base = (uint32_t)instance;

    switch (base)
    {
    case TIM1_BASE_ADDR:
    case TIM10_BASE_ADDR:
        pclk = HAL_RCC_GetPCLK2Freq();
        break;
    case TIM2_BASE_ADDR:
    case TIM3_BASE_ADDR:
    default:
        pclk = HAL_RCC_GetPCLK1Freq();
        break;
    }

    if (HAL_RCC_GetHCLKFreq() != pclk)
    {
        pclk <<= 1;
    }
    return pclk;
}

uint32_t HAL_TIMER_MSP_Init(TIM_HandleTypeDef *htim)
{
    uint32_t Timer_Instance;
    rt_uint32_t ch;

    RT_ASSERT(htim != RT_NULL);

    Timer_Instance = (uint32_t)(htim->Instance);

    switch (Timer_Instance)
    {
    case TIM1_BASE_ADDR:
        __HAL_RCC_TIM1_CLK_ENABLE();
        NVIC_ClearPendingIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
        NVIC_EnableIRQ(TIM1_BRK_UP_TRG_COM_IRQn);
        break;
    case TIM2_BASE_ADDR:
        __HAL_RCC_TIM2_CLK_ENABLE();
        NVIC_ClearPendingIRQ(TIM2_IRQn);
        NVIC_EnableIRQ(TIM2_IRQn);
        break;
    case TIM3_BASE_ADDR:
        __HAL_RCC_TIM3_CLK_ENABLE();
        NVIC_ClearPendingIRQ(TIM3_IRQn);
        NVIC_EnableIRQ(TIM3_IRQn);
        break;
    case TIM6_BASE_ADDR:
        __HAL_RCC_TIM6_CLK_ENABLE();
        NVIC_ClearPendingIRQ(TIM6_IRQn);
        NVIC_EnableIRQ(TIM6_IRQn);
        break;
    case TIM10_BASE_ADDR:
        __HAL_RCC_TIM10_CLK_ENABLE();
        NVIC_ClearPendingIRQ(TIM10_IRQn);
        NVIC_EnableIRQ(TIM10_IRQn);
        break;
    default:
        return HAL_ERROR;
    }

    for (ch = 1; ch <= 4; ch++)
    {
        pwm_config_gpio(htim->Instance, ch);
    }

    return HAL_OK;
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

    timer_clk = pwm_timer_clock_get(htim->Instance);
    timer_clk /= 1000000UL;

    period = (rt_uint64_t)cfg->period * timer_clk / 1000ULL;
    psc = period / PWM_MAX_PERIOD + 1;
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

    htim->Init.Prescaler         = psc - 1;
    htim->Init.Period            = period - 1;
    htim->Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim->Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim->Init.ARRPreLoadEn      = TIM_ARR_PRELOAD_ENABLE;
    htim->Init.RepetitionCounter = 0;

    HAL_TIMER_Base_Init(htim);

    {
        TIM_OC_InitTypeDef oc_cfg = {0};

        oc_cfg.OCMode       = OUTPUT_MODE_PWM1;
        oc_cfg.Pulse        = pulse - 1;
        oc_cfg.OCPolarity   = OUTPUT_POL_ACTIVE_HIGH;
        oc_cfg.OCNPolarity  = OUTPUT_POL_ACTIVE_HIGH;
        oc_cfg.OCFastMode   = OUTPUT_FAST_MODE_DISABLE;
        oc_cfg.OCIdleState  = OUTPUT_DISABLE_IDLE_STATE;
        oc_cfg.OCNIdleState = OUTPUT_DISABLE_IDLE_STATE;

        HAL_TIMER_Output_Config(htim->Instance, &oc_cfg, hal_ch);
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
        HAL_TIMER_OC_Stop(htim->Instance, hal_ch);
    }

    return RT_EOK;
}

static rt_err_t drv_pwm_get(TIM_HandleTypeDef *htim, struct rt_pwm_configuration *cfg)
{
    rt_uint64_t timer_clk;
    rt_uint32_t hal_ch;
    rt_uint32_t ccr_val;

    timer_clk = pwm_timer_clock_get(htim->Instance);
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

    RT_ASSERT(device != RT_NULL);

    htim = (TIM_HandleTypeDef *)device->parent.user_data;

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
