/******************************************************************************
*@file  : hal_exti.c
*@brief : GPIO EXTI module driver.
******************************************************************************/

#include "hal.h" 

#ifdef HAL_EXTI_MODULE_ENABLED

/******************************************************************************
*@brief : EXTI interrupt request.
*@param : Line: Exti line number.
*               This parameter can be a combination of @ref EXTI_Line.
*@return: None
******************************************************************************/
__attribute__((weak)) void HAL_EXTI_IRQHandler(uint32_t Line)
{
    assert_param(IS_EXTI_LINE(Line));
    
    if (EXTI->PDR & (1UL << Line))
    {
        EXTI->PDR = 1UL << Line;
        HAL_EXTI_LineCallback(Line);
    }
}

/******************************************************************************
*@brief : EXTI interrupt callback.
*@param : Line: Exti line number.
*               This parameter can be a combination of @ref EXTI_Line.
*@return: None
******************************************************************************/
__attribute__((weak)) void HAL_EXTI_LineCallback(uint32_t Line)
{
	UNUSED(Line);
}

/******************************************************************************
*@brief : Set configuration of a dedicated Exti line.
*@param : GPIOx: where x can be (A..F) to select the GPIO peripheral.
*                This parameter is only possible for line 0 to 15.
*@param : Line:  Exti line number.
*                This parameter can be a value of @ref EXTI_Line.
*@param : Mode:  The Exit Mode to be configured for a core.
                 This parameter can be a value of @ref EXTI_Mode .
*@return: HAL Status
******************************************************************************/
HAL_StatusTypeDef HAL_EXTI_SetConfigLine(GPIO_TypeDef *GPIOx, uint32_t Line, uint32_t Mode)
{
    uint32_t mask = 1UL << Line;
    uint32_t source;
    
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));
    assert_param(IS_EXTI_MODE(Mode));

    /* disable interrupt and event */
    EXTI->IENR  &= ~mask;
    EXTI->EENR  &= ~mask;
    EXTI->RTENR &= ~mask;
    EXTI->FTENR &= ~mask;
    EXTI->PDR    =  mask;

    /* Line0 ~ 15 trigger from GPIO */
	if (Line <= EXTI_LINE_15)
	{
        /* Check the parameters */
        assert_param(IS_EXTI_GPIO(GPIOx));
//        assert_param(IS_EXTI_GPIO_LINE(GPIOx, Line));
        
        /* Configure EXTI source select */
        source = ((uint32_t)GPIOx - (uint32_t)GPIOA) >> 10;
		
        if (Line < 8U) 
        {
            /* Line0 ~ 7 */
            EXTI->CR1 = (EXTI->CR1 & ~(0xFUL << (Line << 2))) | (source << (Line << 2));
        }
        else 
        {
            /* Line8 ~ 15 */
            EXTI->CR2 = (EXTI->CR2 & ~(0xFUL << ((Line - 8) << 2))) | (source << ((Line - 8) << 2));
        }
    }
	
    if (Mode == EXTI_MODE_IT_RISING)
    {
        /* Interrupt rising edge mode */
        EXTI->RTENR |= mask;
        EXTI->IENR  |= mask;
    }
    else if (Mode == EXTI_MODE_IT_FALLING)
    {
        /* Interrupt falling edge mode */
        EXTI->FTENR |= mask;
        EXTI->IENR  |= mask;
    }
    else if (Mode == EXTI_MODE_IT_RISING_FALLING)
    {
        /* Interrupt rising and falling edge mode */
        EXTI->RTENR |= mask;
        EXTI->FTENR |= mask;
        EXTI->IENR  |= mask;
    }
    else if (Mode == EXTI_MODE_EVT_RISING)
    {
        /* Event rising edge mode */
        EXTI->RTENR |= mask;
        EXTI->EENR  |= mask;
    }
    else if (Mode == EXTI_MODE_EVT_FALLING)
    {
        /* Event falling edge mode */
        EXTI->FTENR |= mask;
        EXTI->EENR  |= mask;
    }
    else if (Mode == EXTI_MODE_EVT_RISING_FALLING)
    {
        /* Event rising and falling edge mode */
        EXTI->RTENR |= mask;
        EXTI->FTENR |= mask;
        EXTI->EENR  |= mask;
    }

    return HAL_OK;
}

