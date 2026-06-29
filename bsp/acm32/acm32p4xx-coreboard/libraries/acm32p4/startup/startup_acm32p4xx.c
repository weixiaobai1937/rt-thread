/**
 * @file    startup_acm32p4xx.s
 * @brief   CMSIS Star-MC1 Core Device Startup File for ACM32P4
 *          Multi-Toolchain Support (GCC, ARM MDK, IAR)
 * @note    P4 has 128KB SRAM (0x20000000), ~1016KB Flash (0x08002000)
 *          70 device-specific interrupts (IRQ 0-69)
 */

#include <stdint.h>
#include "../include/acm32p4.h"

/*----------------------------------------------------------------------------
  Compiler Detection and Macro Definitions
 *----------------------------------------------------------------------------*/
#if defined(__ARMCC_VERSION) && (__ARMCC_VERSION >= 6000000)
    #define TOOLCHAIN_ARMCC6
#elif defined(__ICCARM__)
    #define TOOLCHAIN_IAR
#elif defined(__GNUC__)
    #define TOOLCHAIN_GCC
#else
    #warning "Unknown toolchain, assuming GCC"
    #define TOOLCHAIN_GCC
#endif

/* Weak symbol macro */
#if defined(TOOLCHAIN_ARMCC6) || defined(TOOLCHAIN_GCC)
    #define WEAK __attribute__((weak))
    #define ALIAS(name) __attribute__((weak, alias(#name)))
#elif defined(TOOLCHAIN_IAR)
    #define WEAK __weak
    #define ALIAS(name)
#endif

/*----------------------------------------------------------------------------
  Stack and Heap Configuration
 *----------------------------------------------------------------------------*/
#define STACK_SIZE  0x00002000  /* 8KB Stack */
#define HEAP_SIZE   0x00001000  /* 4KB Heap */

/* Stack Memory */
#if defined(TOOLCHAIN_GCC) || defined(TOOLCHAIN_ARMCC6)
__attribute__((section(".stack"), aligned(8)))
static uint8_t stack_mem[STACK_SIZE];

__attribute__((section(".heap"), aligned(8)))
static uint8_t heap_mem[HEAP_SIZE];
#elif defined(TOOLCHAIN_IAR)
#pragma data_alignment=8
static uint8_t stack_mem[STACK_SIZE] @ ".stack";
#pragma data_alignment=8
static uint8_t heap_mem[HEAP_SIZE] @ ".heap";
#endif

/* Stack Top */
uint32_t __initial_sp = (uint32_t)(&stack_mem[STACK_SIZE]);

/*----------------------------------------------------------------------------
  External References
 *----------------------------------------------------------------------------*/
extern void SystemInit(void);
extern int main(void);

/*----------------------------------------------------------------------------
  Exception / Interrupt Vector Table (70 device-specific interrupts)
 *----------------------------------------------------------------------------*/
/* Weak default handler */
void WEAK Default_Handler(void) { while (1); }

/* Forward declarations */
void WEAK Reset_Handler(void);
void WEAK NMI_Handler(void);
void WEAK HardFault_Handler(void);
void WEAK MemManage_Handler(void);
void WEAK BusFault_Handler(void);
void WEAK UsageFault_Handler(void);
void WEAK SVC_Handler(void);
void WEAK DebugMon_Handler(void);
void WEAK PendSV_Handler(void);
void WEAK SysTick_Handler(void);

