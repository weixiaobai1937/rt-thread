/**
 * @file lptim.c
 * @brief ACM32P4 LPTIM 低功耗定时器驱动实现
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/lptim.h"
#include "hardware/clocks.h"
#include "system.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 内部宏定义
//===========================================

#define LPTIM_TIMEOUT_MS          10U         ///< 超时毫秒数

//===========================================
// 内部状态变量
//===========================================

static LPTIM_TypeDef * const lptim_instances[LPTIM_COUNT] = {
    (LPTIM_TypeDef *)LPTIM1_BASE,
};

static const clock_periph_t lptim_clock_periph[LPTIM_COUNT] = {
    CLK_LPTIM1,
};

static lptim_callback_t lptim_callbacks[LPTIM_COUNT] = { NULL };

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 LPTIM 寄存器指针
 */
static inline LPTIM_TypeDef* lptim_get_instance(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);
    return lptim_instances[lptim];
}

//===========================================
// 第1层：快速初始化 API
//===========================================

void lptim_init_counter(lptim_instance_t lptim, lptim_prescaler_t prescaler,
                        uint16_t arr, bool continuous)
{
    assert(lptim < LPTIM_COUNT);
    assert(arr > 0U);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    lptim_enable_clock(lptim);

    // 确保 ENABLE=0 以配置 CFGR
    lp->CR_f.ENABLE = 0U;

    // 配置为内部时钟计数模式
    lp->CFGR = 0U;
    lp->CFGR_f.CKSEL = 0U;
    lp->CFGR_f.COUNTMODE = 0U;
    lp->CFGR_f.PRESC = (uint32_t)prescaler;
    lp->CFGR_f.TRIGEN = 0U;

    // 禁止中断
    lp->IER = 0U;

    // 设置 ARR
    lp->ARR_f.ARR = arr;

    // 使能
    lp->CR_f.ENABLE = 1U;

    if (continuous) {
        lp->CR_f.CNTSTRT = 1U;
    } else {
        lp->CR_f.SNGSTRT = 1U;
    }
}

void lptim_init_pwm(lptim_instance_t lptim, lptim_prescaler_t prescaler,
                    uint16_t arr, uint16_t cmp)
{
    assert(lptim < LPTIM_COUNT);
    assert(arr > 0U);
    assert(cmp < arr);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    lptim_enable_clock(lptim);

    lp->CR_f.ENABLE = 0U;

    // 配置为内部时钟 PWM 输出
    lp->CFGR = 0U;
    lp->CFGR_f.CKSEL = 0U;
    lp->CFGR_f.COUNTMODE = 0U;
    lp->CFGR_f.PRESC = (uint32_t)prescaler;
    lp->CFGR_f.TRIGEN = 0U;
    lp->CFGR_f.WAVE = 0U;      // PWM/单脉冲模式
    lp->CFGR_f.WAVPOL = 0U;    // 正常极性

    lp->IER = 0U;

    lp->ARR_f.ARR = arr;
    lp->CMP_f.CMP = cmp;

    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
}

void lptim_init_encoder(lptim_instance_t lptim, lptim_edge_t edge, uint16_t arr)
{
    assert(lptim < LPTIM_COUNT);
    assert(arr > 0U);
    assert(edge <= LPTIM_EDGE_BOTH);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    lptim_enable_clock(lptim);

    lp->CR_f.ENABLE = 0U;

    // 编码器模式要求 CKSEL=0, PRESC=000 (1分频)
    lp->CFGR = 0U;
    lp->CFGR_f.CKSEL = 0U;
    lp->CFGR_f.COUNTMODE = 0U;
    lp->CFGR_f.PRESC = 0U;
    lp->CFGR_f.TRIGEN = 0U;
    lp->CFGR_f.ENC = 1U;
    lp->CFGR_f.CKPOL = (uint32_t)edge;

    lp->IER = 0U;

    lp->ARR_f.ARR = arr;

    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
}

