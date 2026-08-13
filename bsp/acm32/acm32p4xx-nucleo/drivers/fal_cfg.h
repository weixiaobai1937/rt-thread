/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-28     AisinoChip   ACM32P4xx on-chip Flash FAL config
 */

#ifndef _FAL_CFG_H_
#define _FAL_CFG_H_

#include <rtconfig.h>
#include <board.h>

#define NOR_FLASH_DEV_NAME             "onchip_flash"

extern const struct fal_flash_dev onchip_flash;

/* flash device table */
#define FAL_FLASH_DEV_TABLE                    \
{                                              \
    &onchip_flash,                             \
}

/* ====================== Partition Configuration ========================== */
#ifdef FAL_PART_HAS_TABLE_CFG
/*
 * On-chip 1MB Flash (base 0x08000000, sector 4KB):
 *   boot       0x000000-0x002000  8KB    (bootloader, read-only)
 *   app        0x002000-0x080000  504KB  (application, read-only)
 *   filesystem 0x080000-0x100000  512KB  (elmfat, writable)
 */
#define FAL_PART_TABLE                               \
{                                                    \
    {FAL_PART_MAGIC_WORD,        "boot",       NOR_FLASH_DEV_NAME, 0x000000, 0x002000, 0}, \
    {FAL_PART_MAGIC_WORD,         "app",       NOR_FLASH_DEV_NAME, 0x002000, 0x07E000, 0}, \
    {FAL_PART_MAGIC_WORD,  "filesystem",       NOR_FLASH_DEV_NAME, 0x080000, 0x080000, 0}, \
}
#endif /* FAL_PART_HAS_TABLE_CFG */

#endif /* _FAL_CFG_H_ */
