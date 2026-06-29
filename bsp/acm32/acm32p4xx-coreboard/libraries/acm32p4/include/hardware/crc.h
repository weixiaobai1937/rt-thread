/**
 * @file crc.h
 * @brief ACM32P4 CRC 计算单元驱动接口
 * 
 * @details
 * 本驱动支持 ACM32P4 芯片的 CRC（循环冗余校验）计算单元。
 * 
 * 特性：
 * - ✅ 4 层 API 架构（快速初始化 → 基础配置 → 高级功能 → 控制查询）
 * - ✅ 类型安全（枚举类型，无魔法数字）
 * - ✅ 支持多种 CRC 标准（CRC8/16/32）
 * - ✅ 可配置多项式、初始值、输入输出反转
 * 
 * 支持的 CRC 标准示例：
 * - CRC-32 (IEEE 802.3): 多项式 0x04C11DB7
 * - CRC-16 (CCITT): 多项式 0x1021
 * - CRC-8: 多项式 0x07
 * 
 * @example
 * @code
 * // 快速初始化（第 1 层 API）
 * crc_init_standard(CRC_STANDARD_CRC32);
 *
 * // 计算数据 CRC
 * uint32_t data = 0x12345678;
 * uint32_t result = crc_calculate(&data, 1);
 *
 * // 自定义配置（第 2 层 API）- 常规 CRC-32 参数
 * crc_config_t config = {
 *     .data_len = CRC_DATA_LEN_32,
 *     .poly_len = CRC_POLY_LEN_32,
 *     .poly = 0x04C11DB7,
 *     .init_value = 0xFFFFFFFF,
 *     .data_reverse = CRC_REVERSE_BYTE,
 *     .result_reverse = true,
 *     .initial_reverse = true,
 *     .outxor_reverse = true,
 *     .poly_reverse = true,
 *     .outxor = 0xFFFFFFFF
 * };
 * crc_configure(&config);
 * @endcode
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-03-03
 */

#ifndef ACM32P4_CRC_H
#define ACM32P4_CRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 1. 类型定义
//=============================================================================

/**
 * @brief CRC 数据长度
 * @note 对应 CTRL.DATA_LEN[2:1]：0=1字节，1=2字节，2=3字节，3=4字节
 */
typedef enum {
    CRC_DATA_LEN_8  = 0,   ///< 8 位数据（1 字节）
    CRC_DATA_LEN_16 = 1,   ///< 16 位数据（2 字节）
    CRC_DATA_LEN_24 = 2,   ///< 24 位数据（3 字节）
    CRC_DATA_LEN_32 = 3    ///< 32 位数据（4 字节）
} crc_data_len_t;

/**
 * @brief CRC 多项式长度
 * @note 对应 CTRL.PLOY_LEN[4:3]：0=32位，1=16位，2=8位，3=7位
 */
typedef enum {
    CRC_POLY_LEN_32 = 0,   ///< 32 位多项式
    CRC_POLY_LEN_16 = 1,   ///< 16 位多项式
    CRC_POLY_LEN_8  = 2,   ///< 8 位多项式
    CRC_POLY_LEN_7  = 3    ///< 7 位多项式
} crc_poly_len_t;

/**
 * @brief CRC 数据反转模式
 */
typedef enum {
    CRC_REVERSE_NONE   = 0,  ///< 不反转
    CRC_REVERSE_BYTE   = 1,  ///< 按字节反转
    CRC_REVERSE_HALF   = 2,  ///< 按半字反转
    CRC_REVERSE_WORD   = 3   ///< 按字反转
} crc_reverse_mode_t;

/**
 * @brief CRC 标准配置预设
 */
