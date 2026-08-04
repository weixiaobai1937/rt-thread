/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * OSPI1 PSRAM (0x80000000, 8MB) MSH tests.
 * Requires DATA_IN_ExtSRAM + System_OSPI_PSRAM_Reclock() after clock init.
 */

#include <rtthread.h>
#include <stdlib.h>
#include <string.h>
#include "board.h"
#include "acm32p4xx.h"

#ifdef DATA_IN_ExtSRAM

#define PSRAM_BASE          0x80000000U
#define PSRAM_SIZE          0x00800000U /* 8MB */
#define PSRAM_YIELD_WORDS   4096U       /* yield every 16KB */

/* PSRAM first 2MB managed by psram memheap (ETH DMA allocates from it).
 * Tests must skip that region to avoid corrupting heap metadata. */
#if defined(DATA_IN_ExtSRAM)
#define PSRAM_TEST_SKIP     0x200000U
#else
#define PSRAM_TEST_SKIP     0U
#endif

#define PSRAM_TEST_BASE     (PSRAM_BASE + PSRAM_TEST_SKIP)
#define PSRAM_TEST_SIZE     (PSRAM_SIZE - PSRAM_TEST_SKIP)

static volatile rt_uint32_t *psram_ptr(rt_uint32_t offset)
{
    return (volatile rt_uint32_t *)(PSRAM_TEST_BASE + offset);
}

static void psram_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  psram_info [reinit]      map / MR / probe\n");
    rt_kprintf("  psram_test [size_kb]     walk R/W (default 64KB)\n");
    rt_kprintf("  psram_test full          full free region walk\n");
    rt_kprintf("  psram_speed [size_kb]    bandwidth (default 256KB)\n");
#if PSRAM_TEST_SKIP
    rt_kprintf("  note: skip ETH DMA zone 0x%08X..0x%08X (%u KB)\n",
               PSRAM_BASE, PSRAM_TEST_BASE - 1U,
               (unsigned)(PSRAM_TEST_SKIP / 1024U));
#endif
}

/* Return 0 if first word R/W OK */
static int psram_probe_word(void)
{
    volatile rt_uint32_t *p = psram_ptr(0);
    rt_uint32_t old, got;

    old = *p;
    *p = 0x5A5AA5A5U;
    __DSB();
    __ISB();
    got = *p;
    *p = old;
    __DSB();

    if (got != 0x5A5AA5A5U)
    {
        rt_kprintf("PSRAM probe FAIL got=0x%08X (bus hang or not mapped)\n", got);
        return -1;
    }
    return 0;
}

static int psram_walk(rt_uint32_t bytes)
{
    volatile rt_uint32_t *p = (volatile rt_uint32_t *)PSRAM_TEST_BASE;
    rt_uint32_t words;
    rt_uint32_t i;
    rt_uint32_t err = 0;
    rt_tick_t t0, t1;

    if (bytes == 0U || bytes > PSRAM_TEST_SIZE)
        bytes = PSRAM_TEST_SIZE;
    if (bytes < 4U)
        bytes = 4U;
    words = bytes / 4U;

    rt_kprintf("PSRAM walk: base=0x%08X size=%u KB (%u words)\n",
               PSRAM_TEST_BASE, (unsigned)(bytes / 1024U), (unsigned)words);

    if (psram_probe_word() != 0)
        return -1;

    t0 = rt_tick_get();
    for (i = 0; i < words; i++)
    {
        p[i] = i ^ 0xA5A5A5A5U;
        if ((i & (PSRAM_YIELD_WORDS - 1U)) == 0U)
        {
            if ((i & 0x3FFFFU) == 0U && i != 0U) /* every 1MB */
                rt_kprintf("  write %u/%u\n", (unsigned)i, (unsigned)words);
            rt_thread_mdelay(1);
        }
    }
    __DSB();
    __ISB();

    for (i = 0; i < words; i++)
    {
        rt_uint32_t expect = i ^ 0xA5A5A5A5U;
        rt_uint32_t got = p[i];
        if (got != expect)
        {
            if (err < 8U)
            {
                rt_kprintf("  mismatch @0x%08X expect=0x%08X got=0x%08X\n",
                           PSRAM_TEST_BASE + i * 4U, expect, got);
            }
            err++;
            if (err >= 32U)
            {
                rt_kprintf("  too many errors, abort\n");
                break;
            }
        }
        if ((i & (PSRAM_YIELD_WORDS - 1U)) == 0U)
            rt_thread_mdelay(1);
    }
    t1 = rt_tick_get();

    if (err == 0U)
        rt_kprintf("PSRAM walk PASS (%u ms)\n",
                   (unsigned)((t1 - t0) * 1000U / RT_TICK_PER_SECOND));
    else
        rt_kprintf("PSRAM walk FAIL errors=%u (%u ms)\n",
                   (unsigned)err,
                   (unsigned)((t1 - t0) * 1000U / RT_TICK_PER_SECOND));

    return (err == 0U) ? 0 : -1;
}

