/**
 * @file exti.c
 * @brief ACM32P4 外部中断/事件控制器(EXTI)驱动实现
 *
 * 本文件实现了ACM32P4芯片的EXTI驱动功能，提供了以下接口：
 * - GPIO外部中断配置（支持16个GPIO线）
 * - 内部模块事件配置（支持9个内部线）
 * - 中断触发方式配置（上升沿/下降沿/双边沿）
 * - 中断/事件模式切换
 * - 软件触发中断
 * - 中断标志查询和清除
 * - 回调函数管理
 * - 便捷初始化接口
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#include <assert.h>
#include "device/acm32p4xx.h"
#include "hardware/clocks.h"
#include "hardware/exti.h"

//===========================================
// 内部宏定义
//===========================================

// 参数有效性检查
#define IS_EXTI_LINE(line)       ((line) <= 24)
#define IS_EXTI_GPIO_LINE(line)  ((line) <= 15)
#define IS_EXTI_PIN(pin)         ((pin) <= 15)
#define IS_EXTI_TRIGGER(trig)    (((trig) >= 1) && ((trig) <= 3))
#define IS_EXTI_MODE(mode)       (((mode) == 0) || ((mode) == 1))

//===========================================
// 内部状态变量
//===========================================

// 回调函数数组 (25个线路: 0-24)
static exti_callback_t s_exti_callbacks[25] = {NULL};

//===========================================
// 第1层：底层寄存器操作 API
//===========================================

/**
 * @brief 设置EXTI线中断使能
 *
 * 直接操作IENR寄存器，使能或禁用指定EXTI线的中断功能。
 *
 * @param[in] line   线号 (0-24)
 * @param[in] enable true=使能中断，false=禁用中断
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 * @note 此为底层API，一般用户应使用更高级的接口
 */
void exti_hw_set_interrupt_enable(uint32_t line, bool enable)
{
    assert(IS_EXTI_LINE(line));

    if (enable) {
        EXTI->IENR |= (1U << line);
    } else {
        EXTI->IENR &= ~(1U << line);
    }
}

/**
 * @brief 设置EXTI线事件使能
 *
 * 直接操作EENR寄存器，使能或禁用指定EXTI线的事件功能。
 *
 * @param[in] line   线号 (0-24)
 * @param[in] enable true=使能事件，false=禁用事件
 *
 * @note 覆盖寄存器：EXTI_EENR.EVENx
 * @note 事件模式不触发中断，仅用于DMA/定时器等外设联动
 */
void exti_hw_set_event_enable(uint32_t line, bool enable)
{
    assert(IS_EXTI_LINE(line));

    if (enable) {
        EXTI->EENR |= (1U << line);
    } else {
        EXTI->EENR &= ~(1U << line);
    }
}

/**
 * @brief 设置上升沿触发使能
 *
 * 配置EXTI线是否响应上升沿触发。
 *
 * @param[in] line   线号 (0-24)
 * @param[in] enable true=使能上升沿触发，false=禁用
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx
 */
void exti_hw_set_rising_edge(uint32_t line, bool enable)
{
    assert(IS_EXTI_LINE(line));

    if (enable) {
        EXTI->RTENR |= (1U << line);
    } else {
        EXTI->RTENR &= ~(1U << line);
    }
}

/**
 * @brief 设置下降沿触发使能
 *
 * 配置EXTI线是否响应下降沿触发。
 *
 * @param[in] line   线号 (0-24)
 * @param[in] enable true=使能下降沿触发，false=禁用
 *
 * @note 覆盖寄存器：EXTI_FTENR.FTENx
 */
void exti_hw_set_falling_edge(uint32_t line, bool enable)
{
    assert(IS_EXTI_LINE(line));

    if (enable) {
        EXTI->FTENR |= (1U << line);
    } else {
        EXTI->FTENR &= ~(1U << line);
    }
}

/**
 * @brief 获取EXTI挂起标志
 *
 * 读取指定EXTI线的挂起标志位。
 *
 * @param[in] line 线号 (0-24)
 * @return true 有挂起的中断/事件
 * @return false 无挂起
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx
 */
