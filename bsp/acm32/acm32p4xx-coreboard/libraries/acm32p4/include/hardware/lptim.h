/**
 * @file lptim.h
 * @brief ACM32P4 LPTIM 低功耗定时器驱动
 *
 * @details
 * 功能特性：
 * - 16 位递增计数器
 * - 3 位预分频器（1/2/4/8/16/32/64/128）
 * - 多时钟源：PCLK/RC32K/RC64M/XTL/外部 Input1
 * - 16 位自动重载（ARR）和比较（CMP）寄存器
 * - 连续/单次模式
 * - 软件/硬件输入触发（8 个触发源）
 * - 可编程数字干扰滤波器（外部时钟 + 触发）
 * - PWM/单脉冲/置1一次 波形输出
 * - 编码器模式
 * - 重复计数器（8 位）
 * - 超时功能
 * - 寄存器预装载（PRELOAD）
 *
 * @note 6 个 LPTIM 实例（LPTIM1 ~ LPTIM6）
 * @note LPTIM1 基地址：0x40007C00，LPTIM2：0x40009400
 * @note LPTIM3~6 基地址：0x51000000 ~ 0x51000C00
 * @note CFGR1/IER 需在 ENABLE=0 时修改；CMP/ARR/RCR/CFGR2 需在 ENABLE=1 时修改
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_LPTIM_H
#define _HARDWARE_LPTIM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define LPTIM_COUNT               1U       ///< LPTIM 实例数量

#define LPTIM_PRESCALER_MAX       7U       ///< 预分频最大值（对应 128 分频）
#define LPTIM_CMP_MAX             65535U   ///< 比较值最大值（16 位）
#define LPTIM_ARR_MAX             65535U   ///< 自动重载最大值（16 位）
#define LPTIM_ARR_MIN             1U       ///< 自动重载最小值
#define LPTIM_REP_MAX             255U     ///< 重复计数器最大值（8 位）
#define LPTIM_TRIGSEL_MAX         15U      ///< 触发源选择最大值
#define LPTIM_FILTER_MAX          3U       ///< 滤波器采样数最大值
#define LPTIM_INPUT_SEL_MAX       15U      ///< 输入多路选择最大值

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief LPTIM 实例选择
 */
typedef enum {
    LPTIM_1 = 0,  ///< LPTIM1
} lptim_instance_t;

/**
 * @brief LPTIM 时钟源选择
 */
typedef enum {
    LPTIM_CLK_INTERNAL = 0,  ///< 内部时钟源（PCLK/RC32K/RC64M/XTL）
    LPTIM_CLK_EXTERNAL = 1,  ///< 外部时钟源（Input1）
} lptim_clock_sel_t;

/**
 * @brief LPTIM 计数模式
 */
typedef enum {
    LPTIM_COUNT_INTERNAL = 0,  ///< 内部时钟脉冲计数
    LPTIM_COUNT_EXTERNAL = 1,  ///< 外部 Input1 有效边沿计数
} lptim_count_mode_t;

/**
 * @brief LPTIM 时钟极性/边沿
 */
typedef enum {
    LPTIM_EDGE_RISING       = 0,  ///< 上升沿
    LPTIM_EDGE_FALLING      = 1,  ///< 下降沿
    LPTIM_EDGE_BOTH         = 2,  ///< 上升沿和下降沿（需内部时钟 ≥4× 外部频率）
} lptim_edge_t;

/**
 * @brief LPTIM 预分频
 */
typedef enum {
    LPTIM_PRESCALER_1   = 0,  ///< 1 分频
    LPTIM_PRESCALER_2   = 1,  ///< 2 分频
    LPTIM_PRESCALER_4   = 2,  ///< 4 分频
    LPTIM_PRESCALER_8   = 3,  ///< 8 分频
    LPTIM_PRESCALER_16  = 4,  ///< 16 分频
    LPTIM_PRESCALER_32  = 5,  ///< 32 分频
    LPTIM_PRESCALER_64  = 6,  ///< 64 分频
    LPTIM_PRESCALER_128 = 7,  ///< 128 分频
} lptim_prescaler_t;

