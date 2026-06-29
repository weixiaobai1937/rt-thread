/**
 * @file iwdt.h
 * @brief ACM32P4 独立看门狗（IWDT）驱动
 *
 * @details
 * 功能特性：
 * - 12位自由运行向下计数器，由专用低速时钟 RC32K 驱动
 * - 计数器减到 0x000 时产生系统复位
 * - 8级可编程预分频（4/8/16/32/64/128/256）
 * - 窗口看门狗功能：计数器在窗口外重装载时产生复位
 * - 唤醒功能：计数器减到预设唤醒值时产生 WAKEUP 信号
 * - 可在 STOP 和 STANDBY 模式下工作
 * - 寄存器写保护机制
 * - 喂狗完成状态指示
 *
 * @note IWDT 寄存器基地址：0x40003000
 * @note 时钟源：RC32K（约32kHz），由 VDD 供电
 * @note 复位信号需要 RCC_RCR.IWDTRSTEN 置位才能生效
 * @note 唤醒中断号：IWDT_WKUP_IRQn (43)，通过 EXTI line 18
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_IWDT_H
#define _HARDWARE_IWDT_H

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
 * @brief IWDT 命令码
 */
#define IWDT_CMD_ENABLE         0xCCCCU      ///< 使能 IWDT
#define IWDT_CMD_WRITE_ENABLE   0x5555U      ///< 禁止写保护（打开寄存器写权限）
#define IWDT_CMD_WAKEUP_ENABLE  0x6666U      ///< 使能唤醒功能
#define IWDT_CMD_WAKEUP_DISABLE 0x9999U      ///< 禁止唤醒功能
#define IWDT_CMD_RELOAD         0xAAAAU      ///< 喂狗（重装载计数器）
#define IWDT_CMD_DISABLE        0xEF01ABCDU  ///< 禁止 IWDT

/**
 * @brief IWDT 数值极限
 */
#define IWDT_RELOAD_MAX         0x0FFFU      ///< 重装载值最大值（12位）
#define IWDT_PRESCALER_MAX      6U           ///< 预分频最大值
#define IWDT_RCL_FREQ_HZ        32000U       ///< RC32K 标称频率

/**
 * @brief IWDT 状态等待超时
 */
#define IWDT_UPDATE_TIMEOUT     1000U        ///< 寄存器更新等待超时（循环次数）

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief IWDT 时钟预分频
 *
 * 预分频值直接对应 IWDT_PR.PR 位域的值（0-7），
 * 实际分频系数见各枚举值说明。
 */
typedef enum {
    IWDT_PRESCALER_4   = 0,  ///< 4分频
    IWDT_PRESCALER_8   = 1,  ///< 8分频
    IWDT_PRESCALER_16  = 2,  ///< 16分频
    IWDT_PRESCALER_32  = 3,  ///< 32分频
    IWDT_PRESCALER_64  = 4,  ///< 64分频
    IWDT_PRESCALER_128 = 5,  ///< 128分频
    IWDT_PRESCALER_256 = 6,  ///< 256分频
} iwdt_prescaler_t;

/**
 * @brief IWDT 状态标志（位掩码，可组合查询）
 */
typedef enum {
    IWDT_FLAG_PVU = (1U << 0),  ///< 预分频值更新中
    IWDT_FLAG_RVU = (1U << 1),  ///< 重装载值更新中
    IWDT_FLAG_WVU = (1U << 2),  ///< 窗口值更新中
    IWDT_FLAG_WTU = (1U << 3),  ///< 唤醒值更新中
    IWDT_FLAG_RLF = (1U << 4),  ///< 喂狗进行中
} iwdt_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief 回调函数类型
 *
 * 唤醒中断回调函数，在 IWDT_WKUP_IRQHandler 中调用。
 */
typedef void (*iwdt_callback_t)(void);

/**
 * @brief IWDT 配置结构体
 *
 * 包含 IWDT 外设的所有可配置参数。
 * 窗口功能在 window < reload 时自动生效；
 * 唤醒功能在 wakeup < reload 且 wakeup_enable = true 时生效。
 */
