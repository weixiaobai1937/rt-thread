
/******************************************************************************
*@file  : hal_i2s.h
*@brief : Header file of I2S HAL module.
******************************************************************************/

#ifndef __HAL_I2S_H__
#define __HAL_I2S_H__

/* Includes ------------------------------------------------------------------*/
#include "hal.h"

/* Exported types ------------------------------------------------------------*/
/**
  * @brief I2S Init structure definition
  */
typedef struct
{
    uint32_t Mode;                          /*!< Specifies the I2S operating mode.
                                                 This parameter can be a value of @ref I2S_Mode              */
    uint32_t Standard;                      /*!< Specifies the standard used for the I2S communication.
                                                 This parameter can be a value of @ref I2S_Standard          */
    uint32_t DataFormat;                    /*!< Specifies the data format for the I2S communication.
                                                 This parameter can be a value of @ref I2S_DataFormat        */
    uint32_t MCLKOutput;                    /*!< Specifies whether the I2S MCLK output is enabled or not.
                                                 This parameter can be a value of @ref I2S_MCLK_Output       */
    uint32_t ClockPolarity;                 /*!< Specifies the idle state of the I2S clock.
                                                 This parameter can be a value of @ref I2S_ClockPolarity     */
    uint32_t IOSwitch;                      /*!< Specifies the idle state of the I2S clock.
                                                 This parameter can be a value of @ref I2S_IOSwitch          */
    uint32_t AudioFreq;                    /*!<  Specifies the frequency selected for the I2S communication. */
    
}I2S_InitTypeDef;

/**
  * @brief  HAL State structures definition
  */
typedef enum
{
    HAL_I2S_STATE_RESET      = 0x00UL,          /*!< I2S not yet initialized or disabled                */
    HAL_I2S_STATE_READY      = 0x01UL,          /*!< I2S initialized and ready for use                  */
    HAL_I2S_STATE_BUSY       = 0x02UL,          /*!< I2S internal process is ongoing                    */
    HAL_I2S_STATE_BUSY_TX    = 0x03UL,          /*!< Data Transmission process is ongoing               */
    HAL_I2S_STATE_BUSY_RX    = 0x04UL,          /*!< Data Reception process is ongoing                  */
    HAL_I2S_STATE_BUSY_TX_RX = 0x05UL,          /*!< Data Transmission and Reception process is ongoing */
    HAL_I2S_STATE_TIMEOUT    = 0x06UL,          /*!< I2S timeout state                                  */
    HAL_I2S_STATE_ERROR      = 0x07UL           /*!< I2S error state                                    */
}HAL_I2S_StateTypeDef;


/**
  * @brief I2S handle Structure definition
  */
typedef struct __I2S_HandleTypeDef
{
    I2S_TypeDef                 *Instance;                   /*!< I2S registers base address             */
    I2S_InitTypeDef             Init;                        /*!< I2S communication parameters           */
    
    const uint32_t              *pTxBuffPtr;                 /*!< Tx buffer pointer                      */
    __IO uint16_t               TxXferSize;                  /*!< I2S Tx transfer size                   */
    __IO uint16_t               TxXferCount;                 /*!< I2S Tx transfer Counter                */
    
    uint32_t                    *pRxBuffPtr;                 /*!< Pointer to I2S Rx transfer buffer      */
    __IO uint16_t               RxXferSize;                  /*!< I2S Rx transfer size                   */
    __IO uint16_t               RxXferCount;                 /*!< I2S Rx transfer counter                */
    
    DMA_HandleTypeDef           *hdmatx;                     /*!< Tx DMA Handle parameters               */
    DMA_HandleTypeDef           *hdmarx;                     /*!< Rx DMA Handle parameters               */
    
    __IO HAL_I2S_StateTypeDef   State;                       /*!< Tx state                               */
    __IO uint32_t               ErrorCode;                   /*!< Tx Error                               */
    
}I2S_HandleTypeDef;

/* Exported constants --------------------------------------------------------*/

/** @defgroup I2S_Error I2S Error
  * @{
  */
#define HAL_I2S_ERROR_NONE                          (0x00000000UL)  /*!< No error                          */
#define HAL_I2S_ERROR_TIMEOUT                       (0x00000001UL)  /*!< Timeout error                     */
#define HAL_I2S_ERROR_OVR                           (0x00000002UL)  /*!< OVR error                         */
#define HAL_I2S_ERROR_UDR                           (0x00000004UL)  /*!< UDR error                         */
#define HAL_I2S_ERROR_DMA                           (0x00000008UL)  /*!< DMA transfer error                */
#define HAL_I2S_ERROR_PRESCALER                     (0x00000010UL)  /*!< Prescaler Calculation error       */
#define HAL_I2S_ERROR_FRE                           (0x00000020UL)  /*!< FRE error                         */
#define HAL_I2S_ERROR_NO_OGT                        (0x00000040UL)  /*!< No On Going Transfer error        */
#define HAL_I2S_ERROR_NOT_SUPPORTED                 (0x00000080UL)  /*!< Requested operation not supported */