/* Device-specific interrupt handlers (IRQ 0-69) */
void WEAK WDT_IRQHandler(void);
void WEAK LVD_IRQHandler(void);
void WEAK RTC_XTLSD_IRQHandler(void);
void WEAK CLKRDY_INT_IRQHandler(void);
void WEAK EXTI0_IRQHandler(void);
void WEAK EXTI1_IRQHandler(void);
void WEAK EXTI2_IRQHandler(void);
void WEAK EXTI3_IRQHandler(void);
void WEAK EXTI4_IRQHandler(void);
void WEAK DMA1_IRQHandler(void);
void WEAK DMA2_IRQHandler(void);
void WEAK ADC_IRQHandler(void);
void WEAK DAC_IRQHandler(void);
void WEAK FDCAN1_IRQHandler(void);
void WEAK EXTI9_5_IRQHandler(void);
void WEAK TIM1_BRK_UP_TRG_COM_IRQHandler(void);
void WEAK TIM1_CC_IRQHandler(void);
void WEAK TIM2_IRQHandler(void);
void WEAK TIM3_IRQHandler(void);
void WEAK TIM6_IRQHandler(void);
void WEAK TIM7_IRQHandler(void);
void WEAK TIM8_BRK_UP_TRG_COM_IRQHandler(void);
void WEAK TIM8_CC_IRQHandler(void);
void WEAK I2C1_IRQHandler(void);
void WEAK I2C2_IRQHandler(void);
void WEAK SPI1_IRQHandler(void);
void WEAK SPI2_IRQHandler(void);
void WEAK SPI3_IRQHandler(void);
void WEAK SPI4_IRQHandler(void);
void WEAK I2S1_IRQHandler(void);
void WEAK I2S2_IRQHandler(void);
void WEAK USART1_IRQHandler(void);
void WEAK USART2_IRQHandler(void);
void WEAK USART3_IRQHandler(void);
void WEAK USART4_IRQHandler(void);
void WEAK EXTI15_10_IRQHandler(void);
void WEAK LPUART1_IRQHandler(void);
void WEAK LPUART2_IRQHandler(void);
void WEAK LPTIM1_IRQHandler(void);
void WEAK TIM4_IRQHandler(void);
void WEAK IWDT_WKUP_IRQHandler(void);
void WEAK AES_SPI1_IRQHandler(void);
void WEAK ETH_IRQHandler(void);
void WEAK ETH_WKUP_IRQHandler(void);
void WEAK USART5_IRQHandler(void);
void WEAK USART6_IRQHandler(void);
void WEAK USART7_IRQHandler(void);
void WEAK USART8_IRQHandler(void);
void WEAK TIM5_IRQHandler(void);
void WEAK TIM9_IRQHandler(void);
void WEAK TIM10_IRQHandler(void);
void WEAK TIM26_IRQHandler(void);
void WEAK SPI7_IRQHandler(void);
void WEAK OSPI1_IRQHandler(void);
void WEAK OSPI2_IRQHandler(void);
void WEAK DMA1_CH0_IRQHandler(void);
void WEAK DMA1_CH1_IRQHandler(void);
void WEAK DMA1_CH2_IRQHandler(void);
void WEAK DMA1_CH3_IRQHandler(void);
void WEAK DMA2_CH0_IRQHandler(void);
void WEAK DMA2_CH1_IRQHandler(void);
void WEAK DMA2_CH2_IRQHandler(void);
void WEAK DMA2_CH3_IRQHandler(void);
void WEAK BKPSRAM_SEC_IRQHandler(void);
void WEAK BKPSRAM_DED_IRQHandler(void);
void WEAK COMP_IRQHandler(void);
void WEAK FDCAN2_IRQHandler(void);

/*----------------------------------------------------------------------------
  Vector Table
 *----------------------------------------------------------------------------*/
