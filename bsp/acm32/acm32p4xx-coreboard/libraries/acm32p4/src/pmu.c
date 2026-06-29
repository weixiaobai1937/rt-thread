/**
 * @file pmu.c
 * @brief ACM32P4 电源管理单元（PMU）驱动实现
 * 
 * 本文件实现了ACM32P4芯片的PMU功能，提供了以下接口：
 * - 低功耗模式控制（SLEEP、STOP、STANDBY）
 * - 电源监控（BOR、LVD）
 * - 唤醒源管理
 * - 待机域IO控制
 * - 备份SRAM管理
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

//===========================================
// 头文件包含
//===========================================

#include <stddef.h>
#include <assert.h>
#include "device/acm32p4xx.h"
#include "hardware/system.h"
#include "hardware/pmu.h"

//===========================================
// 内部宏定义
//===========================================

// 注：PMU寄存器基地址和结构体定义在 device/acm32p4xx.h 中
// PMU 寄存器访问：PMU->CTRL0, PMU->CTRL1, PMU->CTRL2, PMU->CTRL3, PMU->SR, PMU->STCLR, PMU->IOSEL

// PMU_CTRL0位定义
#define PMU_CTRL0_LPMS_POS         0U
#define PMU_CTRL0_LPMS_MASK        (1U << PMU_CTRL0_LPMS_POS)
#define PMU_CTRL0_RCHPDEN_POS      4U
#define PMU_CTRL0_RCHPDEN_MASK     (1U << PMU_CTRL0_RCHPDEN_POS)
#define PMU_CTRL0_RCH_DIV_EN_POS   5U
#define PMU_CTRL0_RCH_DIV_EN_MASK  (1U << PMU_CTRL0_RCH_DIV_EN_POS)
#define PMU_CTRL0_RTC_WE_POS       7U
#define PMU_CTRL0_RTC_WE_MASK      (1U << PMU_CTRL0_RTC_WE_POS)
#define PMU_CTRL0_MLDO12_LV_POS    8U
#define PMU_CTRL0_MLDO12_LV_MASK   (3U << PMU_CTRL0_MLDO12_LV_POS)
#define PMU_CTRL0_MLDO12_LOWP_POS  10U
#define PMU_CTRL0_MLDO12_LOWP_MASK (1U << PMU_CTRL0_MLDO12_LOWP_POS)
#define PMU_CTRL0_LPLDO12_LV_POS   12U
#define PMU_CTRL0_LPLDO12_LV_MASK  (7U << PMU_CTRL0_LPLDO12_LV_POS)
#define PMU_CTRL0_STOP_WPT_POS     16U
#define PMU_CTRL0_STOP_WPT_MASK    (0xFFFU << PMU_CTRL0_STOP_WPT_POS)
#define PMU_CTRL0_BKPRAMSEN_POS    30U
#define PMU_CTRL0_BKPRAMSEN_MASK   (1U << PMU_CTRL0_BKPRAMSEN_POS)
#define PMU_CTRL0_BKPRAMREN_POS    31U
#define PMU_CTRL0_BKPRAMREN_MASK   (1U << PMU_CTRL0_BKPRAMREN_POS)

// PMU_CTRL1位定义
#define PMU_CTRL1_LVDEN_POS        0U
#define PMU_CTRL1_LVDEN_MASK       (1U << PMU_CTRL1_LVDEN_POS)
#define PMU_CTRL1_LVD_SEL_POS      1U
#define PMU_CTRL1_LVD_SEL_MASK     (7U << PMU_CTRL1_LVD_SEL_POS)
#define PMU_CTRL1_LVD_FLTEN_POS    8U
#define PMU_CTRL1_LVD_FLTEN_MASK   (1U << PMU_CTRL1_LVD_FLTEN_POS)
#define PMU_CTRL1_FLT_TIME_POS     9U
#define PMU_CTRL1_FLT_TIME_MASK    (7U << PMU_CTRL1_FLT_TIME_POS)
#define PMU_CTRL1_LVD_FILTER_POS   14U
#define PMU_CTRL1_LVD_FILTER_MASK  (1U << PMU_CTRL1_LVD_FILTER_POS)
#define PMU_CTRL1_LVD_VALUE_POS    15U
#define PMU_CTRL1_LVD_VALUE_MASK   (1U << PMU_CTRL1_LVD_VALUE_POS)

// PMU_CTRL2位定义
#define PMU_CTRL2_EWUPX_POS        0U
#define PMU_CTRL2_EWUPX_MASK       (0xFFU << PMU_CTRL2_EWUPX_POS)
#define PMU_CTRL2_WUXFILEN_POS     8U
#define PMU_CTRL2_WUXFILEN_MASK    (0xFFU << PMU_CTRL2_WUXFILEN_POS)
#define PMU_CTRL2_STDBY_WPT_POS    16U
#define PMU_CTRL2_STDBY_WPT_MASK   (0xFU << PMU_CTRL2_STDBY_WPT_POS)
#define PMU_CTRL2_BORRST_EN_POS    20U
#define PMU_CTRL2_BORRST_EN_MASK   (1U << PMU_CTRL2_BORRST_EN_POS)
#define PMU_CTRL2_BOR_EN_POS       24U
#define PMU_CTRL2_BOR_EN_MASK      (1U << PMU_CTRL2_BOR_EN_POS)
#define PMU_CTRL2_BOR_CFG_POS      26U
#define PMU_CTRL2_BOR_CFG_MASK     (3U << PMU_CTRL2_BOR_CFG_POS)

// PMU_CTRL3位定义
#define PMU_CTRL3_WUPOLX_POS       0U
#define PMU_CTRL3_WUPOLX_MASK      (0xFFU << PMU_CTRL3_WUPOLX_POS)

// PMU_SR位定义
#define PMU_SR_WUPF1_POS           0U
#define PMU_SR_WUPF2_POS           1U
#define PMU_SR_WUPF3_POS           2U
#define PMU_SR_WUPF4_POS           3U
#define PMU_SR_WUPF5_POS           4U
#define PMU_SR_WUPFX_MASK          0xFFU
#define PMU_SR_SBF_POS             8U
#define PMU_SR_SBF_MASK            (1U << PMU_SR_SBF_POS)
#define PMU_SR_RTCWUF_POS          12U
#define PMU_SR_RTCWUF_MASK         (1U << PMU_SR_RTCWUF_POS)
#define PMU_SR_RSTWUF_POS          13U
#define PMU_SR_RSTWUF_MASK         (1U << PMU_SR_RSTWUF_POS)
#define PMU_SR_IWDTWUF_POS         14U
#define PMU_SR_IWDTWUF_MASK        (1U << PMU_SR_IWDTWUF_POS)
#define PMU_SR_BORWUF_POS          15U
#define PMU_SR_BORWUF_MASK         (1U << PMU_SR_BORWUF_POS)
#define PMU_SR_BORN_POS            16U
#define PMU_SR_BORN_MASK           (1U << PMU_SR_BORN_POS)

// PMU_STCLR位定义
#define PMU_STCLR_CWUFX_POS        0U
#define PMU_STCLR_CWUFX_MASK       0xFFU
#define PMU_STCLR_CSBF_POS         8U
#define PMU_STCLR_CSBF_MASK        (1U << PMU_STCLR_CSBF_POS)
#define PMU_STCLR_CRTCWUF_POS      12U
#define PMU_STCLR_CRTCWUF_MASK     (1U << PMU_STCLR_CRTCWUF_POS)
#define PMU_STCLR_CRSTWUF_POS      13U
#define PMU_STCLR_CRSTWUF_MASK     (1U << PMU_STCLR_CRSTWUF_POS)
#define PMU_STCLR_CIWDTWUF_POS     14U
#define PMU_STCLR_CIWDTWUF_MASK    (1U << PMU_STCLR_CIWDTWUF_POS)
#define PMU_STCLR_CBORWUF_POS      15U
#define PMU_STCLR_CBORWUF_MASK     (1U << PMU_STCLR_CBORWUF_POS)

// PMU_IOSEL位定义
#define PMU_IOSEL_PC13_SEL_POS     0U
#define PMU_IOSEL_PC13_SEL_MASK    (3U << PMU_IOSEL_PC13_SEL_POS)
#define PMU_IOSEL_PC14_SEL_POS     2U
#define PMU_IOSEL_PC14_SEL_MASK    (3U << PMU_IOSEL_PC14_SEL_POS)
#define PMU_IOSEL_PC15_SEL_POS     4U
#define PMU_IOSEL_PC15_SEL_MASK    (3U << PMU_IOSEL_PC15_SEL_POS)
#define PMU_IOSEL_PC13_VALUE_POS   8U
#define PMU_IOSEL_PC13_VALUE_MASK  (1U << PMU_IOSEL_PC13_VALUE_POS)
#define PMU_IOSEL_PC14_VALUE_POS   9U
#define PMU_IOSEL_PC14_VALUE_MASK  (1U << PMU_IOSEL_PC14_VALUE_POS)
#define PMU_IOSEL_PC15_VALUE_POS   10U
#define PMU_IOSEL_PC15_VALUE_MASK  (1U << PMU_IOSEL_PC15_VALUE_POS)

// 默认配置值
#define PMU_DEFAULT_STOP_WPT       0x3C0U  ///< 默认STOP唤醒等待时间
#define PMU_DEFAULT_STANDBY_WPT    0xAU    ///< 默认STANDBY唤醒等待时间

// WKUP引脚有效范围（8位掩码，对应8个WAKEUP引脚）
#define PMU_WAKEUP_PIN_MASK        0xFFU

// SCB寄存器位定义
#define SCB_SCR_SLEEPDEEP_POS      2U
#define SCB_SCR_SLEEPDEEP_MASK     (1U << SCB_SCR_SLEEPDEEP_POS)

//===========================================
// 内部类型定义
//===========================================

// (无额外内部类型定义)

//===========================================
// 内部状态变量
//===========================================

// (无全局状态变量)

//===========================================
// 第1层: 快速初始化 / 简单控制 API
//===========================================

/**
 * @brief 进入SLEEP模式（最简单）
 */
