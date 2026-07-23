
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


/******************************************************************************
*@brief : Get Chip SN.
*@param : none.
*@return: HAL_OK,HAL_ERROR
******************************************************************************/
HAL_StatusTypeDef System_Get_ChipSN(uint8_t *pSN)
{
    uint8_t buff[16];
    
    if (NULL == pSN)
    {
        return HAL_ERROR;   
    }
    
    if(HAL_EFUSE_ReadBytes(EFUSE1, 0x40, buff, 13, 10000) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    memcpy(pSN, buff, 13);
    
    if(HAL_EFUSE_ReadBytes(EFUSE1, 0x20, buff, 3, 10000) != HAL_OK) 
    {       
        return HAL_ERROR;
    }
    
    memcpy(pSN + 13, buff, 3);  
    
    if (buff[0] != (uint8_t)(~buff[2]) ) 
    {
        return HAL_ERROR;  
    }
        
    return HAL_OK;  
}


#ifdef DATA_IN_ExtSRAM

/******************************************************************************
*@brief : OSPI1 APS6408L PSRAM early init (register-level, no HAL).
*         Pins: PG5 SCK, PG6-13 IO0-7, PG14 CS, PG15 DQS, PF10 RESET
*         Map:  0x80000000 ~ 0x807FFFFF (8MB)
*
*         SystemInit_ExtMemCtl(): early GPIO/reset + optional MPU (from SystemInit)
*         System_OSPI_PSRAM_Reclock(): full controller init after SystemClock_Config
******************************************************************************/

#ifndef SYSTEM_INIT_OSPI_PSRAM
#define SYSTEM_INIT_OSPI_PSRAM  1U
#endif
/* Non-cacheable PSRAM region (ETH DMA may use OSPI-mapped buffers) */
#ifndef SYSTEM_MPU_OSPI_PSRAM
#define SYSTEM_MPU_OSPI_PSRAM   1U
#endif

#define APS6408L_SYNC_READ_LINEAR_BURST   0x20U
#define APS6408L_SYNC_WRITE_LINEAR_BURST  0xA0U
#define APS6408L_READ_REG                 0x40U
#define APS6408L_WRITE_REG                0xC0U
#define APS6408L_REG_MR0                  0x00U
#define APS6408L_REG_MR1                  0x01U
#define APS6408L_MR0_FIXED_READ_LATENCY   (1U << 5)
#define APS6408L_DEVICE_ID                0x930DU

/* ~1ms @ ~64..180MHz (SystemInit may run before PLL switch) */
#define EXTMEM_RESET_DELAY_CYCLES         180000U
#define EXTMEM_PSRAM_BASE_ADDR            0x80000000U
#define EXTMEM_PSRAM_LIMIT_ADDR           0x807FFFFFU

static volatile uint16_t s_ExtMem_MR0;
static volatile uint16_t s_ExtMem_MR1;
static volatile int s_ExtMem_MR_OK;
static volatile int s_ExtMem_InitOK;

static void ExtMem_Delay(volatile uint32_t cycles)
{
    while (cycles != 0U)
    {
        __NOP();
        cycles--;
    }
}

static void ExtMem_GPIO_Init(void)
{
    /* GPIOF/GPIOG clocks (AHB2), OSPI1 clock (AHB3) */
    RCC->AHB2CKENR |= (1UL << 5) | (1UL << 6);
    RCC->AHB3CKENR |= (1UL << 8);

    /* OSPI1 reset pulse */
    RCC->AHB3RSTR &= ~(1UL << 8);
    ExtMem_Delay(100U);
    RCC->AHB3RSTR |= (1UL << 8);
    ExtMem_Delay(100U);

    /* PG5-15 AF mode (MD=10) */
    GPIOG->MD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                   (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                   (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                   (0x3UL << 28) | (0x3UL << 30));
    GPIOG->MD |=  (2UL << 10) | (2UL << 12) | (2UL << 14) |
                  (2UL << 16) | (2UL << 18) | (2UL << 20) |
                  (2UL << 22) | (2UL << 24) | (2UL << 26) |
                  (2UL << 28) | (2UL << 30);

    /* AF: PG5-9/12-15=AF2, PG10=AF6, PG11=AF5 */
    GPIOG->AF0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    GPIOG->AF0 |=  (2UL << 20) | (2UL << 24) | (2UL << 28);

    GPIOG->AF1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                    (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                    (0xFUL << 24) | (0xFUL << 28));
    GPIOG->AF1 |=  (2UL << 0)  | (2UL << 4)  | (6UL << 8)  |
                   (5UL << 12) | (2UL << 16) | (2UL << 20) |
                   (2UL << 24) | (2UL << 28);

    /* Pull-up + drive 8mA */
    GPIOG->PUPD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                     (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                     (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                     (0x3UL << 28) | (0x3UL << 30));
    GPIOG->PUPD |=  (1UL << 10) | (1UL << 12) | (1UL << 14) |
                    (1UL << 16) | (1UL << 18) | (1UL << 20) |
                    (1UL << 22) | (1UL << 24) | (1UL << 26) |
                    (1UL << 28) | (1UL << 30);

    GPIOG->DS0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    GPIOG->DS0 |=  (4UL << 20) | (4UL << 24) | (4UL << 28);

    GPIOG->DS1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                    (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                    (0xFUL << 24) | (0xFUL << 28));
    GPIOG->DS1 |=  (4UL << 0) | (4UL << 4) | (4UL << 8) |
                   (4UL << 12) | (4UL << 16) | (4UL << 20) |
                   (4UL << 24) | (4UL << 28);

    /* PF10 push-pull output: PSRAM RESET */
    GPIOF->MD &= ~(3UL << 20);
    GPIOF->MD |=  (1UL << 20);
    GPIOF->OTYP &= ~(1UL << 10);
    GPIOF->PUPD &= ~(3UL << 20);

    GPIOF->ODATA &= ~(1UL << 10);
    ExtMem_Delay(EXTMEM_RESET_DELAY_CYCLES);
    GPIOF->ODATA |=  (1UL << 10);
    ExtMem_Delay(EXTMEM_RESET_DELAY_CYCLES);
}

static uint16_t ExtMem_DTR_Cmd(uint8_t cmd)
{
    return (uint16_t)cmd | ((uint16_t)cmd << 8);
}

static int ExtMem_WaitStatus(uint32_t mask)
{
    uint32_t t = 2000000U;

    while (((OSPI1->STATUS & mask) == 0U) && (t != 0U))
        t--;
    return (t != 0U) ? 1 : 0;
}

/* STATUS: bit3=TX_FIFO_FULL, bit4=RX_FIFO_EMPTY (wait until clear) */
static int ExtMem_WriteFifoHalf(uint16_t data)
{
    uint32_t t = 2000000U;

    while ((OSPI1->STATUS & OSPI_STATUS_TX_FIFO_FULL) && (t != 0U))
        t--;
    if (t == 0U)
        return 0;
    OSPI1->DAT = data;
    return 1;
}

static int ExtMem_ReadFifoHalf(uint16_t *data)
{
    uint32_t t = 2000000U;

    while ((OSPI1->STATUS & OSPI_STATUS_RX_FIFO_EMPTY) && (t != 0U))
        t--;
    if (t == 0U)
        return 0;
    *data = (uint16_t)(OSPI1->DAT & 0xFFFFU);
    return 1;
}

static int ExtMem_APS6408L_ReadReg(uint8_t reg_addr, uint16_t *value)
{
    uint16_t rx_data;

    if (value == 0)
        return 0;

    /* stop + clear + fifo reset (align SDK ospi_stop / reset_fifo) */
    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = 0xFFFFU;
    OSPI1->TX_CTL |= (1UL << 1);
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);
    OSPI1->RX_CTL &= ~(1UL << 1);

    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U)
        return 0;

    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    OSPI1->CS = 0U;
    OSPI1->BATCH = 14U;
    OSPI1->TX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!ExtMem_WriteFifoHalf(ExtMem_DTR_Cmd(APS6408L_READ_REG)) ||
        !ExtMem_WriteFifoHalf(0x0000U) ||
        !ExtMem_WriteFifoHalf((uint16_t)reg_addr << 8) ||
        !ExtMem_WriteFifoHalf(0x0000U) ||
        !ExtMem_WriteFifoHalf(0x0000U) ||
        !ExtMem_WriteFifoHalf(0x0000U) ||
        !ExtMem_WriteFifoHalf(0x0000U))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    if (!ExtMem_WaitStatus(OSPI_STATUS_TX_BATCH_DONE))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = OSPI_STATUS_BATCH_DONE | OSPI_STATUS_TX_BATCH_DONE;

    OSPI1->BATCH = 2U;
    OSPI1->RX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!ExtMem_ReadFifoHalf(&rx_data) ||
        !ExtMem_WaitStatus(OSPI_STATUS_RX_BATCH_DONE))
    {
        OSPI1->RX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->CS = 0U;
    OSPI1->STATUS = 0xFFFFU;

    *value = rx_data;
    OSPI1->MEMO_ACC1 |= (1UL << 0);
    return 1;
}

