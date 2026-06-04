
/******************************************************************************
*@file  : hal_i2s.c
*@brief : I2S HAL module driver.
******************************************************************************/

#include "hal.h" 

#ifdef HAL_I2S_MODULE_ENABLED

static void I2S_DMATxCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMATxHalfCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMARxCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMARxHalfCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMATxRxCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMATxRxHalfCplt(DMA_HandleTypeDef *hdma);
static void I2S_DMAError(DMA_HandleTypeDef *hdma);
static void I2S_Transmit_IT(I2S_HandleTypeDef *hi2s);
static void I2S_Receive_IT(I2S_HandleTypeDef *hi2s);
static HAL_StatusTypeDef I2S_WaitFlagStateUntilTimeout(I2S_HandleTypeDef *hi2s, uint32_t Flag, FlagStatus State,
                                                       uint32_t Tickstart, uint32_t Timeout);

/* Exported functions ---------------------------------------------------------*/

/******************************************************************************
*@brief : Initialize the I2S according to the specified.
*         parameters in the I2S_InitTypeDef and create the associated handle.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Init(I2S_HandleTypeDef *hi2s)
{
    double Freq;
    uint32_t div;
    
    if (hi2s == NULL)
    {
        return HAL_ERROR;
    }
    
    /* Check the parameters */
    assert_param(IS_I2S_HANDLE(hi2s));
    assert_param(IS_I2S_INSTANCE(hi2s->Instance));
    assert_param(IS_I2S_MODE(hi2s->Init.Mode));
    assert_param(IS_I2S_STANDARD(hi2s->Init.Standard));
    assert_param(IS_I2S_DATAFORMAT(hi2s->Init.DataFormat));
    assert_param(IS_I2S_MCLKOUTPUT(hi2s->Init.MCLKOutput));
    assert_param(IS_I2S_CLOCKPOLARITY(hi2s->Init.ClockPolarity));
    assert_param(IS_I2S_IOSWITCH(hi2s->Init.IOSwitch));
    
    assert_param(hi2s->Init.AudioFreq != 0u);
    
        /* Init the low level hardware : GPIO, CLOCK, CORTEX...etc */
        HAL_I2S_MspInit(hi2s);
      
    Freq = (double)(HAL_RCC_GetPCLK1Freq());
    if (hi2s->Init.MCLKOutput == I2S_MCLKOUT_DISABLE)
    {
        if (hi2s->Init.DataFormat == I2S_DATA_FORMAT_16B)
            Freq = Freq / 32;
        else
            Freq = Freq / 64;
    }
    else
    {
        Freq = Freq / 256;
    }
    assert_param(Freq >= (double)hi2s->Init.AudioFreq);
    Freq = Freq / (double)hi2s->Init.AudioFreq;
    Freq += 0.5;
    div = (uint32_t)Freq;
    if (div & 1u)
    {
        div = (div - 1) >> 1;
        assert_param(div <= 0x1FFU);
        div += I2S_PR_OF;
    }
    else
    {
        div = div >> 1;
        assert_param(div <= 0x1FFU);
    }
    
    hi2s->State = HAL_I2S_STATE_BUSY;
    
    /* Disable I2S peripheral */
    __HAL_I2S_DISABLE(hi2s);
     
    /*-------------------------- I2Sx I2S_CR Configuration --------------------*/
    hi2s->Instance->CR = hi2s->Init.Mode | hi2s->Init.Standard | hi2s->Init.DataFormat | \
                         hi2s->Init.ClockPolarity | hi2s->Init.IOSwitch;
    
    hi2s->Instance->PR = hi2s->Init.MCLKOutput | div;
    
    /* Enable I2S peripheral */
    __HAL_I2S_ENABLE(hi2s);
    
    hi2s->ErrorCode = HAL_I2S_ERROR_NONE;
    hi2s->State     = HAL_I2S_STATE_READY;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : DeInitialize the I2S peripheral.
*         Before Deinitialization, the sending or receiving process needs to be aborted.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_DeInit(I2S_HandleTypeDef *hi2s)
{
    /* Check the parameters */
    assert_param(IS_I2S_HANDLE(hi2s));
    assert_param(IS_I2S_INSTANCE(hi2s->Instance));
    
    hi2s->State = HAL_I2S_STATE_BUSY;
    
    /* Disable I2S peripheral */
    __HAL_I2S_DISABLE(hi2s);
    
    /* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
    HAL_I2S_MspDeInit(hi2s);
    
    hi2s->ErrorCode = HAL_I2S_ERROR_NONE;
    hi2s->State     = HAL_I2S_STATE_RESET;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Initialize the I2S MSP.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: None
******************************************************************************/
__weak void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s)
{
    UNUSED(hi2s);
}

