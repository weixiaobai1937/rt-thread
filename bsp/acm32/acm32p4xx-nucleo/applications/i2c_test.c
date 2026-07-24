/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx I2C MSH test
 *
 * MSH:
 *   i2c_test                  list buses + scan 0x08..0x77
 *   i2c_test info             bus + pin info
 *   i2c_test scan [bus]       scan addresses (default i2c1)
 *   i2c_test probe <bus> <addr> probe single address
 *   i2c_test read <bus> <addr> <reg> [len]
 *   i2c_test write <bus> <addr> <reg> <val>
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

#if defined(RT_USING_I2C) && (defined(BSP_USING_I2C1) || defined(BSP_USING_I2C2))

struct i2c_test_unit
{
    int             unit;
    const char     *bus_name;
    const char     *scl_pin_hint;
    const char     *sda_pin_hint;
};

static const struct i2c_test_unit g_i2c_units[] =
{
#ifdef BSP_USING_I2C1
    {1, "i2c1", "PB6", "PB7"},
#endif
#ifdef BSP_USING_I2C2
    {2, "i2c2", "PE1", "PE0"},
#endif
};

static const struct i2c_test_unit *find_unit(const char *name)
{
    for (int i = 0; i < (int)(sizeof(g_i2c_units) / sizeof(g_i2c_units[0])); i++)
    {
        if (rt_strcmp(g_i2c_units[i].bus_name, name) == 0)
            return &g_i2c_units[i];
    }
    return &g_i2c_units[0];
}

static void i2c_test_info(void)
{
    rt_kprintf("=== I2C Buses ===\n");
    for (int i = 0; i < (int)(sizeof(g_i2c_units) / sizeof(g_i2c_units[0])); i++)
    {
        const struct i2c_test_unit *u = &g_i2c_units[i];
        rt_kprintf("  %s: SCL=%s SDA=%s\n", u->bus_name, u->scl_pin_hint, u->sda_pin_hint);
    }
}

static int i2c_test_scan(const char *bus_name)
{
    struct rt_i2c_bus_device *bus;
    int found = 0;

    bus = (struct rt_i2c_bus_device *)rt_device_find(bus_name);
    if (bus == RT_NULL)
    {
        rt_kprintf("i2c_test: bus %s not found\n", bus_name);
        return -1;
    }

    rt_kprintf("i2c_test: scanning %s [0x08..0x77]...\n", bus_name);
    for (rt_uint8_t addr = 0x08; addr <= 0x77; addr++)
    {
        struct rt_i2c_msg msgs[1];
        rt_uint8_t dummy;
        rt_int32_t ret;

        msgs[0].addr  = addr;
        msgs[0].flags = RT_I2C_RD;
        msgs[0].buf   = &dummy;
        msgs[0].len   = 1;

        ret = rt_i2c_transfer(bus, msgs, 1);
        if (ret == 1)
        {
            rt_kprintf("  0x%02X ACK\n", addr);
            found++;
        }
    }
    rt_kprintf("i2c_test: %d device(s) found on %s\n", found, bus_name);
    return 0;
}

static int i2c_test_probe(const char *bus_name, rt_uint8_t addr)
{
    struct rt_i2c_bus_device *bus;
    struct rt_i2c_msg msgs[1];
    rt_uint8_t dummy;

    bus = (struct rt_i2c_bus_device *)rt_device_find(bus_name);
    if (bus == RT_NULL)
    {
        rt_kprintf("i2c_test: bus %s not found\n", bus_name);
        return -1;
    }

    msgs[0].addr  = addr;
    msgs[0].flags = RT_I2C_RD;
    msgs[0].buf   = &dummy;
    msgs[0].len   = 1;

    if (rt_i2c_transfer(bus, msgs, 1) == 1)
    {
        rt_kprintf("i2c_test: 0x%02X ACK (device present)\n", addr);
        return 0;
    }
    rt_kprintf("i2c_test: 0x%02X NAK (no device)\n", addr);
    return -1;
}

