/******************************************************************************
* @file  :  hal_dlyb.c
* @brief :  DLYB HAL module driver.
*          This file provides firmware functions to manage the following
*          functionalities of the Delay Block (DLYB).
* @ver   :  V1.0.0
* @date  :  
******************************************************************************/
#include "hal.h" 

#ifdef HAL_DLYB_MODULE_ENABLED

HAL_StatusTypeDef HAL_DLYB_SetConfiguration(DLYB_TypeDef *DLYBx, uint32_t sel, uint32_t unit)
{    
    assert_param(IS_DLYB_SEL(sel));
    assert_param(IS_DLYB_UNIT(unit));
    
    DLYBx->CR = DLYB_CR_DEN;
    DLYBx->CR |= DLYB_CR_SEN;
    DLYBx->CFGR = (sel << DLYB_CFGR_SEL_Pos) | (unit << DLYB_CFGR_UNIT_Pos);
    DLYBx->CR &= ~DLYB_CR_SEN;
    
    return HAL_OK;
}

#endif //HAL_DLYB_MODULE_ENABLED
