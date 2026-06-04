/******************************************************************************
*@file  : hal_rcc.h
*@brief : Header file of RCC HAL module.
******************************************************************************/

#ifndef __HAL_RCC_H__
#define __HAL_RCC_H__

#include "acm32p4xx_hal_conf.h"
   

/**
  * @brief  RCC PLL configuration structure definition
  */
  
typedef struct
{
    uint32_t PLL;               // The new state of the PLLx.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t Source;            // specifies the PLLx entry clock source.
                                // This parameter can be one of @ref RCC_PLL_Source

    uint32_t PLLQCLK;           // The new state of the PLLxQCLK.
                                // This parameter can be: ENABLE or DISABLE.
    
    uint32_t PLLPCLK;           // The new state of the PLLxPCLK.
                                // This parameter can be: ENABLE or DISABLE.
    
    uint32_t PLLQ;              // PLLxQCLK output frequency division control field.

    uint32_t PLLP;              // PLLxPCLK output frequency division control field.

    uint32_t PLLN;              // specifies the PLLx multiplication factor.

    uint32_t PLLF;              // specifies the PLLx down factor divider field.

    uint32_t SSC;               // The new state of the SSC.
                                // This parameter can be: ENABLE or DISABLE.
                                // This parameter is only valid for PLL1 and PLL2.

    uint32_t Mode;              // specifies the SSC mode.
                                // This parameter can be one of the @ref RCC_SSC_Mode
                                // This parameter is only valid for PLL1 and PLL2.

    uint32_t Period;            // specifies the SSC modulation period.
                                // This parameter must be a number between 0 and 8191.
                                // This parameter is only valid for PLL1 and PLL2.

    uint32_t Step;              // specifies the SSC modulation step.
                                // This parameter must be a number between 0 and 32767.
                                // This parameter is only valid for PLL1 and PLL2.

}RCC_PLLInitTypeDef;

/**
  * @brief  RCC Internal/External Oscillator (RCH, RCL, XTH, and XTL) or PLL (PLL1, PLL2, PLL3) 
  *         configuration structure definition
  */

typedef struct
{
    uint32_t    OscType;        // The oscillators or PLL to be configured.
                                // This parameter can be one of @ref RCC_Oscillator_Type

    uint32_t    RCH;            // The new state of the RCH.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    RCHDiv16;       // The new state of the 16th frequency division of RCH.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    RCL;            // The new state of the RCL.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    XTH;            // The new state of the XTH.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    XTHBypass;      // new state of the XTH oscillator bypass.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    XTL;            // The new state of the XTL.
                                // This parameter can be: ENABLE or DISABLE.

    uint32_t    XTLBypass;      // new state of the XTL oscillator bypass.
                                // This parameter can be: ENABLE or DISABLE.

    RCC_PLLInitTypeDef  PLL1;   // PLL1 structure parameters
    
    
}RCC_OscInitTypeDef;


/**
  * @brief  RCC System, AHB and APB busses clock configuration structure definition
  */

typedef struct
{
    uint32_t    ClockType;      // The clock to be configured.
                                // This parameter can be one of @ref RCC_Clock_Type

    uint32_t    SYSCLKSource;   // The clock source (SYSCLK) used as system clock.
                                // This parameter can be one of @ref RCC_Sysclk_Source

    uint32_t    SYSCLKDiv0;     // the first level frequency division of system clock.
                                // This parameter must be a number between 1 and 16.

    uint32_t    SYSCLKDiv1;     // the second level frequency division of system clock.
                                // This parameter must be a number between 1 and 16.

    uint32_t    PCLK1Div;       // The APB1 clock (PCLK1) divider.
                                // This parameter must be one of 1, 2, 4, 8 and 16.

    uint32_t    PCLK2Div;       // The APB2 clock (PCLK2) divider.
                                // This parameter must be one of 1, 2, 4, 8 and 16.

    uint32_t    PCLK3Div;       // The APB3 clock (PCLK3) divider.
                                // This parameter must be one of 1, 2, 4, 8 and 16.

    uint32_t    PCLK4Div;       // The APB4 clock (PCLK4) divider.
                                // This parameter must be one of 1, 2, 4, 8 and 16.

}RCC_ClkInitTypeDef;

/**
  * @brief  RCC MCO configuration structure definition
  */

typedef struct
{
    uint32_t    MCO;            // Selects the clock source.
                                // This parameter can be a value of @ref RCC_MCO_Output                       */
    
    uint32_t    MCO1;           // The new state of the MCO1.
                                // This parameter can be: ENABLE or DISABLE.
    
    uint32_t    MCO1Div;        // MCO1 clock divider.
                                // This parameter must be a number between 1 and 65536.
    
    uint32_t    MCO1RevPol;     // The new state of the MCO1 clock reverse polarity.
                                // This parameter can be: ENABLE or DISABLE.
    
    uint32_t    MCO2;           // The new state of the MCO2.
                                // This parameter can be: ENABLE or DISABLE.
    
    uint32_t    MCO2Div;        // MCO2 clock divider.
                                // This parameter must be a number between 1 and 64.
    
    uint32_t    MCO2RevPol;     // The new state of the MCO2 clock reverse polarity.
                                // This parameter can be: ENABLE or DISABLE.
    
}RCC_MCOInitTypeDef;


/** @defgroup RCC_Oscillator_Type
  * @{
  */

#define RCC_OSC_TYPE_RCH                        ( 0x01U )
#define RCC_OSC_TYPE_RCL                        ( 0x02U )
#define RCC_OSC_TYPE_XTH                        ( 0x04U )
#define RCC_OSC_TYPE_XTL                        ( 0x08U )
#define RCC_OSC_TYPE_PLL1                       ( 0x10U )
#define RCC_OSC_TYPE_PLL2                       ( 0x20U )
#define RCC_OSC_TYPE_PLL3                       ( 0x40U )

/**
  * @}
  */
  
/** @brief  Oscillator type mask for assert test
  */

#define RCC_OSC_TYPE_MASK                       ( RCC_OSC_TYPE_RCH | RCC_OSC_TYPE_RCL | RCC_OSC_TYPE_XTH | RCC_OSC_TYPE_XTL | \
                                                  RCC_OSC_TYPE_PLL1 | RCC_OSC_TYPE_PLL2 | RCC_OSC_TYPE_PLL3 )

/** @defgroup RCC_SSC_Mode
  * @{
  */

#define RCC_PLL_SSC_MODE_CENTER                 ( 0 )
#define RCC_PLL_SSC_MODE_DOWN                   ( RCC_PLL1SCR_PLL1SSCMD )

/**
  * @}
  */
  
/** @defgroup RCC_IT
  * @{
  */

