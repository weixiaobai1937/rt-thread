/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx HWRNG driver
 * 2026-07-28     AisinoChip   add HW AES/SHA1/SHA256/CRC via hwcrypto
 */

#include <board.h>
#include <rtdevice.h>

#if defined(RT_USING_HWCRYPTO) && defined(BSP_USING_HWCRYPTO)
#include <hw_symmetric.h>
#include <hw_hash.h>
#include <hw_crc.h>
#include "hal_aes.h"
#include "hal_sha1.h"
#include "hal_sha256.h"
#include "hal_crc.h"
#include "hal_hrng.h"

#define DBG_TAG "drv.crypto"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ==================== RNG (existing) ==================== */

static rt_uint32_t _rng_update(struct hwcrypto_rng *ctx)
{
    RT_UNUSED(ctx);
    return HAL_HRNG_GetHrng_32();
}

static const struct hwcrypto_rng_ops rng_ops =
{
    .update = _rng_update,
};

/* ==================== AES symmetric ==================== */

static rt_err_t _aes_crypt(struct hwcrypto_symmetric *ctx,
                           struct hwcrypto_symmetric_info *info)
{
    uint8_t key_len, mode, op, sub;
    rt_size_t off;

    switch (ctx->key_bitlen)
    {
    case 128: key_len = AES_KEY_128; break;
    case 192: key_len = AES_KEY_192; break;
    case 256: key_len = AES_KEY_256; break;
    default:  return -RT_EINVAL;
    }

    /* hwcrypto AES subtypes live in bits 8-15 (HWCRYPTO_TYPE_AES_ECB = AES | (0x01<<8)) */
    sub = (uint8_t)((ctx->parent.type >> 8) & 0xFF);
    switch (sub)
    {
    case 0x01: mode = AES_ECB_MODE; break;
    case 0x02: mode = AES_CBC_MODE; break;
    case 0x04: mode = AES_CTR_MODE; break;
    default:
        /* vendor AES core only provides ECB/CBC/CTR */
        LOG_E("AES mode %d unsupported", sub);
        return -RT_EINVAL;
    }

    op = (info->mode == HWCRYPTO_MODE_ENCRYPT) ? AES_ENCRYPTION : AES_DECRYPTION;

    HAL_AES_SetKey_U8(ctx->key, key_len, AES_SWAP_DISABLE);

    /* process 16-byte blocks; CBC/CTR chain via IV carried by the hardware */
    for (off = 0; off < info->length; off += 16)
    {
        HAL_AES_Crypt_U8((uint8_t *)info->in + off, info->out + off, 16,
                         op, mode, ctx->iv, AES_NORMAL_MODE);
    }

    return RT_EOK;
}

static const struct hwcrypto_symmetric_ops _symmetric_ops =
{
    .crypt = _aes_crypt,
};

/* ==================== SHA1 / SHA256 hash ==================== */

/* Per-context state is kept in ctx->contex (malloc'd at create) so that
 * concurrent hash contexts do not corrupt each other. */
static rt_bool_t _hash_is_sha1(struct hwcrypto_hash *ctx)
{
    return (ctx->parent.type & HWCRYPTO_MAIN_TYPE_MASK) == HWCRYPTO_TYPE_SHA1;
}

static rt_err_t _hash_update(struct hwcrypto_hash *ctx,
                             const rt_uint8_t *input, rt_size_t length)
{
    if (ctx->parent.contex == RT_NULL)
        return -RT_ERROR;
    if (_hash_is_sha1(ctx))
        HAL_SHA1_Update((SHA1_CTX *)ctx->parent.contex, (uint8_t *)input, (uint32_t)length);
    else
        HAL_SHA256_Update((SHA256_CTX *)ctx->parent.contex, (uint8_t *)input, (uint32_t)length);
    return RT_EOK;
}

static rt_err_t _hash_finish(struct hwcrypto_hash *ctx,
                             rt_uint8_t *output, rt_size_t length)
{
    if (ctx->parent.contex == RT_NULL)
        return -RT_ERROR;
    if (_hash_is_sha1(ctx))
        HAL_SHA1_Final(output, (SHA1_CTX *)ctx->parent.contex);
    else
        HAL_SHA256_Final(output, (SHA256_CTX *)ctx->parent.contex);
    return RT_EOK;
}

static const struct hwcrypto_hash_ops _hash_ops =
{
    .update = _hash_update,
    .finish = _hash_finish,
};

/* ==================== CRC ==================== */

