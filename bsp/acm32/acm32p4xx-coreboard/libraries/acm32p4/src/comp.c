/**
 * @file comp.c
 * @brief ACM32P4 模拟比较器（COMP）驱动实现
 *
 * 本文件实现了ACM32P4芯片的模拟比较器功能，提供了以下接口：
 * - 基本比较器：正端 > 负端时输出高电平
 * - 迟滞比较器：可编程迟滞窗口（10/20/30/40mV）
 * - 滤波比较器：数字滤波消除输出毛刺
 * - 内部分压模式：VREF/AVDD 可编程分压作为负端输入
 * - 消隐功能：通过定时器切断输出
 * - 寄存器锁定保护
 * - 中断管理和回调机制
 *
 * @note COMP1 基地址 0x40010200
 * @note 中断号：COMP1_IRQn (17)
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/comp.h"
#include "device/acm32p4xx.h"
#include <assert.h>

//===========================================
// 内部宏定义
//===========================================

#define COMP_INPSEL_MASK    0xFU    ///< INPSEL 位域掩码
#define COMP_INMSEL_MASK    0xFU    ///< INMSEL 位域掩码
#define COMP_HYS_MASK       0x7U    ///< HYS 位域掩码
#define COMP_FLTTIME_MASK   0x7U    ///< FLTTIME 位域掩码
#define COMP_BLANKSEL_MASK  0x7U    ///< BLANKSEL 位域掩码
#define COMP_VREF_CFG_MASK  0xFU    ///< CRV_CFG 位域掩码
#define COMP_VREF_CFG_MAX   15U     ///< 分压系数最大值

//===========================================
// 内部类型定义
//===========================================

/**
 * @brief COMP 回调函数集合
 */
typedef struct {
    comp_callback_t callback;   ///< 用户注册的中断回调函数
    bool            enabled;    ///< COMP 使能状态
} comp_state_t;

//===========================================
// 内部状态变量
//===========================================

/**
 * @brief COMP 实例基地址表
 */
static COMP_TypeDef * const comp_instances[COMP_COUNT] = {
    (COMP_TypeDef *)COMP1_BASE,
};

/**
 * @brief COMP 内部状态（按实例索引）
 */
