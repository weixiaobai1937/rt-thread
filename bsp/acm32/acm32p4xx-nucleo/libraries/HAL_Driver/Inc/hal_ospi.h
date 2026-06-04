/******************************************************************************
*@file  : hal_ospi.h
*@brief : Header file of OSPI HAL module.
*@ver   : 1.0.0
*@date  : 2023.06.16
******************************************************************************/

#ifndef __HAL_OSPI_H__
#define __HAL_OSPI_H__

#include  "hal.h" 


/** @defgroup OSPI State machine
 *  @{
 */
#define OSPI_RX_STATE_IDLE                          (0U)
#define OSPI_RX_STATE_RECEIVING                     (1U)
#define OSPI_TX_STATE_IDLE                          (0U)
#define OSPI_TX_STATE_SENDING                       (1U)
/**
  * @}
  */ 

/** @defgroup OSPI_Instances
  * @{
  */
#define IS_OSPI_ALL_INSTANCE(__INSTANCE__)          (((__INSTANCE__) == OSPI1) || \
                                                    ((__INSTANCE__) == OSPI2))  
/**
  * @}
  */   

/** @defgroup OSPI_WORK_MODE
  * @{
  */
#define OSPI_WORK_MODE_0                            (0U)
#define OSPI_WORK_MODE_1                            (1U)
#define OSPI_WORK_MODE_2                            (2U)
#define OSPI_WORK_MODE_3                            (3U)
#define IS_OSPI_WORK_MODE(__WORKMODE__)             (((__WORKMODE__) == OSPI_WORK_MODE_0)  || \
                                                    ((__WORKMODE__) == OSPI_WORK_MODE_1)   || \
                                                    ((__WORKMODE__) == OSPI_WORK_MODE_2)   || \
                                                    ((__WORKMODE__) == OSPI_WORK_MODE_3))
/**
  * @}
  */


/** @defgroup OSPI_X_MODE 
  * @{
  */
#define OSPI_1X_MODE                                (0U)
#define OSPI_2X_MODE                                (1U)
#define OSPI_4X_MODE                                (2U)
#define OSPI_8X_MODE                                (3U)
#define IS_OSPI_X_MODE(__XMODE__)                   (((__XMODE__) == OSPI_1X_MODE) || \
                                                    ((__XMODE__) == OSPI_2X_MODE)  || \
                                                    ((__XMODE__) == OSPI_4X_MODE)  || \
                                                    ((__XMODE__) == OSPI_8X_MODE))
/**
  * @}
  */


/** @defgroup OSPI_MSB_LSB_FIRST
  * @{
  */
#define OSPI_FIRSTBIT_MSB                           (0U)
#define OSPI_FIRSTBIT_LSB                           (1U)
#define IS_OSPI_FIRST_BIT(__FIRSTBIT__)             (((__FIRSTBIT__) == OSPI_FIRSTBIT_MSB) || \
                                                    ((__FIRSTBIT__) == OSPI_FIRSTBIT_LSB))
/**
  * @}
  */


/** @defgroup OSPI_BAUDRATE_PRESCALER
  * @{
  */
#define OSPI_BAUDRATE_PRESCALER_2                   (2U)  
#define OSPI_BAUDRATE_PRESCALER_4                   (4U)
#define OSPI_BAUDRATE_PRESCALER_8                   (8U)
#define OSPI_BAUDRATE_PRESCALER_14                  (14U)
#define OSPI_BAUDRATE_PRESCALER_16                  (16U)
#define OSPI_BAUDRATE_PRESCALER_28                  (28U)
#define OSPI_BAUDRATE_PRESCALER_32                  (32U)
#define OSPI_BAUDRATE_PRESCALER_64                  (64U)
#define OSPI_BAUDRATE_PRESCALER_128                 (128U)
#define OSPI_BAUDRATE_PRESCALER_254                 (254U)
#define IS_OSPI_BAUDRATE_PRESCALER(__BAUDRATE__)    (((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_2)  || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_4)   || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_8)   || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_16)  || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_28)  || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_32)  || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_64)  || \
                                                    ((__BAUDRATE__) == OSPI_BAUDRATE_PRESCALER_128) || \
                                                    ((__BAUDRATE__) <= OSPI_BAUDRATE_PRESCALER_254))
/**
  * @}
  */


/** @defgroup OSPI_MASTER_SAMPLE_SHIFT
  * @{
  */
#define OSPI_SAMPLE_SHIFT_NONE                      (0U)  
#define OSPI_SAMPLE_SHIFT_1HCLK                     (1U)
#define OSPI_SAMPLE_SHIFT_1_5HCLK                   (2U)
#define OSPI_SAMPLE_SHIFT_2HCLK                     (3U)
#define OSPI_SAMPLE_SHIFT_2_5HCLK                   (4U)
#define OSPI_SAMPLE_SHIFT_3HCLK                     (5U)
#define OSPI_SAMPLE_SHIFT_3_5HCLK                   (6U)
#define OSPI_SAMPLE_SHIFT_4HCLK                     (7U)
#define OSPI_SAMPLE_SHIFT_4_5HCLK                   (8U)
#define OSPI_SAMPLE_SHIFT_5HCLK                     (9U)
#define OSPI_SAMPLE_SHIFT_5_5HCLK                   (10U)
#define OSPI_SAMPLE_SHIFT_6HCLK                     (11U)
#define OSPI_SAMPLE_SHIFT_6_5HCLK                   (12U)
#define OSPI_SAMPLE_SHIFT_7HCLK                     (13U)
#define IS_OSPI_SAMPLE_SHIFT(__SHIFT__)             ((__SHIFT__) < 14)
/**
  * @}
  */
  
/** @defgroup OSPI_FIFO_Mode
  * @{
  */
