/**
 * @file lpuart.h
 * @brief ACM32P4 LPUART 低功耗串口驱动
 *
 * @details
 * 功能特性：
 * - 独立时钟源（RCL/XTL/PCLK 分频），低功耗运行
 * - 32.768KHz 时钟下最大支持 9600 波特率
 * - 分数波特率（整数 + 小数部分）
 * - 可编程数据字长（7/8 位）
 * - 奇/偶校验、0/1 校验或无校验
 * - 可配置停止位（1/2 位）
 * - 数据极性控制（TXPOL/RXPOL）
 * - STOP 模式唤醒（START 位/1 字节/地址匹配）
 * - 总线空闲检测
 * - Bit Count Timeout 中断
 * - 支持 DMA 收发
 *
 * @note LPUART1 寄存器基地址：0x40008000
 * @note 时钟源通过 RCC_PERCFGR.LPUART1CLKSEL 和 RCC_PERCFGR.LPUART1DIV 配置
 * @note 寄存器时钟（PCLK）和通讯时钟（Lpuart_clk）独立
 *
 * @author ACM32P4 SDK Team
 * @version 1.1
 */

#ifndef _HARDWARE_LPUART_H
#define _HARDWARE_LPUART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

/**
 * @brief LPUART 实例数量
 */
#define LPUART_COUNT              1U     ///< LPUART 实例数量

/**
 * @brief LPUART 参数极限值
 */
#define LPUART_IBAUD_MIN          2U      ///< IBAUD 最小值（3 分频）
#define LPUART_IBAUD_MAX          254U    ///< IBAUD 最大值（255 分频）
#define LPUART_RXSAM_MAX          255U    ///< RXSAM 最大值
#define LPUART_FBAUD_MASK         0xFFFU  ///< FBAUD 位掩码（12 位）
#define LPUART_BCNT_VALUE_MAX     65535U  ///< BCNT_VALUE 最大值
#define LPUART_ADDR_MAX           255U    ///< 地址寄存器最大值
#define LPUART_DEFAULT_RCLK_HZ    32768U  ///< 默认 RCL 时钟频率

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief LPUART 实例选择
 */
typedef enum {
    LPUART_1 = 0,  ///< LPUART1
} lpuart_instance_t;

/**
 * @brief LPUART 数据字长
 */
typedef enum {
    LPUART_WLEN_8 = 0,  ///< 8 位数据
    LPUART_WLEN_7 = 1,  ///< 7 位数据
} lpuart_wlen_t;

/**
 * @brief LPUART 停止位数
 */
typedef enum {
    LPUART_STOP_1 = 0,  ///< 1 位停止位
    LPUART_STOP_2 = 1,  ///< 2 位停止位
} lpuart_stop_t;

/**
 * @brief LPUART 校验模式
 */
typedef enum {
    LPUART_PARITY_NONE = 0,  ///< 无校验
    LPUART_PARITY_ODD  = 1,  ///< 奇校验
    LPUART_PARITY_EVEN = 2,  ///< 偶校验
    LPUART_PARITY_0    = 3,  ///< 校验位强制为 0（0/1 校验模式）
    LPUART_PARITY_1    = 4,  ///< 校验位强制为 1（0/1 校验模式）
} lpuart_parity_t;

/**
 * @brief LPUART STOP 模式唤醒方式
 */
typedef enum {
    LPUART_WAKE_START  = 0,  ///< START 位检测唤醒
    LPUART_WAKE_1BYTE  = 1,  ///< 1 字节数据接收完成唤醒
    LPUART_WAKE_MATCH  = 2,  ///< 接收数据匹配地址唤醒
    LPUART_WAKE_NONE   = 3,  ///< 无唤醒
} lpuart_wake_mode_t;

/**
 * @brief LPUART 中断类型（位掩码）
 */
typedef enum {
    LPUART_IRQ_RX      = (1U << 0),   ///< 接收完成中断
    LPUART_IRQ_TC      = (1U << 1),   ///< 发送完成中断
    LPUART_IRQ_TXE     = (1U << 2),   ///< 发送缓冲空中断
    LPUART_IRQ_PE      = (1U << 3),   ///< 校验错误中断
    LPUART_IRQ_FE      = (1U << 4),   ///< 帧错误中断
    LPUART_IRQ_RXOV    = (1U << 5),   ///< 接收溢出中断
    LPUART_IRQ_MATCH   = (1U << 8),   ///< 地址匹配中断
    LPUART_IRQ_START   = (1U << 9),   ///< 起始位检测中断
    LPUART_IRQ_BCNT    = (1U << 16),  ///< Bit Count Timeout 中断
    LPUART_IRQ_IDLE    = (1U << 17),  ///< 总线空闲中断
} lpuart_irq_t;

