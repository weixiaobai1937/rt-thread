/**
 * @file adc.h
 * @brief ACM32P4 ADC驱动 - 模数转换器驱动接口
 *
 * @details
 * 本驱动支持ACM32P4系列芯片的ADC模块所有功能：
 * - 3个ADC（ADC1/2/3），12位5Msps逐次比较型
 * - 单次/连续/间断转换模式
 * - 规则通道组（最多16个）和注入通道组（最多4个）
 * - 模拟看门狗（单端/差分阈值）
 * - 过采样（2x-256x）与偏移补偿
 * - 双ADC模式（ADC1主 + ADC2从）
 * - 中断和DMA支持
 * - 温度传感器 / VBAT / VREFINT 内部通道
 *
 * 特性：
 * - ✅ 4层API架构（快速初始化 → 基础配置 → 高级功能 → 控制查询）
 * - ✅ 类型安全（枚举类型，无魔法数字）
 * - ✅ 中断/DMA支持
 * - ✅ 双ADC模式支持
 *
 * @example
 * @code
 * // 快速单通道单次转换（第1层API）
 * adc_init_single_channel(ADC1, ADC_CHANNEL_3, ADC_SAMPLE_TIME_13);
 * uint16_t val = adc_get_result(ADC1);
 *
 * // 完整配置（第2层API）
 * adc_regular_config_t reg = {
 *     .length = 1,
 *     .sequence = {ADC_CHANNEL_3, ADC_CHANNEL_7},
 * };
 * adc_config_regular_sequence(ADC1, &reg);
 * @endcode
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef ACM32P4_ADC_H
#define ACM32P4_ADC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 1. 类型定义
//=============================================================================

/**
 * @brief ADC实例枚举
 */
typedef enum
{
    ADC_1 = 0,    ///< ADC1（主ADC，双ADC模式主器件）
    ADC_2 = 1,    ///< ADC2（从ADC）
    ADC_3 = 2,    ///< ADC3（独立模块）
    ADC_COUNT = 3 ///< ADC总数
} adc_instance_t;

/**
 * @brief ADC通道枚举
 */
typedef enum
{
    ADC_CHANNEL_0 = 0,   ///< 通道0（校准通道）
    ADC_CHANNEL_1 = 1,   ///< 通道1
    ADC_CHANNEL_2 = 2,   ///< 通道2
    ADC_CHANNEL_3 = 3,   ///< 通道3（快速通道）
    ADC_CHANNEL_4 = 4,   ///< 通道4
    ADC_CHANNEL_5 = 5,   ///< 通道5
    ADC_CHANNEL_6 = 6,   ///< 通道6
    ADC_CHANNEL_7 = 7,   ///< 通道7
    ADC_CHANNEL_8 = 8,   ///< 通道8
    ADC_CHANNEL_9 = 9,   ///< 通道9
    ADC_CHANNEL_10 = 10, ///< 通道10
    ADC_CHANNEL_11 = 11, ///< 通道11
    ADC_CHANNEL_12 = 12, ///< 通道12
    ADC_CHANNEL_13 = 13, ///< 通道13
    ADC_CHANNEL_14 = 14, ///< 通道14
    ADC_CHANNEL_15 = 15, ///< 通道15
    ADC_CHANNEL_16 = 16, ///< 通道16
    ADC_CHANNEL_17 = 17, ///< 通道17
    ADC_CHANNEL_18 = 18, ///< 通道18
} adc_channel_t;

/**
 * @brief ADC分辨率
 */
typedef enum
{
    ADC_RES_12BIT = 0, ///< 12位分辨率（默认）
    ADC_RES_10BIT = 1, ///< 10位分辨率
    ADC_RES_8BIT = 2,  ///< 8位分辨率
    ADC_RES_6BIT = 3,  ///< 6位分辨率
} adc_resolution_t;

/**
 * @brief ADC数据对齐方式
 */
typedef enum
{
    ADC_ALIGN_RIGHT = 0, ///< 右对齐（默认）
    ADC_ALIGN_LEFT = 1,  ///< 左对齐（过采样模式下忽略）
} adc_data_align_t;

/**
 * @brief ADC采样时间
 */
typedef enum
{
    ADC_SAMPLE_3 = 0,    ///< 3个ADC_CLK周期
    ADC_SAMPLE_5 = 1,    ///< 5个周期
    ADC_SAMPLE_7 = 2,    ///< 7个周期
    ADC_SAMPLE_10 = 3,   ///< 10个周期
    ADC_SAMPLE_13 = 4,   ///< 13个周期
    ADC_SAMPLE_16 = 5,   ///< 16个周期
    ADC_SAMPLE_20 = 6,   ///< 20个周期
    ADC_SAMPLE_30 = 7,   ///< 30个周期
    ADC_SAMPLE_60 = 8,   ///< 60个周期
    ADC_SAMPLE_80 = 9,   ///< 80个周期
    ADC_SAMPLE_100 = 10, ///< 100个周期
    ADC_SAMPLE_120 = 11, ///< 120个周期
    ADC_SAMPLE_160 = 12, ///< 160个周期
    ADC_SAMPLE_260 = 13, ///< 260个周期
    ADC_SAMPLE_320 = 14, ///< 320个周期
    ADC_SAMPLE_640 = 15, ///< 640个周期
} adc_sample_time_t;

/**
 * @brief ADC外部触发极性
 */
typedef enum
{
    ADC_TRIGGER_DISABLE = 0, ///< 禁止触发检测
    ADC_TRIGGER_RISING = 1,  ///< 上升沿触发
    ADC_TRIGGER_FALLING = 2, ///< 下降沿触发
    ADC_TRIGGER_BOTH = 3,    ///< 双边沿触发
} adc_trigger_polarity_t;

/**
 * @brief ADC规则转换触发源
 */
