/**
 * @file fdcan.h
 * @brief ACM32P4 FDCAN 驱动
 *
 * 支持 CAN2.0B 和 CAN FD 协议，提供完整的 FDCAN 外设控制接口。
 *
 * 功能特性：
 * - CAN2.0B / CAN FD (ISO / Bosch) 双协议
 * - 可配置的标称位时间和数据位时间
 * - 16 组 29 位硬件过滤器
 * - PTB（主发送缓冲）+ STB（次发送缓冲，16 槽）
 * - Single Shot 发送模式
 * - 回环模式（内部/外部）和静默模式
 * - TTCAN (ISO 11898-4) 时间触发通信
 * - CiA 603 时间戳
 * - 发送延迟补偿
 * - 完整的中断和回调机制
 *
 * @note 中断架构：
 *   FDCAN1_IRQn - FDCAN1 全局中断
 *   FDCAN2_IRQn - FDCAN2 全局中断
 *   FDCAN3_IRQn - FDCAN3 全局中断
 *   所有 FDCAN 事件（接收、发送、错误等）共用一条中断线，
 *   在 ISR 中通过查询 IR 寄存器区分事件类型。
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_FDCAN_H
#define _HARDWARE_FDCAN_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define FDCAN_INSTANCE_COUNT      2U       ///< FDCAN外设数量
#define FDCAN_FILTER_COUNT        16U      ///< 过滤器组数量
#define FDCAN_STB_SLOT_COUNT      16U      ///< STB槽数量
#define FDCAN_RX_FIFO_DEPTH       16U      ///< 接收FIFO深度
#define FDCAN_MAX_DATA_LEN        64U      ///< CAN FD最大数据长度
#define FDCAN_CAN20_MAX_DATA_LEN  8U       ///< CAN2.0B最大数据长度
#define FDCAN_MAX_TIMEOUT         0xFFFFFFU ///< 超时最大值

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief FDCAN外设实例
 */
typedef enum {
    FDCAN_INSTANCE_1 = 0,  ///< FDCAN1
    FDCAN_INSTANCE_2 = 1,  ///< FDCAN2
} fdcan_instance_t;

/**
 * @brief CAN FD 协议模式
 */
typedef enum {
    FDCAN_FD_MODE_ISO   = 1,  ///< ISO 11898-1 CAN FD
    FDCAN_FD_MODE_BOSCH = 0,  ///< Bosch CAN FD
} fdcan_fd_mode_t;

/**
 * @brief 帧格式类型
 */
typedef enum {
    FDCAN_FRAME_CLASSIC   = 0,  ///< CAN2.0B 标准帧
    FDCAN_FRAME_FD_NO_BRS = 1,  ///< CAN FD 无速率切换
    FDCAN_FRAME_FD_BRS    = 2,  ///< CAN FD 带速率切换
} fdcan_frame_type_t;

/**
 * @brief 帧 ID 类型
 */
typedef enum {
    FDCAN_ID_STANDARD = 0,  ///< 11位标准ID
    FDCAN_ID_EXTENDED = 1,  ///< 29位扩展ID
} fdcan_id_type_t;

/**
 * @brief 帧类型
 */
typedef enum {
    FDCAN_FRAME_DATA   = 0,  ///< 数据帧
    FDCAN_FRAME_REMOTE = 1,  ///< 远程帧
} fdcan_frame_kind_t;

/**
 * @brief 发送缓冲区选择
 */
typedef enum {
    FDCAN_TX_PTB = 0,  ///< 主发送缓冲（高优先级，单槽）
    FDCAN_TX_STB = 1,  ///< 次发送缓冲（低优先级，16槽）
} fdcan_tx_buffer_t;

/**
 * @brief STB工作模式
 */
typedef enum {
    FDCAN_STB_MODE_FIFO     = 0,  ///< FIFO模式（先入先出）
    FDCAN_STB_MODE_PRIORITY = 1,  ///< 优先级模式（ID低者优先）
} fdcan_stb_mode_t;

/**
 * @brief 回环模式
 */
typedef enum {
    FDCAN_LOOPBACK_NONE     = 0,  ///< 正常模式
    FDCAN_LOOPBACK_INTERNAL = 1,  ///< 内部回环（数据不输出到总线）
    FDCAN_LOOPBACK_EXTERNAL = 2,  ///< 外部回环（需收发器配合）
} fdcan_loopback_mode_t;

