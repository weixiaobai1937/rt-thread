
/******************************************************************************
*@file  : hal_lpuart.c
*@brief : LPUART HAL module driver.
******************************************************************************/

#include "hal.h" 

#ifdef HAL_LPUART_MODULE_ENABLED

/* Clock weighted distribution table */
const uint16_t fBaudRate[4][12] = 
{
//	   0		1		2		3		4		5		6		7		8		9		10		11	
    {0x0000, 0x0010, 0x0044, 0x0092, 0x00AA, 0x0155, 0x01B6, 0x01DD, 0x01EF, 0x0000, 0x0000, 0x0000},//9 bits
    {0x0000, 0x0020, 0x0084, 0x0124, 0x014A, 0x02AA, 0x02B5, 0x036D, 0x03DE, 0x03DF, 0x0000, 0x0000},//10 bits
    {0x0000, 0x0020, 0x0088, 0x0124, 0x0252, 0x02AA, 0x0355, 0x0575, 0x0776, 0x0777, 0x07DF, 0x0000},//11 bits
    {0x0000, 0x0040, 0x0108, 0x0248, 0x02A4, 0x0554, 0x0AAA, 0x0AEA, 0x0DB6, 0x0EEE, 0x0F7B, 0x0FDF},//12 bits
};

static void LPUART_DMATransmitCplt(DMA_HandleTypeDef *hdma);
static void LPUART_DMATxHalfCplt(DMA_HandleTypeDef *hdma);
static void LPUART_DMAReceiveCplt(DMA_HandleTypeDef *hdma);
static void LPUART_DMARxHalfCplt(DMA_HandleTypeDef *hdma);
static void LPUART_DMATxError(DMA_HandleTypeDef *hdma);
static void LPUART_DMARxError(DMA_HandleTypeDef *hdma);
static void LPUART_RxISR(LPUART_HandleTypeDef *hlpuart);
static void LPUART_TxISR(LPUART_HandleTypeDef *hlpuart);
static void LPUART_RxEvent(LPUART_HandleTypeDef *hlpuart, HAL_LPUART_RxEventTypeTypeDef EventType);

/******************************************************************************
*@brief : This function handles LPUART interrupt request.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None
******************************************************************************/
void HAL_LPUART_IRQHandler(LPUART_HandleTypeDef *hlpuart)
{
    uint32_t isrflags  = READ_REG(hlpuart->Instance->SR);
    uint32_t itsource  = READ_REG(hlpuart->Instance->IE);
    
    uint32_t errorflags;

    errorflags = (isrflags & (uint32_t)(LPUART_FLAG_RXOVIF | LPUART_FLAG_FEIF | LPUART_FLAG_PEIF));
    
    /* wakeup from Stop mode interrupt occurred */
    if(EXTI->PDR & (3UL << EXTI_LINE_LPUART1_WAKEUP))
    {
        EXTI->PDR |= (3UL << EXTI_LINE_LPUART1_WAKEUP);
              
        if ((isrflags & (LPUART_FLAG_STARTIF) && (itsource & LPUART_IT_START)))
        {
            /* Start bit wakeup */
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_STARTIF);
            HAL_LPUART_WakeupCallback(hlpuart);
        }
        
        if ((isrflags & (LPUART_FLAG_RXIF) && (itsource & LPUART_IT_RX)))
        {
            /* 1-byte wakeup */
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXIF);
            HAL_LPUART_WakeupCallback(hlpuart);
        }

        if ((isrflags & LPUART_FLAG_MATCHIF) && (itsource & LPUART_IT_MATCH))
        {
            /* address matching wakeup */
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_MATCHIF);
            HAL_LPUART_WakeupCallback(hlpuart);
        } 
    }
    
    /* If no error occurs */
    if(errorflags == 0U)
    {
        /* LPUART in mode Receiver ---------------------------------------------------*/
        if((isrflags & LPUART_FLAG_RXIF) && (itsource & LPUART_IT_RX))
        {
            if (hlpuart->RxISR != NULL)
            {
                hlpuart->RxISR(hlpuart);
            }

            return;
        }
    }

    /* If some errors occur */
    if ((errorflags != 0U) && ((itsource & (LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE)) != 0U))
    {
        /* Over-Run interrupt occurred */
        if ((isrflags & LPUART_FLAG_RXOVIF) && (itsource & LPUART_IT_RXOV))
        {
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXOVIF);
            hlpuart->ErrorCode |= HAL_LPUART_ERROR_ORE;
        }
        
        /* frame error interrupt occurred */
        if ((isrflags & LPUART_FLAG_FEIF) && (itsource & LPUART_IT_FE))
        {
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_FEIF);
            hlpuart->ErrorCode |= HAL_LPUART_ERROR_FE;
        }
        
        /* parity error interrupt occurred */
        if ((isrflags & LPUART_FLAG_PEIF) && (itsource & LPUART_IT_PE))
        {
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_PEIF);
            hlpuart->ErrorCode |= HAL_LPUART_ERROR_PE;
        }
        
        /* LPUART in mode Receiver ---------------------------------------------------*/
        if((isrflags & LPUART_FLAG_RXIF) && (itsource & LPUART_IT_RX))
        {
            if (hlpuart->RxISR != NULL)
            {
                hlpuart->RxISR(hlpuart);
            }
        }

        /* If Error is to be considered as blocking :
            - Overrun error in Reception
            - any error occurs in DMA mode reception
        */
        if((hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA) || (hlpuart->ErrorCode == HAL_LPUART_ERROR_ORE))
        {
            /* Disable the UART Error Interrupt, RX interrupts, and IDLE/BCNT interrupt */
            CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE |
                                             LPUART_IT_RX | LPUART_IT_IDLE | LPUART_IT_BCNT);

            /* Rx process is completed, restore hlpuart->RxState to Ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;

            /* Reset RxIsr function pointer */
            hlpuart->RxISR = NULL;

            /* Abort DMA RX */    
            if(hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA)
            {
                __HAL_LPUART_DISABLE_DMA(hlpuart);

                if(hlpuart->hdmatx != NULL)
                {
                    if(HAL_DMA_Abort(hlpuart->hdmatx) != HAL_OK)
                    {
                        hlpuart->RxXferCount = 0U;
                    }
                }
            }
        }
        /*Call weak error callback*/
        HAL_LPUART_ErrorCallback(hlpuart);

        hlpuart->ErrorCode = HAL_UART_ERROR_NONE;
        return;
    } /* End if some error occurs */

    /* Check current reception Mode :
        If Reception till IDLE event has been selected : */
    if ((hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOIDLE)
        && ((isrflags & LPUART_FLAG_IDLEIF) != 0U)
        && ((itsource & LPUART_IT_IDLE) != 0U))
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_IDLEIF);

        LPUART_RxEvent(hlpuart, HAL_LPUART_RXEVENT_IDLE);
        return;
    }

    /* Check current reception Mode :
        If Reception till BCNT event has been selected : */
    if ((hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOBCNT)
        && ((isrflags & LPUART_FLAG_BCNTIF) != 0U)
        && ((itsource & LPUART_IT_BCNT) != 0U))
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_BCNTIF);

        LPUART_RxEvent(hlpuart, HAL_LPUART_RXEVENT_BCNT);
        return;
    }

    /* transmit buffer empty interrupt */
    if ((isrflags & LPUART_FLAG_TXEIF) && (itsource & LPUART_IT_TXE))
    {
        if (hlpuart->TxISR != NULL)
        {
            hlpuart->TxISR(hlpuart);
        }
        else
        {
            /* Disable Tx buffer empty interrupt */
            __HAL_LPUART_DISABLE_IT(hlpuart, LPUART_IT_TXE);
        }
        return;
    }
    
    /* transmit complete interrupt */
    if ((isrflags & LPUART_FLAG_TCIF) && (itsource & LPUART_IT_TC))
    {
        /* Disable the UART Transmit Complete Interrupt */
        CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_TC);
        
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_TCIF);

        /* Tx process is ended, restore hlpuart->gState to Ready */
        hlpuart->gState = HAL_LPUART_STATE_READY;

        /* Cleat TxISR function pointer */
        hlpuart->TxISR = NULL;
        
        /* transmit complete (TCIF) callback function */
        HAL_LPUART_TxCpltCallback(hlpuart);
        return;
    }
    
}