/** @defgroup I2S_Mode
  * @{
  */
#define I2S_MODE_MASTER                             ( I2S_CR_MODE )
#define I2S_MODE_SLAVE                              ( 0U )


/** @defgroup I2S_Standard
  * @{
  */
#define I2S_STANDARD_PHILIPS                        ( I2S_CR_STD_PHILIPS )
#define I2S_STANDARD_MSB                            ( I2S_CR_STD_MSB )
#define I2S_STANDARD_LSB                            ( I2S_CR_STD_LSB )
#define I2S_STANDARD_PCM_SHORT                      ( I2S_CR_STD_PCM )
#define I2S_STANDARD_PCM_LONG                       ( I2S_CR_PCMMODE | I2S_CR_STD_PCM )

/** @defgroup I2S_DataFormat
  * @{
  */
#define I2S_DATA_FORMAT_16B                         ( 0U )
#define I2S_DATA_FORMAT_16B_EXTENDED_TO_32B         ( I2S_CR_CHLEN | I2S_CR_DTLEN_16B )
#define I2S_DATA_FORMAT_24B_EXTENDED_TO_32B         ( I2S_CR_CHLEN | I2S_CR_DTLEN_24B )
#define I2S_DATA_FORMAT_32B                         ( I2S_CR_CHLEN | I2S_CR_DTLEN_32B )

/** @defgroup I2S_MCLKOut
  * @{
  */
#define I2S_MCLKOUT_ENABLE                          ( I2S_PR_MCKOE )
#define I2S_MCLKOUT_DISABLE                         ( 0U )

/** @defgroup I2S_ClockPolarity
  * @{
  */
#define I2S_CLOCK_POLARITY_LOW                      ( 0U )
#define I2S_CLOCK_POLARITY_HIGH                     ( I2S_CR_CKPL )

/** @defgroup I2S_IOSwitch
  * @{
  */
#define I2S_IO_SWITCH_ENABLE                        ( I2S_CR_IOSWP )
#define I2S_IO_SWITCH_DISABLE                       ( 0U )

/** @defgroup I2S_IT
  * @{
  */
#define I2S_IT_TXE                                  ( I2S_DIER_TXEIE )
#define I2S_IT_RXNE                                 ( I2S_DIER_RXNEIE )
#define I2S_IT_ERR                                  ( I2S_DIER_ERRIE )
#define I2S_IT_MSUSP                                ( I2S_DIER_MSUSPIE )
#define I2S_IT_SVTC                                 ( I2S_DIER_SVTCIE )

#define I2S_IT_MASK                                 ( I2S_IT_TXE | I2S_IT_RXNE | I2S_IT_ERR | I2S_IT_MSUSP | I2S_IT_SVTC )

/** @defgroup I2S_Flags
  * @{
  */
#define I2S_FLAG_SVTC                               ( I2S_SR_SVTC )
#define I2S_FLAG_MSUSP                              ( I2S_SR_MSUSP )
#define I2S_FLAG_FE                                 ( I2S_SR_FE )
#define I2S_FLAG_OVR                                ( I2S_SR_OVR )
#define I2S_FLAG_UDR                                ( I2S_SR_UDR )
#define I2S_FLAG_CH                                 ( I2S_SR_CH )
#define I2S_FLAG_TXE                                ( I2S_SR_TXE )
#define I2S_FLAG_RXNE                               ( I2S_SR_RXNE )

#define I2S_FLAG_MASK                               ( I2S_FLAG_SVTC | I2S_FLAG_MSUSP | I2S_FLAG_FE | I2S_FLAG_OVR | \
                                                      I2S_FLAG_UDR | I2S_FLAG_CH | I2S_FLAG_TXE | I2S_FLAG_RXNE )
/* Exported macros -----------------------------------------------------------*/

//#define __HAL_I2S_ENABLE(__HANDLE__)                                    ((__HANDLE__)->Instance->CR |= I2S_CR_EN)

//#define __HAL_I2S_DISABLE(__HANDLE__)                                   ((__HANDLE__)->Instance->CR &= ~(I2S_CR_EN))