/**
 * @brief 接收FIFO溢出模式
 */
typedef enum {
    FDCAN_RX_OVERFLOW_DISCARD_OLD = 0,  ///< 溢出时丢弃最早帧
    FDCAN_RX_OVERFLOW_DISCARD_NEW = 1,  ///< 溢出时丢弃最新帧
} fdcan_rx_overflow_mode_t;

/**
 * @brief 错误类型
 */
typedef enum {
    FDCAN_ERROR_NONE    = 0,  ///< 无错误
    FDCAN_ERROR_BIT     = 1,  ///< 位错误
    FDCAN_ERROR_FORM    = 2,  ///< 格式错误
    FDCAN_ERROR_STUFF   = 3,  ///< 填充错误
    FDCAN_ERROR_ACK     = 4,  ///< 应答错误
    FDCAN_ERROR_CRC     = 5,  ///< CRC错误
    FDCAN_ERROR_OTHER   = 6,  ///< 其他错误
} fdcan_error_type_t;

/**
 * @brief CAN 节点错误状态
 */
typedef enum {
    FDCAN_ERROR_ACTIVE  = 0,  ///< 错误主动
    FDCAN_ERROR_PASSIVE = 1,  ///< 错误被动
    FDCAN_ERROR_BUS_OFF = 2,  ///< 总线关闭
} fdcan_error_state_t;

/**
 * @brief 接收缓冲状态
 */
typedef enum {
    FDCAN_RX_EMPTY    = 0,  ///< 空
    FDCAN_RX_BELOW_AFWL = 1,  ///< 低于快满阈值
    FDCAN_RX_ABOVE_AFWL = 2,  ///< 高于快满阈值但未满
    FDCAN_RX_FULL     = 3,  ///< 已满
} fdcan_rx_status_t;

/**
 * @brief TTCAN 触发类型
 */
typedef enum {
    FDCAN_TT_TRIG_IMMEDIATE    = 0,  ///< 立即触发
    FDCAN_TT_TRIG_TIME         = 1,  ///< 时间触发（仅产生中断）
    FDCAN_TT_TRIG_SINGLE_TX    = 2,  ///< 单槽发送触发
    FDCAN_TT_TRIG_TX_START     = 3,  ///< 发送开始触发
    FDCAN_TT_TRIG_TX_STOP      = 4,  ///< 发送停止触发
} fdcan_ttcan_trigger_type_t;

/**
 * @brief 时间戳采样位置
 */
typedef enum {
    FDCAN_TIMESTAMP_SOF = 0,  ///< 帧起始采样
    FDCAN_TIMESTAMP_EOF = 1,  ///< 帧结束采样
} fdcan_timestamp_pos_t;

/**
 * @brief 过滤器宽度
 */
typedef enum {
    FDCAN_FILTER_32BIT = 0,  ///< 32位宽度
    FDCAN_FILTER_16BIT = 1,  ///< 16位宽度
} fdcan_filter_width_t;

/**
 * @brief FDCAN中断类型（位掩码，可组合）
 */
typedef enum {
    FDCAN_IRQ_RX              = (1U << 7),   ///< 接收中断
    FDCAN_IRQ_RX_OVERFLOW     = (1U << 6),   ///< 接收溢出中断
    FDCAN_IRQ_RX_FULL         = (1U << 5),   ///< 接收FIFO满中断
    FDCAN_IRQ_RX_ALMOST_FULL  = (1U << 4),   ///< 接收FIFO快满中断
    FDCAN_IRQ_TX_PTB          = (1U << 3),   ///< PTB发送完成中断
    FDCAN_IRQ_TX_STB          = (1U << 2),   ///< STB发送完成中断
    FDCAN_IRQ_ERROR           = (1U << 1),   ///< 错误中断
    FDCAN_IRQ_ARBITRATION_LOST = (1U << 19), ///< 仲裁失败中断
    FDCAN_IRQ_BUS_ERROR       = (1U << 17),  ///< 总线错误中断
    FDCAN_IRQ_ERROR_PASSIVE    = (1U << 21), ///< 错误被动中断
    FDCAN_IRQ_ABORT           = (1U << 8),   ///< 取消发送中断
    FDCAN_IRQ_ALL             = 0x003F3FFFU, ///< 所有中断
} fdcan_irq_type_t;

/**
 * @brief FDCAN中断标志（只读状态）
 */