/******************************************************************************
*@brief : Initialize the LPUART according to the specified.
*         parameters in the LPUART_InitTypeDef and create the associated handle.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Init(LPUART_HandleTypeDef *hlpuart)
{  
    if(hlpuart == NULL)
    {
        return HAL_ERROR;
    }
    
    /* Check the parameters */
    assert_param(IS_LPUART_INSTANCE(hlpuart->Instance));
    assert_param(IS_LPUART_MODE(hlpuart->Init.Mode));
  
    if (hlpuart->Instance == LPUART1)    
        __HAL_RCC_LPUART1_RESET();
    else
        __HAL_RCC_LPUART2_RESET();
    
    if (hlpuart->gState == HAL_LPUART_STATE_RESET)
    {
        /* Init the low level hardware : GPIO, CLOCK */
        HAL_LPUART_MspInit(hlpuart);
    }
    
    hlpuart->gState = HAL_LPUART_STATE_BUSY;
    
    LPUART_SetConfig(hlpuart);
    
    /* Initialize the UART ErrorCode */
    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    
    /* Initialize the UART State */
    hlpuart->gState = HAL_LPUART_STATE_READY;
    hlpuart->RxState = HAL_LPUART_STATE_READY;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;
    
    /* enable tx rx */
    hlpuart->Instance->CR = hlpuart->Init.Mode;

    return HAL_OK; 
}

/******************************************************************************
*@brief : DeInitialize the LPUART peripheral.
*         Before Deinitialization, the sending or receiving process needs to be aborted.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_DeInit(LPUART_HandleTypeDef *hlpuart)
{
    if(hlpuart == NULL)
    {
        return HAL_ERROR;
    }

    /* Check the parameters */
    assert_param(IS_LPUART_HANDLE(hlpuart));

    hlpuart->gState = HAL_LPUART_STATE_BUSY;
    
    /* Disable all */
    hlpuart->Instance->CR = 0;
    
    /* DeInit the low level hardware */
    HAL_LPUART_MspDeInit(hlpuart);
    
    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->gState = HAL_LPUART_STATE_RESET;
    hlpuart->RxState = HAL_LPUART_STATE_RESET;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Initialize the LPUART MSP.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None
******************************************************************************/
__weak void HAL_LPUART_MspInit(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : DeInitialize the LPUART MSP.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None
******************************************************************************/
__weak void HAL_LPUART_MspDeInit(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}


/**
  * @brief Configure the LPUART peripheral.
  * @param hlpuart LPUART handle.
  * @retval HAL status
  */
HAL_StatusTypeDef LPUART_SetConfig(LPUART_HandleTypeDef *hlpuart)
{
    uint32_t iBaudRate;
    uint32_t clockNum;
    uint32_t bits;
    float    tempFloat;

    assert_param(IS_LPUART_WORDLENGTH(hlpuart->Init.WordLength));
    assert_param(IS_LPUART_STOPBITS(hlpuart->Init.StopBits));
    assert_param(IS_LPUART_PARITY(hlpuart->Init.Parity));
    assert_param(IS_LPUART_POLARITY(hlpuart->Init.Polarity));
    assert_param(IS_LPUART_CLOCKSOURCE(hlpuart->Init.ClockSource));

    /* set the clock source */
    if (hlpuart->Instance == LPUART1)
        HAL_RCC_LPUART1ClockSourceConfig(hlpuart->Init.ClockSource);
    else
        HAL_RCC_LPUART2ClockSourceConfig(hlpuart->Init.ClockSource);

    /* Set the communication parameters */
    hlpuart->Instance->LCR  =   hlpuart->Init.WordLength    | \
                                hlpuart->Init.StopBits      | \
                                hlpuart->Init.Parity        | \
                                hlpuart->Init.Polarity      | \
                                LPUART_WAKEUPMETHOD_NONE;
    
    /* Calculate the baud rate division factor */
    if(hlpuart->Instance == LPUART1)
    {
        tempFloat = (float)HAL_RCC_GetLPUART1ClockFreq();
    }
    else
    {
        tempFloat = (float)HAL_RCC_GetLPUART2ClockFreq();
    }
                       
    tempFloat = tempFloat / hlpuart->Init.BaudRate;
    iBaudRate = (uint32_t)tempFloat - 1U;
    assert_param(IS_LPUART_BAUDRATE(iBaudRate));
 
    bits = 12;
    if (hlpuart->Init.WordLength == LPUART_WORDLENGTH_7B)
        bits--;
    if (hlpuart->Init.Parity == LPUART_PARITY_NONE)
        bits--;
    if (hlpuart->Init.StopBits == LPUART_STOPBITS_1B)
        bits--;

    tempFloat = (tempFloat - (iBaudRate + 1)) * bits;
    clockNum = (uint32_t)(tempFloat + 0.5);

    hlpuart->Instance->IBAUD = iBaudRate + ((iBaudRate >> 1) << LPUART_IBAUD_RXSAM_Pos);
    hlpuart->Instance->FBAUD = fBaudRate[bits - 9][clockNum];

    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in blocking mode.
*@note    In blocking mode, the user cannot enable interrupt.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be sent.
*@param : timeout: Timeout duration.
*                  The minimum value of this parameter is 1.
*                  If the value of this parameter is 0xFFFFFFFF, it will be sent until all data are sent.
*                  @note The timeout should be greater than the time of all data transfers.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Transmit(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size, uint32_t timeout)
{
    uint32_t startTick;
    
    /* Check that a Tx process is not already ongoing */
    if (hlpuart->gState != HAL_LPUART_STATE_READY)
    {
        return HAL_BUSY;
    }

    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
             
    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->gState  = HAL_LPUART_STATE_BUSY_TX;
    
    hlpuart->TxXferSize  = size;
    hlpuart->TxXferCount = size;
    
    startTick = HAL_GetTick();
    while (hlpuart->TxXferCount > 0U)
    {
        /* Whether tx buffer is empty */
        if (__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_TXE))
        {
            /* send byte */
            hlpuart->Instance->TXDR = *pdata++;
            hlpuart->TxXferCount--;
        }
        else if(timeout != 0xFFFFFFFFU)
        {
            if (((HAL_GetTick() - startTick) > timeout) || (timeout == 0U))
            {
                hlpuart->gState  = HAL_LPUART_STATE_READY;
                return HAL_TIMEOUT;
            }
        }
    }
    
    while(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_TCIF) == RESET)
    {
        if (((HAL_GetTick() - startTick) > timeout) || (timeout == 0U))
        {
            hlpuart->gState = HAL_LPUART_STATE_READY;
            return HAL_TIMEOUT;
        }
    }
    
    hlpuart->gState = HAL_LPUART_STATE_READY;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in blocking mode.
*@note    In blocking mode, the user cannot enable interrupt.
*@note    Users can add or ignore error handling in RxErrorCallback(). 
*         Error codes are stored in hlpuart->RxError.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@param : timeout: Timeout duration.
*                  If the value of this parameter is 0, the received data will be detected only once and will not wait.
*                  If the value of this parameter is 0xFFFFFFFF, it will be received until all data are received.
*                  @note The timeout should be greater than the time of all data transfers.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Receive(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size, uint32_t timeout)
{
    uint32_t startTick;
    
    /* If LPUART is receiving, an error is returned */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
   
    hlpuart->RxXferSize = size;
    hlpuart->RxXferCount = size;
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
      
    startTick = HAL_GetTick();
    /* as long as data have to be received */
    while (hlpuart->RxXferCount > 0U)
    {
        if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
        {
            /* receive data */
            *pdata++ = hlpuart->Instance->RXDR;
            hlpuart->RxXferCount--;
        }
        else if(timeout != 0xFFFFFFFF)
        {
            /* Whether the receiving time has expired */
            if ((HAL_GetTick() - startTick) >= timeout)
            {
                hlpuart->RxState  = HAL_LPUART_STATE_READY;
                return HAL_TIMEOUT;
            }

            if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXOVIF))
            {
                /* Clear error flag and end rx transfer */
                __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXOVIF);
               
                hlpuart->RxState  = HAL_LPUART_STATE_READY;
                hlpuart->ErrorCode = HAL_LPUART_ERROR_ORE;

                return HAL_ERROR;
            }
        }
    }
    
    hlpuart->RxState  = HAL_LPUART_STATE_READY;

    return HAL_OK;
}

