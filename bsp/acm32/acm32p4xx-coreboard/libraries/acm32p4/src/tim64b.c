/**
 * @file tim64b.c
 * @brief ACM32H5 64位定时器（TIM64B）驱动实现
 *
 * 本文件实现了 ACM32H5 芯片 TIM64B 的驱动功能，支持：
 * - 自由计数模式（无中断，计满 2^64-1 后回零）
 * - 循环计数模式（计数到 ARR 后产生中断并回零）
 * - 完整的中断管理和回调机制
 *
 * @note 延时、时间戳等高层时间服务已移至 system.c，
 *       本驱动只负责寄存器操作和原始计数器读取。
 *
 * @author ACM32H5 SDK Team
 * @version 1.2
 */

#include "hardware/tim64b.h"
#include "hardware/clocks.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <stddef.h>

//===========================================
// 内部宏定义
//===========================================

/** TIM64B 控制寄存器位位置定义 */
#define TIM64B_CTRL_CEN_Pos      (0U)
#define TIM64B_CTRL_CEN_Msk      (0x1UL << TIM64B_CTRL_CEN_Pos)

#define TIM64B_CTRL_CMOD_Pos     (1U)
#define TIM64B_CTRL_CMOD_Msk     (0x1UL << TIM64B_CTRL_CMOD_Pos)

#define TIM64B_CTRL_PSC_Pos      (2U)   ///< 预分频位位置（占 [7:2]，共6位）
#define TIM64B_CTRL_PSC_Msk      (0x3FUL << TIM64B_CTRL_PSC_Pos)

#define TIM64B_CTRL_IE_Pos       (8U)
#define TIM64B_CTRL_IE_Msk       (0x1UL << TIM64B_CTRL_IE_Pos)

#define TIM64B_CTRL_ARPE_Pos     (9U)
#define TIM64B_CTRL_ARPE_Msk     (0x1UL << TIM64B_CTRL_ARPE_Pos)

#define TIM64B_CTRL_ARRLOAD_Pos  (10U)  ///< WO，写1触发ARR加载，硬件自动清零
#define TIM64B_CTRL_ARRLOAD_Msk  (0x1UL << TIM64B_CTRL_ARRLOAD_Pos)

#define TIM64B_CTRL_CNTLOAD_Pos  (11U)  ///< WO，写1触发CNT加载，硬件自动清零
#define TIM64B_CTRL_CNTLOAD_Msk  (0x1UL << TIM64B_CTRL_CNTLOAD_Pos)

/** TIM64B 状态寄存器位位置定义 */
#define TIM64B_SR_UF_Pos         (8U)   ///< 更新事件标志，写1清零
#define TIM64B_SR_UF_Msk         (0x1UL << TIM64B_SR_UF_Pos)

//===========================================
// 内部状态变量
//===========================================