#define RCC_IT_RCL_READY                        ( BIT0 )
#define RCC_IT_XTL_READY                        ( BIT1 )
#define RCC_IT_RCH_READY                        ( BIT2 )
#define RCC_IT_XTH_READY                        ( BIT3 )
#define RCC_IT_PLL_READY                        ( BIT4 )
#define RCC_IT_XTH_STOP                         ( BIT27 )
#define RCC_IT_XTL_STOP                         ( BIT31 )

/**
  * @}
  */
  
/** @brief  RCC clock interrupt source mask
  */

#define RCC_IT_MASK                             ( RCC_IT_RCL_READY | RCC_IT_XTL_READY | RCC_IT_RCH_READY | \
                                                  RCC_IT_XTH_READY | RCC_IT_PLL_READY | \
                                                  RCC_IT_XTH_STOP | RCC_IT_XTL_STOP )
                                                  
/** @brief  RCC clock ready interrupt source mask
  */

#define RCC_IT_READY_MASK                       ( RCC_IT_RCL_READY | RCC_IT_XTL_READY | RCC_IT_RCH_READY | RCC_IT_XTH_READY | \
                                                  RCC_IT_PLL_READY )
                                                  
/** @brief  RCC clock stop interrupt source mask
  */

#define RCC_IT_STOP_MASK                        ( RCC_IT_XTH_STOP | RCC_IT_XTL_STOP )

/** @brief  RCC clear interrupt flag mask
  */

#define RCC_IT_CLEAR_FLAG_MASK                  ( (RCC_IT_READY_MASK << 16) | (RCC_IT_STOP_MASK >> 2) )

/** @defgroup RCC_XTL_Drive_Capacity
  * @{
  */

#define RCC_XTL_DRIVE_NORMAL_LEVEL0             ( 0U )
#define RCC_XTL_DRIVE_NORMAL_LEVEL1             ( 1U )
#define RCC_XTL_DRIVE_NORMAL_LEVEL2             ( 2U )
#define RCC_XTL_DRIVE_NORMAL_LEVEL3             ( 3U )
#define RCC_XTL_DRIVE_LOWPOWER_LEVEL0           ( 4U )
#define RCC_XTL_DRIVE_LOWPOWER_LEVEL1           ( 5U )
#define RCC_XTL_DRIVE_LOWPOWER_LEVEL2           ( 6U )
#define RCC_XTL_DRIVE_LOWPOWER_LEVEL3           ( 7U )
#define RCC_XTL_DRIVE_MAX                       ( 8U )

/**
  * @}
  */
  
/** @defgroup RCC_PLL_Source
  * @{
  */

#define RCC_PLL_SOURCE_RCH                      ( 0U )
#define RCC_PLL_SOURCE_XTH                      ( 1U )
#define RCC_PLL_SOURCE_MAX                      ( 2U )

/**
  * @}
  */
  
/** @defgroup RCC_Clock_Type
  * @{
  */

#define RCC_CLOCK_TYPE_SYSCLK                   ( 0x01U )
#define RCC_CLOCK_TYPE_SYSDIV0                  ( 0x02U )
#define RCC_CLOCK_TYPE_SYSDIV1                  ( 0x04U )
#define RCC_CLOCK_TYPE_PCLK1                    ( 0x08U )
#define RCC_CLOCK_TYPE_PCLK2                    ( 0x10U )
#define RCC_CLOCK_TYPE_PCLK3                    ( 0x20U )
#define RCC_CLOCK_TYPE_PCLK4                    ( 0x40U )

/**
  * @}
  */
  
/** @brief  System Clock type mask for assert test
  */

#define RCC_CLOCK_TYPE_MASK                     ( RCC_CLOCK_TYPE_SYSCLK | RCC_CLOCK_TYPE_SYSDIV0 | RCC_CLOCK_TYPE_SYSDIV1 | \
                                                  RCC_CLOCK_TYPE_PCLK1 | RCC_CLOCK_TYPE_PCLK2 | RCC_CLOCK_TYPE_PCLK3 | RCC_CLOCK_TYPE_PCLK4)

/** @defgroup RCC_Sysclk_Source
  * @{
  */

#define RCC_SYSCLK_SOURCE_RCH                   ( 0U )
#define RCC_SYSCLK_SOURCE_XTH                   ( 1U )
#define RCC_SYSCLK_SOURCE_PLL1PCLK              ( 2U )
#define RCC_SYSCLK_SOURCE_MAX                   ( 3U )

/**
  * @}
  */

/** @defgroup RCC_FLT_CLK_Source
  * @{
  */

#define RCC_FLT_CLK_SOURCE_PCLK1_DIV32          ( 0U )
#define RCC_FLT_CLK_SOURCE_RCL                  ( 1U )
#define RCC_FLT_CLK_SOURCE_MAX                  ( 2U )

/**
  * @}
  */
  
/** @defgroup RCC_LPUART_CLK_Source
  * @{
  */

