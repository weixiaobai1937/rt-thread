/******************************************************************************
*@file  : hal_adc.h
*@brief : Header file of ADC HAL module.
******************************************************************************/
#ifndef __HAL_ADC_H__
#define __HAL_ADC_H__


#include "hal.h"


/* ADC parameter address in EFUSE1 */
#define EFUSE1_ADC_1V2                      (0x33)  //16bit  
#define EFUSE1_ADC_VREFBUF_TRIM             (0x35)  //16bit
#define EFUSE1_ADC_STRIM                    (0x38)  //8bit 
#define EFUSE1_ADC_DTRIM                    (0x39)  //8bit
//#define EFUSE1_ADC_LDO_TRIM                 (0x6E)  //0x6E 8bit   



/** @defgroup ADC1
  * @{
  */   
#define IS_ADC_INSTANCE(__INSTANCE__)               ((__INSTANCE__) == ADC)
/**
  * @}
  */
  
/** @defgroup ADC ClockSource 
  * @{
  */ 
#define ADC_CLOCKSOURCE_HCLK                        (0U)
#define ADC_CLOCKSOURCE_ADC_KER_CLK_RC64M           (2U)
#define ADC_CLOCKSOURCE_ADC_KER_CLK_PLL1Q_CLK       (3U)
#define IS_ADC_CLOCKSOURCE(__SOURCE__)              (((__SOURCE__) == ADC_CLOCKSOURCE_HCLK)             || \
                                                    ((__SOURCE__) == ADC_CLOCKSOURCE_ADC_KER_CLK_RC64M) || \
                                                    ((__SOURCE__) == ADC_CLOCKSOURCE_ADC_KER_CLK_PLL1Q_CLK))
                                             
/**
  * @}
  */
  

/** @defgroup ADC Trig source define
  * @{
  */ 
/*
 *  |        Trig Source        |      Regular           |       Inject            | * 
 *  |     ADC_EXTERNAL_TIG0     |     TIM1_CC1           |      TIM1_TRGO          | *
 *  |     ADC_EXTERNAL_TIG1     |     TIM1_CC2           |      TIM1_CC4           | *
 *  |     ADC_EXTERNAL_TIG2     |     TIM1_CC3           |      TIM2_TRGO          | *
 *  |     ADC_EXTERNAL_TIG3     |     TIM2_CC2           |      TIM2_CC1           | *
 *  |     ADC_EXTERNAL_TIG4     |     TIM3_TRGO          |      TIM3_CC4           | *
 *  |     ADC_EXTERNAL_TIG5     |     TIM4_CC4           |      TIM4_TRGO          | *
 *  |     ADC_EXTERNAL_TIG6     |     EXTI11             |      EXTI15             | * 
 *  |     ADC_EXTERNAL_TIG7     |     TIM8_TRGO          |      TIM8_CC4           | *
 *  |     ADC_EXTERNAL_TIG8     |     TIM8_TRGO2         |      TIM1_TRGO2         | *
 *  |     ADC_EXTERNAL_TIG9     |     TIM1_TRGO          |      TIM8_TRGO          | *
 *  |     ADC_EXTERNAL_TIG10    |     TIM1_TRGO2         |      TIM8_TRGO2         | *
 *  |     ADC_EXTERNAL_TIG11    |     TIM2_TRGO          |      TIM3_CC3           | *
 *  |     ADC_EXTERNAL_TIG12    |     TIM4_TRGO          |      TIM3_TRGO          | *
 *  |     ADC_EXTERNAL_TIG13    |     TIM6_TRGO          |      TIM3_CC1           | * 
 *  |     ADC_EXTERNAL_TIG14    |     NULL               |      TIM6_TRGO          | *
 *  |     ADC_EXTERNAL_TIG15    |     TIM3_CC4           |      NULL               | *
 *  |     ADC_EXTERNAL_TIG16    |     NULL               |      NULL               | * 
 *  |     ADC_EXTERNAL_TIG17    |     NULL               |      NULL               | *
 *  |     ADC_EXTERNAL_TIG18    |     LPTIM1_OUT         |      LPTIM1_OUT         | *
 *  |     ADC_EXTERNAL_TIG19    |     NULL               |      NULL               | * 
 *  |     ADC_EXTERNAL_TIG20    |     NULL               |      NULL               | *
 */
#define ADC_EXTERNAL_TIG0                           (0U)
#define ADC_EXTERNAL_TIG1                           (1U)
#define ADC_EXTERNAL_TIG2                           (2U)
#define ADC_EXTERNAL_TIG3                           (3U)
#define ADC_EXTERNAL_TIG4                           (4U)
#define ADC_EXTERNAL_TIG5                           (5U)
#define ADC_EXTERNAL_TIG6                           (6U)
#define ADC_EXTERNAL_TIG7                           (7U)
#define ADC_EXTERNAL_TIG8                           (8U)
#define ADC_EXTERNAL_TIG9                           (9U)
#define ADC_EXTERNAL_TIG10                          (10U)
#define ADC_EXTERNAL_TIG11                          (11U)
#define ADC_EXTERNAL_TIG12                          (12U)
#define ADC_EXTERNAL_TIG13                          (13U)
#define ADC_EXTERNAL_TIG14                          (14U)
#define ADC_EXTERNAL_TIG15                          (15U)
#define ADC_EXTERNAL_TIG16                          (16U)
#define ADC_EXTERNAL_TIG17                          (17U)
#define ADC_EXTERNAL_TIG18                          (18U)
#define ADC_EXTERNAL_TIG19                          (19U)
#define ADC_EXTERNAL_TIG20                          (20U)
#define ADC_SOFTWARE_START                          (32U)
#define IS_ADC_EXT_TRIG(__TRIG__)                   (((__TRIG__) == ADC_EXTERNAL_TIG0)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG1)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG2)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG3)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG4)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG5)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG6)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG7)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG8)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG9)     || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG10)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG11)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG12)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG13)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG14)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG15)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG16)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG17)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG18)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG19)    || \
                                                    ((__TRIG__) == ADC_EXTERNAL_TIG20)    || \
                                                    ((__TRIG__) == ADC_SOFTWARE_START))  
                                                