typedef enum
{
    ADC_REG_TRIG_TIM1_CC1 = 0,    ///< TIM1_CC1
    ADC_REG_TRIG_TIM1_CC2 = 1,    ///< TIM1_CC2
    ADC_REG_TRIG_TIM1_CC3 = 2,    ///< TIM1_CC3
    ADC_REG_TRIG_TIM2_CC2 = 3,    ///< TIM2_CC2
    ADC_REG_TRIG_TIM3_TRGO = 4,   ///< TIM3_TRGO
    ADC_REG_TRIG_TIM4_CC4 = 5,    ///< TIM4_CC4
    ADC_REG_TRIG_EXTI11 = 6,      ///< EXTI11
    ADC_REG_TRIG_TIM8_TRGO = 7,   ///< TIM8_TRGO
    ADC_REG_TRIG_TIM8_TRGO2 = 8,  ///< TIM8_TRGO2
    ADC_REG_TRIG_TIM1_TRGO = 9,   ///< TIM1_TRGO
    ADC_REG_TRIG_TIM1_TRGO2 = 10, ///< TIM1_TRGO2
    ADC_REG_TRIG_TIM2_TRGO = 11,  ///< TIM2_TRGO
    ADC_REG_TRIG_TIM4_TRGO = 12,  ///< TIM4_TRGO
    ADC_REG_TRIG_TIM6_TRGO = 13,  ///< TIM6_TRGO
    ADC_REG_TRIG_TIM15_TRGO = 14, ///< TIM15_TRGO
    ADC_REG_TRIG_TIM3_CC4 = 15,   ///< TIM3_CC4
    ADC_REG_TRIG_TKEY_OUT = 16,   ///< TKEY_OUT（仅ADC1/2）
    ADC_REG_TRIG_NONE = 17,       ///< 无触发
    ADC_REG_TRIG_LPTIM1_OUT = 18, ///< LPTIM1_OUT
    ADC_REG_TRIG_LPTIM2_OUT = 19, ///< LPTIM2_OUT
    ADC_REG_TRIG_LPTIM3_OUT = 18, ///< LPTIM3_OUT（重复映射）
} adc_regular_trigger_t;

/**
 * @brief ADC注入转换触发源
 */
typedef enum
{
    ADC_INJ_TRIG_TIM1_TRGO = 0,   ///< TIM1_TRGO
    ADC_INJ_TRIG_TIM1_CC4 = 1,    ///< TIM1_CC4
    ADC_INJ_TRIG_TIM2_TRGO = 2,   ///< TIM2_TRGO
    ADC_INJ_TRIG_TIM2_CC1 = 3,    ///< TIM2_CC1
    ADC_INJ_TRIG_TIM3_CC4 = 4,    ///< TIM3_CC4
    ADC_INJ_TRIG_TIM4_TRGO = 5,   ///< TIM4_TRGO
    ADC_INJ_TRIG_EXTI15 = 6,      ///< EXTI15
    ADC_INJ_TRIG_TIM8_CC4 = 7,    ///< TIM8_CC4
    ADC_INJ_TRIG_TIM1_TRGO2 = 8,  ///< TIM1_TRGO2
    ADC_INJ_TRIG_TIM8_TRGO = 9,   ///< TIM8_TRGO
    ADC_INJ_TRIG_TIM8_TRGO2 = 10, ///< TIM8_TRGO2
    ADC_INJ_TRIG_TIM3_CC3 = 11,   ///< TIM3_CC3
    ADC_INJ_TRIG_TIM3_TRGO = 12,  ///< TIM3_TRGO
    ADC_INJ_TRIG_TIM3_CC1 = 13,   ///< TIM3_CC1
    ADC_INJ_TRIG_TIM6_TRGO = 14,  ///< TIM6_TRGO
    ADC_INJ_TRIG_TIM15_TRGO = 15, ///< TIM15_TRGO
    ADC_INJ_TRIG_TKEY_OUT = 16,   ///< TKEY_OUT（仅ADC1/2）
    ADC_INJ_TRIG_NONE = 17,       ///< 无触发
    ADC_INJ_TRIG_LPTIM1_OUT = 18, ///< LPTIM1_OUT
    ADC_INJ_TRIG_LPTIM2_OUT = 19, ///< LPTIM2_OUT
    ADC_INJ_TRIG_LPTIM3_OUT = 18, ///< LPTIM3_OUT（重复映射）
} adc_injected_trigger_t;

/**
 * @brief ADC时钟分频（相对于HCLK或PLLQCLK）
 */
typedef enum
{
    ADC_CLOCK_DIV1 = 0,   ///< 不分频
    ADC_CLOCK_DIV2 = 1,   ///< 2分频
    ADC_CLOCK_DIV3 = 2,   ///< 3分频
    ADC_CLOCK_DIV4 = 3,   ///< 4分频
    ADC_CLOCK_DIV5 = 4,   ///< 5分频
    ADC_CLOCK_DIV6 = 5,   ///< 6分频
    ADC_CLOCK_DIV7 = 6,   ///< 7分频
    ADC_CLOCK_DIV8 = 7,   ///< 8分频
    ADC_CLOCK_DIV9 = 8,   ///< 9分频
    ADC_CLOCK_DIV10 = 9,  ///< 10分频
    ADC_CLOCK_DIV11 = 10, ///< 11分频
    ADC_CLOCK_DIV12 = 11, ///< 12分频
    ADC_CLOCK_DIV13 = 12, ///< 13分频
    ADC_CLOCK_DIV14 = 13, ///< 14分频
    ADC_CLOCK_DIV15 = 14, ///< 15分频
    ADC_CLOCK_DIV16 = 15, ///< 16分频
    ADC_CLOCK_DIV17 = 16, ///< 17分频
    ADC_CLOCK_DIV18 = 17, ///< 18分频
    ADC_CLOCK_DIV19 = 18, ///< 19分频
    ADC_CLOCK_DIV20 = 19, ///< 20分频
    ADC_CLOCK_DIV21 = 20, ///< 21分频
    ADC_CLOCK_DIV22 = 21, ///< 22分频
    ADC_CLOCK_DIV23 = 22, ///< 23分频
    ADC_CLOCK_DIV24 = 23, ///< 24分频
    ADC_CLOCK_DIV25 = 24, ///< 25分频
    ADC_CLOCK_DIV26 = 25, ///< 26分频
    ADC_CLOCK_DIV27 = 26, ///< 27分频
    ADC_CLOCK_DIV28 = 27, ///< 28分频
    ADC_CLOCK_DIV29 = 28, ///< 29分频
    ADC_CLOCK_DIV30 = 29, ///< 30分频
    ADC_CLOCK_DIV31 = 30, ///< 31分频
    ADC_CLOCK_DIV32 = 31, ///< 32分频
    ADC_CLOCK_DIV33 = 32, ///< 33分频
    ADC_CLOCK_DIV34 = 33, ///< 34分频
    ADC_CLOCK_DIV35 = 34, ///< 35分频
    ADC_CLOCK_DIV36 = 35, ///< 36分频
    ADC_CLOCK_DIV37 = 36, ///< 37分频
    ADC_CLOCK_DIV38 = 37, ///< 38分频
    ADC_CLOCK_DIV39 = 38, ///< 39分频
    ADC_CLOCK_DIV40 = 39, ///< 40分频
    ADC_CLOCK_DIV41 = 40, ///< 41分频
    ADC_CLOCK_DIV42 = 41, ///< 42分频
    ADC_CLOCK_DIV43 = 42, ///< 43分频
    ADC_CLOCK_DIV44 = 43, ///< 44分频
    ADC_CLOCK_DIV45 = 44, ///< 45分频
    ADC_CLOCK_DIV46 = 45, ///< 46分频
    ADC_CLOCK_DIV47 = 46, ///< 47分频
    ADC_CLOCK_DIV48 = 47, ///< 48分频
    ADC_CLOCK_DIV49 = 48, ///< 49分频
    ADC_CLOCK_DIV50 = 49, ///< 50分频
    ADC_CLOCK_DIV51 = 50, ///< 51分频
    ADC_CLOCK_DIV52 = 51, ///< 52分频
    ADC_CLOCK_DIV53 = 52, ///< 53分频
    ADC_CLOCK_DIV54 = 53, ///< 54分频
    ADC_CLOCK_DIV55 = 54, ///< 55分频
    ADC_CLOCK_DIV56 = 55, ///< 56分频
    ADC_CLOCK_DIV57 = 56, ///< 57分频
    ADC_CLOCK_DIV58 = 57, ///< 58分频
    ADC_CLOCK_DIV59 = 58, ///< 59分频
    ADC_CLOCK_DIV60 = 59, ///< 60分频
    ADC_CLOCK_DIV61 = 60, ///< 61分频
    ADC_CLOCK_DIV62 = 61, ///< 62分频
    ADC_CLOCK_DIV63 = 62, ///< 63分频
    ADC_CLOCK_DIV64 = 63, ///< 64分频
} adc_clock_divider_t;

