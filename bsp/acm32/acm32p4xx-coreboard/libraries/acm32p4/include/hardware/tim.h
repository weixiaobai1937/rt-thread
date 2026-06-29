/**
 * @file tim.h
 * @brief ACM32P4 TIM驱动 - 通用定时器/高级定时器驱动接口
 * 
 * @details
 * 本驱动支持ACM32P4系列芯片的所有定时器功能：
 * - 高级定时器（TIM1/8/20）：互补输出、死区、刹车
 * - 通用定时器（TIM2-5/9-14/15-19/23-25）：PWM、输入捕获、编码器
 * - 基本定时器（TIM6/7/21/22）：基础定时功能
 * 
 * 特性：
 * - ✅ 4层API架构（快速初始化 → 基础配置 → 高级功能 → 控制查询）
 * - ✅ 类型安全（枚举类型，无魔法数字）
 * - ✅ 中断/DMA支持
 * - ✅ 主从同步机制
 * 
 * @example
 * @code
 * // 快速PWM初始化（第1层API）
 * tim_init_pwm(TIM_1, TIM_CH1, PB0, 10000, 50);
 * 
 * // 完整配置（第2层API）
 * tim_timebase_config_t tb = {
 *     .prescaler = 239,
 *     .period = 9999,
 *     .count_mode = TIM_COUNT_UP,
 *     .clock_division = TIM_CKD_DIV1,
 *     .repetition_counter = 0
 * };
 * tim_config_timebase(TIM_1, &tb);
 * @endcode
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 * @date 2026-02-11
 */

#ifndef ACM32P4_TIM_H
#define ACM32P4_TIM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 1. 类型定义
//=============================================================================

/**
 * @brief TIM实例枚举
 */
typedef enum {
    // 高级定时器（2个）
    TIM_1  = 0,     ///< TIM1 (Advanced, 16-bit, 4CH + 2内部CH)
    TIM_8  = 1,     ///< TIM8 (Advanced, 16-bit, 4CH + 2内部CH)
    
    // 通用定时器 - 32位（2个）
    TIM_2  = 2,     ///< TIM2 (General, 32-bit, 4CH, Encoder)
    TIM_5  = 3,     ///< TIM5 (General, 32-bit, 4CH, Encoder)
    
    // 通用定时器 - 16位全功能（2个）
    TIM_3  = 4,     ///< TIM3 (General, 16-bit, 4CH, Encoder)
    TIM_4  = 5,     ///< TIM4 (General, 16-bit, 4CH, Encoder)
    
    // 通用定时器 - 16位基础（2个）
    TIM_9  = 6,     ///< TIM9 (General, 16-bit, 2CH)
    TIM_10 = 7,     ///< TIM10 (General, 16-bit, 1CH)
    
    // 基本定时器（2个）
    TIM_6  = 8,     ///< TIM6 (Basic, 16-bit)
    TIM_7  = 9,     ///< TIM7 (Basic, 16-bit)
    
    // 特殊定时器（1个）
    TIM_26 = 10,    ///< TIM26 (Special)
    
    TIM_COUNT = 11  ///< 定时器总数
} tim_instance_t;

/**
 * @brief TIM通道枚举
 */
typedef enum {
    TIM_CH1   = 0,  ///< 通道1
    TIM_CH2   = 1,  ///< 通道2
    TIM_CH3   = 2,  ///< 通道3
    TIM_CH4   = 3,  ///< 通道4
    TIM_CH5   = 4,  ///< 通道5（仅TIM1/8/20内部通道）
    TIM_CH6   = 5,  ///< 通道6（仅TIM1/8/20内部通道）
    TIM_CH_ALL = 0xFF  ///< 所有通道
} tim_channel_t;

/**
 * @brief TIM计数模式
 */
typedef enum {
    TIM_COUNT_UP           = 0,  ///< 向上计数
    TIM_COUNT_DOWN         = 1,  ///< 向下计数
    TIM_COUNT_CENTER_1     = 2,  ///< 中央对齐模式1（向下时比较中断）
    TIM_COUNT_CENTER_2     = 3,  ///< 中央对齐模式2（向上时比较中断）
    TIM_COUNT_CENTER_3     = 4   ///< 中央对齐模式3（双向比较中断）
} tim_count_mode_t;

/**
 * @brief TIM时钟分频
 */
typedef enum {
    TIM_CKD_DIV1 = 0,  ///< 不分频（tDTS = tCK_INT）
    TIM_CKD_DIV2 = 1,  ///< 2分频（tDTS = 2 * tCK_INT）
    TIM_CKD_DIV4 = 2   ///< 4分频（tDTS = 4 * tCK_INT）
} tim_clock_division_t;

/**
 * @brief TIM PWM模式
 */
typedef enum {
    TIM_PWM_MODE1 = 0,  ///< PWM模式1：CNT<CCR时输出有效电平
    TIM_PWM_MODE2 = 1   ///< PWM模式2：CNT<CCR时输出无效电平
} tim_pwm_mode_t;

/**
 * @brief TIM输出比较模式
 */
typedef enum {
    TIM_OC_FROZEN       = 0,  ///< 冻结：比较匹配时无动作
    TIM_OC_ACTIVE       = 1,  ///< 匹配时输出有效电平
    TIM_OC_INACTIVE     = 2,  ///< 匹配时输出无效电平
    TIM_OC_TOGGLE       = 3,  ///< 匹配时输出翻转
    TIM_OC_FORCE_LOW    = 4,  ///< 强制输出低电平
    TIM_OC_FORCE_HIGH   = 5,  ///< 强制输出高电平
    TIM_OC_PWM1         = 6,  ///< PWM模式1
    TIM_OC_PWM2         = 7   ///< PWM模式2
} tim_oc_mode_t;

/**
 * @brief TIM输入捕获极性
 */
typedef enum {
    TIM_IC_RISING  = 0,  ///< 上升沿捕获
    TIM_IC_FALLING = 1,  ///< 下降沿捕获
    TIM_IC_BOTH    = 2   ///< 双边沿捕获
} tim_ic_polarity_t;

/**
 * @brief TIM输入捕获预分频
 */
typedef enum {
    TIM_IC_PSC_DIV1 = 0,  ///< 每个边沿触发
    TIM_IC_PSC_DIV2 = 1,  ///< 每2个边沿触发1次
    TIM_IC_PSC_DIV4 = 2,  ///< 每4个边沿触发1次
    TIM_IC_PSC_DIV8 = 3   ///< 每8个边沿触发1次
} tim_ic_prescaler_t;

/**
 * @brief TIM输入捕获选择
 */
typedef enum {
    TIM_IC_SELECT_DIRECT   = 1,  ///< ICx映射到TIx
    TIM_IC_SELECT_INDIRECT = 2,  ///< ICx交叉映射
    TIM_IC_SELECT_TRC      = 3   ///< ICx映射到TRC
} tim_ic_selection_t;

/**
 * @brief TIM编码器模式
 */
typedef enum {
    TIM_ENCODER_MODE_DISABLE = 0,  ///< 编码器禁用
    TIM_ENCODER_MODE_TI1     = 1,  ///< 模式1：仅TI1计数
    TIM_ENCODER_MODE_TI2     = 2,  ///< 模式2：仅TI2计数
    TIM_ENCODER_MODE_TI12    = 3   ///< 模式3：TI1和TI2都计数
} tim_encoder_mode_t;