/**
  * @brief Receive an amount of data in blocking mode till either the expected number of data
  *        is received or an IDLE event occurs.
  * @note  HAL_OK is returned if reception is completed (expected number of data has been received)
  *        or if reception is stopped after IDLE event (less than the expected number of data has been received)
  *        In this case, rxLen output parameter indicates number of data available in reception buffer.
  * @param hlpuart LPUART handle.
  * @param pdata   Pointer to data buffer.
  * @param size    Amount of data elements to be received.
  * @param rxLen   Number of data elements finally received
  *                (could be lower than size, in case reception ends on IDLE event)
  * @param timeout Timeout duration expressed in ms (covers the whole reception sequence).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, 
                                           uint16_t *rxLen, uint32_t timeout)
{
    uint32_t startTick;
    
    /* If LPUART is receiving, an error is returned */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOIDLE;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;
   
    hlpuart->RxXferSize = size;
    hlpuart->RxXferCount = size;
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Clear idle flag */
    __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_IDLEIF);
    
    /* Initialize output number of received elements */
    *rxLen = 0U;
    
    startTick = HAL_GetTick();     
    /* as long as data have to be received */
    while (hlpuart->RxXferCount > 0U) 
    {  
        /* Check if IDLE flag is set */
        if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_IDLEIF))
        {
            /* Clear IDLE flag in SR */
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_IDLEIF);

            /* If Set, but no data ever received, clear flag without exiting loop */
            /* If Set, and data has already been received, this means Idle Event is valid : End reception */
            if (*rxLen > 0U)
            {
                hlpuart->RxEventType = HAL_LPUART_RXEVENT_IDLE;
                hlpuart->RxState = HAL_LPUART_STATE_READY;

                return HAL_OK;
            }
        }
        
        /* Check if RX flag is set */
        if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
        {
            *pdata = (uint8_t)hlpuart->Instance->RXDR;
            pdata++;
            
            /* Increment number of received elements */
            *rxLen += 1U;
            hlpuart->RxXferCount--;
        }
        
        /* Check for the Timeout */
        if(timeout != 0xFFFFFFFF)
        {
            /* Whether the receiving time has expired */
            if ((HAL_GetTick() - startTick) >= timeout)
            {
                hlpuart->RxState  = HAL_LPUART_STATE_READY;
                return HAL_TIMEOUT;
            }
        }
    }
    
    /* Set number of received elements in output parameter : rxLen */
    *rxLen = hlpuart->RxXferSize - hlpuart->RxXferCount;
    /* At end of Rx process, restore huart->RxState to Ready */
    hlpuart->RxState = HAL_LPUART_STATE_READY;

    return HAL_OK;
}

/**
  * @brief Receive an amount of data in blocking mode till either the expected number of data
  *        is received or an BCNT event occurs.
  * @note  HAL_OK is returned if reception is completed (expected number of data has been received)
  *        or if reception is stopped after BCNT event (less than the expected number of data has been received)
  *        In this case, rxLen output parameter indicates number of data available in reception buffer.
  * @param hlpuart LPUART handle.
  * @param pdata   Pointer to data buffer (uint8_t or uint16_t data elements).
  * @param size    Amount of data elements (uint8_t or uint16_t) to be received.
  * @param rxLen   Number of data elements finally received
  *                (could be lower than size, in case reception ends on BCNT event)
  * @param bcntVal value of BCNT_VAULE at LPUART_LCR
  * @param timeout Timeout duration expressed in ms (covers the whole reception sequence).
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, 
                                                uint16_t *rxLen, uint16_t bcntVal, uint32_t timeout)
{
    uint32_t startTick;
    
    /* If LPUART is receiving, an error is returned */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOBCNT;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;
   
    hlpuart->RxXferSize = size;
    hlpuart->RxXferCount = size;
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Clear bcnt flag */
    __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_BCNTIF);
    
    /* Set BCNT and enable auto bit counter */
    MODIFY_REG(hlpuart->Instance->LCR, LPUART_LCR_BCNT_VALUE, (bcntVal << LPUART_LCR_BCNT_VALUE_Pos));
    SET_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);
    
    /* Initialize output number of received elements */
    *rxLen = 0U;
    
    startTick = HAL_GetTick();     
    /* as long as data have to be received */
    while (hlpuart->RxXferCount > 0U) 
    {  
        /* Check if BCNT flag is set */
        if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_BCNTIF))
        {
            /* Clear BCNT flag in SR */
            __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_BCNTIF);
            
            /* If Set, but no data ever received, clear flag without exiting loop */
            /* If Set, and data has already been received, this means Idle Event is valid : End reception */
            if (*rxLen > 0U)
            {
                /* Disable auto bit counter */
                CLEAR_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);
                
                hlpuart->RxEventType = HAL_LPUART_RXEVENT_BCNT;
                hlpuart->RxState = HAL_LPUART_STATE_READY;

                return HAL_OK;
            }
        }
        
        /* Check if RX flag is set */
        if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
        {
            *pdata = (uint8_t)hlpuart->Instance->RXDR;
            pdata++;
            
            /* Increment number of received elements */
            *rxLen += 1U;
            hlpuart->RxXferCount--;
        }
        
        /* Check for the Timeout */
        if(timeout != 0xFFFFFFFF)
        {
            /* Whether the receiving time has expired */
            if ((HAL_GetTick() - startTick) >= timeout)
            {
                /* Disable auto bit counter */
                CLEAR_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);
                
                hlpuart->RxState  = HAL_LPUART_STATE_READY;
                return HAL_TIMEOUT;
            }
        }
    }
    
    /* Disable auto bit counter */
    CLEAR_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);
    
    /* Set number of received elements in output parameter : rxLen */
    *rxLen = hlpuart->RxXferSize - hlpuart->RxXferCount;
    /* At end of Rx process, restore huart->RxState to Ready */
    hlpuart->RxState = HAL_LPUART_STATE_READY;

    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in interrupt mode.
