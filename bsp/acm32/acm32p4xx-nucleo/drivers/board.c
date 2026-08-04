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

#include <rthw.h>
#include <rtthread.h>
#include "board.h"
#include <rtdevice.h>
#include "system_accelerate.h"

#define SOC_SRAM_END_ADDR   (SOC_SRAM_START_ADDR + SOC_SRAM_SIZE * 1024)

extern int  rt_application_init(void);

extern void rt_hw_uart_init(void);
extern volatile uint32_t SystemCoreClock;

#if defined(DATA_IN_ExtSRAM) && defined(RT_USING_MEMHEAP)
struct rt_memheap psram_heap;
#endif

/**
 * This is the timer interrupt service routine.
 *
 */
void SysTick_Handler(void)
{
    /* enter interrupt */
    rt_interrupt_enter();

    rt_tick_increase();
    HAL_IncTick();

    /* leave interrupt */
    rt_interrupt_leave();
}

/**
 * This function will initial EVB board.
 */
void rt_hw_board_init(void)
{
    rt_bool_t psram_warn = RT_FALSE;

    /* HAL initialization (priority grouping, system clock read) */
    HAL_Init();

    /* SystemInit() is already called from startup_acm32p4xx.S
     * (incl. SystemInit_ExtMemCtl for OSPI PSRAM when DATA_IN_ExtSRAM). */
    /* Configure system clock to desired frequency */
    if (SystemClock_Config(SYSCLK_180M_SRC_RCH, PCLK1_DIV_SELECT, PCLK2_DIV_SELECT) != HAL_OK)
    {
        /* Clock config failed: hang with a simple fault path (no console yet). */
        while (1)
        {
        }
    }

    /* Update SystemCoreClock */
    SystemCoreClockUpdate();

#ifdef DATA_IN_ExtSRAM
    /* Full OSPI PSRAM init after HCLK is final (not only BAUD retune) */
    System_OSPI_PSRAM_Reclock();
    if (!System_OSPI_PSRAM_Ready())
        psram_warn = RT_TRUE;
#endif

    /* Re-configure SysTick for RT-Thread (overriding HAL_InitTick) */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

#ifdef RT_USING_HEAP
    /*
     * System heap is SRAM1 only (0x20010000..SOC_SRAM_END).
     * DTCM holds .data/.bss and MSP stack (grows down from 0x20010000);
     * never hand DTCM residual to rt_malloc or it collides with MSP.
     */
    __HAL_RCC_SRAM1_CLK_ENABLE();
    rt_system_heap_init((void *)SOC_SRAM1_START_ADDR, (void *)SOC_SRAM_END_ADDR);
#if defined(DATA_IN_ExtSRAM) && defined(RT_USING_MEMHEAP)
    if (System_OSPI_PSRAM_Ready())
        rt_memheap_init(&psram_heap, "psram", (void *)0x80000000, 0x200000);
#endif
#endif /* RT_USING_HEAP */

    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    if (psram_warn)
        rt_kprintf("WARNING: OSPI PSRAM init failed, psram memheap disabled\n");

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
