/**
 * @file uart.h
 * @brief ACM32P4 通用异步收发器（UART）驱动
 * @author ACM32P4 SDK Team
 * @version 1.0
 * 
 * @details
 * ACM32P4芯片集成了8个UART串口模块，支持：
 * - 全双工异步通信
 * - 可编程波特率（整数+小数分频）
 * - 可编程字长（5~9bit）
 * - 可编程校验位（奇偶校验、0/1校验）
 * - 可编程停止位（1或2位）
 * - UART1~4,6支持16字节FIFO，UART5,7,8支持4字节FIFO
 * - CTS/RTS硬件流控
 * - DMA支持
 * - 总线空闲检测
 * - 扩展功能：LIN、IrDA、智能卡、单线模式、多机通信、RS485、同步模式
 * 
 * @code
 * // 基本使用示例
 * uart_init_default(UART1, 115200);
 * uart_putc(UART1, 'A');
 * uint8_t data = uart_getc(UART1);
 * @endcode
 */

#ifndef _HARDWARE_UART_H
#define _HARDWARE_UART_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//===========================================
// 宏定义
//===========================================

/** @brief UART外设实例总数 */
#define UART_INSTANCE_COUNT 8U

//===========================================
// 类型定义
//===========================================

/**
 * @brief UART外设实例
 */
typedef enum {
    UART_1  = 0,    ///< UART1，基地址0x40013800，IRQn=32
    UART_2  = 1,    ///< UART2，基地址0x40004400，IRQn=33
    UART_3  = 2,    ///< UART3，基地址0x40004800，IRQn=34
    UART_4  = 3,    ///< UART4，基地址0x40004C00，IRQn=35
    UART_5  = 4,    ///< UART5，基地址0x40005000，IRQn=47
    UART_6  = 5,    ///< UART6，基地址0x40013C00，IRQn=48
    UART_7  = 6,    ///< UART7，基地址0x40009800，IRQn=49
    UART_8  = 7,    ///< UART8，基地址0x40009C00，IRQn=50
} uart_num_t;

/**
 * @brief UART字长选择
 */
typedef enum {
    UART_WORD_LENGTH_5BIT = 0x00,  ///< 5位字长
    UART_WORD_LENGTH_6BIT = 0x01,  ///< 6位字长
    UART_WORD_LENGTH_7BIT = 0x02,  ///< 7位字长
    UART_WORD_LENGTH_8BIT = 0x03,  ///< 8位字长
    UART_WORD_LENGTH_9BIT = 0x04   ///< 9位字长
} uart_word_length_t;

/**
 * @brief UART停止位选择
 */
typedef enum {
    UART_STOP_BITS_1 = 0,  ///< 1位停止位
    UART_STOP_BITS_2 = 1   ///< 2位停止位
} uart_stop_bits_t;

/**
 * @brief UART校验方式
 */
typedef enum {
    UART_PARITY_NONE  = 0x00,  ///< 无校验
    UART_PARITY_EVEN  = 0x01,  ///< 偶校验
    UART_PARITY_ODD   = 0x02,  ///< 奇校验
    UART_PARITY_ZERO  = 0x03,  ///< 强制0校验
    UART_PARITY_ONE   = 0x04   ///< 强制1校验
} uart_parity_t;

/**
 * @brief UART数据传输字节序
 */
typedef enum {
    UART_ENDIAN_LSB_FIRST = 0,  ///< 小端传输（LSB先发）
    UART_ENDIAN_MSB_FIRST = 1   ///< 大端传输（MSB先发）
} uart_endian_t;

/**
 * @brief UART FIFO触发阈值（接收）
 */
typedef enum {
    UART_RX_FIFO_THRESHOLD_2_BYTES  = 0x00,  ///< 1/8满（2字节）
    UART_RX_FIFO_THRESHOLD_4_BYTES  = 0x01,  ///< 1/4满（4字节）
    UART_RX_FIFO_THRESHOLD_8_BYTES  = 0x02,  ///< 1/2满（8字节）
    UART_RX_FIFO_THRESHOLD_12_BYTES = 0x03,  ///< 3/4满（12字节）
    UART_RX_FIFO_THRESHOLD_14_BYTES = 0x04,  ///< 7/8满（14字节）
    UART_RX_FIFO_THRESHOLD_1_BYTE   = 0x05   ///< 非空（1字节）
} uart_rx_fifo_threshold_t;

/**
 * @brief UART FIFO触发阈值（发送）
 */
typedef enum {
    UART_TX_FIFO_THRESHOLD_2_BYTES  = 0x00,  ///< 1/8剩余（剩2字节）
    UART_TX_FIFO_THRESHOLD_4_BYTES  = 0x01,  ///< 1/4剩余（剩4字节）
    UART_TX_FIFO_THRESHOLD_8_BYTES  = 0x02,  ///< 1/2剩余（剩8字节）
    UART_TX_FIFO_THRESHOLD_12_BYTES = 0x03,  ///< 3/4剩余（剩12字节）
    UART_TX_FIFO_THRESHOLD_14_BYTES = 0x04,  ///< 7/8剩余（剩14字节）
    UART_TX_FIFO_THRESHOLD_EMPTY    = 0x05   ///< FIFO变空
} uart_tx_fifo_threshold_t;