*@note    In interrupt mode, sending related interrupts (TXE/TC) is forced to enable.
*@note    Users can add or ignore error handling in ErrorCallback(). 
*@note    The user can add the processing of sending completion in TxCpltCallback().
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Transmit_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size)
{
    /* Check that a Tx process is not already ongoing */
    if (hlpuart->gState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    hlpuart->pTxBuffPtr  = pdata;
    hlpuart->TxXferSize  = size;
    hlpuart->TxXferCount = size;
    hlpuart->TxISR       = LPUART_TxISR;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->gState = HAL_LPUART_STATE_BUSY_TX;
    
    /* Enable the Transmit Data Register Empty interrupt */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_TXE);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in interrupt mode.
*@note    In interrupt mode, receiving completion interrupt (RX) and error 
*         interrupt (RXOV, FE, PE) are forced enabled.
*         If an error occurs in the reception, the reception stops automatically.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Receive_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
   
    /* Set Reception type to Standard reception */
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
    
    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;
    hlpuart->RxXferCount = size;
    hlpuart->RxISR       = LPUART_RxISR;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    
    /* enable interrupt */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in interrupt mode till either the expected number of data
*         is received or an IDLE event occurs.
*@note  : Reception is initiated by this function call. Further progress of reception is achieved thanks
*         to LPUART interrupts raised by RX and IDLE events. Callback is called at end of reception indicating
*         number of received data elements.
*@param : hlpuart LPUART handle.
*@param : pdata Pointer to data buffer (uint8_t data elements).
*@param : size  Amount of data elements (uint8_t) to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Set Reception type to reception till IDLE Event*/
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOIDLE;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;

    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;
    hlpuart->RxXferCount = size;
    hlpuart->RxISR       = LPUART_RxISR;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    
    /* enable interrupt */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX);

    if (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOIDLE)
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_IDLEIF);
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_IDLEIE);
    }
    else
    {
        /* In case of errors already pending when reception is started,
            Interrupts may have already been raised and lead to reception abortion.
            (Overrun error for instance).
            In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
        return HAL_ERROR;
    }

    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in interrupt mode till either the expected number of data
*         is received or an BCNT event occurs.
*@note  : Reception is initiated by this function call. Further progress of reception is achieved thanks
*         to LPUART interrupts raised by RX and BCNT events. Callback is called at end of reception indicating
*         number of received data elements.
*@param : hlpuart LPUART handle.
*@param : pdata Pointer to data buffer (uint8_t data elements).
*@param : size  Amount of data elements (uint8_t) to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, uint16_t bcntVal)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Set Reception type to reception till BCNT Event*/
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOBCNT;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;

    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;
    hlpuart->RxXferCount = size;
    hlpuart->RxISR       = LPUART_RxISR;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX;
    
    /* enable interrupt */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX);

    if (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOBCNT)
    {
        /* Set BCNT and enable auto bit counter */
        MODIFY_REG(hlpuart->Instance->LCR, LPUART_LCR_BCNT_VALUE, (bcntVal << LPUART_LCR_BCNT_VALUE_Pos));
        SET_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);

        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_BCNTIF);
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_BCNTIE);
    }
    else
    {
        /* In case of errors already pending when reception is started,
            Interrupts may have already been raised and lead to reception abortion.
            (Overrun error for instance).
            In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
        return HAL_ERROR;
    }

    return HAL_OK;
}

#ifdef HAL_DMA_MODULE_ENABLED
/******************************************************************************
*@brief : Send an amount of data in DMA mode.
*@note    In DMA mode, the transmission related interrupt (TC/TXE) is forced to disabled.
*         the DMA transmission related interrupt (ITC/HFTC/IE) is forced to enabled.
*         If an DMA transfer error occurs in the transmission, 
*         the transmission stops automatically.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Transmit_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size)
{
    /* Check that a Tx process is not already ongoing */
    if (hlpuart->gState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    hlpuart->pTxBuffPtr  = pdata;
    hlpuart->TxXferSize  = size;
    hlpuart->TxXferCount = size;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->gState = HAL_LPUART_STATE_BUSY_TX_DMA;
    
    if(hlpuart->hdmatx != NULL)
    {
        /* Set the LPUART DMA transfer complete callback */
        hlpuart->hdmatx->XferCpltCallback = LPUART_DMATransmitCplt;

        /* Set the LPUART DMA Half transfer complete callback */
        hlpuart->hdmatx->XferHalfCpltCallback = LPUART_DMATxHalfCplt;

        /* Set the DMA error callback */
        hlpuart->hdmatx->XferErrorCallback = LPUART_DMATxError;

        /* Set the DMA abort callback */
        hlpuart->hdmatx->XferAbortCallback = NULL;
               
        /* Start DMA interrupt transfer */
        if (HAL_DMA_Start_IT(hlpuart->hdmatx, (uint32_t)pdata, (uint32_t)&hlpuart->Instance->TXDR, size) != HAL_OK)
        {
            /* Set error code to DMA */
            hlpuart->ErrorCode = HAL_LPUART_ERROR_DMA_TX;

            /* Restore hlpuart->gState to ready */
            hlpuart->gState = HAL_LPUART_STATE_READY;
        }
    }
    /* Clear the TC flag */
    __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_TCIF);
   
    /* enable LPUART DMA */
    __HAL_LPUART_ENABLE_DMA(hlpuart);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in DMA mode.
*@note    In DMA mode, receiving completion interrupt (RX) is forced disable. 
*         the error interrupt (RXOV��FE��PE) are forced enabled.
*         the DMA transmission related interrupt (ITC/HFTC/IE) is forced to enabled.
*         If an error occurs in the reception, the reception stops automatically.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_Receive_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Set Reception type to Standard reception */
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
    
    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX_DMA;
 
    if (hlpuart->hdmarx != NULL)
    {     
        /* Set the LPUART DMA transfer complete callback */
        hlpuart->hdmarx->XferCpltCallback  = LPUART_DMAReceiveCplt;

        /* Set the LPUART DMA Half transfer complete callback */
        hlpuart->hdmarx->XferHalfCpltCallback  = LPUART_DMARxHalfCplt;

        /* Set the DMA error callback */
        hlpuart->hdmarx->XferErrorCallback  = LPUART_DMARxError;

        /* Set the DMA abort callback */
        hlpuart->hdmarx->XferAbortCallback = NULL;
    
        /* Start DMA interrupt transfer */
        if (HAL_DMA_Start_IT(hlpuart->hdmarx, (uint32_t)&hlpuart->Instance->RXDR, (uint32_t)pdata, size) != HAL_OK)
        {
            /* Set error code to DMA */
            hlpuart->ErrorCode = HAL_LPUART_ERROR_DMA_RX;

            /* Restore hlpuart->RxState to ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;

            return HAL_ERROR;
        }
    }
    
    /* enable interrupt (PE,FE,RXOV) */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE);
    
    /* enable LPUART DMA */
    __HAL_LPUART_ENABLE_DMA(hlpuart);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in DMA mode till either the expected number
*       : of data is received or an IDLE event occurs.
*@note  : Reception is initiated by this function call. Further progress of reception is achieved thanks
*       : to DMA services, transferring automatically received data elements in user reception buffer and
*       : calling registered callbacks at half/end of reception. UART IDLE events are also used to consider
*       : reception phase as ended. In all cases, callback execution will indicate number of received data elements.
*@note  : When the UART parity is enabled (PCE = 1), the received data contain
*       : the parity bit (MSB position).
*@note  : When UART parity is not enabled (PCE = 0), and Word Length is configured to 9 bits (M1-M0 = 01),
*       : the received data is handled as a set of uint16_t. In this case, Size must indicate the number
*       : of uint16_t available through pData.
*@param : huart UART handle.
*@param : pData Pointer to data buffer (uint8_t or uint16_t data elements).
*@param : Size  Amount of data elements (uint8_t or uint16_t) to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Set Reception type to reception till IDLE Event*/
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOIDLE;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;

    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX_DMA;
 
    if (hlpuart->hdmarx != NULL)
    {     
        /* Set the LPUART DMA transfer complete callback */
        hlpuart->hdmarx->XferCpltCallback  = LPUART_DMAReceiveCplt;

        /* Set the LPUART DMA Half transfer complete callback */
        hlpuart->hdmarx->XferHalfCpltCallback  = LPUART_DMARxHalfCplt;

        /* Set the DMA error callback */
        hlpuart->hdmarx->XferErrorCallback  = LPUART_DMARxError;

        /* Set the DMA abort callback */
        hlpuart->hdmarx->XferAbortCallback = NULL;
    
        /* Start DMA interrupt transfer */
        if (HAL_DMA_Start_IT(hlpuart->hdmarx, (uint32_t)&hlpuart->Instance->RXDR, (uint32_t)pdata, size) != HAL_OK)
        {
            /* Set error code to DMA */
            hlpuart->ErrorCode = HAL_LPUART_ERROR_DMA_RX;

            /* Restore hlpuart->RxState to ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;

            return HAL_ERROR;
        }
    }
    
    /* enable interrupt (PE,FE,RXOV) */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE);
    
    /* enable LPUART DMA */
    __HAL_LPUART_ENABLE_DMA(hlpuart);

    if (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOIDLE)
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_IDLEIF);
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_IDLEIE);
    }
    else
    {
        /* In case of errors already pending when reception is started,
            Interrupts may have already been raised and lead to reception abortion.
            (Overrun error for instance).
            In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
        return HAL_ERROR;
    }

    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in DMA mode till either the expected number
*       : of data is received or an BCNT event occurs.
*@note  : Reception is initiated by this function call. Further progress of reception is achieved thanks
*       : to DMA services, transferring automatically received data elements in user reception buffer and
*       : calling registered callbacks at half/end of reception. LPUART BCNT events are also used to consider
*       : reception phase as ended. In all cases, callback execution will indicate number of received data elements.
*@param : hlpuart LPUART handle.
*@param : pdata Pointer to data buffer (uint8_t data elements).
*@param : size  Amount of data elements (uint8_t) to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, uint16_t bcntVal)
{
    /* Check that a Rx process is not already ongoing */
    if (hlpuart->RxState != HAL_LPUART_STATE_READY)
        return HAL_BUSY;

    /* Check the parameters */
    if ((pdata == NULL) || (size == 0U))
    {
        return HAL_ERROR;
    }
    
    /* Clear rx buffer */
    if(__HAL_LPUART_GET_FLAG(hlpuart, LPUART_FLAG_RXF))
    {
        volatile uint8_t tmp;
        tmp = hlpuart->Instance->RXDR;
    }
    
    /* Set Reception type to reception till BCNT Event*/
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_TOBCNT;
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;

    hlpuart->pRxBuffPtr  = pdata;
    hlpuart->RxXferSize  = size;

    hlpuart->ErrorCode = HAL_LPUART_ERROR_NONE;
    hlpuart->RxState = HAL_LPUART_STATE_BUSY_RX_DMA;
 
    if (hlpuart->hdmarx != NULL)
    {     
        /* Set the LPUART DMA transfer complete callback */
        hlpuart->hdmarx->XferCpltCallback  = LPUART_DMAReceiveCplt;

        /* Set the LPUART DMA Half transfer complete callback */
        hlpuart->hdmarx->XferHalfCpltCallback  = LPUART_DMARxHalfCplt;

        /* Set the DMA error callback */
        hlpuart->hdmarx->XferErrorCallback  = LPUART_DMARxError;

        /* Set the DMA abort callback */
        hlpuart->hdmarx->XferAbortCallback = NULL;
    
        /* Start DMA interrupt transfer */
        if (HAL_DMA_Start_IT(hlpuart->hdmarx, (uint32_t)&hlpuart->Instance->RXDR, (uint32_t)pdata, size) != HAL_OK)
        {
            /* Set error code to DMA */
            hlpuart->ErrorCode = HAL_LPUART_ERROR_DMA_RX;

            /* Restore hlpuart->RxState to ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;

            return HAL_ERROR;
        }
    }
    
    /* enable interrupt (PE,FE,RXOV) */
    __HAL_LPUART_ENABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE);
    
    /* enable LPUART DMA */
    __HAL_LPUART_ENABLE_DMA(hlpuart);

    if (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOBCNT)
    {
        /* Set BCNT and enable auto bit counter */
        MODIFY_REG(hlpuart->Instance->LCR, LPUART_LCR_BCNT_VALUE, (bcntVal << LPUART_LCR_BCNT_VALUE_Pos));
        SET_BIT(hlpuart->Instance->LCR, LPUART_LCR_AUTO_START_EN);

        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_BCNTIF);
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_BCNTIE);
    }
    else
    {
        /* In case of errors already pending when reception is started,
            Interrupts may have already been raised and lead to reception abortion.
            (Overrun error for instance).
            In such case Reception Type has been reset to HAL_UART_RECEPTION_STANDARD. */
        return HAL_ERROR;
    }

    return HAL_OK;
}

