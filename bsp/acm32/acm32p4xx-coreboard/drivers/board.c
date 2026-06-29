#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"

#define SOC_SRAM_END_ADDR   (SOC_SRAM_START_ADDR + SOC_SRAM_SIZE * 1024)

extern int  rt_application_init(void);

#if defined(__ARMCC_VERSION)
    extern int Image$$RW_IRAM1$$ZI$$Limit;
#elif __ICCARM__
    #pragma section="HEAP"
#else
    extern int __bss_end__;
#endif

extern void rt_hw_uart_init(void);

void SysTick_Handler(void)
{
    rt_interrupt_enter();
    rt_tick_increase();
    rt_interrupt_leave();
}

void rt_hw_board_init(void)
{
    system_init();

    sys_info_t info;
    system_get_info(&info);
    SysTick_Config(info.sysclk_hz / RT_TICK_PER_SECOND);

#ifdef RT_USING_HEAP
#if defined(__ARMCC_VERSION)
    rt_system_heap_init((void *)&Image$$RW_IRAM1$$ZI$$Limit, (void *)SOC_SRAM_END_ADDR);
#elif __ICCARM__
    rt_system_heap_init(__segment_end("HEAP"), (void *)SOC_SRAM_END_ADDR);
#else
    rt_system_heap_init((void *)&__bss_end__, (void *)SOC_SRAM_END_ADDR);
#endif
#endif

    rt_hw_uart_init();
    rt_console_set_device(RT_CONSOLE_DEVICE_NAME);

#ifdef RT_USING_COMPONENTS_INIT
    rt_components_board_init();
#endif
}
