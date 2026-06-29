/**
 * @file tim64b.h
 * @brief ACM32H5 64位定时器（TIM64B）驱动头文件
 *
 * 本文件定义了 TIM64B 驱动的公开 API，提供以下功能：
 * - 自由计数模式（持续计数，供上层读取原始计数器值）
 * - 循环计数模式（周期定时中断）
 * - 中断回调管理
 * - 运行时控制与状态查询
 *
 * @note 延时、时间戳、超时等待等高层时间服务由 system.c 基于
 *       tim64b_get_counter() 实现，不属于本驱动职责。
 *
 * @author ACM32H5 SDK Team
 * @version 1.2
 */

#ifndef _HARDWARE_TIM64B_H
#define _HARDWARE_TIM64B_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define TIM64B_IRQn          TIM26_IRQn         // 向量表名固定为 TIM26_IRQHandler，不可修改
#define TIM64B_IRQHandler    TIM26_IRQHandler   // 向量表名固定为 TIM26_IRQHandler，不可修改
#define TIM64B_PRESCALER_MIN  1U   ///< 最小预分频值（分频系数1，不分频）
#define TIM64B_PRESCALER_MAX  64U  ///< 最大预分频值（分频系数64）

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief TIM64B 计数模式
 */
typedef enum {
    TIM64B_MODE_FREERUN  = 0,  ///< 自由计数模式：计数到 2^64-1 后回零，不产生中断
    TIM64B_MODE_PERIODIC = 1,  ///< 循环计数模式：计数到 ARR 后产生中断并回零
} tim64b_mode_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief TIM64B 完整配置结构体
 *
 * 用于 `tim64b_config()` 函数的参数，支持对 TIM64B 所有功能位进行配置。
 */
typedef struct {
    tim64b_mode_t mode;        ///< 计数模式（自由/循环）
    uint8_t      prescaler;   ///< 预分频值（1-64），计数时钟 = 系统时钟 / prescaler
    uint64_t     arr;         ///< 自动重装载值（循环模式下的计数周期，不能为0）
    uint64_t     counter;     ///< 计数器初始值（通常为0）
    bool         arpe;        ///< ARR预装载使能（true=带缓冲，下次更新事件后生效）
    bool         irq_enable;  ///< 中断使能（true=循环到ARR时产生中断）
} tim64b_config_t;

//===========================================
// 回调函数类型定义
//===========================================

/**
 * @brief TIM64B 更新中断回调函数类型
 *
 * 在循环计数模式下，每次计数器达到 ARR 值并回零后，
 * 由 TIM64B_IRQHandler 在清除 UF 标志后调用。
 *
 * @warning 回调函数在中断上下文中执行，应保持简短，避免阻塞操作
 */
typedef void (*tim64b_callback_t)(void);


//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 初始化为自由计数模式
 *
 * 自动使能 TIM64B 外设时钟，配置为自由计数模式：从0向上计数，
 * 溢出（2^64-1→0）后重新计数，不产生中断。
 * 内部通过 `clock_get_pclk2_hz()` 读取当前 PCLK2 频率，
 * 计算 ticks/us、ticks/ms 换算关系，供后续延时与超时等待函数使用。
 *
 * @param[in] prescaler 预分频值（1-64），计数时钟 = PCLK2 / prescaler
 *
 * @note prescaler 超出范围时自动修正为最近有效值（1或64）
 * @note 为获得 1us 精度，建议选择使 PCLK2 / prescaler 为整MHz的组合，
 *       例如 PCLK2=220MHz 时 prescaler=220 → 计数频率 1MHz
 *
 * @code
 * // PCLK2=220MHz，prescaler=220，计数频率1MHz，精度1us
 * tim64b_init_freerun(220);
 * uint64_t ts_us = tim64b_get_counter();  // 时间戳（单位：us）
 * tim64b_delay_ms(100);                   // 延时100ms
 * @endcode
 */
void tim64b_init_freerun(uint8_t prescaler);

/**
 * @brief 快速初始化为周期中断模式
 *
 * 配置 TIM64B 为循环计数模式，计数至 arr 后触发中断并回零。
 * 自动使能外设级中断，用户需额外配置 NVIC 并注册回调。
 *
 * @param[in] arr       自动重装载值（定时周期），不能为0
 * @param[in] prescaler 预分频值（1-64）
 *
 * @note 调用此函数后需使能 NVIC 中断（`NVIC_EnableIRQ(TIM64B_IRQn)`）
 *
 * @code
 * // 示例：系统时钟220MHz，配置1秒定时中断
 * tim64b_init_periodic(180000000ULL, 1);
 * tim64b_irq_register(my_callback);
 * NVIC_EnableIRQ(TIM64B_IRQn);
 * @endcode
 */
void tim64b_init_periodic(uint64_t arr, uint8_t prescaler);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 使用结构体进行完整配置
 *
 * 按照配置结构体的内容，对 TIM64B 进行完整初始化，涵盖所有功能位。
 *
 * @param[in] config 配置结构体指针，不能为 NULL
 * @return true: 配置成功, false: 配置失败（参数无效）
 *
 *
 * @code
 * tim64b_config_t cfg = {
 *     .mode       = TIM64B_MODE_PERIODIC,
 *     .prescaler  = 220,
 *     .arr        = 1000000ULL,
 *     .counter    = 0,
 *     .arpe       = true,
 *     .irq_enable = true,
 * };
 * tim64b_config(&cfg);
 * @endcode
 */