typedef enum {
    CRC_STANDARD_CRC8,       ///< CRC-8: 多项式 0x07, 初始值 0x00
    CRC_STANDARD_CRC8_ITU,   ///< CRC-8/ITU: 多项式 0x07, 初始值 0x00
    CRC_STANDARD_CRC16,      ///< CRC-16/IBM: 多项式 0x8005, 初始值 0x00
    CRC_STANDARD_CRC16_CCITT,///< CRC-16/CCITT: 多项式 0x1021, 初始值 0xFFFF
    CRC_STANDARD_CRC32,      ///< CRC-32/BZIP2: 多项式 0x04C11DB7, 初始值 0xFFFFFFFF ("123456789"->0xFC891918)
    CRC_STANDARD_CUSTOM      ///< 自定义配置
} crc_standard_t;

/**
 * @brief CRC 配置结构体
 * @note 倒序参数直接对应硬件：true/REVERSE_BYTE 等表示启用对应倒序，参考文档配置流程
 */
typedef struct {
    crc_data_len_t data_len;         ///< 数据长度
    crc_poly_len_t poly_len;         ///< 多项式长度
    uint32_t poly;                   ///< 多项式值
    uint32_t init_value;             ///< 初始值
    crc_reverse_mode_t data_reverse; ///< 数据反转模式
    bool result_reverse;             ///< 结果是否反转
    bool initial_reverse;            ///< 初始值是否反转
    bool outxor_reverse;             ///< 输出异或值是否反转
    bool poly_reverse;               ///< 多项式是否反转
    uint32_t outxor;                 ///< 输出异或值
} crc_config_t;

//=============================================================================
// 2. 第 1 层 API - 快速初始化
//=============================================================================

/**
 * @brief 快速初始化 CRC 为标准配置
 * 
 * @param standard CRC 标准配置
 * 
 * @note 自动使能 CRC 模块时钟
 * 
 * @code
 * crc_init_standard(CRC_STANDARD_CRC32);
 * @endcode
 */
void crc_init_standard(crc_standard_t standard);

//=============================================================================
// 3. 第 2 层 API - 基础配置
//=============================================================================

/**
 * @brief 配置 CRC 模块参数
 * 
 * @param config CRC 配置结构体指针
 * 
 * @return bool 配置是否成功
 *
 * @note 自动使能 CRC 模块时钟
 *
 * @code
 * crc_config_t config = {
 *     .data_len = CRC_DATA_LEN_32,
 *     .poly_len = CRC_POLY_LEN_32,
 *     .poly = 0x04C11DB7,
 *     .init_value = 0xFFFFFFFF,
 *     .data_reverse = CRC_REVERSE_BYTE,
 *     .result_reverse = true,
 *     .initial_reverse = true,
 *     .outxor_reverse = true,
 *     .poly_reverse = true,
 *     .outxor = 0xFFFFFFFF
 * };
 * crc_configure(&config);
 * @endcode
 */
bool crc_configure(const crc_config_t *config);

/**
 * @brief 设置 CRC 数据长度
 * 
 * @param len 数据长度
 * 
 * @code
 * crc_set_data_len(CRC_DATA_LEN_16);
 * @endcode
 */
void crc_set_data_len(crc_data_len_t len);

/**
 * @brief 设置 CRC 多项式
 * 
 * @param poly 多项式值
 * @param len 多项式长度
 * 
 * @code
 * crc_set_poly(0x04C11DB7, CRC_POLY_LEN_32);
 * @endcode
 */
void crc_set_poly(uint32_t poly, crc_poly_len_t len);

/**
 * @brief 设置 CRC 初始值
 * 
 * @param init_value 初始值
 * 
 * @code
 * crc_set_init_value(0xFFFFFFFF);
 * @endcode
 */
void crc_set_init_value(uint32_t init_value);

/**
 * @brief 设置 CRC 数据反转模式
 *
 * @param mode 数据反转模式 (0=不,1=字节,2=半字,3=字)
 *
 * @code
 * crc_set_data_reverse(CRC_REVERSE_BYTE);
 * @endcode
 */
void crc_set_data_reverse(crc_reverse_mode_t mode);

