/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx RTC MSH test
 *
 * MSH:
 *   rtc_test               read current date/time
 *   rtc_test info           device info
 *   rtc_test set <YYYY-MM-DD HH:MM:SS>   set date & time
 *   rtc_test tick           1s tick loop (Ctrl+C to stop)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef BSP_USING_RTC

#define RTC_DEV_NAME    "rtc"

static void rtc_test_info(void)
{
    rt_device_t dev;

    dev = rt_device_find(RTC_DEV_NAME);
    if (dev == RT_NULL)
    {
        rt_kprintf("rtc_test: %s not found\n", RTC_DEV_NAME);
        return;
    }
    rt_kprintf("=== RTC Device ===\n");
    rt_kprintf("  device: %s\n", RTC_DEV_NAME);
    rt_kprintf("  clock: XTL 32.768kHz (default)\n");
}

static void rtc_test_read(void)
{
    time_t now;
    struct tm t;

    now = time(RT_NULL);
    if (now == 0)
    {
        rt_kprintf("rtc_test: RTC not initialized (timestamp=0)\n");
        return;
    }
    gmtime_r(&now, &t);
    rt_kprintf("rtc_test: %04d-%02d-%02d %02d:%02d:%02d UTC (epoch=%llu)\n",
               t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
               t.tm_hour, t.tm_min, t.tm_sec,
               (unsigned long long)now);
}

static int rtc_test_set(const char *datetime)
{
    int year, mon, day, hour, min, sec;
    struct tm t;
    time_t ts;

    if (sscanf(datetime, "%d-%d-%d %d:%d:%d",
               &year, &mon, &day, &hour, &min, &sec) != 6)
    {
        rt_kprintf("rtc_test: format YYYY-MM-DD HH:MM:SS\n");
        return -1;
    }

    rt_memset(&t, 0, sizeof(t));
    t.tm_year = year - 1900;
    t.tm_mon  = mon - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min  = min;
    t.tm_sec  = sec;

    ts = timegm(&t);
    /* timegm returns -1 on error */
    if (ts == (time_t)-1)
    {
        rt_kprintf("rtc_test: invalid date/time\n");
        return -1;
    }

    {
        rt_device_t dev = rt_device_find(RTC_DEV_NAME);
        struct timeval tv = { .tv_sec = ts, .tv_usec = 0 };
        if (rt_device_control(dev, RT_DEVICE_CTRL_RTC_SET_TIMEVAL, &tv) != RT_EOK)
        {
            rt_kprintf("rtc_test: set time failed\n");
            return -1;
        }
    }

    rt_kprintf("rtc_test: set to %04d-%02d-%02d %02d:%02d:%02d (epoch=%llu)\n",
               year, mon, day, hour, min, sec, (unsigned long long)ts);
    rtc_test_read();
    return 0;
}

static int rtc_test_tick(void)
{
    rt_kprintf("rtc_test: 1s tick (press any key or Ctrl+C to stop)...\n");
    for (int i = 0; i < 60; i++)
    {
        time_t now = time(RT_NULL);
        struct tm t;
        gmtime_r(&now, &t);
        rt_kprintf("\r%04d-%02d-%02d %02d:%02d:%02d",
                   t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
                   t.tm_hour, t.tm_min, t.tm_sec);
        rt_thread_mdelay(1000);
    }
    rt_kprintf("\nrtc_test: tick done\n");
    return 0;
}

static void rtc_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  rtc_test                           read current time\n");
    rt_kprintf("  rtc_test info                      device info\n");
    rt_kprintf("  rtc_test set YYYY-MM-DD HH:MM:SS   set time\n");
    rt_kprintf("  rtc_test tick                      60-second tick\n");
}

static int rtc_test(int argc, char **argv)
{
    if (argc < 2)
    {
        rtc_test_info();
        rtc_test_read();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        rtc_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "set") == 0 && argc >= 3)
    {
        char buf[32];
        rt_snprintf(buf, sizeof(buf), "%s %s", argv[2], (argc >= 4) ? argv[3] : "00:00:00");
        return rtc_test_set(buf);
    }

    if (rt_strcmp(argv[1], "tick") == 0)
    {
        return rtc_test_tick();
    }

    rtc_usage();
    return 0;
}
MSH_CMD_EXPORT(rtc_test, RTC date/time read/set/tick);

#endif /* BSP_USING_RTC */
