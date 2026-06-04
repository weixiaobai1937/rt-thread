/**
  *************** (C) COPYRIGHT 2025 aisinochip ************************
  * @file      startup_acm32p4xx.S
  * @brief     acm32p4xx Devices vector table for Atollic toolchain.
  *            This module performs:
  *                - Set the initial SP
  *                - Set the initial PC == Reset_Handler,
  *                - Set the vector table entries with the exceptions ISR address
  *                - Configure the clock system   
  *                - Branches to main in the C library (which eventually
  *                  calls main()).
  *            After Reset the Star-MC1 processor is in Thread mode,
  *            priority is Privileged, and the Stack is set to Main.
  ******************************************************************************
  */

    .syntax unified
    .cpu cortex-m33
    .fpu softvfp 
    .thumb

    .global g_pfnVectors
    .global Default_Handler

/* start address for the initialization values of the .data section.
defined in linker script */
    .word _sidata
/* start address for the .data section. defined in linker script */
    .word _sdata
/* end address for the .data section. defined in linker script */
    .word _edata
/* start address for the .bss section. defined in linker script */
    .word _sbss
/* end address for the .bss section. defined in linker script */
    .word _ebss

/**
 * @brief  This is the code that gets called when the processor first
 *          starts execution following a reset event. Only the absolutely
 *          necessary set is performed, after which the application
 *          supplied main() routine is called.
 * @param  None
 * @retval : None
*/

    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
