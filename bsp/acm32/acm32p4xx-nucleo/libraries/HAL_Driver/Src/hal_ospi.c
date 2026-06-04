/******************************************************************************
*@file  : hal_ospi.c
*@brief : This file provides firmware functions to manage the OSPI HAL module
*@ver   : 1.0.0
*@date  : 2023.06.16
******************************************************************************/
#include  "hal.h" 

#ifdef HAL_OSPI_MODULE_ENABLED

/******************************************************************************
*@brief : OSPI interrupt handler       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: None
******************************************************************************/
__weak void HAL_OSPI_IRQHandler(OSPI_HandleTypeDef *hospi) 
{  
    if ( (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_NOT_EMPTY) && \
        ((hospi->Instance->IE) & OSPI_IE_RX_FIFO_NOT_EMPTY_EN) )
    {
        do{
            if (hospi->Rx_Count < hospi->Rx_Size) 
            {
                hospi->Rx_Buffer[hospi->Rx_Count++] = hospi->Instance->DAT;
            }
            else break;
        }while (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_NOT_EMPTY);
    }

    if ( (hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_HALF_EMPTY) && \
        ((hospi->Instance->IE) & OSPI_IE_TX_FIFO_HALF_EMPTY_EN) )
    {
        while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL))
        {
            if(hospi->Tx_Count < hospi->Tx_Size) 
            {
                hospi->Instance->DAT = hospi->Tx_Buffer[hospi->Tx_Count++];
            }
            else 
            {
                /* Disable TxFIFO half empty interrupt */
                CLEAR_BIT(hospi->Instance->IE, OSPI_IE_TX_FIFO_HALF_EMPTY_EN);
                break;
            }
        }               
    }
    
    if ((hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE) && \
        ((hospi->Instance->IE) & OSPI_IE_TX_BATCH_DONE_EN) )     
    {
        /* Clear Batch Done Flag  */
        WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
        /* Disable TX Batch Done Interrupt, Tx FIFO half empty Interrupt */
        CLEAR_BIT(hospi->Instance->IE, OSPI_IE_TX_BATCH_DONE_EN | OSPI_IE_TX_FIFO_HALF_EMPTY_EN);        

        /* Transmit End */
        HAL_OSPI_CS_Release(hospi);
        
        /* Tx Disable */
        CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_EN);
        CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_DMA_REQ_EN);

        hospi->TxState = OSPI_TX_STATE_IDLE;
    }

    if ( (hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE) && \
        ((hospi->Instance->IE) & OSPI_STATUS_RX_BATCH_DONE) )         
    {
        /* Clear Batch Done Flag  */
        WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);

        /* Disable RX Batch Done Interrupt, RXFIFO not Empty interrupt */
        CLEAR_BIT(hospi->Instance->IE, OSPI_IE_RX_BATCH_DONE_EN | OSPI_IE_RX_FIFO_NOT_EMPTY_EN);

        while (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_NOT_EMPTY)
        {
            if (hospi->Rx_Count < hospi->Rx_Size) 
            {
                hospi->Rx_Buffer[hospi->Rx_Count++] = hospi->Instance->DAT;
            }
            else break;
        }
        
        /* Receive End */
        HAL_OSPI_CS_Release(hospi);
        
        /* Rx Disable */
        CLEAR_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_DMA_REQ_EN);
        CLEAR_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_EN);
        
        hospi->RxState = OSPI_RX_STATE_IDLE;
    }
}

/******************************************************************************
*@brief : Init low level of OSPI module: GPIO, CLK, NVIC        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: None
******************************************************************************/
__weak void HAL_OSPI_MspInit(OSPI_HandleTypeDef *hospi)
{
    /* 
      NOTE : This function is implemented in user xxx_hal_msp.c
    */
    
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hospi);
}

/******************************************************************************
*@brief : OSPI De-Initialize the OSPI clock, GPIO, IRQ.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: None
******************************************************************************/
__weak void HAL_OSPI_MspDeInit(OSPI_HandleTypeDef *hospi)
{
    /* 
      NOTE : This function is implemented in user xxx_hal_msp.c
    */
    
    /* Prevent unused argument(s) compilation warning */
    UNUSED(hospi);
}

/******************************************************************************
*@brief : Initialize the OSPI module with parameters       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Init(OSPI_HandleTypeDef *hospi)
{
    /* Check OSPI Parameter */
    assert_param(IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_WORK_MODE(hospi->Init.WorkMode));
    assert_param(IS_OSPI_X_MODE(hospi->Init.XMode));
    assert_param(IS_OSPI_FIRST_BIT(hospi->Init.FirstBit));
    assert_param(IS_OSPI_BAUDRATE_PRESCALER(hospi->Init.BaudRatePrescaler));
    assert_param(IS_OSPI_SAMPLE_SHIFT(hospi->Init.SampleShifting));
    assert_param(IS_OSPI_FIFO_MODE(hospi->Init.FWMode));
    assert_param(IS_OSPI_FIFO_MODE(hospi->Init.FRMode));
    assert_param(IS_OSPI_CS_SEL(hospi->CSx));

    /* Init the low level hardware : GPIO, CLOCK, NVIC */
    HAL_OSPI_MspInit(hospi);
    
    /* Clear Mst_Mode、CHPA、CPOL、LSB_first、X_Mode、IO_MODE、FW_MODE bit */
    hospi->Instance->CTL &= (~(OSPI_CTL_MST_MODE | SPI_CTL_CPHA | SPI_CTL_CPOL | SPI_CTL_LSB_FIRST | \
                            SPI_CTL_X_MODE | OSPI_CTL_IO_MODE | OSPI_CTL_FW_MODE | OSPI_CTL_MEM_MODE));
    
    /* Set OSPI  Master Mode、WorkMode、FirstBit、XMode、Automatic change direction、FIFO wirte mode*/
    hospi->Instance->CTL |= (OSPI_CTL_MST_MODE                              | \
                            (hospi->Init.WorkMode << SPI_CTL_CPHA_Pos)      | \
                            (hospi->Init.FirstBit << SPI_CTL_LSB_FIRST_Pos) | \
                            (hospi->Init.XMode << SPI_CTL_X_MODE_Pos)       | \
                            OSPI_CTL_IO_MODE                                | \
                            (hospi->Init.FWMode << OSPI_CTL_FW_MODE_Pos)    | \
                            (hospi->Init.FRMode << OSPI_CTL_FR_MODE_Pos));
    
    /* Set OSPI BaudRate Prescaler */
    hospi->Instance->BAUD = ((hospi->Instance->BAUD) & (~(SPI_BAUD_DIV1 | SPI_BAUD_DIV2))) | \
                            (hospi->Init.BaudRatePrescaler);
    
    /* Clear SSHIFT bit */
    hospi->Instance->RX_CTL &= ~(OSPI_RX_CTL_SSHIFT);
    /* Set Master Sample shift*/
    hospi->Instance->RX_CTL |= ((hospi->Init.SampleShifting) << OSPI_RX_CTL_SSHIFT_Pos);
    
    /* CLEAR SPI_CS BIT*/
    hospi->Instance->CS = 0;

    if(hospi->CSx != OSPI_CS_0)
    {
        hospi->Instance->CS |= OSPI_CS_CSMAP_EN;    
    }
    /* Disable All Interrupt */
    hospi->Instance->IE = 0x00000000;
    

    return HAL_OK;
}