/**
 * @brief TIM主模式输出（TRGO）
 */
typedef enum {
    TIM_TRGO_RESET  = 0,  ///< 复位：UG作为TRGO
    TIM_TRGO_ENABLE = 1,  ///< 使能：CNT_EN作为TRGO
    TIM_TRGO_UPDATE = 2,  ///< 更新：更新事件作为TRGO
    TIM_TRGO_CC1IF  = 3,  ///< 比较脉冲：CC1IF标志作为TRGO
    TIM_TRGO_OC1REF = 4,  ///< OC1REF信号作为TRGO
    TIM_TRGO_OC2REF = 5,  ///< OC2REF信号作为TRGO
    TIM_TRGO_OC3REF = 6,  ///< OC3REF信号作为TRGO
    TIM_TRGO_OC4REF = 7   ///< OC4REF信号作为TRGO
} tim_master_mode_t;

/**
 * @brief TIM主模式输出2（TRGO2，仅高级定时器）
 */
typedef enum {
    TIM_TRGO2_RESET      = 0,   ///< 复位
    TIM_TRGO2_ENABLE     = 1,   ///< 使能
    TIM_TRGO2_UPDATE     = 2,   ///< 更新事件
    TIM_TRGO2_CC1IF      = 3,   ///< 比较脉冲
    TIM_TRGO2_OC1REF     = 4,   ///< OC1REF
    TIM_TRGO2_OC2REF     = 5,   ///< OC2REF
    TIM_TRGO2_OC3REF     = 6,   ///< OC3REF
    TIM_TRGO2_OC4REF     = 7,   ///< OC4REF
    TIM_TRGO2_OC5REF     = 8,   ///< OC5REF
    TIM_TRGO2_OC6REF     = 9,   ///< OC6REF
    TIM_TRGO2_CC_PULSE   = 10,  ///< 比较脉冲（OC4_6）
    TIM_TRGO2_CC1_RISING = 11,  ///< OC1REFC上升沿
    TIM_TRGO2_CC2_RISING = 12,  ///< OC2REFC上升沿
    TIM_TRGO2_CC3_RISING = 13,  ///< OC3REFC上升沿
    TIM_TRGO2_CC4_RISING = 14,  ///< OC4REFC上升沿
    TIM_TRGO2_CC5_RISING = 15,  ///< OC5REFC上升沿
    TIM_TRGO2_CC6_RISING = 16   ///< OC6REFC上升沿
} tim_master_mode2_t;

/**
 * @brief TIM从模式
 */
typedef enum {
    TIM_SLAVE_DISABLE    = 0,  ///< 从模式禁用
    TIM_SLAVE_RESET      = 4,  ///< 复位模式
    TIM_SLAVE_GATED      = 5,  ///< 门控模式
    TIM_SLAVE_TRIGGER    = 6,  ///< 触发模式
    TIM_SLAVE_EXT_CLOCK1 = 7   ///< 外部时钟模式1
} tim_slave_mode_t;

/**
 * @brief TIM触发输入源
 */
typedef enum {
    TIM_TS_ITR0  = 0,  ///< 内部触发0
    TIM_TS_ITR1  = 1,  ///< 内部触发1
    TIM_TS_ITR2  = 2,  ///< 内部触发2
    TIM_TS_ITR3  = 3,  ///< 内部触发3
    TIM_TS_TI1F_ED = 4,  ///< TI1边沿检测器
    TIM_TS_TI1FP1  = 5,  ///< 滤波后的TI1
    TIM_TS_TI2FP2  = 6,  ///< 滤波后的TI2
    TIM_TS_ETRF    = 7   ///< 外部触发输入
} tim_trigger_source_t;

/**
 * @brief TIM外部触发极性
 */
typedef enum {
    TIM_ETR_NONINVERTED = 0,  ///< 不反相
    TIM_ETR_INVERTED    = 1   ///< 反相
} tim_etr_polarity_t;

/**
 * @brief TIM外部触发预分频
 */
typedef enum {
    TIM_ETR_PSC_OFF = 0,  ///< 关闭预分频
    TIM_ETR_PSC_DIV2 = 1,  ///< ETRP/2
    TIM_ETR_PSC_DIV4 = 2,  ///< ETRP/4
    TIM_ETR_PSC_DIV8 = 3   ///< ETRP/8
} tim_etr_prescaler_t;

/**
 * @brief TIM输出极性
 */
typedef enum {
    TIM_POLARITY_HIGH = 0,  ///< 高电平有效
    TIM_POLARITY_LOW  = 1   ///< 低电平有效
} tim_polarity_t;

/**
 * @brief TIM输出空闲状态
 */
typedef enum {
    TIM_IDLE_RESET = 0,  ///< 空闲时输出低电平
    TIM_IDLE_SET   = 1   ///< 空闲时输出高电平
} tim_idle_state_t;

/**
 * @brief TIM刹车时Off-state配置
 */
typedef enum {
    TIM_OFF_STATE_HIZ = 0,  ///< 高阻态（禁止输出）
    TIM_OFF_STATE_OIS = 1   ///< 输出OIS定义的空闲电平
} tim_off_state_t;

/**
 * @brief TIM刹车极性
 */
typedef enum {
    TIM_BREAK_POL_LOW  = 0,  ///< 低电平有效
    TIM_BREAK_POL_HIGH = 1   ///< 高电平有效
} tim_break_polarity_t;

/**
 * @brief TIM锁定级别
 */
typedef enum {
    TIM_LOCK_OFF    = 0,  ///< 无锁定
    TIM_LOCK_LEVEL1 = 1,  ///< 锁定级别1
    TIM_LOCK_LEVEL2 = 2,  ///< 锁定级别2
    TIM_LOCK_LEVEL3 = 3   ///< 锁定级别3（完全锁定）
} tim_lock_level_t;

/**
 * @brief TIM中断类型
 */
typedef enum {
    TIM_INT_UPDATE    = (1 << 0),   ///< 更新中断
    TIM_INT_CC1       = (1 << 1),   ///< 捕获/比较1中断
    TIM_INT_CC2       = (1 << 2),   ///< 捕获/比较2中断
    TIM_INT_CC3       = (1 << 3),   ///< 捕获/比较3中断
    TIM_INT_CC4       = (1 << 4),   ///< 捕获/比较4中断
    TIM_INT_COM       = (1 << 5),   ///< COM事件中断
    TIM_INT_TRIGGER   = (1 << 6),   ///< 触发中断
    TIM_INT_BREAK     = (1 << 7),   ///< 刹车中断
    TIM_INT_CC1_OVR   = (1 << 9),   ///< CC1捕获溢出
    TIM_INT_CC2_OVR   = (1 << 10),  ///< CC2捕获溢出
    TIM_INT_CC3_OVR   = (1 << 11),  ///< CC3捕获溢出
    TIM_INT_CC4_OVR   = (1 << 12),  ///< CC4捕获溢出
    TIM_INT_CC5       = (1 << 16),  ///< 捕获/比较5中断（仅高级定时器）
    TIM_INT_CC6       = (1 << 17),  ///< 捕获/比较6中断（仅高级定时器）
    TIM_INT_BREAK2    = (1 << 20)   ///< 刹车2中断（仅高级定时器）
} tim_interrupt_t;

