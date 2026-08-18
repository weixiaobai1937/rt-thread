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
#include <dfs.h>
#include <dfs_file.h>
#include <dfs_fs.h>

#ifdef BSP_USING_ONCHIP_FLASH
#include <fal.h>

#define FS_PART_NAME   "filesystem"

static void flash_info(void)
{
    fal_show_part_table();
}
MSH_CMD_EXPORT(flash_info, show FAL flash/partition table);

/* create (or reuse) the FAL block device for the filesystem partition.
 * fal_blk_device_create mallocs each call, so reuse an already-registered one. */
static struct rt_device *_fs_blk_create(void)
{
    struct rt_device *blk = rt_device_find(FS_PART_NAME);

    if (blk == RT_NULL)
    {
        blk = fal_blk_device_create(FS_PART_NAME);
        if (blk == RT_NULL)
            rt_kprintf("fal_blk_device_create(%s) FAIL\n", FS_PART_NAME);
    }
    return blk;
}

static void fs_mkfs(void)
{
    if (_fs_blk_create() == RT_NULL)
        return;
    if (dfs_mkfs("elm", FS_PART_NAME) != 0)
        rt_kprintf("mkfs elm on %s FAIL\n", FS_PART_NAME);
    else
        rt_kprintf("mkfs elm on %s OK\n", FS_PART_NAME);
}
MSH_CMD_EXPORT(fs_mkfs, format filesystem partition as elmfat);

static void fs_mount(void)
{
    if (_fs_blk_create() == RT_NULL)
        return;
    if (dfs_mount(FS_PART_NAME, "/", "elm", 0, 0) != 0)
        rt_kprintf("mount elm on / FAIL (run 'fs_mkfs' first)\n");
    else
        rt_kprintf("mount elm on / OK\n");
}
MSH_CMD_EXPORT(fs_mount, mount filesystem partition to /);

#endif /* BSP_USING_ONCHIP_FLASH */
/* write/read-back check on the mounted filesystem (requires fs_mount first) */
static void fs_test(void)
{
    struct dfs_file fd;
    int i, n, ok = 1;
    char wbuf[128];
    char rbuf[128];
    rt_size_t off = 0;

    for (i = 0; i < (int)sizeof(wbuf); i++)
        wbuf[i] = (char)(i & 0x7F);

    if (dfs_file_open(&fd, "/test.bin", O_WRONLY | O_CREAT | O_TRUNC) != 0)
    {
        rt_kprintf("fs_test: open for write FAIL (is the FS mounted?)\n");
        return;
    }
    while (off < sizeof(wbuf))
    {
        n = dfs_file_write(&fd, wbuf + off, sizeof(wbuf) - off);
        if (n <= 0) { ok = 0; break; }
        off += n;
    }
    dfs_file_close(&fd);

    if (dfs_file_open(&fd, "/test.bin", O_RDONLY) != 0)
    {
        rt_kprintf("fs_test: open for read FAIL\n");
        return;
    }
    off = 0;
    while (off < sizeof(rbuf))
    {
        n = dfs_file_read(&fd, rbuf + off, sizeof(rbuf) - off);
        if (n <= 0) { ok = 0; break; }
        off += n;
    }
    dfs_file_close(&fd);

    if (ok && rt_memcmp(wbuf, rbuf, sizeof(wbuf)) == 0)
        rt_kprintf("fs_test PASS (%d bytes written/read back)\n", (int)sizeof(wbuf));
    else
        rt_kprintf("fs_test FAIL (data mismatch)\n");
}
MSH_CMD_EXPORT(fs_test, write/read-back check on mounted filesystem);