#if defined(TOOLCHAIN_GCC)
__attribute__((section(".isr_vector")))
#elif defined(TOOLCHAIN_ARMCC6)
__attribute__((section("RESET")))
#elif defined(TOOLCHAIN_IAR)
#pragma section=".intvec"
#endif
const uint32_t __Vectors[] = {
    /* Core Exceptions */
    (uint32_t)&stack_mem[STACK_SIZE],      /* 0: Initial SP */
    (uint32_t)Reset_Handler,               /* 1: Reset */
    (uint32_t)NMI_Handler,                 /* 2: NMI */
    (uint32_t)HardFault_Handler,           /* 3: Hard Fault */
    (uint32_t)MemManage_Handler,           /* 4: Mem Manage */
    (uint32_t)BusFault_Handler,            /* 5: Bus Fault */
    (uint32_t)UsageFault_Handler,          /* 6: Usage Fault */
    0, 0, 0, 0,                            /* Reserved */
    (uint32_t)SVC_Handler,                 /* 11: SVCall */
    (uint32_t)DebugMon_Handler,            /* 12: Debug Monitor */
    0,                                     /* Reserved */
    (uint32_t)PendSV_Handler,              /* 14: PendSV */
    (uint32_t)SysTick_Handler,             /* 15: SysTick */

    /* Device Specific Interrupts (IRQ 0-69) */
    (uint32_t)WDT_IRQHandler,              /* 0: WDT */
    (uint32_t)LVD_IRQHandler,              /* 1: LVD */
    (uint32_t)RTC_XTLSD_IRQHandler,        /* 2: RTC */
    (uint32_t)CLKRDY_INT_IRQHandler,       /* 3: Clock Ready */
    (uint32_t)EXTI0_IRQHandler,            /* 4: EXTI0 */
    (uint32_t)EXTI1_IRQHandler,            /* 5: EXTI1 */
    (uint32_t)EXTI2_IRQHandler,            /* 6: EXTI2 */
    (uint32_t)EXTI3_IRQHandler,            /* 7: EXTI3 */
    (uint32_t)EXTI4_IRQHandler,            /* 8: EXTI4 */
    (uint32_t)DMA1_IRQHandler,             /* 9: DMA1 */
    (uint32_t)DMA2_IRQHandler,             /* 10: DMA2 */
    (uint32_t)ADC_IRQHandler,              /* 11: ADC */
    (uint32_t)DAC_IRQHandler,              /* 12: DAC */
    0,                                     /* 13: Reserved */
    (uint32_t)FDCAN1_IRQHandler,           /* 14: FDCAN1 */
    (uint32_t)EXTI9_5_IRQHandler,          /* 15: EXTI9-5 */
    (uint32_t)TIM1_BRK_UP_TRG_COM_IRQHandler, /* 16: TIM1 */
    (uint32_t)TIM1_CC_IRQHandler,          /* 17: TIM1 CC */
    (uint32_t)TIM2_IRQHandler,             /* 18: TIM2 */
    (uint32_t)TIM3_IRQHandler,             /* 19: TIM3 */
    (uint32_t)TIM6_IRQHandler,             /* 20: TIM6 */
    (uint32_t)TIM7_IRQHandler,             /* 21: TIM7 */
    (uint32_t)TIM8_BRK_UP_TRG_COM_IRQHandler, /* 22: TIM8 */
    (uint32_t)TIM8_CC_IRQHandler,          /* 23: TIM8 CC */
    (uint32_t)I2C1_IRQHandler,             /* 24: I2C1 */
    (uint32_t)I2C2_IRQHandler,             /* 25: I2C2 */
(uint32_t)SPI1_IRQHandler,            /* 26: SPI1 */
(uint32_t)SPI2_IRQHandler,            /* 27: SPI2 */
(uint32_t)SPI3_IRQHandler,            /* 28: SPI3 */
(uint32_t)SPI4_IRQHandler,            /* 29: SPI4 */
    (uint32_t)I2S1_IRQHandler,             /* 30: I2S1 */
    (uint32_t)I2S2_IRQHandler,             /* 31: I2S2 */
    (uint32_t)USART1_IRQHandler,            /* 32: USART1 */
    (uint32_t)USART2_IRQHandler,            /* 33: USART2 */
    (uint32_t)USART3_IRQHandler,            /* 34: USART3 */
    (uint32_t)USART4_IRQHandler,            /* 35: USART4 */
    (uint32_t)EXTI15_10_IRQHandler,        /* 36: EXTI15-10 */
    0,                                     /* 37: Reserved */
    (uint32_t)LPUART1_IRQHandler,          /* 38: LPUART1 */
    (uint32_t)LPUART2_IRQHandler,          /* 39: LPUART2 */
    (uint32_t)LPTIM1_IRQHandler,           /* 40: LPTIM1 */
    0,                                     /* 41: Reserved */
    (uint32_t)TIM4_IRQHandler,             /* 42: TIM4 */
    (uint32_t)IWDT_WKUP_IRQHandler,        /* 43: IWDT */
    (uint32_t)AES_SPI1_IRQHandler,         /* 44: AES/SPI1 */
    (uint32_t)ETH_IRQHandler,              /* 45: ETH */
    (uint32_t)ETH_WKUP_IRQHandler,         /* 46: ETH WKUP */
    (uint32_t)USART5_IRQHandler,            /* 47: USART5 */
    (uint32_t)USART6_IRQHandler,            /* 48: USART6 */
    (uint32_t)USART7_IRQHandler,            /* 49: USART7 */
    (uint32_t)USART8_IRQHandler,            /* 50: USART8 */
    (uint32_t)TIM5_IRQHandler,             /* 51: TIM5 */
    (uint32_t)TIM9_IRQHandler,             /* 52: TIM9 */
    (uint32_t)TIM10_IRQHandler,            /* 53: TIM10 */
    (uint32_t)TIM26_IRQHandler,            /* 54: TIM26 */
    (uint32_t)SPI7_IRQHandler,            /* 55: SPI7 */
    (uint32_t)OSPI1_IRQHandler,            /* 56: OSPI1 */
    (uint32_t)OSPI2_IRQHandler,            /* 57: OSPI2 */
    (uint32_t)DMA1_CH0_IRQHandler,         /* 58: DMA1 CH0 */
    (uint32_t)DMA1_CH1_IRQHandler,         /* 59: DMA1 CH1 */
    (uint32_t)DMA1_CH2_IRQHandler,         /* 60: DMA1 CH2 */
    (uint32_t)DMA1_CH3_IRQHandler,         /* 61: DMA1 CH3 */
    (uint32_t)DMA2_CH0_IRQHandler,         /* 62: DMA2 CH0 */
    (uint32_t)DMA2_CH1_IRQHandler,         /* 63: DMA2 CH1 */
    (uint32_t)DMA2_CH2_IRQHandler,         /* 64: DMA2 CH2 */
    (uint32_t)DMA2_CH3_IRQHandler,         /* 65: DMA2 CH3 */
    (uint32_t)BKPSRAM_SEC_IRQHandler,      /* 66: BKPSRAM SEC */
    (uint32_t)BKPSRAM_DED_IRQHandler,      /* 67: BKPSRAM DED */
    (uint32_t)COMP_IRQHandler,             /* 68: COMP */
    (uint32_t)FDCAN2_IRQHandler,           /* 69: FDCAN2 */
};