/**
 * @brief UART中断类型
 */
typedef enum {
    UART_INT_RX    = (1 << 4),   ///< 接收中断
    UART_INT_TX    = (1 << 5),   ///< 发送中断
    UART_INT_FE    = (1 << 7),   ///< 帧格式错误中断
    UART_INT_PE    = (1 << 8),   ///< 奇偶校验错误中断
    UART_INT_BE    = (1 << 9),   ///< Break错误中断
    UART_INT_OE    = (1 << 10),  ///< Overrun错误中断
    UART_INT_LBD   = (1 << 11),  ///< LIN Break检测中断
    UART_INT_BCNT  = (1 << 12),  ///< 比特计时超时中断
    UART_INT_IDLE  = (1 << 13),  ///< 总线空闲中断
    UART_INT_ABR   = (1 << 14),  ///< 波特率自适应完成中断
    UART_INT_TC    = (1 << 15)   ///< 发送完成中断
} uart_int_t;

/**
 * @brief UART错误标志
 */
typedef enum {
    UART_ERROR_NONE = 0x00,      ///< 无错误
    UART_ERROR_FE   = (1 << 0),  ///< 帧格式错误
    UART_ERROR_PE   = (1 << 1),  ///< 奇偶校验错误
    UART_ERROR_BE   = (1 << 2),  ///< Break错误
    UART_ERROR_OE   = (1 << 3)   ///< Overrun错误
} uart_error_t;

/**
 * @brief UART多机通信唤醒方式
 */
typedef enum {
    UART_WAKEUP_IDLE_LINE = 0,  ///< 空闲总线唤醒
    UART_WAKEUP_ADDRESS   = 1   ///< 地址标记唤醒
} uart_wakeup_method_t;

/**
 * @brief UART多机通信地址模式
 */
typedef enum {
    UART_ADDRESS_MODE_4BIT = 0,  ///< 4位地址模式
    UART_ADDRESS_MODE_7BIT = 1   ///< 7位地址模式
} uart_address_mode_t;

/**
 * @brief UART同步模式时钟极性
 */
typedef enum {
    UART_CLOCK_POLARITY_LOW  = 0,  ///< 空闲时SCLK为低电平
    UART_CLOCK_POLARITY_HIGH = 1   ///< 空闲时SCLK为高电平
} uart_clock_polarity_t;

/**
 * @brief UART同步模式时钟相位
 */
typedef enum {
    UART_CLOCK_PHASE_1ST_EDGE = 0,  ///< 第一个变化沿捕获
    UART_CLOCK_PHASE_2ND_EDGE = 1   ///< 第二个变化沿捕获
} uart_clock_phase_t;

/**
 * @brief RS485 DE信号极性
 */
typedef enum {
    UART_DE_POLARITY_HIGH = 0,  ///< 高电平有效
    UART_DE_POLARITY_LOW  = 1   ///< 低电平有效
} uart_de_polarity_t;

//===========================================
// 配置结构体
//===========================================

/**
 * @brief UART基本配置结构体
 */
typedef struct {
    uint32_t              baudrate;      ///< 波特率
    uart_word_length_t    word_length;   ///< 字长
    uart_stop_bits_t      stop_bits;     ///< 停止位
    uart_parity_t         parity;        ///< 校验方式
    uart_endian_t         endian;        ///< 字节序
    bool                  fifo_enable;   ///< FIFO使能（默认false=禁用）
} uart_config_t;

/**
 * @brief UART FIFO配置结构体
 */
typedef struct {
    bool                      enable;          ///< FIFO使能
    uart_rx_fifo_threshold_t  rx_threshold;    ///< 接收FIFO触发阈值
    uart_tx_fifo_threshold_t  tx_threshold;    ///< 发送FIFO触发阈值
} uart_fifo_config_t;

/**
 * @brief UART硬件流控配置结构体
 */
typedef struct {
    bool cts_enable;  ///< CTS流控使能
    bool rts_enable;  ///< RTS流控使能
} uart_flow_control_config_t;

/**
 * @brief UART DMA配置结构体
 */
typedef struct {
    bool tx_dma_enable;      ///< 发送DMA使能
    bool rx_dma_enable;      ///< 接收DMA使能
    bool dma_disable_on_error;  ///< 接收错误时是否禁止DMA请求（DMAONERR=1时ERROR不产生DMA请求）
} uart_dma_config_t;

/**
 * @brief UART LIN模式配置结构体
 */
typedef struct {
    bool     enable;          ///< LIN模式使能
    uint32_t break_length;    ///< Break信号长度（比特数）
    uint8_t  lbd_cnt;         ///< LIN Break检测计数阈值（0-15），监听到此数量BREAK后置位LBDI
} uart_lin_config_t;