/******************************************************************************
*@brief : e-Initialize the OSPI peripheral       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_DeInit(OSPI_HandleTypeDef *hospi)
{
    /* Check the OSPI handle allocation */
    if (hospi == NULL)
    {
        return HAL_ERROR;
    }

    /* Check OSPI Instance parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));

    hospi->RxState = OSPI_RX_STATE_IDLE;
    hospi->TxState = OSPI_TX_STATE_IDLE;

    /* DeInit the low level hardware: GPIO, CLOCK, NVIC... */
    HAL_OSPI_MspDeInit(hospi);

    hospi->Rx_Size = 0;
    hospi->Tx_Size = 0;
    hospi->Rx_Count = 0;
    hospi->Tx_Count = 0;

    return HAL_OK;
}

/******************************************************************************
*@brief : Initialize the OSPI module Octal communication 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.     
*@param : Octal: a pointer of OSPI_OctalInitTypeDef structure which contains 
*         the configuration information for the specified OSPI Octal communication.
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_OctalInit(OSPI_HandleTypeDef *hospi, OSPI_OctalInitTypeDef *Octal)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_DTRM(Octal->DTRMode));
    assert_param(IS_OSPI_DQSOE(Octal->DQSMode));
    assert_param(IS_OSPI_MEMORY_TYPE(Octal->MemoryType));
    assert_param(IS_OSPI_DQS_SAMPLE(Octal->DQSSample));
    
    
    /* Clear DTRM、Mem_Mode bit */
    hospi->Instance->CTL &= (~(OSPI_CTL_DTRM | OSPI_CTL_MEM_MODE | OSPI_CTL_DQSOE));
    
    /* Set DTRM、DM_EN、DMCTRL、Mem_Mode、APMD_CLK */
    hospi->Instance->CTL |= ((Octal->DTRMode << OSPI_CTL_DTRM_Pos)          | \
                            (Octal->MemoryType << OSPI_CTL_MEM_MODE_Pos)    | \
                            (Octal->DQSMode << OSPI_CTL_DQSOE_Pos));
    
    /* Set DQS_SAMP_EN */
    hospi->Instance->RX_CTL = ((hospi->Instance->RX_CTL & (~OSPI_RX_CTL_DQS_SAMP_EN)) | \
                                (Octal->DQSSample << OSPI_RX_CTL_DQS_SAMP_EN_Pos));
    
    if(Octal->DTRMode == OSPI_DTRM_DTR)
    {
        /* Check OSPI Parameter */
        assert_param (IS_OSPI_TX_OUT_DELAY(Octal->OutDelay));
        /* set DTR TX out delay */
        hospi->Instance->TX_CTL = ((hospi->Instance->TX_CTL) & (~(OSPI_TX_CTL_OUTDLY))) | \
                                    (Octal->OutDelay << OSPI_TX_CTL_OUTDLY_Pos);
    }
                    
    return HAL_OK;
}


