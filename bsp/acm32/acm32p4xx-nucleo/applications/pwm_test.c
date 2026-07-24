/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx PWM MSH test
 *
 * MSH:
 *   pwm_test                  list all PWM channels
 *   pwm_test info             channel-to-pin map
 *   pwm_test set <ch> <freq> <duty%>   set freq(Hz) + duty(0-100)
 *   pwm_test en <ch>          enable
 *   pwm_test dis <ch>         disable
 *
 * ch = timer.channel, e.g. "1.1" = TIM1 CH1
 * Connect CH pin to LED or oscilloscope.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_PWM) && (defined(BSP_USING_PWM1) || defined(BSP_USING_PWM2) || \
    defined(BSP_USING_PWM3) || defined(BSP_USING_PWM10))

struct pwm_ch_info
{
    const char *dev;    /* "pwm1" */
    int         tim;    /* 1,2,3,10 */
    int         ch;     /* 1..4 */
    const char *pin;    /* human-readable */
};

static const struct pwm_ch_info g_pwm_chs[] =
{
#ifdef BSP_USING_PWM1_CH1
    {"pwm1", 1, 1, "PE9"},
#endif
#ifdef BSP_USING_PWM1_CH2
    {"pwm1", 1, 2, "PE11"},
#endif
#ifdef BSP_USING_PWM1_CH3
    {"pwm1", 1, 3, "PE13"},
#endif
#ifdef BSP_USING_PWM1_CH4
    {"pwm1", 1, 4, "PE14"},
#endif
#ifdef BSP_USING_PWM2_CH1
    {"pwm2", 2, 1, "PA0"},
#endif
#ifdef BSP_USING_PWM2_CH2
    {"pwm2", 2, 2, "PA1"},
#endif
#ifdef BSP_USING_PWM2_CH3
    {"pwm2", 2, 3, "PA2"},
#endif
#ifdef BSP_USING_PWM2_CH4
    {"pwm2", 2, 4, "PA3"},
#endif
#ifdef BSP_USING_PWM3_CH1
    {"pwm3", 3, 1, "PA6"},
#endif
#ifdef BSP_USING_PWM3_CH2
    {"pwm3", 3, 2, "PA7"},
#endif
#ifdef BSP_USING_PWM3_CH3
    {"pwm3", 3, 3, "PB0"},
#endif
#ifdef BSP_USING_PWM3_CH4
    {"pwm3", 3, 4, "PB1"},
#endif
#ifdef BSP_USING_PWM10_CH1
    {"pwm10", 10, 1, "PF7"},
#endif
};

static int g_pwm_n = (int)(sizeof(g_pwm_chs) / sizeof(g_pwm_chs[0]));

static const struct pwm_ch_info *pwm_find(const char *dev, int ch)
{
    for (int i = 0; i < g_pwm_n; i++)
    {
        if (rt_strcmp(g_pwm_chs[i].dev, dev) == 0 && g_pwm_chs[i].ch == ch)
            return &g_pwm_chs[i];
    }
    if (g_pwm_n > 0) return &g_pwm_chs[0];
    return RT_NULL;
}

static void pwm_test_list(void)
{
    rt_kprintf("=== PWM Channels ===\n");
    for (int i = 0; i < g_pwm_n; i++)
    {
        const struct pwm_ch_info *c = &g_pwm_chs[i];
        rt_kprintf("  %s CH%d -> %s\n", c->dev, c->ch, c->pin);
    }
}

static int pwm_test_set(const char *dev, int ch, rt_uint32_t freq, rt_uint32_t duty)
{
    struct rt_device_pwm *pwm;
    rt_uint32_t period_ns, pulse_ns;

    pwm = (struct rt_device_pwm *)rt_device_find(dev);
    if (pwm == RT_NULL)
    {
        rt_kprintf("pwm_test: %s not found\n", dev);
        return -1;
    }

    if (freq == 0) { rt_kprintf("pwm_test: freq must be >0\n"); return -1; }

    if (duty > 100) duty = 100;
    period_ns = 1000000000UL / freq;
    pulse_ns  = (rt_uint32_t)((rt_uint64_t)period_ns * duty / 100);

    if (rt_pwm_set(pwm, ch, period_ns, pulse_ns) != RT_EOK)
    {
        rt_kprintf("pwm_test: set failed\n");
        return -1;
    }
    rt_pwm_enable(pwm, ch);

    rt_kprintf("pwm_test: %s CH%d freq=%uHz duty=%u%% period=%uns pulse=%uns\n",
               dev, ch, freq, duty, period_ns, pulse_ns);
    return 0;
}