/******************************************************************************
*@brief : DeInitialize the I2S MSP.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: None
******************************************************************************/
__weak void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s)
{
    UNUSED(hi2s);
}

/******************************************************************************
*@brief : Send an amount of data in blocking mode.
*@note    In blocking mode, the user cannot enable interrupt.
*@note    Users can add or ignore error handling in TxErrorCallback(). 
*         Error codes are stored in hi2s->TxError.
*@note    The last word was being sent when the function exited. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be sent.
*@param : timeout: Timeout duration.
*                  The minimum value of this parameter is 1.
*                  If the value of this parameter is 0xFFFFFFFF, it will be sent until all data are sent.
*                  @node The timeout should be greater than the time of all data transfers.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Transmit(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size, uint32_t Timeout)
{
  uint32_t tickstart;

    if ((pData == NULL) || (Size == 0UL))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }

    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_TX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pTxBuffPtr  = pData;
    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;

    /* Initialize fields not used in handle to zero */
    hi2s->pRxBuffPtr  = NULL;
    hi2s->RxXferSize  = (uint16_t) 0UL;
    hi2s->RxXferCount = (uint16_t) 0UL;

    /* Enable Tx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN);

    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);

    /* Wait until TXE flag is set */
    if (I2S_WaitFlagStateUntilTimeout(hi2s, I2S_FLAG_TXE, SET, tickstart, Timeout) != HAL_OK)
    {
        /* Disable Tx */
        CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);
        /* Set the error code */
        SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
        hi2s->State = HAL_I2S_STATE_READY;

        return HAL_TIMEOUT;
    }

    while (hi2s->TxXferCount > 0UL)
    {     
        /* Transmit data */
        hi2s->Instance->TXDR = *hi2s->pTxBuffPtr;
        hi2s->pTxBuffPtr++;
        hi2s->TxXferCount--;

        /* Wait until TXE flag is set */
        if (I2S_WaitFlagStateUntilTimeout(hi2s, I2S_FLAG_TXE, SET, tickstart, Timeout) != HAL_OK)
        {
            /* Disable Tx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);
            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
            hi2s->State = HAL_I2S_STATE_READY;

            return HAL_TIMEOUT;
        }

        /* Check if an underrun occurs */
        if (__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_UDR) == SET)
        {
            /* Clear underrun flag */
            __HAL_I2S_CLEAR_UDRFLAG(hi2s);

            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_UDR);
        }
    }
  
    /* Wait for the data to be transmitted onto the bus. */
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        if (I2S_WaitFlagStateUntilTimeout(hi2s, I2S_FLAG_MSUSP, SET, tickstart, Timeout) != HAL_OK)
        {
            /* Disable Tx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);
            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
            hi2s->State = HAL_I2S_STATE_READY;

            return HAL_TIMEOUT;
        }

        __HAL_I2S_CLEAR_MSUSPFLAG(hi2s);
    }
    else
    {
        if (I2S_WaitFlagStateUntilTimeout(hi2s, I2S_FLAG_SVTC, SET, tickstart, Timeout) != HAL_OK)
        {
            /* Disable Tx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);
            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
            hi2s->State = HAL_I2S_STATE_READY;

            return HAL_TIMEOUT;
        }

        __HAL_I2S_CLEAR_SVTCFLAG(hi2s);
    }
  
    /* Disable Tx */
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);

    hi2s->State = HAL_I2S_STATE_READY;

    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in blocking mode.
*@note    In blocking mode, the user cannot enable interrupt.
*@note    Users can add or ignore error handling in RxErrorCallback(). 
*         Error codes are stored in hi2s->RxError.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@param : timeout: Timeout duration.
*                  If the value of this parameter is 0, the received data will be detected only once and will not wait.
*                  If the value of this parameter is 0xFFFFFFFF, it will be received until all data are received.
*                  @node The timeout should be greater than the time of all data transfers.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Receive(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size, uint32_t Timeout)
{
    uint32_t tickstart;

    if ((pData == NULL) || (Size == 0UL))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }

    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_RX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pRxBuffPtr  = pData;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;

    /* Initialize fields not used in handle to zero */
    hi2s->pTxBuffPtr  = NULL;
    hi2s->TxXferSize  = (uint16_t) 0UL;
    hi2s->TxXferCount = (uint16_t) 0UL;
  
    /* Enable Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_REN);
  
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }

    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
  
    /* Receive data */
    while (hi2s->RxXferCount > 0UL)
    {
        /* Wait until RXNE flag is set */
        if (I2S_WaitFlagStateUntilTimeout(hi2s, I2S_FLAG_RXNE, SET, tickstart, Timeout) != HAL_OK)
        {
            /* Disable Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_REN);
            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
            hi2s->State = HAL_I2S_STATE_READY;
            //__HAL_UNLOCK(hi2s);
            return HAL_TIMEOUT;
        }
    
        /* Receive data */
        *hi2s->pRxBuffPtr = hi2s->Instance->RXDR;
        hi2s->pRxBuffPtr++;
        hi2s->RxXferCount--;

        /* Check if an overrun occurs */
        if (__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_OVR) == SET)
        {
            /* Clear overrun flag */
            __HAL_I2S_CLEAR_OVRFLAG(hi2s);

            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_UDR);
        }
    }
    /* Disable Rx */
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_REN);

    hi2s->State = HAL_I2S_STATE_READY;

    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in blocking mode.
