
/******************************************************************************
*@file  : system_acm32p4xx.c
*@brief : CMSIS Cortex-M33 Device Peripheral Access Layer System Source File
******************************************************************************/

#include "acm32p4xx_hal_conf.h"

#ifdef DATA_IN_ExtSRAM
  void SystemInit_ExtMemCtl(void); 
#endif

/******************************************************************************
*@note  : g_SystemCoreClock variable is updated in three ways:
*           1) by calling CMSIS function SystemCoreClockUpdate()
*           2) by calling HAL API function HAL_RCC_GetSysCoreClockFreq()
*           3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
*               If you use this function to configure the system clock; then there
*               is no need to call the 2 first functions listed above, since g_SystemCoreClock
*               variable is updated automatically.
******************************************************************************/
volatile uint32_t SystemCoreClock = 64000000;

/******************************************************************************
*@brief : configure FPU and vector table address
*         - This function is called at startup just after reset and before branch to main program. 
*         - This call is made inside the "startup_acm32p4xx.s" file
*@param : none
*@return: none
******************************************************************************/
__attribute__((weak)) void SystemInit(void)
{  
    RCC->CLKOCR &= ~RCC_CLKOCR_MCO1EN; // disable CLKOUT for wakeup 
    
    SCB->VTOR = VECT_TAB_ADDR;
    
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
		/* set CP10 and CP11 Full Access */
		SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));
	#endif
    
    __HAL_RCC_PMU_CLK_ENABLE();
    PMU->TEST_ANATEST_SR = 2; //ANA_OUT select VDD12_RET, avoid electric leakage in run mode
    __HAL_RCC_PMU_CLK_DISABLE();    
    
    RCC->RCHCR |= RCC_RCHCR_RCHEN;
    __NOP();__NOP();

    RCC->CCR1 &= ~RCC_CCR1_SYSCLKSEL;
    
    RCC->PLL1CR &= ~RCC_PLL1CR_PLL1EN;
    RCC->PLL1SCR = 0;
    RCC->PLL1CR |= RCC_PLL1CR_PLL1SLEEP;
               
    RCC->RCHCR &= ~RCC_RCHCR_RCHDIV;
    
    RCC->CCR2 &= ~RCC_CCR2_SYSDIV0;
    __NOP();__NOP();

    while (!(RCC->CCR2 & RCC_CCR2_DIVDONE)) {}
    
    RCC->CCR2 &= ~RCC_CCR2_SYSDIV1;
    __NOP();__NOP();

    while (!(RCC->CCR2 & RCC_CCR2_DIVDONE)) {}
    
    RCC->CCR2 &= ~(RCC_CCR2_PCLK1DIV | RCC_CCR2_PCLK2DIV);
	     
	#ifdef DATA_IN_ExtSRAM
		SystemInit_ExtMemCtl(); 
	#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */  
       
	#if (INS_ACCELERATE_ENABLE)
		System_EnableIAccelerate();
    #else
		System_DisableIAccelerate();
	#endif        
}

/******************************************************************************
*@brief : Update g_SystemCoreClock variable according to Clock Register Values.
*         The SystemCoreClock variable contains the core clock (HCLK), it can
*         be used by the user application to setup the SysTick timer or configure
*         other parameters.
*           
*@note  : Each time the core clock (HCLK) changes, this function must be called
*         to update SystemCoreClock variable value. Otherwise, any configuration
*         based on this variable will be incorrect.  
*@param : none
*@return: none
******************************************************************************/
void SystemCoreClockUpdate(void)
{
    HAL_RCC_GetSysCoreClockFreq();   
}



