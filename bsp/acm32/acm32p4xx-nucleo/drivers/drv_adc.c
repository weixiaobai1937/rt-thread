/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx ADC driver
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_ADC) && defined(BSP_USING_ADC)

#define DBG_TAG "drv.adc"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define ADC_NAME            "adc1"
#define ADC_MAX_CHANNEL     19

/* Manual table 32-4: external channel -> GPIO (index = channel) */
struct adc_gpio_map
{
    GPIO_TypeDef *port;
    uint32_t      pin;
};

static const struct adc_gpio_map adc_ch_gpio[ADC_MAX_CHANNEL + 1] =
{
    {RT_NULL, 0},            /* 0  cal */
    {GPIOA, GPIO_PIN_5},     /* 1  PA5 */
    {GPIOC, GPIO_PIN_2},     /* 2  PC2 */
    {GPIOA, GPIO_PIN_4},     /* 3  PA4 */
    {GPIOC, GPIO_PIN_4},     /* 4  PC4 */
    {GPIOB, GPIO_PIN_1},     /* 5  PB1 */
    {GPIOC, GPIO_PIN_3},     /* 6  PC3 */
    {GPIOA, GPIO_PIN_7},     /* 7  PA7 */
    {GPIOC, GPIO_PIN_5},     /* 8  PC5 */
    {GPIOA, GPIO_PIN_6},     /* 9  PA6 */
    {GPIOC, GPIO_PIN_0},     /* 10 PC0 */
    {GPIOC, GPIO_PIN_1},     /* 11 PC1 */
    {RT_NULL, 0},            /* 12 VREFINT */
    {RT_NULL, 0},            /* 13 VBAT */
    {GPIOA, GPIO_PIN_2},     /* 14 PA2 */
    {RT_NULL, 0},            /* 15 TS */
    {GPIOA, GPIO_PIN_0},     /* 16 PA0 */
    {GPIOA, GPIO_PIN_1},     /* 17 PA1 */
    {GPIOB, GPIO_PIN_0},     /* 18 PB0 */
    {GPIOA, GPIO_PIN_3},     /* 19 PA3 */
};

struct acm32_adc
{
    ADC_HandleTypeDef    handle;
    struct rt_adc_device acm32_adc_device;
};

static struct acm32_adc acm32_adc_obj = {0};

static void acm32_adc_gpio_clk_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
}

static void acm32_adc_config_pin(rt_int8_t channel)
{
    GPIO_InitTypeDef gpio = {0};

    if ((channel < 0) || (channel > ADC_MAX_CHANNEL))
    {
        return;
    }
    if (adc_ch_gpio[channel].port == RT_NULL)
    {
        return;
    }

    acm32_adc_gpio_clk_enable(adc_ch_gpio[channel].port);
    gpio.Pin = adc_ch_gpio[channel].pin;
    /* HAL SDK: CH1(PA5)/CH3(PA4) share DAC pins -> analog switch OFF */
    if ((channel == (rt_int8_t)ADC_CHANNEL_1) || (channel == (rt_int8_t)ADC_CHANNEL_3))
    {
        gpio.Mode = GPIO_MODE_ANALOG_SWITCH_OFF;
    }
    else
    {
        gpio.Mode = GPIO_MODE_ANALOG_SWITCH_ON;
    }
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(adc_ch_gpio[channel].port, &gpio);
}

static rt_int8_t acm32_adc_map_channel(rt_int8_t channel)
{
    if (channel == RT_ADC_INTERN_CH_VREF)
        return (rt_int8_t)ADC_CHANNEL_VREF;
    if (channel == RT_ADC_INTERN_CH_VBAT)
        return (rt_int8_t)ADC_CHANNEL_VBAT;
    if (channel == RT_ADC_INTERN_CH_TEMPER)
        return (rt_int8_t)ADC_CHANNEL_TS;
    return channel;
}

static void acm32_adc_fill_default_init(ADC_HandleTypeDef *hadc)
{
    rt_memset(&hadc->Init, 0, sizeof(hadc->Init));
    hadc->Instance = ADC;
    /* Defaults aligned with HAL SDK adc_regular.c */
    hadc->Init.ClockSource = ADC_CLOCKSOURCE_HCLK;
    hadc->Init.ClockPrescaler = 16;
    hadc->Init.Resolution = ADC_RESOLUTION_12B;
    hadc->Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc->Init.ConConvMode = DISABLE;
    hadc->Init.DiscontinuousConvMode = DISABLE;
    hadc->Init.NbrOfDiscConversion = 0;
    hadc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    hadc->Init.ExternalTrigConvEdge = ADC_EXTERNAL_TRIGGER_NONE;
    hadc->Init.DMAMode = DISABLE;
    hadc->Init.OverMode = DISABLE;
    hadc->Init.OverSampMode = DISABLE;
    hadc->Init.Oversampling.Ratio = ADC_OVERSAMPLING_RATIO_2;
    hadc->Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_NONE;
    hadc->Init.Oversampling.TriggeredMode = ADC_OVERSAMPLING_TRIGER_MORE;
    hadc->Init.AnalogWDGEn = DISABLE;
    hadc->ChannelNum = 0;
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc)
{
    rt_int8_t ch;

    RT_ASSERT(hadc != RT_NULL);

    /* Configure GPIO for every enabled external channel (SDK MspInit style) */
    for (ch = 0; ch <= ADC_MAX_CHANNEL; ch++)
    {
        if (hadc->Init.ChannelEn & (1U << ch))
        {
            acm32_adc_config_pin(ch);
        }
    }

    __HAL_RCC_ADC_CLK_ENABLE();
    NVIC_ClearPendingIRQ(ADC_IRQn);
    NVIC_EnableIRQ(ADC_IRQn);
}