*@note    In blocking mode, the user cannot enable interrupt.
*@note    Users can add or ignore error handling in TxErrorCallback(). 
*         Error codes are stored in hi2s->TxError.
*@note    The last word was being sent when the function exited. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be sent.
*@param : timeout: Timeout duration.
*                  The minimum value of this parameter is 1.
*                  If the value of this parameter is 0xFFFFFFFF, it will be sent until all data are sent.
*                  @node The timeout should be greater than the time of all data transfers.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_TransmitReceive(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                          uint16_t Size, uint32_t Timeout)
{
    uint32_t tmp_TxXferCount;
    uint32_t tmp_RxXferCount;
    uint32_t tickstart;

    if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }
      
    /* Init tickstart for timeout management*/
    tickstart = HAL_GetTick();

    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;
    hi2s->pTxBuffPtr  = pTxData;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;
    hi2s->pRxBuffPtr  = pRxData;

    tmp_TxXferCount = hi2s->TxXferCount;
    tmp_RxXferCount = hi2s->RxXferCount;

    /* Set state and reset error code */
    hi2s->ErrorCode = HAL_I2S_ERROR_NONE;
    hi2s->State = HAL_I2S_STATE_BUSY_TX_RX;
    
    /* Enable Tx and Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
    
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }
    
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);

    while ((tmp_TxXferCount > 0UL) || (tmp_RxXferCount > 0UL))
    {
        if ((__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_TXE) == SET) && (tmp_TxXferCount != 0UL))
        {
            /* Transmit data */
            hi2s->Instance->TXDR = *hi2s->pTxBuffPtr;
            hi2s->pTxBuffPtr++;
            tmp_TxXferCount--;
            
            /* Check if an underrun occurs */
            if (__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_UDR) == SET)
            {
                /* Clear underrun flag */
                __HAL_I2S_CLEAR_UDRFLAG(hi2s);

                /* Set the error code */
                SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_UDR);
            }
        }
        
        if ((__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_RXNE) == SET) && (tmp_RxXferCount != 0UL))
        {
            /* Receive data in 32 Bit mode */
            *hi2s->pRxBuffPtr = hi2s->Instance->RXDR;
            hi2s->pRxBuffPtr++;
            tmp_RxXferCount--;
            
            /* Check if an overrun occurs */
            if (__HAL_I2S_GET_FLAG(hi2s, I2S_FLAG_OVR) == SET)
            {
                /* Clear overrun flag */
                __HAL_I2S_CLEAR_OVRFLAG(hi2s);

                /* Set the error code */
                SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_OVR);
            }
        }
        
        /* Timeout management */
        if ((((HAL_GetTick() - tickstart) >= Timeout) && (Timeout != HAL_MAX_DELAY)) || (Timeout == 0U))
        {
            /* Disable Tx and Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
            /* Set the error code */
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_TIMEOUT);
            hi2s->State = HAL_I2S_STATE_READY;
            return HAL_TIMEOUT;
        }
    }
 
    /* Disable Tx and Rx */
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
    
    hi2s->State = HAL_I2S_STATE_READY;
    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in interrupt mode.