typedef struct {
    iwdt_prescaler_t prescaler;      ///< 时钟预分频
    uint16_t         reload;         ///< 重装载值 (0-0xFFF)
    uint16_t         window;         ///< 窗口值 (0-0xFFF)，设为 0xFFF 时禁用窗口功能
    uint16_t         wakeup;         ///< 唤醒值 (0-0xFFF)，设为 0xFFF 时禁用唤醒功能
    bool             wakeup_enable;  ///< 是否使能唤醒功能
} iwdt_config_t;

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 IWDT 为复位模式并启动
 *
 * 一行代码完成复位模式独立看门狗的配置和启动。
 * 窗口功能和唤醒功能均禁用。
 *
 * @param[in] prescaler 时钟预分频
 * @param[in] reload    重装载值 (0-0xFFF)
 *
 * @note 覆盖寄存器：CMDR.CMD, PR.PR, RLR.PL, WUTR.WUT, WINR.WIN
 * @note 超时时间 t_IWDT = (reload + 1) × 预分频系数 / 32000（秒）
 * @note 需要先使能 RC32K 时钟（RCC_STDBYCTRL.RCLEN = 1）
 * @note 需要先使能 IWDT 复位信号（RCC_RCR.IWDTRSTEN = 1）
 *
 * @code
 * // 初始化 IWDT 复位模式，32分频，重装载值 4000
 * iwdt_init_reset(IWDT_PRESCALER_32, 4000);
 * @endcode
 */
void iwdt_init_reset(iwdt_prescaler_t prescaler, uint16_t reload);

/**
 * @brief 快速初始化 IWDT 为窗口模式并启动
 *
 * 一行代码完成窗口看门狗的配置和启动。
 * 当计数器值大于窗口值时喂狗会触发复位。
 *
 * @param[in] prescaler 时钟预分频
 * @param[in] reload    重装载值 (0-0xFFF)，必须大于 window
 * @param[in] window    窗口值 (0-0xFFF)，必须小于 reload
 *
 * @note 覆盖寄存器：CMDR.CMD, PR.PR, RLR.PL, WUTR.WUT, WINR.WIN
 * @note 窗口功能仅在 window < reload 时生效
 * @note 配置窗口值会自动触发计数器重装载
 *
 * @code
 * // 初始化 IWDT 窗口模式，窗口值 2000，重装载值 4000
 * iwdt_init_window(IWDT_PRESCALER_32, 4000, 2000);
 * @endcode
 */
void iwdt_init_window(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t window);

/**
 * @brief 快速初始化 IWDT 为唤醒模式并启动
 *
 * 一行代码完成唤醒模式独立看门狗的配置和启动。
 * 计数器减到唤醒值时产生 WAKEUP 信号，可将 MCU 从 STOP 模式唤醒。
 *
 * @param[in] prescaler 时钟预分频
 * @param[in] reload    重装载值 (0-0xFFF)，必须大于 wakeup
 * @param[in] wakeup    唤醒值 (0-0xFFF)，必须小于 reload
 *
 * @note 覆盖寄存器：CMDR.CMD, PR.PR, RLR.PL, WINR.WIN, WUTR.WUT
 * @note 唤醒功能仅在 wakeup < reload 时生效
 * @note 需要额外配置 EXTI 使能唤醒（EXTI_IERN bit18, EXTI_RTENR bit18）
 *
 * @code
 * // 初始化 IWDT 唤醒模式，唤醒值 1000，重装载值 4000
 * iwdt_init_wakeup(IWDT_PRESCALER_32, 4000, 1000);
 * @endcode
 */
void iwdt_init_wakeup(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t wakeup);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 IWDT 并启动
 *
 * 灵活的完整配置接口，适用于需要精确控制所有参数的场景。
 *
 * @param[in] config IWDT 配置结构体指针
 * @return true  配置成功
 * @return false 参数无效（config 为 NULL 或参数超出范围）
 *
 * @note 覆盖寄存器：CMDR.CMD, PR.PR, RLR.PL, WUTR.WUT, WINR.WIN
 * @note 此函数会先使能 IWDT，打开写保护，然后依次配置各寄存器
 * @note 配置完成后自动喂狗启动计数
 *
 * @code
 * iwdt_config_t cfg = {
 *     .prescaler = IWDT_PRESCALER_32,
 *     .reload = 4000,
 *     .window = 0xFFF,
 *     .wakeup = 0xFFF,
 *     .wakeup_enable = false,
 * };
 * iwdt_config(&cfg);
 * @endcode
 */
bool iwdt_config(const iwdt_config_t *config);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 注册 IWDT 唤醒中断回调函数
 *
 * 注册的回调函数将在 IWDT_WKUP_IRQHandler 中被调用。
 * 传递 NULL 可取消回调。
 *
 * @param[in] callback 回调函数指针，NULL 表示取消回调
 *
 * @note 回调函数在中断上下文中执行，应尽量简短
 * @note 回调函数中应调用 iwdt_feed() 喂狗并清除 EXTI pending
 *
 * @code
 * void my_iwdt_handler(void)
 * {
 *     iwdt_feed();
 * }
 * iwdt_register_callback(my_iwdt_handler);
 * @endcode
 */
void iwdt_register_callback(iwdt_callback_t callback);

/**
 * @brief 使能 IWDT 唤醒功能
 *
 * 开启后计数器减到唤醒值时产生 WAKEUP 信号。
 *
 * @note 覆盖寄存器：CMDR.CMD（写入 0x6666）
 * @note 需要 wakeup < reload 才有效
 * @note 需要配置 EXTI（EXTI_IERN bit18 = 1, EXTI_RTENR bit18 = 1）
 */