typedef enum {
    FDCAN_FLAG_RX              = (1U << 15),  ///< 接收标志
    FDCAN_FLAG_RX_OVERFLOW     = (1U << 14),  ///< 接收溢出标志
    FDCAN_FLAG_RX_FULL         = (1U << 13),  ///< 接收FIFO满标志
    FDCAN_FLAG_RX_ALMOST_FULL  = (1U << 12),  ///< 接收FIFO快满标志
    FDCAN_FLAG_TX_PTB          = (1U << 11),  ///< PTB发送完成标志
    FDCAN_FLAG_TX_STB          = (1U << 10),  ///< STB发送完成标志
    FDCAN_FLAG_ERROR           = (1U << 9),   ///< 错误标志
    FDCAN_FLAG_ABORT           = (1U << 8),   ///< 取消发送标志
    FDCAN_FLAG_BUS_ERROR       = (1U << 16),  ///< 总线错误标志
    FDCAN_FLAG_ARBITRATION_LOST = (1U << 18), ///< 仲裁失败标志
    FDCAN_FLAG_ERROR_PASSIVE    = (1U << 20), ///< 错误被动标志
    FDCAN_FLAG_ERROR_WARNING   = (1U << 23),  ///< 错误警告标志
    FDCAN_FLAG_STB_FULL        = (1U << 0),   ///< STB满标志
} fdcan_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief FDCAN位时间配置
 */
typedef struct {
    uint8_t prescaler;   ///< 预分频值 (0-255)，实际分频 = prescaler + 1
    uint8_t seg1;        ///< 时间段1 (1-255)，TQ数 = seg1 + 2
    uint8_t seg2;        ///< 时间段2 (1-127)，TQ数 = seg2 + 1
    uint8_t sjw;         ///< 同步跳转宽度 (0-127)，TQ数 = sjw + 1
} fdcan_bit_timing_config_t;

/**
 * @brief FDCAN帧结构
 */
typedef struct {
    uint32_t id;                ///< 帧ID（标准11位或扩展29位）
    fdcan_id_type_t id_type;    ///< ID类型
    fdcan_frame_kind_t kind;    ///< 帧类型（数据帧/远程帧）
    fdcan_frame_type_t frame_type; ///< 帧格式（CAN2.0 / CAN FD / CAN FD BRS）
    uint8_t dlc;                ///< 数据长度码 (0-15)
    uint8_t data_len;           ///< 实际数据字节数 (0-64)，由dlc自动换算
    uint8_t data[FDCAN_MAX_DATA_LEN]; ///< 数据字节
    bool esi;                   ///< 错误状态指示（仅CAN FD接收）
    uint16_t cycle_time;        ///< TTCAN周期时间（接收时有效）
    uint64_t timestamp;         ///< CiA 603时间戳（64位，接收时有效）
    uint8_t error_code;         ///< 错误码（接收时有效）
} fdcan_frame_t;

/**
 * @brief FDCAN 过滤器配置
 */
typedef struct {
    fdcan_filter_width_t width;  ///< 过滤器宽度（16位或32位）
    bool mask_enabled;           ///< 屏蔽模式使能（true=使用掩码，false=不使用掩码）
    uint32_t code;               ///< ID CODE值
    uint32_t mask;               ///< ID MASK值（仅mask_enabled=true时有效）
    bool enabled;                ///< 过滤器使能
} fdcan_filter_config_t;

/**
 * @brief FDCAN 全局过滤配置
 */
typedef struct {
    bool accept_all;          ///< 接收所有帧（包括错误帧）
    fdcan_rx_overflow_mode_t overflow_mode; ///< FIFO溢出模式
    struct {
        uint8_t afwl;         ///< 接收FIFO快满阈值 (0-15)
        uint8_t ewl;          ///< 错误警告阈值 (0-15)，阈值 = (ewl + 1) * 8
    } limit;                  ///< 阈值配置
} fdcan_global_filter_config_t;

/**
 * @brief TTCAN配置
 */
typedef struct {
    bool ttcan_enabled;        ///< TTCAN使能
    bool ttcan_tbm_mode;       ///< TTCAN发送缓冲模式（true=组合模式，false=事件驱动+时间戳）
    uint8_t time_prescaler;    ///< 时间预分频 (0-3)：0=1分频，1=2分频，2=4分频，3=8分频
    uint32_t ref_id;           ///< 参考消息ID
    bool ref_ide;              ///< 参考消息IDE位
} fdcan_ttcan_config_t;

