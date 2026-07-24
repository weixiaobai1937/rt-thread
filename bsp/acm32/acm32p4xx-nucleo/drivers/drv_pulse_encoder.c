/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx pulse encoder driver
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_PULSE_ENCODER) && (defined(BSP_USING_PULSE_ENCODER2) || defined(BSP_USING_PULSE_ENCODER3))

#define DBG_TAG "drv.pulse_encoder"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

enum
{
#ifdef BSP_USING_PULSE_ENCODER2
    PULSE_ENCODER2_INDEX,
#endif
#ifdef BSP_USING_PULSE_ENCODER3
    PULSE_ENCODER3_INDEX,
#endif
};

struct acm32_pulse_encoder
{
    struct rt_pulse_encoder_device parent;
    TIM_HandleTypeDef              tim_handle;
    char                          *name;
};

static struct acm32_pulse_encoder acm32_pulse_encoder_obj[] =
{
#ifdef BSP_USING_PULSE_ENCODER2
    { .tim_handle.Instance = TIM2, .name = "pulse2" },
#endif
#ifdef BSP_USING_PULSE_ENCODER3
    { .tim_handle.Instance = TIM3, .name = "pulse3" },
#endif
};

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

static void acm32_pulse_encoder_gpio_init(struct acm32_pulse_encoder *dev)
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
        HAL_GPIO_Init(GPIOA, &gpio);
        gpio.Pin = GPIO_PIN_1;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
    else
    {
        gpio.Alternate = GPIO_FUNCTION_1;
        gpio.Pin = GPIO_PIN_6;
        HAL_GPIO_Init(GPIOA, &gpio);
        gpio.Pin = GPIO_PIN_7;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

static rt_err_t pulse_encoder_init(struct rt_pulse_encoder_device *pulse_encoder)
{
    struct acm32_pulse_encoder *dev;
    TIM_SlaveConfigTypeDef slave_cfg = {0};

    dev = (struct acm32_pulse_encoder *)pulse_encoder;

    acm32_pulse_encoder_gpio_init(dev);

    dev->tim_handle.Init.Prescaler = 0;
    dev->tim_handle.Init.CounterMode = TIM_COUNTERMODE_UP;
    dev->tim_handle.Init.Period = (dev->tim_handle.Instance == TIM2) ? 0xFFFFFFFF : 0xFFFF;
    dev->tim_handle.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    dev->tim_handle.Init.ARRPreLoadEn = TIM_ARR_PRELOAD_ENABLE;
    dev->tim_handle.Init.RepetitionCounter = 0;

    HAL_TIMER_MSP_Init(&dev->tim_handle);

    if (HAL_TIMER_Base_Init(&dev->tim_handle) != 0)
    {
        LOG_E("%s base init failed", dev->name);
        return -RT_ERROR;
    }

    slave_cfg.SlaveMode = TIM_SLAVE_MODE_ENC3;
    slave_cfg.InputTrigger = TIM_TS_ITR0;
    HAL_TIMER_Slave_Mode_Config(&dev->tim_handle, &slave_cfg);

    dev->tim_handle.Instance->CCMR1 = (dev->tim_handle.Instance->CCMR1 & ~((uint32_t)(BIT0 | BIT1 | BIT8 | BIT9))) | (BIT0 | BIT8);

    dev->tim_handle.Instance->CCER &= ~(BIT1 | BIT3 | BIT5 | BIT7);

    HAL_TIMER_Base_Start(dev->tim_handle.Instance);

    return RT_EOK;
}

static rt_int32_t pulse_encoder_get_count(struct rt_pulse_encoder_device *pulse_encoder)
{
    struct acm32_pulse_encoder *dev;
    dev = (struct acm32_pulse_encoder *)pulse_encoder;
    return (rt_int32_t)dev->tim_handle.Instance->CNT;
}

static rt_err_t pulse_encoder_clear_count(struct rt_pulse_encoder_device *pulse_encoder)
{
    struct acm32_pulse_encoder *dev;
    dev = (struct acm32_pulse_encoder *)pulse_encoder;
    dev->tim_handle.Instance->CNT = 0;
    return RT_EOK;
}

static rt_err_t pulse_encoder_control(struct rt_pulse_encoder_device *pulse_encoder, rt_uint32_t cmd, void *args)
{
    struct acm32_pulse_encoder *dev;
    dev = (struct acm32_pulse_encoder *)pulse_encoder;

    switch (cmd)
    {
    case PULSE_ENCODER_CMD_ENABLE:
        HAL_TIMER_Base_Start(dev->tim_handle.Instance);
        break;
    case PULSE_ENCODER_CMD_DISABLE:
        HAL_TIMER_Base_Stop(dev->tim_handle.Instance);
        break;
    case PULSE_ENCODER_CMD_GET_TYPE:
        *(enum rt_pulse_encoder_type *)args = AB_PHASE_PULSE_ENCODER;
        break;
    case PULSE_ENCODER_CMD_CLEAR_COUNT:
        dev->tim_handle.Instance->CNT = 0;
        break;
    default:
        return -RT_EINVAL;
    }
    return RT_EOK;
}

static const struct rt_pulse_encoder_ops _ops =
{
    .init = pulse_encoder_init,
    .get_count = pulse_encoder_get_count,
    .clear_count = pulse_encoder_clear_count,
    .control = pulse_encoder_control,
};

static int rt_hw_pulse_encoder_init(void)
{
    int i;
    int result = RT_EOK;

    for (i = 0; i < (int)(sizeof(acm32_pulse_encoder_obj) / sizeof(acm32_pulse_encoder_obj[0])); i++)
    {
        acm32_pulse_encoder_obj[i].parent.type = AB_PHASE_PULSE_ENCODER;
        acm32_pulse_encoder_obj[i].parent.ops = &_ops;
        if (rt_device_pulse_encoder_register(&acm32_pulse_encoder_obj[i].parent,
                                             acm32_pulse_encoder_obj[i].name,
                                             RT_NULL) != RT_EOK)
        {
            LOG_E("%s register failed", acm32_pulse_encoder_obj[i].name);
            result = -RT_ERROR;
        }
    }
    return result;
}
INIT_DEVICE_EXPORT(rt_hw_pulse_encoder_init);

#endif /* RT_USING_PULSE_ENCODER && BSP_USING_PULSE_ENCODERx */