/**
  * @}
  */                                                 
                                                
   
/** @defgroup ADC External Trigger Edge 
  * @{
  */ 
#define ADC_EXTERNAL_TRIGGER_NONE                   (0U)
#define ADC_EXTERNAL_TRIGGER_RISING                 (1U)
#define ADC_EXTERNAL_TRIGGER_FALLING                (2U)
#define ADC_EXTERNAL_TRIGGER_RISING_FALLING         (3U)
#define IS_ADC_EXT_TRIG_EDGE(__EDGE__)              (((__EDGE__) == ADC_EXTERNAL_TRIGGER_NONE)      || \
                                                    ((__EDGE__) == ADC_EXTERNAL_TRIGGER_RISING)     || \
                                                    ((__EDGE__) == ADC_EXTERNAL_TRIGGER_FALLING)    || \
                                                    ((__EDGE__) == ADC_EXTERNAL_TRIGGER_RISING_FALLING))
/**
  * @}
  */
  
/** @defgroup ADC Data Align
* @{
*/ 
#define ADC_DATAALIGN_RIGHT                         (0U)
#define ADC_DATAALIGN_LEFT                          ((uint32_t)ADC_CR2_ALIGN)
#define IS_ADC_DATA_ALIGN(__ALIGN__)                (((__ALIGN__) == ADC_DATAALIGN_RIGHT) || \
                                                    ((__ALIGN__) == ADC_DATAALIGN_LEFT))
/**
  * @}
  */ 


/** @defgroup ADC Continuous mode
* @{
*/ 
#define ADC_CONT_DISABLE                            (0U)
#define ADC_CONT_ENABLE                             (ADC_CR1_CONT)
#define IS_ADC_CONT(__CONT__)                       (((__CONT__) == ADC_CONT_DISABLE) || \
                                                    ((__CONT__) == ADC_CONT_ENABLE))
/**
  * @}
  */    
  
/** @defgroup ADC Resolution
  * @{
  */
#define ADC_RESOLUTION_12B                          (0U)   
#define ADC_RESOLUTION_10B                          (ADC_CR2_RES_0) 
#define ADC_RESOLUTION_8B                           (ADC_CR2_RES_1)   
#define ADC_RESOLUTION_6B                           (ADC_CR2_RES_0 | ADC_CR2_RES_1)   
#define IS_ADC_RESOLUTION(__RESOLUTION__)           (((__RESOLUTION__) == ADC_RESOLUTION_12B)   || \
                                                    ((__RESOLUTION__) == ADC_RESOLUTION_10B)    || \
                                                    ((__RESOLUTION__) == ADC_RESOLUTION_8B)     || \
                                                    ((__RESOLUTION__) == ADC_RESOLUTION_6B))
/**
  * @}
  */
 
 
/** @defgroup ADC ClockPrescale 
  * @{
  */ 
#define IS_ADC_CLOCKDIV(__CLOCKDIV__)               (((__CLOCKDIV__) >= 1U) && (__CLOCKDIV__) <= 64U)
                                             
/**
  * @}
  */
  

/** @defgroup ADC analog watchdog selection
  * @{
  */
#define ADC_ANALOGWATCHDOG_NONE                     (0U) 
#define ADC_ANALOGWATCHDOG_RCH_SINGLE               (ADC_CR1_AWDSGL | ADC_CR1_AWDEN)   
#define ADC_ANALOGWATCHDOG_JCH_SINGLE               (ADC_CR1_AWDSGL | ADC_CR1_JAWDEN)   
#define ADC_ANALOGWATCHDOG_RCH_OR_JCH_SINGLE        (ADC_CR1_AWDSGL | ADC_CR1_AWDEN | ADC_CR1_JAWDEN)   
#define ADC_ANALOGWATCHDOG_RCH_ALL                  (ADC_CR1_AWDEN)   
#define ADC_ANALOGWATCHDOG_JCH_ALL                  (ADC_CR1_JAWDEN)   
#define ADC_ANALOGWATCHDOG_RCH_AND_JCH_ALL          (ADC_CR1_AWDEN | ADC_CR1_JAWDEN)   
#define IS_ADC_ANALOG_WATCHDOG(__WATCHDOG__)        (((__WATCHDOG__) == ADC_ANALOGWATCHDOG_RCH_SINGLE)          || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_JCH_SINGLE)           || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_RCH_OR_JCH_SINGLE)    || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_RCH_ALL)              || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_JCH_ALL)              || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_RCH_AND_JCH_ALL)      || \
                                                    ((__WATCHDOG__) == ADC_ANALOGWATCHDOG_NONE))  
/**
  * @}
  */
  
  
/** @defgroup ADC analog watchdog thresholds 
  * @{
  */ 
#define IS_ADC_THRESHOLD(__THRESHOLD__)             ((__THRESHOLD__) <= 0xFFFU)
/**
  * @}
  */   
  
  