/**
 * @brief TTCAN触发配置
 */
typedef struct {
    fdcan_ttcan_trigger_type_t type; ///< 触发类型
    uint8_t slot;               ///< 目标TB槽位置（0=PTB, 1-16=STB槽）
    uint16_t trigger_time;      ///< 触发周期时间
    uint8_t tx_enable_window;   ///< 发送使能窗口 (1-16)，仅单槽发送触发有效
} fdcan_ttcan_trigger_config_t;

/**
 * @brief 时间戳配置
 */
typedef struct {
    bool enabled;               ///< 时间戳使能
    fdcan_timestamp_pos_t position; ///< 时间戳采样位置（SOF 或 EOF）
} fdcan_timestamp_config_t;

/**
 * @brief FDCAN错误计数器
 */
typedef struct {
    uint8_t tecnt;        ///< 发送错误计数
    uint8_t recnt;        ///< 接收错误计数
} fdcan_error_counters_t;

/**
 * @brief FDCAN回调函数类型
 * @param inst FDCAN实例
 * @param flags 触发回调的中断标志组合
 */
typedef void (*fdcan_callback_t)(fdcan_instance_t inst, uint32_t flags);

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief FDCAN默认初始化（CAN2.0B，500Kbps，APB时钟=110MHz）
 *
 * @param inst FDCAN外设实例
 * @return true: 成功, false: 失败
 *
 * @note 默认配置：CAN2.0B模式、500Kbps、正常模式、滤波器0接收所有帧
 *
 * @code
 * clock_periph_enable(CLK_FDCAN1);
 * fdcan_init_default(FDCAN_INSTANCE_1);
 * @endcode
 */
bool fdcan_init_default(fdcan_instance_t inst);

/**
 * @brief FDCAN CAN2.0B 快速初始化
 *
 * @param inst        FDCAN外设实例
 * @param baud_rate   波特率 (Hz)，如 500000 表示 500Kbps
 * @param pclk_hz     APB总线时钟频率 (Hz)
 * @return true: 成功, false: 失败
 *
 * @note 自动计算最优位时间配置（采样点约80%）
 *
 * @code
 * fdcan_init_can20(FDCAN_INSTANCE_1, 500000, 110000000);
 * @endcode
 */
bool fdcan_init_can20(fdcan_instance_t inst, uint32_t baud_rate, uint32_t pclk_hz);

/**
 * @brief FDCAN CAN FD 快速初始化
 *
 * @param inst          FDCAN外设实例
 * @param nom_baud      标称波特率 (Hz)
 * @param data_baud     数据波特率 (Hz)
 * @param pclk_hz       APB总线时钟频率 (Hz)
 * @param fd_mode       FD协议模式（ISO或Bosch）
 * @return true: 成功, false: 失败
 *
 * @note 自动计算标称位时间和数据位时间的最优配置
 *
 * @code
 * fdcan_init_canfd(FDCAN_INSTANCE_1, 500000, 4000000, 110000000, FDCAN_FD_MODE_ISO);
 * @endcode
 */
bool fdcan_init_canfd(fdcan_instance_t inst, uint32_t nom_baud, uint32_t data_baud,
                      uint32_t pclk_hz, fdcan_fd_mode_t fd_mode);

/**
 * @brief FDCAN反初始化，复位外设并禁用时钟
 *
 * @param inst FDCAN外设实例
 *
 */
void fdcan_deinit(fdcan_instance_t inst);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 配置标称位时间（仲裁场和控制场速率）
 *
 * @param inst    FDCAN外设实例
 * @param config  位时间配置结构体
 * @return true: 成功, false: 参数错误
 *
 * @note 计算公式：
 *       TQ = (prescaler + 1) / pclk
 *       标称位时间 = (seg1 + 2 + seg2 + 1) × TQ
 *       采样点 = (seg1 + 2) / (seg1 + seg2 + 3)
 * @note 仅在 CR.RESET=1 时可配置
 */
bool fdcan_config_nominal_bit_timing(fdcan_instance_t inst,
                                     const fdcan_bit_timing_config_t *config);

/**
 * @brief 配置数据位时间（CAN FD 数据场速率）
 *
 * @param inst    FDCAN外设实例
 * @param config  位时间配置结构体
 * @return true: 成功, false: 参数错误
 *
 * @note 仅在 CR.RESET=1 时可配置
 */
