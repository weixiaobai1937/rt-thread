
/******************************************************************************
* @file      : HAL_ADC.c
* @version   : 1.0
* @date      : 2022.10.29
* @brief     : ADC HAL module driver
*   
* @history   :	
*   2022.10.25    lwq    create
*   
******************************************************************************/
#include "hal.h"


#ifdef HAL_ADC_MODULE_ENABLED



/******************************************************************************
*@brief : This function handles ADC interrupt request.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: None
******************************************************************************/
__weak void HAL_ADC_IRQCallback(ADC_HandleTypeDef* hadc)
{
    /* 
        NOTE: This function should be modified, when the callback is needed
    */
    UNUSED(hadc);
}

/******************************************************************************
*@brief : Handle ADC interrupt request.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: None
******************************************************************************/
void HAL_ADC_IRQHandler(ADC_HandleTypeDef* hadc)
{
    HAL_ADC_IRQCallback(hadc);
}


/******************************************************************************
*@brief : Initialize the ADC MSP.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: None
******************************************************************************/
__weak void HAL_ADC_MspInit(ADC_HandleTypeDef* hadc)
{   
    /* 
      NOTE : This function should be modified by the user.
      the HAL_ADC_MspInit can be implemented in the user file.
    */
    UNUSED(hadc);  
}

/******************************************************************************
*@brief : Deinitialize the ADC MSP.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: None
******************************************************************************/
__weak void HAL_ADC_MspDeInit(ADC_HandleTypeDef* hadc)
{    
    /* 
    NOTE : This function should be modified by the user.
    the HAL_ADC_MspDeInit can be implemented in the user file.
    */
    UNUSED(hadc);        
}

/******************************************************************************
* @brief : Initialize the ADC differential MSP.
* @param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                 the configuration information for ADC module.
* @return: none
******************************************************************************/
__weak void HAL_ADC_Differential_MspInit(ADC_HandleTypeDef* hadc)
{   
    /* 
      NOTE : This function should be modified by the user.
      the HAL_ADC_MspInit can be implemented in the user file.
    */
    UNUSED(hadc);  
}

/******************************************************************************
* @brief : Deinitialize the ADC differential MSP.
* @param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                 the configuration information for ADC module.
* @return: none
******************************************************************************/
__weak void HAL_ADC_Differential_MspDeInit(ADC_HandleTypeDef* hadc)
{    
    /* 
    NOTE : This function should be modified by the user.
    the HAL_ADC_MspDeInit can be implemented in the user file.
    */
    UNUSED(hadc);        
}

 /******************************************************************************
*@brief : Init the ADC module.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Init(ADC_HandleTypeDef* hadc)
{
    /* Check the ADC handle allocation */
    if (hadc == NULL)
    {
        return HAL_ERROR;
    }

    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ConConvMode));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DMAMode));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.OverMode));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.OverSampMode));       
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.AnalogWDGEn));
    assert_param(IS_ADC_CLOCKSOURCE(hadc->Init.ClockSource)); 
    assert_param(IS_ADC_CLOCKDIV(hadc->Init.ClockPrescaler));   
    assert_param(IS_ADC_RESOLUTION(hadc->Init.Resolution));
    assert_param(IS_ADC_DATA_ALIGN(hadc->Init.DataAlign));
    assert_param(IS_ADC_REGULAR_DISC_NUMBER(hadc->Init.NbrOfDiscConversion));    
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DiscontinuousConvMode));
    assert_param(IS_ADC_OVERSAMPLING_RATIO(hadc->Init.Oversampling.Ratio));
    assert_param(IS_ADC_RIGHTBITSHIFT(hadc->Init.Oversampling.RightBitShift));
    assert_param(IS_ADC_OVERSAMPLING_TRIGER(hadc->Init.Oversampling.TriggeredMode));
    assert_param(IS_ADC_EXT_TRIG(hadc->Init.ExternalTrigConv));
    assert_param(IS_ADC_EXT_TRIG_EDGE(hadc->Init.ExternalTrigConvEdge));  


    /* Init the low level hardware : GPIO, CLOCK, NVIC, DMA */
    HAL_ADC_MspInit(hadc);
    
    /* Enable ADC Clock */
    __HAL_RCC_ADC_CLK_ENABLE();
    
    /* ADC regulator enable */
    hadc->Instance->CR2 |= ADC_CR2_ADCVREGEN;
    
    HAL_SimpleDelay(50000);//delay 20us
    
    /* Release Reset AFE */ 
    hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;
    
    /* Close ADC */
    hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;
    
    /* Set Clock Source */
    if(hadc->Init.ClockSource)
    {
        RCC->PERCFGR = (RCC->PERCFGR & (~RCC_PERCFGR_ADCCKS)) | \
                        ((hadc->Init.ClockSource - 2) << RCC_PERCFGR_ADCCKS_Pos);
        hadc->Instance->CCR |= ADC_CCR_CLKMODE;//ADC_KER_CLK
    }    
    else
    {
        hadc->Instance->CCR &= ~ADC_CCR_CLKMODE;//HCLK
    }
    
    /* Set Clock DIV */
    hadc->Instance->CCR = (hadc->Instance->CCR & (~ADC_CCR_ADCDIV_Msk)) | \
                        ((hadc->Init.ClockPrescaler - 1) << ADC_CCR_ADCDIV_Pos);
    

    if(hadc->Init.ChannelEn & ADC_CHANNEL_VBAT_EN)
    {
        /* Enable VBAT */
        hadc->Instance->CCR |= ADC_CCR_VBATEN;  
    }
    
    if(hadc->Init.ChannelEn & ADC_CHANNEL_VREF_EN)
    {
        /* Enable VREF */
        hadc->Instance->CCR |= ADC_CCR_VREFINTEN;      
    }
    
    if(hadc->Init.ChannelEn & ADC_CHANNEL_TS_EN)
    {
        /* Enable TS */
        hadc->Instance->CCR |= ADC_CCR_TSEN;      
    }

    /* Resolution */
    hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_RES)) | hadc->Init.Resolution;

    /* Set ADC data alignment */
    hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_ALIGN)) | hadc->Init.DataAlign;

    /* Set continued convert mode */
    if(hadc->Init.ConConvMode)
    {
        hadc->Instance->CR1 |= ADC_CR1_CONT;
    }
    else
    {
        hadc->Instance->CR1 &= ~ADC_CR1_CONT;
    }
    /* Set DMA mode */
    hadc->Instance->CR1 = (hadc->Instance->CR1 & (~ADC_CR1_DMA_Msk)) | \
                            (hadc->Init.DMAMode << ADC_CR1_DMA_Pos);
    
    /* Set Discontinued convert mode */
    if(hadc->Init.DiscontinuousConvMode)
    {
        /* Enable the selected ADC regular discontinuous mode */
        hadc->Instance->CR1 |= ADC_CR1_DISCEN;

        /* Set the number of channels to be converted in discontinuous mode */
        hadc->Instance->CR1 = (hadc->Instance->CR1 & (~ADC_CR1_DISCNUM_Msk)) | \
                                ((hadc->Init.NbrOfDiscConversion - 1) << ADC_CR1_DISCNUM_Pos);
    }
    else
    {
        /* Disable the selected ADC regular discontinuous mode */
        hadc->Instance->CR1 &= ~(ADC_CR1_DISCEN);
    }

    /* Overflow */
    if(hadc->Init.OverMode)
    {
        hadc->Instance->CR2 |= ADC_CR2_OVRMOD;
    }
    else
    {
        hadc->Instance->CR2 &= ~ADC_CR2_OVRMOD;
    }
    /* Over Sample Set */
    if(hadc->Init.OverSampMode)
    {
        /* over sample rate */
        hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_OVSR_Msk)) | \
                                (hadc->Init.Oversampling.Ratio << ADC_CR2_OVSR_Pos);
        
        /* over sample right shift */   
        hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_OVSS_Msk)) | \
                                (hadc->Init.Oversampling.RightBitShift << ADC_CR2_OVSS_Pos); 
        
        /* Regular channel over sample en */
        hadc->Instance->CR2 |= ADC_CR2_OVSE; 
        if(hadc->Init.Oversampling.TriggeredMode)
        {
            /* 1 time sample every trig */
            hadc->Instance->CR2 |= ADC_CR2_TROVS; 
        }
        else
        {
            /* N times sample every trig */
            hadc->Instance->CR2 &= ~ADC_CR2_TROVS; 
        }
    }

    /* ExTrigSel set */
    if(hadc->Init.ExternalTrigConv != ADC_SOFTWARE_START)
    {
        /* Select external trigger to start conversion */
        hadc->Instance->CR1 = (hadc->Instance->CR1 & (~ADC_CR1_EXTSEL_Msk)) | \
                                hadc->Init.ExternalTrigConv << ADC_CR1_EXTSEL_Pos;

        /* Select external trigger polarity */
        hadc->Instance->CR1 = (hadc->Instance->CR1 & (~ADC_CR1_EXTEN_Msk)) | \
                                hadc->Init.ExternalTrigConvEdge << ADC_CR1_EXTEN_Pos;
    }
    else
    {
        /* Reset the external trigger */
        hadc->Instance->CR1 &= ~(ADC_CR1_EXTSEL_Msk);
        hadc->Instance->CR1 &= ~(ADC_CR1_EXTEN_Msk);
    }

    /* Clear the sequence length */ 
    hadc->Instance->SQR1 &= ~ADC_SQR1_L; 

    return  HAL_OK;
}