/**
 * @brief ADC时钟源
 */
typedef enum
{
    ADC_CLK_SRC_HCLK = 0,       ///< HCLK作为ADC时钟源
    ADC_CLK_SRC_PLLQCLK = 1,    ///< PLL Q输出作为ADC时钟源
} adc_clock_source_t;

/**
 * @brief ADC过采样率
 */
typedef enum
{
    ADC_OVERSAMPLE_2X = 0,   ///< 2倍过采样
    ADC_OVERSAMPLE_4X = 1,   ///< 4倍过采样
    ADC_OVERSAMPLE_8X = 2,   ///< 8倍过采样
    ADC_OVERSAMPLE_16X = 3,  ///< 16倍过采样
    ADC_OVERSAMPLE_32X = 4,  ///< 32倍过采样
    ADC_OVERSAMPLE_64X = 5,  ///< 64倍过采样
    ADC_OVERSAMPLE_128X = 6, ///< 128倍过采样
    ADC_OVERSAMPLE_256X = 7, ///< 256倍过采样
} adc_oversample_rate_t;

/**
 * @brief ADC过采样移位（右移位数）
 */
typedef enum
{
    ADC_OVSS_SHIFT0 = 0, ///< 不移位
    ADC_OVSS_SHIFT1 = 1, ///< 右移1位
    ADC_OVSS_SHIFT2 = 2, ///< 右移2位
    ADC_OVSS_SHIFT3 = 3, ///< 右移3位
    ADC_OVSS_SHIFT4 = 4, ///< 右移4位
    ADC_OVSS_SHIFT5 = 5, ///< 右移5位
    ADC_OVSS_SHIFT6 = 6, ///< 右移6位
    ADC_OVSS_SHIFT7 = 7, ///< 右移7位
    ADC_OVSS_SHIFT8 = 8, ///< 右移8位
} adc_oversample_shift_t;

/**
 * @brief ADC溢出模式
 */
typedef enum
{
    ADC_OVERFLOW_KEEP_OLD = 0,  ///< 溢出时保留旧数据
    ADC_OVERFLOW_OVERWRITE = 1, ///< 溢出时写入新数据
} adc_overflow_mode_t;

/**
 * @brief ADC模拟看门狗模式
 */
typedef enum
{
    ADC_AWD_DISABLED = 0,        ///< 禁用看门狗
    ADC_AWD_ALL_REGULAR = 1,     ///< 所有规则通道
    ADC_AWD_ALL_INJECTED = 2,    ///< 所有注入通道
    ADC_AWD_ALL = 3,             ///< 所有规则和注入通道
    ADC_AWD_SINGLE_REGULAR = 5,  ///< 单个规则通道（AWDCH选择）
    ADC_AWD_SINGLE_INJECTED = 6, ///< 单个注入通道（AWDJCH选择）
    ADC_AWD_SINGLE_ALL = 7,      ///< 单个规则或注入通道
} adc_awd_mode_t;

/**
 * @brief ADC双ADC模式
 */
typedef enum
{
    ADC_DUAL_INDEPENDENT = 0,              ///< 独立模式（默认）
    ADC_DUAL_REG_SYNC_INJ_SYNC = 1,        ///< 规则同步 + 注入同步混合
    ADC_DUAL_REG_SYNC_ALT_TRIG = 2,        ///< 规则同步 + 交替触发混合
    ADC_DUAL_REG_INTERLEAVED_INJ_SYNC = 3, ///< 规则交叉 + 注入同步混合
    ADC_DUAL_INJ_SYNC_ONLY = 5,            ///< 仅注入同步模式
    ADC_DUAL_REG_SYNC_ONLY = 6,            ///< 仅规则同步模式
    ADC_DUAL_REG_INTERLEAVED_ONLY = 7,     ///< 仅规则交叉模式
    ADC_DUAL_ALT_TRIG_ONLY = 9,            ///< 仅交替触发模式
} adc_dual_mode_t;

/**
 * @brief ADC双ADC DMA模式
 */
typedef enum
{
    ADC_DUAL_DMA_INDEPENDENT = 0, ///< 主从ADC各自DMA通道独立
    ADC_DUAL_DMA_COMBINED_12 = 2, ///< 主ADC1产生DMA请求，CDR读取（12/10位）
    ADC_DUAL_DMA_COMBINED_8 = 3,  ///< 主ADC1产生DMA请求，CDR读取（8/6位）
} adc_dual_dma_mode_t;

