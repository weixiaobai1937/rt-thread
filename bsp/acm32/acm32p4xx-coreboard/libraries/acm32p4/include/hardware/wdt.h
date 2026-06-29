/**
 * @file wdt.h
 * @brief ACM32P4 系统看门狗（WDT）驱动
 *
 * @details
 * 功能特性：
 * - 32位递减计数器，可编程加载值
 * - 8级可编程预分频（1/2/4/8/16/32/64/128）
 * - 复位模式：计数器减到0时产生系统复位
 * - 中断模式：计数器减到0时先产生中断，超时后复位
 * - 可编程中断清除时限
 * - 原始中断状态查询
 * - 喂狗特征值写入保护
 *
 * @note WDT寄存器基地址：0x40002C00
 * @note 复位信号需要 RCC_RCR.WDTRSTEN 置位才能生效
 * @note 中断号：WDT_IRQn (0)
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_WDT_H
#define _HARDWARE_WDT_H

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
 * @brief WDT_CTRL 寄存器位定义
 */
#define WDT_CTRL_DIVISOR_Pos     (0U)                       ///< DIVISOR 位偏移
#define WDT_CTRL_DIVISOR_Msk     (0x7UL << WDT_CTRL_DIVISOR_Pos)  ///< DIVISOR 掩码
#define WDT_CTRL_DIVISOR         WDT_CTRL_DIVISOR_Msk        ///< DIVISOR 掩码

#define WDT_CTRL_INTEN_Pos       (4U)                       ///< INTEN 位偏移
#define WDT_CTRL_INTEN_Msk       (0x1UL << WDT_CTRL_INTEN_Pos)    ///< INTEN 掩码
#define WDT_CTRL_INTEN           WDT_CTRL_INTEN_Msk          ///< INTEN 掩码

#define WDT_CTRL_MODE_Pos        (6U)                       ///< MODE 位偏移
#define WDT_CTRL_MODE_Msk        (0x1UL << WDT_CTRL_MODE_Pos)     ///< MODE 掩码
#define WDT_CTRL_MODE            WDT_CTRL_MODE_Msk           ///< MODE 掩码

#define WDT_CTRL_EN_Pos          (7U)                       ///< EN 位偏移
#define WDT_CTRL_EN_Msk          (0x1UL << WDT_CTRL_EN_Pos)       ///< EN 掩码
#define WDT_CTRL_EN              WDT_CTRL_EN_Msk             ///< EN 掩码

/**
 * @brief WDT_RIS 寄存器位定义
 * @note 设备头字段名为 RIS_f.RIS，此处沿用手册名 WDTRIS
 */
#define WDT_RIS_WDTRIS_Pos       (0U)                       ///< WDTRIS 位偏移
#define WDT_RIS_WDTRIS_Msk       (0x1UL << WDT_RIS_WDTRIS_Pos)    ///< WDTRIS 掩码
#define WDT_RIS_WDTRIS           WDT_RIS_WDTRIS_Msk          ///< WDTRIS 掩码

/**
 * @brief WDT_FEED 喂狗特征值
 */
#define WDT_FEED_MAGIC           0xAA55A55AU                 ///< 喂狗特征值

/**
 * @brief WDT 默认复位值
 */
#define WDT_LOAD_DEFAULT         0xFFFFFFFFU                 ///< 加载寄存器上电默认值（手册 19.5.1）
#define WDT_INTCLRTIME_DEFAULT   0x00001000U                 ///< 中断清除时限默认值

/**
 * @brief 超时时间计算的极限值
 */
#define WDT_LOAD_MAX             0xFFFFFFFFU                 ///< 最大加载值
#define WDT_INTCLRTIME_MAX       0x0000FFFFU                 ///< 最大中断清除时限

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief WDT 工作模式
 */
typedef enum {
    WDT_MODE_RESET     = 0,  ///< 复位模式：计数器减到0时产生系统复位
    WDT_MODE_INTERRUPT = 1,  ///< 中断模式：计数器减到0时先产生中断，超时后复位
} wdt_mode_t;

/**
 * @brief WDT 时钟预分频
 *
 * 预分频值直接对应 DIVISOR 位域的值（0-7），
 * 实际分频系数 = 2^DIVISOR
 */
typedef enum {
    WDT_DIVISOR_1   = 0,  ///< 不分频（1分频）
    WDT_DIVISOR_2   = 1,  ///< 2分频
    WDT_DIVISOR_4   = 2,  ///< 4分频
    WDT_DIVISOR_8   = 3,  ///< 8分频
    WDT_DIVISOR_16  = 4,  ///< 16分频
    WDT_DIVISOR_32  = 5,  ///< 32分频
    WDT_DIVISOR_64  = 6,  ///< 64分频（复位默认值）
    WDT_DIVISOR_128 = 7,  ///< 128分频
} wdt_divisor_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief 回调函数类型
 */