/**
 * @brief LPUART 状态标志（位掩码）
 */
typedef enum {
    LPUART_FLAG_RXIF    = (1U << 0),   ///< 接收完成标志
    LPUART_FLAG_TCIF    = (1U << 1),   ///< 发送完成标志
    LPUART_FLAG_TXEIF   = (1U << 2),   ///< 发送缓冲空标志
    LPUART_FLAG_PEIF    = (1U << 3),   ///< 校验错误标志
    LPUART_FLAG_FEIF    = (1U << 4),   ///< 帧错误标志
    LPUART_FLAG_RXOVIF  = (1U << 5),   ///< 接收溢出标志
    LPUART_FLAG_RXF     = (1U << 6),   ///< 接收缓冲满
    LPUART_FLAG_TXOVF   = (1U << 7),   ///< 发送溢出标志
    LPUART_FLAG_MATCHIF = (1U << 8),   ///< 地址匹配标志
    LPUART_FLAG_STARTIF = (1U << 9),   ///< 起始位检测标志
    LPUART_FLAG_TXE     = (1U << 10),  ///< 发送缓冲空（非中断标志）
    LPUART_FLAG_BCNTIF  = (1U << 16),  ///< Bit Count Timeout 标志
    LPUART_FLAG_IDLEIF  = (1U << 17),  ///< 总线空闲标志
} lpuart_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief LPUART 配置结构体
 */
typedef struct {
    lpuart_wlen_t      wlen;              ///< 数据字长
    lpuart_stop_t      stop;              ///< 停止位数
    lpuart_parity_t    parity;            ///< 校验模式
    bool               txpol;             ///< 发送极性（true=取反）
    bool               rxpol;             ///< 接收极性（true=取反）
    lpuart_wake_mode_t wake_mode;         ///< STOP 模式唤醒方式
    bool               wake_check;        ///< 唤醒时检查校验位和 STOP 位
    bool               auto_start;        ///< 硬件自动计时控制
    uint16_t           bcnt_value;        ///< Bit Count Timeout 时长（比特为单位）
    uint8_t            addr;              ///< 地址匹配值
} lpuart_config_t;

//===========================================
// 回调函数类型
//===========================================

