/******************************************************************************
*@file  : hal_eth.c
*@brief : ETH HAL module driver.
******************************************************************************/
#include "math.h"
#include "hal.h"

/*
    ==============================================================================
                    ##### How to use this driver #####
    ==============================================================================
     [..]
     The ETH HAL driver can be used as follows:

      (#)Declare a ETH_HandleTypeDef handle structure, for example:
         ETH_HandleTypeDef  heth;

      (#)Fill parameters of Init structure in heth handle

      (#)Call HAL_ETH_Init() API to initialize the Ethernet peripheral (MAC, DMA, ...)

      (#)Initialize the ETH low level resources through the HAL_ETH_MspInit() API:
          (##) Enable the Ethernet interface clock using
                (+++)  __HAL_RCC_ETHMAC_CLK_ENABLE()
                (+++)  __HAL_RCC_ETHTX_CLK_ENABLE()
                (+++)  __HAL_RCC_ETHRX_CLK_ENABLE()
          (##) Initialize the related GPIO clocks
          (##) Configure Ethernet pinout
          (##) Configure Ethernet NVIC interrupt (in Interrupt mode)

      (#) Ethernet data reception is asynchronous, so call the following API
          to start the listening mode:
          (##) HAL_ETH_Start():
               This API starts the MAC and DMA transmission and reception process,
               without enabling end of transfer interrupts, in this mode user
               has to poll for data reception by calling HAL_ETH_ReadData()
          (##) HAL_ETH_Start_IT():
               This API starts the MAC and DMA transmission and reception process,
               end of transfer interrupts are enabled in this mode,
               HAL_ETH_RxCpltCallback() will be executed when an Ethernet packet is received

      (#) When data is received user can call the following API to get received data:
          (##) HAL_ETH_ReadData(): Read a received packet

      (#) For transmission path, two APIs are available:
         (##) HAL_ETH_Transmit(): Transmit an ETH frame in blocking mode
         (##) HAL_ETH_Transmit_IT(): Transmit an ETH frame in interrupt mode,
              HAL_ETH_TxCpltCallback() will be executed when end of transfer occur

      (#) Communication with an external PHY device:
         (##) HAL_ETH_ReadPHYRegister(): Read a register from an external PHY
         (##) HAL_ETH_WritePHYRegister(): Write data to an external RHY register

      (#) Configure the Ethernet MAC after ETH peripheral initialization
          (##) HAL_ETH_GetMACConfig(): Get MAC actual configuration into ETH_MACConfigTypeDef
          (##) HAL_ETH_SetMACConfig(): Set MAC configuration based on ETH_MACConfigTypeDef

      (#) Configure the Ethernet DMA after ETH peripheral initialization
          (##) HAL_ETH_GetDMAConfig(): Get DMA actual configuration into ETH_DMAConfigTypeDef
          (##) HAL_ETH_SetDMAConfig(): Set DMA configuration based on ETH_DMAConfigTypeDef

      (#) Configure the Ethernet PTP after ETH peripheral initialization
          (##) Define HAL_ETH_USE_PTP to use PTP APIs.
          (##) HAL_ETH_PTP_GetConfig(): Get PTP actual configuration into ETH_PTP_ConfigTypeDef
          (##) HAL_ETH_PTP_SetConfig(): Set PTP configuration based on ETH_PTP_ConfigTypeDef
          (##) HAL_ETH_PTP_GetTime(): Get Seconds and Nanoseconds for the Ethernet PTP registers
          (##) HAL_ETH_PTP_SetTime(): Set Seconds and Nanoseconds for the Ethernet PTP registers
          (##) HAL_ETH_PTP_AddTimeOffset(): Add Seconds and Nanoseconds offset for the Ethernet PTP registers
          (##) HAL_ETH_PTP_InsertTxTimestamp(): Insert Timestamp in transmission
          (##) HAL_ETH_PTP_GetTxTimestamp(): Get transmission timestamp
          (##) HAL_ETH_PTP_GetRxTimestamp(): Get reception timestamp

      -@- The ARP offload feature is not supported in this driver.

      -@- The PTP offload feature is not supported in this driver.

    *** Callback registration ***
    =============================================

    The compilation define  USE_HAL_ETH_REGISTER_CALLBACKS when set to 1
    allows the user to configure dynamically the driver callbacks.
    Use Function HAL_ETH_RegisterCallback() to register an interrupt callback.

    Function HAL_ETH_RegisterCallback() allows to register following callbacks:
    (+) TxCpltCallback   : Tx Complete Callback.
    (+) RxCpltCallback   : Rx Complete Callback.
    (+) ErrorCallback    : Error Callback.
    (+) PMTCallback      : Power Management Callback
    (+) EEECallback      : EEE Callback.
    (+) WakeUpCallback   : Wake UP Callback
    (+) MspInitCallback  : MspInit Callback.
    (+) MspDeInitCallback: MspDeInit Callback.

    This function takes as parameters the HAL peripheral handle, the Callback ID
    and a pointer to the user callback function.

    For specific callbacks RxAllocateCallback use dedicated register callbacks:
    respectively HAL_ETH_RegisterRxAllocateCallback().

    For specific callbacks RxLinkCallback use dedicated register callbacks:
    respectively HAL_ETH_RegisterRxLinkCallback().

    For specific callbacks TxFreeCallback use dedicated register callbacks:
    respectively HAL_ETH_RegisterTxFreeCallback().

    For specific callbacks TxPtpCallback use dedicated register callbacks:
    respectively HAL_ETH_RegisterTxPtpCallback().

    Use function HAL_ETH_UnRegisterCallback() to reset a callback to the default
    weak function.
    HAL_ETH_UnRegisterCallback takes as parameters the HAL peripheral handle,
    and the Callback ID.
    This function allows to reset following callbacks:
    (+) TxCpltCallback   : Tx Complete Callback.
    (+) RxCpltCallback   : Rx Complete Callback.
    (+) ErrorCallback    : Error Callback.
    (+) PMTCallback      : Power Management Callback
    (+) EEECallback      : EEE Callback.
    (+) WakeUpCallback   : Wake UP Callback
    (+) MspInitCallback  : MspInit Callback.
    (+) MspDeInitCallback: MspDeInit Callback.

    For specific callbacks RxAllocateCallback use dedicated unregister callbacks:
    respectively HAL_ETH_UnRegisterRxAllocateCallback().

    For specific callbacks RxLinkCallback use dedicated unregister callbacks:
    respectively HAL_ETH_UnRegisterRxLinkCallback().

    For specific callbacks TxFreeCallback use dedicated unregister callbacks:
    respectively HAL_ETH_UnRegisterTxFreeCallback().

    For specific callbacks TxPtpCallback use dedicated unregister callbacks:
    respectively HAL_ETH_UnRegisterTxPtpCallback().

    By default, after the HAL_ETH_Init and when the state is HAL_ETH_STATE_RESET
    all callbacks are set to the corresponding weak functions:
    examples HAL_ETH_TxCpltCallback(), HAL_ETH_RxCpltCallback().
    Exception done for MspInit and MspDeInit functions that are
    reset to the legacy weak function in the HAL_ETH_Init/ HAL_ETH_DeInit only when
    these callbacks are null (not registered beforehand).
    if not, MspInit or MspDeInit are not null, the HAL_ETH_Init/ HAL_ETH_DeInit
    keep and use the user MspInit/MspDeInit callbacks (registered beforehand)

    Callbacks can be registered/unregistered in HAL_ETH_STATE_READY state only.
    Exception done MspInit/MspDeInit that can be registered/unregistered
    in HAL_ETH_STATE_READY or HAL_ETH_STATE_RESET state,
    thus registered (user) MspInit/DeInit callbacks can be used during the Init/DeInit.
    In that case first register the MspInit/MspDeInit user callbacks
    using HAL_ETH_RegisterCallback() before calling HAL_ETH_DeInit
    or HAL_ETH_Init function.

    When The compilation define USE_HAL_ETH_REGISTER_CALLBACKS is set to 0 or
    not defined, the callback registration feature is not available and all callbacks
    are set to the corresponding weak functions.

    ******************************************************************************
 */

#ifdef HAL_ETH_MODULE_ENABLED

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define ETH_MACCR_MASK          0x7ADF7EF3U
#define ETH_MACFFR_MASK         0x802007FFU
#define ETH_MACFCR_MASK         0xFFFF000EU
#define ETH_MACWTR_MASK         0x00013FFFU
#define ETH_MACVLANTR_MASK      0x000FFFFFU
#define ETH_MACVTIRR_MASK       0x000FFFFFU
#define ETH_MACL4CR_MASK        0x003D0000U
#define ETH_MACL3CR_MASK        0x0000FFFDU

#define ETH_DMAOMR_MASK         0x0721C0FCU
#define ETH_DMABMR_MASK         0x8FFFFFFEU

#define ETH_DMAMR_MASK          0x00007802U
#define ETH_DMASBMR_MASK        0x0000D001U
#define ETH_DMACCR_MASK         0x00013FFFU
#define ETH_DMACTCR_MASK        0x003F1010U
#define ETH_DMACRCR_MASK        0x803F0000U
#define ETH_MACPMTCSR_MASK      (ETH_MACPMTCSR_PD | ETH_MACPMTCSR_WFE | \
                                 ETH_MACPMTCSR_MPE | ETH_MACPMTCSR_GU)
                                 

#define ETH_MAC_L4_SRCP_MASK          0x0000FFFFU
#define ETH_MAC_L4_DSTP_MASK          0xFFFF0000U

/* Timeout values */
#define ETH_SWRESET_TIMEOUT     500U
#define ETH_MDIO_BUS_TIMEOUT    1000U

#define ETH_DMARXDESC_ERRORS_MASK ((uint32_t)(ETH_DMARXDESC_DBE | ETH_DMARXDESC_RE | \
                                              ETH_DMARXDESC_OE  | ETH_DMARXDESC_RWT |\
                                              ETH_DMARXDESC_LC | ETH_DMARXDESC_CE |\
                                              ETH_DMARXDESC_DE | ETH_DMARXDESC_IPHCETSV))

#define ETH_MAC_US_TICK         1000000U

#define ETH_MACTSCR_MASK        0x0087FF2FU

#define ETH_PTPTSHR_VALUE       0xFFFFFFFFU
#define ETH_PTPTSLR_VALUE       0xBB9ACA00U

/* Ethernet MACMIIAR register Mask */
#define ETH_MACMIIAR_CR_MASK    0xFFFFFFE3U

/* Delay to wait when writing to some Ethernet registers */
#define ETH_REG_WRITE_DELAY     0x00000001U

/* ETHERNET MACCR register Mask */
#define ETH_MACCR_CLEAR_MASK    0xFD20810FU

/* ETHERNET MACFCR register Mask */
#define ETH_MACFCR_CLEAR_MASK   0x0000FF41U

/* ETHERNET DMAOMR register Mask */
#define ETH_DMAOMR_CLEAR_MASK   0xF8DE3F23U

/* ETHERNET MAC address offsets */
#define ETH_MAC_ADDR_HBASE      (uint32_t)(ETH_MAC_BASE + 0x40U)  /* ETHERNET MAC address high offset */
#define ETH_MAC_ADDR_LBASE      (uint32_t)(ETH_MAC_BASE + 0x44U)  /* ETHERNET MAC address low offset */

/* ETHERNET DMA Rx descriptors Frame length Shift */
#define  ETH_DMARXDESC_FRAMELENGTHSHIFT            16U

/* Private macros ------------------------------------------------------------*/
/* Helper macros for TX descriptor handling */
#define INCR_TX_DESC_INDEX(inx, offset) do {\
                                             (inx) += (offset);\
                                             if ((inx) >= (uint32_t)ETH_TX_DESC_CNT){\
                                             (inx) = ((inx) - (uint32_t)ETH_TX_DESC_CNT);}\
                                           } while (0)

/* Helper macros for RX descriptor handling */
#define INCR_RX_DESC_INDEX(inx, offset) do {\
                                             (inx) += (offset);\
                                             if ((inx) >= (uint32_t)ETH_RX_DESC_CNT){\
                                             (inx) = ((inx) - (uint32_t)ETH_RX_DESC_CNT);}\
                                           } while (0)

/* Private function prototypes -----------------------------------------------*/
static void ETH_SetMACConfig(ETH_HandleTypeDef *heth, const ETH_MACConfigTypeDef *macconf);
static void ETH_SetDMAConfig(ETH_HandleTypeDef *heth, const ETH_DMAConfigTypeDef *dmaconf);
static void ETH_MACDMAConfig(ETH_HandleTypeDef *heth);
static void ETH_DMATxDescListInit(ETH_HandleTypeDef *heth);
static void ETH_DMARxDescListInit(ETH_HandleTypeDef *heth);
static uint32_t ETH_Prepare_Tx_Descriptors(ETH_HandleTypeDef *heth, const ETH_TxPacketConfigTypeDef *pTxConfig,
                                           uint32_t ItMode);
static void ETH_UpdateDescriptor(ETH_HandleTypeDef *heth);
static void ETH_FlushTransmitFIFO(ETH_HandleTypeDef *heth);
static void ETH_MACAddressConfig(ETH_HandleTypeDef *heth, uint32_t MacAddr, uint8_t *Addr);
static void ETH_DelayBlockConfig(uint32_t uint, uint32_t len);

/* Exported functions ---------------------------------------------------------*/

/** 
 ===============================================================================
            ##### Initialization and Configuration functions #####
 ===============================================================================
    [..]  This subsection provides a set of functions allowing to initialize and
          deinitialize the ETH peripheral:

      (+) User must Implement HAL_ETH_MspInit() function in which he configures
          all related peripherals resources (CLOCK, GPIO and NVIC ).

      (+) Call the function HAL_ETH_Init() to configure the selected device with
          the selected configuration:
        (++) MAC address
        (++) Media interface (MII or RMII)
        (++) Rx DMA Descriptors Tab
        (++) Tx DMA Descriptors Tab
        (++) Length of Rx Buffers

      (+) Call the function HAL_ETH_DeInit() to restore the default configuration
          of the selected ETH peripheral.
  */

/**
  * @brief  Initialize the Ethernet peripheral registers.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Init(ETH_HandleTypeDef *heth)
{
    uint32_t tickstart;
    
    if (heth == NULL)
    {
        return HAL_ERROR;
    }
    
    if (heth->gState == HAL_ETH_STATE_RESET)
    {
        heth->gState = HAL_ETH_STATE_BUSY;

        /* Init the low level hardware : GPIO, CLOCK, NVIC. */
        HAL_ETH_MspInit(heth);
    }

    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* Select MII or RMII Mode*/
    SYSCFG->SYSCR &= ~SYSCFG_SYSCR_EPIS;
    SYSCFG->SYSCR |= (uint32_t)heth->Init.MediaInterface;
    
    /* Ethernet Software reset */
    /* Set the SWR bit: resets all MAC subsystem internal registers and logic */
    /* After reset all the registers holds their respective reset values */
    SET_BIT(heth->Instance->DMABMR, ETH_DMABMR_SWR);

    /* Get tick */
    tickstart = HAL_GetTick();
    
    /*------------------------ Wait for software reset -------------------------*/
    while (READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_SWR) != (uint32_t)RESET)
    {
        if (((HAL_GetTick() - tickstart) > ETH_SWRESET_TIMEOUT))
        {
            /* Set Error Code */
            heth->ErrorCode = HAL_ETH_ERROR_TIMEOUT;
            /* Set State as Error */
            heth->gState = HAL_ETH_STATE_ERROR;
            /* Return Error */
            return HAL_ERROR;
        }
    }
    
    /*--------------- Delay Block of ETH RX Clock Configuration ----------------*/
    ETH_DelayBlockConfig(10, 15);
    //ETH_DelayBlockConfig(1, 5);
    
    /*------------------ MDIO CSR Clock Range Configuration --------------------*/
    HAL_ETH_SetMDIOClockRange(heth);

