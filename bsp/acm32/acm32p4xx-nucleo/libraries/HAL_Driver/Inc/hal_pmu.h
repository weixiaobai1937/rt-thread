/*
  ******************************************************************************
  * @file    HAL_PMU.h
  * @version V1.0.0
  * @date    2022
  * @brief   Header file of PMU HAL module.
  ******************************************************************************
*/

#ifndef __HAL_PMU_H__
#define __HAL_PMU_H__

#include "hal.h"







/**
  * @brief  IO wake-up source
  */
typedef enum
{
    PMU_WAKEUP1  = 0x01U,      //PA0 
    PMU_WAKEUP2  = 0x02U,      //PA2  
    PMU_WAKEUP3  = 0x04U,      //PE6  
    PMU_WAKEUP4  = 0x08U,      //PC13
    PMU_WAKEUP5  = 0x10U,      //PB5 
    PMU_WAKEUP6  = 0x20U,      //PC5
    PMU_WAKEUP7  = 0x40U,      //PB15
    PMU_WAKEUP8  = 0x80U,      //PF8   
}PMU_WakeUpIo_t;

/**
  * @brief  RTC wake-up source
  */
typedef enum
{
    STANDBY_WAKEUP_WUTIE  = RTC_IE_WUTIE,
    STANDBY_WAKEUP_TAMP2R = RTC_IE_STP2RIE,
    STANDBY_WAKEUP_TAMP2F = RTC_IE_STP2FIE,
    STANDBY_WAKEUP_TAMP1R = RTC_IE_STP1RIE,
    STANDBY_WAKEUP_TAMP1F = RTC_IE_STP1FIE,
    STANDBY_WAKEUP_32S    = RTC_IE_ADJ32_IE,
    STANDBY_WAKEUP_ALARM  = RTC_IE_ALM_IE,
    STANDBY_WAKEUP_1KHZ   = RTC_IE_1KHZ_IE,
    STANDBY_WAKEUP_256HZ  = RTC_IE_256HZ_IE,
    STANDBY_WAKEUP_64HZ   = RTC_IE_64HZ_IE ,
    STANDBY_WAKEUP_16HZ   = RTC_IE_16HZ_IE ,
    STANDBY_WAKEUP_8HZ    = RTC_IE_8HZ_IE ,
    STANDBY_WAKEUP_4HZ    = RTC_IE_4HZ_IE ,
    STANDBY_WAKEUP_2HZ    = RTC_IE_2HZ_IE ,   
    STANDBY_WAKEUP_SEC    = RTC_IE_SEC_IE ,
    STANDBY_WAKEUP_MIN    = RTC_IE_MIN_IE ,
    STANDBY_WAKEUP_HOUR   = RTC_IE_HOUR_IE ,
    STANDBY_WAKEUP_DATE   = RTC_IE_DATE_IE ,
}PMU_WakeUpRtc_t;  


/**
  * @brief  Wakeup Polarity Configuration
  */
typedef enum
{
    PMU_WAKEUP_HIGH  = 0U,      
    PMU_WAKEUP_LOW   = 1U,         
}PMU_WakeUpPolarity_t;


/**
  * @brief  Wakeup flag
  */
#define PMU_FLAG_WAKEUP1                        ( PMU_SR_WUPFX_0 )
#define PMU_FLAG_WAKEUP2                        ( PMU_SR_WUPFX_1 )
#define PMU_FLAG_WAKEUP3                        ( PMU_SR_WUPFX_2 )
#define PMU_FLAG_WAKEUP4                        ( PMU_SR_WUPFX_3 )
#define PMU_FLAG_WAKEUP5                        ( PMU_SR_WUPFX_4 )
#define PMU_FLAG_WAKEUP6                        ( PMU_SR_WUPFX_5 )
#define PMU_FLAG_WAKEUP7                        ( PMU_SR_WUPFX_6 )
#define PMU_FLAG_WAKEUP8                        ( PMU_SR_WUPFX_7 )
#define PMU_FLAG_STANDBY                        ( PMU_SR_SBF )
#define PMU_FLAG_RTC                            ( PMU_SR_RTCWUF )
#define PMU_FLAG_RSET                           ( PMU_SR_RSTWUF )
#define PMU_FLAG_IWDT                           ( PMU_SR_IWDTWUF )
#define PMU_FLAG_BOR                            ( PMU_SR_BORWUF )



/**
  * @brief  Wait for an interrupt or event
  */

#define WAIT_FOR_INT                            ( 0U )  
#define WAIT_FOR_EVENT                          ( 1U )

/**
  * @brief  Lowpower mode
  */
