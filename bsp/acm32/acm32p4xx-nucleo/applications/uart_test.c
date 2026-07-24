/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * UART1~4 / LPUART1~2 MSH tests (Serial V2 + optional DMA).
 *
 * Echo needs TX-RX loopback wire (or external USB-UART peer).
 * uart1 is usually console — echo on uart1 fights MSH.
 *
 * MSH:
 *   uart_test                    reopen+write all enabled ports
 *   uart_test info
 *   uart_test write [name] [n]   TX pattern (default 64 B)
 *   uart_test echo [name] [ms]   echo until timeout (default 30s)
 *   uart_test reopen [name]
 *   uart_test all [ms]           +echo non-console when ms>0
 *
 * name: uart1 uart2 uart3 uart4 lpuart1 lpuart2
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>
#include "uart_config.h"

#if defined(BSP_USING_UART1) || defined(BSP_USING_UART2) || \
    defined(BSP_USING_UART3) || defined(BSP_USING_UART4) || \
    defined(BSP_USING_LPUART1) || defined(BSP_USING_LPUART2)

#define UART_TEST_BUF           512
#define UART_TEST_WRITE_DEF     64
#define UART_TEST_ECHO_DEF_MS   30000

struct uart_test_unit
{
    const char *name;
    const char *pins_hint;
    rt_bool_t   dma_en;
    rt_bool_t   is_console_hint;
};

static const struct uart_test_unit g_uart_units[] =
{
#ifdef BSP_USING_UART1
    {
        .name = "uart1",
        .pins_hint = "TX/RX (Kconfig pin)",
#ifdef BSP_USING_UART1_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
        .is_console_hint = RT_TRUE,
    },
#endif
#ifdef BSP_USING_UART2
    {
        .name = "uart2",
        .pins_hint = "TX/RX (Kconfig pin)",
#ifdef BSP_USING_UART2_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
        .is_console_hint = RT_FALSE,
    },
#endif
#ifdef BSP_USING_UART3
    {
        .name = "uart3",
        .pins_hint = "TX/RX (Kconfig pin)",
#ifdef BSP_USING_UART3_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
        .is_console_hint = RT_FALSE,
    },
#endif
#ifdef BSP_USING_UART4
    {
        .name = "uart4",
        .pins_hint = "TX/RX (Kconfig pin)",
#ifdef BSP_USING_UART4_DMA
        .dma_en = RT_TRUE,
#else
        .dma_en = RT_FALSE,
#endif
        .is_console_hint = RT_FALSE,
    },
#endif
#ifdef BSP_USING_LPUART1
    {
        .name = "lpuart1",
        .pins_hint = "TX/RX (Kconfig pin)",
        .dma_en = RT_FALSE,
        .is_console_hint = RT_FALSE,
    },
#endif
#ifdef BSP_USING_LPUART2
    {
        .name = "lpuart2",
        .pins_hint = "TX/RX (Kconfig pin)",
        .dma_en = RT_FALSE,
        .is_console_hint = RT_FALSE,
    },
#endif
};

static rt_bool_t uart_test_is_console(const char *name)
{
    return (rt_bool_t)(strcmp(name, RT_CONSOLE_DEVICE_NAME) == 0);
}

static const struct uart_test_unit *uart_test_find(const char *name)
{
    rt_size_t i;

    if (name == RT_NULL)
        return RT_NULL;
    for (i = 0; i < sizeof(g_uart_units) / sizeof(g_uart_units[0]); i++)
    {
        if (!strcmp(g_uart_units[i].name, name))
            return &g_uart_units[i];
    }
    return RT_NULL;
}

static void uart_test_print_dma(const char *name)
{
#ifdef BSP_USING_UART1_DMA
    if (!strcmp(name, "uart1"))
    {
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d\n",
                   BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH,
                   BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH);
        return;
    }
#endif
#ifdef BSP_USING_UART2_DMA
    if (!strcmp(name, "uart2"))
    {
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d\n",
                   BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH,
                   BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH);
        return;
    }
#endif
#ifdef BSP_USING_UART3_DMA
    if (!strcmp(name, "uart3"))
    {
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d\n",
                   BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH,
                   BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH);
        return;
    }