//    /*------------------ MAC LPI 1US Tic Counter Configuration --------------------*/
//    WRITE_REG(heth->Instance->MAC1USTCR, (((uint32_t)HAL_RCC_GetHCLKFreq() / ETH_MAC_US_TICK) - 1U));

    /*------------------ MAC, MTL and DMA default Configuration ----------------*/
    ETH_MACDMAConfig(heth);

    /*--------- Set Receive Buffers Length (must be a multiple of 4) -----------*/
    if ((heth->Init.RxBuffLen % 0x4U) != 0x0U)
    {
        /* Set Error Code */
        heth->ErrorCode = HAL_ETH_ERROR_PARAM;
        /* Set State as Error */
        heth->gState = HAL_ETH_STATE_ERROR;
        /* Return Error */
        return HAL_ERROR;
    }
    
    /*------------------ DMA Tx Descriptors Configuration ----------------------*/
    ETH_DMATxDescListInit(heth);

    /*------------------ DMA Rx Descriptors Configuration ----------------------*/
    ETH_DMARxDescListInit(heth);
    
    /*-------------------- ETHERNET MAC Address Configuration ------------------*/
    /*----------------------- Set MAC addr bits 32 to 47 -----------------------*/
    heth->Instance->MACA0HR = (((uint32_t)(heth->Init.MACAddr[5]) << 8) | (uint32_t)heth->Init.MACAddr[4]);
    /*------------------------ Set MAC addr bits 0 to 31 -----------------------*/
    heth->Instance->MACA0LR = (((uint32_t)(heth->Init.MACAddr[3]) << 24) | ((uint32_t)(heth->Init.MACAddr[2]) << 16) |
                             ((uint32_t)(heth->Init.MACAddr[1]) << 8) | (uint32_t)heth->Init.MACAddr[0]);

    /*-------------- Disable PMT, Timestamp, and LPI Interrupts ----------------*/
    SET_BIT(heth->Instance->MACIMR, ETH_MACIMR_LPIIM | ETH_MACIMR_TIM | ETH_MACIMR_PIM);

    /*------------------------ Disable Rx MMC Interrupts -----------------------*/
    SET_BIT(heth->Instance->MMCRIMR, ETH_MMCRIMR_RGUFIM | ETH_MMCRIMR_RFAEIM | ETH_MMCRIMR_RFCEIM);

    /*------------------------ Disable Tx MMC Interrupts -----------------------*/
    SET_BIT(heth->Instance->MMCTIMR, ETH_MMCTIMR_TGFIM | ETH_MMCTIMR_TMCGFCIM | ETH_MMCTIMR_TSCGFCIM);

    heth->ErrorCode = HAL_ETH_ERROR_NONE;
    heth->gState = HAL_ETH_STATE_READY;

    return HAL_OK;
}

/**
  * @brief  DeInitializes the ETH peripheral.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_DeInit(ETH_HandleTypeDef *heth)
{
    /* Set the ETH peripheral state to BUSY */
    heth->gState = HAL_ETH_STATE_BUSY;

    /* De-Init the low level hardware : GPIO, CLOCK, NVIC. */
    HAL_ETH_MspDeInit(heth);

    /* Set ETH HAL state to Disabled */
    heth->gState = HAL_ETH_STATE_RESET;

    /* Return function status */
    return HAL_OK;
}

/**
  * @brief  Initializes the ETH MSP.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_MspInit could be implemented in the user file
    */
}

/**
  * @brief  DeInitializes ETH MSP.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_MspDeInit(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_MspDeInit could be implemented in the user file
    */
}
   
/**
  ==============================================================================
                      ##### IO operation functions #####
  ==============================================================================
  [..]
    This subsection provides a set of functions allowing to manage the ETH
    data transfer.
  */

/**
  * @brief  Enables Ethernet MAC and DMA reception and transmission
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Start(ETH_HandleTypeDef *heth)
{
    uint32_t tmpreg1;

    if (heth->gState == HAL_ETH_STATE_READY)
    {
        heth->gState = HAL_ETH_STATE_BUSY;

        /* Set number of descriptors to build */
        heth->RxDescList.RxBuildDescCnt = ETH_RX_DESC_CNT;

        /* Build all descriptors */
        ETH_UpdateDescriptor(heth);

        /* Enable the MAC transmission */
        SET_BIT(heth->Instance->MACCR, ETH_MACCR_TE);

        /* Enable the MAC reception */
        SET_BIT(heth->Instance->MACCR, ETH_MACCR_RE);

        /* Flush Transmit FIFO */
        ETH_FlushTransmitFIFO(heth);

        /* Enable the DMA transmission */
        SET_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_ST);

        /* Enable the DMA reception */
        SET_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_SR);

        heth->gState = HAL_ETH_STATE_STARTED;

        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Enables Ethernet MAC and DMA reception/transmission in Interrupt mode
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Start_IT(ETH_HandleTypeDef *heth)
{
    uint32_t tmpreg1;

    if (heth->gState == HAL_ETH_STATE_READY)
    {
        heth->gState = HAL_ETH_STATE_BUSY;

        /* save IT mode to ETH Handle */
        heth->RxDescList.ItMode = 1U;

        /* Set number of descriptors to build */
        heth->RxDescList.RxBuildDescCnt = ETH_RX_DESC_CNT;

        /* Build all descriptors */
        ETH_UpdateDescriptor(heth);

        /* Enable the DMA transmission */
        SET_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_ST);

        /* Enable the DMA reception */
        SET_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_SR);

        /* Flush Transmit FIFO */
        ETH_FlushTransmitFIFO(heth);

        /* Enable the MAC transmission */
        SET_BIT(heth->Instance->MACCR, ETH_MACCR_TE);

        /* Enable the MAC reception */
        SET_BIT(heth->Instance->MACCR, ETH_MACCR_RE);

        /* Enable ETH DMA interrupts:
        - Tx complete interrupt
        - Rx complete interrupt
        - Fatal bus interrupt
        */
        __HAL_ETH_DMA_ENABLE_IT(heth, (ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE  |
                                       ETH_DMAIER_FBEIE | ETH_DMAIER_AISE | ETH_DMAIER_RBUIE));

        heth->gState = HAL_ETH_STATE_STARTED;
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}


/**
  * @brief  Stop Ethernet MAC and DMA reception/transmission
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Stop(ETH_HandleTypeDef *heth)
{
    uint32_t tmpreg1;

    if (heth->gState == HAL_ETH_STATE_STARTED)
    {
        /* Set the ETH peripheral state to BUSY */
        heth->gState = HAL_ETH_STATE_BUSY;

        /* Disable the DMA transmission */
        CLEAR_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_ST);

        /* Disable the DMA reception */
        CLEAR_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_SR);

        /* Disable the MAC reception */
        CLEAR_BIT(heth->Instance->MACCR, ETH_MACCR_RE);

        /* Flush Transmit FIFO */
        ETH_FlushTransmitFIFO(heth);

        /* Disable the MAC transmission */
        CLEAR_BIT(heth->Instance->MACCR, ETH_MACCR_TE);

        heth->gState = HAL_ETH_STATE_READY;

        /* Return function status */
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    } 
}

/**
  * @brief  Stop Ethernet MAC and DMA reception/transmission in Interrupt mode
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Stop_IT(ETH_HandleTypeDef *heth)
{
    ETH_DMADescTypeDef *dmarxdesc;
    uint32_t descindex;
    uint32_t tmpreg1;

    if (heth->gState == HAL_ETH_STATE_STARTED)
    {
        /* Set the ETH peripheral state to BUSY */
        heth->gState = HAL_ETH_STATE_BUSY;

        __HAL_ETH_DMA_DISABLE_IT(heth, (ETH_DMAIER_NISE | ETH_DMAIER_RIE | ETH_DMAIER_TIE  |
                                        ETH_DMAIER_FBEIE | ETH_DMAIER_AISE | ETH_DMAIER_RBUIE));

        /* Disable the DMA transmission */
        CLEAR_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_ST);

        /* Disable the DMA reception */
        CLEAR_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_SR);

        /* Disable the MAC reception */
        CLEAR_BIT(heth->Instance->MACCR, ETH_MACCR_RE);

        /* Flush Transmit FIFO */
        ETH_FlushTransmitFIFO(heth);

        /* Disable the MAC transmission */
        CLEAR_BIT(heth->Instance->MACCR, ETH_MACCR_TE);

        /* Clear DIC(Disable interrupt on completion) bit to all Rx descriptors */
        for (descindex = 0; descindex < (uint32_t)ETH_RX_DESC_CNT; descindex++)
        {
            dmarxdesc = (ETH_DMADescTypeDef *)heth->RxDescList.RxDesc[descindex];
            SET_BIT(dmarxdesc->DESC1, ETH_DMARXDESC_DIC);
        }

        heth->RxDescList.ItMode = 0U;

        heth->gState = HAL_ETH_STATE_READY;

        /* Return function status */
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Sends an Ethernet Packet in polling mode.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pTxConfig: Hold the configuration of packet to be transmitted
  * @param  Timeout: timeout value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Transmit(ETH_HandleTypeDef *heth, ETH_TxPacketConfigTypeDef *pTxConfig, uint32_t Timeout)
{
    uint32_t tickstart;
    ETH_DMADescTypeDef *dmatxdesc;

    if (pTxConfig == NULL)
    {
        heth->ErrorCode |= HAL_ETH_ERROR_PARAM;
        return HAL_ERROR;
    }

    if (heth->gState == HAL_ETH_STATE_STARTED)
    {
        /* Config DMA Tx descriptor by Tx Packet info */
        if (ETH_Prepare_Tx_Descriptors(heth, pTxConfig, 0) != HAL_ETH_ERROR_NONE)
        {
            /* Set the ETH error code */
            heth->ErrorCode |= HAL_ETH_ERROR_BUSY;
            return HAL_ERROR;
        }

        /* Ensure completion of descriptor preparation before transmission start */
        __DSB();

        dmatxdesc = (ETH_DMADescTypeDef *)(&heth->TxDescList)->TxDesc[heth->TxDescList.CurTxDesc];

        /* Incr current tx desc index */
        INCR_TX_DESC_INDEX(heth->TxDescList.CurTxDesc, 1U);

        /* Start transmission */
        /* issue a poll command to Tx DMA by writing address of next immediate free descriptor */
        WRITE_REG(heth->Instance->DMATPDR, (uint32_t)(heth->TxDescList.TxDesc[heth->TxDescList.CurTxDesc]));

        tickstart = HAL_GetTick();

        /* Wait for data to be transmitted or timeout occurred */
        while ((dmatxdesc->DESC0 & ETH_DMATXDESC_OWN) != (uint32_t)RESET)
        {
            if ((heth->Instance->DMASR & ETH_DMASR_FBES) != (uint32_t)RESET)
            {
                heth->ErrorCode |= HAL_ETH_ERROR_DMA;
                heth->DMAErrorCode = heth->Instance->DMASR;
                /* Return function status */
                return HAL_ERROR;
            }

            /* Check for the Timeout */
            if (Timeout != HAL_MAX_DELAY)
            {
                if (((HAL_GetTick() - tickstart) > Timeout) || (Timeout == 0U))
                {
                    heth->ErrorCode |= HAL_ETH_ERROR_TIMEOUT;
                    /* Clear TX descriptor so that we can proceed */
                    dmatxdesc->DESC0 = (ETH_DMATXDESC_FS | ETH_DMATXDESC_LS);
                    return HAL_ERROR;
                }
            }
        }

        /* Return function status */
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Sends an Ethernet Packet in interrupt mode.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pTxConfig: Hold the configuration of packet to be transmitted
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_Transmit_IT(ETH_HandleTypeDef *heth, ETH_TxPacketConfigTypeDef *pTxConfig)
{
    if (pTxConfig == NULL)
    {
        heth->ErrorCode |= HAL_ETH_ERROR_PARAM;
        return HAL_ERROR;
    }

    if (heth->gState == HAL_ETH_STATE_STARTED)
    {
        /* Save the packet pointer to release.  */
        heth->TxDescList.CurrentPacketAddress = (uint32_t *)pTxConfig->pData;

        /* Config DMA Tx descriptor by Tx Packet info */
        if (ETH_Prepare_Tx_Descriptors(heth, pTxConfig, 1) != HAL_ETH_ERROR_NONE)
        {
            heth->ErrorCode |= HAL_ETH_ERROR_BUSY;
            return HAL_ERROR;
        }
        
//        //������Ϣ
//        {
//        uint32_t index;
//        ETH_DMADescTypeDef *desc0;
//        
//        index = heth->TxDescList.CurTxDesc;
//        desc0 = (ETH_DMADescTypeDef *)heth->TxDescList.TxDesc[index];
//        
//            printf("index[%d], TX_DESC0[%08X]\n", index, desc0->DESC0);
//            printf("index[%d], TX_DESC1[%08X]\n", index, desc0->DESC1);
//        }

        /* Ensure completion of descriptor preparation before transmission start */
        __DSB();
        
        /* Incr current tx desc index */
        INCR_TX_DESC_INDEX(heth->TxDescList.CurTxDesc, 1U);
        
//        //T003֡���϶(IFG)����
//        extern uint8_t IFG_TEST;
//        if(IFG_TEST == 0)
//        {
        /* Start transmission */
        /* issue a poll command to Tx DMA by writing address of next immediate free descriptor */
        WRITE_REG(heth->Instance->DMATPDR, (uint32_t)(heth->TxDescList.TxDesc[heth->TxDescList.CurTxDesc]));
//        }
//   
//        //T003�в���֡ˢ�´���
//        extern uint8_t ERR_TEST_DMA_FLUSHFIFO;
//        if(ERR_TEST_DMA_FLUSHFIFO)
//        {
//            ETH->DMAOMR |= (1 << 20);
//        }
        
        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Read a received packet.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pAppBuff: Pointer to an application buffer to receive the packet.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_ReadData(ETH_HandleTypeDef *heth, void **pAppBuff)
{
    uint32_t descidx;
    ETH_DMADescTypeDef *dmarxdesc;
    uint32_t desccnt = 0U;
    uint32_t desccntmax;
    uint32_t bufflength;
    uint8_t rxdataready = 0U;

    if (pAppBuff == NULL)
    {
        heth->ErrorCode |= HAL_ETH_ERROR_PARAM;
        return HAL_ERROR;
    }

    if (heth->gState != HAL_ETH_STATE_STARTED)
    {
        return HAL_ERROR;
    }

    descidx = heth->RxDescList.RxDescIdx;
    dmarxdesc = (ETH_DMADescTypeDef *)heth->RxDescList.RxDesc[descidx];
    desccntmax = ETH_RX_DESC_CNT - heth->RxDescList.RxBuildDescCnt;

    /* Check if descriptor is not owned by DMA */
    while ((READ_BIT(dmarxdesc->DESC0, ETH_DMARXDESC_OWN) == (uint32_t)RESET) && (desccnt < desccntmax)
           && (rxdataready == 0U))
    {
        if (READ_BIT(dmarxdesc->DESC0,  ETH_DMARXDESC_LS)  != (uint32_t)RESET)
        {
            /* Get timestamp high */
            heth->RxDescList.TimeStamp.TimeStampHigh = dmarxdesc->DESC7;
            /* Get timestamp low */
            heth->RxDescList.TimeStamp.TimeStampLow  = dmarxdesc->DESC6;
        }
        if ((READ_BIT(dmarxdesc->DESC0, ETH_DMARXDESC_FS) != (uint32_t)RESET) || (heth->RxDescList.pRxStart != NULL))
        {
            /* Check if first descriptor */
            if (READ_BIT(dmarxdesc->DESC0, ETH_DMARXDESC_FS) != (uint32_t)RESET)
            {
                heth->RxDescList.RxDescCnt = 0;
                heth->RxDescList.RxDataLength = 0;
            }

            /* Get the Frame Length of the received packet */
            bufflength = ((dmarxdesc->DESC0 & ETH_DMARXDESC_FL) >> ETH_DMARXDESC_FRAMELENGTHSHIFT);

            /* Check if last descriptor */
            if (READ_BIT(dmarxdesc->DESC0, ETH_DMARXDESC_LS) != (uint32_t)RESET)
            {
                /* Save Last descriptor index */
                //heth->RxDescList.pRxLastRxDesc = dmarxdesc->DESC0;
                heth->RxDescList.pRxLastRxDesc = descidx;

                /* Packet ready */
                rxdataready = 1;
            }

            /* Link data */
            WRITE_REG(dmarxdesc->BackupAddr0, dmarxdesc->DESC2);
            /* Link callback */
            HAL_ETH_RxLinkCallback(&heth->RxDescList.pRxStart, &heth->RxDescList.pRxEnd,
                             (uint8_t *)dmarxdesc->BackupAddr0, (uint16_t) bufflength);

            heth->RxDescList.RxDescCnt++;
            //heth->RxDescList.RxDataLength += bufflength; //bufferLength represents the cumulative length.
            heth->RxDescList.RxDataLength = bufflength;

            /* Clear buffer pointer */
            dmarxdesc->BackupAddr0 = 0;
        }

        /* Increment current rx descriptor index */
        INCR_RX_DESC_INDEX(descidx, 1U);
        /* Get current descriptor address */
        dmarxdesc = (ETH_DMADescTypeDef *)heth->RxDescList.RxDesc[descidx];
        desccnt++;
    }

    heth->RxDescList.RxBuildDescCnt += desccnt;
    if ((heth->RxDescList.RxBuildDescCnt) != 0U)
    {
        /* Update Descriptors */
        ETH_UpdateDescriptor(heth);
    }

    heth->RxDescList.RxDescIdx = descidx;

    if (rxdataready == 1U)
    {
        /* Return received packet */
        *pAppBuff = heth->RxDescList.pRxStart;
        /* Reset first element */
        heth->RxDescList.pRxStart = NULL;

        return HAL_OK;
    }

    /* Packet not ready */
    return HAL_ERROR;
}

/**
  * @brief  This function gives back Rx Desc of the last received Packet
  *         to the DMA, so ETH DMA will be able to use these descriptors
  *         to receive next Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
static void ETH_UpdateDescriptor(ETH_HandleTypeDef *heth)
{
    uint32_t descidx;
    uint32_t tailidx;
    uint32_t desccount;
    ETH_DMADescTypeDef *dmarxdesc;
    uint8_t *buff = NULL;
    uint8_t allocStatus = 1U;

    descidx = heth->RxDescList.RxBuildDescIdx;
    dmarxdesc = (ETH_DMADescTypeDef *)heth->RxDescList.RxDesc[descidx];
    desccount = heth->RxDescList.RxBuildDescCnt;

    while ((desccount > 0U) && (allocStatus != 0U))
    {
        /* Check if a buffer's attached the descriptor */
        if (READ_REG(dmarxdesc->BackupAddr0) == 0U)
        {
            /* Get a new buffer. */
            /* Allocate callback */
            HAL_ETH_RxAllocateCallback(&buff);
            if (buff == NULL)
            {
                allocStatus = 0U;
            }
            else
            {
                WRITE_REG(dmarxdesc->BackupAddr0, (uint32_t)buff);
                WRITE_REG(dmarxdesc->DESC2, (uint32_t)buff);
            }
        }

        if (allocStatus != 0U)
        {
            if (heth->RxDescList.ItMode == 0U)
            {
                WRITE_REG(dmarxdesc->DESC1, heth->Init.RxBuffLen | ETH_DMARXDESC_DIC | ETH_DMARXDESC_RCH);
            }
            else
            {
                WRITE_REG(dmarxdesc->DESC1, heth->Init.RxBuffLen | ETH_DMARXDESC_RCH);
            }
            
            SET_BIT(dmarxdesc->DESC0, ETH_DMARXDESC_OWN);

            /* Increment current rx descriptor index */
            INCR_RX_DESC_INDEX(descidx, 1U);
            /* Get current descriptor address */
            dmarxdesc = (ETH_DMADescTypeDef *)heth->RxDescList.RxDesc[descidx];
            desccount--;
        }
    }

    if (heth->RxDescList.RxBuildDescCnt != desccount)
    {
        /* Set the tail pointer index */
        tailidx = (ETH_RX_DESC_CNT + descidx - 1U) % ETH_RX_DESC_CNT;

        /* DMB instruction to avoid race condition */
        __DMB();

        /* Set the Tail pointer address */
        WRITE_REG(heth->Instance->DMARPDR, ((uint32_t)(heth->Init.RxDesc + (tailidx))));

        heth->RxDescList.RxBuildDescIdx = descidx;
        heth->RxDescList.RxBuildDescCnt = desccount;
    }
}

/**
  * @brief  Rx Allocate callback.
  * @param  buff: pointer to allocated buffer
  * @retval None
  */
__weak void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(buff);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_RxAllocateCallback could be implemented in the user file
    */
}

/**
  * @brief  Rx Link callback.
  * @param  pStart: pointer to packet start
  * @param  pEnd: pointer to packet end
  * @param  buff: pointer to received data
  * @param  Length: received data length
  * @retval None
  */
__weak void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff, uint16_t Length)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(pStart);
    UNUSED(pEnd);
    UNUSED(buff);
    UNUSED(Length);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_RxLinkCallback could be implemented in the user file
    */
}