static int ExtMem_APS6408L_WriteReg(uint8_t reg_addr, uint16_t value)
{
    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = 0xFFFFU;
    OSPI1->TX_CTL |= (1UL << 1);
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);
    OSPI1->RX_CTL &= ~(1UL << 1);

    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U)
        return 0;

    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    OSPI1->CS = 0U;
    OSPI1->BATCH = 8U;
    OSPI1->TX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!ExtMem_WriteFifoHalf(ExtMem_DTR_Cmd(APS6408L_WRITE_REG)) ||
        !ExtMem_WriteFifoHalf(0x0000U) ||
        !ExtMem_WriteFifoHalf((uint16_t)reg_addr << 8) ||
        !ExtMem_WriteFifoHalf(value) ||
        !ExtMem_WaitStatus(OSPI_STATUS_TX_BATCH_DONE))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->CS = 0U;
    OSPI1->STATUS = 0xFFFFU;

    OSPI1->MEMO_ACC1 |= (1UL << 0);
    return 1;
}

static int ExtMem_ConfigMR0_FixedLatency(void)
{
    uint16_t mr0_val = 0;
    uint16_t mr1 = 0;
    uint16_t verify = 0;

    s_ExtMem_MR0 = 0;
    s_ExtMem_MR1 = 0;
    s_ExtMem_MR_OK = 0;

    if (!ExtMem_APS6408L_ReadReg(APS6408L_REG_MR0, &mr0_val))
        return 0;
    if (!ExtMem_APS6408L_ReadReg(APS6408L_REG_MR1, &mr1))
        return 0;

    s_ExtMem_MR0 = mr0_val;
    s_ExtMem_MR1 = mr1;

    if ((mr0_val == 0x0000U) || (mr0_val == 0xFFFFU))
        return 0;
    if ((mr1 != APS6408L_DEVICE_ID) && (mr1 != 0x0D93U))
    {
        if ((mr0_val & 0xFF00U) == 0U)
            return 0;
    }

    if ((mr0_val & APS6408L_MR0_FIXED_READ_LATENCY) == 0U)
    {
        mr0_val |= APS6408L_MR0_FIXED_READ_LATENCY;
        if (!ExtMem_APS6408L_WriteReg(APS6408L_REG_MR0, mr0_val))
            return 0;
        if (!ExtMem_APS6408L_ReadReg(APS6408L_REG_MR0, &verify))
            return 0;
        s_ExtMem_MR0 = verify;
        if ((verify & APS6408L_MR0_FIXED_READ_LATENCY) == 0U)
            return 0;
    }

    OSPI1->CS = 0x01U;
    s_ExtMem_MR_OK = 1;
    return 1;
}

