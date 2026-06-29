/**
 * @file dac.h
 * @brief ACM32P4 DAC驱动 - 数模转换器驱动接口
 *
 * @details
 * 本驱动支持ACM32P4系列芯片的DAC模块所有功能：
 * - 2个DAC（DAC1/2），每个包含2个通道（通道1/2）
 * - 8位或12位分辨率，左对齐或右对齐
 * - 有符号/无符号数输入
 * - LFSR噪声波形生成
 * - 三角波波形生成
 * - 锯齿波波形生成
 * - 双通道独立或同时转换
 * - DMA双数据模式
 * - 每个通道独立DMA功能
 * - 外部触发转换
 * - 输出BUFFER可选，BUFFER偏差校准
 * - 内部互联输出
 * - 采样保持模式
 *
 * 特性：
 * - 4层API架构（快速初始化 → 基础配置 → 高级功能 → 控制查询）
 * - 类型安全（枚举类型，无魔法数字）
 * - 中断/DMA支持
 *
 * @example
 * @code
 * // 快速输出直流电压（第1层API）
 * dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT);
 * dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);  // 输出约1.65V
 *
 * // 完整配置（第2层API）
 * dac_channel_config_t cfg = {
 *     .trigger_source = DAC_TRIG_SW,
 *     .trigger_enable = false,
 *     .waveform = DAC_WAVE_NONE,
 *     .output_mode = DAC_MODE_NORMAL_BUF_PAD,
 * };
 * dac_config_channel(DAC_1, DAC_CHANNEL_1, &cfg);
 * @endcode
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef ACM32P4_DAC_H
#define ACM32P4_DAC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//=============================================================================
// 1. 类型定义
//=============================================================================

/**
 * @brief DAC实例枚举
 */
typedef enum {
    DAC_1     = 0,  ///< DAC1（基地址 0x50000800）
    DAC_COUNT = 1   ///< DAC实例总数
} dac_instance_t;

/**
 * @brief DAC通道枚举（每个DAC实例有2个通道）
 */
typedef enum {
    DAC_CHANNEL_1 = 0,  ///< DAC通道1
    DAC_CHANNEL_2 = 1,  ///< DAC通道2
} dac_channel_t;

/**
 * @brief DAC数据对齐格式
 */
typedef enum {
    DAC_ALIGN_12B_RIGHT = 0,  ///< 12位右对齐
    DAC_ALIGN_12B_LEFT  = 1,  ///< 12位左对齐
    DAC_ALIGN_8B_RIGHT  = 2,  ///< 8位右对齐
} dac_data_align_t;

/**
 * @brief DAC触发源选择（TSELx[3:0]）
 */
typedef enum {
    DAC_TRIG_SW         = 0,  ///< 软件触发（SWTRIG）
    DAC_TRIG_TIM8_TRGO  = 1,  ///< TIM8 TRGO
    DAC_TRIG_TIM7_TRGO  = 2,  ///< TIM7 TRGO
    DAC_TRIG_TIM15_TRGO = 3,  ///< TIM15 TRGO
    DAC_TRIG_TIM2_TRGO  = 4,  ///< TIM2 TRGO
    DAC_TRIG_TIM4_TRGO  = 5,  ///< TIM4 TRGO
    DAC_TRIG_EXTI9      = 6,  ///< EXTI9
    DAC_TRIG_TIM6_TRGO  = 7,  ///< TIM6 TRGO
    DAC_TRIG_TIM3_TRGO  = 8,  ///< TIM3 TRGO
} dac_trigger_source_t;

/**
 * @brief DAC锯齿波递增触发选择（STINCTRIGSELx[3:0]）
 */
typedef enum {
    DAC_ST_TRIG_SW         = 0,  ///< 软件触发B（SWTRIGB）
    DAC_ST_TRIG_TIM8_TRGO  = 1,  ///< TIM8 TRGO
    DAC_ST_TRIG_TIM7_TRGO  = 2,  ///< TIM7 TRGO
    DAC_ST_TRIG_TIM15_TRGO = 3,  ///< TIM15 TRGO
    DAC_ST_TRIG_TIM2_TRGO  = 4,  ///< TIM2 TRGO
    DAC_ST_TRIG_TIM4_TRGO  = 5,  ///< TIM4 TRGO
    DAC_ST_TRIG_EXTI10     = 6,  ///< EXTI10
    DAC_ST_TRIG_TIM6_TRGO  = 7,  ///< TIM6 TRGO
    DAC_ST_TRIG_TIM3_TRGO  = 8,  ///< TIM3 TRGO
} dac_sawtooth_trigger_t;

