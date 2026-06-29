/**
 * @file iwdt.c
 * @brief ACM32P4 独立看门狗（IWDT）驱动实现
 *
 * 本文件实现了ACM32P4芯片的独立看门狗功能，提供了以下接口：
 * - 复位模式看门狗：计数器减到 0x000 时产生系统复位
 * - 窗口看门狗：计数器在窗口外重装载时产生复位
 * - 唤醒功能：计数器减到唤醒值时产生 WAKEUP 信号
 * - 寄存器写保护机制
 * - 灵活的参数配置（预分频、重装载值、窗口值、唤醒值）
 * - 喂狗操作和状态查询
 * - 唤醒中断管理和回调机制
 *
 * @note IWDT 只有一个实例，基地址 0x40003000
 * @note 时钟源：RC32K，由 VDD 供电，STOP/STANDBY 模式下可工作
 * @note 复位信号需要 RCC_RCR.IWDTRSTEN 置位才能生效
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/iwdt.h"
#include "hardware/exti.h"
#include "device/acm32p4xx.h"

//===========================================
// 内部宏定义
//===========================================

#define IWDT_PRESCALER_MASK   0x7U    ///< PR 位域掩码
#define IWDT_RELOAD_MASK      0xFFFU  ///< PL 位域掩码
#define IWDT_WINDOW_MASK      0xFFFU  ///< WIN 位域掩码
#define IWDT_WAKEUP_MASK      0xFFFU  ///< WUT 位域掩码

//===========================================
// 内部状态变量
//===========================================

/**
 * @brief IWDT 内部状态
 */
static struct {
    iwdt_callback_t callback;   ///< 用户注册的唤醒中断回调函数
    bool            enabled;    ///< IWDT 使能状态
} iwdt_state = {
    .callback = NULL,
    .enabled = false,
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 检查 prescaler 参数是否有效
 *
 * @param[in] prescaler 预分频值
 * @return true  有效
 * @return false 无效
 */
static inline bool iwdt_is_valid_prescaler(iwdt_prescaler_t prescaler)
{
    return (prescaler <= IWDT_PRESCALER_256);
}

/**
 * @brief 检查 reload 参数是否有效
 *
 * @param[in] reload 重装载值
 * @return true  有效
 * @return false 无效
 */
static inline bool iwdt_is_valid_reload(uint16_t reload)
{
    return (reload <= IWDT_RELOAD_MAX);
}

/**
 * @brief 检查 window 参数是否有效
 *
 * @param[in] window 窗口值
 * @return true  有效
 * @return false 无效
 */
static inline bool iwdt_is_valid_window(uint16_t window)
{
    return (window <= IWDT_RELOAD_MAX);
}

/**
 * @brief 检查 wakeup 参数是否有效
 *
 * @param[in] wakeup 唤醒值
 * @return true  有效
 * @return false 无效
 */
static inline bool iwdt_is_valid_wakeup(uint16_t wakeup)
{
    return (wakeup <= IWDT_RELOAD_MAX);
}

/**
 * @brief 等待指定状态标志清零
 *
 * @param[in] flag     要等待的标志
 * @param[in] timeout  超时循环次数
 * @return true  标志已清零
 * @return false 超时
 */
static bool iwdt_wait_flag_clear(uint32_t flag, uint32_t timeout)
{
    while ((IWDT->SR & flag) != 0U) {
        if (--timeout == 0U) {
            return false;
        }
    }
    return true;
}

/**
 * @brief 获取预分频分频系数
 *
 * @param[in] prescaler 预分频枚举值
 * @return 实际分频系数（4/8/16/32/64/128/256）
 */
static inline uint32_t iwdt_get_prescaler_div(iwdt_prescaler_t prescaler)
{
    // 分频系数 = 4 << prescaler（4/8/16/32/64/128/256）
    return 4U << (uint32_t)prescaler;
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 IWDT 为复位模式并启动
 */
void iwdt_init_reset(iwdt_prescaler_t prescaler, uint16_t reload)
{
    // 使能 IWDT
    IWDT->CMDR = IWDT_CMD_ENABLE;

    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待预分频更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_PVU, IWDT_UPDATE_TIMEOUT);

    // 配置预分频
    IWDT->PR = (uint32_t)prescaler;

    // 等待重装载更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_RVU, IWDT_UPDATE_TIMEOUT);

    // 配置重装载值
    IWDT->RLR = (uint32_t)reload;

    // 等待唤醒值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WTU, IWDT_UPDATE_TIMEOUT);

    // 禁用唤醒功能（写入 0xFFF）
    IWDT->WUTR = IWDT_RELOAD_MAX;

    // 等待窗口值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WVU, IWDT_UPDATE_TIMEOUT);

    // 禁用窗口功能（写入 0xFFF）
    IWDT->WINR = IWDT_RELOAD_MAX;

    // 喂狗启动计数
    IWDT->CMDR = IWDT_CMD_RELOAD;
    iwdt_state.enabled = true;
}

/**
 * @brief 快速初始化 IWDT 为窗口模式并启动
 */
void iwdt_init_window(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t window)
{
    // 使能 IWDT
    IWDT->CMDR = IWDT_CMD_ENABLE;

    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待预分频更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_PVU, IWDT_UPDATE_TIMEOUT);

    // 配置预分频
    IWDT->PR = (uint32_t)prescaler;

    // 等待重装载更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_RVU, IWDT_UPDATE_TIMEOUT);

    // 配置重装载值
    IWDT->RLR = (uint32_t)reload;

    // 等待唤醒值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WTU, IWDT_UPDATE_TIMEOUT);

    // 禁用唤醒功能
    IWDT->WUTR = IWDT_RELOAD_MAX;

    // 配置窗口值（自动触发计数器重装载）
    if (window < reload) {
        IWDT->WINR = (uint32_t)window;
    } else {
        IWDT->WINR = IWDT_RELOAD_MAX;
    }

    // 等待窗口值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WVU, IWDT_UPDATE_TIMEOUT);

    // 喂狗启动计数
    IWDT->CMDR = IWDT_CMD_RELOAD;
    iwdt_state.enabled = true;
}