/**
  * @brief  Get the error state of the last received packet.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pErrorCode: pointer to uint32_t to hold the error code
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_GetRxDataErrorCode(const ETH_HandleTypeDef *heth, uint32_t *pErrorCode)
{
    /* Get error bits. */
    *pErrorCode = READ_BIT(heth->RxDescList.pRxLastRxDesc, ETH_DMARXDESC_ERRORS_MASK);

    return HAL_OK;
}

/**
  * @brief  Tx Free callback.
  * @param  buff: pointer to buffer to free
  * @retval None
  */
__weak void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(buff);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_TxFreeCallback could be implemented in the user file
    */
}

/**
  * @brief  Release transmitted Tx packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_ReleaseTxPacket(ETH_HandleTypeDef *heth)
{
    ETH_TxDescListTypeDef *dmatxdesclist = &heth->TxDescList;
    uint32_t numOfBuf =  dmatxdesclist->BuffersInUse;
    uint32_t idx =       dmatxdesclist->releaseIndex;
    uint8_t pktTxStatus = 1U;
    uint8_t pktInUse;
#ifdef HAL_ETH_USE_PTP
    ETH_TimeStampTypeDef *timestamp = &heth->TxTimestamp;
#endif /* HAL_ETH_USE_PTP */

    /* Loop through buffers in use.  */
    while ((numOfBuf != 0U) && (pktTxStatus != 0U))
    {
        pktInUse = 1U;
        numOfBuf--;
        /* If no packet, just examine the next packet.  */
        if (dmatxdesclist->PacketAddress[idx] == NULL)
        {
            /* No packet in use, skip to next.  */
            INCR_TX_DESC_INDEX(idx, 1U);
            pktInUse = 0U;
        }

        if (pktInUse != 0U)
        {
            /* Determine if the packet has been transmitted.  */
            if ((heth->Init.TxDesc[idx].DESC0 & ETH_DMATXDESC_OWN) == 0U)
            {
#ifdef HAL_ETH_USE_PTP
                if ((heth->Init.TxDesc[idx].DESC3 & ETH_DMATXDESC_LS)
                    && (heth->Init.TxDesc[idx].DESC3 & ETH_DMATXDESC_TTSS))
                {
                    /* Get timestamp low */
                    timestamp->TimeStampLow = heth->Init.TxDesc[idx].DESC6;
                    /* Get timestamp high */
                    timestamp->TimeStampHigh = heth->Init.TxDesc[idx].DESC7;
                }
                else
                {
                    timestamp->TimeStampHigh = timestamp->TimeStampLow = UINT32_MAX;
                }
#endif /* HAL_ETH_USE_PTP */

        /* Call callbacks */
#ifdef HAL_ETH_USE_PTP
            /* Handle Ptp  */
                if (timestamp->TimeStampHigh != UINT32_MAX && timestamp->TimeStampLow != UINT32_MAX)
                {
                    HAL_ETH_TxPtpCallback(dmatxdesclist->PacketAddress[idx], timestamp);
                }
#endif  /* HAL_ETH_USE_PTP */
                /* Release the packet.  */
                HAL_ETH_TxFreeCallback(dmatxdesclist->PacketAddress[idx]);

                /* Clear the entry in the in-use array.  */
                dmatxdesclist->PacketAddress[idx] = NULL;

                /* Update the transmit relesae index and number of buffers in use.  */
                INCR_TX_DESC_INDEX(idx, 1U);
                dmatxdesclist->BuffersInUse = numOfBuf;
                dmatxdesclist->releaseIndex = idx;
            }
            else
            {
                /* Get out of the loop!  */
                pktTxStatus = 0U;
            }
        }
    }
    return HAL_OK;
}