/******************************************************************************
*@brief : OSPI module memory mode initialization
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.     
*@param : Memory: a pointer of OSPI_MemoryInitTypeDef structure which contains 
*         the configuration information for the specified OSPI memory mode.
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_MemoryInit(OSPI_HandleTypeDef *hospi, OSPI_MemoryInitTypeDef *Memory)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    assert_param(IS_OSPI_BURST_TYPE(Memory->HyperBurstType));
    assert_param(IS_OSPI_DATA_MODE(Memory->DataMode));
    assert_param(IS_OSPI_ALTER_BYTE_MODE(Memory->AlterByteMode));
    assert_param(IS_OSPI_ADDR_MODE(Memory->AddrMode));
    assert_param(IS_OSPI_INSRT_MODE(Memory->InstrMode));
    assert_param(IS_OSPI_ADDR_WIDTH(Memory->AddrWidth));
    assert_param(IS_OSPI_DUMMY_CYCLE(Memory->ReadDummyCycleSize));
    assert_param(IS_OSPI_READ_DUMMY_STATE(Memory->ReadDummyByteEnable));
    assert_param(IS_OSPI_WRITE_DUMMY_STATE(Memory->WriteDummyByteEnable));
    assert_param(IS_OSPI_ALTER_BYTE_SIZE(Memory->AlterByteSize));
    assert_param(IS_OSPI_READ_ALTER_STATE(Memory->ReadAlterByteEnable));
    assert_param(IS_OSPI_WRITE_ALTER_STATE(Memory->WriteAlterByteEnable));
    assert_param(IS_OSPI_CMD_SEND_STATE(Memory->SendInstrOnce));
    assert_param(IS_OSPI_CONTINUOUS_STATE(Memory->ContinuousModeEnable));
    assert_param(IS_OSPI_CS_TIMEOUT_STATE(Memory->CsTimeoutEnable));
    
    assert_param(IS_OSPI_CMD(Memory->WriteCmd));
    assert_param(IS_OSPI_CMD(Memory->ReadCmd));
    
    assert_param(IS_OSPI_CS_DELAY_VAL(Memory->CsDelayVal));
    assert_param(IS_OSPI_CS_TIMEOUT_VAL(Memory->CsTimeoutVal));
    
    assert_param(IS_OSPI_DUMMY_CYCLE(Memory->WriteDummyCycleSize));
    assert_param(IS_OSPI_DUMMY_CYCLE(Memory->ReadDummyoffSize));
    assert_param(IS_OSPI_DUMMY_OFF_STATE(Memory->DummyoffEnable));
    assert_param(IS_OSPI_BURST_LEN(Memory->WriteBurstLen));
    assert_param(IS_OSPI_WRAP_SIZE(Memory->WrapSize));
    assert_param(IS_OSPI_BURST_LEN(Memory->ReadBurstLen));
    assert_param(IS_OSPI_LATENCY_CYCLE(Memory->HyperXspiLC1));
    assert_param(IS_OSPI_LATENCY_CYCLE(Memory->HyperXspiLc0));
    
    assert_param(IS_OSPI_DUMMY_CYCLE(Memory->WriteDummyoffSize));
    assert_param(IS_OSPI_CMD_LEN(Memory->CmdLength));
    assert_param(IS_OSPI_CMD_EDGE(Memory->CmdEdge));
    
    /* Set Wr_Cmd/Rd_Cmd bits of CMD register */
    hospi->Instance->CMD = (Memory->WriteCmd << OSPI_CMD_WR_CMD_Pos) | \
                            Memory->ReadCmd;
    
    /* Set Alter_Byte bit of ALTER_BYTE register */
    hospi->Instance->ALTER_BYTE = Memory->AlterByte;

    /* Set CS_DELAY_VAL/CS_TOUT_VAL bits of CS_TIMEOUT_VAL register */
    hospi->Instance->CS_TIMEOUT_VAL = (Memory->CsDelayVal << OSPI_CS_DELAY_VAL_Pos) | \
                                        Memory->CsTimeoutVal;
    
    
    /* Disable Memory mode and clear 0*/                                
    hospi->Instance->MEMO_ACC1 = 0;  
    
    /* Set Hyper_BT/Data_Mode/Alter_Byte_Mode/Addr_Mode/Instr_Mode/Addr_width/Dummy_Cycle_Size 
       Rd_Db_EN/Wr_Db_EN/Alter_Byte_Size/Rd_Ab_EN/Wr_Ab_EN/disable_CMD/Con_Mode_EN/CS_Tout_EN  
       bits of MEMO_ACC1 register */   
    hospi->Instance->MEMO_ACC1 = ((Memory->HyperBurstType << OSPI_MEMO_ACC1_HYPER_BT_Pos)               | \
                                (Memory->DataMode << OSPI_MEMO_ACC1_DATA_MODE_Pos)                      | \
                                (Memory->AlterByteMode << OSPI_MEMO_ACC1_ALTER_BYTE_MODE_Pos)           | \
                                (Memory->AddrMode << OSPI_MEMO_ACC1_ADDR_MODE_Pos)                      | \
                                (Memory->InstrMode << OSPI_MEMO_ACC1_INSTR_MODE_Pos)                    | \
                                (Memory->AddrWidth << OSPI_MEMO_ACC1_ADDR_WIDTH_Pos)                    | \
                                (Memory->ReadDummyCycleSize << OSPI_MEMO_ACC1_DUMMY_CYCLE_SIZE_Pos)     | \
                                (Memory->ReadDummyByteEnable << OSPI_MEMO_ACC1_RD_DB_EN_Pos)            | \
                                (Memory->WriteDummyByteEnable << OSPI_MEMO_ACC1_WR_DB_EN_Pos)            | \
                                (Memory->AlterByteSize << OSPI_MEMO_ACC1_ALTER_BYTE_SIZE_Pos)           | \
                                (Memory->ReadAlterByteEnable << OSPI_MEMO_ACC1_RD_AB_EN_Pos)            | \
                                (Memory->WriteAlterByteEnable << OSPI_MEMO_ACC1_WR_AB_EN_Pos)           | \
                                (Memory->SendInstrOnce << OSPI_MEMO_ACC1_SEND_INSTR_ONCE_EN_Pos)        | \
                                (Memory->ContinuousModeEnable << OSPI_MEMO_ACC1_CON_MODE_EN_Pos)        | \
                                (Memory->CsTimeoutEnable << OSPI_MEMO_ACC1_CS_TIMEOUT_EN_Pos));
                             
    
    /* Set MEMO_ACC2 register */
    hospi->Instance->MEMO_ACC2 = 0;  
    hospi->Instance->MEMO_ACC2 = ((Memory->WriteDummyCycleSize  << OSPI_MEMO_ACC2_WRITE_DUMMY_SIZE_Pos) | \
                                (Memory->ReadDummyoffSize << OSPI_MEMO_ACC2_READ_DUMMY_OFF_SIZE_Pos)    | \
                                (Memory->WriteBurstLen << OSPI_MEMO_ACC2_WBL_Pos)                       | \
                                (Memory->WrapSize << OSPI_MEMO_ACC2_WRPS_Pos)                           | \
                                (Memory->ReadBurstLen << OSPI_MEMO_ACC2_RBL_Pos)                        | \
                                (Memory->HyperXspiLC1 << OSPI_MEMO_ACC2_HXLC1_Pos)                      | \
                                (Memory->HyperXspiLc0));

    /* Set MEMO_ACC3 register */
    hospi->Instance->MEMO_ACC3 = 0;
    hospi->Instance->MEMO_ACC3 = ((Memory->WriteDummyoffSize << OSPI_MEMO_ACC3_WRITE_DUMMY_OFF_SIZE_Pos)    | \
                                (Memory->CmdLength << OSPI_MEMO_ACC3_CMD_BYTE_Pos)                          | \
                                (Memory->CmdEdge));
    
    return HAL_OK;
}

/******************************************************************************
*@brief : CS Select       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: None
******************************************************************************/
void HAL_OSPI_CS_Select(OSPI_HandleTypeDef *hospi)
{
    uint32_t tempreg = 0;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Get MULTCS_EN value */
    tempreg = hospi->Instance->CS & OSPI_CS_CSMAP_EN;
    /* Set bit0~2 */
    tempreg |= hospi->CSx;
    /* Set OSPI_CS */
    hospi->Instance->CS = tempreg;      
}