/**
 * @brief TIM DMA请求
 */
typedef enum {
    TIM_DMA_UPDATE    = (1 << 8),   ///< 更新DMA请求
    TIM_DMA_CC1       = (1 << 9),   ///< CC1 DMA请求
    TIM_DMA_CC2       = (1 << 10),  ///< CC2 DMA请求
    TIM_DMA_CC3       = (1 << 11),  ///< CC3 DMA请求
    TIM_DMA_CC4       = (1 << 12),  ///< CC4 DMA请求
    TIM_DMA_COM       = (1 << 13),  ///< COM DMA请求
    TIM_DMA_TRIGGER   = (1 << 14)   ///< 触发DMA请求
} tim_dma_request_t;

/**
 * @brief TIM DMA突发长度
 */
typedef enum {
    TIM_DMA_BURST_1TRANSFER  = 0,   ///< 1次传输
    TIM_DMA_BURST_2TRANSFERS = 1,   ///< 2次传输
    TIM_DMA_BURST_3TRANSFERS = 2,   ///< 3次传输
    TIM_DMA_BURST_4TRANSFERS = 3,   ///< 4次传输
    TIM_DMA_BURST_5TRANSFERS = 4,   ///< 5次传输
    TIM_DMA_BURST_6TRANSFERS = 5,   ///< 6次传输
    TIM_DMA_BURST_7TRANSFERS = 6,   ///< 7次传输
    TIM_DMA_BURST_8TRANSFERS = 7,   ///< 8次传输
    TIM_DMA_BURST_9TRANSFERS = 8,   ///< 9次传输
    TIM_DMA_BURST_10TRANSFERS = 9,  ///< 10次传输
    TIM_DMA_BURST_11TRANSFERS = 10, ///< 11次传输
    TIM_DMA_BURST_12TRANSFERS = 11, ///< 12次传输
    TIM_DMA_BURST_13TRANSFERS = 12, ///< 13次传输
    TIM_DMA_BURST_14TRANSFERS = 13, ///< 14次传输
    TIM_DMA_BURST_15TRANSFERS = 14, ///< 15次传输
    TIM_DMA_BURST_16TRANSFERS = 15, ///< 16次传输
    TIM_DMA_BURST_17TRANSFERS = 16, ///< 17次传输
    TIM_DMA_BURST_18TRANSFERS = 17  ///< 18次传输
} tim_dma_burst_length_t;

/**
 * @brief TIM DMA突发基地址
 */
typedef enum {
    TIM_DMA_BASE_CR1    = 0,   ///< TIMx_CR1
    TIM_DMA_BASE_CR2    = 1,   ///< TIMx_CR2
    TIM_DMA_BASE_SMCR   = 2,   ///< TIMx_SMCR
    TIM_DMA_BASE_DIER   = 3,   ///< TIMx_DIER
    TIM_DMA_BASE_SR     = 4,   ///< TIMx_SR
    TIM_DMA_BASE_EGR    = 5,   ///< TIMx_EGR
    TIM_DMA_BASE_CCMR1  = 6,   ///< TIMx_CCMR1
    TIM_DMA_BASE_CCMR2  = 7,   ///< TIMx_CCMR2
    TIM_DMA_BASE_CCER   = 8,   ///< TIMx_CCER
    TIM_DMA_BASE_CNT    = 9,   ///< TIMx_CNT
    TIM_DMA_BASE_PSC    = 10,  ///< TIMx_PSC
    TIM_DMA_BASE_ARR    = 11,  ///< TIMx_ARR
    TIM_DMA_BASE_RCR    = 12,  ///< TIMx_RCR
    TIM_DMA_BASE_CCR1   = 13,  ///< TIMx_CCR1
    TIM_DMA_BASE_CCR2   = 14,  ///< TIMx_CCR2
    TIM_DMA_BASE_CCR3   = 15,  ///< TIMx_CCR3
    TIM_DMA_BASE_CCR4   = 16,  ///< TIMx_CCR4
    TIM_DMA_BASE_BDTR   = 17,  ///< TIMx_BDTR
    TIM_DMA_BASE_CCMR3  = 18,  ///< TIMx_CCMR3
    TIM_DMA_BASE_CCR5   = 19,  ///< TIMx_CCR5
    TIM_DMA_BASE_CCR6   = 20,  ///< TIMx_CCR6
    TIM_DMA_BASE_AF1    = 21,  ///< TIMx_AF1
    TIM_DMA_BASE_AF2    = 22   ///< TIMx_AF2
} tim_dma_base_address_t;

//=============================================================================
// 2. 数据结构定义
//=============================================================================

/**
 * @brief TIM时基配置结构体
 */
typedef struct {
    uint32_t           prescaler;            ///< 预分频器值（0-65535）
    uint32_t           period;               ///< 自动重载值（16位TIM: 0-65535, 32位TIM: 0-0xFFFFFFFF）
    tim_count_mode_t   count_mode;           ///< 计数模式
    tim_clock_division_t clock_division;     ///< 时钟分频
    uint8_t            repetition_counter;   ///< 重复计数器（0-255，仅高级定时器）
} tim_timebase_config_t;

/**
 * @brief TIM PWM配置结构体
 */
typedef struct {
    tim_oc_mode_t      mode;                 ///< 输出比较模式（通常为PWM1或PWM2）
    uint32_t           pulse;                ///< 脉宽值（比较值）
    tim_polarity_t     polarity;             ///< 输出极性
    tim_polarity_t     npolarity;            ///< 互补输出极性（仅支持互补输出的通道）
    tim_idle_state_t   idle_state;           ///< 空闲状态（仅高级定时器）
    tim_idle_state_t   nidle_state;          ///< 互补输出空闲状态（仅高级定时器）
    bool               fast_mode;            ///< 快速模式使能
    bool               preload;              ///< 预装载使能
} tim_pwm_config_t;

/**
 * @brief TIM输出比较配置结构体
 */
typedef struct {
    tim_oc_mode_t      mode;                 ///< 输出比较模式
    uint32_t           pulse;                ///< 比较值
    tim_polarity_t     polarity;             ///< 输出极性
    tim_polarity_t     npolarity;            ///< 互补输出极性
    tim_idle_state_t   idle_state;           ///< 空闲状态
    tim_idle_state_t   nidle_state;          ///< 互补输出空闲状态
    bool               fast_mode;            ///< 快速模式
    bool               preload;              ///< 预装载使能
} tim_oc_config_t;

/**
 * @brief TIM输入捕获配置结构体
 */
typedef struct {
    tim_ic_polarity_t  polarity;             ///< 捕获极性
    tim_ic_selection_t selection;            ///< 输入选择
    tim_ic_prescaler_t prescaler;            ///< 输入预分频
    uint8_t            filter;               ///< 输入滤波器（0-15）
} tim_ic_config_t;

/**
 * @brief TIM编码器配置结构体
 */
