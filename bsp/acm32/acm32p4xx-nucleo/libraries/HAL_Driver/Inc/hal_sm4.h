/***********************************************************************
 * Copyright (c)  2008 - 2019, Shanghai AisinoChip Co.,Ltd .
 * All rights reserved.
 * Filename    : hal_sm4.h
 * Description : sm4 driver header file
 * Author(s)   : AisinoChip  
 * version     : V1.0
 * Modify date : 2016-03-24
 ***********************************************************************/
#ifndef _SM4_H_
#define _SM4_H_

#include "hal.h"

#define SM4_ECB_MODE        0
#define SM4_CBC_MODE        1
#define SM4_ENCRYPTION      1
#define SM4_DECRYPTION      0
#define SM4_SWAP_ENABLE     1
#define SM4_SWAP_DISABLE    0
#define SM4_NORMAL_MODE     0x12345678
#define SM4_SECURITY_MODE   0
#define SM4_FAIL            0x0
#define SM4_PASS            0xa59ada68


typedef struct {                                  
  UINT8 *sm4_crypt_msbox0;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox1;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox2;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox3;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox4;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox5;  //安全模式时，需要分配256字节空间。
	UINT8 *sm4_crypt_msbox6;  //安全模式时，需要分配256字节空间。	
	UINT8 *sm4_crypt_msbox7;  //安全模式时，需要分配256字节空间。
  unsigned int *sm4_mask;   //安全模式时，需要分配32字节空间。
} SM4_G_STR; 

/****************************************************************************** 
Name:        sm4_set_key
Function:    set sm4 key for encryption and decryption
Input:
             key     --    pointer to buffer of key, 4 words
             rk      --    pointer to buffer for key expend, 32 words              
Return:		 None
*******************************************************************************/
void sm4_set_key(UINT32 key[], UINT8 swap_en);   

/******************************************************************************
Name:        sm4_crypt
Function:    Function for des encryption and decryption
Input:
             p_sm4_str     --     pinter to buffer for SM4 internal use
             indata        --     pointer to buffer of input
             outdata       --     pointer to buffer of result
             block_len     --     block(128bit) length for sm4 cryption
             operation     --     SM4_ENCRYPTION,SM4_DECRYPTION
             mode          --     SM4_ECB_MODE, SM4_CBC_MODE,
             iv            --     initial vector for CBC mode
             security_mode --     SM4_NORMAL_MODE, SM4_SECURITY_MDOE
Return:		 SM4_FAIL(0x00) or SM4_PASS(0xa59ada68)
*******************************************************************************/
UINT32 sm4_crypt(
    SM4_G_STR* p_sm4_str,
    UINT32 *indata,
    UINT32 *outdata,
    UINT32 block_len,
    UINT8  operation,
    UINT8  mode,
    UINT32 *iv,
    UINT32 security_mode
);

#endif
/******************************************************************************
 * end of file
*******************************************************************************/