/** @defgroup ADC channel number 
* @{
*/
#define ADC_CHANNEL_0                               (0U)    
#define ADC_CHANNEL_1                               (1U)    
#define ADC_CHANNEL_2                               (2U)    
#define ADC_CHANNEL_3                               (3U)    
#define ADC_CHANNEL_4                               (4U)    
#define ADC_CHANNEL_5                               (5U)    
#define ADC_CHANNEL_6                               (6U)    
#define ADC_CHANNEL_7                               (7U)    
#define ADC_CHANNEL_8                               (8U)    
#define ADC_CHANNEL_9                               (9U)    
#define ADC_CHANNEL_10                              (10U)    
#define ADC_CHANNEL_11                              (11U)
#define ADC_CHANNEL_12                              (12U)
#define ADC_CHANNEL_13                              (13U)
#define ADC_CHANNEL_VREF                            (12U)
#define ADC_CHANNEL_VBAT                            (13U)      
#define ADC_CHANNEL_14                              (14U)
#define ADC_CHANNEL_15                              (15U)
#define ADC_CHANNEL_TS                              (15U)        
#define ADC_CHANNEL_16                              (16U) 
#define ADC_CHANNEL_17                              (17U)        
#define ADC_CHANNEL_18                              (18U)
#define ADC_CHANNEL_19                              (19U)                                                   

#define  ADC_CHANNEL_0_EN                           (1U << ADC_CHANNEL_0)    
#define  ADC_CHANNEL_1_EN                           (1U << ADC_CHANNEL_1)    
#define  ADC_CHANNEL_2_EN                           (1U << ADC_CHANNEL_2)    
#define  ADC_CHANNEL_3_EN                           (1U << ADC_CHANNEL_3)    
#define  ADC_CHANNEL_4_EN                           (1U << ADC_CHANNEL_4)    
#define  ADC_CHANNEL_5_EN                           (1U << ADC_CHANNEL_5)    
#define  ADC_CHANNEL_6_EN                           (1U << ADC_CHANNEL_6)    
#define  ADC_CHANNEL_7_EN                           (1U << ADC_CHANNEL_7)    
#define  ADC_CHANNEL_8_EN                           (1U << ADC_CHANNEL_8)    
#define  ADC_CHANNEL_9_EN                           (1U << ADC_CHANNEL_9)    
#define  ADC_CHANNEL_10_EN                          (1U << ADC_CHANNEL_10)    
#define  ADC_CHANNEL_11_EN                          (1U << ADC_CHANNEL_11) 
#define  ADC_CHANNEL_12_EN                          (1U << ADC_CHANNEL_12) 
#define  ADC_CHANNEL_13_EN                          (1U << ADC_CHANNEL_13)    
#define  ADC_CHANNEL_VREF_EN                        (1U << ADC_CHANNEL_VREF) 
#define  ADC_CHANNEL_VBAT_EN                        (1U << ADC_CHANNEL_VBAT)  
#define  ADC_CHANNEL_14_EN                          (1U << ADC_CHANNEL_14)
#define  ADC_CHANNEL_15_EN                          (1U << ADC_CHANNEL_15)      
#define  ADC_CHANNEL_TS_EN                          (1U << ADC_CHANNEL_TS)    
#define  ADC_CHANNEL_16_EN                          (1U << ADC_CHANNEL_16)    
#define  ADC_CHANNEL_17_EN                          (1U << ADC_CHANNEL_17)
#define  ADC_CHANNEL_18_EN                          (1U << ADC_CHANNEL_18)    
#define  ADC_CHANNEL_19_EN                          (1U << ADC_CHANNEL_19)   

     
#define IS_ADC_CHANNEL(CHANNEL)                     (((CHANNEL) == ADC_CHANNEL_0)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_1)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_2)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_3)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_4)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_5)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_6)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_7)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_8)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_9)    || \
                                                    ((CHANNEL) == ADC_CHANNEL_10)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_11)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_VREF) || \
                                                    ((CHANNEL) == ADC_CHANNEL_VBAT) || \
                                                    ((CHANNEL) == ADC_CHANNEL_14)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_TS)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_16)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_17)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_18)   || \
                                                    ((CHANNEL) == ADC_CHANNEL_19)   || \
                                                    ((CHANNEL) < 20))                                                                                                
/**
  * @}
  */ 


/** @defgroup Sampling time
  * @{
  */
#define ADC_SMP_CLOCK_3                             (0U)
#define ADC_SMP_CLOCK_5                             (1U)
#define ADC_SMP_CLOCK_7                             (2U)
#define ADC_SMP_CLOCK_10                            (3U)
#define ADC_SMP_CLOCK_13                            (4U)
#define ADC_SMP_CLOCK_16                            (5U)
#define ADC_SMP_CLOCK_20                            (6U)
#define ADC_SMP_CLOCK_30                            (7U)
#define ADC_SMP_CLOCK_60                            (8U)
#define ADC_SMP_CLOCK_80                            (9U)
#define ADC_SMP_CLOCK_100                           (10U)
#define ADC_SMP_CLOCK_120                           (11U)
#define ADC_SMP_CLOCK_160                           (12U)
#define ADC_SMP_CLOCK_260                           (13U)
#define ADC_SMP_CLOCK_320                           (14U)
#define ADC_SMP_CLOCK_640                           (15U)
#define IS_ADC_SMPCLOCK(__SMPCLOCK__)               (((__SMPCLOCK__) == ADC_SMP_CLOCK_3)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_5)     || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_7)     || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_10)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_13)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_16)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_20)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_30)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_60)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_80)    || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_100)   || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_120)   || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_160)   || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_260)   || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_320)   || \
                                                    ((__SMPCLOCK__) == ADC_SMP_CLOCK_640))   
/**
  * @}
  */ 
  
  