typedef struct {
    tim_encoder_mode_t   mode;               ///< 编码器模式
    tim_ic_polarity_t    ic1_polarity;       ///< TI1极性
    tim_ic_selection_t   ic1_selection;      ///< TI1选择
    tim_ic_prescaler_t   ic1_prescaler;      ///< TI1预分频
    uint8_t              ic1_filter;         ///< TI1滤波器（0-15）
    tim_ic_polarity_t    ic2_polarity;       ///< TI2极性
    tim_ic_selection_t   ic2_selection;      ///< TI2选择
    tim_ic_prescaler_t   ic2_prescaler;      ///< TI2预分频
    uint8_t              ic2_filter;         ///< TI2滤波器（0-15）
} tim_encoder_config_t;

/**
 * @brief TIM单脉冲模式配置结构体
 */
typedef struct {
    tim_oc_mode_t      oc_mode;              ///< 输出比较模式
    uint32_t           pulse;                ///< 脉宽值
    tim_polarity_t     oc_polarity;          ///< 输出极性
    tim_ic_polarity_t  ic_polarity;          ///< 输入捕获极性
    tim_ic_selection_t ic_selection;         ///< 输入选择
    uint8_t            ic_filter;            ///< 输入滤波器
} tim_opm_config_t;

/**
 * @brief TIM主从同步配置结构体
 */
typedef struct {
    tim_slave_mode_t     slave_mode;         ///< 从模式
    tim_trigger_source_t trigger_source;     ///< 触发源
    tim_master_mode_t    master_mode;        ///< 主模式（TRGO）
    tim_master_mode2_t   master_mode2;       ///< 主模式2（TRGO2，仅高级定时器）
    bool                 msm_enable;         ///< 主从模式使能
} tim_master_slave_config_t;

/**
 * @brief TIM刹车功能配置结构体
 */
typedef struct {
    bool                   brk_enable;       ///< 刹车功能使能
    bool                   brk_pin_enable;   ///< 刹车引脚(BRK)使能
    tim_break_polarity_t   brk_pin_polarity; ///< 刹车引脚(BRK)极性
    uint8_t                brk_filter;       ///< 刹车滤波器（0-15）
    bool                   comp1_enable;     ///< 比较器1作为刹车源使能
    tim_break_polarity_t   comp1_polarity;   ///< 比较器1极性
    bool                   comp2_enable;     ///< 比较器2作为刹车源使能
    tim_break_polarity_t   comp2_polarity;   ///< 比较器2极性
    bool                   comp3_enable;     ///< 比较器3作为刹车源使能
    tim_break_polarity_t   comp3_polarity;   ///< 比较器3极性
    bool                   comp4_enable;     ///< 比较器4作为刹车源使能
    tim_break_polarity_t   comp4_polarity;   ///< 比较器4极性
    bool                   automatic_output; ///< 自动输出使能
    tim_off_state_t        off_state_idle;   ///< 空闲模式下Off-state选择（OSSI）
    tim_off_state_t        off_state_run;    ///< 运行模式下Off-state选择（OSSR）
    tim_lock_level_t       lock_level;       ///< 锁定级别（最后配置）
} tim_break_config_t;

/**
 * @brief TIM互补输出配置结构体
 */
typedef struct {
    uint8_t              dead_time;          ///< 死区时间（0-255）
    tim_polarity_t       n_polarity;         ///< 互补输出极性
    tim_idle_state_t     idle_state;         ///< 主输出空闲状态
    tim_idle_state_t     nidle_state;        ///< 互补输出空闲状态
    bool                 automatic_output;   ///< 自动输出使能
} tim_complementary_config_t;

/**
 * @brief TIM DMA突发配置结构体
 */
typedef struct {
    tim_dma_base_address_t base_address;     ///< DMA突发基地址
    tim_dma_burst_length_t burst_length;     ///< DMA突发长度
} tim_dma_burst_config_t;

/**
 * @brief TIM霍尔传感器配置结构体
 */
typedef struct {
    tim_ic_polarity_t    ic1_polarity;       ///< 输入捕获1极性
    tim_ic_prescaler_t   ic1_prescaler;      ///< 输入预分频
    uint8_t              ic1_filter;         ///< 输入滤波器
    uint16_t             commutation_delay;  ///< 换向延迟
} tim_hall_sensor_config_t;

/**
 * @brief TIM外部触发配置结构体
 */
typedef struct {
    tim_etr_polarity_t   polarity;           ///< 极性
    tim_etr_prescaler_t  prescaler;          ///< 预分频
    uint8_t              filter;             ///< 滤波器（0-15）
} tim_etr_config_t;

/**
 * @brief TIM回调函数类型定义
 */
typedef void (*tim_callback_t)(tim_instance_t tim);
typedef void (*tim_cc_callback_t)(tim_instance_t tim, tim_channel_t channel);

//=============================================================================
// 3. 能力查询宏
//=============================================================================

/** @brief 检查是否为高级定时器 */
#define TIM_IS_ADVANCED(tim)  ((tim) == TIM_1 || (tim) == TIM_8)

/** @brief 检查是否为32位定时器 */
#define TIM_IS_32BIT(tim)     ((tim) == TIM_2 || (tim) == TIM_5)

/** @brief 检查是否为基本定时器 */
#define TIM_IS_BASIC(tim)     ((tim) == TIM_6 || (tim) == TIM_7)

/** @brief 检查是否支持互补输出 */
#define TIM_HAS_COMPLEMENTARY(tim)  TIM_IS_ADVANCED(tim)

/** @brief 检查是否支持刹车功能 */
#define TIM_HAS_BREAK(tim)    TIM_IS_ADVANCED(tim)

/** @brief 检查是否支持重复计数器 */
#define TIM_HAS_REPETITION(tim)  TIM_IS_ADVANCED(tim)

/** @brief 检查是否支持编码器模式 */
#define TIM_HAS_ENCODER(tim)  ((tim) >= TIM_2 && (tim) <= TIM_4)

/** @brief 获取定时器通道数 */
#define TIM_GET_CHANNEL_COUNT(tim)  \
    (TIM_IS_BASIC(tim) ? 0 : \
     ((tim) == TIM_10) ? 1 : \
     ((tim) == TIM_9) ? 2 : 4)

//=============================================================================
// 4. 第1层：快速初始化API（4个）
//=============================================================================

/**
 * @brief 快速初始化PWM输出
 * 
 * 一行代码快速配置PWM输出，适用于90%的简单应用场景。
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @param frequency_hz PWM频率（Hz）
 * @param duty_percent 占空比（0-100%）
 * @return true 初始化成功
 * @return false 初始化失败（参数无效或不支持）
 * 
 * @note 自动计算预分频和周期值
 * @note 自动使能定时器时钟
 * @note 自动启动PWM输出
 * @note GPIO需要单独配置为复用功能
 * 
 * @code
 * // 输出10kHz、50%占空比的PWM
 * tim_init_pwm(TIM_1, TIM_CH1, 10000, 50);
 * @endcode
 */
bool tim_init_pwm(tim_instance_t tim, tim_channel_t channel,
                  uint32_t frequency_hz, uint8_t duty_percent);

/**
 * @brief 快速初始化输入捕获
 * 
 * @param tim 定时器实例
 * @param channel 输入通道
 * @param polarity 捕获极性
 * @param filter 输入滤波器（0-15）
 * @return true 初始化成功
 * @return false 初始化失败
 * 
 * @note 自动配置：直接映射、无预分频
 * @note GPIO需要单独配置为复用功能
 * 
 * @code
 * // 捕获上升沿，滤波器=8
 * tim_init_input_capture(TIM_1, TIM_CH1, TIM_IC_RISING, 8);
 * @endcode
 */