/******************************************************************************
*@brief : CS Release       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: None
******************************************************************************/
void HAL_OSPI_CS_Release(OSPI_HandleTypeDef *hospi)
{
    uint32_t tempreg = 0;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Get MULTCS_EN value */
    tempreg = hospi->Instance->CS & OSPI_CS_CSMAP_EN;
    /* Set OSPI_CS */
    hospi->Instance->CS = tempreg; 
}

/******************************************************************************
*@brief : Transmits an amount of data by loop mode.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@param : Timeout : Transmit Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;

    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if(!Size)    return HAL_ERROR;
    if (pData == NULL)    return HAL_ERROR;
    
    uiTimeout = Timeout;

    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;
    
    /* RX Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN;

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;
    
    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size > 0)
    {
        /* Wait Tx FIFO Not Full */
        while (hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }        
        hospi->Instance->DAT = *pData++;
        Size--;
        uiTimeout = Timeout;
    }
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    Status = HAL_OK;
    
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);
    
    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);
    

    return Status;
}

/******************************************************************************
*@brief : Transmits an amount of data by loop mode and retain CS.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@param : Timeout : Transmit Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_TransmitKeepCS(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;

    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if(!Size)    return HAL_ERROR;
    if (pData == NULL)    return HAL_ERROR;
    
    uiTimeout = Timeout;

    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;

    /* RX Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN;
    
    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;
    
    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size > 0)
    {
        /* Wait Tx FIFO Not Full */
        while (hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }        
        hospi->Instance->DAT = *pData++;
        Size--;
        uiTimeout = Timeout;
    }
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    Status = HAL_OK;
    
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);
    
    ///* Transmit End */
    //HAL_OSPI_CS_Release(hospi);
    

    return Status;
}

/******************************************************************************
*@brief : Receive an amount of data by loop mode.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@param : Timeout  : Receive Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Receive(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if (pData == NULL)    return HAL_ERROR;

    uiTimeout = Timeout;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;
    
    /* Tx Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size)
    {
        /* have no timeout */
        if (uiTimeout == 0) 
        {
            /* Wait Rx FIFO Not Empty */
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY);
        }
        else
        {
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY)
            {
                if (uiTimeout-- == 0) 
                {
                    Status = HAL_TIMEOUT;
                    goto End; 
                }
            }
        }      
        *pData++ = hospi->Instance->DAT;
        Size--;
        uiTimeout = Timeout;
    }

    Status = HAL_OK;
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));

End:   
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Receive End */
    HAL_OSPI_CS_Release(hospi);
    
    return Status;
}

/******************************************************************************
*@brief : Receive an amount of data by loop mode and retain CS.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@param : Timeout  : Receive Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_ReceiveKeepCS(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if (pData == NULL)    return HAL_ERROR;

    uiTimeout = Timeout;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;
    
    /* Tx Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size)
    {
        /* have no timeout */
        if (uiTimeout == 0) 
        {
            /* Wait Rx FIFO Not Empty */
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY);
        }
        else
        {
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY)
            {
                if (uiTimeout-- == 0) 
                {
                    Status = HAL_TIMEOUT;
                    goto End; 
                }
            }
        }      
        *pData++ = hospi->Instance->DAT;
        Size--;
        uiTimeout = Timeout;
    }

    Status = HAL_OK;
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));

End:   
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    ///* Receive End */
    //HAL_OSPI_CS_Release(hospi);
    
    return Status;
}