static int i2c_test_read(const char *bus_name, rt_uint8_t addr,
                          rt_uint8_t reg, int len)
{
    struct rt_i2c_bus_device *bus;
    rt_uint8_t buf[16];
    struct rt_i2c_msg msgs[2];

    if (len < 1) len = 1;
    if (len > (int)sizeof(buf)) len = sizeof(buf);

    bus = (struct rt_i2c_bus_device *)rt_device_find(bus_name);
    if (bus == RT_NULL)
    {
        rt_kprintf("i2c_test: bus %s not found\n", bus_name);
        return -1;
    }

    msgs[0].addr  = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = &reg;
    msgs[0].len   = 1;

    msgs[1].addr  = addr;
    msgs[1].flags = RT_I2C_RD;
    msgs[1].buf   = buf;
    msgs[1].len   = len;

    if (rt_i2c_transfer(bus, msgs, 2) == 2)
    {
        rt_kprintf("i2c_test: 0x%02X reg 0x%02X:", addr, reg);
        for (int i = 0; i < len; i++)
            rt_kprintf(" %02X", buf[i]);
        rt_kprintf("\n");
        return 0;
    }
    rt_kprintf("i2c_test: read failed\n");
    return -1;
}

static int i2c_test_write(const char *bus_name, rt_uint8_t addr,
                           rt_uint8_t reg, rt_uint8_t val)
{
    struct rt_i2c_bus_device *bus;
    rt_uint8_t buf[2];
    struct rt_i2c_msg msgs[1];

    bus = (struct rt_i2c_bus_device *)rt_device_find(bus_name);
    if (bus == RT_NULL)
    {
        rt_kprintf("i2c_test: bus %s not found\n", bus_name);
        return -1;
    }

    buf[0] = reg;
    buf[1] = val;

    msgs[0].addr  = addr;
    msgs[0].flags = RT_I2C_WR;
    msgs[0].buf   = buf;
    msgs[0].len   = 2;

    if (rt_i2c_transfer(bus, msgs, 1) == 1)
    {
        rt_kprintf("i2c_test: wrote 0x%02X to 0x%02X reg 0x%02X\n",
                   val, addr, reg);
        return 0;
    }
    rt_kprintf("i2c_test: write failed\n");
    return -1;
}

static void i2c_usage(void)
{
    rt_kprintf("Usage:\n");
    rt_kprintf("  i2c_test                  scan all buses\n");
    rt_kprintf("  i2c_test info             bus info\n");
    rt_kprintf("  i2c_test scan [bus]       scan device addresses\n");
    rt_kprintf("  i2c_test probe <bus> <addr>   probe single addr (hex)\n");
    rt_kprintf("  i2c_test read  <bus> <addr> <reg> [len=1]\n");
    rt_kprintf("  i2c_test write <bus> <addr> <reg> <val>\n");
    rt_kprintf("  bus = i2c1 / i2c2 (default i2c1)\n");
}

static int i2c_test(int argc, char **argv)
{
    if (argc < 2)
    {
        i2c_test_info();
        for (int i = 0; i < (int)(sizeof(g_i2c_units) / sizeof(g_i2c_units[0])); i++)
            i2c_test_scan(g_i2c_units[i].bus_name);
        return 0;
    }

    if (rt_strcmp(argv[1], "info") == 0)
    {
        i2c_test_info();
        return 0;
    }

    if (rt_strcmp(argv[1], "scan") == 0)
    {
        const char *bus = (argc >= 3) ? argv[2] : "i2c1";
        return i2c_test_scan(bus);
    }

    if (rt_strcmp(argv[1], "probe") == 0 && argc >= 4)
    {
        return i2c_test_probe(argv[2], (rt_uint8_t)strtoul(argv[3], RT_NULL, 16));
    }

    if (rt_strcmp(argv[1], "read") == 0 && argc >= 5)
    {
        int len = (argc >= 6) ? atoi(argv[5]) : 1;
        return i2c_test_read(argv[2], (rt_uint8_t)strtoul(argv[3], RT_NULL, 16),
                              (rt_uint8_t)strtoul(argv[4], RT_NULL, 16), len);
    }

    if (rt_strcmp(argv[1], "write") == 0 && argc >= 6)
    {
        return i2c_test_write(argv[2], (rt_uint8_t)strtoul(argv[3], RT_NULL, 16),
                               (rt_uint8_t)strtoul(argv[4], RT_NULL, 16),
                               (rt_uint8_t)strtoul(argv[5], RT_NULL, 16));
    }

    i2c_usage();
    return 0;
}
MSH_CMD_EXPORT(i2c_test, I2C bus scan / probe / read / write);

#endif /* RT_USING_I2C && (BSP_USING_I2C1 || BSP_USING_I2C2) */