bool fdcan_config_data_bit_timing(fdcan_instance_t inst,
                                  const fdcan_bit_timing_config_t *config);

/**
 * @brief 配置单组硬件过滤器
 *
 * @param inst          FDCAN外设实例
 * @param filter_index  过滤器索引 (0-15)
 * @param config        过滤器配置结构体
 * @return true: 成功, false: 参数错误
 *
 * @note 仅在 CR.RESET=1 时可配置
 */
bool fdcan_config_filter(fdcan_instance_t inst, uint8_t filter_index,
                         const fdcan_filter_config_t *config);

/**
 * @brief 配置全局过滤参数
 *
 * @param inst    FDCAN外设实例
 * @param config  全局过滤配置结构体
 *
 */
void fdcan_config_global_filter(fdcan_instance_t inst,
                                const fdcan_global_filter_config_t *config);

/**
 * @brief 设置 FD 协议模式（ISO 或 Bosch）
 *
 * @param inst  FDCAN外设实例
 * @param mode  FD协议模式
 *
 */
void fdcan_set_fd_mode(fdcan_instance_t inst, fdcan_fd_mode_t mode);

/**
 * @brief 配置自动重发
 *
 * @param inst    FDCAN外设实例
 * @param buf     目标发送缓冲（PTB或STB）
 * @param enable  true=自动重发（默认），false=Single Shot模式
 *
 */
void fdcan_config_auto_retransmit(fdcan_instance_t inst, fdcan_tx_buffer_t buf,
                                  bool enable);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 配置回环模式
 *
 * @param inst  FDCAN外设实例
 * @param mode  回环模式
 *
 * @note 从回环模式返回正常模式时需调用 fdcan_reset()
 * @note 外部回环模式依赖外部收发器
 */
void fdcan_config_loopback(fdcan_instance_t inst, fdcan_loopback_mode_t mode);

/**
 * @brief 配置静默模式（只监听不发送）
 *
 * @param inst    FDCAN外设实例
 * @param enable  true=静默模式, false=正常模式
 *
 * @note 静默模式可与外部回环组合使用
 */
void fdcan_config_silent_mode(fdcan_instance_t inst, bool enable);

/**
 * @brief 配置自我应答（外部回环模式下的自应答）
 *
 * @param inst    FDCAN外设实例
 * @param enable  true=使能自我应答, false=禁止
 *
 * @note 仅在外部回环模式 (LBME=1) 时有效
 */
void fdcan_config_self_ack(fdcan_instance_t inst, bool enable);

/**
 * @brief 配置发送延迟补偿（CAN FD 高速模式必需）
 *
 * @param inst       FDCAN外设实例
 * @param enable     true=使能补偿, false=禁止
 * @param ssp_offset 次级采样点偏移 (0-127)
 *
 * @note 数据波特率 > 1Mbps 时推荐使能
 */
void fdcan_config_tx_delay_compensation(fdcan_instance_t inst, bool enable,
                                        uint8_t ssp_offset);

/**
 * @brief 配置时间戳功能
 *
 * @param inst    FDCAN外设实例
 * @param config  时间戳配置结构体
 *
 */
void fdcan_config_timestamp(fdcan_instance_t inst,
                            const fdcan_timestamp_config_t *config);

/**
 * @brief 获取当前16位周期时间戳（TTCAN）
 *
 * @param inst  FDCAN外设实例
 * @return 16位周期时间戳
 *
 * @note 仅在 TTEN=1 时有效
 */
uint16_t fdcan_get_cycle_time(fdcan_instance_t inst);

/**
 * @brief 配置 TTCAN 模式
 *
 * @param inst    FDCAN外设实例
 * @param config  TTCAN配置结构体
 *
 */
void fdcan_config_ttcan(fdcan_instance_t inst, const fdcan_ttcan_config_t *config);

/**
 * @brief 配置 TTCAN 触发条件
 *
 * @param inst    FDCAN外设实例
 * @param config  触发配置结构体
 *
 */
void fdcan_config_ttcan_trigger(fdcan_instance_t inst,
                                const fdcan_ttcan_trigger_config_t *config);

/**
 * @brief 配置 TTCAN 看门狗触发时间
 *
 * @param inst         FDCAN外设实例
 * @param trigger_time 看门狗触发周期时间
 *
 * @note 设为0则禁用看门狗触发
 */