/*----------------------------------------------------------------------------
  Reset Handler
 *----------------------------------------------------------------------------*/
void Reset_Handler(void)
{
    SystemInit();

#if defined(TOOLCHAIN_ARMCC6)
    extern void __main(void);
    __main();
#elif defined(TOOLCHAIN_GCC)
    extern uint32_t _sidata, _sdata, _edata, _sbss, _ebss;
    /* Copy data section */
    for (uint32_t *src = &_sidata, *dst = &_sdata; dst < &_edata; src++, dst++) {
        *dst = *src;
    }
    /* Clear BSS section */
    for (uint32_t *dst = &_sbss; dst < &_ebss; dst++) {
        *dst = 0;
    }
    main();
#elif defined(TOOLCHAIN_IAR)
    __iar_program_start();
#endif

    while (1);
}

/* Alias all interrupt handlers to Default_Handler */
void WEAK NMI_Handler(void)                    { Default_Handler(); }
void WEAK HardFault_Handler(void)              { Default_Handler(); }
void WEAK MemManage_Handler(void)              { Default_Handler(); }
void WEAK BusFault_Handler(void)               { Default_Handler(); }
void WEAK UsageFault_Handler(void)             { Default_Handler(); }
void WEAK SVC_Handler(void)                    { Default_Handler(); }
void WEAK DebugMon_Handler(void)               { Default_Handler(); }
void WEAK PendSV_Handler(void)                 { Default_Handler(); }
void WEAK SysTick_Handler(void)                { Default_Handler(); }

