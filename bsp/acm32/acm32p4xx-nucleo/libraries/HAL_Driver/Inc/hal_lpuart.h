
/******************************************************************************
*@file  : hal_lpuart.h
*@brief : Header file of LPUART HAL module.
******************************************************************************/

#ifndef __HAL_LPUART_H__
#define __HAL_LPUART_H__

#include "hal.h"

/** 
  * @brief  LPUART Init structure definition
  */
typedef struct
{
    uint32_t    BaudRate;           /*!< This member configures the UART communication baud rate.
                                         This parameter can be a value of @ref LPUART_BaudRate. */

    uint32_t    WordLength;         /*!< Specifies the number of data bits transmitted or received in a frame.
                                         This parameter can be a value of @ref LPUART_WordLength. */
          
    uint32_t    StopBits;           /*!< Specifies the number of stop bits transmitted.
                                         This parameter can be a value of @ref LPUART_StopBits. */

    uint32_t    Parity;             /*!< Specifies the parity mode.
                                         This parameter can be a value of @ref LPUART_Parity*/
    
    uint32_t    Polarity;           /*!< Specifies the polarity mode.
                                         This parameter can be a value of @ref LPUART_Polarity. */

    uint32_t    Mode;               /*!< Specifies whether the Receive or Transmit mode is enabled or disabled.
                                         This parameter can be a value of @ref LPUART_Mode. */

    uint32_t    ClockSource;        /*!< Specifies the clock source of LPUART.
                                         This parameter can be a value of @ref LPUART_ClockSource. */
} LPUART_InitTypeDef;

typedef uint32_t HAL_LPUART_StateTypeDef;

/**
  * @brief HAL LPUART Reception type definition
  * @note  HAL LPUART Reception type value aims to identify which type of Reception is ongoing.
  *        This parameter can be a value of @ref LPUART_Reception_Type_Values :
  *           HAL_LPUART_RECEPTION_STANDARD         = 0x00U,
  *           HAL_LPUART_RECEPTION_TOIDLE           = 0x01U,
  *           HAL_LPUART_RECEPTION_TOBCNT           = 0x02U,
  *           HAL_LPUART_RECEPTION_TOCHARMATCH      = 0x03U,
  */
typedef uint32_t HAL_LPUART_RxTypeTypeDef;

/**
  * @brief HAL LPUART Rx Event type definition
  * @note  HAL LPUART Rx Event type value aims to identify which type of Event has occurred
  *        leading to call of the RxEvent callback.
  *        This parameter can be a value of @ref LPUART_RxEvent_Type_Values :
  *           HAL_LPUART_RXEVENT_TC                 = 0x00U,
  *           HAL_LPUART_RXEVENT_HT                 = 0x01U,
  *           HAL_LPUART_RXEVENT_IDLE               = 0x02U,
  *           HAL_LPUART_RXEVENT_BCNT               = 0x03U,
  */
typedef uint32_t HAL_LPUART_RxEventTypeTypeDef;

/** 
  * @brief  LPUART  handle structure definition
  */
typedef struct __LPUART_HandleTypeDef
{
    LPUART_TypeDef                     *Instance;          /*!< Register base address      */
    LPUART_InitTypeDef                 Init;               /*!< communication parameters   */

    DMA_HandleTypeDef                  *hdmatx;            /*!< Tx DMA Handle parameters   */
    DMA_HandleTypeDef                  *hdmarx;            /*!< Rx DMA Handle parameters   */
    
    const uint8_t                      *pTxBuffPtr;        /*!< Pointer to LPUART Tx transfer Buffer */
    uint16_t                           TxXferSize;         /*!< LPUART Tx Transfer size              */
    __IO uint16_t                      TxXferCount;        /*!< LPUART Tx Transfer Counter           */

    uint8_t                            *pRxBuffPtr;        /*!< Pointer to LPUART Rx transfer Buffer */
    uint16_t                           RxXferSize;         /*!< LPUART Rx Transfer size              */
    __IO uint16_t                      RxXferCount;        /*!< LPUART Rx Transfer Counter           */

    __IO HAL_LPUART_RxTypeTypeDef      ReceptionType;      /*!< Type of ongoing reception          */
    __IO HAL_LPUART_RxEventTypeTypeDef RxEventType;        /*!< Type of Rx Event                   */    

    void (*RxISR)(struct __LPUART_HandleTypeDef *hlpuart); /*!< Function pointer on Rx IRQ handler */
    void (*TxISR)(struct __LPUART_HandleTypeDef *hlpuart); /*!< Function pointer on Tx IRQ handler */

    __IO HAL_LPUART_StateTypeDef       gState;             /*!< LPUART state information related to global Handle management
                                                                and also related to Tx operations. This parameter
                                                                can be a value of @ref HAL_LPUART_StateTypeDef */
    __IO HAL_LPUART_StateTypeDef       RxState;            /*!< LPUART state information related to Rx operations. This
                                                                parameter can be a value of @ref HAL_LPUART_StateTypeDef */
    __IO uint32_t                      ErrorCode;          /*!< LPUART Error code */
}LPUART_HandleTypeDef;