static comp_state_t comp_state[COMP_COUNT] = {
    [COMP_1] = { .callback = NULL, .enabled = false },
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取 COMP 寄存器指针
 * @param comp COMP 实例编号
 * @return COMP 寄存器指针
 */
static inline COMP_TypeDef* comp_get_instance(comp_instance_t comp)
{
    assert(comp < COMP_COUNT);
    return comp_instances[comp];
}

/**
 * @brief 检查 inp_sel 参数是否有效
 */
static inline bool comp_is_valid_inp_sel(comp_inp_sel_t inp_sel)
{
    return (inp_sel <= COMP_INP_PB2);
}

/**
 * @brief 检查 inm_sel 参数是否有效
 */
static inline bool comp_is_valid_inm_sel(comp_inm_sel_t inm_sel)
{
    return (inm_sel <= COMP_INM_VREFDIV);
}

/**
 * @brief 检查 hysteresis 参数是否有效
 */
static inline bool comp_is_valid_hysteresis(comp_hysteresis_t hysteresis)
{
    return (hysteresis == COMP_HYS_DISABLE)
        || (hysteresis == COMP_HYS_10MV)
        || (hysteresis == COMP_HYS_20MV)
        || (hysteresis == COMP_HYS_30MV)
        || (hysteresis == COMP_HYS_40MV);
}

/**
 * @brief 检查 filter_time 参数是否有效
 */
static inline bool comp_is_valid_filter_time(comp_filter_time_t filter_time)
{
    return (filter_time <= COMP_FLT_4095CYCLES);
}

/**
 * @brief 检查 blank_sel 参数是否有效
 */
static inline bool comp_is_valid_blank_sel(comp_blank_sel_t blank_sel)
{
    return (blank_sel <= COMP_BLANK_ALL);
}

/**
 * @brief 检查 vref_cfg 参数是否有效
 */
static inline bool comp_is_valid_vref_cfg(uint8_t vref_cfg)
{
    return (vref_cfg <= COMP_VREF_CFG_MAX);
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化比较器为基本模式
 */
void comp_init_basic(comp_instance_t comp, comp_inp_sel_t inp_sel,
                     comp_inm_sel_t inm_sel)
{
    COMP_TypeDef *p = comp_get_instance(comp);

    // 禁用比较器
    p->CR_f.EN = 0U;

    // 配置正端和负端输入
    p->CR_f.INPSEL = (uint32_t)inp_sel;
    p->CR_f.INMSEL = (uint32_t)inm_sel;

    // 默认配置：直接输出，无迟滞，无滤波，无消隐
    p->CR_f.POLARITY = 0U;
    p->CR_f.HYS = 0U;
    p->CR_f.FLTEN = 0U;
    p->CR_f.FLTTIME = 0U;
    p->CR_f.BLANKSEL = 0U;
    p->CR_f.CRV_EN = 0U;

    // 使能比较器
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;
}

/**
 * @brief 快速初始化比较器为迟滞模式
 */
void comp_init_hysteresis(comp_instance_t comp, comp_inp_sel_t inp_sel,
                          comp_inm_sel_t inm_sel, comp_hysteresis_t hysteresis)
{
    COMP_TypeDef *p = comp_get_instance(comp);

    // 禁用比较器
    p->CR_f.EN = 0U;

    // 配置正端和负端输入
    p->CR_f.INPSEL = (uint32_t)inp_sel;
    p->CR_f.INMSEL = (uint32_t)inm_sel;

    // 默认配置
    p->CR_f.POLARITY = 0U;
    p->CR_f.FLTEN = 0U;
    p->CR_f.FLTTIME = 0U;
    p->CR_f.BLANKSEL = 0U;
    p->CR_f.CRV_EN = 0U;

    // 配置迟滞
    p->CR_f.HYS = (uint32_t)hysteresis;

    // 使能比较器
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;
}

/**
 * @brief 快速初始化比较器为滤波模式
 */
void comp_init_filtered(comp_instance_t comp, comp_inp_sel_t inp_sel,
                        comp_inm_sel_t inm_sel, comp_filter_time_t filter_time)
{
    COMP_TypeDef *p = comp_get_instance(comp);

    // 禁用比较器
    p->CR_f.EN = 0U;

    // 配置正端和负端输入
    p->CR_f.INPSEL = (uint32_t)inp_sel;
    p->CR_f.INMSEL = (uint32_t)inm_sel;

    // 默认配置
    p->CR_f.POLARITY = 0U;
    p->CR_f.HYS = 0U;
    p->CR_f.BLANKSEL = 0U;
    p->CR_f.CRV_EN = 0U;

    // 配置滤波
    p->CR_f.FLTEN = 1U;
    p->CR_f.FLTTIME = (uint32_t)filter_time;

    // 使能比较器
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;
}

/**
 * @brief 快速初始化比较器为内部分压模式
 */
void comp_init_vref_divider(comp_instance_t comp, comp_inp_sel_t inp_sel,
                            comp_vref_sel_t vref_sel, uint8_t vref_cfg)
{
    COMP_TypeDef *p = comp_get_instance(comp);

    // 禁用比较器
    p->CR_f.EN = 0U;

    // 配置正端输入，负端选择内部分压
    p->CR_f.INPSEL = (uint32_t)inp_sel;
    p->CR_f.INMSEL = (uint32_t)COMP_INM_VREFDIV;

    // 默认配置
    p->CR_f.POLARITY = 0U;
    p->CR_f.HYS = 0U;
    p->CR_f.FLTEN = 0U;
    p->CR_f.FLTTIME = 0U;
    p->CR_f.BLANKSEL = 0U;

    // 配置内部分压
    p->CR_f.CRV_SEL = (uint32_t)vref_sel;
    p->CR_f.CRV_CFG = (uint32_t)vref_cfg;
    p->CR_f.CRV_EN = 1U;

    // 使能比较器
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 COMP 并启动
 */
bool comp_config(comp_instance_t comp, const comp_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (!comp_is_valid_inp_sel(config->inp_sel)) {
        return false;
    }

    if (!comp_is_valid_inm_sel(config->inm_sel)) {
        return false;
    }

    if (!comp_is_valid_hysteresis(config->hysteresis)) {
        return false;
    }

    if (config->filter_enable && !comp_is_valid_filter_time(config->filter_time)) {
        return false;
    }

    if (!comp_is_valid_blank_sel(config->blank_sel)) {
        return false;
    }

    if (config->vref_enable && !comp_is_valid_vref_cfg(config->vref_cfg)) {
        return false;
    }

    COMP_TypeDef *p = comp_get_instance(comp);

    // 禁用比较器
    p->CR_f.EN = 0U;

    // 配置输入源
    p->CR_f.INPSEL = (uint32_t)config->inp_sel;
    p->CR_f.INMSEL = (uint32_t)config->inm_sel;

    // 配置极性
    p->CR_f.POLARITY = (uint32_t)config->polarity;

    // 配置迟滞
    p->CR_f.HYS = (uint32_t)config->hysteresis;

    // 配置消隐
    p->CR_f.BLANKSEL = (uint32_t)config->blank_sel;

    // 配置滤波
    if (config->filter_enable) {
        p->CR_f.FLTEN = 1U;
        p->CR_f.FLTTIME = (uint32_t)config->filter_time;
    } else {
        p->CR_f.FLTEN = 0U;
        p->CR_f.FLTTIME = 0U;
    }

    // 配置内部分压
    if (config->vref_enable) {
        p->CR_f.CRV_SEL = (uint32_t)config->vref_sel;
        p->CR_f.CRV_CFG = (uint32_t)config->vref_cfg;
        p->CR_f.CRV_EN = 1U;
    } else {
        p->CR_f.CRV_EN = 0U;
    }

    // 使能比较器
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;

    // 锁定配置（如需要）
    if (config->lock) {
        p->CR_f.LOCK = 1U;
    }

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 锁定 COMP 配置寄存器
 */
void comp_lock(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.LOCK = 1U;
}

/**
 * @brief 注册 COMP 中断回调函数
 */
void comp_register_callback(comp_instance_t comp, comp_callback_t callback)
{
    comp_state[comp].callback = callback;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能比较器
 */
void comp_enable(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.EN = 1U;
    comp_state[comp].enabled = true;
}

/**
 * @brief 禁止比较器
 */
void comp_disable(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.EN = 0U;
    comp_state[comp].enabled = false;
}

/**
 * @brief 设置正端输入源
 */
void comp_set_positive_input(comp_instance_t comp, comp_inp_sel_t inp_sel)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.INPSEL = (uint32_t)inp_sel;
}

/**
 * @brief 获取正端输入源配置
 */
comp_inp_sel_t comp_get_positive_input(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (comp_inp_sel_t)(p->CR_f.INPSEL);
}

/**
 * @brief 设置负端输入源
 */
void comp_set_negative_input(comp_instance_t comp, comp_inm_sel_t inm_sel)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.INMSEL = (uint32_t)inm_sel;
}

/**
 * @brief 获取负端输入源配置
 */
comp_inm_sel_t comp_get_negative_input(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (comp_inm_sel_t)(p->CR_f.INMSEL);
}

/**
 * @brief 设置输出极性
 */
void comp_set_polarity(comp_instance_t comp, comp_polarity_t polarity)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.POLARITY = (uint32_t)polarity;
}

/**
 * @brief 获取输出极性配置
 */
comp_polarity_t comp_get_polarity(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (comp_polarity_t)(p->CR_f.POLARITY);
}

/**
 * @brief 设置迟滞窗口
 */
void comp_set_hysteresis(comp_instance_t comp, comp_hysteresis_t hysteresis)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.HYS = (uint32_t)hysteresis;
}

/**
 * @brief 获取迟滞窗口配置
 */
comp_hysteresis_t comp_get_hysteresis(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (comp_hysteresis_t)(p->CR_f.HYS);
}

/**
 * @brief 配置输出滤波
 */
void comp_set_filter(comp_instance_t comp, bool enable,
                     comp_filter_time_t filter_time)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    if (enable) {
        p->CR_f.FLTEN = 1U;
        p->CR_f.FLTTIME = (uint32_t)filter_time;
    } else {
        p->CR_f.FLTEN = 0U;
        p->CR_f.FLTTIME = 0U;
    }
}