/**
 * @brief 快速初始化 IWDT 为唤醒模式并启动
 */
void iwdt_init_wakeup(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t wakeup)
{
    // 使能 IWDT
    IWDT->CMDR = IWDT_CMD_ENABLE;

    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待预分频更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_PVU, IWDT_UPDATE_TIMEOUT);

    // 配置预分频
    IWDT->PR = (uint32_t)prescaler;

    // 等待重装载更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_RVU, IWDT_UPDATE_TIMEOUT);

    // 配置重装载值
    IWDT->RLR = (uint32_t)reload;

    // 等待窗口值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WVU, IWDT_UPDATE_TIMEOUT);

    // 禁用窗口功能
    IWDT->WINR = IWDT_RELOAD_MAX;

    // 等待唤醒值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WTU, IWDT_UPDATE_TIMEOUT);

    // 配置唤醒值
    if (wakeup < reload) {
        IWDT->WUTR = (uint32_t)wakeup;
    } else {
        IWDT->WUTR = IWDT_RELOAD_MAX;
    }

    // 使能唤醒功能
    if (wakeup < reload) {
        IWDT->CMDR = IWDT_CMD_WAKEUP_ENABLE;
    }

    // 喂狗启动计数
    IWDT->CMDR = IWDT_CMD_RELOAD;
    iwdt_state.enabled = true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 IWDT 并启动
 */