/******************************************************************************
*@brief : fast config system core clock. 
*@param : sysclkSel: system core clk select, see SYSCLK_SelectTypeDef  enum
*@param : pclk1Div: pclk1 div select
*           @arg RCC_PCLK1_DIV_1
*           @arg RCC_PCLK1_DIV_2
*           @arg RCC_PCLK1_DIV_4
*           @arg RCC_PCLK1_DIV_8
*           @arg RCC_PCLK1_DIV_16
*@param : pclk2Div: pclk2 div select
*           @arg RCC_PCLK2_DIV_1
*           @arg RCC_PCLK2_DIV_2
*           @arg RCC_PCLK2_DIV_4
*           @arg RCC_PCLK2_DIV_8
*           @arg RCC_PCLK2_DIV_16
*@note  : PLLPCLK = Fin * PLLF / PLLN / PLLP
*         PLLQCLK = Fin * PLLF / PLLN / PLLQ
*         1MHz <= ( Fin / PLLN ) <= 2MHz
*         100MHz <= ((Fin / PLLN) * PLLF) <= 550MHz
*         30MHz <= ((Fin / PLLN) * PLLF / PLLP) <= 220MHz
*         16MHz <= ((Fin / PLLN) * PLLF / PLLQ) <= 220MHz
*@return: none
******************************************************************************/
HAL_StatusTypeDef SystemClock_Config(uint32_t sysclkSel, uint32_t pclk1Div, uint32_t pclk2Div)
{
    RCC_OscInitTypeDef      RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef      RCC_ClkInitStruct = {0};  
    
    switch(sysclkSel)
    {      
        // PLL-RCH
        case SYSCLK_180M_SRC_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_RCH;
            RCC_OscInitStruct.PLL1.PLLF = 242; 
            RCC_OscInitStruct.PLL1.PLLN = 43;
            RCC_OscInitStruct.PLL1.PLLP = 2; // DIV2
            RCC_OscInitStruct.PLL1.PLLQ = 2; // DIV2 
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE; 
            
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
        
        case SYSCLK_160M_SRC_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_RCH;
            RCC_OscInitStruct.PLL1.PLLF = 215;
            RCC_OscInitStruct.PLL1.PLLN = 43;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
        
        case SYSCLK_120M_SRC_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_RCH;
            RCC_OscInitStruct.PLL1.PLLF = 161;
            RCC_OscInitStruct.PLL1.PLLN = 43;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
        
        case SYSCLK_80M_SRC_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_RCH;
            RCC_OscInitStruct.PLL1.PLLF = 215;
            RCC_OscInitStruct.PLL1.PLLN = 43;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 2;  
            break;
            
        // PLL-XTH               
        case SYSCLK_180M_SRC_XTH_12M:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 360;
            RCC_OscInitStruct.PLL1.PLLN = 12; 
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
            
            
        case SYSCLK_160M_SRC_XTH_12M:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 320;
            RCC_OscInitStruct.PLL1.PLLN = 12;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
            
            
        case SYSCLK_120M_SRC_XTH_12M:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 240;
            RCC_OscInitStruct.PLL1.PLLN = 12;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
            
        case SYSCLK_100M_SRC_XTH_12M:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 200;
            RCC_OscInitStruct.PLL1.PLLN = 12;
            RCC_OscInitStruct.PLL1.PLLP = 2;
            RCC_OscInitStruct.PLL1.PLLQ = 2;  
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;

            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
        
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;
            

        case SYSCLK_80M_SRC_XTH_12M:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_PLL1;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = ENABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 160;
            RCC_OscInitStruct.PLL1.PLLN = 12;
            RCC_OscInitStruct.PLL1.PLLP = 2;  
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = ENABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            #ifdef RCC_PLL_SSC_ENABLE
            RCC_OscInitStruct.PLL1.SSC = ENABLE;
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #else
            RCC_OscInitStruct.PLL1.SSC = DISABLE;  
            RCC_OscInitStruct.PLL1.Mode = RCC_PLL1SCR_PLL1SSCMD; // down
            RCC_OscInitStruct.PLL1.Period = 512; //1KHz 
            RCC_OscInitStruct.PLL1.Step = 126;  
            #endif 
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_PLL1PCLK;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1;
            break;  

        case SYSCLK_12M_XTH_12M:      
        case SYSCLK_6M_XTH_12M:  
        case SYSCLK_4M_XTH_12M:    
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_XTH;
            RCC_OscInitStruct.XTH = ENABLE;
            RCC_OscInitStruct.XTHBypass = DISABLE;
            RCC_OscInitStruct.PLL1.PLL = DISABLE;
            RCC_OscInitStruct.PLL1.Source = RCC_PLL_SOURCE_XTH;
            RCC_OscInitStruct.PLL1.PLLF = 160;
            RCC_OscInitStruct.PLL1.PLLN = 12;
            RCC_OscInitStruct.PLL1.PLLP = 2;  
            RCC_OscInitStruct.PLL1.PLLQ = 2;
            RCC_OscInitStruct.PLL1.PLLPCLK = DISABLE;
            RCC_OscInitStruct.PLL1.PLLQCLK = DISABLE;
            RCC_OscInitStruct.PLL1.SSC = DISABLE;
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_XTH;  
            RCC_ClkInitStruct.SYSCLKDiv0 = 1 + (sysclkSel - SYSCLK_12M_XTH_12M);  
            break;  
         
        // RCH       
        case SYSCLK_64M_RCH:
        case SYSCLK_32M_RCH:
        case SYSCLK_16M_RCH:
        case SYSCLK_8M_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_RCH;
            RCC_ClkInitStruct.SYSCLKDiv0 = 1UL << (sysclkSel - SYSCLK_64M_RCH);
            break;
            
        case SYSCLK_4M_RCH:
            RCC_OscInitStruct.OscType = RCC_OSC_TYPE_RCH;
            RCC_OscInitStruct.RCH = ENABLE;
            RCC_OscInitStruct.RCHDiv16 = DISABLE;
            RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLK_SOURCE_RCH;
            RCC_ClkInitStruct.SYSCLKDiv0 = 4;
            break;
            
            
        default:
            return (HAL_ERROR);       
    }
    
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
        return (HAL_ERROR);

    RCC_ClkInitStruct.ClockType = RCC_CLOCK_TYPE_SYSCLK | \
                                  RCC_CLOCK_TYPE_SYSDIV0 | RCC_CLOCK_TYPE_SYSDIV1 | \
                                  RCC_CLOCK_TYPE_PCLK1 | RCC_CLOCK_TYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKDiv1 = 1;
    RCC_ClkInitStruct.PCLK1Div = pclk1Div;
    RCC_ClkInitStruct.PCLK2Div = pclk2Div;
    
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct) != HAL_OK)
        return (HAL_ERROR);
    return (HAL_OK);
}

#ifdef DATA_IN_ExtSRAM

__attribute__((weak)) void SystemInit_ExtMemCtl(void)
{
}

#endif /* (DATA_IN_ExtSRAM) || defined (DATA_IN_ExtSDRAM) */