//#define __HAL_I2S_TX_RX_ENABLE(__HANDLE__)                              ((__HANDLE__)->Instance->CR |= I2S_CR_TEN | I2S_CR_REN)

//#define __HAL_I2S_TX_RX_DISABLE(__HANDLE__)                             ((__HANDLE__)->Instance->CR &= ~(I2S_CR_TEN | I2S_CR_REN))

//#define __HAL_I2S_TX_ENABLE(__HANDLE__)                                 ((__HANDLE__)->Instance->CR |= I2S_CR_TEN)

//#define __HAL_I2S_TX_DISABLE(__HANDLE__)                                ((__HANDLE__)->Instance->CR &= ~(I2S_CR_TEN))

//#define __HAL_I2S_RX_ENABLE(__HANDLE__)                                 ((__HANDLE__)->Instance->CR |= I2S_CR_REN)

//#define __HAL_I2S_RX_DISABLE(__HANDLE__)                                ((__HANDLE__)->Instance->CR &= ~(I2S_CR_REN))

//#define __HAL_I2S_START_ENABLE(__HANDLE__)                              ((__HANDLE__)->Instance->CR |= I2S_CR_START)

//#define __HAL_I2S_START_DISABLE(__HANDLE__)                             ((__HANDLE__)->Instance->CR &= ~(I2S_CR_START))

//#define __HAL_I2S_STOP_ENABLE(__HANDLE__)                               ((__HANDLE__)->Instance->CR |= I2S_CR_STOP)

//#define __HAL_I2S_STOP_DISABLE(__HANDLE__)                              ((__HANDLE__)->Instance->CR &= ~(I2S_CR_STOP))

//#define __HAL_I2S_WRITE_DATA(__HANDLE__, __DATA__)                      ((__HANDLE__)->Instance->TXDR = (__DATA__))

//#define __HAL_I2S_READ_DATA(__HANDLE__)                                 ((__HANDLE__)->Instance->RXDR)

//#define __HAL_I2S_TX_DMA_ENABLE(__HANDLE__)                             ((__HANDLE__)->Instance->CR |= I2S_CR_TXDMAEN)
// 
//#define __HAL_I2S_TX_DMA_DISABLE(__HANDLE__)                            ((__HANDLE__)->Instance->CR &= ~(I2S_CR_TXDMAEN))


///**
//  * @brief  Enable the RX DMA.
//  * @param  __HANDLE__ I2S handle
//  * @retval None
//  */

//#define __HAL_I2S_RX_DMA_ENABLE(__HANDLE__)                             ((__HANDLE__)->Instance->CR |= I2S_CR_RXDMAEN)

///**
//  * @brief  Disable the RX DMA.
//  * @param  __HANDLE__ I2S handle
//  * @retval None
//  */
//  
//#define __HAL_I2S_RX_DMA_DISABLE(__HANDLE__)                            ((__HANDLE__)->Instance->CR &= ~(I2S_CR_RXDMAEN))

///**
//  * @brief  Enable the TX RX DMA.
//  * @param  __HANDLE__ I2S handle
//  * @retval None
//  */
//  
//#define __HAL_I2S_TX_RX_DMA_ENABLE(__HANDLE__)                          ((__HANDLE__)->Instance->CR |= I2S_CR_TXDMAEN | I2S_CR_RXDMAEN)

///**
//  * @brief  Disable the TX RX DMA.
//  * @param  __HANDLE__ I2S handle
//  * @retval None
//  */
//  
//#define __HAL_I2S_TX_RX_DMA_DISABLE(__HANDLE__)                         ((__HANDLE__)->Instance->CR &= ~(I2S_CR_TXDMAEN | I2S_CR_RXDMAEN))

/** @brief  Enable the specified I2S peripheral.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_ENABLE(__HANDLE__)    (SET_BIT((__HANDLE__)->Instance->CR, I2S_CR_EN))

/** @brief  Disable the specified I2S peripheral.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_DISABLE(__HANDLE__) (CLEAR_BIT((__HANDLE__)->Instance->CR, I2S_CR_EN))

/**
  * @brief  Enable the specified I2S interrupt.
  * @param  __HANDLE__ I2S handle.
  * @param  __INTERRUPT__ I2S interrupt to set.
  *            This parameter can be a combination of:
  *            @arg I2S_IT_TXE  : Tx buffer empty Interrupt.
  *            @arg I2S_IT_RXNE : Rx buffer non empty Interrupt.
  *            @arg I2S_IT_ERR  : Error Interrupt.
  *            @arg I2S_IT_MSUSP: Master suspend Interrupt.
  *            @arg I2S_IT_SVTC : Slave transmission complete Interrupt.
  * @retval None.
  */