void iwdt_wakeup_enable(void);

/**
 * @brief 禁止 IWDT 唤醒功能
 *
 * 关闭唤醒功能后不再产生 WAKEUP 信号。
 */

void iwdt_wakeup_disable(void);

/**
 * @brief 关闭 IWDT 寄存器写保护（打开写权限）
 *
 * 修改 IWDT_PR、IWDT_RLR、IWDT_WINR、IWDT_WUTR 前需要先调用此函数。
 *
 * @note 覆盖寄存器：CMDR.CMD（写入 0x5555）
 * @note 写保护会在以下操作后自动恢复：喂狗（0xAAAA）、写入其他命令值
 */
void iwdt_enable_write_access(void);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能独立看门狗
 *
 * 使能后计数器从复位值 0xFFF 开始递减计数。
 *
 * @note 覆盖寄存器：CMDR.CMD（写入 0xCCCC）
 * @note 使能后 RC32K 时钟自动开启
 */
void iwdt_enable(void);

/**
 * @brief 禁止独立看门狗
 *
 * 停止计数器递减，看门狗不再产生复位。
 *
 * @note 覆盖寄存器：CMDR.CMD（写入 0xEF01ABCD）
 * @note 某些安全应用中可能不允许软件禁止
 */
void iwdt_disable(void);

/**
 * @brief 查询 IWDT 当前是否处于使能状态
 *
 * 读取内部软件状态，反映最后一次 iwdt_enable/iwdt_disable 的调用结果。
 *
 * @return true  IWDT 已使能
 * @return false IWDT 已禁止
 *
 * @note 此函数读取的是软件状态变量，不直接读取硬件寄存器
 */
bool iwdt_is_enabled(void);

/**
 * @brief 喂狗（重装载计数器）
 *
 * 向 IWDT_CMDR 写入 0xAAAA，将 IWDT_RLR 的值重新加载到计数器。
 * 窗口模式下，若计数器值大于窗口值则触发复位。
 *
 * @note 覆盖寄存器：CMDR.CMD（写入 0xAAAA），重装载计数器为 RLR.PL
 * @note 喂狗后写保护自动恢复
 * @note 进入 STOP 模式前需等待 SR.RLF = 0
 *
 * @code
 * iwdt_feed();  // 在主循环中定期喂狗
 * @endcode
 */
void iwdt_feed(void);

/**
 * @brief 设置预分频值
 *
 * 修改 IWDT 时钟预分频系数。
 *
 * @param[in] prescaler 预分频值
 *
 * @note 覆盖寄存器：CMDR.CMD, PR.PR
 * @note 写入前需确保已调用 iwdt_enable_write_access()
 * @note 修改后需等待 SR.PVU = 0
 */
void iwdt_set_prescaler(iwdt_prescaler_t prescaler);

/**
 * @brief 获取预分频值
 *
 * 读取当前 IWDT 时钟预分频配置。
 *
 * @return 当前预分频值
 * @note 覆盖寄存器：PR.PR
 *
 */
iwdt_prescaler_t iwdt_get_prescaler(void);

/**
 * @brief 设置重装载值
 *
 * 修改 IWDT 递减计数器的重装载值。
 *
 * @param[in] reload 重装载值 (0-0xFFF)
 *
 * @note 覆盖寄存器：CMDR.CMD, RLR.PL
 * @note 写入前需确保已调用 iwdt_enable_write_access()
 * @note 修改后需等待 SR.RVU = 0
 */
void iwdt_set_reload(uint16_t reload);

/**
 * @brief 获取重装载值
 *
 * 读取当前 IWDT 重装载寄存器配置。
 *
 * @return 当前重装载值
 * @note 覆盖寄存器：RLR.PL
 *
 */
uint16_t iwdt_get_reload(void);

/**
 * @brief 设置窗口值
 *
 * 修改 IWDT 窗口看门狗的窗口上限值。
 * 当计数器值大于窗口值时，喂狗操作会触发复位。
 *
 * @param[in] window 窗口值 (0-0xFFF)
 *
 * @note 覆盖寄存器：CMDR.CMD, WINR.WIN
 * @note 写入前需确保已调用 iwdt_enable_write_access()
 * @note 修改后需等待 SR.WVU = 0
 * @note 设置为 0xFFF 相当于禁用窗口功能
 * @note 配置窗口值会自动触发计数器重装载为 IWDT_RLR 中的 PL 值
 */
void iwdt_set_window(uint16_t window);

/**
 * @brief 获取窗口值
 *
 * 读取当前 IWDT 窗口寄存器配置。
 *
 * @return 当前窗口值
 * @note 覆盖寄存器：WINR.WIN
 *
 */
