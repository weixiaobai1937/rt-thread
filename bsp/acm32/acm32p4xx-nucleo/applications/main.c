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
#include <stdlib.h>
#include "board.h"

#define LED_PIN_NUM    0     /* PA0 - LED on ACM32P4xx Nucleo board */

int uart2_echo_test(int argc, char **argv)
{
    rt_device_t dev;
    rt_uint8_t buf[512];
    rt_uint32_t total_rx = 0;
    rt_uint32_t total_tx = 0;
    rt_int32_t timeout_ms = 30000;
    rt_tick_t start;
    rt_tick_t timeout_tick;
    rt_tick_t rx_to;
    rt_err_t ret;

    if (argc >= 2)
        timeout_ms = atoi(argv[1]);
    if (timeout_ms <= 0)
        timeout_ms = 30000;

    dev = rt_device_find("uart2");
    if (dev == RT_NULL)
    {
        rt_kprintf("uart2 not found\n");
        return -1;
    }

    /* V2: DMA 能力在注册 flag 中；open 用 BLOCKING 即可映射到 DMA */
    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_RX_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
    if (ret != RT_EOK)
    {
        rt_kprintf("uart2 open failed: %d\n", ret);
        return -1;
    }

    rx_to = RT_TICK_PER_SECOND / 10;
    rt_device_control(dev, RT_SERIAL_CTRL_SET_RX_TIMEOUT, &rx_to);

    rt_kprintf("UART2 DMA echo: timeout=%d ms, send data on PD6/RX\n", timeout_ms);
    start = rt_tick_get();
    timeout_tick = rt_tick_from_millisecond(timeout_ms);

    while (rt_tick_get() - start < timeout_tick)
    {
        rt_ssize_t n = rt_device_read(dev, -1, buf, sizeof(buf));
        if (n > 0)
        {
            rt_ssize_t w = rt_device_write(dev, -1, buf, n);
            total_rx += (rt_uint32_t)n;
            if (w > 0)
                total_tx += (rt_uint32_t)w;
            rt_kprintf("echo %d bytes (rx=%u tx=%u)\n",
                       (int)n, total_rx, total_tx);
        }
    }

    rt_device_close(dev);
    rt_kprintf("UART2 echo done: rx=%u tx=%u %s\n",
               total_rx, total_tx,
               (total_rx > 0 && total_rx == total_tx) ? "PASS" : "FAIL/idle");
    return (total_rx > 0 && total_rx == total_tx) ? 0 : -1;
}
MSH_CMD_EXPORT(uart2_echo_test, "UART2 DMA echo test [timeout_ms]");

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