#define PMU_CTRL0_LPMS_STOP                     ( 0U )  
#define PMU_CTRL0_LPMS_STANDBY                  ( PMU_CTRL0_LPMS )  

/**
  * @brief  LVD Voltage
  */
#define PMU_LVD_1V71                            ( 0x0U << PMU_CTRL1_LVDSEL_Pos)
#define PMU_LVD_2V01                            ( 0x1U << PMU_CTRL1_LVDSEL_Pos)  
#define PMU_LVD_2V23                            ( 0x2U << PMU_CTRL1_LVDSEL_Pos)
#define PMU_LVD_2V43                            ( 0x3U << PMU_CTRL1_LVDSEL_Pos)
#define PMU_LVD_2V51                            ( 0x4U << PMU_CTRL1_LVDSEL_Pos)
#define PMU_LVD_2V73                            ( 0x5U << PMU_CTRL1_LVDSEL_Pos)  
#define PMU_LVD_2V80                            ( 0x6U << PMU_CTRL1_LVDSEL_Pos)
#define PMU_LVD_2V90                            ( 0x7U << PMU_CTRL1_LVDSEL_Pos)
#define IS_PMU_LVD_VOLTAGE(VOLTAGE)             (((VOLTAGE) == PMU_LVD_1V71)    || \
                                                ((VOLTAGE) == PMU_LVD_2V01)     || \
                                                ((VOLTAGE) == PMU_LVD_2V23)     || \
                                                ((VOLTAGE) == PMU_LVD_2V43)     || \
                                                ((VOLTAGE) == PMU_LVD_2V51)     || \
                                                ((VOLTAGE) == PMU_LVD_2V73)     || \
                                                ((VOLTAGE) == PMU_LVD_2V80)     || \
                                                ((VOLTAGE) == PMU_LVD_2V90))

/**
  * @brief  LVD Filter
  */
#define PMU_LVD_FILTER_1                        ( 0x0U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_2                        ( 0x1U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_4                        ( 0x2U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_16                       ( 0x3U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_64                       ( 0x4U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_256                      ( 0x5U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_1024                     ( 0x6U << PMU_CTRL1_FLTTIME_Pos)
#define PMU_LVD_FILTER_4095                     ( 0x7U << PMU_CTRL1_FLTTIME_Pos)
#define IS_PMU_LVD_FILTER(FILTER)               (((FILTER) == PMU_LVD_FILTER_1)     || \
                                                ((FILTER) == PMU_LVD_FILTER_2)      || \
                                                ((FILTER) == PMU_LVD_FILTER_4)      || \
                                                ((FILTER) == PMU_LVD_FILTER_16)     || \
                                                ((FILTER) == PMU_LVD_FILTER_64)     || \
                                                ((FILTER) == PMU_LVD_FILTER_256)    || \
                                                ((FILTER) == PMU_LVD_FILTER_1024)   || \
                                                ((FILTER) == PMU_LVD_FILTER_4095))

/**
  * @brief  LVD Filter State
  */
#define PMU_LVD_FILTER_DISABLE                  ( 0U )
#define PMU_LVD_FILTER_ENABLE                   ( PMU_CTRL1_LVDFLTEN )
#define IS_PMU_LVD_FILTER_STATE(STATE)          (((STATE) == PMU_LVD_FILTER_DISABLE) || \
                                                ((STATE) == PMU_LVD_FILTER_ENABLE))
  
/**
  * @brief  BOR Voltage
  */
#define PMU_BOR_2V0_2V1                         ( 0x0U << PMU_CTRL2_BORCFG_Pos)
#define PMU_BOR_2V2_2V3                         ( 0x1U << PMU_CTRL2_BORCFG_Pos)
#define PMU_BOR_2V49_2V61                       ( 0x2U << PMU_CTRL2_BORCFG_Pos)
#define PMU_BOR_2V77_2V90                       ( 0x3U << PMU_CTRL2_BORCFG_Pos)
#define IS_PMU_BOR_VOLTAGE(VOLTAGE)             (((VOLTAGE) == PMU_BOR_2V0_2V1)     || \
                                                ((VOLTAGE) == PMU_BOR_2V2_2V3)      || \
                                                ((VOLTAGE) == PMU_BOR_2V49_2V61)    || \
                                                ((VOLTAGE) == PMU_BOR_2V77_2V90))

/** @defgroup STANDBY mode wake-up waiting time
  * @{
  */                        
#define IS_PMU_STANDBY_WAKE_WAIT(WAIT)          ((WAIT) <= 0xFU)

                                                
/** @defgroup PMU STOP wakeup wait time 
  * @{
  */ 