/**
 * @brief ADC中断类型（位掩码）
 */
typedef enum
{
    ADC_INT_EOSMP = (1 << 1), ///< 规则通道采样完成中断
    ADC_INT_EOC = (1 << 2),   ///< 规则通道转换结束中断
    ADC_INT_EOG = (1 << 3),   ///< 规则组转换结束中断
    ADC_INT_OVERF = (1 << 4), ///< 规则通道数据溢出中断
    ADC_INT_JEOC = (1 << 5),  ///< 注入通道转换结束中断
    ADC_INT_JEOG = (1 << 6),  ///< 注入组转换结束中断
    ADC_INT_AWD = (1 << 7),   ///< 模拟看门狗中断
} adc_interrupt_t;

/**
 * @brief ADC间断模式每触发转换数
 */
typedef enum
{
    ADC_DISCNUM_1 = 0, ///< 每次触发1个通道
    ADC_DISCNUM_2 = 1, ///< 每次触发2个通道
    ADC_DISCNUM_3 = 2, ///< 每次触发3个通道
    ADC_DISCNUM_4 = 3, ///< 每次触发4个通道
    ADC_DISCNUM_5 = 4, ///< 每次触发5个通道
    ADC_DISCNUM_6 = 5, ///< 每次触发6个通道
    ADC_DISCNUM_7 = 6, ///< 每次触发7个通道
    ADC_DISCNUM_8 = 7, ///< 每次触发8个通道
} adc_discnum_t;

/**
 * @brief ADC通道输入模式
 */
typedef enum
{
    ADC_INPUT_SINGLE_ENDED = 0, ///< 单端输入
    ADC_INPUT_DIFFERENTIAL = 1, ///< 差分输入
} adc_input_mode_t;

/**
 * @brief ADC偏移计算方式
 */
typedef enum
{
    ADC_OFFSET_SUBTRACT = 0, ///< 转换结果减去偏移量
    ADC_OFFSET_ADD = 1,      ///< 转换结果加上偏移量
} adc_offset_mode_t;

/**
 * @brief ADC偏移结果格式
 */
typedef enum
{
    ADC_OFFSET_SIGNED = 0,   ///< 有符号数（16位补码）
    ADC_OFFSET_UNSIGNED = 1, ///< 无符号数（最小0x000，最大0xFFF）
} adc_offset_format_t;

/**
 * @brief ADC过采样触发模式
 */
typedef enum
{
    ADC_TROVS_MULTI = 0,  ///< 一次触发进行N次ADC转换（N=过采样率）
    ADC_TROVS_SINGLE = 1, ///< 一次触发进行1次ADC转换
} adc_trovs_mode_t;

//=============================================================================
// 2. 数据结构定义
//=============================================================================

/**
 * @brief ADC规则序列配置结构体
 */
typedef struct
{
    uint8_t length;             ///< 规则序列长度（1-16）
    adc_channel_t sequence[16]; ///< 规则序列通道（SQ1-SQ16）
} adc_regular_config_t;

/**
 * @brief ADC注入序列配置结构体
 */
typedef struct
{
    uint8_t length;            ///< 注入序列长度（1-4）
    adc_channel_t sequence[4]; ///< 注入序列通道（JSQ1-JSQ4）
} adc_injected_config_t;

/**
 * @brief ADC采样时间配置结构体
 */
typedef struct
{
    adc_sample_time_t smp[19]; ///< 各通道采样时间（通道0-18）
} adc_sampling_config_t;

/**
 * @brief ADC模拟看门狗配置结构体
 */
typedef struct
{
    adc_awd_mode_t mode;          ///< 看门狗模式
    adc_channel_t channel;        ///< 单通道模式下选择的通道
    uint16_t high_threshold;      ///< 高阈值（12位无符号）
    uint16_t low_threshold;       ///< 低阈值（12位无符号）
    uint16_t high_threshold_diff; ///< 差分高阈值（12位无符号）
    uint16_t low_threshold_diff;  ///< 差分低阈值（12位无符号）
    bool enable_interrupt;        ///< 使能看门狗中断
} adc_watchdog_config_t;

/**
 * @brief ADC过采样配置结构体
 */
typedef struct
{
    bool enable_regular;           ///< 规则组过采样使能
    bool enable_injected;          ///< 注入组过采样使能
    adc_oversample_rate_t rate;    ///< 过采样率
    adc_oversample_shift_t shift;  ///< 右移位数（0-8）
    adc_trovs_mode_t trigger_mode; ///< 触发模式
} adc_oversampling_config_t;

/**
 * @brief ADC偏移补偿配置结构体
 */
typedef struct
{
    uint8_t index;              ///< 偏移寄存器索引（0-3）
    adc_channel_t channel;      ///< 目标通道
    int16_t offset_value;       ///< 偏移量（12位有符号，0-4095）
    adc_offset_mode_t mode;     ///< 计算方式（加/减）
    adc_offset_format_t format; ///< 结果格式（有符号/无符号）
} adc_offset_config_t;

/**
 * @brief ADC双ADC模式配置结构体（仅ADC12）
 */
typedef struct
{
    adc_dual_mode_t mode;         ///< 双ADC模式
    adc_dual_dma_mode_t dma_mode; ///< 双ADC DMA模式
    uint8_t delay;                ///< 交叉模式下采样延迟（0-15，单位ADC_CLK）
} adc_dual_config_t;

/**
 * @brief ADC时钟配置结构体
 */
typedef struct
{
    adc_clock_source_t source;   ///< 时钟源
    adc_clock_divider_t divider; ///< 分频系数
} adc_clock_config_t;

/**
 * @brief ADC内部通道使能
 */
typedef enum
{
    ADC_INTERNAL_VREFINT = 0,    ///< VREFINT（ADC2）
    ADC_INTERNAL_TEMPSENSOR = 1, ///< 温度传感器（ADC3）
    ADC_INTERNAL_VBAT = 2,       ///< VBAT（ADC3）
} adc_internal_channel_t;

/**
 * @brief ADC回调函数类型
 */
typedef void (*adc_callback_t)(adc_instance_t adc);

//=============================================================================
// 3. 能力查询宏
//=============================================================================