void fdcan_config_ttcan_watchdog(fdcan_instance_t inst, uint16_t trigger_time);

/**
 * @brief 设置 TTBUF 槽的空/满状态（TTCAN模式）
 *
 * @param inst  FDCAN外设实例
 * @param slot  槽位置（0=PTB, 1-16=STB槽）
 * @param full  true=标记为满, false=标记为空
 *
 */
void fdcan_set_ttbuf_slot_status(fdcan_instance_t inst, uint8_t slot, bool full);

/**
 * @brief 设置 STB 工作模式
 *
 * @param inst  FDCAN外设实例
 * @param mode  STB工作模式（FIFO或优先级）
 *
 */
void fdcan_set_stb_mode(fdcan_instance_t inst, fdcan_stb_mode_t mode);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 复位FDCAN控制器
 *
 * @param inst FDCAN外设实例
 *
 * @note RESET=1 时才能修改 BTR、PSCR、ACFCR 等寄存器
 * @note 将 RESET 清0后需等待11个位时间才能参与总线通信
 */
void fdcan_reset(fdcan_instance_t inst);

/**
 * @brief 释放FDCAN复位（启动CAN通信）
 *
 * @param inst FDCAN外设实例
 *
 */
void fdcan_start(fdcan_instance_t inst);

/**
 * @brief 通过PTB发送一帧数据
 *
 * @param inst   FDCAN外设实例
 * @param frame  待发送的帧数据
 * @return true: 发送请求已提交, false: PTB忙或参数错误
 *
 * @note PTB仅有一槽，发送前需检查TPIF或等待完成
 */
bool fdcan_send_ptb(fdcan_instance_t inst, const fdcan_frame_t *frame);

/**
 * @brief 向STB写入一帧数据并请求发送
 *
 * @param inst   FDCAN外设实例
 * @param frame  待发送的帧数据
 * @return true: 写入成功, false: STB满或参数错误
 *
 * @note 写入后自动置位 TSNEXT
 */
bool fdcan_send_stb(fdcan_instance_t inst, const fdcan_frame_t *frame);

/**
 * @brief 请求发送STB中最早的一帧
 *
 * @param inst FDCAN外设实例
 *
 */
void fdcan_transmit_stb_one(fdcan_instance_t inst);

/**
 * @brief 请求发送STB中的所有帧
 *
 * @param inst FDCAN外设实例
 *
 */
void fdcan_transmit_stb_all(fdcan_instance_t inst);

/**
 * @brief 取消PTB发送请求
 *
 * @param inst FDCAN外设实例
 *
 * @note 仅取消尚未实际开始发送的帧
 */
void fdcan_abort_ptb(fdcan_instance_t inst);

/**
 * @brief 取消STB发送请求
 *
 * @param inst FDCAN外设实例
 *
 * @note TSALL发送时取消所有未开始帧，TSONE发送时仅取消一帧
 */
void fdcan_abort_stb(fdcan_instance_t inst);

/**
 * @brief 从接收FIFO读取一帧数据
 *
 * @param inst   FDCAN外设实例
 * @param frame  输出参数，接收到的帧数据
 * @return true: 成功读取, false: 接收FIFO为空
 *
 * @note 读取后应调用 fdcan_release_rx_buffer() 释放该槽
 */
bool fdcan_receive(fdcan_instance_t inst, fdcan_frame_t *frame);

/**
 * @brief 检查接收FIFO中是否有可用帧
 *
 * @param inst FDCAN外设实例
 * @return true: 有可用帧, false: 接收FIFO为空
 */
bool fdcan_is_frame_available(fdcan_instance_t inst);

/**
 * @brief 释放接收FIFO的一个槽（读取后必须调用）
 *
 * @param inst FDCAN外设实例
 *
 */
void fdcan_release_rx_buffer(fdcan_instance_t inst);

/**
 * @brief 设置待机模式（控制外部收发器进入低功耗）
 *
 * @param inst    FDCAN外设实例
 * @param enable  true=待机, false=正常
 *
 * @note 此位连接到外部收发器的STBY引脚
 */
void fdcan_set_standby(fdcan_instance_t inst, bool enable);

/**
 * @brief 检查总线是否处于Bus Off状态
 *
 * @param inst FDCAN外设实例
 * @return true: Bus Off, false: 正常
 */
bool fdcan_is_bus_off(fdcan_instance_t inst);