#ifdef HAL_ETH_USE_PTP
/**
  * @brief  Set the Ethernet PTP configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  ptpconfig: pointer to a ETH_PTP_ConfigTypeDef structure that contains
  *         the configuration information for PTP
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_SetConfig(ETH_HandleTypeDef *heth, ETH_PTP_ConfigTypeDef *ptpconfig)
{
  uint32_t tmpTSCR;
  ETH_TimeTypeDef time;

  if (ptpconfig == NULL)
  {
    return HAL_ERROR;
  }

  tmpTSCR = ptpconfig->Timestamp |
            ((uint32_t)ptpconfig->TimestampUpdate << ETH_PTPTSCR_TSFCU_Pos) |
            ((uint32_t)ptpconfig->TimestampAll << ETH_PTPTSCR_TSSARFE_Pos) |
            ((uint32_t)ptpconfig->TimestampRolloverMode << ETH_PTPTSCR_TSSSR_Pos) |
            ((uint32_t)ptpconfig->TimestampV2 << ETH_PTPTSCR_TSPTPPSV2E_Pos) |
            ((uint32_t)ptpconfig->TimestampEthernet << ETH_PTPTSCR_TSSPTPOEFE_Pos) |
            ((uint32_t)ptpconfig->TimestampIPv6 << ETH_PTPTSCR_TSSIPV6FE_Pos) |
            ((uint32_t)ptpconfig->TimestampIPv4 << ETH_PTPTSCR_TSSIPV4FE_Pos) |
            ((uint32_t)ptpconfig->TimestampEvent << ETH_PTPTSCR_TSSEME_Pos) |
            ((uint32_t)ptpconfig->TimestampMaster << ETH_PTPTSCR_TSSMRME_Pos) |
            ((uint32_t)ptpconfig->TimestampFilter << ETH_PTPTSCR_TSPFFMAE_Pos) |
            ((uint32_t)ptpconfig->TimestampClockType << ETH_PTPTSCR_TSCNT_Pos);

  /* Write to MACTSCR */
  MODIFY_REG(heth->Instance->PTPTSCR, ETH_MACTSCR_MASK, tmpTSCR);

  /* Enable Timestamp */
  SET_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSE);
  WRITE_REG(heth->Instance->PTPSSIR, ptpconfig->TimestampSubsecondInc);
  WRITE_REG(heth->Instance->PTPTSAR, ptpconfig->TimestampAddend);

  /* Enable Timestamp */
  if (ptpconfig->TimestampAddendUpdate == ENABLE)
  {
    SET_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSARU);
    while ((heth->Instance->PTPTSCR & ETH_PTPTSCR_TSARU) != 0)
    {

    }
  }

  /* Ptp Init */
  SET_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSSTI);

  /* Set PTP Configuration done */
  heth->IsPtpConfigured = HAL_ETH_PTP_CONFIGURED;

  /* Set Seconds */
  time.Seconds = heth->Instance->PTPTSHR;
  /* Set NanoSeconds */
  time.NanoSeconds = heth->Instance->PTPTSLR;

  HAL_ETH_PTP_SetTime(heth, &time);

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Get the Ethernet PTP configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  ptpconfig: pointer to a ETH_PTP_ConfigTypeDef structure that contains
  *         the configuration information for PTP
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_GetConfig(ETH_HandleTypeDef *heth, ETH_PTP_ConfigTypeDef *ptpconfig)
{
  if (ptpconfig == NULL)
  {
    return HAL_ERROR;
  }
  ptpconfig->Timestamp = READ_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSE);
  ptpconfig->TimestampUpdate = ((READ_BIT(heth->Instance->PTPTSCR,
                                          ETH_PTPTSCR_TSFCU) >> ETH_PTPTSCR_TSFCU_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampAll = ((READ_BIT(heth->Instance->PTPTSCR,
                                       ETH_PTPTSCR_TSSARFE) >> ETH_PTPTSCR_TSSARFE_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampRolloverMode = ((READ_BIT(heth->Instance->PTPTSCR,
                                                ETH_PTPTSCR_TSSSR) >> ETH_PTPTSCR_TSSSR_Pos) > 0U)
                                     ? ENABLE : DISABLE;
  ptpconfig->TimestampV2 = ((READ_BIT(heth->Instance->PTPTSCR,
                                      ETH_PTPTSCR_TSPTPPSV2E) >> ETH_PTPTSCR_TSPTPPSV2E_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampEthernet = ((READ_BIT(heth->Instance->PTPTSCR,
                                            ETH_PTPTSCR_TSSPTPOEFE) >> ETH_PTPTSCR_TSSPTPOEFE_Pos) > 0U)
                                 ? ENABLE : DISABLE;
  ptpconfig->TimestampIPv6 = ((READ_BIT(heth->Instance->PTPTSCR,
                                        ETH_PTPTSCR_TSSIPV6FE) >> ETH_PTPTSCR_TSSIPV6FE_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampIPv4 = ((READ_BIT(heth->Instance->PTPTSCR,
                                        ETH_PTPTSCR_TSSIPV4FE) >> ETH_PTPTSCR_TSSIPV4FE_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampEvent = ((READ_BIT(heth->Instance->PTPTSCR,
                                         ETH_PTPTSCR_TSSEME) >> ETH_PTPTSCR_TSSEME_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampMaster = ((READ_BIT(heth->Instance->PTPTSCR,
                                          ETH_PTPTSCR_TSSMRME) >> ETH_PTPTSCR_TSSMRME_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampFilter = ((READ_BIT(heth->Instance->PTPTSCR,
                                          ETH_PTPTSCR_TSPFFMAE) >> ETH_PTPTSCR_TSPFFMAE_Pos) > 0U) ? ENABLE : DISABLE;
  ptpconfig->TimestampClockType = ((READ_BIT(heth->Instance->PTPTSCR,
                                             ETH_PTPTSCR_TSCNT) >> ETH_PTPTSCR_TSCNT_Pos) > 0U) ? ENABLE : DISABLE;

  /* Return function status */
  return HAL_OK;
}

/**
  * @brief  Set Seconds and Nanoseconds for the Ethernet PTP registers.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  time: pointer to a ETH_TimeTypeDef structure that contains
  *         time to set
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_SetTime(ETH_HandleTypeDef *heth, ETH_TimeTypeDef *time)
{
  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    /* Set Seconds */
    heth->Instance->PTPTSHUR = time->Seconds;

    /* Set NanoSeconds */
    heth->Instance->PTPTSLUR = time->NanoSeconds;

    /* the system time is updated */
    SET_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSSTU);

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Get Seconds and Nanoseconds for the Ethernet PTP registers.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  time: pointer to a ETH_TimeTypeDef structure that contains
  *         time to get
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_GetTime(ETH_HandleTypeDef *heth, ETH_TimeTypeDef *time)
{
  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    /* Get Seconds */
    time->Seconds = heth->Instance->PTPTSHR;
    /* Get NanoSeconds */
    time->NanoSeconds = heth->Instance->PTPTSLR;

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Update time for the Ethernet PTP registers.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  timeoffset: pointer to a ETH_PtpUpdateTypeDef structure that contains
  *         the time update information
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_AddTimeOffset(ETH_HandleTypeDef *heth, ETH_PtpUpdateTypeDef ptpoffsettype,
                                            ETH_TimeTypeDef *timeoffset)
{
  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    if (ptpoffsettype ==  HAL_ETH_PTP_NEGATIVE_UPDATE)
    {
      /* Set Seconds update */
      heth->Instance->PTPTSHUR = ETH_PTPTSHR_VALUE - timeoffset->Seconds + 1U;

      if (READ_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSSSR) == ETH_PTPTSCR_TSSSR)
      {
        /* Set nanoSeconds update */
        heth->Instance->PTPTSLUR = ETH_PTPTSLR_VALUE - timeoffset->NanoSeconds;
      }
      else
      {
        heth->Instance->PTPTSLUR = ETH_PTPTSHR_VALUE - timeoffset->NanoSeconds + 1U;
      }
    }
    else
    {
      /* Set Seconds update */
      heth->Instance->PTPTSHUR = timeoffset->Seconds;
      /* Set nanoSeconds update */
      heth->Instance->PTPTSLUR = timeoffset->NanoSeconds;
    }

    SET_BIT(heth->Instance->PTPTSCR, ETH_PTPTSCR_TSSTU);

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Insert Timestamp in transmission.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_InsertTxTimestamp(ETH_HandleTypeDef *heth)
{
  ETH_TxDescListTypeDef *dmatxdesclist = &heth->TxDescList;
  uint32_t descidx = dmatxdesclist->CurTxDesc;
  ETH_DMADescTypeDef *dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[descidx];

  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    /* Enable Time Stamp transmission */
    SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_TTSE);

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Get transmission timestamp.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  timestamp: pointer to ETH_TIMESTAMPTypeDef structure that contains
  *         transmission timestamp
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_GetTxTimestamp(ETH_HandleTypeDef *heth, ETH_TimeStampTypeDef *timestamp)
{
  ETH_TxDescListTypeDef *dmatxdesclist = &heth->TxDescList;
  uint32_t idx =       dmatxdesclist->releaseIndex;
  ETH_DMADescTypeDef *dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[idx];

  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    /* Get timestamp low */
    timestamp->TimeStampLow = dmatxdesc->DESC0;
    /* Get timestamp high */
    timestamp->TimeStampHigh = dmatxdesc->DESC1;

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Get receive timestamp.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  timestamp: pointer to ETH_TIMESTAMPTypeDef structure that contains
  *         receive timestamp
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_PTP_GetRxTimestamp(ETH_HandleTypeDef *heth, ETH_TimeStampTypeDef *timestamp)
{
  if (heth->IsPtpConfigured == HAL_ETH_PTP_CONFIGURED)
  {
    /* Get timestamp low */
    timestamp->TimeStampLow = heth->RxDescList.TimeStamp.TimeStampLow;
    /* Get timestamp high */
    timestamp->TimeStampHigh = heth->RxDescList.TimeStamp.TimeStampHigh;

    /* Return function status */
    return HAL_OK;
  }
  else
  {
    /* Return function status */
    return HAL_ERROR;
  }
}

/**
  * @brief  Register the Tx Ptp callback.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  txPtpCallback: Function to handle Ptp transmission
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_RegisterTxPtpCallback(ETH_HandleTypeDef *heth, pETH_txPtpCallbackTypeDef txPtpCallback)
{
  if (txPtpCallback == NULL)
  {
    /* No buffer to save */
    return HAL_ERROR;
  }
  /* Set Function to handle Tx Ptp */
  heth->txPtpCallback = txPtpCallback;

  return HAL_OK;
}

/**
  * @brief  Unregister the Tx Ptp callback.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_UnRegisterTxPtpCallback(ETH_HandleTypeDef *heth)
{
  /* Set function to allocate buffer */
  heth->txPtpCallback = HAL_ETH_TxPtpCallback;

  return HAL_OK;
}

/**
  * @brief  Tx Ptp callback.
  * @param  buff: pointer to application buffer
  * @param  timestamp: pointer to ETH_TimeStampTypeDef structure that contains
  *         transmission timestamp
  * @retval None
  */
__weak void HAL_ETH_TxPtpCallback(uint32_t *buff, ETH_TimeStampTypeDef *timestamp)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(buff);
  /* NOTE : This function Should not be modified, when the callback is needed,
  the HAL_ETH_TxPtpCallback could be implemented in the user file
  */
}
#endif  /* HAL_ETH_USE_PTP */
    


/**
  * @brief  This function handles ETH interrupt request.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
void HAL_ETH_IRQHandler(ETH_HandleTypeDef *heth)
{
    uint32_t mac_flag = READ_REG(heth->Instance->MACISR);
    uint32_t dma_flag = READ_REG(heth->Instance->DMASR);
    uint32_t dma_itsource = READ_REG(heth->Instance->DMAIER);
//    uint32_t exti_flag = READ_REG(EXTI->PR);
    
    /* Packet received */
    if (((dma_flag & ETH_DMASR_RS) != 0U) && ((dma_itsource & ETH_DMAIER_RIE) != 0U))
    {
        /* Clear the Eth DMA Rx IT pending bits */
        __HAL_ETH_DMA_CLEAR_IT(heth, ETH_DMASR_RS | ETH_DMASR_NIS);

        /* Receive complete callback */
        HAL_ETH_RxCpltCallback(heth);
        return;
    }

    /* Packet transmitted */
    if (((dma_flag & ETH_DMASR_TS) != 0U) && ((dma_itsource & ETH_DMAIER_TIE) != 0U))
    {
        /* Clear the Eth DMA Tx IT pending bits */
        __HAL_ETH_DMA_CLEAR_IT(heth, ETH_DMASR_TS | ETH_DMASR_NIS);

        /* Transfer complete callback */
        HAL_ETH_TxCpltCallback(heth);
        
        return;
    }

    /* ETH DMA Error */
    if (((dma_flag & ETH_DMASR_AIS) != 0U) && ((dma_itsource & ETH_DMAIER_AISE) != 0U))
    {
        heth->ErrorCode |= HAL_ETH_ERROR_DMA;
        /* if fatal bus error occurred */
        if ((dma_flag & ETH_DMASR_FBES) != 0U)
        {
            /* Get DMA error code  */
            heth->DMAErrorCode = READ_BIT(heth->Instance->DMASR, (ETH_DMASR_FBES | ETH_DMASR_TPS | ETH_DMASR_RPS));

            /* Disable all interrupts */
            __HAL_ETH_DMA_DISABLE_IT(heth, ETH_DMAIER_NISE | ETH_DMAIER_AISE);

            /* Set HAL state to ERROR */
            heth->gState = HAL_ETH_STATE_ERROR;
        }
        else
        {
            /* Get DMA error status  */
            heth->DMAErrorCode = READ_BIT(heth->Instance->DMASR, (ETH_DMASR_ETS | ETH_DMASR_RWTS |
                                                                ETH_DMASR_RBUS | ETH_DMASR_AIS));

            /* Clear the interrupt summary flag */
            __HAL_ETH_DMA_CLEAR_IT(heth, (ETH_DMASR_ETS | ETH_DMASR_RWTS |
                                        ETH_DMASR_RBUS | ETH_DMASR_AIS));
        }

        /* Ethernet DMA Error callback */
        HAL_ETH_ErrorCallback(heth);
    }

    /* ETH PMT IT */
    if ((mac_flag & ETH_MAC_PMT_IT) != 0U)
    {
        /* Get MAC Wake-up source and clear the status register pending bit */
        heth->MACWakeUpEvent = READ_BIT(heth->Instance->MACPMTCSR, (ETH_MACPMTCSR_WFR | ETH_MACPMTCSR_MPR));

        /* Ethernet PMT callback */
        HAL_ETH_PMTCallback(heth);

        heth->MACWakeUpEvent = (uint32_t)(0x0U);
    }

//    /* check ETH WAKEUP exti flag */
//    if ((exti_flag & ETH_WAKEUP_EXTI_LINE) != 0U)
//    {
//        /* Clear ETH WAKEUP Exti pending bit */
//        __HAL_ETH_WAKEUP_EXTI_CLEAR_FLAG(ETH_WAKEUP_EXTI_LINE);

//        /* ETH WAKEUP callback */
//        HAL_ETH_WakeUpCallback(heth);
//    }
}


/**
  * @brief  Tx Transfer completed callbacks.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_TxCpltCallback could be implemented in the user file
    */
}

/**
  * @brief  Rx Transfer completed callbacks.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_RxCpltCallback could be implemented in the user file
    */
}

/**
  * @brief  Ethernet transfer error callbacks
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_ErrorCallback could be implemented in the user file
    */
}

/**
  * @brief  Ethernet Power Management module IT callback
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_PMTCallback(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
    the HAL_ETH_PMTCallback could be implemented in the user file
    */
}


/**
  * @brief  ETH WAKEUP interrupt callback
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
__weak void HAL_ETH_WakeUpCallback(ETH_HandleTypeDef *heth)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(heth);
    /* NOTE : This function Should not be modified, when the callback is needed,
            the HAL_ETH_WakeUpCallback could be implemented in the user file
    */
}

/**
  * @brief  Read a PHY register
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  PHYAddr: PHY port address, must be a value from 0 to 31
  * @param  PHYReg: PHY register address, must be a value from 0 to 31
  * @param pRegValue: parameter to hold read value
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_ReadPHYRegister(ETH_HandleTypeDef *heth, uint32_t PHYAddr, uint32_t PHYReg,
                                          uint32_t *pRegValue)
{
    uint32_t tmpreg;
    uint32_t tickstart;
      
    /* Check for the Busy flag */
    if (READ_BIT(heth->Instance->MACMIIAR, ETH_MACMIIAR_MB) != (uint32_t)RESET)
    {
        return HAL_ERROR;
    }

    /* Get the  MACMIIAR value */
    WRITE_REG(tmpreg, heth->Instance->MACMIIAR);
    
    /* Prepare the MDIO Address Register value
       - Set the PHY device address
       - Set the PHY register address
       - Set the read mode
       - Set the MII Busy bit */

    MODIFY_REG(tmpreg, ETH_MACMIIAR_PA, (PHYAddr << 11));
    MODIFY_REG(tmpreg, ETH_MACMIIAR_MR, (PHYReg << 6));
    CLEAR_BIT(tmpreg, ETH_MACMIIAR_MW);
    SET_BIT(tmpreg, ETH_MACMIIAR_MB);
    
    /* Write the result value into the MDII Address register */
    WRITE_REG(heth->Instance->MACMIIAR, tmpreg);

    tickstart = HAL_GetTick();

    /* Wait for the Busy flag */
    while (READ_BIT(heth->Instance->MACMIIAR, ETH_MACMIIAR_MB) > 0U)
    {
        if (((HAL_GetTick() - tickstart) > ETH_MDIO_BUS_TIMEOUT))
        {
            return HAL_ERROR;
        }
    }

    /* Get MACMIIDR value */
    WRITE_REG(*pRegValue, (uint16_t)heth->Instance->MACMIIDR);

    return HAL_OK;
}

/**
  * @brief  Writes to a PHY register.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  PHYAddr: PHY port address, must be a value from 0 to 31
  * @param  PHYReg: PHY register address, must be a value from 0 to 31
  * @param  RegValue: the value to write
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_WritePHYRegister(const ETH_HandleTypeDef *heth, uint32_t PHYAddr, uint32_t PHYReg,
                                           uint32_t RegValue)
{
    uint32_t tickstart;
    uint32_t tmpreg;

    /* Check for the Busy flag */
    if (READ_BIT(heth->Instance->MACMIIAR, ETH_MACMIIAR_MB) != (uint32_t)RESET)
    {
        return HAL_ERROR;
    }

    /* Get the  MACMIIAR value */
    WRITE_REG(tmpreg, heth->Instance->MACMIIAR);
    
    /* Prepare the MDIO Address Register value
       - Set the PHY device address
       - Set the PHY register address
       - Set the read mode
       - Set the MII Busy bit */

    MODIFY_REG(tmpreg, ETH_MACMIIAR_PA, (PHYAddr << 11));
    MODIFY_REG(tmpreg, ETH_MACMIIAR_MR, (PHYReg << 6));
    SET_BIT(tmpreg, ETH_MACMIIAR_MW);
    SET_BIT(tmpreg, ETH_MACMIIAR_MB);
    
    /* Give the value to the MII data register */
    WRITE_REG(heth->Instance->MACMIIDR, (uint16_t)RegValue); 

    /* Write the result value into the MDII Address register */
    WRITE_REG(heth->Instance->MACMIIAR, tmpreg);

    tickstart = HAL_GetTick();

    /* Wait for the Busy flag */
    while (READ_BIT(heth->Instance->MACMIIAR, ETH_MACMIIAR_MB) > 0U)
    {
        if (((HAL_GetTick() - tickstart) > ETH_MDIO_BUS_TIMEOUT))
        {
            return HAL_ERROR;
        }
    }

    return HAL_OK;
}

/** 
  ==============================================================================
                      ##### Peripheral Control functions #####
  ==============================================================================
  [..]
    This subsection provides a set of functions allowing to control the ETH
    peripheral.
  */

/**
  * @brief  Get the configuration of the MAC and MTL subsystems.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  macconf: pointer to a ETH_MACConfigTypeDef structure that will hold
  *         the configuration of the MAC.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_ETH_GetMACConfig(const ETH_HandleTypeDef *heth, ETH_MACConfigTypeDef *macconf)
{
    if (macconf == NULL)
    {
        return HAL_ERROR;
    }

    /* Get MAC parameters */
    macconf->SourceAddrControl = READ_BIT(heth->Instance->MACCR, ETH_MACCR_SARC);
    macconf->Support2KPacket = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_S2KP) >> 22) > 0U) ? ENABLE : DISABLE;
    macconf->CRCStripTypePacket = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_CSTF) >> 25U) > 0U) ? ENABLE : DISABLE;
    macconf->Watchdog = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_WD) >> 23) == 0U) ? ENABLE : DISABLE;
    macconf->Jabber = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_JD) >> 22) == 0U) ? ENABLE : DISABLE;
    macconf->JumboPacket = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_JE) >> 16) > 0U) ? ENABLE : DISABLE;
    macconf->InterPacketGapVal = READ_BIT(heth->Instance->MACCR, ETH_MACCR_IFG);
    macconf->CarrierSenseDuringTransmit = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_CSD) >> 16) > 0U)
                                        ? ENABLE : DISABLE;
    macconf->Speed = READ_BIT(heth->Instance->MACCR, ETH_MACCR_FES);
    macconf->ReceiveOwn = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_ROD) >> 13) == 0U) ? ENABLE : DISABLE;
    macconf->LoopbackMode = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_LM) >> 12) > 0U) ? ENABLE : DISABLE;
    macconf->DuplexMode = READ_BIT(heth->Instance->MACCR, ETH_MACCR_DM);
    macconf->ChecksumOffload = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_IPCO) >> 10U) > 0U) ? ENABLE : DISABLE;
    macconf->RetryTransmission = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_DR) >> 9) == 0U) ? ENABLE : DISABLE;
    macconf->AutomaticPadCRCStrip = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_APCS) >> 7) > 0U) ? ENABLE : DISABLE;
    macconf->BackOffLimit = READ_BIT(heth->Instance->MACCR, ETH_MACCR_BL);
    macconf->DeferralCheck = ((READ_BIT(heth->Instance->MACCR, ETH_MACCR_DC) >> 4) > 0U) ? ENABLE : DISABLE;
    macconf->PreambleLength = READ_BIT(heth->Instance->MACCR, ETH_MACCR_PRELEN);
    macconf->PauseTime = (READ_BIT(heth->Instance->MACFCR, ETH_MACFCR_PT) >> 16); 
    macconf->UnicastPausePacketDetect = ((READ_BIT(heth->Instance->MACFCR, ETH_MACFCR_UPFD) >> 3U) > 0U)
                                      ? ENABLE : DISABLE;
    macconf->ReceiveFlowControl = ((READ_BIT(heth->Instance->MACFCR, ETH_MACFCR_RFCE) >> 2U) > 0U) ? ENABLE : DISABLE;
    macconf->TransmitFlowControl = ((READ_BIT(heth->Instance->MACFCR, ETH_MACFCR_TFCE) >> 1) > 0U) ? ENABLE : DISABLE;
    macconf->ProgrammableWatchdog = ((READ_BIT(heth->Instance->MACWTR, ETH_MACWTR_PWE) >> 8) > 0U) ? ENABLE : DISABLE;
    macconf->WatchdogTimeout = READ_BIT(heth->Instance->MACWTR, ETH_MACWTR_WTO);    

    return HAL_OK;
}

