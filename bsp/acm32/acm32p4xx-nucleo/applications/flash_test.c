/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-28     AisinoChip   on-chip Flash / elmfat MSH test
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <dfs_fs.h>

#ifdef BSP_USING_ONCHIP_FLASH
#include <fal.h>

#define FS_PART_NAME   "filesystem"

static void flash_info(void)
{
    fal_show_part_table();
}
MSH_CMD_EXPORT(flash_info, show FAL flash/partition table);

static void fs_mkfs(void)
{
    if (dfs_mkfs("elm", FS_PART_NAME) != 0)
        rt_kprintf("mkfs elm on %s FAIL\n", FS_PART_NAME);
    else
        rt_kprintf("mkfs elm on %s OK\n", FS_PART_NAME);
}
MSH_CMD_EXPORT(fs_mkfs, format filesystem partition as elmfat);

static void fs_mount(void)
{
    struct rt_device *blk;

    blk = fal_blk_device_create(FS_PART_NAME);
    if (blk == RT_NULL)
    {
        rt_kprintf("fal_blk_device_create(%s) FAIL\n", FS_PART_NAME);
        return;
    }

    if (dfs_mount(FS_PART_NAME, "/", "elm", 0, 0) != 0)
        rt_kprintf("mount elm on / FAIL (run 'fs_mkfs' first)\n");
    else
        rt_kprintf("mount elm on / OK\n");
}
MSH_CMD_EXPORT(fs_mount, mount filesystem partition to /);

#endif /* BSP_USING_ONCHIP_FLASH */