typedef void (*lpuart_callback_t)(lpuart_instance_t lpuart, uint32_t flags);  ///< LPUART 中断回调

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 LPUART（32.768KHz 时钟，9600 波特率）
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] wlen   数据字长
 * @param[in] parity 校验模式
 * @param[in] stop   停止位数
 *
 * @note 使用 RCL 32.768KHz 时钟，整数分频 3（值=2），小数部分 12'b100101010010
 *
 * @code
 * lpuart_init_9600(LPUART_1, LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
 * @endcode
 */
void lpuart_init_9600(lpuart_instance_t lpuart, lpuart_wlen_t wlen,
                      lpuart_parity_t parity, lpuart_stop_t stop);

/**
 * @brief 快速初始化 LPUART（PCLK 分频时钟，任意波特率）
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] baud    目标波特率
 * @param[in] clk_hz  LPUART 通讯时钟频率 (Hz)
 * @param[in] wlen    数据字长
 * @param[in] parity  校验模式
 * @param[in] stop    停止位数
 *
 * @note 自动计算 IBAUD/FBAUD/RXSAM
 * @note PCLK 分频时钟通过 RCC_PERCFGR.LPUART1CLKSEL 和 LPUART1DIV 选择
 *
 * @code
 * lpuart_init_pclk(LPUART_1, 115200U, 2000000U, LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
 * @endcode
 */
void lpuart_init_pclk(lpuart_instance_t lpuart, uint32_t baud, uint32_t clk_hz,
                      lpuart_wlen_t wlen, lpuart_parity_t parity, lpuart_stop_t stop);

/**
 * @brief 快速初始化 LPUART 为 STOP 模式唤醒接收
 *
 * @param[in] lpuart    LPUART 实例
 * @param[in] wake_mode 唤醒方式
 * @param[in] addr      地址匹配值（仅 LPUART_WAKE_MATCH 时有效）
 * @param[in] wlen      数据字长
 * @param[in] parity    校验模式
 * @param[in] stop      停止位数
 *
 * @note 配置后会仅使能接收（TXE=0, RXE=1）
 *
 * @code
 * lpuart_init_wakeup(LPUART_1, LPUART_WAKE_MATCH, 0x55, LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
 * @endcode
 */
void lpuart_init_wakeup(lpuart_instance_t lpuart, lpuart_wake_mode_t wake_mode, uint8_t addr,
                        lpuart_wlen_t wlen, lpuart_parity_t parity, lpuart_stop_t stop);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体完整配置 LPUART
 *
 * @param[in] lpuart  LPUART 实例
 * @param[in] config  配置结构体
 * @return true  配置成功
 * @return false 参数无效
 *
 * @note 不覆盖 CR/IBAUD/FBAUD/IE（需单独配置）
 *
 * @code
 * lpuart_config_t cfg = {
 *     .wlen = LPUART_WLEN_8,
 *     .stop = LPUART_STOP_1,
 *     .parity = LPUART_PARITY_NONE,
 *     .txpol = false, .rxpol = false,
 *     .wake_mode = LPUART_WAKE_NONE,
 *     .wake_check = true,
 *     .auto_start = false,
 *     .bcnt_value = 11U,
 *     .addr = 0U,
 * };
 * lpuart_config(LPUART_1, &cfg);
 * @endcode
 */
bool lpuart_config(lpuart_instance_t lpuart, const lpuart_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

// --- 数据收发 ---

/**
 * @brief 发送单字节
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] data   数据字节
 *
 * @note 发送前应检查 TXEIF 标志确保发送缓冲区为空
 */
void lpuart_send_byte(lpuart_instance_t lpuart, uint8_t data);

/**
 * @brief 接收单字节
 *
 * @param[in] lpuart LPUART 实例
 * @return 接收到的数据
 *
 * @note 接收前应检查 RXF 标志确保数据有效
 */
uint8_t lpuart_read_byte(lpuart_instance_t lpuart);

/**
 * @brief 发送数据缓冲区（阻塞模式）
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] data   数据缓冲区指针
 * @param[in] len    发送长度（字节）
 *
 * @note 每字节发送前等待 TXEIF 置位
 */
void lpuart_send_buffer(lpuart_instance_t lpuart, const uint8_t *data, size_t len);

/**
 * @brief 接收数据缓冲区（阻塞模式）
 *
 * @param[in]  lpuart   LPUART 实例
 * @param[out] data     接收数据缓冲区
 * @param[in]  len      期望接收长度（字节）
 * @param[in]  timeout  超时时间（系统 tick），0 表示无超时
 * @return 实际接收字节数
 *
 * @note 每字节接收前等待 RXF 置位
 */
size_t lpuart_read_buffer(lpuart_instance_t lpuart, uint8_t *data, size_t len, uint32_t timeout);

/**
 * @brief 发送字符串（以 '\0' 结尾）
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] str    字符串指针
 */
void lpuart_send_string(lpuart_instance_t lpuart, const char *str);

/**
 * @brief 等待发送完成
 *
 * @param[in] lpuart   LPUART 实例
 * @param[in] timeout  超时时间（系统 tick）
 * @return true: 发送完成, false: 超时
 *
 * @note 等待 TCIF 标志置位
 */
bool lpuart_flush_tx(lpuart_instance_t lpuart, uint32_t timeout);

// --- 波特率 ---

/**
 * @brief 设置波特率
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] ibaud  整数分频值（2~254），硬件值=Fsys/BAUD 整数部分-1
 * @param[in] fbaud  小数部分（12 位），每位对应 Start→STOP 是否加1时钟
 * @param[in] rxsam  接收采样点（通常设为 ibaud>>1）
 *
 */
void lpuart_set_baudrate(lpuart_instance_t lpuart, uint8_t ibaud, uint16_t fbaud, uint8_t rxsam);

/**
 * @brief 自动计算并设置波特率
 *
 * @param[in] lpuart          LPUART 实例
 * @param[in] baud            目标波特率
 * @param[in] clk_hz          通讯时钟频率 (Hz)
 * @param[in] bits_per_frame  每帧比特数（Start + Data + Parity + Stop）
 *
 * @note LCR 需在此函数调用前完成配置，以正确确定 bit 位数
 *
 * @code
 * // 8N1: Start(1) + Data(8) + Parity(0) + Stop(1) = 10
 * lpuart_calculate_baudrate(LPUART_1, 115200U, 2000000U, 10U);
 * @endcode
 */
void lpuart_calculate_baudrate(lpuart_instance_t lpuart, uint32_t baud, uint32_t clk_hz,
                               uint8_t bits_per_frame);

// --- DMA ---

/**
 * @brief 使能/禁止 DMA
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 覆盖寄存器：LPUART_CR.DMA_EN
 * @note 使能后 LPUART 接收/发送可触发 DMA 请求
 */
void lpuart_dma_enable(lpuart_instance_t lpuart, bool enable);

/**
 * @brief 获取 DMA 使能状态
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 已使能, false: 已禁止
 */
bool lpuart_dma_is_enabled(lpuart_instance_t lpuart);

/**
 * @brief 获取 LPUART RXDR 寄存器地址（用于 DMA 配置）
 *
 * @param[in] lpuart LPUART 实例
 * @return RXDR 寄存器地址
 */
uint32_t lpuart_get_rxdr_addr(lpuart_instance_t lpuart);

/**
 * @brief 获取 LPUART TXDR 寄存器地址（用于 DMA 配置）
 *
 * @param[in] lpuart LPUART 实例
 * @return TXDR 寄存器地址
 */
uint32_t lpuart_get_txdr_addr(lpuart_instance_t lpuart);

//===========================================
// 第4层：控制与查询 API
//===========================================

// --- CR 控制寄存器 ---

/**
 * @brief 使能/禁止发送
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 覆盖寄存器：LPUART_CR.TX_EN
 */
void lpuart_tx_enable(lpuart_instance_t lpuart, bool enable);

/**
 * @brief 使能/禁止接收
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 覆盖寄存器：LPUART_CR.RX_EN
 */
void lpuart_rx_enable(lpuart_instance_t lpuart, bool enable);

/**
 * @brief 获取发送使能状态
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 已使能, false: 已禁止
 */
bool lpuart_tx_is_enabled(lpuart_instance_t lpuart);

/**
 * @brief 获取接收使能状态
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 已使能, false: 已禁止
 */
bool lpuart_rx_is_enabled(lpuart_instance_t lpuart);

// --- LCR 线控寄存器 ---

/**
 * @brief 设置数据字长
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] wlen   数据字长
 *
 * @note 覆盖寄存器：LPUART_LCR.WLEN
 */
void lpuart_set_wlen(lpuart_instance_t lpuart, lpuart_wlen_t wlen);

/**
 * @brief 获取数据字长
 *
 * @param[in] lpuart LPUART 实例
 * @return 数据字长
 */
lpuart_wlen_t lpuart_get_wlen(lpuart_instance_t lpuart);

/**
 * @brief 设置停止位数
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] stop   停止位数
 *
 * @note 覆盖寄存器：LPUART_LCR.STP2
 */
void lpuart_set_stop(lpuart_instance_t lpuart, lpuart_stop_t stop);

/**
 * @brief 获取停止位数
 *
 * @param[in] lpuart LPUART 实例
 * @return 停止位数
 */
lpuart_stop_t lpuart_get_stop(lpuart_instance_t lpuart);

/**
 * @brief 设置校验模式
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] parity 校验模式
 *
 * @note 覆盖寄存器：LPUART_LCR.PEN, LPUART_LCR.SPS, LPUART_LCR.EPS
 */
void lpuart_set_parity(lpuart_instance_t lpuart, lpuart_parity_t parity);

/**
 * @brief 获取校验模式
 *
 * @param[in] lpuart LPUART 实例
 * @return 校验模式
 */
lpuart_parity_t lpuart_get_parity(lpuart_instance_t lpuart);

/**
 * @brief 设置发送数据极性
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] invert true: 取反, false: 不取反
 *
 * @note 覆盖寄存器：LPUART_LCR.TXPOL
 */
void lpuart_set_tx_polarity(lpuart_instance_t lpuart, bool invert);

/**
 * @brief 获取发送数据极性
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 取反, false: 不取反
 */
bool lpuart_get_tx_polarity(lpuart_instance_t lpuart);

/**
 * @brief 设置接收数据极性
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] invert true: 取反, false: 不取反
 *
 * @note 覆盖寄存器：LPUART_LCR.RXPOL
 */
void lpuart_set_rx_polarity(lpuart_instance_t lpuart, bool invert);

/**
 * @brief 获取接收数据极性
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 取反, false: 不取反
 */
bool lpuart_get_rx_polarity(lpuart_instance_t lpuart);

/**
 * @brief 设置 STOP 模式唤醒方式
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] mode   唤醒方式
 *
 * @note 覆盖寄存器：LPUART_LCR.RXWKS
 */
void lpuart_set_wake_mode(lpuart_instance_t lpuart, lpuart_wake_mode_t mode);

/**
 * @brief 获取 STOP 模式唤醒方式
 *
 * @param[in] lpuart LPUART 实例
 * @return 唤醒方式
 */
lpuart_wake_mode_t lpuart_get_wake_mode(lpuart_instance_t lpuart);

/**
 * @brief 设置唤醒校验检查
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] check  true: 检查校验位和 STOP 位, false: 不检查
 *
 * @note 覆盖寄存器：LPUART_LCR.WKCK
 */
void lpuart_set_wake_check(lpuart_instance_t lpuart, bool check);

/**
 * @brief 获取唤醒校验检查设置
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 检查, false: 不检查
 */
bool lpuart_get_wake_check(lpuart_instance_t lpuart);

/**
 * @brief 设置硬件自动计时
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 接收数据时，接收到 STOP 位自动启动一次比特计时
 */
void lpuart_set_auto_start(lpuart_instance_t lpuart, bool enable);

/**
 * @brief 获取硬件自动计时状态
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 使能, false: 禁止
 */
bool lpuart_get_auto_start(lpuart_instance_t lpuart);

/**
 * @brief 设置 Bit Count Timeout 时长
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] bcnt   计时时长（以比特位为单位）
 *
 * @note 设置为 0 则不清除 BCNT_VALUE
 */
void lpuart_set_bcnt_value(lpuart_instance_t lpuart, uint16_t bcnt);

/**
 * @brief 获取 Bit Count Timeout 时长
 *
 * @param[in] lpuart LPUART 实例
 * @return 计时时长
 */
uint16_t lpuart_get_bcnt_value(lpuart_instance_t lpuart);

// --- IBAUD/FBAUD ---

/**
 * @brief 设置波特率整数部分
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] ibaud  整数分频值（2~254），实际分频 = ibaud+1
 *
 * @note 覆盖寄存器：LPUART_IBAUD.IBAUD
 */
void lpuart_set_ibaud(lpuart_instance_t lpuart, uint8_t ibaud);

/**
 * @brief 获取波特率整数部分
 *
 * @param[in] lpuart LPUART 实例
 * @return 整数分频值
 */
uint8_t lpuart_get_ibaud(lpuart_instance_t lpuart);

/**
 * @brief 设置波特率小数部分
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] fbaud  小数部分（12 位）
 *
 * @note 覆盖寄存器：LPUART_FBAUD.FBAUD
 */
void lpuart_set_fbaud(lpuart_instance_t lpuart, uint16_t fbaud);

/**
 * @brief 获取波特率小数部分
 *
 * @param[in] lpuart LPUART 实例
 * @return 小数部分
 */
uint16_t lpuart_get_fbaud(lpuart_instance_t lpuart);

/**
 * @brief 设置接收采样点
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] rxsam  采样点位置（通常设为 IBAUD>>1）
 *
 * @note 覆盖寄存器：LPUART_IBAUD.RXSAM
 */
void lpuart_set_rxsam(lpuart_instance_t lpuart, uint8_t rxsam);

/**
 * @brief 获取接收采样点
 *
 * @param[in] lpuart LPUART 实例
 * @return 采样点位置
 */
uint8_t lpuart_get_rxsam(lpuart_instance_t lpuart);

// --- ADDR ---

/**
 * @brief 设置地址匹配值
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] addr   匹配地址
 *
 * @note 唤醒模式为 LPUART_WAKE_MATCH 时有效
 */
void lpuart_set_addr(lpuart_instance_t lpuart, uint8_t addr);

/**
 * @brief 获取地址匹配值
 *
 * @param[in] lpuart LPUART 实例
 * @return 匹配地址
 */
uint8_t lpuart_get_addr(lpuart_instance_t lpuart);

// --- IE 中断使能 ---

/**
 * @brief 使能/禁止指定中断
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] irq    中断类型（可组合）
 * @param[in] enable true: 使能, false: 禁止
 *
 */
void lpuart_irq_enable(lpuart_instance_t lpuart, lpuart_irq_t irq, bool enable);

/**
 * @brief 检查指定中断是否已使能
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] irq    中断类型
 * @return true: 已使能, false: 已禁止
 */
bool lpuart_irq_is_enabled(lpuart_instance_t lpuart, lpuart_irq_t irq);

/**
 * @brief 禁用所有中断
 *
 * @param[in] lpuart LPUART 实例
 *
 */
void lpuart_irq_disable_all(lpuart_instance_t lpuart);

// --- SR 状态查询与清除 ---

/**
 * @brief 获取状态标志
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] flag   状态标志
 * @return true: 置位, false: 未置位
 *
 */
bool lpuart_get_flag(lpuart_instance_t lpuart, lpuart_flag_t flag);

/**
 * @brief 清除状态标志
 *
 * @param[in] lpuart LPUART 实例
 * @param[in] flag   要清除的标志
 *
 * @note RXF 通过读 RXDR 清除，TXEIF 通过写 TXDR 清除
 */
void lpuart_clear_flag(lpuart_instance_t lpuart, lpuart_flag_t flag);

/**
 * @brief 获取状态寄存器原始值
 *
 * @param[in] lpuart LPUART 实例
 * @return 状态寄存器值
 */
uint32_t lpuart_get_status(lpuart_instance_t lpuart);

/**
 * @brief 检查接收缓冲区是否有数据
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 有数据（RXF=1）, false: 空
 */
bool lpuart_rx_available(lpuart_instance_t lpuart);

/**
 * @brief 检查发送缓冲区是否为空
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 空（TXEIF=1）, false: 满
 */
bool lpuart_tx_empty(lpuart_instance_t lpuart);

/**
 * @brief 检查发送是否完成
 *
 * @param[in] lpuart LPUART 实例
 * @return true: 完成（TCIF=1）, false: 未完成
 */
bool lpuart_tx_done(lpuart_instance_t lpuart);

// --- 中断回调 ---

/**
 * @brief 注册 LPUART 中断回调
 *
 * @param[in] lpuart   LPUART 实例
 * @param[in] callback 回调函数（传入 NULL 取消注册）
 *
 * @note 中断源：LPUART1_IRQn
 * @note 回调参数 flags 为当前 SR 值
 */
void lpuart_register_callback(lpuart_instance_t lpuart, lpuart_callback_t callback);

// --- 时钟 ---

/**
 * @brief 使能 LPUART 时钟
 *
 * @param[in] lpuart LPUART 实例
 */
void lpuart_enable_clock(lpuart_instance_t lpuart);

/**
 * @brief 禁止 LPUART 时钟
 *
 * @param[in] lpuart LPUART 实例
 */
void lpuart_disable_clock(lpuart_instance_t lpuart);

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 根据目标波特率计算波特率参数
 *
 * @param[in]  baud             目标波特率
 * @param[in]  clk_hz           通讯时钟频率 (Hz)
 * @param[in]  bits_per_frame   每帧比特数
 * @param[out] ibaud            整数分频值输出
 * @param[out] fbaud            小数部分输出
 * @param[out] rxsam            接收采样点输出
 *
 * @note 不写入寄存器，仅计算
 */
void lpuart_baudrate_calc(uint32_t baud, uint32_t clk_hz, uint8_t bits_per_frame,
                          uint8_t *ibaud, uint16_t *fbaud, uint8_t *rxsam);

/**
 * @brief 计算实际波特率
 *
 * @param[in] clk_hz   通讯时钟频率 (Hz)
 * @param[in] ibaud    整数分频值
 * @param[in] fbaud    小数部分
 * @param[in] bits_per_frame 每帧比特数
 * @return 实际波特率
 */
uint32_t lpuart_get_actual_baudrate(uint32_t clk_hz, uint8_t ibaud,
                                    uint16_t fbaud, uint8_t bits_per_frame);

//===========================================
// 中断处理函数声明
//===========================================

/**
 * @brief LPUART 中断处理函数
 *
 * @param[in] lpuart LPUART 实例
 *
 * @note 用户需在 LPUART1_IRQHandler 中调用此函数
 */
void lpuart_irq_handler(lpuart_instance_t lpuart);

#ifdef __cplusplus
}
#endif
#endif // _HARDWARE_LPUART_H
