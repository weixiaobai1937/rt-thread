/**
 * @file system.c
 * @brief ACM32P4 系统初始化实现
 *
 * 本文件实现应用层系统初始化，包括：
 * - 系统时钟初始化（内部 RCH + PLL1，220MHz）
 * - TIM64B 时基（自由计数模式，PCLK2 频率计数，无中断）
 * - 微秒/毫秒精确阻塞延时（直接读取 TIM64B 硬件计数器）
 * - NVIC 中断优先级分组
 * - 系统信息查询
 *
 * @note 时基架构：
 *   TIM64B 以 PCLK2 / TIM64B_TIMEBASE_PRESCALER 的频率持续自由计数。
 *   本模块在时基初始化时计算 ticks_per_us / ticks_per_ms，
 *   所有延时和时间戳函数均直接读取 tim64b_get_counter()，
 *   无需中断，不占用中断向量，不干扰 RTOS 调度器。
 *
 * @author ACM32P4 SDK Team
 * @version 2.1
 */

#include "hardware/system.h"
#include "../include/acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/tim64b.h"
#include <assert.h>

//===========================================
// 内部时基状态
//===========================================

/** 每微秒对应的 TIM64B 计数 tick 数 */
static uint32_t s_ticks_per_us = 0U;

/** 每毫秒对应的 TIM64B 计数 tick 数 */
static uint32_t s_ticks_per_ms = 0U;

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 配置 NVIC 中断优先级分组
 */
static void nvic_priority_group_config(uint32_t group)
{
    uint32_t reg = SCB->AIRCR;
    reg &= ~((uint32_t)0xFFFF8700U);
    reg |= (0x5FAUL << SCB_AIRCR_VECTKEY_Pos) | group;
    SCB->AIRCR = reg;
}

//===========================================
// 核心初始化实现
//===========================================

sys_status_t system_clock_init(void)
{
    /* 使用内部 RCH（64MHz）经 PLL1 倍频到目标频率
     *
     * PLL 参数计算（以 SYSTEM_CLOCK_HZ=220MHz 为例）：
     *   F_in  = RCH / 16 = 64MHz / 16 = 4MHz
     *   F_pfd = F_in / N = 4MHz / 2 = 2MHz        （N=2）
     *   F_vco = F_pfd * F = 2MHz * 220 = 440MHz    （F=220）
     *   F_pclk = F_vco / P = 440MHz / 2 = 220MHz   （P=0 → ÷2）
     *   HCLK   = SYSCLK = 220MHz
     *   PCLK2  = HCLK / 2 = 110MHz（供 TIM64B 时基使用）
     */
    bool ok;

    clock_set_sys_divider(1U, 1U);
    clock_set_apb_divider(2U, 2U);

    const pll_config_t pll1 = {
        .src = 0U,                                      /* 时钟源：RCH/16 (4MHz) */
        .n   = 2U,                                      /* 输入分频 N=2 → PFD=2MHz */
        .f   = (uint16_t)(SYSTEM_CLOCK_HZ / 1000000U), /* VCO 倍频系数 */
        .p   = 0U,                                      /* P 输出分频：÷2 */
        .q   = 3U,                                      /* Q 输出分频：÷4 */
    };
    ok = clock_pll1_configure(&pll1);
    if (!ok) {
        return SYS_ERR_CLOCK;
    }

    ok = clock_set_sysclk_source(SYSCLK_SRC_PLL1);
    if (!ok) {
        return SYS_ERR_CLOCK;
    }

    return SYS_OK;
}

