/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx clock_timer MSH test
 * 2026-08-04     AisinoChip   tick measures hardware timer count
 *
 * MSH:
 *   timer_test                 list all timers
 *   timer_test info [name]     detail for one timer (default timer1)
 *   timer_test freq <name> <hz>  set frequency
 *   timer_test tick <name> [ms]  measure hardware counter over delay
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_CLOCK_TIME) && (defined(BSP_USING_TIM1) || defined(BSP_USING_TIM2) || \
    defined(BSP_USING_TIM3) || defined(BSP_USING_TIM6) || defined(BSP_USING_TIM10))

#include <drivers/clock_time.h>

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

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("timer_test: open %s failed\n", name);
        return -1;
    }

    if (rt_device_control(dev, CLOCK_TIMER_CTRL_FREQ_SET, &freq) == RT_EOK)
    {
        rt_kprintf("timer_test: %s freq set to %u Hz\n", name, (unsigned)freq);
        rt_device_close(dev);
        return 0;
    }
    rt_kprintf("timer_test: freq set failed\n");
    rt_device_close(dev);
    return -1;
}

/*
 * Measure hardware timer elapsed time via device read (count_get + overflow).
 * Starts a long period so the free-running counter advances during mdelay.
 */
static int timer_test_tick(const char *name, int ms)
{
    rt_device_t dev;
    rt_clock_timerval_t t0, t1, period;
    rt_uint32_t freq = 1000000U; /* 1 MHz count clock */
    rt_int64_t us0, us1, dus;
    rt_err_t err;

    dev = rt_device_find(name);
    if (dev == RT_NULL)
    {
        rt_kprintf("timer_test: %s not found\n", name);
        return -1;
    }

    if (ms < 100)
        ms = 100;

    if (rt_device_open(dev, RT_DEVICE_OFLAG_RDWR) != RT_EOK)
    {
        rt_kprintf("timer_test: open %s failed\n", name);
        return -1;
    }

    rt_device_control(dev, CLOCK_TIMER_CTRL_FREQ_SET, &freq);

    /* Period ~1s at 1 MHz so overflow is rare during short mdelay */
    period.sec = 1;
    period.usec = 0;
    if (rt_device_write(dev, 0, &period, sizeof(period)) == 0)
    {
        rt_kprintf("timer_test: start %s failed\n", name);
        rt_device_close(dev);
        return -1;
    }

    if (rt_device_read(dev, 0, &t0, sizeof(t0)) != sizeof(t0))
    {
        rt_kprintf("timer_test: read t0 failed\n");
        rt_device_control(dev, CLOCK_TIMER_CTRL_STOP, RT_NULL);
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("timer_test: %s HW measure ~%d ms (freq=%u)...\n",
               name, ms, (unsigned)freq);
    rt_thread_mdelay(ms);

    if (rt_device_read(dev, 0, &t1, sizeof(t1)) != sizeof(t1))
    {
        rt_kprintf("timer_test: read t1 failed\n");
        rt_device_control(dev, CLOCK_TIMER_CTRL_STOP, RT_NULL);
        rt_device_close(dev);
        return -1;
    }

    us0 = (rt_int64_t)t0.sec * 1000000 + t0.usec;
    us1 = (rt_int64_t)t1.sec * 1000000 + t1.usec;
    dus = us1 - us0;
    if (dus < 0)
        dus = -dus;

    rt_kprintf("timer_test: %s HW elapsed %ld us (expect ~%d000 us)\n",
               name, (long)dus, ms);
    rt_kprintf("timer_test: systick window %d ms (for comparison)\n", ms);

    err = rt_device_control(dev, CLOCK_TIMER_CTRL_STOP, RT_NULL);
    RT_UNUSED(err);
    rt_device_close(dev);
    return 0;
}

static void timer_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  timer_test                   list timers\n");
    rt_kprintf("  timer_test info [name]       info (default timer1)\n");
    rt_kprintf("  timer_test freq <name> <Hz>  set frequency\n");
    rt_kprintf("  timer_test tick <name> [ms]  measure HW counter over delay\n");
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
        int ms = (argc >= 4) ? atoi(argv[3]) : 1000;
        return timer_test_tick(argv[2], ms);
    }

    timer_usage();
    return 0;
}
MSH_CMD_EXPORT(timer_test, clock_timer list / freq / tick);

#endif /* RT_USING_CLOCK_TIME && BSP_USING_TIMx */