#endif
#ifdef BSP_USING_UART4_DMA
    if (!strcmp(name, "uart4"))
    {
        rt_kprintf("    DMA TX=DMA%d_CH%d RX=DMA%d_CH%d\n",
                   BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH,
                   BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH);
        return;
    }
#endif
    (void)name;
    rt_kprintf("    DMA: off (INT path)\n");
}

static void uart_test_info(void)
{
    rt_size_t i;

    rt_kprintf("UART test units (enabled in Kconfig):\n");
    rt_kprintf("  console device: %s\n", RT_CONSOLE_DEVICE_NAME);

    if (sizeof(g_uart_units) / sizeof(g_uart_units[0]) == 0)
    {
        rt_kprintf("  (none)\n");
        return;
    }

    for (i = 0; i < sizeof(g_uart_units) / sizeof(g_uart_units[0]); i++)
    {
        const struct uart_test_unit *u = &g_uart_units[i];
        rt_device_t dev = rt_device_find(u->name);
        rt_bool_t is_con = uart_test_is_console(u->name) || u->is_console_hint;

        rt_kprintf("  %s  %s  %s%s\n",
                   u->name,
                   dev ? "DEV_OK" : "DEV_MISSING",
                   u->pins_hint,
                   is_con ? "  [console]" : "");
        if (u->dma_en)
            uart_test_print_dma(u->name);
        else
            rt_kprintf("    DMA: off\n");
    }
    rt_kprintf("echo needs TX-RX short (or peer); avoid console for echo\n");
}

static rt_err_t uart_test_open(rt_device_t dev)
{
    return rt_device_open(dev,
                         RT_DEVICE_OFLAG_RDWR |
                         RT_DEVICE_FLAG_RX_BLOCKING |
                         RT_DEVICE_FLAG_TX_BLOCKING);
}