bool tim_init_input_capture(tim_instance_t tim, tim_channel_t channel,
                             tim_ic_polarity_t polarity, uint8_t filter);

/**
 * @brief 快速初始化编码器模式
 * 
 * @param tim 定时器实例（必须支持编码器）
 * @param mode 编码器模式
 * @return true 初始化成功
 * @return false 初始化失败
 * 
 * @note 自动配置：直接映射、无预分频、滤波器=8
 * @note GPIO需要单独配置为复用功能
 * 
 * @code
 * // TIM2作为正交编码器
 * tim_init_encoder(TIM_2, TIM_ENCODER_MODE_TI12);
 * @endcode
 */
bool tim_init_encoder(tim_instance_t tim, tim_encoder_mode_t mode);

/**
 * @brief 快速初始化周期定时器（用于定时中断）
 * 
 * @param tim 定时器实例
 * @param period_us 定时周期（微秒）
 * @return true 初始化成功
 * @return false 初始化失败
 * 
 * @note 自动计算预分频和周期值
 * @note 自动使能更新中断
 * 
 * @code
 * // 1ms定时中断
 * tim_init_periodic(TIM_6, 1000);
 * tim_start(TIM_6);
 * @endcode
 */
bool tim_init_periodic(tim_instance_t tim, uint32_t period_us);

//=============================================================================
// 5. 第2层：基础配置API（8个）
//=============================================================================

/**
 * @brief 配置时基单元
 * 
 * @param tim 定时器实例
 * @param config 时基配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_timebase_config_t cfg = {
 *     .prescaler = 239,          // 240MHz -> 1MHz
 *     .period = 9999,            // 1MHz -> 100Hz
 *     .count_mode = TIM_COUNT_UP,
 *     .clock_division = TIM_CKD_DIV1,
 *     .repetition_counter = 0
 * };
 * tim_config_timebase(TIM_1, &cfg);
 * @endcode
 */
bool tim_config_timebase(tim_instance_t tim, const tim_timebase_config_t *config);

/**
 * @brief 配置PWM输出通道
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @param config PWM配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 *                   BDTR[OISx,OISxN], CCRx
 * 
 * @code
 * tim_pwm_config_t pwm = {
 *     .mode = TIM_PWM_MODE1,
 *     .pulse = 5000,
 *     .polarity = TIM_POLARITY_HIGH,
 *     .fast_mode = false,
 *     .preload = true
 * };
 * tim_config_channel_pwm(TIM_1, TIM_CH1, &pwm);
 * @endcode
 */
bool tim_config_channel_pwm(tim_instance_t tim, tim_channel_t channel,
                             const tim_pwm_config_t *config);

/**
 * @brief 配置输入捕获通道
 * 
 * @param tim 定时器实例
 * @param channel 输入通道
 * @param config 输入捕获配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_ic_config_t ic = {
 *     .polarity = TIM_IC_RISING,
 *     .selection = TIM_IC_SELECT_DIRECT,
 *     .prescaler = TIM_IC_PSC_DIV1,
 *     .filter = 8
 * };
 * tim_config_channel_input_capture(TIM_2, TIM_CH1, &ic);
 * @endcode
 */
bool tim_config_channel_input_capture(tim_instance_t tim, tim_channel_t channel,
                                       const tim_ic_config_t *config);

/**
 * @brief 配置输出比较通道
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @param config 输出比较配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 *                   BDTR[OISx,OISxN], CCRx
 * 
 * @code
 * tim_oc_config_t oc = {
 *     .mode = TIM_OC_TOGGLE,
 *     .pulse = 5000,
 *     .polarity = TIM_POLARITY_HIGH,
 *     .preload = true
 * };
 * tim_config_channel_output_compare(TIM_3, TIM_CH2, &oc);
 * @endcode
 */
bool tim_config_channel_output_compare(tim_instance_t tim, tim_channel_t channel,
                                        const tim_oc_config_t *config);

/**
 * @brief 配置编码器模式
 * 
 * @param tim 定时器实例
 * @param config 编码器配置结构体指针
 * @return true 配置成功
 * @return false 配置失败（定时器不支持编码器）
 * 
 *                   CCER[CC1P,CC1NP,CC2P,CC2NP]
 * 
 * @code
 * tim_encoder_config_t enc = {
 *     .mode = TIM_ENCODER_MODE_TI12,
 *     .ic1_polarity = TIM_IC_RISING,
 *     .ic1_selection = TIM_IC_SELECT_DIRECT,
 *     .ic1_prescaler = TIM_IC_PSC_DIV1,
 *     .ic1_filter = 10,
 *     .ic2_polarity = TIM_IC_RISING,
 *     .ic2_selection = TIM_IC_SELECT_DIRECT,
 *     .ic2_prescaler = TIM_IC_PSC_DIV1,
 *     .ic2_filter = 10
 * };
 * tim_config_encoder(TIM_3, &enc);
 * @endcode
 */
bool tim_config_encoder(tim_instance_t tim, const tim_encoder_config_t *config);

/**
 * @brief 配置单脉冲模式
 * 
 * @param tim 定时器实例
 * @param config 单脉冲配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_opm_config_t opm = {
 *     .oc_mode = TIM_PWM_MODE2,
 *     .pulse = 1000,
 *     .oc_polarity = TIM_POLARITY_HIGH,
 *     .ic_polarity = TIM_IC_RISING,
 *     .ic_selection = TIM_IC_SELECT_DIRECT,
 *     .ic_filter = 0
 * };
 * tim_config_one_pulse_mode(TIM_2, &opm);
 * @endcode
 */
bool tim_config_one_pulse_mode(tim_instance_t tim, const tim_opm_config_t *config);

/**
 * @brief 配置主从同步
 * 
 * @param tim 定时器实例
 * @param config 主从同步配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_master_slave_config_t ms = {
 *     .slave_mode = TIM_SLAVE_TRIGGER,
 *     .trigger_source = TIM_TS_ITR0,
 *     .master_mode = TIM_TRGO_UPDATE,
 *     .msm_enable = true
 * };
 * tim_config_master_slave(TIM_2, &ms);
 * @endcode
 */
bool tim_config_master_slave(tim_instance_t tim, const tim_master_slave_config_t *config);

/**
 * @brief 配置霍尔传感器接口
 * 
 * @param tim 定时器实例
 * @param config 霍尔传感器配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_hall_sensor_config_t hall = {
 *     .ic1_polarity = TIM_IC_RISING,
 *     .ic1_prescaler = TIM_IC_PSC_DIV1,
 *     .ic1_filter = 10,
 *     .commutation_delay = 0
 * };
 * tim_config_hall_sensor(TIM_1, &hall);
 * @endcode
 */
bool tim_config_hall_sensor(tim_instance_t tim, const tim_hall_sensor_config_t *config);

//=============================================================================
// 6. 第3层：高级功能API（30+个）
//=============================================================================

