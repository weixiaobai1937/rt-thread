/**
 * @file i2s.h
 * @brief ACM32P4 I2S 驱动头文件 — 片上音频接口
 * @author ACM32P4 SDK Team
 * @version 1.0
 *
 * @details
 * I2S 用于数字音频设备之间的串行数据传输，支持：
 * - 4 种音频标准：飞利浦、MSB 对齐、LSB 对齐、PCM
 * - 主/从模式 + 全双工
 * - 数据宽度 16/24/32 位，通道长度 16/32 位
 * - 9 位可编程预分频器
 * - 主时钟输出（MCK）
 * - DMA 支持
 *
 * @note P4 新增外设（H5 无 I2S），I2S1 基地址 0x40003400，I2S2 基地址 0x40003800
 */

#ifndef _HARDWARE_I2S_H
#define _HARDWARE_I2S_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define I2S_DEFAULT_TIMEOUT_MS       100U

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief I2S 外设实例
 */
typedef enum {
    I2S_1     = 0,  ///< I2S1
    I2S_2     = 1,  ///< I2S2
    I2S_COUNT = 2,
} i2s_instance_t;

/**
 * @brief I2S 主从模式
 */
typedef enum {
    I2S_MODE_SLAVE  = 0,  ///< 从模式
    I2S_MODE_MASTER = 1,  ///< 主模式
} i2s_mode_t;

/**
 * @brief I2S 音频标准
 *
 * @note 对应 CR.STD[5:4]
 */
typedef enum {
    I2S_STD_PHILIPS  = 0,  ///< I2S 飞利浦标准
    I2S_STD_MSB      = 1,  ///< MSB 对齐标准
    I2S_STD_LSB      = 2,  ///< LSB 对齐标准
    I2S_STD_PCM      = 3,  ///< PCM 标准
} i2s_std_t;

/**
 * @brief I2S 数据宽度
 *
 * @note 对应 CR.DTLEN[2:1]
 */
typedef enum {
    I2S_DATA_WIDTH_16  = 0,  ///< 16 位数据
    I2S_DATA_WIDTH_24  = 1,  ///< 24 位数据
    I2S_DATA_WIDTH_32  = 2,  ///< 32 位数据
} i2s_data_width_t;

/**
 * @brief I2S 通道长度
 *
 * @note 对应 CR.CHLEN[0]
 */
typedef enum {
    I2S_CHLEN_16 = 0,  ///< 16 位通道
    I2S_CHLEN_32 = 1,  ///< 32 位通道
} i2s_chlen_t;

/**
 * @brief I2S 空闲时钟极性
 *
 * @note 对应 CR.CKPL[3]
 */
typedef enum {
    I2S_CKPL_LOW  = 0,  ///< I2S_CK 空闲状态为低电平
    I2S_CKPL_HIGH = 1,  ///< I2S_CK 空闲状态为高电平
} i2s_ckpl_t;

/**
 * @brief PCM 帧同步模式
 *
 * @note 对应 CR.PCMMODE[7]，仅在 PCM 标准下有效
 */
typedef enum {
    I2S_PCM_SHORT_FRAME = 0,  ///< 短帧同步（WS 保持 1 位数据长度）
    I2S_PCM_LONG_FRAME  = 1,  ///< 长帧同步（WS 保持 13 位数据长度）
} i2s_pcm_mode_t;

/**
 * @brief I2S 运行方向
 */
typedef enum {
    I2S_DIR_TX     = 0,  ///< 仅发送
    I2S_DIR_RX     = 1,  ///< 仅接收
    I2S_DIR_FULL   = 2,  ///< 全双工
} i2s_direction_t;

/**
 * @brief I2S 中断类型（IER 寄存器位掩码）
 */
typedef enum {
    I2S_IRQ_RXNE   = (1U << 0),   ///< 接收缓冲区非空中断
    I2S_IRQ_TXE    = (1U << 1),   ///< 发送缓冲区空中断
    I2S_IRQ_ERROR  = (1U << 5),   ///< 错误中断（FE/OVR/UDR）
    I2S_IRQ_MSUSP  = (1U << 9),   ///< 主机发送挂起中断
    I2S_IRQ_SVTC   = (1U << 10),  ///< 从机有效数据发送完成中断
    I2S_IRQ_ALL    = (1U << 0) | (1U << 1) | (1U << 5) | (1U << 9) | (1U << 10),
} i2s_irq_t;

/**
 * @brief I2S 状态标志（SR 寄存器位掩码）
 */