/******************************************************************************
*@brief : Transmits an amount of data by IT mode.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit_IT(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Tx machine is running */
    if (hospi->TxState != OSPI_TX_STATE_IDLE) 
    {
        return HAL_ERROR;
    }
    
    hospi->Tx_Size   = Size;
    hospi->Tx_Buffer = pData; 
    hospi->Tx_Count = 0;   

    /* Set machine is Sending */
    hospi->TxState = OSPI_TX_STATE_SENDING;
    
    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Clear Tx FIFO half empty Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_FIFO_HALF_EMPTY);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;

    /* Rx Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN;    
    
    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN; 

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    
    while (hospi->Tx_Count < hospi->Tx_Size)   
    {
        if (!(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL))
        {
            hospi->Instance->DAT = hospi->Tx_Buffer[hospi->Tx_Count++];
        }
        else
        {
            break; 
        }            
    }

    /* Enable Tx FIFO half empty Interrupt and Tx batch done Interrupt*/
    SET_BIT(hospi->Instance->IE, (OSPI_IE_TX_FIFO_HALF_EMPTY_EN | OSPI_IE_TX_BATCH_DONE_EN));

    
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data by IT mode.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Receive_IT(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Rx machine is running */
    if (hospi->RxState != OSPI_RX_STATE_IDLE) 
    {
        return HAL_ERROR;
    }
    
    /* Set Slave machine is receiving */
    hospi->RxState = OSPI_RX_STATE_RECEIVING;   

    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;
    
    /* TX Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);

    hospi->Rx_Size   = Size;
    hospi->Rx_Buffer = pData; 
    hospi->Rx_Count = 0;     

    /* Enable Rx FIFO Not Empty Interrupt */
    SET_BIT(hospi->Instance->IE, OSPI_STATUS_RX_FIFO_NOT_EMPTY | OSPI_IE_RX_BATCH_DONE_EN);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Transmits an amount of data by DMA mode.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit_DMA(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Rx machine is running */
    if (hospi->TxState != OSPI_TX_STATE_IDLE) 
    {
        return HAL_ERROR;
    }
    /* Set machine is Sending */
    hospi->TxState = OSPI_TX_STATE_SENDING;

    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Enable Tx Batch Done Interrupt */
    SET_BIT(hospi->Instance->IE, OSPI_STATUS_TX_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = Size;
    
    /* Tx DMA FIFO */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_DMA_LEVEL;
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_DMA_LEVEL_0;

    /* Rx Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN;  

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;
    
    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);

#ifdef HAL_DMA_MODULE_ENABLED
    HAL_DMA_Start(hospi->HDMA_Tx, (uint32_t)pData, (uint32_t)&hospi->Instance->DAT, Size);
#endif
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_DMA_REQ_EN;  
    return HAL_OK;
}

/******************************************************************************
*@brief : Receive an amount of data by DMA mode.
*         
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Receive_DMA(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    /* Rx machine is running */
    if (hospi->RxState != OSPI_RX_STATE_IDLE) 
    {
        return HAL_ERROR;
    }
    /* Set Slave machine is receiving */
    hospi->RxState = OSPI_RX_STATE_RECEIVING;


    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Enable Rx Batch Done Interrupt */
    SET_BIT(hospi->Instance->IE, OSPI_STATUS_RX_BATCH_DONE);

    /* Set Data Size */
    hospi->Instance->BATCH = Size;

    /* Rx FIFO */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_DMA_LEVEL_0;

    /* TX Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);

#ifdef HAL_DMA_MODULE_ENABLED
    HAL_DMA_Start(hospi->HDMA_Rx, (uint32_t)&hospi->Instance->DAT, (uint32_t)pData, Size);
#endif    
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_DMA_REQ_EN; 
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Simultaneously transmits and recieve an amount of data in loop mode.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pTxData : Pointer to transmit data buffer
*@param : pRxData : Pointer to recieve data buffer
*@param : Size  : Amount of data to be sent and receive
*@param : Timeout : sent and receive timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_SimultTransmitReceive(OSPI_HandleTypeDef *hospi, uint8_t *pTxData, \
                                                    uint8_t *pRxData, uint32_t Size, uint32_t Timeout)
{
    __IO uint32_t TxFlag = 1U, uiTimeout;
    HAL_StatusTypeDef Status = HAL_OK;

    /* Check SPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if ((pTxData == NULL)||(pRxData == NULL))    return HAL_ERROR;
    
    hospi->Tx_Count = 0;
    hospi->Rx_Count = 0;
    hospi->Tx_Buffer = pTxData;
    hospi->Rx_Buffer = pRxData;
    hospi->Tx_Size = Size;   
    hospi->Rx_Size = Size;   
    uiTimeout = Timeout;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;

    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Set Data Size */
    hospi->Instance->BATCH = hospi->Tx_Size;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    TxFlag = 1;

    
    while((hospi->Tx_Size>0) || (hospi->Rx_Size>0))
    {       
        /* Wait Tx FIFO Not Full */
        if((!(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)) && (hospi->Tx_Size > 0) && (TxFlag == 1U))
        {
            hospi->Instance->DAT = hospi->Tx_Buffer[hospi->Tx_Count++];
            hospi->Tx_Size--;
            TxFlag = 0;
        }
        
        /* Wait Rx FIFO Not Empty */
        if((!(hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_EMPTY)) && (hospi->Rx_Size > 0))
        {
            hospi->Rx_Buffer[hospi->Rx_Count++] = hospi->Instance->DAT;
            hospi->Rx_Size--;
            TxFlag = 1U;
        }
        
        
        /* Wait Timeout */
        if(uiTimeout)
        {
            uiTimeout--;
            if(uiTimeout == 0)
            {
                Status = HAL_TIMEOUT;
                goto End;
            }
        }
    }        
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    
    Status = HAL_OK;

End:    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);
    
    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);
    
    return Status;
}


/******************************************************************************
*@brief : Transmit data through FIFO Half Word mode.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@param : Timeout : Transmit Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_TransmitByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pData, \
                                                    uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;

    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if(!Size)    return HAL_ERROR;
    if (pData == NULL)    return HAL_ERROR;
    
    uiTimeout = Timeout;

    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Set Data Size */
    hospi->Instance->BATCH = (Size * 2);
    
    /* Rx Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN;     
    
    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size > 0)
    {
        /* Wait Tx FIFO Not Full */
        while(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }        
        hospi->Instance->DAT = *pData++;
        Size--;
        uiTimeout = Timeout;
    }
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    Status = HAL_OK;
    
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);
    
    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);
    

    return Status;
}

/******************************************************************************
*@brief : Transmit data through FIFO Word mode.        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to be sent
*@param : Timeout : Transmit Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_TransmitByWord(OSPI_HandleTypeDef *hospi, uint32_t *pData, \
                                                uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;

    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if(!Size)    return HAL_ERROR;
    if (pData == NULL)    return HAL_ERROR;
    
    uiTimeout = Timeout;

    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Set Data Size */
    hospi->Instance->BATCH = (Size * 4);
    
    /* Rx Disable */
    hospi->Instance->RX_CTL &= ~OSPI_RX_CTL_RX_EN; 
    
    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size > 0)
    {
        /* Wait Tx FIFO Not Full */
        while(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }        
        hospi->Instance->DAT = *pData++;
        Size--;
        uiTimeout = Timeout;
    }
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    Status = HAL_OK;
    
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);
    
    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);
    

    return Status;
}


/******************************************************************************
*@brief : Receive an amount of data through FIFO HalfWord mode.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@param : Timeout  : Receive Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_ReceiveByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pData, \
                                                    uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if (pData == NULL)    return HAL_ERROR;

    uiTimeout = Timeout;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = (Size * 2);
    
    /* TX Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;    
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size)
    {
        /* have no timeout */
        if (uiTimeout == 0) 
        {
            /* Wait Rx FIFO Not Empty */
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY);
        }
        else
        {
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY)
            {
                if (uiTimeout-- == 0) 
                {
                    Status = HAL_TIMEOUT;
                    goto End; 
                }
            }
        }      
        *pData++ = hospi->Instance->DAT;
        Size--;
        uiTimeout = Timeout;
    }

    Status = HAL_OK;
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));

End:   
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Receive End */
    HAL_OSPI_CS_Release(hospi);
    
    return Status;
}