/**
 * @brief UART IrDA模式配置结构体
 */
typedef struct {
    bool     enable;          ///< IrDA模式使能
    bool     low_power;       ///< 低功耗模式使能
    uint8_t  prescaler;       ///< 预分频值
} uart_irda_config_t;

/**
 * @brief UART智能卡模式配置结构体
 */
typedef struct {
    bool     enable;                 ///< 智能卡模式使能
    bool     nack_enable;            ///< NACK使能（校验错误时发送NACK）
    uint8_t  prescaler;              ///< 时钟分频器（0-31）
    uint8_t  guard_time;             ///< 保护时间（ETU单位）
    bool     auto_retry;             ///< 自动重发使能
    uint32_t block_timeout;          ///< 块超时时间（比特数）
} uart_smartcard_config_t;

/**
 * @brief UART多机通信配置结构体
 */
typedef struct {
    bool                   enable;          ///< 多机通信模式使能
    uart_wakeup_method_t   wakeup_method;   ///< 唤醒方式
    uart_address_mode_t    address_mode;    ///< 地址模式
    uint8_t                address;         ///< 节点地址
} uart_multiprocessor_config_t;

/**
 * @brief UART同步模式配置结构体
 */
typedef struct {
    bool                    enable;           ///< 同步模式使能
    uart_clock_polarity_t   clock_polarity;   ///< 时钟极性
    uart_clock_phase_t      clock_phase;      ///< 时钟相位
    bool                    last_bit_clock;   ///< 最后一位时钟脉冲使能
} uart_sync_config_t;

/**
 * @brief RS485配置结构体
 */
typedef struct {
    bool                enable;        ///< RS485模式使能
    uart_de_polarity_t  de_polarity;   ///< DE信号极性
    uint8_t             de_assert;     ///< DE信号提前使能时间（比特数）
    uint8_t             de_deassert;   ///< DE信号延迟禁用时间（比特数）
} uart_rs485_config_t;

//===========================================
// 回调函数类型定义
//===========================================

/**
 * @brief UART接收回调函数类型
 * @param uart UART外设编号
 * @param data 接收的数据
 *
 * @note 对应中断：UART_INT_RX
 */
typedef void (*uart_rx_callback_t)(uart_num_t uart, uint8_t data);

/**
 * @brief UART发送缓冲区空中断回调函数类型
 * @param uart UART外设编号
 *
 * @note TX 在数据从发送缓冲区移入移位寄存器时触发（FIFO 模式下按阈值触发），
 *       与 TC 不同：TX 触发时移位寄存器可能仍在发送
 * @note 对应中断：UART_INT_TX
 */
typedef void (*uart_tx_callback_t)(uart_num_t uart);

/**
 * @brief UART错误回调函数类型
 * @param uart UART外设编号
 * @param error 错误标志
 *
 * @note 对应中断：UART_INT_FE | UART_INT_PE | UART_INT_BE | UART_INT_OE
 */
typedef void (*uart_error_callback_t)(uart_num_t uart, uart_error_t error);

/**
 * @brief UART空闲回调函数类型
 * @param uart UART外设编号
 *
 * @note 对应中断：UART_INT_IDLE
 */
typedef void (*uart_idle_callback_t)(uart_num_t uart);

/**
 * @brief UART发送完成（TC）回调函数类型
 * @param uart UART外设编号
 *
 * @note TC（Transmit Complete）在移位寄存器发送完最后一帧数据后触发，
 *       与 TX（发送缓冲区空）不同：TX 在数据从缓冲区移入移位寄存器时触发，
 *       TC 在移位寄存器完全空闲时触发
 * @note 对应中断：UART_INT_TC
 */
typedef void (*uart_tc_callback_t)(uart_num_t uart);

/**
 * @brief UART LIN Break检测回调函数类型
 * @param uart UART外设编号
 *
 * @note 对应中断：UART_INT_LBD
 */
typedef void (*uart_lbd_callback_t)(uart_num_t uart);

/**
 * @brief UART比特计数超时回调函数类型
 * @param uart UART外设编号
 *
 * @note 对应中断：UART_INT_BCNT
 */
typedef void (*uart_bcnt_callback_t)(uart_num_t uart);

/**
 * @brief UART波特率自适应完成回调函数类型
 * @param uart UART外设编号
 *
 * @note 对应中断：UART_INT_ABR
 */
typedef void (*uart_abr_callback_t)(uart_num_t uart);

//===========================================
// 第1层：快速初始化API
//===========================================

/**
 * @brief 使用默认配置初始化UART
 * 
 * 默认配置：8位数据位，1位停止位，无校验，无流控，禁用FIFO
 * 自动使能对应UART时钟
 * 
 * @param uart UART外设编号
 * @param baudrate 波特率
 * 
 * @return 成功返回true，失败返回false
 * 
 * @note 自动使能对应UART外设时钟
 * 
 * @code
 * uart_init_default(UART1, 115200);
 * @endcode
 * @note 覆盖寄存器：UART_BRR.IBAUD/FBAUD, UART_CR1.UARTEN/TXE[8]/RXE[9], UART_CR3.WLEN/STP2/EPS/PEN
 */