/** @brief 检查ADC是否为主ADC1（用于双ADC模式） */
#define ADC_IS_MASTER(adc)     ((adc) == ADC_1)

/** @brief 检查ADC是否为从ADC2 */
#define ADC_IS_SLAVE(adc)      ((adc) == ADC_2)

/** @brief 检查ADC是否支持双ADC模式 */
#define ADC_HAS_DUAL_MODE(adc) ((adc) == ADC_1 || (adc) == ADC_2)

/** @brief 检查是否为ADC3 */
#define ADC_IS_ADC3(adc)       ((adc) == ADC_3)

//=============================================================================
// 4. 第1层：快速初始化API（4个）
//=============================================================================

/**
 * @brief 快速初始化单通道单次转换
 *
 * 一行代码配置单通道单次转换模式，适用于简单电压采集。
 *
 * @param adc ADC实例
 * @param channel 转换通道
 * @param sample_time 采样时间
 * @return true 初始化成功
 * @return false 初始化失败
 *
 * @note 自动使能ADC时钟和稳压器，等待就绪
 * @note GPIO需要单独配置为模拟模式
 *
 * @code
 * // 初始化ADC1通道3单次转换
 * adc_init_single_channel(ADC1, ADC_CHANNEL_3, ADC_SAMPLE_13);
 * adc_start_regular_conversion(ADC1);  // 启动转换
 * uint16_t val = adc_get_result(ADC1); // 读取结果
 * @endcode
 */
bool adc_init_single_channel(adc_instance_t adc, adc_channel_t channel,
                             adc_sample_time_t sample_time);

/**
 * @brief 快速初始化单通道连续转换
 *
 * @param adc ADC实例
 * @param channel 转换通道
 * @param sample_time 采样时间
 * @return true 初始化成功
 * @return false 初始化失败
 *
 * @note 连续转换模式下，转换结果持续更新
 *
 * @code
 * adc_init_continuous_channel(ADC1, ADC_CHANNEL_3, ADC_SAMPLE_13);
 * // 连续转换自动进行，随时读取最新值
 * uint16_t val = adc_get_result(ADC1);
 * @endcode
 */
bool adc_init_continuous_channel(adc_instance_t adc, adc_channel_t channel,
                                 adc_sample_time_t sample_time);

/**
 * @brief 快速初始化多通道连续转换（DMA模式）
 *
 * @param adc ADC实例
 * @param channels 通道数组
 * @param channel_count 通道数量（1-16）
 * @param sample_time 采样时间（所有通道相同）
 * @param dma_buffer DMA目标缓冲区
 * @return true 初始化成功
 * @return false 初始化失败
 *
 * @note 自动使能DMA请求，转换结果自动传输到dma_buffer
 * @note 需要先初始化DMA控制器并配置DMA通道
 *
 * @code
 * static uint16_t adc_buf[4];
 * adc_channel_t chs[] = {ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5, ADC_CHANNEL_6};
 * adc_init_multi_channel_dma(ADC1, chs, 4, ADC_SAMPLE_13, adc_buf);
 * @endcode
 */
bool adc_init_multi_channel_dma(adc_instance_t adc, const adc_channel_t* channels,
                                uint8_t channel_count, adc_sample_time_t sample_time,
                                uint16_t* dma_buffer);

/**
 * @brief 快速初始化内部通道（温度传感器/VBAT/VREFINT）
 *
 * @param adc ADC实例
 * @param channel 内部通道类型
 * @param sample_time 采样时间（内部通道需要较长时间）
 * @return true 初始化成功
 * @return false 初始化失败
 *
 * @note 自动使能温度传感器/VBAT/VREFINT内部通道
 * @note 温度传感器需要等待1.5us稳定时间
 * @note ADC_INTERNAL_TEMPSENSOR只能用于ADC3
 * @note ADC_INTERNAL_VREFINT只能用于ADC2
 * @note ADC_INTERNAL_VBAT只能用于ADC3
 *
 * @code
 * uint16_t temp_val = adc_init_internal_channel(ADC3, ADC_INTERNAL_TEMPSENSOR, ADC_SAMPLE_160);
 * @endcode
 */
bool adc_init_internal_channel(adc_instance_t adc, adc_internal_channel_t channel,
                               adc_sample_time_t sample_time);

//=============================================================================
// 5. 第2层：基础配置API（8个）
//=============================================================================

/**
 * @brief 配置ADC时钟
 *
 * @param adc ADC实例
 * @param config 时钟配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note ADC_CLK不得超过75MHz
 *
 * @code
 * adc_clock_config_t clk = {
 *     .source = ADC_CLK_SRC_HCLK,
 *     .divider = ADC_CLOCK_DIV4
 * };
 * adc_config_clock(ADC1, &clk);
 * @endcode
 */
bool adc_config_clock(adc_instance_t adc, const adc_clock_config_t* config);

/**
 * @brief 配置分辨率
 *
 * @param adc ADC实例
 * @param resolution 分辨率
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 降低分辨率可缩短转换时间
 *
 * @code
 * adc_config_resolution(ADC1, ADC_RES_10BIT);
 * @endcode
 */
bool adc_config_resolution(adc_instance_t adc, adc_resolution_t resolution);

/**
 * @brief 配置规则序列
 *
 * @param adc ADC实例
 * @param config 规则序列配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_regular_config_t reg = {
 *     .length = 3,
 *     .sequence = {ADC_CHANNEL_3, ADC_CHANNEL_4, ADC_CHANNEL_5}
 * };
 * adc_config_regular_sequence(ADC1, &reg);
 * @endcode
 */
bool adc_config_regular_sequence(adc_instance_t adc, const adc_regular_config_t* config);

/**
 * @brief 配置注入序列
 *
 * @param adc ADC实例
 * @param config 注入序列配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_injected_config_t inj = {
 *     .length = 2,
 *     .sequence = {ADC_CHANNEL_3, ADC_CHANNEL_7}
 * };
 * adc_config_injected_sequence(ADC1, &inj);
 * @endcode
 */
bool adc_config_injected_sequence(adc_instance_t adc, const adc_injected_config_t* config);

/**
 * @brief 配置各通道采样时间
 *
 * @param adc ADC实例
 * @param config 采样时间配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_sampling_config_t smp = {0};
 * smp.smp[3] = ADC_SAMPLE_13;
 * smp.smp[4] = ADC_SAMPLE_13;
 * adc_config_sampling_time(ADC1, &smp);
 * @endcode
 */