/******************************************************************************
*@brief : Receive an amount of data through FIFO Word mode.       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pData : Pointer to data buffer
*@param : Size  : Amount of data to receive
*@param : Timeout  : Receive Timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_ReceiveByWord(OSPI_HandleTypeDef *hospi, uint32_t *pData, \
                                                uint32_t Size, uint32_t Timeout)
{
    HAL_StatusTypeDef Status = HAL_OK;
    __IO uint32_t uiTimeout;
    
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if (pData == NULL)    return HAL_ERROR;

    uiTimeout = Timeout;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    
    /* Set Data Size */
    hospi->Instance->BATCH = (Size * 4);
    
    /* TX Disable */
    hospi->Instance->TX_CTL &= ~OSPI_TX_CTL_TX_EN;    
    
    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;
    
    /* Receive Start */
    HAL_OSPI_CS_Select(hospi);
    
    while(Size)
    {
        /* have no timeout */
        if (uiTimeout == 0) 
        {
            /* Wait Rx FIFO Not Empty */
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY);
        }
        else
        {
            while (hospi->Instance->STATUS & SPI_STATUS_RX_FIFO_EMPTY)
            {
                if (uiTimeout-- == 0) 
                {
                    Status = HAL_TIMEOUT;
                    goto End; 
                }
            }
        }      
        *pData++ = hospi->Instance->DAT;
        Size--;
        uiTimeout = Timeout;
    }

    Status = HAL_OK;
    
    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));

End:   
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Receive End */
    HAL_OSPI_CS_Release(hospi);
    
    return Status;
}

/******************************************************************************
*@brief : transmits an amount of data in FIFO Half Word mode before receiving an amount of data through FIFO Byte        
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pTxData : Pointer to transmit data buffer
*@param : pRxData : Pointer to recieve data buffer
*@param : TxSize  : Amount of data to be sent
*@param : RxSize  : Amount of data to be receive
*@param : Timeout : sent and receive timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByByte(OSPI_HandleTypeDef *hospi, uint8_t *pTxData, uint32_t TXSize, \
                            uint8_t *pRxData, uint32_t RXSize, uint32_t Timeout)
{
    __IO uint32_t uiTimeout;
    HAL_StatusTypeDef Status = HAL_OK;

    /* Check SPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if ((pTxData == NULL)||(pRxData == NULL))    return HAL_ERROR;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Clear RX FIFO */
    SET_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET); 
    
    /* Set Data Size */
    hospi->Instance->BATCH = TXSize;

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(TXSize)
    {
        /* Wait Tx FIFO Not Full */       
        while(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    /* Clear Batch Done Flag  */
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

                    /* Tx Disable */
                    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

                    /* Transmit End */
                    HAL_OSPI_CS_Release(hospi);

                    Status = HAL_TIMEOUT;
                    return Status;
                }
            }
        }            
        hospi->Instance->DAT = *pTxData++;
        TXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

    /* Set Data Size */
    hospi->Instance->BATCH = RXSize;

    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;

    /* recv reStart */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(RXSize)
    {
        /* Wait Rx FIFO Not Empty */     
        while (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_EMPTY)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }
        *pRxData++ = hospi->Instance->DAT;
        RXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);

    return Status;
}

/******************************************************************************
*@brief : transmits an amount of data in FIFO Half Word mode before receiving an amount of data through FIFO Half Word      
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pTxData : Pointer to transmit data buffer
*@param : pRxData : Pointer to recieve data buffer
*@param : TxSize  : Amount of data to be sent
*@param : RxSize  : Amount of data to be receive
*@param : Timeout : sent and receive timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pTxData, \
                         uint32_t TXSize, uint16_t *pRxData, uint32_t RXSize, uint32_t Timeout)
{
    __IO uint32_t uiTimeout;
    HAL_StatusTypeDef Status = HAL_OK;

    /* Check SPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if ((pTxData == NULL)||(pRxData == NULL))    return HAL_ERROR;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Clear RX FIFO */
    SET_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET); 
    
    /* Set Data Size */
    hospi->Instance->BATCH = (TXSize * 2);

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(TXSize)
    {
        /* Wait Tx FIFO Not Full */       
        while(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    /* Clear Batch Done Flag  */
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

                    /* Tx Disable */
                    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

                    /* Transmit End */
                    HAL_OSPI_CS_Release(hospi);

                    Status = HAL_TIMEOUT;
                    return Status;
                }
            }
        }            
        hospi->Instance->DAT = *pTxData++;
        TXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

    /* Set Data Size */
    hospi->Instance->BATCH = (RXSize * 2);

    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;

    /* recv reStart */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(RXSize)
    {
        /* Wait Rx FIFO Not Empty */     
        while (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_EMPTY)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }
        *pRxData++ = hospi->Instance->DAT;
        RXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);

    return Status;
}

/******************************************************************************
*@brief : transmits an amount of data in FIFO Word mode before receiving an amount of data through FIFO Word       
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : pTxData : Pointer to transmit data buffer
*@param : pRxData : Pointer to recieve data buffer
*@param : TxSize  : Amount of data to be sent
*@param : RxSize  : Amount of data to be receive
*@param : Timeout : sent and receive timeout
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByWord(OSPI_HandleTypeDef *hospi, uint32_t *pTxData, uint32_t TXSize, \
                                                        uint32_t *pRxData, uint32_t RXSize, uint32_t Timeout)
{
    __IO uint32_t uiTimeout;
    HAL_StatusTypeDef Status = HAL_OK;

    /* Check SPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    
    if ((pTxData == NULL)||(pRxData == NULL))    return HAL_ERROR;
    
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

    /* Clear TX FIFO */
    SET_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->TX_CTL, OSPI_TX_CTL_TX_FIFO_RESET);
    
    /* Clear RX FIFO */
    SET_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET);
    CLEAR_BIT(hospi->Instance->RX_CTL, OSPI_RX_CTL_RX_FIFO_RESET); 
    
    /* Set Data Size */
    hospi->Instance->BATCH = (TXSize * 4);

    /* Tx Enable */
    hospi->Instance->TX_CTL |= OSPI_TX_CTL_TX_EN;

    /* Transmit Start */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(TXSize)
    {
        /* Wait Tx FIFO Not Full */       
        while(hospi->Instance->STATUS & OSPI_STATUS_TX_FIFO_FULL)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    /* Clear Batch Done Flag  */
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_TX_BATCH_DONE);
                    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);

                    /* Tx Disable */
                    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

                    /* Transmit End */
                    HAL_OSPI_CS_Release(hospi);

                    Status = HAL_TIMEOUT;
                    return Status;
                }
            }
        }            
        hospi->Instance->DAT = *pTxData++;
        TXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_TX_BATCH_DONE));
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Tx Disable */
    hospi->Instance->TX_CTL &= (~OSPI_TX_CTL_TX_EN);

    /* Set Data Size */
    hospi->Instance->BATCH = (RXSize * 4);

    /* Rx Enable */
    hospi->Instance->RX_CTL |= OSPI_RX_CTL_RX_EN;

    /* recv reStart */
    HAL_OSPI_CS_Select(hospi);
    
    uiTimeout = Timeout;
    
    while(RXSize)
    {
        /* Wait Rx FIFO Not Empty */     
        while (hospi->Instance->STATUS & OSPI_STATUS_RX_FIFO_EMPTY)
        {
            if(uiTimeout)
            {
                uiTimeout--;
                if (uiTimeout == 0)
                {
                    Status = HAL_TIMEOUT;
                    goto End;
                }
            }
        }
        *pRxData++ = hospi->Instance->DAT;
        RXSize--;
        uiTimeout = Timeout;
    }

    /* Wait Transmit Done */
    while (!(hospi->Instance->STATUS & OSPI_STATUS_RX_BATCH_DONE));
    