#define RCC_LPUART_CLK_SOURCE_RCL              ( 0U )
#define RCC_LPUART_CLK_SOURCE_XTL              ( 1U )
#define RCC_LPUART_CLK_SOURCE_PCLK1_DIV4       ( 2U )
#define RCC_LPUART_CLK_SOURCE_PCLK1_DIV8       ( 3U )
#define RCC_LPUART_CLK_SOURCE_PCLK1_DIV16      ( 4U )
#define RCC_LPUART_CLK_SOURCE_PCLK1_DIV32      ( 5U )
#define RCC_LPUART_CLK_SOURCE_MAX              ( 6U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM1_CLK_Source
  * @{
  */

#define RCC_LPTIM1_CLK_SOURCE_PCLK1             ( 0U )
#define RCC_LPTIM1_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM1_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM1_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM1_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM2_CLK_Source
  * @{
  */

#define RCC_LPTIM2_CLK_SOURCE_PCLK1             ( 0U )
#define RCC_LPTIM2_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM2_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM2_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM2_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM3_CLK_Source
  * @{
  */

#define RCC_LPTIM3_CLK_SOURCE_PCLK3             ( 0U )
#define RCC_LPTIM3_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM3_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM3_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM3_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM345_CLK_Source
  * @{
  */

#define RCC_LPTIM4_CLK_SOURCE_PCLK3             ( 0U )
#define RCC_LPTIM4_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM4_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM4_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM4_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM345_CLK_Source
  * @{
  */

#define RCC_LPTIM5_CLK_SOURCE_PCLK3             ( 0U )
#define RCC_LPTIM5_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM5_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM5_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM5_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_LPTIM6_CLK_Source
  * @{
  */

#define RCC_LPTIM6_CLK_SOURCE_PCLK3             ( 0U )
#define RCC_LPTIM6_CLK_SOURCE_RCL               ( 1U )
#define RCC_LPTIM6_CLK_SOURCE_RCH               ( 2U )
#define RCC_LPTIM6_CLK_SOURCE_XTL               ( 3U )
#define RCC_LPTIM6_CLK_SOURCE_MAX               ( 4U )

/**
  * @}
  */

/** @defgroup RCC_SDMMC_CLK_Source
  * @{
  */

#define RCC_SDMMC_CLK_SOURCE_SYSCLK             ( 0U )
#define RCC_SDMMC_CLK_SOURCE_PLL2PCLK           ( 1U )
#define RCC_SDMMC_CLK_SOURCE_MAX                ( 2U )

/**
  * @}
  */

/** @defgroup RCC_SDMMC_SCLK_Source
  * @{
  */

#define RCC_SDMMC_SCLK_SOURCE_SDMMC_CLK_DELAY       ( 0U )
#define RCC_SDMMC_SCLK_SOURCE_SDMMC1_CKIN           ( 1U )
#define RCC_SDMMC_SCLK_SOURCE_SDMMC2_CKIN           ( 2U )
#define RCC_SDMMC_SCLK_SOURCE_SDMMC_CLK_NO_DELAY    ( 3U )
#define RCC_SDMMC_SCLK_SOURCE_MAX                   ( 4U )

/**
  * @}
  */

/** @defgroup RCC_RTC_CLK_Source
  * @{
  */

#define RCC_RTC_CLK_SOURCE_RCL                  ( 0U )
#define RCC_RTC_CLK_SOURCE_XTL                  ( 1U )
#define RCC_RTC_CLK_SOURCE_MAX                  ( 2U )

/**
  * @}
  */

/** @defgroup RCC_MCO_Output
  * @{
  */

#define RCC_MCO_HCLK                            ( 0U )
#define RCC_MCO_RCH                             ( 1U )
#define RCC_MCO_RCL                             ( 2U )
#define RCC_MCO_XTH                             ( 3U )
#define RCC_MCO_XTL                             ( 4U )
#define RCC_MCO_PLLPCLK                         ( 5U )
#define RCC_MCO_PLLQCLK                         ( 6U )
//#define RSV                                   ( 7U )
//#define RSV                                   ( 8U )
#define RCC_MCO_LPUART2_CLK                     ( 9U )
#define RCC_MCO_SYSCLK                          ( 10U )
#define RCC_MCO_LPUART1_CLK                     ( 11U )
#define RCC_MCO_FCLK_DIV8                       ( 12U )
#define RCC_MCO_USB_SOF                         ( 13U )
#define RCC_MCO_USB_CLK_48M                     ( 14U )
#define RCC_MCO_RTC_PCLK                        ( 15U )
#define RCC_MCO_SDMMC_SAMPLE_CLK                ( 16U )
#define RCC_MCO_SDMMC_DRIVE_CLK                 ( 17U )
#define RCC_MCO_MAX                             ( 18U )

/**
  * @}
  */

/** @defgroup  RCC_Reset_Source RCC Reset Source
  * @{
  */

#define RCC_RESET_SOURCE_PWR                    ( RCC_RSR_PWRRSTF )
#define RCC_RESET_SOURCE_POR12                  ( RCC_RSR_POR12RSTF )
#define RCC_RESET_SOURCE_SRST                   ( RCC_RSR_SRSTF )
#define RCC_RESET_SOURCE_RSTN                   ( RCC_RSR_RSTNF )
#define RCC_RESET_SOURCE_SYSREQ                 ( RCC_RSR_SYSREQRSTF )
#define RCC_RESET_SOURCE_LOCKUP                 ( RCC_RSR_LOCKUPRSTF )
#define RCC_RESET_SOURCE_IWDT                   ( RCC_RSR_IWDTRSTF )
#define RCC_RESET_SOURCE_WDT                    ( RCC_RSR_WDTRSTF )
#define RCC_RESET_SOURCE_LVD                    ( RCC_RSR_LVDRSTF )

/**
  * @}
  */
  
/** @brief  RCC_Reset_Source RCC Reset Source
  */

#define RCC_RESET_SOURCE_MASK                   ( RCC_RESET_SOURCE_PWR | RCC_RESET_SOURCE_POR12 | RCC_RESET_SOURCE_SRST | \
                                                  RCC_RESET_SOURCE_RSTN | RCC_RESET_SOURCE_SYSREQ | RCC_RESET_SOURCE_LOCKUP | \
                                                  RCC_RESET_SOURCE_IWDT | RCC_RESET_SOURCE_WDT | RCC_RESET_SOURCE_LVD)

/** @brief  RCC clock timeout
  */

#define RCC_RCH_READY_TIMEOUT                   ( 0xFFFFU )
#define RCC_RCH_UNREADY_TIMEOUT                 ( 0xFFFFU )
#define RCC_RCHP_READY_TIMEOUT                  ( 0xFFFFU )
#define RCC_RCHP_UNREADY_TIMEOUT                ( 0xFFFFU )
#define RCC_RCL_READY_TIMEOUT                   ( 0xFFFFU )
#define RCC_RCL_UNREADY_TIMEOUT                 ( 0xFFFFU )
#define RCC_XTH_READY_TIMEOUT                   ( 0xFFFFFU )
#define RCC_XTH_UNREADY_TIMEOUT                 ( 0xFFFFFU )
#define RCC_XTL_READY_TIMEOUT                   ( 0xFFFFFFU )
#define RCC_XTL_UNREADY_TIMEOUT                 ( 0xFFFFU )
#define RCC_PLL1_READY_TIMEOUT                  ( 0xFFFFU )
#define RCC_PLL2_READY_TIMEOUT                  ( 0xFFFFU )
#define RCC_PLL3_READY_TIMEOUT                  ( 0xFFFFU )

/* Exported macros -----------------------------------------------------------*/

#define __HAL_RCC_RTC_ENABLE()                  ( RCC->STDBYCTRL |= RCC_STDBYCTRL_RTCEN )
#define __HAL_RCC_RTC_DISABLE()                 ( RCC->STDBYCTRL &= ~RCC_STDBYCTRL_RTCEN )

#define __HAL_RCC_XTH_STOP_ENABLE()             ( RCC->XTHCR |= RCC_XTHCR_XTHSDEN )
#define __HAL_RCC_XTH_STOP_DISABLE()            ( RCC->XTHCR &= ~RCC_XTHCR_XTHSDEN )

#define __HAL_RCC_XTL_STOP_ENABLE()             ( RCC->STDBYCTRL |= RCC_STDBYCTRL_XTLSDEN )
#define __HAL_RCC_XTL_STOP_DISABLE()            ( RCC->STDBYCTRL &= ~RCC_STDBYCTRL_XTLSDEN )

#define RCC_PLL_DELAY()                         do                                                  \
                                                {                                                   \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                    __NOP(); __NOP(); __NOP(); __NOP(); __NOP();    \
                                                }while (0)
                                             
#define __HAL_RCC_FDCAN1_RESET()                do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_FDCAN1RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_FDCAN1RST;    \
                                                }while (0)

#define __HAL_RCC_FDCAN2_RESET()                do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_FDCAN2RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_FDCAN2RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_USB1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_USB1CRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_USB1CRST;     \
                                                }while (0)
                                                                                               
#define __HAL_RCC_SPI4_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_SPI4RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_SPI4RST;      \
                                                }while (0)
                                                
#define __HAL_RCC_SPI3_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_SPI3RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_SPI3RST;      \
                                                }while (0)
                                                
#define __HAL_RCC_SPI2_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_SPI2RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_SPI2RST;      \
                                                }while (0)
                                                