bool adc_config_sampling_time(adc_instance_t adc, const adc_sampling_config_t* config);

/**
 * @brief 配置规则转换触发源
 *
 * @param adc ADC实例
 * @param trigger 触发源
 * @param polarity 触发极性
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_config_regular_trigger(ADC1, ADC_REG_TRIG_TIM1_CC1, ADC_TRIGGER_RISING);
 * @endcode
 */
bool adc_config_regular_trigger(adc_instance_t adc, adc_regular_trigger_t trigger,
                                adc_trigger_polarity_t polarity);

/**
 * @brief 配置注入转换触发源
 *
 * @param adc ADC实例
 * @param trigger 触发源
 * @param polarity 触发极性
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_config_injected_trigger(ADC1, ADC_INJ_TRIG_TIM1_TRGO, ADC_TRIGGER_RISING);
 * @endcode
 */
bool adc_config_injected_trigger(adc_instance_t adc, adc_injected_trigger_t trigger,
                                 adc_trigger_polarity_t polarity);

/**
 * @brief 配置模拟看门狗
 *
 * @param adc ADC实例
 * @param config 看门狗配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_watchdog_config_t awd = {
 *     .mode = ADC_AWD_SINGLE_REGULAR,
 *     .channel = ADC_CHANNEL_3,
 *     .high_threshold = 3000,
 *     .low_threshold = 1000,
 *     .enable_interrupt = true
 * };
 * adc_config_watchdog(ADC1, &awd);
 * @endcode
 */
bool adc_config_watchdog(adc_instance_t adc, const adc_watchdog_config_t* config);

//=============================================================================
// 6. 第3层：高级功能API（12个）
//=============================================================================

/**
 * @brief 配置过采样
 *
 * @param adc ADC实例
 * @param config 过采样配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 过采样模式下不支持左对齐和偏移校准
 *
 * @code
 * adc_oversampling_config_t os = {
 *     .enable_regular = true,
 *     .rate = ADC_OVERSAMPLE_16X,
 *     .shift = ADC_OVSS_SHIFT4,
 *     .trigger_mode = ADC_TROVS_MULTI
 * };
 * adc_config_oversampling(ADC1, &os);
 * @endcode
 */
bool adc_config_oversampling(adc_instance_t adc, const adc_oversampling_config_t* config);

/**
 * @brief 配置偏移补偿
 *
 * @param adc ADC实例
 * @param config 偏移补偿配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 过采样模式下偏移补偿被忽略
 *
 * @code
 * adc_offset_config_t of = {
 *     .index = 0,
 *     .channel = ADC_CHANNEL_3,
 *     .offset_value = 100,
 *     .mode = ADC_OFFSET_SUBTRACT,
 *     .format = ADC_OFFSET_SIGNED
 * };
 * adc_config_offset(ADC1, &of);
 * @endcode
 */
bool adc_config_offset(adc_instance_t adc, const adc_offset_config_t* config);

/**
 * @brief 配置通道输入模式（单端/差分）
 *
 * @param adc ADC实例
 * @param channel 通道号
 * @param mode 输入模式
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 通道0为校准通道，其差分模式由ADCCALDIF决定
 * @note 内部通道恒定为单端模式
 *
 * @code
 * adc_config_channel_input_mode(ADC1, ADC_CHANNEL_3, ADC_INPUT_DIFFERENTIAL);
 * @endcode
 */
bool adc_config_channel_input_mode(adc_instance_t adc, adc_channel_t channel,
                                   adc_input_mode_t mode);

/**
 * @brief 配置数据对齐方式
 *
 * @param adc ADC实例
 * @param align 对齐方式
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 过采样模式下不支持左对齐，会被强制右对齐
 *
 * @code
 * adc_config_data_alignment(ADC1, ADC_ALIGN_RIGHT);
 * @endcode
 */
bool adc_config_data_alignment(adc_instance_t adc, adc_data_align_t align);

/**
 * @brief 配置溢出模式
 *
 * @param adc ADC实例
 * @param mode 溢出模式
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_config_overflow_mode(ADC1, ADC_OVERFLOW_KEEP_OLD);
 * @endcode
 */
bool adc_config_overflow_mode(adc_instance_t adc, adc_overflow_mode_t mode);

/**
 * @brief 配置间断模式
 *
 * @param adc ADC实例
 * @param regular_enable 规则组间断使能
 * @param injected_enable 注入组间断使能
 * @param discnum 每触发转换数（规则组）
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 不能同时使用自动注入和间断采样模式
 *
 * @code
 * adc_config_discontinuous_mode(ADC1, true, false, ADC_DISCNUM_3);
 * @endcode
 */
bool adc_config_discontinuous_mode(adc_instance_t adc, bool regular_enable, bool injected_enable,
                                   adc_discnum_t discnum);

/**
 * @brief 使能/禁用自动注入模式
 *
 * @param adc ADC实例
 * @param enable true=使能，false=禁用
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 使能后注入通道在规则组之后自动转换
 * @note 不能同时使用自动注入和间断采样模式
 *
 * @code
 * adc_enable_auto_injected(ADC1, true);
 * @endcode
 */
bool adc_enable_auto_injected(adc_instance_t adc, bool enable);

/**
 * @brief 配置双ADC模式（仅ADC12）
 *
 * @param config 双ADC配置结构体指针
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 必须在使能ADC之前配置
 *
 * @code
 * adc_dual_config_t dual = {
 *     .mode = ADC_DUAL_REG_SYNC_ONLY,
 *     .dma_mode = ADC_DUAL_DMA_COMBINED_12,
 *     .delay = 0
 * };
 * adc_config_dual_mode(&dual);
 * @endcode
 */
bool adc_config_dual_mode(const adc_dual_config_t* config);

/**
 * @brief 使能/禁用内部通道（温度传感器/VBAT/VREFINT）
 *
 * @param adc ADC实例
 * @param channel 内部通道类型
 * @param enable true=使能，false=禁用
 * @return true 配置成功
 * @return false 配置失败
 *
 *
 * @code
 * adc_enable_internal_channel(ADC3, ADC_INTERNAL_TEMPSENSOR, true);
 * @endcode
 */
bool adc_enable_internal_channel(adc_instance_t adc, adc_internal_channel_t channel, bool enable);