typedef void (*wdt_callback_t)(void);

/**
 * @brief WDT 配置结构体
 *
 * 包含 WDT 外设的所有可配置参数。
 * 复位模式下仅需设置 load、divisor、mode；
 * 中断模式下还需设置 int_enable 和 intclrtime。
 */
typedef struct {
    uint32_t      load;         ///< 加载值，递减计数器的起始值
    wdt_divisor_t divisor;      ///< 时钟预分频，决定看门狗时钟频率
    wdt_mode_t    mode;         ///< 看门狗工作模式（复位/中断）
    bool          int_enable;   ///< 中断使能，仅中断模式下需要置 true
    uint16_t      intclrtime;   ///< 中断清除时限，仅中断模式下有效
} wdt_config_t;

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 WDT 为复位模式并启动
 *
 * 一行代码完成复位模式看门狗的配置和启动。
 *
 * @param[in] load    加载值，决定超时时间
 * @param[in] divisor 时钟预分频，决定计数时钟
 *
 * @note 超时时间 T_WDT = T_PCLK × 2^divisor × (load + 1)
 *
 * @code
 * // 初始化复位模式看门狗，64分频，加载值0xFFFFFF
 * wdt_init_reset(0x00FFFFFF, WDT_DIVISOR_64);
 * @endcode
 */
void wdt_init_reset(uint32_t load, wdt_divisor_t divisor);

/**
 * @brief 快速初始化 WDT 为中断模式并启动
 *
 * 一行代码完成中断模式看门狗的配置和启动。
 * 计数器减到0时产生中断，超过 intclrtime 未喂狗则复位。
 *
 * @param[in] load       加载值，决定中断触发时间
 * @param[in] divisor    时钟预分频
 * @param[in] intclrtime 中断清除时限，超时未喂狗则复位
 *
 * @note 中断触发时间 T_WDT = T_PCLK × 2^divisor × (load + 1)
 * @note 复位时间 = T_WDT + T_PCLK × 2^divisor × (intclrtime + 1)
 * @note 需在中断服务程序中调用 wdt_feed() 清除中断
 *
 * @code
 * // 初始化中断模式看门狗，64分频，中断清除时限0x1000
 * wdt_init_interrupt(0x00FFFFFF, WDT_DIVISOR_64, 0x1000);
 * @endcode
 */
void wdt_init_interrupt(uint32_t load, wdt_divisor_t divisor, uint16_t intclrtime);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 WDT 并启动
 *
 * 灵活的完整配置接口，适用于需要精确控制所有参数的场景。
 *
 * @param[in] config WDT 配置结构体指针
 * @return true  配置成功
 * @return false 参数无效（config 为 NULL 或参数超出范围）
 *
 * @note 此函数会先禁用 WDT，配置完成后重新使能
 * @note 配置成功后即刻开始递减计数
 *
 * @code
 * wdt_config_t cfg = {
 *     .load = 0x00FFFFFF,
 *     .divisor = WDT_DIVISOR_64,
 *     .mode = WDT_MODE_RESET,
 *     .int_enable = false,
 *     .intclrtime = 0,
 * };
 * wdt_config(&cfg);
 * @endcode
 */
