/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-27     AisinoChip   提取共享定时器时钟获取函数
 * 2026-08-01     AisinoChip   move common TIM MspInit here (PWM independent)
 */

#include "board.h"
#include <rtthread.h>
#include "drv_tim_utils.h"

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

static void pwm_init_pin(GPIO_TypeDef *port, uint16_t pin, uint8_t af)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Mode     = GPIO_MODE_AF_PP;
    gpio.Pull     = GPIO_NOPULL;
    gpio.Drive    = GPIO_DRIVE_LEVEL3;
    gpio.Pin      = pin;
    gpio.Alternate = af;
    pwm_pin_gpio_clk_enable(port);
    HAL_GPIO_Init(port, &gpio);
}

static void pwm_config_gpio(TIM_TypeDef *instance, rt_uint32_t channel)
{
    uint32_t base = (uint32_t)instance;

    switch (base)
    {
    case TIM1_BASE_ADDR:
        switch (channel)
        {
#ifdef BSP_USING_PWM1_CH1
        case 1:
#ifdef BSP_PWM1_CH1_PE9
            pwm_init_pin(GPIOE, GPIO_PIN_9,  GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOA, GPIO_PIN_8,  GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH2
        case 2:
#ifdef BSP_PWM1_CH2_PE11
            pwm_init_pin(GPIOE, GPIO_PIN_11, GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOA, GPIO_PIN_9,  GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH3
        case 3:
#ifdef BSP_PWM1_CH3_PE13
            pwm_init_pin(GPIOE, GPIO_PIN_13, GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOA, GPIO_PIN_10, GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM1_CH4
        case 4:
#ifdef BSP_PWM1_CH4_PE14
            pwm_init_pin(GPIOE, GPIO_PIN_14, GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOA, GPIO_PIN_11, GPIO_FUNCTION_2);
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
            pwm_init_pin(GPIOA, GPIO_PIN_0,  GPIO_FUNCTION_2);
#else
            pwm_init_pin(GPIOA, GPIO_PIN_5,  GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH2
        case 2:
#ifdef BSP_PWM2_CH2_PA1
            pwm_init_pin(GPIOA, GPIO_PIN_1,  GPIO_FUNCTION_2);
#else
            pwm_init_pin(GPIOB, GPIO_PIN_3,  GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH3
        case 3:
#ifdef BSP_PWM2_CH3_PA2
            pwm_init_pin(GPIOA, GPIO_PIN_2,  GPIO_FUNCTION_2);
#else
            pwm_init_pin(GPIOB, GPIO_PIN_10, GPIO_FUNCTION_2);
#endif
            break;
#endif
#ifdef BSP_USING_PWM2_CH4
        case 4:
#ifdef BSP_PWM2_CH4_PA3
            pwm_init_pin(GPIOA, GPIO_PIN_3,  GPIO_FUNCTION_2);
#else
            pwm_init_pin(GPIOB, GPIO_PIN_11, GPIO_FUNCTION_2);
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
            pwm_init_pin(GPIOA, GPIO_PIN_6,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM3_CH1_PB4)
            pwm_init_pin(GPIOB, GPIO_PIN_4,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM3_CH1_PC6)
            pwm_init_pin(GPIOC, GPIO_PIN_6,  GPIO_FUNCTION_0);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH2
        case 2:
#ifdef BSP_PWM3_CH2_PA7
            pwm_init_pin(GPIOA, GPIO_PIN_7,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM3_CH2_PB5)
            pwm_init_pin(GPIOB, GPIO_PIN_5,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM3_CH2_PC7)
            pwm_init_pin(GPIOC, GPIO_PIN_7,  GPIO_FUNCTION_0);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH3
        case 3:
#ifdef BSP_PWM3_CH3_PB0
            pwm_init_pin(GPIOB, GPIO_PIN_0,  GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOC, GPIO_PIN_8,  GPIO_FUNCTION_0);
#endif
            break;
#endif
#ifdef BSP_USING_PWM3_CH4
        case 4:
#ifdef BSP_PWM3_CH4_PB1
            pwm_init_pin(GPIOB, GPIO_PIN_1,  GPIO_FUNCTION_1);
#else
            pwm_init_pin(GPIOC, GPIO_PIN_9,  GPIO_FUNCTION_0);
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
            pwm_init_pin(GPIOF, GPIO_PIN_7,  GPIO_FUNCTION_0);
#elif defined(BSP_PWM10_CH1_PA4)
            pwm_init_pin(GPIOA, GPIO_PIN_4,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM10_CH1_PB8)
            pwm_init_pin(GPIOB, GPIO_PIN_8,  GPIO_FUNCTION_1);
#elif defined(BSP_PWM10_CH1_PE3)
            pwm_init_pin(GPIOE, GPIO_PIN_3,  GPIO_FUNCTION_1);
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