#define IS_PMU_STOP_WAKE_WAIT(WAIT)             ((WAIT) <= 0xFFFU)


/** @defgroup STANDBY domain IO  
  * @{
  */
#define PMU_PIN_PC13                            ( 0U )
#define PMU_PIN_PC14                            ( 1U )
#define PMU_PIN_PC15                            ( 2U )
#define IS_PMU_PIN(PIN)                         (((PIN) == PMU_PIN_PC13)    || \
                                                ((PIN) == PMU_PIN_PC14)     || \
                                                ((PIN) == PMU_PIN_PC15))

/** @defgroup STANDBY domain IO function selection  
  * @{
  */
#define PMU_PIN_FUNCTION_GPIO                   ( 0U )
#define PMU_PIN_FUNCTION_PC14_VALUE             ( 1U )
#define PMU_PIN_FUNCTION_PC15_VALUE             ( 1U )
#define PMU_PIN_FUNCTION_PC13_RTC_SIGNAL        ( 1U )
#define PMU_PIN_FUNCTION_PC13L_RTC_TAMPER       ( 2U )
#define PMU_PIN_FUNCTION_PC13_VALUE             ( 3U )
#define IS_PMU_PIN_FUNCTION(__FUNC__)           (((__FUNC__) == PMU_PIN_FUNCTION_GPIO)              || \
                                                ((__FUNC__) == PMU_PIN_FUNCTION_PC14_VALUE)         || \
                                                ((__FUNC__) == PMU_PIN_FUNCTION_PC15_VALUE)         || \
                                                ((__FUNC__) == PMU_PIN_FUNCTION_PC13_RTC_SIGNAL)    || \
                                                ((__FUNC__) == PMU_PIN_FUNCTION_PC13L_RTC_TAMPER)   || \
                                                ((__FUNC__) == PMU_PIN_FUNCTION_PC13_VALUE))
/**
  * @}
  */

/** @defgroup STANDBY domain IO Pin Value 
  * @{
  */
#define PMU_PIN_VALUE_0                         ( 0U )
#define PMU_PIN_VALUE_1                         ( 1U )
#define IS_PMU_PIN_VALUE(VALUE)                 (((VALUE) == PMU_PIN_VALUE_0) || \
                                                ((VALUE) == PMU_PIN_VALUE_1))


/** @defgroup PMU domain IO Pin (PC13) output mode 
  * @{
  */
#define PMU_PIN_PC13_OD                         ( 0U )
#define PMU_PIN_PC13_PP                         ( 1U )
#define IS_PMU_PIN_PC13_OUTPUT(__MODE__)        (((__MODE__) == PMU_PIN_PC13_OD) || \
                                                ((__MODE__) == PMU_PIN_PC13_PP))
/**
  * @}
  */
  
  
/**
  * @brief  Clear all wake-up status
  */
#define ALL_WANKEUP_STATUS                      ( 0xFFFFFFFFU ) 

/** @defgroup PMU LDO18 trim 
  * @{
  */ 
#define IS_PMU_LDO18_TRIM(TRIM)                 ((TRIM) <= 0x7U)

/** @defgroup ANAOUT signal selection
  * @{
  */
#define PMU_ANAOUT_FLOATING                     ( 0U )
#define PMU_ANAOUT_VREFINT                      ( 1U )
#define PMU_ANAOUT_LPLDO12                      ( 2U )
#define PMU_ANAOUT_MLDO12                       ( 3U )
#define PMU_ANAOUT_VREF1V                       ( 4U )
#define PMU_ANAOUT_VDD25                        ( 5U )
#define PMU_ANAOUT_VRF_LP                       ( 6U )
#define PMU_ANAOUT_VBG_LP                       ( 7U )
#define IS_PMU_ANAOUT(__SEL__)                  (((__SEL__) == PMU_ANAOUT_FLOATING) || \
                                                ((__SEL__) == PMU_ANAOUT_VREFINT)   || \
                                                ((__SEL__) == PMU_ANAOUT_LPLDO12)   || \
                                                ((__SEL__) == PMU_ANAOUT_MLDO12)    || \
                                                ((__SEL__) == PMU_ANAOUT_VREF1V)    || \
                                                ((__SEL__) == PMU_ANAOUT_VDD25)     || \
                                                ((__SEL__) == PMU_ANAOUT_VRF_LP)    || \
                                                ((__SEL__) == PMU_ANAOUT_VBG_LP))
/**
  * @}
  */

/**
  * @brief  RTC module write enable or diasble
  */