uint16_t iwdt_get_window(void);

/**
 * @brief 设置唤醒值
 *
 * 修改 IWDT 唤醒值，计数器减到此值时产生 WAKEUP 信号。
 *
 * @param[in] wakeup 唤醒值 (0-0xFFF)
 *
 * @note 覆盖寄存器：CMDR.CMD, WUTR.WUT
 * @note 写入前需确保已调用 iwdt_enable_write_access()
 * @note 修改后需等待 SR.WTU = 0
 * @note 设置为 0xFFF 相当于禁用唤醒功能
 * @note 唤醒功能需要 wakeup < reload 才能工作
 */
void iwdt_set_wakeup(uint16_t wakeup);

/**
 * @brief 获取唤醒值
 *
 * 读取当前 IWDT 唤醒寄存器配置。
 *
 * @return 当前唤醒值
 * @note 覆盖寄存器：WUTR.WUT
 *
 */
uint16_t iwdt_get_wakeup(void);

/**
 * @brief 获取 IWDT 状态标志
 *
 * 读取 IWDT 状态寄存器，返回所有状态标志的组合。
 *
 * @return 状态标志组合（iwdt_flag_t 位掩码）
 * @note 覆盖寄存器：SR（全部状态位）
 *
 */
uint32_t iwdt_get_status(void);

/**
 * @brief 检查指定状态标志是否置位
 *
 * @param[in] flag 要检查的状态标志
 * @return true  标志置位
 * @return false 标志未置位
 * @note 覆盖寄存器：SR.{PVU,RVU,WVU,WTU,RLF}
 *
 */
bool iwdt_is_flag_set(iwdt_flag_t flag);

/**
 * @brief 等待所有状态标志清零
 *
 * 等待 IWDT_SR 寄存器中所有更新标志变为 0。
 * 包含超时保护，超时后返回 false。
 *
 * @param[in] timeout 超时循环次数
 * @return true  所有标志已清零
 * @return false 超时
 * @note 覆盖寄存器：SR.{PVU,RVU,WVU,WTU,RLF}
 * @note 包含超时保护，超时后返回 false
 *
 */
bool iwdt_wait_ready(uint32_t timeout);

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 计算 IWDT 超时时间（毫秒）
 *
 * 根据预分频和重装载值计算看门狗超时时间。
 *
 * @param[in] prescaler 预分频值
 * @param[in] reload    重装载值
 * @return 超时时间（毫秒）
 *
 * @note 纯计算函数，不操作硬件寄存器
 * @note 公式：t = (reload + 1) × prescaler_div × 1000 / 32000
 * @note 使用 RC32K 标称频率 32kHz，实际频率在 30kHz~45kHz 之间
 */
uint32_t iwdt_calculate_timeout_ms(iwdt_prescaler_t prescaler, uint16_t reload);

/**
 * @brief 根据目标超时时间计算重装载值
 *
 * 反向计算达到目标超时时间所需的重装载寄存器值。
 *
 * @param[in] prescaler 预分频值
 * @param[in] timeout_ms 目标超时时间（毫秒）
 * @return 计算出的重装载值，超时时间太长则返回 IWDT_RELOAD_MAX
 *
 * @note 纯计算函数，不操作硬件寄存器
 * @note 公式：reload = timeout_ms × 32000 / (prescaler_div × 1000) - 1
 * @note 使用 RC32K 标称频率 32kHz
 */
uint16_t iwdt_calculate_reload(iwdt_prescaler_t prescaler, uint32_t timeout_ms);

/**
 * @brief 计算 IWDT 唤醒时间（毫秒）
 *
 * 根据预分频、重装载值和唤醒值计算唤醒时间。
 *
 * @param[in] prescaler 预分频值
 * @param[in] reload    重装载值
 * @param[in] wakeup    唤醒值
 * @return 唤醒时间（毫秒）
 *
 * @note 纯计算函数，不操作硬件寄存器
 * @note 公式：t_wu = (reload - wakeup + 1) × prescaler_div × 1000 / 32000
 * @note 使用 RC32K 标称频率 32kHz
 */
uint32_t iwdt_calculate_wakeup_ms(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t wakeup);

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief IWDT 唤醒中断服务程序
 *
 * 应用程序应在中断向量表中映射此函数。
 * 此函数会调用已注册的用户回调函数，并清除 EXTI pending。
 *
 * @note 中断号：IWDT_WKUP_IRQn (43)
 * @note 需要在 NVIC 中使能 IWDT_WKUP_IRQn
 * @note 需要在 EXTI 中配置 IWDT 唤醒线（EXTI line 18）
 * @note 回调中应调用 iwdt_feed() 喂狗并清除中断
 */
void IWDT_WKUP_IRQHandler(void);

#ifdef __cplusplus
}
#endif
#endif // _HARDWARE_IWDT_H
