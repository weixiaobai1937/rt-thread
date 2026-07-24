/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx input capture driver
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_INPUT_CAPTURE) && (defined(BSP_USING_CAPTURE2) || defined(BSP_USING_CAPTURE3))

#define DBG_TAG "drv.inputcapture"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

enum
{
#ifdef BSP_USING_CAPTURE2
    CAPTURE2_INDEX,
#endif
#ifdef BSP_USING_CAPTURE3
    CAPTURE3_INDEX,
#endif
};

struct acm32_inputcapture
{
    struct rt_inputcapture_device parent;
    TIM_HandleTypeDef             tim_handle;
    char                         *name;
    rt_uint32_t                   pulsewidth_us;
    rt_uint32_t                   tim_clock_hz;
};

static struct acm32_inputcapture acm32_inputcapture_obj[] =
{
#ifdef BSP_USING_CAPTURE2
    { .tim_handle.Instance = TIM2, .name = "capture2" },
#endif
#ifdef BSP_USING_CAPTURE3
    { .tim_handle.Instance = TIM3, .name = "capture3" },
#endif
};

static rt_uint32_t acm32_timer_clock_get(TIM_TypeDef *instance)
{
    rt_uint32_t pclk;
    uint32_t base = (uint32_t)instance;

    switch (base)
    {
    case TIM1_BASE_ADDR:
        pclk = HAL_RCC_GetPCLK2Freq();
        break;
    case TIM2_BASE_ADDR:
    case TIM3_BASE_ADDR:
    default:
        pclk = HAL_RCC_GetPCLK1Freq();
        break;
    }

    if (HAL_RCC_GetHCLKFreq() != pclk)
        pclk <<= 1;
    return pclk;
}

static void acm32_gpio_clk_enable(GPIO_TypeDef *port)
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
}

static void acm32_capture_gpio_init(struct acm32_inputcapture *dev)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Drive = GPIO_DRIVE_LEVEL3;

    acm32_gpio_clk_enable(GPIOA);

    if (dev->tim_handle.Instance == TIM2)
    {
        gpio.Alternate = GPIO_FUNCTION_2;
        gpio.Pin = GPIO_PIN_0;
    }
    else
    {
        gpio.Alternate = GPIO_FUNCTION_1;
        gpio.Pin = GPIO_PIN_6;
    }

    HAL_GPIO_Init(GPIOA, &gpio);
}

