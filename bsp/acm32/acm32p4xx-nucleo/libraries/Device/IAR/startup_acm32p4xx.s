;********************************************************************************
;* File Name          : startup_acm32p4xx.s
;* Description        : This module performs:
;*                      - Set the initial SP
;*                      - Set the initial PC == _iar_program_start,
;*                      - Set the vector table entries with the exceptions ISR
;*                        address.
;*                      - Branches to main in the C library (which eventually
;*                        calls main()).
;*                      After Reset the Star-MC1(Cortex-M33) processor is in Thread mode,
;*                      priority is Privileged, and the Stack is set to Main.
;*******************************************************************************


        MODULE  ?cstartup

        ;; Forward declaration of sections.
        SECTION CSTACK:DATA:NOROOT(3)

        SECTION .intvec:CODE:NOROOT(2)

        EXTERN  __iar_program_start
        EXTERN  SystemInit
        PUBLIC  __vector_table
        PUBLIC  __Vectors
        PUBLIC  __Vectors_End
        PUBLIC  __Vectors_Size

        DATA
__vector_table
        DCD     sfe(CSTACK)
        DCD     Reset_Handler              ; Reset Handler
        DCD     NMI_Handler                ; NMI Handler
        DCD     HardFault_Handler          ; Hard Fault Handler
        DCD     MemManage_Handler          ; MPU Fault Handler
        DCD     BusFault_Handler           ; Bus Fault Handler
        DCD     UsageFault_Handler         ; Usage Fault Handler
        DCD     0                          ; Reserved
        DCD     0                          ; Reserved
        DCD     0                          ; Reserved
        DCD     0                          ; Reserved
        DCD     SVC_Handler                ; SVCall Handler
        DCD     DebugMon_Handler           ; Debug Monitor Handler
        DCD     0                          ; Reserved
        DCD     PendSV_Handler             ; PendSV Handler
        DCD     SysTick_Handler            ; SysTick Handler

         ; External Interrupts
        DCD     WDT_IRQHandler			; 0 
        DCD     LVD_IRQHandler			; 1				
        DCD     RTC_XTLSD_IRQHandler            ; 2
        DCD     RSV3_IRQHandler		        ; 3					
        DCD     RSV4_IRQHandler	                ; 4
        DCD     RSV5_IRQHandler                 ; 5
        DCD     CLKRDY_IRQHandler               ; 6
        DCD     EXTI0_IRQHandler		; 7
        DCD     EXTI1_IRQHandler		; 8
        DCD     EXTI2_IRQHandler		; 9
        DCD     EXTI3_IRQHandler		; 10
        DCD     EXTI4_IRQHandler		; 11
        DCD     RSV12_IRQHandler		; 12                   
        DCD     RSV13_IRQHandler		; 13
        DCD     ADC12_IRQHandler		; 14
        DCD     ADC3_IRQHandler		        ; 15
        DCD     DAC1_IRQHandler		        ; 16
        DCD     COMP1_IRQHandler                ; 17
        DCD     USBOTG1_IRQHandler		; 18                      
        DCD     FDCAN1_IRQHandler               ; 19	
        DCD     FDCAN2_IRQHandler               ; 20 	
        DCD     EXTI9_5_IRQHandler              ; 21
        DCD     TIM1_BRK_UP_TRG_COM_IRQHandler  ; 22
        DCD     TIM1_CC_IRQHandler              ; 23
        DCD     TIM2_IRQHandler	                ; 24
        DCD     TIM3_IRQHandler                 ; 25
        DCD     TIM6_IRQHandler	                ; 26
        DCD    	TIM7_IRQHandler		        ; 27
        DCD    	TIM8_BRK_UP_TRG_COM_IRQHandler  ; 28	
        DCD     TIM8_CC_IRQHandler              ; 29	
        DCD     TIM15_IRQHandler                ; 30
        DCD	TIM16_IRQHandler		; 31	
        DCD     TIM17_IRQHandler                ; 32
        DCD     I2C1_IRQHandler                 ; 33
        DCD     I2C2_IRQHandler                 ; 34                   
        DCD     SPI1_IRQHandler                 ; 35                     
        DCD     SPI2_IRQHandler	                ; 36
        DCD     SPI3_IRQHandler                 ; 37
        DCD     I2S1_IRQHandler	                ; 38
        DCD     I2S2_IRQHandler	                ; 39
        DCD     USART1_IRQHandler               ; 40                    
        DCD     USART2_IRQHandler		; 41
        DCD     USART3_IRQHandler		; 42                    
        DCD     USART4_IRQHandler		; 43
        DCD     EXTI15_10_IRQHandler	    	; 44  
        DCD     USBOTG1_WKUP_IRQHandler         ; 45
        DCD     LPUART1_IRQHandler              ; 46                    
        DCD     LPTIM1_IRQHandler               ; 47
        DCD     USBOTG2_WKUP_IRQHandler         ; 48
        DCD     AES_IRQHandler                  ; 49
        DCD     FPU_IRQHandler                  ; 50
        DCD     USBOTG2_IRQHandler              ; 51
        DCD     DCMI_IRQHandler                 ; 52
        DCD     TIM4_IRQHandler                 ; 53
        DCD     RSV54_IRQHandler                ; 54
        DCD     IWDT_WKUP_IRQHandler            ; 55
        DCD     LTDC_IRQHandler                 ; 56
        DCD     LTDC_ERR_IRQHandler             ; 57
        DCD     DMA2D_IRQHandler                ; 58
        DCD     LPTIM2_IRQHandler               ; 59
        DCD     LPTIM3_IRQHandler               ; 60
        DCD     LPTIM4_IRQHandler               ; 61
        DCD     LPTIM5_IRQHandler               ; 62
        DCD     LPTIM6_IRQHandler               ; 63
        DCD     AES_SPI1_IRQHandler             ; 64
        DCD     I2S3_IRQHandler                 ; 65
        DCD     SPI4_IRQHandler                 ; 66
        DCD     SPI5_IRQHandler                 ; 67
        DCD     SPI6_IRQHandler                 ; 68
        DCD     I2C3_IRQHandler                 ; 69
        DCD     I2C4_IRQHandler                 ; 70
        DCD     FDCAN3_IRQHandler               ; 71
        DCD     RSV72_IRQHandler                ; 72
        DCD     ETH_IRQHandler                  ; 73
        DCD     ETH_WKUP_IRQHandler             ; 74
        DCD     SDMMC_IRQHandler                ; 75
        DCD     USART5_IRQHandler               ; 76
        DCD     USART6_IRQHandler               ; 77
        DCD     USART7_IRQHandler               ; 78
        DCD     USART8_IRQHandler               ; 79
        DCD     USART9_IRQHandler               ; 80
        DCD     USART10_IRQHandler              ; 81
        DCD     DAC2_IRQHandler                 ; 82
        DCD     TIM5_IRQHandler                 ; 83
        DCD     TIM9_IRQHandler                 ; 84
        DCD     TIM10_IRQHandler                ; 85
        DCD     TIM11_IRQHandler                ; 86
        DCD     TIM12_IRQHandler                ; 87
        DCD     TIM13_IRQHandler                ; 88
        DCD     TIM14_IRQHandler                ; 89
        DCD     TIM18_IRQHandler                ; 90
        DCD     TIM19_IRQHandler                ; 91
        DCD     TIM20_BRK_UP_TRG_COM_IRQHandler ; 92
        DCD     TIM20_CC_IRQHandler             ; 93
        DCD     TIM21_IRQHandler                ; 94
        DCD     TIM22_IRQHandler                ; 95
        DCD     TIM23_IRQHandler                ; 96
        DCD     TIM24_IRQHandler                ; 97
        DCD     TIM25_IRQHandler                ; 98
        DCD     TIM26_IRQHandler                ; 99
        DCD     SPI7_IRQHandler                 ; 100
        DCD     SPI8_IRQHandler                 ; 101
        DCD     OSPI1_IRQHandler                ; 102
        DCD     OSPI2_IRQHandler                ; 103
        DCD     RSV104_IRQHandler               ; 104
        DCD     TKEY_IRQHandler                 ; 105
        DCD     RSV106_IRQHandler               ; 106
        DCD     RSV107_IRQHandler               ; 107
        DCD     OTG1_HS_EP_OUT_IRQHandler       ; 108
        DCD     OTG1_HS_EP_IN_IRQHandler        ; 109
        DCD     OTG2_HS_EP_OUT_IRQHandler       ; 110
        DCD     OTG2_HS_EP_IN_IRQHandler        ; 111
        DCD     RSV112_IRQHandler                  ; 112
        DCD     RSV113_IRQHandler                  ; 113
        DCD     RSV114_IRQHandler             ; 114
        DCD     RSV115_IRQHandler             ; 115
        DCD     RSV116_IRQHandler             ; 116
        DCD     RSV117_IRQHandler             ; 117
        DCD     RSV118_IRQHandler             ; 118
        DCD     RSV119_IRQHandler             ; 119
        DCD     RSV120_IRQHandler             ; 120
        DCD     RSV121_IRQHandler             ; 121
        DCD     RSV122_IRQHandler             ; 122
        DCD     RSV123_IRQHandler             ; 123
        DCD     RSV124_IRQHandler             ; 124
        DCD     RSV125_IRQHandler             ; 125
        DCD     NAND_IRQHandler                 ; 126
        DCD     BCH_IRQHandler                  ; 127
        DCD     SDRAM_IRQHandler                ; 128
        DCD     DMA1_CH0_IRQHandler             ; 129
        DCD     DMA1_CH1_IRQHandler             ; 130
        DCD     DMA1_CH2_IRQHandler             ; 131
        DCD     DMA1_CH3_IRQHandler             ; 132
        DCD     DMA1_CH4_IRQHandler             ; 133
        DCD     DMA1_CH5_IRQHandler             ; 134
        DCD     DMA1_CH6_IRQHandler             ; 135
        DCD     DMA1_CH7_IRQHandler             ; 136
        DCD     DMA2_CH0_IRQHandler             ; 137
        DCD     DMA2_CH1_IRQHandler             ; 138
        DCD     DMA2_CH2_IRQHandler             ; 139
        DCD     DMA2_CH3_IRQHandler             ; 140
        DCD     DMA2_CH4_IRQHandler             ; 141
        DCD     DMA2_CH5_IRQHandler             ; 142
        DCD     DMA2_CH6_IRQHandler             ; 143
        DCD     DMA2_CH7_IRQHandler             ; 144
        DCD     SRAM1_SEC_IRQHandler            ; 145
        DCD     SRAM1_DED_IRQHandler            ; 146
        DCD     SRAM3_SEC_IRQHandler            ; 147
        DCD     SRAM3_DED_IRQHandler            ; 148
        DCD     BKPSRAM_SEC_IRQHandler          ; 149
        DCD     BKPSRAM_DED_IRQHandler          ; 150
