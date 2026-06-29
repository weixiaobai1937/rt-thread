/**
 * @file system.h
 * @brief ACM32P4 系统初始化模块
 *
 * 提供应用层系统初始化接口，包括：
 * - 系统时钟初始化（220MHz PLL）
 * - TIM64B 硬件时基（自由计数模式，无中断）
 * - 微秒/毫秒精确阻塞延时（硬件计数器实现）
 * - NVIC 中断优先级分组
 * - 系统信息查询
 * - 软件复位
 *
 * @note 时基说明：
 *   本模块使用 TIM64B（映射到 TIM26）替代 SysTick 作为系统时基。
 *   TIM64B 以 PCLK2 频率（默认 110MHz）自由计数，64 位计数器约
 *   584,542 年才会溢出，无需处理回绕。延时精度取决于 PCLK2 频率。
 *
 * 典型使用方式：
 * @code
 * int main(void)
 * {
 *     system_init();           // 一行完成全部系统初始化
 *     delay_ms(100);           // 延时 100ms（硬件计数器实现）
 *     delay_us(50);            // 延时 50us
 *
 *     uint64_t t0 = system_get_tick();
 *     do_something();
 *     if (system_elapsed(t0, 200)) {
 *         // do_something() 耗时超过 200ms
 *     }
 * }
 * @endcode
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#ifndef _HARDWARE_SYSTEM_H
#define _HARDWARE_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

//===========================================
// 配置宏（可在编译时覆盖）
//===========================================

/**
 * @brief 系统目标时钟频率（默认 220MHz）
 *
 * 可通过编译选项覆盖，例如：
 *   -DSYSTEM_CLOCK_HZ=160000000  // 使用 160MHz
 */
#ifndef SYSTEM_CLOCK_HZ
#define SYSTEM_CLOCK_HZ     180000000U
#endif

/**
 * @brief TIM64B 预分频值（默认 1，不分频）
 *
 * 计数频率 = PCLK2 / TIM64B_PRESCALER。
 * 在 PCLK2=110MHz、预分频=1 时，计数频率 110MHz，精度约 9ns。
 * 有效范围：1-64。
 */
#ifndef TIM64B_TIMEBASE_PRESCALER
#define TIM64B_TIMEBASE_PRESCALER   1U
#endif

/**
 * @brief NVIC 中断优先级分组配置
 *
 *   NVIC_PRIGROUP_4_0 = 4 位抢占 / 0 位子优先级（推荐）
 *   NVIC_PRIGROUP_3_1 = 3 位抢占 / 1 位子优先级
 *   NVIC_PRIGROUP_2_2 = 2 位抢占 / 2 位子优先级
 */
#define NVIC_PRIGROUP_4_0   0x00000300U
#define NVIC_PRIGROUP_3_1   0x00000400U
#define NVIC_PRIGROUP_2_2   0x00000500U

#ifndef SYSTEM_NVIC_PRIGROUP
#define SYSTEM_NVIC_PRIGROUP    NVIC_PRIGROUP_4_0
#endif

//===========================================
// 类型定义
//===========================================

/**
 * @brief 系统初始化状态码
 */
typedef enum {
    SYS_OK              = 0,    ///< 初始化成功
    SYS_ERR_CLOCK       = 1,    ///< 时钟初始化失败（PLL 配置超时）
    SYS_ERR_TIMEBASE    = 2,    ///< 时基初始化失败
} sys_status_t;

/**
 * @brief 系统信息结构体
 */
typedef struct {
    uint32_t sysclk_hz;     ///< 系统时钟频率（Hz）
    uint32_t hclk_hz;       ///< AHB 总线时钟频率（Hz）
    uint32_t pclk1_hz;      ///< APB1 总线时钟频率（Hz）
    uint32_t pclk2_hz;      ///< APB2 总线时钟频率（Hz）
    uint64_t uptime_ms;     ///< 系统运行毫秒数（TIM64B 计数换算）
} sys_info_t;

//===========================================
// 核心初始化 API
//===========================================

/**
 * @brief 系统全量初始化（推荐入口）
 *
 * 按顺序执行：
 *   1. NVIC 中断优先级分组配置
 *   2. 系统时钟初始化（RCH → PLL1 → 220MHz）
 *   3. TIM64B 时基初始化（自由计数模式，无中断）
 *
 * @return SYS_OK 表示成功，其他值表示对应步骤失败
 *
 * @note 本函数应在 main() 的第一行调用，不依赖任何其他模块
 */
sys_status_t system_init(void);

/**
 * @brief 单独初始化系统时钟
 *
 * 使用内部 RCH（64MHz）经 PLL1 倍频至 SYSTEM_CLOCK_HZ。
 * 分频配置：HCLK = SYSCLK，PCLK1/2/3/4 = SYSCLK / 2。
 *
 * @return SYS_OK 成功，SYS_ERR_CLOCK PLL 配置失败
 */
sys_status_t system_clock_init(void);

/**
 * @brief 单独初始化 TIM64B 时基
 *
 * 配置 TIM64B（TIM26）为自由计数模式，以 PCLK2 / TIM64B_TIMEBASE_PRESCALER
 * 的频率持续计数，不产生中断。
 *
 * @note 必须在 system_clock_init() 之后调用（需要 PCLK2 频率信息）
 * @note TIM64B 时基不占用任何中断向量，不干扰 RTOS 或其他定时器
 *
 * @return SYS_OK（当前实现始终成功）
 */
sys_status_t system_timebase_init(void);

//===========================================
// 延时 API
//===========================================

/**
 * @brief 阻塞延时（毫秒）
 *
 * 基于 TIM64B 硬件计数器实现，精度由 PCLK2 决定。
 *
 * @param ms 延时毫秒数（0 表示立即返回）
 *
 * @note 依赖 TIM64B 时基，需先调用 system_init() 或 system_timebase_init()
 */
void delay_ms(uint32_t ms);

/**
 * @brief 阻塞延时（微秒）
 *
 * 基于 TIM64B 硬件计数器实现，精度约为 1/PCLK2（默认 ~9ns@110MHz）。
 *
 * @param us 延时微秒数（0 表示立即返回）
 *
 * @note 依赖 TIM64B 时基，需先调用 system_init() 或 system_timebase_init()
 */
void delay_us(uint32_t us);

//===========================================
// 系统时间 API
//===========================================

/**
 * @brief 获取系统运行毫秒数
 *
 * 读取 TIM64B 计数器并换算为毫秒，64 位精度，约 584,542 年才溢出。
 *
 * @return 自 TIM64B 时基初始化以来的毫秒数（uint64_t）
 */
uint64_t system_get_tick(void);

/**
 * @brief 检查是否已经过了指定毫秒数（非阻塞超时判断）
 *
 * @code
 * uint64_t t0 = system_get_tick();
 * while (!system_elapsed(t0, 100)) {
 *     // 执行其他任务
 * }
 * @endcode
 *
 * @param start_tick 起始 tick（来自 system_get_tick()）
 * @param ms         期望经过的毫秒数
 * @return true 已超过 ms 毫秒，false 尚未到达
 */
bool system_elapsed(uint64_t start_tick, uint32_t ms);

//===========================================
// 系统信息 API
//===========================================

/**
 * @brief 获取系统时钟信息快照
 *
 * @param info 输出结构体指针（不能为 NULL）
 */
void system_get_info(sys_info_t *info);

/**
 * @brief 软件复位系统
 *
 * @note 此函数不会返回
 */
void system_reset(void) __attribute__((noreturn));

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_SYSTEM_H