/* Exported constants --------------------------------------------------------*/

/** @defgroup LPUART_State_Definition LPUART State Code Definition
  * @{
  */ 
#define  HAL_LPUART_STATE_RESET             (0x00000000U)    /*!< Peripheral is not initialized
                                                                  Value is allowed for gState and RxState */
#define  HAL_LPUART_STATE_READY             (0x00000020U)    /*!< Peripheral Initialized and ready for use
                                                                  Value is allowed for gState and RxState */
#define  HAL_LPUART_STATE_BUSY              (0x00000026U)    /*!< an internal process is ongoing
                                                                  Value is allowed for gState only */
#define  HAL_LPUART_STATE_BUSY_TX           (0x00000021U)    /*!< Data Transmission process is ongoing
                                                                  Value is allowed for gState only */
#define  HAL_LPUART_STATE_BUSY_RX           (0x00000022U)    /*!< Data Reception process is ongoing
                                                                  Value is allowed for RxState only */
#define  HAL_LPUART_STATE_BUSY_TX_RX        (0x00000023U)    /*!< Data Transmission and Reception process is ongoing
                                                                  Not to be used for neither gState nor RxState.Value is result
                                                                  of combination (Or) between gState and RxState values */
#define  HAL_LPUART_STATE_BUSY_TX_DMA       (0x00000024U)    /*!< Data DMA Transmission process is ongoing
                                                                  Value is allowed for gState only */
#define  HAL_LPUART_STATE_BUSY_RX_DMA       (0x00000028U)    /*!< Data DMA Reception process is ongoing
                                                                  Value is allowed for RxState only */
#define  HAL_LPUART_STATE_TIMEOUT           (0x000000A0U)    /*!< Timeout state
                                                                  Value is allowed for gState only */
#define  HAL_LPUART_STATE_ERROR             (0x000000E0U)    /*!< Error
                                                                  Value is allowed for gState only */
/** @defgroup UART_Error_Definition   UART Error Definition
  * @{
  */
#define  HAL_LPUART_ERROR_NONE              (0x00000000U)    /*!< No error                */
#define  HAL_LPUART_ERROR_PE                (0x00000001U)    /*!< Parity error            */
#define  HAL_LPUART_ERROR_FE                (0x00000002U)    /*!< Frame error             */
#define  HAL_LPUART_ERROR_ORE               (0x00000004U)    /*!< Overrun error           */
#define  HAL_LPUART_ERROR_DMA_TX            (0x00000008U)    /*!< DMA transmite error     */
#define  HAL_LPUART_ERROR_DMA_RX            (0x00000010U)    /*!< DMA receive error       */

/** @defgroup LPUART_DMA_State_Definition   LPUART DMA State Definition
  * @{
  */
#define  HAL_LPUART_DMA_DISABLE             (0x00000000U)    /*!< DMA is disabled */
#define  HAL_LPUART_DMA_TX_ENABLE           (0x00000001U)    /*!< LPUART TX DMA is enabled */
#define  HAL_LPUART_DMA_RX_ENABLE           (0x00000002U)    /*!< LPUART RX DMA is enabled */

/** @defgroup LPUART_Stop_Bits   LPUART Number of Stop Bits
  * @{
  */
 #define LPUART_STOPBITS_1B                 ( 0x00000000U )
 #define LPUART_STOPBITS_2B                 ( LPUART_LCR_STP2 )

/** @defgroup LPUART_Parity  LPUART Parity
  * @{
  */
