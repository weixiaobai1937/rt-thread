/**
  ******************************************************************************
  * @file      startup_acm32p4xx_gcc.S
  * @brief     ACM32P4xx startup for GCC (Cortex-M33 / Star-MC1).
  ******************************************************************************
  */

    .syntax unified
    .cpu    cortex-m33
    .fpu    softvfp
    .thumb

/* Stack and Heap sizes */
    .equ    Stack_Size, 0x4000
    .equ    Heap_Size,  0x1000

    .global __initial_sp
    .global __heap_start__
    .global __heap_end__

/* Linker-defined symbols */
    .word   _sidata
    .word   _sdata
    .word   _edata
    .word   _sbss
    .word   _ebss

/* ==========================================================================
   Vector Table
   ========================================================================== */
    .section .isr_vector, "a", %progbits
    .align  2

    .long   __initial_sp                /* Top of stack */
    .long   Reset_Handler               /* Reset */
    .long   NMI_Handler                 /* NMI */
    .long   HardFault_Handler           /* HardFault */
    .long   MemManage_Handler           /* MemManage */
    .long   BusFault_Handler            /* BusFault */
    .long   UsageFault_Handler          /* UsageFault */
    .long   0                           /* Reserved */
    .long   0                           /* Reserved */
    .long   0                           /* Reserved */
    .long   0                           /* Reserved */
    .long   SVC_Handler                 /* SVCall */
    .long   DebugMon_Handler            /* Debug Monitor */
    .long   0                           /* Reserved */
    .long   PendSV_Handler              /* PendSV */
    .long   SysTick_Handler             /* SysTick */

/* External Interrupts */
    .long   WDT_IRQHandler              /* IRQ0 */
    .long   LVD_IRQHandler              /* IRQ1 */
    .long   RTC_XTLSD_IRQHandler        /* IRQ2 */
    .long   CLKRDY_IRQHandler           /* IRQ3 */
    .long   EXTI0_IRQHandler            /* IRQ4 */
    .long   EXTI1_IRQHandler            /* IRQ5 */
    .long   EXTI2_IRQHandler            /* IRQ6 */
    .long   EXTI3_IRQHandler            /* IRQ7 */
    .long   EXTI4_IRQHandler            /* IRQ8 */
    .long   DMA1_IRQHandler             /* IRQ9 */
    .long   DMA2_IRQHandler             /* IRQ10 */
    .long   ADC_IRQHandler              /* IRQ11 */
    .long   DAC_IRQHandler              /* IRQ12 */
    .long   FSUSB_IRQHandler            /* IRQ13 */
    .long   FDCAN1_IRQHandler           /* IRQ14 */
    .long   EXTI9_5_IRQHandler          /* IRQ15 */
    .long   TIM1_BRK_UP_TRG_COM_IRQHandler  /* IRQ16 */
    .long   TIM1_CC_IRQHandler          /* IRQ17 */
    .long   TIM2_IRQHandler             /* IRQ18 */
    .long   TIM3_IRQHandler             /* IRQ19 */
    .long   TIM6_IRQHandler             /* IRQ20 */
    .long   TIM7_IRQHandler             /* IRQ21 */
    .long   TIM8_BRK_UP_TRG_COM_IRQHandler  /* IRQ22 */
    .long   TIM8_CC_IRQHandler          /* IRQ23 */
    .long   I2C1_IRQHandler             /* IRQ24 */
    .long   I2C2_IRQHandler             /* IRQ25 */
    .long   SPI1_IRQHandler             /* IRQ26 */
    .long   SPI2_IRQHandler             /* IRQ27 */
    .long   SPI3_IRQHandler             /* IRQ28 */
    .long   SPI4_IRQHandler             /* IRQ29 */
    .long   I2S1_IRQHandler             /* IRQ30 */
    .long   I2S2_IRQHandler             /* IRQ31 */
    .long   USART1_IRQHandler           /* IRQ32 */
    .long   USART2_IRQHandler           /* IRQ33 */
    .long   USART3_IRQHandler           /* IRQ34 */
    .long   USART4_IRQHandler           /* IRQ35 */
    .long   EXTI15_10_IRQHandler        /* IRQ36 */
    .long   USB_WKUP_IRQHandler         /* IRQ37 */
    .long   LPUART1_IRQHandler          /* IRQ38 */
    .long   LPUART2_IRQHandler          /* IRQ39 */
    .long   LPTIM1_IRQHandler           /* IRQ40 */
    .long   FPU_IRQHandler              /* IRQ41 */
    .long   TIM4_IRQHandler             /* IRQ42 */
    .long   IWDT_WKUP_IRQHandler        /* IRQ43 */
    .long   AES_SPI1_IRQHandler         /* IRQ44 */
    .long   ETH_IRQHandler              /* IRQ45 */
    .long   ETH_WKUP_IRQHandler         /* IRQ46 */
    .long   USART5_IRQHandler           /* IRQ47 */
    .long   USART6_IRQHandler           /* IRQ48 */
    .long   USART7_IRQHandler           /* IRQ49 */
    .long   USART8_IRQHandler           /* IRQ50 */
    .long   TIM5_IRQHandler             /* IRQ51 */
    .long   TIM9_IRQHandler             /* IRQ52 */
    .long   TIM10_IRQHandler            /* IRQ53 */
    .long   TIM26_IRQHandler            /* IRQ54 */
    .long   SPI7_IRQHandler             /* IRQ55 */
    .long   OSPI1_IRQHandler            /* IRQ56 */
    .long   OSPI2_IRQHandler            /* IRQ57 */
    .long   DMA1_CH0_IRQHandler         /* IRQ58 */
    .long   DMA1_CH1_IRQHandler         /* IRQ59 */
    .long   DMA1_CH2_IRQHandler         /* IRQ60 */
    .long   DMA1_CH3_IRQHandler         /* IRQ61 */
    .long   DMA2_CH0_IRQHandler         /* IRQ62 */
    .long   DMA2_CH1_IRQHandler         /* IRQ63 */
    .long   DMA2_CH2_IRQHandler         /* IRQ64 */
    .long   DMA2_CH3_IRQHandler         /* IRQ65 */
    .long   BKPSRAM_SEC_IRQHandler      /* IRQ66 */
    .long   BKPSRAM_DED_IRQHandler      /* IRQ67 */
    .long   COMP1_IRQHandler            /* IRQ68 */
    .long   FDCAN2_IRQHandler           /* IRQ69 */
    .long   RSV70_IRQHandler            /* IRQ70 */
    .long   RSV71_IRQHandler            /* IRQ71 */
    .long   SDMMC_IRQHandler            /* IRQ72 */

