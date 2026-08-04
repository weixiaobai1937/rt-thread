/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx WDT/IWDT MSH test
 * 2026-08-04     AisinoChip   fix GET_TIMEOUT arg + IWDT path
 *
 * MSH:
 *   wdt_test               device info
 *   wdt_test start [sec]   start wdt with timeout (default 5s)
 *   wdt_test feed           feed wdt
 *   wdt_test stop           stop wdt
 *   wdt_test iwdt [sec]    start iwdt (default 5s)
 *
 * WARNING: starting without feeding will cause MCU reset!
 *   wdt_test start 5    -> reset after 5s
 *   wdt_test start 5 && wdt_test feed (repeat within 5s)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_WDT) && (defined(BSP_USING_WDT) || defined(BSP_USING_IWDT))

#define WDT_DEV_NAME    "wdt"
#define IWDT_DEV_NAME   "iwdt"

static int wdt_test_start(const char *dev_name, int sec)
{
    rt_device_t dev;
    rt_uint32_t timeout = 0;

    dev = rt_device_find(dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("wdt_test: %s not found\n", dev_name);
        return -1;
    }

    if (sec < 1)
        sec = 1;

    if (rt_device_init(dev) != RT_EOK)
    {
        rt_kprintf("wdt_test: init %s failed\n", dev_name);
        return -1;
    }

    if (rt_device_control(dev, RT_DEVICE_CTRL_WDT_SET_TIMEOUT, &sec) != RT_EOK)
    {
        rt_kprintf("wdt_test: set timeout failed\n");
        return -1;
    }

    if (rt_device_control(dev, RT_DEVICE_CTRL_WDT_GET_TIMEOUT, &timeout) != RT_EOK)
    {
        timeout = (rt_uint32_t)sec;
        rt_kprintf("wdt_test: GET_TIMEOUT unsupported, assume %us\n",
                   (unsigned)timeout);
    }

    if (rt_device_control(dev, RT_DEVICE_CTRL_WDT_START, RT_NULL) != RT_EOK)
    {
        rt_kprintf("wdt_test: start %s failed\n", dev_name);
        return -1;
    }

    rt_kprintf("wdt_test: %s started, timeout=%us (feed every %us to avoid reset)\n",
               dev_name, (unsigned)timeout,
               (unsigned)(timeout > 1 ? timeout / 2 : 1));
    return 0;
}

static int wdt_test_feed(const char *dev_name)
{
    rt_device_t dev;

    dev = rt_device_find(dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("wdt_test: %s not found\n", dev_name);
        return -1;
    }

    if (rt_device_control(dev, RT_DEVICE_CTRL_WDT_KEEPALIVE, RT_NULL) != RT_EOK)
    {
        rt_kprintf("wdt_test: feed %s failed\n", dev_name);
        return -1;
    }

    rt_kprintf("wdt_test: %s fed\n", dev_name);
    return 0;
}

static int wdt_test_stop(const char *dev_name)
{
    rt_device_t dev;

    dev = rt_device_find(dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("wdt_test: %s not found\n", dev_name);
        return -1;
    }

    if (rt_device_control(dev, RT_DEVICE_CTRL_WDT_STOP, RT_NULL) != RT_EOK)
    {
        rt_kprintf("wdt_test: stop %s failed (IWDT may be non-stoppable)\n", dev_name);
        return -1;
    }

    rt_kprintf("wdt_test: %s stopped\n", dev_name);
    return 0;
}

static void wdt_test_info(void)
{
    rt_kprintf("=== WDT Devices ===\n");
#ifdef BSP_USING_WDT
    rt_kprintf("  %s: APB1 window watchdog, PCLK source\n", WDT_DEV_NAME);
#endif
#ifdef BSP_USING_IWDT
    rt_kprintf("  %s: independent watchdog, RC32K source\n", IWDT_DEV_NAME);
#endif
}

static void wdt_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  wdt_test                    info\n");
    rt_kprintf("  wdt_test start [sec=5]      start WDT\n");
    rt_kprintf("  wdt_test feed               feed WDT\n");
    rt_kprintf("  wdt_test stop               stop WDT\n");
    rt_kprintf("  wdt_test iwdt [sec=5]       start IWDT\n");
    rt_kprintf("\nWARNING: WDT reset will reboot the MCU!\n");
    rt_kprintf("  Keep feeding: wdt_test start 5  then periodically wdt_test feed\n");
}

static int wdt_test(int argc, char **argv)
{
    if (argc < 2)
    {
        wdt_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        wdt_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "start") == 0)
    {
        int sec = (argc >= 3) ? atoi(argv[2]) : 5;
#ifdef BSP_USING_WDT
        return wdt_test_start(WDT_DEV_NAME, sec);
#else
        rt_kprintf("wdt_test: WDT not enabled; try 'wdt_test iwdt'\n");
        return -1;
#endif
    }

    if (rt_strcmp(argv[1], "feed") == 0)
    {
#ifdef BSP_USING_WDT
        return wdt_test_feed(WDT_DEV_NAME);
#elif defined(BSP_USING_IWDT)
        return wdt_test_feed(IWDT_DEV_NAME);
#else
        return -1;
#endif
    }

    if (rt_strcmp(argv[1], "stop") == 0)
    {
#ifdef BSP_USING_WDT
        return wdt_test_stop(WDT_DEV_NAME);
#elif defined(BSP_USING_IWDT)
        return wdt_test_stop(IWDT_DEV_NAME);
#else
        return -1;
#endif
    }

    if (rt_strcmp(argv[1], "iwdt") == 0)
    {
        int sec = (argc >= 3) ? atoi(argv[2]) : 5;
#ifdef BSP_USING_IWDT
        return wdt_test_start(IWDT_DEV_NAME, sec);
#else
        rt_kprintf("wdt_test: IWDT not enabled in BSP\n");
        return -1;
#endif
    }

    wdt_usage();
    return 0;
}
MSH_CMD_EXPORT(wdt_test, WDT/IWDT watchdog test);

#endif /* RT_USING_WDT && (BSP_USING_WDT || BSP_USING_IWDT) */