/* Enable DTCM */ 
    movw r0, #0xe014
    movt r0, #0xe001    
    ldr r1, [r0,#0]
    orr r1, r1,#1
    str r1, [r0,#0]
    
/* Call the clock system intitialization function.*/    
    bl  SystemInit
 
/* Copy the data segment initializers from flash to SRAM */
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
    movs r3, #0
    b LoopCopyDataInit

CopyDataInit:
    ldr r4, [r2, r3]
    str r4, [r0, r3]
    adds r3, r3, #4

LoopCopyDataInit:
    adds r4, r0, r3
    cmp r4, r1
    bcc CopyDataInit
  
/* Zero fill the bss segment. */
    ldr r2, =_sbss
    ldr r4, =_ebss
    movs r3, #0
    b LoopFillZerobss

FillZerobss:
    str  r3, [r2]
    adds r2, r2, #4

LoopFillZerobss:
    cmp r2, r4
    bcc FillZerobss

/* Call static constructors */
  /*  bl __libc_init_array */
  
/* Call the applications entry point.*/
    bl entry
  
LoopForever:
    b LoopForever
    
.size Reset_Handler, .-Reset_Handler

/**
 * @brief  This is the code that gets called when the processor receives an
 *         unexpected interrupt.  This simply enters an infinite loop, preserving
 *         the system state for examination by a debugger.
 *
 * @param  None
 * @retval : None
*/
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler
/******************************************************************************
*
* The minimal vector table for a Cortex M3.  Note that the proper constructs
* must be placed on this to ensure that it ends up at physical address
* 0x0000.0000.
*
******************************************************************************/
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object
    .size g_pfnVectors, .-g_pfnVectors

g_pfnVectors:
    .word _estack                           // top of stack 
    .word Reset_Handler                     // -14
    .word NMI_Handler                       // -13
    .word HardFault_Handler                 // -12
    .word MemManage_Handler                 // -12
    .word BusFault_Handler                  // -11
    .word UsageFault_Handler                // -10
    .word 0                                 // -9
    .word 0                                 // -8
    .word 0                                 // -7
    .word 0                                 // -6
    .word SVC_Handler                       // -5
    .word DebugMon_Handler                  // -4
    .word 0                                 // -3
    .word PendSV_Handler                    // -2
    .word SysTick_Handler                   // -0

/* External Interrupts */
    .word     WDT_IRQHandler				    // 0 
    .word     LVD_IRQHandler				    // 1				
    .word     RTC_XTLSD_IRQHandler              // 2
    .word     CLKRDY_IRQHandler                 // 3
    .word     EXTI0_IRQHandler		            // 4
    .word     EXTI1_IRQHandler		            // 5
    .word     EXTI2_IRQHandler		            // 6
    .word     EXTI3_IRQHandler		            // 7
    .word     EXTI4_IRQHandler		            // 8
    .word     DMA1_IRQHandler		            // 9                   
    .word     DMA2_IRQHandler		            // 10
    .word     ADC_IRQHandler		            // 11
    .word     DAC_IRQHandler		            // 12
    .word     FSUSB_IRQHandler	                // 13                      
    .word     FDCAN1_IRQHandler                 // 14		
    .word     EXTI9_5_IRQHandler                // 15
    .word     TIM1_BRK_UP_TRG_COM_IRQHandler    // 16
    .word     TIM1_CC_IRQHandler                // 17
    .word     TIM2_IRQHandler	                // 18
    .word     TIM3_IRQHandler                   // 19
    .word     TIM6_IRQHandler	                // 20
    .word    	TIM7_IRQHandler		            // 21
    .word    	TIM8_BRK_UP_TRG_COM_IRQHandler  // 22	
    .word     TIM8_CC_IRQHandler                // 23	
    .word     I2C1_IRQHandler                   // 24
    .word     I2C2_IRQHandler                   // 25                   
    .word     SPI1_IRQHandler                   // 26                     
    .word     SPI2_IRQHandler	                // 27               
    .word     SPI3_IRQHandler                   // 28
    .word     SPI4_IRQHandler                   // 29
    .word     I2S1_IRQHandler	                // 30
    .word     I2S2_IRQHandler	                // 31
    .word     USART1_IRQHandler                 // 32                    
    .word     USART2_IRQHandler		    	    // 33
    .word     USART3_IRQHandler		  	        // 34                    
    .word     USART4_IRQHandler		    	    // 35
    .word     EXTI15_10_IRQHandler	    	    // 36  
    .word     USB_WKUP_IRQHandler               // 37
    .word     LPUART1_IRQHandler                // 38 
    .word     LPUART2_IRQHandler                // 39                
    .word     LPTIM1_IRQHandler                 // 40
    .word     FPU_IRQHandler                    // 41
    .word     TIM4_IRQHandler                   // 42
    .word     IWDT_WKUP_IRQHandler              // 43
    .word     AES_SPI1_IRQHandler               // 44
    .word     ETH_IRQHandler                    // 45
    .word     ETH_WKUP_IRQHandler               // 46
    .word     USART5_IRQHandler                 // 47
    .word     USART6_IRQHandler                 // 48
    .word     USART7_IRQHandler                 // 49
    .word     USART8_IRQHandler                 // 50 
    .word     TIM5_IRQHandler                   // 51
    .word     TIM9_IRQHandler                   // 52
    .word     TIM10_IRQHandler                  // 53
    .word     TIM26_IRQHandler                  // 54
    .word     SPI7_IRQHandler                   // 55 
    .word     OSPI1_IRQHandler                  // 56
    .word     OSPI2_IRQHandler                  // 57   
    .word     DMA1_CH0_IRQHandler               // 58
    .word     DMA1_CH1_IRQHandler               // 59
    .word     DMA1_CH2_IRQHandler               // 60
    .word     DMA1_CH3_IRQHandler               // 61
    .word     DMA2_CH0_IRQHandler               // 62
    .word     DMA2_CH1_IRQHandler               // 63
    .word     DMA2_CH2_IRQHandler               // 64
    .word     DMA2_CH3_IRQHandler               // 65
    .word     BKPSRAM_SEC_IRQHandler            // 66
    .word     BKPSRAM_DED_IRQHandler            // 67
    .word     COMP1_IRQHandler                  // 68
    .word     FDCAN2_IRQHandler                 // 69
    .word     RSV70_IRQHandler                  // 70
    .word     RSV71_IRQHandler                  // 71
    .word     SDMMC_IRQHandler                  // 72   

/*******************************************************************************
*
* Provide weak aliases for each Exception handler to the Default_Handler.
* As they are weak aliases, any function with the same name will override
* this definition.
*
*******************************************************************************/

    .weak   NMI_Handler                     // -14
    .weak   HardFault_Handler               // -13
    .weak   MemManage_Handler               // -12
    .weak   BusFault_Handler                // -11
    .weak   UsageFault_Handler              // -10
    .weak   SVC_Handler                     // -5
    .weak   DebugMon_Handler                // -4
    .weak   PendSV_Handler                  // -2
    .weak   SysTick_Handler                 // -1
    
    .weak     WDT_IRQHandler				    // 0 
    .weak     LVD_IRQHandler				    // 1				
    .weak     RTC_XTLSD_IRQHandler              // 2
    .weak     CLKRDY_IRQHandler                 // 3
    .weak     EXTI0_IRQHandler		            // 4
    .weak     EXTI1_IRQHandler		            // 5
    .weak     EXTI2_IRQHandler		            // 6
    .weak     EXTI3_IRQHandler		            // 7
    .weak     EXTI4_IRQHandler		            // 8
    .weak     DMA1_IRQHandler		            // 9                   
    .weak     DMA2_IRQHandler		            // 10
    .weak     ADC_IRQHandler		            // 11
    .weak     DAC_IRQHandler		            // 12
    .weak     FSUSB_IRQHandler	                // 13                      
    .weak     FDCAN1_IRQHandler                 // 14		
    .weak     EXTI9_5_IRQHandler                // 15
    .weak     TIM1_BRK_UP_TRG_COM_IRQHandler    // 16
    .weak     TIM1_CC_IRQHandler                // 17
    .weak     TIM2_IRQHandler	                // 18
    .weak     TIM3_IRQHandler                   // 19
    .weak     TIM6_IRQHandler	                // 20
    .weak     TIM7_IRQHandler		            // 21
    .weak     TIM8_BRK_UP_TRG_COM_IRQHandler    // 22	
    .weak     TIM8_CC_IRQHandler                // 23	
    .weak     I2C1_IRQHandler                   // 24
    .weak     I2C2_IRQHandler                   // 25                   
    .weak     SPI1_IRQHandler                   // 26                     
    .weak     SPI2_IRQHandler	                // 27               
    .weak     SPI3_IRQHandler                   // 28
    .weak     SPI4_IRQHandler                   // 29
    .weak     I2S1_IRQHandler	                // 30
    .weak     I2S2_IRQHandler	                // 31
    .weak     USART1_IRQHandler                 // 32                    
    .weak     USART2_IRQHandler		    	    // 33
    .weak     USART3_IRQHandler		  	        // 34                    
    .weak     USART4_IRQHandler		    	    // 35
    .weak     EXTI15_10_IRQHandler	    	    // 36  
    .weak     USB_WKUP_IRQHandler               // 37
    .weak     LPUART1_IRQHandler                // 38 
    .weak     LPUART2_IRQHandler                // 39                
    .weak     LPTIM1_IRQHandler                 // 40
    .weak     FPU_IRQHandler                    // 41
    .weak     TIM4_IRQHandler                   // 42
    .weak     IWDT_WKUP_IRQHandler              // 43
    .weak     AES_SPI1_IRQHandler               // 44
    .weak     ETH_IRQHandler                    // 45
    .weak     ETH_WKUP_IRQHandler               // 46
    .weak     USART5_IRQHandler                 // 47
    .weak     USART6_IRQHandler                 // 48
    .weak     USART7_IRQHandler                 // 49
    .weak     USART8_IRQHandler                 // 50 
    .weak     TIM5_IRQHandler                   // 51
    .weak     TIM9_IRQHandler                   // 52
    .weak     TIM10_IRQHandler                  // 53
    .weak     TIM26_IRQHandler                  // 54
    .weak     SPI7_IRQHandler                   // 55 
    .weak     OSPI1_IRQHandler                  // 56
    .weak     OSPI2_IRQHandler                  // 57   
    .weak     DMA1_CH0_IRQHandler               // 58
    .weak     DMA1_CH1_IRQHandler               // 59
    .weak     DMA1_CH2_IRQHandler               // 60
    .weak     DMA1_CH3_IRQHandler               // 61
    .weak     DMA2_CH0_IRQHandler               // 62
    .weak     DMA2_CH1_IRQHandler               // 63
    .weak     DMA2_CH2_IRQHandler               // 64
    .weak     DMA2_CH3_IRQHandler               // 65
    .weak     BKPSRAM_SEC_IRQHandler            // 66
    .weak     BKPSRAM_DED_IRQHandler            // 67
    .weak     COMP1_IRQHandler                  // 68
    .weak     FDCAN2_IRQHandler                 // 69
    .weak     RSV70_IRQHandler                  // 70
    .weak     RSV71_IRQHandler                  // 71
    .weak     SDMMC_IRQHandler                  // 72    

    .thumb_set   NMI_Handler           ,Default_Handler             // -14
    .thumb_set   HardFault_Handler     ,Default_Handler             // -13
    .thumb_set   MemManage_Handler     ,Default_Handler             // -12
    .thumb_set   BusFault_Handler      ,Default_Handler             // -11
    .thumb_set   UsageFault_Handler    ,Default_Handler             // -10
    .thumb_set   SVC_Handler           ,Default_Handler             // -5
    .thumb_set   DebugMon_Handler      ,Default_Handler             // -4
    .thumb_set   PendSV_Handler        ,Default_Handler             // -2
    .thumb_set   SysTick_Handler       ,Default_Handler             // -1
    
    .thumb_set     WDT_IRQHandler				    ,Default_Handler  // 0 
    .thumb_set     LVD_IRQHandler				    ,Default_Handler  // 1				
    .thumb_set     RTC_XTLSD_IRQHandler             ,Default_Handler  // 2
    .thumb_set     CLKRDY_IRQHandler                ,Default_Handler  // 3
    .thumb_set     EXTI0_IRQHandler		            ,Default_Handler  // 4
    .thumb_set     EXTI1_IRQHandler		            ,Default_Handler  // 5
    .thumb_set     EXTI2_IRQHandler		            ,Default_Handler  // 6
    .thumb_set     EXTI3_IRQHandler		            ,Default_Handler  // 7
    .thumb_set     EXTI4_IRQHandler		            ,Default_Handler  // 8
    .thumb_set     DMA1_IRQHandler		            ,Default_Handler  // 9                   
    .thumb_set     DMA2_IRQHandler		            ,Default_Handler  // 10
    .thumb_set     ADC_IRQHandler		            ,Default_Handler  // 11
    .thumb_set     DAC_IRQHandler		            ,Default_Handler  // 12
    .thumb_set     FSUSB_IRQHandler	                ,Default_Handler  // 13                      
    .thumb_set     FDCAN1_IRQHandler                ,Default_Handler  // 14		
    .thumb_set     EXTI9_5_IRQHandler               ,Default_Handler  // 15
    .thumb_set     TIM1_BRK_UP_TRG_COM_IRQHandler   ,Default_Handler  // 16
    .thumb_set     TIM1_CC_IRQHandler               ,Default_Handler  // 17
    .thumb_set     TIM2_IRQHandler	                ,Default_Handler  // 18
    .thumb_set     TIM3_IRQHandler                  ,Default_Handler  // 19
    .thumb_set     TIM6_IRQHandler	                ,Default_Handler  // 20
    .thumb_set     TIM7_IRQHandler		            ,Default_Handler  // 21
    .thumb_set     TIM8_BRK_UP_TRG_COM_IRQHandler   ,Default_Handler  // 22	
    .thumb_set     TIM8_CC_IRQHandler               ,Default_Handler  // 23	
    .thumb_set     I2C1_IRQHandler                  ,Default_Handler  // 24
    .thumb_set     I2C2_IRQHandler                  ,Default_Handler  // 25                   
    .thumb_set     SPI1_IRQHandler                  ,Default_Handler  // 26                     
    .thumb_set     SPI2_IRQHandler	                ,Default_Handler  // 27               
    .thumb_set     SPI3_IRQHandler                  ,Default_Handler  // 28
    .thumb_set     SPI4_IRQHandler                  ,Default_Handler  // 29
    .thumb_set     I2S1_IRQHandler	                ,Default_Handler  // 30
    .thumb_set     I2S2_IRQHandler	                ,Default_Handler  // 31
    .thumb_set     USART1_IRQHandler                ,Default_Handler  // 32                    
    .thumb_set     USART2_IRQHandler		    	,Default_Handler  // 33
    .thumb_set     USART3_IRQHandler		  	    ,Default_Handler  // 34                    
    .thumb_set     USART4_IRQHandler		    	,Default_Handler  // 35
    .thumb_set     EXTI15_10_IRQHandler	    	    ,Default_Handler  // 36  
    .thumb_set     USB_WKUP_IRQHandler              ,Default_Handler  // 37
    .thumb_set     LPUART1_IRQHandler               ,Default_Handler  // 38 
    .thumb_set     LPUART2_IRQHandler               ,Default_Handler  // 39                
    .thumb_set     LPTIM1_IRQHandler                ,Default_Handler  // 40
    .thumb_set     FPU_IRQHandler                   ,Default_Handler  // 41
    .thumb_set     TIM4_IRQHandler                  ,Default_Handler  // 42
    .thumb_set     IWDT_WKUP_IRQHandler             ,Default_Handler  // 43
    .thumb_set     AES_SPI1_IRQHandler              ,Default_Handler  // 44
    .thumb_set     ETH_IRQHandler                   ,Default_Handler  // 45
    .thumb_set     ETH_WKUP_IRQHandler              ,Default_Handler  // 46
    .thumb_set     USART5_IRQHandler                ,Default_Handler  // 47
    .thumb_set     USART6_IRQHandler                ,Default_Handler  // 48
    .thumb_set     USART7_IRQHandler                ,Default_Handler  // 49
    .thumb_set     USART8_IRQHandler                ,Default_Handler  // 50 
    .thumb_set     TIM5_IRQHandler                  ,Default_Handler  // 51
    .thumb_set     TIM9_IRQHandler                  ,Default_Handler  // 52
    .thumb_set     TIM10_IRQHandler                 ,Default_Handler  // 53
    .thumb_set     TIM26_IRQHandler                 ,Default_Handler  // 54
    .thumb_set     SPI7_IRQHandler                  ,Default_Handler  // 55 
    .thumb_set     OSPI1_IRQHandler                 ,Default_Handler  // 56
    .thumb_set     OSPI2_IRQHandler                 ,Default_Handler  // 57   
    .thumb_set     DMA1_CH0_IRQHandler              ,Default_Handler  // 58
    .thumb_set     DMA1_CH1_IRQHandler              ,Default_Handler  // 59
    .thumb_set     DMA1_CH2_IRQHandler              ,Default_Handler  // 60
    .thumb_set     DMA1_CH3_IRQHandler              ,Default_Handler  // 61
    .thumb_set     DMA2_CH0_IRQHandler              ,Default_Handler  // 62
    .thumb_set     DMA2_CH1_IRQHandler              ,Default_Handler  // 63
    .thumb_set     DMA2_CH2_IRQHandler              ,Default_Handler  // 64
    .thumb_set     DMA2_CH3_IRQHandler              ,Default_Handler  // 65
    .thumb_set     BKPSRAM_SEC_IRQHandler           ,Default_Handler  // 66
    .thumb_set     BKPSRAM_DED_IRQHandler           ,Default_Handler  // 67
    .thumb_set     COMP1_IRQHandler                 ,Default_Handler  // 68
    .thumb_set     FDCAN2_IRQHandler                ,Default_Handler  // 69
    .thumb_set     RSV70_IRQHandler                 ,Default_Handler  // 70
    .thumb_set     RSV71_IRQHandler                 ,Default_Handler  // 71
    .thumb_set     SDMMC_IRQHandler                 ,Default_Handler  // 72    


/*******************END OF FILE**************/

