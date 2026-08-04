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
    rt_kprintf("  LPTIM1: hardware initialized, timer NOT started\n");
    rt_kprintf("  Clock: PCLK1, prescaler /128, period 0xFFFF\n");
    rt_kprintf("  Role: stub for future PM tickless / low-power wake\n");
    rt_kprintf("  Status: no rt_device node, no periodic IRQ (by design)\n");
    rt_kprintf("  See drivers/drv_lptimer.c and drivers/Kconfig help.\n");
    return 0;
}
MSH_CMD_EXPORT(lptim_test, LPTIMER status (stub for PM));

#endif /* BSP_USING_LPTIMER */
