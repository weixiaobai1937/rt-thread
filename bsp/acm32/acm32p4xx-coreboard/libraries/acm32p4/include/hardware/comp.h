/**
 * @file comp.h
 * @brief ACM32P4 模拟比较器（COMP）驱动
 *
 * @details
 * 功能特性：
 * - 超低功耗模拟电压比较器（COMP1）
 * - 正端输入可配置：PB0、PB2
 * - 负端输入可配置：DAC1输出、PB1、PC4、VREF/AVDD分压
 * - 可编程迟滞窗口（10/20/30/40mV）
 * - 输出极性可配置
 * - 数字滤波功能，滤波周期可配置（1~4095个滤波时钟周期）
 * - 输出消隐功能，消隐源可选TIM1/TIM3/TIM8/TIM15
 * - 可编程VREF/AVDD分压（1/20~16/20）
 * - 寄存器锁定保护
 * - 输出可重定向至定时器刹车/捕获
 * - 输出可作为EXTI中断源，支持Sleep和Stop模式唤醒
 *
 * @note COMP1寄存器基地址：0x40010200
 * @note 中断号：COMP1_IRQn (17)
 * @note EXTI线：EXTI_LINE_COMP1 (19)
 * @note 使用前需使能 COMP 时钟（CLK_CMP1）和滤波时钟
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_COMP_H
#define _HARDWARE_COMP_H

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
 * @brief COMP 数值极限
 */
#define COMP_VREF_CFG_MAX        15U       ///< 分压系数最大值
#define COMP_VREF_DIV_BASE       20U       ///< 分压基数（分母）

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief COMP 实例编号
 */
typedef enum {
    COMP_1 = 0,     ///< COMP1
    COMP_COUNT      ///< COMP 实例总数
} comp_instance_t;

/**
 * @brief COMP 正端输入选择
 */
typedef enum {
    COMP_INP_PB0 = 0,  ///< PB0（模拟功能）
    COMP_INP_PB2 = 1,  ///< PB2（模拟功能）
} comp_inp_sel_t;

/**
 * @brief COMP 负端输入选择
 */
typedef enum {
    COMP_INM_DAC1    = 0,  ///< DAC1 输出（需先使能 DAC）
    COMP_INM_PB1     = 1,  ///< PB1（模拟功能）
    COMP_INM_PC4     = 2,  ///< PC4（模拟功能）
    COMP_INM_VREFDIV = 3,  ///< VREF 或 AVDD 分压（需使能分压并配置分压系数）
} comp_inm_sel_t;

/**
 * @brief COMP 输出极性
 */
typedef enum {
    COMP_POL_NORMAL   = 0,  ///< 直接输出（同相）
    COMP_POL_INVERTED = 1,  ///< 取反输出（反相）
} comp_polarity_t;

/**
 * @brief COMP 迟滞窗口
 *
 * 迟滞比较可防止输入电压接近时输出产生振荡。
 * 最高位为 0 时禁止迟滞功能。
 */
typedef enum {
    COMP_HYS_DISABLE = 0,  ///< 禁止迟滞
    COMP_HYS_10MV    = 4,  ///< 10mV 迟滞窗口
    COMP_HYS_20MV    = 5,  ///< 20mV 迟滞窗口
    COMP_HYS_30MV    = 6,  ///< 30mV 迟滞窗口
    COMP_HYS_40MV    = 7,  ///< 40mV 迟滞窗口
} comp_hysteresis_t;

/**
 * @brief COMP 滤波时间
 *
 * 滤波时钟（FILT_CLK）由 RCC_CCR2.FLTCLK_SEL 选择：
 * - 0：PCLK 的 32 分频
 * - 1：RC32K
 */
