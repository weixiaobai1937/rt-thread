/**
 * @file wdt.c
 * @brief ACM32P4 系统看门狗（WDT）驱动实现
 *
 * 本文件实现了ACM32P4芯片的系统看门狗功能，提供了以下接口：
 * - 复位模式看门狗：计数器减到0时产生系统复位
 * - 中断模式看门狗：计数器减到0时产生中断，超时后复位
 * - 灵活的参数配置（加载值、预分频、中断清除时限）
 * - 喂狗操作和状态查询
 * - 中断管理和回调机制
 *
 * @note WDT 只有一个实例，基地址 0x40002C00
 * @note 复位信号需要 RCC_RCR.WDTRSTEN 置位才能生效
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/wdt.h"
#include "device/acm32p4xx.h"

//===========================================
// 内部宏定义
//===========================================

#define WDT_DIVISOR_MASK  0x7U   ///< DIVISOR 位域掩码
#define WDT_MODE_MASK     0x1U   ///< MODE 位域掩码
#define WDT_INTEN_MASK    0x1U   ///< INTEN 位域掩码

//===========================================
// 内部状态变量
//===========================================

/**
 * @brief WDT 内部状态
 */
static struct {
    wdt_callback_t callback;   ///< 用户注册的中断回调函数
    bool           enabled;    ///< WDT 使能状态
} wdt_state = {
    .callback = NULL,
    .enabled = false,
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 检查 divisor 参数是否有效
 *
 * @param[in] divisor 预分频值
 * @return true  有效
 * @return false 无效
 */
static inline bool wdt_is_valid_divisor(wdt_divisor_t divisor)
{
    return (divisor <= WDT_DIVISOR_128);
}

/**
 * @brief 检查 intclrtime 参数是否有效
 *
 * @param[in] intclrtime 中断清除时限值
 * @return true  有效
 * @return false 无效
 */
static inline bool wdt_is_valid_intclrtime(uint16_t intclrtime)
{
    return (intclrtime <= WDT_INTCLRTIME_MAX);
}

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化 WDT 为复位模式并启动
 */
void wdt_init_reset(uint32_t load, wdt_divisor_t divisor)
{
    // 先禁用 WDT，确保配置期间不触发复位
    WDT->CTRL &= ~WDT_CTRL_EN;

    // 配置加载值
    WDT->LOAD = load;

    // 配置控制寄存器：复位模式，禁止中断，设置预分频
    WDT->CTRL &= ~(WDT_CTRL_MODE | WDT_CTRL_INTEN | WDT_CTRL_DIVISOR);
    WDT->CTRL |= ((uint32_t)divisor << WDT_CTRL_DIVISOR_Pos);

    // 使能 WDT
    WDT->CTRL |= WDT_CTRL_EN;
    wdt_state.enabled = true;
}

/**
 * @brief 快速初始化 WDT 为中断模式并启动
 */
void wdt_init_interrupt(uint32_t load, wdt_divisor_t divisor, uint16_t intclrtime)
{
    // 先禁用 WDT，确保配置期间不触发复位
    WDT->CTRL &= ~WDT_CTRL_EN;

    // 配置加载值
    WDT->LOAD = load;

    // 配置中断清除时限
    WDT->INTCLRTIME = intclrtime;

    // 配置控制寄存器：中断模式，使能中断，设置预分频
    WDT->CTRL &= ~(WDT_CTRL_MODE | WDT_CTRL_INTEN | WDT_CTRL_DIVISOR);
    WDT->CTRL |= WDT_CTRL_MODE | WDT_CTRL_INTEN;
    WDT->CTRL |= ((uint32_t)divisor << WDT_CTRL_DIVISOR_Pos);

    // 使能 WDT
    WDT->CTRL |= WDT_CTRL_EN;
    wdt_state.enabled = true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 通过结构体配置 WDT 并启动
 */
bool wdt_config(const wdt_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    if (!wdt_is_valid_divisor(config->divisor)) {
        return false;
    }

    if (config->mode == WDT_MODE_INTERRUPT) {
        if (!wdt_is_valid_intclrtime(config->intclrtime)) {
            return false;
        }
    }

    // 先禁用 WDT，确保配置期间不触发复位
    WDT->CTRL &= ~WDT_CTRL_EN;

    // 配置加载值
    WDT->LOAD = config->load;

    // 清零全部可配置位：DIVISOR, INTEN, MODE, EN
    WDT->CTRL &= ~(WDT_CTRL_DIVISOR | WDT_CTRL_INTEN | WDT_CTRL_MODE);

    // 设置预分频
    WDT->CTRL |= ((uint32_t)config->divisor << WDT_CTRL_DIVISOR_Pos);

    // 设置工作模式
    if (config->mode == WDT_MODE_INTERRUPT) {
        WDT->CTRL |= WDT_CTRL_MODE;

        // 中断模式下设置中断清除时限
        WDT->INTCLRTIME = config->intclrtime;

        // 配置中断使能
        if (config->int_enable) {
            WDT->CTRL |= WDT_CTRL_INTEN;
        }
    }

    // 使能 WDT
    WDT->CTRL |= WDT_CTRL_EN;
    wdt_state.enabled = true;

    return true;
}

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 注册 WDT 中断回调函数
 */
void wdt_register_callback(wdt_callback_t callback)
{
    wdt_state.callback = callback;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 使能看门狗定时器
 */
void wdt_enable(void)
{
    WDT->CTRL |= WDT_CTRL_EN;
    wdt_state.enabled = true;
}

/**
 * @brief 禁用看门狗定时器
 */
void wdt_disable(void)
{
    WDT->CTRL &= ~WDT_CTRL_EN;
    wdt_state.enabled = false;
}

/**
 * @brief 查询 WDT 使能状态
 *
 * @return true  WDT 已使能
 * @return false WDT 已禁用
 *
 * @note 覆盖寄存器：CTRL.EN
 */
bool wdt_is_enabled(void)
{
    return wdt_state.enabled;
}

/**
 * @brief 喂狗（刷新看门狗）
 */
void wdt_feed(void)
{
    WDT->FEED = WDT_FEED_MAGIC;
}

/**
 * @brief 获取当前计数值
 */
uint32_t wdt_get_counter(void)
{
    return WDT->COUNT;
}

/**
 * @brief 获取当前加载值
 */
uint32_t wdt_get_load(void)
{
    return WDT->LOAD;
}

/**
 * @brief 获取原始中断状态
 */
bool wdt_get_ris(void)
{
    return ((WDT->RIS & WDT_RIS_WDTRIS) != 0U);
}

/**
 * @brief 设置加载值
 */
void wdt_set_load(uint32_t load)
{
    WDT->LOAD = load;
}

/**
 * @brief 设置时钟预分频
 */
void wdt_set_divisor(wdt_divisor_t divisor)
{
    WDT->CTRL &= ~WDT_CTRL_DIVISOR;
    WDT->CTRL |= ((uint32_t)divisor << WDT_CTRL_DIVISOR_Pos);
}

/**
 * @brief 设置工作模式
 */
void wdt_set_mode(wdt_mode_t mode)
{
    if (mode == WDT_MODE_INTERRUPT) {
        WDT->CTRL |= WDT_CTRL_MODE;
    } else {
        WDT->CTRL &= ~WDT_CTRL_MODE;
    }
}

/**
 * @brief 设置中断清除时限
 */
void wdt_set_intclrtime(uint16_t intclrtime)
{
    WDT->INTCLRTIME = (uint32_t)intclrtime;
}

/**
 * @brief 使能或禁用 WDT 中断
 */
void wdt_set_interrupt(bool enable)
{
    if (enable) {
        WDT->CTRL |= WDT_CTRL_INTEN;
    } else {
        WDT->CTRL &= ~WDT_CTRL_INTEN;
    }
}

//===========================================
// 辅助计算函数
//===========================================

/**
 * @brief 计算 WDT 超时时间
 */
uint64_t wdt_calculate_timeout_us(uint32_t pclk_hz, uint32_t load, wdt_divisor_t divisor)
{
    if (pclk_hz == 0U) {
        return 0U;
    }

    // T_WDT = (1 / pclk_hz) × 2^divisor × (load + 1)
    // 使用微秒为单位，先乘后除避免整除精度丢失
    uint64_t prescaler = 1ULL << (uint32_t)divisor;
    uint64_t count     = (uint64_t)load + 1ULL;

    return count * prescaler * 1000000ULL / (uint64_t)pclk_hz;
}

/**
 * @brief 根据目标超时时间计算加载值
 */
uint32_t wdt_calculate_load(uint32_t pclk_hz, uint64_t timeout_us, wdt_divisor_t divisor)
{
    if (pclk_hz == 0U) {
        return 0U;
    }

    // load = (timeout_us × pclk_hz) / (2^divisor × 10^6) - 1
    uint64_t numerator = timeout_us * (uint64_t)pclk_hz;
    uint64_t denominator = (1ULL << (uint32_t)divisor) * 1000000ULL;

    if (denominator == 0U) {
        return 0U;
    }

    uint64_t load = numerator / denominator;

    if (load == 0U) {
        return 0U;
    }

    // load - 1，因为实际计数 = load + 1
    load -= 1ULL;

    if (load > WDT_LOAD_MAX) {
        return WDT_LOAD_MAX;
    }

    return (uint32_t)load;
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief WDT 中断服务程序
 */
void WDT_IRQHandler(void)
{
    if ((WDT->RIS & WDT_RIS_WDTRIS) == 0U) {
        return;
    }

    if (wdt_state.callback != NULL) {
        wdt_state.callback();
    }
}