bool uart_init_default(uart_num_t uart, uint32_t baudrate);

/**
 * @brief 使用自定义配置初始化UART
 * 
 * 根据配置结构体初始化UART，自动使能对应UART时钟
 * 
 * @param uart UART外设编号
 * @param config UART基本配置结构体指针
 * 
 * @return 成功返回true，失败返回false
 * 
 * @note 自动使能对应UART外设时钟
 * 
 * @code
 * uart_config_t config = {
 *     .baudrate = 9600,
 *     .word_length = UART_WORD_LENGTH_8BIT,
 *     .stop_bits = UART_STOP_BITS_1,
 *     .parity = UART_PARITY_EVEN,
 *     .endian = UART_ENDIAN_LSB_FIRST,
 *     .fifo_enable = false
 * };
 * uart_init_custom(UART1, &config);
 * @endcode
 * @note 覆盖寄存器：UART_BRR, UART_CR1, UART_CR2, UART_CR3
 */
bool uart_init_custom(uart_num_t uart, const uart_config_t *config);

/**
 * @brief 反初始化UART
 * 
 * 禁用UART，关闭时钟
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_deinit(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.UARTEN
 */
void uart_deinit(uart_num_t uart);

//===========================================
// 第2层：基础配置API
//===========================================

/**
 * @brief 配置UART波特率
 * 
 * @param uart UART外设编号
 * @param baudrate 波特率
 * 
 * @return 成功返回true，失败返回false
 * 
 * @note 需要在UART禁用状态下配置
 * 
 * @code
 * uart_config_baudrate(UART1, 115200);
 * @endcode
 * @note 覆盖寄存器：UART_BRR.IBAUD/FBAUD
 */
bool uart_config_baudrate(uart_num_t uart, uint32_t baudrate);

/**
 * @brief 配置UART字长
 * 
 * @param uart UART外设编号
 * @param word_length 字长选择
 * 
 * @code
 * uart_config_word_length(UART1, UART_WORD_LENGTH_8BIT);
 * @endcode
 * @note 覆盖寄存器：UART_CR3.WLEN[8:6]
 */
void uart_config_word_length(uart_num_t uart, uart_word_length_t word_length);

/**
 * @brief 配置UART停止位
 * 
 * @param uart UART外设编号
 * @param stop_bits 停止位选择
 * 
 * @code
 * uart_config_stop_bits(UART1, UART_STOP_BITS_1);
 * @endcode
 * @note 覆盖寄存器：UART_CR3.STP2[3]
 */
void uart_config_stop_bits(uart_num_t uart, uart_stop_bits_t stop_bits);

/**
 * @brief 配置UART校验方式
 * 
 * @param uart UART外设编号
 * @param parity 校验方式
 * 
 * @code
 * uart_config_parity(UART1, UART_PARITY_EVEN);
 * @endcode
 * @note 覆盖寄存器：UART_CR3.EPS[2]/PEN[1]
 */
void uart_config_parity(uart_num_t uart, uart_parity_t parity);

/**
 * @brief 配置UART字节序
 * 
 * @param uart UART外设编号
 * @param endian 字节序选择
 * 
 * @code
 * uart_config_endian(UART1, UART_ENDIAN_LSB_FIRST);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.ENDIAN_SEL[16]
 */
void uart_config_endian(uart_num_t uart, uart_endian_t endian);

/**
 * @brief 配置UART FIFO
 * 
 * @param uart UART外设编号
 * @param config FIFO配置结构体指针
 * 
 * @code
 * uart_fifo_config_t fifo_config = {
 *     .enable = true,
 *     .rx_threshold = UART_RX_FIFO_THRESHOLD_8_BYTES,
 *     .tx_threshold = UART_TX_FIFO_THRESHOLD_8_BYTES
 * };
 * uart_config_fifo(UART1, &fifo_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR3.FEN[5], UART_CR3.RXIFLSEL[15:13]/TXIFLSEL[12:10]
 */
void uart_config_fifo(uart_num_t uart, const uart_fifo_config_t *config);

/**
 * @brief 配置UART硬件流控
 * 
 * @param uart UART外设编号
 * @param config 流控配置结构体指针
 * 
 * @code
 * uart_flow_control_config_t flow_config = {
 *     .cts_enable = true,
 *     .rts_enable = true
 * };
 * uart_config_flow_control(UART1, &flow_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.CTSEN/RTSEN
 */
void uart_config_flow_control(uart_num_t uart, const uart_flow_control_config_t *config);

/**
 * @brief 配置UART LIN模式
 * 
 * @param uart UART外设编号
 * @param config LIN配置结构体指针
 * 
 * @code
 * uart_lin_config_t lin_config = {
 *     .enable = true,
 *     .break_length = 13
 * };
 * uart_config_lin(UART1, &lin_config);
 * @endcode
 * @note 覆盖寄存器：UART_BCNT.BCNT_VALUE/LBD_CNT, UART_CR3.BRK[0]
 */