static int uart_test_do_reopen(const struct uart_test_unit *u)
{
    rt_device_t dev;
    rt_err_t ret;
    int pass = 1;
    rt_bool_t is_console;

    if (u == RT_NULL)
        return -1;

    dev = rt_device_find(u->name);
    if (dev == RT_NULL)
    {
        rt_kprintf("[uart_test] %s not found FAIL\n", u->name);
        return -1;
    }

    is_console = uart_test_is_console(u->name);
    rt_kprintf("[uart_test] %s reopen (%s)%s\n",
               u->name, u->pins_hint, u->dma_en ? " DMA" : "");

    if (!is_console)
        rt_device_close(dev);

    ret = uart_test_open(dev);
    if (ret != RT_EOK)
    {
        /* console may already be open — treat as soft ok if open says busy */
        if (is_console)
        {
            rt_kprintf("  console already open (ok)\n");
        }
        else
        {
            rt_kprintf("  open failed: %d\n", ret);
            pass = 0;
        }
    }
    else if (!is_console)
    {
        rt_device_close(dev);
        ret = uart_test_open(dev);
        if (ret != RT_EOK)
        {
            rt_kprintf("  reopen failed: %d\n", ret);
            pass = 0;
        }
        else
            rt_device_close(dev);
    }
    else
    {
        /* leave console open */
    }

    rt_kprintf("[uart_test] %s reopen %s\n", u->name, pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}

static int uart_test_do_write(const struct uart_test_unit *u, int count)
{
    rt_device_t dev;
    static rt_uint8_t buf[UART_TEST_BUF];
    rt_err_t ret;
    rt_ssize_t n, total = 0;
    int i, pass = 1;
    rt_bool_t is_console;

    if (u == RT_NULL)
        return -1;
    if (count <= 0)
        count = UART_TEST_WRITE_DEF;
    if (count > UART_TEST_BUF)
        count = UART_TEST_BUF;

    dev = rt_device_find(u->name);
    if (dev == RT_NULL)
    {
        rt_kprintf("[uart_test] %s not found FAIL\n", u->name);
        return -1;
    }

    is_console = uart_test_is_console(u->name);

    for (i = 0; i < count; i++)
        buf[i] = (rt_uint8_t)('A' + (i % 26));

    rt_kprintf("[uart_test] %s write %d B (%s)%s\n",
               u->name, count, u->pins_hint, u->dma_en ? " DMA" : "");

    if (!is_console)
    {
        rt_device_close(dev);
        ret = uart_test_open(dev);
        if (ret != RT_EOK)
        {
            rt_kprintf("  open failed: %d\n", ret);
            return -1;
        }
    }

    n = rt_device_write(dev, -1, buf, (rt_size_t)count);
    if (n != count)
    {
        pass = 0;
        rt_kprintf("  write returned %d expect %d\n", (int)n, count);
    }
    else
        total = n;

    n = rt_device_write(dev, -1, buf, (rt_size_t)count);
    if (n != count)
    {
        pass = 0;
        rt_kprintf("  2nd write returned %d\n", (int)n);
    }
    else
        total += n;

    if (!is_console)
        rt_device_close(dev);

    rt_kprintf("[uart_test] %s write total=%d %s\n",
               u->name, (int)total, pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}

static int uart_test_do_echo(const struct uart_test_unit *u, int timeout_ms)
{
    rt_device_t dev;
    static rt_uint8_t buf[UART_TEST_BUF];
    rt_uint32_t total_rx = 0, total_tx = 0;
    rt_tick_t start, timeout_tick, rx_to;
    rt_err_t ret;
    rt_bool_t is_console;

    if (u == RT_NULL)
        return -1;
    if (timeout_ms <= 0)
        timeout_ms = UART_TEST_ECHO_DEF_MS;

    dev = rt_device_find(u->name);
    if (dev == RT_NULL)
    {
        rt_kprintf("[uart_test] %s not found FAIL\n", u->name);
        return -1;
    }

    is_console = uart_test_is_console(u->name);
    if (is_console)
    {
        rt_kprintf("[uart_test] WARN: %s is console; echo steals MSH input\n",
                   u->name);
    }

    rt_kprintf("[uart_test] %s echo timeout=%d ms (%s)%s\n",
               u->name, timeout_ms, u->pins_hint,
               u->dma_en ? " DMA" : " INT");
    rt_kprintf("  short TX-RX or send data from peer...\n");

    if (!is_console)
        rt_device_close(dev);

    ret = uart_test_open(dev);
    if (ret != RT_EOK)
    {
        rt_kprintf("  open failed: %d\n", ret);
        return -1;
    }

    rx_to = RT_TICK_PER_SECOND / 10;
    rt_device_control(dev, RT_SERIAL_CTRL_SET_RX_TIMEOUT, &rx_to);

    start = rt_tick_get();
    timeout_tick = rt_tick_from_millisecond(timeout_ms);

    while (rt_tick_get() - start < timeout_tick)
    {
        rt_ssize_t nr = rt_device_read(dev, -1, buf, sizeof(buf));
        if (nr > 0)
        {
            rt_ssize_t nw = rt_device_write(dev, -1, buf, (rt_size_t)nr);
            total_rx += (rt_uint32_t)nr;
            if (nw > 0)
                total_tx += (rt_uint32_t)nw;
        }
    }

    if (!is_console)
    {
        rt_device_close(dev);
        ret = uart_test_open(dev);
        if (ret != RT_EOK)
            rt_kprintf("  post-echo reopen failed: %d\n", ret);
        else
            rt_device_close(dev);
    }

    rt_kprintf("[uart_test] %s echo done: rx=%u tx=%u %s\n",
               u->name, total_rx, total_tx,
               (total_rx > 0 && total_rx == total_tx) ? "PASS" : "FAIL/idle");
    return (total_rx > 0 && total_rx == total_tx) ? 0 : -1;
}

static int uart_test_run_all(int echo_ms)
{
    rt_size_t i;
    int fail = 0, ran = 0;

    rt_kprintf("======== uart_test all ========\n");
    uart_test_info();

    for (i = 0; i < sizeof(g_uart_units) / sizeof(g_uart_units[0]); i++)
    {
        const struct uart_test_unit *u = &g_uart_units[i];
        rt_bool_t is_con = uart_test_is_console(u->name) || u->is_console_hint;

        ran++;
        rt_kprintf("---- %s ----\n", u->name);

        if (uart_test_do_reopen(u) != 0)
            fail++;
        if (uart_test_do_write(u, UART_TEST_WRITE_DEF) != 0)
            fail++;

        if (is_con)
        {
            rt_kprintf("[uart_test] %s echo skipped (console)\n", u->name);
            continue;
        }

        if (echo_ms > 0)
        {
            if (uart_test_do_echo(u, echo_ms) != 0)
                fail++;
        }
        else
        {
            rt_kprintf("[uart_test] %s echo skipped (use: uart_test echo %s)\n",
                       u->name, u->name);
        }
    }

    if (ran == 0)
    {
        rt_kprintf("[uart_test] no UART enabled FAIL\n");
        return -1;
    }

    rt_kprintf("======== uart_test all: ran=%d fail=%d %s ========\n",
               ran, fail, fail ? "FAIL" : "PASS");
    rt_kprintf("tips: uart_test echo uart2 30000  (short TX-RX)\n");
    return fail ? -1 : 0;
}

static void uart_test_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  uart_test                     reopen+write all (no echo wait)\n");
    rt_kprintf("  uart_test info                pin/DMA map\n");
    rt_kprintf("  uart_test write [name] [n]    TX pattern\n");
    rt_kprintf("  uart_test echo [name] [ms]    TX-RX loopback / peer\n");
    rt_kprintf("  uart_test reopen [name]\n");
    rt_kprintf("  uart_test all [ms]            +echo non-console if ms>0\n");
    rt_kprintf("name: uart1..4 lpuart1 lpuart2\n");
}

static int uart_test_pick_default_name(char *out, rt_size_t outsz)
{
    rt_size_t i;

    for (i = 0; i < sizeof(g_uart_units) / sizeof(g_uart_units[0]); i++)
    {
        if (!uart_test_is_console(g_uart_units[i].name))
        {
            rt_strncpy(out, g_uart_units[i].name, outsz - 1);
            out[outsz - 1] = '\0';
            return 0;
        }
    }
    if (sizeof(g_uart_units) / sizeof(g_uart_units[0]) > 0)
    {
        rt_strncpy(out, g_uart_units[0].name, outsz - 1);
        out[outsz - 1] = '\0';
        return 0;
    }
    return -1;
}

static int uart_test(int argc, char **argv)
{
    const struct uart_test_unit *u;
    char name[16];
    int n;

    if (argc < 2)
        return uart_test_run_all(0);

    if (!strcmp(argv[1], "help") || !strcmp(argv[1], "-h"))
    {
        uart_test_usage();
        return 0;
    }
    if (!strcmp(argv[1], "info"))
    {
        uart_test_info();
        return 0;
    }
    if (!strcmp(argv[1], "all"))
    {
        n = (argc >= 3) ? atoi(argv[2]) : 0;
        return uart_test_run_all(n);
    }

    if (!strcmp(argv[1], "reopen") || !strcmp(argv[1], "write") ||
        !strcmp(argv[1], "echo"))
    {
        if (argc >= 3)
        {
            rt_strncpy(name, argv[2], sizeof(name) - 1);
            name[sizeof(name) - 1] = '\0';
        }
        else if (uart_test_pick_default_name(name, sizeof(name)) != 0)
        {
            rt_kprintf("[uart_test] no UART enabled\n");
            return -1;
        }

        u = uart_test_find(name);
        if (u == RT_NULL)
        {
            rt_kprintf("[uart_test] unknown %s (not enabled in Kconfig?)\n", name);
            return -1;
        }

        if (!strcmp(argv[1], "reopen"))
            return uart_test_do_reopen(u);
        if (!strcmp(argv[1], "write"))
        {
            n = (argc >= 4) ? atoi(argv[3]) : UART_TEST_WRITE_DEF;
            return uart_test_do_write(u, n);
        }
        n = (argc >= 4) ? atoi(argv[3]) : UART_TEST_ECHO_DEF_MS;
        return uart_test_do_echo(u, n);
    }

    /* uart_test uart2 [ms] -> echo */
    u = uart_test_find(argv[1]);
    if (u != RT_NULL)
    {
        n = (argc >= 3) ? atoi(argv[2]) : UART_TEST_ECHO_DEF_MS;
        return uart_test_do_echo(u, n);
    }

    uart_test_usage();
    return 0;
}
MSH_CMD_EXPORT(uart_test, "UART/LPUART test: info/write/echo/reopen/all");

#endif /* any UART */
