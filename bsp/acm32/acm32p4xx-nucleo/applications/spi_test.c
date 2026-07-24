/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * SPI1~SPI4 MSH tests (loopback / poll / DMA half-duplex).
 *
 * Hardware: short MOSI-MISO on the bus under test for loopback PASS.
 * DMA path (when BSP_USING_SPIx_DMA): half-duplex TX/RX length >= 32.
 *
 * MSH:
 *   spi_test                 list enabled buses + auto all
 *   spi_test info            pin / CS / DMA map
 *   spi_test loopback [n] [len]
 *   spi_test poll [n] [len]
 *   spi_test dma [n] [len]
 *   spi_test all [len]
 *
 * n = 1..4 (SPI unit). Default n = first enabled SPI.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include "spi_config.h"

#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2) || \
    defined(BSP_USING_SPI3) || defined(BSP_USING_SPI4)

extern rt_err_t rt_hw_spi_device_attach(const char *bus_name,
                                       const char *device_name,
                                       rt_base_t cs_pin);

#define SPI_TEST_MAX_LEN    512
#define SPI_TEST_DEF_LEN    16
#define SPI_TEST_DMA_LEN    256

struct spi_test_unit
{
    int         unit;           /* 1..4 */
    const char *bus_name;       /* "spi1" */
    const char *dev_name;       /* "spi10" */
    rt_base_t   cs_pin;
    const char *pins_hint;      /* human-readable MOSI/MISO */
    rt_bool_t   dma_en;
};

static const struct spi_test_unit g_spi_units[] =
{
#ifdef BSP_USING_SPI1
    {
        .unit = 1,
        .bus_name = "spi1",
        .dev_name = "spi10",
        .cs_pin = SPI1_DEFAULT_CS_PIN_INDEX,
        .pins_hint = "SCK/MOSI/MISO/CS (Kconfig pin)",
#ifdef BSP_USING_SPI1_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
    },
#endif
#ifdef BSP_USING_SPI2
    {
        .unit = 2,
        .bus_name = "spi2",
        .dev_name = "spi20",
        .cs_pin = SPI2_DEFAULT_CS_PIN_INDEX,
        .pins_hint = "SCK/MOSI/MISO/CS (Kconfig pin)",
#ifdef BSP_USING_SPI2_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
    },
#endif
#ifdef BSP_USING_SPI3
    {
        .unit = 3,
        .bus_name = "spi3",
        .dev_name = "spi30",
        .cs_pin = SPI3_DEFAULT_CS_PIN_INDEX,
        .pins_hint = "SCK/MOSI/MISO/CS (Kconfig pin)",
#ifdef BSP_USING_SPI3_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
    },
#endif
#ifdef BSP_USING_SPI4
    {
        .unit = 4,
        .bus_name = "spi4",
        .dev_name = "spi40",
        .cs_pin = SPI4_DEFAULT_CS_PIN_INDEX,
        .pins_hint = "SCK/MOSI/MISO/CS (Kconfig pin)",
#ifdef BSP_USING_SPI4_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
    },
#endif
};

static const struct spi_test_unit *spi_test_find(int unit)
{
    rt_size_t i;

    for (i = 0; i < sizeof(g_spi_units) / sizeof(g_spi_units[0]); i++)
    {
        if (g_spi_units[i].unit == unit)
            return &g_spi_units[i];
    }
    return RT_NULL;
}

static const struct spi_test_unit *spi_test_default(void)
{
    if (sizeof(g_spi_units) / sizeof(g_spi_units[0]) == 0)
        return RT_NULL;
    return &g_spi_units[0];
}

static void spi_test_print_dma(int unit)
{
    switch (unit)
    {
#ifdef BSP_USING_SPI1_DMA
    case 1:
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d (min %d B half-duplex)\n",
                   BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH,
                   BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH,
                   SPI_DMA_MIN_SIZE);
        break;
#endif
#ifdef BSP_USING_SPI2_DMA
    case 2:
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d (min %d B half-duplex)\n",
                   BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH,
                   BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH,
                   SPI_DMA_MIN_SIZE);
        break;
#endif
#ifdef BSP_USING_SPI3_DMA
    case 3:
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d (min %d B half-duplex)\n",
                   BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH,
                   BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH,
                   SPI_DMA_MIN_SIZE);
        break;
#endif
#ifdef BSP_USING_SPI4_DMA
    case 4:
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d (min %d B half-duplex)\n",
                   BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH,
                   BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH,
                   SPI_DMA_MIN_SIZE);
        break;