typedef enum {
    COMP_FLT_1CYCLE    = 0,  ///< 1 个滤波时钟周期
    COMP_FLT_2CYCLES   = 1,  ///< 2 个滤波时钟周期
    COMP_FLT_4CYCLES   = 2,  ///< 4 个滤波时钟周期
    COMP_FLT_16CYCLES  = 3,  ///< 16 个滤波时钟周期
    COMP_FLT_64CYCLES  = 4,  ///< 64 个滤波时钟周期
    COMP_FLT_256CYCLES = 5,  ///< 256 个滤波时钟周期
    COMP_FLT_1024CYCLES = 6, ///< 1024 个滤波时钟周期
    COMP_FLT_4095CYCLES = 7, ///< 4095 个滤波时钟周期
} comp_filter_time_t;

/**
 * @brief COMP 消隐源选择
 *
 * 消隐功能通过定时器输出切断比较器输出，用于消除尖峰脉冲。
 */
typedef enum {
    COMP_BLANK_NONE      = 0,  ///< 不消隐
    COMP_BLANK_TIM1_OC5  = 1,  ///< TIM1_OC5
    COMP_BLANK_TIM1_OC3  = 2,  ///< TIM1_OC3
    COMP_BLANK_TIM3_OC3  = 3,  ///< TIM3_OC3
    COMP_BLANK_TIM3_OC4  = 4,  ///< TIM3_OC4
    COMP_BLANK_TIM8_OC5  = 5,  ///< TIM8_OC5
    COMP_BLANK_TIM15_OC1 = 6,  ///< TIM15_OC1
    COMP_BLANK_ALL       = 7,  ///< 全消隐
} comp_blank_sel_t;

/**
 * @brief COMP 基准分压源选择
 */
typedef enum {
    COMP_VREF_AVDD = 0,  ///< 选择 AVDD 作为分压源
    COMP_VREF_VREF = 1,  ///< 选择 VREF 作为分压源
} comp_vref_sel_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief 回调函数类型
 *
 * 比较器中断回调函数，在 COMPx_IRQHandler 中调用。
 *
 * @param[in] comp 触发中断的 COMP 实例
 */
typedef void (*comp_callback_t)(comp_instance_t comp);

/**
 * @brief COMP 配置结构体
 *
 * 包含 COMP 外设的所有可配置参数。
 */
typedef struct {
    comp_inp_sel_t     inp_sel;       ///< 正端输入选择
    comp_inm_sel_t     inm_sel;       ///< 负端输入选择
    comp_polarity_t    polarity;      ///< 输出极性
    comp_hysteresis_t  hysteresis;    ///< 迟滞窗口
    bool               filter_enable; ///< 滤波使能
    comp_filter_time_t filter_time;   ///< 滤波时间
    comp_blank_sel_t   blank_sel;     ///< 消隐源选择
    bool               vref_enable;   ///< 基准分压使能
    comp_vref_sel_t    vref_sel;      ///< 基准分压源选择
    uint8_t            vref_cfg;      ///< 分压系数 (0-15)，分压 = (vref_cfg+1)/20
    bool               lock;          ///< 寄存器锁定（置1后无法软件清除）
} comp_config_t;

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化比较器为基本模式
 *
 * 一行代码完成基本比较器的配置和启动。
 * 正端 > 负端时输出高电平（极性为 NORMAL）。
 *
 * @param[in] comp     COMP 实例编号
 * @param[in] inp_sel  正端输入选择
 * @param[in] inm_sel  负端输入选择
 *
 * @note 使用前需配置对应 GPIO 为模拟功能
 * @note 负端选择 DAC1 时需先使能 DAC
 *
 * @code
 * // 初始化 COMP1：正端 PB0，负端 PB1
 * comp_init_basic(COMP_1, COMP_INP_PB0, COMP_INM_PB1);
 * @endcode
 */
void comp_init_basic(comp_instance_t comp, comp_inp_sel_t inp_sel,
                     comp_inm_sel_t inm_sel);