typedef enum {
    I2S_FLAG_RXNE   = (1U << 0),   ///< 接收缓冲区非空
    I2S_FLAG_TXE    = (1U << 1),   ///< 发送缓冲区空
    I2S_FLAG_CH     = (1U << 2),   ///< I2S 通道标志（0=左, 1=右）
    I2S_FLAG_UDR    = (1U << 3),   ///< 发送欠载错误
    I2S_FLAG_OVR    = (1U << 4),   ///< 接收过载错误
    I2S_FLAG_FE     = (1U << 5),   ///< 帧错误
    I2S_FLAG_MSUSP  = (1U << 9),   ///< 主机发送挂起
    I2S_FLAG_SVTC   = (1U << 10),  ///< 从机有效数据发送完成
} i2s_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief I2S 基础配置结构体
 */
typedef struct {
    i2s_mode_t       mode;         ///< 主/从模式
    i2s_std_t        standard;     ///< 协议标准
    i2s_data_width_t data_width;   ///< 数据宽度
    i2s_chlen_t      chlen;        ///< 通道长度（必须 ≥ 数据宽度）
    i2s_ckpl_t       ckpl;         ///< 空闲时钟极性
    i2s_pcm_mode_t   pcm_mode;     ///< PCM 帧同步模式（仅 PCM 标准有效）
    bool             ioswp;        ///< IO 交换（交换 MISO/MOSI 引脚功能）
    i2s_direction_t  direction;    ///< 运行方向（仅发送/仅接收/全双工）
} i2s_config_t;

/**
 * @brief I2S 时钟预分频配置结构体
 *
 * @note 分频公式请参考用户手册表 28-1 / 28-2
 */
typedef struct {
    uint16_t div;      ///< 分频系数（1-511），实际分频 = DIV*2 + OF
    bool     of;       ///< 奇系数（0=偶数, 1=奇数）
    bool     mckoe;    ///< 主时钟输出使能
} i2s_clock_config_t;

//===========================================
// 回调类型定义
//===========================================

/**
 * @brief I2S 中断回调函数类型
 *
 * @param[in] i2s   I2S 实例
 * @param[in] flags 触发中断的状态标志位掩码
 */
typedef void (*i2s_callback_t)(i2s_instance_t i2s, uint32_t flags);

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 I2S 主机发送模式
 *
 * 默认配置：飞利浦标准、16 位数据、16 位通道、CK 低电平空闲。
 * 时钟频率自动分频到不高于 sample_rate 的最近值。
 *
 * @param[in] i2s         I2S 实例
 * @param[in] sample_rate 目标音频采样率（Hz），如 44100, 48000
 * @return true: 初始化成功, false: 参数非法
 *
 * @note 覆盖寄存器：CR, PR
 *
 * @code
 * i2s_init_master_tx(I2S_1, 48000);
 * @endcode
 */
bool i2s_init_master_tx(i2s_instance_t i2s, uint32_t sample_rate);

/**
 * @brief 快速初始化 I2S 主机接收模式
 *
 * @param[in] i2s         I2S 实例
 * @param[in] sample_rate 目标音频采样率（Hz）
 * @return true: 初始化成功, false: 参数非法
 *
 * @note 覆盖寄存器：CR, PR
 */
bool i2s_init_master_rx(i2s_instance_t i2s, uint32_t sample_rate);

/**
 * @brief 快速初始化 I2S 从机发送模式
 *
 * @param[in] i2s  I2S 实例
 * @return true: 初始化成功, false: 参数非法
 *
 * @note 覆盖寄存器：CR
 */
bool i2s_init_slave_tx(i2s_instance_t i2s);

/**
 * @brief 快速初始化 I2S 从机接收模式
 *
 * @param[in] i2s  I2S 实例
 * @return true: 初始化成功, false: 参数非法
 *
 * @note 覆盖寄存器：CR
 */
bool i2s_init_slave_rx(i2s_instance_t i2s);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置 I2S 工作参数
 *
 * @param[in] i2s    I2S 实例
 * @param[in] config 配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：CR
 * @note 当 I2S 模式关闭时才能配置（EN=0）
 */
bool i2s_configure(i2s_instance_t i2s, const i2s_config_t *config);

/**
 * @brief 配置 I2S 时钟预分频器
 *
 * @param[in] i2s    I2S 实例
 * @param[in] config 时钟配置结构体，不能为 NULL
 * @return true: 配置成功, false: 参数非法
 *
 * @note 覆盖寄存器：PR
 * @note 仅主模式有效，从模式下忽略
 * @note 当 I2S 模式关闭时才能配置
 */
