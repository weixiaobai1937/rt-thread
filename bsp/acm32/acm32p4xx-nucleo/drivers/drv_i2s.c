/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx-Nucleo I2S1 audio driver
 */

#include "board.h"

#if defined(RT_USING_AUDIO) && defined(BSP_USING_I2S1)

#include <rtdevice.h>
#include "hal_i2s.h"
#include "hal_dma.h"
#include "hal_gpio.h"
#include "hal_rcc.h"

#define DBG_TAG "drv.i2s"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define I2S_TX_DMA_BLK_SIZE    2048U
#define I2S_TX_DMA_BLK_COUNT   2U
#define I2S_TX_DMA_BUF_TOTAL   (I2S_TX_DMA_BLK_SIZE * I2S_TX_DMA_BLK_COUNT)

/* I2S1 pin definitions: all AF7, verified against GPIO AF table */
/* Per-signal pin selection via Kconfig */

/* WS */
#ifdef BSP_I2S1_WS_PA15
#define I2S1_WS_PORT     GPIOA
#define I2S1_WS_PIN      GPIO_PIN_15
#else
#define I2S1_WS_PORT     GPIOA
#define I2S1_WS_PIN      GPIO_PIN_4
#endif

/* CK */
#ifdef BSP_I2S1_CK_PC10
#define I2S1_CK_PORT     GPIOC
#define I2S1_CK_PIN      GPIO_PIN_10
#elif defined(BSP_I2S1_CK_PB3)
#define I2S1_CK_PORT     GPIOB
#define I2S1_CK_PIN      GPIO_PIN_3
#else
#define I2S1_CK_PORT     GPIOA
#define I2S1_CK_PIN      GPIO_PIN_5
#endif

/* SDI */
#ifdef BSP_I2S1_SDI_PC11
#define I2S1_SDI_PORT    GPIOC
#define I2S1_SDI_PIN     GPIO_PIN_11
#elif defined(BSP_I2S1_SDI_PB4)
#define I2S1_SDI_PORT    GPIOB
#define I2S1_SDI_PIN     GPIO_PIN_4
#else
#define I2S1_SDI_PORT    GPIOA
#define I2S1_SDI_PIN     GPIO_PIN_6
#endif

/* SDO */
#ifdef BSP_I2S1_SDO_PB5
#define I2S1_SDO_PORT    GPIOB
#define I2S1_SDO_PIN     GPIO_PIN_5
#elif defined(BSP_I2S1_SDO_PD7)
#define I2S1_SDO_PORT    GPIOD
#define I2S1_SDO_PIN     GPIO_PIN_7
#else
#define I2S1_SDO_PORT    GPIOC
#define I2S1_SDO_PIN     GPIO_PIN_12
#endif

/* MCK */
#ifdef BSP_I2S1_MCK_PC7
#define I2S1_MCK_PORT    GPIOC
#define I2S1_MCK_PIN     GPIO_PIN_7
#else
#define I2S1_MCK_PORT    GPIOC
#define I2S1_MCK_PIN     GPIO_PIN_4
#endif

struct acm32_i2s
{
    I2S_HandleTypeDef         handle;
    DMA_HandleTypeDef         dma_tx;
    struct rt_audio_device    audio_dev;
    struct rt_audio_configure config;
    rt_uint8_t                tx_buf[I2S_TX_DMA_BLK_COUNT][I2S_TX_DMA_BLK_SIZE];
    rt_uint8_t                tx_idx;       /* 当前 DMA 传输的缓冲区索引 */
    rt_uint8_t                tx_next_idx;  /* 下一个待填充的缓冲区索引 */
    rt_uint8_t                running;
    rt_uint8_t                hal_inited;   /* HAL + DMA 已初始化，下次 start 跳过 */
};

static struct acm32_i2s g_i2s_dev;

static void acm32_i2s_gpio_clk_enable(GPIO_TypeDef *port)
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
    else if (port == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
}