/**
 * @brief 配置刹车功能（完整配置）
 * 
 * @param tim 定时器实例（必须支持刹车）
 * @param config 刹车配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 *                  AF1[BKINE,BKCMP1E-4E,BKINP,BKCMP1P-4P]
 * @note 死区时间(DTG)由tim_config_complementary_output()配置
 * 
 * @code
 * tim_break_config_t brk = {
 *     .brk_pin_enable = true,
 *     .brk_pin_polarity = TIM_BREAK_POL_LOW,
 *     .brk_filter = 3,
 *     .comp1_enable = false,   // 可选：使用比较器作为刹车源
 *     .comp1_polarity = TIM_BREAK_POL_HIGH,
 *     .comp2_enable = false,
 *     .comp2_polarity = TIM_BREAK_POL_HIGH,
 *     .comp3_enable = false,
 *     .comp3_polarity = TIM_BREAK_POL_HIGH,
 *     .comp4_enable = false,
 *     .comp4_polarity = TIM_BREAK_POL_HIGH,
 *     .automatic_output = true,
 *     .off_state_idle = TIM_OFF_STATE_HIZ,
 *     .off_state_run = TIM_OFF_STATE_HIZ,
 *     .lock_level = TIM_LOCK_LEVEL1
 * };
 * tim_config_break(TIM_1, &brk);
 * @endcode
 */
bool tim_config_break(tim_instance_t tim, const tim_break_config_t *config);

/**
 * @brief 配置互补输出
 * 
 * @param tim 定时器实例（必须支持互补输出）
 * @param channel 主输出通道
 * @param config 互补输出配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_complementary_config_t comp = {
 *     .dead_time = 72,  // 死区时间
 *     .n_polarity = TIM_POLARITY_HIGH,
 *     .idle_state = TIM_IDLE_RESET,
 *     .nidle_state = TIM_IDLE_RESET,
 *     .automatic_output = true
 * };
 * tim_config_complementary_output(TIM_1, TIM_CH1, &comp);
 * @endcode
 */
bool tim_config_complementary_output(tim_instance_t tim, tim_channel_t channel,
                                      const tim_complementary_config_t *config);

/**
 * @brief 配置DMA Burst传输
 * 
 * @param tim 定时器实例
 * @param config DMA突发配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_dma_burst_config_t dma_burst = {
 *     .base_address = TIM_DMA_BASE_CCR1,
 *     .burst_length = TIM_DMA_BURST_4TRANSFERS
 * };
 * tim_config_dma_burst(TIM_1, &dma_burst);
 * @endcode
 */
bool tim_config_dma_burst(tim_instance_t tim, const tim_dma_burst_config_t *config);

/**
 * @brief 配置外部触发输入
 * 
 * @param tim 定时器实例
 * @param config 外部触发配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 * 
 * 
 * @code
 * tim_etr_config_t etr = {
 *     .polarity = TIM_ETR_NONINVERTED,
 *     .prescaler = TIM_ETR_PSC_OFF,
 *     .filter = 0
 * };
 * tim_config_external_trigger(TIM_2, &etr);
 * @endcode
 */
bool tim_config_external_trigger(tim_instance_t tim, const tim_etr_config_t *config);

/**
 * @brief 使能外部时钟模式2（ETR作为时钟源）
 * 
 * @param tim 定时器实例
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_external_clock_mode2(tim_instance_t tim, bool enable);

/**
 * @brief 设置死区时间（仅支持互补输出的定时器）
 * 
 * @param tim 定时器实例
 * @param dead_time 死区时间值（0-255）
 * @return true 设置成功
 * @return false 设置失败
 * 
 * @note DTG计算公式详见数据手册
 * 
 * @code
 * tim_set_dead_time(TIM_1, 72);  // 设置死区时间
 * @endcode
 */
bool tim_set_dead_time(tim_instance_t tim, uint8_t dead_time);

/**
 * @brief 设置重复计数器（仅高级定时器）
 * 
 * @param tim 定时器实例
 * @param repetition 重复计数值（0-255）
 * @return true 设置成功
 * @return false 设置失败
 * 
 * @note 更新事件在(repetition+1)次溢出后触发
 * 
 * @code
 * tim_set_repetition_counter(TIM_1, 3);  // 每4次溢出触发1次更新
 * @endcode
 */
bool tim_set_repetition_counter(tim_instance_t tim, uint8_t repetition);

/**
 * @brief 设置锁定级别（防止寄存器被意外修改）
 * 
 * @param tim 定时器实例
 * @param lock_level 锁定级别
 * @return true 设置成功
 * @return false 设置失败
 * 
 * @note 锁定后只能通过复位解除
 */
bool tim_set_lock_level(tim_instance_t tim, tim_lock_level_t lock_level);

/**
 * @brief 使能/禁用预装载功能
 * 
 * @param tim 定时器实例
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_autoreload_preload(tim_instance_t tim, bool enable);

/**
 * @brief 使能/禁用单脉冲模式
 * 
 * @param tim 定时器实例
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_one_pulse_mode(tim_instance_t tim, bool enable);

/**
 * @brief 使能/禁用更新事件
 * 
 * @param tim 定时器实例
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_update_event(tim_instance_t tim, bool enable);

/**
 * @brief 使能/禁用更新请求源选择
 * 
 * @param tim 定时器实例
 * @param only_overflow true=仅溢出触发UEV，false=任何事件触发UEV
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_set_update_request_source(tim_instance_t tim, bool only_overflow);

/**
 * @brief 配置TI1输入通道映射（用于霍尔传感器）
 * 
 * @param tim 定时器实例
 * @param xor_enable true=TI1连接到XOR，false=TI1连接到IO
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_ti1_selection(tim_instance_t tim, bool xor_enable);

/**
 * @brief 配置输出空闲状态（仅高级定时器）
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @param idle_state 空闲状态
 * @param nidle_state 互补输出空闲状态
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_output_idle_state(tim_instance_t tim, tim_channel_t channel,
                                tim_idle_state_t idle_state, tim_idle_state_t nidle_state);

/**
 * @brief 配置OCREF清除功能
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @param enable true=使能，false=禁用
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_ocref_clear(tim_instance_t tim, tim_channel_t channel, bool enable);

/**
 * @brief 配置通道输入滤波器
 * 
 * @param tim 定时器实例
 * @param channel 输入通道
 * @param filter 滤波器值（0-15）
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_input_filter(tim_instance_t tim, tim_channel_t channel, uint8_t filter);

/**
 * @brief 配置通道输入预分频
 * 
 * @param tim 定时器实例
 * @param channel 输入通道
 * @param prescaler 预分频值
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_input_prescaler(tim_instance_t tim, tim_channel_t channel,
                              tim_ic_prescaler_t prescaler);

/**
 * @brief 选择通道输入映射
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param selection 输入选择
 * @return true 配置成功
 * @return false 配置失败
 * 
 * @note TISEL寄存器用于更高级的通道映射
 */
bool tim_set_channel_input_selection(tim_instance_t tim, tim_channel_t channel,
                                      tim_ic_selection_t selection);