/******************************************************************************
*@brief : DeInit the ADC module.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_DeInit(ADC_HandleTypeDef* hadc)
{
    /* Check the ADC handle allocation */
    if (hadc == NULL)
    {
        return HAL_ERROR;
    }
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    
    HAL_ADC_MspDeInit(hadc);
    HAL_ADC_Differential_MspDeInit(hadc);

    hadc->ChannelNum = 0;
    hadc->ConvCpltCallback = NULL;
    hadc->InjectedConvCpltCallback = NULL;
    hadc->LevelOutOfWindowCallback = NULL;
    memset(&hadc->Init, 0, sizeof(hadc->Init));

    return  HAL_OK;
}
    

/******************************************************************************
*@brief : Config the regular channel.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@param : sConfig : pointer to a ADC_ChannelConfTypeDef structure that contains
*                   the configuration information for ADC channel
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_ConfigChannel(ADC_HandleTypeDef* hadc, ADC_ChannelConfTypeDef* sConfig)
{   
    __IO uint32_t *Reg_ADC_OFR;	

    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_ADC_CHANNEL(sConfig->Channel));
    assert_param(IS_ADC_SEQUENCE(sConfig->Sq));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.OverSampMode)); 
    assert_param(IS_FUNCTIONAL_STATE(sConfig->Diff));
    assert_param(IS_ADC_OFFSET_NUMBER(sConfig->OffsetNumber));
    assert_param(IS_ADC_OFFSET(sConfig->Offset));
    assert_param(IS_ADC_OFFSET_CALCULATE(sConfig->OffsetCalculate));
    assert_param(IS_ADC_OFFSET_SIGN(sConfig->Offsetsign));
    assert_param(IS_ADC_REGULAR_LENGTH(hadc->ChannelNum));
    assert_param(IS_ADC_SMPCLOCK(sConfig->Smp));
    
    /* In hardware oversampling mode, the offset compensation function is ignored */
    if((sConfig->OffsetNumber != ADC_OFR_NONE) && (hadc->Init.OverSampMode))
    {
        return HAL_ERROR;
    }
    /* Differential mode set*/
    if(sConfig->Diff)
    {
        HAL_ADC_Differential_MspInit(hadc);
        hadc->Instance->DIFSEL |= 1 << sConfig->Channel;
    }
    else
    {
        hadc->Instance->DIFSEL &= ~(1 << sConfig->Channel);
    }

    /* Set Rule Sequence Conversion Order */
    if((sConfig->Sq >= 1)&&(sConfig->Sq <= 5))
    {    
        hadc->Instance->SQR1 = (hadc->Instance->SQR1 & (~(ADC_SQR1_SQ1_Msk << (5 * (sConfig->Sq - 1))))) | \
        (sConfig->Channel << (5 * sConfig->Sq));
    }
    else if((sConfig->Sq >= 6)&&(sConfig->Sq <= 11))
    {
        hadc->Instance->SQR2 = (hadc->Instance->SQR2 & (~(ADC_SQR2_SQ6_Msk << ( 5 * (sConfig->Sq - 6))))) | \
        (sConfig->Channel << (5 * (sConfig->Sq - 6)));
    }
    else if((sConfig->Sq >= 12)&&(sConfig->Sq <= 16))
    {
        hadc->Instance->SQR3 = (hadc->Instance->SQR3 & (~(ADC_SQR3_SQ12_Msk << ( 5 * (sConfig->Sq - 12))))) | \
        (sConfig->Channel << (5 * (sConfig->Sq - 12)));
    }
    else
    {
        return HAL_ERROR;
    }
    /* Set the length of the regular channel sequence */
    hadc->Instance->SQR1 = (hadc->Instance->SQR1 & (~ADC_SQR1_L_Msk)) | ( hadc->ChannelNum - 1);

    /* Set the SMPR to every register*/
    if(sConfig->Channel <= ADC_CHANNEL_7) 
    {
        hadc->Instance->SMPR1 = (hadc->Instance->SMPR1 & (~(ADC_SMPR1_SMP0_Msk << (4 * sConfig->Channel)))) | \
        (sConfig->Smp << (4 * sConfig->Channel ));
    }
    else if((sConfig->Channel >= ADC_CHANNEL_8)&&(sConfig->Channel <= ADC_CHANNEL_15))
    {
        hadc->Instance->SMPR2 = (hadc->Instance->SMPR2 & (~(ADC_SMPR2_SMP8_Msk << (4 * (sConfig->Channel-8))))) | \
        (sConfig->Smp << (4 * (sConfig->Channel - 8)));
    }
    else if((sConfig->Channel >= ADC_CHANNEL_16)&&(sConfig->Channel <= ADC_CHANNEL_19))
    {
        hadc->Instance->SMPR3 = (hadc->Instance->SMPR3 & (~(ADC_SMPR3_SMP16_Msk << ( 4 * (sConfig->Channel - 16))))) | \
        (sConfig->Smp << (4 * (sConfig->Channel - 16)));
    }
    else
    {
        return HAL_ERROR;
    }

    /* set Offset */
    if (sConfig->OffsetNumber != ADC_OFR_NONE)
    {
        hadc->Instance->OFR[sConfig->OffsetNumber] = (hadc->Instance->OFR[sConfig->OffsetNumber] & (~0XFFFFFFFF)) | \
        (ADC_OFRX_OFFSETX_EN | (sConfig->Channel << ADC_OFRX_OFFSETX_CH_Pos) | sConfig->OffsetCalculate | \
            sConfig->Offsetsign |sConfig->Offset);    			
    }
    else
    {
        hadc->Instance->OFR[0] &= ~ADC_OFRX_OFFSETX_EN;
        hadc->Instance->OFR[1] &= ~ADC_OFRX_OFFSETX_EN;
        hadc->Instance->OFR[2] &= ~ADC_OFRX_OFFSETX_EN;
        hadc->Instance->OFR[3] &= ~ADC_OFRX_OFFSETX_EN;
    }
    return HAL_OK;
}


