/******************************************************************************
* File Name:			aes.c
* Author:				  AisinoChip
* Data First Issued:	2020-06-17
* Description:		aes module driver
******************************************************************************/
#include "hal.h"
#include "ic_w25qxx.h"
#include "hal_spi.h"

extern SPI_HandleTypeDef SPI_Handle_Nor;

//otfdec_en : 1: select PUF + RANDOM  3: UID+RANDOM
//write only once after sysreset
void otfdec_en(uint8_t otfdec_en, uint8_t spi_sel)
{
	AES_SPI1->OTFDEC_CTRL = otfdec_en;
    AES_SPI1->OTFDEC_SPI_CTRL = spi_sel;
}

void otfdec_encrypt1(uint32_t *datain, uint32_t start_addr, uint32_t length,uint32_t *dataout)
{	
    uint32_t i;
    uint32_t ctr_o[4];	
    uint32_t addr;
    uint8_t otfdec_en;

    otfdec_en = AES_SPI1->OTFDEC_CTRL;

    if(otfdec_en)
    {
        AES_SPI1->CTRL = 0x8000;//encrypt
        
        addr = start_addr;
        for( i = length; i > 0; i-- )
        {
            AES_SPI1->ADDR1 = addr;
            addr += 16;  
            AES_SPI1->CTRL |= 0x01;
            while(  (AES_SPI1->STATE & 0x01) == 0);
            AES_SPI1->STATE = 0x01;		

            ctr_o[3] = AES_SPI1->DATAOUT1;
            ctr_o[2] = AES_SPI1->DATAOUT1;
            ctr_o[1] = AES_SPI1->DATAOUT1;
            ctr_o[0] = AES_SPI1->DATAOUT1;		
            
            dataout[0] = ctr_o[0] ^ datain[0];
            dataout[1] = ctr_o[1] ^ datain[1];
            dataout[2] = ctr_o[2] ^ datain[2];
            dataout[3] = ctr_o[3] ^ datain[3];		

            datain  += 4;
            dataout += 4;					
        }
    }
	else
	{
		for( i = 0; i <(length<<2); i++)
		{
			dataout[i] = datain[i];
		}
	}
    
    AES_SPI1->CTRL &= ~0x8000;//encrypt
}

void otfdec_encrypt(uint32_t *datain, uint32_t start_addr, uint32_t length,uint32_t *dataout)
{	
    uint32_t i;
    uint32_t ctr_o[4];	
    uint32_t addr;
    uint8_t otfdec_en;

    otfdec_en = AES_SPI1->OTFDEC_CTRL;
    HAL_SimpleDelay(5000);
       
    if(otfdec_en)
    {
        AES_SPI1->CTRL = 0x000;//encrypt
        addr = start_addr;
        
        for( i = length; i > 0; i-- )
        {
            AES_SPI1->ADDR = addr; 
            addr += 16;  
            AES_SPI1->CTRL |= 0x01; 
            while(  (AES_SPI1->STATE & 0x01) == 0){;}
            AES_SPI1->STATE = 0x01;		

            ctr_o[0] = AES_SPI1->DATAOUT;
            ctr_o[1] = AES_SPI1->DATAOUT;
            ctr_o[2] = AES_SPI1->DATAOUT;
            ctr_o[3] = AES_SPI1->DATAOUT;					
            
            dataout[0] = ctr_o[0] ^ datain[0];
            dataout[1] = ctr_o[1] ^ datain[1];
            dataout[2] = ctr_o[2] ^ datain[2];
            dataout[3] = ctr_o[3] ^ datain[3];		
            
            datain  += 4;
            dataout += 4;			
        }
    }
	else
	{
		for( i = 0; i <(length<<2); i++)
		{
			dataout[i] = datain[i];
		}
	}
}

//write only once after sysreset
void otfdec_uid_set_key(uint32_t *uid, uint32_t *random)
{   
	AES_SPI1->UID = uid[0]; 
	AES_SPI1->UID = uid[1];
	AES_SPI1->UID = uid[2];
	AES_SPI1->UID = uid[3];

	AES_SPI1->RAND0 = random[0]; 
	AES_SPI1->RAND1 = random[1];
	AES_SPI1->ENDADDR = MEM_ADDR + FIRST_SIZE -1; //32k
}

void otfdec_set_encrypt_key1(uint32_t *key1)
{
	AES_SPI1->KEYIN1 = key1[0]; 
	AES_SPI1->KEYIN1 = key1[1];
	AES_SPI1->KEYIN1 = key1[2];
	AES_SPI1->KEYIN1 = key1[3];
}

void otfdec_reload_owner_key1(void)
{
	AES_SPI1->OTFDEC_SPI_CTRL |= 0x40;
}

void otfdec_uid_set_key2(uint32_t *key2, uint32_t *random)
{
	AES_SPI1->KEYIN2 = key2[0]; 
	AES_SPI1->KEYIN2 = key2[1];
	AES_SPI1->KEYIN2 = key2[2];
	AES_SPI1->KEYIN2 = key2[3];

	AES_SPI1->RAND2 = random[0]; 
	AES_SPI1->RAND3 = random[1];
	
	AES_SPI1->ENDADDR2 = MEM_ADDR + SECOND_SIZE -1; //128k
}

void otfdec_uid_set_key3(uint32_t *key3)
{
	AES_SPI1->KEYIN3 = key3[0]; 
	AES_SPI1->KEYIN3 = key3[1];
	AES_SPI1->KEYIN3 = key3[2];
	AES_SPI1->KEYIN3 = key3[3];
    
    AES_SPI1->ENDADDR3 = MEM_ADDR + SECOND_SIZE -1 + 0x1000; //4k
}

void otfdec_update_random23(uint32_t *random)
{
	uint32_t temp;
	AES_SPI1->OTFDEC_SPI_CTRL |= 0x20;
	temp = AES_SPI1->OTFDEC_SPI_CTRL;
	random[0] = AES_SPI1->RAND2;
	random[1] = AES_SPI1->RAND3;
}

