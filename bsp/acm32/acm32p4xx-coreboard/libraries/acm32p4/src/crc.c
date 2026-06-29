/**
 * @file crc.c
 * @brief ACM32P4 CRC 计算单元驱动实现
 * 
 * @details
 * 本文件实现了 ACM32P4 芯片的 CRC（循环冗余校验）计算单元驱动。
 * 
 * 特性：
 * - 4 层 API 架构
 * - 类型安全
 * - 支持多种 CRC 标准
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-03
 */

#include "hardware/crc.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <string.h>

//===========================================
// 1. 寄存器位定义
//===========================================

// CTRL 寄存器位定义
#define CRC_CTRL_RST_Pos         (0U)
#define CRC_CTRL_RST_Msk         (0x1UL << CRC_CTRL_RST_Pos)
#define CRC_CTRL_DATA_LEN_Pos    (1U)
#define CRC_CTRL_DATA_LEN_Msk    (0x3UL << CRC_CTRL_DATA_LEN_Pos)
#define CRC_CTRL_POLY_LEN_Pos    (3U)
#define CRC_CTRL_POLY_LEN_Msk    (0x3UL << CRC_CTRL_POLY_LEN_Pos)
#define CRC_CTRL_DATA_REV_Pos    (5U)
#define CRC_CTRL_DATA_REV_Msk    (0x3UL << CRC_CTRL_DATA_REV_Pos)
#define CRC_CTRL_RSLT_REV_Pos    (7U)
#define CRC_CTRL_RSLT_REV_Msk    (0x1UL << CRC_CTRL_RSLT_REV_Pos)
#define CRC_CTRL_INITIAL_REV_Pos (8U)
#define CRC_CTRL_INITIAL_REV_Msk (0x1UL << CRC_CTRL_INITIAL_REV_Pos)
#define CRC_CTRL_OUTXOR_REV_Pos  (9U)
#define CRC_CTRL_OUTXOR_REV_Msk  (0x1UL << CRC_CTRL_OUTXOR_REV_Pos)
#define CRC_CTRL_POLY_REV_Pos    (10U)
#define CRC_CTRL_POLY_REV_Msk    (0x1UL << CRC_CTRL_POLY_REV_Pos)

//===========================================
// 2. 内部辅助函数
//===========================================

/**
 * @brief 使能 CRC 模块时钟
 */
static void crc_clock_enable(void)
{
    clock_periph_enable(CLK_CRC);
}

/**
 * @brief 获取 CRC 寄存器指针
 * 
 * @return CRC_TypeDef* CRC 寄存器基地址
 */