/******************************************************************************
*@brief : Enable and start the ADC convertion.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Start(ADC_HandleTypeDef* hadc)
{
    /* Check the parameters */
    if(hadc == NULL)
    {
        return HAL_ERROR;
    }
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_ADC_EXT_TRIG(hadc->Init.ExternalTrigConv)); 
    
    /* clear flag */
    hadc->Instance->SR = (ADC_SR_AWD | ADC_SR_JEOG | ADC_SR_JEOC | ADC_SR_OVERF | ADC_SR_EOG  | ADC_SR_EOC | ADC_SR_EOSMP); 

    /* Enable the ADC */
    hadc->Instance->CR2 |= ADC_CR2_ADC_EN;
    
    /* Wait ADC ready */
    while(!(hadc->Instance->SR & ADC_SR_ADRDY));

    if(hadc->Init.ExternalTrigConv == ADC_SOFTWARE_START)
    {
        /* Start covertion */
        hadc->Instance->CR1 |= ADC_CR1_SWSTART;
    }

    /* Return function status */
    return HAL_OK;
}


/******************************************************************************
*@brief : Stop ADC conversion of regular group (and injected channels in 
*         case of auto_injection mode), disable ADC peripheral.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Stop(ADC_HandleTypeDef* hadc)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));

    if(hadc->Init.ConConvMode)
    {
        /* Set stop flag */
        hadc->Instance->CR2 |= ADC_CR2_STP;        
        /* Waitting stop flag be cleared */    
        while(hadc->Instance->CR2 & ADC_CR2_STP);
    }
    
    /* Reset ADC AFE */
    hadc->Instance->CR2 &= ~ADC_CR2_ADCRSTN;

    /* Disable the ADC peripheral */
    hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;
    
    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_AWD | ADC_SR_JEOG | ADC_SR_JEOC | ADC_SR_OVERF | ADC_SR_EOG  | ADC_SR_EOC | ADC_SR_EOSMP); 
    
    /* Release ADC AFE */
    hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;
    
    /* Return function status */
    return HAL_OK;
}


/******************************************************************************
*@brief : Enable ADC, start conversion of regular group with interruption.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Start_IT(ADC_HandleTypeDef* hadc)
{  
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.OverMode));
    
    /* Enable the ADC */
    hadc->Instance->CR2 |= ADC_CR2_ADC_EN;
    /* Wait ADC ready */
    while(!(hadc->Instance->SR & ADC_SR_ADRDY));

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_EOG | ADC_SR_EOC);      

    hadc->Instance->IE |= (ADC_IE_EOCIE | ADC_IE_EOGIE);

    /* Enable ADC overrun interrupt */
    /* If hadc->Init.OverMode is set to ADC_OVERMODE_DISABLE, only then is
    ADC_IE_OVERFIE enabled; otherwise data overwrite is considered as normal
    behavior and no CPU time is lost for a non-processed interruption */
    if (hadc->Init.OverMode)
    {
        hadc->Instance->SR |= ADC_SR_OVERF;  
        hadc->Instance->IE |= ADC_IE_OVERFIE;
    }

    if(hadc->Init.ExternalTrigConv == ADC_SOFTWARE_START)	
    {
        /* Start covertion */
        hadc->Instance->CR1 |= ADC_CR1_SWSTART;
    }

    /* Return function status */
    return HAL_OK;
}


/******************************************************************************
*@brief : Stop ADC conversion of regular group (and injected group in 
*         case of auto_injection mode), disable interrution of 
*         end-of-conversion, disable ADC peripheral.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Stop_IT(ADC_HandleTypeDef* hadc)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ConConvMode));
    
    if(hadc->Init.ConConvMode)
    {
        /* Set stop flag */
        hadc->Instance->CR2 |= ADC_CR2_STP;        
        /* Waitting stop flag be cleared */    
        while(hadc->Instance->CR2 & ADC_CR2_STP);
    }
    
    /* Reset ADC AFE */
    hadc->Instance->CR2 &= ~ADC_CR2_ADCRSTN;

    /* Disable the ADC peripheral */
    hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;

    /* Disable rehular interruptions */
    hadc->Instance->IE &= ~(ADC_IE_EOCIE | ADC_IE_EOGIE);  

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_EOG | ADC_SR_EOC);
    
    /* Release ADC AFE */
    hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;

    /* Return function status */
    return HAL_OK;
}

/******************************************************************************
*@brief : ADC retrieve conversion value intended to be used with polling or interruption.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@return: the ADC covert result
******************************************************************************/
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *hadc)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));

    return (hadc->Instance->DR);
}