#endif
    default:
        rt_kprintf("    DMA: off (poll only)\n");
        break;
    }
}

static void spi_test_info(void)
{
    rt_size_t i;

    rt_kprintf("SPI test units (enabled in Kconfig):\n");
    if (sizeof(g_spi_units) / sizeof(g_spi_units[0]) == 0)
    {
        rt_kprintf("  (none)\n");
        return;
    }
    for (i = 0; i < sizeof(g_spi_units) / sizeof(g_spi_units[0]); i++)
    {
        const struct spi_test_unit *u = &g_spi_units[i];
        rt_device_t bus = rt_device_find(u->bus_name);

        rt_kprintf("  SPI%d bus=%s dev=%s cs_pin=%d %s\n",
                   u->unit, u->bus_name, u->dev_name, (int)u->cs_pin,
                   bus ? "BUS_OK" : "BUS_MISSING");
        rt_kprintf("    pins: %s\n", u->pins_hint);
        if (u->dma_en)
            spi_test_print_dma(u->unit);
        else
            rt_kprintf("    DMA: off\n");
    }
    rt_kprintf("note: full-duplex always poll; half-duplex >=%d uses DMA if enabled\n",
               SPI_DMA_MIN_SIZE);
}

static struct rt_spi_device *spi_test_prepare(const struct spi_test_unit *u)
{
    struct rt_spi_device *dev;
    struct rt_spi_configuration cfg;
    rt_err_t ret;

    if (u == RT_NULL)
        return RT_NULL;

    if (rt_device_find(u->bus_name) == RT_NULL)
    {
        rt_kprintf("[spi_test] bus %s not found\n", u->bus_name);
        return RT_NULL;
    }

    dev = (struct rt_spi_device *)rt_device_find(u->dev_name);
    if (dev == RT_NULL)
    {
        ret = rt_hw_spi_device_attach(u->bus_name, u->dev_name, u->cs_pin);
        if (ret != RT_EOK)
        {
            rt_kprintf("[spi_test] attach %s failed: %d\n", u->dev_name, ret);
            return RT_NULL;
        }
        dev = (struct rt_spi_device *)rt_device_find(u->dev_name);
    }
    if (dev == RT_NULL)
    {
        rt_kprintf("[spi_test] %s not found after attach\n", u->dev_name);
        return RT_NULL;
    }

    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = 1000000;
    ret = rt_spi_configure(dev, &cfg);
    if (ret != RT_EOK)
    {
        rt_kprintf("[spi_test] configure failed: %d\n", ret);
        return RT_NULL;
    }
    return dev;
}

static int spi_test_clamp_len(int len, int def_len)
{
    if (len <= 0)
        len = def_len;
    if (len > SPI_TEST_MAX_LEN)
        len = SPI_TEST_MAX_LEN;
    return len;
}

/* Full-duplex loopback: needs MOSI-MISO short */
static int spi_test_do_loopback(const struct spi_test_unit *u, int len)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[SPI_TEST_MAX_LEN];
    static rt_uint8_t rx[SPI_TEST_MAX_LEN];
    int i, pass = 1;

    len = spi_test_clamp_len(len, SPI_TEST_DEF_LEN);
    dev = spi_test_prepare(u);
    if (dev == RT_NULL)
        return -1;

    for (i = 0; i < len; i++)
        tx[i] = (rt_uint8_t)(0xA0 + (i & 0x0F));
    rt_memset(rx, 0, (rt_size_t)len);

    rt_kprintf("[spi_test] SPI%d loopback len=%d (%s)\n",
               u->unit, len, u->pins_hint);

    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != (rt_size_t)len)
    {
        rt_kprintf("[spi_test] SPI%d transfer failed FAIL\n", u->unit);
        return -1;
    }

    for (i = 0; i < len; i++)
    {
        if (rx[i] != tx[i])
        {
            pass = 0;
            rt_kprintf("  mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
            if (i > 8)
            {
                rt_kprintf("  ... (more mismatches suppressed)\n");
                break;
            }
        }
    }

    rt_kprintf("[spi_test] SPI%d loopback %s\n", u->unit, pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}

/* Short poll TX-only + RX-only (no external short required for TX) */
static int spi_test_do_poll(const struct spi_test_unit *u, int len)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[SPI_TEST_MAX_LEN];
    static rt_uint8_t rx[SPI_TEST_MAX_LEN];
    rt_size_t n;
    int pass = 1;

    len = spi_test_clamp_len(len, SPI_TEST_DEF_LEN);
    if (len >= SPI_DMA_MIN_SIZE)
        len = SPI_DMA_MIN_SIZE - 1; /* force poll path */

    dev = spi_test_prepare(u);
    if (dev == RT_NULL)
        return -1;

    for (n = 0; n < (rt_size_t)len; n++)
        tx[n] = (rt_uint8_t)(0x5A ^ (n & 0xFF));
    rt_memset(rx, 0x00, (rt_size_t)len);

    rt_kprintf("[spi_test] SPI%d poll len=%d (TX then RX, no DMA)\n", u->unit, len);

    n = rt_spi_send(dev, tx, (rt_size_t)len);
    if (n != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  rt_spi_send failed: %d\n", (int)n);
    }

    n = rt_spi_recv(dev, rx, (rt_size_t)len);
    if (n != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  rt_spi_recv failed: %d\n", (int)n);
    }
    else
    {
        rt_kprintf("  recv first=%02X last=%02X (float MISO ok)\n",
                   rx[0], rx[len - 1]);
    }

    /* short full-duplex still poll */
    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  fdx poll transfer failed\n");
    }

    rt_kprintf("[spi_test] SPI%d poll %s\n", u->unit, pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}