__Vectors_End

__Vectors       EQU   __vector_table
__Vectors_Size  EQU   __Vectors_End - __Vectors

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Default interrupt handlers.
;;
        THUMB
        PUBWEAK Reset_Handler
        PUBWEAK WDT_IRQHandler
        PUBWEAK LVD_IRQHandler
        PUBWEAK RTC_XTLSD_IRQHandler
        PUBWEAK EFC_IRQHandler
        PUBWEAK RSV3_IRQHandler
        PUBWEAK RSV4_IRQHandler
        PUBWEAK RSV5_IRQHandler
        PUBWEAK CLKRDY_IRQHandler
        PUBWEAK EXTI0_IRQHandler
        PUBWEAK EXTI1_IRQHandler
        PUBWEAK EXTI2_IRQHandler
        PUBWEAK EXTI3_IRQHandler
        PUBWEAK EXTI4_IRQHandler
        PUBWEAK RSV12_IRQHandler
        PUBWEAK RSV13_IRQHandler        
        PUBWEAK ADC12_IRQHandler
        PUBWEAK ADC3_IRQHandler
        PUBWEAK DAC1_IRQHandler
        PUBWEAK COMP1_IRQHandler
        PUBWEAK USBOTG1_IRQHandler
        PUBWEAK FDCAN1_IRQHandler
        PUBWEAK FDCAN2_IRQHandler
        PUBWEAK EXTI9_5_IRQHandler
        PUBWEAK TIM1_BRK_UP_TRG_COM_IRQHandler
        PUBWEAK TIM1_CC_IRQHandler               
        PUBWEAK TIM2_IRQHandler                  
        PUBWEAK TIM3_IRQHandler                  
        PUBWEAK TIM6_IRQHandler                  
        PUBWEAK TIM7_IRQHandler                  
        PUBWEAK TIM8_BRK_UP_TRG_COM_IRQHandler   
        PUBWEAK TIM8_CC_IRQHandler               
        PUBWEAK TIM15_IRQHandler                 
        PUBWEAK TIM16_IRQHandler                 
        PUBWEAK TIM17_IRQHandler                 
        PUBWEAK I2C1_IRQHandler                  
        PUBWEAK I2C2_IRQHandler                  
        PUBWEAK SPI1_IRQHandler                  
        PUBWEAK SPI2_IRQHandler                  
        PUBWEAK SPI3_IRQHandler                  
        PUBWEAK I2S1_IRQHandler                  
        PUBWEAK I2S2_IRQHandler                  
        PUBWEAK USART1_IRQHandler                
        PUBWEAK USART2_IRQHandler                
        PUBWEAK USART3_IRQHandler                
        PUBWEAK USART4_IRQHandler                
        PUBWEAK EXTI15_10_IRQHandler             
        PUBWEAK USBOTG1_WKUP_IRQHandler          
        PUBWEAK LPUART1_IRQHandler               
        PUBWEAK LPTIM1_IRQHandler                
        PUBWEAK USBOTG2_WKUP_IRQHandler          
        PUBWEAK AES_IRQHandler                   
        PUBWEAK FPU_IRQHandler                   
        PUBWEAK USBOTG2_IRQHandler               
        PUBWEAK DCMI_IRQHandler                  
        PUBWEAK TIM4_IRQHandler                  
        PUBWEAK RSV54_IRQHandler                 
        PUBWEAK IWDT_WKUP_IRQHandler             
        PUBWEAK LTDC_IRQHandler                  
        PUBWEAK LTDC_ERR_IRQHandler              
        PUBWEAK DMA2D_IRQHandler                 
        PUBWEAK LPTIM2_IRQHandler                
        PUBWEAK LPTIM3_IRQHandler                
        PUBWEAK LPTIM4_IRQHandler                
        PUBWEAK LPTIM5_IRQHandler                
        PUBWEAK LPTIM6_IRQHandler                
        PUBWEAK AES_SPI1_IRQHandler              
        PUBWEAK I2S3_IRQHandler                  
        PUBWEAK SPI4_IRQHandler                  
        PUBWEAK SPI5_IRQHandler                  
        PUBWEAK SPI6_IRQHandler                  
        PUBWEAK I2C3_IRQHandler                  
        PUBWEAK I2C4_IRQHandler                  
        PUBWEAK FDCAN3_IRQHandler                
        PUBWEAK RSV72_IRQHandler                 
        PUBWEAK ETH_IRQHandler                   
        PUBWEAK ETH_WKUP_IRQHandler              
        PUBWEAK SDMMC_IRQHandler                 
        PUBWEAK USART5_IRQHandler                
        PUBWEAK USART6_IRQHandler                
        PUBWEAK USART7_IRQHandler                
        PUBWEAK USART8_IRQHandler                
        PUBWEAK USART9_IRQHandler                
        PUBWEAK USART10_IRQHandler               
        PUBWEAK DAC2_IRQHandler                  
        PUBWEAK TIM5_IRQHandler                  
        PUBWEAK TIM9_IRQHandler                  
        PUBWEAK TIM10_IRQHandler                 
        PUBWEAK TIM11_IRQHandler                 
        PUBWEAK TIM12_IRQHandler                 
        PUBWEAK TIM13_IRQHandler                 
        PUBWEAK TIM14_IRQHandler                 
        PUBWEAK TIM18_IRQHandler                 
        PUBWEAK TIM19_IRQHandler                 
        PUBWEAK TIM20_BRK_UP_TRG_COM_IRQHandler  
        PUBWEAK TIM20_CC_IRQHandler              
        PUBWEAK TIM21_IRQHandler                 
        PUBWEAK TIM22_IRQHandler                 
        PUBWEAK TIM23_IRQHandler                 
        PUBWEAK TIM24_IRQHandler                 
        PUBWEAK TIM25_IRQHandler                 
        PUBWEAK TIM26_IRQHandler                 
        PUBWEAK SPI7_IRQHandler                  
        PUBWEAK SPI8_IRQHandler                  
        PUBWEAK OSPI1_IRQHandler                 
        PUBWEAK OSPI2_IRQHandler                 
        PUBWEAK RSV104_IRQHandler                
        PUBWEAK TKEY_IRQHandler                  
        PUBWEAK RSV106_IRQHandler                   
        PUBWEAK RSV107_IRQHandler                
        PUBWEAK OTG1_HS_EP_OUT_IRQHandler        
        PUBWEAK OTG1_HS_EP_IN_IRQHandler         
        PUBWEAK OTG2_HS_EP_OUT_IRQHandler        
        PUBWEAK OTG2_HS_EP_IN_IRQHandler         
        PUBWEAK RSV112_IRQHandler                   
        PUBWEAK RSV113_IRQHandler                   
        PUBWEAK RSV114_IRQHandler              
        PUBWEAK RSV115_IRQHandler              
        PUBWEAK RSV116_IRQHandler              
        PUBWEAK RSV117_IRQHandler              
        PUBWEAK RSV118_IRQHandler              
        PUBWEAK RSV119_IRQHandler              
        PUBWEAK RSV120_IRQHandler              
        PUBWEAK RSV121_IRQHandler              
        PUBWEAK RSV122_IRQHandler              
        PUBWEAK RSV123_IRQHandler              
        PUBWEAK RSV124_IRQHandler              
        PUBWEAK RSV125_IRQHandler              
        PUBWEAK NAND_IRQHandler                  
        PUBWEAK BCH_IRQHandler                   
        PUBWEAK SDRAM_IRQHandler                 
        PUBWEAK DMA1_CH0_IRQHandler              
        PUBWEAK DMA1_CH1_IRQHandler              
        PUBWEAK DMA1_CH2_IRQHandler              
        PUBWEAK DMA1_CH3_IRQHandler              
        PUBWEAK DMA1_CH4_IRQHandler              
        PUBWEAK DMA1_CH5_IRQHandler              
        PUBWEAK DMA1_CH6_IRQHandler              
        PUBWEAK DMA1_CH7_IRQHandler              
        PUBWEAK DMA2_CH0_IRQHandler              
        PUBWEAK DMA2_CH1_IRQHandler              
        PUBWEAK DMA2_CH2_IRQHandler              
        PUBWEAK DMA2_CH3_IRQHandler              
        PUBWEAK DMA2_CH4_IRQHandler              
        PUBWEAK DMA2_CH5_IRQHandler              
        PUBWEAK DMA2_CH6_IRQHandler              
        PUBWEAK DMA2_CH7_IRQHandler              
        PUBWEAK SRAM1_SEC_IRQHandler             
        PUBWEAK SRAM1_DED_IRQHandler             
        PUBWEAK SRAM3_SEC_IRQHandler             
        PUBWEAK SRAM3_DED_IRQHandler             
        PUBWEAK BKPSRAM_SEC_IRQHandler           
        PUBWEAK BKPSRAM_DED_IRQHandler           
        
        SECTION .text:CODE:NOROOT:REORDER(2)