End:
    /* Clear Batch Done Flag  */
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_RX_BATCH_DONE);
    WRITE_BIT(hospi->Instance->STATUS, OSPI_STATUS_BATCH_DONE);
    /* Rx Disable */
    hospi->Instance->RX_CTL &= (~OSPI_RX_CTL_RX_EN);

    /* Transmit End */
    HAL_OSPI_CS_Release(hospi);

    return Status;
}

/******************************************************************************
*@brief : Get Tx state.
*         
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: OSPI Tx State
******************************************************************************/
uint8_t HAL_OSPI_GetTxState(OSPI_HandleTypeDef *hospi)
{
    return hospi->TxState;
}

/******************************************************************************
*@brief : Get Rx state.
*         
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@return: OSPI Rx State
******************************************************************************/
uint8_t HAL_OSPI_GetRxState(OSPI_HandleTypeDef *hospi)
{
    return hospi->RxState;
}

/******************************************************************************
*@brief : OSPI Wire Config     
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : X_Mode : 1x/2x/4x/8x Mode, see@ref X_MODE 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_WireConfig(OSPI_HandleTypeDef *hospi, uint32_t X_Mode)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_X_MODE(X_Mode));
    
    /* Set OSPI X_Mode */
    hospi->Instance->CTL = ((hospi->Instance->CTL) & (~SPI_CTL_X_MODE_Msk)) | \
                            (X_Mode << SPI_CTL_X_MODE_Pos);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : OSPI Transmission rate mode    
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : mode : OSPI_DTRM_STR or OSPI_DTRM_DTR, see@ref OSPI_DTR_STR 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_TransmitRateMode(OSPI_HandleTypeDef *hospi, uint32_t mode)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_DTRM(mode));
    
    /* Set OSPI DTRM */
    hospi->Instance->CTL = ((hospi->Instance->CTL) & (~OSPI_CTL_DTRM_Msk)) | \
                            (mode << OSPI_CTL_DTRM_Pos);
    
    return HAL_OK;
}


/******************************************************************************
*@brief : OSPI FIFO Write mode     
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : mode : Byte/Halfword/Word Mode, see@ref OSPI_FIFO_Mode 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_FifoWriteMode(OSPI_HandleTypeDef *hospi, uint32_t mode)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_FIFO_MODE(mode));
    
    /* Set OSPI FIFO Write Mode */
    hospi->Instance->CTL = ((hospi->Instance->CTL) & (~OSPI_CTL_FW_MODE_Msk)) | \
                            (mode << OSPI_CTL_FW_MODE_Pos);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : OSPI FIFO Read mode     
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : mode : Byte/Halfword/Word Mode, see@ref OSPI_FIFO_Mode 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_FifoReadMode(OSPI_HandleTypeDef *hospi, uint32_t mode)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_FIFO_MODE(mode));
    
    /* Set OSPI FIFO Write Mode */
    hospi->Instance->CTL = ((hospi->Instance->CTL) & (~OSPI_CTL_FR_MODE_Msk)) | \
                            (mode << OSPI_CTL_FR_MODE_Pos);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : set APM OPI Memory Dummy Clock    
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : APMClock : see@ref OSPI_APM_Dummy_Clock 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_APMDummyClock(OSPI_HandleTypeDef *hospi, uint32_t APMClock)
{
    assert_param(IS_OSPI_APM_DUMMY_CLOCK(APMClock));
    /* Clear APMD_CLK bit */
    hospi->Instance->CTL &= (~OSPI_CTL_APMD_CLK_Msk );
    /* Set APMD_CLK */
    hospi->Instance->CTL |= (APMClock << OSPI_CTL_APMD_CLK_Pos);
    
    return HAL_OK;
}

/******************************************************************************
*@brief : Send output delay 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : Outdelay : see@ref OSPI_TX_OUT_Delay 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_TxOutDelay(OSPI_HandleTypeDef *hospi, uint32_t Outdelay)
{
    assert_param(IS_OSPI_TX_OUT_DELAY(Outdelay));
    /* Clear OUTDLY bit */
    hospi->Instance->TX_CTL &= (~(OSPI_TX_CTL_OUTDLY_Msk));
    /* Set OUTDLY */
    hospi->Instance->TX_CTL |= (Outdelay << OSPI_TX_CTL_OUTDLY_Pos);
    
    return HAL_OK;
    
}

/******************************************************************************
*@brief : Receive sampling delay 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : Sampledelay : see@ref OSPI_MASTER_SAMPLE_SHIFT 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_RxSampleDelay(OSPI_HandleTypeDef *hospi, uint32_t Sampledelay)
{
    assert_param(IS_OSPI_SAMPLE_SHIFT(Sampledelay));
    /* Clear SSHIFT bit */
    hospi->Instance->RX_CTL &= ~(OSPI_RX_CTL_SSHIFT_Msk);
    /* Set SSHIFT */
    hospi->Instance->RX_CTL |= (Sampledelay << OSPI_RX_CTL_SSHIFT_Pos);
    
    return HAL_OK;
    
}