static rt_err_t acm32_i2s_getcaps(struct rt_audio_device *audio, struct rt_audio_caps *caps)
{
    struct acm32_i2s *i2s_dev;

    RT_ASSERT(audio != RT_NULL);
    RT_ASSERT(caps != RT_NULL);

    i2s_dev = rt_container_of(audio, struct acm32_i2s, audio_dev);

    switch (caps->main_type)
    {
    case AUDIO_TYPE_QUERY:
        switch (caps->sub_type)
        {
        case AUDIO_TYPE_QUERY:
            caps->udata.mask = AUDIO_TYPE_OUTPUT;
            break;
        default:
            return -RT_ERROR;
        }
        break;

    case AUDIO_TYPE_OUTPUT:
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            caps->udata.config.channels   = i2s_dev->config.channels;
            caps->udata.config.samplebits = i2s_dev->config.samplebits;
            caps->udata.config.samplerate = i2s_dev->config.samplerate;
            break;
        case AUDIO_DSP_SAMPLERATE:
            caps->udata.config.samplerate = i2s_dev->config.samplerate;
            break;
        case AUDIO_DSP_CHANNELS:
            caps->udata.config.channels = i2s_dev->config.channels;
            break;
        case AUDIO_DSP_SAMPLEBITS:
            caps->udata.config.samplebits = i2s_dev->config.samplebits;
            break;
        default:
            return -RT_ERROR;
        }
        break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t acm32_i2s_configure(struct rt_audio_device *audio, struct rt_audio_caps *caps)
{
    struct acm32_i2s *i2s_dev;

    RT_ASSERT(audio != RT_NULL);
    RT_ASSERT(caps != RT_NULL);

    i2s_dev = rt_container_of(audio, struct acm32_i2s, audio_dev);

    switch (caps->main_type)
    {
    case AUDIO_TYPE_OUTPUT:
        switch (caps->sub_type)
        {
        case AUDIO_DSP_PARAM:
            rt_memcpy(&i2s_dev->config, &caps->udata.config,
                      sizeof(struct rt_audio_configure));
            break;
        case AUDIO_DSP_SAMPLERATE:
            i2s_dev->config.samplerate = caps->udata.config.samplerate;
            break;
        case AUDIO_DSP_CHANNELS:
            i2s_dev->config.channels = caps->udata.config.channels;
            break;
        case AUDIO_DSP_SAMPLEBITS:
            i2s_dev->config.samplebits = caps->udata.config.samplebits;
            break;
        default:
            return -RT_ERROR;
        }
        break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t acm32_i2s_init(struct rt_audio_device *audio)
{
    return RT_EOK;
}

static rt_err_t acm32_i2s_start(struct rt_audio_device *audio, int stream)
{
    struct acm32_i2s *i2s_dev;
    I2S_HandleTypeDef *hi2s;

    RT_ASSERT(audio != RT_NULL);
    i2s_dev = rt_container_of(audio, struct acm32_i2s, audio_dev);

    if (stream != AUDIO_STREAM_REPLAY)
        return -RT_EINVAL;

    hi2s = &i2s_dev->handle;

    hi2s->Instance           = I2S1;
    hi2s->Init.Mode          = I2S_MODE_MASTER;
    hi2s->Init.Standard      = I2S_STANDARD_PHILIPS;
    hi2s->Init.DataFormat    = I2S_DATA_FORMAT_16B;
    hi2s->Init.MCLKOutput    = I2S_MCLKOUT_DISABLE;
    hi2s->Init.ClockPolarity = I2S_CLOCK_POLARITY_LOW;
    hi2s->Init.IOSwitch      = I2S_IO_SWITCH_DISABLE;
    hi2s->Init.AudioFreq     = (uint32_t)i2s_dev->config.samplerate;

    if (!i2s_dev->hal_inited)
    {
        if (HAL_I2S_Init(hi2s) != HAL_OK)
        {
            LOG_E("HAL_I2S_Init failed");
            return -RT_ERROR;
        }

        __HAL_RCC_DMA1_CLK_ENABLE();

        hi2s->hdmatx                           = &i2s_dev->dma_tx;
        i2s_dev->dma_tx.Instance               = DMA1_Channel0;
        i2s_dev->dma_tx.Channel                = 0U;
        i2s_dev->dma_tx.DMA                    = DMA1;
        i2s_dev->dma_tx.Parent                 = (void *)hi2s;
        i2s_dev->dma_tx.Init.Mode              = DMA_MODE_NORMAL;
        i2s_dev->dma_tx.Init.DataFlow          = DMA_DATAFLOW_M2P;
        i2s_dev->dma_tx.Init.ReqID             = DMA1_REQ_I2S1_TX;
        i2s_dev->dma_tx.Init.SrcIncDec         = DMA_SRCINCDEC_INC;
        i2s_dev->dma_tx.Init.DestIncDec        = DMA_DESTINCDEC_DISABLE;
        i2s_dev->dma_tx.Init.SrcWidth          = DMA_SRCWIDTH_WORD;
        i2s_dev->dma_tx.Init.DestWidth         = DMA_DESTWIDTH_WORD;
        i2s_dev->dma_tx.Init.SrcBurst          = DMA_SRCBURST_1;
        i2s_dev->dma_tx.Init.DestBurst         = DMA_DESTBURST_1;
        i2s_dev->dma_tx.Init.SrcMaster         = DMA_SRCMASTER_1;
        i2s_dev->dma_tx.Init.DestMaster        = DMA_DESTMASTER_1;
        i2s_dev->dma_tx.Init.Lock              = 0U;
        i2s_dev->dma_tx.Init.NextMaster        = 0U;

        if (HAL_DMA_Init(&i2s_dev->dma_tx) != HAL_OK)
        {
            LOG_E("DMA_Init failed");
            return -RT_ERROR;
        }

        NVIC_SetPriority(DMA1_CH0_IRQn, 2);
        NVIC_EnableIRQ(DMA1_CH0_IRQn);

        i2s_dev->hal_inited = 1;
    }

    i2s_dev->tx_idx      = 0;
    i2s_dev->tx_next_idx = 0;
    i2s_dev->running     = 1;

    rt_memset(i2s_dev->tx_buf[0], 0, I2S_TX_DMA_BLK_SIZE);
    HAL_I2S_Transmit_DMA(hi2s, (const uint32_t *)i2s_dev->tx_buf[0],
                         (uint16_t)(I2S_TX_DMA_BLK_SIZE / sizeof(uint32_t)));

    LOG_I("I2S playback started, rate=%d", (int)i2s_dev->config.samplerate);
    return RT_EOK;
}

static rt_err_t acm32_i2s_stop(struct rt_audio_device *audio, int stream)
{
    struct acm32_i2s *i2s_dev;

    RT_ASSERT(audio != RT_NULL);
    i2s_dev = rt_container_of(audio, struct acm32_i2s, audio_dev);

    if (stream != AUDIO_STREAM_REPLAY)
        return -RT_EINVAL;

    if (i2s_dev->running)
    {
        HAL_I2S_DMAStop(&i2s_dev->handle);
        i2s_dev->running = 0;
        rt_memset(i2s_dev->tx_buf, 0, I2S_TX_DMA_BUF_TOTAL);
        i2s_dev->tx_idx = 0;
        i2s_dev->tx_next_idx = 0;
        LOG_I("I2S playback stopped");
    }

    return RT_EOK;
}

static rt_ssize_t acm32_i2s_transmit(struct rt_audio_device *audio,
                                      const void *writeBuf, void *readBuf,
                                      rt_size_t size)
{
    struct acm32_i2s *i2s_dev;

    RT_ASSERT(audio != RT_NULL);

    i2s_dev = rt_container_of(audio, struct acm32_i2s, audio_dev);

    if (writeBuf == RT_NULL || size == 0)
        return 0;

    if (size > I2S_TX_DMA_BLK_SIZE)
        size = I2S_TX_DMA_BLK_SIZE;

    /* 检查下一个缓冲区是否正被 DMA 使用（双缓冲竞态保护） */
    if (i2s_dev->tx_next_idx == i2s_dev->tx_idx)
    {
        /* 缓冲区已满，DMA 尚未完成当前传输，无法写入 */
        return 0;
    }

    /* 写入下一个可用缓冲区，避免覆盖正在传输的缓冲区 */
    rt_memcpy(i2s_dev->tx_buf[i2s_dev->tx_next_idx], writeBuf, size);

    /* 更新下一个缓冲区索引 */
    i2s_dev->tx_next_idx = (i2s_dev->tx_next_idx + 1) % I2S_TX_DMA_BLK_COUNT;

    return (rt_ssize_t)size;
}

static void acm32_i2s_buffer_info(struct rt_audio_device *audio,
                                   struct rt_audio_buf_info *info)
{
    RT_ASSERT(audio != RT_NULL);
    RT_ASSERT(info != RT_NULL);

    info->buffer      = (rt_uint8_t *)g_i2s_dev.tx_buf;
    info->total_size  = I2S_TX_DMA_BUF_TOTAL;
    info->block_size  = (rt_uint16_t)I2S_TX_DMA_BLK_SIZE;
    info->block_count = (rt_uint16_t)I2S_TX_DMA_BLK_COUNT;
}

static struct rt_audio_ops acm32_i2s_audio_ops =
{
    .getcaps     = acm32_i2s_getcaps,
    .configure   = acm32_i2s_configure,
    .init        = acm32_i2s_init,
    .start       = acm32_i2s_start,
    .stop        = acm32_i2s_stop,
    .transmit    = acm32_i2s_transmit,
    .buffer_info = acm32_i2s_buffer_info,
};

void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    GPIO_InitTypeDef gpio = {0};

    if (hi2s->Instance != I2S1)
        return;

    __HAL_RCC_I2S1_CLK_ENABLE();

    acm32_i2s_gpio_clk_enable(I2S1_WS_PORT);
    acm32_i2s_gpio_clk_enable(I2S1_CK_PORT);
    acm32_i2s_gpio_clk_enable(I2S1_SDI_PORT);
    acm32_i2s_gpio_clk_enable(I2S1_SDO_PORT);
    acm32_i2s_gpio_clk_enable(I2S1_MCK_PORT);

    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Drive     = GPIO_DRIVE_LEVEL7;
    gpio.Alternate = GPIO_FUNCTION_7;

    gpio.Pin = I2S1_WS_PIN;
    HAL_GPIO_Init(I2S1_WS_PORT, &gpio);

    gpio.Pin = I2S1_CK_PIN;
    HAL_GPIO_Init(I2S1_CK_PORT, &gpio);

    gpio.Pin = I2S1_SDI_PIN;
    HAL_GPIO_Init(I2S1_SDI_PORT, &gpio);

    gpio.Pin = I2S1_SDO_PIN;
    HAL_GPIO_Init(I2S1_SDO_PORT, &gpio);

    gpio.Pin = I2S1_MCK_PIN;
    HAL_GPIO_Init(I2S1_MCK_PORT, &gpio);
}

void HAL_I2S_DMATxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    struct acm32_i2s *i2s_dev = &g_i2s_dev;

    if (!i2s_dev->running)
        return;

    rt_audio_tx_complete(&i2s_dev->audio_dev);

    i2s_dev->tx_idx = (i2s_dev->tx_idx + 1) % I2S_TX_DMA_BLK_COUNT;

    HAL_I2S_Transmit_DMA(hi2s,
                         (const uint32_t *)i2s_dev->tx_buf[i2s_dev->tx_idx],
                         (uint16_t)(I2S_TX_DMA_BLK_SIZE / sizeof(uint32_t)));
}

void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    struct acm32_i2s *i2s_dev = &g_i2s_dev;

    LOG_E("I2S error: 0x%08X", (unsigned int)hi2s->ErrorCode);

    HAL_I2S_DMAStop(hi2s);
    i2s_dev->running = 0;
}

void DMA1_Channel0_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_DMA_IRQHandler(&g_i2s_dev.dma_tx);
    rt_interrupt_leave();
}

int rt_hw_i2s_init(void)
{
    g_i2s_dev.config.samplerate = 48000;
    g_i2s_dev.config.channels   = 2;
    g_i2s_dev.config.samplebits = 16;
    g_i2s_dev.tx_idx            = 0;
    g_i2s_dev.running           = 0;

    g_i2s_dev.audio_dev.ops = &acm32_i2s_audio_ops;

    rt_audio_register(&g_i2s_dev.audio_dev, "sound0", RT_DEVICE_FLAG_WRONLY, &g_i2s_dev);

    LOG_I("I2S1 audio device registered");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_i2s_init);

#endif /* RT_USING_AUDIO && BSP_USING_I2S1 */
