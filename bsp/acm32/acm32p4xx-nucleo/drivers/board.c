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

#define SOC_SRAM_END_ADDR   (SOC_SRAM_START_ADDR + SOC_SRAM_SIZE * 1024)

extern int  rt_application_init(void);

#if defined(__ARMCC_VERSION)
    extern int Image$$RW_IRAM1$$ZI$$Limit;
#elif __ICCARM__
    #pragma section="HEAP"
#else
    extern int __bss_end;
#endif

extern void rt_hw_uart_init(void);
extern volatile uint32_t SystemCoreClock;

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
    /* HAL initialization (priority grouping, system clock read) */
    HAL_Init();

    /* SystemInit() is already called from startup_acm32p4xx.S */
    /* Configure system clock to desired frequency */
    SystemClock_Config(SYSCLK_180M_SRC_RCH, PCLK1_DIV_SELECT, PCLK2_DIV_SELECT);

    /* Update SystemCoreClock */
    SystemCoreClockUpdate();

    /* Re-configure SysTick for RT-Thread (overriding HAL_InitTick) */
    SysTick_Config(SystemCoreClock / RT_TICK_PER_SECOND);

#ifdef RT_USING_HEAP
#if defined(__ARMCC_VERSION)
    rt_system_heap_init((void *)&Image$$RW_IRAM1$$ZI$$Limit, (void *)SOC_SRAM_END_ADDR);
#elif __ICCARM__
    rt_system_heap_init(__segment_end("HEAP"), (void *)SOC_SRAM_END_ADDR);
#else
    /* init memory system - MUST be before UART/console init for V2 serial */
    rt_system_heap_init((void *)&__bss_end, (void *)SOC_SRAM_END_ADDR);
#endif
#endif /* RT_USING_HEAP */

    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