void uart_config_lin(uart_num_t uart, const uart_lin_config_t *config);

/**
 * @brief 配置UART IrDA模式
 * 
 * @param uart UART外设编号
 * @param config IrDA配置结构体指针
 * 
 * @code
 * uart_irda_config_t irda_config = {
 *     .enable = true,
 *     .low_power = true,
 *     .prescaler = 1
 * };
 * uart_config_irda(UART1, &irda_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.SIREN[1]/SIRLP[2], UART_GTPR.PSC
 */
void uart_config_irda(uart_num_t uart, const uart_irda_config_t *config);

/**
 * @brief 配置UART智能卡模式
 * 
 * @param uart UART外设编号
 * @param config 智能卡配置结构体指针
 * 
 * @code
 * uart_smartcard_config_t sc_config = {
 *     .enable = true,
 *     .nack_enable = true,
 *     .prescaler = 1,
 *     .guard_time = 16,
 *     .auto_retry = true,
 *     .block_timeout = 0
 * };
 * uart_config_smartcard(UART1, &sc_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.SCEN[5]/HDSEL[0]/CLKEN[7]/NACK[6], UART_CR3.PEN[1], UART_GTPR.PSC/GT, UART_BCNT
 */
void uart_config_smartcard(uart_num_t uart, const uart_smartcard_config_t *config);

/**
 * @brief 配置UART单线半双工模式
 * 
 * @param uart UART外设编号
 * @param enable 使能标志
 * 
 * @note 单线模式下，发送和接收都使用TX引脚
 * @note 覆盖寄存器：UART_CR2.HDSEL
 * 
 * @code
 * uart_config_half_duplex(UART1, true);
 * @endcode
 */
void uart_config_half_duplex(uart_num_t uart, bool enable);

/**
 * @brief 配置UART多机通信模式
 * 
 * @param uart UART外设编号
 * @param config 多机通信配置结构体指针
 * 
 * @code
 * uart_multiprocessor_config_t mp_config = {
 *     .enable = true,
 *     .wakeup_method = UART_WAKEUP_ADDRESS,
 *     .address_mode = UART_ADDRESS_MODE_4BIT,
 *     .address = 0x05
 * };
 * uart_config_multiprocessor(UART1, &mp_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.WAKE/ADDM7/ADDR
 */
void uart_config_multiprocessor(uart_num_t uart, const uart_multiprocessor_config_t *config);

/**
 * @brief 配置UART同步模式
 * 
 * @param uart UART外设编号
 * @param config 同步模式配置结构体指针
 * 
 * @code
 * uart_sync_config_t sync_config = {
 *     .enable = true,
 *     .clock_polarity = UART_CLOCK_POLARITY_LOW,
 *     .clock_phase = UART_CLOCK_PHASE_1ST_EDGE,
 *     .last_bit_clock = true
 * };
 * uart_config_sync(UART1, &sync_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.CLKEN[7]/CPOL[10]/CPHA[9]/LBCL[8]
 */
void uart_config_sync(uart_num_t uart, const uart_sync_config_t *config);

/**
 * @brief 配置RS485模式
 * 
 * @param uart UART外设编号
 * @param config RS485配置结构体指针
 * 
 * @code
 * uart_rs485_config_t rs485_config = {
 *     .enable = true,
 *     .de_polarity = UART_DE_POLARITY_HIGH,
 *     .de_assert = 1,
 *     .de_deassert = 1
 * };
 * uart_config_rs485(UART1, &rs485_config);
 * @endcode
 * @note 覆盖寄存器：UART_BCNT.DEM/DEP, UART_BCNT.BCNT_VALUE
 */
void uart_config_rs485(uart_num_t uart, const uart_rs485_config_t *config);

//===========================================
// 第3层：高级功能API（DMA、中断）
//===========================================

/**
 * @brief 配置UART DMA
 * 
 * @param uart UART外设编号
 * @param config DMA配置结构体指针
 * 
 * @code
 * uart_dma_config_t dma_config = {
 *     .tx_dma_enable = true,
 *     .rx_dma_enable = true,
 *     .dma_disable_on_error = false
 * };
 * uart_config_dma(UART1, &dma_config);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.TXDMAE[4]/RXDMAE[3]/DMAONERR[5]
 */
void uart_config_dma(uart_num_t uart, const uart_dma_config_t *config);

/**
 * @brief 使能UART中断
 * 
 * @param uart UART外设编号
 * @param interrupt_mask 中断类型掩码（可使用位或组合多个中断）
 * 
 * @code
 * uart_interrupt_enable(UART1, UART_INT_RX | UART_INT_IDLE);
 * @endcode
 * @note 覆盖寄存器：UART_IE
 */
void uart_interrupt_enable(uart_num_t uart, uint32_t interrupt_mask);