/******************************************************************************
*@brief : Polling to get the results of the ADC converter.
*@param : hadc : pointer to a ADC_HandleTypeDef structure that contains
*                the configuration information for ADC module.
*@param : pData : Destination Buffer address.
*@param : Length : Number of data to be transferred from ADC peripheral to memory.
*@param : Timeout : Polling timeout.
*@return: the ADC covert result
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Polling(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length, uint32_t Timeout)
{
    uint32_t tmp_hal_status;
    __IO uint32_t uiTimeout; 

    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));    

    if(HAL_ADC_Start(hadc) != HAL_OK)
    {    
        return HAL_ERROR; 
    }
    if(!pData)
    {  
        return HAL_ERROR;
    }
    hadc->AdcResults = pData;
    uiTimeout = Timeout;

    while(Length)
    {
        tmp_hal_status = hadc->Instance->SR;
        if(tmp_hal_status & ADC_SR_EOC)
        {
            *hadc->AdcResults = hadc->Instance->DR;  
            hadc->Instance->SR = ADC_SR_EOC;
            hadc->AdcResults++;
            Length--;
        }
        if(tmp_hal_status & ADC_SR_OVERF)
        {
            hadc->Instance->SR = ADC_SR_OVERF;
        }        
        if(tmp_hal_status & ADC_SR_EOG)
        {
            hadc->Instance->SR = ADC_SR_EOG;
            break;
        }

        if(uiTimeout)
        {
            uiTimeout--;
            if(uiTimeout == 0)
            {
                return HAL_TIMEOUT;
            }
        }
    }

    HAL_ADC_Stop(hadc); 

    return HAL_OK;
}


/******************************************************************************
*@brief: Configures for the selected ADC injected channel its corresponding
*        rank in the sequencer and its sample time.
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*               the configuration information for the specified ADC.
*@param: sConfigInjected: ADC configuration structure for injected channel. 
*@return: None
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedConfigChannel(ADC_HandleTypeDef* hadc, ADC_InjectionConfTypeDef* sConfigInjected)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjecOversamplingMode)); 
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjectedDiscontinuousConvMode)); 
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->AutoInjectedConv)); 
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.DiscontinuousConvMode)); 
    assert_param(IS_FUNCTIONAL_STATE(sConfigInjected->InjectedDiff)); 
    assert_param(IS_ADC_JDR_NUMBER(sConfigInjected->InjectedOffsetNumber));
    assert_param(IS_ADC_CHANNEL(sConfigInjected->InjectedChannel)); 
    assert_param(IS_ADC_SMPCLOCK(sConfigInjected->InjectedSamplingTime)); 
    assert_param(IS_ADC_INJECTED_RANK(sConfigInjected->InjectedRank));   
    assert_param(IS_ADC_INJECTED_LENGTH(sConfigInjected->InjectedNbrOfConversion));
    assert_param(IS_ADC_EXT_TRIG(sConfigInjected->ExternalTrigInjecConv));
    assert_param(IS_ADC_EXT_TRIG_EDGE(sConfigInjected->ExternalTrigInjecConvEdge));
    assert_param(IS_ADC_OFFSET(sConfigInjected->InjectedOffset));
    assert_param(IS_ADC_OFFSET_CALCULATE(sConfigInjected->InjectedOffsetCalculate));
    assert_param(IS_ADC_OFFSET_SIGN(sConfigInjected->InjectedOffsetSign));
    assert_param(IS_ADC_OVERSAMPLING_RATIO(sConfigInjected->InjecOversampling.Ratio));
    assert_param(IS_ADC_RIGHTBITSHIFT(sConfigInjected->InjecOversampling.RightBitShift));    
    
    //In hardware oversampling mode, the offset compensation function is ignored
    if((sConfigInjected->InjectedOffsetNumber!= ADC_JDR_NONE) && (sConfigInjected->InjecOversamplingMode))
    {       
        return HAL_ERROR;
    }
    /* JDISCEN and JAUTO bits can't be set at the same time  */
    if((sConfigInjected->InjectedDiscontinuousConvMode) && (sConfigInjected->AutoInjectedConv))
    {
        return HAL_ERROR;
    }
    /*  DISCEN and JAUTO bits can't be set at the same time */
    if((hadc->Init.DiscontinuousConvMode) && (sConfigInjected->AutoInjectedConv))
    {    
        return HAL_ERROR;
    }

    /* Diff Channel COnfig */
    if(sConfigInjected->InjectedDiff)
    {
        hadc->Instance->DIFSEL |= (1 << sConfigInjected->InjectedChannel);       
    }
    else 
    {
        hadc->Instance->DIFSEL &= ~(1 << sConfigInjected->InjectedChannel);
    }

    /* Set the SMPR to every register*/
    if (sConfigInjected->InjectedChannel  <= ADC_CHANNEL_7)
    {
        hadc->Instance->SMPR1 = (hadc->Instance->SMPR1 & (~(ADC_SMPR1_SMP0_Msk << (4 * sConfigInjected->InjectedChannel)))) | \
        (sConfigInjected->InjectedSamplingTime << (4 * sConfigInjected->InjectedChannel));	
    }
    else if((sConfigInjected->InjectedChannel > ADC_CHANNEL_7) || (sConfigInjected->InjectedChannel <= ADC_CHANNEL_15))
    {
        hadc->Instance->SMPR2 = (hadc->Instance->SMPR2 & (~(ADC_SMPR2_SMP8_Msk << ( 4 * (sConfigInjected->InjectedChannel - 8))))) | \
        (sConfigInjected->InjectedSamplingTime << ( 4 * (sConfigInjected->InjectedChannel - 8)));
    }
    else if((sConfigInjected->InjectedChannel > ADC_CHANNEL_16) || (sConfigInjected->InjectedChannel <= ADC_CHANNEL_18))
    {
        hadc->Instance->SMPR3 = (hadc->Instance->SMPR3 & (~(ADC_SMPR3_SMP16_Msk << (4 * (sConfigInjected->InjectedChannel - 16))))) | \
        (sConfigInjected->InjectedSamplingTime << ( 4 * (sConfigInjected->InjectedChannel - 16)));
    }
    else
    {
        return HAL_ERROR;
    }
    /*---------------------------- ADCx JSQR Configuration -----------------*/
    /* JL configuration */
    hadc->Instance->JSQR &= ~ADC_JSQR_JL;
    hadc->Instance->JSQR |= (sConfigInjected->InjectedNbrOfConversion - 1) << ADC_JSQR_JL_Pos;

    /* Rank configuration */ 
    hadc->Instance->JSQR |= sConfigInjected->InjectedChannel <<(5*sConfigInjected->InjectedRank);

    /* Enable external trigger if trigger selection is different of software  */
    /* start.                                                                 */
    /* Note: This configuration keeps the hardware feature of parameter       */
    /*       ExternalTrigConvEdge "trigger edge none" equivalent to           */
    /*       software start.                                                  */ 
    if(sConfigInjected->ExternalTrigInjecConv != ADC_SOFTWARE_START)
    {  
        /* Select external trigger to start conversion */
        hadc->Instance->JSQR &= ~ADC_JSQR_JEXTSEL;
        hadc->Instance->JSQR |= sConfigInjected->ExternalTrigInjecConv << ADC_JSQR_JEXTSEL_Pos;

        /* Select external trigger polarity */
        hadc->Instance->JSQR &= ~ADC_JSQR_JEXTEN;
        hadc->Instance->JSQR |= sConfigInjected->ExternalTrigInjecConvEdge << ADC_JSQR_JEXTEN_Pos;
    }
    else
    {
        /* Reset the external trigger */
        hadc->Instance->JSQR &= ~ADC_JSQR_JEXTSEL;
        hadc->Instance->JSQR &= ~ADC_JSQR_JEXTEN;  
    }

    //JAUTO configuration
    if (sConfigInjected->AutoInjectedConv)
    {
        /* Enable the selected ADC automatic injected group conversion */
        hadc->Instance->CR1 |= ADC_CR1_JAUTO;
    }
    else
    {
        /* Disable the selected ADC automatic injected group conversion */
        hadc->Instance->CR1 &= ~ADC_CR1_JAUTO;
    }

    //InjectedDiscontinuousConvMode
    if (sConfigInjected->InjectedDiscontinuousConvMode)
    {
        /* Enable the selected ADC injected discontinuous mode */
        hadc->Instance->CR1 |= ADC_CR1_JDISCEN;
    }
    else
    {
        /* Disable the selected ADC injected discontinuous mode */
        hadc->Instance->CR1 &= ~ADC_CR1_JDISCEN;
    }

    /*set Offset */
    if (sConfigInjected->InjectedOffsetNumber != ADC_OFR_NONE)
    {
        hadc->Instance->OFR[sConfigInjected->InjectedOffsetNumber] = (hadc->Instance->OFR[sConfigInjected->InjectedOffsetNumber] & \
        (~0XFFFFFFFF)) | (ADC_OFRX_OFFSETX_EN | (sConfigInjected->InjectedChannel << ADC_OFRX_OFFSETX_CH_Pos) | \
        sConfigInjected->InjectedOffsetCalculate | sConfigInjected->InjectedOffsetSign | sConfigInjected->InjectedOffset); 
    }

    //Oversampling
    if (sConfigInjected->InjecOversamplingMode)
    {
        /* over sample rate */
        hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_OVSR_Msk)) | \
                              (sConfigInjected->InjecOversampling.Ratio << ADC_CR2_OVSR_Pos);
        /* over sample right shift */   
        hadc->Instance->CR2 = (hadc->Instance->CR2 & (~ADC_CR2_OVSS_Msk)) | \
                              (sConfigInjected->InjecOversampling.RightBitShift << ADC_CR2_OVSS_Pos);
        /* Inject channel over sample en */
        hadc->Instance->CR2 |= ADC_CR2_JOVSE; 		
    }
    else
    {
        /* Inject channel over sample Disable */
        hadc->Instance->CR2 &= ~ADC_CR2_JOVSE; 
    }

    return HAL_OK;	
}	

/******************************************************************************
*@brief: Enables the selected ADC software start conversion of the injected channels.
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*              the configuration information for the specified ADC.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedStart(ADC_HandleTypeDef* hadc)
{
    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_AWD | ADC_SR_JEOG | ADC_SR_JEOC | ADC_SR_OVERF | ADC_SR_EOG  | ADC_SR_EOC | ADC_SR_EOSMP); 
    
    /* Enable the ADC */
    hadc->Instance->CR2 |= ADC_CR2_ADC_EN;
    
    /* Wait ADC ready */
    while(!(hadc->Instance->SR & ADC_SR_ADRDY));

    if(((hadc->Instance->JSQR & ADC_JSQR_JEXTEN) == 0U) && ((hadc->Instance->CR1 & ADC_CR1_JAUTO) == 0U))
    {
        /* Start covertion */
        hadc->Instance->CR1 |= ADC_CR1_JSWSTART;
    }

    return HAL_OK;
}


/******************************************************************************
*@brief:  Stop ADC conversion of regular group (and injected channels in 
*         case of auto_injection mode), disable ADC peripheral.
*@param:  hadc: pointer to a ADC_HandleTypeDef structure that contains
*               the configuration information for the specified ADC.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedStop(ADC_HandleTypeDef* hadc)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ConConvMode)); 
    
    if(hadc->Init.ConConvMode)
    {
        /* Set stop flag */
        hadc->Instance->CR2 |= ADC_CR2_STP;        
        /* Waitting stop flag be cleared */    
        while(hadc->Instance->CR2 & ADC_CR2_STP);
    }
    
    /* Reset ADC AFE */
    hadc->Instance->CR2 &= ~ADC_CR2_ADCRSTN;

    /* Disable the ADC peripheral */
    hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_AWD | ADC_SR_JEOG | ADC_SR_JEOC | ADC_SR_OVERF | ADC_SR_EOG  | ADC_SR_EOC | ADC_SR_EOSMP); 
    
    /* Release ADC AFE */
    hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;

    /* Return function status */
    return HAL_OK;
}