static CRC_TypeDef* crc_get_reg(void)
{
    return CRC;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

void crc_init_standard(crc_standard_t standard)
{
    crc_config_t config;
    memset(&config, 0, sizeof(config));
    
    assert(standard <= CRC_STANDARD_CUSTOM);
    
    crc_clock_enable();
    
    switch (standard) {
        case CRC_STANDARD_CRC8:
            config.data_len = CRC_DATA_LEN_8;
            config.poly_len = CRC_POLY_LEN_8;
            config.poly = 0x07;
            config.init_value = 0x00;
            config.data_reverse = CRC_REVERSE_NONE;
            config.result_reverse = false;
            config.initial_reverse = false;
            config.outxor_reverse = false;
            config.poly_reverse = false;
            config.outxor = 0x00;
            break;
            
        case CRC_STANDARD_CRC8_ITU:
            config.data_len = CRC_DATA_LEN_8;
            config.poly_len = CRC_POLY_LEN_8;
            config.poly = 0x07;
            config.init_value = 0x00;
            config.data_reverse = CRC_REVERSE_NONE;
            config.result_reverse = false;
            config.initial_reverse = false;
            config.outxor_reverse = false;
            config.poly_reverse = false;
            config.outxor = 0x00;
            break;
            
        case CRC_STANDARD_CRC16:
            config.data_len = CRC_DATA_LEN_16;
            config.poly_len = CRC_POLY_LEN_16;
            config.poly = 0x8005;
            config.init_value = 0x0000;
            config.data_reverse = CRC_REVERSE_BYTE;   // 常规 CRC-16/IBM：按字节反转
            config.result_reverse = false;
            config.initial_reverse = false;
            config.outxor_reverse = false;
            config.poly_reverse = false;
            config.outxor = 0x0000;
            break;
            
        case CRC_STANDARD_CRC16_CCITT:
            config.data_len = CRC_DATA_LEN_16;
            config.poly_len = CRC_POLY_LEN_16;
            config.poly = 0x1021;
            config.init_value = 0xFFFF;
            config.data_reverse = CRC_REVERSE_NONE;
            config.result_reverse = false;
            config.initial_reverse = false;
            config.outxor_reverse = false;
            config.poly_reverse = false;
            config.outxor = 0x0000;
            break;
            
        case CRC_STANDARD_CRC32:
            /* CRC-32/BZIP2：本硬件全倒序关闭，"123456789"->0xFC891918 */
            config.data_len = CRC_DATA_LEN_32;
            config.poly_len = CRC_POLY_LEN_32;
            config.poly = 0x04C11DB7;
            config.init_value = 0xFFFFFFFF;
            config.data_reverse = CRC_REVERSE_NONE;
            config.result_reverse = false;
            config.initial_reverse = false;
            config.outxor_reverse = false;
            config.poly_reverse = false;
            config.outxor = 0xFFFFFFFF;
            break;
            
        case CRC_STANDARD_CUSTOM:
        default:
            return;
    }
    
    crc_configure(&config);
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool crc_configure(const crc_config_t *config)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    if (config == NULL) {
        return false;
    }
    
    assert(config->data_len <= CRC_DATA_LEN_32);
    assert(config->poly_len <= CRC_POLY_LEN_7);
    assert(config->data_reverse <= CRC_REVERSE_WORD);
    
    crc_clock_enable();
    
    uint32_t ctrl = 0;
    
    ctrl |= (uint32_t)config->data_len << CRC_CTRL_DATA_LEN_Pos;
    ctrl |= (uint32_t)config->poly_len << CRC_CTRL_POLY_LEN_Pos;
    ctrl |= (uint32_t)config->data_reverse << CRC_CTRL_DATA_REV_Pos;
    if (config->result_reverse)  ctrl |= CRC_CTRL_RSLT_REV_Msk;
    if (config->initial_reverse) ctrl |= CRC_CTRL_INITIAL_REV_Msk;
    if (config->outxor_reverse)  ctrl |= CRC_CTRL_OUTXOR_REV_Msk;
    if (config->poly_reverse)    ctrl |= CRC_CTRL_POLY_REV_Msk;
    
    reg->CTRL = ctrl;  // 先写 CTRL
    
    // 2) 往多项式寄存器写入多项式
    reg->POLY = config->poly;
    
    // 3) 往初始值寄存器写入初始值
    reg->INIT = config->init_value;
    
    // 4) 往结果异或寄存器写入结果异或值
    reg->OUTXOR = config->outxor;
    
    return true;
}

void crc_set_data_len(crc_data_len_t len)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    assert(len <= CRC_DATA_LEN_32);
    
    crc_clock_enable();
    
    reg->CTRL = (reg->CTRL & ~CRC_CTRL_DATA_LEN_Msk) | 
                ((uint32_t)len << CRC_CTRL_DATA_LEN_Pos);
}

void crc_set_poly(uint32_t poly, crc_poly_len_t len)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    assert(len <= CRC_POLY_LEN_7);
    
    crc_clock_enable();
    
    reg->CTRL = (reg->CTRL & ~CRC_CTRL_POLY_LEN_Msk) | 
                ((uint32_t)len << CRC_CTRL_POLY_LEN_Pos);
    reg->POLY = poly;
}

void crc_set_init_value(uint32_t init_value)
{
    crc_clock_enable();
    crc_get_reg()->INIT = init_value;
}

void crc_set_data_reverse(crc_reverse_mode_t mode)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    assert(mode <= CRC_REVERSE_WORD);
    
    crc_clock_enable();
    
    reg->CTRL = (reg->CTRL & ~CRC_CTRL_DATA_REV_Msk) | 
                ((uint32_t)mode << CRC_CTRL_DATA_REV_Pos);
}