/******************************************************************************
*@brief : Memory mode enabled or disabled 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : state : see@ref OSPI_Memory_State 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_MemoryEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state)
{
    assert_param(IS_OSPI_MEMORY_STATE(state));
    
    if(state == MEMOACC1_MEMORY_DISABLE)
    {
        hospi->Instance->MEMO_ACC1 &= ~(MEMOACC1_MEMORY_ENABLE << OSPI_MEMO_ACC1_ACC_EN_Pos);
    }
    else
    {
        hospi->Instance->MEMO_ACC1 |= (MEMOACC1_MEMORY_ENABLE << OSPI_MEMO_ACC1_ACC_EN_Pos);
    }
    
    return HAL_OK;
    
}


/******************************************************************************
*@brief : DQS output enable or disable
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : state : see@ref OSPI_DQS_Output 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_DQSOutputEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state)
{
    assert_param(IS_OSPI_DQSOE(state));
    
    if(state == OSPI_DQSOE_DISABLE)
    {
        hospi->Instance->CTL &= ~(OSPI_DQSOE_ENABLE << OSPI_CTL_DQSOE_Pos);
    }
    else
    {
        hospi->Instance->CTL |= (OSPI_DQSOE_ENABLE << OSPI_CTL_DQSOE_Pos);
    }
    
    return HAL_OK;
    
}


/******************************************************************************
*@brief : DQS Sample enable or disable
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : state : see@ref OSPI_DQS_Smaple 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_DQSSampleEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state)
{
    assert_param(IS_OSPI_DQS_SAMPLE(state));
    
    if(state == OSPI_DQS_SAMPLE_DISABLE)
    {
        hospi->Instance->RX_CTL &= ~(OSPI_DQS_SAMPLE_ENABLE << OSPI_RX_CTL_DQS_SAMP_EN_Pos);
    }
    else
    {
        hospi->Instance->RX_CTL |= (OSPI_DQS_SAMPLE_ENABLE << OSPI_RX_CTL_DQS_SAMP_EN_Pos);
    }
    
    return HAL_OK;
    
}


/******************************************************************************
*@brief : Initialize the OSPI module Octal communication 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.     
*@param : DataMaskMode: see@ref OSPI_Data_Mask_Mode.
*@param : DataMaskCtrl: see@ref OSPI_DataMask_Ctrl.
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_DataMaskConfig(OSPI_HandleTypeDef *hospi, uint32_t DataMaskMode, uint32_t DataMaskCtrl)
{
    /* Check OSPI Parameter */
    assert_param (IS_OSPI_ALL_INSTANCE(hospi->Instance));
    assert_param(IS_OSPI_DM(DataMaskMode));
    assert_param(IS_OSPI_DMCTRL_ADDR(DataMaskCtrl));
    
    /* Clear DTRM、DM_EN、DMCTRL、Mem_Mode bit */
    hospi->Instance->CTL &= (~(OSPI_CTL_DM_EN | OSPI_CTL_DMCTRL));
    
    /* Set DM_EN、DMCTRL */
    hospi->Instance->CTL |= ((DataMaskMode << OSPI_CTL_DM_EN_Pos) | \
                            (DataMaskCtrl << OSPI_CTL_DMCTRL_Pos));
   
                    
    return HAL_OK;
}

/******************************************************************************
*@brief : dummy cycle to close SCK
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : state : DISABLE or ENABLE 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_DummyCyclewitchCloseClk(OSPI_HandleTypeDef *hospi, FunctionalState state)
{
    assert_param(IS_FUNCTIONAL_STATE(state));
    
    if(state == DISABLE)
    {
        hospi->Instance->MEMO_ACC2 &= ~(1 << OSPI_MEMO_ACC2_DUMMY_OFF_EN_Pos);
    }
    else
    {
        hospi->Instance->MEMO_ACC2 |= (1 << OSPI_MEMO_ACC2_DUMMY_OFF_EN_Pos);
    }
    
    return HAL_OK;
    
}


/******************************************************************************
*@brief : CS timeout pull up 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : state : see@ref OSPI_Cs_Timeout_State 
*@param : Timeout : see@ref OSPI_CSTimeout_Val 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_CSTimeoutPullup(OSPI_HandleTypeDef *hospi, uint32_t state, uint32_t Timeout)
{
    assert_param(IS_OSPI_CS_TIMEOUT_VAL(Timeout));
    assert_param(IS_OSPI_CS_TIMEOUT_STATE(state));
    
    hospi->Instance->CS_TIMEOUT_VAL = (hospi->Instance->CS_TIMEOUT_VAL & (~OSPI_CS_TIMEOUT_VAL)) | Timeout;
    
    if(state == MEMOACC1_CS_TIMEOUT_DISABLE)
    {
        hospi->Instance->MEMO_ACC1 &= ~OSPI_MEMO_ACC1_CS_TIMEOUT_EN;
    }
    else
    {
        hospi->Instance->MEMO_ACC1 |= OSPI_MEMO_ACC1_CS_TIMEOUT_EN;    
    }
    
    return HAL_OK;
    
}

/******************************************************************************
*@brief : CS pulls down the delay count value of the first clock edge 
*@param : hospi: a pointer of OSPI_HandleTypeDef structure which contains 
*         the configuration information for the specified OSPI.
*@param : delay : see@ref OSPI_CSDelay_Val 
*@return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_OSPI_CSPulldownToClkRisingEdge(OSPI_HandleTypeDef *hospi, uint32_t delay)
{
    assert_param(IS_OSPI_CS_DELAY_VAL(delay));

    hospi->Instance->CS_TIMEOUT_VAL = (hospi->Instance->CS_TIMEOUT_VAL & (~OSPI_CS_DELAY_VAL)) | \
                (delay << OSPI_CS_DELAY_VAL_Pos);  

    
    return HAL_OK;
    
}

void HAL_OSPI_Updata_RX_Shift_Clk(OSPI_TypeDef *Instance, uint32_t shift_clk)
{
    Instance->RX_CTL &= ~(OSPI_RX_CTL_SSHIFT);
    /* Set Master Sample shift*/
    Instance->RX_CTL |= (shift_clk << OSPI_RX_CTL_SSHIFT_Pos);
}


#endif //HAL_OSPI_MODULE_ENABLED