/**
 * @brief 配置外部触发极性
 *
 * @param adc ADC实例
 * @param polarity 触发极性
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 同时配置规则和注入触发极性
 */
bool adc_config_trigger_polarity(adc_instance_t adc, adc_trigger_polarity_t polarity);

/**
 * @brief 配置连续转换模式
 *
 * @param adc ADC实例
 * @param enable true=连续转换，false=单次转换
 * @return true 配置成功
 * @return false 配置失败
 *
 * @note 连续转换仅对规则组有效
 *
 * @code
 * adc_enable_continuous_mode(ADC1, true);
 * @endcode
 */
bool adc_enable_continuous_mode(adc_instance_t adc, bool enable);

//=============================================================================
// 7. 第4层：控制与查询API（30个）
//=============================================================================

/**
 * @brief 启动规则转换（软件触发）
 *
 * @param adc ADC实例
 * @return true 启动成功
 * @return false 启动失败
 *
 *
 * @code
 * adc_start_regular_conversion(ADC1);
 * @endcode
 */
bool adc_start_regular_conversion(adc_instance_t adc);

/**
 * @brief 启动注入转换（软件触发）
 *
 * @param adc ADC实例
 * @return true 启动成功
 * @return false 启动失败
 *
 *
 * @code
 * adc_start_injected_conversion(ADC1);
 * @endcode
 */
bool adc_start_injected_conversion(adc_instance_t adc);

/**
 * @brief 停止ADC转换
 *
 * @param adc ADC实例
 * @return true 停止成功
 * @return false 停止失败
 *
 * @note 当前通道转换完成后停止
 * @note 硬件会自动清除ADC_STP位
 *
 * @code
 * adc_stop_conversion(ADC1);
 * @endcode
 */
bool adc_stop_conversion(adc_instance_t adc);

/**
 * @brief 使能ADC
 *
 * @param adc ADC实例
 * @return true 使能成功
 * @return false 使能失败
 *
 * @note 必须先使能稳压器并等待20us再使能ADC
 *
 * @code
 * adc_enable(ADC1);
 * @endcode
 */
bool adc_enable(adc_instance_t adc);

/**
 * @brief 禁用ADC
 *
 * @param adc ADC实例
 * @return true 禁用成功
 * @return false 禁用失败
 *
 *
 * @code
 * adc_disable(ADC1);
 * @endcode
 */
bool adc_disable(adc_instance_t adc);

/**
 * @brief 获取规则转换结果
 *
 * @param adc ADC实例
 * @return 16位转换数据
 *
 * @note 读取ADC_DR寄存器会自动清除EOC标志
 * @note 读取寄存器：DR[DATA]
 *
 * @code
 * uint16_t val = adc_get_result(ADC1);
 * @endcode
 */
uint16_t adc_get_result(adc_instance_t adc);

/**
 * @brief 获取规则转换结果和对应通道号
 *
 * @param adc ADC实例
 * @param channel 输出：转换结果对应的通道号
 * @return 16位转换数据
 *
 * @note 读取寄存器：DR[DATA,CH]
 *
 * @code
 * adc_channel_t ch;
 * uint16_t val = adc_get_result_with_channel(ADC1, &ch);
 * @endcode
 */
uint16_t adc_get_result_with_channel(adc_instance_t adc, adc_channel_t* channel);

/**
 * @brief 获取注入转换结果
 *
 * @param adc ADC实例
 * @param index 注入数据寄存器索引（0-3）
 * @return 16位转换数据
 *
 * @note 读取寄存器：JDRx[JDATA]
 *
 * @code
 * uint16_t val = adc_get_injected_result(ADC1, 0);
 * @endcode
 */
uint16_t adc_get_injected_result(adc_instance_t adc, uint8_t index);

/**
 * @brief 获取双ADC模式组合结果
 *
 * @return 32位组合数据（低16位=ADC1，高16位=ADC2）
 *
 * @note 仅ADC12双ADC模式下使用
 * @note 读取寄存器：ADC12_CDR[DATA1,DATA2]
 *
 * @code
 * uint32_t combined = adc_get_dual_result();
 * uint16_t adc1_val = combined & 0xFFFF;
 * uint16_t adc2_val = combined >> 16;
 * @endcode
 */
uint32_t adc_get_dual_result(void);

/**
 * @brief 获取通道专用数据寄存器的值
 *
 * @param adc ADC实例
 * @param channel 通道号（1-18）
 * @return 16位转换数据
 *
 * @note 读取寄存器：CHDRx
 *
 * @code
 * uint16_t val = adc_get_channel_data(ADC1, ADC_CHANNEL_3);
 * @endcode
 */
uint16_t adc_get_channel_data(adc_instance_t adc, adc_channel_t channel);

/**
 * @brief 检查ADC是否就绪
 *
 * @param adc ADC实例
 * @return true ADC就绪
 *
 * @note 读取寄存器：SR[ADRDY]
 *
 * @code
 * while (!adc_is_ready(ADC1)) {}
 * @endcode
 */
bool adc_is_ready(adc_instance_t adc);

/**
 * @brief 检查规则通道转换是否结束
 *
 * @param adc ADC实例
 * @return true 转换结束
 *
 * @note 读取寄存器：SR[EOC]
 *
 * @code
 * if (adc_is_eoc(ADC1)) {
 *     uint16_t val = adc_get_result(ADC1);
 * }
 * @endcode
 */
bool adc_is_eoc(adc_instance_t adc);

/**
 * @brief 检查规则组转换是否结束
 *
 * @param adc ADC实例
 * @return true 规则组转换完成
 *
 * @note 读取寄存器：SR[EOG]
 */
bool adc_is_eog(adc_instance_t adc);

/**
 * @brief 检查注入通道转换是否结束
 *
 * @param adc ADC实例
 * @return true 注入转换完成
 *
 * @note 读取寄存器：SR[JEOC]
 */
bool adc_is_jeoc(adc_instance_t adc);

/**
 * @brief 检查注入组转换是否结束
 *
 * @param adc ADC实例
 * @return true 注入组转换完成
 *
 * @note 读取寄存器：SR[JEOG]
 */
bool adc_is_jeog(adc_instance_t adc);

/**
 * @brief 检查模拟看门狗事件
 *
 * @param adc ADC实例
 * @return true 看门狗事件发生
 *
 * @note 读取寄存器：SR[AWD]
 */
bool adc_is_awd(adc_instance_t adc);