static rt_uint32_t _crc_update(struct hwcrypto_crc *ctx,
                               const rt_uint8_t *in, rt_size_t length)
{
    CRC_HandleTypeDef hcrc;
    uint32_t res;

    rt_memset(&hcrc, 0, sizeof(hcrc));
    hcrc.Instance = CRC;

    /* width selects the poly length; the data stream is always byte-wise so
     * HAL_CRC_Calculate walks every byte (DataLen==1B) and no tail is dropped */
    switch (ctx->crc_cfg.width)
    {
    case 8:  hcrc.Init.PolyLen = CRC_POLTY_LEN_8;  break;
    case 16: hcrc.Init.PolyLen = CRC_POLTY_LEN_16; break;
    case 32:
    default: hcrc.Init.PolyLen = CRC_POLTY_LEN_32; break;
    }
    hcrc.Init.DataLen = CRC_DATA_LEN_1B;

    hcrc.Init.PolyData  = ctx->crc_cfg.poly;
    hcrc.Init.OutXorData = ctx->crc_cfg.xorout;
    hcrc.Init.InitData  = ctx->crc_cfg.last_val;
    /* refin -> poly + data (byte) reverse; refout -> result + outxor reverse */
    hcrc.Init.PolyRev   = (ctx->crc_cfg.flags & CRC_FLAG_REFIN) ? CRC_POLY_REV_EN : CRC_POLY_REV_DIS;
    hcrc.Init.DataRev   = (ctx->crc_cfg.flags & CRC_FLAG_REFIN) ? CRC_DATA_REV_BY_BYTE : CRC_DATA_REV_DISABLE;
    hcrc.Init.RsltRev   = (ctx->crc_cfg.flags & CRC_FLAG_REFOUT) ? CRC_RSLT_REV_EN : CRC_RSLT_REV_DIS;
    hcrc.Init.OutxorRev = (ctx->crc_cfg.flags & CRC_FLAG_REFOUT) ? CRC_OUTXOR_REV_EN : CRC_OUTXOR_REV_DIS;

    hcrc.CRC_Data_Buff = (uint8_t *)in;
    hcrc.CRC_Data_Len  = (uint32_t)length;

    HAL_CRC_Init(&hcrc);
    res = HAL_CRC_Calculate(&hcrc);

    /* keep state for chained update() calls */
    ctx->crc_cfg.last_val = res;
    return res;
}

static const struct hwcrypto_crc_ops _crc_ops =
{
    .update = _crc_update,
};

/* ==================== context create / destroy ==================== */

static rt_uint32_t rng_ref_count;

static rt_err_t _crypto_create(struct rt_hwcrypto_ctx *ctx)
{
    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK)
    {
    case HWCRYPTO_TYPE_RNG:
    {
        rt_base_t level = rt_hw_interrupt_disable();
        if (rng_ref_count == 0)
            HAL_HRNG_Init();
        rng_ref_count++;
        rt_hw_interrupt_enable(level);
        ctx->contex = RT_NULL;
        ((struct hwcrypto_rng *)ctx)->ops = &rng_ops;
        LOG_D("HRNG created");
        break;
    }

    case HWCRYPTO_TYPE_AES:
        ((struct hwcrypto_symmetric *)ctx)->ops = &_symmetric_ops;
        LOG_D("AES context created");
        break;

    case HWCRYPTO_TYPE_SHA1:
        ctx->contex = rt_malloc(sizeof(SHA1_CTX));
        if (ctx->contex == RT_NULL)
            return -RT_ENOMEM;
        HAL_SHA1_Init((SHA1_CTX *)ctx->contex);
        ((struct hwcrypto_hash *)ctx)->ops = &_hash_ops;
        LOG_D("SHA1 context created");
        break;

    case HWCRYPTO_TYPE_SHA2:
        /* only SHA256 (subtype 0x02, bits 8-15) is wired to the hardware core */
        if (((ctx->type >> 8) & 0xFF) != 0x02)
        {
            LOG_E("SHA2 subtype %02x unsupported", ctx->type & 0xFF);
            return -RT_EINVAL;
        }
        ctx->contex = rt_malloc(sizeof(SHA256_CTX));
        if (ctx->contex == RT_NULL)
            return -RT_ENOMEM;
        HAL_SHA256_Init((SHA256_CTX *)ctx->contex);
        ((struct hwcrypto_hash *)ctx)->ops = &_hash_ops;
        LOG_D("SHA256 context created");
        break;

    case HWCRYPTO_TYPE_CRC:
        ((struct hwcrypto_crc *)ctx)->ops = &_crc_ops;
        LOG_D("CRC context created");
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
    {
        rt_base_t level = rt_hw_interrupt_disable();
        if (rng_ref_count > 0)
            rng_ref_count--;
        if (rng_ref_count == 0)
            HAL_HRNG_DeInit();
        rt_hw_interrupt_enable(level);
        LOG_D("HRNG destroyed");
        break;
    }

    case HWCRYPTO_TYPE_SHA1:
    case HWCRYPTO_TYPE_SHA2:
        if (ctx->contex != RT_NULL)
        {
            rt_free(ctx->contex);
            ctx->contex = RT_NULL;
        }
        break;

    default:
        break;
    }
}

static rt_err_t _crypto_copy(struct rt_hwcrypto_ctx *des, const struct rt_hwcrypto_ctx *src)
{
    /* RNG/AES/SHA/CRC are stateless in this driver; context copy not needed */
    RT_UNUSED(des);
    RT_UNUSED(src);
    return RT_EOK;
}

static void _crypto_reset(struct rt_hwcrypto_ctx *ctx)
{
    switch (ctx->type & HWCRYPTO_MAIN_TYPE_MASK)
    {
    case HWCRYPTO_TYPE_SHA1:
        if (ctx->contex != RT_NULL)
            HAL_SHA1_Init((SHA1_CTX *)ctx->contex);
        break;
    case HWCRYPTO_TYPE_SHA2:
        if (ctx->contex != RT_NULL)
            HAL_SHA256_Init((SHA256_CTX *)ctx->contex);
        break;
    default:
        break;
    }
}

static const struct rt_hwcrypto_ops _ops =
{
    .create  = _crypto_create,
    .destroy = _crypto_destroy,
    .copy    = _crypto_copy,
    .reset   = _crypto_reset,
};

static int rt_hw_crypto_init(void)
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
INIT_DEVICE_EXPORT(rt_hw_crypto_init);

#endif /* RT_USING_HWCRYPTO && BSP_USING_HWCRYPTO */