/** @defgroup ADC sequence number
* @{
*/
#define ADC_SEQUENCE_SQ1                            (1U)
#define ADC_SEQUENCE_SQ2                            (2U)
#define ADC_SEQUENCE_SQ3                            (3U)
#define ADC_SEQUENCE_SQ4                            (4U)
#define ADC_SEQUENCE_SQ5                            (5U)
#define ADC_SEQUENCE_SQ6                            (6U)
#define ADC_SEQUENCE_SQ7                            (7U)
#define ADC_SEQUENCE_SQ8                            (8U)
#define ADC_SEQUENCE_SQ9                            (9U)
#define ADC_SEQUENCE_SQ10                           (10U)
#define ADC_SEQUENCE_SQ11                           (11U)
#define ADC_SEQUENCE_SQ12                           (12U)
#define ADC_SEQUENCE_SQ13                           (13U)
#define ADC_SEQUENCE_SQ14                           (14U)
#define ADC_SEQUENCE_SQ15                           (15U)
#define ADC_SEQUENCE_SQ16                           (16U)
#define IS_ADC_SEQUENCE(__SEQUENCE__)               (((__SEQUENCE__) == ADC_SEQUENCE_SQ1)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ2)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ3)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ4)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ5)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ6)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ7)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ8)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ9)    || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ10)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ11)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ12)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ13)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ14)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ15)   || \
                                                    ((__SEQUENCE__) == ADC_SEQUENCE_SQ16))
/**
  * @}
  */   
  

/** @defgroup ADC regular length 
  * @{
  */ 
#define IS_ADC_REGULAR_LENGTH(__LENGTH__)           (((__LENGTH__) >= 1U) && ((__LENGTH__) <= 16U))
/**
  * @}
  */
  
  
/** @defgroup ADC regular discontinuous mode number 
  * @{
  */ 
#define IS_ADC_REGULAR_DISC_NUMBER(__NUMBER__)      ((__NUMBER__) <= 8U)
/**
  * @}
  */ 
  

/** @defgroup ADC injected rank 
  * @{
  */ 
#define ADC_INJECTED_RANK_1                         (1U)
#define ADC_INJECTED_RANK_2                         (2U)
#define ADC_INJECTED_RANK_3                         (3U)
#define ADC_INJECTED_RANK_4                         (4U)
#define IS_ADC_INJECTED_RANK(__RANK__)              (((__RANK__) >= 1U) && ((__RANK__) <= 4U))
/**
  * @}
  */ 

/** @defgroup ADC injected length 
  * @{
  */ 
#define IS_ADC_INJECTED_LENGTH(__LENGTH__)          (((__LENGTH__) >= 1U) && ((__LENGTH__) <= 4U))
/**
  * @}
  */  
  
/** @defgroup Offset Number 
  * @{
  */ 
#define ADC_OFR_1                                   (0U) 
#define ADC_OFR_2                                   (1U) 
#define ADC_OFR_3                                   (2U) 
#define ADC_OFR_4                                   (3U) 
#define ADC_OFR_NONE                                (4U) 
#define IS_ADC_OFFSET_NUMBER(__NUMBER__)            (((__NUMBER__) < 5U))
/**
  * @}
  */ 
  
  
/** @defgroup offset 
  * @{
  */ 
#define IS_ADC_OFFSET(__OFFSET__)                   ((__OFFSET__) <= 0xFFFU)
/**
  * @}
  */ 

 /** @defgroup Offset Calculate
  * @{
  */
#define ADC_OFFSET_MINUS                            (0U)  
#define ADC_OFFSET_PLUS                             (ADC_OFRX_OFFSETX_POS) 
#define IS_ADC_OFFSET_CALCULATE(__CALCULATE__)      (((__CALCULATE__) == ADC_OFFSET_MINUS) || \
                                                    ((__CALCULATE__) == ADC_OFFSET_PLUS))
/**
  * @}
  */
  

/** @defgroup Offset sign
  * @{
  */
#define ADC_OFFSET_SIGN_SIGNED                      (0U)                 
#define ADC_OFFSET_SIGN_UNSIGNED                    (ADC_OFRX_OFFSETX_SAT)          
#define IS_ADC_OFFSET_SIGN(__SIGN__)                (((__SIGN__) == ADC_OFFSET_SIGN_SIGNED) || \
                                                    ((__SIGN__) == ADC_OFFSET_SIGN_UNSIGNED))
/**
  * @}
  */ 
  

/** @defgroup JDR Number 
  * @{
  */ 
#define ADC_JDR_1                                   (0U) 
#define ADC_JDR_2                                   (1U) 
#define ADC_JDR_3                                   (2U) 
#define ADC_JDR_4                                   (3U) 
#define ADC_JDR_NONE                                (4U) 
#define IS_ADC_JDR_NUMBER(__NUMBER__)               ((__NUMBER__) < 5U)
/**
  * @}
  */ 
  

/** @defgroup  Oversampling Data shift
  * @{
  */
#define ADC_RIGHTBITSHIFT_NONE                      (0U) 
#define ADC_RIGHTBITSHIFT_1                         (1U) 
#define ADC_RIGHTBITSHIFT_2                         (2U) 
#define ADC_RIGHTBITSHIFT_3                         (3U) 
#define ADC_RIGHTBITSHIFT_4                         (4U) 
#define ADC_RIGHTBITSHIFT_5                         (5U) 
#define ADC_RIGHTBITSHIFT_6                         (6U) 
#define ADC_RIGHTBITSHIFT_7                         (7U) 
#define ADC_RIGHTBITSHIFT_8                         (8U) 
#define IS_ADC_RIGHTBITSHIFT(__SHIFT__)             (((__SHIFT__) == ADC_RIGHTBITSHIFT_NONE)    || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_1)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_2)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_3)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_4)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_5)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_6)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_7)        || \
                                                    ((__SHIFT__) == ADC_RIGHTBITSHIFT_8))