bool tim64b_config(const tim64b_config_t *config);

/**
 * @brief 设置预分频器值
 *
 * 修改预分频器值，新值在下一次更新事件后生效（PSC带缓冲器）。
 *
 * @param[in] prescaler 预分频值（1-64），超出范围自动修正
 *
 */
void tim64b_set_prescaler(uint8_t prescaler);

/**
 * @brief 设置自动重装载值（64位）
 *
 * 写入 ARR 值并触发 ARRLOAD，使新值加载到影子寄存器。
 * 当 arr 为 0 时，计数器不工作。
 *
 * @param[in] arr 自动重装载值（0 ~ 2^64-1）
 *
 * @note 若 ARPE=1，新值在下次更新事件后才对影子寄存器生效
 */
void tim64b_set_arr(uint64_t arr);

/**
 * @brief 设置计数器当前值（64位）
 *
 * 写入计数器值并触发 CNTLOAD，将新值加载到实际计数寄存器。
 *
 * @param[in] cnt 计数器初始值（0 ~ 2^64-1）
 *
 * @warning 每次写完 CNT 寄存器后，必须触发 CNTLOAD 才能生效
 */
void tim64b_set_counter(uint64_t cnt);

/**
 * @brief 设置 ARR 预装载模式
 *
 * 控制 ARR 寄存器是否带缓冲：
 * - false（无缓冲）：写入 ARR 立即生效
 * - true（带缓冲）：写入 ARR 在下次更新事件后生效，避免竞争
 *
 * @param[in] enable true=使能预装载缓冲，false=禁用
 *
 */
void tim64b_set_arpe(bool enable);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 使能或禁用 TIM64B 更新中断
 *
 * 控制 TIM64B 在计数器达到 ARR 并回零时是否产生中断请求。
 * 仅在循环计数模式（TIM64B_MODE_PERIODIC）下有意义。
 *
 * @param[in] enable true=使能中断，false=禁用中断
 *
 * @note 此函数只控制外设级中断使能，NVIC 中断使能需另行配置
 */
void tim64b_irq_enable(bool enable);

/**
 * @brief 注册 TIM64B 更新中断回调函数
 *
 * 注册一个回调函数，在 TIM64B 产生更新中断时由 ISR 调用。
 * 重复注册会覆盖之前的回调。
 *
 * @param[in] callback 回调函数指针，传入 NULL 等同于调用 tim64b_irq_unregister()
 *
 * @note 回调函数在中断上下文中执行，应保持简短，避免阻塞操作
 *
 * @code
 * static void on_tim64b_update(void)
 * {
 *     led_toggle();
 * }
 * tim64b_irq_register(on_tim64b_update);
 * @endcode
 */
void tim64b_irq_register(tim64b_callback_t callback);

/**
 * @brief 注销 TIM64B 更新中断回调函数
 *
 * 清除已注册的中断回调，后续中断产生时不再调用任何用户函数。
 *
 * @note 不影响中断使能状态
 */
void tim64b_irq_unregister(void);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 启动 TIM64B 计数器
 *
 * 使能计数器，TIM64B 开始从当前 CNT 值向上计数。
 *
 */
void tim64b_start(void);

/**
 * @brief 停止 TIM64B 计数器
 *
 * 禁用计数器，CNT 值保持不变。
 *
 */
void tim64b_stop(void);

/**
 * @brief 读取当前 64 位计数器值
 *
 * 按照硬件要求，先读低32位，再读高32位，拼接成64位值返回。
 *
 * @return 当前计数器值（64位无符号整数）
 *
 * @note 必须先读 CNTL 再读 CNTH，以保证读取的原子性
 */
uint64_t tim64b_get_counter(void);

/**
 * @brief 读取当前自动重装载值（64位）
 *
 * @return 当前 ARR 值（64位无符号整数）
 *
 */
uint64_t tim64b_get_arr(void);

/**
 * @brief 读取当前预分频值
 *
 * @return 当前预分频值（1-64）
 *
 */
uint8_t tim64b_get_prescaler(void);

/**
 * @brief 查询计数器是否正在运行
 *
 * @return true: 计数器正在运行，false: 计数器已停止
 *
 */
bool tim64b_is_running(void);

/**
 * @brief 查询更新事件标志（UF）是否置位
 *
 * 在循环计数模式下，计数器达到 ARR 并回零时，硬件置位 UF 标志。
 *
 * @return true: 更新事件已发生，false: 无更新事件
 *
 */
bool tim64b_flag_is_set(void);

/**
 * @brief 清除更新事件标志（UF）
 *
 * 向 UF 位写1以清零。在中断处理函数或轮询模式下调用。
 *
 * @warning 必须在处理更新事件后及时清除，否则会反复触发中断
 */
void tim64b_flag_clear(void);


#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_TIM64B_H