bool iwdt_config(const iwdt_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (!iwdt_is_valid_prescaler(config->prescaler)) {
        return false;
    }

    if (!iwdt_is_valid_reload(config->reload)) {
        return false;
    }

    if (!iwdt_is_valid_window(config->window)) {
        return false;
    }

    if (!iwdt_is_valid_wakeup(config->wakeup)) {
        return false;
    }

    if (config->wakeup_enable && config->wakeup >= config->reload) {
        return false;  // 唤醒使能时 wakeup 必须小于 reload
    }

    // 使能 IWDT
    IWDT->CMDR = IWDT_CMD_ENABLE;

    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待并配置预分频
    if (!iwdt_wait_flag_clear(IWDT_FLAG_PVU, IWDT_UPDATE_TIMEOUT)) {
        return false;
    }
    IWDT->PR = (uint32_t)config->prescaler;

    // 等待并配置重装载值
    if (!iwdt_wait_flag_clear(IWDT_FLAG_RVU, IWDT_UPDATE_TIMEOUT)) {
        return false;
    }
    IWDT->RLR = (uint32_t)config->reload;

    // 等待并配置唤醒值
    if (!iwdt_wait_flag_clear(IWDT_FLAG_WTU, IWDT_UPDATE_TIMEOUT)) {
        return false;
    }
    if (config->wakeup < config->reload) {
        IWDT->WUTR = (uint32_t)config->wakeup;
    } else {
        IWDT->WUTR = IWDT_RELOAD_MAX;
    }

    // 配置窗口值（自动触发计数器重装载）
    if (config->window < config->reload) {
        IWDT->WINR = (uint32_t)config->window;
    } else {
        IWDT->WINR = IWDT_RELOAD_MAX;
    }

    // 使能唤醒功能
    if (config->wakeup_enable && config->wakeup < config->reload) {
        IWDT->CMDR = IWDT_CMD_WAKEUP_ENABLE;
    }

    // 喂狗启动计数
    IWDT->CMDR = IWDT_CMD_RELOAD;
    iwdt_state.enabled = true;

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 注册 IWDT 唤醒中断回调函数
 */
void iwdt_register_callback(iwdt_callback_t callback)
{
    iwdt_state.callback = callback;
}

/**
 * @brief 使能 IWDT 唤醒功能
 */
void iwdt_wakeup_enable(void)
{
    IWDT->CMDR = IWDT_CMD_WAKEUP_ENABLE;
}

/**
 * @brief 禁止 IWDT 唤醒功能
 */
void iwdt_wakeup_disable(void)
{
    IWDT->CMDR = IWDT_CMD_WAKEUP_DISABLE;
}

/**
 * @brief 关闭 IWDT 寄存器写保护（打开写权限）
 */
void iwdt_enable_write_access(void)
{
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能独立看门狗
 */
void iwdt_enable(void)
{
    IWDT->CMDR = IWDT_CMD_ENABLE;
    iwdt_state.enabled = true;
}

/**
 * @brief 禁止独立看门狗
 */
void iwdt_disable(void)
{
    IWDT->CMDR = IWDT_CMD_DISABLE;
    iwdt_state.enabled = false;
}

/**
 * @brief 查询 IWDT 当前是否处于使能状态
 */
bool iwdt_is_enabled(void)
{
    return iwdt_state.enabled;
}

/**
 * @brief 喂狗（重装载计数器）
 */
void iwdt_feed(void)
{
    IWDT->CMDR = IWDT_CMD_RELOAD;
}

/**
 * @brief 设置预分频值
 */
void iwdt_set_prescaler(iwdt_prescaler_t prescaler)
{
    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待预分频更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_PVU, IWDT_UPDATE_TIMEOUT);

    // 配置预分频
    IWDT->PR = (uint32_t)prescaler;
}

/**
 * @brief 获取预分频值
 */
iwdt_prescaler_t iwdt_get_prescaler(void)
{
    return (iwdt_prescaler_t)(IWDT->PR & IWDT_PRESCALER_MASK);
}

/**
 * @brief 设置重装载值
 */
void iwdt_set_reload(uint16_t reload)
{
    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待重装载更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_RVU, IWDT_UPDATE_TIMEOUT);

    // 配置重装载值
    IWDT->RLR = (uint32_t)reload;
}

/**
 * @brief 获取重装载值
 */
uint16_t iwdt_get_reload(void)
{
    return (uint16_t)(IWDT->RLR & IWDT_RELOAD_MASK);
}

/**
 * @brief 设置窗口值
 */
void iwdt_set_window(uint16_t window)
{
    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待窗口值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WVU, IWDT_UPDATE_TIMEOUT);

    // 配置窗口值
    IWDT->WINR = (uint32_t)window;
}

