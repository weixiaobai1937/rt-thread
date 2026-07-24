/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx ADC MSH test
 *
 * MSH:
 *   adc_test                  read all external channels (CH1..CH19)
 *   adc_test info             channel-to-pin map
 *   adc_test ch <N> [cnt]     read channel N repeatedly (default 10)
 *   adc_test vref             read VREFINT channel (12)
 *   adc_test temp             read temperature (TS, CH15)
 *
 * Voltage: Vref = 3.3V, 12-bit => LSB = 3.3 / 4096 ≈ 0.806 mV
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_ADC) && defined(BSP_USING_ADC)

#define ADC_DEV_NAME    "adc1"
#define ADC_BITS        12
#define ADC_MAX_VAL     ((1U << ADC_BITS) - 1)

static const rt_uint32_t adc_ext_channels[] = {1,2,3,4,5,6,7,8,9,10,11,14,16,17,18,19};
static const int adc_ext_n = (int)(sizeof(adc_ext_channels) / sizeof(adc_ext_channels[0]));

static void adc_test_info(void)
{
    rt_kprintf("=== ADC Device ===\n");
    rt_kprintf("  device: %s, %d-bit\n", ADC_DEV_NAME, ADC_BITS);
    rt_kprintf("  external channels: ");
    for (int i = 0; i < adc_ext_n; i++)
        rt_kprintf("CH%u ", adc_ext_channels[i]);
    rt_kprintf("\n");
}

static void adc_test_channel(rt_uint32_t ch, int cnt)
{
    rt_adc_device_t adc;
    rt_uint32_t min_val, max_val, sum;

    adc = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc == RT_NULL)
    {
        rt_kprintf("adc_test: %s not found\n", ADC_DEV_NAME);
        return;
    }

    rt_adc_enable(adc, ch);

    min_val = ADC_MAX_VAL;
    max_val = 0;
    sum = 0;

    for (int i = 0; i < cnt; i++)
    {
        rt_uint32_t v = rt_adc_read(adc, ch);
        if (v < min_val) min_val = v;
        if (v > max_val) max_val = v;
        sum += v;
        rt_thread_mdelay(1);
    }

    rt_adc_disable(adc, ch);

    rt_kprintf("  CH%-2u: avg=%4u (%5.3fV)  min=%4u  max=%4u  (cnt=%d)\n",
               ch, sum / cnt,
               (float)(sum / cnt) * 3.3f / ADC_MAX_VAL,
               min_val, max_val, cnt);
}

static int adc_test_read(rt_uint32_t ch, int cnt)
{
    rt_adc_device_t adc;

    adc = (rt_adc_device_t)rt_device_find(ADC_DEV_NAME);
    if (adc == RT_NULL)
    {
        rt_kprintf("adc_test: %s not found\n", ADC_DEV_NAME);
        return -1;
    }

    rt_adc_enable(adc, ch);
    rt_kprintf("adc_test: CH%u", ch);
    for (int i = 0; i < cnt; i++)
    {
        rt_uint32_t v = rt_adc_read(adc, ch);
        rt_kprintf(" %u", v);
        if (i < cnt - 1) rt_thread_mdelay(10);
    }
    rt_kprintf("\n");
    rt_adc_disable(adc, ch);
    return 0;
}

static void adc_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  adc_test                  scan all external channels\n");
    rt_kprintf("  adc_test info             channel map\n");
    rt_kprintf("  adc_test ch <N> [cnt=10]  read channel N\n");
    rt_kprintf("  adc_test vref             VREFINT (CH12)\n");
    rt_kprintf("  adc_test temp             temperature (CH15)\n");
}

static int adc_test(int argc, char **argv)
{
    if (argc < 2)
    {
        adc_test_info();
        rt_kprintf("\n=== Scan all external channels ===\n");
        for (int i = 0; i < adc_ext_n; i++)
            adc_test_channel(adc_ext_channels[i], 4);
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        adc_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "ch") == 0 && argc >= 3)
    {
        rt_uint32_t ch = (rt_uint32_t)atoi(argv[2]);
        int cnt = (argc >= 4) ? atoi(argv[3]) : 10;
        if (ch > 19) { rt_kprintf("adc_test: channel 1..19\n"); return -1; }
        return adc_test_read(ch, cnt);
    }

    if (rt_strcmp(argv[1], "vref") == 0)
    {
        return adc_test_read(12, 1);
    }

    if (rt_strcmp(argv[1], "temp") == 0)
    {
        return adc_test_read(15, 1);
    }

    adc_usage();
    return 0;
}
MSH_CMD_EXPORT(adc_test, ADC read / channel scan);

#endif /* RT_USING_ADC && BSP_USING_ADC */
