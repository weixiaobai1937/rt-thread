/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx Input Capture test
 * 2026-08-04     AisinoChip   fix device names + read API
 *
 * MSH:
 *   cap_test                  list capture devices
 *   cap_test read [dev]       read pulsewidth (default capture2)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_INPUT_CAPTURE) && (defined(BSP_USING_CAPTURE2) || defined(BSP_USING_CAPTURE3))

#include <drivers/rt_inputcapture.h>

struct cap_unit
{
    const char *name;
    const char *pin;
};

static const struct cap_unit g_caps[] =
{
#ifdef BSP_USING_CAPTURE2
    {"capture2", "PA0"},
#endif
#ifdef BSP_USING_CAPTURE3
    {"capture3", "PA6"},
#endif
};

static int cap_test_read(const char *name)
{
    rt_device_t dev;
    struct rt_inputcapture_data data;
    rt_ssize_t n;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("cap_test: %s not found\n", name);
        return -1;
    }

    if (rt_device_open(dev, RT_DEVICE_FLAG_RDONLY) != RT_EOK)
    {
        rt_kprintf("cap_test: open %s failed\n", name);
        return -1;
    }

    n = rt_device_read(dev, 0, &data, 1);
    if (n <= 0)
    {
        rt_kprintf("cap_test: %s no data yet (apply a pulse on the pin)\n", name);
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("cap_test: %s pulsewidth=%lu us (%s)\n",
               name, (unsigned long)data.pulsewidth_us,
               data.is_high ? "high" : "low");

    rt_device_close(dev);
    return 0;
}

static void cap_list(void)
{
    rt_kprintf("=== Input Capture ===\n");
    for (int i = 0; i < (int)(sizeof(g_caps) / sizeof(g_caps[0])); i++)
        rt_kprintf("  %s: pin=%s\n", g_caps[i].name, g_caps[i].pin);
}

static int cap_test(int argc, char **argv)
{
    if (argc < 2)
    {
        cap_list();
        for (int i = 0; i < (int)(sizeof(g_caps) / sizeof(g_caps[0])); i++)
            cap_test_read(g_caps[i].name);
        return 0;
    }

    if (rt_strcmp(argv[1], "read") == 0)
        return cap_test_read((argc >= 3) ? argv[2] : "capture2");

    cap_list();
    return 0;
}
MSH_CMD_EXPORT(cap_test, input capture pulsewidth read);

#endif /* RT_USING_INPUT_CAPTURE */