/**
 * @brief DAC波形生成类型（WAVEx[1:0]）
 */
typedef enum {
    DAC_WAVE_NONE     = 0,  ///< 关闭波形生成
    DAC_WAVE_LFSR     = 1,  ///< LFSR噪声波形
    DAC_WAVE_TRIANGLE = 2,  ///< 三角波
    DAC_WAVE_SAWTOOTH = 3,  ///< 锯齿波
} dac_waveform_t;

/**
 * @brief DAC波形幅度/噪声屏蔽位宽（MAMPx[3:0]）
 */
typedef enum {
    DAC_MAMP_1    = 0,   ///< LFSR[0]有效 / 三角波幅值=1
    DAC_MAMP_3    = 1,   ///< LFSR[1:0]有效 / 三角波幅值=3
    DAC_MAMP_7    = 2,   ///< LFSR[2:0]有效 / 三角波幅值=7
    DAC_MAMP_15   = 3,   ///< LFSR[3:0]有效 / 三角波幅值=15
    DAC_MAMP_31   = 4,   ///< LFSR[4:0]有效 / 三角波幅值=31
    DAC_MAMP_63   = 5,   ///< LFSR[5:0]有效 / 三角波幅值=63
    DAC_MAMP_127  = 6,   ///< LFSR[6:0]有效 / 三角波幅值=127
    DAC_MAMP_255  = 7,   ///< LFSR[7:0]有效 / 三角波幅值=255
    DAC_MAMP_511  = 8,   ///< LFSR[8:0]有效 / 三角波幅值=511
    DAC_MAMP_1023 = 9,   ///< LFSR[9:0]有效 / 三角波幅值=1023
    DAC_MAMP_2047 = 10,  ///< LFSR[10:0]有效 / 三角波幅值=2047
    DAC_MAMP_4095 = 11,  ///< LFSR[11:0]有效 / 三角波幅值=4095
} dac_mamp_t;

/**
 * @brief DAC输出模式（MODEx[2:0]）
 */
typedef enum {
    DAC_MODE_NORMAL_BUF_PAD       = 0,  ///< 正常模式，Buffer使能，输出到PAD
    DAC_MODE_NORMAL_BUF_PAD_OPA   = 1,  ///< 正常模式，Buffer使能，输出到PAD和内部OPA
    DAC_MODE_NORMAL_NOBUF_PAD     = 2,  ///< 正常模式，Buffer禁止，输出到PAD
    DAC_MODE_NORMAL_NOBUF_OPA     = 3,  ///< 正常模式，Buffer禁止，输出到内部OPA
    DAC_MODE_SH_BUF_PAD           = 4,  ///< 采样保持，Buffer使能，输出到PAD
    DAC_MODE_SH_BUF_PAD_OPA       = 5,  ///< 采样保持，Buffer使能，输出到PAD和内部OPA
    DAC_MODE_SH_NOBUF_PAD_OPA     = 6,  ///< 采样保持，Buffer禁止，输出到PAD和内部OPA
    DAC_MODE_SH_NOBUF_OPA         = 7,  ///< 采样保持，Buffer禁止，输出到内部OPA
} dac_output_mode_t;

/**
 * @brief DAC有符号/无符号数格式
 */
typedef enum {
    DAC_FORMAT_UNSIGNED = 0,  ///< 无符号数模式
    DAC_FORMAT_SIGNED   = 1,  ///< 有符号数模式（2s补码）
} dac_signed_format_t;

/**
 * @brief DAC中断类型（位掩码）
 */
typedef enum {
    DAC_INT_DMA_UNDERRUN1 = (1U << 0),  ///< 通道1 DMA下溢中断
    DAC_INT_DMA_UNDERRUN2 = (1U << 1),  ///< 通道2 DMA下溢中断
} dac_interrupt_t;

/**
 * @brief DAC回调函数类型
 *
 * @param dac     DAC实例
 * @param channel 触发回调的通道
 */
typedef void (*dac_callback_t)(dac_instance_t dac, dac_channel_t channel);

//=============================================================================
// 2. 数据结构定义
//=============================================================================

/**
 * @brief DAC通道配置结构体
 *
 * 用于 dac_config_channel() 函数，一次配置通道的所有基本参数。
 */