/**
 * @brief 获取错误计数器
 *
 * @param inst      FDCAN外设实例
 * @param counters  输出参数，错误计数值
 */
void fdcan_get_error_counters(fdcan_instance_t inst, fdcan_error_counters_t *counters);

/**
 * @brief 获取最近一次错误类型
 *
 * @param inst FDCAN外设实例
 * @return 错误类型枚举值
 */
fdcan_error_type_t fdcan_get_error_type(fdcan_instance_t inst);

/**
 * @brief 获取仲裁丢失位置
 *
 * @param inst FDCAN外设实例
 * @return 仲裁丢失位位置 (0-31)，0=SOF后的第一个ID位
 */
uint8_t fdcan_get_arbitration_lost_pos(fdcan_instance_t inst);

/**
 * @brief 获取接收缓冲状态
 *
 * @param inst FDCAN外设实例
 * @return 接收缓冲状态
 */
fdcan_rx_status_t fdcan_get_rx_status(fdcan_instance_t inst);

/**
 * @brief 获取接收FIFO中已存放的帧数
 *
 * @param inst FDCAN外设实例
 * @return 帧数 (0-16)
 */
uint8_t fdcan_get_rx_count(fdcan_instance_t inst);

/**
 * @brief 获取STB中已存放的帧数
 *
 * @param inst FDCAN外设实例
 * @return 帧数 (0-16)
 */
uint8_t fdcan_get_stb_count(fdcan_instance_t inst);

/**
 * @brief 检查接收FIFO是否溢出
 *
 * @param inst FDCAN外设实例
 * @return true: 溢出, false: 正常
 */
bool fdcan_is_rx_overflow(fdcan_instance_t inst);

/**
 * @brief 检查STB是否已满
 *
 * @param inst FDCAN外设实例
 * @return true: 已满, false: 未满
 */
bool fdcan_is_stb_full(fdcan_instance_t inst);

//===========================================
// 中断管理 API
//===========================================

/**
 * @brief 使能指定的FDCAN中断
 *
 * @param inst  FDCAN外设实例
 * @param irqs  中断类型（可用 | 组合多个）
 *
 */
void fdcan_irq_enable(fdcan_instance_t inst, uint32_t irqs);

/**
 * @brief 禁用指定的FDCAN中断
 *
 * @param inst  FDCAN外设实例
 * @param irqs  中断类型（可用 | 组合多个）
 *
 */
void fdcan_irq_disable(fdcan_instance_t inst, uint32_t irqs);

/**
 * @brief 获取当前中断标志状态
 *
 * @param inst FDCAN外设实例
 * @return 中断标志位组合
 */
uint32_t fdcan_get_irq_status(fdcan_instance_t inst);

/**
 * @brief 清除指定的中断标志
 *
 * @param inst   FDCAN外设实例
 * @param flags  要清除的中断标志（可用 | 组合多个）
 *
 * @note 对中断标志位写1清零
 */
void fdcan_clear_irq_flags(fdcan_instance_t inst, uint32_t flags);

/**
 * @brief 获取当前错误状态
 *
 * @param inst FDCAN外设实例
 * @return 错误状态枚举值
 */
fdcan_error_state_t fdcan_get_error_state(fdcan_instance_t inst);

//===========================================
// 回调管理 API
//===========================================

/**
 * @brief 注册FDCAN事件回调函数
 *
 * @param inst      FDCAN外设实例
 * @param callback  回调函数（NULL则取消注册）
 *
 * @note 在FDCAN中断处理函数中调用此回调
 * @note 回调上下文中传入触发事件的标志位组合
 */
void fdcan_register_callback(fdcan_instance_t inst, fdcan_callback_t callback);

//===========================================
// 查询 API
//===========================================

/**
 * @brief 获取FDCAN外设基地址
 *
 * @param inst FDCAN外设实例
 * @return 外设基地址指针
 */
volatile void *fdcan_get_base(fdcan_instance_t inst);

/**
 * @brief 检查FDCAN是否正在发送数据
 *
 * @param inst FDCAN外设实例
 * @return true: 正在发送, false: 空闲
 */
bool fdcan_is_transmitting(fdcan_instance_t inst);

/**
 * @brief 检查FDCAN是否正在接收数据
 *
 * @param inst FDCAN外设实例
 * @return true: 正在接收, false: 空闲
 */
bool fdcan_is_receiving(fdcan_instance_t inst);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_FDCAN_H