*@note    In interrupt mode, the transmission related interrupts (TXE/ERR) are forced enabled.
*@note    Users can add or ignore error handling in ErrorCallback(). 
*         Error codes are stored in hi2s->TxError.
*@note    When the callback function TxCpltCallback() is called, the last word is being sent. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Transmit_IT(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size)
{
    if ((pData == NULL) || (Size == 0UL))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }
     
    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_TX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pTxBuffPtr  = pData;
    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;

    /* Initialize fields not used in handle to zero */
    hi2s->pRxBuffPtr  = NULL;
    hi2s->RxXferSize  = (uint16_t) 0UL;
    hi2s->RxXferCount = (uint16_t) 0UL;
    
    /* Enable Tx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN);
    
    /* Enable TXE and ERR interrupt */
    __HAL_I2S_ENABLE_IT(hi2s, (I2S_IT_TXE | I2S_IT_ERR));
       
    if (hi2s->Instance->CR & I2S_CR_MODE) //Master
    {
        __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_MSUSP);
    }
    else
    {
        __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_SVTC);
    }
    
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in interrupt mode.
*@note    In interrupt mode, receiving completion interrupt (RXNE/ERR) are forced enabled.
*         If an error occurs in the reception, the reception stops automatically.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Receive_IT(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size)
{
    if ((pData == NULL) || (Size == 0UL))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }
    
    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_RX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pRxBuffPtr  = pData;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;

    /* Initialize fields not used in handle to zero */
    hi2s->pTxBuffPtr  = NULL;
    hi2s->TxXferSize  = (uint16_t) 0UL;
    hi2s->TxXferCount = (uint16_t) 0UL;
    
    /* Enable Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_REN);
    
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }
    
    /* Enable RXNE and ERR interrupt */    
    __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_RXNE | I2S_IT_ERR);
    
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in interrupt mode.
*@note    In interrupt mode, the transmission related interrupts (TXE/ERR) are forced enabled.
*@note    Users can add or ignore error handling in ErrorCallback(). 
*         Error codes are stored in hi2s->TxError.
*@note    When the callback function TxCpltCallback() is called, the last word is being sent. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_TransmitReceive_IT(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                             uint16_t Size)
{
    if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U))
    {
    return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
    return HAL_BUSY;
    }

    hi2s->pTxBuffPtr  = pTxData;
    hi2s->pRxBuffPtr  = pRxData;

    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;

    hi2s->ErrorCode = HAL_I2S_ERROR_NONE;
    hi2s->State     = HAL_I2S_STATE_BUSY_TX_RX;
    
    /* Enable Tx and Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
    
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }
    
    /* enable interrupt (TXE/RXNE/ERR) */
    __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR);
    
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Send an amount of data in DMA mode.
*@note    In DMA mode, the tx buffer empty interrupt (TXE) is forced to disabled.
*         the error interrupt (ERR) is forced to enabled.
*         the DMA transmission related interrupt (ITC/HFTC/IE) is forced to enabled.
*         If an DMA transfer error occurs in the transmission, 
*         the transmission stops automatically.
*@note    When the callback function TxCpltCallback() is called, the last word is being sent. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size)
{
    if ((pData == NULL) || (Size == 0UL))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }
    
    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_TX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pTxBuffPtr  = pData;
    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;

    /* Init field not used in handle to zero */
    hi2s->pRxBuffPtr  = NULL;
    hi2s->RxXferSize  = (uint16_t)0UL;
    hi2s->RxXferCount = (uint16_t)0UL;

    /* Set the I2S Tx DMA Half transfer complete callback */
    hi2s->hdmatx->XferHalfCpltCallback = I2S_DMATxHalfCplt;

    /* Set the I2S Tx DMA transfer complete callback */
    hi2s->hdmatx->XferCpltCallback = I2S_DMATxCplt;

    /* Set the DMA error callback */
    hi2s->hdmatx->XferErrorCallback = I2S_DMAError;
     
    /* Enable the Tx DMA Stream/Channel */
    if (HAL_OK != HAL_DMA_Start_IT(hi2s->hdmatx, (uint32_t)hi2s->pTxBuffPtr, (uint32_t)&hi2s->Instance->TXDR,
                                 hi2s->TxXferCount))
    {
        /* Update I2S error code */
        SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
        hi2s->State = HAL_I2S_STATE_READY;

        return HAL_ERROR;
    }
    
    if (hi2s->Instance->CR & I2S_CR_MODE) //Master
    {
        __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_MSUSP);
    }
    else
    {
        __HAL_I2S_ENABLE_IT(hi2s, I2S_IT_SVTC);
    }
    
    /* Enable Tx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN);
    
    /* Enable Tx DMA */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TXDMAEN);
    
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
   
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data in DMA mode.
*@note    In DMA mode, rx buffer non empty interrupt (RXNE) is forced disable. 
*         the error interrupt (ERR) is forced enabled.
*         the DMA transmission related interrupt (ITC/HFTC/IE) is forced to enabled.
*         If an DMA transfer error occurs in the reception, 
*         the reception stops automatically.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_Receive_DMA(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size)
{
    if ((pData == NULL) || (Size == 0UL))
    {
        return HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }

    /* Set state and reset error code */
    hi2s->State       = HAL_I2S_STATE_BUSY_RX;
    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->pRxBuffPtr  = pData;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;

    /* Init field not used in handle to zero */
    hi2s->pTxBuffPtr  = NULL;
    hi2s->TxXferSize  = (uint16_t)0UL;
    hi2s->TxXferCount = (uint16_t)0UL;

    /* Set the I2S Rx DMA Half transfer complete callback */
    hi2s->hdmarx->XferHalfCpltCallback = I2S_DMARxHalfCplt;

    /* Set the I2S Rx DMA transfer complete callback */
    hi2s->hdmarx->XferCpltCallback = I2S_DMARxCplt;

    /* Set the DMA error callback */
    hi2s->hdmarx->XferErrorCallback = I2S_DMAError;
    
    /* Enable the Rx DMA Stream/Channel */
    if (HAL_OK != HAL_DMA_Start_IT(hi2s->hdmarx, (uint32_t)&hi2s->Instance->RXDR, (uint32_t)hi2s->pRxBuffPtr,
                                 hi2s->RxXferCount))
    {
        /* Update I2S error code */
        SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
        hi2s->State = HAL_I2S_STATE_READY;

        return HAL_ERROR;
    }
    
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }
    
    /* Enable Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_REN);
    
    /* Enable Rx DMA */
    SET_BIT(hi2s->Instance->CR, I2S_CR_RXDMAEN);
      
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
    
    return HAL_OK;
}
/******************************************************************************
*@brief : Send an amount of data in DMA mode.
*@note    In DMA mode, the tx buffer empty interrupt (TXE) is forced to disabled.
*         the error interrupt (ERR) is forced to enabled.
*         the DMA transmission related interrupt (ITC/HFTC/IE) is forced to enabled.
*         If an DMA transfer error occurs in the transmission, 
*         the transmission stops automatically.
*@note    When the callback function TxCpltCallback() is called, the last word is being sent. 
*         The application also needs to call the HAL_I2S_GetBusyStatus() function 
*         to detect the end of sending.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@param : pdata: Pointer to data buffer.
*@param : size: Amount of data elements to be received.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_TransmitReceive_DMA(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                              uint16_t Size)
{
    if ((pTxData == NULL) || (pRxData == NULL) || (Size == 0U))
    {
        return  HAL_ERROR;
    }

    if (hi2s->State != HAL_I2S_STATE_READY)
    {
        return HAL_BUSY;
    }

    hi2s->pTxBuffPtr = pTxData;
    hi2s->pRxBuffPtr = pRxData;

    hi2s->TxXferSize  = Size;
    hi2s->TxXferCount = Size;
    hi2s->RxXferSize  = Size;
    hi2s->RxXferCount = Size;

    hi2s->ErrorCode   = HAL_I2S_ERROR_NONE;
    hi2s->State       = HAL_I2S_STATE_BUSY_TX_RX;
    
    /* Reset the Tx/Rx DMA bits */
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TXDMAEN | I2S_CR_RXDMAEN);

    /* Set the I2S Rx and Tx DMA Half transfer complete callback */
    hi2s->hdmarx->XferHalfCpltCallback = I2S_DMATxRxHalfCplt;
    hi2s->hdmatx->XferHalfCpltCallback = NULL;

    /* Set the I2S Rx and Tx DMA transfer complete callback */
    hi2s->hdmarx->XferCpltCallback  = I2S_DMATxRxCplt;
    hi2s->hdmatx->XferCpltCallback  = NULL;

    /* Set the I2S Rx and Tx DMA error callback */
    hi2s->hdmarx->XferErrorCallback = I2S_DMAError;
    hi2s->hdmatx->XferErrorCallback = I2S_DMAError;
    
    /* Enable the Tx DMA Stream/Channel */
    if (HAL_OK != HAL_DMA_Start_IT(hi2s->hdmatx, (uint32_t)hi2s->pTxBuffPtr, (uint32_t)&hi2s->Instance->TXDR,
                                 hi2s->TxXferCount))
    {
        /* Update I2S error code */
        SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
        hi2s->State = HAL_I2S_STATE_READY;

        return HAL_ERROR;
    }
    
    /* Enable the Rx DMA Stream/Channel */
    if (HAL_OK != HAL_DMA_Start_IT(hi2s->hdmarx, (uint32_t)&hi2s->Instance->RXDR, (uint32_t)hi2s->pRxBuffPtr,
                                 hi2s->RxXferCount))
    {
        /* Update I2S error code */
        SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
        hi2s->State = HAL_I2S_STATE_READY;

        return HAL_ERROR;
    }
        
    if(READ_BIT(hi2s->Instance->CR, I2S_CR_MODE) && (hi2s->hdmarx->Init.Mode == DMA_MODE_NORMAL)) //Master
    {
        hi2s->Instance->RSIZE = hi2s->RxXferCount;
    }
    else
    {
        hi2s->Instance->RSIZE = 0;
    }
    
    /* Enable Tx and Rx */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
    
	/* Enable Tx and Rx DMA */
    SET_BIT(hi2s->Instance->CR, I2S_CR_TXDMAEN | I2S_CR_RXDMAEN);
      
    /* Start the transfer */
    SET_BIT(hi2s->Instance->CR, I2S_CR_START);
    
    return HAL_OK;
}