#define LPUART_PARITY_NONE                  ( 0U )
#define LPUART_PARITY_ODD                   ( LPUART_LCR_PEN )
#define LPUART_PARITY_EVEN                  ( LPUART_LCR_PEN | LPUART_LCR_EPS )
#define LPUART_PARITY_0                     ( LPUART_LCR_PEN | LPUART_LCR_SPS | LPUART_LCR_EPS)
#define LPUART_PARITY_1                     ( LPUART_LCR_PEN | LPUART_LCR_SPS )

/** @defgroup LPUART_Word_Length LPUART Word Length
  * @{
  */
#define LPUART_WORDLENGTH_7B                ( LPUART_LCR_WLEN )
#define LPUART_WORDLENGTH_8B                ( 0x00000000U )

/** @defgroup LPUART_Polarity LPUART Polarity
  * @{
  */
#define LPUART_POLARITY_NONE                ( 0U )
#define LPUART_POLARITY_TX_OPPOSITE         ( LPUART_LCR_TXPOL )
#define LPUART_POLARITY_RX_OPPOSITE         ( LPUART_LCR_RXPOL )
#define LPUART_POLARITY_BOTH_OPPOSITE       ( LPUART_LCR_TXPOL | LPUART_LCR_RXPOL )
  
/** @defgroup LPUART_ClockSource LPUART Clock Source
  * @{
  */
#define LPUART_CLOCKSOURCE_RCL              ( RCC_LPUART_CLK_SOURCE_RCL )
#define LPUART_CLOCKSOURCE_XTL              ( RCC_LPUART_CLK_SOURCE_XTL )
#define LPUART_CLOCKSOURCE_PCLK_DIV4        ( RCC_LPUART_CLK_SOURCE_PCLK1_DIV4)
#define LPUART_CLOCKSOURCE_PCLK_DIV8        ( RCC_LPUART_CLK_SOURCE_PCLK1_DIV8)
#define LPUART_CLOCKSOURCE_PCLK_DIV16       ( RCC_LPUART_CLK_SOURCE_PCLK1_DIV16)
#define LPUART_CLOCKSOURCE_PCLK_DIV32       ( RCC_LPUART_CLK_SOURCE_PCLK1_DIV32)

/** @defgroup LPUART_Mode LPUART Transfer Mode
  * @{
  */
#define LPUART_MODE_TX                      ( LPUART_CR_TX_EN )
#define LPUART_MODE_RX                      ( LPUART_CR_RX_EN )
#define LPUART_MODE_TXRX                    ( LPUART_CR_TX_EN | LPUART_CR_RX_EN )

/** @defgroup LPUART_WakeUp_Methods   LPUART WakeUp Methods
  * @{
  */
#define LPUART_WAKEUPMETHOD_NONE            ( LPUART_LCR_RXWKS_NONE )
#define LPUART_WAKEUPMETHOD_STARTBIT        ( LPUART_LCR_RXWKS_START_BIT )
#define LPUART_WAKEUPMETHOD_ONEBYTENOCHECK  ( LPUART_LCR_RXWKS_ONE_BYTE )
#define LPUART_WAKEUPMETHOD_ONEBYTECHECK    ( LPUART_LCR_RXWKS_ONE_BYTE | LPUART_LCR_WKCK )
#define LPUART_WAKEUPMETHOD_ADDRNOCHECK     ( LPUART_LCR_RXWKS_ADDR_MATCH )
#define LPUART_WAKEUPMETHOD_ADDRCHECK       ( LPUART_LCR_RXWKS_ADDR_MATCH | LPUART_LCR_WKCK)

/** @defgroup LPUART_Interrupt_definition   LPUART Interrupts Definition
  * @{
  */
#define LPUART_IT_START                     ( LPUART_IE_STARTIE )
#define LPUART_IT_MATCH                     ( LPUART_IE_MATCHIE )
#define LPUART_IT_RXOV                      ( LPUART_IE_RXOVIE )
#define LPUART_IT_FE                        ( LPUART_IE_FEIE )
#define LPUART_IT_PE                        ( LPUART_IE_PEIE )
#define LPUART_IT_TXE                       ( LPUART_IE_TXEIE )
#define LPUART_IT_TC                        ( LPUART_IE_TCIE )
#define LPUART_IT_RX                        ( LPUART_IE_RXIE )
#define LPUART_IT_IDLE                      ( LPUART_IE_IDLEIE )
#define LPUART_IT_BCNT                      ( LPUART_IE_BCNTIE )

/** 
  * @brief  LPUART_Interruption_Mask    LPUART Interruptions Flag Mask
  */