/** TIM64B 驱动内部状态 */
static struct {
    tim64b_callback_t callback;     ///< 用户注册的更新中断回调函数
} tim64b_state = {
    .callback = NULL,
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 修正预分频参数到有效范围（1-64）
 */
static uint8_t clamp_prescaler(uint8_t prescaler)
{
    if (prescaler < TIM64B_PRESCALER_MIN) {
        return TIM64B_PRESCALER_MIN;
    }
    if (prescaler > TIM64B_PRESCALER_MAX) {
        return TIM64B_PRESCALER_MAX;
    }
    return prescaler;
}

/**
 * @brief 将 ARR 值写入寄存器并触发加载
 *
 * 先写低32位，再写高32位，最后置位 ARRLOAD。
 */
static void write_arr(uint64_t arr)
{
    TIM26->ARRL = (uint32_t)(arr & 0xFFFFFFFFUL);
    TIM26->ARRH = (uint32_t)(arr >> 32);
    TIM26->CTRL |= TIM64B_CTRL_ARRLOAD_Msk;
}

/**
 * @brief 将 CNT 值写入寄存器并触发加载
 *
 * 先写低32位，再写高32位，最后置位 CNTLOAD。
 */
static void write_cnt(uint64_t cnt)
{
    TIM26->CNTL = (uint32_t)(cnt & 0xFFFFFFFFUL);
    TIM26->CNTH = (uint32_t)(cnt >> 32);
    TIM26->CTRL |= TIM64B_CTRL_CNTLOAD_Msk;
}

//===========================================
//===========================================
// 第1层：快速初始化 API
//===========================================

void tim64b_init_freerun(uint8_t prescaler)
{
    clock_periph_enable(CLK_TIM26);
    prescaler = clamp_prescaler(prescaler);

    /* 配置为自由计数模式：CMOD=0，IE=0，ARPE=0，CEN=0
     * 一次写入整个 CTRL 寄存器，避免中间状态 */
    TIM26->CTRL = (uint32_t)(prescaler - 1U) << TIM64B_CTRL_PSC_Pos;

    write_cnt(0U);

    TIM26->CTRL |= TIM64B_CTRL_CEN_Msk;
}

void tim64b_init_periodic(uint64_t arr, uint8_t prescaler)
{
    clock_periph_enable(CLK_TIM26);
    prescaler = clamp_prescaler(prescaler);

    // 停止计数器，配置为循环模式：CMOD=1，IE=1
    TIM26->CTRL = TIM64B_CTRL_CMOD_Msk
                | TIM64B_CTRL_IE_Msk
                | ((uint32_t)(prescaler - 1U) << TIM64B_CTRL_PSC_Pos);

    // 写入 ARR 并触发加载
    write_arr(arr);

    // 计数器清零并触发加载
    write_cnt(0U);

    // 清除残留的更新标志
    TIM26->SR = TIM64B_SR_UF_Msk;

    // 启动计数器
    TIM26->CTRL |= TIM64B_CTRL_CEN_Msk;
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool tim64b_config(const tim64b_config_t *config)
{
    assert(config != NULL);

    clock_periph_enable(CLK_TIM26);
    
    uint8_t prescaler = clamp_prescaler(config->prescaler);

    // 循环模式下 ARR 不能为0（ARR=0 时计数器不工作）
    if (config->mode == TIM64B_MODE_PERIODIC && config->arr == 0U) {
        return false;
    }

    // 构造 CTRL 寄存器值（CEN=0，停止计数器）
    uint32_t ctrl = (uint32_t)(prescaler - 1U) << TIM64B_CTRL_PSC_Pos;

    if (config->mode == TIM64B_MODE_PERIODIC) {
        ctrl |= TIM64B_CTRL_CMOD_Msk;
    }
    if (config->irq_enable) {
        ctrl |= TIM64B_CTRL_IE_Msk;
    }
    if (config->arpe) {
        ctrl |= TIM64B_CTRL_ARPE_Msk;
    }

    // 写入 CTRL（停止并配置，CEN 位为0）
    TIM26->CTRL = ctrl;

    // 写入 ARR 并触发加载
    write_arr(config->arr);

    // 写入 CNT 并触发加载
    write_cnt(config->counter);

    // 清除残留更新标志
    TIM26->SR = TIM64B_SR_UF_Msk;

    // 启动计数器
    TIM26->CTRL |= TIM64B_CTRL_CEN_Msk;

    return true;
}

void tim64b_set_prescaler(uint8_t prescaler)
{
    prescaler = clamp_prescaler(prescaler);
    TIM26->CTRL = (TIM26->CTRL & ~TIM64B_CTRL_PSC_Msk)
                | ((uint32_t)(prescaler - 1U) << TIM64B_CTRL_PSC_Pos);
}

void tim64b_set_arr(uint64_t arr)
{
    write_arr(arr);
}

void tim64b_set_counter(uint64_t cnt)
{
    write_cnt(cnt);
}

void tim64b_set_arpe(bool enable)
{
    if (enable) {
        TIM26->CTRL |= TIM64B_CTRL_ARPE_Msk;
    } else {
        TIM26->CTRL &= ~TIM64B_CTRL_ARPE_Msk;
    }
}

//===========================================
// 第3层：高级功能 API
//===========================================

void tim64b_irq_enable(bool enable)
{
    if (enable) {
        TIM26->CTRL |= TIM64B_CTRL_IE_Msk;
    } else {
        TIM26->CTRL &= ~TIM64B_CTRL_IE_Msk;
    }
}

void tim64b_irq_register(tim64b_callback_t callback)
{
    tim64b_state.callback = callback;
}

void tim64b_irq_unregister(void)
{
    tim64b_state.callback = NULL;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void tim64b_start(void)
{
    TIM26->CTRL |= TIM64B_CTRL_CEN_Msk;
}

void tim64b_stop(void)
{
    TIM26->CTRL &= ~TIM64B_CTRL_CEN_Msk;
}

uint64_t tim64b_get_counter(void)
{
    // 必须先读 CNTL（低位），硬件会同时锁存 CNTH（高位）
    uint32_t lo = TIM26->CNTL;
    uint32_t hi = TIM26->CNTH;
    return ((uint64_t)hi << 32) | lo;
}

uint64_t tim64b_get_arr(void)
{
    uint32_t lo = TIM26->ARRL;
    uint32_t hi = TIM26->ARRH;
    return ((uint64_t)hi << 32) | lo;
}

uint8_t tim64b_get_prescaler(void)
{
    uint32_t psc_reg = (TIM26->CTRL & TIM64B_CTRL_PSC_Msk) >> TIM64B_CTRL_PSC_Pos;
    return (uint8_t)(psc_reg + 1U);
}

bool tim64b_is_running(void)
{
    return (TIM26->CTRL & TIM64B_CTRL_CEN_Msk) != 0U;
}

bool tim64b_flag_is_set(void)
{
    return (TIM26->SR & TIM64B_SR_UF_Msk) != 0U;
}

void tim64b_flag_clear(void)
{
    // 写1清零（写0无效）
    TIM26->SR = TIM64B_SR_UF_Msk;
}


//===========================================
// 中断处理函数
//===========================================

/**
 * @brief TIM64B 中断服务函数
 *
 * 检查并清除更新事件标志（UF），然后调用用户注册的回调函数。
 *
 * @note 用户不可重定义此函数，请通过 tim64b_irq_register() 注册回调
 */
/* 向量表名固定为 TIM26_IRQHandler，不可修改 */
void TIM64B_IRQHandler(void)
{
    if (tim64b_flag_is_set()) {
        tim64b_flag_clear();
        if (tim64b_state.callback != NULL) {
            tim64b_state.callback();
        }
    }
}