typedef struct {
    dac_trigger_source_t trigger_source;   ///< 触发源选择
    bool                  trigger_enable;   ///< 硬件触发使能（TENx）
    dac_waveform_t        waveform;         ///< 波形生成类型
    dac_mamp_t            mamp;             ///< 波形幅度/LFSR屏蔽位宽
    dac_output_mode_t     output_mode;      ///< 输出模式（Buffer和PAD配置）
    bool                  dma_enable;       ///< DMA请求使能
    bool                  dma_double_data;  ///< DMA双数据模式使能
    dac_signed_format_t   signed_format;    ///< 有符号/无符号数选择
} dac_channel_config_t;

/**
 * @brief DAC锯齿波配置结构体
 *
 * 用于 dac_config_sawtooth() 函数。
 * @note 锯齿波模式下触发不受TENx控制，由STRSTTRIGSELx和STINCTRIGSELx控制
 */
typedef struct {
    uint16_t               reset_value;    ///< 复位值 STRSTDATAx[11:0]（12位）
    uint16_t               increment;      ///< 递增值 STINCDATAx[15:0]（16位，12.4格式）
    bool                   direction;      ///< 方向（false=递减, true=递增）
    dac_sawtooth_trigger_t inc_trigger;    ///< 递增触发源选择
    dac_trigger_source_t   reset_trigger;  ///< 复位触发源选择（与TSELx一致）
} dac_sawtooth_config_t;

/**
 * @brief DAC采样保持配置结构体
 *
 * 用于 dac_config_sample_hold() 函数。
 * @note 时间单位为RC32K时钟周期（约31.25us/周期）
 */
typedef struct {
    uint16_t sample_time;   ///< 采样时间 TSAMPLEx[9:0]（周期数 = 值 + 1）
    uint16_t hold_time;     ///< 保持时间 THOLDx[9:0]（周期数 = 值）
    uint8_t  refresh_time;  ///< 刷新时间 TREFRESHx[7:0]（周期数 = 值）
} dac_sh_config_t;

//=============================================================================
// 3. 能力查询宏
//=============================================================================

/** @brief 检查DAC实例是否有效 */
#define DAC_IS_VALID(dac) ((dac) == DAC_1 || (dac) == DAC_2)

/** @brief 检查DAC通道是否有效 */
#define DAC_CHANNEL_IS_VALID(ch) ((ch) == DAC_CHANNEL_1 || (ch) == DAC_CHANNEL_2)

//=============================================================================
// 4. 第1层：快速初始化API（3个）
//=============================================================================

/**
 * @brief 快速初始化DAC通道（静态直流输出）
 *
 * 一行代码配置DAC通道输出静态直流电压。
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param align   数据对齐格式
 * @return true  初始化成功
 * @return false 初始化失败
 *
 * @note 自动使能DAC时钟和对应通道
 * @note GPIO需要单独配置为模拟模式
 *
 * @code
 * // 初始化DAC1通道1，12位右对齐
 * dac_init_channel_basic(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT);
 * dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);
 * @endcode
 */
bool dac_init_channel_basic(dac_instance_t dac, dac_channel_t channel,
                             dac_data_align_t align);

/**
 * @brief 快速初始化DAC通道（触发输出模式）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param trigger 触发源
 * @param align   数据对齐格式
 * @return true  初始化成功
 * @return false 初始化失败
 *
 * @note 使能硬件触发，转换为触发驱动模式
 *
 * @code
 * dac_init_channel_triggered(DAC_1, DAC_CHANNEL_1, DAC_TRIG_TIM2_TRGO, DAC_ALIGN_12B_RIGHT);
 * @endcode
 */
bool dac_init_channel_triggered(dac_instance_t dac, dac_channel_t channel,
                                 dac_trigger_source_t trigger, dac_data_align_t align);

/**
 * @brief 快速初始化双通道（同步输出）
 *
 * @param dac   DAC实例
 * @param align 数据对齐格式
 * @return true  初始化成功
 * @return false 初始化失败
 *
 * @note 同时初始化两个通道，用于同步输出电压
 *
 * @code
 * dac_init_dual_channel(DAC_1, DAC_ALIGN_12B_RIGHT);
 * dac_set_dual_data_12r(DAC_1, 2048, 3072);
 * @endcode
 */
bool dac_init_dual_channel(dac_instance_t dac, dac_data_align_t align);

//=============================================================================
// 5. 第2层：基础配置API（5个）
//=============================================================================