static rt_err_t capture_init(struct rt_inputcapture_device *inputcapture)
{
    struct acm32_inputcapture *dev;
    dev = (struct acm32_inputcapture *)inputcapture;

    acm32_capture_gpio_init(dev);

    dev->tim_clock_hz = acm32_timer_clock_get(dev->tim_handle.Instance);

    dev->tim_handle.Init.Prescaler = (dev->tim_clock_hz / 1000000) - 1;
    dev->tim_handle.Init.Period = (dev->tim_handle.Instance == TIM2) ? 0xFFFFFFFF : 0xFFFF;
    dev->tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    dev->tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    dev->tim_handle.Init.ARRPreLoadEn = TIM_ARR_PRELOAD_ENABLE;
    dev->tim_handle.Init.RepetitionCounter = 0;

    HAL_TIMER_MSP_Init(&dev->tim_handle);

    if (HAL_TIMER_Base_Init(&dev->tim_handle) != 0)
    {
        LOG_E("%s base init failed", dev->name);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t capture_open(struct rt_inputcapture_device *inputcapture)
{
    struct acm32_inputcapture *dev;
    TIM_IC_InitTypeDef ic_cfg = {0};

    dev = (struct acm32_inputcapture *)inputcapture;

    ic_cfg.ICPolarity = TIM_SLAVE_CAPTURE_ACTIVE_RISING_FALLING;
    ic_cfg.ICSelection = TIM_ICSELECTION_DIRECTTI;
    ic_cfg.ICPrescaler = TIM_IC1_PRESCALER_1;
    ic_cfg.TIFilter = 0;

    HAL_TIMER_Capture_Config(dev->tim_handle.Instance, &ic_cfg, TIM_CHANNEL_1);

    HAL_TIM_ENABLE_IT(&dev->tim_handle, TIM_IT_CC1);

    HAL_TIM_Capture_Start(dev->tim_handle.Instance, TIM_CHANNEL_1);
    HAL_TIMER_Base_Start(dev->tim_handle.Instance);

    return RT_EOK;
}

static rt_err_t capture_close(struct rt_inputcapture_device *inputcapture)
{
    struct acm32_inputcapture *dev;
    dev = (struct acm32_inputcapture *)inputcapture;

    HAL_TIM_DISABLE_IT(&dev->tim_handle, TIM_IT_CC1);
    HAL_TIM_Capture_Stop(dev->tim_handle.Instance, TIM_CHANNEL_1);
    HAL_TIMER_Base_Stop(dev->tim_handle.Instance);

    return RT_EOK;
}

static rt_err_t capture_get_pulsewidth(struct rt_inputcapture_device *inputcapture, rt_uint32_t *pulsewidth_us)
{
    struct acm32_inputcapture *dev;
    dev = (struct acm32_inputcapture *)inputcapture;

    *pulsewidth_us = dev->pulsewidth_us;
    return RT_EOK;
}

static const struct rt_inputcapture_ops _ops =
{
    .init = capture_init,
    .open = capture_open,
    .close = capture_close,
    .get_pulsewidth = capture_get_pulsewidth,
};

void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim)
{
    int i;
    struct acm32_inputcapture *dev = RT_NULL;
    rt_uint32_t captured;
    rt_bool_t level;

    for (i = 0; i < (int)(sizeof(acm32_inputcapture_obj) / sizeof(acm32_inputcapture_obj[0])); i++)
    {
        if (htim == &acm32_inputcapture_obj[i].tim_handle)
        {
            dev = &acm32_inputcapture_obj[i];
            break;
        }
    }
    if (dev == RT_NULL)
        return;

    captured = HAL_TIMER_ReadCapturedValue(htim, TIM_CHANNEL_1);

    /*
     * 使用定时器 CCER 寄存器的 CC1P 位检测捕获极性:
     * CC1P=0 表示上升沿捕获 (当前电平为高)
     * CC1P=1 表示下降沿捕获 (当前电平为低)
     * 这比直接读 GPIO 更可靠，避免高频下的时序问题。
     */
    level = (htim->Instance->CCER & TIM_CCER_CC1P) ? RT_FALSE : RT_TRUE;

    dev->pulsewidth_us = captured;

    rt_hw_inputcapture_isr(&dev->parent, level);
}

#if defined(BSP_USING_CAPTURE2) && !defined(BSP_USING_TIM2) && !defined(BSP_USING_PWM2)
void TIM2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_TIM_IRQHandler(&acm32_inputcapture_obj[CAPTURE2_INDEX].tim_handle);
    rt_interrupt_leave();
}
#endif

#if defined(BSP_USING_CAPTURE3) && !defined(BSP_USING_TIM3) && !defined(BSP_USING_PWM3)
void TIM3_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_TIM_IRQHandler(&acm32_inputcapture_obj[CAPTURE3_INDEX].tim_handle);
    rt_interrupt_leave();
}
#endif

static int rt_hw_inputcapture_init(void)
{
    int i;
    int result = RT_EOK;

    for (i = 0; i < (int)(sizeof(acm32_inputcapture_obj) / sizeof(acm32_inputcapture_obj[0])); i++)
    {
        if (rt_device_inputcapture_register(&acm32_inputcapture_obj[i].parent,
                                            acm32_inputcapture_obj[i].name,
                                            RT_NULL) != RT_EOK)
        {
            LOG_E("%s register failed", acm32_inputcapture_obj[i].name);
            result = -RT_ERROR;
        }
    }
    return result;
}
INIT_DEVICE_EXPORT(rt_hw_inputcapture_init);

#endif /* RT_USING_INPUT_CAPTURE && BSP_USING_CAPTUREx */