/******************************************************************************
*@brief : Pause the DMA Transfer.
*@param : hlpuart LPUART handle.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_DMAPause(LPUART_HandleTypeDef *hlpuart)
{
    if (hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA)
    {
        /* Disable ERR (Frame error, parity error, overrun error) interrupts */
        CLEAR_BIT(hlpuart->Instance->IE, LPUART_IE_FEIE | LPUART_IE_PEIE | LPUART_IE_RXOVIE);
    }

    /* Disable the LPUART DMA request */
    __HAL_LPUART_DISABLE_DMA(hlpuart);

    return HAL_OK;
}

/******************************************************************************
*@brief : Resume the DMA Transfer.
*@param : hlpuart LPUART handle.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_DMAResume(LPUART_HandleTypeDef *hlpuart)
{
    if (hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA)
    {
        /* Clear the Overrun flag before resuming the Rx transfer */
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXOVIF);

        /* Re-enable PE and ERR (Frame error, noise error, overrun error) interrupts */
        if (hlpuart->Init.Parity != LPUART_PARITY_NONE)
        {
            SET_BIT(hlpuart->Instance->IE, LPUART_IE_PEIE);
        }
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_FEIE | LPUART_IE_RXOVIE);

    }

    /* Enable the LPUART DMA request */
    __HAL_LPUART_ENABLE_DMA(hlpuart);

    return HAL_OK;
}