/**
  * @brief  Get the configuration of the DMA.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  dmaconf: pointer to a ETH_DMAConfigTypeDef structure that will hold
  *         the configuration of the ETH DMA.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_ETH_GetDMAConfig(const ETH_HandleTypeDef *heth, ETH_DMAConfigTypeDef *dmaconf)
{
    if (dmaconf == NULL)
    {
        return HAL_ERROR;
    }

    dmaconf->RebuildINCRxBurst = ((READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_RIB) >> 31) > 0U) ? ENABLE : DISABLE;
    dmaconf->DMAArbitration = READ_BIT(heth->Instance->DMABMR, (ETH_DMABMR_TXPR | ETH_DMABMR_PR | ETH_DMABMR_DA));
    dmaconf->AddressAlignedBeats = ((READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_RIB) >> 25U) > 0U) ? ENABLE : DISABLE;
    dmaconf->BurstMode = READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_FB | ETH_DMABMR_MB);
    dmaconf->RxDMABurstLength = READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_RDP | ETH_DMABMR_EPM);
    dmaconf->TxDMABurstLength = READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_PBL | ETH_DMABMR_EPM);
    dmaconf->EnhancedDescriptorFormat = ((READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_EDFE) >> 7) > 0U) ? ENABLE : DISABLE;
    dmaconf->DescriptorSkipLength = READ_BIT(heth->Instance->DMABMR, ETH_DMABMR_DSL) >> 2;

    dmaconf->DropTCPIPChecksumErrorFrame = ((READ_BIT(heth->Instance->DMAOMR,
                                                    ETH_DMAOMR_DTCEFD) >> 26) > 0U) ? DISABLE : ENABLE;
    dmaconf->ReceiveStoreForward = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_RSF) >> 25) > 0U) ? ENABLE : DISABLE;
    dmaconf->FlushRxPacket = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_FTF) >> 20) > 0U) ? DISABLE : ENABLE;
    dmaconf->TransmitStoreForward = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_TSF) >> 21) > 0U) ? ENABLE : DISABLE;
    dmaconf->TransmitThresholdControl = READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_TTC);
    dmaconf->ForwardErrorFrames = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_FEF) >> 7) > 0U) ? ENABLE : DISABLE;
    dmaconf->ForwardUndersizedGoodFrames = ((READ_BIT(heth->Instance->DMAOMR,
                                                    ETH_DMAOMR_FUGF) >> 6) > 0U) ? ENABLE : DISABLE;
    dmaconf->DropGiantFrames = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_DGF) >> 5) > 0U) ? ENABLE : DISABLE;
    dmaconf->ReceiveThresholdControl = READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_RTC);
    dmaconf->SecondFrameOperate = ((READ_BIT(heth->Instance->DMAOMR, ETH_DMAOMR_OSF) >> 2) > 0U) ? ENABLE : DISABLE;

    return HAL_OK;
}

/**
  * @brief  Set the MAC configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  macconf: pointer to a ETH_MACConfigTypeDef structure that contains
  *         the configuration of the MAC.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetMACConfig(ETH_HandleTypeDef *heth,  ETH_MACConfigTypeDef *macconf)
{
    if (macconf == NULL)
    {
        return HAL_ERROR;
    }

    if (heth->gState == HAL_ETH_STATE_READY)
    {
        ETH_SetMACConfig(heth, macconf);

        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Set the ETH DMA configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  dmaconf: pointer to a ETH_DMAConfigTypeDef structure that will hold
  *         the configuration of the ETH DMA.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetDMAConfig(ETH_HandleTypeDef *heth,  ETH_DMAConfigTypeDef *dmaconf)
{
    if (dmaconf == NULL)
    {
        return HAL_ERROR;
    }

    if (heth->gState == HAL_ETH_STATE_READY)
    {
        ETH_SetDMAConfig(heth, dmaconf);

        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/**
  * @brief  Configures the Clock range of ETH MDIO interface.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
void HAL_ETH_SetMDIOClockRange(ETH_HandleTypeDef *heth)
{
    uint32_t hclk;
    uint32_t tmpreg;

    /* Get the ETHERNET MACMIIAR value */
    tmpreg = (heth->Instance)->MACMIIAR;
    /* Clear CSR Clock Range CR[2:0] bits */
    tmpreg &= ETH_MACMIIAR_CR_MASK;

    /* Get hclk frequency value */
    hclk = HAL_RCC_GetHCLKFreq();

    /* Set CR bits depending on hclk value */
    if (hclk < 35000000U)
    {
        /* CSR Clock Range between 0-35 MHz */
        tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div16;
    }
    else if (hclk < 60000000U)
    {
        /* CSR Clock Range between 35-60 MHz */
        tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div26;
    }
    else if (hclk < 100000000U)
    {
        /* CSR Clock Range between 60-100 MHz */
        tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div42;
    }
    else if (hclk < 150000000U)
    {
        /* CSR Clock Range between 100-150 MHz */
        tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div62;
    }
    else /* (hclk >= 150000000)  */
    {
        /* CSR Clock >= 150 MHz */
        tmpreg |= (uint32_t)ETH_MACMIIAR_CR_Div102;
    }

    /* Write to ETHERNET MAC MIIAR: Configure the ETHERNET CSR Clock Range */
    (heth->Instance)->MACMIIAR = (uint32_t)tmpreg;
}

/**
  * @brief  Set the ETH MAC (L2) Filters configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pFilterConfig: pointer to a ETH_MACFilterConfigTypeDef structure that contains
  *         the configuration of the ETH MAC filters.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetMACFilterConfig(ETH_HandleTypeDef *heth, const ETH_MACFilterConfigTypeDef *pFilterConfig)
{
    uint32_t filterconfig;

    if (pFilterConfig == NULL)
    {
        return HAL_ERROR;
    }

    filterconfig = ((uint32_t)pFilterConfig->PromiscuousMode |
                  ((uint32_t)pFilterConfig->HashUnicast << 1) |
                  ((uint32_t)pFilterConfig->HashMulticast << 2)  |
                  ((uint32_t)pFilterConfig->DestAddrInverseFiltering << 3) |
                  ((uint32_t)pFilterConfig->PassAllMulticast << 4) |
                  ((uint32_t)((pFilterConfig->BroadcastFilter == DISABLE) ? 1U : 0U) << 5) |
                  ((uint32_t)pFilterConfig->SrcAddrInverseFiltering << 8) |
                  ((uint32_t)pFilterConfig->SrcAddrFiltering << 9) |
                  ((uint32_t)pFilterConfig->HachOrPerfectFilter << 10) |
                  ((uint32_t)pFilterConfig->DropNonTCPUDPPPackets << 21) |
                  ((uint32_t)pFilterConfig->ReceiveAllMode << 31) |
                  pFilterConfig->ControlPacketsFilter);

    MODIFY_REG(heth->Instance->MACFFR, ETH_MACFFR_MASK, filterconfig);

    return HAL_OK;
}

/**
  * @brief  Get the ETH MAC (L2) Filters configuration.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pFilterConfig: pointer to a ETH_MACFilterConfigTypeDef structure that will hold
  *         the configuration of the ETH MAC filters.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_GetMACFilterConfig(const ETH_HandleTypeDef *heth, ETH_MACFilterConfigTypeDef *pFilterConfig)
{
    if (pFilterConfig == NULL)
    {
        return HAL_ERROR;
    }

    pFilterConfig->PromiscuousMode = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_PM)) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->HashUnicast = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_HU) >> 1) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->HashMulticast = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_HM) >> 2) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->DestAddrInverseFiltering = ((READ_BIT(heth->Instance->MACFFR,
                                                       ETH_MACFFR_DAIF) >> 3) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->PassAllMulticast = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_PAM) >> 4) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->BroadcastFilter = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_DBF) >> 5) == 0U) ? ENABLE : DISABLE;
    pFilterConfig->ControlPacketsFilter = READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_PCF);
    pFilterConfig->SrcAddrInverseFiltering = ((READ_BIT(heth->Instance->MACFFR,
                                                      ETH_MACFFR_SAIF) >> 8) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->SrcAddrFiltering = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_SAF) >> 9) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->HachOrPerfectFilter = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_HPF) >> 10) > 0U)
                                       ? ENABLE : DISABLE;
    pFilterConfig->DropNonTCPUDPPPackets = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_DNTU) >> 21) > 0U) ? ENABLE : DISABLE;
    pFilterConfig->ReceiveAllMode = ((READ_BIT(heth->Instance->MACFFR, ETH_MACFFR_RA) >> 31) > 0U) ? ENABLE : DISABLE;

    return HAL_OK;
}

/**
  * @brief  Set the source MAC Address to be matched.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  AddrNbr: The MAC address to configure
  *          This parameter must be a value of the following:
  *            ETH_MAC_ADDRESS1
  *            ETH_MAC_ADDRESS2
  *            ETH_MAC_ADDRESS3
  * @param  pMACAddr: Pointer to MAC address buffer data (6 bytes)
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetSourceMACAddrMatch(const ETH_HandleTypeDef *heth, uint32_t AddrNbr,
                                                const uint8_t *pMACAddr)
{
    uint32_t macaddrlr;
    uint32_t macaddrhr;

    if (pMACAddr == NULL)
    {
        return HAL_ERROR;
    }

    /* Get mac addr high reg offset */
    macaddrhr = ((uint32_t) &(heth->Instance->MACA0HR) + AddrNbr);
    /* Get mac addr low reg offset */
    macaddrlr = ((uint32_t) &(heth->Instance->MACA0LR) + AddrNbr);

    /* Set MAC addr bits 32 to 47 */
    (*(__IO uint32_t *)macaddrhr) = (((uint32_t)(pMACAddr[5]) << 8) | (uint32_t)pMACAddr[4]);
    /* Set MAC addr bits 0 to 31 */
    (*(__IO uint32_t *)macaddrlr) = (((uint32_t)(pMACAddr[3]) << 24) | ((uint32_t)(pMACAddr[2]) << 16) |
                                   ((uint32_t)(pMACAddr[1]) << 8) | (uint32_t)pMACAddr[0]);

    /* Enable address and set source address bit */
    (*(__IO uint32_t *)macaddrhr) |= (ETH_MACAHR_SA | ETH_MACAHR_AE);

    return HAL_OK;
}

/**
  * @brief  Set the ETH Hash Table Value.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pHashTable: pointer to a table of two 32 bit values, that contains
  *         the 64 bits of the hash table.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetHashTable(ETH_HandleTypeDef *heth, uint32_t *pHashTable)
{
    if (pHashTable == NULL)
    {
        return HAL_ERROR;
    }

    heth->Instance->MACHTLR = pHashTable[0];
    heth->Instance->MACHTHR = pHashTable[1];

    return HAL_OK;
}

/**
  * @brief  Get the VLAN Configuration for Receive Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pVlanConfig: pointer to a ETH_RxVLANConfigTypeDef structure
  *         that will contain the VLAN filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_GetRxVLANConfig(const ETH_HandleTypeDef *heth, ETH_RxVLANConfigTypeDef *pVlanConfig)
{
    if (pVlanConfig == NULL)
    {
        return HAL_ERROR;
    }

    pVlanConfig->VLANTagHashTableMatch = ((READ_BIT(heth->Instance->MACVLANTR,
                                           ETH_MACVLANTR_VTHM) >> 19) == 0U) ? DISABLE : ENABLE;
    
    pVlanConfig->SVLANType = ((READ_BIT(heth->Instance->MACVLANTR,
                               ETH_MACVLANTR_ESVL ) >> 18) == 0U) ? DISABLE : ENABLE;
    
    pVlanConfig->VLANTagInverseMatch = ((READ_BIT(heth->Instance->MACVLANTR, 
                                         ETH_MACVLANTR_VTIM) >> 17) == 0U) ? DISABLE : ENABLE;
    
    pVlanConfig->VLANTagComparison12Bit = ((READ_BIT(heth->Instance->MACVLANTR, 
                                            ETH_MACVLANTR_ETV) >> 16) == 0U) ? DISABLE : ENABLE;  
    
    return HAL_OK;
}

/**
  * @brief  Set the VLAN Configuration for Receive Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pVlanConfig: pointer to a ETH_RxVLANConfigTypeDef structure
  *         that contains VLAN filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetRxVLANConfig(ETH_HandleTypeDef *heth, ETH_RxVLANConfigTypeDef *pVlanConfig)
{
    if (pVlanConfig == NULL)
    {
        return HAL_ERROR;
    }

    /* Write config to MACVTR */
    MODIFY_REG(heth->Instance->MACVLANTR, ETH_MACVLANTR_MASK, (((uint32_t)pVlanConfig->VLANTagHashTableMatch << 19) |
                                                              ((uint32_t)pVlanConfig->SVLANType << 18) |
                                                              ((uint32_t)pVlanConfig->VLANTagInverseMatch << 17) |
                                                              ((uint32_t)pVlanConfig->VLANTagComparison12Bit << 16)));

    return HAL_OK;
}

/**
  * @brief  Set the VLAN Identifier for Rx packets
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  VLANIdentifier: VLAN Identifier value
  * @retval None
  */
void HAL_ETH_SetRxVLANIdentifier(ETH_HandleTypeDef *heth, uint32_t VLANIdentifier)
{
    MODIFY_REG(heth->Instance->MACVLANTR, ETH_MACVLANTR_VL, VLANIdentifier);
}

/**
  * @brief  Set the VLAN Hash Table
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  VLANHashTable: VLAN hash table 16 bit value
  * @retval None
  */
void HAL_ETH_SetVLANHashTable(ETH_HandleTypeDef *heth, uint32_t VLANHashTable)
{
    MODIFY_REG(heth->Instance->MACVHTR, ETH_MACVHTR_VLHT, VLANHashTable);
}

/**
  * @brief  Get the VLAN Configuration for Transmit Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pVlanConfig: pointer to a ETH_TxVLANConfigTypeDef structure
  *         that will contain the Tx VLAN filter configuration.
  * @retval HAL Status.
  */
