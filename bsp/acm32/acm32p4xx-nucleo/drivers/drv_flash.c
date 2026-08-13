/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-28     AisinoChip   on-chip 1MB Flash FAL device via HAL_NORFLASH
 */

#ifdef BSP_USING_ONCHIP_FLASH
#include <fal.h>
#include <rtthread.h>
#include "board.h"
#include "hal_norflash.h"

#define FLASH_BASE_ADDR     0x08000000UL

static int flash_init(void)
{
    return 0;
}

static int flash_read(long offset, uint8_t *buf, size_t size)
{
    if (HAL_NORFLASH_Read(FLASH_BASE_ADDR + (uint32_t)offset, buf, (uint32_t)size) != 0)
        return -1;
    return (int)size;
}

static int flash_write(long offset, const uint8_t *buf, size_t size)
{
    if (HAL_NORFLASH_Program(FLASH_BASE_ADDR + (uint32_t)offset, (uint8_t *)buf, (uint32_t)size) != 0)
        return -1;
    return (int)size;
}

static int flash_erase(long offset, size_t size)
{
    if (HAL_NORFLASH_Erase(FLASH_BASE_ADDR + (uint32_t)offset, (uint32_t)size) != 0)
        return -1;
    return (int)size;
}

/* FAL flash device: 1MB on-chip Flash at 0x08000000, 4KB sector */
const struct fal_flash_dev onchip_flash =
{
    .name       = NOR_FLASH_DEV_NAME,
    .addr       = FLASH_BASE_ADDR,
    .len        = 1024 * 1024,
    .blk_size   = 4096,
    .ops        = {flash_init, flash_read, flash_write, flash_erase},
    .write_gran = 1,
};

/* FAL auto-registers devices from FAL_FLASH_DEV_TABLE (fal_cfg.h) in fal_init() */
INIT_COMPONENT_EXPORT(fal_init);

#endif /* BSP_USING_ONCHIP_FLASH */