#endif

/******************************************************************************
*@brief : Abort ongoing transmit transfer(block mode/interrupt mode/dma mode).
*         In blocking mode, check TxState to exit the abort function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_AbortTransmit(LPUART_HandleTypeDef *hlpuart)
{
    /* disable interrupt (TXE/TC) */
    __HAL_LPUART_DISABLE_IT(hlpuart, LPUART_IT_TXE | LPUART_IT_TC);
    
#ifdef HAL_DMA_MODULE_ENABLED    
    /* Abort the LPUART DMA if enabled */
    if (hlpuart->gState == HAL_LPUART_STATE_BUSY_TX_DMA)
    {
        /* Disable the LPUART DMA request */
        __HAL_LPUART_DISABLE_DMA(hlpuart);

        if (hlpuart->hdmatx != NULL)
        {
            /* Set the LPUART DMA Abort callback to Null.
               No call back execution at end of DMA abort procedure */
            hlpuart->hdmatx->XferAbortCallback = NULL;
            
            /* abort dma transfer */
            if(HAL_DMA_Abort(hlpuart->hdmatx) == HAL_TIMEOUT)
            {
                return HAL_TIMEOUT;
            }
        }
    }
#endif   
    
    /* Reset Tx transfer counter */
    hlpuart->TxXferCount = 0U;
    
    /* end of sending */
    hlpuart->gState = HAL_LPUART_STATE_READY;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Abort ongoing reception transfer(block mode/interrupt mode/dma mode).
*         In blocking mode, check TxState to exit the abort function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_AbortReceive(LPUART_HandleTypeDef *hlpuart)
{
    /* disable interrupt (RXOV/FE/PE/RX) */
    __HAL_LPUART_DISABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX | \
                                     LPUART_IE_BCNTIE | LPUART_IE_IDLEIE);
   
#ifdef HAL_DMA_MODULE_ENABLED
    if (hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA)
    {
        /* Disable the LPUART DMA request */
        __HAL_LPUART_DISABLE_DMA(hlpuart);

        if (hlpuart->hdmarx != NULL)
        {
            /* Set the LPUART DMA Abort callback to Null.
               No call back execution at end of DMA abort procedure */
            hlpuart->hdmarx->XferAbortCallback = NULL;
            
            /* abort dma transfer */
            if(HAL_DMA_Abort(hlpuart->hdmarx) == HAL_TIMEOUT)
            {
                return HAL_TIMEOUT;
            }
        }
    }
#endif  
    
    /* Reset Tx transfer counter */
    hlpuart->TxXferCount = 0U;
    
    /* Clear the Error flags */
    __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXOVIF | LPUART_FLAG_FEIF | LPUART_FLAG_PEIF | LPUART_FLAG_RXIF);
    
    /* Restore huart->RxState to Ready */
    hlpuart->RxState = HAL_LPUART_STATE_READY;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Get sending status.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: sending status.
*         The return value can be @ ref LPUART_State value
*         @arg HAL_LPUART_STATE_READY: LPUART not sent.
*         @arg HAL_LPUART_STATE_BUSY_TX: LPUART sending(block mode or interrupt mode).
*         @arg HAL_LPUART_STATE_BUSY_TX_DMA: LPUART sending(DMA mode).
******************************************************************************/
uint32_t HAL_LPUART_GetTxState(LPUART_HandleTypeDef *hlpuart)
{
    return (hlpuart->gState);
}

/******************************************************************************
*@brief : Get the receiving status.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: receiving status.
*         The return value can be @ ref LPUART_State value
*         @arg HAL_LPUART_STATE_READY: LPUART did not receive.
*         @arg HAL_LPUART_STATE_BUSY_RX: LPUART is receiving(block mode or interrupt mode).
*         @arg HAL_LPUART_STATE_BUSY_RX_DMA: LPUART is receiving(DMA mode).
******************************************************************************/
uint32_t HAL_LPUART_GetRxState(LPUART_HandleTypeDef *hlpuart)
{
    return (hlpuart->RxState);
}

/******************************************************************************
*@brief : Return the LPUART handle error code.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: LPUART Error Code.
******************************************************************************/
uint32_t HAL_LPUART_GetErrorCode(LPUART_HandleTypeDef *hlpuart)
{
    return (hlpuart->ErrorCode);
}

/******************************************************************************
*@brief : Tx transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_TxCpltCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : Tx Half transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_TxHalfCpltCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : Rx transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_RxCpltCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : Rx transfer harf callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_RxHalfCpltCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : Error callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_ErrorCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
*@brief : Wakeup callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_WakeupCallback(LPUART_HandleTypeDef *hlpuart)
{
    UNUSED(hlpuart);
}

/******************************************************************************
* @brief  Reception Event Callback (Rx event notification called after use of advanced reception service).
* @param  hlpuart LPUART handle
* @param  size  Number of data available in application reception buffer (indicates a position in
*               reception buffer until which, data are available)
*@return: None.
******************************************************************************/
__weak void HAL_LPUART_RxEventCallback(LPUART_HandleTypeDef *hlpuart, uint16_t size)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hlpuart);
    UNUSED(size);
}