sys_status_t system_timebase_init(void)
{
    /* 若 TIM64B 尚未启动（CEN=0），则初始化为自由计数模式。
     * 若已由 SystemInit() 提前启动，则跳过硬件初始化，保留已有计数值，
     * 仅重新计算 tick 换算系数（适应时钟频率变更场景）。
     *
     * 注：clock_get_pclk2_hz() 直接读 RCC 硬件寄存器，无需 clock_state
     * 已更新，因此本函数可在 system_clock_init() 之前安全调用。
     *
     * 计数频率 = PCLK2 / prescaler
     * 默认：PCLK2=110MHz，prescaler=1 → 计数频率 110MHz，精度 ~9ns
     */
    if (!tim64b_is_running()) {
        tim64b_init_freerun(TIM64B_TIMEBASE_PRESCALER);
    }

    /* 计算 tick 换算系数，供延时和时间戳函数使用 */
    uint32_t count_freq_hz = clock_get_pclk2_hz() / TIM64B_TIMEBASE_PRESCALER;
    s_ticks_per_us = count_freq_hz / 1000000U;
    s_ticks_per_ms = count_freq_hz / 1000U;

    /* 保护：极低频率下至少保留 1，避免除零 */
    if (s_ticks_per_us == 0U) {
        s_ticks_per_us = 1U;
    }
    if (s_ticks_per_ms == 0U) {
        s_ticks_per_ms = 1U;
    }

    return SYS_OK;
}
/* 系统时钟更新换算系数*/
sys_status_t system_timbase_update(void)
{
    uint32_t count_freq_hz = clock_get_pclk2_hz() / TIM64B_TIMEBASE_PRESCALER;
    s_ticks_per_us = count_freq_hz / 1000000U;
    s_ticks_per_ms = count_freq_hz / 1000U;
    return SYS_OK;
}


sys_status_t system_init(void)
{
    sys_status_t ret;

    /* 1. 配置 NVIC 中断优先级分组 */
    nvic_priority_group_config(SYSTEM_NVIC_PRIGROUP);

    /* 2. 校准 TIM64B 时基换算系数
     *    TIM64B 已由 SystemInit() 提前启动，此处仅计算 ticks_per_us/ms。
     *    必须在 system_clock_init() 之前完成，使后续调用 delay_ms/delay_us
     *    的驱动初始化代码（以及 system_clock_init 内部如有需要）可以正常工作。 */
    ret = system_timebase_init();
    if (ret != SYS_OK) {
        return ret;
    }

    /* 3. 同步时钟状态（更新 clock_state 并重配 PLL）
     *    clock_get_pclk2_hz() 读硬件寄存器，不依赖 clock_state，
     *    故步骤 2 放在前面不影响 tick 换算精度。 */
    ret = system_clock_init();
    if (ret != SYS_OK) {
        return ret;
    }

    /* 4. 更新时基换算系数*/
    ret = system_timbase_update();
    if (ret != SYS_OK) {
        return ret;
    }

    return SYS_OK;
}

//===========================================
// 延时实现（直接读取 TIM64B 硬件计数器）
//===========================================

void delay_us(uint32_t us)
{
    if (us == 0U) {
        return;
    }
    uint64_t ticks = (uint64_t)us * s_ticks_per_us;
    uint64_t start = tim64b_get_counter();
    while ((tim64b_get_counter() - start) < ticks) {
        /* 忙等待：64位无符号减法自然处理计数器回绕 */
    }
}

void delay_ms(uint32_t ms)
{
    if (ms == 0U) {
        return;
    }
    uint64_t ticks = (uint64_t)ms * s_ticks_per_ms;
    uint64_t start = tim64b_get_counter();
    while ((tim64b_get_counter() - start) < ticks) {
        /* 忙等待 */
    }
}

//===========================================
// 系统时间实现
//===========================================

uint64_t system_get_tick(void)
{
    return tim64b_get_counter() / s_ticks_per_ms;
}

bool system_elapsed(uint64_t start_tick, uint32_t ms)
{
    uint64_t now = tim64b_get_counter() / s_ticks_per_ms;
    return (now - start_tick) >= (uint64_t)ms;
}

//===========================================
// 系统信息实现
//===========================================

void system_get_info(sys_info_t *info)
{
    assert(info != NULL);

    info->sysclk_hz  = clock_get_hz();
    info->hclk_hz    = clock_get_hclk_hz();
    info->pclk1_hz   = clock_get_pclk1_hz();
    info->pclk2_hz   = clock_get_pclk2_hz();
    info->uptime_ms  = tim64b_get_counter() / s_ticks_per_ms;
}

void system_reset(void)
{
    clock_system_reset();
}
