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
 * 2026-07-14     AisinoChip   SPI1 MOSI-MISO loopback test
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <stdlib.h>
#include "board.h"
#include "spi_config.h"

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
        }
    }

    rt_device_close(dev);

    /* 验证 close/reopen 生命周期 */
    ret = rt_device_open(dev, RT_DEVICE_OFLAG_RDWR | RT_DEVICE_FLAG_RX_BLOCKING | RT_DEVICE_FLAG_TX_BLOCKING);
    if (ret != RT_EOK)
    {
        rt_kprintf("uart2 reopen failed: %d\n", ret);
        return -1;
    }
    rt_device_close(dev);

    rt_kprintf("UART2 echo done: rx=%u tx=%u %s\n",
               total_rx, total_tx,
               (total_rx > 0 && total_rx == total_tx) ? "PASS" : "FAIL/idle");
    return (total_rx > 0 && total_rx == total_tx) ? 0 : -1;
}
MSH_CMD_EXPORT(uart2_echo_test, "UART2 DMA echo test [timeout_ms]");

#ifdef BSP_USING_SPI1
extern rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin);

static struct rt_spi_device *spi1_test_prepare(void)
{
    struct rt_spi_device *dev;
    struct rt_spi_configuration cfg;
    rt_err_t ret;

    ret = rt_hw_spi_device_attach("spi1", "spi10", SPI1_DEFAULT_CS_PIN_INDEX);
    if (ret != RT_EOK)
    {
        /* 可能已 attach：继续 find */
        rt_kprintf("attach ret=%d (may already exist)\n", ret);
    }

    dev = (struct rt_spi_device *)rt_device_find("spi10");
    if (dev == RT_NULL)
    {
        rt_kprintf("spi10 not found\n");
        return RT_NULL;
    }

    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = 1000000;
    ret = rt_spi_configure(dev, &cfg);
    if (ret != RT_EOK)
    {
        rt_kprintf("spi configure failed: %d\n", ret);
        return RT_NULL;
    }

    return dev;
}

int spi1_loopback_test(int argc, char **argv)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[512];
    static rt_uint8_t rx[512];
    int i;
    int len = 16;
    int pass = 1;

    if (argc >= 2)
        len = atoi(argv[1]);
    if (len <= 0 || len > 512)
        len = 16;

    dev = spi1_test_prepare();
    if (dev == RT_NULL)
        return -1;

    for (i = 0; i < len; i++)
        tx[i] = (rt_uint8_t)(0xA0 + (i & 0x0F));
    rt_memset(rx, 0, (rt_size_t)len);

    rt_kprintf("SPI1 loopback len=%d: short MOSI(PE11)-MISO(PE10)\n", len);
#ifdef BSP_USING_SPI1_DMA
    rt_kprintf("DMA enabled, half-duplex TX>=32 uses DMA\n");
