#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define SOC_SRAM_END_ADDR   (SOC_SRAM_START_ADDR + SOC_SRAM_SIZE * 1024)

extern int __bss_end__;
extern void rt_hw_uart_init(void);

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

void rt_hw_board_init(void)
{
    /* 1. ACM32P4 SDK system init (clock/NVIC/TIM64B/PSRAM) */
    system_init();

    /* 2. Configure SysTick */
    sys_info_t info;
    system_get_info(&info);
    SysTick_Config(info.sysclk_hz / RT_TICK_PER_SECOND);

    /* 3. UART + console */
    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

    /* 4. Heap memory */
#ifdef RT_USING_HEAP
    rt_system_heap_init((void *)&__bss_end__, (void *)SOC_SRAM_END_ADDR);
#endif

    /* 5. Component init */
#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