#define __HAL_I2S_ENABLE_IT(__HANDLE__, __INTERRUPT__)                  ((__HANDLE__)->Instance->IER  |= (__INTERRUPT__))

/**
  * @brief  Disable the specified I2S interrupt.
  * @param  __HANDLE__ I2S handle.
  * @param  __INTERRUPT__ I2S interrupt to set.
  *            This parameter can be a combination of:
  *            @arg I2S_IT_TXE  : Tx buffer empty Interrupt.
  *            @arg I2S_IT_RXNE : Rx buffer non empty Interrupt.
  *            @arg I2S_IT_ERR  : Error Interrupt.
  *            @arg I2S_IT_MSUSP: Master suspend Interrupt.
  *            @arg I2S_IT_SVTC : Slave transmission complete Interrupt.
  * @retval None.
  */
#define __HAL_I2S_DISABLE_IT(__HANDLE__, __INTERRUPT__)                 ((__HANDLE__)->Instance->IER  &= (~(__INTERRUPT__)))

/**
  * @brief  Check whether the specified I2S interrupt source is enabled or not.
  * @param  __HANDLE__ I2S handle.
  * @param  __INTERRUPT__ I2S interrupt to check.
  *            This parameter can be a value of:
  *            @arg I2S_IT_TXE  : Tx buffer empty Interrupt.
  *            @arg I2S_IT_RXNE : Rx buffer non empty Interrupt.
  *            @arg I2S_IT_ERR  : Error Interrupt.
  *            @arg I2S_IT_MSUSP: Master suspend Interrupt.
  *            @arg I2S_IT_SVTC : Slave transmission complete Interrupt.
  * @retval Interrupt status.
  */
#define __HAL_I2S_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)              ((__HANDLE__)->Instance->IER & (__INTERRUPT__))

/**
  * @brief  Check whether the specified I2S flag is set or not.
  * @param  __HANDLE__ I2S handle
  * @param  __FLAG__ I2S flag to check
  *            This parameter can be a value of:
  *            @arg I2S_FLAG_SVTC  : Slave transmission complete flag.
  *            @arg I2S_FLAG_MSUSP : Master suspend flag.
  *            @arg I2S_FLAG_FE    : Frame error flag.
  *            @arg I2S_FLAG_OVR   : Overrun flag.
  *            @arg I2S_FLAG_UDR   : Underrun flag.
  *            @arg I2S_FLAGR_TXE  : tx buffer empty flag.
  *            @arg I2S_FLAG_RXNE  : rx buffer non empty flag.
  * @retval The state of the specified flag (SET or RESET).
  */
#define __HAL_I2S_GET_FLAG(__HANDLE__, __FLAG__) ((__HANDLE__)->Instance->SR & (__FLAG__))

/** @brief Clear the I2S SVTC pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_SVTCFLAG(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->SR , I2S_FLAG_SVTC)

/** @brief Clear the I2S MSUSP pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_MSUSPFLAG(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->SR , I2S_FLAG_MSUSP)

/** @brief  Clear the I2S FRE pending flag.
  * @param  __HANDLE__: specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_FREFLAG(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->SR , I2S_FLAG_FE)

/** @brief Clear the I2S OVR pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_OVRFLAG(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->SR , I2S_FLAG_OVR)

/** @brief Clear the I2S UDR pending flag.
  * @param  __HANDLE__ specifies the I2S Handle.
  * @retval None
  */
#define __HAL_I2S_CLEAR_UDRFLAG(__HANDLE__) CLEAR_BIT((__HANDLE__)->Instance->SR , I2S_FLAG_UDR)



/* Exported functions --------------------------------------------------------*/

/* Initialization/de-initialization functions  ********************************/
HAL_StatusTypeDef HAL_I2S_Init(I2S_HandleTypeDef *hi2s);
HAL_StatusTypeDef HAL_I2S_DeInit(I2S_HandleTypeDef *hi2s);
void HAL_I2S_MspInit(I2S_HandleTypeDef *hi2s);
void HAL_I2S_MspDeInit(I2S_HandleTypeDef *hi2s);