#define __HAL_RCC_SPI1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_SPI1RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_SPI1RST;      \
                                                }while (0)
                                                
#define __HAL_RCC_ETH_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_ETHRST;      \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_ETHRST;       \
                                                }while (0)
                                                
#define __HAL_RCC_CRC_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_CRCRST;      \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_CRCRST;       \
                                                }while (0)
                                                
#define __HAL_RCC_DMA2_RESET()                 do                                               \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_DMA2RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_DMA2RST;      \
                                                }while (0)
                                                
#define __HAL_RCC_DMA1_RESET()                 do                                               \
                                                {                                               \
                                                    RCC->AHB1RSTR &= ~RCC_AHB1RSTR_DMA1RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB1RSTR |= RCC_AHB1RSTR_DMA1RST;      \
                                                }while (0)
                                                
/** @brief  the AHB2 peripheral reset.
  */
                                                                                                                 
#define __HAL_RCC_UAC_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_UACRST;      \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_UACRST;       \
                                                }while (0)
                                                                                               
#define __HAL_RCC_DAC1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_DAC1RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_DAC1RST;      \
                                                }while (0)
                                                                                               
#define __HAL_RCC_ADC_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_ADCRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_ADCRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOH_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOHRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOHRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOG_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOGRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOGRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOF_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOFRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOFRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOE_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOERST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOERST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOD_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIODRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIODRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOC_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOCRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOCRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOB_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOBRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOBRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_GPIOA_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB2RSTR &= ~RCC_AHB2RSTR_GPIOARST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB2RSTR |= RCC_AHB2RSTR_GPIOARST;     \
                                                }while (0)
             
/** @brief  the AHB3 peripheral reset.
  */
              
                        
#define __HAL_RCC_OSPI2_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_OSPI2RST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB3RSTR |= RCC_AHB3RSTR_OSPI2RST;     \
                                                }while (0)
                                                
#define __HAL_RCC_OSPI1_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_OSPI1RST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB3RSTR |= RCC_AHB3RSTR_OSPI1RST;     \
                                                }while (0)
                                                
#define __HAL_RCC_SDIO_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SDMMCRST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB3RSTR |= RCC_AHB3RSTR_SDMMCRST;      \
                                                }while (0)
                                              
#define __HAL_RCC_SPI7_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->AHB3RSTR &= ~RCC_AHB3RSTR_SPI7RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->AHB3RSTR |= RCC_AHB3RSTR_SPI7RST;      \
                                                }while (0)
                                                
/** @brief  the APB1 peripheral reset.
  */
                        
#define __HAL_RCC_LPUART1_RESET()               do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_LPUART1RST;\
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_LPUART1RST; \
                                                }while (0)
                                                
#define __HAL_RCC_LPTIM1_RESET()                do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_LPTIM1RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_LPTIM1RST;  \
                                                }while (0)

#define __HAL_RCC_LPUART2_RESET()               do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_LPUART2RST;\
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_LPUART2RST; \
                                                }while (0)
                                                
#define __HAL_RCC_PMU_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_PMURST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_PMURST;     \
                                                }while (0)
                                                                                              
#define __HAL_RCC_I2C2_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C2RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C2RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_I2C1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2C1RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2C1RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_USART5_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_USART5RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_USART5RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_USART4_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_USART4RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_USART4RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_USART3_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_USART3RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_USART3RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_USART2_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_USART2RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_USART2RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_I2S2_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2S2RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2S2RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_I2S1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_I2S1RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_I2S1RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_WDT_RESET()                   do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_WDTRST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_WDTRST;     \
                                                }while (0)
                                                
#define __HAL_RCC_TIM7_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM7RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM7RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_TIM6_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM6RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM6RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_TIM5_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM5RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM5RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_TIM4_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM4RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM4RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_TIM3_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM3RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM3RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_TIM2_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR1 &= ~RCC_APB1RSTR1_TIM2RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR1 |= RCC_APB1RSTR1_TIM2RST;    \
                                                }while (0)
                                                
#define __HAL_RCC_EFUSE1_RESET()                do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR2 &= ~RCC_APB1RSTR2_EFUSE1RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR2 |= RCC_APB1RSTR2_EFUSE1RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_TIM26_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->APB1RSTR2 &= ~RCC_APB1RSTR2_TIM26RST;  \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR2 |= RCC_APB1RSTR2_TIM26RST;   \
                                                }while (0)
                     
#define __HAL_RCC_USART8_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR2 &= ~RCC_APB1RSTR2_USART8RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR2 |= RCC_APB1RSTR2_USART8RST;  \
                                                }while (0)
                                                
#define __HAL_RCC_USART7_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB1RSTR2 &= ~RCC_APB1RSTR2_USART7RST; \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB1RSTR2 |= RCC_APB1RSTR2_USART7RST;  \
                                                }while (0)
                                                
/** @brief  the APB2 peripheral reset.
  */

#define __HAL_RCC_TIM10_RESET()                 do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM10RST;    \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_TIM10RST;     \
                                                }while (0)

#define __HAL_RCC_TIM9_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM9RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_TIM9RST;      \
                                                }while (0)

#define __HAL_RCC_USART6_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART6RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_USART6RST;    \
                                                }while (0)

#define __HAL_RCC_USART1_RESET()                 do                                             \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_USART1RST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_USART1RST;    \
                                                }while (0)

#define __HAL_RCC_TIM8_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM8RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_TIM8RST;      \
                                                }while (0)

#define __HAL_RCC_TIM1_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_TIM1RST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_TIM1RST;      \
                                                }while (0)

#define __HAL_RCC_EXTI_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_EXTIRST;     \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_EXTIRST;      \
                                                }while (0)

#define __HAL_RCC_COMP_RESET()                  do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_COMPRST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_COMPRST;    \
                                                }while (0)

#define __HAL_RCC_SYSCFG_RESET()                do                                              \
                                                {                                               \
                                                    RCC->APB2RSTR &= ~RCC_APB2RSTR_SYSCFGRST;   \
                                                    __NOP();__NOP();__NOP();                    \
                                                    RCC->APB2RSTR |= RCC_APB2RSTR_SYSCFGRST;    \
                                                }while (0)

                                                