#endif
    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != len)
    {
        rt_kprintf("transfer failed\n");
        return -1;
    }

    for (i = 0; i < len; i++)
    {
        if (rx[i] != tx[i])
        {
            pass = 0;
            rt_kprintf("mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
        }
    }

    rt_kprintf("SPI1 loopback %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}
MSH_CMD_EXPORT(spi1_loopback_test, "SPI1 MOSI-MISO loopback test [len]");

int spi1_dma_test(int argc, char **argv)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[512];
    static rt_uint8_t rx[512];
    int i;
    int len = 256;
    int pass = 1;
    rt_size_t n;

    if (argc >= 2)
        len = atoi(argv[1]);
    if (len < 32 || len > 512)
        len = 256;

    dev = spi1_test_prepare();
    if (dev == RT_NULL)
        return -1;

    for (i = 0; i < len; i++)
        tx[i] = 0x5A;
    rt_memset(rx, 0, (rt_size_t)len);

    rt_kprintf("SPI1 dma test len=%d\n", len);
#ifdef BSP_USING_SPI1_DMA
    rt_kprintf("DMA enabled, half-duplex TX>=32 uses DMA\n");
#else
    rt_kprintf("DMA not enabled (poll path)\n");
#endif

    /* full-duplex 环回：轮询路径仍应 PASS */
    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != len)
    {
        rt_kprintf("transfer failed\n");
        return -1;
    }
    for (i = 0; i < len; i++)
    {
        if (rx[i] != tx[i])
        {
            pass = 0;
            rt_kprintf("fdx mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
        }
    }

    /* half-duplex TX：length>=32 时走 DMA */
    n = rt_spi_send(dev, tx, (rt_size_t)len);
    if (n != (rt_size_t)len)
    {
        pass = 0;
        rt_kprintf("rt_spi_send failed: %d\n", (int)n);
    }

    /* 短包 transfer 再校验环回通路 */
    for (i = 0; i < 16; i++)
        tx[i] = (rt_uint8_t)(0xA0 + i);
    rt_memset(rx, 0, 16);
    if (rt_spi_transfer(dev, tx, rx, 16) != 16)
    {
        pass = 0;
        rt_kprintf("short transfer after DMA failed\n");
    }
    else
    {
        for (i = 0; i < 16; i++)
        {
            if (rx[i] != tx[i])
            {
                pass = 0;
                rt_kprintf("short mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
            }
        }
    }

    rt_kprintf("SPI1 dma test %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}
MSH_CMD_EXPORT(spi1_dma_test, "SPI1 long xfer test [len]");
#endif

#ifdef BSP_USING_SPI2
#ifndef BSP_USING_SPI1
extern rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin);
#endif

static struct rt_spi_device *spi2_test_prepare(void)
{
    struct rt_spi_device *dev;
    struct rt_spi_configuration cfg;
    rt_err_t ret;

    ret = rt_hw_spi_device_attach("spi2", "spi20", SPI2_DEFAULT_CS_PIN_INDEX);
    if (ret != RT_EOK)
    {
        rt_kprintf("attach ret=%d (may already exist)\n", ret);
    }

    dev = (struct rt_spi_device *)rt_device_find("spi20");
    if (dev == RT_NULL)
    {
        rt_kprintf("spi20 not found\n");
        return RT_NULL;
    }

    cfg.mode = RT_SPI_MASTER | RT_SPI_MODE_0 | RT_SPI_MSB;
    cfg.data_width = 8;
    cfg.max_hz = 1000000;
    ret = rt_spi_configure(dev, &cfg);
    if (ret != RT_EOK)
    {
        rt_kprintf("spi configure failed: %d\n", ret);
        return RT_NULL;
    }

    return dev;
}

int spi2_loopback_test(int argc, char **argv)
{
    struct rt_spi_device *dev;
    static rt_uint8_t tx[512];
    static rt_uint8_t rx[512];
    int i;
    int len = 16;
    int pass = 1;

    if (argc >= 2)
        len = atoi(argv[1]);
    if (len <= 0 || len > 512)
        len = 16;

    dev = spi2_test_prepare();
    if (dev == RT_NULL)
        return -1;

    for (i = 0; i < len; i++)
        tx[i] = (rt_uint8_t)(0xA0 + (i & 0x0F));
    rt_memset(rx, 0, (rt_size_t)len);

    rt_kprintf("SPI2 loopback len=%d: short MOSI(PB15)-MISO(PB14)\n", len);
#ifdef BSP_USING_SPI2_DMA
    rt_kprintf("DMA enabled, half-duplex TX>=32 uses DMA\n");
#endif
    if (rt_spi_transfer(dev, tx, rx, (rt_size_t)len) != len)
    {
        rt_kprintf("transfer failed\n");
        return -1;
    }

    for (i = 0; i < len; i++)
    {
        if (rx[i] != tx[i])
        {
            pass = 0;
            rt_kprintf("mismatch @%d tx=%02X rx=%02X\n", i, tx[i], rx[i]);
        }
    }

    rt_kprintf("SPI2 loopback %s\n", pass ? "PASS" : "FAIL");
    return pass ? 0 : -1;
}
MSH_CMD_EXPORT(spi2_loopback_test, "SPI2 MOSI-MISO loopback [len]");
#endif

int main(void)
{
    rt_kprintf("ACM32P4xx-Nucleo BSP boot success!\n");
    rt_kprintf("Run 'uart2_echo_test' to test UART2 DMA echo\n");
    rt_kprintf("Run 'spi1_loopback_test' after shorting MOSI-MISO\n");
#ifdef BSP_USING_SPI1
    rt_kprintf("Run 'spi1_dma_test' for long/half-duplex TX path\n");
#endif
#ifdef BSP_USING_SPI2
    rt_kprintf("Run 'spi2_loopback_test' after shorting MOSI(PB15)-MISO(PB14)\n");
#endif

    rt_pin_mode(LED_PIN_NUM, PIN_MODE_OUTPUT);

    while (1)
    {
        rt_thread_delay(RT_TICK_PER_SECOND / 2);
    }
}