/* DMA half-duplex + short after DMA + optional fdx loopback */
static int spi_test_do_dma(const struct spi_test_unit *u, int len)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[SPI_TEST_MAX_LEN];
    static rt_uint8_t rx[SPI_TEST_MAX_LEN];
    int i, pass = 1;
    rt_size_t n;

    if (!u->dma_en)
    {
        rt_kprintf("[spi_test] SPI%d DMA not enabled in Kconfig, skip\n", u->unit);
        return 0; /* not a hard fail */
    }

    len = spi_test_clamp_len(len, SPI_TEST_DMA_LEN);
    if (len < SPI_DMA_MIN_SIZE)
        len = SPI_DMA_MIN_SIZE;

    dev = spi_test_prepare(u);
    if (dev == RT_NULL)
        return -1;

    for (i = 0; i < len; i++)
        tx[i] = 0x5A;
    rt_memset(rx, 0, (rt_size_t)len);

    rt_kprintf("[spi_test] SPI%d dma half-duplex len=%d\n", u->unit, len);
    spi_test_print_dma(u->unit);

    /* full-duplex: always poll even with DMA enabled */
    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  fdx transfer failed\n");
    }
    else
    {
        /* loopback only if shorted; report but don't hard-fail on mismatch */
        int lb_ok = 1;
        for (i = 0; i < len; i++)
        {
            if (rx[i] != tx[i])
            {
                lb_ok = 0;
                break;
            }
        }
        rt_kprintf("  fdx poll %s (need MOSI-MISO short for match)\n",
                   lb_ok ? "data match" : "no match / open MISO");
    }

    n = rt_spi_send(dev, tx, (rt_size_t)len);
    if (n != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  DMA TX (rt_spi_send) failed: %d\n", (int)n);
    }
    else
        rt_kprintf("  DMA TX (rt_spi_send) OK\n");

    rt_memset(rx, 0xA5, (rt_size_t)len);
    n = rt_spi_recv(dev, rx, (rt_size_t)len);
    if (n != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("  DMA RX (rt_spi_recv) failed: %d\n", (int)n);
    }
    else
        rt_kprintf("  DMA RX (rt_spi_recv) OK first=%02X last=%02X\n",
                   rx[0], rx[len - 1]);

    /* short transfer after DMA must still work (poll path) */
    for (i = 0; i < 16; i++)
        tx[i] = (rt_uint8_t)(0xA0 + i);
    rt_memset(rx, 0, 16);
    if (rt_spi_transfer(dev, tx, rx, 16) != 16)
    {
        pass = 0;
        rt_kprintf("  short transfer after DMA failed\n");
    }
    else
        rt_kprintf("  short after DMA OK\n");

    rt_kprintf("[spi_test] SPI%d dma %s\n", u->unit, pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}