/** @brief  Enable or Disable the AHB1 peripheral clock.
  */

#define __HAL_RCC_SRAM1_CLK_ENABLE()            ( RCC->AHB1CKENR |= RCC_AHB1CKENR_SRAM1CKEN )
#define __HAL_RCC_SRAM1_CLK_DISABLE()           ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_SRAM1CKEN )
#define __HAL_RCC_ROM_CLK_ENABLE()              ( RCC->AHB1CKENR |= RCC_AHB1CKENR_ROMCKEN )
#define __HAL_RCC_ROM_CLK_DISABLE()             ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_ROMCKEN )
#define __HAL_RCC_BKPSRAM_CLK_ENABLE()          ( RCC->AHB1CKENR |= RCC_AHB1CKENR_BKPSRAMCKEN )
#define __HAL_RCC_BKPSRAM_CLK_DISABLE()         ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_BKPSRAMCKEN )
#define __HAL_RCC_FDCAN1_CLK_ENABLE()           ( RCC->AHB1CKENR |= RCC_AHB1CKENR_FDCAN1CKEN )
#define __HAL_RCC_FDCAN1_CLK_DISABLE()          ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_FDCAN1CKEN )
#define __HAL_RCC_FDCAN2_CLK_ENABLE()           ( RCC->AHB1CKENR |= RCC_AHB1CKENR_FDCAN2CKEN )
#define __HAL_RCC_FDCAN2_CLK_DISABLE()          ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_FDCAN2CKEN )                                                
#define __HAL_RCC_USB1_CLK_ENABLE()             ( RCC->AHB1CKENR |= RCC_AHB1CKENR_USB1CCKEN )
#define __HAL_RCC_USB1_CLK_DISABLE()            ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_USB1CCKEN )
#define __HAL_RCC_SPI4_CLK_ENABLE()             ( RCC->AHB1CKENR |= RCC_AHB1CKENR_SPI4CKEN )
#define __HAL_RCC_SPI4_CLK_DISABLE()            ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_SPI4CKEN )
#define __HAL_RCC_SPI3_CLK_ENABLE()             ( RCC->AHB1CKENR |= RCC_AHB1CKENR_SPI3CKEN )
#define __HAL_RCC_SPI3_CLK_DISABLE()            ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_SPI3CKEN )
#define __HAL_RCC_SPI2_CLK_ENABLE()             ( RCC->AHB1CKENR |= RCC_AHB1CKENR_SPI2CKEN )
#define __HAL_RCC_SPI2_CLK_DISABLE()            ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_SPI2CKEN )
#define __HAL_RCC_SPI1_CLK_ENABLE()             ( RCC->AHB1CKENR |= RCC_AHB1CKENR_SPI1CKEN )
#define __HAL_RCC_SPI1_CLK_DISABLE()            ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_SPI1CKEN )
#define __HAL_RCC_ETHRX_CLK_ENABLE()            ( RCC->AHB1CKENR |= RCC_AHB1CKENR_ETHRXCKEN )
#define __HAL_RCC_ETHRX_CLK_DISABLE()           ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_ETHRXCKEN )
#define __HAL_RCC_ETHTX_CLK_ENABLE()            ( RCC->AHB1CKENR |= RCC_AHB1CKENR_ETHTXCKEN )
#define __HAL_RCC_ETHTX_CLK_DISABLE()           ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_ETHTXCKEN )
#define __HAL_RCC_ETHMAC_CLK_ENABLE()           ( RCC->AHB1CKENR |= RCC_AHB1CKENR_ETHMACCKEN )
#define __HAL_RCC_ETHMAC_CLK_DISABLE()          ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_ETHMACCKEN )
#define __HAL_RCC_CRC_CLK_ENABLE()              ( RCC->AHB1CKENR |= RCC_AHB1CKENR_CRCCKEN )
#define __HAL_RCC_CRC_CLK_DISABLE()             ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_CRCCKEN )
#define __HAL_RCC_DMA2_CLK_ENABLE()            ( RCC->AHB1CKENR |= RCC_AHB1CKENR_DMA2CKEN )
#define __HAL_RCC_DMA2_CLK_DISABLE()           ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_DMA2CKEN )
#define __HAL_RCC_DMA1_CLK_ENABLE()            ( RCC->AHB1CKENR |= RCC_AHB1CKENR_DMA1CKEN )
#define __HAL_RCC_DMA1_CLK_DISABLE()           ( RCC->AHB1CKENR &= ~RCC_AHB1CKENR_DMA1CKEN )

/** @brief  Enable or Disable the AHB2 peripheral clock.
  */
  
#define __HAL_RCC_HRNG_CLK_ENABLE()             ( RCC->AHB2CKENR |= RCC_AHB2CKENR_HRNGCKEN )
#define __HAL_RCC_HRNG_CLK_DISABLE()            ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_HRNGCKEN )
#define __HAL_RCC_AES_SPI1_CLK_ENABLE()         ( RCC->AHB2CKENR |= RCC_AHB2CKENR_AESSPI1CKEN )
#define __HAL_RCC_AES_SPI1_CLK_DISABLE()        ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_AESSPI1CKEN )
#define __HAL_RCC_DAC1_CLK_ENABLE()             ( RCC->AHB2CKENR |= RCC_AHB2CKENR_DAC1CKEN )
#define __HAL_RCC_DAC1_CLK_DISABLE()            ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_DAC1CKEN )
#define __HAL_RCC_ADC_CLK_ENABLE()              ( RCC->AHB2CKENR |= RCC_AHB2CKENR_ADCCKEN )
#define __HAL_RCC_ADC_CLK_DISABLE()             ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_ADCCKEN )
#define __HAL_RCC_GPIOH_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOHCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOH_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOHCKEN )
#define __HAL_RCC_GPIOG_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOGCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOG_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOGCKEN )
#define __HAL_RCC_GPIOF_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOFCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOF_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOFCKEN )
#define __HAL_RCC_GPIOE_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOECKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOE_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOECKEN )
#define __HAL_RCC_GPIOD_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIODCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOD_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIODCKEN )
#define __HAL_RCC_GPIOC_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOCCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOC_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOCCKEN )
#define __HAL_RCC_GPIOB_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOBCKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOB_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOBCKEN )
#define __HAL_RCC_GPIOA_CLK_ENABLE()            do                                              \
                                                {                                               \
                                                    RCC->AHB2CKENR |= RCC_AHB2CKENR_GPIOACKEN;  \
                                                    RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN;   \
                                                } while (0)
#define __HAL_RCC_GPIOA_CLK_DISABLE()           ( RCC->AHB2CKENR &= ~RCC_AHB2CKENR_GPIOACKEN )
        