#define OSPI_FIFO_BYTE                              (0U)
#define OSPI_FIFO_HALFWORD                          (1U)
#define OSPI_FIFO_WORD                              (2U)
#define IS_OSPI_FIFO_MODE(__FIFOMODE__)             (((__FIFOMODE__) == OSPI_FIFO_BYTE)     || \
                                                    ((__FIFOMODE__) == OSPI_FIFO_HALFWORD)  || \
                                                    ((__FIFOMODE__) == OSPI_FIFO_WORD))  
/**
  * @}
  */ 
  
/** @defgroup OSPI_CS
  * @{
  */
#define OSPI_CS_0                                   (1U)
#define OSPI_CS_1                                   (2U)
#define OSPI_CS_2                                   (4U)
#define IS_OSPI_CS_SEL(__CSX__)                     (((__CSX__) == OSPI_CS_0)  || \
                                                    ((__CSX__) == OSPI_CS_1)   || \
                                                    ((__CSX__) == OSPI_CS_2))  
/**
  * @}
  */ 
  
/** @defgroup OSPI_DTR_STR
  * @{
  */
#define OSPI_DTRM_STR                               (0U)
#define OSPI_DTRM_DTR                               (1U)
#define IS_OSPI_DTRM(__DTRM__)                      (((__DTRM__) == OSPI_DTRM_STR)  || \
                                                    ((__DTRM__) == OSPI_DTRM_DTR))  
/**
  * @}
  */   
  
/** @defgroup OSPI_DQS_Output
  * @{
  */
#define OSPI_DQSOE_DISABLE                          (0U)
#define OSPI_DQSOE_ENABLE                           (1U)
#define IS_OSPI_DQSOE(__DQSOE__)                    (((__DQSOE__) == OSPI_DQSOE_DISABLE)  || \
                                                    ((__DQSOE__) == OSPI_DQSOE_ENABLE))  
/**
  * @}
  */ 

/** @defgroup OSPI_Data_Mask_Mode
  * @{
  */
#define OSPI_DM_DISABLE                             (0U)
#define OSPI_DM_ENABLE                              (1U)
#define IS_OSPI_DM(__DM_EN__)                       (((__DM_EN__) == OSPI_DM_DISABLE)  || \
                                                    ((__DM_EN__) == OSPI_DM_ENABLE))  
/**
  * @}
  */  
  
/** @defgroup OSPI_DataMask_Ctrl
  * @{
  */
#define OSPI_DMCTRL_ODD_ADDR                        (0U)
#define OSPI_DMCTRL_EVEN_ADDR                       (1U)
#define IS_OSPI_DMCTRL_ADDR(__MASK__)               (((__MASK__) == OSPI_DMCTRL_ODD_ADDR) || \
                                                    ((__MASK__) == OSPI_DMCTRL_EVEN_ADDR))  
/**
  * @}
  */  

/** @defgroup OSPI_Memory_Type
  * @{
  */
#define OSPI_MEM_XCCELA_OPI                         (0U)
#define OSPI_MEM_APM_OPI                            (1U)
#define OSPI_MEM_HYPERBUS                           (2U)
#define OSPI_MEM_XSPI                               (3U)
#define IS_OSPI_MEMORY_TYPE(__TYPE__)               (((__TYPE__) == OSPI_MEM_XCCELA_OPI)    || \
                                                    ((__TYPE__) == OSPI_MEM_APM_OPI)        || \
                                                    ((__TYPE__) == OSPI_MEM_HYPERBUS)       || \
                                                    ((__TYPE__) == OSPI_MEM_XSPI))   
/**
  * @}
  */  

/** @defgroup OSPI_APM_Dummy_Clock
  * @{
  */
#define OSPI_APM_DUMMY_NONE                         (0U)
#define OSPI_APM_DUMMY_1CLOCK                       (1U)
#define OSPI_APM_DUMMY_2CLOCK                       (2U)
#define OSPI_APM_DUMMY_3CLOCK                       (3U)
#define OSPI_APM_DUMMY_4CLOCK                       (4U)
#define OSPI_APM_DUMMY_5CLOCK                       (5U)
#define OSPI_APM_DUMMY_6CLOCK                       (6U)
#define OSPI_APM_DUMMY_7CLOCK                       (7U)
#define IS_OSPI_APM_DUMMY_CLOCK(__CLOCK__)          (((__CLOCK__) == OSPI_APM_DUMMY_NONE    || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_1CLOCK)  || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_2CLOCK)  || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_3CLOCK)  || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_4CLOCK)  || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_5CLOCK)  || \
                                                    ((__CLOCK__) == OSPI_APM_DUMMY_6CLOCK)  || \
                                                    ((__CLOCK__) <= OSPI_APM_DUMMY_7CLOCK)))
/**
  * @}
  */ 
  

/** @defgroup OSPI_TX_OUT_Delay
  * @{
  */  
#define OSPI_TX_OUT_DELAY_NONE                      (0U)
#define OSPI_TX_OUT_DELAY_HALF_HCLK                 (1U)
#define OSPI_TX_OUT_DELAY_1HCLK                     (2U)
#define OSPI_TX_OUT_DELAY_2HCLK                     (3U)    
#define IS_OSPI_TX_OUT_DELAY(__DELAY__)             (((__DELAY__) == OSPI_TX_OUT_DELAY_NONE)        || \
                                                    ((__DELAY__) == OSPI_TX_OUT_DELAY_HALF_HCLK)    || \
                                                    ((__DELAY__) == OSPI_TX_OUT_DELAY_1HCLK)        || \
                                                    ((__DELAY__) == OSPI_TX_OUT_DELAY_2HCLK))   
/**
  * @}
  */ 
  
  /** @defgroup OSPI_DQS_Smaple
  * @{
  */  