bool exti_hw_get_pending_flag(uint32_t line)
{
    assert(IS_EXTI_LINE(line));

    return (EXTI->PDR & (1U << line)) != 0;
}

/**
 * @brief 清除EXTI挂起标志
 *
 * 通过写1清除指定EXTI线的挂起标志位。
 *
 * @param[in] line 线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx (写1清除)
 * @note 必须在中断处理函数中调用以清除中断标志
 */
void exti_hw_clear_pending_flag(uint32_t line)
{
    assert(IS_EXTI_LINE(line));

    // 写1清除挂起标志 (RC_W1)
    EXTI->PDR = (1U << line);
}

/**
 * @brief 配置EXTI线的GPIO源
 *
 * 设置GPIO线0-15对应的GPIO端口源（GPIOA/B/C等）。
 * 每线占用4位：线0-7写入EXTICR1，线8-15写入EXTICR2。
 *
 * @param[in] line       GPIO线号 (0-15)
 * @param[in] port_value GPIO端口编号 (0x0-0x7 对应 PA-PH)
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy
 * @note 仅GPIO线0-15需要配置，内部线16-24无需此配置
 */
void exti_hw_set_exticr(uint32_t line, uint32_t port_value)
{
    assert(IS_EXTI_GPIO_LINE(line));

    // 每个线占用4位，线0-7在EXTICR1，线8-15在EXTICR2
    uint32_t reg_index = line / 8;
    uint32_t bit_offset = (line % 8) * 4;
    uint32_t mask = 0x0FU << bit_offset;

    volatile uint32_t *exticr_reg;
    switch (reg_index) {
        case 0: exticr_reg = &EXTI->EXTICR1; break;
        case 1: exticr_reg = &EXTI->EXTICR2; break;
        default: return;
    }

    // 清除旧值并设置新值（仅低4位有效）
    *exticr_reg = (*exticr_reg & ~mask) | ((port_value & 0x0F) << bit_offset);
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 设置EXTI线的GPIO源
 *
 * 配置GPIO外部中断线对应的GPIO端口。
 *
 * @param[in] line      GPIO线号 (0-15)
 * @param[in] gpio_port GPIO端口
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy
 * @note 仅适用于GPIO线0-15
 *
 * @code
 * // 配置EXTI0连接到GPIOA的引脚0
 * exti_set_gpio_source(EXTI_LINE_0, EXTI_GPIO_PORTA);
 * @endcode
 */
void exti_set_gpio_source(exti_line_t line, exti_gpio_port_t gpio_port)
{
    assert(IS_EXTI_GPIO_LINE(line));
    exti_hw_set_exticr(line, gpio_port);
}

/**
 * @brief 设置EXTI触发方式
 *
 * 配置EXTI线的触发边沿类型。
 *
 * @param[in] line    线号 (0-24)
 * @param[in] trigger 触发方式
 *                    - EXTI_TRIGGER_RISING: 上升沿触发
 *                    - EXTI_TRIGGER_FALLING: 下降沿触发
 *                    - EXTI_TRIGGER_BOTH: 双边沿触发
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx, EXTI_FTENR.FTENx
 *
 * @code
 * // 配置为双边沿触发
 * exti_set_trigger(EXTI_LINE_0, EXTI_TRIGGER_BOTH);
 * @endcode
 */
void exti_set_trigger(exti_line_t line, exti_trigger_t trigger)
{
    assert(IS_EXTI_LINE(line));
    assert(IS_EXTI_TRIGGER(trigger));

    // 配置触发边沿
    bool rising = (trigger & EXTI_TRIGGER_RISING) != 0;
    bool falling = (trigger & EXTI_TRIGGER_FALLING) != 0;

    exti_hw_set_rising_edge(line, rising);
    exti_hw_set_falling_edge(line, falling);
}

/**
 * @brief 设置EXTI工作模式
 *
 * 配置EXTI线工作在中断模式或事件模式。
 *
 * @param[in] line 线号 (0-24)
 * @param[in] mode 工作模式
 *                 - EXTI_MODE_INTERRUPT: 中断模式（触发CPU中断）
 *                 - EXTI_MODE_EVENT: 事件模式（仅触发硬件事件，不触发CPU中断）
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx, EXTI_EENR.EVENx
 * @note 事件模式用于DMA、定时器等外设的硬件触发，不产生CPU中断
 *
 * @code
 * // 配置为中断模式
 * exti_set_mode(EXTI_LINE_0, EXTI_MODE_INTERRUPT);
 * @endcode
 */
void exti_set_mode(exti_line_t line, exti_mode_t mode)
{
    assert(IS_EXTI_LINE(line));
    assert(IS_EXTI_MODE(mode));

    if (mode == EXTI_MODE_INTERRUPT) {
        // 中断模式: 使能中断,禁用事件
        exti_hw_set_interrupt_enable(line, true);
        exti_hw_set_event_enable(line, false);
    } else {
        // 事件模式: 使能事件,禁用中断
        exti_hw_set_interrupt_enable(line, false);
        exti_hw_set_event_enable(line, true);
    }
}

/**
 * @brief 使能EXTI线
 *
 * 使能指定EXTI线的中断功能。
 *
 * @param[in] line 线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 * @note 使能前需要先配置GPIO源、触发方式等
 * @note 还需在NVIC中使能对应的中断向量
 *
 * @code
 * // 使能EXTI0
 * exti_enable(EXTI_LINE_0);
 * @endcode
 */
void exti_enable(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));

    // 使能中断模式
    exti_hw_set_interrupt_enable(line, true);
}

