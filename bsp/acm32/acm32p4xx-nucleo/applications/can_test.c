/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx CAN (FDCAN) MSH test
 * 2026-08-04     AisinoChip   open device before read/write
 *
 * MSH:
 *   can_test                  list devices + info
 *   can_test send <id> <data> send CAN frame (hex bytes)
 *   can_test recv              receive one frame (blocking 5s)
 *   can_test loopback <dev>    set loopback + send/receive self-test
 *
 * Host: USB-CAN adapter or another CAN node.
 * On-board loopback: can_test loopback fdcan1
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_CAN) && (defined(BSP_USING_FDCAN1) || defined(BSP_USING_FDCAN2))

#define CAN_OPEN_FLAGS  (RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX)

struct can_test_unit
{
    const char *name;
    const char *rx_pin;
    const char *tx_pin;
};

static const struct can_test_unit g_can_units[] =
{
#ifdef BSP_USING_FDCAN1
    {"fdcan1", "PD0", "PD1"},
#endif
#ifdef BSP_USING_FDCAN2
    {"fdcan2", "PE5", "PE6"},
#endif
};

static rt_device_t can_open(const char *dev_name)
{
    rt_device_t dev = rt_device_find(dev_name);
    if (dev == RT_NULL)
    {
        rt_kprintf("can_test: %s not found\n", dev_name);
        return RT_NULL;
    }

    if (rt_device_open(dev, CAN_OPEN_FLAGS) != RT_EOK)
    {
        rt_kprintf("can_test: open %s failed\n", dev_name);
        return RT_NULL;
    }
    return dev;
}

static void can_test_info(void)
{
    rt_kprintf("=== CAN Devices ===\n");
    for (int i = 0; i < (int)(sizeof(g_can_units) / sizeof(g_can_units[0])); i++)
    {
        rt_kprintf("  %s: RX=%s TX=%s\n",
                   g_can_units[i].name, g_can_units[i].rx_pin, g_can_units[i].tx_pin);
    }
}

static int can_test_send(const char *dev_name, rt_uint32_t id,
                          const rt_uint8_t *data, rt_uint8_t len)
{
    struct rt_can_msg msg = {0};
    rt_device_t dev;
    rt_size_t sz;

    msg.id  = id;
    msg.ide = RT_CAN_STDID;
    msg.rtr = RT_CAN_DTR;
    msg.len = len > 8 ? 8 : len;
    if (len > 0)
        rt_memcpy(msg.data, data, msg.len);

    dev = can_open(dev_name);
    if (dev == RT_NULL)
        return -1;

    sz = rt_device_write(dev, 0, &msg, sizeof(msg));
    if (sz == 0)
    {
        rt_kprintf("can_test: send failed\n");
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("can_test: sent id=0x%03X len=%d:", id, msg.len);
    for (int i = 0; i < msg.len; i++)
        rt_kprintf(" %02X", msg.data[i]);
    rt_kprintf("\n");

    rt_device_close(dev);
    return 0;
}

static int can_test_recv(const char *dev_name)
{
    struct rt_can_msg msg = {0};
    rt_device_t dev;
    rt_size_t sz;

    dev = can_open(dev_name);
    if (dev == RT_NULL)
        return -1;

    rt_kprintf("can_test: waiting on %s...\n", dev_name);
    sz = rt_device_read(dev, 0, &msg, sizeof(msg));
    if (sz == 0)
    {
        rt_kprintf("can_test: recv timeout\n");
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("can_test: recv id=0x%03X %s len=%d:",
               msg.id, msg.ide ? "EXT" : "STD", msg.len);
    for (int i = 0; i < msg.len; i++)
        rt_kprintf(" %02X", msg.data[i]);
    rt_kprintf("\n");

    rt_device_close(dev);
    return 0;
}

static int can_test_loopback(const char *dev_name)
{
    struct rt_can_msg msg = {0};
    rt_uint8_t data[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
    rt_device_t dev;
    rt_size_t sz;
    rt_bool_t ok;

    dev = can_open(dev_name);
    if (dev == RT_NULL)
        return -1;

    rt_device_control(dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_LOOPBACK);
    rt_device_control(dev, RT_CAN_CMD_START, (void *)1);

    rt_kprintf("can_test: %s loopback self-test\n", dev_name);

    msg.id = 0x123;
    msg.ide = RT_CAN_STDID;
    msg.rtr = RT_CAN_DTR;
    msg.len = 8;
    rt_memcpy(msg.data, data, 8);

    sz = rt_device_write(dev, 0, &msg, sizeof(msg));
    if (sz == 0)
    {
        rt_kprintf("can_test: loopback send failed\n");
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("can_test: sent id=0x123, waiting echo...\n");
    rt_memset(&msg, 0, sizeof(msg));
    sz = rt_device_read(dev, 0, &msg, sizeof(msg));
    if (sz == 0)
    {
        rt_kprintf("can_test: loopback recv failed\n");
        rt_device_close(dev);
        return -1;
    }

    rt_kprintf("can_test: loopback recv id=0x%03X len=%d:", msg.id, msg.len);
    for (int i = 0; i < msg.len; i++)
        rt_kprintf(" %02X", msg.data[i]);
    rt_kprintf("\n");

    ok = (msg.id == 0x123 && msg.len == 8 &&
          rt_memcmp(msg.data, data, 8) == 0);
    rt_kprintf("can_test: loopback %s\n", ok ? "PASS" : "FAIL");

    rt_device_control(dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
    rt_device_close(dev);
    return ok ? 0 : -1;
}

static void can_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  can_test                       list devices\n");
    rt_kprintf("  can_test send <id> <byte>...   send CAN frame\n");
    rt_kprintf("  can_test recv [dev=fdcan1]     receive one frame\n");
    rt_kprintf("  can_test loopback [dev]        loopback self-test\n");
}

static int can_test(int argc, char **argv)
{
    if (argc < 2)
    {
        can_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        can_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "send") == 0 && argc >= 4)
    {
        rt_uint32_t id = (rt_uint32_t)strtoul(argv[2], RT_NULL, 16);
        rt_uint8_t data[8];
        int len = argc - 3;
        if (len > 8)
            len = 8;
        for (int i = 0; i < len; i++)
            data[i] = (rt_uint8_t)strtoul(argv[3 + i], RT_NULL, 16);
        return can_test_send("fdcan1", id, data, (rt_uint8_t)len);
    }

    if (rt_strcmp(argv[1], "recv") == 0)
    {
        const char *dev = (argc >= 3) ? argv[2] : "fdcan1";
        return can_test_recv(dev);
    }

    if (rt_strcmp(argv[1], "loopback") == 0)
    {
        const char *dev = (argc >= 3) ? argv[2] : "fdcan1";
        return can_test_loopback(dev);
    }

    can_usage();
    return 0;
}
MSH_CMD_EXPORT(can_test, FDCAN send/recv/loopback test);

#endif /* RT_USING_CAN */