/**
  * @}
  */
  
  
/** @defgroup  Oversampling Ratio
  * @{
  */
#define ADC_OVERSAMPLING_RATIO_2                    (0U)  
#define ADC_OVERSAMPLING_RATIO_4                    (1U)  
#define ADC_OVERSAMPLING_RATIO_8                    (2U)  
#define ADC_OVERSAMPLING_RATIO_16                   (3U) 
#define ADC_OVERSAMPLING_RATIO_32                   (4U)  
#define ADC_OVERSAMPLING_RATIO_64                   (5U) 
#define ADC_OVERSAMPLING_RATIO_128                  (6U)  
#define ADC_OVERSAMPLING_RATIO_256                  (7U)  
#define IS_ADC_OVERSAMPLING_RATIO(__RATIO__)        (((__RATIO__) == ADC_OVERSAMPLING_RATIO_2)  || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_4)   || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_8)   || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_16)  || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_32)  || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_64)  || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_128) || \
                                                    ((__RATIO__) == ADC_OVERSAMPLING_RATIO_256))
/**
  * @}
  */  
  

/** @defgroup overflow mode
  * @{
  */
#define ADC_OVERFLOWMODE_LAST                       (0U)  
#define ADC_OVERFLOWMODE_NEW                        (ADC_CR2_OVRMOD)
#define IS_ADC_OVERFLOW_MODE(__MODE__)              (((__MODE__) == ADC_OVERFLOWMODE_LAST) || \
                                                    ((__MODE__) == ADC_OVERFLOWMODE_NEW))
/**
  * @}
  */
  
  
/** @defgroup oversampling triger mode
  * @{
  */
#define ADC_OVERSAMPLING_TRIGER_MORE                (0U)  
#define ADC_OVERSAMPLING_TRIGER_ONCE                (ADC_CR2_TROVS) 
#define IS_ADC_OVERSAMPLING_TRIGER(__MODE__)        (((__MODE__) == ADC_OVERSAMPLING_TRIGER_MORE) || \
                                                    ((__MODE__) == ADC_OVERSAMPLING_TRIGER_ONCE))
/**
  * @}
  */
  

/** @defgroup ADC interrupts and flag definition 
  * @{
  */ 
#define ADC_FLAG_ADRDY                              (ADC_SR_ADRDY)
#define ADC_IT_FLAG_EOSMP                           (ADC_SR_EOSMP)  
#define ADC_IT_FLAG_EOC                             (ADC_SR_EOC)  
#define ADC_IT_FLAG_EOG                             (ADC_SR_EOG)  
#define ADC_IT_FLAG_OVERF                           (ADC_SR_OVERF) 
#define ADC_IT_FLAG_JEOC                            (ADC_SR_JEOC) 
#define ADC_IT_FLAG_JEOG                            (ADC_SR_JEOG) 
#define ADC_IT_FLAG_AWD                             (ADC_SR_AWD)
#define ADC_IT_FLAG_ALL                             (0xFFU)    
#define IS_ADC_IT_FLAG(__FLAG__)                    (((__FLAG__) == ADC_IT_FLAG_EOSMP)  || \
                                                    ((__FLAG__) == ADC_IT_FLAG_EOC)     || \
                                                    ((__FLAG__) == ADC_IT_FLAG_EOG)     || \
                                                    ((__FLAG__) == ADC_IT_FLAG_OVERF)   || \
                                                    ((__FLAG__) == ADC_IT_FLAG_JEOC)    || \
                                                    ((__FLAG__) == ADC_IT_FLAG_JEOG)    || \
                                                    ((__FLAG__) == ADC_IT_FLAG_AWD)     || \
                                                    ((__FLAG__) <= ADC_IT_FLAG_ALL))   
#define IS_ADC_FLAG(__FLAG__)                       (((__FLAG__) == ADC_IT_FLAG_EOSMP)  || \
                                                    ((__FLAG__) == ADC_IT_FLAG_EOC)     || \
                                                    ((__FLAG__) == ADC_IT_FLAG_EOG)     || \
                                                    ((__FLAG__) == ADC_IT_FLAG_OVERF)   || \
                                                    ((__FLAG__) == ADC_IT_FLAG_JEOC)    || \
                                                    ((__FLAG__) == ADC_IT_FLAG_JEOG)    || \
                                                    ((__FLAG__) == ADC_IT_FLAG_AWD)     || \
                                                    ((__FLAG__) == ADC_FLAG_ADRDY)      || \
                                                    ((__FLAG__) <= ADC_IT_FLAG_ALL))                                                     
/**
  * @}
  */ 
  
/** @defgroup Single-ended or differential 
* @{
*/
#define ADC_DIFF_SINGLE                             (0U)  
#define ADC_DIFF_DIFFERENTIAL                       (1U)  
#define IS_ADC_DIFF(__MODE__)                       (((__MODE__) == ADC_DIFF_SINGLE) || \
                                                    ((__MODE__) == ADC_DIFF_DIFFERENTIAL))
/**
  * @}
  */ 
  

/** @defgroup Signed or unsigned number selection
* @{
*/
#define ADC_SIGN_UNSIGNED                           (0U)   
#define ADC_SIGN_SIGNED                             (1U)   
#define IS_ADC_SIGN(__MODE__)                       (((__MODE__) == ADC_SIGN_SIGNED) || \
                                                    ((__MODE__) == ADC_SIGN_UNSIGNED))
/**
  * @}
  */