/** @brief  Enable or Disable the AHB3 peripheral clock.
  */
              
#define __HAL_RCC_OSPI2_CLK_ENABLE()            ( RCC->AHB3CKENR |= RCC_AHB3CKENR_OSPI2CKEN )
#define __HAL_RCC_OSPI2_CLK_DISABLE()           ( RCC->AHB3CKENR &= ~RCC_AHB3CKENR_OSPI2CKEN )
#define __HAL_RCC_OSPI1_CLK_ENABLE()            ( RCC->AHB3CKENR |= RCC_AHB3CKENR_OSPI1CKEN )
#define __HAL_RCC_OSPI1_CLK_DISABLE()           ( RCC->AHB3CKENR &= ~RCC_AHB3CKENR_OSPI1CKEN )
#define __HAL_RCC_SDMMC_CLK_ENABLE()            ( RCC->AHB3CKENR |= RCC_AHB3CKENR_SDMMCCKEN )
#define __HAL_RCC_SDMMC_CLK_DISABLE()           ( RCC->AHB3CKENR &= ~RCC_AHB3CKENR_SDMMCCKEN )
#define __HAL_RCC_SPI7_CLK_ENABLE()             ( RCC->AHB3CKENR |= RCC_AHB3CKENR_SPI7CKEN)
#define __HAL_RCC_SPI7_CLK_DISABLE()            ( RCC->AHB3CKENR &= ~RCC_AHB3CKENR_SPI7CKEN)

/** @brief  Enable or Disable the APB1 peripheral clock.
  */

#define __HAL_RCC_LPUART1_CLK_ENABLE()          ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_LPUART1CKEN )
#define __HAL_RCC_LPUART1_CLK_DISABLE()         ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_LPUART1CKEN )
#define __HAL_RCC_LPTIM1_CLK_ENABLE()           ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_LPTIM1CKEN )
#define __HAL_RCC_LPTIM1_CLK_DISABLE()          ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_LPTIM1CKEN )
#define __HAL_RCC_LPUART2_CLK_ENABLE()          ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_LPUART2CKEN )
#define __HAL_RCC_LPUART2_CLK_DISABLE()         ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_LPUART2CKEN )
#define __HAL_RCC_PMU_CLK_ENABLE()              ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_PMUCKEN )
#define __HAL_RCC_PMU_CLK_DISABLE()             ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_PMUCKEN )
#define __HAL_RCC_I2C2_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_I2C2CKEN )
#define __HAL_RCC_I2C2_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_I2C2CKEN )
#define __HAL_RCC_I2C1_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_I2C1CKEN )
#define __HAL_RCC_I2C1_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_I2C1CKEN )
#define __HAL_RCC_USART5_CLK_ENABLE()           ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_USART5CKEN )
#define __HAL_RCC_USART5_CLK_DISABLE()          ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_USART5CKEN )
#define __HAL_RCC_USART4_CLK_ENABLE()           ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_USART4CKEN )
#define __HAL_RCC_USART4_CLK_DISABLE()          ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_USART4CKEN )
#define __HAL_RCC_USART3_CLK_ENABLE()           ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_USART3CKEN )
#define __HAL_RCC_USART3_CLK_DISABLE()          ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_USART3CKEN )
#define __HAL_RCC_USART2_CLK_ENABLE()           ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_USART2CKEN )
#define __HAL_RCC_USART2_CLK_DISABLE()          ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_USART2CKEN )
#define __HAL_RCC_I2S2_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_I2S2CKEN )
#define __HAL_RCC_I2S2_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_I2S2CKEN )
#define __HAL_RCC_I2S1_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_I2S1CKEN )
#define __HAL_RCC_I2S1_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_I2S1CKEN )
#define __HAL_RCC_WDT_CLK_ENABLE()              ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_WDTCKEN )
#define __HAL_RCC_WDT_CLK_DISABLE()             ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_WDTCKEN )
#define __HAL_RCC_RTC_CLK_ENABLE()              ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_RTCCKEN )
#define __HAL_RCC_RTC_CLK_DISABLE()             ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_RTCCKEN )
#define __HAL_RCC_TIM7_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM7CKEN )
#define __HAL_RCC_TIM7_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM7CKEN )
#define __HAL_RCC_TIM6_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM6CKEN )
#define __HAL_RCC_TIM6_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM6CKEN )
#define __HAL_RCC_TIM5_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM5CKEN )
#define __HAL_RCC_TIM5_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM5CKEN )
#define __HAL_RCC_TIM4_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM4CKEN )
#define __HAL_RCC_TIM4_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM4CKEN )
#define __HAL_RCC_TIM3_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM3CKEN )
#define __HAL_RCC_TIM3_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM3CKEN )
#define __HAL_RCC_TIM2_CLK_ENABLE()             ( RCC->APB1CKENR1 |= RCC_APB1CKENR1_TIM2CKEN )
#define __HAL_RCC_TIM2_CLK_DISABLE()            ( RCC->APB1CKENR1 &= ~RCC_APB1CKENR1_TIM2CKEN )
#define __HAL_RCC_EFUSE1_CLK_ENABLE()           ( RCC->APB1CKENR2 |= RCC_APB1CKENR2_EFUSE1CKEN )
#define __HAL_RCC_EFUSE1_CLK_DISABLE()          ( RCC->APB1CKENR2 &= ~RCC_APB1CKENR2_EFUSE1CKEN )
#define __HAL_RCC_TIM26_CLK_ENABLE()            ( RCC->APB1CKENR2 |= RCC_APB1CKENR2_TIM26CKEN )
#define __HAL_RCC_TIM26_CLK_DISABLE()           ( RCC->APB1CKENR2 &= ~RCC_APB1CKENR2_TIM26CKEN )
#define __HAL_RCC_USART8_CLK_ENABLE()           ( RCC->APB1CKENR2 |= RCC_APB1CKENR2_USART8CKEN )
#define __HAL_RCC_USART8_CLK_DISABLE()          ( RCC->APB1CKENR2 &= ~RCC_APB1CKENR2_USART8CKEN )
#define __HAL_RCC_USART7_CLK_ENABLE()           ( RCC->APB1CKENR2 |= RCC_APB1CKENR2_USART7CKEN )
#define __HAL_RCC_USART7_CLK_DISABLE()          ( RCC->APB1CKENR2 &= ~RCC_APB1CKENR2_USART7CKEN )

/** @brief  Enable or Disable the APB2 peripheral clock.
  */