/******************************************************************************
* @brief:  Enable ADC, start conversion of injected channel with interruption.
* @param:  hadc: pointer to a ADC_HandleTypeDef structure that contains
*          the configuration information for the specified ADC.
* @return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedStart_IT(ADC_HandleTypeDef* hadc)
{  
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));

    /* Enable the ADC */
    hadc->Instance->CR2 |= ADC_CR2_ADC_EN;
    /* Wait ADC ready */
    while(!(hadc->Instance->SR & ADC_SR_ADRDY));

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_JEOC | ADC_SR_JEOG);      

    hadc->Instance->IE |= (ADC_IE_JEOCIE | ADC_IE_JEOGIE);    

    if(((hadc->Instance->JSQR & ADC_JSQR_JEXTEN) == 0U) && ((hadc->Instance->CR1 & ADC_CR1_JAUTO) == 0U))
    {
        /* Enable the selected ADC software conversion for injected group */
        hadc->Instance->CR1 |= ADC_CR1_JSWSTART;
    }	

    /* Return function status */
    return HAL_OK;
}

/******************************************************************************
*@brief:  Stop ADC conversion of regular group (and injected group in 
*         case of auto_injection mode), disable interrution of 
*         end-of-conversion, disable ADC peripheral.
*@param:  hadc: pointer to a ADC_HandleTypeDef structure that contains
*               the configuration information for the specified ADC.
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedStop_IT(ADC_HandleTypeDef* hadc)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.ConConvMode));
    
    if(hadc->Init.ConConvMode)
    {
        /* Set stop flag */
        hadc->Instance->CR2 |= ADC_CR2_STP;        
        /* Waitting stop flag be cleared */    
        while(hadc->Instance->CR2 & ADC_CR2_STP);
    }
    
    /* Reset ADC AFE */
    hadc->Instance->CR2 &= ~ADC_CR2_ADCRSTN;

    /* Disable the ADC peripheral */
    hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;

    /* Disable Inject interruptions */
    hadc->Instance->IE &= ~(ADC_IE_JEOCIE | ADC_IE_JEOGIE);    

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_JEOG | ADC_SR_JEOC); 
    
    /* Release ADC AFE */
    hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;

    /* Return function status */
    return HAL_OK;
}

/******************************************************************************
*@brief: Gets the converted value from data register of injected channel.
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*              the configuration information for the specified ADC.
*@param: InjectedRank: the ADC injected rank.
*                      This parameter can be one of the following values:
*                      @arg ADC_INJECTED_RANK_1: Injected Channel1 selected
*                      @arg ADC_INJECTED_RANK_2: Injected Channel2 selected
*                      @arg ADC_INJECTED_RANK_3: Injected Channel3 selected
*                      @arg ADC_INJECTED_RANK_4: Injected Channel4 selected
*@return: the ADC covert result
******************************************************************************/
uint32_t HAL_ADC_InjectedGetValue(ADC_HandleTypeDef* hadc, uint32_t InjectedRank)
{
    __IO uint32_t tmp = 0U;
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_ADC_INJECTED_RANK(InjectedRank));

    /* Clear injected group conversion flag to have similar behaviour as        */
    /* regular group: reading data register also clears end of conversion flag. */
    hadc->Instance->SR = ADC_SR_JEOC; 

    /* Return the selected ADC converted value */ 
    tmp =  hadc->Instance->JDR[InjectedRank - 1];

    return tmp;
}

/******************************************************************************
*@brief: Poll for injected conversion complete
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*              the configuration information for the specified ADC.
*@param: Timeout: Timeout value in millisecond.  
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_InjectedPolling(ADC_HandleTypeDef* hadc, uint32_t InjectedRank, uint32_t* pData, \
                                            uint32_t Length, uint32_t Timeout)
{
    uint32_t tmp_hal_status;
    __IO uint32_t uiTimeout;  

    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));    

    if(HAL_ADC_Start(hadc) != HAL_OK)  return HAL_ERROR; 
    if(!pData)  return HAL_ERROR;

    hadc->AdcResults = pData;
    uiTimeout = Timeout;

    while(Length)
    {
        tmp_hal_status = hadc->Instance->SR;
        if(tmp_hal_status & ADC_SR_JEOC)
        {
            HAL_ADC_InjectedGetValue(hadc, InjectedRank);
            hadc->Instance->SR = ADC_SR_JEOC;
            hadc->AdcResults++;
            Length--;
        }

        if(tmp_hal_status & ADC_SR_OVERF)
        {
            hadc->Instance->SR = ADC_SR_OVERF;
        }        
        if(tmp_hal_status & ADC_SR_JEOG)
        {
            hadc->Instance->SR = ADC_SR_JEOG;
            break;
        }

        if(uiTimeout)
        {
            uiTimeout--;
            if(uiTimeout == 0)
            {
                return HAL_TIMEOUT;
            }
        }
    }

    return HAL_OK;
}

/******************************************************************************
*@brief: Enable ADC, start conversion of regular group and transfer result through DMA
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*              the configuration information for the specified ADC.
*@param: pData : Destination Buffer address. 
*@param: Length : Number of data to be transferred from ADC peripheral to memory. 
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_Start_DMA(ADC_HandleTypeDef* hadc, uint32_t* pData, uint32_t Length)
{
    HAL_StatusTypeDef tmp_hal_status;

    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));

    /* Specific case for first call occurrence of this function (DMA transfer */
    /* not activated and ADC disabled), DMA transfer must be activated        */
    /* with ADC disabled.                                                     */
    if ((hadc->Instance->CR1 & ADC_CR1_DMA) == 0UL)
    {
        if(hadc->Instance->CR2 & ADC_CR2_ADC_EN)
        {
            /* Reset ADC AFE */
            hadc->Instance->CR2 &= ~ADC_CR2_ADCRSTN;
            
            /* Disable ADC */
            hadc->Instance->CR2 &= ~ADC_CR2_ADC_EN;
            
            /* Release ADC AFE */
            hadc->Instance->CR2 |= ADC_CR2_ADCRSTN;
        }

        /* Enable ADC DMA mode */
        hadc->Instance->CR1 |= ADC_CR1_DMA;
    }

    /* Enable the ADC peripheral */
    hadc->Instance->CR2 |= ADC_CR2_ADC_EN;

    /* Clear the SR register */
    hadc->Instance->SR = (ADC_SR_AWD | ADC_SR_JEOG | ADC_SR_JEOC | ADC_SR_OVERF | \
                            ADC_SR_EOG  | ADC_SR_EOC | ADC_SR_EOSMP);

    /* Disable all interruptions before enabling the desired ones */
    hadc->Instance->IE &= ~(ADC_IE_EOSMPIE | ADC_IE_EOCIE | ADC_IE_EOGIE | ADC_IE_OVERFIE | \
                            ADC_IE_JEOCIE | ADC_IE_JEOGIE | ADC_IE_AWDIE); 
                            
#ifdef HAL_DMA_MODULE_ENABLED  
    /* Start the DMA channel */
    tmp_hal_status = HAL_DMA_Start_IT(hadc->DMA_Handle, (uint32_t)&hadc->Instance->DR, (uint32_t)pData, Length);
#endif

    /* Return function status */
    return tmp_hal_status;
}