/* BAUD.div1: OSPI_CLK = HCLK / (2 * (div1 + 1)); target ~18MHz */
static uint32_t ExtMem_CalcBaudDiv(void)
{
    uint32_t hclk = SystemCoreClock;

    if (hclk == 0U)
        hclk = 180000000U;
    /* match SDK: 180MHz HCLK -> div1=4 -> 18MHz OSPI */
    if (hclk >= 160000000U)
        return 4U;
    if (hclk >= 100000000U)
        return 2U;
    return 1U;
}

static int ExtMem_OSPI_Config(void)
{
    OSPI_TypeDef *ospi = OSPI1;
    uint32_t baud = ExtMem_CalcBaudDiv();

    ospi->CTL = 0U;
    ospi->TX_CTL = 0U;
    ospi->RX_CTL = 0U;

    ospi->BAUD = baud;

    /* master, 8-line, IO auto, DTR, DQS, half-word FIFO, Xccela — match SDK */
    ospi->CTL = (1U << 0) | (3U << 5) | (1U << 7) | (1U << 8) |
                (1U << 9) | (1U << 28) | (1U << 30);

    /* OUTDLY=1, SSHIFT=3 (SDK working value) */
    ospi->TX_CTL = (1U << 16);
    ospi->RX_CTL = (3U << 28);
    ospi->OUT_EN = 0xFFU;
    ospi->CS = 0x01U;
    ospi->CMD = 0xA0A02020U;

    ospi->MEMO_ACC1 = (1U << 0) | (1U << 3) | (1U << 10) | (1U << 11) |
                      (4U << 12) | (3U << 17);

    ospi->MEMO_ACC2 = (1U << 0) | (1U << 4) | (7U << 8) | (7U << 15) |
                      (3U << 27);

    ospi->MEMO_ACC3 = (1U << 0) | (1U << 1);

    ospi->CS_TIMEOUT_VAL = (1U << 16) | 500U;

    if (!ExtMem_ConfigMR0_FixedLatency())
    {
        ospi->MEMO_ACC1 = (ospi->MEMO_ACC1 & ~(0x1FUL << 12)) | (8U << 12);
        ospi->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    ospi->MEMO_ACC1 = (ospi->MEMO_ACC1 & ~(0x1FUL << 12)) | (8U << 12);
    ospi->MEMO_ACC1 |= (1UL << 0);
    return 1;
}

static int ExtMem_OSPI_Init(void)
{
    ExtMem_GPIO_Init();
    if (!ExtMem_OSPI_Config())
    {
        s_ExtMem_InitOK = 0;
        return 0;
    }

    __DSB();
    __ISB();
    ExtMem_Delay(10000U);
    s_ExtMem_InitOK = 1;
    return 1;
}

#if SYSTEM_MPU_OSPI_PSRAM
static void ExtMem_MPU_Config(void)
{
#if defined(__MPU_PRESENT) && (__MPU_PRESENT == 1U)
    /* ARMv8-M RBAR: RO=0 (RW), NP=1 (unpriv OK), XN=1 (no-exec) */
    ARM_MPU_Disable();
    ARM_MPU_SetMemAttr(1U,
                       ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE,
                                    ARM_MPU_ATTR_NON_CACHEABLE));
    ARM_MPU_SetRegion(0U,
                      ARM_MPU_RBAR(EXTMEM_PSRAM_BASE_ADDR,
                                   ARM_MPU_SH_NON,
                                   0U,
                                   1U,
                                   1U),
                      ARM_MPU_RLAR(EXTMEM_PSRAM_LIMIT_ADDR, 1U));
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
#endif
}
#endif /* SYSTEM_MPU_OSPI_PSRAM */