void lptim_init_ext_counter(lptim_instance_t lptim, lptim_edge_t edge,
                            uint16_t arr, bool continuous)
{
    assert(lptim < LPTIM_COUNT);
    assert(arr > 0U);
    assert(edge <= LPTIM_EDGE_BOTH);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    lptim_enable_clock(lptim);

    lp->CR_f.ENABLE = 0U;

    // 配置为外部时钟计数模式
    lp->CFGR = 0U;
    lp->CFGR_f.CKSEL = 1U;
    lp->CFGR_f.CKPOL = (uint32_t)edge;
    lp->CFGR_f.TRIGEN = 0U;

    lp->IER = 0U;

    lp->ARR_f.ARR = arr;

    lp->CR_f.ENABLE = 1U;

    if (continuous) {
        lp->CR_f.CNTSTRT = 1U;
    } else {
        lp->CR_f.SNGSTRT = 1U;
    }
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool lptim_config(lptim_instance_t lptim, const lptim_config_t *config)
{
    assert(config != NULL);
    if (lptim >= LPTIM_COUNT) return false;
    if (config == NULL) return false;

    if (config->clock_sel > LPTIM_CLK_EXTERNAL) return false;
    if (config->count_mode > LPTIM_COUNT_EXTERNAL) return false;
    if (config->edge > LPTIM_EDGE_BOTH) return false;
    if (config->prescaler > LPTIM_PRESCALER_MAX) return false;
    if (config->trig_en > LPTIM_TRIG_BOTH) return false;
    if (config->trig_sel > LPTIM_TRIGSEL_MAX) return false;
    if (config->ck_filter > LPTIM_FILTER_MAX) return false;
    if (config->trig_filter > LPTIM_FILTER_MAX) return false;
    if (config->wave > LPTIM_WAVE_SET_ONCE) return false;
    if (config->polarity > LPTIM_POL_INVERT) return false;
    if (config->preload > LPTIM_UPDATE_PRELOAD) return false;
    if (config->rep_load > LPTIM_REP_LOAD_UNDERFLOW) return false;

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    lp->CFGR_f.CKSEL = (uint32_t)config->clock_sel;
    lp->CFGR_f.COUNTMODE = (uint32_t)config->count_mode;
    lp->CFGR_f.CKPOL = (uint32_t)config->edge;
    lp->CFGR_f.TRIGEN = (uint32_t)config->trig_en;
    lp->CFGR_f.TRIGSEL = (uint32_t)config->trig_sel;
    lp->CFGR_f.CKFLT = (uint32_t)config->ck_filter;
    lp->CFGR_f.TRGFLT = (uint32_t)config->trig_filter;
    lp->CFGR_f.WAVE = (uint32_t)config->wave;
    lp->CFGR_f.WAVPOL = (uint32_t)config->polarity;
    lp->CFGR_f.PRELOAD = (uint32_t)config->preload;
    lp->CFGR_f.REP_LODMOD = (uint32_t)config->rep_load;
    lp->CFGR_f.TIMEOUT = config->timeout_en ? 1U : 0U;
    lp->CFGR_f.ENC = config->encoder_en ? 1U : 0U;

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

// --- 启动/停止 ---

void lptim_start_continuous(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CR_f.CNTSTRT = 1U;
}

void lptim_start_single(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CR_f.SNGSTRT = 1U;
}

// --- CMP/ARR ---

void lptim_set_compare(lptim_instance_t lptim, uint16_t cmp)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CMP_f.CMP = cmp;
}

uint16_t lptim_get_compare(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (uint16_t)lp->CMP_f.CMP;
}

void lptim_set_autoreload(lptim_instance_t lptim, uint16_t arr)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->ARR_f.ARR = arr;
}

uint16_t lptim_get_autoreload(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (uint16_t)lp->ARR_f.ARR;
}

// --- 重复计数器 ---

void lptim_set_repeat(lptim_instance_t lptim, uint8_t rep)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->RCR_f.REP = (uint32_t)rep;
}

uint8_t lptim_get_repeat(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (uint8_t)lp->RCR_f.REP;
}

// --- 更新等待 ---

bool lptim_wait_arr_ok(lptim_instance_t lptim, uint32_t timeout)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    while (!lp->ISR_f.ARROK) {
        if (timeout > 0U) {
            timeout--;
            if (timeout == 0U) return false;
        }
    }

    return true;
}

bool lptim_wait_cmp_ok(lptim_instance_t lptim, uint32_t timeout)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    while (!lp->ISR_f.CMPOK) {
        if (timeout > 0U) {
            timeout--;
            if (timeout == 0U) return false;
        }
    }

    return true;
}

bool lptim_wait_rep_ok(lptim_instance_t lptim, uint32_t timeout)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    while (!lp->ISR_f.REPOK) {
        if (timeout > 0U) {
            timeout--;
            if (timeout == 0U) return false;
        }
    }

    return true;
}

// --- 输入选择 ---