/**
 * @brief 快速初始化比较器为迟滞模式
 *
 * 一行代码完成带迟滞的比较器配置和启动。
 *
 * @param[in] comp        COMP 实例编号
 * @param[in] inp_sel     正端输入选择
 * @param[in] inm_sel     负端输入选择
 * @param[in] hysteresis  迟滞窗口
 *
 * @note 迟滞比较可防止输入电压接近时输出振荡
 *
 * @code
 * // 初始化 COMP1 迟滞比较器：20mV 迟滞窗口
 * comp_init_hysteresis(COMP_1, COMP_INP_PB0, COMP_INM_PB1, COMP_HYS_20MV);
 * @endcode
 */
void comp_init_hysteresis(comp_instance_t comp, comp_inp_sel_t inp_sel,
                          comp_inm_sel_t inm_sel, comp_hysteresis_t hysteresis);

/**
 * @brief 快速初始化比较器为滤波模式
 *
 * 一行代码完成带输出滤波的比较器配置和启动。
 *
 * @param[in] comp        COMP 实例编号
 * @param[in] inp_sel     正端输入选择
 * @param[in] inm_sel     负端输入选择
 * @param[in] filter_time 滤波时间
 *
 * @note 滤波时钟由 RCC_CCR2.FLTCLK_SEL 选择
 *
 * @code
 * // 初始化 COMP1 滤波比较器：64 个滤波时钟周期
 * comp_init_filtered(COMP_1, COMP_INP_PB0, COMP_INM_PB1, COMP_FLT_64CYCLES);
 * @endcode
 */
void comp_init_filtered(comp_instance_t comp, comp_inp_sel_t inp_sel,
                        comp_inm_sel_t inm_sel, comp_filter_time_t filter_time);

/**
 * @brief 快速初始化比较器为内部分压模式
 *
 * 一行代码完成使用 VREF/AVDD 内部分压作为负端输入的配置和启动。
 *
 * @param[in] comp      COMP 实例编号
 * @param[in] inp_sel   正端输入选择
 * @param[in] vref_sel  分压源选择（AVDD 或 VREF）
 * @param[in] vref_cfg  分压系数 (0-15)，分压 = (vref_cfg+1)/20 × 分压源电压
 *
 * @note 负端电压 = (vref_cfg + 1) / 20 × VREF（或 AVDD）
 *
 * @code
 * // 使用 VREF 分压，系数 10 → 负端 = 11/20 × VREF
 * comp_init_vref_divider(COMP_1, COMP_INP_PB0, COMP_VREF_VREF, 10);
 * @endcode
 */
void comp_init_vref_divider(comp_instance_t comp, comp_inp_sel_t inp_sel,
                            comp_vref_sel_t vref_sel, uint8_t vref_cfg);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 COMP 并启动
 *
 * 灵活的完整配置接口，适用于需要精确控制所有参数的场景。
 *
 * @param[in] comp    COMP 实例编号
 * @param[in] config  COMP 配置结构体指针
 * @return true  配置成功
 * @return false 参数无效（config 为 NULL 或参数超出范围）
 *
 * @note 此函数会先禁用 COMP，配置完成后重新使能
 *
 * @code
 * comp_config_t cfg = {
 *     .inp_sel = COMP_INP_PB0,
 *     .inm_sel = COMP_INM_PB1,
 *     .polarity = COMP_POL_NORMAL,
 *     .hysteresis = COMP_HYS_DISABLE,
 *     .filter_enable = false,
 *     .filter_time = COMP_FLT_1CYCLE,
 *     .blank_sel = COMP_BLANK_NONE,
 *     .vref_enable = false,
 *     .vref_sel = COMP_VREF_AVDD,
 *     .vref_cfg = 0,
 *     .lock = false,
 * };
 * comp_config(COMP_1, &cfg);
 * @endcode
 */