static int spi_test_parse_unit(int argc, char **argv, int argi, int *unit_out)
{
    const struct spi_test_unit *u;

    if (argc > argi)
    {
        int n = atoi(argv[argi]);
        if (n < 1 || n > 4)
        {
            rt_kprintf("[spi_test] unit must be 1..4\n");
            return -1;
        }
        u = spi_test_find(n);
        if (u == RT_NULL)
        {
            rt_kprintf("[spi_test] SPI%d not enabled in Kconfig\n", n);
            return -1;
        }
        *unit_out = n;
        return 0;
    }

    u = spi_test_default();
    if (u == RT_NULL)
    {
        rt_kprintf("[spi_test] no SPI enabled\n");
        return -1;
    }
    *unit_out = u->unit;
    return 0;
}

static int spi_test_run_all(int len)
{
    rt_size_t i;
    int fail = 0, ran = 0;

    rt_kprintf("======== spi_test all ========\n");
    spi_test_info();

    for (i = 0; i < sizeof(g_spi_units) / sizeof(g_spi_units[0]); i++)
    {
        const struct spi_test_unit *u = &g_spi_units[i];
        int lb_len = (len > 0) ? len : SPI_TEST_DEF_LEN;
        int dma_len = (len >= SPI_DMA_MIN_SIZE) ? len : SPI_TEST_DMA_LEN;

        ran++;
        rt_kprintf("---- SPI%d ----\n", u->unit);
        if (spi_test_do_poll(u, SPI_TEST_DEF_LEN) != 0)
            fail++;
        if (spi_test_do_loopback(u, lb_len) != 0)
            fail++;
        if (u->dma_en)
        {
            if (spi_test_do_dma(u, dma_len) != 0)
                fail++;
        }
        else
            rt_kprintf("[spi_test] SPI%d DMA skipped (not in Kconfig)\n", u->unit);
    }

    if (ran == 0)
    {
        rt_kprintf("[spi_test] no SPI bus enabled FAIL\n");
        return -1;
    }

    rt_kprintf("======== spi_test all: ran=%d fail=%d %s ========\n",
               ran, fail, fail ? "FAIL" : "PASS");
    return fail ? -1 : 0;
}

static void spi_test_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  spi_test                    auto all enabled SPI\n");
    rt_kprintf("  spi_test info               pin/CS/DMA map\n");
    rt_kprintf("  spi_test loopback [n] [len] MOSI-MISO short required\n");
    rt_kprintf("  spi_test poll [n] [len]     short TX/RX poll path\n");
    rt_kprintf("  spi_test dma [n] [len]      half-duplex DMA (>=%d)\n", SPI_DMA_MIN_SIZE);
    rt_kprintf("  spi_test all [len]          poll+loopback+dma each unit\n");
    rt_kprintf("n=1..4 SPI unit; short MOSI-MISO for loopback PASS\n");
}

static int spi_test(int argc, char **argv)
{
    int unit, len;
    const struct spi_test_unit *u;

    if (argc < 2)
        return spi_test_run_all(0);

    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h"))
    {
        spi_test_usage();
        return 0;
    }
    if (!strcmp(argv[1], "info"))
    {
        spi_test_info();
        return 0;
    }
    if (!strcmp(argv[1], "all"))
    {
        len = (argc >= 3) ? atoi(argv[2]) : 0;
        return spi_test_run_all(len);
    }

    if (!strcmp(argv[1], "loopback") || !strcmp(argv[1], "poll") ||
        !strcmp(argv[1], "dma"))
    {
        if (spi_test_parse_unit(argc, argv, 2, &unit) != 0)
            return -1;
        u = spi_test_find(unit);
        len = (argc >= 4) ? atoi(argv[3]) : 0;

        if (!strcmp(argv[1], "loopback"))
            return spi_test_do_loopback(u, len ? len : SPI_TEST_DEF_LEN);
        if (!strcmp(argv[1], "poll"))
            return spi_test_do_poll(u, len ? len : SPI_TEST_DEF_LEN);
        return spi_test_do_dma(u, len ? len : SPI_TEST_DMA_LEN);
    }

    /* spi_test 1 [len] -> loopback on unit */
    if (argv[1][0] >= '1' && argv[1][0] <= '4' && argv[1][1] == '\0')
    {
        unit = argv[1][0] - '0';
        u = spi_test_find(unit);
        if (u == RT_NULL)
        {
            rt_kprintf("[spi_test] SPI%d not enabled\n", unit);
            return -1;
        }
        len = (argc >= 3) ? atoi(argv[2]) : SPI_TEST_DEF_LEN;
        return spi_test_do_loopback(u, len);
    }

    spi_test_usage();
    return 0;
}
MSH_CMD_EXPORT(spi_test, "SPI1-4 test: info/loopback/poll/dma/all");

#endif /* any SPI */