void lptim_set_input1_sel(lptim_instance_t lptim, uint8_t sel)
{
    assert(lptim < LPTIM_COUNT);
    assert(sel <= LPTIM_INPUT_SEL_MAX);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR2_f.IN1SEL = (uint32_t)sel;
}

void lptim_set_input2_sel(lptim_instance_t lptim, uint8_t sel)
{
    assert(lptim < LPTIM_COUNT);
    assert(sel <= LPTIM_INPUT_SEL_MAX);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR2_f.IN2SEL = (uint32_t)sel;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

// --- CR 控制寄存器 ---

void lptim_enable(lptim_instance_t lptim, bool enable)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CR_f.ENABLE = enable ? 1U : 0U;
}

bool lptim_is_enabled(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lp->CR_f.ENABLE != 0U);
}

void lptim_counter_reset(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    uint64_t start = system_get_tick();
    while (lp->CR_f.COUNTRST && !system_elapsed(start, LPTIM_TIMEOUT_MS)) {
    }

    lp->CR_f.COUNTRST = 1U;
}

void lptim_set_reset_after_read(lptim_instance_t lptim, bool enable)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CR_f.RSTARE = enable ? 1U : 0U;
}

// --- CFGR ---

void lptim_set_clock_sel(lptim_instance_t lptim, lptim_clock_sel_t sel)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.CKSEL = (uint32_t)sel;
}

lptim_clock_sel_t lptim_get_clock_sel(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_clock_sel_t)lp->CFGR_f.CKSEL;
}

void lptim_set_count_mode(lptim_instance_t lptim, lptim_count_mode_t mode)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.COUNTMODE = (uint32_t)mode;
}

lptim_count_mode_t lptim_get_count_mode(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_count_mode_t)lp->CFGR_f.COUNTMODE;
}

void lptim_set_edge(lptim_instance_t lptim, lptim_edge_t edge)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.CKPOL = (uint32_t)edge;
}

lptim_edge_t lptim_get_edge(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_edge_t)lp->CFGR_f.CKPOL;
}

void lptim_set_prescaler(lptim_instance_t lptim, lptim_prescaler_t prescaler)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.PRESC = (uint32_t)prescaler;
}

lptim_prescaler_t lptim_get_prescaler(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_prescaler_t)lp->CFGR_f.PRESC;
}

void lptim_set_trigger(lptim_instance_t lptim, lptim_trig_en_t trig, uint8_t sel)
{
    assert(lptim < LPTIM_COUNT);
    assert(sel <= LPTIM_TRIGSEL_MAX);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.TRIGEN = (uint32_t)trig;
    lp->CFGR_f.TRIGSEL = (uint32_t)sel;
}

lptim_trig_en_t lptim_get_trigger(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_trig_en_t)lp->CFGR_f.TRIGEN;
}

uint8_t lptim_get_trigger_sel(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (uint8_t)lp->CFGR_f.TRIGSEL;
}

void lptim_set_filter(lptim_instance_t lptim, lptim_filter_t ck_filter,
                      lptim_filter_t trig_filter)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.CKFLT = (uint32_t)ck_filter;
    lp->CFGR_f.TRGFLT = (uint32_t)trig_filter;
}

lptim_filter_t lptim_get_ck_filter(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_filter_t)lp->CFGR_f.CKFLT;
}

lptim_filter_t lptim_get_trig_filter(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_filter_t)lp->CFGR_f.TRGFLT;
}

void lptim_set_wave_mode(lptim_instance_t lptim, lptim_wave_mode_t mode)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.WAVE = (uint32_t)mode;
}

lptim_wave_mode_t lptim_get_wave_mode(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_wave_mode_t)lp->CFGR_f.WAVE;
}

void lptim_set_polarity(lptim_instance_t lptim, lptim_polarity_t pol)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.WAVPOL = (uint32_t)pol;
}

lptim_polarity_t lptim_get_polarity(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_polarity_t)lp->CFGR_f.WAVPOL;
}

void lptim_set_preload(lptim_instance_t lptim, lptim_update_mode_t mode)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.PRELOAD = (uint32_t)mode;
}

lptim_update_mode_t lptim_get_preload(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_update_mode_t)lp->CFGR_f.PRELOAD;
}

void lptim_set_timeout(lptim_instance_t lptim, bool enable)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.TIMEOUT = enable ? 1U : 0U;
}

bool lptim_get_timeout(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lp->CFGR_f.TIMEOUT != 0U);
}

