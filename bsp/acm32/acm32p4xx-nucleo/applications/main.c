/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-09-17     AisinoChip   the first version
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 * 2026-07-23     AisinoChip   move SPI/UART/ETH tests to dedicated files
 */

#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define LED_PIN_NUM    0     /* PA0 - LED on ACM32P4xx Nucleo board */

int main(void)
{
    rt_kprintf("ACM32P4xx-Nucleo BSP boot success!\n");
#ifdef BSP_USING_UART1
    rt_kprintf("Run 'uart_test' / 'uart_test info' / 'uart_test echo uart2'\n");
#endif
#if defined(BSP_USING_SPI1) || defined(BSP_USING_SPI2) || \
    defined(BSP_USING_SPI3) || defined(BSP_USING_SPI4)
    rt_kprintf("Run 'spi_test' / 'spi_test info' / 'spi_test loopback 1' (short MOSI-MISO)\n");
#endif
#ifdef DATA_IN_ExtSRAM
    rt_kprintf("Run 'psram_info' / 'psram_test' / 'psram_speed' for OSPI PSRAM\n");
#endif
#ifdef BSP_USING_ETH
    rt_kprintf("Run 'eth_test' / 'eth_test iperf -s' for Ethernet; eth_ifconfig\n");
#endif

    rt_pin_mode(LED_PIN_NUM, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    }
}