#define LPUART_IT_MASK                      ( LPUART_IT_START | LPUART_IT_MATCH | \
                                              LPUART_IT_RXOV | LPUART_IT_FE | \
                                              LPUART_IT_PE | LPUART_IT_TXE | \
                                              LPUART_IT_TC | LPUART_IT_RX | \
                                              LPUART_IT_BCNT | LPUART_IT_IDLE)

/** @defgroup LPUART_IT_CLEAR_Flags  LPUART Interruption Clear Flags
  * @{
  */
#define LPUART_FLAG_TXE                     ( LPUART_SR_TXE )
#define LPUART_FLAG_STARTIF                 ( LPUART_SR_STARTIF )
#define LPUART_FLAG_MATCHIF                 ( LPUART_SR_MATCHIF )
#define LPUART_FLAG_TXOVF                   ( LPUART_SR_TXOVF )
#define LPUART_FLAG_RXF                     ( LPUART_SR_RXF )
#define LPUART_FLAG_RXOVIF                  ( LPUART_SR_RXOVIF )
#define LPUART_FLAG_FEIF                    ( LPUART_SR_FEIF )
#define LPUART_FLAG_PEIF                    ( LPUART_SR_PEIF )
#define LPUART_FLAG_TXEIF                   ( LPUART_SR_TXEIF )
#define LPUART_FLAG_TCIF                    ( LPUART_SR_TCIF )
#define LPUART_FLAG_RXIF                    ( LPUART_SR_RXIF )
#define LPUART_FLAG_IDLEIF                  ( LPUART_SR_IDLEIF )
#define LPUART_FLAG_BCNTIF                  ( LPUART_SR_BCNTIF )

/** @defgroup LPUART_IT_FLAG_Mask  LPUART Interruption Flag Mask
  * @{
  */
#define LPUART_IT_FLAG_MASK                 ( LPUART_FLAG_STARTIF | LPUART_FLAG_MATCHIF | \
                                              LPUART_FLAG_RXOVIF | LPUART_FLAG_FEIF | \
                                              LPUART_FLAG_PEIF | LPUART_FLAG_TXEIF | \
                                              LPUART_FLAG_TCIF | LPUART_FLAG_RXIF |\
                                              LPUART_FLAG_IDLEIF | LPUART_FLAG_BCNTIF)

#define LPUART_FLAG_MASK                    ( LPUART_FLAG_TXE | LPUART_FLAG_TXOVF | \
                                              LPUART_FLAG_RXF | LPUART_IT_FLAG_MASK)

#define LPUART_CLEAR_FLAG_MASK              ( LPUART_FLAG_STARTIF | LPUART_FLAG_MATCHIF | \
                                              LPUART_FLAG_TXOVF | LPUART_FLAG_RXOVIF | \
                                              LPUART_FLAG_FEIF | LPUART_FLAG_PEIF | \
                                              LPUART_FLAG_TCIF | LPUART_FLAG_RXIF |\
                                              LPUART_FLAG_IDLEIF | LPUART_FLAG_BCNTIF)
                                              
/** @defgroup LPUART_Reception_Type_Values  LPUART Reception type values
  * @{
  */
#define HAL_LPUART_RECEPTION_STANDARD          (0x00000000U)             /*!< Standard reception                       */
#define HAL_LPUART_RECEPTION_TOIDLE            (0x00000001U)             /*!< Reception till completion or IDLE event  */
#define HAL_LPUART_RECEPTION_TOBCNT            (0x00000002U)             /*!< Reception till completion or BCNT event   */
#define HAL_LPUART_RECEPTION_TOCHARMATCH       (0x00000003U)             /*!< Reception till completion or CM event    */

/** @defgroup LPUART_RxEvent_Type_Values  LPUART RxEvent type values
  * @{
  */
#define HAL_LPUART_RXEVENT_TC                  (0x00000000U)             /*!< RxEvent linked to Transfer Complete event */
#define HAL_LPUART_RXEVENT_HT                  (0x00000001U)             /*!< RxEvent linked to Half Transfer event     */
#define HAL_LPUART_RXEVENT_IDLE                (0x00000002U)             /*!< RxEvent linked to IDLE event              */       
#define HAL_LPUART_RXEVENT_BCNT                (0x00000003U)             /*!< RxEvent linked to BCNT event              */   

/* Exported macros -----------------------------------------------------------*/
/** @defgroup  LPUART Private Macros
  * @{
  */