#define __HAL_RCC_TIM10_CLK_ENABLE()            ( RCC->APB2CKENR |= RCC_APB2CKENR_TIM10CKEN )
#define __HAL_RCC_TIM10_CLK_DISABLE()           ( RCC->APB2CKENR &= ~RCC_APB2CKENR_TIM10CKEN )
#define __HAL_RCC_TIM9_CLK_ENABLE()             ( RCC->APB2CKENR |= RCC_APB2CKENR_TIM9CKEN )
#define __HAL_RCC_TIM9_CLK_DISABLE()            ( RCC->APB2CKENR &= ~RCC_APB2CKENR_TIM9CKEN )
#define __HAL_RCC_USART6_CLK_ENABLE()           ( RCC->APB2CKENR |= RCC_APB2CKENR_USART6CKEN )
#define __HAL_RCC_USART6_CLK_DISABLE()          ( RCC->APB2CKENR &= ~RCC_APB2CKENR_USART6CKEN )
#define __HAL_RCC_USART1_CLK_ENABLE()           ( RCC->APB2CKENR |= RCC_APB2CKENR_USART1CKEN )
#define __HAL_RCC_USART1_CLK_DISABLE()          ( RCC->APB2CKENR &= ~RCC_APB2CKENR_USART1CKEN )
#define __HAL_RCC_TIM8_CLK_ENABLE()             ( RCC->APB2CKENR |= RCC_APB2CKENR_TIM8CKEN )
#define __HAL_RCC_TIM8_CLK_DISABLE()            ( RCC->APB2CKENR &= ~RCC_APB2CKENR_TIM8CKEN )
#define __HAL_RCC_TIM1_CLK_ENABLE()             ( RCC->APB2CKENR |= RCC_APB2CKENR_TIM1CKEN )
#define __HAL_RCC_TIM1_CLK_DISABLE()            ( RCC->APB2CKENR &= ~RCC_APB2CKENR_TIM1CKEN )
#define __HAL_RCC_EXTI_CLK_ENABLE()             ( RCC->APB2CKENR |= RCC_APB2CKENR_EXTICKEN )
#define __HAL_RCC_EXTI_CLK_DISABLE()            ( RCC->APB2CKENR &= ~RCC_APB2CKENR_EXTICKEN )
#define __HAL_RCC_COMP1_CLK_ENABLE()             ( RCC->APB2CKENR |= RCC_APB2CKENR_COMP1CKEN )
#define __HAL_RCC_COMP1_CLK_DISABLE()            ( RCC->APB2CKENR &= ~RCC_APB2CKENR_COMP1CKEN )
#define __HAL_RCC_SYSCFG_CLK_ENABLE()           ( RCC->APB2CKENR |= RCC_APB2CKENR_SYSCFGCKEN )
#define __HAL_RCC_SYSCFG_CLK_DISABLE()          ( RCC->APB2CKENR &= ~RCC_APB2CKENR_SYSCFGCKEN )


/******************************************************************************/

/** @defgroup  RCC Private Macros
  * @{
  */
  
#define IS_RCC_RESET_SOURCE(SOURCE)             ((((SOURCE) & RCC_RESET_SOURCE_MASK) != 0U) && \
                                                 (((SOURCE) & ~RCC_RESET_SOURCE_MASK) == 0U))

#define IS_RCC_OSC_TYPE(TYPE)                   ((((TYPE) & RCC_OSC_TYPE_MASK) != 0U) && \
                                                 (((TYPE) & ~RCC_OSC_TYPE_MASK) == 0U))

#define IS_RCC_OSC_XTL_DRIVE(DRIVE)             ((DRIVE) < RCC_XTL_DRIVE_MAX)

#define IS_RCC_IT(IT)                           ((((IT) & RCC_IT_MASK) != 0U) && \
                                                 (((IT) & ~RCC_IT_MASK) == 0U))

#define IS_RCC_IT_FLAG(FLAG)                    ((((FLAG) & RCC_IT_MASK) != 0U) && \
                                                 (((FLAG) & ~RCC_IT_MASK) == 0U))
                                                 
#define IS_RCC_PLL_CLOCK_SOURCE(SOURCE)         ((SOURCE) < RCC_PLL_SOURCE_MAX)

#define IS_RCC_PLL1_PLLQ(PLLQ)                  (((PLLQ) != 0) && ((PLLQ) <= 15))

#define IS_RCC_PLL1_PLLP(PLLP)                  (((PLLP) == 2U)   || \
                                                 ((PLLP) == 4U)   || \
                                                 ((PLLP) == 6U)   || \
                                                 ((PLLP) == 8U))

#define IS_RCC_PLL1_PLLN(PLLN)                  (((PLLN) != 0) && ((PLLN) <= 63U))

#define IS_RCC_PLL1_PLLF(PLLF)                  (((PLLF) >= 50U) && ((PLLF) <= 511U))

#define IS_RCC_CLOCK_TYPE(TYPE)                 ((((TYPE) & RCC_CLOCK_TYPE_MASK) != 0U) && \
                                                 (((TYPE) & ~RCC_CLOCK_TYPE_MASK) == 0U))

#define IS_RCC_SYSCLK_SOURCE(SOURCE)            ((SOURCE) < RCC_SYSCLK_SOURCE_MAX)

#define IS_RCC_SYSCLK_DIV(DIV)                  (((DIV) != 0) && ((DIV) <= 16U))

#define IS_RCC_SYSCLK_DIV(DIV)                  (((DIV) != 0) && ((DIV) <= 16U))

#define IS_RCC_PCLK_DIV(DIV)                    (((DIV) == 1)   || \
                                                 ((DIV) == 2)   || \
                                                 ((DIV) == 4)   || \
                                                 ((DIV) == 8)   || \
                                                 ((DIV) == 16))

#define IS_RCC_HRNGS_CLK_DIV(DIV)               (((DIV) != 0) && ((DIV) <= 128U))

#define IS_RCC_FLT_CLK_SOURCE(SOURCE)           ((SOURCE) < RCC_FLT_CLK_SOURCE_MAX)

#define IS_RCC_LPUART_CLK_SOURCE(SOURCE)       ((SOURCE) < RCC_LPUART_CLK_SOURCE_MAX)

#define IS_RCC_LPTIM1_CLK_SOURCE(SOURCE)        ((SOURCE) < RCC_LPTIM1_CLK_SOURCE_MAX)

#define IS_RCC_SDMMC_CLK_SOURCE(SOURCE)         ((SOURCE) < RCC_SDMMC_CLK_SOURCE_MAX)

#define IS_RCC_SDMMC_SCLK_SOURCE(SOURCE)         ((SOURCE) < RCC_SDMMC_SCLK_SOURCE_MAX)

#define IS_RCC_RTC_CLK_SOURCE(SOURCE)           ((SOURCE) < RCC_RTC_CLK_SOURCE_MAX)

#define IS_RCC_MCO(MCO)                         ((MCO) < RCC_MCO_MAX)

#define IS_RCC_MCO1_DIV(DIV)                    (((DIV) != 0) && ((DIV) <= 65536U))

#define IS_RCC_MCO2_DIV(DIV)                    (((DIV) != 0) && ((DIV) <= 64U))