/******************************************************************************
  * @brief  Stops the audio DMA Stream/Channel playing from the Media.
  * @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
  *         the configuration information for I2S module
  * @retval HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_I2S_DMAStop(I2S_HandleTypeDef *hi2s)
{
    HAL_StatusTypeDef errorcode = HAL_OK;

    /* Disable the I2S Tx/Rx DMA requests */
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TXDMAEN);
    CLEAR_BIT(hi2s->Instance->CR, I2S_CR_RXDMAEN);

    /* Abort the I2S DMA tx Stream/Channel */
    if (hi2s->hdmatx != NULL)
    {
        /* Disable the I2S DMA tx Stream/Channel */
        if (HAL_OK != HAL_DMA_Abort(hi2s->hdmatx))
        {
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
            errorcode = HAL_ERROR;
        }
    }

    /* Abort the I2S DMA rx Stream/Channel */
    if (hi2s->hdmarx != NULL)
    {
        /* Disable the I2S DMA rx Stream/Channel */
        if (HAL_OK != HAL_DMA_Abort(hi2s->hdmarx))
        {
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
            errorcode = HAL_ERROR;
        }
    }
    
    __HAL_I2S_DISABLE_IT(hi2s, I2S_IT_MSUSP | I2S_IT_SVTC);
    
    hi2s->Instance->CR &= ~(I2S_CR_TEN | I2S_CR_REN);

    hi2s->State = HAL_I2S_STATE_READY;

    return errorcode;
}