/**
 * @brief 配置DAC通道全部参数
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param config  通道配置结构体指针
 * @return true  配置成功
 * @return false 配置失败
 *
 *
 * @code
 * dac_channel_config_t cfg = {
 *     .trigger_source = DAC_TRIG_SW,
 *     .trigger_enable = false,
 *     .waveform = DAC_WAVE_LFSR,
 *     .mamp = DAC_MAMP_7,
 *     .output_mode = DAC_MODE_NORMAL_BUF_PAD,
 *     .dma_enable = false,
 *     .dma_double_data = false,
 *     .signed_format = DAC_FORMAT_UNSIGNED,
 * };
 * dac_config_channel(DAC_1, DAC_CHANNEL_1, &cfg);
 * @endcode
 */
bool dac_config_channel(dac_instance_t dac, dac_channel_t channel,
                         const dac_channel_config_t *config);

/**
 * @brief 配置DAC通道触发源
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param trigger 触发源
 * @param enable  true=使能硬件触发，false=自动传输（软件触发）
 * @return true  配置成功
 * @return false 配置失败
 *
 *
 * @code
 * dac_config_trigger(DAC_1, DAC_CHANNEL_1, DAC_TRIG_TIM2_TRGO, true);
 * @endcode
 */
bool dac_config_trigger(dac_instance_t dac, dac_channel_t channel,
                         dac_trigger_source_t trigger, bool enable);

/**
 * @brief 配置DAC通道波形生成
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param waveform 波形类型
 * @param mamp     幅度/屏蔽位宽
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note LFSR和三角波模式需要使能硬件触发（TENx=1）
 * @note 锯齿波模式需另外调用 dac_config_sawtooth()
 *
 * @code
 * dac_config_waveform(DAC_1, DAC_CHANNEL_1, DAC_WAVE_TRIANGLE, DAC_MAMP_15);
 * @endcode
 */
bool dac_config_waveform(dac_instance_t dac, dac_channel_t channel,
                          dac_waveform_t waveform, dac_mamp_t mamp);

/**
 * @brief 配置DAC输出模式
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param mode    输出模式（Buffer + PAD + 采样保持组合）
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note 必须在DAC通道使能前配置（ENx=0时设置）
 *
 * @code
 * dac_config_output_mode(DAC_1, DAC_CHANNEL_1, DAC_MODE_NORMAL_BUF_PAD);
 * @endcode
 */
bool dac_config_output_mode(dac_instance_t dac, dac_channel_t channel,
                             dac_output_mode_t mode);

/**
 * @brief 配置DAC有符号/无符号数格式
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param format  数据格式
 * @return true  配置成功
 * @return false 配置失败
 *
 *
 * @code
 * dac_config_signed_format(DAC_1, DAC_CHANNEL_1, DAC_FORMAT_SIGNED);
 * @endcode
 */
bool dac_config_signed_format(dac_instance_t dac, dac_channel_t channel,
                               dac_signed_format_t format);

//=============================================================================
// 6. 第3层：高级功能API（5个）
//=============================================================================

/**
 * @brief 配置DAC通道锯齿波参数
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param config  锯齿波配置结构体指针
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note 需要同时将WAVEx设为DAC_WAVE_SAWTOOTH
 * @note 锯齿波模式下触发不受TENx控制
 *
 * @code
 * dac_sawtooth_config_t st = {
 *     .reset_value = 0,
 *     .increment = 0x0100,  // 12.4格式，每次递增16
 *     .direction = true,     // 递增
 *     .inc_trigger = DAC_ST_TRIG_TIM2_TRGO,
 *     .reset_trigger = DAC_TRIG_TIM3_TRGO,
 * };
 * dac_config_sawtooth(DAC_1, DAC_CHANNEL_1, &st);
 * @endcode
 */
bool dac_config_sawtooth(dac_instance_t dac, dac_channel_t channel,
                          const dac_sawtooth_config_t *config);

/**
 * @brief 配置DAC通道采样保持模式
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param config  采样保持配置结构体指针（NULL=禁用采样保持）
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note 采样保持模式需要先将MODEx设为采样保持值
 * @note 时间单位：RC32K周期（约31.25us/周期 @ 32KHz）
 *
 * @code
 * dac_sh_config_t sh = {
 *     .sample_time = 11,   // 12个RC32K周期 ≈ 375us
 *     .hold_time = 62,     // 62个RC32K周期 ≈ 2ms
 *     .refresh_time = 4,   // 4个RC32K周期 ≈ 125us
 * };
 * dac_config_sample_hold(DAC_1, DAC_CHANNEL_1, &sh);
 * @endcode
 */