/**
 * @brief 禁用EXTI线
 *
 * 禁用指定EXTI线的中断和事件功能，并清除挂起标志。
 *
 * @param[in] line 线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx, EXTI_EENR.EVENx, EXTI_PDR.PDx
 * @note 不会禁用NVIC中的中断向量（因为可能影响共享线）
 *
 * @code
 * // 禁用EXTI0
 * exti_disable(EXTI_LINE_0);
 * @endcode
 */
void exti_disable(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));

    // 禁用中断和事件
    exti_hw_set_interrupt_enable(line, false);
    exti_hw_set_event_enable(line, false);

    // 清除挂起标志，避免残留中断触发
    exti_hw_clear_pending_flag(line);

    // 注: 不禁用NVIC中断,因为可能影响其他共享线
}

/**
 * @brief 检查EXTI线是否已使能
 *
 * @param[in] line 线号 (0-24)
 * @return true EXTI线已使能
 * @return false EXTI线未使能
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 *
 * @code
 * if (exti_is_enabled(EXTI_LINE_0)) {
 *     printf("EXTI0 is enabled\n");
 * }
 * @endcode
 */
bool exti_is_enabled(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));

    return (EXTI->IENR & (1U << line)) != 0;
}

/**
 * @brief 软件触发EXTI中断
 *
 * 通过软件设置SWIER寄存器对应位，模拟硬件事件触发。
 * 前提条件：EXTI线必须已使能中断（IENR对应位置位）。
 * SWIER写1后，硬件自动将PDR对应位置位；若该线中断已使能且
 * NVIC已开启，则触发中断进入对应ISR。
 *
 * @param[in] line 线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_SWIER.SWIEx （间接影响 EXTI_PDR.PDx）
 * @note 用于测试中断处理程序或模拟外部事件
 *
 * @code
 * // 软件触发EXTI0中断
 * exti_trigger_software(EXTI_LINE_0);
 * @endcode
 */
void exti_trigger_software(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));

    EXTI->SWIER = (1U << line);
}

/**
 * @brief 获取EXTI挂起状态
 *
 * 查询指定EXTI线是否有挂起的中断/事件。
 *
 * @param[in] line 线号 (0-24)
 * @return true 有挂起的中断/事件
 * @return false 无挂起
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx
 *
 * @code
 * if (exti_get_pending(EXTI_LINE_0)) {
 *     // 处理中断...
 *     exti_clear_pending(EXTI_LINE_0);
 * }
 * @endcode
 */