void pmu_enter_sleep(void)
{
    // 清除NVIC中断挂起和SysTick中断
    NVIC->ICPR[0] = 0xFFFFFFFFU;
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    
    // 清除SLEEPDEEP位（SLEEP模式）
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_MASK;
    
    // 执行WFI指令进入SLEEP模式
    __WFI();
}

/**
 * @brief 进入STOP模式（默认配置）
 */
void pmu_enter_stop_simple(void)
{
    pmu_stop_config_t config;
    pmu_get_default_stop_config(&config);
    pmu_enter_stop(&config);
}

/**
 * @brief 进入STANDBY模式（默认配置）
 */
void pmu_enter_standby_simple(uint32_t wakeup_pins)
{
    assert((wakeup_pins & ~PMU_WAKEUP_PIN_MASK) == 0);
    
    pmu_standby_config_t config;
    pmu_get_default_standby_config(&config);
    config.wakeup_pins = wakeup_pins;
    pmu_enter_standby(&config);
}

//===========================================
// 第2层: 基础配置 API
//===========================================

/**
 * @brief 配置并进入STOP模式
 */
void pmu_enter_stop(const pmu_stop_config_t *config)
{
    assert(config != NULL);
    
    uint32_t ctrl0 = PMU->CTRL0;
    
    // 1. 配置RCH自动关闭
    if (config->rch_auto_off) {
        ctrl0 |= PMU_CTRL0_RCHPDEN_MASK;
    } else {
        ctrl0 &= ~PMU_CTRL0_RCHPDEN_MASK;
    }
    
    // 2. 配置RCH 16分频
    if (config->rch_div16_mode) {
        ctrl0 |= PMU_CTRL0_RCH_DIV_EN_MASK;
    } else {
        ctrl0 &= ~PMU_CTRL0_RCH_DIV_EN_MASK;
    }
    
    // 3. 配置MLDO12电压
    ctrl0 &= ~PMU_CTRL0_MLDO12_LV_MASK;
    ctrl0 |= ((uint32_t)config->mldo_voltage << PMU_CTRL0_MLDO12_LV_POS);
    
    // 4. 配置MLDO12低功耗模式
    if (config->mldo_lowpower) {
        ctrl0 |= PMU_CTRL0_MLDO12_LOWP_MASK;
    } else {
        ctrl0 &= ~PMU_CTRL0_MLDO12_LOWP_MASK;
    }
    
    // 5. 配置唤醒等待时间（STOP_WPT 12位，max 4095）
    ctrl0 &= ~PMU_CTRL0_STOP_WPT_MASK;
    ctrl0 |= (((uint32_t)config->wakeup_wait_cycles & 0xFFFU) << PMU_CTRL0_STOP_WPT_POS);
    
    // 6. 设置LPMS=0（STOP模式）
    ctrl0 &= ~PMU_CTRL0_LPMS_MASK;
    
    PMU->CTRL0 = ctrl0;
    
    // 7. 清除NVIC中断挂起和SysTick中断
    NVIC->ICPR[0] = 0xFFFFFFFFU;
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    
    // 8. 设置SLEEPDEEP位
    SCB->SCR |= SCB_SCR_SLEEPDEEP_MASK;
    
    // 9. 执行WFI进入STOP模式
    __WFI();
    
    // 唤醒后清除SLEEPDEEP位，恢复浅睡眠模式
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_MASK;
}