/* ==========================================================================
   Reset Handler
   ========================================================================== */
    .section .text.Reset_Handler, "ax", %progbits
    .weak   Reset_Handler
    .type   Reset_Handler, %function
Reset_Handler:
    /* Copy .data from flash to SRAM */
    ldr     r0, =_sdata
    ldr     r1, =_edata
    ldr     r2, =_sidata
    movs    r3, #0
    b       .L_loop_copy_data

.L_copy_data:
    ldr     r4, [r2, r3]
    str     r4, [r0, r3]
    adds    r3, r3, #4

.L_loop_copy_data:
    adds    r4, r0, r3
    cmp     r4, r1
    bcc     .L_copy_data

    /* Zero .bss */
    ldr     r2, =_sbss
    ldr     r4, =_ebss
    movs    r3, #0
    b       .L_loop_zero_bss

.L_zero_bss:
    str     r3, [r2]
    adds    r2, r2, #4

.L_loop_zero_bss:
    cmp     r2, r4
    bcc     .L_zero_bss

    /* Call system init */
    bl      system_init

    /* Call static constructors */
    bl      __libc_init_array

    /* Call main */
    bl      main

.L_dead_loop:
    b       .L_dead_loop

    .size   Reset_Handler, .-Reset_Handler

/* ==========================================================================
   Default Handler
   ========================================================================== */
    .section .text.Default_Handler, "ax", %progbits
    .weak   Default_Handler
    .type   Default_Handler, %function
Default_Handler:
    b       .

    .size   Default_Handler, .-Default_Handler