static int psram_speed(rt_uint32_t bytes)
{
    volatile rt_uint32_t *p = (volatile rt_uint32_t *)PSRAM_TEST_BASE;
    rt_uint32_t words;
    rt_uint32_t i;
    rt_uint32_t sum = 0;
    rt_tick_t t0, t1, tw, tr;
    rt_uint32_t ms_w, ms_r;
    rt_uint32_t kbps_w, kbps_r;

    if (bytes == 0U || bytes > PSRAM_TEST_SIZE)
        bytes = 256U * 1024U;
    if (bytes > PSRAM_TEST_SIZE)
        bytes = PSRAM_TEST_SIZE;
    if (bytes < 4U)
        bytes = 4U;
    words = bytes / 4U;

    rt_kprintf("PSRAM speed base=0x%08X size=%u KB ...\n",
               PSRAM_TEST_BASE, (unsigned)(bytes / 1024U));
    if (psram_probe_word() != 0)
        return -1;

    t0 = rt_tick_get();
    for (i = 0; i < words; i++)
    {
        p[i] = i;
        if ((i & (PSRAM_YIELD_WORDS - 1U)) == 0U)
            rt_thread_mdelay(1);
    }
    __DSB();
    t1 = rt_tick_get();
    tw = t1 - t0;
    if (tw == 0)
        tw = 1;

    t0 = rt_tick_get();
    for (i = 0; i < words; i++)
    {
        sum += p[i];
        if ((i & (PSRAM_YIELD_WORDS - 1U)) == 0U)
            rt_thread_mdelay(1);
    }
    t1 = rt_tick_get();
    tr = t1 - t0;
    if (tr == 0)
        tr = 1;

    ms_w = (rt_uint32_t)(tw * 1000U / RT_TICK_PER_SECOND);
    ms_r = (rt_uint32_t)(tr * 1000U / RT_TICK_PER_SECOND);
    if (ms_w == 0)
        ms_w = 1;
    if (ms_r == 0)
        ms_r = 1;

    kbps_w = (rt_uint32_t)((rt_uint64_t)bytes * 8ULL * 1000ULL / ms_w / 1000ULL);
    kbps_r = (rt_uint32_t)((rt_uint64_t)bytes * 8ULL * 1000ULL / ms_r / 1000ULL);

    rt_kprintf("PSRAM speed checksum=0x%08X\n", sum);
    rt_kprintf("  write: %u ms, %u kbit/s\n", (unsigned)ms_w, (unsigned)kbps_w);
    rt_kprintf("  read : %u ms, %u kbit/s\n", (unsigned)ms_r, (unsigned)kbps_r);
    return 0;
}

static int psram_test(int argc, char **argv)
{
    rt_uint32_t size_kb = 64U;

    rt_kprintf("psram_test enter\n");

    if (argc >= 2)
    {
        if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h"))
        {
            psram_usage();
            return 0;
        }
        if (!strcmp(argv[1], "full"))
            return psram_walk(PSRAM_TEST_SIZE);

        size_kb = (rt_uint32_t)atoi(argv[1]);
        if (size_kb == 0U)
            size_kb = 64U;
        if (size_kb > 8192U)
            size_kb = 8192U;
    }

    return psram_walk(size_kb * 1024U);
}
MSH_CMD_EXPORT(psram_test, "PSRAM walk R/W test [size_kb|full]");