/** @defgroup vrefp selection
* @{
*/
#define VREFP_INTERN                                (0U)
#define VREFP_EXTERN                                (1U)
#define VREFP_INTERN_1V5                            (0U)
#define VREFP_INTERN_1V8                            (ADC_CVRB_VRS_0)
#define VREFP_INTERN_2V0                            (ADC_CVRB_VRS_1)
#define VREFP_INTERN_2V5                            (ADC_CVRB_VRS_0 | ADC_CVRB_VRS_1)
#define VREFP_EXTERN_VREF                           (4U) 
#define IS_ADC_VREFP(VREFP)                         (((VREFP) == VREFP_INTERN_1V5)  || \
                                                    ((VREFP) == VREFP_INTERN_1V8)   || \
                                                    ((VREFP) == VREFP_INTERN_2V0)   || \
                                                    ((VREFP) == VREFP_INTERN_2V5)   || \
                                                    ((VREFP) == VREFP_EXTERN_VREF)) 
/**
  * @}
  */  



/** 
  * @brief   ADC Common Init structure definition  
  */ 
typedef struct 
{
    uint32_t OffsetNumber;                      /*!< Select the offset number
                                                    Caution: Only one offset is allowed per channel. This parameter overwrites the last setting. */

    uint32_t Offset;                            /*!< Define the offset to be applied on the raw converted data.
                                                    Offset value must be a positive number.
                                                    Depending of ADC resolution selected (12, 10, 8 or 6 bits), this parameter must be a number between Min_Data = 0x000 and Max_Data = 0xFFF,
                                                    0x3FF, 0xFF or 0x3F respectively.
                                                    Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled
                                                    without continuous mode or external trigger that could launch a conversion). */             

    uint32_t OffsetCalculate;                   /*!< Define ADC conversion result minus or plus offset.
                                                    Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled
                                                    without continuous mode or external trigger that could launch a conversion). */
    uint32_t Offsetsign;                        /*!< Specify whether the result format is signed or unsigned.
                                                    Note: This parameter must be modified when no conversion is on going on both regular and injected groups (ADC disabled, or ADC enabled
                                                    without continuous mode or external trigger that could launch a conversion). */
}ADC_OffsetConfTypeDef;
/**
  * @}
  */ 
  
  

/** 
  * @brief  ADC ExTigger structure definition
  */
typedef struct
{
    uint32_t ExTrigSel;                     /*!< Configures the regular channel trig mode. */
    
    uint32_t JExTrigSel;                    /*!< Configures the inject channel trig mode. */
    
}ADC_ExTrigTypeDef;
/**
  * @}
  */ 
  

/** 
  * @brief  ADC group regular oversampling structure definition
  */
typedef struct
{
    uint32_t Ratio;                         /*!< Configures the oversampling ratio. */

    uint32_t RightBitShift;                 /*!< Configures the division coefficient for the Oversampler. */

    uint32_t TriggeredMode;                 /*!< Selects the regular triggered oversampling mode. */
    
}ADC_OversamplingTypeDef;
/**
  * @}
  */ 


/**
  * @brief  ADC Configuration Structure definition
  */
typedef struct
{
    uint32_t ClockSource;                   /*!< Specify the ADC clock source. */
    
    uint32_t ClockPrescaler;                /*!< Specify the ADC clock div from the PCLK. */

    uint32_t Resolution;                    /*!< Configure the ADC resolution. */

    uint32_t DataAlign;                     /*!< Specify ADC data alignment in conversion data register (right or left). */
    
    FunctionalState ConConvMode;            /*!< Specify whether the conversion is performed in single mode (one conversion) or continuous mode for 
                                              ADC group regular,after the first ADC conversion start trigger occurred (software start or external trigger). */	
    FunctionalState DiscontinuousConvMode;  /*!< Specifies whether the conversions sequence of regular group is performed in Complete-sequence/Discontinuous-sequence 
                                              (main sequence subdivided in successive parts).Discontinuous mode is used only if sequencer is enabled 
                                              (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.Discontinuous mode can be enabled only 
                                              if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded. */
    uint32_t NbrOfDiscConversion;           /*!< Regular channel intermittent mode channel count. */
                                               
    uint32_t ExternalTrigConv;              /*!< Selects the external event used to trigger the conversion start of regular group.
                                              If set to ADC_SOFTWARE_START, external triggers are disabled.
                                              If set to external trigger source, triggering is on event rising edge by default. */
    uint32_t ExternalTrigConvEdge;          /*!< Selects the external trigger edge of regular group.
                                              If trigger is set to ADC_SOFTWARE_START, this parameter is discarded. */

    uint32_t ChannelEn;                     /*!< Specify the enable ADC channels. */

                                                                                                
    uint32_t DMAMode;                       /*!< Specify whether the DMA requests are performed in one shot mode (DMA transfer stops when number of conversions is reached)
                                              or in continuous mode (DMA transfer unlimited, whatever number of conversions). */
    uint32_t OverMode;                      /*!<  ADC_OVERMODE_DISABLE,ADC_OVERMODE_ENABLE */

    uint32_t OverSampMode;                  /*!< Specify whether the oversampling feature is enabled or disabled. */
    
    ADC_OversamplingTypeDef Oversampling;   /*!< Specify ADC group regular oversampling structure. */

    uint32_t AnalogWDGEn;
	
											  
	 
}ADC_InitTypeDef;
/**
  * @}
  */ 
  