/**
 * @brief LPTIM 触发使能与极性
 */
typedef enum {
    LPTIM_TRIG_SOFTWARE  = 0,  ///< 软件触发（CNTSTRT/SNGSTRT）
    LPTIM_TRIG_RISING    = 1,  ///< 外部触发上升沿
    LPTIM_TRIG_FALLING   = 2,  ///< 外部触发下降沿
    LPTIM_TRIG_BOTH      = 3,  ///< 外部触发双沿
} lptim_trig_en_t;

/**
 * @brief LPTIM 数字滤波器采样数
 */
typedef enum {
    LPTIM_FILTER_NONE  = 0,  ///< 无滤波（直接采样）
    LPTIM_FILTER_2     = 1,  ///< 2 个连续相同采样
    LPTIM_FILTER_4     = 2,  ///< 4 个连续相同采样
    LPTIM_FILTER_8     = 3,  ///< 8 个连续相同采样
} lptim_filter_t;

/**
 * @brief LPTIM 波形模式
 */
typedef enum {
    LPTIM_WAVE_PWM_PULSE = 0,  ///< PWM 或单脉冲（取决于 CNTSTRT/SNGSTRT）
    LPTIM_WAVE_SET_ONCE  = 1,  ///< 置1一次模式
} lptim_wave_mode_t;

/**
 * @brief LPTIM 波形极性
 */
typedef enum {
    LPTIM_POL_NORMAL = 0,  ///< CNT < CMP 输出低，CNT ≥ CMP 输出高
    LPTIM_POL_INVERT = 1,  ///< CNT < CMP 输出高，CNT ≥ CMP 输出低
} lptim_polarity_t;

/**
 * @brief LPTIM 寄存器更新模式
 */
typedef enum {
    LPTIM_UPDATE_IMMEDIATE = 0,  ///< 立即更新（APB 写后生效）
    LPTIM_UPDATE_PRELOAD   = 1,  ///< 预装载更新（当前周期结束时更新）
} lptim_update_mode_t;

/**
 * @brief LPTIM 重复寄存器加载模式
 */
typedef enum {
    LPTIM_REP_LOAD_IMMEDIATE = 0,  ///< APB 写后立即更新（4 个内部时钟后有效）
    LPTIM_REP_LOAD_UNDERFLOW = 1,  ///< 重复计数器下溢时更新
} lptim_rep_load_mode_t;

/**
 * @brief LPTIM 中断类型（位掩码）
 */
typedef enum {
    LPTIM_IRQ_CMPM    = (1U << 0),  ///< 比较匹配中断
    LPTIM_IRQ_ARRM    = (1U << 1),  ///< 自动重载匹配中断
    LPTIM_IRQ_EXTTRIG = (1U << 2),  ///< 外部触发中断
    LPTIM_IRQ_CMPOK   = (1U << 3),  ///< 比较寄存器更新完成中断
    LPTIM_IRQ_ARROK   = (1U << 4),  ///< 自动重载寄存器更新完成中断
    LPTIM_IRQ_UP      = (1U << 5),  ///< 方向变递增中断
    LPTIM_IRQ_DOWN    = (1U << 6),  ///< 方向变递减中断
    LPTIM_IRQ_REPUE   = (1U << 7),  ///< 重复计数器下溢中断
    LPTIM_IRQ_REPOK   = (1U << 8),  ///< 重复寄存器更新完成中断
} lptim_irq_t;

/**
 * @brief LPTIM 状态标志（位掩码）
 */
typedef enum {
    LPTIM_FLAG_CMPM    = (1U << 0),  ///< 比较匹配标志
    LPTIM_FLAG_ARRM    = (1U << 1),  ///< 自动重载匹配标志
    LPTIM_FLAG_EXTTRIG = (1U << 2),  ///< 外部触发标志
    LPTIM_FLAG_CMPOK   = (1U << 3),  ///< 比较寄存器更新完成标志
    LPTIM_FLAG_ARROK   = (1U << 4),  ///< 自动重载寄存器更新完成标志
    LPTIM_FLAG_UP      = (1U << 5),  ///< 方向变递增标志
    LPTIM_FLAG_DOWN    = (1U << 6),  ///< 方向变递减标志
    LPTIM_FLAG_REPUE   = (1U << 7),  ///< 重复计数器下溢标志
    LPTIM_FLAG_REPOK   = (1U << 8),  ///< 重复寄存器更新完成标志
} lptim_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief LPTIM 配置结构体
 */