/* ==========================================================================
   Weak aliases for all handlers -> Default_Handler
   ========================================================================== */
    .weak   NMI_Handler
    .weak   HardFault_Handler
    .weak   MemManage_Handler
    .weak   BusFault_Handler
    .weak   UsageFault_Handler
    .weak   SVC_Handler
    .weak   DebugMon_Handler
    .weak   PendSV_Handler
    .weak   SysTick_Handler

    .weak   WDT_IRQHandler
    .weak   LVD_IRQHandler
    .weak   RTC_XTLSD_IRQHandler
    .weak   CLKRDY_IRQHandler
    .weak   EXTI0_IRQHandler
    .weak   EXTI1_IRQHandler
    .weak   EXTI2_IRQHandler
    .weak   EXTI3_IRQHandler
    .weak   EXTI4_IRQHandler
    .weak   DMA1_IRQHandler
    .weak   DMA2_IRQHandler
    .weak   ADC_IRQHandler
    .weak   DAC_IRQHandler
    .weak   FSUSB_IRQHandler
    .weak   FDCAN1_IRQHandler
    .weak   EXTI9_5_IRQHandler
    .weak   TIM1_BRK_UP_TRG_COM_IRQHandler
    .weak   TIM1_CC_IRQHandler
    .weak   TIM2_IRQHandler
    .weak   TIM3_IRQHandler
    .weak   TIM6_IRQHandler
    .weak   TIM7_IRQHandler
    .weak   TIM8_BRK_UP_TRG_COM_IRQHandler
    .weak   TIM8_CC_IRQHandler
    .weak   I2C1_IRQHandler
    .weak   I2C2_IRQHandler
    .weak   SPI1_IRQHandler
    .weak   SPI2_IRQHandler
    .weak   SPI3_IRQHandler
    .weak   SPI4_IRQHandler
    .weak   I2S1_IRQHandler
    .weak   I2S2_IRQHandler
    .weak   USART1_IRQHandler
    .weak   USART2_IRQHandler
    .weak   USART3_IRQHandler
    .weak   USART4_IRQHandler
    .weak   EXTI15_10_IRQHandler
    .weak   USB_WKUP_IRQHandler
    .weak   LPUART1_IRQHandler
    .weak   LPUART2_IRQHandler
    .weak   LPTIM1_IRQHandler
    .weak   FPU_IRQHandler
    .weak   TIM4_IRQHandler
    .weak   IWDT_WKUP_IRQHandler
    .weak   AES_SPI1_IRQHandler
    .weak   ETH_IRQHandler
    .weak   ETH_WKUP_IRQHandler
    .weak   USART5_IRQHandler
    .weak   USART6_IRQHandler
    .weak   USART7_IRQHandler
    .weak   USART8_IRQHandler
    .weak   TIM5_IRQHandler
    .weak   TIM9_IRQHandler
    .weak   TIM10_IRQHandler
    .weak   TIM26_IRQHandler
    .weak   SPI7_IRQHandler
    .weak   OSPI1_IRQHandler
    .weak   OSPI2_IRQHandler
    .weak   DMA1_CH0_IRQHandler
    .weak   DMA1_CH1_IRQHandler
    .weak   DMA1_CH2_IRQHandler
    .weak   DMA1_CH3_IRQHandler
    .weak   DMA2_CH0_IRQHandler
    .weak   DMA2_CH1_IRQHandler
    .weak   DMA2_CH2_IRQHandler
    .weak   DMA2_CH3_IRQHandler
    .weak   BKPSRAM_SEC_IRQHandler
    .weak   BKPSRAM_DED_IRQHandler
    .weak   COMP1_IRQHandler
    .weak   FDCAN2_IRQHandler
    .weak   RSV70_IRQHandler
    .weak   RSV71_IRQHandler
    .weak   SDMMC_IRQHandler

    .thumb_set NMI_Handler,                 Default_Handler
    .thumb_set HardFault_Handler,           Default_Handler
    .thumb_set MemManage_Handler,           Default_Handler
    .thumb_set BusFault_Handler,            Default_Handler
    .thumb_set UsageFault_Handler,          Default_Handler
    .thumb_set SVC_Handler,                 Default_Handler
    .thumb_set DebugMon_Handler,            Default_Handler
    .thumb_set PendSV_Handler,              Default_Handler
    .thumb_set SysTick_Handler,             Default_Handler

    .thumb_set WDT_IRQHandler,              Default_Handler
    .thumb_set LVD_IRQHandler,              Default_Handler
    .thumb_set RTC_XTLSD_IRQHandler,        Default_Handler
    .thumb_set CLKRDY_IRQHandler,           Default_Handler
    .thumb_set EXTI0_IRQHandler,            Default_Handler
    .thumb_set EXTI1_IRQHandler,            Default_Handler
    .thumb_set EXTI2_IRQHandler,            Default_Handler
    .thumb_set EXTI3_IRQHandler,            Default_Handler
    .thumb_set EXTI4_IRQHandler,            Default_Handler
    .thumb_set DMA1_IRQHandler,             Default_Handler
    .thumb_set DMA2_IRQHandler,             Default_Handler
    .thumb_set ADC_IRQHandler,              Default_Handler
    .thumb_set DAC_IRQHandler,              Default_Handler
    .thumb_set FSUSB_IRQHandler,            Default_Handler
    .thumb_set FDCAN1_IRQHandler,           Default_Handler
    .thumb_set EXTI9_5_IRQHandler,          Default_Handler
    .thumb_set TIM1_BRK_UP_TRG_COM_IRQHandler, Default_Handler
    .thumb_set TIM1_CC_IRQHandler,          Default_Handler
    .thumb_set TIM2_IRQHandler,             Default_Handler
    .thumb_set TIM3_IRQHandler,             Default_Handler
    .thumb_set TIM6_IRQHandler,             Default_Handler
    .thumb_set TIM7_IRQHandler,             Default_Handler
    .thumb_set TIM8_BRK_UP_TRG_COM_IRQHandler, Default_Handler
    .thumb_set TIM8_CC_IRQHandler,          Default_Handler
    .thumb_set I2C1_IRQHandler,             Default_Handler
    .thumb_set I2C2_IRQHandler,             Default_Handler
    .thumb_set SPI1_IRQHandler,             Default_Handler
    .thumb_set SPI2_IRQHandler,             Default_Handler
    .thumb_set SPI3_IRQHandler,             Default_Handler
    .thumb_set SPI4_IRQHandler,             Default_Handler
    .thumb_set I2S1_IRQHandler,             Default_Handler
    .thumb_set I2S2_IRQHandler,             Default_Handler
    .thumb_set USART1_IRQHandler,           Default_Handler
    .thumb_set USART2_IRQHandler,           Default_Handler
    .thumb_set USART3_IRQHandler,           Default_Handler
    .thumb_set USART4_IRQHandler,           Default_Handler
    .thumb_set EXTI15_10_IRQHandler,        Default_Handler
    .thumb_set USB_WKUP_IRQHandler,         Default_Handler
    .thumb_set LPUART1_IRQHandler,          Default_Handler
    .thumb_set LPUART2_IRQHandler,          Default_Handler
    .thumb_set LPTIM1_IRQHandler,           Default_Handler
    .thumb_set FPU_IRQHandler,              Default_Handler
    .thumb_set TIM4_IRQHandler,             Default_Handler
    .thumb_set IWDT_WKUP_IRQHandler,        Default_Handler
    .thumb_set AES_SPI1_IRQHandler,         Default_Handler
    .thumb_set ETH_IRQHandler,              Default_Handler
    .thumb_set ETH_WKUP_IRQHandler,         Default_Handler
    .thumb_set USART5_IRQHandler,           Default_Handler
    .thumb_set USART6_IRQHandler,           Default_Handler
    .thumb_set USART7_IRQHandler,           Default_Handler
    .thumb_set USART8_IRQHandler,           Default_Handler
    .thumb_set TIM5_IRQHandler,             Default_Handler
    .thumb_set TIM9_IRQHandler,             Default_Handler
    .thumb_set TIM10_IRQHandler,            Default_Handler
    .thumb_set TIM26_IRQHandler,            Default_Handler
    .thumb_set SPI7_IRQHandler,             Default_Handler
    .thumb_set OSPI1_IRQHandler,            Default_Handler
    .thumb_set OSPI2_IRQHandler,            Default_Handler
    .thumb_set DMA1_CH0_IRQHandler,         Default_Handler
    .thumb_set DMA1_CH1_IRQHandler,         Default_Handler
    .thumb_set DMA1_CH2_IRQHandler,         Default_Handler
    .thumb_set DMA1_CH3_IRQHandler,         Default_Handler
    .thumb_set DMA2_CH0_IRQHandler,         Default_Handler
    .thumb_set DMA2_CH1_IRQHandler,         Default_Handler
    .thumb_set DMA2_CH2_IRQHandler,         Default_Handler
    .thumb_set DMA2_CH3_IRQHandler,         Default_Handler
    .thumb_set BKPSRAM_SEC_IRQHandler,      Default_Handler
    .thumb_set BKPSRAM_DED_IRQHandler,      Default_Handler
    .thumb_set COMP1_IRQHandler,            Default_Handler
    .thumb_set FDCAN2_IRQHandler,           Default_Handler
    .thumb_set RSV70_IRQHandler,            Default_Handler
    .thumb_set RSV71_IRQHandler,            Default_Handler
    .thumb_set SDMMC_IRQHandler,            Default_Handler

/* ==========================================================================
   Stack Section
   ========================================================================== */
    .section .stack, "aw", %nobits
    .align  3
    .space  Stack_Size
    .size   __initial_sp, . - __initial_sp

/* ==========================================================================
   Heap Section
   ========================================================================== */
    .section .heap, "aw", %nobits
    .align  3
__heap_start__:
    .space  Heap_Size
__heap_end__:

    .end