/******************************************************************************
*@brief: Config the analog watchdog.   
*@param: hadc: pointer to a ADC_HandleTypeDef structure that contains
*              the configuration information for the specified ADC.
*@param: AnalogWDGConfig : pointer to a ADC_AnalogWDGConfTypeDef structure that contains
*                          the configuration information for ADC analog watchdog. 
*@return: HAL status
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_AnalogWDGConfig(ADC_HandleTypeDef* hadc, ADC_AnalogWDGConfTypeDef* AnalogWDGConfig)
{                                 
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(hadc->Instance));
    assert_param(IS_FUNCTIONAL_STATE(hadc->Init.AnalogWDGEn));
    assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->RegularChannel));
    assert_param(IS_ADC_CHANNEL(AnalogWDGConfig->InjectChannel));
    assert_param(IS_ADC_ANALOG_WATCHDOG(AnalogWDGConfig->WatchdogMode));
    assert_param(IS_ADC_THRESHOLD(AnalogWDGConfig->HighThreshold));
    assert_param(IS_ADC_THRESHOLD(AnalogWDGConfig->LowThreshold));
    assert_param(IS_FUNCTIONAL_STATE(AnalogWDGConfig->ITMode));
    
    
    if (hadc->Init.AnalogWDGEn)
    {
        /* Clear AWDEN, JAWDEN and AWDSGL bits */
        hadc->Instance->CR1 &= (~(ADC_CR1_AWDSGL_Msk | ADC_CR1_JAWDEN_Msk | ADC_CR1_AWDEN_Msk));
        
        /* Set the analog watchdog enable mode */
        hadc->Instance->CR1 |= AnalogWDGConfig->WatchdogMode;
        
        /* Configure ADC analog watchdog interrupt */
        if(AnalogWDGConfig->ITMode)
        {
            hadc->Instance->IE |= ADC_IE_AWDIE;
        }
        else
        {
            hadc->Instance->IE &= ~ADC_IE_AWDIE;
        }
       
        if(AnalogWDGConfig->WatchdogMode & ADC_CR1_AWDSGL) 
        {
            /* Select the analog watchdog regular channel */
            if(AnalogWDGConfig->WatchdogMode & ADC_CR1_AWDEN)
            {
                /* Clear AWDCH bits */
                hadc->Instance->CR1 &= ~ADC_CR1_AWDCH_Msk;
                hadc->Instance->CR1 |= AnalogWDGConfig->RegularChannel;
            }
            /* Select the analog watchdog inject channel */
            if(AnalogWDGConfig->WatchdogMode & ADC_CR1_JAWDEN)
            {
                /* Clear AWDCH bits */
                hadc->Instance->CR1 &= ~ADC_CR1_AWDJCH_Msk;
                hadc->Instance->CR1 |= (AnalogWDGConfig->InjectChannel << ADC_CR1_AWDJCH_Pos);                
            }
        }
    }
    
    if(AnalogWDGConfig->Diff)
    {
        hadc->Instance->HTR = AnalogWDGConfig->HighThreshold << 16;
        hadc->Instance->LTR = AnalogWDGConfig->LowThreshold << 16;
    }
    else
    {
        hadc->Instance->HTR = AnalogWDGConfig->HighThreshold;
        hadc->Instance->LTR = AnalogWDGConfig->LowThreshold;
    }
    
    /* Return function status */
    return HAL_OK;
}


/******************************************************************************
*@brief: Checks whether the specified ADC flag is set or not.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: ADC_FLAG: specifies the flag to check.                                               
*@return: The new state of ADC_FLAG (SET or RESET).
******************************************************************************/
FlagStatus HAL_ADC_GetFlagStatus(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{
    FlagStatus bitstatus = RESET;
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_FLAG(ADC_FLAG));

    /* Check the status of the specified ADC flag */
    if ((ADCx->SR & ADC_FLAG) != RESET)
    {
        /* ADC_FLAG is set */
        bitstatus = SET;
    }
    else
    {
        /* ADC_FLAG is reset */
        bitstatus = RESET;
    }
    /* Return the ADC_FLAG status */
    return  bitstatus;
}

/******************************************************************************
*@brief: Clears the ADCx's pending flags.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: ADC_FLAG: specifies the flag to clear.                       
*@return: None
******************************************************************************/
void HAL_ADC_ClearFlag(ADC_TypeDef* ADCx, uint8_t ADC_FLAG)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_FLAG(ADC_FLAG));

    /* Clear the selected ADC flags */
    ADCx->SR = ADC_FLAG;
}


/******************************************************************************
*@brief: Enables or disables the specified ADC interrupts.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: ADC_IT: specifies the ADC interrupt sources to be enabled or disabled.                   
*@param: NewState: new state of the specified ADC interrupts.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None.
******************************************************************************/
void HAL_ADC_ITConfig(ADC_TypeDef* ADCx, uint16_t ADC_IT, FunctionalState NewState)  
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    assert_param(IS_ADC_IT_FLAG(ADC_IT));    

    if (NewState != DISABLE)
    {
        /* Enable the selected ADC interrupts */
        ADCx->IE |= ADC_IT;
    }
    else
    {
        /* Disable the selected ADC interrupts */
        ADCx->IE &= (~ADC_IT);
    }
}

/******************************************************************************
*@brief: Checks whether the specified ADC interrupt has occurred or not.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: ADC_IT: specifies the ADC interrupt source to check.                       
*@return: The new state of ADC_FLAG (SET or RESET).
******************************************************************************/
ITStatus HAL_ADC_GetITStatus(ADC_TypeDef* ADCx, uint16_t ADC_IT)
{
    ITStatus bitstatus = RESET;
    uint32_t enablestatus = 0;

    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_IT_FLAG(ADC_IT));

    /* Get the ADC_IT enable bit status */
    enablestatus = (ADCx->IE & ADC_IT) ;

    /* Check the status of the specified ADC interrupt */
    if (((ADCx->SR & ADC_IT) != RESET) && enablestatus)
    {
        /* ADC_IT is set */
        bitstatus = SET;
    }
    else
    {
        /* ADC_IT is reset */
        bitstatus = RESET;
    }
    /* Return the ADC_IT status */
    return  bitstatus;
}

/******************************************************************************
*@brief: Clears the ADC's interrupt pending bits.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: ADC_IT: specifies the ADC interrupt pending bit to clear.
*@return: None
******************************************************************************/
void HAL_ADC_ClearITPendingBit(ADC_TypeDef* ADCx, uint32_t ADC_IT)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_IT_FLAG(ADC_IT));
    
    if((ADCx->IE & ADC_IT)!= RESET)
    {
        /* Clear the interrupt pending bits in the RTC_SR register */
        ADCx->SR = ADC_IT;
    } 
}


/******************************************************************************
*@brief: Configure resolution and data alignment.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: resolution: Data resolution. 
*@param: dataAlign: Data align. 
*@return: None
******************************************************************************/
void HAL_ADC_ResolutionDataAlignConfig(ADC_TypeDef* ADCx, uint32_t resolution, uint32_t dataAlign)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_RESOLUTION(resolution)); 
    assert_param(IS_ADC_DATA_ALIGN(dataAlign)); 
    
    /* Clear ALIGN, Resolution bits */
    ADCx->CR2 &= (~(ADC_CR2_RES_Msk | ADC_CR2_ALIGN_Msk));
  
    /* set Resolution */
    /* Set ADC data alignment */
    ADCx->CR2 |= dataAlign | resolution ;
}

/******************************************************************************
*@brief: Enables or disables the ADC stop control.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the ADC stop control. 
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_StopControlCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the ADC stop control */
        ADCx->CR2 |= ADC_CR2_STP;
    }
    else
    {
        /* Disable the ADC stop control */
        ADCx->CR2 &= (~ADC_CR2_STP);
    }
}

/******************************************************************************
*@brief: Configure overflow mode.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: mode: ADC saves the new or the last sampling data when overflow occurs. 
*@return: None
******************************************************************************/
void HAL_ADC_OverflowmodeConfig(ADC_TypeDef* ADCx, uint32_t mode)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_OVERFLOW_MODE(mode));
    
    /* Clear OVRMOD bits */
    ADCx->CR2 &= (~ADC_CR2_OVRMOD_Msk);
    
    /* Store the new register value */
    ADCx->CR2 |= mode;
    
}

