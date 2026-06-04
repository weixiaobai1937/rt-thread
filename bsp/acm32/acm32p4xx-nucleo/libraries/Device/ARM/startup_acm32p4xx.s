;/*****************************************************************************
; * @file:    startup_acm32p4xx.s
; * @purpose: CMSIS Star-MC1 Core Device Startup File 
; * @version: V1.0
; * @date:    
; *
; *****************************************************************************/
Stack_Size      EQU     0x00002000
Heap_Size       EQU     0x00000000
;__initial_sp    EQU    0x20003000       ;Define SP size

                AREA    STACK, NOINIT, READWRITE, ALIGN=3
Stack_Mem       SPACE   Stack_Size
__initial_sp

                AREA    HEAP, NOINIT, READWRITE, ALIGN=3
__heap_base
Heap_Mem        SPACE   Heap_Size
__heap_limit

                PRESERVE8
                THUMB

; Vector Table Mapped to Address 0 at Reset

                AREA    RESET, DATA, READONLY
                EXPORT __Vectors

__Vectors       
				DCD     __initial_sp                    ; Top of Stack
                DCD     Reset_Handler                   ; Reset Handler
                DCD     NMI_Handler                     ; NMI Handler
                DCD     HardFault_Handler               ; Hard Fault Handler
                DCD     MemManage_Handler               ; MPU Fault Handler
                DCD     BusFault_Handler                ; Bus Fault Handler
                DCD     UsageFault_Handler              ; Usage Fault Handler
                DCD     0                               ; Reserved
                DCD     0                               ; Reserved
                DCD     0                               ; Reserved
                DCD     0                               ; Reserved
                DCD     SVC_Handler                     ; SVCall Handler,SWI
                DCD     DebugMon_Handler                ; Debug Monitor Handler
                DCD     0                               ; Reserved
                DCD     PendSV_Handler                  ; PendSV Handler
                DCD     SysTick_Handler                 ; SysTick Handler

                ; External Interrupts
                DCD     WDT_IRQHandler				    ; 0 
				DCD     LVD_IRQHandler				    ; 1				
                DCD     RTC_XTLSD_IRQHandler            ; 2
				DCD     CLKRDY_IRQHandler               ; 3
				DCD     EXTI0_IRQHandler		        ; 4
				DCD     EXTI1_IRQHandler		        ; 5
				DCD     EXTI2_IRQHandler		        ; 6
				DCD     EXTI3_IRQHandler		        ; 7
				DCD     EXTI4_IRQHandler		        ; 8
				DCD     DMA1_IRQHandler		            ; 9                   
				DCD     DMA2_IRQHandler		            ; 10
				DCD     ADC_IRQHandler		            ; 11
				DCD     DAC_IRQHandler		            ; 12
				DCD     FSUSB_IRQHandler	            ; 13                      
				DCD     FDCAN1_IRQHandler                ; 14		
				DCD     EXTI9_5_IRQHandler              ; 15
				DCD     TIM1_BRK_UP_TRG_COM_IRQHandler  ; 16
				DCD     TIM1_CC_IRQHandler              ; 17
				DCD     TIM2_IRQHandler	                ; 18
				DCD     TIM3_IRQHandler                 ; 19
				DCD     TIM6_IRQHandler	                ; 20
				DCD    	TIM7_IRQHandler		            ; 21
				DCD    	TIM8_BRK_UP_TRG_COM_IRQHandler  ; 22	
				DCD     TIM8_CC_IRQHandler              ; 23	
				DCD     I2C1_IRQHandler                 ; 24
                DCD     I2C2_IRQHandler                 ; 25                   
                DCD     SPI1_IRQHandler                 ; 26                     
                DCD     SPI2_IRQHandler	                ; 27               
				DCD     SPI3_IRQHandler                 ; 28
                DCD     SPI4_IRQHandler                 ; 29
				DCD     I2S1_IRQHandler	                ; 30
                DCD     I2S2_IRQHandler	                ; 31
                DCD     USART1_IRQHandler               ; 32                    
				DCD     USART2_IRQHandler		    	; 33
				DCD     USART3_IRQHandler		  	    ; 34                    
				DCD     USART4_IRQHandler		    	; 35
				DCD     EXTI15_10_IRQHandler	    	; 36  
				DCD     USB_WKUP_IRQHandler             ; 37
                DCD     LPUART1_IRQHandler              ; 38 
                DCD     LPUART2_IRQHandler              ; 39                
                DCD     LPTIM1_IRQHandler               ; 40
				DCD     FPU_IRQHandler                  ; 41
				DCD     TIM4_IRQHandler                 ; 42
				DCD     IWDT_WKUP_IRQHandler            ; 43
				DCD     AES_SPI1_IRQHandler             ; 44
				DCD     ETH_IRQHandler                  ; 45
				DCD     ETH_WKUP_IRQHandler             ; 46
				DCD     USART5_IRQHandler               ; 47
				DCD     USART6_IRQHandler               ; 48
				DCD     USART7_IRQHandler               ; 49
				DCD     USART8_IRQHandler               ; 50 
				DCD     TIM5_IRQHandler                 ; 51
				DCD     TIM9_IRQHandler                 ; 52
				DCD     TIM10_IRQHandler                ; 53
				DCD     TIM26_IRQHandler                ; 54
				DCD     SPI7_IRQHandler                 ; 55 
				DCD     OSPI1_IRQHandler                ; 56
				DCD     OSPI2_IRQHandler                ; 57   
				DCD     DMA1_CH0_IRQHandler             ; 58
				DCD     DMA1_CH1_IRQHandler             ; 59
				DCD     DMA1_CH2_IRQHandler             ; 60
				DCD     DMA1_CH3_IRQHandler             ; 61
				DCD     DMA2_CH0_IRQHandler             ; 62
				DCD     DMA2_CH1_IRQHandler             ; 63
				DCD     DMA2_CH2_IRQHandler             ; 64
				DCD     DMA2_CH3_IRQHandler             ; 65
				DCD     BKPSRAM_SEC_IRQHandler          ; 66
				DCD     BKPSRAM_DED_IRQHandler          ; 67
				DCD     COMP1_IRQHandler                ; 68
				DCD     FDCAN2_IRQHandler                ; 69
				DCD     RSV70_IRQHandler                ; 70
				DCD     RSV71_IRQHandler                ; 71
				DCD     SDMMC_IRQHandler                ; 72    

                AREA    |.text|, CODE, READONLY