/******************************************************************************
*@brief : Clear whole configuration of a dedicated Exti line.
*@param : Line:  Exti line number.
*                This parameter can be a value of @ref EXTI_Line.
*@return: HAL Status
******************************************************************************/
HAL_StatusTypeDef HAL_EXTI_ClearConfigLine(uint32_t Line)
{
    uint32_t mask = 1UL << Line;
    
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));

    /* disable interrupt and event */
    EXTI->IENR  &= ~mask;
    EXTI->EENR  &= ~mask;
    EXTI->RTENR &= ~mask;
    EXTI->FTENR &= ~mask;
    EXTI->PDR    =  mask;

    /* Line0 ~ 15 trigger from GPIO */
	if (Line < 16U)
	{
        if (Line < 8U) 
        {
            /* Line0 ~ 7 */
            EXTI->CR1 = EXTI->CR1 & ~(0x0FUL << (Line << 2));
        }
        else
        {
            /* Line8~ 15 */
            EXTI->CR2 = EXTI->CR2 & ~(0x0FUL << ((Line - 8) << 2));
        }
    }
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Check whether the LINE of the specified GPIOx is configured.
*@param : GPIOx: where x can be (A..F) to select the GPIO peripheral.
*                This parameter is only possible for line 0 to 15.
*@param : Line:  Exti line number.
*                This parameter can be a value of @ref EXTI_Line.
*@return: configuration status
*         This parameter can be a value of @ref FunctionalState.
*             @arg ENABLE: Configured.
*             @arg DISABLE: Not configured.
******************************************************************************/
FunctionalState HAL_EXTI_IsConfigLine(GPIO_TypeDef *GPIOx, uint32_t Line)
{
    uint32_t mask = 1UL << Line;
    uint32_t source;
    
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));

    if (((EXTI->IENR & mask) == 0) && ((EXTI->IENR & mask) == 0))
    {
        return (DISABLE);
    }
    
	if (Line >= 16U)
    {
        return (ENABLE);
    }
    
    /* Line0 ~ 15 trigger from GPIO */
    
    /* Check the parameters */
    assert_param(IS_EXTI_GPIO(GPIOx));
    
    /* Configure EXTI source select */
    source = (GPIOx - GPIOA) >> 10;
    
    if (Line < 8U) 
    {
        /* Line0 ~ 7 */
        if (((EXTI->CR1 >> (Line << 2)) & 0x0FU) == source)
            return (ENABLE);
        else
            return (DISABLE);
    }
    else
    {
        /* Line8 ~ 15 */
        if (((EXTI->CR2 >> ((Line - 8) << 2)) & 0x0FU) == source)
            return (ENABLE);
        else
            return (DISABLE);
    }
}

/******************************************************************************
*@brief : The specified Line generates a software interrupt.
*@param : Line:  Exti line number.
*                This parameter can be a combination of @ref EXTI_Line.
*@return: None.
******************************************************************************/
void HAL_EXTI_GenerateSWI(uint32_t Line)
{
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));

    EXTI->SWIER &= ~(1UL << Line);
    EXTI->SWIER |=  (1UL << Line);
}

/******************************************************************************
*@brief : Get interrupt pending bit of a dedicated line.
*@param : Line:  Exti line number.
*                This parameter can be a value of @ref EXTI_Line.
*@return: interrupt pending bit status
*         This parameter can be a value of @ref FlagStatus.
*             @arg SET: interrupt is pending.
*             @arg RESET: interrupt is not pending.
******************************************************************************/
FlagStatus HAL_EXTI_GetPending(uint32_t Line)
{
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));

    if (EXTI->PDR & (1UL << Line))
        return (SET);
    else
        return (RESET);
}

/******************************************************************************
*@brief : Clear interrupt pending bit of a dedicated line.
*@param : Line:  Exti line number.
*                This parameter can be a combination of @ref EXTI_Line.
*@return: None.
******************************************************************************/
void HAL_EXTI_ClearPending(uint32_t Line)
{
    /* Check the parameters */
    assert_param(IS_EXTI_LINE(Line));

    EXTI->PDR = 1UL << Line;
}

/******************************************************************************
*@brief : Clear all interrupt pending bit.
*@return: None.
******************************************************************************/
void HAL_EXTI_ClearAllPending(void)
{
    /* Clear pending status */
    EXTI->PDR = EXTI_ALL_LINE_MASK;  
}



#endif /* HAL_EXTI_MODULE_ENABLED */