/******************************************************************************
*@brief: Configure single-ended/differential conversion mode.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: Channel: the channel is set with single terminal or differential. 
*@param: mode: single-ended/differential conversion mode.
*@return: None
******************************************************************************/
void HAL_ADC_DifferentialConfig(ADC_TypeDef* ADCx, uint8_t channel, uint8_t differential)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_CHANNEL(channel));
    assert_param(IS_ADC_DIFF(differential));

    /* Clear the old single-ended/differential conversion mode */
    ADCx->DIFSEL &= (~ADC_DIFSEL_DIFSEL_Msk);

    /* Set the new single-ended/differential conversion mode */
    ADCx->DIFSEL |= (differential << channel);
}

/******************************************************************************
*@brief: Set the injected or regular channels conversion value offset
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: channel: the ADC channel to set its offset. 
*@param: hoffset: pointer to an ADC_OffsetConfTypeDef structure that contains
*                 the configuration information for Offset Register.
*@return: None
******************************************************************************/
void HAL_ADC_OffsetConfig(ADC_TypeDef* ADCx, uint8_t channel, ADC_OffsetConfTypeDef *hoffset)
{
    uint32_t tmpreg1 = 0;
    __IO uint32_t tmpADC_OFR = 0;
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_CHANNEL(channel));
    assert_param(IS_ADC_OFFSET_NUMBER(hoffset->OffsetNumber));
    assert_param(IS_ADC_OFFSET(hoffset->Offset));
    assert_param(IS_ADC_OFFSET_CALCULATE(hoffset->OffsetCalculate));
    assert_param(IS_ADC_OFFSET_SIGN(hoffset->Offsetsign));

    /*---------------------------- ADCx OFRx Configuration -----------------*/
    // tmpADC_OFR = (uint32_t)ADCx;
    // tmpADC_OFR += (uint32_t)(ADC_OFR1_ADDR_OFFSET + 4 * (Offset_InitStruct->OffsetNumber));

    /* Set the new sample time */
    tmpreg1 = hoffset->Offset | \
              hoffset->OffsetCalculate | \
              hoffset->Offsetsign | \
              (channel << ADC_OFRX_OFFSETX_CH_Pos) |\
              ADC_OFRX_OFFSETX_EN;

    /* Store the new register value */
    // *(__IO uint32_t*)(tmpADC_OFR) = tmpreg1;
    ADCx->OFR[hoffset->OffsetNumber] = tmpreg1;
}

/******************************************************************************
*@brief: Configure oversampling
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: hOversampling: pointer to an ADC_OversamplingTypeDef structure that contains
*                       the configuration information for Oversampling Register.
*@return: None
******************************************************************************/
void HAL_ADC_OverSamplingConfig(ADC_TypeDef* ADCx, ADC_OversamplingTypeDef *hOversampling)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_OVERSAMPLING_RATIO(hOversampling->Ratio));
    assert_param(IS_ADC_RIGHTBITSHIFT(hOversampling->RightBitShift));
    assert_param(IS_ADC_OVERSAMPLING_TRIGER(hOversampling->TriggeredMode));
    
    /* Clear OVSR、OVSS、TROVS bits */
    ADCx->CR2 &= ~(ADC_CR2_OVSR_Msk | ADC_CR2_OVSS_Msk | ADC_CR2_TROVS_Msk);
      
    /* set new rate、right shift、Triger Mode */   
    ADCx->CR2 |= (hOversampling->Ratio << ADC_CR2_OVSR_Pos) | (hOversampling->RightBitShift << ADC_CR2_OVSS_Pos) | ADC_CR2_TROVS; 

}

/******************************************************************************
*@brief: Enables or disables the Regular group oversampling
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the Regular group oversampling.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_OverSamplingCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the Regular group oversampling */
        ADCx->CR2 |= ADC_CR2_OVSE;
    }
    else
    {
        /* Disable the Regular group oversampling */
        ADCx->CR2 &= (~ADC_CR2_OVSE);
    }
}

/******************************************************************************
*@brief: Enables or disables the Inject group oversampling
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the Inject group oversampling.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_InjectOverSamplingCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the Inject group oversampling */
        ADCx->CR2 |= ADC_CR2_JOVSE;
    }
    else
    {
        /* Disable the Inject group oversampling */
        ADCx->CR2 &= (~ADC_CR2_JOVSE);
    }
}


/******************************************************************************
*@brief: Enables or disables the ADC continuous conversion mode 
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the selected ADC continuous conversion mode
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_ContinuousModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState != DISABLE)
    {
        /* Enable the selected ADC continuous conversion mode */
        ADCx->CR1 |= ADC_CR1_CONT;
    }
    else
    {
        /* Disable the selected ADC continuous conversion mode */
        ADCx->CR1 &= (~ADC_CR1_CONT);
    }
}

/******************************************************************************
*@brief: Configures the discontinuous mode for the selected ADC regular group channel.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: number: specifies the discontinuous mode regular channel count value.
*                This number must be between 1 and 8.
*@return: None
******************************************************************************/
void HAL_ADC_DiscModeChannelCountConfig(ADC_TypeDef* ADCx, uint8_t number)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_REGULAR_DISC_NUMBER(number));

    /* Clear the old discontinuous mode channel count */
    ADCx->CR1 &= (~ADC_CR1_DISCNUM_Msk);

    /* Set the discontinuous mode channel count */
    ADCx->CR1 |= ((number - 1) << ADC_CR1_DISCNUM_Pos);
}

/******************************************************************************
*@brief: Enables or disables the discontinuous mode on regular group channel for the ADC
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the selected ADC discontinuous mode on regular group channel.
*                  This parameter can be: ENABLE or DISABLE.
* @return: None
******************************************************************************/
void HAL_ADC_DiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState != DISABLE)
    {
        /* Enable the selected ADC regular discontinuous mode */
        ADCx->CR1 |= ADC_CR1_DISCEN;
    }
    else
    {
        /* Disable the selected ADC regular discontinuous mode */
        ADCx->CR1 &= (~ADC_CR1_DISCEN);
    }
}


/******************************************************************************
*@brief: Enables or disables the discontinuous mode on inject group channel for the ADC
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the selected ADC discontinuous mode on Inject group channel.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_InjectDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));

    if (NewState != DISABLE)
    {
        /* Enable the selected ADC inject  discontinuous mode */
        ADCx->CR1 |= ADC_CR1_JDISCEN;
    }
    else
    {
        /* Disable the selected ADC inject discontinuous mode */
        ADCx->CR1 &= (~ADC_CR1_JDISCEN);
    }
}


/******************************************************************************
*@brief: Enables or disables the VBGA channel.
*@param: NewState: new state of the VBGA channel.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_VREFCmd(ADC_TypeDef* ADCx, FunctionalState NewState)                
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    

    if (NewState != DISABLE)
    {
        /* Enable the VBGA channel */
        ADCx->CCR |= ADC_CCR_VREFINTEN;
    }
    else
    {
        /* Disable the VBGA channel */
        ADCx->CCR &= (~ADC_CCR_VREFINTEN);
    }
}

/******************************************************************************
*@brief: Enables or disables the VBAT channel.
*@param: NewState: new state of the VBAT channel.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_VBATCmd(ADC_TypeDef* ADCx, FunctionalState NewState)                
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    

    if (NewState != DISABLE)
    {
        /* Enable the VBAT channel */
        ADCx->CCR |= ADC_CCR_VBATEN;
    }
    else
    {
        /* Disable the VBAT channel */
        ADCx->CCR &= (~ADC_CCR_VBATEN);
    }
}

/******************************************************************************
*@brief: Enables or disables the TS channel.
*@param: NewState: new state of the temperature sensor channel.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_TSCmd(ADC_TypeDef* ADCx, FunctionalState NewState)                
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    

    if (NewState != DISABLE)
    {
        /* Enable the TS channel */
        ADCx->CCR |= ADC_CCR_TSEN;
    }
    else
    {
        /* Disable the TS channel */
        ADCx->CCR &= (~ADC_CCR_TSEN);
    }
}