/* Device interrupt weak aliases */
void WEAK WDT_IRQHandler(void)                 { Default_Handler(); }
void WEAK LVD_IRQHandler(void)                 { Default_Handler(); }
void WEAK RTC_XTLSD_IRQHandler(void)           { Default_Handler(); }
void WEAK CLKRDY_INT_IRQHandler(void)          { Default_Handler(); }
void WEAK EXTI0_IRQHandler(void)               { Default_Handler(); }
void WEAK EXTI1_IRQHandler(void)               { Default_Handler(); }
void WEAK EXTI2_IRQHandler(void)               { Default_Handler(); }
void WEAK EXTI3_IRQHandler(void)               { Default_Handler(); }
void WEAK EXTI4_IRQHandler(void)               { Default_Handler(); }
void WEAK DMA1_IRQHandler(void)                { Default_Handler(); }
void WEAK DMA2_IRQHandler(void)                { Default_Handler(); }
void WEAK ADC_IRQHandler(void)                 { Default_Handler(); }
void WEAK DAC_IRQHandler(void)                 { Default_Handler(); }
void WEAK FDCAN1_IRQHandler(void)              { Default_Handler(); }
void WEAK EXTI9_5_IRQHandler(void)             { Default_Handler(); }
void WEAK TIM1_BRK_UP_TRG_COM_IRQHandler(void) { Default_Handler(); }
void WEAK TIM1_CC_IRQHandler(void)             { Default_Handler(); }
void WEAK TIM2_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM3_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM6_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM7_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM8_BRK_UP_TRG_COM_IRQHandler(void) { Default_Handler(); }
void WEAK TIM8_CC_IRQHandler(void)             { Default_Handler(); }
void WEAK I2C1_IRQHandler(void)                { Default_Handler(); }
void WEAK I2C2_IRQHandler(void)                { Default_Handler(); }
void WEAK SPI1_IRQHandler(void)               { Default_Handler(); }
void WEAK SPI2_IRQHandler(void)               { Default_Handler(); }
void WEAK SPI3_IRQHandler(void)               { Default_Handler(); }
void WEAK SPI4_IRQHandler(void)               { Default_Handler(); }
void WEAK I2S1_IRQHandler(void)                { Default_Handler(); }
void WEAK I2S2_IRQHandler(void)                { Default_Handler(); }
void WEAK USART1_IRQHandler(void)               { Default_Handler(); }
void WEAK USART2_IRQHandler(void)               { Default_Handler(); }
void WEAK USART3_IRQHandler(void)               { Default_Handler(); }
void WEAK USART4_IRQHandler(void)               { Default_Handler(); }
void WEAK EXTI15_10_IRQHandler(void)           { Default_Handler(); }
void WEAK LPUART1_IRQHandler(void)             { Default_Handler(); }
void WEAK LPUART2_IRQHandler(void)             { Default_Handler(); }
void WEAK LPTIM1_IRQHandler(void)              { Default_Handler(); }
void WEAK TIM4_IRQHandler(void)                { Default_Handler(); }
void WEAK IWDT_WKUP_IRQHandler(void)           { Default_Handler(); }
void WEAK AES_SPI1_IRQHandler(void)             { Default_Handler(); }
void WEAK ETH_IRQHandler(void)                 { Default_Handler(); }
void WEAK ETH_WKUP_IRQHandler(void)            { Default_Handler(); }
void WEAK USART5_IRQHandler(void)               { Default_Handler(); }
void WEAK USART6_IRQHandler(void)               { Default_Handler(); }
void WEAK USART7_IRQHandler(void)               { Default_Handler(); }
void WEAK USART8_IRQHandler(void)               { Default_Handler(); }
void WEAK TIM5_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM9_IRQHandler(void)                { Default_Handler(); }
void WEAK TIM10_IRQHandler(void)               { Default_Handler(); }
void WEAK TIM26_IRQHandler(void)               { Default_Handler(); }
void WEAK SPI7_IRQHandler(void)               { Default_Handler(); }
void WEAK OSPI1_IRQHandler(void)               { Default_Handler(); }
void WEAK OSPI2_IRQHandler(void)               { Default_Handler(); }
void WEAK DMA1_CH0_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA1_CH1_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA1_CH2_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA1_CH3_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA2_CH0_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA2_CH1_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA2_CH2_IRQHandler(void)            { Default_Handler(); }
void WEAK DMA2_CH3_IRQHandler(void)            { Default_Handler(); }
void WEAK BKPSRAM_SEC_IRQHandler(void)         { Default_Handler(); }
void WEAK BKPSRAM_DED_IRQHandler(void)         { Default_Handler(); }
void WEAK COMP_IRQHandler(void)                { Default_Handler(); }
void WEAK FDCAN2_IRQHandler(void)              { Default_Handler(); }