static int pwm_test_enable(const char *dev, int ch, rt_bool_t en)
{
    struct rt_device_pwm *pwm;

    pwm = (struct rt_device_pwm *)rt_device_find(dev);
    if (pwm == RT_NULL)
    {
        rt_kprintf("pwm_test: %s not found\n", dev);
        return -1;
    }

    if (en)
        rt_pwm_enable(pwm, ch);
    else
        rt_pwm_disable(pwm, ch);

    rt_kprintf("pwm_test: %s CH%d %s\n", dev, ch, en ? "on" : "off");
    return 0;
}

static void pwm_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  pwm_test                    list channels\n");
    rt_kprintf("  pwm_test set <ch> <Hz> <duty%%>\n");
    rt_kprintf("  pwm_test en  <ch>           enable\n");
    rt_kprintf("  pwm_test dis <ch>           disable\n");
    rt_kprintf("  <ch> = dev.ch, e.g. pwm1.1 pwm2.3\n");
    rt_kprintf("Examples:\n");
    rt_kprintf("  pwm_test set pwm1.1 1000 50   (1kHz 50%% on PE9)\n");
    rt_kprintf("  pwm_test dis pwm1.1\n");
}

static int pwm_test_parse_ch(const char *s, char *dev_out, int dev_out_size, int *ch_out)
{
    const char *dot = rt_strchr(s, '.');
    rt_size_t len;
    if (dot == RT_NULL) return -1;

    len = (rt_size_t)(dot - s);
    if (len >= (rt_size_t)dev_out_size)
    {
        return -1;
    }

    rt_strncpy(dev_out, s, len);
    dev_out[len] = '\0';
    *ch_out = atoi(dot + 1);
    if (*ch_out < 1 || *ch_out > 4) return -1;
    return 0;
}

static int pwm_test(int argc, char **argv)
{
    if (argc < 2)
    {
        pwm_test_list();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0 || rt_strcmp(argv[1], "list") == 0)
    {
        pwm_test_list();
        return 0;
    }

    if (rt_strcmp(argv[1], "set") == 0 && argc >= 5)
    {
        char dev[16]; int ch;
        if (pwm_test_parse_ch(argv[2], dev, sizeof(dev), &ch) != 0)
        { rt_kprintf("pwm_test: invalid ch '%s', use pwm1.1\n", argv[2]); return -1; }
        return pwm_test_set(dev, ch, (rt_uint32_t)strtoul(argv[3], RT_NULL, 10),
                            (rt_uint32_t)atoi(argv[4]));
    }

    if (rt_strcmp(argv[1], "en") == 0 && argc >= 3)
    {
        char dev[16]; int ch;
        if (pwm_test_parse_ch(argv[2], dev, sizeof(dev), &ch) != 0)
        { rt_kprintf("pwm_test: invalid ch\n"); return -1; }
        return pwm_test_enable(dev, ch, RT_TRUE);
    }

    if (rt_strcmp(argv[1], "dis") == 0 && argc >= 3)
    {
        char dev[16]; int ch;
        if (pwm_test_parse_ch(argv[2], dev, sizeof(dev), &ch) != 0)
        { rt_kprintf("pwm_test: invalid ch\n"); return -1; }
        return pwm_test_enable(dev, ch, RT_FALSE);
    }

    pwm_usage();
    return 0;
}
MSH_CMD_EXPORT(pwm_test, PWM freq/duty set / enable / disable);

#endif /* RT_USING_PWM */