#define OSPI_DQS_SAMPLE_DISABLE                     (0U)
#define OSPI_DQS_SAMPLE_ENABLE                      (1U) 
#define IS_OSPI_DQS_SAMPLE(__DQS_SAMP__)            (((__DQS_SAMP__) == OSPI_DQS_SAMPLE_DISABLE)    || \
                                                    ((__DQS_SAMP__) == OSPI_DQS_SAMPLE_ENABLE))   
/**
  * @}
  */ 
  

/** @defgroup OSPI_Dummy_Data
  * @{
  */                                             
#define IS_OSPI_DUMMY_DATA(__DUMMMY__)              ((__DUMMMY__) <= 0xff)
/**
  * @}
  */ 

/** @defgroup OSPI_CMD
  * @{
  */                                             
#define IS_OSPI_CMD(__CMD__)                        ((__CMD__) <= 0xffff)
/**
  * @}
  */ 
  
/** @defgroup OSPI_CSDelay_Val
  * @{
  */                                             
#define IS_OSPI_CS_DELAY_VAL(__VAL__)               ((__VAL__) <= 0xffff)
/**
  * @}
  */ 
  
/** @defgroup OSPI_CSTimeout_Val
  * @{
  */                                             
#define IS_OSPI_CS_TIMEOUT_VAL(__VAL__)             ((__VAL__) <= 0xffff)
/**
  * @}
  */ 
  
/** @defgroup OSPI_Burst_Type
  * @{
  */ 
#define MEMOACC1_BURST_WRAPPED                      (0U) 
#define MEMOACC1_BURST_LINEAR                       (1U) 
#define IS_OSPI_BURST_TYPE(__BURST__)               (((__BURST__) == MEMOACC1_BURST_WRAPPED)  || \
                                                    ((__BURST__) == MEMOACC1_BURST_LINEAR)) 
/**
  * @}
  */ 
  
/** @defgroup OSPI_Data_Mode
  * @{
  */ 
#define MEMOACC1_DATA_MODE_x1                       (0U)
#define MEMOACC1_DATA_MODE_x2                       (1U)
#define MEMOACC1_DATA_MODE_x4                       (2U)  
#define IS_OSPI_DATA_MODE(__DATAMODE__)             (((__DATAMODE__) == MEMOACC1_DATA_MODE_x1)  || \
                                                    ((__DATAMODE__) == MEMOACC1_DATA_MODE_x2)   || \
                                                    ((__DATAMODE__) == MEMOACC1_DATA_MODE_x4))  
/**
  * @}
  */ 

/** @defgroup OSPI_Alter_Byte_Mode
  * @{
  */   
#define MEMOACC1_ALTER_BYTE_MODE_x1	                (0U)
#define MEMOACC1_ALTER_BYTE_MODE_x2                 (1U)
#define MEMOACC1_ALTER_BYTE_MODE_x4                 (2U)
#define IS_OSPI_ALTER_BYTE_MODE(__ALTERMODE__)      (((__ALTERMODE__) == MEMOACC1_ALTER_BYTE_MODE_x1)  || \
                                                    ((__ALTERMODE__) == MEMOACC1_ALTER_BYTE_MODE_x2)   || \
                                                    ((__ALTERMODE__) == MEMOACC1_ALTER_BYTE_MODE_x4))  
/**
  * @}
  */ 

/** @defgroup OSPI_Addr_Mode
  * @{
  */  
#define MEMOACC1_ADDR_MODE_x1                       (0U)
#define MEMOACC1_ADDR_MODE_x2                       (1U)
#define MEMOACC1_ADDR_MODE_x4                       (2U)
#define IS_OSPI_ADDR_MODE(__ADDRMODE__)             (((__ADDRMODE__) == MEMOACC1_ADDR_MODE_x1)  || \
                                                    ((__ADDRMODE__) == MEMOACC1_ADDR_MODE_x2)   || \
                                                    ((__ADDRMODE__) == MEMOACC1_ADDR_MODE_x4))  
/**
  * @}
  */ 

/** @defgroup OSPI_Instr_Mode
  * @{
  */   
#define MEMOACC1_INSTR_MODE_x1                      (0U)
#define MEMOACC1_INSTR_MODE_x2                      (1U)
#define MEMOACC1_INSTR_MODE_x4                      (2U)
#define IS_OSPI_INSRT_MODE(__INSTRMODE__)           (((__INSTRMODE__) == MEMOACC1_INSTR_MODE_x1)  || \
                                                    ((__INSTRMODE__) == MEMOACC1_INSTR_MODE_x2)   || \
                                                    ((__INSTRMODE__) == MEMOACC1_INSTR_MODE_x4))  
/**
  * @}
  */ 

/** @defgroup OSPI_Addr_Width
  * @{
  */   