typedef struct {
    lptim_clock_sel_t   clock_sel;       ///< 时钟源选择
    lptim_count_mode_t  count_mode;      ///< 计数模式
    lptim_edge_t        edge;            ///< 时钟极性/边沿
    lptim_prescaler_t   prescaler;       ///< 预分频
    lptim_trig_en_t     trig_en;         ///< 触发使能与极性
    uint8_t             trig_sel;        ///< 触发源选择（0~15）
    lptim_filter_t      ck_filter;       ///< 外部时钟滤波器
    lptim_filter_t      trig_filter;     ///< 触发信号滤波器
    lptim_wave_mode_t   wave;            ///< 波形模式
    lptim_polarity_t    polarity;        ///< 波形极性
    lptim_update_mode_t preload;         ///< 寄存器更新模式
    lptim_rep_load_mode_t rep_load;      ///< 重复寄存器加载模式
    bool                timeout_en;      ///< 超时功能使能
    bool                encoder_en;      ///< 编码器模式使能
} lptim_config_t;

//===========================================
// 回调函数类型
//===========================================

typedef void (*lptim_callback_t)(lptim_instance_t lptim, uint32_t flags);  ///< LPTIM 中断回调

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 LPTIM 为内部时钟计数器模式
 *
 * @param[in] lptim     LPTIM 实例
 * @param[in] prescaler 预分频
 * @param[in] arr       自动重载值（1~65535）
 * @param[in] continuous true: 连续模式, false: 单次模式
 *
 * @note 使用内部时钟源，软件触发启动
 *
 * @code
 * lptim_init_counter(LPTIM_1, LPTIM_PRESCALER_1, 1000U, true);
 * @endcode
 */
void lptim_init_counter(lptim_instance_t lptim, lptim_prescaler_t prescaler,
                        uint16_t arr, bool continuous);

/**
 * @brief 快速初始化 LPTIM 为 PWM 输出模式
 *
 * @param[in] lptim     LPTIM 实例
 * @param[in] prescaler 预分频
 * @param[in] arr       自动重载值（1~65535）
 * @param[in] cmp       比较值（0 < CMP < ARR）
 *
 * @note CMP < ARR 必须满足，否则无输出
 * @note 使用内部时钟源，连续模式
 *
 * @code
 * lptim_init_pwm(LPTIM_1, LPTIM_PRESCALER_1, 1000U, 500U);
 * @endcode
 */
void lptim_init_pwm(lptim_instance_t lptim, lptim_prescaler_t prescaler,
                    uint16_t arr, uint16_t cmp);

/**
 * @brief 快速初始化 LPTIM 为编码器模式
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] edge   编码器子模式（有效边沿）
 * @param[in] arr    自动重载值（1~65535）
 *
 * @note 编码器模式要求 CKSEL=0（内部时钟）、PRESC=000（1分频）
 * @note Input1 和 Input2 信号频率不得超过内部时钟 4 分频
 *
 * @code
 * lptim_init_encoder(LPTIM_1, LPTIM_EDGE_RISING, 4096U);
 * @endcode
 */
void lptim_init_encoder(lptim_instance_t lptim, lptim_edge_t edge, uint16_t arr);

/**
 * @brief 快速初始化 LPTIM 为外部时钟脉冲计数器
 *
 * @param[in] lptim     LPTIM 实例
 * @param[in] edge      有效计数边沿
 * @param[in] arr       自动重载值（1~65535）
 * @param[in] continuous true: 连续模式, false: 单次模式
 *
 * @note 无需内部时钟源即可工作（使能滤波器时除外）
 * @note 使能后前 5 个 Input1 有效边沿将丢失
 *
 * @code
 * lptim_init_ext_counter(LPTIM_1, LPTIM_EDGE_RISING, 100U, true);
 * @endcode
 */
