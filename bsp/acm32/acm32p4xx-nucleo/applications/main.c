/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-17     AisinoChip   the first version
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 * 2026-07-13     AisinoChip   UART2 DMA echo test
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define LED_PIN_NUM    0     /* PA0 - LED on ACM32P4xx Nucleo board */

int uart2_echo_test(void)
{
    rt_device_t dev = rt_device_find("uart2");
    if (dev == RT_NULL)
    {
        rt_kprintf("uart2 not found\n");
        return -1;
    }

    rt_err_t ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_DMA_RX);
    if (ret != RT_EOK)
    {
        rt_kprintf("uart2 open failed: %d\n", ret);
        return -1;
    }

    rt_uint8_t buf[512];

    while (1)
    {
        rt_ssize_t n = rt_device_read(dev, -1, buf, sizeof(buf));
        if (n > 0)
        {
            rt_device_write(dev, -1, buf, n);
        }
    }
}
MSH_CMD_EXPORT(uart2_echo_test, "UART2 DMA echo test: receive and send back");

int main(void)
{
    rt_kprintf("ACM32P4xx-Nucleo BSP boot success!\n");
    rt_kprintf("Run 'uart2_echo_test' to test UART2 DMA echo\n");

    rt_pin_mode(LED_PIN_NUM, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    }
}