#define MEMOACC1_ADDR_WIDTH_8                       (0U)
#define MEMOACC1_ADDR_WIDTH_16                      (1U)
#define MEMOACC1_ADDR_WIDTH_24                      (2U)
#define MEMOACC1_ADDR_WIDTH_32                      (3U)
#define IS_OSPI_ADDR_WIDTH(__ADDRWIDTH__)           (((__ADDRWIDTH__) == MEMOACC1_ADDR_WIDTH_8)   || \
                                                    ((__ADDRWIDTH__) == MEMOACC1_ADDR_WIDTH_16)   || \
                                                    ((__ADDRWIDTH__) == MEMOACC1_ADDR_WIDTH_24)   || \
                                                    ((__ADDRWIDTH__) == MEMOACC1_ADDR_WIDTH_32)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Dummy_Cycle
  * @{
  */    
#define MEMOACC_DUMMY_CYCLE_1                       (0U)
#define MEMOACC_DUMMY_CYCLE_2                       (1U)
#define MEMOACC_DUMMY_CYCLE_3                       (2U)
#define MEMOACC_DUMMY_CYCLE_4                       (3U)
#define MEMOACC_DUMMY_CYCLE_5                       (4U)
#define MEMOACC_DUMMY_CYCLE_6                       (5U)
#define MEMOACC_DUMMY_CYCLE_7                       (6U)
#define MEMOACC_DUMMY_CYCLE_8                       (7U)
#define MEMOACC_DUMMY_CYCLE_9                       (8U)
#define MEMOACC_DUMMY_CYCLE_10                      (9U)
#define MEMOACC_DUMMY_CYCLE_11                      (10U)
#define MEMOACC_DUMMY_CYCLE_12                      (11U)
#define MEMOACC_DUMMY_CYCLE_13                      (12U)
#define MEMOACC_DUMMY_CYCLE_14                      (13U)
#define MEMOACC_DUMMY_CYCLE_15                      (14U)
#define MEMOACC_DUMMY_CYCLE_16                      (15U)
#define MEMOACC_DUMMY_CYCLE_17                      (16U)
#define MEMOACC_DUMMY_CYCLE_18                      (17U)
#define MEMOACC_DUMMY_CYCLE_19                      (18U)
#define MEMOACC_DUMMY_CYCLE_20                      (19U)
#define MEMOACC_DUMMY_CYCLE_21                      (20U)
#define MEMOACC_DUMMY_CYCLE_22                      (21U)
#define MEMOACC_DUMMY_CYCLE_23                      (22U)
#define MEMOACC_DUMMY_CYCLE_24                      (23U)
#define MEMOACC_DUMMY_CYCLE_25                      (24U)
#define MEMOACC_DUMMY_CYCLE_26                      (25U)
#define MEMOACC_DUMMY_CYCLE_27                      (26U)
#define MEMOACC_DUMMY_CYCLE_28                      (27U)
#define MEMOACC_DUMMY_CYCLE_29                      (28U)
#define MEMOACC_DUMMY_CYCLE_30                      (29U)
#define MEMOACC_DUMMY_CYCLE_31                      (30U)
#define MEMOACC_DUMMY_CYCLE_32                      (31U)
#define IS_OSPI_DUMMY_CYCLE(__CYCLE__)              ((__CYCLE__) < 32)
/**
  * @}
  */ 

/** @defgroup OSPI_Read_Dummy_State
  * @{
  */    
#define MEMOACC1_READ_DUMMY_DISABLE                 (0U)
#define MEMOACC1_READ_DUMMY_ENABLE                  (1U)
#define IS_OSPI_READ_DUMMY_STATE(__STATE__)         (((__STATE__) == MEMOACC1_READ_DUMMY_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_READ_DUMMY_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Write_Dummy_State
  * @{
  */    
#define MEMOACC1_WRITE_DUMMY_DISABLE                (0U)
#define MEMOACC1_WRITE_DUMMY_ENABLE                 (1U)
#define IS_OSPI_WRITE_DUMMY_STATE(__STATE__)        (((__STATE__) == MEMOACC1_WRITE_DUMMY_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_WRITE_DUMMY_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Alter_Byte_Size
  * @{
  */    
#define MEMOACC1_ALTER_BYTE_SIZE_8                  (0U)
#define MEMOACC1_ALTER_BYTE_SIZE_16                 (1U)
#define MEMOACC1_ALTER_BYTE_SIZE_24                 (2U)
#define MEMOACC1_ALTER_BYTE_SIZE_32                 (3U) 
#define IS_OSPI_ALTER_BYTE_SIZE(__ALTERSIZE__)      (((__ALTERSIZE__) == MEMOACC1_ALTER_BYTE_SIZE_8)    || \
                                                    ((__ALTERSIZE__) == MEMOACC1_ALTER_BYTE_SIZE_16)    || \
                                                    ((__ALTERSIZE__) == MEMOACC1_ALTER_BYTE_SIZE_24)    || \
                                                    ((__ALTERSIZE__) == MEMOACC1_ALTER_BYTE_SIZE_32))   
/**
  * @}
  */ 

/** @defgroup OSPI_Read_Alter_State
  * @{
  */  
#define MEMOACC1_READ_ALTER_DISABLE                 (0U)
#define MEMOACC1_READ_ALTER_ENABLE                  (1U)
#define IS_OSPI_READ_ALTER_STATE(__STATE__)         (((__STATE__) == MEMOACC1_READ_ALTER_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_READ_ALTER_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Write_Alte_State
  * @{
  */  
#define MEMOACC1_WRITE_ALTER_DISABLE                (0U)
#define MEMOACC1_WRITE_ALTER_ENABLE                 (1U)
#define IS_OSPI_WRITE_ALTER_STATE(__STATE__)        (((__STATE__) == MEMOACC1_WRITE_ALTER_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_WRITE_ALTER_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_CMD_Send_State
  * @{
  */   
#define MEMOACC1_CMD_SEND_ONCE_DISABLE              (0U)
#define MEMOACC1_CMD_SEND_ONCE_ENABLE               (1U)
#define IS_OSPI_CMD_SEND_STATE(__STATE__)           (((__STATE__) == MEMOACC1_CMD_SEND_ONCE_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_CMD_SEND_ONCE_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Continuous_State
  * @{
  */     
#define MEMOACC1_CON_DISABLE                        (0U)
#define MEMOACC1_CON_ENABLE                         (1U)
#define IS_OSPI_CONTINUOUS_STATE(__STATE__)         (((__STATE__) == MEMOACC1_CON_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_CON_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Cs_Timeout_State
  * @{
  */   
#define MEMOACC1_CS_TIMEOUT_DISABLE                 (0U)
#define MEMOACC1_CS_TIMEOUT_ENABLE                  (1U)
#define IS_OSPI_CS_TIMEOUT_STATE(__STATE__)         (((__STATE__) == MEMOACC1_CS_TIMEOUT_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_CS_TIMEOUT_ENABLE)) 
/**
  * @}
  */ 

/** @defgroup OSPI_Memory_State
  * @{
  */    
#define MEMOACC1_MEMORY_DISABLE                     (0U)
#define MEMOACC1_MEMORY_ENABLE                      (1U)
#define IS_OSPI_MEMORY_STATE(__STATE__)             (((__STATE__) == MEMOACC1_MEMORY_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC1_MEMORY_ENABLE)) 
/**
  * @}
  */

/** @defgroup OSPI_Dummyoff_State
  * @{
  */    
#define MEMOACC_DUMMY_OFF_DISABLE                   (0U)
#define MEMOACC_DUMMY_OFF_ENABLE                    (1U)
#define IS_OSPI_DUMMY_OFF_STATE(__STATE__)          (((__STATE__) == MEMOACC_DUMMY_OFF_DISABLE)  || \
                                                    ((__STATE__) == MEMOACC_DUMMY_OFF_ENABLE)) 
/**
  * @}
  */   

/** @defgroup OSPI_Wrap_Size
  * @{
  */ 
#define MEMOACC2_WRAP_SIZE_0                        (0U) 
#define MEMOACC2_WRAP_SIZE_16                       (2U)
#define MEMOACC2_WRAP_SIZE_32                       (3U)
#define MEMOACC2_WRAP_SIZE_64                       (4U)
#define IS_OSPI_WRAP_SIZE(__WRAPSIZE__)             (((__WRAPSIZE__) == MEMOACC2_WRAP_SIZE_0)    || \
                                                    ((__WRAPSIZE__) == MEMOACC2_WRAP_SIZE_16)    || \
                                                    ((__WRAPSIZE__) == MEMOACC2_WRAP_SIZE_32)    || \
                                                    ((__WRAPSIZE__) == MEMOACC2_WRAP_SIZE_64))     
/**
  * @}
  */ 
  
/** @defgroup OSPI_Burst_Length
  * @{
  */ 
#define MEMOACC2_BURST_LEN_0                        (0U)
#define MEMOACC2_BURST_LEN_16                       (1U)
#define MEMOACC2_BURST_LEN_32                       (2U)
#define MEMOACC2_BURST_LEN_64                       (3U)
#define MEMOACC2_BURST_LEN_128                      (4U)
#define MEMOACC2_BURST_LEN_256                      (5U)
#define MEMOACC2_BURST_LEN_512                      (6U)
#define MEMOACC2_BURST_LEN_1024                     (7U)
#define IS_OSPI_BURST_LEN(__LEN__)                  (((__LEN__) == MEMOACC2_BURST_LEN_0)        || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_16)        || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_32)        || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_64)        || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_128)       || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_256)       || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_512)       || \
                                                    ((__LEN__) == MEMOACC2_BURST_LEN_1024))   
/**
  * @}
  */ 

/** @defgroup OSPI_Latency_Cycles
  * @{
  */   
#define MEMOACC2_LATENCY_1                          (0U)
#define MEMOACC2_LATENCY_2                          (1U)
#define MEMOACC2_LATENCY_3                          (2U)
#define MEMOACC2_LATENCY_4                          (3U)
#define MEMOACC2_LATENCY_5                          (4U)
#define MEMOACC2_LATENCY_6                          (5U)
#define MEMOACC2_LATENCY_7                          (6U)
#define MEMOACC2_LATENCY_8                          (7U)
#define MEMOACC2_LATENCY_9                          (8U)
#define MEMOACC2_LATENCY_10                         (9U)
#define MEMOACC2_LATENCY_11                         (10U)
#define MEMOACC2_LATENCY_12                         (11U)
#define MEMOACC2_LATENCY_13                         (12U)
#define MEMOACC2_LATENCY_14                         (13U)
#define MEMOACC2_LATENCY_15                         (14U)
#define MEMOACC2_LATENCY_16                         (15U)
#define IS_OSPI_LATENCY_CYCLE(__CYCLE__)            ((__CYCLE__) < 16)
/**
  * @}
  */

/** @defgroup OSPI_Cmd_Len
  * @{
  */    
#define MEMOACC_CMD_LEN_1                           (0U)
#define MEMOACC_CMD_LEN_2                           (1U)
#define IS_OSPI_CMD_LEN(__LEN__)                    (((__LEN__) == MEMOACC_CMD_LEN_1)  || \
                                                    ((__LEN__) == MEMOACC_CMD_LEN_2)) 
/**
  * @}
  */

/** @defgroup OSPI_Cmd_Edge
  * @{
  */    
#define MEMOACC_CMD_SDR                             (0U)
#define MEMOACC_CMD_DDR                             (1U)
#define IS_OSPI_CMD_EDGE(__EDGE__)                  (((__EDGE__) == MEMOACC_CMD_SDR)  || \
                                                    ((__EDGE__) == MEMOACC_CMD_DDR)) 
/**
  * @}
  */  

  
/** @defgroup OSPI_DLYB Exported Constants
* @{
*/
#define OSPI_DLYB_MAX_UNIT                          ((uint32_t)0x0000000FU) /*!< Max UNIT value (15)  */ 
#define OSPI_DLYB_MAX_SELECT                        ((uint32_t)0x0000000CU) /*!< Max SELECT value (12) */  
#define OSPI_DLYB_FLAG_LENF                         (DLYB_CFGR_LENF)
#define OSPI_DLYB_LNG_10_0_MASK                     (0x07FF0000U)
#define OSPI_DLYB_LNG_11_10_MASK                    (0x0C000000U)
/**
  * @}
  */ 
  
/** @defgroup OSPI_DLYB_Instances
  * @{
  */
#define IS_OSPI_DLYB_ALL_INSTANCE(__INSTANCE__)     (((__INSTANCE__) == OSPI1_DLYB) || \
                                                    ((__INSTANCE__) == OSPI2IO0_DLYB) || \
                                                    ((__INSTANCE__) == OSPI2IO1_DLYB) || \
                                                    ((__INSTANCE__) == OSPI2IO2_DLYB) || \
                                                    ((__INSTANCE__) == OSPI2IO3_DLYB) || \
                                                    ((__INSTANCE__) == OSPI2_DLYB))  