void lptim_init_ext_counter(lptim_instance_t lptim, lptim_edge_t edge,
                            uint16_t arr, bool continuous);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体完整配置 LPTIM
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] config 配置结构体
 * @return true  配置成功
 * @return false 参数无效
 *
 * @note 需在 ENABLE=0 时调用
 *
 * @code
 * lptim_config_t cfg = {
 *     .clock_sel = LPTIM_CLK_INTERNAL,
 *     .count_mode = LPTIM_COUNT_INTERNAL,
 *     .edge = LPTIM_EDGE_RISING,
 *     .trig_en = LPTIM_TRIG_SOFTWARE,
 *     .trig_sel = 0,
 *     .ck_filter = LPTIM_FILTER_NONE,
 *     .trig_filter = LPTIM_FILTER_NONE,
 *     .wave = LPTIM_WAVE_PWM_PULSE,
 *     .polarity = LPTIM_POL_NORMAL,
 *     .preload = LPTIM_UPDATE_IMMEDIATE,
 *     .rep_load = LPTIM_REP_LOAD_IMMEDIATE,
 *     .timeout_en = false,
 *     .encoder_en = false,
 * };
 * lptim_config(LPTIM_1, &cfg);
 * @endcode
 */
bool lptim_config(lptim_instance_t lptim, const lptim_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

// --- 启动/停止 ---

/**
 * @brief 以连续模式启动计数器
 *
 * @param[in] lptim LPTIM 实例
 *
 * @note ENABLE=1 时才能置位，硬件自动清零
 */
void lptim_start_continuous(lptim_instance_t lptim);

/**
 * @brief 以单次模式启动计数器
 *
 * @param[in] lptim LPTIM 实例
 *
 * @note ENABLE=1 时才能置位，硬件自动清零
 */
void lptim_start_single(lptim_instance_t lptim);

// --- CMP/ARR ---

/**
 * @brief 设置比较值
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] cmp   比较值（0~65535）
 *
 * @note 必须在 ENABLE=1 时修改
 * @note CMP < ARR 时才能生成有效的波形输出
 */
void lptim_set_compare(lptim_instance_t lptim, uint16_t cmp);

/**
 * @brief 获取比较值
 *
 * @param[in] lptim LPTIM 实例
 * @return 当前比较值
 */
uint16_t lptim_get_compare(lptim_instance_t lptim);

/**
 * @brief 设置自动重载值
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] arr   自动重载值（1~65535）
 *
 * @note 必须在 ENABLE=1 时修改
 * @note ARR 必须严格大于 CMP
 */
void lptim_set_autoreload(lptim_instance_t lptim, uint16_t arr);

/**
 * @brief 获取自动重载值
 *
 * @param[in] lptim LPTIM 实例
 * @return 当前自动重载值
 */
uint16_t lptim_get_autoreload(lptim_instance_t lptim);

// --- 重复计数器 ---

/**
 * @brief 设置重复计数器值
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] rep   重复值（0~255），0 表示每个溢出都产生下溢事件
 *
 * @note 必须在 ENABLE=1 时修改
 * @note 使用重复计数器时 PRELOAD 必须为 1
 */
void lptim_set_repeat(lptim_instance_t lptim, uint8_t rep);

/**
 * @brief 获取重复计数器值
 *
 * @param[in] lptim LPTIM 实例
 * @return 当前重复寄存器值
 */
uint8_t lptim_get_repeat(lptim_instance_t lptim);

// --- 更新等待 ---

/**
 * @brief 等待 ARR 寄存器写操作完成
 *
 * @param[in] lptim   LPTIM 实例
 * @param[in] timeout 超时时间（系统 tick）
 * @return true: 完成, false: 超时
 *
 * @note 查询 ARROK 标志
 */
bool lptim_wait_arr_ok(lptim_instance_t lptim, uint32_t timeout);

/**
 * @brief 等待 CMP 寄存器写操作完成
 *
 * @param[in] lptim   LPTIM 实例
 * @param[in] timeout 超时时间（系统 tick）
 * @return true: 完成, false: 超时
 *
 * @note 查询 CMPOK 标志
 */
bool lptim_wait_cmp_ok(lptim_instance_t lptim, uint32_t timeout);

/**
 * @brief 等待 REP 寄存器写操作完成
 *
 * @param[in] lptim   LPTIM 实例
 * @param[in] timeout 超时时间（系统 tick）
 * @return true: 完成, false: 超时
 *
 * @note 查询 REPOK 标志
 */
bool lptim_wait_rep_ok(lptim_instance_t lptim, uint32_t timeout);

// --- 输入选择 ---

/**
 * @brief 设置 Input1 多路选择
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] sel   输入选择（0~15）
 *
 * @note 必须在 ENABLE=1 时修改
 */
void lptim_set_input1_sel(lptim_instance_t lptim, uint8_t sel);

/**
 * @brief 设置 Input2 多路选择
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] sel   输入选择（0~15）
 *
 * @note 必须在 ENABLE=1 时修改
 */
void lptim_set_input2_sel(lptim_instance_t lptim, uint8_t sel);

//===========================================
// 第4层：控制与查询 API
//===========================================

// --- CR 控制寄存器 ---

/**
 * @brief 使能/禁止 LPTIM
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 使能后延迟 2 个计数器时钟周期生效
 */
void lptim_enable(lptim_instance_t lptim, bool enable);

/**
 * @brief 检查 LPTIM 是否已使能
 *
 * @param[in] lptim LPTIM 实例
 * @return true: 已使能, false: 已禁止
 */
bool lptim_is_enabled(lptim_instance_t lptim);

/**
 * @brief 同步复位计数器（COUNTRST）
 *
 * @param[in] lptim LPTIM 实例
 *
 * @note 复位在约 3 个内核时钟后生效
 * @note COUNTRST 硬件清零前不可再次置位
 */
void lptim_counter_reset(lptim_instance_t lptim);

/**
 * @brief 使能/禁止读后复位（RSTARE）
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] enable true: 使能读后异步复位, false: 禁止
 *
 * @note 使能后任何对 CNT 的读访问都会异步复位 CNT
 * @note 该位 WO，不可读回验证
 */
void lptim_set_reset_after_read(lptim_instance_t lptim, bool enable);

// --- CFGR1 ---

/**
 * @brief 设置时钟源
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] sel   时钟源
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_clock_sel(lptim_instance_t lptim, lptim_clock_sel_t sel);

/**
 * @brief 获取时钟源
 *
 * @param[in] lptim LPTIM 实例
 * @return 时钟源
 */
lptim_clock_sel_t lptim_get_clock_sel(lptim_instance_t lptim);

/**
 * @brief 设置计数模式
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] mode  计数模式
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_count_mode(lptim_instance_t lptim, lptim_count_mode_t mode);

/**
 * @brief 获取计数模式
 *
 * @param[in] lptim LPTIM 实例
 * @return 计数模式
 */
lptim_count_mode_t lptim_get_count_mode(lptim_instance_t lptim);

/**
 * @brief 设置时钟边沿
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] edge  边沿
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_edge(lptim_instance_t lptim, lptim_edge_t edge);

/**
 * @brief 获取时钟边沿
 *
 * @param[in] lptim LPTIM 实例
 * @return 时钟边沿
 */
lptim_edge_t lptim_get_edge(lptim_instance_t lptim);

/**
 * @brief 设置预分频
 *
 * @param[in] lptim     LPTIM 实例
 * @param[in] prescaler 预分频
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_prescaler(lptim_instance_t lptim, lptim_prescaler_t prescaler);

/**
 * @brief 获取预分频
 *
 * @param[in] lptim LPTIM 实例
 * @return 预分频
 */
lptim_prescaler_t lptim_get_prescaler(lptim_instance_t lptim);

/**
 * @brief 设置触发配置
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] trig   触发使能与极性
 * @param[in] sel    触发源选择（0~15）
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_trigger(lptim_instance_t lptim, lptim_trig_en_t trig, uint8_t sel);

/**
 * @brief 获取触发使能配置
 *
 * @param[in] lptim LPTIM 实例
 * @return 触发使能与极性
 */
lptim_trig_en_t lptim_get_trigger(lptim_instance_t lptim);

/**
 * @brief 获取触发源选择
 *
 * @param[in] lptim LPTIM 实例
 * @return 触发源选择
 */
uint8_t lptim_get_trigger_sel(lptim_instance_t lptim);

/**
 * @brief 设置数字滤波器
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] ck_filter   外部时钟滤波器
 * @param[in] trig_filter 触发信号滤波器
 *
 * @note 需在 ENABLE=0 时修改
 * @note 使能滤波器必须有内部时钟源
 */
void lptim_set_filter(lptim_instance_t lptim, lptim_filter_t ck_filter,
                      lptim_filter_t trig_filter);

/**
 * @brief 获取外部时钟滤波器配置
 *
 * @param[in] lptim LPTIM 实例
 * @return 滤波器配置
 */
lptim_filter_t lptim_get_ck_filter(lptim_instance_t lptim);

/**
 * @brief 获取触发信号滤波器配置
 *
 * @param[in] lptim LPTIM 实例
 * @return 滤波器配置
 */
lptim_filter_t lptim_get_trig_filter(lptim_instance_t lptim);

/**
 * @brief 设置波形模式
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] mode  波形模式
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_wave_mode(lptim_instance_t lptim, lptim_wave_mode_t mode);

/**
 * @brief 获取波形模式
 *
 * @param[in] lptim LPTIM 实例
 * @return 波形模式
 */
lptim_wave_mode_t lptim_get_wave_mode(lptim_instance_t lptim);

/**
 * @brief 设置波形极性
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] pol   波形极性
 *
 * @note 更改立即生效（即使 ENABLE=0 时也会更改默认输出电平）
 */
void lptim_set_polarity(lptim_instance_t lptim, lptim_polarity_t pol);

/**
 * @brief 获取波形极性
 *
 * @param[in] lptim LPTIM 实例
 * @return 波形极性
 */
lptim_polarity_t lptim_get_polarity(lptim_instance_t lptim);

/**
 * @brief 设置寄存器更新模式
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] mode  更新模式
 *
 * @note 需在 ENABLE=0 时修改
 * @note 使用重复计数器时 PRELOAD 必须为 1
 */
void lptim_set_preload(lptim_instance_t lptim, lptim_update_mode_t mode);

/**
 * @brief 获取寄存器更新模式
 *
 * @param[in] lptim LPTIM 实例
 * @return 更新模式
 */
lptim_update_mode_t lptim_get_preload(lptim_instance_t lptim);

/**
 * @brief 使能/禁止超时功能
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 需在 ENABLE=0 时修改
 * @note 使能后触发事件将复位计数器
 */
void lptim_set_timeout(lptim_instance_t lptim, bool enable);

/**
 * @brief 获取超时功能状态
 *
 * @param[in] lptim LPTIM 实例
 * @return true: 使能, false: 禁止
 */
bool lptim_get_timeout(lptim_instance_t lptim);

/**
 * @brief 使能/禁止编码器模式
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 需在 ENABLE=0 时修改
 * @note 编码器模式要求 CKSEL=0、PRESC=1（1分频）
 */
void lptim_set_encoder(lptim_instance_t lptim, bool enable);

/**
 * @brief 获取编码器模式状态
 *
 * @param[in] lptim LPTIM 实例
 * @return true: 使能, false: 禁止
 */
bool lptim_get_encoder(lptim_instance_t lptim);

/**
 * @brief 设置重复寄存器加载模式
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] mode  加载模式
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_set_rep_load_mode(lptim_instance_t lptim, lptim_rep_load_mode_t mode);

/**
 * @brief 获取重复寄存器加载模式
 *
 * @param[in] lptim LPTIM 实例
 * @return 加载模式
 */
lptim_rep_load_mode_t lptim_get_rep_load_mode(lptim_instance_t lptim);

// --- CNT 计数器 ---

/**
 * @brief 获取计数器值（可靠读取）
 *
 * @param[in] lptim LPTIM 实例
 * @return 计数器值（16位）
 *
 * @note 异步时钟时连续读取两次，相等时取值；RSTARE 使能时不可用
 */
uint16_t lptim_get_counter(lptim_instance_t lptim);

// --- IE 中断使能 ---

/**
 * @brief 使能/禁止指定中断
 *
 * @param[in] lptim  LPTIM 实例
 * @param[in] irq    中断类型（可组合）
 * @param[in] enable true: 使能, false: 禁止
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_irq_enable(lptim_instance_t lptim, lptim_irq_t irq, bool enable);

/**
 * @brief 检查指定中断是否已使能
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] irq   中断类型
 * @return true: 已使能, false: 已禁止
 */
bool lptim_irq_is_enabled(lptim_instance_t lptim, lptim_irq_t irq);

/**
 * @brief 禁用所有中断
 *
 * @param[in] lptim LPTIM 实例
 *
 * @note 需在 ENABLE=0 时修改
 */
void lptim_irq_disable_all(lptim_instance_t lptim);

// --- ISR 状态查询与清除 ---

/**
 * @brief 获取状态标志
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] flag  状态标志
 * @return true: 置位, false: 未置位
 *
 */
bool lptim_get_flag(lptim_instance_t lptim, lptim_flag_t flag);

/**
 * @brief 清除状态标志
 *
 * @param[in] lptim LPTIM 实例
 * @param[in] flag  要清除的标志
 *
 */
void lptim_clear_flag(lptim_instance_t lptim, lptim_flag_t flag);

/**
 * @brief 获取中断状态寄存器原始值
 *
 * @param[in] lptim LPTIM 实例
 * @return ISR 寄存器值
 */
uint32_t lptim_get_status(lptim_instance_t lptim);

// --- 中断回调 ---

/**
 * @brief 注册 LPTIM 中断回调
 *
 * @param[in] lptim    LPTIM 实例
 * @param[in] callback 回调函数（传入 NULL 取消注册）
 *
 * @note 回调参数 flags 为当前 ISR 值
 */
void lptim_register_callback(lptim_instance_t lptim, lptim_callback_t callback);

// --- 时钟 ---

/**
 * @brief 使能 LPTIM 时钟
 *
 * @param[in] lptim LPTIM 实例
 */
void lptim_enable_clock(lptim_instance_t lptim);

/**
 * @brief 禁止 LPTIM 时钟
 *
 * @param[in] lptim LPTIM 实例
 */
void lptim_disable_clock(lptim_instance_t lptim);

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 根据预分频和时钟频率计算定时周期
 *
 * @param[in] prescaler 预分频值
 * @param[in] clk_hz    时钟频率 (Hz)
 * @return 一个计数周期的时间（纳秒）
 *
 * @note 实际周期 = (prescaler + 1) / clk_hz * 1e9 ns
 */
uint32_t lptim_calculate_tick_ns(lptim_prescaler_t prescaler, uint32_t clk_hz);

/**
 * @brief 根据目标时间和时钟频率计算 ARR 值
 *
 * @param[in] period_ns  目标周期（纳秒）
 * @param[in] prescaler  预分频值
 * @param[in] clk_hz     时钟频率 (Hz)
 * @return ARR 值（1~65535），0 表示无法达到目标周期
 */
uint16_t lptim_calculate_arr(uint32_t period_ns, lptim_prescaler_t prescaler,
                             uint32_t clk_hz);

//===========================================
// 中断处理函数声明
//===========================================

/**
 * @brief LPTIM 中断处理函数
 *
 * @param[in] lptim LPTIM 实例
 *
 * @note 用户需在对应的 LPTIMx_IRQHandler 中调用此函数
 */
void lptim_irq_handler(lptim_instance_t lptim);

#ifdef __cplusplus
}
#endif
#endif // _HARDWARE_LPTIM_H
