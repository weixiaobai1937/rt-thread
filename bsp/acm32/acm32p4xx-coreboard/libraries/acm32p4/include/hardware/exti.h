/**
 * @file exti.h
 * @brief EXTI (外部中断/事件控制器) 驱动接口
 *
 * 本文件提供了ACM32P4芯片EXTI模块的完整驱动接口，采用简洁API设计。
 *
 * 主要特性：
 * - 支持25个EXTI线 (GPIO 0-15 + 内部模块 16-24)
 * - 三种触发方式：上升沿、下降沿、双沿
 * - 中断/事件双模式
 * - 软件触发支持
 * - 回调函数机制
 *
 * API分层：
 * - Level 3 (便捷API): 一行代码完成配置
 * - Level 2 (高级API): 精细控制各项配置
 * - Level 1 (底层API): 直接寄存器操作
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */
#ifndef _HARDWARE_EXTI_H
#define _HARDWARE_EXTI_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 类型定义
//===========================================

/**
 * @brief EXTI 线号定义
 *
 * 0-15:  GPIO 引脚线 (可配置不同GPIO端口)
 * 16-24: 内部模块线 (固定源)
 */
typedef enum {
    // GPIO 线 (0-15)
    EXTI_LINE_0  = 0,   ///< GPIO Pin 0 (PA0/PB0/PC0/...)
    EXTI_LINE_1  = 1,   ///< GPIO Pin 1 (PA1/PB1/PC1/...)
    EXTI_LINE_2  = 2,   ///< GPIO Pin 2
    EXTI_LINE_3  = 3,   ///< GPIO Pin 3
    EXTI_LINE_4  = 4,   ///< GPIO Pin 4
    EXTI_LINE_5  = 5,   ///< GPIO Pin 5
    EXTI_LINE_6  = 6,   ///< GPIO Pin 6
    EXTI_LINE_7  = 7,   ///< GPIO Pin 7
    EXTI_LINE_8  = 8,   ///< GPIO Pin 8
    EXTI_LINE_9  = 9,   ///< GPIO Pin 9
    EXTI_LINE_10 = 10,  ///< GPIO Pin 10
    EXTI_LINE_11 = 11,  ///< GPIO Pin 11
    EXTI_LINE_12 = 12,  ///< GPIO Pin 12
    EXTI_LINE_13 = 13,  ///< GPIO Pin 13
    EXTI_LINE_14 = 14,  ///< GPIO Pin 14
    EXTI_LINE_15 = 15,  ///< GPIO Pin 15

    // 内部模块线 (16-24)
    EXTI_LINE_LVD           = 16,  ///< 低电压检测
    EXTI_LINE_RTC           = 17,  ///< RTC 中断/XTL停振
    EXTI_LINE_IWDT          = 18,  ///< 独立看门狗
    EXTI_LINE_COMP          = 19,  ///< 比较器
    EXTI_LINE_USB_WAKEUP    = 20,  ///< USB 唤醒
    EXTI_LINE_LPTIM1_WAKEUP = 21,  ///< LPTIM1 唤醒
    EXTI_LINE_LPUART1_WAKEUP = 22, ///< LPUART1 唤醒
    EXTI_LINE_LPUART2_WAKEUP = 23, ///< LPUART2 唤醒
    EXTI_LINE_ETH_WAKEUP    = 24,  ///< 以太网唤醒
} exti_line_t;

/**
 * @brief EXTI 触发方式
 *
 * 注意: 内部模块线(16-24)默认只支持上升沿触发
 */
typedef enum {
    EXTI_TRIGGER_RISING  = 0x01,  ///< 上升沿触发
    EXTI_TRIGGER_FALLING = 0x02,  ///< 下降沿触发
    EXTI_TRIGGER_BOTH    = 0x03,  ///< 双沿触发 (上升+下降)
} exti_trigger_t;

/**
 * @brief EXTI 工作模式
 */
typedef enum {
    EXTI_MODE_INTERRUPT = 0,  ///< 中断模式 (会产生NVIC中断)
    EXTI_MODE_EVENT     = 1,  ///< 事件模式 (唤醒CPU, 不产生中断)
} exti_mode_t;

/**
 * @brief GPIO 端口选择 (用于EXTI线0-15的GPIO源配置)
 *
 * EXTICR寄存器每线4位，有效值 0x0 ~ 0x7 (PA~PH)
 */
typedef enum {
    EXTI_GPIO_PORTA = 0x00,  ///< PA[x]
    EXTI_GPIO_PORTB = 0x01,  ///< PB[x]
    EXTI_GPIO_PORTC = 0x02,  ///< PC[x]
    EXTI_GPIO_PORTD = 0x03,  ///< PD[x]
    EXTI_GPIO_PORTE = 0x04,  ///< PE[x]
    EXTI_GPIO_PORTF = 0x05,  ///< PF[x]
    EXTI_GPIO_PORTG = 0x06,  ///< PG[x]
    EXTI_GPIO_PORTH = 0x07,  ///< PH[x]
} exti_gpio_port_t;

