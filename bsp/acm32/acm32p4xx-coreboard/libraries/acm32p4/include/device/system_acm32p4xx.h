/**
 * @file system_acm32p4xx.h
 * @brief ACM32P4XX 系统配置头文件
 */

#ifndef __SYSTEM_ACM32P4XX_H
#define __SYSTEM_ACM32P4XX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* 系统时钟频率变量 */
extern uint32_t SystemCoreClock;

/* 时钟值定义 */
#if !defined (HSE_VALUE)
#define HSE_VALUE    ((uint32_t)24000000U) /*!< 外部高速晶振频率 24MHz */
#endif

#if !defined (HSI_VALUE)
#define HSI_VALUE    ((uint32_t)16000000U) /*!< 内部高速振荡器频率 16MHz */
#endif

#if !defined (LSE_VALUE)
#define LSE_VALUE    ((uint32_t)32768U)    /*!< 外部低速晶振频率 32.768KHz */
#endif

#if !defined (LSI_VALUE)
#define LSI_VALUE    ((uint32_t)32000U)    /*!< 内部低速振荡器频率 32KHz */
#endif

/* 系统初始化函数 */
extern void SystemInit(void);

/* 系统时钟更新函数 */
extern void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_ACM32P4XX_H */