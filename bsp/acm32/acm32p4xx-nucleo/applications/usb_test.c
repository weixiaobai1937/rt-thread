/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx USB Device test
 *
 * MSH:
 *   usb_test                  USB device status info
 */

#include <rtthread.h>
#include <rtdevice.h>

#ifdef BSP_USING_FSUSB

static int usb_test(int argc, char **argv)
{
    (void)argc; (void)argv;
    rt_kprintf("=== USB Device ===\n");
    rt_kprintf("  FSUSB: skeleton driver loaded (TODO: endpoint logic)\n");
    rt_kprintf("  Pins: DM=PA11 DP=PA12 AF10\n");
    return 0;
}
MSH_CMD_EXPORT(usb_test, USB device status info);

#endif /* BSP_USING_FSUSB */
