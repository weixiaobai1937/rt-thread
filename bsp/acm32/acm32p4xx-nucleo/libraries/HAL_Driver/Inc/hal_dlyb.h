/*
  ******************************************************************************
  * @file    HAL_DLYB.h
  * @version V1.0.0
  * @date    2020
  * @brief   Header file of DLYB HAL module.
  ******************************************************************************
*/
#ifndef __HAL_DLYB_H__
#define __HAL_DLYB_H__

#include "acm32p4xx_hal_conf.h"

/******************************************************************************/
/*                    Peripheral Registers Bits Definition                    */
/******************************************************************************/

#define DLYB_SEL_MAX							12
#define DLYB_UNIT_MAX							16

/******************************** Check DLYB Parameter *******************************/
#define IS_DLYB_ALL_INSTANCE(INSTANCE)      (((INSTANCE) == ETH_DLYB)         || \
                                            ((INSTANCE) == SDMMC_DLYBS)       || \
                                            ((INSTANCE) == SDMMC_DLYBD)       || \
                                            ((INSTANCE) == OSPI1_DLYB)        || \
                                            ((INSTANCE) == OSPI2_DLYB)        || \
                                            ((INSTANCE) == OSPI2IO0_DLYB)     || \
                                            ((INSTANCE) == OSPI2IO1_DLYB)     || \
                                            ((INSTANCE) == OSPI2IO2_DLYB)     || \
                                            ((INSTANCE) == OSPI2IO3_DLYB)     || \
                                            ((INSTANCE) == SDMMC_SAMPLE_DLYB) || \
                                            ((INSTANCE) == SDMMC_DRIVE_DLYB))
                                            
#define IS_DLYB_SEL(SEL)    (((SEL) >= 0U) && ((SEL) <= DLYB_SEL_MAX))

#define IS_DLYB_UNIT(UNIT)    (((UNIT) >= 0U) && ((UNIT) < DLYB_UNIT_MAX))

HAL_StatusTypeDef HAL_DLYB_SetConfiguration(DLYB_TypeDef *DLYBx, uint32_t sel, uint32_t unit);

#endif