bool dac_config_sample_hold(dac_instance_t dac, dac_channel_t channel,
                             const dac_sh_config_t *config);

/**
 * @brief 使能DAC通道DMA功能
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param enable  true=使能，false=禁用
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note 使能后，外部触发完成数据传输后自动产生DMA请求
 *
 * @code
 * dac_enable_dma(DAC_1, DAC_CHANNEL_1, true);
 * @endcode
 */
bool dac_enable_dma(dac_instance_t dac, dac_channel_t channel, bool enable);

/**
 * @brief 使能DAC通道DMA双数据模式
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param enable  true=使能双数据模式，false=普通模式
 * @return true  配置成功
 * @return false 配置失败
 *
 * @note 双数据模式：每2次触发产生1次DMA请求，传输2个12位数据
 * @note 仅单通道模式有效，切换模式需先关闭DAC和DMA
 *
 * @code
 * dac_enable_dma_double_data(DAC_1, DAC_CHANNEL_1, true);
 * @endcode
 */
bool dac_enable_dma_double_data(dac_instance_t dac, dac_channel_t channel,
                                 bool enable);

/**
 * @brief 校准DAC通道BUFFER偏差
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  校准完成
 * @return false 校准失败
 *
 * @note 校准前需使能通道（ENx=1），配置CENx位启动校准
 * @note 用SR中CAL_FLAGx检查校准完成
 *
 * @code
 * dac_calibrate_offset(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
bool dac_calibrate_offset(dac_instance_t dac, dac_channel_t channel);

//=============================================================================
// 7. 第4层：控制与查询API（18个）
//=============================================================================

/**
 * @brief 写入12位右对齐数据（单通道）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param data    12位数据（0x000-0xFFF）
 * @return true  写入成功
 * @return false 写入失败
 *
 * @note 若TENx=0，数据自动传输到DORx；若TENx=1，等待触发
 *
 * @code
 * dac_set_data_12r(DAC_1, DAC_CHANNEL_1, 2048);  // 半量程
 * @endcode
 */
bool dac_set_data_12r(dac_instance_t dac, dac_channel_t channel, uint16_t data);

/**
 * @brief 写入12位左对齐数据（单通道）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param data    12位数据（放入[15:4]位）
 * @return true  写入成功
 *
 *
 * @code
 * dac_set_data_12l(DAC_1, DAC_CHANNEL_1, 0x8000);  // 半量程
 * @endcode
 */
bool dac_set_data_12l(dac_instance_t dac, dac_channel_t channel, uint16_t data);

/**
 * @brief 写入8位右对齐数据（单通道）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param data    8位数据（0x00-0xFF，内部映射到[11:4]位）
 * @return true  写入成功
 *
 *
 * @code
 * dac_set_data_8r(DAC_1, DAC_CHANNEL_1, 128);  // 半量程
 * @endcode
 */
bool dac_set_data_8r(dac_instance_t dac, dac_channel_t channel, uint8_t data);

/**
 * @brief 写入双通道12位右对齐数据
 *
 * @param dac   DAC实例
 * @param data1 通道1的12位数据
 * @param data2 通道2的12位数据
 * @return true 写入成功
 *
 *
 * @code
 * dac_set_dual_data_12r(DAC_1, 2048, 3072);
 * @endcode
 */
bool dac_set_dual_data_12r(dac_instance_t dac, uint16_t data1, uint16_t data2);

/**
 * @brief 写入双通道12位左对齐数据
 *
 * @param dac   DAC实例
 * @param data1 通道1的12位数据（放入[15:4]位）
 * @param data2 通道2的12位数据（放入[31:20]位）
 * @return true 写入成功
 *
 */
bool dac_set_dual_data_12l(dac_instance_t dac, uint16_t data1, uint16_t data2);

/**
 * @brief 写入双通道8位右对齐数据
 *
 * @param dac   DAC实例
 * @param data1 通道1的8位数据
 * @param data2 通道2的8位数据
 * @return true 写入成功
 *
 */
bool dac_set_dual_data_8r(dac_instance_t dac, uint8_t data1, uint8_t data2);