/**
 * @brief 配置通道输入/输出映射（高级功能）
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param remap 映射值（参考手册TISEL寄存器）
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_channel_remap(tim_instance_t tim, tim_channel_t channel, uint8_t remap);

/**
 * @brief 配置刹车输入源（高级刹车功能）
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入编号（1或2）
 * @param source_mask 刹车源掩码（GPIO, COMP等）
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_config_break_input_source(tim_instance_t tim, uint8_t break_input, uint32_t source_mask);

/**
 * @brief 使能/禁用刹车输入双边沿检测
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入编号（1或2）
 * @param enable true=使能，false=禁用
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_enable_break_bidirectional(tim_instance_t tim, uint8_t break_input, bool enable);

/**
 * @brief 配置刹车输入禁用
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入编号（1或2）
 * @param disable_mask 禁用掩码
 * @return true 配置成功
 * @return false 配置失败
 * 
 */
bool tim_set_break_input_disable(tim_instance_t tim, uint8_t break_input, uint32_t disable_mask);

//=============================================================================
// 7. 第4层：控制与查询API（40+个）
//=============================================================================

/**
 * @brief 启动定时器
 * 
 * @param tim 定时器实例
 * @return true 启动成功
 * @return false 启动失败
 * 
 */
bool tim_start(tim_instance_t tim);

/**
 * @brief 停止定时器
 * 
 * @param tim 定时器实例
 * @return true 停止成功
 * @return false 停止失败
 * 
 */
bool tim_stop(tim_instance_t tim);

/**
 * @brief 使能定时器主输出（仅高级定时器）
 * 
 * @param tim 定时器实例
 * @return true 使能成功
 * @return false 使能失败
 * 
 * @note 必须在启动PWM前调用
 */
bool tim_enable_main_output(tim_instance_t tim);

/**
 * @brief 禁用定时器主输出
 * 
 * @param tim 定时器实例
 * @return true 禁用成功
 * @return false 禁用失败
 * 
 */
bool tim_disable_main_output(tim_instance_t tim);

/**
 * @brief 启动PWM输出（组合：使能通道+启动定时器+MOE）
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @return true 启动成功
 * @return false 启动失败
 * 
 */