#define IS_RCC_SSC_MODE(MODE)                   (((MODE) == RCC_PLL_SSC_MODE_CENTER) || \
                                                 ((MODE) == RCC_PLL_SSC_MODE_DOWN))

#define IS_RCC_SSC_PERIOD(PERIOD)               ((PERIOD) < 8192U)

#define IS_RCC_SSC_STEP(STEP)                   ((STEP) < 32768U)

                            
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/

void HAL_RCC_ReadyIRQHandler(void);
void HAL_RCC_XTHStopIRQHandler(void);
void HAL_RCC_XTLStopIRQHandler(void);
void HAL_RCC_RCHReadyCallback(void);
void HAL_RCC_RCLReadyCallback(void);
void HAL_RCC_XTHReadyCallback(void);
void HAL_RCC_XTLReadyCallback(void);
void HAL_RCC_PLL1ReadyCallback(void);
void HAL_RCC_XTHStopCallback(void);
void HAL_RCC_XTLStopCallback(void);

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef  *RCC_OscInit);

HAL_StatusTypeDef HAL_RCC_RCHConfig(uint32_t RCH, uint32_t Div16);
HAL_StatusTypeDef HAL_RCC_RCLConfig(uint32_t RCL);
HAL_StatusTypeDef HAL_RCC_XTHConfig(uint32_t XTH, uint32_t Bypass);
HAL_StatusTypeDef HAL_RCC_XTLConfig(uint32_t XTL, uint32_t Bypass);
HAL_StatusTypeDef HAL_RCC_PLL1Config(uint32_t PLL1, uint32_t ClockSource, \
                                     uint32_t PLLN, uint32_t PLLF, uint32_t PLLP, uint32_t PLLQ);
HAL_StatusTypeDef HAL_RCC_PLL1SSCConfig(uint32_t PLL1, uint32_t ClockSource, \
                                     uint32_t PLLN, uint32_t PLLF, uint32_t PLLP, uint32_t PLLQ, \
                                     uint32_t SSC, uint32_t Mode, uint32_t Period, uint32_t Step);
HAL_StatusTypeDef HAL_RCC_PLL1PCLKConfig(uint32_t PLL1PCLK);
HAL_StatusTypeDef HAL_RCC_PLL1QCLKConfig(uint32_t PLL1QCLK);

HAL_StatusTypeDef HAL_RCC_ITConfig(uint32_t IT, uint32_t NewStatus);
uint32_t HAL_RCC_GetITFlag(uint32_t IT);
HAL_StatusTypeDef HAL_RCC_ClearITFlag(uint32_t IT);

HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef  *RCC_ClkInit);
HAL_StatusTypeDef HAL_RCC_SYSCLKSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_SYSCLKDiv0Config(uint32_t Div);
HAL_StatusTypeDef HAL_RCC_SYSCLKDiv1Config(uint32_t Div);
HAL_StatusTypeDef HAL_RCC_PCLK1DivConfig(uint32_t Div);
HAL_StatusTypeDef HAL_RCC_PCLK2DivConfig(uint32_t Div);
HAL_StatusTypeDef HAL_RCC_HRNGSClockDivConfig(uint32_t Div);
HAL_StatusTypeDef HAL_RCC_FLTClockSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_LPUART1ClockSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_LPUART2ClockSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_LPTIM1ClockSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_SDMMCClockSourceConfig(uint32_t ClockSource);
HAL_StatusTypeDef HAL_RCC_SDMMCSampleClockSourceConfig(uint32_t SampleClockSource);
HAL_StatusTypeDef HAL_RCC_RTCClockSourceConfig(uint32_t ClockSource);

HAL_StatusTypeDef HAL_RCC_GetClock(RCC_ClkInitTypeDef  *RCC_ClkInit);
HAL_StatusTypeDef HAL_RCC_GetSYSCLKSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetHRNGSlowClockDiv(uint32_t *pClockDiv);
HAL_StatusTypeDef HAL_RCC_GetFLTClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetLPUART1ClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetLPUART2ClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetLPTIM1ClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetSDMMCClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetSDMMCSampleClockSource(uint32_t *pClockSource);
HAL_StatusTypeDef HAL_RCC_GetRTCClockSource(uint32_t *pClockSource);

uint32_t HAL_RCC_GetRCHTrimFreq(void);
uint32_t HAL_RCC_GetRCLTrimFreq(void);
uint32_t HAL_RCC_GetRCHFreq(void);
uint32_t HAL_RCC_GetRCLFreq(void);
uint32_t HAL_RCC_GetXTHFreq(void);
uint32_t HAL_RCC_GetXTLFreq(void);
uint32_t HAL_RCC_GetPLL1Freq(void);
uint32_t HAL_RCC_GetPLL1PCLKFreq(void);
uint32_t HAL_RCC_GetPLL1QCLKFreq(void);   
uint32_t HAL_RCC_GetSYSCLKFreq(void);
uint32_t HAL_RCC_GetSysCoreClockFreq(void);
uint32_t HAL_RCC_GetFCLKFreq(void);
uint32_t HAL_RCC_GetHCLKFreq(void);
uint32_t HAL_RCC_GetPCLK1Freq(void);
uint32_t HAL_RCC_GetPCLK2Freq(void);
uint32_t HAL_RCC_GetHRNGSClockFreq(void);
uint32_t HAL_RCC_GetFLTClockFreq(void);
uint32_t HAL_RCC_GetLPUART1ClockFreq(void);
uint32_t HAL_RCC_GetLPUART2ClockFreq(void);
uint32_t HAL_RCC_GetLPTIM1ClockFreq(void);
uint32_t HAL_RCC_GetSDMMCClockFreq(void);
uint32_t HAL_RCC_GetRTCClockFreq(void);

uint32_t HAL_RCC_MCOConfig(RCC_MCOInitTypeDef *MCO_InitStruct);
uint32_t HAL_RCC_MCO1Config(uint32_t MCO, uint32_t NewStatus, uint32_t Div);
uint32_t HAL_RCC_MCO2Config(uint32_t MCO, uint32_t NewStatus, uint32_t Div);

void HAL_RCC_SoftwareReset(void);
void HAL_RCC_StandbyReset(void);

void HAL_RCC_LockupResetConfig(uint32_t NewState);
void HAL_RCC_IWDTResetConfig(uint32_t NewState);
void HAL_RCC_WDTResetConfig(uint32_t NewState);
void HAL_RCC_LVDResetConfig(uint32_t NewState);

uint32_t HAL_RCC_GetResetSource(void);
void HAL_RCC_ClearAllResetSource(void);

void HAL_RCC_Enable_XTH_Failure_NMI(void);
void HAL_RCC_Disable_XTH_Failure_NMI(void);  

#endif
