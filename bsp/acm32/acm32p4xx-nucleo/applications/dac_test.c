/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx DAC MSH test
 *
 * MSH:
 *   dac_test                  info
 *   dac_test out <ch> <mv>    output DC voltage on CH1(PA4) or CH2(PA5)
 *   dac_test sweep <ch>       ramp 0..3300mV step 100mV
 *   dac_test stop <ch>        disable channel
 *
 * Vref = 3.3V, 12-bit => LSB ≈ 0.806 mV
 * Connect PA4 / PA5 to voltmeter or ADC input for verification.
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_DAC) && defined(BSP_USING_DAC)

#define DAC_DEV_NAME    "dac1"
#define DAC_BITS        12
#define DAC_MAX_VAL     ((1U << DAC_BITS) - 1)

static void dac_test_info(void)
{
    rt_kprintf("=== DAC Device ===\n");
    rt_kprintf("  device: %s, %d-bit, Vref=3.3V\n", DAC_DEV_NAME, DAC_BITS);
    rt_kprintf("  CH1: PA4   CH2: PA5\n");
}

static int dac_mv_to_val(int mv)
{
    if (mv < 0) mv = 0;
    if (mv > 3300) mv = 3300;
    return (int)((rt_uint32_t)mv * DAC_MAX_VAL / 3300U);
}

static int dac_test_out(rt_uint32_t ch, int mv)
{
    rt_dac_device_t dac;
    rt_uint32_t val;

    dac = (rt_dac_device_t)rt_device_find(DAC_DEV_NAME);
    if (dac == RT_NULL)
    {
        rt_kprintf("dac_test: %s not found\n", DAC_DEV_NAME);
        return -1;
    }
    if (ch != 1 && ch != 2)
    {
        rt_kprintf("dac_test: channel must be 1 or 2\n");
        return -1;
    }

    val = (rt_uint32_t)dac_mv_to_val(mv);

    rt_dac_enable(dac, ch);
    rt_dac_write(dac, ch, val);
    rt_kprintf("dac_test: CH%u = %u (%d mV)\n", ch, val, mv);
    return 0;
}

static int dac_test_sweep(rt_uint32_t ch)
{
    rt_dac_device_t dac;

    dac = (rt_dac_device_t)rt_device_find(DAC_DEV_NAME);
    if (dac == RT_NULL)
    {
        rt_kprintf("dac_test: %s not found\n", DAC_DEV_NAME);
        return -1;
    }
    if (ch != 1 && ch != 2)
    {
        rt_kprintf("dac_test: channel must be 1 or 2\n");
        return -1;
    }

    rt_dac_enable(dac, ch);
    rt_kprintf("dac_test: CH%u sweep 0..3300 mV step 100 mV...\n", ch);
    for (int mv = 0; mv <= 3300; mv += 100)
    {
        rt_uint32_t val = (rt_uint32_t)dac_mv_to_val(mv);
        rt_dac_write(dac, ch, val);
        rt_kprintf("  %d mV (%u)\n", mv, val);
        rt_thread_mdelay(200);
    }
    rt_kprintf("dac_test: sweep done\n");
    return 0;
}

static int dac_test_stop(rt_uint32_t ch)
{
    rt_dac_device_t dac;

    dac = (rt_dac_device_t)rt_device_find(DAC_DEV_NAME);
    if (dac == RT_NULL)
    {
        rt_kprintf("dac_test: %s not found\n", DAC_DEV_NAME);
        return -1;
    }
    if (ch != 1 && ch != 2)
    {
        rt_kprintf("dac_test: channel must be 1 or 2\n");
        return -1;
    }

    rt_dac_disable(dac, ch);
    rt_kprintf("dac_test: CH%u disabled\n", ch);
    return 0;
}

static void dac_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  dac_test                   info\n");
    rt_kprintf("  dac_test out  <ch> <mv>    output DC (mV), ch=1|2\n");
    rt_kprintf("  dac_test sweep <ch>        ramp 0-3300mV step 100mV\n");
    rt_kprintf("  dac_test stop  <ch>        disable channel\n");
}

static int dac_test(int argc, char **argv)
{
    if (argc < 2)
    {
        dac_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        dac_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "out") == 0 && argc >= 4)
    {
        rt_uint32_t ch = (rt_uint32_t)atoi(argv[2]);
        int mv = atoi(argv[3]);
        if (ch != 1 && ch != 2) { rt_kprintf("dac_test: ch must be 1 or 2\n"); return -1; }
        if (mv < 0 || mv > 3300) { rt_kprintf("dac_test: mv range 0..3300\n"); return -1; }
        return dac_test_out(ch, mv);
    }

    if (rt_strcmp(argv[1], "sweep") == 0 && argc >= 3)
    {
        rt_uint32_t ch = (rt_uint32_t)atoi(argv[2]);
        return dac_test_sweep(ch);
    }

    if (rt_strcmp(argv[1], "stop") == 0 && argc >= 3)
    {
        rt_uint32_t ch = (rt_uint32_t)atoi(argv[2]);
        return dac_test_stop(ch);
    }

    dac_usage();
    return 0;
}
MSH_CMD_EXPORT(dac_test, DAC output voltage test);

#endif /* RT_USING_DAC && BSP_USING_DAC */
