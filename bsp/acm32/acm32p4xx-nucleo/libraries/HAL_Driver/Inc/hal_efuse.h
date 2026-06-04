/******************************************************************************
*@file  : hal_efuse.h
*@brief : Header file of EFUSE HAL module.
******************************************************************************/

#ifndef __HAL_EFUSE_H__
#define __HAL_EFUSE_H__

#include "hal.h"

#define EFUSE_1US_CYCLES     8
#define EFUSE_PGT_US         4
#define EFUSE_PGWT_US        2
#define EFUSE_AVDD_HD_US     2
#define EFUSE_AVDD_SETUP_US  11  
#define EFUSE_USER_REGION_ADDR_START   0xB0   

/** @defgroup EFUSE FLAG
  * @{
  */
  
#define EFUSE_PREREAD_FLAG		EFUSE_SR_PREREAD_DONE
#define EFUSE_UNPG_FLAG			EFUSE_SR_UNPG
#define EFUSE_DONE_FLAG			EFUSE_SR_DONE

/**
  * @}
  */

/** @defgroup EFUSE MODE
  * @{
  */
  

#define EFUSE_RBYTE		(0U << EFUSE_CTRL_MODE_Pos)
#define EFUSE_WBYTE		(1U << EFUSE_CTRL_MODE_Pos)

/**
  * @}
  */


/** @defgroup  EFUSE Private Macros
  * @{
  */
  
#define IS_EFUSE_ALL_INSTANCE(__INSTANCE__)				((__INSTANCE__) == EFUSE1)

#define IS_EFUSE_BYTE_ADDR(__ADDR__)					((__ADDR__ )< 0x100)

												
										
/**
  * @}
  */


/* Exported functions --------------------------------------------------------*/

/* HAL_EFUSE_Init */
HAL_StatusTypeDef HAL_EFUSE_Init(EFUSE_TypeDef *EFUSEx);

/* HAL_EFUSE_DeInit */
void HAL_EFUSE_DeInit(EFUSE_TypeDef *EFUSEx);

HAL_StatusTypeDef HAL_EFUSE_ReadBytes(EFUSE_TypeDef *EFUSEx,uint16_t byteaddr,uint8_t* data,uint32_t len,uint32_t timeout);
HAL_StatusTypeDef HAL_EFUSE_WriteBytes(EFUSE_TypeDef *EFUSEx,uint16_t byteaddr,uint8_t* data,uint32_t len,uint32_t timeout);

void HAL_EFUSE_ReadDsr(EFUSE_TypeDef *EFUSEx,uint8_t str_addr,uint8_t len,uint8_t* data);
void HAL_EFUSE_RpEnable(EFUSE_TypeDef *EFUSEx);
void HAL_EFUSE_ByteWpEnable(EFUSE_TypeDef *EFUSEx);
void HAL_EFUSE_WpEnable(EFUSE_TypeDef *EFUSEx);

uint16_t HAL_EFUSE_GetCrc16(uint8_t *buffer, uint32_t len, uint16_t crc_init);
HAL_StatusTypeDef HAL_EFUSE_GetICCode(uint16_t *icCode);
HAL_StatusTypeDef HAL_EFUSE_GetICModel(uint8_t icModel[12]);

#endif
