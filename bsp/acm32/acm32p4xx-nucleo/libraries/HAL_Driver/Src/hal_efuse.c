/******************************************************************************
*@file  : hal_efuse.c
*@brief : EFUSE HAL module driver.
******************************************************************************/

#include "hal.h"

#ifdef HAL_EFUSE_MODULE_ENABLED

/******************************************************************************
*@brief : Initializes the EFUSEx peripheral according to the specified parameters
*         in the EFUSE_Init.
*@param : EFUSEx: where x can be (A..F) to select the EFUSE peripheral.
*@param : EFUSE_Init: pointer to an EFUSE_InitTypeDef structure that contains
*         the configuration information for the specified EFUSE peripheral.
*@return: None
******************************************************************************/
HAL_StatusTypeDef HAL_EFUSE_Init(EFUSE_TypeDef *EFUSEx)
{
    /* Check the parameters */
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));

    if(EFUSEx == EFUSE1)
    {
        __HAL_RCC_EFUSE1_CLK_ENABLE();

    }

    if ((RCC->RCHCR & (RCC_RCHCR_RCHEN | RCC_RCHCR_RCHRDY)) != (RCC_RCHCR_RCHEN | RCC_RCHCR_RCHRDY))
		return HAL_ERROR;

    EFUSEx->PGCFG = EFUSE_1US_CYCLES*EFUSE_PGT_US | (EFUSE_1US_CYCLES * EFUSE_PGWT_US << 8) | (EFUSE_1US_CYCLES * EFUSE_AVDD_HD_US << 16) | (EFUSE_1US_CYCLES * EFUSE_AVDD_SETUP_US << 24);  
	
	return HAL_OK;
}

/******************************************************************************
*@brief : De-initializes the EFUSEx peripheral registers to their default reset values.
*@param : EFUSEx: where x can be (A..F) to select the EFUSE peripheral.
*@param : EFUSE_Pin: specifies the port bit to be written.
*@return: None
******************************************************************************/
void HAL_EFUSE_DeInit(EFUSE_TypeDef *EFUSEx)
{

    /* Check the parameters */
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));

    if(EFUSEx == EFUSE1)
    {
        __HAL_RCC_EFUSE1_RESET();
        __HAL_RCC_EFUSE1_CLK_DISABLE();
    }


}


/******************************************************************************
*@brief : Write multiple data(in byte) of byteaddr.
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@param : byteaddr: specifies the byteaddr to be write.
*@param : data: specifies the data written to byteaddr.
*@param : timeout: specifies the time wait for write byte to complete
*@return: None
******************************************************************************/
HAL_StatusTypeDef HAL_EFUSE_WriteBytes(EFUSE_TypeDef *EFUSEx,uint16_t byteaddr,uint8_t* data,uint32_t len,uint32_t timeout)
{
    volatile uint32_t time_cnt, i;  

    /* Check the parameters */
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));
    assert_param(IS_EFUSE_BYTE_ADDR(byteaddr));

    if (RCC->RCHCR & RCC_RCHCR_RCHDIV)
        return HAL_ERROR;  
	
    if ((RCC->RCHCR & (RCC_RCHCR_RCHEN | RCC_RCHCR_RCHRDY)) != (RCC_RCHCR_RCHEN | RCC_RCHCR_RCHRDY))
		return HAL_ERROR;

    time_cnt = timeout;  
    
    EFUSEx->CTRL = EFUSE_WBYTE;

    for(i=0; i<len; i++)
    {
        EFUSEx->AR = byteaddr+i;

        EFUSEx->DWR = *(data+i);

        EFUSEx->CTRL |= EFUSE_CTRL_TRIG; 

        while( (!(EFUSEx->SR&EFUSE_SR_UNPG)) && (!(EFUSEx->SR&EFUSE_SR_DONE)) && (time_cnt--) );
        EFUSEx->CLR = EFUSE_CLR_CDONE|EFUSE_CLR_CUNPG;

        if(time_cnt == 0)
        {
            return HAL_TIMEOUT;
        }

        time_cnt = timeout;
    }

    return HAL_OK;

}