bool exti_get_pending(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));
    return exti_hw_get_pending_flag(line);
}

/**
 * @brief 清除EXTI挂起标志
 *
 * 清除指定EXTI线的挂起标志位。
 *
 * @param[in] line 线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx (写1清除)
 * @note 必须在中断处理函数中调用以避免重复触发
 *
 * @code
 * // 在中断处理函数中清除标志
 * exti_clear_pending(EXTI_LINE_0);
 * @endcode
 */
void exti_clear_pending(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));
    exti_hw_clear_pending_flag(line);
}

/**
 * @brief 设置EXTI回调函数
 *
 * 为指定EXTI线注册中断回调函数。
 *
 * @param[in] line     线号 (0-24)
 * @param[in] callback 回调函数指针
 *
 * @note 覆盖寄存器：无 (纯软件管理)
 * @note 回调函数在中断上下文中执行，应保持简短
 *
 * @code
 * void exti0_handler(exti_line_t line) {
 *     printf("EXTI0 triggered\n");
 * }
 *
 * exti_set_callback(EXTI_LINE_0, exti0_handler);
 * @endcode
 */
void exti_set_callback(exti_line_t line, exti_callback_t callback)
{
    assert(IS_EXTI_LINE(line));
    s_exti_callbacks[line] = callback;
}

/**
 * @brief 获取EXTI回调函数
 *
 * 获取指定EXTI线的回调函数指针。
 *
 * @param[in] line 线号 (0-24)
 * @return exti_callback_t 回调函数指针，如果未设置则返回NULL
 */
exti_callback_t exti_get_callback(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));
    return s_exti_callbacks[line];
}

/**
 * @brief 移除EXTI回调函数
 *
 * 清除指定EXTI线的回调函数。
 *
 * @param[in] line 线号 (0-24)
 *
 * @code
 * // 移除EXTI0的回调函数
 * exti_remove_callback(EXTI_LINE_0);
 * @endcode
 */
void exti_remove_callback(exti_line_t line)
{
    assert(IS_EXTI_LINE(line));
    s_exti_callbacks[line] = NULL;
}

//===========================================
// 第3层：高级配置 API
//===========================================

/**
 * @brief 配置EXTI线（完整配置接口）
 *
 * 使用配置结构体一次性完成EXTI线的所有配置。
 *
 * @param[in] config EXTI配置结构体指针
 *                   - line: 线号 (0-24)
 *                   - gpio_port: GPIO端口（仅GPIO线0-15有效）
 *                   - trigger: 触发方式
 *                   - mode: 工作模式
 *                   - callback: 回调函数
 *                   - enabled: 是否立即使能
 * @return true 配置成功
 * @return false 配置失败（参数无效）
 *
 * @note 覆盖寄存器：EXTI_EXTICRx, EXTI_RTENR, EXTI_FTENR, EXTI_IENR, EXTI_EENR
 * @note 自动使能EXTI模块时钟
 * @note 需要在NVIC中手动使能对应的中断向量
 *
 * @code
 * void my_callback(exti_line_t line) {
 *     // 处理中断...
 * }
 *
 * exti_config_t cfg = {
 *     .line = EXTI_LINE_0,
 *     .gpio_port = EXTI_GPIO_PORTA,
 *     .trigger = EXTI_TRIGGER_FALLING,
 *     .mode = EXTI_MODE_INTERRUPT,
 *     .callback = my_callback,
 *     .enabled = true
 * };
 * exti_configure(&cfg);
 *
 * // 还需在NVIC中使能中断
 * NVIC_EnableIRQ(EXTI0_IRQn);
 * @endcode
 */
