/**
 * @file crc_example.c
 * @brief ACM32P4 CRC 计算单元驱动示例
 *
 * @details
 * 演示 8 个场景：
 * 1. CRC-32 标准计算（基础使用）
 * 2. CRC-16/CCITT 计算
 * 3. CRC-8 计算
 * 4. 自定义 CRC 配置
 * 5. 8 位数据 CRC 计算
 * 6. 16 位数据 CRC 计算
 * 7. 逐字节计算 CRC
 * 8. CRC 配置查询与验证
 */

#include "acm32p4.h"
#include "hardware/system.h"
#include "hardware/crc.h"
#include "SEGGER_RTT.h"
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（修改此值来切换场景）
#define DEFAULT_SCENARIO 2

//===========================================
// 全局变量
//===========================================

// ============================================================================
// 工具函数
// ============================================================================

/* CRC-32/BZIP2 软件参考（"123456789" -> 0xFC891918），与硬件配置一致 */
static uint32_t crc32_bzip2_ref(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t poly = 0x04C11DB7U;
    uint32_t j;
    while (len--) {
        crc ^= (uint32_t)*data++ << 24;
        for (j = 0; j < 8; j++) {
            crc = (crc & 0x80000000U) ? (crc << 1) ^ poly : (crc << 1);
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/* CRC-16/CCITT-FALSE 软件参考（poly 0x1021, init 0xFFFF, 无反射） */
static uint32_t crc16_ccitt_ref(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFF;
    const uint32_t poly = 0x1021U;
    uint32_t j;
    while (len--) {
        crc ^= (uint32_t)*data++ << 8;
        for (j = 0; j < 8; j++) {
            crc = (crc & 0x8000U) ? (crc << 1) ^ poly : (crc << 1);
        }
    }
    return (crc & 0xFFFF);
}

/* CRC-8 软件参考（poly 0x07, init 0x00） */
static uint32_t crc8_ref(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0x00;
    const uint32_t poly = 0x07U;
    uint32_t j;
    while (len--) {
        crc ^= *data++;
        for (j = 0; j < 8; j++) {
            crc = (crc & 1U) ? (crc >> 1) ^ poly : (crc >> 1);
        }
    }
    return (crc & 0xFF);
}

/* CRC-16/IBM 软件参考（poly 0x8005 反射形式 0xA001, init 0, RefIn/RefOut） */
static uint32_t crc16_ibm_ref(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0x0000;
    const uint32_t poly = 0xA001U;
    uint32_t j;
    while (len--) {
        crc ^= *data++;
        for (j = 0; j < 8; j++) {
            crc = (crc & 1U) ? (crc >> 1) ^ poly : (crc >> 1);
        }
    }
    return (crc & 0xFFFF);
}

/* CRC-32/ISO-HDLC 软件参考（"123456789" -> 0xCBF43926），场景4自定义配置用 */
static uint32_t crc32_iso_ref(const uint8_t *data, uint32_t len)
{
    uint32_t crc = 0xFFFFFFFF;
    const uint32_t poly = 0xEDB88320U;
    uint32_t j;
    while (len--) {
        crc ^= *data++;
        for (j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (poly & (0U - (crc & 1U)));
        }
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief 打印分隔线
 */
void print_separator(void)
{
    printf("==========================================\n");
}

/**
 * @brief 打印 32 位数据
 * 
 * @param label 标签
 * @param value 值
 */
void print_hex32(const char *label, uint32_t value)
{
    printf("%s: 0x%08lX\n", label, value);
}

// ============================================================================
// 场景 1：CRC-32 标准计算（最基础场景）
// ============================================================================

void scenario_1_crc32_standard(void)
{
    uint32_t data[] = {0x12345678, 0x9ABCDEF0};
    uint32_t data_swapped[] = {0x78563412, 0xF0DEBC9A};  // 字节交换
    uint8_t test_data[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    uint32_t result;
    uint32_t result_swapped;
    uint32_t test_result;
    crc_config_t dbg_config;
    
    print_separator();
    printf("=== 场景 1：CRC-32/BZIP2 计算 ===\n\n");
    
    printf("步骤 1：初始化 CRC-32/BZIP2 配置\n");
    crc_init_standard(CRC_STANDARD_CRC32);
    
    // 读取并显示实际配置
    crc_get_config(&dbg_config);
    printf("  多项式：0x%08lX\n", dbg_config.poly);
    printf("  初始值：0x%08lX\n", dbg_config.init_value);
    printf("  输出异或：0x%08lX\n", dbg_config.outxor);
    printf("  数据反转：%d (0=不,1=字节,2=半字,3=字)\n", dbg_config.data_reverse);
    printf("  结果反转：%d\n", dbg_config.result_reverse);
    printf("  初始值反转：%d\n", dbg_config.initial_reverse);
    printf("  输出异或反转：%d\n", dbg_config.outxor_reverse);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算数据 CRC（原始数据）\n");
    printf("  数据：0x%08lX, 0x%08lX\n", data[0], data[1]);
    result = crc_calculate(data, 2);
    printf("  CRC-32 结果：0x%08lX\n", result);
    printf("  完成！\n\n");
    
    printf("步骤 3：计算数据 CRC（字节交换后）\n");
    printf("  数据：0x%08lX, 0x%08lX\n", data_swapped[0], data_swapped[1]);
    result_swapped = crc_calculate(data_swapped, 2);
    printf("  CRC-32 结果：0x%08lX\n", result_swapped);
    printf("  完成！\n\n");
    
    printf("步骤 4：验证标准测试数据 \"123456789\"（逐字节）\n");
    printf("  数据：\"123456789\"\n");
    test_result = crc_calculate8(test_data, 9);
    printf("  硬件 CRC-32 结果：0x%08lX （CRC-32/BZIP2 理论 0xFC891918）\n", test_result);
    printf("  软件 CRC-32/BZIP2：0x%08lX\n", crc32_bzip2_ref(test_data, 9));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 2：CRC-16/CCITT 计算
// ============================================================================

void scenario_2_crc16_ccitt(void)
{
    uint16_t data[] = {0x1234, 0x5678, 0x9ABC, 0xDEF0};
    uint32_t result;
    
    print_separator();
    printf("=== 场景 2：CRC-16/CCITT 计算 ===\n\n");
    
    printf("步骤 1：初始化 CRC-16/CCITT 标准配置\n");
    crc_init_standard(CRC_STANDARD_CRC16_CCITT);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算 16 位数据 CRC\n");
    printf("  数据：0x%04X, 0x%04X, 0x%04X, 0x%04X\n", 
           data[0], data[1], data[2], data[3]);
    result = crc_calculate16(data, 4);
    printf("  硬件 CRC-16/CCITT 结果：0x%04lX\n", result & 0xFFFF);
    printf("  软件参考：0x%04lX\n", crc16_ccitt_ref((const uint8_t *)data, 8));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 3：CRC-8 计算
// ============================================================================

void scenario_3_crc8(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t result;
    
    print_separator();
    printf("=== 场景 3：CRC-8 计算 ===\n\n");
    
    printf("步骤 1：初始化 CRC-8 标准配置\n");
    crc_init_standard(CRC_STANDARD_CRC8);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算 8 位数据 CRC\n");
    printf("  数据：0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X\n", 
           data[0], data[1], data[2], data[3], data[4]);
    result = crc_calculate8(data, 5);
    printf("  硬件 CRC-8 结果：0x%02lX\n", result & 0xFF);
    printf("  软件参考：0x%02lX\n", crc8_ref(data, 5));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 4：自定义 CRC 配置
// ============================================================================

void scenario_4_custom_crc(void)
{
    uint32_t data = 0x12345678;
    uint32_t result;
    crc_config_t config;
    memset(&config, 0, sizeof(config));
    
    print_separator();
    printf("=== 场景 4：自定义 CRC 配置 ===\n\n");
    
    printf("步骤 1：配置自定义 CRC 参数\n");
    config.data_len = CRC_DATA_LEN_32;
    config.poly_len = CRC_POLY_LEN_32;
    config.poly = 0x04C11DB7;        // CRC-32 多项式
    config.init_value = 0xFFFFFFFF;  // 初始值
    config.data_reverse = CRC_REVERSE_BYTE;  // 按字节倒序
    config.result_reverse = true;    // 结果倒序
    config.initial_reverse = true;  // 初始值倒序
    config.outxor_reverse = true;    // 输出异或值倒序
    config.poly_reverse = true;      // 多项式倒序
    config.outxor = 0xFFFFFFFF;      // 输出异或值
    
    crc_configure(&config);
    printf("  多项式：0x%08lX\n", config.poly);
    printf("  初始值：0x%08lX\n", config.init_value);
    printf("  输出异或值：0x%08lX\n", config.outxor);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算数据 CRC\n");
    printf("  数据：0x%08lX\n", data);
    result = crc_calculate(&data, 1);
    printf("  硬件 CRC 结果：0x%08lX\n", result);
    printf("  软件 CRC-32/ISO 参考：0x%08lX\n", crc32_iso_ref((const uint8_t *)&data, 4));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 5：8 位数据 CRC 计算
// ============================================================================

void scenario_5_crc8_byte_data(void)
{
    uint8_t message[] = "Hello, CRC!";
    uint32_t result;
    
    print_separator();
    printf("=== 场景 5:8 位数据 CRC 计算 ===\n\n");
    
    printf("步骤 1：初始化 CRC-8 配置\n");
    crc_init_standard(CRC_STANDARD_CRC8);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算字符串 CRC\n");
    printf("  字符串：\"%s\"\n", message);
    printf("  长度：%d 字节\n", (int)(sizeof(message) - 1));
    result = crc_calculate8(message, sizeof(message) - 1);
    printf("  硬件 CRC-8 结果：0x%02lX (%lu)\n", result & 0xFF, result & 0xFF);
    printf("  软件参考：0x%02lX\n", crc8_ref(message, sizeof(message) - 1));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 6：16 位数据 CRC 计算
// ============================================================================

void scenario_6_crc16_word_data(void)
{
    uint16_t data[] = {0x0001, 0x0002, 0x0003, 0x0004, 0x0005};
    uint32_t result;
    
    print_separator();
    printf("=== 场景 6:16 位数据 CRC 计算 ===\n\n");
    
    printf("步骤 1：初始化 CRC-16 配置\n");
    crc_init_standard(CRC_STANDARD_CRC16);
    printf("  完成！\n\n");
    
    printf("步骤 2：计算 16 位数据 CRC\n");
    printf("  数据：");
    for (int i = 0; i < 5; i++) {
        printf("0x%04X ", data[i]);
    }
    printf("\n");
    result = crc_calculate16(data, 5);
    printf("  硬件 CRC-16 结果：0x%04lX (%lu)\n", result & 0xFFFF, result & 0xFFFF);
    printf("  软件参考：0x%04lX\n", crc16_ibm_ref((const uint8_t *)data, 10));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 7：逐字节计算 CRC
// ============================================================================

void scenario_7_byte_by_byte_crc(void)
{
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t result;
    
    print_separator();
    printf("=== 场景 7：逐字节计算 CRC ===\n\n");
    
    printf("步骤 1：初始化 CRC-8 配置\n");
    crc_init_standard(CRC_STANDARD_CRC8);
    crc_reset();
    printf("  完成！\n\n");
    
    printf("步骤 2：逐字节写入数据\n");
    for (int i = 0; i < 5; i++) {
        result = crc_write_data(data[i]);
        printf("  写入 0x%02X, 中间结果：0x%08lX\n", data[i], result);
    }
    printf("  完成！\n\n");
    
    printf("步骤 3：获取最终 CRC 结果\n");
    result = crc_get_result();
    printf("  硬件最终 CRC: 0x%02lX\n", result & 0xFF);
    printf("  软件参考：0x%02lX\n", crc8_ref(data, 5));
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 场景 8：CRC 配置查询与验证
// ============================================================================

void scenario_8_config_query(void)
{
    crc_config_t config;
    crc_config_t readback_config;
    memset(&config, 0, sizeof(config));
    memset(&readback_config, 0, sizeof(readback_config));
    
    print_separator();
    printf("=== 场景 8:CRC 配置查询与验证 ===\n\n");
    
    printf("步骤 1：配置 CRC 参数\n");
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
    
    crc_configure(&config);
    printf("  配置完成！\n\n");
    
    printf("步骤 2：读取 CRC 配置\n");
    crc_get_config(&readback_config);
    printf("  数据长度：%d\n", readback_config.data_len);
    printf("  多项式长度：%d\n", readback_config.poly_len);
    printf("  多项式：0x%08lX\n", readback_config.poly);
    printf("  初始值：0x%08lX\n", readback_config.init_value);
    printf("  数据反转：%d\n", readback_config.data_reverse);
    printf("  结果反转：%d\n", readback_config.result_reverse);
    printf("  输出异或值：0x%08lX\n", readback_config.outxor);
    printf("  完成！\n\n");
    
    printf("步骤 3：验证配置一致性\n");
    bool match = true;
    if (config.data_len != readback_config.data_len) {
        printf("  数据长度不匹配！\n");
        match = false;
    }
    if (config.poly_len != readback_config.poly_len) {
        printf("  多项式长度不匹配！\n");
        match = false;
    }
    if (config.poly != readback_config.poly) {
        printf("  多项式不匹配！\n");
        match = false;
    }
    if (config.init_value != readback_config.init_value) {
        printf("  初始值不匹配！\n");
        match = false;
    }
    if (match) {
        printf("  配置验证通过！\n");
    }
    printf("  完成！\n\n");
    
    printf("步骤 4：检查 CRC 模块状态\n");
    if (crc_is_enabled()) {
        printf("  CRC 模块已使能\n");
    } else {
        printf("  CRC 模块未使能\n");
    }
    printf("  完成！\n\n");
    
    printf("完成！\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main(void)
{
    system_init();
    
    printf("\n");
    print_separator();
    printf("ACM32P4 CRC 计算单元示例\n");
    print_separator();
    printf("\n");
    
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_crc32_standard();
            break;
        case 2:
            scenario_2_crc16_ccitt();
            break;
        case 3:
            scenario_3_crc8();
            break;
        case 4:
            scenario_4_custom_crc();
            break;
        case 5:
            scenario_5_crc8_byte_data();
            break;
        case 6:
            scenario_6_crc16_word_data();
            break;
        case 7:
            scenario_7_byte_by_byte_crc();
            break;
        case 8:
            scenario_8_config_query();
            break;
        default:
            printf("无效的场景编号：%d\n", DEFAULT_SCENARIO);
            break;
    }
    
    while (1) {
        __WFI();
    }
}
