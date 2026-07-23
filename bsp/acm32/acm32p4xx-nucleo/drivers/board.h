/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-08-25     AisinoChip   first implementation
 * 2026-06-04     AisinoChip   add ACM32P4xx support
 */

#ifndef __BOARD_H__
#define __BOARD_H__

#include <rtconfig.h>
#include "acm32p4xx_hal_conf.h"

/* UART/LPUART/SPI pins and DMA: drivers/uart_config.h, spi_config.h + menuconfig
 * Resource conflicts: auto block in rtconfig.h (SCons check_bsp_resources). */

/* board configuration */

#ifndef SOC_SRAM_START_ADDR
#define SOC_SRAM_START_ADDR     (0x20000000)
#endif
#ifndef SOC_SRAM_SIZE
#define SOC_SRAM_SIZE           (128)
#endif

void rt_hw_board_init(void);

#endif /* __BOARD_H__ */