bool exti_configure(const exti_config_t *config)
{
    if (config == NULL) {
        return false;
    }
    if (!IS_EXTI_LINE(config->line)) {
        return false;
    }
    if (!IS_EXTI_TRIGGER(config->trigger)) {
        return false;
    }
    if (!IS_EXTI_MODE(config->mode)) {
        return false;
    }

    uint32_t line = config->line;

    // 1. 使能EXTI模块时钟
    clock_periph_enable(CLK_EXTI);

    // 2. 配置GPIO源 (仅GPIO线0-15)
    if (IS_EXTI_GPIO_LINE(line)) {
        exti_set_gpio_source((exti_line_t)line, config->gpio_port);
    }

    // 3. 配置触发方式
    exti_set_trigger((exti_line_t)line, config->trigger);

    // 4. 配置工作模式（内部已包含使能/禁用逻辑）
    exti_set_mode((exti_line_t)line, config->mode);

    // 5. 根据配置决定最终使能状态
    if (config->enabled) {
        if (config->mode == EXTI_MODE_INTERRUPT) {
            exti_hw_set_interrupt_enable(line, true);
        } else {
            exti_hw_set_event_enable(line, true);
        }
    } else {
        exti_hw_set_interrupt_enable(line, false);
        exti_hw_set_event_enable(line, false);
    }

    // 6. 设置回调函数
    if (config->callback != NULL) {
        exti_set_callback(config->line, config->callback);
    }

    return true;
}

//===========================================
// 第4层：便捷初始化 API
//===========================================

/**
 * @brief GPIO外部中断快速初始化
 *
 * 快速配置GPIO外部中断，使用默认的中断模式。
 *
 * @param[in] gpio_port GPIO端口
 * @param[in] pin       引脚号 (0-15)
 * @param[in] trigger   触发方式
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy, EXTI_RTENR.RTENx, EXTI_FTENR.FTENx, EXTI_IENR.INTENx
 * @note 自动使能EXTI模块时钟
 * @note 自动配置为中断模式并使能
 * @note 需要手动在NVIC中使能对应的中断向量
 *
 * @code
 * // 配置GPIOA的引脚0为下降沿触发的外部中断
 * exti_gpio_init(EXTI_GPIO_PORTA, 0, EXTI_TRIGGER_FALLING);
 *
 * // 使能NVIC中断
 * NVIC_EnableIRQ(EXTI0_IRQn);
 *
 * // 设置回调函数
 * exti_set_callback(EXTI_LINE_0, my_callback);
 * @endcode
 */
void exti_gpio_init(exti_gpio_port_t gpio_port, uint32_t pin, exti_trigger_t trigger)
{
    assert(IS_EXTI_PIN(pin));
    assert(IS_EXTI_TRIGGER(trigger));

    exti_line_t line = (exti_line_t)pin;

    // 1. 使能EXTI模块时钟
    clock_periph_enable(CLK_EXTI);

    // 2. 配置GPIO源
    exti_set_gpio_source(line, gpio_port);

    // 3. 配置触发方式
    exti_set_trigger(line, trigger);

    // 4. 使能中断模式（set_mode内部已处理 IENR/EENR）
    exti_set_mode(line, EXTI_MODE_INTERRUPT);
}

/**
 * @brief 内部模块EXTI快速初始化
 *
 * 快速配置内部模块的EXTI线（线16-24）。
 *
 * @param[in] line    内部模块线号 (16-24)
 * @param[in] trigger 触发方式
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx, EXTI_FTENR.FTENx, EXTI_IENR.INTENx
 * @note 自动使能EXTI模块时钟
 * @note 自动配置为中断模式并使能
 * @note 内部线16-24各有独立NVIC向量，请查看设备头文件 IRQn_Type 获取对应中断号
 *
 * @code
 * // 配置RTC唤醒事件为上升沿触发
 * exti_internal_init(EXTI_LINE_RTC, EXTI_TRIGGER_RISING);
 *
 * // 使能NVIC中断（RTC使用独立向量 RTC_XTLSD_IRQn）
 * NVIC_EnableIRQ(RTC_XTLSD_IRQn);
 *
 * // 设置回调函数
 * exti_set_callback(EXTI_LINE_RTC, rtc_callback);
 * @endcode
 */
