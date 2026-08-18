/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-28     AisinoChip   HW crypto self test (AES/SHA1/SHA256/CRC/RNG)
 */

#include <rtthread.h>
#include <rtdevice.h>

#if defined(RT_USING_HWCRYPTO) && defined(BSP_USING_HWCRYPTO)
#include <hw_symmetric.h>
#include <hw_hash.h>
#include <hw_crc.h>
#include <hw_rng.h>

#define DBG_TAG "crypto_test"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

static int _fail;

#define CHECK(cond, name)                                            \
    do {                                                             \
        if (cond) { rt_kprintf("  %-28s PASS\n", name); }          \
        else      { rt_kprintf("  %-28s FAIL\n", name); _fail++; } \
    } while (0)

/* ---- AES-128 vectors (FIPS-197 / SP 800-38A, single block) ---- */

static const rt_uint8_t _aes_key[16] =
{ 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x0e,0x0f };

static const rt_uint8_t _aes_pt[16] =
{ 0x00,0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99,0xaa,0xbb,0xcc,0xdd,0xee,0xff };

static const rt_uint8_t _aes_ecb_ct[16] =
{ 0x69,0xc4,0xe0,0xd8,0x6a,0x7b,0x04,0x30,0xd8,0xcd,0xb7,0x80,0x70,0xb4,0xc5,0x5a };

static const rt_uint8_t _aes_cbc_ct[16] =
{ 0x76,0x49,0xab,0xac,0x81,0x19,0xb2,0x46,0xce,0xe9,0x8e,0x9b,0x12,0xe9,0x19,0x7d };

static const rt_uint8_t _aes_ctr_ct[16] =
{ 0x87,0x4d,0x61,0x91,0xb6,0x20,0xe3,0x26,0x1b,0xef,0x68,0x64,0x99,0x0d,0xb6,0xce };

static void _aes_test(hwcrypto_type type, const rt_uint8_t *expect, const char *name)
{
    struct rt_hwcrypto_ctx *ctx;
    rt_uint8_t out[16];
    rt_uint8_t iv[16];

    rt_memcpy(iv, _aes_key, 16);   /* SP 800-38A uses 000102..0F as IV/counter */
    ctx = rt_hwcrypto_symmetric_create(rt_hwcrypto_dev_default(), type);
    if (ctx == RT_NULL)
    {
        rt_kprintf("  %-28s FAIL (create)\n", name);
        _fail++;
        return;
    }

    rt_hwcrypto_symmetric_setkey(ctx, _aes_key, 128);
    rt_hwcrypto_symmetric_setiv(ctx, iv, 16);
    if (rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_ENCRYPT, 16, _aes_pt, out) == RT_EOK)
        CHECK(rt_memcmp(out, expect, 16) == 0, name);
    else
    {
        rt_kprintf("  %-28s FAIL (crypt)\n", name);
        _fail++;
    }

    /* decrypt back and compare with plaintext */
    if (rt_hwcrypto_symmetric_crypt(ctx, HWCRYPTO_MODE_DECRYPT, 16, out, out) == RT_EOK)
        CHECK(rt_memcmp(out, _aes_pt, 16) == 0, name);
    else
    {
        rt_kprintf("  %-28s FAIL (decrypt)\n", name);
        _fail++;
    }

    rt_hwcrypto_ctx_destroy(ctx);
}

/* ---- SHA vectors (FIPS 180-x, input "abc") ---- */

static const rt_uint8_t _sha1_digest[20] =
{ 0xa9,0x99,0x3e,0x36,0x47,0x06,0x81,0x6a,0xba,0x3e,0x25,0x71,0x78,0x50,0xc2,0x6c,0x9c,0xd0,0xd8,0x9d };

static const rt_uint8_t _sha256_digest[32] =
{ 0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
  0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad };

static void _hash_test(hwcrypto_type type, const rt_uint8_t *expect, rt_size_t len, const char *name)
{
    struct rt_hwcrypto_ctx *ctx;
    rt_uint8_t out[32];

    ctx = rt_hwcrypto_hash_create(rt_hwcrypto_dev_default(), type);
    if (ctx == RT_NULL)
    {
        rt_kprintf("  %-28s FAIL (create)\n", name);
        _fail++;
        return;
    }
    rt_hwcrypto_hash_update(ctx, (const rt_uint8_t *)"abc", 3);
    rt_hwcrypto_hash_finish(ctx, out, len);
    CHECK(rt_memcmp(out, expect, len) == 0, name);
    rt_hwcrypto_ctx_destroy(ctx);
}

/* ---- CRC-32 (ISO-HDLC, "123456789" -> 0xCBF43926) ---- */

static const rt_uint8_t _crc_data[] = "123456789";

static void _crc_test(void)
{
    struct rt_hwcrypto_ctx *ctx;
    struct hwcrypto_crc_cfg cfg;
    rt_uint32_t v;

    /* standard CRC-32: init=0xFFFFFFFF, refin/refout, xorout=0xFFFFFFFF */
    cfg.last_val = 0xFFFFFFFF;
    cfg.poly     = 0x04C11DB7;
    cfg.width    = 32;
    cfg.xorout   = 0xFFFFFFFF;
    cfg.flags    = CRC_FLAG_REFIN | CRC_FLAG_REFOUT;

    ctx = rt_hwcrypto_crc_create(rt_hwcrypto_dev_default(), HWCRYPTO_CRC_CUSTOM);
    if (ctx == RT_NULL)
    {
        rt_kprintf("  %-28s FAIL (create)\n", "CRC32");
        _fail++;
        return;
    }
    rt_hwcrypto_crc_cfg(ctx, &cfg);
    v = rt_hwcrypto_crc_update(ctx, _crc_data, sizeof(_crc_data) - 1);
    CHECK(v == 0xCBF43926UL, "CRC32");
    rt_hwcrypto_ctx_destroy(ctx);
}

/* ---- RNG basic sanity ---- */

static void _rng_test(void)
{
    struct rt_hwcrypto_ctx *ctx;
    rt_uint32_t a, b, i;
    rt_bool_t varied = RT_FALSE;

    ctx = rt_hwcrypto_rng_create(rt_hwcrypto_dev_default());
    if (ctx == RT_NULL)
    {
        rt_kprintf("  %-28s FAIL (create)\n", "RNG");
        _fail++;
        return;
    }

    a = rt_hwcrypto_rng_update();
    for (i = 0; i < 8; i++)
    {
        b = rt_hwcrypto_rng_update();
        if (b != a)
            varied = RT_TRUE;
    }
    CHECK(a != 0 && a != 0xFFFFFFFFUL && varied, "RNG");
    rt_hwcrypto_ctx_destroy(ctx);
}

static void crypto_test(void)
{
    _fail = 0;
    rt_kprintf("crypto_test (HW hwcrypto self test)\n");

    _aes_test(HWCRYPTO_TYPE_AES_ECB, _aes_ecb_ct, "AES-128-ECB");
    _aes_test(HWCRYPTO_TYPE_AES_CBC, _aes_cbc_ct, "AES-128-CBC");
    _aes_test(HWCRYPTO_TYPE_AES_CTR, _aes_ctr_ct, "AES-128-CTR");
    _hash_test(HWCRYPTO_TYPE_SHA1, _sha1_digest, 20, "SHA1");
    _hash_test(HWCRYPTO_TYPE_SHA256, _sha256_digest, 32, "SHA256");
    _crc_test();
    _rng_test();

    rt_kprintf("crypto_test %s (%d fail)\n", _fail ? "FAIL" : "PASS", _fail);
}
MSH_CMD_EXPORT(crypto_test, HW crypto self test: AES/SHA1/SHA256/CRC/RNG);

#endif /* RT_USING_HWCRYPTO && BSP_USING_HWCRYPTO */