/**
  * @}
  */  
  
/**
  * @brief  OSPI Configuration Structure definition
  */
typedef struct
{
    uint8_t WorkMode;                   /* OSPI working mode selection. 
                                            This parameter can be a value of @ref OSPI_WORK_MODE */
    
    uint8_t XMode;                      /* OSPI multi line mode selection. 
                                            This parameter can be a value of @ref OSPI_X_MODE */
    
    uint8_t FirstBit;                   /* MSB/LSB selects the bits first. 
                                            This parameter can be a value of @ref SPI_MSB_LSB_FIRST */
    
    uint16_t BaudRatePrescaler;         /* OSPI BaudRate Prescaler. 
                                            This parameter can be a value of @ref OSPI_BAUDRATE_PRESCALER */   
    
    uint8_t SampleShifting;             /* The master delay n hclk to sample data. 
                                            This parameter can be a value of @ref OSPI_MASTER_SAMPLE_SHIFT */                                                                                 

    uint8_t FWMode;                     /* FIFO Byte/Half Word/Word Write Mode Selection. 
                                            This parameter can be a value of @ref OSPI_FIFO_Mode */    
                                            
    uint8_t FRMode;                     /* FIFO Byte/Half Word/Word Read Mode Selection. 
                                            This parameter can be a value of @ref OSPI_FIFO_Mode */  

}OSPI_InitTypeDef;
/**
  * @}
  */ 
                                           