void exti_internal_init(exti_line_t line, exti_trigger_t trigger)
{
    assert(IS_EXTI_LINE(line));
    assert(!IS_EXTI_GPIO_LINE(line));  // 必须是内部模块线
    assert(IS_EXTI_TRIGGER(trigger));

    // 1. 使能EXTI模块时钟
    clock_periph_enable(CLK_EXTI);

    // 2. 配置触发方式
    exti_set_trigger(line, trigger);

    // 3. 使能中断模式（set_mode内部已处理 IENR/EENR）
    exti_set_mode(line, EXTI_MODE_INTERRUPT);
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief EXTI通用中断处理函数
 *
 * 处理指定范围内所有EXTI线的中断，自动调用回调函数并清除标志。
 *
 * @param[in] start_line 起始线号
 * @param[in] end_line   结束线号
 *
 * @note 此函数由各个IRQHandler调用，不应被用户直接调用
 */
static void exti_irq_handler_common(uint32_t start_line, uint32_t end_line)
{
    for (uint32_t line = start_line; line <= end_line; line++) {
        // 检查挂起标志
        if (exti_hw_get_pending_flag(line)) {
            // 调用回调函数
            if (s_exti_callbacks[line] != NULL) {
                s_exti_callbacks[line]((exti_line_t)line);
            }

            // 清除挂起标志
            exti_hw_clear_pending_flag(line);
        }
    }
}

/**
 * @brief EXTI0 中断服务程序
 *
 * 处理EXTI线0的中断。
 *
 * @note 对应中断号：EXTI0_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI0_IRQn)
 */
void EXTI0_IRQHandler(void)
{
    exti_irq_handler_common(0, 0);
}

/**
 * @brief EXTI1 中断服务程序
 *
 * 处理EXTI线1的中断。
 *
 * @note 对应中断号：EXTI1_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI1_IRQn)
 */
void EXTI1_IRQHandler(void)
{
    exti_irq_handler_common(1, 1);
}

/**
 * @brief EXTI2 中断服务程序
 *
 * 处理EXTI线2的中断。
 *
 * @note 对应中断号：EXTI2_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI2_IRQn)
 */
void EXTI2_IRQHandler(void)
{
    exti_irq_handler_common(2, 2);
}

/**
 * @brief EXTI3 中断服务程序
 *
 * 处理EXTI线3的中断。
 *
 * @note 对应中断号：EXTI3_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI3_IRQn)
 */
void EXTI3_IRQHandler(void)
{
    exti_irq_handler_common(3, 3);
}

/**
 * @brief EXTI4 中断服务程序
 *
 * 处理EXTI线4的中断。
 *
 * @note 对应中断号：EXTI4_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI4_IRQn)
 */
void EXTI4_IRQHandler(void)
{
    exti_irq_handler_common(4, 4);
}

/**
 * @brief EXTI5-9 共享中断服务程序
 *
 * 处理EXTI线5-9的中断（共享中断向量）。
 *
 * @note 对应中断号：EXTI9_5_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI9_5_IRQn)
 * @note 5条EXTI线共享同一个中断向量
 */
void EXTI9_5_IRQHandler(void)
{
    exti_irq_handler_common(5, 9);
}

/**
 * @brief EXTI10-15 共享中断服务程序
 *
 * 处理EXTI线10-15的中断（共享中断向量）。
 * 内部模块线16-24各有独立NVIC向量（LVD_IRQn, RTC_XTLSD_IRQn, IWDT_WKUP_IRQn 等），不在此处理。
 *
 * @note 对应中断号：EXTI15_10_IRQn
 * @note 需要在NVIC中使能：NVIC_EnableIRQ(EXTI15_10_IRQn)
 * @note 6条GPIO EXTI线共享同一个中断向量
 */
void EXTI15_10_IRQHandler(void)
{
    exti_irq_handler_common(10, 15);
}