/**
 * @brief 检查溢出事件
 *
 * @param adc ADC实例
 * @return true 溢出发生
 *
 * @note 读取寄存器：SR[OVERF]
 */
bool adc_is_overflow(adc_instance_t adc);

/**
 * @brief 检查采样是否完成
 *
 * @param adc ADC实例
 * @return true 采样完成
 *
 * @note 读取寄存器：SR[EOSMP]
 */
bool adc_is_eosmp(adc_instance_t adc);

/**
 * @brief 清除状态标志
 *
 * @param adc ADC实例
 * @param flag 要清除的标志位（ADC_INT_xxx可组合）
 * @return true 清除成功
 *
 *
 * @code
 * adc_clear_status_flag(ADC1, ADC_INT_EOC | ADC_INT_OVERF);
 * @endcode
 */
bool adc_clear_status_flag(adc_instance_t adc, uint32_t flag);

/**
 * @brief 使能中断
 *
 * @param adc ADC实例
 * @param interrupt 中断类型（ADC_INT_xxx可组合）
 * @return true 使能成功
 *
 *
 * @code
 * adc_enable_interrupt(ADC1, ADC_INT_EOC | ADC_INT_OVERF);
 * @endcode
 */
bool adc_enable_interrupt(adc_instance_t adc, uint32_t interrupt);

/**
 * @brief 禁用中断
 *
 * @param adc ADC实例
 * @param interrupt 中断类型
 * @return true 禁用成功
 *
 */
bool adc_disable_interrupt(adc_instance_t adc, uint32_t interrupt);

/**
 * @brief 使能DMA请求
 *
 * @param adc ADC实例
 * @return true 使能成功
 *
 *
 * @code
 * adc_enable_dma(ADC1);
 * @endcode
 */
bool adc_enable_dma(adc_instance_t adc);

/**
 * @brief 禁用DMA请求
 *
 * @param adc ADC实例
 * @return true 禁用成功
 *
 */
bool adc_disable_dma(adc_instance_t adc);

/**
 * @brief 执行ADC校准
 *
 * @param adc ADC实例
 * @param differential_mode true=差分模式校准，false=单端模式校准
 * @return true 校准完成
 * @return false 校准失败（超时）
 *
 * @note 校准前ADC必须处于禁用状态（ADC_EN=0）
 * @note 校准时间约数百个ADC_CLK周期
 *
 * @code
 * adc_calibrate(ADC1, false);  // 单端校准
 * @endcode
 */
bool adc_calibrate(adc_instance_t adc, bool differential_mode);

/**
 * @brief 使能ADC稳压器
 *
 * @param adc ADC实例
 * @return true 使能成功
 * @return false 使能失败
 *
 * @note 使能后需要等待20us稳压器稳定
 *
 * @code
 * adc_enable_regulator(ADC1);
 * delay_us(20);
 * adc_enable(ADC1);
 * @endcode
 */
bool adc_enable_regulator(adc_instance_t adc);

/**
 * @brief 禁用ADC稳压器
 *
 * @param adc ADC实例
 * @return true 禁用成功
 *
 */
bool adc_disable_regulator(adc_instance_t adc);

/**
 * @brief 复位ADC模拟前端
 *
 * @param adc ADC实例
 * @return true 复位成功
 *
 * @note 在强制退出某次转换，需要立刻启动一次新的转换时使用
 *
 * @code
 * adc_reset_afe(ADC1);
 * @endcode
 */
bool adc_reset_afe(adc_instance_t adc);

/**
 * @brief 获取校准系数
 *
 * @param adc ADC实例
 * @param differential true=差分系数，false=单端系数
 * @return 7位校准系数值
 *
 * @note 读取寄存器：CALFACT[CALFACT_S,CALFACT_D]
 */
uint8_t adc_get_calibration_factor(adc_instance_t adc, bool differential);

/**
 * @brief 设置校准系数
 *
 * @param adc ADC实例
 * @param differential true=差分系数，false=单端系数
 * @param factor 7位校准系数值
 * @return true 设置成功
 *
 *
 * @code
 * adc_set_calibration_factor(ADC1, false, 0x40);
 * @endcode
 */
bool adc_set_calibration_factor(adc_instance_t adc, bool differential, uint8_t factor);

/**
 * @brief 启用参考电压缓冲器（ADC3）
 *
 * @param voltage 参考电压值（0=1.5V, 1=1.8V, 2=2.0V, 3=2.5V）
 * @return true 启用成功
 *
 *
 * @code
 * adc_enable_vref_buffer(1);  // 1.8V参考电压输出
 * @endcode
 */
bool adc_enable_vref_buffer(uint8_t voltage);

/**
 * @brief 禁用参考电压缓冲器
 *
 * @return true 禁用成功
 *
 */
bool adc_disable_vref_buffer(void);

//=============================================================================
// 8. 回调函数注册
//=============================================================================

/**
 * @brief 注册规则通道转换结束回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_eoc_callback(adc_instance_t adc, adc_callback_t callback);

/**
 * @brief 注册规则组转换结束回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_eog_callback(adc_instance_t adc, adc_callback_t callback);

/**
 * @brief 注册注入通道转换结束回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_jeoc_callback(adc_instance_t adc, adc_callback_t callback);

/**
 * @brief 注册注入组转换结束回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_jeog_callback(adc_instance_t adc, adc_callback_t callback);

/**
 * @brief 注册模拟看门狗事件回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_awd_callback(adc_instance_t adc, adc_callback_t callback);

/**
 * @brief 注册溢出事件回调函数
 *
 * @param adc ADC实例
 * @param callback 回调函数指针
 * @return true 注册成功
 */
bool adc_register_overflow_callback(adc_instance_t adc, adc_callback_t callback);

//=============================================================================
// 9. 辅助函数
//=============================================================================

/**
 * @brief 计算ADC采样率
 *
 * @param adc_clk_hz ADC时钟频率（Hz）
 * @param sample_cycles 采样周期数
 * @param resolution 分辨率（12/10/8/6）
 * @return 采样率（Hz）
 *
 * @code
 * uint32_t rate = adc_calculate_sample_rate(40000000, 13, 12);
 * @endcode
 */
uint32_t adc_calculate_sample_rate(uint32_t adc_clk_hz, uint32_t sample_cycles,
                                   uint32_t resolution);

#ifdef __cplusplus
}
#endif

#endif // ACM32P4_ADC_H