#define __HAL_LPUART_RESET_HANDLE_STATE(__HANDLE__)   do{                                                       \
                                                            (__HANDLE__)->gState = HAL_LPUART_STATE_RESET;      \
                                                            (__HANDLE__)->RxState = HAL_LPUART_STATE_RESET;     \
                                                        } while(0U)
  
#define __HAL_LPUART_ENABLE_DMA(__HANDLE__)                             ((__HANDLE__)->Instance->CR |= LPUART_CR_DMA_EN)

#define __HAL_LPUART_DISABLE_DMA(__HANDLE__)                            ((__HANDLE__)->Instance->CR &= ~LPUART_CR_DMA_EN)

#define __HAL_LPUART_ENABLE_TX(__HANDLE__)                             ((__HANDLE__)->Instance->CR |= LPUART_CR_TX_EN)

#define __HAL_LPUART_DISABLE_TX(__HANDLE__)                            ((__HANDLE__)->Instance->CR &= ~LPUART_CR_TX_EN)

#define __HAL_LPUART_ENABLE_RX(__HANDLE__)                             ((__HANDLE__)->Instance->CR |= LPUART_CR_RX_EN)

#define __HAL_LPUART_DISABLE_RX(__HANDLE__)                            ((__HANDLE__)->Instance->CR &= ~LPUART_CR_RX_EN)

#define __HAL_LPUART_ENABLE_IT(__HANDLE__, __INTERRUPT__)               ((__HANDLE__)->Instance->IE |= (__INTERRUPT__) & LPUART_IT_MASK)

#define __HAL_LPUART_DISABLE_IT(__HANDLE__, __INTERRUPT__)              ((__HANDLE__)->Instance->IE &= ~((__INTERRUPT__) & LPUART_IT_MASK))

#define __HAL_LPUART_GET_FLAG(__HANDLE__, __FLAG__)                     ((__HANDLE__)->Instance->SR & ((__FLAG__) & LPUART_FLAG_MASK))

#define __HAL_LPUART_GET_IT_FLAG(__HANDLE__, __FLAG__)                  ((__HANDLE__)->Instance->SR & ((__FLAG__) & LPUART_IT_FLAG_MASK))

#define __HAL_LPUART_GET_IT_SOURCE(__HANDLE__, __INTERRUPT__)           ((__HANDLE__)->Instance->IE & ((__INTERRUPT__) & LPUART_IT_MASK))

#define __HAL_LPUART_CLEAR_FLAG(__HANDLE__, __FLAG__)                   ((__HANDLE__)->Instance->SR = ((__FLAG__) & LPUART_CLEAR_FLAG_MASK))


/** @defgroup  LPUART Private Macros
  * @{
  */
  
#define IS_LPUART_HANDLE(__HANDLE__)                    ((__HANDLE__) != NULL)

#define IS_LPUART_INSTANCE(__INSTANCE__)                (((__INSTANCE__) == LPUART1) || ((__INSTANCE__) == LPUART2))

#define IS_LPUART_WORDLENGTH(__WORDLENGTH__)            (((__WORDLENGTH__) == LPUART_WORDLENGTH_7B)   ||\
                                                         ((__WORDLENGTH__) == LPUART_WORDLENGTH_8B))

#define IS_LPUART_STOPBITS(__STOPBITS__)                (((__STOPBITS__) == LPUART_STOPBITS_1B)   ||\
                                                         ((__STOPBITS__) == LPUART_STOPBITS_2B))

#define IS_LPUART_PARITY(__PARITY__)                    (((__PARITY__) == LPUART_PARITY_NONE)   ||\
                                                         ((__PARITY__) == LPUART_PARITY_ODD)   ||\
                                                         ((__PARITY__) == LPUART_PARITY_EVEN)   ||\
                                                         ((__PARITY__) == LPUART_PARITY_0)   ||\
                                                         ((__PARITY__) == LPUART_PARITY_1))
                                                         
#define IS_LPUART_POLARITY(__POLARITY__)                (((__POLARITY__) == LPUART_POLARITY_NONE)   ||\
                                                         ((__POLARITY__) == LPUART_POLARITY_RX_OPPOSITE)  ||\
                                                         ((__POLARITY__) == LPUART_POLARITY_TX_OPPOSITE)  ||\
                                                         ((__POLARITY__) == LPUART_POLARITY_BOTH_OPPOSITE))