/******************************************************************************
*@brief : Get the I2S state
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: HAL state
******************************************************************************/
HAL_I2S_StateTypeDef HAL_I2S_GetState(const I2S_HandleTypeDef *hi2s)
{
    return hi2s->State;
}

/******************************************************************************
*@brief : Get the I2S error code.
*@param : hi2s: pointer to a I2S_HandleTypeDef structure that contains
*               the configuration information for I2S module.
*@return: I2S Error Code
******************************************************************************/
uint32_t HAL_I2S_GetError(const I2S_HandleTypeDef *hi2s)
{
    return hi2s->ErrorCode;
}

/******************************************************************************
* @brief  This function handles I2S interrupt request.
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
void HAL_I2S_IRQHandler(I2S_HandleTypeDef *hi2s)
{
    uint32_t i2sier   = hi2s->Instance->IER;
    uint32_t i2ssr    = hi2s->Instance->SR;
    uint32_t trigger  = i2sier & i2ssr;

    if (hi2s->State == HAL_I2S_STATE_BUSY_RX)
    {
        /* I2S in mode Receiver ------------------------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_RXNE) && HAL_IS_BIT_CLR(trigger, I2S_FLAG_OVR))
        {
            I2S_Receive_IT(hi2s);
        }
        
        /* I2S Overrun error interrupt occurred -------------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_OVR))
        {
            /* Disable RXNE and ERR interrupt */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_RXNE | I2S_IT_ERR));

            /* Clear Overrun flag */
            __HAL_I2S_CLEAR_OVRFLAG(hi2s);
            
            /* Disable Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_REN);

            /* Set the I2S State ready */
            hi2s->State = HAL_I2S_STATE_READY;

            /* Set the error code and execute error callback*/
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_OVR);
            /* Call user error callback */
            HAL_I2S_ErrorCallback(hi2s);
        }
    }
    else if (hi2s->State == HAL_I2S_STATE_BUSY_TX)
    {
        /* I2S in mode Transmitter -----------------------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_TXE) && HAL_IS_BIT_CLR(trigger, I2S_FLAG_UDR))
        {
            I2S_Transmit_IT(hi2s);
        }
        
        if ((I2S_FLAG_MSUSP | I2S_FLAG_SVTC) & trigger)
        {
            /* Disable Tx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);
            
            /* Disable MSUSP and SVTC interrupt */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_MSUSP | I2S_IT_SVTC));
            
            /* Clear MSUSP or SVTC flag */
            CLEAR_BIT(hi2s->Instance->SR, (I2S_FLAG_MSUSP | I2S_FLAG_SVTC) & trigger);            
            
            hi2s->State = HAL_I2S_STATE_READY;
            
            HAL_I2S_TxCpltCallback(hi2s);
        }
        
        /* I2S Underrun error interrupt occurred --------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_UDR))
        {
            /* Disable TXP and ERR interrupt */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_TXE | I2S_IT_MSUSP | I2S_IT_SVTC | I2S_IT_ERR));

            /* Clear Underrun flag */
            __HAL_I2S_CLEAR_UDRFLAG(hi2s);
            
            /* Disable Tx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN);

            /* Set the I2S State ready */
            hi2s->State = HAL_I2S_STATE_READY;

            /* Set the error code and execute error callback*/
            SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_UDR);
            /* Call user error callback */
            HAL_I2S_ErrorCallback(hi2s);
        }
    }
    else if (hi2s->State == HAL_I2S_STATE_BUSY_TX_RX)
    {
        /* I2S in mode Transmitter -----------------------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_TXE))
        {
            I2S_Transmit_IT(hi2s);
        }
        
        /* I2S in mode Receiver -----------------------------------------------*/
        if (HAL_IS_BIT_SET(trigger, I2S_FLAG_TXE))
        {
            I2S_Receive_IT(hi2s);
        }

        /* I2S Underrun and/or Overrun error interrupt occurred --------------------------------*/
        if ((I2S_FLAG_UDR | I2S_FLAG_OVR) & trigger)
        {
            /* Disable TXE, RXNE and ERR interrupt */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR));

            if(READ_BIT(hi2s->Instance->SR, I2S_SR_UDR))
            {
                /* Clear Underrun flag  */
                __HAL_I2S_CLEAR_UDRFLAG(hi2s);
                    
                /* Set the error code and execute error callback*/
                SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_UDR);
            }
            
            if(READ_BIT(hi2s->Instance->SR, I2S_SR_OVR))
            {
                /* Clear Overrun flag  */
                __HAL_I2S_CLEAR_OVRFLAG(hi2s);
                    
                /* Set the error code and execute error callback*/
                SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_OVR);
            }
            
            /* Disable Tx and Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);

            /* Set the I2S State ready */
            hi2s->State = HAL_I2S_STATE_READY;

            /* Call user error callback */
            HAL_I2S_ErrorCallback(hi2s);
        }
    }
}