/**
 * @brief EXTI 中断回调函数类型
 *
 * @param line 触发中断的EXTI线号
 */
typedef void (*exti_callback_t)(exti_line_t line);

/**
 * @brief EXTI 完整配置结构体 (用于高级配置)
 */
typedef struct {
    exti_line_t        line;      ///< EXTI 线号 (0-24)
    exti_trigger_t     trigger;   ///< 触发方式
    exti_mode_t        mode;      ///< 工作模式 (中断/事件)
    exti_gpio_port_t   gpio_port; ///< GPIO端口 (仅线0-15有效)
    exti_callback_t    callback;  ///< 中断回调函数 (可选)
    bool               enabled;   ///< 是否立即使能
} exti_config_t;

//===========================================
// Level 3: 便捷 API (推荐日常使用)
//===========================================

/**
 * @brief 初始化GPIO引脚的EXTI中断 (便捷函数)
 *
 * 功能: 快速配置GPIO外部中断
 * - 配置EXTI线和GPIO端口映射
 * - 配置触发方式
 * - 使能中断模式
 * - 使能EXTI线
 *
 * @param[in] gpio_port GPIO端口 (EXTI_GPIO_PORTA ~ EXTI_GPIO_PORTH)
 * @param[in] pin       引脚号 (0-15)
 * @param[in] trigger   触发方式 (EXTI_TRIGGER_RISING/FALLING/BOTH)
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy, EXTI_RTENR.RTENx, EXTI_FTENR.FTENx, EXTI_IENR.INTENx
 * @note 需要用户自行配置NVIC中断优先级和使能NVIC
 *
 * @code
 * exti_gpio_init(EXTI_GPIO_PORTA, 5, EXTI_TRIGGER_RISING);
 * NVIC_SetPriority(EXTI9_5_IRQn, 2);
 * NVIC_EnableIRQ(EXTI9_5_IRQn);
 * @endcode
 */
void exti_gpio_init(exti_gpio_port_t gpio_port, uint32_t pin, exti_trigger_t trigger);

/**
 * @brief 初始化内部模块的EXTI中断 (便捷函数)
 *
 * 功能: 快速配置内部模块中断
 * - 配置触发方式
 * - 使能中断模式
 * - 使能EXTI线
 *
 * @param[in] line    内部模块线号 (16-24)
 * @param[in] trigger 触发方式 (通常为EXTI_TRIGGER_RISING)
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx, EXTI_FTENR.FTENx, EXTI_IENR.INTENx
 * @note 内部线16-24各有独立NVIC向量，请查看设备头文件 IRQn_Type 获取对应中断号
 *
 * @code
 * exti_internal_init(EXTI_LINE_RTC, EXTI_TRIGGER_RISING);
 * NVIC_EnableIRQ(RTC_XTLSD_IRQn);
 * @endcode
 */
void exti_internal_init(exti_line_t line, exti_trigger_t trigger);

//===========================================
// Level 2: 高级 API (精细控制)
//===========================================

/**
 * @brief 使用配置结构体初始化EXTI (高级配置)
 *
 * 功能: 提供完整的配置选项
 * - 支持中断/事件模式切换
 * - 支持回调函数设置
 * - 支持延迟使能
 *
 * @param[in] config 配置结构体指针
 * @return true 配置成功, false 配置失败
 *
 * @note 覆盖寄存器：EXTI_EXTICRx, EXTI_RTENR, EXTI_FTENR, EXTI_IENR, EXTI_EENR
 * @note 适用于需要精细控制的场景
 */
bool exti_configure(const exti_config_t *config);

/**
 * @brief 配置EXTI线的GPIO源端口 (线0-15专用)
 *
 * 功能: 选择EXTI线连接到哪个GPIO端口
 *
 * @param[in] line      EXTI线号 (0-15)
 * @param[in] gpio_port GPIO端口选择
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy
 *
 * @code
 * exti_set_gpio_source(EXTI_LINE_5, EXTI_GPIO_PORTA);
 * @endcode
 */
void exti_set_gpio_source(exti_line_t line, exti_gpio_port_t gpio_port);

/**
 * @brief 设置EXTI线的触发方式
 *
 * @param[in] line    EXTI线号 (0-24)
 * @param[in] trigger 触发方式
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx, EXTI_FTENR.FTENx
 * @note 内部模块线(16-24)通常只支持上升沿触发
 */
void exti_set_trigger(exti_line_t line, exti_trigger_t trigger);

/**
 * @brief 设置EXTI线的工作模式 (中断/事件)
 *
 * @param[in] line EXTI线号 (0-24)
 * @param[in] mode 工作模式
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx, EXTI_EENR.EVENx
 * @note 中断模式: 产生NVIC中断；事件模式: 仅唤醒CPU, 不产生中断
 */
void exti_set_mode(exti_line_t line, exti_mode_t mode);