HAL_StatusTypeDef HAL_ETH_GetTxVLANConfig(const ETH_HandleTypeDef *heth, ETH_TxVLANConfigTypeDef *pVlanConfig)
{ 
    if (pVlanConfig == NULL)
    {
        return HAL_ERROR;
    }

    pVlanConfig->SVLANType = ((READ_BIT(heth->Instance->MACVTIRR, ETH_MACVTIRR_CSVL) >> 19) == 0U) ? DISABLE : ENABLE;
    pVlanConfig->VLANTagControl = READ_BIT(heth->Instance->MACVTIRR, (ETH_MACVTIRR_VLP | ETH_MACVTIRR_VLC));

    return HAL_OK;;
}

/**
  * @brief  Set the VLAN Configuration for Transmit Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pVlanConfig: pointer to a ETH_TxVLANConfigTypeDef structure
  *         that contains Tx VLAN filter configuration.
  * @retval HAL Status
  */
HAL_StatusTypeDef HAL_ETH_SetTxVLANConfig(ETH_HandleTypeDef *heth, const ETH_TxVLANConfigTypeDef *pVlanConfig)
{
    if (pVlanConfig == NULL)
    {
        return HAL_ERROR;
    }
    
    MODIFY_REG(heth->Instance->MACVTIRR, ETH_MACVTIRR_MASK, (((uint32_t)pVlanConfig->SVLANType << 19) |
                                                            pVlanConfig->VLANTagControl));

    return HAL_OK;
}

/**
  * @brief  Set the VLAN Tag Identifier for Transmit Packets.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  VLANIdentifier: VLAN Identifier 16 bit value
  * @retval None
  */
void HAL_ETH_SetTxVLANIdentifier(ETH_HandleTypeDef *heth, uint32_t VLANIdentifier)
{
    MODIFY_REG(heth->Instance->MACVTIRR, ETH_MACVTIRR_VLT, VLANIdentifier);
}

/**
  * @brief  Enables the VLAN Tag Filtering process.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None.
  */
void HAL_ETH_EnableVLANProcessing(ETH_HandleTypeDef *heth)
{
    /* Enable VLAN processing */
    SET_BIT(heth->Instance->MACFFR, ETH_MACFFR_VTFE);
}

/**
  * @brief  Disables the VLAN Tag Filtering process.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None.
  */
void HAL_ETH_DisableVLANProcessing(ETH_HandleTypeDef *heth)
{
    /* Disable VLAN processing */
    CLEAR_BIT(heth->Instance->MACFFR, ETH_MACFFR_VTFE);
}


/**
  * @brief  Configures the L4 Filter, this function allow to:
  *         set the layer 4 protocol to be matched (TCP or UDP)
  *         enable/disable L4 source/destination port perfect/inverse match.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  Filter: L4 filter to configured, this parameter must be one of the following
  *           ETH_L4_FILTER_0
  *           ETH_L4_FILTER_1
  * @param  pL4FilterConfig: pointer to a ETH_L4FilterConfigTypeDef structure
  *         that contains L4 filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetL4FilterConfig(ETH_HandleTypeDef *heth, uint32_t Filter,
                                              const ETH_L4FilterConfigTypeDef *pL4FilterConfig)
{
    if (pL4FilterConfig == NULL)
    {
        return HAL_ERROR;
    }

    if (Filter == ETH_L4_FILTER_0)
    {
        /* Write configuration to MACL3L4C0R register */
        MODIFY_REG(heth->Instance->MACL3L4C0R, ETH_MACL4CR_MASK, (pL4FilterConfig->Protocol |
                                                                  pL4FilterConfig->SrcPortFilterMatch |
                                                                  pL4FilterConfig->DestPortFilterMatch));

        /* Write configuration to MACL4A0R register */
        WRITE_REG(heth->Instance->MACL4A0R, (pL4FilterConfig->SourcePort | (pL4FilterConfig->DestinationPort << 16)));

    }
    else /* Filter == ETH_L4_FILTER_1 */
    {
        /* Write configuration to MACL3L4C1R register */
        MODIFY_REG(heth->Instance->MACL3L4C1R, ETH_MACL4CR_MASK, (pL4FilterConfig->Protocol |
                                                                  pL4FilterConfig->SrcPortFilterMatch |
                                                                  pL4FilterConfig->DestPortFilterMatch));

        /* Write configuration to MACL4A1R register */
        WRITE_REG(heth->Instance->MACL4A1R, (pL4FilterConfig->SourcePort | (pL4FilterConfig->DestinationPort << 16)));
    }

    /* Enable L4 filter */
    SET_BIT(heth->Instance->MACFFR, ETH_MACFFR_IPFE);

    return HAL_OK;
}

/**
  * @brief  Configures the L4 Filter, this function allow to:
  *         set the layer 4 protocol to be matched (TCP or UDP)
  *         enable/disable L4 source/destination port perfect/inverse match.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  Filter: L4 filter to configured, this parameter must be one of the following
  *           ETH_L4_FILTER_0
  *           ETH_L4_FILTER_1
  * @param  pL4FilterConfig: pointer to a ETH_L4FilterConfigTypeDef structure
  *         that contains L4 filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_GetL4FilterConfig(const ETH_HandleTypeDef *heth, uint32_t Filter,
                                              ETH_L4FilterConfigTypeDef *pL4FilterConfig)
{
    if (pL4FilterConfig == NULL)
    {
        return HAL_ERROR;
    }

    if (Filter == ETH_L4_FILTER_0)
    {
        /* Get configuration from MACL3L4C0R register */
        pL4FilterConfig->Protocol = READ_BIT(heth->Instance->MACL3L4C0R, ETH_MACL3L4CR_L4PEN);
        pL4FilterConfig->DestPortFilterMatch = READ_BIT(heth->Instance->MACL3L4C0R,
                                                        (ETH_MACL3L4CR_L4DPM | ETH_MACL3L4CR_L4DPIM));
        pL4FilterConfig->SrcPortFilterMatch = READ_BIT(heth->Instance->MACL3L4C0R,
                                                       (ETH_MACL3L4CR_L4SPM | ETH_MACL3L4CR_L4SPIM));

        /* Get configuration from MACL4A0R register */
        pL4FilterConfig->DestinationPort = (READ_BIT(heth->Instance->MACL4A0R, ETH_MAC_L4_DSTP_MASK) >> 16);
        pL4FilterConfig->SourcePort = READ_BIT(heth->Instance->MACL4A0R, ETH_MAC_L4_SRCP_MASK);
    }
    else /* Filter == ETH_L4_FILTER_1 */
    {
        /* Get configuration from MACL3L4C1R register */
        pL4FilterConfig->Protocol = READ_BIT(heth->Instance->MACL3L4C1R, ETH_MACL3L4CR_L4PEN);
        pL4FilterConfig->DestPortFilterMatch = READ_BIT(heth->Instance->MACL3L4C1R,
                                                        (ETH_MACL3L4CR_L4DPM | ETH_MACL3L4CR_L4DPIM));
        pL4FilterConfig->SrcPortFilterMatch = READ_BIT(heth->Instance->MACL3L4C1R,
                                                       (ETH_MACL3L4CR_L4SPM | ETH_MACL3L4CR_L4SPIM));

        /* Get configuration from MACL4A1R register */
        pL4FilterConfig->DestinationPort = (READ_BIT(heth->Instance->MACL4A1R, ETH_MAC_L4_DSTP_MASK) >> 16);
        pL4FilterConfig->SourcePort = READ_BIT(heth->Instance->MACL4A1R, ETH_MAC_L4_SRCP_MASK);
    }

    return HAL_OK;
}

/**
  * @brief  Configures the L3 Filter, this function allow to:
  *         set the layer 3 protocol to be matched (IPv4 or IPv6)
  *         enable/disable L3 source/destination port perfect/inverse match.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  Filter: L3 filter to configured, this parameter must be one of the following
  *           ETH_L3_FILTER_0
  *           ETH_L3_FILTER_1
  * @param  pL3FilterConfig: pointer to a ETH_L3FilterConfigTypeDef structure
  *         that contains L3 filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_SetL3FilterConfig(ETH_HandleTypeDef *heth, uint32_t Filter,
                                              const ETH_L3FilterConfigTypeDef *pL3FilterConfig)
{
    if (pL3FilterConfig == NULL)
    {
        return HAL_ERROR;
    }

    if (Filter == ETH_L3_FILTER_0)
    {
        /* Write configuration to MACL3L4C0R register */
        MODIFY_REG(heth->Instance->MACL3L4C0R, ETH_MACL3CR_MASK, (pL3FilterConfig->Protocol |
                                                                  pL3FilterConfig->SrcAddrFilterMatch |
                                                                  pL3FilterConfig->DestAddrFilterMatch |
                                                                  (pL3FilterConfig->SrcAddrHigherBitsMatch << 6) |
                                                                  (pL3FilterConfig->DestAddrHigherBitsMatch << 11)));
    }
    else  /* Filter == ETH_L3_FILTER_1 */
    {
        /* Write configuration to MACL3L4C1R register */
        MODIFY_REG(heth->Instance->MACL3L4C1R, ETH_MACL3CR_MASK, (pL3FilterConfig->Protocol |
                                                                  pL3FilterConfig->SrcAddrFilterMatch |
                                                                  pL3FilterConfig->DestAddrFilterMatch |
                                                                  (pL3FilterConfig->SrcAddrHigherBitsMatch << 6) |
                                                                  (pL3FilterConfig->DestAddrHigherBitsMatch << 11)));
    }

    if (Filter == ETH_L3_FILTER_0)
    {
        /* Check if IPv6 protocol is selected */
        if (pL3FilterConfig->Protocol != ETH_L3_IPV4_MATCH)
        {
            /* Set the IPv6 address match */
            /* Set Bits[31:0] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A00R, pL3FilterConfig->Ip6Addr[0]);
            /* Set Bits[63:32] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A10R, pL3FilterConfig->Ip6Addr[1]);
            /* update Bits[95:64] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A20R, pL3FilterConfig->Ip6Addr[2]);
            /* update Bits[127:96] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A30R, pL3FilterConfig->Ip6Addr[3]);
        }
        else /* IPv4 protocol is selected */
        {
            /* Set the IPv4 source address match */
            WRITE_REG(heth->Instance->MACL3A00R, pL3FilterConfig->Ip4SrcAddr);
            /* Set the IPv4 destination address match */
            WRITE_REG(heth->Instance->MACL3A10R, pL3FilterConfig->Ip4DestAddr);
        }
    }
    else  /* Filter == ETH_L3_FILTER_1 */
    {
        /* Check if IPv6 protocol is selected */
        if (pL3FilterConfig->Protocol != ETH_L3_IPV4_MATCH)
        {
            /* Set the IPv6 address match */
            /* Set Bits[31:0] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A01R, pL3FilterConfig->Ip6Addr[0]);
            /* Set Bits[63:32] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A11R, pL3FilterConfig->Ip6Addr[1]);
            /* update Bits[95:64] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A11R, pL3FilterConfig->Ip6Addr[2]);
            /* update Bits[127:96] of 128-bit IP addr */
            WRITE_REG(heth->Instance->MACL3A11R, pL3FilterConfig->Ip6Addr[3]);
        }
        else /* IPv4 protocol is selected */
        {
            /* Set the IPv4 source address match */
            WRITE_REG(heth->Instance->MACL3A01R, pL3FilterConfig->Ip4SrcAddr);
            /* Set the IPv4 destination address match */
            WRITE_REG(heth->Instance->MACL3A01R, pL3FilterConfig->Ip4DestAddr);
        }
    }

    /* Enable L3 filter */
    SET_BIT(heth->Instance->MACFFR, ETH_MACFFR_IPFE);

    return HAL_OK;
}

/**
  * @brief  Configures the L3 Filter, this function allow to:
  *         set the layer 3 protocol to be matched (IPv4 or IPv6)
  *         enable/disable L3 source/destination port perfect/inverse match.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  Filter: L3 filter to configured, this parameter must be one of the following
  *           ETH_L3_FILTER_0
  *           ETH_L3_FILTER_1
  * @param  pL3FilterConfig: pointer to a ETH_L3FilterConfigTypeDef structure
  *         that will contain the L3 filter configuration.
  * @retval HAL status
  */
HAL_StatusTypeDef HAL_ETH_GetL3FilterConfig(const ETH_HandleTypeDef *heth, uint32_t Filter,
                                              ETH_L3FilterConfigTypeDef *pL3FilterConfig)
{
    if (pL3FilterConfig == NULL)
    {
        return HAL_ERROR;
    }
    pL3FilterConfig->Protocol = READ_BIT(*((__IO uint32_t *)(&(heth->Instance->MACL3L4C0R) + Filter)),
                                       ETH_MACL3L4CR_L3PEN);
    pL3FilterConfig->SrcAddrFilterMatch = READ_BIT(*((__IO uint32_t *)(&(heth->Instance->MACL3L4C0R) + Filter)),
                                                 (ETH_MACL3L4CR_L3SAM | ETH_MACL3L4CR_L3SAIM));
    pL3FilterConfig->DestAddrFilterMatch = READ_BIT(*((__IO uint32_t *)(&(heth->Instance->MACL3L4C0R) + Filter)),
                                                  (ETH_MACL3L4CR_L3DAM | ETH_MACL3L4CR_L3DAIM));
    pL3FilterConfig->SrcAddrHigherBitsMatch = (READ_BIT(*((__IO uint32_t *)(&(heth->Instance->MACL3L4C0R) + Filter)),
                                                      ETH_MACL3L4CR_L3HSBM) >> 6);
    pL3FilterConfig->DestAddrHigherBitsMatch = (READ_BIT(*((__IO uint32_t *)(&(heth->Instance->MACL3L4C0R) + Filter)),
                                                       ETH_MACL3L4CR_L3HDBM) >> 11);

    if (Filter == ETH_L3_FILTER_0)
    {
        if (pL3FilterConfig->Protocol != ETH_L3_IPV4_MATCH)
        {
            WRITE_REG(pL3FilterConfig->Ip6Addr[0], heth->Instance->MACL3A00R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[1], heth->Instance->MACL3A10R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[2], heth->Instance->MACL3A20R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[3], heth->Instance->MACL3A30R);
        }
        else
        {
            WRITE_REG(pL3FilterConfig->Ip4SrcAddr, heth->Instance->MACL3A00R);
            WRITE_REG(pL3FilterConfig->Ip4DestAddr, heth->Instance->MACL3A10R);
        }
    }
    else /* ETH_L3_FILTER_1 */
    {
        if (pL3FilterConfig->Protocol != ETH_L3_IPV4_MATCH)
        {
            WRITE_REG(pL3FilterConfig->Ip6Addr[0], heth->Instance->MACL3A01R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[1], heth->Instance->MACL3A11R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[2], heth->Instance->MACL3A21R);
            WRITE_REG(pL3FilterConfig->Ip6Addr[3], heth->Instance->MACL3A31R);
        }
        else
        {
            WRITE_REG(pL3FilterConfig->Ip4SrcAddr, heth->Instance->MACL3A01R);
            WRITE_REG(pL3FilterConfig->Ip4DestAddr, heth->Instance->MACL3A11R);
        }
    }

    return HAL_OK;
}

/**
  * @brief  Enables L3 and L4 filtering process.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None.
  */
void HAL_ETH_EnableL3L4Filtering(ETH_HandleTypeDef *heth)
{
    /* Enable L3/L4 filter */
    SET_BIT(heth->Instance->MACFFR, ETH_MACFFR_IPFE);
}

/**
  * @brief  Disables L3 and L4 filtering process.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None.
  */
void HAL_ETH_DisableL3L4Filtering(ETH_HandleTypeDef *heth)
{
    /* Disable L3/L4 filter */
    CLEAR_BIT(heth->Instance->MACFFR, ETH_MACFFR_IPFE);
}