Reset_Handler   PROC
                EXPORT  Reset_Handler                       [WEAK]
                IMPORT  __main
                IMPORT  main
                IMPORT  SystemInit

                ;Enable DTCM     
                MOVW     r0,#0xe014
                MOVT     r0,#0xe001
                LDR      r1,[r0,#0]
                ORR      r1,r1,#1
                STR      r1,[r0,#0] 
                
                LDR     R0, =SystemInit
                BLX     R0
 
                
                LDR     R0, =__main
                BX      R0                  ;
                ENDP


NMI_Handler     PROC
                EXPORT  NMI_Handler                         [WEAK]
                B       .
                ENDP
HardFault_Handler\
                PROC
                EXPORT  HardFault_Handler                   [WEAK]
                B       .
                ENDP
MemManage_Handler\
                PROC
                EXPORT  MemManage_Handler                   [WEAK]
                B       .
                ENDP
BusFault_Handler\
                PROC
                EXPORT  BusFault_Handler                    [WEAK]
                B       .
                ENDP
UsageFault_Handler\
                PROC
                EXPORT  UsageFault_Handler                  [WEAK]
                B       .
                ENDP
SVC_Handler     PROC
                EXPORT  SVC_Handler                         [WEAK]
                B       .
                ENDP
DebugMon_Handler\
                PROC
                EXPORT  DebugMon_Handler                    [WEAK]
                B       .
                ENDP
PendSV_Handler  PROC
                EXPORT  PendSV_Handler                      [WEAK]
                B       .
                ENDP
SysTick_Handler PROC
                EXPORT  SysTick_Handler                     [WEAK]
                B       .
                ENDP

Default_Handler PROC
                EXPORT     WDT_IRQHandler                   [WEAK]
                EXPORT     LVD_IRQHandler                   [WEAK]                
                EXPORT     RTC_XTLSD_IRQHandler             [WEAK]
                EXPORT     CLKRDY_IRQHandler                [WEAK]
                EXPORT     EXTI0_IRQHandler                 [WEAK]
                EXPORT     EXTI1_IRQHandler                 [WEAK]
                EXPORT     EXTI2_IRQHandler                 [WEAK]
                EXPORT     EXTI3_IRQHandler                 [WEAK]
                EXPORT     EXTI4_IRQHandler                 [WEAK]
                EXPORT     DMA1_IRQHandler                  [WEAK]           
                EXPORT     DMA2_IRQHandler                  [WEAK]
                EXPORT     ADC_IRQHandler                   [WEAK]
                EXPORT     DAC_IRQHandler                   [WEAK]
                EXPORT     FSUSB_IRQHandler                 [WEAK]
                EXPORT     FDCAN1_IRQHandler                 [WEAK]
                EXPORT     EXTI9_5_IRQHandler               [WEAK]
                EXPORT     TIM1_BRK_UP_TRG_COM_IRQHandler   [WEAK]
                EXPORT     TIM1_CC_IRQHandler               [WEAK]
                EXPORT     TIM2_IRQHandler                  [WEAK]
                EXPORT     TIM3_IRQHandler                  [WEAK] 
                EXPORT     TIM6_IRQHandler                  [WEAK]
                EXPORT     TIM7_IRQHandler                  [WEAK]
                EXPORT     TIM8_BRK_UP_TRG_COM_IRQHandler   [WEAK]
                EXPORT     TIM8_CC_IRQHandler               [WEAK]
                EXPORT     I2C1_IRQHandler                  [WEAK]
                EXPORT     I2C2_IRQHandler                  [WEAK]
                EXPORT     SPI1_IRQHandler                  [WEAK]
                EXPORT     SPI2_IRQHandler                  [WEAK]
                EXPORT     SPI3_IRQHandler                  [WEAK]   
                EXPORT     SPI4_IRQHandler                  [WEAK]                 
                EXPORT     I2S1_IRQHandler                  [WEAK]
                EXPORT     I2S2_IRQHandler                  [WEAK]
                EXPORT     USART1_IRQHandler                [WEAK]
                EXPORT     USART2_IRQHandler                [WEAK]
                EXPORT     USART3_IRQHandler                [WEAK]
                EXPORT     USART4_IRQHandler                [WEAK]
                EXPORT     EXTI15_10_IRQHandler             [WEAK]
                EXPORT     USB_WKUP_IRQHandler              [WEAK]
                EXPORT     LPUART1_IRQHandler               [WEAK]
                EXPORT     LPUART2_IRQHandler               [WEAK]
                EXPORT     LPTIM1_IRQHandler                [WEAK]
                EXPORT     FPU_IRQHandler                   [WEAK]
                EXPORT     TIM4_IRQHandler                  [WEAK]
                EXPORT     IWDT_WKUP_IRQHandler             [WEAK]                
                EXPORT     AES_SPI1_IRQHandler              [WEAK]
                EXPORT     ETH_IRQHandler                   [WEAK]
                EXPORT     ETH_WKUP_IRQHandler              [WEAK] 
                EXPORT     USART5_IRQHandler                [WEAK]
                EXPORT     USART6_IRQHandler                [WEAK]
                EXPORT     USART7_IRQHandler                [WEAK]
                EXPORT     USART8_IRQHandler                [WEAK]
                EXPORT     TIM5_IRQHandler                  [WEAK]
                EXPORT     TIM9_IRQHandler                  [WEAK]
                EXPORT     TIM10_IRQHandler                 [WEAK]
                EXPORT     TIM26_IRQHandler                 [WEAK]   
                EXPORT     SPI7_IRQHandler                  [WEAK]
                EXPORT     OSPI1_IRQHandler                 [WEAK]
                EXPORT     OSPI2_IRQHandler                 [WEAK]                
                EXPORT     DMA1_CH0_IRQHandler              [WEAK]
                EXPORT     DMA1_CH1_IRQHandler              [WEAK]
                EXPORT     DMA1_CH2_IRQHandler              [WEAK]
                EXPORT     DMA1_CH3_IRQHandler              [WEAK]
                EXPORT     DMA2_CH0_IRQHandler              [WEAK]
                EXPORT     DMA2_CH1_IRQHandler              [WEAK]
                EXPORT     DMA2_CH2_IRQHandler              [WEAK]
                EXPORT     DMA2_CH3_IRQHandler              [WEAK]   
                EXPORT     BKPSRAM_SEC_IRQHandler           [WEAK]
                EXPORT     BKPSRAM_DED_IRQHandler           [WEAK]
                EXPORT     COMP1_IRQHandler                 [WEAK]
                EXPORT     FDCAN2_IRQHandler                 [WEAK]
                EXPORT     RSV70_IRQHandler                 [WEAK]
                EXPORT     RSV71_IRQHandler                 [WEAK]
                EXPORT     SDMMC_IRQHandler                 [WEAK]

                


                




WDT_IRQHandler				   
LVD_IRQHandler				   			
RTC_XTLSD_IRQHandler           
CLKRDY_IRQHandler               
EXTI0_IRQHandler		       
EXTI1_IRQHandler		      
EXTI2_IRQHandler		     
EXTI3_IRQHandler		      
EXTI4_IRQHandler		      
DMA1_IRQHandler		                           
DMA2_IRQHandler		           
ADC_IRQHandler		           
DAC_IRQHandler		          
FSUSB_IRQHandler		                                
FDCAN1_IRQHandler             		
EXTI9_5_IRQHandler              
TIM1_BRK_UP_TRG_COM_IRQHandler  
TIM1_CC_IRQHandler             
TIM2_IRQHandler	              
TIM3_IRQHandler               
TIM6_IRQHandler	            
TIM7_IRQHandler		            
TIM8_BRK_UP_TRG_COM_IRQHandler  	
TIM8_CC_IRQHandler              	
I2C1_IRQHandler                 
I2C2_IRQHandler                                   
SPI1_IRQHandler                                  
SPI2_IRQHandler	                               
SPI3_IRQHandler                
SPI4_IRQHandler                 
I2S1_IRQHandler	                
I2S2_IRQHandler	               
USART1_IRQHandler                                 
USART2_IRQHandler		    	
USART3_IRQHandler		  	                        
USART4_IRQHandler		    	
EXTI15_10_IRQHandler	    	
USB_WKUP_IRQHandler        
LPUART1_IRQHandler              
LPUART2_IRQHandler                            
LPTIM1_IRQHandler              
FPU_IRQHandler                  
TIM4_IRQHandler                
IWDT_WKUP_IRQHandler            
AES_SPI1_IRQHandler             
ETH_IRQHandler                 
ETH_WKUP_IRQHandler           
USART5_IRQHandler               
USART6_IRQHandler               
USART7_IRQHandler               
USART8_IRQHandler             
TIM5_IRQHandler                
TIM9_IRQHandler                
TIM10_IRQHandler                
TIM26_IRQHandler                
SPI7_IRQHandler                 
OSPI1_IRQHandler                
OSPI2_IRQHandler               
DMA1_CH0_IRQHandler            
DMA1_CH1_IRQHandler             
DMA1_CH2_IRQHandler            
DMA1_CH3_IRQHandler            
DMA2_CH0_IRQHandler            
DMA2_CH1_IRQHandler             
DMA2_CH2_IRQHandler           
DMA2_CH3_IRQHandler             
BKPSRAM_SEC_IRQHandler         
BKPSRAM_DED_IRQHandler         
COMP1_IRQHandler                
FDCAN2_IRQHandler                
RSV70_IRQHandler             
RSV71_IRQHandler             
SDMMC_IRQHandler               


                B        .
                ENDP                                                                  
                 
                ALIGN
                                 
; User Initial Stack & Heap
                 
                IF      :DEF:__MICROLIB
                 
                EXPORT  __initial_sp
                EXPORT  __heap_base
                EXPORT  __heap_limit
                 
                ELSE
                 
                IMPORT  __use_two_region_memory
                EXPORT  __user_initial_stackheap
__user_initial_stackheap
                 
                LDR     R0, =  Heap_Mem
                LDR     R1, =(Stack_Mem + Stack_Size)
                LDR     R2, = (Heap_Mem +  Heap_Size)
                LDR     R3, = Stack_Mem
                BX      LR

                ALIGN

                ENDIF

                END