#define IS_LPUART_MODE(__MODE__)                        (((__MODE__) == LPUART_MODE_TX)   ||\
                                                         ((__MODE__) == LPUART_MODE_RX)   ||\
                                                         ((__MODE__) == LPUART_MODE_TXRX))

#define IS_LPUART_WAKEUPMETHOD(__WAKEUPMETHOD__)        (((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_NONE)   ||\
                                                         ((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_STARTBIT)   ||\
                                                         ((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_ONEBYTENOCHECK)   ||\
                                                         ((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_ONEBYTECHECK)   ||\
                                                         ((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_ADDRNOCHECK)   ||\
                                                         ((__WAKEUPMETHOD__) == LPUART_WAKEUPMETHOD_ADDRCHECK))


#define IS_LPUART_CLOCKSOURCE(__CLOCKSOURCE__)         (((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_XTL)	       || \
                                                        ((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_RCL)		     || \
                                                        ((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_PCLK_DIV4)	 || \
                                                        ((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_PCLK_DIV8)	 || \
                                                        ((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_PCLK_DIV16) || \
                                                        ((__CLOCKSOURCE__) == LPUART_CLOCKSOURCE_PCLK_DIV32))
								
#define IS_LPUART_WAKEUPADDR(__WAKEUPADDR__)            ((__WAKEUPADDR__) <= 0xFFU)

#define IS_LPUART_DMASIZE(__DMASIZE__)                  ((__DMASIZE__) <= 0xFFFFU)

#define IS_LPUART_BAUDRATE(__BAUDRATE__)                (((__BAUDRATE__) >= 2U) && ((__BAUDRATE__) <= 254U))


/* Exported functions --------------------------------------------------------*/
void HAL_LPUART_IRQHandler(LPUART_HandleTypeDef *hlpuart);
HAL_StatusTypeDef HAL_LPUART_Init(LPUART_HandleTypeDef *hlpuart);
HAL_StatusTypeDef HAL_LPUART_DeInit(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_MspInit(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_MspDeInit(LPUART_HandleTypeDef *hlpuart);

//HAL_StatusTypeDef HAL_LPUART_SetBaudRate(LPUART_HandleTypeDef *hlpuart, uint32_t baudRate);

HAL_StatusTypeDef HAL_LPUART_Transmit(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size, uint32_t timeout);
HAL_StatusTypeDef HAL_LPUART_Receive(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size, uint32_t timeout);
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, 
                                           uint16_t *rxLen, uint32_t timeout);
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, 
                                           uint16_t *rxLen, uint16_t bcntVal, uint32_t timeout);
                                           
HAL_StatusTypeDef HAL_LPUART_Transmit_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size);
HAL_StatusTypeDef HAL_LPUART_Receive_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size);
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size);
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt_IT(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, uint16_t bcntVal);

HAL_StatusTypeDef HAL_LPUART_Transmit_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size);
HAL_StatusTypeDef HAL_LPUART_Receive_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint32_t size);
HAL_StatusTypeDef HAL_LPUART_ReceiveToIdle_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size);
HAL_StatusTypeDef HAL_LPUART_ReceiveToBcnt_DMA(LPUART_HandleTypeDef *hlpuart, uint8_t *pdata, uint16_t size, uint16_t bcntVal);

HAL_StatusTypeDef HAL_LPUART_AbortTransmit(LPUART_HandleTypeDef *hlpuart);
HAL_StatusTypeDef HAL_LPUART_AbortReceive(LPUART_HandleTypeDef *hlpuart);

uint32_t HAL_LPUART_GetTxState(LPUART_HandleTypeDef *hlpuart);
uint32_t HAL_LPUART_GetRxState(LPUART_HandleTypeDef *hlpuart);

HAL_StatusTypeDef LPUART_SetConfig(LPUART_HandleTypeDef *hlpuart);

HAL_StatusTypeDef HAL_LPUART_WakeUpConfig(LPUART_HandleTypeDef *hlpuart, uint32_t WakeUpMethod, uint8_t Address);

void HAL_LPUART_TxCpltCallback(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_TxHalfCpltCallback(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_RxCpltCallback(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_RxHalfCpltCallback(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_WakeupCallback(LPUART_HandleTypeDef *hlpuart);
void HAL_LPUART_RxEventCallback(LPUART_HandleTypeDef *hlpuart, uint16_t size);
void HAL_LPUART_ErrorCallback(LPUART_HandleTypeDef *hlpuart);

#endif