/**
 * @brief 设置 CRC 结果反转
 *
 * @param enable 是否反转结果
 *
 * @code
 * crc_set_result_reverse(true);
 * @endcode
 */
void crc_set_result_reverse(bool enable);

/**
 * @brief 设置 CRC 输出异或值
 * 
 * @param outxor 输出异或值
 * 
 * @code
 * crc_set_outxor(0xFFFFFFFF);
 * @endcode
 */
void crc_set_outxor(uint32_t outxor);

//=============================================================================
// 4. 第 3 层 API - 高级功能
//=============================================================================

/**
 * @brief 复位 CRC 模块
 * 
 * @note 复位后将 CRC_DATA 恢复为 CRC_INIT 寄存器中的值，不影响配置寄存器
 * 
 * @code
 * crc_reset();
 * @endcode
 */
void crc_reset(void);

/**
 * @brief 使能 CRC 模块
 * 
 * @code
 * crc_enable();
 * @endcode
 */
void crc_enable(void);

/**
 * @brief 禁用 CRC 模块
 * 
 * @code
 * crc_disable();
 * @endcode
 */
void crc_disable(void);

//=============================================================================
// 5. 第 4 层 API - 控制与查询
//=============================================================================

/**
 * @brief 计算数据的 CRC 值
 * 
 * @param data 数据缓冲区指针
 * @param count 数据个数（按配置的 data_len 为单位）
 * 
 * @return uint32_t CRC 计算结果
 * 
 * @note 计算前会自动复位 CRC 数据寄存器
 * 
 * @code
 * uint32_t data[] = {0x12345678, 0x9ABCDEF0};
 * uint32_t result = crc_calculate(data, 2);
 * @endcode
 */
uint32_t crc_calculate(const uint32_t *data, uint32_t count);

/**
 * @brief 计算 8 位数据的 CRC 值
 * 
 * @param data 8 位数据缓冲区指针
 * @param count 数据字节数
 * 
 * @return uint32_t CRC 计算结果
 * 
 * @code
 * uint8_t data[] = {0x12, 0x34, 0x56, 0x78};
 * uint32_t result = crc_calculate8(data, 4);
 * @endcode
 */
uint32_t crc_calculate8(const uint8_t *data, uint32_t count);

/**
 * @brief 计算 16 位数据的 CRC 值
 * 
 * @param data 16 位数据缓冲区指针
 * @param count 数据个数
 * 
 * @return uint32_t CRC 计算结果
 * 
 * @code
 * uint16_t data[] = {0x1234, 0x5678};
 * uint32_t result = crc_calculate16(data, 2);
 * @endcode
 */
uint32_t crc_calculate16(const uint16_t *data, uint32_t count);

/**
 * @brief 写入单个数据到 CRC 寄存器
 * 
 * @param data 数据值
 * 
 * @return uint32_t 写入后的 CRC 结果
 * 
 * @code
 * uint32_t result = crc_write_data(0x12345678);
 * @endcode
 */
uint32_t crc_write_data(uint32_t data);

/**
 * @brief 获取当前 CRC 结果
 * 
 * @return uint32_t CRC 结果
 * 
 * @code
 * uint32_t result = crc_get_result();
 * @endcode
 */
uint32_t crc_get_result(void);

/**
 * @brief 获取 CRC 模块是否已使能
 * 
 * @return bool true-已使能，false-未使能
 * 
 * @code
 * if (crc_is_enabled()) {
 *     // CRC 模块已使能
 * }
 * @endcode
 */
bool crc_is_enabled(void);

/**
 * @brief 获取 CRC 配置
 * 
 * @param config 配置结构体指针
 * 
 * @code
 * crc_config_t config;
 * crc_get_config(&config);
 * @endcode
 */
void crc_get_config(crc_config_t *config);

#ifdef __cplusplus
}
#endif

#endif /* ACM32P4_CRC_H */