/* I/O operation functions  ***************************************************/
/* Blocking mode: Polling */
HAL_StatusTypeDef HAL_I2S_Transmit(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2S_Receive(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_I2S_TransmitReceive(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                          uint16_t Size, uint32_t Timeout);

/* Non-Blocking mode: Interrupt */
HAL_StatusTypeDef HAL_I2S_Transmit_IT(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_Receive_IT(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_TransmitReceive_IT(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                             uint16_t Size);

void HAL_I2S_IRQHandler(I2S_HandleTypeDef *hi2s);

/* Non-Blocking mode: DMA */
HAL_StatusTypeDef HAL_I2S_Transmit_DMA(I2S_HandleTypeDef *hi2s, const uint32_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_Receive_DMA(I2S_HandleTypeDef *hi2s, uint32_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_I2S_TransmitReceive_DMA(I2S_HandleTypeDef *hi2s, uint32_t *pRxData, const uint32_t *pTxData, 
                                              uint16_t Size);

HAL_StatusTypeDef HAL_I2S_DMAStop(I2S_HandleTypeDef *hi2s);

/* Callbacks used in non blocking modes (Interrupt and DMA) *******************/
void HAL_I2S_TxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxRxHalfCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_TxRxCpltCallback(I2S_HandleTypeDef *hi2s);
void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);      
void HAL_I2S_DMATxCpltCallback(I2S_HandleTypeDef *hi2s);

//void HAL_I2S_TxEmptyCallback(I2S_HandleTypeDef *hi2s);
//void HAL_I2S_TxCompleteCallback(I2S_HandleTypeDef *hi2s);
//void HAL_I2S_MsuspCallback(I2S_HandleTypeDef *hi2s);
//void HAL_I2S_SvtcCallback(I2S_HandleTypeDef *hi2s);
//void HAL_I2S_RxNonEmptyCallback(I2S_HandleTypeDef *hi2s);
//void HAL_I2S_ErrorCallback(I2S_HandleTypeDef *hi2s);

//void HAL_I2S_DMATxCpltCallback(DMA_HandleTypeDef *hdmac);
//void HAL_I2S_DMATxHalfCpltCallback(DMA_HandleTypeDef *hdmac);
//void HAL_I2S_DMARxCpltCallback(DMA_HandleTypeDef *hdmac);
//void HAL_I2S_DMARxHalfCpltCallback(DMA_HandleTypeDef *hdmac);
//void HAL_I2S_DMAErrorCallback(DMA_HandleTypeDef *hdmac);
    
/* Peripheral Control and State functions  ************************************/
HAL_I2S_StateTypeDef HAL_I2S_GetState(const I2S_HandleTypeDef *hi2s);
uint32_t HAL_I2S_GetError(const I2S_HandleTypeDef *hi2s);

/* Private macros ------------------------------------------------------------*/
#define IS_I2S_HANDLE(__HANDLE__)                   ((__HANDLE__) != NULL)
  
#define IS_I2S_INSTANCE(__INSTANCE__)               (((__INSTANCE__) == I2S1)   || \
                                                     ((__INSTANCE__) == I2S2))

#define IS_I2S_MODE(__MODE__)                       (((__MODE__) == I2S_MODE_MASTER)              || \
                                                     ((__MODE__) == I2S_MODE_SLAVE))

#define IS_I2S_STANDARD(__STANDARD__)               (((__STANDARD__) == I2S_STANDARD_PHILIPS)   || \
                                                     ((__STANDARD__) == I2S_STANDARD_MSB)       || \
                                                     ((__STANDARD__) == I2S_STANDARD_LSB)       || \
                                                     ((__STANDARD__) == I2S_STANDARD_PCM_SHORT) || \
                                                     ((__STANDARD__) == I2S_STANDARD_PCM_LONG))

#define IS_I2S_DATAFORMAT(__DATAFORMAT__)           (((__DATAFORMAT__) == I2S_DATA_FORMAT_16B) || \
                                                     ((__DATAFORMAT__) == I2S_DATA_FORMAT_16B_EXTENDED_TO_32B) || \
                                                     ((__DATAFORMAT__) == I2S_DATA_FORMAT_24B_EXTENDED_TO_32B) || \
                                                     ((__DATAFORMAT__) == I2S_DATA_FORMAT_32B))


#define IS_I2S_MCLKOUTPUT(__OUTPUT__)               (((__OUTPUT__) == I2S_MCLKOUT_ENABLE)   || \
                                                     ((__OUTPUT__) == I2S_MCLKOUT_DISABLE))

#define IS_I2S_CLOCKPOLARITY(__POLARITY__)          (((__POLARITY__) == I2S_CLOCK_POLARITY_LOW)  || \
                                                     ((__POLARITY__) == I2S_CLOCK_POLARITY_HIGH))

#define IS_I2S_IOSWITCH(__SWITCH__)                 (((__SWITCH__) == I2S_IO_SWITCH_ENABLE)  || \
                                                     ((__SWITCH__) == I2S_IO_SWITCH_DISABLE))

#endif