void lptim_set_encoder(lptim_instance_t lptim, bool enable)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.ENC = enable ? 1U : 0U;
}

bool lptim_get_encoder(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lp->CFGR_f.ENC != 0U);
}

void lptim_set_rep_load_mode(lptim_instance_t lptim, lptim_rep_load_mode_t mode)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->CFGR_f.REP_LODMOD = (uint32_t)mode;
}

lptim_rep_load_mode_t lptim_get_rep_load_mode(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return (lptim_rep_load_mode_t)lp->CFGR_f.REP_LODMOD;
}

// --- CNT 计数器 ---

uint16_t lptim_get_counter(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    uint16_t first;
    uint16_t second;

    first = (uint16_t)lp->CNT_f.CNT;
    second = (uint16_t)lp->CNT_f.CNT;

    if (first == second) {
        return first;
    }

    // 第三次读取进行打破平局
    uint16_t third = (uint16_t)lp->CNT_f.CNT;
    if (second == third) {
        return second;
    }

    return third;
}

// --- IE 中断使能 ---

void lptim_irq_enable(lptim_instance_t lptim, lptim_irq_t irq, bool enable)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);

    if (enable) {
        lp->IER |= (uint32_t)irq;
    } else {
        lp->IER &= ~((uint32_t)irq);
    }
}

bool lptim_irq_is_enabled(lptim_instance_t lptim, lptim_irq_t irq)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return ((lp->IER & (uint32_t)irq) != 0U);
}

void lptim_irq_disable_all(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->IER = 0U;
}

// --- ISR 状态查询与清除 ---

bool lptim_get_flag(lptim_instance_t lptim, lptim_flag_t flag)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return ((lp->ISR & (uint32_t)flag) != 0U);
}

void lptim_clear_flag(lptim_instance_t lptim, lptim_flag_t flag)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    lp->ICR = (uint32_t)flag;
}

uint32_t lptim_get_status(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    return lp->ISR;
}

// --- 中断回调 ---

void lptim_register_callback(lptim_instance_t lptim, lptim_callback_t callback)
{
    assert(lptim < LPTIM_COUNT);

    lptim_callbacks[lptim] = callback;
}

// --- 时钟 ---

void lptim_enable_clock(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    clock_periph_enable(lptim_clock_periph[lptim]);
}

void lptim_disable_clock(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    clock_periph_disable(lptim_clock_periph[lptim]);
}

//===========================================
// 辅助计算函数
//===========================================

uint32_t lptim_calculate_tick_ns(lptim_prescaler_t prescaler, uint32_t clk_hz)
{
    if (clk_hz == 0U) {
        return 0U;
    }

    uint32_t divisor = (uint32_t)(1U << (uint8_t)prescaler);

    // tick_ns = (divisor * 1e9) / clk_hz
    return (divisor * 1000000000U) / clk_hz;
}

uint16_t lptim_calculate_arr(uint32_t period_ns, lptim_prescaler_t prescaler,
                             uint32_t clk_hz)
{
    if (clk_hz == 0U || period_ns == 0U) {
        return 0U;
    }

    uint32_t divisor = (uint32_t)(1U << (uint8_t)prescaler);
    uint64_t tick_ns = ((uint64_t)divisor * 1000000000ULL) / (uint64_t)clk_hz;

    if (tick_ns == 0ULL) {
        return 0U;
    }

    uint64_t arr_val = (uint64_t)period_ns / tick_ns;
    if (arr_val > (uint64_t)LPTIM_ARR_MAX) {
        return 0U;
    }
    if (arr_val < 1ULL) {
        return 1U;
    }

    return (uint16_t)arr_val;
}

//===========================================
// 中断处理函数
//===========================================

void lptim_irq_handler(lptim_instance_t lptim)
{
    assert(lptim < LPTIM_COUNT);

    LPTIM_TypeDef *lp = lptim_get_instance(lptim);
    uint32_t isr = lp->ISR;
    uint32_t ier = lp->IER;
    uint32_t flags = isr & ier;

    if (flags == 0U) {
        return;
    }

    if (lptim_callbacks[lptim] != NULL) {
        lptim_callbacks[lptim](lptim, isr);
    }

    // 清除已产生中断的标志位（写 1 清零）
    lp->ICR = flags;
}

//===========================================
// 各实例中断处理入口
//===========================================

void LPTIM1_IRQHandler(void) { lptim_irq_handler(LPTIM_1); }
