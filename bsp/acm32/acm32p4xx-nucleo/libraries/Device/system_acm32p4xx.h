
/******************************************************************************
*@file  : system_acm32p4xx.h
*@brief : CMSIS Cortex-M33 Device Peripheral Access Layer System Source File
******************************************************************************/

#ifndef __SYSTEM_ACM32P4XX_H
#define __SYSTEM_ACM32P4XX_H

#include "acm32p4xx_hal_conf.h"
    
typedef enum 
{
    SYSCLK_180M_SRC_RCH = 0,
    SYSCLK_160M_SRC_RCH,
    SYSCLK_120M_SRC_RCH,
    SYSCLK_80M_SRC_RCH,
    
    SYSCLK_180M_SRC_XTH_12M, // OSC=12M, FCLK = 180M PLL Clock 
    SYSCLK_170M_SRC_XTH_12M,
    SYSCLK_160M_SRC_XTH_12M,
    SYSCLK_150M_SRC_XTH_12M,
    SYSCLK_140M_SRC_XTH_12M,
    SYSCLK_130M_SRC_XTH_12M,
    SYSCLK_120M_SRC_XTH_12M,
    SYSCLK_110M_SRC_XTH_12M,
    SYSCLK_100M_SRC_XTH_12M,
    SYSCLK_90M_SRC_XTH_12M,
    SYSCLK_80M_SRC_XTH_12M,
    
    SYSCLK_12M_XTH_12M,
    SYSCLK_6M_XTH_12M,  // OSC=12M, FCLK = 6M
    SYSCLK_4M_XTH_12M,  // OSC=12M, FCLK = 4M
    
    SYSCLK_64M_RCH,
    SYSCLK_32M_RCH,
    SYSCLK_16M_RCH,
    SYSCLK_8M_RCH,
    SYSCLK_4M_RCH,

  
    
    SYSCLK_MAX,

}SYSCLK_SelectTypeDef;

/****** system core clock select, uesr config ***********/
#define SYSCLK_SELECT       SYSCLK_180M_SRC_RCH 

/******************************************************************************
*@brief : PCLK1_DIV_SELECT: pclk1 div select, uesr config
*           @arg 1, 2, 4, 8, 16
******************************************************************************/
#define PCLK1_DIV_SELECT    1

/******************************************************************************
*@brief : PCLK2_DIV_SELECT: pclk2 div select, uesr config
*           @arg 1, 2, 4, 8, 16
******************************************************************************/
#define PCLK2_DIV_SELECT    1


/******************************************************************************
*@brief : PCLK3_DIV_SELECT: pclk3 div select, uesr config
*           @arg 1, 2, 4, 8, 16
******************************************************************************/
#define PCLK3_DIV_SELECT    2


/******************************************************************************
*@brief : PCLK4_DIV_SELECT: pclk4 div select, uesr config
*           @arg 1, 2, 4, 8, 16
******************************************************************************/
#define PCLK4_DIV_SELECT    2

#define RCH_TRIM_LOW_NVR_INDEX   0x23  
#define RCH_TRIM_HIGH_NVR_INDEX  0x24
#define RCH_FREQ_NVR_INDEX       0x25


/******************************************************************************
*@note  : SystemCoreClock variable is updated in three ways:
*           1) by calling CMSIS function SystemCoreClockUpdate()
*           2) by calling HAL API function HAL_RCC_GetSysClkFreq()
*           3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency 
*               If you use this function to configure the system clock; then there
*               is no need to call the 2 first functions listed above, since SystemCoreClock
*               variable is updated automatically.
******************************************************************************/
extern volatile uint32_t SystemCoreClock; /*!< System Clock Frequency (Core Clock) */

void SystemInit(void);
void SystemCoreClockUpdate(void);
HAL_StatusTypeDef SystemClock_Config(uint32_t sysclkSel, uint32_t pclk1Div, uint32_t pclk2Div);

#endif /* __SYSTEM_ACM32P4XX_H */