/**
 * @brief 配置并进入STANDBY模式
 */
void pmu_enter_standby(const pmu_standby_config_t *config)
{
    assert(config != NULL);
    
    uint32_t ctrl0 = PMU->CTRL0;
    uint32_t ctrl2 = PMU->CTRL2;
    uint32_t ctrl3 = PMU->CTRL3;
    
    // 1. 配置WAKEUP引脚使能
    ctrl2 &= ~PMU_CTRL2_EWUPX_MASK;
    ctrl2 |= ((config->wakeup_pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_EWUPX_POS);
    
    // 2. 配置唤醒极性
    ctrl3 &= ~PMU_CTRL3_WUPOLX_MASK;
    ctrl3 |= ((config->wakeup_polarity & PMU_WAKEUP_PIN_MASK) << PMU_CTRL3_WUPOLX_POS);
    
    // 3. 配置滤波
    ctrl2 &= ~PMU_CTRL2_WUXFILEN_MASK;
    ctrl2 |= ((config->wakeup_filter & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_WUXFILEN_POS);
    
    // 4. 配置唤醒延时（STDBY_WPT 4位，max 15）
    ctrl2 &= ~PMU_CTRL2_STDBY_WPT_MASK;
    ctrl2 |= (((uint32_t)config->wakeup_delay_rtc & 0xFU) << PMU_CTRL2_STDBY_WPT_POS);
    
    PMU->CTRL2 = ctrl2;
    PMU->CTRL3 = ctrl3;
    
    // 5. 配置备份SRAM
    if (config->bkp_sram_retain) {
        ctrl0 |= PMU_CTRL0_BKPRAMREN_MASK;
        ctrl0 &= ~PMU_CTRL0_BKPRAMSEN_MASK;
    } else {
        ctrl0 |= PMU_CTRL0_BKPRAMSEN_MASK;
        ctrl0 &= ~PMU_CTRL0_BKPRAMREN_MASK;
    }
    
    // 6. 配置LPLDO12电压（STANDBY模式待机域LDO）
    ctrl0 &= ~PMU_CTRL0_LPLDO12_LV_MASK;
    ctrl0 |= ((uint32_t)config->lpldo_voltage << PMU_CTRL0_LPLDO12_LV_POS);
    
    // 7. 设置LPMS=1（STANDBY模式）
    ctrl0 |= PMU_CTRL0_LPMS_MASK;
    
    PMU->CTRL0 = ctrl0;
    
    // 8. 清除NVIC中断挂起和SysTick中断
    NVIC->ICPR[0] = 0xFFFFFFFFU;
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
    
    // 9. 设置SLEEPDEEP位
    SCB->SCR |= SCB_SCR_SLEEPDEEP_MASK;
    
    // 10. 执行WFI进入STANDBY模式
    __WFI();
    
    // 注意：从STANDBY唤醒后系统会复位，不会执行到这里
}

/**
 * @brief 获取默认STOP配置
 */
void pmu_get_default_stop_config(pmu_stop_config_t *config)
{
    assert(config != NULL);
    
    config->rch_auto_off = false;
    config->rch_div16_mode = false;
    config->mldo_lowpower = false;
    config->mldo_voltage = PMU_MLDO_1V2;
    config->wakeup_wait_cycles = PMU_DEFAULT_STOP_WPT;
}

/**
 * @brief 获取默认STANDBY配置
 */
void pmu_get_default_standby_config(pmu_standby_config_t *config)
{
    assert(config != NULL);
    
    config->wakeup_pins = 0;
    config->wakeup_polarity = 0;
    config->wakeup_filter = 0;
    config->wakeup_delay_rtc = PMU_DEFAULT_STANDBY_WPT;
    config->lpldo_voltage = PMU_LPLDO_1V1;
    config->bkp_sram_retain = false;
}

/**
 * @brief 配置WAKEUP引脚
 */
void pmu_wakeup_pin_configure(uint32_t pins, bool high_active, bool filter_enable)
{
    assert((pins & ~PMU_WAKEUP_PIN_MASK) == 0);
    
    uint32_t ctrl2 = PMU->CTRL2;
    uint32_t ctrl3 = PMU->CTRL3;
    
    // 配置极性（WUPOLX）：0=高电平唤醒，1=低电平唤醒
    if (high_active) {
        ctrl3 &= ~((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL3_WUPOLX_POS);
    } else {
        ctrl3 |= ((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL3_WUPOLX_POS);
    }
    
    // 配置滤波（WUXFILEN）
    if (filter_enable) {
        ctrl2 |= ((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_WUXFILEN_POS);
    } else {
        ctrl2 &= ~((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_WUXFILEN_POS);
    }
    
    PMU->CTRL2 = ctrl2;
    PMU->CTRL3 = ctrl3;
}

/**
 * @brief 使能WAKEUP引脚
 */
void pmu_wakeup_pin_enable(uint32_t pins)
{
    assert((pins & ~PMU_WAKEUP_PIN_MASK) == 0);
    PMU->CTRL2 |= ((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_EWUPX_POS);
}

/**
 * @brief 禁用WAKEUP引脚
 */
void pmu_wakeup_pin_disable(uint32_t pins)
{
    assert((pins & ~PMU_WAKEUP_PIN_MASK) == 0);
    PMU->CTRL2 &= ~((pins & PMU_WAKEUP_PIN_MASK) << PMU_CTRL2_EWUPX_POS);
}

/**
 * @brief 设置STANDBY唤醒延时
 */
void pmu_set_standby_wakeup_delay(uint8_t rtc_cycles)
{
    uint32_t ctrl2 = PMU->CTRL2;
    
    // 参数范围检查：0~15
    if (rtc_cycles > 15) {
        rtc_cycles = 15;
    }
    
    // 清除旧值，设置新值
    ctrl2 &= ~PMU_CTRL2_STDBY_WPT_MASK;
    ctrl2 |= ((uint32_t)rtc_cycles << PMU_CTRL2_STDBY_WPT_POS);
    
    PMU->CTRL2 = ctrl2;
}

//===========================================
// 第3层: 高级功能 API
//===========================================

/**
 * @brief 配置BOR（欠压复位）
 */
void pmu_bor_configure(const pmu_bor_config_t *config)
{
    assert(config != NULL);
    
    uint32_t ctrl2 = PMU->CTRL2;
    
    if (!config->enabled) {
        // 禁用BOR：先禁用BOR复位，再禁用BOR
        ctrl2 &= ~PMU_CTRL2_BORRST_EN_MASK;
        PMU->CTRL2 = ctrl2;
        
        ctrl2 &= ~PMU_CTRL2_BOR_EN_MASK;
        PMU->CTRL2 = ctrl2;
    } else {
        // 使能BOR
        // 1. 配置BOR阈值
        ctrl2 &= ~PMU_CTRL2_BOR_CFG_MASK;
        ctrl2 |= ((uint32_t)config->level << PMU_CTRL2_BOR_CFG_POS);
        
        // 2. 使能BOR
        ctrl2 |= PMU_CTRL2_BOR_EN_MASK;
        PMU->CTRL2 = ctrl2;
        
        // 3. 等待1us稳定
        delay_us(PMU_BOR_STABLE_US);
        
        // 4. 使能BOR复位（如果需要）
        if (config->reset_enabled) {
            ctrl2 = PMU->CTRL2;
            ctrl2 |= PMU_CTRL2_BORRST_EN_MASK;
            PMU->CTRL2 = ctrl2;
        }
    }
}

/**
 * @brief 使能BOR
 */
void pmu_bor_enable(pmu_bor_level_t level, bool reset_enable)
{
    pmu_bor_config_t config = {
        .enabled = true,
        .level = level,
        .reset_enabled = reset_enable,
    };
    
    pmu_bor_configure(&config);
}

/**
 * @brief 禁用BOR
 */
void pmu_bor_disable(void)
{
    uint32_t ctrl2 = PMU->CTRL2;
    
    // 先禁用BOR复位
    ctrl2 &= ~PMU_CTRL2_BORRST_EN_MASK;
    PMU->CTRL2 = ctrl2;
    
    // 再禁用BOR
    ctrl2 &= ~PMU_CTRL2_BOR_EN_MASK;
    PMU->CTRL2 = ctrl2;
}

/**
 * @brief 配置LVD（低压检测）
 */
void pmu_lvd_configure(const pmu_lvd_config_t *config)
{
    assert(config != NULL);
    
    uint32_t ctrl1 = PMU->CTRL1;
    
    if (!config->enabled) {
        // 禁用LVD
        ctrl1 &= ~PMU_CTRL1_LVDEN_MASK;
        PMU->CTRL1 = ctrl1;
    } else {
        // 配置LVD
        // 1. 配置阈值
        ctrl1 &= ~PMU_CTRL1_LVD_SEL_MASK;
        ctrl1 |= ((uint32_t)config->level << PMU_CTRL1_LVD_SEL_POS);
        
        // 2. 配置滤波
        if (config->filter_enabled) {
            ctrl1 |= PMU_CTRL1_LVD_FLTEN_MASK;
            ctrl1 &= ~PMU_CTRL1_FLT_TIME_MASK;
            ctrl1 |= ((uint32_t)config->filter_time << PMU_CTRL1_FLT_TIME_POS);
        } else {
            ctrl1 &= ~PMU_CTRL1_LVD_FLTEN_MASK;
        }
        
        // 3. 使能LVD
        ctrl1 |= PMU_CTRL1_LVDEN_MASK;
        PMU->CTRL1 = ctrl1;
        
        // 4. 等待2us稳定（使能后2us内可能误报警）
        delay_us(PMU_LVD_STABLE_US);
    }
}

/**
 * @brief 使能LVD
 */
void pmu_lvd_enable(pmu_lvd_level_t level)
{
    pmu_lvd_config_t config = {
        .enabled = true,
        .level = level,
        .filter_enabled = false,
        .filter_time = PMU_LVD_FILTER_1_CYCLE,
    };
    
    pmu_lvd_configure(&config);
}

/**
 * @brief 禁用LVD
 */
void pmu_lvd_disable(void)
{
    PMU->CTRL1 &= ~PMU_CTRL1_LVDEN_MASK;
}

/**
 * @brief 获取LVD原始状态
 */
bool pmu_lvd_get_status(void)
{
    return (PMU->CTRL1 & PMU_CTRL1_LVD_VALUE_MASK) != 0;
}

/**
 * @brief 获取LVD滤波后状态
 */
bool pmu_lvd_get_filtered_status(void)
{
    return (PMU->CTRL1 & PMU_CTRL1_LVD_FILTER_MASK) != 0;
}

/**
 * @brief 获取BOR状态
 */
bool pmu_bor_get_status(void)
{
    return (PMU->SR & PMU_SR_BORN_MASK) != 0;
}

/**
 * @brief 配置待机域IO（PC13）
 */
void pmu_stby_io_pc13_configure(const pmu_stby_io_config_t *config)
{
    assert(config != NULL);
    
    uint32_t iosel = PMU->IOSEL;
    
    // 清除旧配置
    iosel &= ~(PMU_IOSEL_PC13_SEL_MASK | PMU_IOSEL_PC13_VALUE_MASK);
    
    // 设置功能选择
    iosel |= ((uint32_t)config->func << PMU_IOSEL_PC13_SEL_POS);
    
    // 设置PC13_Value：RTC_FOUT模式时控制输出模式(0=开漏,1=推挽)，OUTPUT模式时控制输出值
    if (config->func == PMU_STBY_IO_RTC_FOUT && config->output_pushpull) {
        iosel |= PMU_IOSEL_PC13_VALUE_MASK;
    } else if (config->func == PMU_STBY_IO_OUTPUT && config->output_value) {
        iosel |= PMU_IOSEL_PC13_VALUE_MASK;
    }
    
    PMU->IOSEL = iosel;
}

/**
 * @brief 配置待机域IO（PC14）
 */
void pmu_stby_io_pc14_configure(const pmu_stby_io_config_t *config)
{
    assert(config != NULL);
    
    uint32_t iosel = PMU->IOSEL;
    
    // 清除旧配置
    iosel &= ~(PMU_IOSEL_PC14_SEL_MASK | PMU_IOSEL_PC14_VALUE_MASK);
    
    // 设置功能选择
    iosel |= ((uint32_t)config->func << PMU_IOSEL_PC14_SEL_POS);
    
    // 设置输出值
    if (config->output_value) {
        iosel |= PMU_IOSEL_PC14_VALUE_MASK;
    }
    
    PMU->IOSEL = iosel;
}

/**
 * @brief 配置待机域IO（PC15）
 */
void pmu_stby_io_pc15_configure(const pmu_stby_io_config_t *config)
{
    assert(config != NULL);
    
    uint32_t iosel = PMU->IOSEL;
    
    // 清除旧配置
    iosel &= ~(PMU_IOSEL_PC15_SEL_MASK | PMU_IOSEL_PC15_VALUE_MASK);
    
    // 设置功能选择
    iosel |= ((uint32_t)config->func << PMU_IOSEL_PC15_SEL_POS);
    
    // 设置输出值
    if (config->output_value) {
        iosel |= PMU_IOSEL_PC15_VALUE_MASK;
    }
    
    PMU->IOSEL = iosel;
}

/**
 * @brief 控制备份SRAM状态
 */
void pmu_backup_sram_control(bool retain)
{
    uint32_t ctrl0 = PMU->CTRL0;
    
    if (retain) {
        // 保持状态：设置BKPRAMREN，清除BKPRAMSEN
        ctrl0 |= PMU_CTRL0_BKPRAMREN_MASK;
        ctrl0 &= ~PMU_CTRL0_BKPRAMSEN_MASK;
    } else {
        // 睡眠状态：设置BKPRAMSEN，清除BKPRAMREN
        ctrl0 |= PMU_CTRL0_BKPRAMSEN_MASK;
        ctrl0 &= ~PMU_CTRL0_BKPRAMREN_MASK;
    }
    
    PMU->CTRL0 = ctrl0;
}

/**
 * @brief 使能RTC模块写操作
 */
void pmu_rtc_write_enable(void)
{
    PMU->CTRL0 |= PMU_CTRL0_RTC_WE_MASK;
}

/**
 * @brief 禁用RTC模块写操作
 */
void pmu_rtc_write_disable(void)
{
    PMU->CTRL0 &= ~PMU_CTRL0_RTC_WE_MASK;
}

//===========================================
// 第4层: 控制与查询 API
//===========================================

/**
 * @brief 获取唤醒源标志
 */
uint32_t pmu_get_wakeup_flags(void)
{
    uint32_t flags = 0;
    uint32_t sr = PMU->SR;
    
    // 检查WAKEUP引脚标志
    if (sr & PMU_SR_WUPFX_MASK) {
        flags |= PMU_WAKEUP_SOURCE_PIN;
    }
    
    // 检查RTC唤醒标志
    if (sr & PMU_SR_RTCWUF_MASK) {
        flags |= PMU_WAKEUP_SOURCE_RTC;
    }
    
    // 检查IWDT唤醒标志
    if (sr & PMU_SR_IWDTWUF_MASK) {
        flags |= PMU_WAKEUP_SOURCE_IWDT;
    }
    
    // 检查RSTN唤醒标志
    if (sr & PMU_SR_RSTWUF_MASK) {
        flags |= PMU_WAKEUP_SOURCE_RSTN;
    }
    
    // 检查BOR唤醒标志
    if (sr & PMU_SR_BORWUF_MASK) {
        flags |= PMU_WAKEUP_SOURCE_BOR;
    }
    
    return flags;
}

/**
 * @brief 获取WAKEUP引脚唤醒标志
 */
uint32_t pmu_get_wakeup_pin_flags(void)
{
    return PMU->SR & PMU_SR_WUPFX_MASK;
}

/**
 * @brief 清除WAKEUP引脚唤醒标志
 */
void pmu_clear_wakeup_pin_flags(uint32_t pins)
{
    PMU->STCLR = (pins & PMU_STCLR_CWUFX_MASK);
}

/**
 * @brief 清除RTC唤醒标志
 */
void pmu_clear_rtc_wakeup_flag(void)
{
    PMU->STCLR = PMU_STCLR_CRTCWUF_MASK;
}

/**
 * @brief 清除IWDT唤醒标志
 */
void pmu_clear_iwdt_wakeup_flag(void)
{
    PMU->STCLR = PMU_STCLR_CIWDTWUF_MASK;
}

/**
 * @brief 清除RSTN唤醒标志
 */
void pmu_clear_rstn_wakeup_flag(void)
{
    PMU->STCLR = PMU_STCLR_CRSTWUF_MASK;
}

/**
 * @brief 清除BOR唤醒标志
 */
void pmu_clear_bor_wakeup_flag(void)
{
    PMU->STCLR = PMU_STCLR_CBORWUF_MASK;
}

/**
 * @brief 检查是否从STANDBY唤醒
 */
bool pmu_is_standby_wakeup(void)
{
    return (PMU->SR & PMU_SR_SBF_MASK) != 0;
}

/**
 * @brief 清除STANDBY标志
 */
void pmu_clear_standby_flag(void)
{
    PMU->STCLR = PMU_STCLR_CSBF_MASK;
}

/**
 * @brief 清除所有唤醒标志
 */
void pmu_clear_all_wakeup_flags(void)
{
    PMU->STCLR = PMU_STCLR_CWUFX_MASK | 
                 PMU_STCLR_CSBF_MASK | 
                 PMU_STCLR_CRTCWUF_MASK | 
                 PMU_STCLR_CRSTWUF_MASK | 
                 PMU_STCLR_CIWDTWUF_MASK |
                 PMU_STCLR_CBORWUF_MASK;
}