/**
 * @brief 使能EXTI线
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 */
void exti_enable(exti_line_t line);

/**
 * @brief 禁用EXTI线
 *
 * 禁用中断和事件功能，并清除挂起标志。
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx, EXTI_EENR.EVENx, EXTI_PDR.PDx
 * @note 不会禁用NVIC中的中断向量（因为可能影响共享线）
 */
void exti_disable(exti_line_t line);

/**
 * @brief 检查EXTI线是否使能
 *
 * @param[in] line EXTI线号 (0-24)
 * @return true 已使能, false 未使能
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 */
bool exti_is_enabled(exti_line_t line);

/**
 * @brief 软件触发EXTI中断
 *
 * 通过软件方式设置挂起标志，模拟硬件事件触发。
 * 前提条件：EXIT线必须已使能中断（IENR相应位置位）且NVIC中断已使能。
 * 软件触发后，EXTI_PDR对应位被置位，若中断已使能且NVIC已开启，
 * 则产生中断进入ISR。
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_SWIER.SWIEx
 * @note 用于软件模拟外部中断，可用于测试
 */
void exti_trigger_software(exti_line_t line);

/**
 * @brief 检查EXTI挂起标志
 *
 * @param[in] line EXTI线号 (0-24)
 * @return true 有挂起中断, false 无挂起中断
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx
 */
bool exti_get_pending(exti_line_t line);

/**
 * @brief 清除EXTI挂起标志
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx (写1清除)
 * @note 必须在中断处理函数中调用以清除中断标志
 */
void exti_clear_pending(exti_line_t line);

/**
 * @brief 设置EXTI线的中断回调函数
 *
 * @param[in] line     EXTI线号 (0-24)
 * @param[in] callback 回调函数指针
 *
 * @note 覆盖寄存器：无 (纯软件管理)
 * @note 回调函数在EXTI中断服务程序中被调用，应保持简短
 */
void exti_set_callback(exti_line_t line, exti_callback_t callback);

/**
 * @brief 获取EXTI线的回调函数
 *
 * @param[in] line EXTI线号 (0-24)
 * @return 回调函数指针, NULL表示未设置
 *
 * @note 覆盖寄存器：无 (纯软件管理)
 */
exti_callback_t exti_get_callback(exti_line_t line);

/**
 * @brief 移除EXTI线的回调函数
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：无 (纯软件管理)
 */
void exti_remove_callback(exti_line_t line);

//===========================================
// Level 1: 底层 API (寄存器操作, 仅供高级用户)
//===========================================

/**
 * @brief 直接设置中断使能寄存器
 *
 * @param[in] line   EXTI线号 (0-24)
 * @param[in] enable true=使能, false=禁用
 *
 * @note 覆盖寄存器：EXTI_IENR.INTENx
 */
void exti_hw_set_interrupt_enable(uint32_t line, bool enable);

/**
 * @brief 直接设置事件使能寄存器
 *
 * @param[in] line   EXTI线号 (0-24)
 * @param[in] enable true=使能, false=禁用
 *
 * @note 覆盖寄存器：EXTI_EENR.EVENx
 */
void exti_hw_set_event_enable(uint32_t line, bool enable);

/**
 * @brief 直接设置上升沿触发使能
 *
 * @param[in] line   EXTI线号 (0-24)
 * @param[in] enable true=使能, false=禁用
 *
 * @note 覆盖寄存器：EXTI_RTENR.RTENx
 */
void exti_hw_set_rising_edge(uint32_t line, bool enable);

/**
 * @brief 直接设置下降沿触发使能
 *
 * @param[in] line   EXTI线号 (0-24)
 * @param[in] enable true=使能, false=禁用
 *
 * @note 覆盖寄存器：EXTI_FTENR.FTENx
 */
void exti_hw_set_falling_edge(uint32_t line, bool enable);

/**
 * @brief 读取挂起标志
 *
 * @param[in] line EXTI线号 (0-24)
 * @return true 有挂起, false 无挂起
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx
 */
bool exti_hw_get_pending_flag(uint32_t line);

/**
 * @brief 清除挂起标志
 *
 * @param[in] line EXTI线号 (0-24)
 *
 * @note 覆盖寄存器：EXTI_PDR.PDx (写1清除)
 */
void exti_hw_clear_pending_flag(uint32_t line);

/**
 * @brief 配置EXTI线的GPIO源 (底层寄存器操作)
 *
 * 每线占用4位，线0-7写入EXTICR1，线8-15写入EXTICR2。
 *
 * @param[in] line       EXTI线号 (0-15)
 * @param[in] port_value GPIO端口值 (0x0~0x7 对应 PA~PH)
 *
 * @note 覆盖寄存器：EXTI_EXTICRx.EXTIy
 */
void exti_hw_set_exticr(uint32_t line, uint32_t port_value);

#ifdef __cplusplus
}
#endif

#endif  // _HARDWARE_EXTI_H
