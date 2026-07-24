/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx Input Capture test
 *
 * MSH:
 *   cap_test                  list capture devices
 *   cap_test read [dev]       read pulsewidth (default cap2)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_INPUT_CAPTURE) && (defined(BSP_USING_CAPTURE2) || defined(BSP_USING_CAPTURE3))

struct cap_unit
{
    const char *name;
    const char *pin;
};

static const struct cap_unit g_caps[] =
{
#ifdef BSP_USING_CAPTURE2
    {"cap2", "PA0"},
#endif
#ifdef BSP_USING_CAPTURE3
    {"cap3", "PA6"},
#endif
};

static int cap_test_read(const char *name)
{
    rt_device_t dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("cap_test: %s not found\n", name);
        return -1;
    }

    rt_uint32_t pw_us = 0;
    rt_err_t ret = rt_device_control(dev, 0, &pw_us);
    (void)ret;

    rt_kprintf("cap_test: %s pulsewidth=%lu us\n", name, (unsigned long)pw_us);
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
        return cap_test_read((argc >= 3) ? argv[2] : "cap2");

    cap_list();
    return 0;
}
MSH_CMD_EXPORT(cap_test, input capture pulsewidth read);

#endif /* RT_USING_INPUT_CAPTURE */