typedef struct
{
    uint32_t Channel;                       /*!< Specify the channel to configure into ADC regular group. */

    uint32_t Sq;                            /*!< Add or remove the channel from ADC regular group sequencer and specify its conversion rank. */

    uint32_t Smp;                           /*!< Sampling time value to be set for the selected channel. */
    
    uint32_t Diff;                          /*!< Select differential input.In differential mode: Differential measurement is carried out 
                                              between the selected channel 'i' (positive input) and channel 'i+1' (negative input).
                                              Only channel 'i' has to be configured,  channel 'i+1' is configured automatically. */
    uint32_t OffsetNumber;                  /*!< Select the offset number. */

    uint32_t Offset;                        /*!< Define the offset to be applied on the raw converted data.
                                              Offset value must be a positive number. */

    uint32_t OffsetCalculate;               /*!< Define if the offset should be substracted (negative sign) 
                                              or added (positive sign) from or to the raw converted data. */
    uint32_t Offsetsign;                    /*!< Define if the offset should be saturated upon under or over flow. */
    
}ADC_ChannelConfTypeDef;
/**
  * @}
  */ 
  
  
typedef struct
{
    uint32_t WatchdogMode;                  /*!< Configure the ADC analog watchdog mode: single/all/none channels. For Analog Watchdog 1: 
                                              Configure the ADC analog watchdog mode: single channel or all channels, ADC group regular.
                                              For Analog Watchdog 2 and 3: Several channels can be monitored by applying successively the AWD init structure. */

    uint32_t RegularChannel;                /*!< Select the analog watchdog regular channe */
    
    uint32_t InjectChannel;                /*!< Select the analog watchdog inject channe */

    uint32_t ITMode;                        /*!< Specify whether the analog watchdog is configured in interrupt or polling mode. */

    uint32_t HighThreshold;                 /*!< Configure the ADC analog watchdog High threshold value. */

    uint32_t LowThreshold;                  /*!< Configures the ADC analog watchdog Low threshold value. */

    uint32_t Diff;                          /*!< Select differential input. */
    
}ADC_AnalogWDGConfTypeDef;
/**
  * @}
  */ 
  
  
/**
  * @brief  ADC Injected Conversion Oversampling structure definition
  */
typedef struct
{
    uint32_t Ratio;                         /*!< Configures the oversampling ratio. */

    uint32_t RightBitShift;                 /*!< Configures the division coefficient for the Oversampler. */
    
} ADC_InjOversamplingTypeDef;
/**
  * @}
  */ 
  
  

typedef struct 
{
    uint32_t InjectedChannel;                       /*!< Selection of ADC channel to configure */
    
    uint32_t InjectedRank;                          /*!< Rank in the injected group sequencer */
    
    uint32_t InjectedSamplingTime;                  /*!< Sampling time value to be set for the selected channel. */
    
    uint32_t InjectedDiff;                          /*!< Selection of differential input. */

    uint32_t InjectedOffsetNumber;                  /*!< Selects the offset number. */

    uint32_t InjectedOffset;                        /*!< Defines the offset to be applied on the raw converted data. */

    uint32_t InjectedOffsetCalculate;               /*!< Define if the offset should be substracted (negative sign) or added (positive sign) from or to the raw converted data. */
    
    uint32_t InjectedOffsetSign;                    /*!< Define if the offset should be saturated upon under or over flow. */
                                                     
    uint32_t InjectedNbrOfConversion;               /*!< Specifies the number of ranks that will be converted within the injected group sequencer. */
    
    FunctionalState InjectedDiscontinuousConvMode;  /*!< Specifies whether the conversions sequence of injected group is performed in Complete-sequence/Discontinuous-sequence (main sequence subdivided in successive parts).
                                                      Discontinuous mode is used only if sequencer is enabled (parameter 'ScanConvMode'). If sequencer is disabled, this parameter is discarded.
                                                      Discontinuous mode can be enabled only if continuous mode is disabled. If continuous mode is enabled, this parameter setting is discarded. */
    FunctionalState AutoInjectedConv;               /*!< Enables or disables the selected ADC automatic injected group conversion after regular one */
    
    uint32_t ExternalTrigInjecConv;                 /*!< Selects the external event used to trigger the conversion start of injected group.
                                                      If set to ADC_INJECTED_SOFTWARE_START, external triggers are disabled.
                                                      If set to external trigger source, triggering is on event rising edge. */
    uint32_t ExternalTrigInjecConvEdge;             /*!< Selects the external trigger edge of injected group. */
                                                               
    FunctionalState InjecOversamplingMode;          /*!< Specifies whether the oversampling feature is enabled or disabled. */

    ADC_InjOversamplingTypeDef  InjecOversampling;  /*!< Specifies the Oversampling parameters. */															   
															  
}ADC_InjectionConfTypeDef; 
/**
  * @}
  */ 
  
  


/**
  * @brief  ADC handle Structure definition
  */
typedef struct __ADC_HandleTypeDef
{
    ADC_TypeDef                     *Instance;                                  /*!< Register base address */
    
    ADC_InitTypeDef                 Init;                                       /*!< ADC initialization parameters and regular conversions setting */
#ifdef HAL_DMA_MODULE_ENABLED    
    DMA_HandleTypeDef               *DMA_Handle;                                /*!< Pointer DMA Handler */
#endif    
    uint32_t                        ChannelNum;                                 /*!< Total enable regular group channel number*/
    
    uint32_t                        *AdcResults;                                /*!< Point to the convert results*/
    
    void (*ConvCpltCallback)(struct __ADC_HandleTypeDef *hadc);                 /*!< ADC conversion complete callback */
    
    void (*GroupCpltCallback)(struct __ADC_HandleTypeDef *hadc);                /*!< ADC regular group conversion complete callback */
    
    void (*InjectedConvCpltCallback)(struct __ADC_HandleTypeDef *hadc);         /*!< ADC injected conversion complete callback */
    
    void (*InjectedGroupConvCpltCallback)(struct __ADC_HandleTypeDef *hadc);    /*!< ADC injected Group conversion complete callback */
    
    void (*LevelOutOfWindowCallback)(struct __ADC_HandleTypeDef *hadc);         /*!< ADC analog watchdog callback */
    
}ADC_HandleTypeDef;
/**
  * @}
  */ 
  
  
  