#define __HAL_PMU_RTC_WRITE_ENABLE              (PMU->CTRL0 |= PMU_CTRL0_RTC_WE)
#define __HAL_PMU_RTC_WRITE_DISABLE             (PMU->CTRL0 &= ~PMU_CTRL0_RTC_WE)


/** @brief  PC13 function select 
  * @param  __FUNC__: PC13 function select.
  *         This parameter can be 0: GPIO£¬1£ºRTC Fout£¬2£ºRTC tamper 3£ºPC13 Value
  */
#define __HAL_PMU_PC13_SEL(__FUNC__)            (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC13SEL_Msk)) | \
                                                (__FUNC__ << PMU_IOSEL_PC13SEL_Pos))

/** @brief  PC14 function select 
  * @param  __FUNC__: PC14 function select.
  *         This parameter can be 0: GPIO£¬1£ºPC14 Value
  */
#define __HAL_PMU_PC14_SEL(__FUNC__)            (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC14SEL_Msk)) | \
                                                (__FUNC__ << PMU_IOSEL_PC14SEL_Pos))

/** @brief  PC15 function select 
  * @param  __FUNC__: PC15 function select.
  *         This parameter can be 0: GPIO£¬1£ºPC15 Value
  */
#define __HAL_PMU_PC15_SEL(__FUNC__)            (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC15SEL_Msk)) | \
                                                (__FUNC__ << PMU_IOSEL_PC15SEL_Pos))


/** @brief  PC13 Value set 
  * @param  __FUNC__: PC13 Value set.
  *         This parameter can be 0: set£¬1£ºclaer
  */
#define __HAL_PMU_PC13_VALUE(__VALUE__)         (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC13VALUE_Msk)) | \
                                                (__VALUE__ << PMU_IOSEL_PC13VALUE_Pos))

/** @brief  PC14 Value set 
  * @param  __FUNC__: PC14 Value set.
  *         This parameter can be 0: set£¬1£ºclaer
  */
#define __HAL_PMU_PC14_VALUE(__VALUE__)         (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC14VALUE_Msk)) | \
                                                (__VALUE__ << PMU_IOSEL_PC14VALUE_Pos))

/** @brief  PC15 Value set 
  * @param  __FUNC__: PC15 Value set.
  *         This parameter can be 0: set£¬1£ºclaer
  */
#define __HAL_PMU_PC15_VALUE(__VALUE__)         (PMU->IOSEL |= (PMU->IOSEL & ~(PMU_IOSEL_PC15VALUE_Msk)) | \
                                                (__VALUE__ << PMU_IOSEL_PC15VALUE_Pos))

void HAL_PMU_Init(void);

void HAL_PMU_EnterSleep(uint32_t mode);

void HAL_PMU_EnterStop(uint32_t mode);

void HAL_PMU_EnterStandbyMode(uint32_t mode);

void HAL_PMU_WakeupIOInit(PMU_WakeUpIo_t wakeup_io, PMU_WakeUpPolarity_t polarity);

void HAL_PMU_WakeupIODeInit(PMU_WakeUpIo_t wakeup_io);

void HAL_PMU_StandbyWakeupRTCConfig(PMU_WakeUpRtc_t wakeup_rtc);

void HAL_PMU_StandbyWakeupRTCRelease(PMU_WakeUpRtc_t wakeup_rtc);

bool HAL_PMU_CheckStandbyStatus(void);

bool HAL_PMU_CheckDeepSleepStatus(void);

uint32_t HAL_PMU_GetWakeupSource(void);

void HAL_PMU_ClearWakeupStatus(uint32_t status);

void HAL_PMU_LvdEnable(uint32_t voltage, uint32_t filter, uint32_t filter_en);

void HAL_PMU_LvdDisable(void);

void HAL_PMU_BorResetEnable(uint32_t voltage);

void HAL_PMU_BorResetDisable(void);

void HAL_PMU_BorIrqEnable(uint32_t voltage);

void HAL_PMU_BorIrqtDisable(void);

void HAL_PMU_StopWaitTime(uint32_t stopWaitTime);

void HAL_PMU_StandbypWaitTime(uint32_t standbyWaitTime);

void HAL_PMU_PMUDomainPinConfig(uint32_t PMU_Pin, uint32_t PMU_Func);

void HAL_PMU_SetPMUDomainPinValue(uint32_t PMU_Pin, uint32_t PMU_PinValue);

uint32_t HAL_PMU_GetPMUDomainPinValue(uint32_t PMU_Pin);

void HAL_PMU_PMUDomainPinPC13OutputType(uint32_t type);

void HAL_PMU_LDO18_Init(FunctionalState NewState, uint8_t trim);

void HAL_PMU_ANAOUT_Init(uint8_t sel);


#endif