/******************************************************************************
*@brief : Read multiple data(in byte) of byteaddr.
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@param : byteaddr: specifies the byteaddr to be read.
*@param : data: the data read from byteaddr.
*@param : timeout: specifies the time wait for read byte to complete
*@return: None
******************************************************************************/
HAL_StatusTypeDef HAL_EFUSE_ReadBytes(EFUSE_TypeDef *EFUSEx,uint16_t byteaddr,uint8_t* data,uint32_t len,uint32_t timeout)
{
    volatile uint32_t time_cnt, i;  

    /* Check the parameters */
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));
    assert_param(IS_EFUSE_BYTE_ADDR(byteaddr));

    EFUSEx->CTRL = EFUSE_RBYTE;

    time_cnt = timeout;
    
    for(i=0; i<len; i++)
    {
        assert_param(IS_EFUSE_BYTE_ADDR(byteaddr+i));

        EFUSEx->AR = byteaddr+i;

        EFUSEx->CTRL |= EFUSE_CTRL_TRIG;

        while( (!(EFUSEx->SR&EFUSE_SR_DONE)) && (time_cnt--) );  

        if(time_cnt == 0)
        {
            return HAL_TIMEOUT;
        }

        *(data+i) = EFUSEx->DR;
        EFUSEx->CLR = EFUSE_CLR_CDONE;  
        time_cnt = timeout;  
    }

    return HAL_OK;

}

/******************************************************************************
*@brief : Config EFUSE write protection.
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@return: None
******************************************************************************/
void HAL_EFUSE_WpEnable(EFUSE_TypeDef *EFUSEx)
{
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));
    EFUSEx->WP = 0xEF59A6CB;

}

/******************************************************************************
*@brief : Config EFUSE read protection(0~15 byte).
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@return: None
******************************************************************************/
void HAL_EFUSE_RpEnable(EFUSE_TypeDef *EFUSEx)
{
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));
    EFUSEx->DSDP = 0xACF56B49;

}

/******************************************************************************
*@brief : Read data from data shadow register.
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@param : str_addr:where x can be (0..15), specifies the first data shadow register to be read.
*@param : len: specifies the number of data shadow register to be read.
*@param : data: the data read from of data shadow register.
*@return: None
******************************************************************************/
void HAL_EFUSE_ReadDsr(EFUSE_TypeDef *EFUSEx,uint8_t str_addr,uint8_t len,uint8_t* data)
{
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));

    if(str_addr+len >16)
        return;
    if(len==0)
        return;

    for(uint8_t i=0; i<len; i++)
    {
        *(data+i)=(EFUSEx->DSR[str_addr+i])&0xff;
    }

}

/******************************************************************************
*@brief : Config EFUSE byte write protection.
*@param : EFUSEx: where x can be (1..2) to select the EFUSE peripheral.
*@return: None
******************************************************************************/
void HAL_EFUSE_ByteWpEnable(EFUSE_TypeDef *EFUSEx)
{
    assert_param(IS_EFUSE_ALL_INSTANCE(EFUSEx));
    EFUSEx->BYTEWP = 0xC98E1A6D;

}


uint16_t HAL_EFUSE_GetCrc16(uint8_t *buffer, uint32_t len, uint16_t crc_init)
{
    uint32_t i, j;
    uint16_t crc_rst;      //reg for calculate CRC value
    uint16_t current;
	
    crc_rst = crc_init;  //initial value for CRC16-CCITT, GOOD_CRC = 0xf0b8
    for(i = 0; i < len; i++)
    {
        current = *buffer++;
        for(j = 0; j < 8; j++)
        {
            if((crc_rst ^ current) & 0x0001) crc_rst = (crc_rst >> 1) ^ 0x8408;  //CRC16-CCITT的生成多项式是	0x1021,将0x1021按位颠倒后为：0x8408
            else crc_rst >>= 1;
            current >>= 1;
        }
    }
    return crc_rst;
}

/******************************************************************************
*@brief : Get IC model.
*@param : icCode: save IC model.
*@return: HAL_OK,HAL_ERROR
******************************************************************************/
HAL_StatusTypeDef HAL_EFUSE_GetICModel(uint8_t icModel[12])
{
    uint8_t buff[12];
    int i;
    
    icModel[0]=0;
    if(HAL_EFUSE_ReadBytes(EFUSE1, 0x72, buff, 12, 10000) != HAL_OK)
    {
        return HAL_ERROR;
    }
    
    for(i=0; i<12; i+=4)
    {
        icModel[i+0] = buff[i+3];
        icModel[i+1] = buff[i+2];
        icModel[i+2] = buff[i+1];
        icModel[i+3] = buff[i+0]; 
    }
    
    return HAL_OK;
}


#endif /* HAL_EFUSE_MODULE_ENABLED */