/**
 * @brief 禁用UART中断
 * 
 * @param uart UART外设编号
 * @param interrupt_mask 中断类型掩码（可使用位或组合多个中断）
 * 
 * @code
 * uart_interrupt_disable(UART1, UART_INT_RX);
 * @endcode
 * @note 覆盖寄存器：UART_IE
 */
void uart_interrupt_disable(uart_num_t uart, uint32_t interrupt_mask);

/**
 * @brief 注册UART接收回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_rx_handler(uart_num_t uart, uint8_t data) {
 *     // 处理接收数据
 * }
 * uart_register_rx_callback(UART1, my_rx_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_rx_callback(uart_num_t uart, uart_rx_callback_t callback);

/**
 * @brief 注册UART发送完成回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_tx_handler(uart_num_t uart) {
 *     // 处理发送完成
 * }
 * uart_register_tx_callback(UART1, my_tx_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_tx_callback(uart_num_t uart, uart_tx_callback_t callback);

/**
 * @brief 注册UART错误回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_error_handler(uart_num_t uart, uart_error_t error) {
 *     // 处理错误
 * }
 * uart_register_error_callback(UART1, my_error_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_error_callback(uart_num_t uart, uart_error_callback_t callback);

/**
 * @brief 注册UART空闲回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_idle_handler(uart_num_t uart) {
 *     // 处理空闲事件
 * }
 * uart_register_idle_callback(UART1, my_idle_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_idle_callback(uart_num_t uart, uart_idle_callback_t callback);

/**
 * @brief 注册UART发送完成回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_tc_handler(uart_num_t uart) {
 *     // 处理发送完成事件
 * }
 * uart_register_tc_callback(UART1, my_tc_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_tc_callback(uart_num_t uart, uart_tc_callback_t callback);

/**
 * @brief 注册UART LIN Break检测回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_lbd_handler(uart_num_t uart) {
 *     // 处理LIN Break检测事件
 * }
 * uart_register_lbd_callback(UART1, my_lbd_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_lbd_callback(uart_num_t uart, uart_lbd_callback_t callback);

/**
 * @brief 注册UART比特计数超时回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_bcnt_handler(uart_num_t uart) {
 *     // 处理比特计数超时事件
 * }
 * uart_register_bcnt_callback(UART1, my_bcnt_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_bcnt_callback(uart_num_t uart, uart_bcnt_callback_t callback);

/**
 * @brief 注册UART波特率自适应完成回调函数
 * 
 * @param uart UART外设编号
 * @param callback 回调函数指针
 * 
 * @code
 * void my_abr_handler(uart_num_t uart) {
 *     // 处理波特率自适应完成事件
 * }
 * uart_register_abr_callback(UART1, my_abr_handler);
 * @endcode
 * @note 覆盖寄存器：无（仅软件回调）
 */
void uart_register_abr_callback(uart_num_t uart, uart_abr_callback_t callback);

//===========================================
// 第4层：控制与查询API
//===========================================

/**
 * @brief 使能UART外设时钟
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_enable_clock(UART1);
 * @endcode
 * @note 覆盖寄存器：时钟控制（无UART寄存器）
 */
void uart_enable_clock(uart_num_t uart);

/**
 * @brief 禁用UART外设时钟
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_disable_clock(UART1);
 * @endcode
 * @note 覆盖寄存器：时钟控制（无UART寄存器）
 */
void uart_disable_clock(uart_num_t uart);

/**
 * @brief 使能UART
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_enable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.UARTEN
 */
void uart_enable(uart_num_t uart);

/**
 * @brief 禁用UART
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_disable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.UARTEN
 */
void uart_disable(uart_num_t uart);

/**
 * @brief 使能UART发送
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_tx_enable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.TXE[8]
 */
void uart_tx_enable(uart_num_t uart);

/**
 * @brief 禁用UART发送
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_tx_disable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.TXE[8]
 */
void uart_tx_disable(uart_num_t uart);

/**
 * @brief 使能UART接收
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_rx_enable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.RXE[9]
 */
void uart_rx_enable(uart_num_t uart);

/**
 * @brief 禁用UART接收
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_rx_disable(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR1.RXE[9]
 */
void uart_rx_disable(uart_num_t uart);

/**
 * @brief 设置RTS输出状态（手动控制）
 * 
 * @param uart UART外设编号
 * @param state RTS引脚状态（true=低电平/有效，false=高电平/无效）
 * 
 * @note 仅在未使能RTS硬件流控时有效
 * @note 覆盖寄存器：UART_CR1.RTS（通过写入控制）
 * 
 * @code
 * uart_set_rts(UART1, true);  // RTS拉低，表示准备接收
 * @endcode
 */
void uart_set_rts(uart_num_t uart, bool state);

/**
 * @brief 获取CTS输入状态
 * 
 * @param uart UART外设编号
 * 
 * @return CTS引脚状态（true=低电平/有效，false=高电平/无效）
 * 
 * @code
 * if (uart_get_cts(UART1)) {
 *     // CTS有效，可以发送
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.CTS
 */
bool uart_get_cts(uart_num_t uart);