/* Exported functions --------------------------------------------------------*/

/* HAL_RTC_Config */
void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc);

/* HAL_ADC_MspInit */
void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc);

/* HAL_ADC_MspDeInit */
void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Differential_MspInit */
void HAL_ADC_Differential_MspInit(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Differential_MspDeInit */
void HAL_ADC_Differential_MspDeInit(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Init */
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc);

/* HAL_ADC_DeInit */
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc);

/* HAL_ADC_ConfigChannel */
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig);

/* HAL_ADC_AnalogWDGConfig */
HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig);

/* HAL_ADC_Start */
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Stop */
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Start_IT */
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Stop_IT */
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc);

/* HAL_ADC_Start_DMA */
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length);

/* HAL_ADC_Stop_DMA */
HAL_StatusTypeDef HAL_ADC_Stop_DMA(ADC_HandleTypeDef* hadc);

/* HAL_ADC_GetValue */
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc);

/* HAL_ADC_Polling */
HAL_StatusTypeDef HAL_ADC_Polling(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length, uint32_t Timeout);

/* HAL_ADC_InjectedConfigChannel */
HAL_StatusTypeDef HAL_ADC_InjectedConfigChannel(ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected);

/* HAL_ADC_InjectedStart */
HAL_StatusTypeDef HAL_ADC_InjectedStart(ADC_HandleTypeDef* hadc);

/* HAL_ADC_InjectedStop */
HAL_StatusTypeDef HAL_ADC_InjectedStop(ADC_HandleTypeDef* hadc);

/* HAL_ADC_InjectedStart_IT */
HAL_StatusTypeDef HAL_ADC_InjectedStart_IT(ADC_HandleTypeDef* hadc);

/* HAL_ADC_InjectedStop_IT */
HAL_StatusTypeDef HAL_ADC_InjectedStop_IT(ADC_HandleTypeDef* hadc);

/* HAL_ADC_InjectedGetValue */
uint32_t HAL_ADC_InjectedGetValue(ADC_HandleTypeDef* hadc, uint32_t InjectedRank);

/* HAL_ADC_GetFlagStatus */
FlagStatus HAL_ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG);

/* HAL_ADC_ClearFlag */
void HAL_ADC_ClearFlag(ADC_TypeDef* ADCx, uint8_t ADC_FLAG);

/* HAL_ADC_ITConfig */
void HAL_ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState);

/* HAL_ADC_GetITStatus */
ITStatus HAL_ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT);

/* HAL_ADC_ClearITPendingBit */
void HAL_ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint32_t ADC_IT);

/* HAL_ADC_ResolutionDataAlignConfig */
void HAL_ADC_ResolutionDataAlignConfig(ADC_TypeDef* ADCx, uint32_t resolution, uint32_t dataAlign);

/* HAL_ADC_StopControlCmd */
void HAL_ADC_StopControlCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_OverflowmodeConfig */
void HAL_ADC_OverflowmodeConfig(ADC_TypeDef* ADCx, uint32_t mode);

/* HAL_ADC_DifferentialConfig */
void HAL_ADC_DifferentialConfig(ADC_TypeDef* ADCx, uint8_t channel, uint8_t differential);

/* HAL_ADC_OffsetConfig */
void HAL_ADC_OffsetConfig(ADC_TypeDef* ADCx, uint8_t channel, ADC_OffsetConfTypeDef *hoffset);

/* HAL_ADC_OverSamplingConfig */
void HAL_ADC_OverSamplingConfig(ADC_TypeDef* ADCx, ADC_OversamplingTypeDef *hOversampling);

/* HAL_ADC_OverSamplingCmd */
void HAL_ADC_OverSamplingCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_InjectOverSamplingCmd */
void HAL_ADC_InjectOverSamplingCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_ContinuousModeCmd */
void HAL_ADC_ContinuousModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_DiscModeChannelCountConfig */
void HAL_ADC_DiscModeChannelCountConfig(ADC_TypeDef* ADCx, uint8_t number);

/* HAL_ADC_DiscModeCmd */
void HAL_ADC_DiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_InjectDiscModeCmd */
void HAL_ADC_InjectDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_VREFCmd */
void HAL_ADC_VREFCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_VBATCmd */
void HAL_ADC_VBATCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_TSCmd */
void HAL_ADC_TSCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_DMAmd */
void HAL_ADC_DMAmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_InjectedExternalTriggerConfig */
void HAL_ADC_InjectedExternalTriggerConfig(ADC_TypeDef* ADCx, uint32_t trigger, uint32_t edge);

/* HAL_ADC_RegularExternalTriggerConfig */
void HAL_ADC_RegularExternalTriggerConfig(ADC_TypeDef* ADCx, uint32_t trigger, uint32_t edge);

/* HAL_ADC_AutoInjectedConvCmd */
void HAL_ADC_AutoInjectedConvCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_InjectedDiscModeCmd */
void HAL_ADC_InjectedDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState);

/* HAL_ADC_VrefpConfig */
void HAL_ADC_VrefpConfig(uint8_t mode, uint8_t voltage);

/* HAL_ADC_FactoryBasicParameterConfig */
HAL_StatusTypeDef HAL_ADC_FactoryBasicParameterConfig(void);















#endif