/**
 * @brief 获取滤波配置
 */
bool comp_get_filter(comp_instance_t comp, comp_filter_time_t *filter_time)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    if (filter_time != NULL) {
        *filter_time = (comp_filter_time_t)(p->CR_f.FLTTIME);
    }
    return (p->CR_f.FLTEN != 0U);
}

/**
 * @brief 设置消隐源
 */
void comp_set_blanking(comp_instance_t comp, comp_blank_sel_t blank_sel)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    p->CR_f.BLANKSEL = (uint32_t)blank_sel;
}

/**
 * @brief 获取消隐源配置
 */
comp_blank_sel_t comp_get_blanking(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (comp_blank_sel_t)(p->CR_f.BLANKSEL);
}

/**
 * @brief 配置基准分压
 */
void comp_set_vref_divider(comp_instance_t comp, bool enable,
                           comp_vref_sel_t vref_sel, uint8_t vref_cfg)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    if (enable) {
        p->CR_f.CRV_SEL = (uint32_t)vref_sel;
        p->CR_f.CRV_CFG = (uint32_t)vref_cfg;
        p->CR_f.CRV_EN = 1U;
    } else {
        p->CR_f.CRV_EN = 0U;
    }
}

/**
 * @brief 获取分压系数
 */
uint8_t comp_get_vref_cfg(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (uint8_t)(p->CR_f.CRV_CFG);
}

/**
 * @brief 获取比较器滤波输出状态
 */
bool comp_get_output(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (p->SR_f.VCOUT1 != 0U);
}

/**
 * @brief 获取比较器原始输出状态
 */
bool comp_get_output_raw(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (p->SR_f.VCOUT1_ORG != 0U);
}

/**
 * @brief 检查 COMP 是否已锁定
 */
bool comp_is_locked(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (p->CR_f.LOCK != 0U);
}

/**
 * @brief 检查 COMP 是否已使能
 */
bool comp_is_enabled(comp_instance_t comp)
{
    COMP_TypeDef *p = comp_get_instance(comp);
    return (p->CR_f.EN != 0U);
}

/**
 * @brief 计算内部分压电压值
 */
uint32_t comp_calculate_vref_divider_mv(comp_vref_sel_t vref_sel,
                                        uint32_t vref_mv, uint8_t vref_cfg)
{
    (void)vref_sel;

    if (vref_cfg > COMP_VREF_CFG_MAX) {
        return 0U;
    }

    // V_inm = (vref_cfg + 1) / 20 × vref_mv
    return ((uint32_t)vref_cfg + 1U) * vref_mv / COMP_VREF_DIV_BASE;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief COMP1 中断服务程序
 */
void COMP1_IRQHandler(void)
{
    if (comp_state[COMP_1].callback != NULL) {
        comp_state[COMP_1].callback(COMP_1);
    }
}