void crc_set_result_reverse(bool enable)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    crc_clock_enable();
    
    if (enable) {
        reg->CTRL |= CRC_CTRL_RSLT_REV_Msk;
    } else {
        reg->CTRL &= ~CRC_CTRL_RSLT_REV_Msk;
    }
}

void crc_set_outxor(uint32_t outxor)
{
    crc_clock_enable();
    crc_get_reg()->OUTXOR = outxor;
}

//===========================================
// 第3层：高级功能 API
//===========================================

void crc_reset(void)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    crc_clock_enable();
    
    reg->CTRL |= CRC_CTRL_RST_Msk;
    __DSB();
    reg->CTRL &= ~CRC_CTRL_RST_Msk;
}

void crc_enable(void)
{
    crc_clock_enable();
}

void crc_disable(void)
{
    clock_periph_disable(CLK_CRC);
}

//===========================================
// 第4层：控制与查询 API
//===========================================

uint32_t crc_calculate(const uint32_t *data, uint32_t count)
{
    CRC_TypeDef *reg = crc_get_reg();
    uint32_t i;
    
    assert(data != NULL);
    
    crc_reset();
    
    for (i = 0; i < count; i++) {
        reg->DATA = data[i];
    }
    
    return reg->DATA;
}

uint32_t crc_calculate8(const uint8_t *data, uint32_t count)
{
    CRC_TypeDef *reg = crc_get_reg();
    uint32_t i;
    
    assert(data != NULL);
    
    crc_data_len_t original_len = (crc_data_len_t)((reg->CTRL & CRC_CTRL_DATA_LEN_Msk) >> CRC_CTRL_DATA_LEN_Pos);
    
    crc_set_data_len(CRC_DATA_LEN_8);
    crc_reset();
    
    for (i = 0; i < count; i++) {
        reg->DATA = data[i];
    }
    
    crc_set_data_len(original_len);
    
    return reg->DATA;
}

uint32_t crc_calculate16(const uint16_t *data, uint32_t count)
{
    CRC_TypeDef *reg = crc_get_reg();
    uint32_t i;
    
    assert(data != NULL);
    
    crc_data_len_t original_len = (crc_data_len_t)((reg->CTRL & CRC_CTRL_DATA_LEN_Msk) >> CRC_CTRL_DATA_LEN_Pos);
    
    crc_set_data_len(CRC_DATA_LEN_16);
    crc_reset();
    
    for (i = 0; i < count; i++) {
        reg->DATA = data[i];
    }
    
    crc_set_data_len(original_len);
    
    return reg->DATA;
}

uint32_t crc_write_data(uint32_t data)
{
    CRC_TypeDef *reg = crc_get_reg();
    
    reg->DATA = data;
    
    return reg->DATA;
}

uint32_t crc_get_result(void)
{
    return crc_get_reg()->DATA;
}

bool crc_is_enabled(void)
{
    return clock_periph_is_enabled(CLK_CRC);
}

void crc_get_config(crc_config_t *config)
{
    CRC_TypeDef *reg = crc_get_reg();
    uint32_t ctrl;
    
    assert(config != NULL);
    
    ctrl = reg->CTRL;
    
    config->data_len = (crc_data_len_t)((ctrl & CRC_CTRL_DATA_LEN_Msk) >> CRC_CTRL_DATA_LEN_Pos);
    config->poly_len = (crc_poly_len_t)((ctrl & CRC_CTRL_POLY_LEN_Msk) >> CRC_CTRL_POLY_LEN_Pos);
    config->data_reverse = (crc_reverse_mode_t)((ctrl & CRC_CTRL_DATA_REV_Msk) >> CRC_CTRL_DATA_REV_Pos);
    config->result_reverse = (ctrl & CRC_CTRL_RSLT_REV_Msk) ? true : false;
    config->initial_reverse = (ctrl & CRC_CTRL_INITIAL_REV_Msk) ? true : false;
    config->outxor_reverse = (ctrl & CRC_CTRL_OUTXOR_REV_Msk) ? true : false;
    config->poly_reverse = (ctrl & CRC_CTRL_POLY_REV_Msk) ? true : false;
    config->poly = reg->POLY;
    config->init_value = reg->INIT;
    config->outxor = reg->OUTXOR;
}