/******************************************************************************
* @brief : Initialize the LPUART wake-up from stop mode parameters.
* @param : hlpuart        LPUART handle.
* @param : WakeUpMethod   Specifies the wakeup method.
*                         This parameter can be a value of @ref LPUART_WakeupMethod.
*                           @arg LPUART_WAKEUPMETHOD_NONE
*                           @arg LPUART_WAKEUPMETHOD_STARTBIT
*                           @arg LPUART_WAKEUPMETHOD_ONEBYTENOCHECK
*                           @arg LPUART_WAKEUPMETHOD_ONEBYTECHECK
*                           @arg LPUART_WAKEUPMETHOD_ADDRNOCHECK
*                           @arg LPUART_WAKEUPMETHOD_ADDRCHECK
* @param : Address        Address value to wakeup on.
* @return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_LPUART_WakeUpConfig(LPUART_HandleTypeDef *hlpuart, uint32_t WakeUpMethod, uint8_t Address)
{
    /* Check the parameters */
    if (hlpuart == NULL)
    {
        return HAL_ERROR;
    }
    
    /* Check the parameters */
    assert_param(IS_LPUART_WAKEUPMETHOD(WakeUpMethod));
    
    /* Set the wakeup method */
    MODIFY_REG(hlpuart->Instance->LCR, LPUART_LCR_RXWKS | LPUART_LCR_WKCK, WakeUpMethod);
    
    /* Set the address matching for wakeup */
    if ((WakeUpMethod == LPUART_WAKEUPMETHOD_ADDRNOCHECK) || \
        (WakeUpMethod == LPUART_WAKEUPMETHOD_ADDRCHECK))
    {
        hlpuart->Instance->ADDR =  (uint32_t)Address;
    }
    
    if ((hlpuart->Instance->LCR & LPUART_LCR_RXWKS) == LPUART_LCR_RXWKS_START_BIT)
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_SR_STARTIF);
        
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_STARTIE);
    }
    else if((hlpuart->Instance->LCR & LPUART_LCR_RXWKS) == LPUART_LCR_RXWKS_ONE_BYTE)
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_SR_RXIF);
        
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_RXIE);
    }
    else if((hlpuart->Instance->LCR & LPUART_LCR_RXWKS) == LPUART_LCR_RXWKS_ADDR_MATCH)
    {
        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_SR_MATCHIF);
        
        SET_BIT(hlpuart->Instance->IE, LPUART_IE_MATCHIE);
    }
    
    __HAL_RCC_EXTI_CLK_ENABLE();
    
    if (hlpuart->Instance == LPUART1)
        HAL_EXTI_SetConfigLine(NULL, EXTI_LINE_LPUART1_WAKEUP, EXTI_MODE_IT_RISING);
    else
        HAL_EXTI_SetConfigLine(NULL, EXTI_LINE_LPUART2_WAKEUP, EXTI_MODE_IT_RISING);
    
    return HAL_OK;
}

#ifdef HAL_DMA_MODULE_ENABLED
/******************************************************************************
*@brief : DMA UART transmit process complete callback.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMATransmitCplt(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;
    
    /* DMA Normal mode */
    if (hdma->Init.Mode != DMA_MODE_CIRCULAR)
    {
        hlpuart->TxXferCount = 0U;

        /* Disable the DMA transfer for transmit request by resetting the DMA_EN bit
           in the LPUART CR register */
        __HAL_LPUART_DISABLE_DMA(hlpuart);

        /* Enable the LPUART Transmit Complete Interrupt */
        SET_BIT(hlpuart->Instance->IE, LPUART_IT_TC);
    }
    else /* DMA Circular mode */
    {
        /*Call weak Tx complete callback*/
        HAL_LPUART_TxCpltCallback(hlpuart);
    }
}

/******************************************************************************
*@brief : Tx DMA half transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMATxHalfCplt(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;
       
    /*Call weak Tx Half complete callback*/
    HAL_LPUART_TxHalfCpltCallback(hlpuart);
}

/******************************************************************************
*@brief : Rx DMA transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMAReceiveCplt(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;
    
    /* DMA Normal mode */
    if (hdma->Init.Mode != DMA_MODE_CIRCULAR)
    {
        hlpuart->RxXferCount = 0U;
        
        /* Disable error interrupts (RXOV/FE/PE) */
        __HAL_LPUART_DISABLE_IT(hlpuart, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE);
        
        /* Disable the DMA transfer for transmit request by resetting the DMA_EN bit
           in the LPUART CR register */
        CLEAR_BIT(hlpuart->Instance->CR, LPUART_CR_DMA_EN);
        
        hlpuart->RxState = HAL_LPUART_STATE_READY;
        
        /* If Reception till IDLE/BCNT event has been selected, Disable IDLE/BCNT Interrupt */
        CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_IDLE | LPUART_IT_BCNT);
    }

    /* Initialize type of RxEvent that correspond to RxEvent callback execution;
       In this case, Rx Event type is Transfer Complete */
    hlpuart->RxEventType = HAL_LPUART_RXEVENT_TC;

    /* Check current reception Mode :
       If Reception till IDLE/BCNT event has been selected : use Rx Event callback */
    if ((hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOIDLE) || 
        (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOBCNT))
    {
        /*Call weak Rx Event callback*/
        HAL_LPUART_RxEventCallback(hlpuart, hlpuart->RxXferSize);
    }
    else
    {
        /*Call weak Rx complete callback*/
        HAL_LPUART_RxCpltCallback(hlpuart);
    }
}

/******************************************************************************
*@brief : Rx DMA hlaf transfer complete callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMARxHalfCplt(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;

  /* Initialize type of RxEvent that correspond to RxEvent callback execution;
     In this case, Rx Event type is Half Transfer */
     hlpuart->RxEventType = HAL_LPUART_RXEVENT_HT;
   
    /* Check current reception Mode :
       If Reception till IDLE/BCNT event has been selected : use Rx Event callback */
    if ((hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOIDLE) || 
        (hlpuart->ReceptionType == HAL_LPUART_RECEPTION_TOBCNT))
    {
        /*Call weak Rx Event callback*/
        HAL_LPUART_RxEventCallback(hlpuart, hlpuart->RxXferSize / 2);
    }
    else
    {
        /*Call weak Rx complete callback*/
        HAL_LPUART_RxCpltCallback(hlpuart);
    }
}

/******************************************************************************
*@brief : Tx DMA transfer error callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMATxError(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;
   
    hlpuart->TxXferCount = 0U;
    
    CLEAR_BIT(hlpuart->Instance->IE, (LPUART_IT_TXE | LPUART_IT_TC));   
    
    /* End of reception */
    hlpuart->gState = HAL_LPUART_STATE_READY;

    hlpuart->ErrorCode |= HAL_LPUART_ERROR_DMA_TX;
    
    /*Call weak error callback*/
    HAL_LPUART_ErrorCallback(hlpuart);
}


/******************************************************************************
*@brief : Rx DMA transfer error callback function.
*@param : hlpuart: pointer to a LPUART_HandleTypeDef structure that contains
*                  the configuration information for LPUART module.
*@return: None.
******************************************************************************/
static void LPUART_DMARxError(DMA_HandleTypeDef *hdma)
{
    LPUART_HandleTypeDef *hlpuart = (LPUART_HandleTypeDef *)hdma->Parent;

    hlpuart->RxXferCount = 0U;
    
    /* Disable RX, ERR (Frame error, parity error, overrun error) interrupts */
    CLEAR_BIT(hlpuart->Instance->IE, (LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX)); 

    /* In case of reception waiting for IDLE/BCNT event, disable also the IDLE/BCNT IE interrupt source */
    CLEAR_BIT(hlpuart->Instance->IE, (LPUART_IT_IDLE | LPUART_IT_BCNT)); 
    
    /* At end of Rx process, restore hlpuart->RxState to Ready */
    hlpuart->RxState = HAL_LPUART_STATE_READY;
    hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;

    hlpuart->ErrorCode |= HAL_LPUART_ERROR_DMA_RX;
    
    HAL_LPUART_ErrorCallback(hlpuart);
}
#endif

