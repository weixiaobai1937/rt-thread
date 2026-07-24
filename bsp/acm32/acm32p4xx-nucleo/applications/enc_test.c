/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx Pulse Encoder test
 *
 * MSH:
 *   enc_test                  list encoder devices
 *   enc_test count [dev]      read count (default enc2)
 *   enc_test clear [dev]      clear count
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_PULSE_ENCODER) && (defined(BSP_USING_PULSE_ENCODER2) || defined(BSP_USING_PULSE_ENCODER3))

struct enc_unit
{
    const char *name;
    const char *a_pin;
    const char *b_pin;
};

static const struct enc_unit g_encs[] =
{
#ifdef BSP_USING_PULSE_ENCODER2
    {"enc2", "PA0", "PA1"},
#endif
#ifdef BSP_USING_PULSE_ENCODER3
    {"enc3", "PA6", "PA7"},
#endif
};

static int enc_test_count(const char *name)
{
    rt_device_t dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("enc_test: %s not found\n", name);
        return -1;
    }

    rt_int32_t cnt = rt_device_read(dev, 0, RT_NULL, 0);
    rt_kprintf("enc_test: %s count=%ld\n", name, (long)cnt);
    return 0;
}

static int enc_test_clear(const char *name)
{
    rt_device_t dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("enc_test: %s not found\n", name);
        return -1;
    }

    rt_device_control(dev, PULSE_ENCODER_CMD_CLEAR_COUNT, RT_NULL);
    rt_kprintf("enc_test: %s cleared\n", name);
    return 0;
}

static void enc_list(void)
{
    rt_kprintf("=== Pulse Encoders ===\n");
    for (int i = 0; i < (int)(sizeof(g_encs) / sizeof(g_encs[0])); i++)
        rt_kprintf("  %s: A=%s B=%s\n", g_encs[i].name, g_encs[i].a_pin, g_encs[i].b_pin);
}

static int enc_test(int argc, char **argv)
{
    if (argc < 2)
    {
        enc_list();
        for (int i = 0; i < (int)(sizeof(g_encs) / sizeof(g_encs[0])); i++)
            enc_test_count(g_encs[i].name);
        return 0;
    }

    if (rt_strcmp(argv[1], "count") == 0)
        return enc_test_count((argc >= 3) ? argv[2] : "enc2");

    if (rt_strcmp(argv[1], "clear") == 0)
        return enc_test_clear((argc >= 3) ? argv[2] : "enc2");

    enc_list();
    return 0;
}
MSH_CMD_EXPORT(enc_test, pulse encoder count / clear);

#endif /* RT_USING_PULSE_ENCODER */