/******************************************************************************
* @brief  Tx Transfer Half completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  Tx Transfer completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  Rx Transfer half completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  Rx Transfer completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  Rx Transfer half completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  Rx Transfer completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_TxRxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  I2S error callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  DMA Tx Transfer completed callbacks
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
__weak void HAL_I2S_DMATxCpltCallback(I2S_HandleTypeDef *hi2s)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hi2s);
}

/******************************************************************************
* @brief  DMA I2S transmit process complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*                the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMATxCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* if DMA is configured in NORMAL Mode */
    if (hdma->Init.Mode == DMA_MODE_NORMAL)
    {
        /* Disable Tx DMA Request */
        CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TXDMAEN);
        
        hi2s->TxXferCount = (uint16_t)0UL;
    }
    
    HAL_I2S_DMATxCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S transmit process half complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*         the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMATxHalfCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* Call user Tx half complete callback */
    HAL_I2S_TxHalfCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S receive process complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*         the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMARxCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* if DMA is configured in NORMAL Mode */
    if (hdma->Init.Mode == DMA_MODE_NORMAL)
    {
        /* Disable Rx DMA Request */
        CLEAR_BIT(hi2s->Instance->CR, I2S_CR_RXDMAEN);
        
        /* Disable Rx */
        CLEAR_BIT(hi2s->Instance->CR, I2S_CR_REN);
        
        hi2s->RxXferCount = (uint16_t)0UL;
        hi2s->State = HAL_I2S_STATE_READY;
    }

    /* Call user Rx complete callback */
    HAL_I2S_RxCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S receive process half complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*         the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMARxHalfCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* Call user Rx half complete callback */
    HAL_I2S_RxHalfCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S transmit receive process complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*               the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMATxRxCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* if DMA is configured in NORMAL Mode */
    if (hdma->Init.Mode == DMA_MODE_NORMAL)
    {
        /* Disable Rx and Tx DMA Request */
        CLEAR_BIT(hi2s->Instance->CR, (I2S_CR_RXDMAEN | I2S_CR_TXDMAEN));
        
        /* Disable Tx and Rx */
        CLEAR_BIT(hi2s->Instance->CR, (I2S_CR_TEN | I2S_CR_REN));
        
        __HAL_I2S_CLEAR_MSUSPFLAG(hi2s);
        __HAL_I2S_CLEAR_SVTCFLAG(hi2s);
        
        hi2s->TxXferCount = (uint16_t) 0UL;
        hi2s->RxXferCount = (uint16_t) 0UL;
        
        /* Updated HAL State */
        hi2s->State = HAL_I2S_STATE_READY;
    }

    /* Call user Tx Rx complete callback */
    HAL_I2S_TxRxCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S transmit receive process half complete callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*               the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMATxRxHalfCplt(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* Call user TxRx Half complete callback */
    HAL_I2S_TxRxHalfCpltCallback(hi2s);
}