static int psram_speed_cmd(int argc, char **argv)
{
    rt_uint32_t size_kb = 256U;

    rt_kprintf("psram_speed enter\n");
    if (argc >= 2)
    {
        size_kb = (rt_uint32_t)atoi(argv[1]);
        if (size_kb == 0U)
            size_kb = 256U;
        if (size_kb > 8192U)
            size_kb = 8192U;
    }
    return psram_speed(size_kb * 1024U);
}
MSH_CMD_EXPORT_ALIAS(psram_speed_cmd, psram_speed, "PSRAM bandwidth [size_kb]");

static int psram_info(int argc, char **argv)
{
    volatile rt_uint32_t *p = psram_ptr(0);
    rt_uint32_t v;
    uint16_t mr0 = 0, mr1 = 0;
    int mr_ok = 0;
    rt_kprintf("psram_info enter\n");

    if (argc >= 2 && !strcmp(argv[1], "reinit"))
    {
#ifdef BSP_USING_ETH
        /*
         * ETH DMA descriptors/buffers live in the first 2MB of PSRAM.
         * Reclocking OSPI while ETH is active will corrupt DMA traffic.
         * Require explicit "force" after the user has stopped network use.
         */
        if (argc < 3 || strcmp(argv[2], "force") != 0)
        {
            rt_kprintf("PSRAM reinit refused: ETH uses PSRAM DMA zone.\n");
            rt_kprintf("  Use 'psram_info reinit force' only if ETH is idle.\n");
            return -1;
        }
#endif
        System_OSPI_PSRAM_Reclock();
        rt_kprintf("PSRAM reinit done\n");
    }

    System_OSPI_PSRAM_GetMR(&mr0, &mr1, &mr_ok);

    rt_kprintf("PSRAM base=0x%08X size=%u MB\n",
               PSRAM_BASE, (unsigned)(PSRAM_SIZE / (1024U * 1024U)));
    rt_kprintf("  map end=0x%08X\n", PSRAM_BASE + PSRAM_SIZE - 1U);
#if PSRAM_TEST_SKIP
    rt_kprintf("  ETH DMA reserved: 0x%08X..0x%08X (%u KB)\n",
               PSRAM_BASE, PSRAM_TEST_BASE - 1U,
               (unsigned)(PSRAM_TEST_SKIP / 1024U));
    rt_kprintf("  test region: 0x%08X..0x%08X\n",
               PSRAM_TEST_BASE, PSRAM_BASE + PSRAM_SIZE - 1U);
#endif
    rt_kprintf("  init: %s  MR: %s  MR0=0x%04X MR1=0x%04X (expect ID 0x930D)\n",
               System_OSPI_PSRAM_Ready() ? "OK" : "FAIL",
               mr_ok ? "OK" : "FAIL",
               (unsigned)mr0, (unsigned)mr1);
    rt_kprintf("  OSPI1 BAUD=0x%08X CTL=0x%08X ACC1=0x%08X ACC2=0x%08X\n",
               (unsigned)OSPI1->BAUD, (unsigned)OSPI1->CTL,
               (unsigned)OSPI1->MEMO_ACC1, (unsigned)OSPI1->MEMO_ACC2);
    rt_kprintf("  SystemCoreClock=%u\n", (unsigned)SystemCoreClock);

    v = *p;
    *p = 0x5A5AA5A5U;
    __DSB();
    __ISB();
    if (*p == 0x5A5AA5A5U)
        rt_kprintf("  probe first word: OK\n");
    else
        rt_kprintf("  probe first word: FAIL got=0x%08X\n", *p);
    *p = v;
    return 0;
}
MSH_CMD_EXPORT(psram_info, "show PSRAM map/MR; 'psram_info reinit' to reinit");

#endif /* DATA_IN_ExtSRAM */