/******************************************************************************
*@brief : RX interrupt handler for 7 or 8 bits data word length .
*@param : hlpuart UART handle.
*@return: None.
******************************************************************************/
static void LPUART_RxISR(LPUART_HandleTypeDef *hlpuart)
{
    /* Check that a Rx process is ongoing */
    if(hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX)
    {
        *hlpuart->pRxBuffPtr = (uint8_t) READ_REG(hlpuart->Instance->RXDR);
        hlpuart->pRxBuffPtr++;
        hlpuart->RxXferCount--;

        if(hlpuart->RxXferCount == 0U)
        {
            /* Disable the UART Error Interrupt, RX interrupts, and IDLE/BCNT interrupt */
            CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE |
                                             LPUART_IT_RX | LPUART_IT_IDLE | LPUART_IT_BCNT);

            /* Rx process is completed, restore hlpuart->RxState to Ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;
        }

        /* Receive completion (RXIF) callback function */
        HAL_LPUART_RxCpltCallback(hlpuart);
    }
    else
    {
//        /* Clear the receive completion interrupt flag */
//        __HAL_LPUART_CLEAR_FLAG(hlpuart, LPUART_FLAG_RXIF);
        
        volatile uint8_t tmp;
        /* Read data and clear the receive completion interrupt flag */
        tmp = READ_REG(hlpuart->Instance->RXDR);
    }
}

/******************************************************************************
*@brief : TX interrupt handler for 7 or 8 bits data word length .
*@param : hlpuart LPUART handle.
*@return: None.
******************************************************************************/
static void LPUART_TxISR(LPUART_HandleTypeDef *hlpuart)
{
    /* Check that a Tx process is ongoing */
    if (hlpuart->gState == HAL_LPUART_STATE_BUSY_TX)
    {
        if (hlpuart->TxXferCount == 0U)
        {
            /* Disable the UART Transmit Data Register Empty Interrupt */
            CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_TXE);

            /* Enable the UART Transmit Complete Interrupt */
            SET_BIT(hlpuart->Instance->IE, LPUART_IT_TC);
        }
        else
        {
            hlpuart->Instance->TXDR = (uint8_t)(*hlpuart->pTxBuffPtr);
            hlpuart->pTxBuffPtr++;
            hlpuart->TxXferCount--;
        }
    }
}

/******************************************************************************
*@brief : RX Event handler .
*@param : hlpuart LPUART handle.
*@param : EventType DMA event type.
*@return: None.
******************************************************************************/
static void LPUART_RxEvent(LPUART_HandleTypeDef *hlpuart, HAL_LPUART_RxEventTypeTypeDef EventType)
{
    /* Check if DMA mode is enabled in UART */
    if(hlpuart->RxState == HAL_LPUART_STATE_BUSY_RX_DMA)
    {
        /* DMA mode enabled */
        /* Check received length : If all expected data are received, do nothing,
            (DMA cplt callback will be called).
            Otherwise, if at least one data has already been received, IDLE event is to be notified to user */
        uint16_t nb_remaining_rx_data = (uint16_t) __HAL_DMA_GET_TRANSFER_SIZE(hlpuart->hdmarx);

        if ((nb_remaining_rx_data > 0U) && (nb_remaining_rx_data < hlpuart->RxXferSize))
        {
            /* Reception is not complete */
            hlpuart->RxXferCount = nb_remaining_rx_data;

            /* In Normal mode, end DMA xfer and HAL LPUART Rx process*/
            if (hlpuart->hdmarx->Init.Mode != DMA_MODE_CIRCULAR)
            {
                /* Disable ERR (Frame error, parity error, overrun error) interrupts */
                CLEAR_BIT(hlpuart->Instance->IE, (LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE));

                /* Disable the DMA transfer for the receiver request */
                __HAL_LPUART_ENABLE_DMA(hlpuart);

                /* At end of Rx process, restore hlpuart->RxState to Ready */
                hlpuart->RxState = HAL_LPUART_STATE_READY;
                hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;

                /* Disable IDLE/BCNT interrupts */
                if(EventType == HAL_LPUART_RXEVENT_IDLE)
                {
                    CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_IDLE);
                }
                else if(EventType == HAL_LPUART_RXEVENT_BCNT)
                {
                    CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_BCNT);
                }

                /* Last bytes received, so no need as the abort is immediate */
                (void)HAL_DMA_Abort(hlpuart->hdmarx);
            }
            
            /* Initialize type of RxEvent that correspond to RxEvent callback execution;
                In this case, Rx Event type is Idle Event */
            hlpuart->RxEventType = EventType;

            /*Call weak Rx Event callback*/
            HAL_LPUART_RxEventCallback(hlpuart, (hlpuart->RxXferSize - hlpuart->RxXferCount));
        }
        else
        {
            /* If DMA is in Circular mode, Idle event is to be reported to user
                even if occurring after a Transfer Complete event from DMA */
            if (nb_remaining_rx_data == hlpuart->RxXferSize)
            {
                if (hlpuart->hdmarx->Init.Mode == DMA_MODE_CIRCULAR)
                {
                    /* Initialize type of RxEvent that correspond to RxEvent callback execution;
                        In this case, Rx Event type is Idle Event */
                    hlpuart->RxEventType = EventType;

                    /*Call legacy weak Rx Event callback*/
                    HAL_LPUART_RxEventCallback(hlpuart, hlpuart->RxXferSize);
                }
            }
        }
        return;
    }
    else
    {
        /* DMA mode not enabled */
        /* Check received length : If all expected data are received, do nothing.
           Otherwise, if at least one data has already been received, IDLE event is to be notified to user */
        uint16_t nb_rx_data = hlpuart->RxXferSize - hlpuart->RxXferCount;
        if ((hlpuart->RxXferCount > 0U) && (nb_rx_data > 0U))
        {
            /* Disable ERR (Frame error, parity error, overrun error) interrupts and RX interrupts */
            CLEAR_BIT(hlpuart->Instance->IE, (LPUART_IT_RXOV | LPUART_IT_FE | LPUART_IT_PE | LPUART_IT_RX));

            /* Rx process is completed, restore hlpuart->RxState to Ready */
            hlpuart->RxState = HAL_LPUART_STATE_READY;
            hlpuart->ReceptionType = HAL_LPUART_RECEPTION_STANDARD;

            /* Clear RxISR function pointer */
            hlpuart->RxISR = NULL;

            /* Disable IDLE/BCNT interrupts */
            if(EventType == HAL_LPUART_RXEVENT_IDLE)
            {
                CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_IDLE);
            }
            else if(EventType == HAL_LPUART_RXEVENT_BCNT)
            {
                CLEAR_BIT(hlpuart->Instance->IE, LPUART_IT_BCNT);
            }

            /* Initialize type of RxEvent that correspond to RxEvent callback execution;
                In this case, Rx Event type is Idle Event */
            hlpuart->RxEventType = EventType;

            /*Call legacy weak Rx Event callback*/
            HAL_LPUART_RxEventCallback(hlpuart, nb_rx_data);
        }
        
        return;
    }
}


#endif /* HAL_LPUART_MODULE_ENABLED */

/************************ (C) COPYRIGHT AISINOCHIP *****END OF FILE****/