/**
  * @brief  OSPI handle Structure definition
  */
typedef struct
{
    OSPI_TypeDef        *Instance;      /* OSPI registers base address. 
                                            This parameter can be a value of @ref OSPI_Instances */

    OSPI_InitTypeDef    Init;           /* OSPI communication parameters */
    
    uint32_t            CSx;            /* OSPI chip selection signal. 
                                            This parameter can be a value of @ref OSPI_CS */
    
    uint32_t            RxState;        /* OSPI state machine */
    uint32_t            TxState;        /* OSPI state machine */
    
    uint8_t             *Rx_Buffer;     /* OSPI Rx Buffer */
    uint8_t             *Tx_Buffer;     /* OSPI Tx Buffer */
    
    uint32_t            Rx_Size;        /* OSPI Rx Size */
    uint32_t            Tx_Size;        /* OSPI Tx Size */
    
    uint32_t            Rx_Count;       /* OSPI RX Count */
    uint32_t            Tx_Count;       /* OSPI TX Count */
    uint32_t            Reset_Pin;   

#ifdef HAL_DMA_MODULE_ENABLED
    DMA_HandleTypeDef  *HDMA_Rx;       /* OSPI Rx DMA handle parameters */
    DMA_HandleTypeDef  *HDMA_Tx;       /* OSPI Tx DMA handle parameters */
#endif
    
}OSPI_HandleTypeDef;
/**
  * @}
  */ 
  
/**
  * @brief  OSPI Octal communication Configuration Structure definition
  */
typedef struct
{
    uint8_t DTRMode;                    /* Single/double transmission rate mode. 
                                            This parameter can be a value of @ref OSPI_DTR_STR*/
                                            
    uint8_t DQSMode;                    /* It enables or not the data strobe management. 
                                            This parameter can be a value of @ref OSPI_Data_Strobe*/                                        
     
    uint8_t MemoryType;                 /* It indicates the external device type connected to the OSPI. 
                                            This parameter can be a value of @ref OSPI_Memory_Type */

    uint8_t OutDelay;                   /* Output delay in DTR mode. 
                                            This parameter can be a value of @ref OSPI_TX_OUT_Delay */ 

    uint8_t DQSSample;                  /* It enables or not DQS is used as a clock for sampling. 
                                            This parameter can be a value of @ref OSPI_DQS_Smaple*/                                                                  
                                            

}OSPI_OctalInitTypeDef;
/**
  * @}
  */ 
  
/**
  * @brief  OSPI Memory Configuration Structure definition
  */ 
