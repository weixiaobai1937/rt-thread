/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx DAC driver
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_DAC) && defined(BSP_USING_DAC)

#define DBG_TAG "drv.dac"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define DAC_NAME    "dac1"

struct acm32_dac
{
    DAC_HandleTypeDef    handle;
    struct rt_dac_device dac_device;
    rt_uint8_t           ch_enabled;
};

static struct acm32_dac acm32_dac_obj = {0};

static uint32_t acm32_dac_hal_channel(rt_uint32_t channel)
{
    if (channel == 1)
        return DAC_CHANNEL_1;
    if (channel == 2)
        return DAC_CHANNEL_2;
    return 0xFFFFFFFFU;
}

void HAL_DAC_MspInit(DAC_HandleTypeDef *hdac)
{
    GPIO_InitTypeDef gpio = {0};

    if (hdac->Instance != DAC1)
    {
        return;
    }

    /* HAL SDK DAC MspInit: PA4=OUT1, PA5=OUT2，按需配置避免冲突 */
    __HAL_RCC_DAC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    gpio.Pull = GPIO_NOPULL;
    gpio.Mode = GPIO_MODE_ANALOG_SWITCH_ON;
    gpio.Drive = GPIO_DRIVE_LEVEL7;

    if (acm32_dac_obj.ch_enabled & (1U << 1))
    {
        gpio.Pin = GPIO_PIN_4;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
    if (acm32_dac_obj.ch_enabled & (1U << 2))
    {
        gpio.Pin = GPIO_PIN_5;
        HAL_GPIO_Init(GPIOA, &gpio);
    }
}

static rt_err_t _dac_enabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    struct acm32_dac *dacObj;
    DAC_ChannelConfTypeDef sConfig = {0};
    uint32_t hal_ch;

    RT_ASSERT(device != RT_NULL);
    dacObj = rt_container_of(device, struct acm32_dac, dac_device);
    hal_ch = acm32_dac_hal_channel(channel);
    if (hal_ch == 0xFFFFFFFFU)
    {
        return -RT_EINVAL;
    }

    dacObj->ch_enabled |= (1U << channel);

    if (dacObj->handle.Instance == RT_NULL)
    {
        dacObj->handle.Instance = DAC1;
        if (HAL_DAC_Init(&dacObj->handle) != HAL_OK)
        {
            dacObj->ch_enabled &= ~(1U << channel);
            return -RT_ERROR;
        }
    }
    else
    {
        /* 重新调用 MspInit 配置新通道的 GPIO 引脚 */
        if (HAL_DAC_Init(&dacObj->handle) != HAL_OK)
        {
            dacObj->ch_enabled &= ~(1U << channel);
            return -RT_ERROR;
        }
    }

    /* Align with HAL SDK DAC_Config_OutPut_Voltage (DC output) */
    sConfig.DAC_DMADoubleDataMode = DISABLE;
    sConfig.DAC_SignedFormat = DISABLE;
    sConfig.DAC_SampleAndHold = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.u.DAC_Trigger = DAC_TRIGGER_NONE;
    sConfig.SawtoothStepTrigger = DAC_TRIGGER_SOFTWARE;
    sConfig.DAC_OutputBuffer = DAC_OUTPUTBUFFER_DISABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_EXTERNAL;
    sConfig.DAC_UserTrimming = DAC_TRIMMING_FACTORY;
    sConfig.DAC_TrimmingValue = 1;

    if (HAL_DAC_ConfigChannel(&dacObj->handle, &sConfig, hal_ch) != HAL_OK)
    {
        dacObj->ch_enabled &= ~(1U << channel);
        return -RT_ERROR;
    }
    if (HAL_DAC_Start(&dacObj->handle, hal_ch) != HAL_OK)
    {
        dacObj->ch_enabled &= ~(1U << channel);
        return -RT_ERROR;
    }
    return RT_EOK;
}

static rt_err_t _dac_disabled(struct rt_dac_device *device, rt_uint32_t channel)
{
    struct acm32_dac *dacObj;
    uint32_t hal_ch;

    RT_ASSERT(device != RT_NULL);
    dacObj = rt_container_of(device, struct acm32_dac, dac_device);
    hal_ch = acm32_dac_hal_channel(channel);
    if (hal_ch == 0xFFFFFFFFU)
    {
        return -RT_EINVAL;
    }

    /* 检查通道是否已启用 */
    if (0 == (dacObj->ch_enabled & (1U << channel)))
    {
        return RT_EOK;
    }

    HAL_DAC_Stop(&dacObj->handle, hal_ch);
    dacObj->ch_enabled &= ~(1U << channel);
    return RT_EOK;
}

static rt_err_t _dac_convert(struct rt_dac_device *device, rt_uint32_t channel, rt_uint32_t *value)
{
    struct acm32_dac *dacObj;
    uint32_t hal_ch;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(value != RT_NULL);

    dacObj = rt_container_of(device, struct acm32_dac, dac_device);
    hal_ch = acm32_dac_hal_channel(channel);
    if (hal_ch == 0xFFFFFFFFU)
    {
        return -RT_EINVAL;
    }

    if (0 == (dacObj->ch_enabled & (1U << channel)))
    {
        return -RT_ERROR;
    }

    if (HAL_DAC_SetValue(&dacObj->handle, hal_ch, DAC_ALIGN_12B_R, (*value) & 0xFFFU) != HAL_OK)
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_uint8_t _dac_get_resolution(struct rt_dac_device *device)
{
    RT_UNUSED(device);
    return 12;
}

static const struct rt_dac_ops acm_dac_ops =
{
    .disabled = _dac_disabled,
    .enabled = _dac_enabled,
    .convert = _dac_convert,
    .get_resolution = _dac_get_resolution,
};

static int rt_hw_dac_init(void)
{
    return rt_hw_dac_register(&acm32_dac_obj.dac_device,
                              DAC_NAME,
                              &acm_dac_ops,
                              RT_NULL);
}
INIT_DEVICE_EXPORT(rt_hw_dac_init);

#endif /* RT_USING_DAC && BSP_USING_DAC */
