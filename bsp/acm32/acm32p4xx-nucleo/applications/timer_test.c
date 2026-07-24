/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx clock_timer MSH test
 *
 * MSH:
 *   timer_test                 list all timers
 *   timer_test info [name]     detail for one timer (default timer1)
 *   timer_test freq <name> <hz>  set frequency
 *   timer_test tick <name> [ms]  count 1s ticks from timer
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_CLOCK_TIME) && (defined(BSP_USING_TIM1) || defined(BSP_USING_TIM2) || \
    defined(BSP_USING_TIM3) || defined(BSP_USING_TIM6) || defined(BSP_USING_TIM10))

struct timer_unit
{
    int         unit;
    const char *name;
    rt_bool_t   is_32bit;
};

static const struct timer_unit g_timers[] =
{
#ifdef BSP_USING_TIM1
    {1, "timer1", RT_FALSE},
#endif
#ifdef BSP_USING_TIM2
    {2, "timer2", RT_TRUE},
#endif
#ifdef BSP_USING_TIM3
    {3, "timer3", RT_FALSE},
#endif
#ifdef BSP_USING_TIM6
    {6, "timer6", RT_FALSE},
#endif
#ifdef BSP_USING_TIM10
    {10, "timer10", RT_FALSE},
#endif
};

static int g_timer_n = (int)(sizeof(g_timers) / sizeof(g_timers[0]));

static const struct timer_unit *find_timer(const char *name)
{
    for (int i = 0; i < g_timer_n; i++)
    {
        if (rt_strcmp(g_timers[i].name, name) == 0)
            return &g_timers[i];
    }
    if (g_timer_n > 0)
        return &g_timers[0];
    return RT_NULL;
}

static void timer_test_list(void)
{
    rt_kprintf("=== Clock Timers ===\n");
    for (int i = 0; i < g_timer_n; i++)
    {
        const struct timer_unit *u = &g_timers[i];
        rt_kprintf("  %s: TIM%d %s\n", u->name, u->unit,
                   u->is_32bit ? "32-bit" : "16-bit");
    }
}

static void timer_test_info(const char *name)
{
    const struct timer_unit *u;
    rt_device_t dev;

    u = find_timer(name);
    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("timer_test: %s not found\n", name);
        return;
    }

    rt_kprintf("=== Timer: %s ===\n", name);
    rt_kprintf("  TIM%d %s\n", u->unit, u->is_32bit ? "32-bit" : "16-bit");

    {
        struct rt_clock_timer_info devinfo;
        if (rt_device_control(dev, CLOCK_TIMER_CTRL_INFO_GET, &devinfo) == RT_EOK)
        {
            rt_kprintf("  maxfreq=%d Hz  minfreq=%d Hz  maxcnt=0x%08X\n",
                       devinfo.maxfreq, devinfo.minfreq, devinfo.maxcnt);
        }
    }
}

static int timer_test_freq(const char *name, rt_uint32_t freq)
{
    rt_device_t dev;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("timer_test: %s not found\n", name);
        return -1;
    }

    if (rt_device_control(dev, CLOCK_TIMER_CTRL_FREQ_SET, &freq) == RT_EOK)
    {
        rt_kprintf("timer_test: %s freq set to %u Hz\n", name, freq);
        return 0;
    }
    rt_kprintf("timer_test: freq set failed\n");
    return -1;
}

static int timer_test_tick(const char *name, int ms)
{
    rt_device_t dev;
    rt_tick_t start_tick, end_tick;
    rt_tick_t timeout;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("timer_test: %s not found\n", name);
        return -1;
    }

    if (ms < 100) ms = 100;

    /* get current counter before and after */
    rt_kprintf("timer_test: %s measuring ~%dms...\n", name, ms);
    start_tick = rt_tick_get();
    rt_thread_mdelay(ms);
    end_tick = rt_tick_get();

    rt_kprintf("timer_test: %s elapsed %u ticks (~%d ms)\n",
               name, (rt_uint32_t)(end_tick - start_tick), ms);
    return 0;
}

static void timer_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  timer_test                   list timers\n");
    rt_kprintf("  timer_test info [name]       info (default timer1)\n");
    rt_kprintf("  timer_test freq <name> <Hz>  set frequency\n");
    rt_kprintf("  timer_test tick <name> [ms]  measure delay with systick\n");
}

static int timer_test(int argc, char **argv)
{
    if (argc < 2)
    {
        timer_test_list();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        const char *name = (argc >= 3) ? argv[2] : "timer1";
        timer_test_info(name);
        return 0;
    }

    if (rt_strcmp(argv[1], "freq") == 0 && argc >= 4)
    {
        rt_uint32_t freq = (rt_uint32_t)strtoul(argv[3], RT_NULL, 10);
        return timer_test_freq(argv[2], freq);
    }

    if (rt_strcmp(argv[1], "tick") == 0 && argc >= 3)
    {
        int ms = (argc >= 4) ? atoi(argv[4]) : 1000;
        return timer_test_tick(argv[2], ms);
    }

    timer_usage();
    return 0;
}
MSH_CMD_EXPORT(timer_test, clock_timer list / freq / tick);

#endif /* RT_USING_CLOCK_TIME && BSP_USING_TIMx */