static rt_err_t _adc_enabled(struct rt_adc_device *device, rt_int8_t channel, rt_bool_t enabled)
{
    struct acm32_adc *adcObj;
    rt_int8_t ch;

    RT_ASSERT(device != RT_NULL);
    adcObj = rt_container_of(device, struct acm32_adc, acm32_adc_device);
    ch = acm32_adc_map_channel(channel);

    if ((ch < 0) || (ch > ADC_MAX_CHANNEL))
    {
        return -RT_EINVAL;
    }

    if (enabled)
    {
        if (adcObj->handle.Init.ChannelEn & (1U << ch))
        {
            return RT_EOK;
        }

        if (adcObj->handle.Instance == RT_NULL)
        {
            acm32_adc_fill_default_init(&adcObj->handle);
        }

        adcObj->handle.Init.ChannelEn |= (1U << ch);
        /* GPIO + clock in HAL_ADC_MspInit (called by HAL_ADC_Init) */
        if (HAL_ADC_Init(&adcObj->handle) != HAL_OK)
        {
            return -RT_ERROR;
        }
        /* 使用临界区保护 ChannelNum 的原子操作 */
        {
            rt_base_t level = rt_hw_interrupt_disable();
            adcObj->handle.ChannelNum++;
            rt_hw_interrupt_enable(level);
        }
    }
    else
    {
        if (0 == (adcObj->handle.Init.ChannelEn & (1U << ch)))
        {
            return RT_EOK;
        }
        adcObj->handle.Init.ChannelEn &= ~(1U << ch);
        /* 使用临界区保护 ChannelNum 的原子操作 */
        {
            rt_base_t level = rt_hw_interrupt_disable();
            if (adcObj->handle.ChannelNum > 0)
            {
                adcObj->handle.ChannelNum--;
            }
            rt_hw_interrupt_enable(level);
        }
    }

    return RT_EOK;
}

static rt_err_t _get_adc_value(struct rt_adc_device *device, rt_int8_t channel, rt_uint32_t *value)
{
    struct acm32_adc *adcObj;
    ADC_ChannelConfTypeDef channelConf = {0};
    rt_int8_t ch;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value != RT_NULL);

    ch = acm32_adc_map_channel(channel);
    if ((ch < 0) || (ch > ADC_MAX_CHANNEL))
    {
        return -RT_EINVAL;
    }

    *value = 0;
    adcObj = rt_container_of(device, struct acm32_adc, acm32_adc_device);

    if (0 == (adcObj->handle.Init.ChannelEn & (1U << ch)))
    {
        return -RT_ERROR;
    }

    channelConf.Channel = (uint32_t)ch;
    channelConf.Sq = ADC_SEQUENCE_SQ1;
    channelConf.Smp = ADC_SMP_CLOCK_320;
    channelConf.Diff = DISABLE;
#ifdef ADC_OFR_NONE
    channelConf.OffsetNumber = ADC_OFR_NONE;
#else
    channelConf.OffsetNumber = 0;
#endif
    channelConf.Offset = 0;
    channelConf.OffsetCalculate = 0;
    channelConf.Offsetsign = 0;

    if (HAL_ADC_ConfigChannel(&adcObj->handle, &channelConf) != HAL_OK)
    {
        return -RT_ERROR;
    }

    if (HAL_ADC_Polling(&adcObj->handle, value, 1, 100) != HAL_OK)
    {
        return -RT_ERROR;
    }

    *value &= 0xFFFU;
    return RT_EOK;
}

static rt_uint8_t _adc_get_resolution(struct rt_adc_device *device)
{
    RT_UNUSED(device);
    return 12;
}

static rt_int16_t _adc_get_vref(struct rt_adc_device *device)
{
    RT_UNUSED(device);
    return 3300;
}

static const struct rt_adc_ops acm_adc_ops =
{
    .enabled = _adc_enabled,
    .convert = _get_adc_value,
    .get_resolution = _adc_get_resolution,
    .get_vref = _adc_get_vref,
};

static int rt_hw_adc_init(void)
{
    acm32_adc_fill_default_init(&acm32_adc_obj.handle);
    return rt_hw_adc_register(&acm32_adc_obj.acm32_adc_device,
                              ADC_NAME,
                              &acm_adc_ops,
                              RT_NULL);
}
INIT_BOARD_EXPORT(rt_hw_adc_init);

#endif /* RT_USING_ADC && BSP_USING_ADC */