/******************************************************************************
*@brief: Enables or disables the specified ADC DMA request.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the selected ADC DMA transfer.
*                  This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_DMAmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the selected ADC DMA request */
        ADCx->CR1 |= ADC_CR1_DMA;
    }
    else
    {
        /* Disable the selected ADC DMA request */
        ADCx->CR1 &= (~ADC_CR1_DMA);
    }
}

/******************************************************************************
*@brief: Configures the ADCx external trigger for injected channels conversion.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: trigger: specifies the ADC trigger to start injected conversion. 
*@param: edge: specifies the ADC trigger to start injected conversion.                        
*@return: None
******************************************************************************/
void HAL_ADC_InjectedExternalTriggerConfig(ADC_TypeDef* ADCx, uint32_t trigger, uint32_t edge)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_EXT_TRIG(trigger));
    assert_param(IS_ADC_EXT_TRIG_EDGE(edge)); 
    
    /* Clear the old external trigger for injected group */
    ADCx->JSQR &= (~(ADC_JSQR_JEXTSEL_Msk | ADC_JSQR_JEXTEN_Msk));

    /* Set the external trigger for injected group */
    ADCx->JSQR |= (trigger << ADC_JSQR_JEXTSEL_Pos) | (edge << ADC_JSQR_JEXTEN_Pos);
}


/******************************************************************************
*@brief: Configures the ADCx external trigger edge for regular channels conversion.
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: trigger: specifies the ADC external trigger edge to start regular conversion.
*@param: edge: specifies the ADC trigger to start regular conversion.   
*@return: None
******************************************************************************/
void HAL_ADC_RegularExternalTriggerConfig(ADC_TypeDef* ADCx, uint32_t trigger, uint32_t edge)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_ADC_EXT_TRIG(trigger));
    assert_param(IS_ADC_EXT_TRIG_EDGE(edge));     
    
    /* Clear the old external trigger edge for injected group */
    ADCx->CR1 &= (~(ADC_CR1_EXTSEL_Msk | ADC_CR1_EXTEN_Msk));
    
    /* Set the new external trigger edge for injected group */
    ADCx->CR1 |= (trigger << ADC_CR1_EXTSEL_Pos) | (edge << ADC_CR1_EXTEN_Pos);
}

/******************************************************************************
*@brief:  Enables or disables the selected ADC automatic injected group conversion after regular one.
*@param:  ADCx: where x can be 1 to select the ADC peripheral.
*@param:  NewState: new state of the selected ADC auto injected conversion
*                   This parameter can be: ENABLE or DISABLE.
*@return: None
******************************************************************************/
void HAL_ADC_AutoInjectedConvCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the selected ADC automatic injected group conversion */
        ADCx->CR1 |= ADC_CR1_JAUTO;
    }
    else
    {
        /* Disable the selected ADC automatic injected group conversion */
        ADCx->CR1 &= (~ADC_CR1_JAUTO);
    }
}

/******************************************************************************
*@brief: Enables or disables the discontinuous mode for injected group channel for the specified ADC
*@param: ADCx: where x can be 1 to select the ADC peripheral.
*@param: NewState: new state of the selected ADC auto injected conversion
*                  This parameter can be: ENABLE or DISABLE.
* @return: None
******************************************************************************/
void HAL_ADC_InjectedDiscModeCmd(ADC_TypeDef* ADCx, FunctionalState NewState)
{
    /* Check the parameters */
    assert_param(IS_ADC_INSTANCE(ADCx));
    assert_param(IS_FUNCTIONAL_STATE(NewState));
    if (NewState != DISABLE)
    {
        /* Enable the selected ADC injected discontinuous mode */
        ADCx->CR1 |= ADC_CR1_JDISCEN;
    }
    else
    {
        /* Disable the selected ADC injected discontinuous mode */
        ADCx->CR1 &= (~ADC_CR1_JDISCEN);
    }
}


/******************************************************************************
* @brief : ADC VREFP configuration.
* @param : mode: VREFP comes from internal VREFBUF or external input
* @param : voltage: Voltage of VREFBUF
* @return: none
******************************************************************************/ 
void HAL_ADC_VrefpConfig(uint8_t mode, uint8_t voltage)
{
    uint32_t TrimValue;
    uint8_t temp;
    /* Check the parameters */
    assert_param(IS_ADC_VREFP(voltage));
    
    __HAL_RCC_ADC_CLK_ENABLE();
    if(mode == VREFP_INTERN)
    {
        /* Select internal VREFBUF */
        ADC->CVRB = (ADC->CVRB & (~0xf)) | 0x1 | voltage;
        HAL_EFUSE_ReadBytes(EFUSE1, EFUSE1_ADC_1V2, (uint8_t *)&TrimValue, 2, 100000);//Read the 1.2v trim value in 3.0v vrefp.
        if(TrimValue)//Used to detect whether CP testing has been conducted
        {
            HAL_EFUSE_ReadBytes(EFUSE1, EFUSE1_ADC_VREFBUF_TRIM, (uint8_t *)&TrimValue, 3, 100000);//Read VREFBUF VTRIM.
            /*
                [4:0]   VREFBI为1.5V是VTRIM
                [9:5]   VREFBI为1.8V是VTRIM
                [14:10] VREFBI为2.0V是VTRIM
                [19:15] VREFBI为2.5V是VTRIM
            */
            if(voltage == VREFP_INTERN_1V5)
            {
                temp = TrimValue & 0x1f;
                ADC->CVRB = (ADC->CVRB & (~ADC_CVRB_VRBTRIM)) | (temp << ADC_CVRB_VRBTRIM_Pos);
            }
            if(voltage == VREFP_INTERN_1V8)
            {
                temp = (TrimValue >> 5) & 0x1f;
                ADC->CVRB = (ADC->CVRB & (~ADC_CVRB_VRBTRIM)) | (temp << ADC_CVRB_VRBTRIM_Pos);                
            }
            else if(voltage == VREFP_INTERN_2V0)
            {
                temp = (TrimValue >> 10) & 0x1f;
                ADC->CVRB = (ADC->CVRB & (~ADC_CVRB_VRBTRIM)) | (temp << ADC_CVRB_VRBTRIM_Pos);
            }
            else if(voltage == VREFP_INTERN_2V5)
            {
                temp = (TrimValue >> 15) & 0x1f;
                ADC->CVRB = (ADC->CVRB & (~ADC_CVRB_VRBTRIM)) | (temp << ADC_CVRB_VRBTRIM_Pos);
            }
        }
    }
    else
    {
        /* Select external VREF+ */
        ADC->CVRB = (ADC->CVRB & (~0xf)) | 0x2;
    }
}

/******************************************************************************
* @brief : ADC factory basic parameter settings.
* @param : none
* @param : none
* @return: HAL_StatusTypeDef
******************************************************************************/
HAL_StatusTypeDef HAL_ADC_FactoryBasicParameterConfig(void)
{
    uint8_t data[2];
    uint16_t TrimValue;
    
    HAL_EFUSE_ReadBytes(EFUSE1, EFUSE1_ADC_1V2, (uint8_t *)&TrimValue, 2, 100000);//Read the 1.2v trim value in 3.0v vrefp.
    
    if(TrimValue == 0)//Used to detect whether CP testing has been conducted
    {
        return HAL_ERROR;
    }
    
    HAL_EFUSE_ReadBytes(EFUSE1, EFUSE1_ADC_STRIM, data, 2, 100000);

    __HAL_RCC_ADC_CLK_ENABLE(); 
    /*
        data0 [6:0]: ADC1_CALFACT[6:0] (CALFACT_S)
        data1 [6:0]: ADC1_CALFACT[22:16] (CALFACT_D)
    */
    ADC->CALFACT = (data[0] & 0x7f) | ((data[1] & 0x7f) << ADC_CALFACT_CALFACT_D_Pos);     
    //ADC->CR2 = (ADC->CR2 & (~ADC_CR2_TRIMLDO12)) | ((data[2] & 0xf) << ADC_CR2_TRIMLDO12_Pos); 

    return HAL_OK;   
}







#endif