/******************************************************************************
* @brief  DMA I2S communication error callback
* @param  hdma pointer to a DMA_HandleTypeDef structure that contains
*         the configuration information for the specified DMA module.
* @retval None
******************************************************************************/
static void I2S_DMAError(DMA_HandleTypeDef *hdma)
{
    I2S_HandleTypeDef *hi2s = (I2S_HandleTypeDef *)((DMA_HandleTypeDef *)hdma)->Parent;

    /* Disable Rx and Tx DMA Request */
    CLEAR_BIT(hi2s->Instance->CR, (I2S_CR_RXDMAEN | I2S_CR_TXDMAEN));
    
    /* Disable Tx and Rx */
    CLEAR_BIT(hi2s->Instance->CR, (I2S_CR_TEN | I2S_CR_REN));
    
    hi2s->TxXferCount = (uint16_t) 0UL;
    hi2s->RxXferCount = (uint16_t) 0UL;

    hi2s->State = HAL_I2S_STATE_READY;

    /* Set the error code and execute error callback*/
    SET_BIT(hi2s->ErrorCode, HAL_I2S_ERROR_DMA);
    /* Call user error callback */
    HAL_I2S_ErrorCallback(hi2s);
}

/******************************************************************************
* @brief  Manage the transmission 16-bit in Interrupt context
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
static void I2S_Transmit_IT(I2S_HandleTypeDef *hi2s)
{
    /* Transmit data */
    hi2s->Instance->TXDR = *hi2s->pTxBuffPtr;
    hi2s->pTxBuffPtr++;
    hi2s->TxXferCount--;

    if (hi2s->TxXferCount == 0UL)
    {
        /* Disable TXE, ERR interrupts */
        __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_TXE | I2S_IT_ERR));
    }
}

/******************************************************************************
* @brief  Manage the reception 32-bit in Interrupt context
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @retval None
******************************************************************************/
static void I2S_Receive_IT(I2S_HandleTypeDef *hi2s)
{
    /* Receive data */
    *((uint32_t *)hi2s->pRxBuffPtr) = hi2s->Instance->RXDR;
    hi2s->pRxBuffPtr++;
    hi2s->RxXferCount--;

    if (hi2s->RxXferCount == 0UL)
    {
        if ((hi2s->Instance->CR & (I2S_CR_TEN | I2S_CR_REN)) == (I2S_CR_TEN | I2S_CR_REN)) //Fullduplex
        {
            /* Disable TXP, RXP, DXP, ERR interrupts */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR));
            
            /* Disable Tx and Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_TEN | I2S_CR_REN);
            
            hi2s->State = HAL_I2S_STATE_READY;
            
            HAL_I2S_TxRxCpltCallback(hi2s);
        }
        else
        {
            /* Disable RXP and ERR interrupt */
            __HAL_I2S_DISABLE_IT(hi2s, (I2S_IT_RXNE | I2S_IT_ERR));
            
            /* Disable Rx */
            CLEAR_BIT(hi2s->Instance->CR, I2S_CR_REN);
            
            hi2s->State = HAL_I2S_STATE_READY;
            
            HAL_I2S_RxCpltCallback(hi2s);
        }
    }
}

/******************************************************************************
* @brief  This function handles I2S Communication Timeout.
* @param  hi2s pointer to a I2S_HandleTypeDef structure that contains
*         the configuration information for I2S module
* @param  Flag Flag checked
* @param  State Value of the flag expected
* @param  Tickstart Tick start value
* @param  Timeout Duration of the timeout
* @retval HAL status
******************************************************************************/
static HAL_StatusTypeDef I2S_WaitFlagStateUntilTimeout(I2S_HandleTypeDef *hi2s, uint32_t Flag, FlagStatus State,
                                                       uint32_t Tickstart, uint32_t Timeout)
{
    /* Wait until flag is set to status*/
    while (((__HAL_I2S_GET_FLAG(hi2s, Flag)) ? SET : RESET) != State)
    {
        if (Timeout != HAL_MAX_DELAY)
        {
            if (((HAL_GetTick() - Tickstart) >= Timeout) || (Timeout == 0UL))
            {
                /* Set the I2S State ready */
                hi2s->State = HAL_I2S_STATE_READY;

                return HAL_TIMEOUT;
            }
        }
    }
    return HAL_OK;
}

#endif /* HAL_I2S_MODULE_ENABLED */