bool i2s_config_clock(i2s_instance_t i2s, const i2s_clock_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 配置主模式接收数据量
 *
 * 设置 RSIZE 后，主机将接收指定帧数后自动停止。
 *
 * @param[in] i2s   I2S 实例
 * @param[in] count 接收帧数（0 = 无限接收）
 *
 * @note 覆盖寄存器：RSIZE
 * @note 仅主接收模式有效
 */
void i2s_set_rx_count(i2s_instance_t i2s, uint16_t count);

/**
 * @brief 启动 I2S 传输（主模式）
 *
 * 软件置位，硬件自动清零。仅主模式下 EN 使能后有效。
 *
 * @param[in] i2s I2S 实例
 *
 * @note 覆盖寄存器：CR.START
 */
void i2s_start(i2s_instance_t i2s);

/**
 * @brief 停止 I2S 传输（主模式）
 *
 * 下一帧数据传输完成后停止。主要用于主机接收数据。
 *
 * @param[in] i2s I2S 实例
 *
 * @note 覆盖寄存器：CR.STOP
 */
void i2s_stop(i2s_instance_t i2s);

/**
 * @brief 使能 I2S DMA 发送
 *
 * @param[in] i2s    I2S 实例
 * @param[in] enable true: 使能, false: 禁用
 *
 * @note 覆盖寄存器：CR.TXDMAEN
 */
void i2s_dma_tx_enable(i2s_instance_t i2s, bool enable);

/**
 * @brief 使能 I2S DMA 接收
 *
 * @param[in] i2s    I2S 实例
 * @param[in] enable true: 使能, false: 禁用
 *
 * @note 覆盖寄存器：CR.RXDMAEN
 */
void i2s_dma_rx_enable(i2s_instance_t i2s, bool enable);

/**
 * @brief 注册 I2S 中断回调
 *
 * @param[in] i2s      I2S 实例
 * @param[in] callback 回调函数，可为 NULL 取消注册
 */
void i2s_irq_register(i2s_instance_t i2s, i2s_callback_t callback);

/**
 * @brief 使能 I2S 中断
 *
 * @param[in] i2s  I2S 实例
 * @param[in] irq  中断掩码，可组合
 *
 * @note 覆盖寄存器：IER
 */
void i2s_irq_enable(i2s_instance_t i2s, i2s_irq_t irq);

/**
 * @brief 禁用 I2S 中断
 *
 * @param[in] i2s  I2S 实例
 * @param[in] irq  中断掩码，可组合
 *
 * @note 覆盖寄存器：IER
 */
void i2s_irq_disable(i2s_instance_t i2s, i2s_irq_t irq);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能 I2S 外设
 *
 * @param[in] i2s I2S 实例
 *
 * @note 覆盖寄存器：CR.EN
 */
void i2s_enable(i2s_instance_t i2s);

/**
 * @brief 禁用 I2S 外设
 *
 * @param[in] i2s I2S 实例
 *
 * @note 覆盖寄存器：CR.EN
 * @note 主机模式应在 MSUSP=1 后清零；从机发送模式应在 TXE=1 且 SVTC=1 后清零
 */
void i2s_disable(i2s_instance_t i2s);

/**
 * @brief 发送数据（阻塞，等待 TXE=1）
 *
 * @param[in] i2s  I2S 实例
 * @param[in] data 待发送的 32 位音频数据
 * @return true: 发送成功, false: 超时
 *
 * @note 覆盖寄存器：TXDR
 */
bool i2s_send(i2s_instance_t i2s, uint32_t data);

/**
 * @brief 接收数据（阻塞，等待 RXNE=1）
 *
 * @param[in]  i2s  I2S 实例
 * @param[out] data 接收数据指针，不能为 NULL
 * @return true: 接收成功, false: 超时或参数非法
 *
 * @note 覆盖寄存器：RXDR
 */
bool i2s_receive(i2s_instance_t i2s, uint32_t *data);

/**
 * @brief 获取 SR 状态寄存器值
 *
 * @param[in] i2s I2S 实例
 * @return SR 寄存器快照
 *
 * @note 覆盖寄存器：SR
 */
uint32_t i2s_get_flags(i2s_instance_t i2s);

/**
 * @brief 判断指定状态标志是否置位
 *
 * @param[in] i2s  I2S 实例
 * @param[in] flag 状态标志
 * @return true: 已置位, false: 未置位
 *
 * @note 覆盖寄存器：SR
 */
bool i2s_flag_is_set(i2s_instance_t i2s, i2s_flag_t flag);

/**
 * @brief 清除状态标志
 *
 * 对可清除的标志（UDR/OVR/FE/MSUSP/SVTC）写 0 清除。
 *
 * @param[in] i2s   I2S 实例
 * @param[in] flags 要清除的标志掩码
 *
 * @note 覆盖寄存器：SR
 * @note OVR 清除序列：先读 RXDR，再写 SR 对应位清零
 */
void i2s_clear_flags(i2s_instance_t i2s, uint32_t flags);

//===========================================
// 中断服务函数声明
//===========================================

/**
 * @brief I2S1 中断服务函数
 *
 * @note 由中断向量表直接调用
 */
void I2S1_IRQHandler(void);

/**
 * @brief I2S2 中断服务函数
 *
 * @note 由中断向量表直接调用
 */
void I2S2_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_I2S_H */