bool wdt_config(const wdt_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 注册 WDT 中断回调函数
 *
 * 注册的回调函数将在 WDT_IRQHandler 中被调用。
 * 传递 NULL 可取消回调。
 *
 * @param[in] callback 回调函数指针，NULL 表示取消回调
 *
 * @note 回调函数在中断上下文中执行，应尽量简短
 * @note 回调函数中必须调用 wdt_feed() 以清除中断
 *
 * @code
 * void my_wdt_handler(void)
 * {
 *     wdt_feed();  // 在回调中喂狗清中断
 * }
 * wdt_register_callback(my_wdt_handler);
 * @endcode
 */
void wdt_register_callback(wdt_callback_t callback);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能看门狗定时器
 *
 * 使能后计数器将从加载值开始递减计数。
 *
 * @note 系统复位后 WDT 默认处于禁用状态
 * @note 覆盖寄存器：CTRL.EN
 */
void wdt_enable(void);

/**
 * @brief 禁用看门狗定时器
 *
 * 停止计数器递减。禁用后看门狗不会产生复位或中断。
 *
 * @note 一旦使能，某些应用中可能不允许软件禁用（取决于系统设计）
 * @note 覆盖寄存器：CTRL.EN
 */
void wdt_disable(void);

/**
 * @brief 查询 WDT 使能状态
 *
 * @return true  WDT 已使能
 * @return false WDT 已禁用
 *
 * @note 覆盖寄存器：CTRL.EN
 */
bool wdt_is_enabled(void);

/**
 * @brief 喂狗（刷新看门狗）
 *
 * 向 WDT_FEED 寄存器写入特征值 0xAA55A55A，
 * 使计数器从加载寄存器重新加载，并清除中断。
 *
 * @note 必须在超时前调用，否则将产生复位
 * @note 中断模式下，喂狗可清除中断标志
 * @note 覆盖寄存器：FEED[31:0]
 */
void wdt_feed(void);

/**
 * @brief 获取当前计数值
 *
 * 读取递减计数器的当前值。
 *
 * @return 当前计数值（32位）
 *
 * @note 覆盖寄存器：COUNT[31:0]
 */
uint32_t wdt_get_counter(void);

/**
 * @brief 获取当前加载值
 *
 * 读取加载寄存器的当前配置值。
 *
 * @return 加载值（32位）
 *
 * @note 覆盖寄存器：LOAD[31:0]
 */
uint32_t wdt_get_load(void);

/**
 * @brief 获取原始中断状态
 *
 * 读取看门狗原始中断状态标志位。
 *
 * @return true  已产生原始中断
 * @return false 未产生原始中断
 *
 * @note 即使中断未使能，原始中断标志仍会产生
 * @note 覆盖寄存器：RIS.WDTRIS
 */
bool wdt_get_ris(void);

/**
 * @brief 设置加载值
 *
 * 修改看门狗递减计数器的重载值。
 * 建议在 WDT 禁用时修改。
 *
 * @param[in] load 加载值
 *
 * @note 覆盖寄存器：LOAD[31:0]
 */
void wdt_set_load(uint32_t load);

/**
 * @brief 设置时钟预分频
 *
 * 修改看门狗计数时钟的分频系数。
 * 建议在 WDT 禁用时修改。
 *
 * @param[in] divisor 预分频值，决定分频系数 = 2^divisor
 *
 * @note 覆盖寄存器：CTRL.DIVISOR
 */
void wdt_set_divisor(wdt_divisor_t divisor);

/**
 * @brief 设置工作模式
 *
 * 切换看门狗工作模式（复位模式或中断模式）。
 * 建议在 WDT 禁用时修改。
 *
 * @param[in] mode 工作模式
 *
 * @note 覆盖寄存器：CTRL.MODE
 */
void wdt_set_mode(wdt_mode_t mode);

/**
 * @brief 设置中断清除时限
 *
 * 设置中断产生后允许的清除时间窗口。
 * 仅中断模式下有效。
 *
 * @param[in] intclrtime 中断清除时限值 (0-65535)
 *                        实际时限 = T_PCLK × 2^divisor × (intclrtime + 1)
 *
 * @note 超过此时限未喂狗将产生系统复位
 * @note 覆盖寄存器：INTCLRTIME[15:0]
 */
void wdt_set_intclrtime(uint16_t intclrtime);

/**
 * @brief 使能或禁用 WDT 中断
 *
 * 控制中断模式下的中断产生。
 *
 * @param[in] enable true: 使能中断, false: 禁止中断
 *
 * @note 仅中断模式（WDT_CTRL.MODE = 1）下此设置有意义
 * @note 禁止中断后，原始中断标志仍然会产生
 * @note 覆盖寄存器：CTRL.INTEN
 */
void wdt_set_interrupt(bool enable);

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 计算 WDT 超时时间
 *
 * 根据当前配置计算看门狗的超时时间。
 *
 * @param[in] pclk_hz  APB 时钟频率 (Hz)
 * @param[in] load     加载值
 * @param[in] divisor  预分频值
 * @return 超时时间（微秒）
 *
 * @note 公式：T_WDT = (1 / pclk_hz) × 2^divisor × (load + 1)
 */
uint64_t wdt_calculate_timeout_us(uint32_t pclk_hz, uint32_t load, wdt_divisor_t divisor);

/**
 * @brief 根据目标超时时间计算加载值
 *
 * 反向计算达到目标超时时间所需的加载寄存器值。
 *
 * @param[in] pclk_hz     APB 时钟频率 (Hz)
 * @param[in] timeout_us  目标超时时间（微秒）
 * @param[in] divisor     预分频值
 * @return 计算出的加载值，如果超时时间太长导致溢出则返回 WDT_LOAD_MAX
 */
uint32_t wdt_calculate_load(uint32_t pclk_hz, uint64_t timeout_us, wdt_divisor_t divisor);

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief WDT 中断服务程序
 *
 * 应用程序应在中断向量表中映射此函数。
 * 此函数会调用已注册的用户回调函数。
 *
 * @note 需要在 NVIC 中使能 WDT_IRQn
 * @note 中断标志通过喂狗操作清除（写入 WDT_FEED 特征值）
 * @note 如果注册了回调，在回调中必须调用 wdt_feed()
 */
void WDT_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif // _HARDWARE_WDT_H
