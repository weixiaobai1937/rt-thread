/**
 * @file i2s.h
 * @brief ACM32P4 I2S驱动 - 集成电路内置音频总线接口
 *
 * P4 新增外设，H5 没有 I2S。
 * I2S 用于数字音频设备之间的串行数据传输。
 */

#ifndef ACM32P4_I2S_H
#define ACM32P4_I2S_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 类型定义
//=============================================================================

/**
 * @brief I2S实例
 */
typedef uint32_t i2s_instance_t;

/**
 * @brief I2S工作模式
 */
typedef enum {
    I2S_MODE_SLAVE  = 0,  ///< 从模式
    I2S_MODE_MASTER = 1,  ///< 主模式
} i2s_mode_t;

/**
 * @brief I2S标准协议
 */
typedef enum {
    I2S_STD_PHILIPS  = 0,  ///< I2S Philips 标准
    I2S_STD_MSB      = 1,  ///< MSB 对齐标准
    I2S_STD_LSB      = 2,  ///< LSB 对齐标准
} i2s_std_t;

/**
 * @brief I2S数据宽度
 */
typedef enum {
    I2S_DATA_WIDTH_16  = 0,  ///< 16位数据
    I2S_DATA_WIDTH_24  = 1,  ///< 24位数据
    I2S_DATA_WIDTH_32  = 2,  ///< 32位数据
} i2s_data_width_t;

/**
 * @brief I2S配置结构体
 */
typedef struct {
    i2s_mode_t      mode;         ///< 主/从模式
    i2s_std_t       standard;     ///< 协议标准
    i2s_data_width_t data_width;  ///< 数据宽度
    uint32_t        sample_rate;  ///< 采样率（Hz）
    bool            tx_enable;    ///< 发送使能
    bool            rx_enable;    ///< 接收使能
} i2s_config_t;

//=============================================================================
// I2S实例编号（使用设备头文件中的宏定义）
//=============================================================================

//=============================================================================
// API函数声明
//=============================================================================

/**
 * @brief 初始化I2S
 * @param i2s I2S实例（I2S1, I2S2）
 * @param config 配置结构体
 * @return true: 成功, false: 失败
 */
bool i2s_init(i2s_instance_t i2s, const i2s_config_t *config);

/**
 * @brief 使能I2S
 * @param i2s I2S实例
 */
void i2s_enable(i2s_instance_t i2s);

/**
 * @brief 禁用I2S
 * @param i2s I2S实例
 */
void i2s_disable(i2s_instance_t i2s);

/**
 * @brief 发送数据（阻塞）
 * @param i2s I2S实例
 * @param data 待发送数据
 * @return true: 成功, false: 失败
 */
bool i2s_send(i2s_instance_t i2s, uint32_t data);

/**
 * @brief 接收数据（阻塞）
 * @param i2s I2S实例
 * @param data 接收数据指针
 * @return true: 成功, false: 失败
 */
bool i2s_receive(i2s_instance_t i2s, uint32_t *data);

/**
 * @brief 获取发送FIFO等级
 * @param i2s I2S实例
 * @return FIFO中剩余数据个数
 */
uint32_t i2s_get_tx_fifo_level(i2s_instance_t i2s);

/**
 * @brief 获取接收FIFO等级
 * @param i2s I2S实例
 * @return FIFO中待读取数据个数
 */
uint32_t i2s_get_rx_fifo_level(i2s_instance_t i2s);

/**
 * @brief 检查I2S是否忙
 * @param i2s I2S实例
 * @return true: 忙, false: 空闲
 */
bool i2s_is_busy(i2s_instance_t i2s);

#ifdef __cplusplus
}
#endif

#endif // ACM32P4_I2S_H