/**
 * @brief 写入单通道DMA双数据（DACCxDHRB）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @param align   数据对齐格式
 * @param data_a  当前数据写入DACCxDHR
 * @param data_b  备用数据写入DACCxDHRB
 * @return true  写入成功
 * @return false 写入失败
 *
 * @note DMA双数据模式下，第1次触发输出DHR数据，第2次触发输出DHRB数据
 *
 * @code
 * dac_set_data_double(DAC_1, DAC_CHANNEL_1, DAC_ALIGN_12B_RIGHT, 1000, 2000);
 * @endcode
 */
bool dac_set_data_double(dac_instance_t dac, dac_channel_t channel,
                          dac_data_align_t align, uint16_t data_a, uint16_t data_b);

/**
 * @brief 软件触发DAC转换
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  触发成功
 * @return false 触发失败
 *
 * @note 硬件自动清除SWTRIGx位
 *
 * @code
 * dac_software_trigger(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
bool dac_software_trigger(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 软件触发锯齿波递增（SWTRIGB）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  触发成功
 * @return false 触发失败
 *
 * @note 硬件自动清除SWTRIGBx位
 *
 * @code
 * dac_software_trigger_swtrigb(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
bool dac_software_trigger_swtrigb(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 使能DAC通道
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  使能成功
 * @return false 使能失败
 *
 * @note 使能后需等待tWAKEUP启动时间
 *
 * @code
 * dac_enable(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
bool dac_enable(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 禁用DAC通道
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  禁用成功
 *
 */
bool dac_disable(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 读取DAC通道输出数据寄存器
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return 12位输出数据（DORx[DACCxDOR]）
 *
 * @note 读取寄存器：DORx[DACCxDOR]
 *
 * @code
 * uint16_t val = dac_get_output_data(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
uint16_t dac_get_output_data(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 读取DAC通道输出数据B（DMA双数据模式）
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return 12位备用输出数据（DORx[DACCxDORB]）
 *
 * @note 读取寄存器：DORx[DACCxDORB]
 */
uint16_t dac_get_output_data_b(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 检查DAC通道DMA下溢标志
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  发生DMA下溢
 *
 * @note 读取寄存器：SR[DMAUDRx]
 */
bool dac_is_dma_underrun(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 检查DAC通道校准完成标志
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  校准完成（偏移值达到或超过offset value）
 *
 * @note 读取寄存器：SR[CAL_FLAGx]
 */
bool dac_is_calibration_done(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 检查DAC通道DMA双数据模式当前转换数据
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  当前使用DORB[11:0]数据
 * @return false 当前使用DOR[11:0]数据
 *
 * @note 读取寄存器：SR[DORSTATx]
 */
bool dac_get_dorstat(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 清除DAC状态标志
 *
 * @param dac     DAC实例
 * @param channel DAC通道
 * @return true  清除成功
 *
 * @note 清除DMAUDRx标志（写1清零）
 *
 * @code
 * dac_clear_dma_underrun(DAC_1, DAC_CHANNEL_1);
 * @endcode
 */
bool dac_clear_dma_underrun(dac_instance_t dac, dac_channel_t channel);

/**
 * @brief 使能DAC中断
 *
 * @param dac     DAC实例
 * @param interrupt 中断类型（DAC_INT_xxx可组合）
 * @return true  使能成功
 *
 *
 * @code
 * dac_enable_interrupt(DAC_1, DAC_INT_DMA_UNDERRUN1);
 * @endcode
 */
bool dac_enable_interrupt(dac_instance_t dac, uint32_t interrupt);

/**
 * @brief 禁用DAC中断
 *
 * @param dac     DAC实例
 * @param interrupt 中断类型
 * @return true  禁用成功
 *
 */
bool dac_disable_interrupt(dac_instance_t dac, uint32_t interrupt);

//=============================================================================
// 8. 回调函数注册
//=============================================================================

/**
 * @brief 注册DMA下溢中断回调函数
 *
 * @param dac      DAC实例
 * @param callback 回调函数指针
 * @return true  注册成功
 *
 * @note 回调在中断上下文中执行，需保持简短
 *
 * @code
 * void my_dma_underrun_handler(dac_instance_t dac, dac_channel_t channel)
 * {
 *     // 处理DMA下溢
 * }
 * dac_register_dma_underrun_callback(DAC_1, my_dma_underrun_handler);
 * @endcode
 */
bool dac_register_dma_underrun_callback(dac_instance_t dac, dac_callback_t callback);

#ifdef __cplusplus
}
#endif

#endif // ACM32P4_DAC_H