typedef struct
{   
    uint8_t HyperBurstType;             /* Hyperbus burst type 
                                           This parameter can be a value of @ref OSPI_Burst_Type */           
    uint8_t DataMode;                   /* Data mode 
                                           This parameter can be a value of @ref OSPI_Data_Mode */  
    uint8_t AlterByteMode;              /* Alter byte mode 
                                           This parameter can be a value of @ref OSPI_Alter_Byte_Mode */
    uint8_t AddrMode;                   /* Address mode 
                                           This parameter can be a value of @ref OSPI_Addr_Mode */
    uint8_t InstrMode;                  /* Instruction mode 
                                           This parameter can be a value of @ref OSPI_Instr_Mode */
    uint8_t AddrWidth;                  /* Address width 
                                           This parameter can be a value of @ref OSPI_Addr_Width */
    uint8_t ReadDummyCycleSize;         /* Read dummy cycle size 
                                           This parameter can be a value of @ref OSPI_Dummy_Cycle */            
    uint8_t ReadDummyByteEnable;        /* Read dummy byte enable or disable 
                                           This parameter can be a value of @ref OSPI_Read_Dummy_State */ 
    uint8_t WriteDummyByteEnable;       /* Write dummy byte enable or disable  
                                           This parameter can be a value of @ref OSPI_Write_Dummy_State */ 
    uint8_t AlterByteSize;              /* Alter byte size 
                                           This parameter can be a value of @ref OSPI_Alter_Byte_Size */ 
    uint8_t ReadAlterByteEnable;        /* Read alter byte enable or disable  
                                           This parameter can be a value of @ref OSPI_Read_Alter_State */           
    uint8_t WriteAlterByteEnable;       /* Write alter byte enable or disable  
                                           This parameter can be a value of @ref OSPI_Write_Alte_State */ 
    uint8_t SendInstrOnce;              /* Send command once or not 
                                           This parameter can be a value of @ref OSPI_CMD_Send_State */ 
    uint8_t ContinuousModeEnable;       /* Continuous mode enable or disable
                                           This parameter can be a value of @ref OSPI_Continuous_State */ 
    uint8_t CsTimeoutEnable;            /* Cs timeout enable or disable
                                           This parameter can be a value of @ref OSPI_Cs_Timeout_State */ 
    
    uint16_t WriteCmd;                  /* Memory write instruction. */     
    uint16_t ReadCmd;                   /* Memory read instruction. */

    uint32_t AlterByte;                 /* Alter byte */
    
    uint16_t CsTimeoutVal;              /* The time from CS lowering to forced raising */
    uint16_t CsDelayVal;                /* Time from CS pulling down to the first rising edge of CLK */                                                                                       
    
    uint8_t WriteDummyCycleSize;        /* Write dummy cycle size 
                                           This parameter can be a value of @ref OSPI_Dummy_Cycle */  
    uint8_t ReadDummyoffSize;           /* Closed reading dummy SCK output size 
                                           This parameter can be a value of @ref OSPI_Dummy_Cycle */ 
    uint8_t DummyoffEnable;             /* Enable or disable dummy SCK output size 
                                           This parameter can be a value of @ref OSPI_Dummyoff_State*/                                            
    uint8_t WriteBurstLen;              /* Write burst length
                                           This parameter can be a value of @ref OSPI_Burst_Length */
    uint8_t WrapSize;                   /* Wrap size
                                           This parameter can be a value of @ref OSPI_Wrap_Size */                                            
    uint8_t ReadBurstLen;               /* Read burst length
                                           This parameter can be a value of @ref OSPI_Burst_Length */                                             
    uint8_t HyperXspiLC1;               /* The number of Latency cycles with RWDS being one
                                           This parameter can be a value of @ref OSPI_Latency_Cycles */ 
    uint8_t HyperXspiLc0;               /* The number of Latency cycles with RWDS being zero
                                           This parameter can be a value of @ref OSPI_Latency_Cycles */ 

    uint8_t WriteDummyoffSize;          /* Close write operation with dummy cycle size 
                                           This parameter can be a value of @ref OSPI_Dummy_Cycle */    
    uint8_t CmdLength;                  /* CMD length, one byte or two bytes 
                                           This parameter can be a value of @ref OSPI_Cmd_Len*/                                            
    uint8_t CmdEdge;                    /* CMD sending mode, SDR or DTR 
                                           This parameter can be a value of @ref OSPI_Cmd_Edge*/  
                                           
}OSPI_MemoryInitTypeDef;

/**
  * @}
  */ 

/**
  * @brief  DLYB Configuration Structure definition
 */

typedef struct
{
    uint32_t Units;                 /*!< Specifies the Delay of a unit delay cell.
                                        This parameter can be a value between 0 and DLYB_MAX_UNIT               */

    uint32_t PhaseSel;              /*!< Specifies the Phase for the output clock.
                                        This parameter can be a value between 0 and DLYB_MAX_SELECT             */
}OSPI_DLYB_CfgTypeDef;  
/**
  * @}
  */ 

/* HAL_OSPI_Init */
HAL_StatusTypeDef HAL_OSPI_Init(OSPI_HandleTypeDef *hospi);

/* HAL_OSPI_DeInit */
HAL_StatusTypeDef HAL_OSPI_DeInit(OSPI_HandleTypeDef *hospi);

/* HAL_OSPI_OctalInit */
HAL_StatusTypeDef HAL_OSPI_OctalInit(OSPI_HandleTypeDef *hospi, OSPI_OctalInitTypeDef *Octal);

/* HAL_OSPI_MemoryInit */
HAL_StatusTypeDef HAL_OSPI_MemoryInit(OSPI_HandleTypeDef *hospi, OSPI_MemoryInitTypeDef *Memory);

/* HAL_OSPI_CS_Select */
void HAL_OSPI_CS_Select(OSPI_HandleTypeDef *hospi);

/* HAL_OSPI_CS_Release */
void HAL_OSPI_CS_Release(OSPI_HandleTypeDef *hospi);

/* HAL_OSPI_Transmit */
HAL_StatusTypeDef HAL_OSPI_Transmit(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout);

/* HAL_OSPI_TransmitKeepCS */                                        
HAL_StatusTypeDef HAL_OSPI_TransmitKeepCS(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout);                                        

/* HAL_OSPI_Receive */
HAL_StatusTypeDef HAL_OSPI_Receive(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout);
                                        
/* HAL_OSPI_ReceiveKeepCS */
HAL_StatusTypeDef HAL_OSPI_ReceiveKeepCS(OSPI_HandleTypeDef *hospi, uint8_t *pData, \
                                        uint32_t Size, uint32_t Timeout);                                        

/* HAL_OSPI_Transmit_IT */
HAL_StatusTypeDef HAL_OSPI_Transmit_IT(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size);

/* HAL_OSPI_Receive_IT */
HAL_StatusTypeDef HAL_OSPI_Receive_IT(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size);