/******************************************************************************
*@brief : Called from SystemInit() when DATA_IN_ExtSRAM is defined.
*         Early pad/reset only; full OSPI map after SystemClock_Config via
*         System_OSPI_PSRAM_Reclock() so BAUD matches final HCLK.
*@param : none
*@return: none
******************************************************************************/
void SystemInit_ExtMemCtl(void)
{
#if SYSTEM_MPU_OSPI_PSRAM
    ExtMem_MPU_Config();
#endif
#if SYSTEM_INIT_OSPI_PSRAM
    ExtMem_GPIO_Init();
#endif
}

/******************************************************************************
*@brief : Full OSPI PSRAM re-init after SystemClock_Config()
*@param : none
*@return: none
******************************************************************************/
void System_OSPI_PSRAM_Reclock(void)
{
#if SYSTEM_INIT_OSPI_PSRAM
    (void)ExtMem_OSPI_Init();
#endif
}

/******************************************************************************
*@brief : Query last ExtMem OSPI init status
*@param : none
*@return: 1 = OK, 0 = fail / not inited
******************************************************************************/
int System_OSPI_PSRAM_Ready(void)
{
    return s_ExtMem_InitOK;
}

/******************************************************************************
*@brief : Read last APS6408L MR0/MR1 snapshot from init
*@param : mr0 / mr1 / mr_ok — optional output pointers
*@return: none
******************************************************************************/
void System_OSPI_PSRAM_GetMR(uint16_t *mr0, uint16_t *mr1, int *mr_ok)
{
    if (mr0)
        *mr0 = s_ExtMem_MR0;
    if (mr1)
        *mr1 = s_ExtMem_MR1;
    if (mr_ok)
        *mr_ok = s_ExtMem_MR_OK;
}

#endif /* DATA_IN_ExtSRAM */