bool comp_config(comp_instance_t comp, const comp_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 锁定 COMP 配置寄存器
 *
 * 锁定后 COMP_CR 变为只读，无法再修改配置。
 * 只能通过复位 MCU 或 COMP 模块复位清除。
 *
 * @param[in] comp  COMP 实例编号
 *
 * @note 此操作不可逆（无法通过软件解锁）
 * @warning 锁定前请确认配置正确
 */
void comp_lock(comp_instance_t comp);

/**
 * @brief 注册 COMP 中断回调函数
 *
 * 注册的回调函数将在 COMPx_IRQHandler 中被调用。
 * 传递 NULL 可取消回调。
 *
 * @param[in] comp     COMP 实例编号
 * @param[in] callback 回调函数指针，NULL 表示取消回调
 *
 * @note 回调函数在中断上下文中执行，应尽量简短
 *
 * @code
 * void my_comp_handler(comp_instance_t comp)
 * {
 *     if (comp_get_output(comp)) {
 *         // 正端 > 负端
 *     }
 * }
 * comp_register_callback(COMP_1, my_comp_handler);
 * @endcode
 */
void comp_register_callback(comp_instance_t comp, comp_callback_t callback);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能比较器
 *
 * @param[in] comp  COMP 实例编号
 */
void comp_enable(comp_instance_t comp);

/**
 * @brief 禁止比较器
 *
 * @param[in] comp  COMP 实例编号
 */
void comp_disable(comp_instance_t comp);

/**
 * @brief 设置正端输入源
 *
 * @param[in] comp    COMP 实例编号
 * @param[in] inp_sel 正端输入选择
 *
 * @note 对应 GPIO 需配置为模拟功能
 */
void comp_set_positive_input(comp_instance_t comp, comp_inp_sel_t inp_sel);

/**
 * @brief 获取正端输入源配置
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前正端输入选择
 */
comp_inp_sel_t comp_get_positive_input(comp_instance_t comp);

/**
 * @brief 设置负端输入源
 *
 * @param[in] comp    COMP 实例编号
 * @param[in] inm_sel 负端输入选择
 *
 * @note 选择 DAC1 时需先使能 DAC
 * @note 选择 VREFDIV 时需配置分压参数
 */
void comp_set_negative_input(comp_instance_t comp, comp_inm_sel_t inm_sel);

/**
 * @brief 获取负端输入源配置
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前负端输入选择
 */
comp_inm_sel_t comp_get_negative_input(comp_instance_t comp);

/**
 * @brief 设置输出极性
 *
 * @param[in] comp      COMP 实例编号
 * @param[in] polarity  输出极性
 */
void comp_set_polarity(comp_instance_t comp, comp_polarity_t polarity);

/**
 * @brief 获取输出极性配置
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前输出极性
 */
comp_polarity_t comp_get_polarity(comp_instance_t comp);

/**
 * @brief 设置迟滞窗口
 *
 * @param[in] comp        COMP 实例编号
 * @param[in] hysteresis  迟滞窗口
 *
 * @note 设为 COMP_HYS_DISABLE 可禁止迟滞功能
 */
void comp_set_hysteresis(comp_instance_t comp, comp_hysteresis_t hysteresis);

/**
 * @brief 获取迟滞窗口配置
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前迟滞窗口
 */
comp_hysteresis_t comp_get_hysteresis(comp_instance_t comp);

/**
 * @brief 配置输出滤波
 *
 * @param[in] comp        COMP 实例编号
 * @param[in] enable      是否使能滤波
 * @param[in] filter_time 滤波时间
 *
 * @note 滤波时钟由 RCC_CCR2.FLTCLK_SEL 选择
 */
void comp_set_filter(comp_instance_t comp, bool enable,
                     comp_filter_time_t filter_time);

/**
 * @brief 获取滤波配置
 *
 * @param[in]  comp         COMP 实例编号
 * @param[out] filter_time  滤波时间输出（可为 NULL）
 * @return true  滤波已使能
 * @return false 滤波未使能
 */
bool comp_get_filter(comp_instance_t comp, comp_filter_time_t *filter_time);

/**
 * @brief 设置消隐源
 *
 * @param[in] comp       COMP 实例编号
 * @param[in] blank_sel  消隐源选择
 *
 * @note 消隐源来自定时器，需先配置对应定时器
 */
void comp_set_blanking(comp_instance_t comp, comp_blank_sel_t blank_sel);

/**
 * @brief 获取消隐源配置
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前消隐源选择
 */
comp_blank_sel_t comp_get_blanking(comp_instance_t comp);

/**
 * @brief 配置基准分压
 *
 * 使能并配置 VREF/AVDD 内部分压，作为比较器负端输入。
 *
 * @param[in] comp      COMP 实例编号
 * @param[in] enable    是否使能分压
 * @param[in] vref_sel  分压源选择
 * @param[in] vref_cfg  分压系数 (0-15)，分压 = (vref_cfg+1)/20 × 分压源
 *
 * @note 仅在负端选择 VREFDIV (COMP_INM_VREFDIV) 时有效
 */
void comp_set_vref_divider(comp_instance_t comp, bool enable,
                           comp_vref_sel_t vref_sel, uint8_t vref_cfg);

/**
 * @brief 获取分压系数
 *
 * @param[in] comp  COMP 实例编号
 * @return 当前分压系数 (0-15)
 */
uint8_t comp_get_vref_cfg(comp_instance_t comp);

/**
 * @brief 获取比较器滤波输出状态
 *
 * 读取经过滤波和极性处理后的比较器输出。
 *
 * @param[in] comp  COMP 实例编号
 * @return true  正端 > 负端（极性处理后）
 * @return false 正端 < 负端（极性处理后）
 *
 * @note 若使能了滤波，此值为滤波后的结果
 */
bool comp_get_output(comp_instance_t comp);

/**
 * @brief 获取比较器原始输出状态
 *
 * 读取未经滤波的比较器原始输出。
 *
 * @param[in] comp  COMP 实例编号
 * @return true  正端 > 负端（原始输出）
 * @return false 正端 < 负端（原始输出）
 */
bool comp_get_output_raw(comp_instance_t comp);

/**
 * @brief 检查 COMP 是否已锁定
 *
 * @param[in] comp  COMP 实例编号
 * @return true  已锁定（COMP_CR 只读）
 * @return false 未锁定
 */
bool comp_is_locked(comp_instance_t comp);

/**
 * @brief 检查 COMP 是否已使能
 *
 * @param[in] comp  COMP 实例编号
 * @return true  已使能
 * @return false 未使能
 */
bool comp_is_enabled(comp_instance_t comp);

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief COMP1 中断服务程序
 *
 * 应用程序应在中断向量表中映射此函数。
 * 此函数会调用已注册的用户回调函数。
 *
 * @note 中断号：COMP1_IRQn (17)
 * @note 需要在 NVIC 中使能 COMP1_IRQn
 * @note 比较器输出变化（上升沿/下降沿）会触发此中断
 */
void COMP1_IRQHandler(void);

/**
 * @brief 计算内部分压电压值
 *
 * 根据分压源电压和分压系数计算负端输入电压。
 * 此函数为纯计算函数，不访问硬件寄存器。
 *
 * @param[in] vref_sel  分压源选择
 * @param[in] vref_mv   分压源电压（mV），AVDD 或 VREF 的实际电压
 * @param[in] vref_cfg  分压系数 (0-15)
 * @return 负端输入电压（mV）
 *
 * @note 公式：V_inm = (vref_cfg + 1) / 20 × vref_mv
 */
uint32_t comp_calculate_vref_divider_mv(comp_vref_sel_t vref_sel,
                                        uint32_t vref_mv, uint8_t vref_cfg);

#ifdef __cplusplus
}
#endif
#endif // _HARDWARE_COMP_H