/* HAL_OSPI_Transmit_DMA */
HAL_StatusTypeDef HAL_OSPI_Transmit_DMA(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size);

/* HAL_OSPI_Receive_DMA */
HAL_StatusTypeDef HAL_OSPI_Receive_DMA(OSPI_HandleTypeDef *hospi, uint8_t *pData, uint32_t Size);

/* HAL_OSPI_SimultTransmitReceive */
HAL_StatusTypeDef HAL_OSPI_SimultTransmitReceive(OSPI_HandleTypeDef *hospi, uint8_t *pTxData, \
                                                    uint8_t *pRxData, uint32_t Size, uint32_t Timeout);


/* HAL_OSPI_TransmitByHalfWord */
HAL_StatusTypeDef HAL_OSPI_TransmitByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pData, \
                                                    uint32_t Size, uint32_t Timeout);

/* HAL_OSPI_TransmitByWord */
HAL_StatusTypeDef HAL_OSPI_TransmitByWord(OSPI_HandleTypeDef *hospi, uint32_t *pData, \
                                                uint32_t Size, uint32_t Timeout);


/* HAL_OSPI_ReceiveByHalfWord */
HAL_StatusTypeDef HAL_OSPI_ReceiveByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pData, \
                                                    uint32_t Size, uint32_t Timeout);

/* HAL_OSPI_ReceiveByWord */
HAL_StatusTypeDef HAL_OSPI_ReceiveByWord(OSPI_HandleTypeDef *hospi, uint32_t *pData, \
                                                uint32_t Size, uint32_t Timeout);

/* HAL_OSPI_Transmit_Recieve_ByByte */  
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByByte(OSPI_HandleTypeDef *hospi, uint8_t *pTxData, uint32_t TXSize, \
                            uint8_t *pRxData, uint32_t RXSize, uint32_t Timeout);
                            
/* HAL_OSPI_Transmit_Recieve_ByHalfWord */
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByHalfWord(OSPI_HandleTypeDef *hospi, uint16_t *pTxData, uint32_t TXSize, \
                        uint16_t *pRxData, uint32_t RXSize, uint32_t Timeout);

/* HAL_OSPI_Transmit_Recieve_ByWord */
HAL_StatusTypeDef HAL_OSPI_Transmit_Recieve_ByWord(OSPI_HandleTypeDef *hospi, uint32_t *pTxData, uint32_t TXSize, \
                                                        uint32_t *pRxData, uint32_t RXSize, uint32_t Timeout);

/* HAL_OSPI_GetTxState */
uint8_t HAL_OSPI_GetTxState(OSPI_HandleTypeDef *hospi);
/* HAL_OSPI_GetRxState */
uint8_t HAL_OSPI_GetRxState(OSPI_HandleTypeDef *hospi);

/* HAL_OSPI_WireConfig */
HAL_StatusTypeDef HAL_OSPI_WireConfig(OSPI_HandleTypeDef *hospi, uint32_t X_Mode);

/* HAL_OSPI_TransmitRateMode */
HAL_StatusTypeDef HAL_OSPI_TransmitRateMode(OSPI_HandleTypeDef *hospi, uint32_t mode);

/* HAL_OSPI_FifoWriteMode */
HAL_StatusTypeDef HAL_OSPI_FifoWriteMode(OSPI_HandleTypeDef *hospi, uint32_t mode);

/* HAL_OSPI_FifoReadMode */
HAL_StatusTypeDef HAL_OSPI_FifoReadMode(OSPI_HandleTypeDef *hospi, uint32_t mode);

/* HAL_OSPI_APMDummyClock */
HAL_StatusTypeDef HAL_OSPI_APMDummyClock(OSPI_HandleTypeDef *hospi, uint32_t APMClock);

/* HAL_OSPI_TxOutDelay */
HAL_StatusTypeDef HAL_OSPI_TxOutDelay(OSPI_HandleTypeDef *hospi, uint32_t Outdelay);

/* HAL_OSPI_RxSampleDelay */
HAL_StatusTypeDef HAL_OSPI_RxSampleDelay(OSPI_HandleTypeDef *hospi, uint32_t Sampledelay);

/* HAL_OSPI_MemoryEnableDisable */
HAL_StatusTypeDef HAL_OSPI_MemoryEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state);

/* HAL_OSPI_DQSOutputEnableDisable */
HAL_StatusTypeDef HAL_OSPI_DQSOutputEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state);

/* HAL_OSPI_DQSSampleEnableDisable */
HAL_StatusTypeDef HAL_OSPI_DQSSampleEnableDisable(OSPI_HandleTypeDef *hospi, uint32_t state);

/* HAL_OSPI_DataMaskConfig */
HAL_StatusTypeDef HAL_OSPI_DataMaskConfig(OSPI_HandleTypeDef *hospi, uint32_t DataMaskMode, uint32_t DataMaskCtrl);

/* HAL_OSPI_DummyCyclewitchCloseClk */
HAL_StatusTypeDef HAL_OSPI_DummyCyclewitchCloseClk(OSPI_HandleTypeDef *hospi, FunctionalState state);

/* HAL_OSPI_CSTimeoutPullup */
HAL_StatusTypeDef HAL_OSPI_CSTimeoutPullup(OSPI_HandleTypeDef *hospi, uint32_t state, uint32_t Timeout);

/* HAL_OSPI_CSPulldownToClkRisingEdge */
HAL_StatusTypeDef HAL_OSPI_CSPulldownToClkRisingEdge(OSPI_HandleTypeDef *hospi, uint32_t delay);

void HAL_OSPI_Updata_RX_Shift_Clk(OSPI_TypeDef *Instance, uint32_t shift_clk);  

#endif 
