/**
  * @brief  Enters the Power down mode.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pPowerDownConfig: a pointer to ETH_PowerDownConfigTypeDef structure
  *         that contains the Power Down configuration
  * @retval None.
  */
void HAL_ETH_EnterPowerDownMode(ETH_HandleTypeDef *heth, const ETH_PowerDownConfigTypeDef *pPowerDownConfig)
{
    uint32_t powerdownconfig;

    powerdownconfig = (((uint32_t)pPowerDownConfig->MagicPacket << ETH_MACPMTCSR_MPE_Pos) |
                       ((uint32_t)pPowerDownConfig->WakeUpPacket << ETH_MACPMTCSR_WFE_Pos) |
                       ((uint32_t)pPowerDownConfig->GlobalUnicast << ETH_MACPMTCSR_GU_Pos) |
                       ETH_MACPMTCSR_PD);
     
    /* Enable PMT interrupt */
    CLEAR_BIT(heth->Instance->MACIMR, ETH_MACIMR_PIM);

    MODIFY_REG(heth->Instance->MACPMTCSR, ETH_MACPMTCSR_MASK, powerdownconfig);
}

/**
  * @brief  Exits from the Power down mode.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None.
  */
void HAL_ETH_ExitPowerDownMode(ETH_HandleTypeDef *heth)
{
    /* clear wake up sources */
    CLEAR_BIT(heth->Instance->MACPMTCSR, ETH_MACPMTCSR_WFE | ETH_MACPMTCSR_MPE | ETH_MACPMTCSR_GU);

    if (READ_BIT(heth->Instance->MACPMTCSR, ETH_MACPMTCSR_PD) != (uint32_t)RESET)
    {
        /* Exit power down mode */
        CLEAR_BIT(heth->Instance->MACPMTCSR, ETH_MACPMTCSR_PD);
    }

    /* Disable PMT interrupt */
    SET_BIT(heth->Instance->MACIMR, ETH_MACIMR_PIM);
}

/**
  * @brief  Set the WakeUp filter.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pFilter: pointer to filter registers values
  * @param  Count: number of filter registers, must be from 1 to 8.
  * @retval None.
  */
HAL_StatusTypeDef HAL_ETH_SetWakeUpFilter(ETH_HandleTypeDef *heth, uint32_t *pFilter, uint32_t Count)
{
    uint32_t regindex;

    if (pFilter == NULL)
    {
        return HAL_ERROR;
    }

    /* Reset Filter Pointer */
    SET_BIT(heth->Instance->MACPMTCSR, ETH_MACPMTCSR_RWFFPR);

    /* Wake up packet filter config */
    for (regindex = 0; regindex < Count; regindex++)
    {
        /* Write filter regs */
        WRITE_REG(heth->Instance->MACRWUFF, pFilter[regindex]);
    }

    return HAL_OK;
}


/**
  * @brief  Set the Low Power Idle (LPI) timers
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  LsTimer: LPI LS Timer.The default value of the LPI LS Timer is 
            1000 (1 sec) as defined in the IEEE standard.
  * @param  TwTimer: LPI TW Timer.
  * @retval None
  */
void HAL_ETH_SetLPITimer(ETH_HandleTypeDef *heth, uint32_t LsTimer, uint32_t TwTimer)
{
    heth->Instance->MACLPITCR = (LsTimer << ETH_MACLPITCR_LST_Pos) | TwTimer;
}

/**
  * @brief  Enters the Low Power Idle (LPI) mode
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  TxAutomate: Enable/Disable automate enter/exit LPI mode.
  * @param  TxClockStop: Enable/Disable Tx clock stop in LPI mode.
  * @retval None
  */
void HAL_ETH_EnterLPIMode(ETH_HandleTypeDef *heth, FunctionalState TxAutomate, FunctionalState TxClockStop)
{
    /* Enable LPI Interrupts */
    CLEAR_BIT(heth->Instance->MACIMR, ETH_MACIMR_LPIIM);
    
    /* Enable/Disable Tx clock stop in LPI mode */
    SYSCFG->SYSCR = (SYSCFG->SYSCR & ~SYSCFG_SYSCR_ETHMAC_TX_CLKGE) | (TxClockStop << SYSCFG_SYSCR_ETHMAC_TX_CLKGE_Pos);

    /* Write to LPI Control register: Enter low power mode */
    MODIFY_REG(heth->Instance->MACLPICSR, (ETH_MACLPICSR_LPIEN | ETH_MACLPICSR_LPITXA),
              (((uint32_t)TxAutomate << 19) |
              ETH_MACLPICSR_LPIEN));
}

/**
  * @brief  Exits the Low Power Idle (LPI) mode.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
void HAL_ETH_ExitLPIMode(ETH_HandleTypeDef *heth)
{
    /* Enable Tx clock stop in LPI mode */
    SYSCFG->SYSCR = (SYSCFG->SYSCR & ~SYSCFG_SYSCR_ETHMAC_TX_CLKGE);
    
    /* Clear the LPI Config and exit low power mode */
    CLEAR_BIT(heth->Instance->MACLPICSR, (ETH_MACLPICSR_LPIEN | ETH_MACLPICSR_LPITXA));

    /* Disable LPI Interrupts */
    SET_BIT(heth->Instance->MACIMR, ETH_MACIMR_LPIIM);
}

/**
  * @brief  Returns the ETH MAC LPI event
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH MAC WakeUp event
  */
uint32_t HAL_ETH_GetMACLPIEvent(const ETH_HandleTypeDef *heth)
{
    return heth->MACLPIEvent;
}

/******************************************************************************
*@brief : Gets the MMC parameter value. 
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@return: HAL status.
******************************************************************************/
HAL_StatusTypeDef HAL_ETH_GetMMCConfig(ETH_HandleTypeDef *heth, ETH_MMCConfigTypeDef *mmcconf)
{ 
    if (mmcconf == NULL)
    {
        return HAL_ERROR;
    }
    
    /* Get MMC parameters */
    mmcconf->CntStopRollover = ((READ_BIT(heth->Instance->MMCCR, ETH_MMCCR_CNTSTOPRO) >> 1) == 0) ? DISABLE : ENABLE;
    mmcconf->ResetOnRead = ((READ_BIT(heth->Instance->MMCCR, ETH_MMCCR_RSTONRD) >> 2) == 0) ? DISABLE : ENABLE;
    mmcconf->UpdateCntforDBF = ((READ_BIT(heth->Instance->MMCCR, ETH_MMCCR_UCDBC) >> 8) == 0) ? DISABLE : ENABLE;
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Configuration the MMC. 
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@return: HAL status.
******************************************************************************/
HAL_StatusTypeDef HAL_ETH_SetMMCConfig(ETH_HandleTypeDef *heth, ETH_MMCConfigTypeDef *mmcconf)
{
    if (mmcconf == NULL)
    {
        return HAL_ERROR;
    }

    if (heth->gState == HAL_ETH_STATE_READY)
    {      
        MODIFY_REG(heth->Instance->MMCCR, (ETH_MMCCR_CNTSTOPRO | ETH_MMCCR_RSTONRD | ETH_MMCCR_UCDBC),
                  ((uint32_t)mmcconf->CntStopRollover << 1) |
                  ((uint32_t)mmcconf->ResetOnRead << 2) |
                  ((uint32_t)mmcconf->UpdateCntforDBF << 8));

        return HAL_OK;
    }
    else
    {
        return HAL_ERROR;
    }
}

/******************************************************************************
*@brief : Freeze MMC counter.
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@return: none.
******************************************************************************/
void HAL_ETH_FreezeMMCCounter(ETH_HandleTypeDef *heth)
{ 
    SET_BIT(heth->Instance->MMCCR, ETH_MMCCR_CNTFREEZ);
}

/******************************************************************************
*@brief : Unfreeze MMC counter.
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@return: HAL status.
******************************************************************************/
void HAL_ETH_UnfreezeMMCCounter(ETH_HandleTypeDef *heth)
{
    CLEAR_BIT(heth->Instance->MMCCR, ETH_MMCCR_CNTFREEZ);
}

/******************************************************************************
*@brief : Preset MMC counter.
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@param : FullPreset: Enables Full preset.
*@return: none.
******************************************************************************/
void HAL_ETH_PresetMMCCounter(ETH_HandleTypeDef *heth, FunctionalState FullPreset)
{
    MODIFY_REG(heth->Instance->MMCCR, (ETH_MMCCR_CNTPRSTLVL | ETH_MMCCR_CNTPRST),
               ((uint32_t)FullPreset <<  5) |
               ETH_MMCCR_CNTPRST);
}

/******************************************************************************
*@brief : Reset the MMC counter.
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@return: HAL status.
******************************************************************************/
void HAL_ETH_ResetMMCCounter(ETH_HandleTypeDef *heth)
{  
    SET_BIT(heth->Instance->MMCCR, ETH_MMCCR_CNTRST);
}

/** 
  ==============================================================================
                 ##### Peripheral State and Errors functions #####
  ==============================================================================
 [..]
   This subsection provides a set of functions allowing to return the State of
   ETH communication process, return Peripheral Errors occurred during communication
   process
  */

/**
  * @brief  Returns the ETH state.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL state
  */
HAL_ETH_StateTypeDef HAL_ETH_GetState(const ETH_HandleTypeDef *heth)
{
    return heth->gState;
}

/**
  * @brief  Returns the ETH error code
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH Error Code
  */
uint32_t HAL_ETH_GetError(const ETH_HandleTypeDef *heth)
{
    return heth->ErrorCode;
}

/**
  * @brief  Returns the ETH DMA error code
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH DMA Error Code
  */
uint32_t HAL_ETH_GetDMAError(const ETH_HandleTypeDef *heth)
{
    return heth->DMAErrorCode;
}

/**
  * @brief  Returns the ETH MAC error code
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH MAC Error Code
  */
uint32_t HAL_ETH_GetMACError(const ETH_HandleTypeDef *heth)
{
    return heth->MACErrorCode;
}

/**
  * @brief  Returns the ETH MAC WakeUp event source
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH MAC WakeUp event source
  */
uint32_t HAL_ETH_GetMACWakeUpSource(const ETH_HandleTypeDef *heth)
{
    return heth->MACWakeUpEvent;
}

/**
  * @brief  Returns the ETH Tx Buffers in use number
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval ETH Tx Buffers in use number
  */
uint32_t HAL_ETH_GetTxBuffersNumber(const ETH_HandleTypeDef *heth)
{
    return heth->TxDescList.BuffersInUse;
}

///******************************************************************************
//*@brief : Configuration MAC address. 
//*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
//*          the configuration information for ETHERNET module.
//*@return: HAL status.
//******************************************************************************/
//HAL_StatusTypeDef HAL_ETH_MACAddrConfig(ETH_HandleTypeDef *heth)
//{
//    heth->Instance->MACA0HR = ETH_MACA0HR_AE | \
//                               ((uint32_t)heth->Init.MACAddr[5] << 8) | \
//                               ((uint32_t)heth->Init.MACAddr[4] << 0);
//    
//    // MACA1LR
//    heth->Instance->MACA0LR = ((uint32_t)heth->Init.MACAddr[3] << 24) | \
//                               ((uint32_t)heth->Init.MACAddr[2] << 16) | \
//                               ((uint32_t)heth->Init.MACAddr[1] << 8) | \
//                               ((uint32_t)heth->Init.MACAddr[0] << 0);
//    
//    return HAL_OK;
//}

/**
  * @brief  Clears the ETHERNET transmit FIFO.
  * @param  heth pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
static void ETH_FlushTransmitFIFO(ETH_HandleTypeDef *heth)
{
    /* Set the Flush Transmit FIFO bit */
    (heth->Instance)->DMAOMR |= ETH_DMAOMR_FTF;
}

static void ETH_SetMACConfig(ETH_HandleTypeDef *heth, const ETH_MACConfigTypeDef *macconf)
{
    uint32_t macregval;

    /*------------------------ MACCR Configuration --------------------*/
    macregval = (uint32_t)(macconf->SourceAddrControl |
                        ((uint32_t)macconf->Support2KPacket  << 27) |
                        ((uint32_t)macconf->CRCStripTypePacket << 25U) |
                        ((uint32_t)((macconf->Watchdog == DISABLE) ? 1U : 0U) << 23U) |
                        ((uint32_t)((macconf->Jabber == DISABLE) ? 1U : 0U) << 22U) |
                        ((uint32_t)macconf->JumboPacket << 16) |
                        (uint32_t)macconf->InterPacketGapVal |
                        ((uint32_t)macconf->CarrierSenseDuringTransmit << 16U) |
                        macconf->Speed |
                        ((uint32_t)((macconf->ReceiveOwn == DISABLE) ? 1U : 0U) << 13U) |
                        ((uint32_t)macconf->LoopbackMode << 12U) |
                        macconf->DuplexMode |
                        ((uint32_t)macconf->ChecksumOffload << 10U) |
                        ((uint32_t)((macconf->RetryTransmission == DISABLE) ? 1U : 0U) << 9U) |
                        ((uint32_t)macconf->AutomaticPadCRCStrip << 7U) |
                        macconf->BackOffLimit |
                        ((uint32_t)macconf->DeferralCheck << 4U) |
                        macconf->PreambleLength);

    /* Write to ETHERNET MACCR */
    MODIFY_REG(heth->Instance->MACCR, ETH_MACCR_MASK, macregval);

    /*------------------------ MACFCR Configuration --------------------*/
    macregval = (uint32_t)((macconf->PauseTime << 16U) |
                        ((uint32_t)((macconf->UnicastPausePacketDetect == ENABLE) ? 1U : 0U) << 3U) |
                        ((uint32_t)((macconf->ReceiveFlowControl == ENABLE) ? 1U : 0U) << 2U) |
                        ((uint32_t)((macconf->TransmitFlowControl == ENABLE) ? 1U : 0U) << 1U));

    /* Write to ETHERNET MACFCR */
    MODIFY_REG(heth->Instance->MACFCR, ETH_MACFCR_MASK, macregval);

    /*------------------------ MACWTR Configuration --------------------*/
    macregval = (((uint32_t)macconf->ProgrammableWatchdog << 16) |
                          macconf->WatchdogTimeout);

    /* Write to MACWTR */
    MODIFY_REG(heth->Instance->MACWTR, ETH_MACWTR_MASK, macregval);
}

static void ETH_SetDMAConfig(ETH_HandleTypeDef *heth, const ETH_DMAConfigTypeDef *dmaconf)
{
    uint32_t dmaregval;

    /*------------------------ DMAOMR Configuration ---------------------*/
    dmaregval = (uint32_t)(((uint32_t)((dmaconf->DropTCPIPChecksumErrorFrame == DISABLE) ? 1U : 0U) << 26U) |
                        ((uint32_t)dmaconf->ReceiveStoreForward << 25U) |
                        ((uint32_t)((dmaconf->FlushRxPacket == DISABLE) ? 1U : 0U) << 24U) |
                        ((uint32_t)dmaconf->TransmitStoreForward << 21U) |
                        dmaconf->TransmitThresholdControl |
                        ((uint32_t)dmaconf->ForwardErrorFrames << 7U) |
                        ((uint32_t)dmaconf->ForwardUndersizedGoodFrames << 6U) |
                        ((uint32_t)dmaconf->DropGiantFrames << 5U) |
                        dmaconf->ReceiveThresholdControl |
                        ((uint32_t)dmaconf->SecondFrameOperate << 2U));

    /* Write to ETHERNET DMAOMR */
    MODIFY_REG(heth->Instance->DMAOMR, ETH_DMAOMR_MASK, dmaregval);

    /*------------------------ DMABMR Configuration ---------------------*/
    (heth->Instance)->DMABMR = (uint32_t)(((uint32_t)dmaconf->RebuildINCRxBurst << 31U) |
                                        dmaconf->DMAArbitration |
                                        ((uint32_t)dmaconf->AddressAlignedBeats << 25U) |
                                        dmaconf->BurstMode |
                                        dmaconf->RxDMABurstLength |
                                        dmaconf->TxDMABurstLength |
                                        ((uint32_t)dmaconf->EnhancedDescriptorFormat << 7U) |
                                        (dmaconf->DescriptorSkipLength << 2U) |
                                        ETH_DMABMR_USP); /* Enable use of separate PBL for Rx and Tx */

    /* Write to DMABMR */
    MODIFY_REG(heth->Instance->DMABMR, ETH_DMABMR_MASK, dmaregval);
}