/**
 * @brief 发送单个字符
 * 
 * @param uart UART外设编号
 * @param data 要发送的字符
 * 
 * @note 如果FIFO满或发送寄存器忙，函数会等待
 * @note 覆盖寄存器：UART_DR
 * 
 * @code
 * uart_putc(UART1, 'A');
 * @endcode
 */
bool uart_putc(uart_num_t uart, uint8_t data);

/**
 * @brief 接收单个字符
 * 
 * @param uart UART外设编号
 * 
 * @return 接收到的字符
 * 
 * @note 如果FIFO空或接收寄存器无数据，函数会等待
 * @note 覆盖寄存器：UART_DR
 * 
 * @code
 * uint8_t ch = uart_getc(UART1);
 * @endcode
 */
uint8_t uart_getc(uart_num_t uart);

/**
 * @brief 发送字符串
 * 
 * @param uart UART外设编号
 * @param str 要发送的字符串（以'\0'结尾）
 * 
 * @code
 * uart_puts(UART1, "Hello World\r\n");
 * @endcode
 * @note 覆盖寄存器：UART_DR
 */
void uart_puts(uart_num_t uart, const char *str);

/**
 * @brief 发送数据缓冲区
 * 
 * @param uart UART外设编号
 * @param data 数据缓冲区指针
 * @param length 数据长度
 * 
 * @return 实际发送的字节数
 * 
 * @code
 * uint8_t buffer[10] = {0x01, 0x02, 0x03};
 * uart_write(UART1, buffer, 10);
 * @endcode
 * @note 覆盖寄存器：UART_DR
 */
uint32_t uart_write(uart_num_t uart, const uint8_t *data, uint32_t length);

/**
 * @brief 接收数据到缓冲区
 * 
 * @param uart UART外设编号
 * @param buffer 接收缓冲区指针
 * @param length 期望接收的字节数
 * 
 * @return 实际接收的字节数
 * 
 * @code
 * uint8_t buffer[10];
 * uint32_t received = uart_read(UART1, buffer, 10);
 * @endcode
 * @note 覆盖寄存器：UART_DR
 */
uint32_t uart_read(uart_num_t uart, uint8_t *buffer, uint32_t length);

/**
 * @brief 尝试发送单个字符（非阻塞）
 * 
 * @param uart UART外设编号
 * @param data 要发送的字符
 * 
 * @return 成功返回true，FIFO满或发送寄存器忙返回false
 * 
 * @code
 * if (uart_putc_try(UART1, 'A')) {
 *     // 发送成功
 * }
 * @endcode
 * @note 覆盖寄存器：UART_DR
 */
bool uart_putc_try(uart_num_t uart, uint8_t data);

/**
 * @brief 尝试接收单个字符（非阻塞）
 * 
 * @param uart UART外设编号
 * @param data 接收数据存储指针
 * 
 * @return 成功返回true，FIFO空或接收寄存器无数据返回false
 * 
 * @code
 * uint8_t ch;
 * if (uart_getc_try(UART1, &ch)) {
 *     // 接收成功，处理数据
 * }
 * @endcode
 * @note 覆盖寄存器：UART_DR
 */
bool uart_getc_try(uart_num_t uart, uint8_t *data);

/**
 * @brief 检查发送FIFO是否为空
 * 
 * @param uart UART外设编号
 * 
 * @return FIFO为空返回true，否则返回false
 * 
 * @code
 * if (uart_is_tx_fifo_empty(UART1)) {
 *     // FIFO为空
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.TXFE
 */
bool uart_is_tx_fifo_empty(uart_num_t uart);

/**
 * @brief 检查发送FIFO是否已满
 * 
 * @param uart UART外设编号
 * 
 * @return FIFO已满返回true，否则返回false
 * 
 * @code
 * if (!uart_is_tx_fifo_full(UART1)) {
 *     // FIFO未满，可以写入
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.TXFF
 */
bool uart_is_tx_fifo_full(uart_num_t uart);

/**
 * @brief 检查接收FIFO是否为空
 * 
 * @param uart UART外设编号
 * 
 * @return FIFO为空返回true，否则返回false
 * 
 * @code
 * if (!uart_is_rx_fifo_empty(UART1)) {
 *     // FIFO非空，有数据可读
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.RXFE
 */
bool uart_is_rx_fifo_empty(uart_num_t uart);

/**
 * @brief 检查接收FIFO是否已满
 * 
 * @param uart UART外设编号
 * 
 * @return FIFO已满返回true，否则返回false
 * 
 * @code
 * if (uart_is_rx_fifo_full(UART1)) {
 *     // FIFO已满
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.RXFF
 */
bool uart_is_rx_fifo_full(uart_num_t uart);

/**
 * @brief 检查UART是否正在发送
 * 
 * @param uart UART外设编号
 * 
 * @return 正在发送返回true，空闲返回false
 * 
 * @code
 * while (uart_is_busy(UART1)) {
 *     // 等待发送完成
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.BUSY
 */
bool uart_is_busy(uart_num_t uart);