Reset_Handler
        MOVW     r0,#0xe014
        MOVT     r0,#0xe001
        LDR      r1,[r0,#0]
        ORR      r1,r1,#1
        STR      r1,[r0,#0] 
        
        LDR     R0, =SystemInit
        BLX     R0
        LDR     R0, =__iar_program_start
        BX      R0

        PUBWEAK NMI_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
NMI_Handler
        B .

        PUBWEAK HardFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
HardFault_Handler
        B .

        PUBWEAK MemManage_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
MemManage_Handler
        B .

        PUBWEAK BusFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
BusFault_Handler
        B .

        PUBWEAK UsageFault_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
UsageFault_Handler
        B UsageFault_Handler

        PUBWEAK SVC_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SVC_Handler
        B .

        PUBWEAK DebugMon_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
DebugMon_Handler
        B .

        PUBWEAK PendSV_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
PendSV_Handler
        B .

        PUBWEAK SysTick_Handler
        SECTION .text:CODE:NOROOT:REORDER(1)
SysTick_Handler
        B .

        SECTION .text:CODE:NOROOT:REORDER(1)
WDT_IRQHandler
LVD_IRQHandler
RTC_XTLSD_IRQHandler
EFC_IRQHandler
RSV3_IRQHandler
RSV4_IRQHandler
RSV5_IRQHandler
CLKRDY_IRQHandler
EXTI0_IRQHandler
EXTI1_IRQHandler
EXTI2_IRQHandler
EXTI3_IRQHandler
EXTI4_IRQHandler
RSV12_IRQHandler
RSV13_IRQHandler
ADC12_IRQHandler
ADC3_IRQHandler
DAC1_IRQHandler
COMP1_IRQHandler
USBOTG1_IRQHandler
FDCAN1_IRQHandler
FDCAN2_IRQHandler
EXTI9_5_IRQHandler
TIM1_BRK_UP_TRG_COM_IRQHandler
TIM1_CC_IRQHandler      
TIM2_IRQHandler	        
TIM3_IRQHandler         
TIM6_IRQHandler	        
TIM7_IRQHandler
TIM8_BRK_UP_TRG_COM_IRQHandler
TIM8_CC_IRQHandler
TIM15_IRQHandler
TIM16_IRQHandler
TIM17_IRQHandler
I2C1_IRQHandler
I2C2_IRQHandler
SPI1_IRQHandler
SPI2_IRQHandler
SPI3_IRQHandler
I2S1_IRQHandler
I2S2_IRQHandler
USART1_IRQHandler
USART2_IRQHandler
USART3_IRQHandler
USART4_IRQHandler
EXTI15_10_IRQHandler
USBOTG1_WKUP_IRQHandler
LPUART1_IRQHandler
LPTIM1_IRQHandler
USBOTG2_WKUP_IRQHandler
AES_IRQHandler
FPU_IRQHandler
USBOTG2_IRQHandler
DCMI_IRQHandler
TIM4_IRQHandler
RSV54_IRQHandler
IWDT_WKUP_IRQHandler
LTDC_IRQHandler
LTDC_ERR_IRQHandler
DMA2D_IRQHandler
LPTIM2_IRQHandler
LPTIM3_IRQHandler
LPTIM4_IRQHandler
LPTIM5_IRQHandler
LPTIM6_IRQHandler
AES_SPI1_IRQHandler
I2S3_IRQHandler
SPI4_IRQHandler
SPI5_IRQHandler
SPI6_IRQHandler
I2C3_IRQHandler
I2C4_IRQHandler
FDCAN3_IRQHandler
RSV72_IRQHandler
ETH_IRQHandler
ETH_WKUP_IRQHandler
SDMMC_IRQHandler
USART5_IRQHandler
USART6_IRQHandler
USART7_IRQHandler
USART8_IRQHandler
USART9_IRQHandler
USART10_IRQHandler
DAC2_IRQHandler
TIM5_IRQHandler
TIM9_IRQHandler
TIM10_IRQHandler
TIM11_IRQHandler
TIM12_IRQHandler
TIM13_IRQHandler
TIM14_IRQHandler
TIM18_IRQHandler
TIM19_IRQHandler
TIM20_BRK_UP_TRG_COM_IRQHandler
TIM20_CC_IRQHandler
TIM21_IRQHandler
TIM22_IRQHandler
TIM23_IRQHandler
TIM24_IRQHandler
TIM25_IRQHandler
TIM26_IRQHandler
SPI7_IRQHandler
SPI8_IRQHandler
OSPI1_IRQHandler
OSPI2_IRQHandler
RSV104_IRQHandler
TKEY_IRQHandler
RSV106_IRQHandler
RSV107_IRQHandler
OTG1_HS_EP_OUT_IRQHandler
OTG1_HS_EP_IN_IRQHandler
OTG2_HS_EP_OUT_IRQHandler
OTG2_HS_EP_IN_IRQHandler
RSV112_IRQHandler
RSV113_IRQHandler
RSV114_IRQHandler
RSV115_IRQHandler
RSV116_IRQHandler
RSV117_IRQHandler
RSV118_IRQHandler
RSV119_IRQHandler
RSV120_IRQHandler
RSV121_IRQHandler
RSV122_IRQHandler
RSV123_IRQHandler
RSV124_IRQHandler
RSV125_IRQHandler
NAND_IRQHandler
BCH_IRQHandler
SDRAM_IRQHandler
DMA1_CH0_IRQHandler
DMA1_CH1_IRQHandler
DMA1_CH2_IRQHandler
DMA1_CH3_IRQHandler
DMA1_CH4_IRQHandler
DMA1_CH5_IRQHandler
DMA1_CH6_IRQHandler
DMA1_CH7_IRQHandler
DMA2_CH0_IRQHandler
DMA2_CH1_IRQHandler
DMA2_CH2_IRQHandler
DMA2_CH3_IRQHandler
DMA2_CH4_IRQHandler
DMA2_CH5_IRQHandler
DMA2_CH6_IRQHandler
DMA2_CH7_IRQHandler
SRAM1_SEC_IRQHandler
SRAM1_DED_IRQHandler      
SRAM3_SEC_IRQHandler
SRAM3_DED_IRQHandler
BKPSRAM_SEC_IRQHandler
BKPSRAM_DED_IRQHandler
      B .

       
        END
/************************ (C) COPYRIGHT Aisinochip *****END OF FILE****/