bool tim_start_pwm(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 停止PWM输出
 * 
 * @param tim 定时器实例
 * @param channel 输出通道
 * @return true 停止成功
 * @return false 停止失败
 * 
 */
bool tim_stop_pwm(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 使能/禁用通道
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_channel(tim_instance_t tim, tim_channel_t channel, bool enable);

/**
 * @brief 使能/禁用互补通道
 * 
 * @param tim 定时器实例
 * @param channel 主通道
 * @param enable true=使能，false=禁用
 * @return true 操作成功
 * @return false 操作失败
 * 
 */
bool tim_enable_complementary_channel(tim_instance_t tim, tim_channel_t channel, bool enable);

/**
 * @brief 设置计数器值
 * 
 * @param tim 定时器实例
 * @param value 计数器值
 * @return true 设置成功
 * @return false 设置失败
 * 
 */
bool tim_set_counter(tim_instance_t tim, uint32_t value);

/**
 * @brief 获取计数器值
 * 
 * @param tim 定时器实例
 * @return 当前计数器值
 * 
 * @note 读取寄存器：CNT
 */
uint32_t tim_get_counter(tim_instance_t tim);

/**
 * @brief 设置自动重载值
 * 
 * @param tim 定时器实例
 * @param value 自动重载值
 * @return true 设置成功
 * @return false 设置失败
 * 
 */
bool tim_set_autoreload(tim_instance_t tim, uint32_t value);

/**
 * @brief 获取自动重载值
 * 
 * @param tim 定时器实例
 * @return 当前自动重载值
 * 
 * @note 读取寄存器：ARR
 */
uint32_t tim_get_autoreload(tim_instance_t tim);

/**
 * @brief 设置预分频器
 * 
 * @param tim 定时器实例
 * @param prescaler 预分频值（0-65535）
 * @return true 设置成功
 * @return false 设置失败
 * 
 */
bool tim_set_prescaler(tim_instance_t tim, uint16_t prescaler);

/**
 * @brief 获取预分频器
 * 
 * @param tim 定时器实例
 * @return 当前预分频值
 * 
 * @note 读取寄存器：PSC
 */
uint16_t tim_get_prescaler(tim_instance_t tim);

/**
 * @brief 设置捕获/比较值
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param value 比较值
 * @return true 设置成功
 * @return false 设置失败
 * 
 */
bool tim_set_compare(tim_instance_t tim, tim_channel_t channel, uint32_t value);

/**
 * @brief 获取比较值
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return 当前比较值
 * 
 * @note 读取寄存器：CCRx
 */
uint32_t tim_get_compare(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 获取捕获值
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return 捕获值
 * 
 * @note 读取寄存器：CCRx
 */
uint32_t tim_get_capture(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 设置PWM占空比（百分比）
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param duty_percent 占空比（0-100%）
 * @return true 设置成功
 * @return false 设置失败
 * 
 * @note 自动根据ARR计算CCR值
 */
bool tim_set_duty(tim_instance_t tim, tim_channel_t channel, uint8_t duty_percent);

/**
 * @brief 获取PWM占空比
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return 占空比（0-100%）
 */
uint8_t tim_get_duty(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 设置PWM频率（自动计算PSC和ARR）
 * 
 * @param tim 定时器实例
 * @param frequency_hz 目标频率（Hz）
 * @return true 设置成功
 * @return false 设置失败（频率超出范围）
 * 
 * @note 保持当前占空比不变
 */
bool tim_set_pwm_frequency(tim_instance_t tim, uint32_t frequency_hz);

/**
 * @brief 获取PWM频率
 * 
 * @param tim 定时器实例
 * @return 当前频率（Hz）
 */
uint32_t tim_get_pwm_frequency(tim_instance_t tim);

/**
 * @brief 使能中断
 * 
 * @param tim 定时器实例
 * @param interrupt 中断类型（可OR组合）
 * @return true 使能成功
 * @return false 使能失败
 * 
 */
bool tim_enable_interrupt(tim_instance_t tim, uint32_t interrupt);

/**
 * @brief 禁用中断
 * 
 * @param tim 定时器实例
 * @param interrupt 中断类型
 * @return true 禁用成功
 * @return false 禁用失败
 * 
 */
bool tim_disable_interrupt(tim_instance_t tim, uint32_t interrupt);

/**
 * @brief 获取中断状态
 * 
 * @param tim 定时器实例
 * @param interrupt 中断类型
 * @return true 中断已触发
 * @return false 中断未触发
 * 
 * @note 读取寄存器：SR
 */
bool tim_get_interrupt_status(tim_instance_t tim, uint32_t interrupt);

/**
 * @brief 清除中断标志
 * 
 * @param tim 定时器实例
 * @param interrupt 中断类型
 * @return true 清除成功
 * @return false 清除失败
 * 
 */
bool tim_clear_interrupt_flag(tim_instance_t tim, uint32_t interrupt);

/**
 * @brief 使能DMA请求
 * 
 * @param tim 定时器实例
 * @param dma_request DMA请求类型（可OR组合）
 * @return true 使能成功
 * @return false 使能失败
 * 
 */
bool tim_enable_dma_request(tim_instance_t tim, uint32_t dma_request);

/**
 * @brief 禁用DMA请求
 * 
 * @param tim 定时器实例
 * @param dma_request DMA请求类型
 * @return true 禁用成功
 * @return false 禁用失败
 * 
 */
bool tim_disable_dma_request(tim_instance_t tim, uint32_t dma_request);

/**
 * @brief 获取DMA突发地址
 * 
 * @param tim 定时器实例
 * @return DMA突发传输寄存器地址
 * 
 * @note 返回DMAR寄存器地址
 */
volatile uint32_t* tim_get_dma_burst_address(tim_instance_t tim);

/**
 * @brief 生成软件更新事件
 * 
 * @param tim 定时器实例
 * @return true 生成成功
 * @return false 生成失败
 * 
 */
bool tim_generate_update_event(tim_instance_t tim);

/**
 * @brief 生成捕获/比较事件
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return true 生成成功
 * @return false 生成失败
 * 
 */
bool tim_generate_cc_event(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 生成COM事件（仅高级定时器）
 * 
 * @param tim 定时器实例
 * @return true 生成成功
 * @return false 生成失败
 * 
 */
bool tim_generate_com_event(tim_instance_t tim);

/**
 * @brief 生成触发事件
 * 
 * @param tim 定时器实例
 * @return true 生成成功
 * @return false 生成失败
 * 
 */
bool tim_generate_trigger_event(tim_instance_t tim);

/**
 * @brief 生成刹车事件
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入（1或2）
 * @return true 生成成功
 * @return false 生成失败
 * 
 */
bool tim_generate_break_event(tim_instance_t tim, uint8_t break_input);

/**
 * @brief 检查定时器是否运行
 * 
 * @param tim 定时器实例
 * @return true 定时器正在运行
 * @return false 定时器已停止
 * 
 * @note 读取寄存器：CR1[CEN]
 */
bool tim_is_running(tim_instance_t tim);

/**
 * @brief 获取当前计数方向
 * 
 * @param tim 定时器实例
 * @return true 向下计数
 * @return false 向上计数
 * 
 * @note 读取寄存器：CR1[DIR]
 */
bool tim_get_direction(tim_instance_t tim);

/**
 * @brief 获取捕获溢出标志
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return true 发生溢出
 * @return false 未溢出
 * 
 * @note 读取寄存器：SR[CCxOF]
 */
bool tim_get_capture_overflow_flag(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 清除捕获溢出标志
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @return true 清除成功
 * @return false 清除失败
 * 
 */
bool tim_clear_capture_overflow_flag(tim_instance_t tim, tim_channel_t channel);

/**
 * @brief 获取刹车标志
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入（1或2）
 * @return true 刹车已触发
 * @return false 刹车未触发
 * 
 * @note 读取寄存器：SR[BIF,B2IF]
 */
bool tim_get_break_flag(tim_instance_t tim, uint8_t break_input);

/**
 * @brief 清除刹车标志
 * 
 * @param tim 定时器实例
 * @param break_input 刹车输入（1或2）
 * @return true 清除成功
 * @return false 清除失败
 * 
 */
bool tim_clear_break_flag(tim_instance_t tim, uint8_t break_input);

//=============================================================================
// 8. 能力查询API
//=============================================================================

/**
 * @brief 检查是否为32位定时器
 * 
 * @param tim 定时器实例
 * @return true 32位定时器
 * @return false 16位定时器
 */
bool tim_is_32bit(tim_instance_t tim);

/**
 * @brief 检查是否为高级定时器
 * 
 * @param tim 定时器实例
 * @return true 高级定时器
 * @return false 其他定时器
 */
bool tim_is_advanced(tim_instance_t tim);

/**
 * @brief 检查是否支持互补输出
 * 
 * @param tim 定时器实例
 * @return true 支持
 * @return false 不支持
 */
bool tim_has_complementary_output(tim_instance_t tim);

/**
 * @brief 检查是否支持刹车功能
 * 
 * @param tim 定时器实例
 * @return true 支持
 * @return false 不支持
 */
bool tim_has_break(tim_instance_t tim);

/**
 * @brief 检查是否支持重复计数器
 * 
 * @param tim 定时器实例
 * @return true 支持
 * @return false 不支持
 */
bool tim_has_repetition_counter(tim_instance_t tim);

/**
 * @brief 检查是否支持编码器模式
 * 
 * @param tim 定时器实例
 * @return true 支持
 * @return false 不支持
 */
bool tim_has_encoder(tim_instance_t tim);

/**
 * @brief 获取定时器通道数量
 * 
 * @param tim 定时器实例
 * @return 通道数（0, 1, 2, 4）
 */
uint8_t tim_get_channel_count(tim_instance_t tim);

//=============================================================================
// 9. 辅助计算API
//=============================================================================

/**
 * @brief 根据目标频率计算最优PSC和ARR
 * 
 * @param tim_clock TIM时钟频率（Hz）
 * @param target_freq 目标频率（Hz）
 * @param prescaler 输出：计算出的预分频值
 * @param period 输出：计算出的周期值
 * @param is_32bit 是否为32位定时器
 * @return true 计算成功
 * @return false 计算失败（频率超出范围）
 */
bool tim_calculate_frequency(uint32_t tim_clock, uint32_t target_freq,
                              uint16_t *prescaler, uint32_t *period, bool is_32bit);

/**
 * @brief 根据目标周期（微秒）计算PSC和ARR
 * 
 * @param tim_clock TIM时钟频率（Hz）
 * @param period_us 目标周期（微秒）
 * @param prescaler 输出：计算出的预分频值
 * @param period 输出：计算出的周期值
 * @param is_32bit 是否为32位定时器
 * @return true 计算成功
 * @return false 计算失败
 */
bool tim_calculate_period_us(uint32_t tim_clock, uint32_t period_us,
                              uint16_t *prescaler, uint32_t *period, bool is_32bit);

//=============================================================================
// 10. 中断回调注册
//=============================================================================

/**
 * @brief 注册更新事件回调函数
 * 
 * @param tim 定时器实例
 * @param callback 回调函数指针
 * @return true 注册成功
 * @return false 注册失败
 */
bool tim_register_update_callback(tim_instance_t tim, tim_callback_t callback);

/**
 * @brief 注册捕获/比较事件回调函数
 * 
 * @param tim 定时器实例
 * @param channel 通道
 * @param callback 回调函数指针
 * @return true 注册成功
 * @return false 注册失败
 */
bool tim_register_cc_callback(tim_instance_t tim, tim_channel_t channel,
                               tim_cc_callback_t callback);

/**
 * @brief 注册刹车事件回调函数
 * 
 * @param tim 定时器实例
 * @param callback 回调函数指针
 * @return true 注册成功
 * @return false 注册失败
 */
bool tim_register_break_callback(tim_instance_t tim, tim_callback_t callback);

/**
 * @brief 注册触发事件回调函数
 * 
 * @param tim 定时器实例
 * @param callback 回调函数指针
 * @return true 注册成功
 * @return false 注册失败
 */
bool tim_register_trigger_callback(tim_instance_t tim, tim_callback_t callback);

/**
 * @brief 注册COM事件回调函数（仅高级定时器）
 * 
 * @param tim 定时器实例
 * @param callback 回调函数指针
 * @return true 注册成功
 * @return false 注册失败
 */
bool tim_register_com_callback(tim_instance_t tim, tim_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // ACM32P4_TIM_H
