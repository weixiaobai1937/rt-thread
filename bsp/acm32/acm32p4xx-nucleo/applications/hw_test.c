/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx HWRNG / SDMMC / I2S test
 *
 * MSH:
 *   hwrng_test               read 8 random 32-bit values
 *   sd_test                   SD card detect + info
 *   sd_test read <sector> [n] read n sectors (default 1)
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <string.h>
#include <stdlib.h>

/* ==================== HWRNG ==================== */

#ifdef BSP_USING_HWCRYPTO

static int hwrng_test(int argc, char **argv)
{
    (void)argc; (void)argv;

    rt_kprintf("hwrng_test: HW random numbers:\n");
    for (int i = 0; i < 8; i++)
    {
        rt_kprintf("  %2d: 0x%08lX\n", i + 1, (unsigned long)HAL_HRNG_GetHrng_32());
    }
    return 0;
}
MSH_CMD_EXPORT(hwrng_test, HW random number generator test);

#endif /* BSP_USING_HWCRYPTO */

/* ==================== SD/MMC ==================== */

#ifdef BSP_USING_SDMMC1

static int sd_test_read(int sector, int n)
{
    int fd;
    char buf[512];
    int total = 0;

    fd = open("/dev/sd0", 0); /* raw block device */
    if (fd < 0)
    {
        rt_kprintf("sd_test: cannot open sd0, is SD card inserted?\n");
        return -1;
    }

    lseek(fd, sector * 512, 0);

    for (int i = 0; i < n; i++)
    {
        int r = read(fd, buf, sizeof(buf));
        if (r <= 0) break;
        total++;
        rt_kprintf("sd_test: sector %d:", sector + i);
        for (int j = 0; j < 16; j++)
            rt_kprintf(" %02X", buf[j]);
        rt_kprintf("\n");
    }

    close(fd);
    rt_kprintf("sd_test: read %d/%d sectors\n", total, n);
    return 0;
}

static int sd_test(int argc, char **argv)
{
    if (argc < 2)
    {
        rt_device_t dev = rt_device_find("sd0");
        rt_kprintf("=== SD Card ===\n");
        if (dev == RT_NULL)
            rt_kprintf("  device: not detected (insert SD card)\n");
        else
            rt_kprintf("  device: sd0 found\n");
        rt_kprintf("Usage: sd_test read <sector> [n=1]\n");
        return 0;
    }

    if (rt_strcmp(argv[1], "read") == 0 && argc >= 3)
    {
        int sector = atoi(argv[2]);
        int n = (argc >= 4) ? atoi(argv[3]) : 1;
        return sd_test_read(sector, n);
    }

    return 0;
}
MSH_CMD_EXPORT(sd_test, SD card detect / read sectors);

#endif /* BSP_USING_SDMMC1 */

/* ==================== I2S / Audio ==================== */

#ifdef BSP_USING_I2S1

static int audio_test(int argc, char **argv)
{
    (void)argc; (void)argv;

    rt_device_t dev = rt_device_find("sound0");
    rt_kprintf("=== Audio (I2S) ===\n");
    if (dev == RT_NULL)
        rt_kprintf("  sound0: not found (enable BSP_USING_I2S1 in menuconfig)\n");
    else
        rt_kprintf("  sound0: found (I2S1, default 48kHz/16bit/stereo)\n");
    rt_kprintf("  Pins: WS=PA4 CK=PA5 SDI=PA6 SDO=PC12 MCK=PC4 (AF7)\n");
    rt_kprintf("  DMA: DMA1_CH0 (I2S1_TX)\n");
    return 0;
}
MSH_CMD_EXPORT(audio_test, I2S audio device info);

#endif /* BSP_USING_I2S1 */