/**
 * @brief 获取窗口值
 */
uint16_t iwdt_get_window(void)
{
    return (uint16_t)(IWDT->WINR & IWDT_WINDOW_MASK);
}

/**
 * @brief 设置唤醒值
 */
void iwdt_set_wakeup(uint16_t wakeup)
{
    // 打开写保护
    IWDT->CMDR = IWDT_CMD_WRITE_ENABLE;

    // 等待唤醒值更新完成
    iwdt_wait_flag_clear(IWDT_FLAG_WTU, IWDT_UPDATE_TIMEOUT);

    // 配置唤醒值
    IWDT->WUTR = (uint32_t)wakeup;
}

/**
 * @brief 获取唤醒值
 */
uint16_t iwdt_get_wakeup(void)
{
    return (uint16_t)(IWDT->WUTR & IWDT_WAKEUP_MASK);
}

/**
 * @brief 获取 IWDT 状态标志
 */
uint32_t iwdt_get_status(void)
{
    return IWDT->SR;
}

/**
 * @brief 检查指定状态标志是否置位
 */
bool iwdt_is_flag_set(iwdt_flag_t flag)
{
    return ((IWDT->SR & (uint32_t)flag) != 0U);
}

/**
 * @brief 等待所有状态标志清零
 */
bool iwdt_wait_ready(uint32_t timeout)
{
    return iwdt_wait_flag_clear(IWDT_FLAG_PVU | IWDT_FLAG_RVU | IWDT_FLAG_WVU
                                | IWDT_FLAG_WTU | IWDT_FLAG_RLF, timeout);
}

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 计算 IWDT 超时时间（毫秒）
 */
uint32_t iwdt_calculate_timeout_ms(iwdt_prescaler_t prescaler, uint16_t reload)
{
    // t = (reload + 1) × prescaler_div × 1000 / 32000
    uint32_t prescaler_div = iwdt_get_prescaler_div(prescaler);
    uint32_t count = (uint32_t)reload + 1U;

    return (count * prescaler_div * 1000U) / IWDT_RCL_FREQ_HZ;
}

/**
 * @brief 根据目标超时时间计算重装载值
 */
uint16_t iwdt_calculate_reload(iwdt_prescaler_t prescaler, uint32_t timeout_ms)
{
    // reload = timeout_ms × 32000 / (prescaler_div × 1000) - 1
    uint32_t prescaler_div = iwdt_get_prescaler_div(prescaler);
    uint32_t denominator = prescaler_div * 1000U;

    if (denominator == 0U) {
        return 0U;
    }

    uint64_t numerator = (uint64_t)timeout_ms * IWDT_RCL_FREQ_HZ;
    uint64_t result = numerator / denominator;

    if (result == 0U) {
        return 0U;
    }

    result -= 1U;

    if (result > IWDT_RELOAD_MAX) {
        return IWDT_RELOAD_MAX;
    }

    return (uint16_t)result;
}

/**
 * @brief 计算 IWDT 唤醒时间（毫秒）
 */
uint32_t iwdt_calculate_wakeup_ms(iwdt_prescaler_t prescaler, uint16_t reload, uint16_t wakeup)
{
    if (wakeup >= reload) {
        return 0U;
    }

    // t_wu = (reload - wakeup + 1) × prescaler_div × 1000 / 32000
    uint32_t prescaler_div = iwdt_get_prescaler_div(prescaler);
    uint32_t diff = (uint32_t)reload - (uint32_t)wakeup + 1U;

    return (diff * prescaler_div * 1000U) / IWDT_RCL_FREQ_HZ;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief IWDT 唤醒中断服务程序
 */
void IWDT_WKUP_IRQHandler(void)
{
    // 检查 EXTI pending
    if (exti_get_pending(EXTI_LINE_IWDT)) {
        // 清除 EXTI pending
        exti_clear_pending(EXTI_LINE_IWDT);

        // 调用用户注册的回调函数
        if (iwdt_state.callback != NULL) {
            iwdt_state.callback();
        }
    }
}