/**
 * @brief 获取UART中断状态
 * 
 * @param uart UART外设编号
 * 
 * @return 中断状态寄存器值
 * 
 * @code
 * uint32_t status = uart_get_interrupt_status(UART1);
 * if (status & UART_INT_RX) {
 *     // 接收中断产生
 * }
 * @endcode
 * @note 覆盖寄存器：UART_ISR
 */
uint32_t uart_get_interrupt_status(uart_num_t uart);

/**
 * @brief 清除UART中断标志
 * 
 * @param uart UART外设编号
 * @param interrupt_mask 中断类型掩码（可使用位或组合多个中断）
 * 
 * @code
 * uart_clear_interrupt(UART1, UART_INT_RX | UART_INT_IDLE);
 * @endcode
 * @note 覆盖寄存器：UART_ISR
 */
void uart_clear_interrupt(uart_num_t uart, uint32_t interrupt_mask);

/**
 * @brief 获取UART错误状态
 * 
 * @param uart UART外设编号
 * 
 * @return 错误标志
 * 
 * @code
 * uart_error_t error = uart_get_error(UART1);
 * if (error & UART_ERROR_PE) {
 *     // 校验错误
 * }
 * @endcode
 * @note 覆盖寄存器：UART_FR.FE/PE/BE/OE, UART_ISR.FEI/PEI/BEI/OEI
 */
uart_error_t uart_get_error(uart_num_t uart);

/**
 * @brief 清除UART错误状态
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_clear_error(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_DR, UART_ISR.OEI
 */
void uart_clear_error(uart_num_t uart);

/**
 * @brief 启动波特率自适应
 * 
 * @param uart UART外设编号
 * 
 * @note 需要主机发送0x7F字符，自适应完成后需要软件清除使能位
 * @note 覆盖寄存器：UART_CR2.ABREN[4]
 * 
 * @code
 * uart_autobaud_start(UART1);
 * @endcode
 */
void uart_autobaud_start(uart_num_t uart);

/**
 * @brief 停止波特率自适应
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_autobaud_stop(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.ABREN[4]
 */
void uart_autobaud_stop(uart_num_t uart);

/**
 * @brief 检查波特率自适应是否完成
 * 
 * @param uart UART外设编号
 * 
 * @return 完成返回true，否则返回false
 * 
 * @code
 * if (uart_is_autobaud_complete(UART1)) {
 *     // 自适应完成
 *     uart_autobaud_stop(UART1);
 * }
 * @endcode
 * @note 覆盖寄存器：UART_ISR.ABRI
 */
bool uart_is_autobaud_complete(uart_num_t uart);

/**
 * @brief 进入静默模式（多机通信）
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_enter_mute_mode(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.RWU[2]
 */
void uart_enter_mute_mode(uart_num_t uart);

/**
 * @brief 退出静默模式（多机通信）
 * 
 * @param uart UART外设编号
 * 
 * @code
 * uart_exit_mute_mode(UART1);
 * @endcode
 * @note 覆盖寄存器：UART_CR2.RWU[2]
 */
void uart_exit_mute_mode(uart_num_t uart);

/**
 * @brief 发送LIN Break信号
 * 
 * @param uart UART外设编号
 * @param length Break信号长度（比特数，建议13）
 * 
 * @code
 * uart_send_lin_break(UART1, 13);
 * @endcode
 * @note 覆盖寄存器：UART_BCNT, UART_CR3.BRK[0]
 */
void uart_send_lin_break(uart_num_t uart, uint32_t length);

/**
 * @brief 配置BCNT自动启动使能
 * 
 * @param uart UART外设编号
 * @param enable 使能状态
 * 
 * @note 覆盖寄存器：UART_BCNT.AUTO_START_EN[25]
 */
void uart_bcnt_auto_start_enable(uart_num_t uart, bool enable);

/**
 * @brief 获取UART基地址
 * 
 * @param uart UART外设编号
 * 
 * @return UART寄存器基地址
 * 
 * @note 用于DMA配置等需要访问寄存器地址的场景
 * @note 覆盖寄存器：无（仅返回基地址）
 * 
 * @code
 * uint32_t base = uart_get_base_address(UART_1);
 * @endcode
 */
uint32_t uart_get_base_address(uart_num_t uart);

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief UART中断处理函数
 * 
 * @param uart UART外设编号
 * 
 * @note 用户需要在对应的UART中断服务函数中调用此函数
 * @note 覆盖寄存器：ISR, DR, FR
 * 
 * @code
 * // ISR 已在驱动中定义，用户只需注册回调 + 使能 NVIC：
 * uart_register_rx_callback(UART_1, my_rx_handler);
 * uart_interrupt_enable(UART_1, UART_INT_RX);
 * NVIC_SetPriority(USART1_IRQn, 2U);
 * NVIC_EnableIRQ(USART1_IRQn);
 * @endcode
 */
void uart_irq_handler(uart_num_t uart);

#ifdef __cplusplus
}
#endif

#endif /* _HARDWARE_UART_H */