/**
  * @brief  Configures Ethernet MAC and DMA with default parameters.
  *         called by HAL_ETH_Init() API.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval HAL status
  */
static void ETH_MACDMAConfig(ETH_HandleTypeDef *heth)
{
    ETH_MACConfigTypeDef macDefaultConf;
    ETH_DMAConfigTypeDef dmaDefaultConf;

    /*--------------- ETHERNET MAC registers default Configuration --------------*/
    macDefaultConf.SourceAddrControl = ETH_SOURCEADDRESS_DISABLE;
    macDefaultConf.Support2KPacket = DISABLE;
    macDefaultConf.CRCStripTypePacket = ENABLE;
    macDefaultConf.Watchdog = ENABLE;
    macDefaultConf.Jabber = ENABLE;
    macDefaultConf.JumboPacket = DISABLE;
    macDefaultConf.InterPacketGapVal = ETH_INTERFRAMEGAP_96BIT;
    macDefaultConf.CarrierSenseDuringTransmit = DISABLE;
    macDefaultConf.Speed = ETH_SPEED_100M;
    macDefaultConf.ReceiveOwn = ENABLE;
    macDefaultConf.LoopbackMode = DISABLE;
    macDefaultConf.DuplexMode = ETH_FULLDUPLEX_MODE;
    macDefaultConf.ChecksumOffload = DISABLE;//ENABLE;
    macDefaultConf.RetryTransmission = DISABLE;
    macDefaultConf.AutomaticPadCRCStrip = ENABLE;
    macDefaultConf.BackOffLimit = ETH_BACKOFFLIMIT_10;
    macDefaultConf.DeferralCheck = DISABLE;
    macDefaultConf.PreambleLength = ETH_PREAMBLELENGTH_7;
    macDefaultConf.PauseTime = 0x0U;
    macDefaultConf.UnicastPausePacketDetect = DISABLE;
    macDefaultConf.ReceiveFlowControl = DISABLE;
    macDefaultConf.TransmitFlowControl = DISABLE;
    macDefaultConf.ProgrammableWatchdog = DISABLE;
    macDefaultConf.WatchdogTimeout = 2000;

    /* MAC default configuration */
    ETH_SetMACConfig(heth, &macDefaultConf);

    /*--------------- ETHERNET DMA registers default Configuration --------------*/
    dmaDefaultConf.RebuildINCRxBurst = DISABLE;
    dmaDefaultConf.DMAArbitration = ETH_DMAARBITRATION_RX1_TX1;
    dmaDefaultConf.AddressAlignedBeats = ENABLE;
    dmaDefaultConf.BurstMode = ETH_BURSTLENGTH_FIXED;
    dmaDefaultConf.TxDMABurstLength = ETH_TXDMABURSTLENGTH_32BEAT;
    dmaDefaultConf.RxDMABurstLength = ETH_RXDMABURSTLENGTH_32BEAT;
    dmaDefaultConf.EnhancedDescriptorFormat = ENABLE;
    dmaDefaultConf.DescriptorSkipLength = 0x0U;   
    dmaDefaultConf.DropTCPIPChecksumErrorFrame = ENABLE;
    dmaDefaultConf.ReceiveStoreForward = ENABLE;
    dmaDefaultConf.FlushRxPacket = ENABLE;
    dmaDefaultConf.TransmitStoreForward = ENABLE;
    dmaDefaultConf.TransmitThresholdControl = ETH_TRANSMITTHRESHOLDCONTROL_64BYTES;
    dmaDefaultConf.ForwardErrorFrames = DISABLE;
    dmaDefaultConf.ForwardUndersizedGoodFrames = DISABLE;
    dmaDefaultConf.DropGiantFrames = DISABLE;
    dmaDefaultConf.ReceiveThresholdControl = ETH_RECEIVEDTHRESHOLDCONTROL_64BYTES;
    dmaDefaultConf.SecondFrameOperate = ENABLE;
    
    /* DMA default configuration */
    ETH_SetDMAConfig(heth, &dmaDefaultConf);
}

/**
  * @brief  Initializes the DMA Tx descriptors.
  *         called by HAL_ETH_Init() API.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
static void ETH_DMATxDescListInit(ETH_HandleTypeDef *heth)
{
    ETH_DMADescTypeDef *dmatxdesc;
    uint32_t i;
  
    /* Fill each DMATxDesc descriptor with the right values */
    for (i = 0; i < (uint32_t)ETH_TX_DESC_CNT; i++)
    {
        dmatxdesc = heth->Init.TxDesc + i;

        WRITE_REG(dmatxdesc->DESC0, 0x0U);
        WRITE_REG(dmatxdesc->DESC1, 0x0U);
        WRITE_REG(dmatxdesc->DESC2, 0x0U);
        WRITE_REG(dmatxdesc->DESC3, 0x0U);

        WRITE_REG(heth->TxDescList.TxDesc[i], (uint32_t)dmatxdesc);

        /* Set Second Address Chained bit */
        SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_TCH);

        if (i < ((uint32_t)ETH_TX_DESC_CNT - 1U))
        {
            WRITE_REG(dmatxdesc->DESC3, (uint32_t)(heth->Init.TxDesc + i + 1U));
        }
        else
        {
            WRITE_REG(dmatxdesc->DESC3, (uint32_t)(heth->Init.TxDesc));
        }
        
        WRITE_REG(heth->TxDescList.PacketAddress[i], NULL);
    }

    WRITE_REG(heth->TxDescList.CurTxDesc, 0);
    WRITE_REG(heth->TxDescList.BuffersInUse, 0);
    WRITE_REG(heth->TxDescList.releaseIndex, 0);

    /* Set Transmit Descriptor List Address */
    WRITE_REG(heth->Instance->DMATDLAR, (uint32_t) heth->Init.TxDesc);
}

/**
  * @brief  Initializes the DMA Rx descriptors in chain mode.
  *         called by HAL_ETH_Init() API.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @retval None
  */
static void ETH_DMARxDescListInit(ETH_HandleTypeDef *heth)
{
    ETH_DMADescTypeDef *dmarxdesc;
    uint32_t i;

    for (i = 0; i < (uint32_t)ETH_RX_DESC_CNT; i++)
    {
        dmarxdesc =  heth->Init.RxDesc + i;

        WRITE_REG(dmarxdesc->DESC0, 0x0U);
        WRITE_REG(dmarxdesc->DESC1, 0x0U);
        WRITE_REG(dmarxdesc->DESC2, 0x0U);
        WRITE_REG(dmarxdesc->DESC3, 0x0U);
        WRITE_REG(dmarxdesc->BackupAddr0, 0x0U);
        WRITE_REG(dmarxdesc->BackupAddr1, 0x0U);

        /* Set Own bit of the Rx descriptor Status */
        dmarxdesc->DESC0 = ETH_DMARXDESC_OWN;

        /* Set Buffer1 size and Second Address Chained bit */
        dmarxdesc->DESC1 = heth->Init.RxBuffLen | ETH_DMARXDESC_RCH;
        
        /* Set Rx descritors addresses */
        WRITE_REG(heth->RxDescList.RxDesc[i], (uint32_t)dmarxdesc);

        if (i < ((uint32_t)ETH_RX_DESC_CNT - 1U))
        {
            WRITE_REG(dmarxdesc->DESC3, (uint32_t)(heth->Init.RxDesc + i + 1U));
        }
        else
        {
            WRITE_REG(dmarxdesc->DESC3, (uint32_t)(heth->Init.RxDesc));
        }
    }

    WRITE_REG(heth->RxDescList.RxDescIdx, 0U);
    WRITE_REG(heth->RxDescList.RxDescCnt, 0U);
    WRITE_REG(heth->RxDescList.RxBuildDescIdx, 0U);
    WRITE_REG(heth->RxDescList.RxBuildDescCnt, 0U);
    WRITE_REG(heth->RxDescList.ItMode, 0U);

    /* Set Receive Descriptor List Address */
    WRITE_REG(heth->Instance->DMARDLAR, (uint32_t) heth->Init.RxDesc);
}

/**
  * @brief  Prepare Tx DMA descriptor before transmission.
  *         called by HAL_ETH_Transmit_IT and HAL_ETH_Transmit_IT() API.
  * @param  heth: pointer to a ETH_HandleTypeDef structure that contains
  *         the configuration information for ETHERNET module
  * @param  pTxConfig: Tx packet configuration
  * @param  ItMode: Enable or disable Tx EOT interrept
  * @retval Status
  */
static uint32_t ETH_Prepare_Tx_Descriptors(ETH_HandleTypeDef *heth, const ETH_TxPacketConfigTypeDef *pTxConfig,
                                           uint32_t ItMode)
{
    ETH_TxDescListTypeDef *dmatxdesclist = &heth->TxDescList;
    uint32_t descidx = dmatxdesclist->CurTxDesc;
    uint32_t firstdescidx = dmatxdesclist->CurTxDesc;
    uint32_t idx;
    uint32_t descnbr = 0;
    ETH_DMADescTypeDef *dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[descidx];

    ETH_BufferTypeDef *txbuffer = pTxConfig->TxBuffer;
    uint32_t bd_count = 0;
    uint32_t primask_bit;

    /* Current Tx Descriptor Owned by DMA: cannot be used by the application  */
    if ((READ_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_OWN) == ETH_DMATXDESC_OWN)
         || (dmatxdesclist->PacketAddress[descidx] != NULL))
    {
        return HAL_ETH_ERROR_BUSY;
    }

    descnbr += 1U;

        /* Set header or buffer 1 address */
        WRITE_REG(dmatxdesc->DESC2, (uint32_t)txbuffer->buffer);

    /* Set header or buffer 1 Length */
    MODIFY_REG(dmatxdesc->DESC1, ETH_DMATXDESC_TBS1, txbuffer->len);

    if (READ_BIT(pTxConfig->Attributes, ETH_TX_PACKETS_FEATURES_CSUM) != 0U)
    {
        MODIFY_REG(dmatxdesc->DESC0, ETH_DMATXDESC_CIC, pTxConfig->ChecksumCtrl);
    }

    if (READ_BIT(pTxConfig->Attributes, ETH_TX_PACKETS_FEATURES_CRCPAD) != 0U)
    {
        MODIFY_REG(dmatxdesc->DESC0, ETH_CRC_PAD_CTRL, pTxConfig->CRCPadCtrl);
    }
    
    if (READ_BIT(pTxConfig->Attributes, ETH_TX_PACKETS_FEATURES_VLANTAG) != 0U)
    {   
        /* Set Vlan Type */
        SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_VF);
        
        /* Set Vlan Tag control */
        MODIFY_REG(dmatxdesc->DESC0, ETH_DMATXDESC_VIC, pTxConfig->VlanCtrl);
    }

    /* Mark it as First Descriptor */
    SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_FS);

    /* only if the packet is split into more than one descriptors > 1 */
    while (txbuffer->next != NULL)
    {
        /* Clear the LD bit of previous descriptor */
        CLEAR_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_LS);
        if (ItMode != ((uint32_t)RESET))
        {
            /* Set Interrupt on completion bit */
            SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_IC);
        }
        else
        {
            /* Clear Interrupt on completion bit */
            CLEAR_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_IC);
        }
        /* Increment current tx descriptor index */
        INCR_TX_DESC_INDEX(descidx, 1U);
        /* Get current descriptor address */
        dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[descidx];

        /* Current Tx Descriptor Owned by DMA: cannot be used by the application  */
        if ((READ_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_OWN) == ETH_DMATXDESC_OWN)
            || (dmatxdesclist->PacketAddress[descidx] != NULL))
        {
            descidx = firstdescidx;
            dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[descidx];

            /* clear previous desc own bit */
            for (idx = 0; idx < descnbr; idx ++)
            {
                /* Ensure rest of descriptor is written to RAM before the OWN bit */
                __DMB();

                CLEAR_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_OWN);

                /* Increment current tx descriptor index */
                INCR_TX_DESC_INDEX(descidx, 1U);
                /* Get current descriptor address */
                dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[descidx];
            }

            return HAL_ETH_ERROR_BUSY;
        }

        /* Clear the FS bit of new Descriptor */
        CLEAR_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_FS);

        descnbr += 1U;

        /* Get the next Tx buffer in the list */
        txbuffer = txbuffer->next;

        /* Set header or buffer 1 address */
        WRITE_REG(dmatxdesc->DESC2, (uint32_t)txbuffer->buffer);

        /* Set header or buffer 1 Length */
        MODIFY_REG(dmatxdesc->DESC1, ETH_DMATXDESC_TBS1, txbuffer->len);

        bd_count += 1U;

        /* Ensure rest of descriptor is written to RAM before the OWN bit */
        __DMB();
        /* Set Own bit */
        SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_OWN);
    }

    if (ItMode != ((uint32_t)RESET))
    {
        /* Set Interrupt on completion bit */
        SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_IC);
    }
    else
    {
        /* Clear Interrupt on completion bit */
        CLEAR_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_IC);
    }

    /* Mark it as LAST descriptor */
    SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_LS);

    /* Get address of first descriptor */
    dmatxdesc = (ETH_DMADescTypeDef *)dmatxdesclist->TxDesc[firstdescidx];
    /* Ensure rest of descriptor is written to RAM before the OWN bit */
    __DMB();
    /* set OWN bit of FIRST descriptor */
    SET_BIT(dmatxdesc->DESC0, ETH_DMATXDESC_OWN);
    /* Save the current packet address to expose it to the application */
    dmatxdesclist->PacketAddress[descidx] = dmatxdesclist->CurrentPacketAddress;

    dmatxdesclist->CurTxDesc = descidx;
    
    /* Enter critical section */
    primask_bit = __get_PRIMASK();
    __set_PRIMASK(1);

    dmatxdesclist->BuffersInUse += bd_count + 1U;
    /* Exit critical section: restore previous priority mask */
    __set_PRIMASK(primask_bit);

    /* Return function status */
    return HAL_ETH_ERROR_NONE;
}

/******************************************************************************
*@brief : Configuration the Ethernet Rx clock delay. 
*@param : heth: pointer to a ETH_HandleTypeDef structure that contains
*          the configuration information for ETHERNET module.
*@param : uint: Specifies the number of delay steps per unit delay.
*@param : len: Specifies the phase of the output clock.
*@return: HAL status.
******************************************************************************/
static void ETH_DelayBlockConfig(uint32_t uint, uint32_t len)
{ 
    SYSCFG->SYSCR |= SYSCFG_SYSCR_ETHMAC_RX_DLYSEL;
    ETH_DLYB->CR = DLYB_CR_DEN;
    ETH_DLYB->CR |= DLYB_CR_SEN;
    ETH_DLYB->CFGR = ((uint << DLYB_CFGR_UNIT_Pos) & DLYB_CFGR_UNIT_Msk) | ((len << DLYB_CFGR_SEL_Pos) & DLYB_CFGR_SEL_Msk);
    HAL_SimpleDelay(10);
    ETH_DLYB->CR &= ~DLYB_CR_SEN;
}

#endif /* HAL_ETH_MODULE_ENABLED */
