/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx HWRNG driver
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_HWCRYPTO) && defined(BSP_USING_HWCRYPTO)

#define DBG_TAG "drv.crypto"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static rt_uint32_t _rng_update(struct hwcrypto_rng *ctx)
{
    RT_UNUSED(ctx);
    return HAL_HRNG_GetHrng_32();
}

static const struct hwcrypto_rng_ops rng_ops =
{
    .update = _rng_update,
};

static rt_err_t _crypto_create(struct rt_hwcrypto_ctx *ctx)
{
    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK)
    {
    case HWCRYPTO_TYPE_RNG:
        HAL_HRNG_Init();
        ctx->contex = RT_NULL;
        ((struct hwcrypto_rng *)ctx)->ops = &rng_ops;
        LOG_D("HRNG created");
        break;

    default:
        LOG_E("unsupported crypto type: %08x", ctx->type);
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void _crypto_destroy(struct rt_hwcrypto_ctx *ctx)
{
    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK)
    {
    case HWCRYPTO_TYPE_RNG:
        HAL_HRNG_DeInit();
        LOG_D("HRNG destroyed");
        break;

    default:
        break;
    }
}

static rt_err_t _crypto_copy(struct rt_hwcrypto_ctx *des, const struct rt_hwcrypto_ctx *src)
{
    RT_UNUSED(des);
    RT_UNUSED(src);
    return RT_EOK;
}

static void _crypto_reset(struct rt_hwcrypto_ctx *ctx)
{
    RT_UNUSED(ctx);
}

static const struct rt_hwcrypto_ops _ops =
{
    .create  = _crypto_create,
    .destroy = _crypto_destroy,
    .copy    = _crypto_copy,
    .reset   = _crypto_reset,
};

static int acm32_hw_crypto_init(void)
{
    static struct rt_hwcrypto_device _crypto_dev;

    _crypto_dev.ops       = &_ops;
    _crypto_dev.id        = 0;
    _crypto_dev.user_data = RT_NULL;

    if (rt_hwcrypto_register(&_crypto_dev, RT_HWCRYPTO_DEFAULT_NAME) != RT_EOK)
    {
        LOG_E("hwcrypto register failed");
        return -RT_ERROR;
    }

    LOG_D("hwcrypto initialized");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(acm32_hw_crypto_init);

#endif /* RT_USING_HWCRYPTO && BSP_USING_HWCRYPTO */
