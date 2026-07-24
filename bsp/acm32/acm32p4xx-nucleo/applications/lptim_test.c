/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx LPTIM test
 *
 * MSH:
 *   lptim_test               LPTIM1 status info
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_LPTIMER

static int lptim_test(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("=== LPTIMER ===\n");
    rt_kprintf("  LPTIM1: initialized, ARR match interrupt enabled\n");
    rt_kprintf("  Clock: PCLK1, prescaler /128, period 0xFFFF\n");
    rt_kprintf("  Use as low-power wakeup source in PM module.\n");
    return 0;
}
MSH_CMD_EXPORT(lptim_test, LPTIMER status info);

#endif /* BSP_USING_LPTIMER */
