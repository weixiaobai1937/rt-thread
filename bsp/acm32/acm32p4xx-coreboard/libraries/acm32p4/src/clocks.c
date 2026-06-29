/**
 * @file clocks.c
 * @brief ACM32P4 时钟管理驱动实现
 * 
 * 本文件实现了ACM32P4芯片的时钟管理功能，提供了以下接口：
 * - 时钟源控制（RCH/XTH/XTL/RCL）
 * - PLL锁相环配置（PLL1）
 * - 系统时钟切换和分频配置
 * - 外设时钟使能/禁用控制
 * - 时钟频率查询接口
 * - 时钟安全系统（CSS）
 * - 时钟就绪中断管理
 * - 外设复位控制
 * - 系统复位控制
 * - 外设时钟源配置（LPUART/LPTIM/SDMMC等）
 * - MCO时钟输出配置
 * - PLL扩频时钟（SSC）配置
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include <assert.h>
#include "device/acm32p4xx.h"
#include "hardware/clocks.h"

//===========================================
// 内部宏定义
//===========================================

#define TIMEOUT_MS              1000    ///< 默认超时时间 (ms)
#define CLOCK_READY_TIMEOUT     100000  ///< 时钟就绪超时计数

//===========================================
// 内部状态变量
//===========================================

/**
 * @brief 时钟状态结构体
 */
static struct {
    uint32_t sysclk_hz;             ///< 系统时钟频率
    uint32_t hclk_hz;               ///< AHB时钟频率
    uint32_t pclk1_hz;              ///< APB1时钟频率
    uint32_t pclk2_hz;              ///< APB2时钟频率
    uint32_t rch_hz;                ///< RCH频率
    uint32_t xth_hz;                ///< XTH频率
    clock_css_callback_t xth_css_cb;///< XTH停振回调
    clock_css_callback_t xtl_css_cb;///< XTL停振回调
    // 时钟就绪中断回调
    clock_ready_callback_t rch_ready_cb;  ///< RCH就绪回调
    clock_ready_callback_t xth_ready_cb;  ///< XTH就绪回调
    clock_ready_callback_t xtl_ready_cb;  ///< XTL就绪回调
    clock_ready_callback_t rcl_ready_cb;  ///< RCL就绪回调
    clock_ready_callback_t pll1_lock_cb;  ///< PLL1锁定回调
    clock_rtc_irq_callback_t rtc_irq_cb;  ///< RTC模块中断回调
} clock_state = {
    .sysclk_hz = RCH_FREQ_HZ,
    .hclk_hz = RCH_FREQ_HZ,
    .pclk1_hz = RCH_FREQ_HZ,
    .pclk2_hz = RCH_FREQ_HZ,
    .rch_hz = RCH_FREQ_HZ,
    .xth_hz = 0,
    .xth_css_cb = NULL,
    .xtl_css_cb = NULL,
    .rch_ready_cb = NULL,
    .xth_ready_cb = NULL,
    .xtl_ready_cb = NULL,
    .rcl_ready_cb = NULL,
    .pll1_lock_cb = NULL,
    .rtc_irq_cb = NULL,
};

/**
 * @brief 外设时钟使能寄存器映射表
 */
static volatile uint32_t* const clock_ckenr_regs[] = {
    &RCC->AHB1CKENR,   // 0: AHB1
    &RCC->AHB2CKENR,   // 1: AHB2
    &RCC->AHB3CKENR,   // 2: AHB3
    &RCC->APB1CKENR1,  // 3: APB1_1
    &RCC->APB1CKENR2,  // 4: APB1_2
    &RCC->APB2CKENR,   // 5: APB2
};

/**
 * @brief 外设复位寄存器映射表
 */
static volatile uint32_t* const clock_rstr_regs[] = {
    &RCC->AHB1RSTR,    // 0: AHB1
    &RCC->AHB2RSTR,    // 1: AHB2
    &RCC->AHB3RSTR,    // 2: AHB3
    &RCC->APB1RSTR1,   // 3: APB1_1
    &RCC->APB1RSTR2,   // 4: APB1_2
    &RCC->APB2RSTR,    // 5: APB2
};

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 简单延时函数
 * 
 * 通过NOP指令实现的CPU周期级延时
 * 
 * @param[in] count 延时计数（CPU周期数）
 */
static inline void delay_cycles(uint32_t count)
{
    while (count--) {
        __NOP();
    }
}


/**
 * @brief 更新SystemCoreClock全局变量
 * 
 * 同步时钟状态到CMSIS标准的SystemCoreClock全局变量
 * 
 * @note 符合CMSIS规范要求
 */
static void clock_update_system_core_clock(void)
{
    extern uint32_t SystemCoreClock;
    SystemCoreClock = clock_state.sysclk_hz;
}

/**
 * @brief 计算最优PLL参数
 * 
 * 遍历N/F/P组合寻找最接近目标频率的PLL配置
 * 
 * @param[in] input_hz PLL输入频率 (Hz)
 * @param[in] target_hz PLL目标输出频率 (Hz)
 * @param[out] config 输出的PLL配置参数
 * @return true 找到合适的配置（误差<2%）
 * @return false 无法达到目标频率
 * 
 * @note PFD频率限制: 1-2MHz
 * @note VCO频率限制: 100-550MHz
 * @note 允许±2%的频率误差
 */
static bool clock_calculate_pll_params(uint32_t input_hz, uint32_t target_hz, 
                                       pll_config_t *config)
{
    assert(config != NULL);
    
    // 简化实现：尝试常用的配置
    // 实际实现需要遍历所有可能的N/F/P组合
    
    // 目标：PFD频率在1-2MHz范围内
    // VCO频率在100-550MHz范围内
    // 输出频率匹配target_hz
    
    for (uint8_t n = 1; n <= 64; n++) {
        uint32_t pfd_hz = input_hz / n;
        if (pfd_hz < 1000000 || pfd_hz > 2000000) {
            continue;
        }
        
        for (uint16_t f = 50; f <= 511; f++) {
            uint32_t vco_hz = pfd_hz * f;
            if (vco_hz < PLL_VCO_MIN_HZ || vco_hz > PLL_VCO_MAX_HZ) {
                continue;
            }
            
            for (uint8_t p = 1; p <= 4; p++) {
                uint32_t out_hz = vco_hz / (p * 2);
                if (out_hz >= target_hz * 0.98 && out_hz <= target_hz * 1.02) {
                    config->n = n;
                    config->f = f;
                    config->p = p - 1;  // 寄存器值
                    config->q = 4;       // 默认Q分频
                    return true;
                }
            }
        }
    }
    
    return false;
}

//===========================================
// 第1层: 快速初始化 API
//===========================================

/**
 * @brief 默认时钟初始化
 * 
 * 使用RCH作为时钟源，配置系统运行在180MHz
 * - 启用RCH (16分频模式)
 * - 配置PLL1: RCH/16 -> 4MHz -> PFD 2MHz -> VCO 360MHz -> 180MHz
 * - 配置AHB: 180MHz
 * - 配置APB1/APB2: 90MHz (2分频)
 * 
 * @note 默认配置，不依赖外部晶振
 * @see clock_init_xtal()
 */
void clock_init_default(void)
{
    // 1. 启用RCH (16分频模式用于PLL)
    clock_rch_enable(true);
    
    // 2. 配置PLL1
    // 输入: RCH/16 = 4MHz
    // PFD: 4MHz / 2 = 2MHz
    // VCO: 2MHz * 180 = 360MHz
    // 输出: 360MHz / 2 = 180MHz
    pll_config_t pll = {
        .src = 0,    // RCH
        .n = 2,      // 4MHz / 2 = 2MHz
        .f = 180,    // 2MHz * 180 = 360MHz
        .p = 0,      // 360MHz / 2 = 180MHz (p=0表示2分频)
        .q = 2,      // 360MHz / 2 = 180MHz
    };
    clock_pll1_configure(&pll);
    
    // 3. 配置系统分频
    clock_set_sys_divider(1, 1);  // 不分频
    clock_set_apb_divider(2, 2);  // APB1/APB2 2分频
    
    // 4. 切换到PLL1
    clock_set_sysclk_source(SYSCLK_SRC_PLL1);
    
    // 5. 更新时钟状态
    clock_state.sysclk_hz = 180000000;
    clock_state.hclk_hz = 180000000;
    clock_state.pclk1_hz = 90000000;
    clock_state.pclk2_hz = 90000000;
    
    clock_update_system_core_clock();
}

/**
 * @brief 使用外部晶振初始化时钟
 * 
 * 使用XTH作为时钟源，通过PLL配置系统运行在180MHz
 * - 启用XTH晶振
 * - 自动计算PLL参数达到180MHz
 * - 配置AHB和APB分频
 * 
 * @param[in] xtal_freq_hz 外部晶振频率 (Hz)
 * 
 * @note 如果XTH启动失败或无法达到目标频率，自动降级到RCH模式
 * @see clock_init_default()
 */
void clock_init_xtal(uint32_t xtal_freq_hz)
{
    // 1. 启用XTH
    if (!clock_xth_enable(xtal_freq_hz, false)) {
        // XTH启动失败，降级到RCH
        clock_init_default();
        return;
    }
    
    // 2. 配置PLL1使用XTH
    pll_config_t pll;
    pll.src = 1;  // XTH
    
    if (!clock_calculate_pll_params(xtal_freq_hz, 180000000, &pll)) {
        // 无法达到目标频率，降级
        clock_init_default();
        return;
    }
    
    clock_pll1_configure(&pll);
    
    // 3. 配置系统分频
    clock_set_sys_divider(1, 1);
    clock_set_apb_divider(2, 2);  // APB1/APB2 2分频
    
    // 4. 切换到PLL1
    clock_set_sysclk_source(SYSCLK_SRC_PLL1);
    
    // 5. 更新状态
    clock_state.sysclk_hz = 180000000;
    clock_state.hclk_hz = 180000000;
    clock_state.pclk1_hz = 90000000;
    clock_state.pclk2_hz = 90000000;
    
    clock_update_system_core_clock();
}

/**
 * @brief 获取系统时钟频率
 * 
 * 动态计算当前实际的系统时钟频率，而非返回缓存值
 * 
 * @return uint32_t 当前系统时钟频率 (Hz)
 * 
 * @note 此函数会读取寄存器并计算实时频率，确保准确性
 */
uint32_t clock_get_hz(void)
{
    sysclk_src_t src = clock_get_sysclk_source();
    uint32_t sysclk_hz = 0;
    
    switch (src) {
        case SYSCLK_SRC_RCH:
            // RCH 频率：64MHz 或 4MHz（16分频）
            if (RCC->RCHCR & (1U << 3)) {  // RCHDIV
                sysclk_hz = RCH_FREQ_HZ / 16;
            } else {
                sysclk_hz = RCH_FREQ_HZ;
            }
            break;
            
        case SYSCLK_SRC_XTH:
            // XTH 频率（从缓存中读取，启动时设置）
            sysclk_hz = clock_state.xth_hz;
            break;
            
        case SYSCLK_SRC_PLL1:
            // 从 PLL1 配置计算输出频率
            {
                // 获取 PLL1 输入时钟源
                uint32_t input_hz;
                if (RCC->PLL1CR & (1U << 1)) {  // PLL1SRCSEL = XTH
                    input_hz = clock_state.xth_hz;
                } else {  // PLL1SRCSEL = RCH，根据 RCHDIV 确定实际输入频率
                    input_hz = (RCC->RCHCR & (1U << 3)) ? (RCH_FREQ_HZ / 16) : RCH_FREQ_HZ;
                }
                
                // 读取 PLL1 配置参数
                uint32_t n = ((RCC->PLL1CFR >> 12) & 0x3F);  // PLL1N (1-64)
                uint32_t f = (RCC->PLL1CFR & 0x1FF);         // PLL1F (50-511)
                uint32_t p = ((RCC->PLL1CFR >> 20) & 0x3);   // PLL1P (0-3)
                
                // 计算 PLL1 输出频率
                // Fout = (Fin / N) * F / (2 * (P + 1))
                if (n > 0) {
                    uint32_t pfd_hz = input_hz / n;
                    uint64_t vco_hz = (uint64_t)pfd_hz * f;
                    sysclk_hz = (uint32_t)(vco_hz / (2 * (p + 1)));
                } else {
                    sysclk_hz = RCH_FREQ_HZ;  // 错误配置，返回默认值
                }
            }
            break;
            
        default:
            sysclk_hz = RCH_FREQ_HZ;
            break;
    }
    
    // 应用系统分频器
    uint32_t sysdiv0 = (RCC->CCR2 & 0xF) + 1;         // SYSDIV0 (1-16)
    uint32_t sysdiv1 = ((RCC->CCR2 >> 4) & 0xF) + 1;  // SYSDIV1 (1-16)
    
    uint32_t hclk_hz = sysclk_hz / (sysdiv0 * sysdiv1);
    
    // 更新缓存值（供其他函数使用）
    clock_state.sysclk_hz = sysclk_hz;
    clock_state.hclk_hz = hclk_hz;
    
    return sysclk_hz;
}

//===========================================
// 第2层: 基础配置 API — 时钟源控制
//===========================================

/**
 * @brief 启用RCH（内部高速RC振荡器）
 * 
 * 启用RCH并可选择16分频模式
 * 
 * @param[in] div16 true=启用16分频（64MHz/16=4MHz，用于PLL），false=不分频（64MHz）
 * @return true 启用成功
 * @return false 启用超时失败
 * 
 * @note RCH典型频率：64MHz
 * @note 16分频模式常用于PLL输入源
 */
bool clock_rch_enable(bool div16)
{
    // 使能RCH
    RCC->RCHCR |= (1U << 0);  // RCHEN
    
    // 配置分频
    if (div16) {
        RCC->RCHCR |= (1U << 3);   // RCHDIV
    } else {
        RCC->RCHCR &= ~(1U << 3);
    }
    
    // 等待RCH就绪
    uint32_t timeout = CLOCK_READY_TIMEOUT;
    while (!(RCC->RCHCR & (1U << 4))) {  // RCHRDY
        if (--timeout == 0) {
            return false;
        }
    }
    
    clock_state.rch_hz = div16 ? (RCH_FREQ_HZ / 16) : RCH_FREQ_HZ;
    return true;
}

/**
 * @brief 禁用RCH时钟
 * 
 * @warning 如果系统时钟源使用RCH，禁用会导致系统停止运行
 */
void clock_rch_disable(void)
{
    RCC->RCHCR &= ~(1U << 0);  // RCHEN
}

/**
 * @brief 设置RCH频率微调值
 * 
 * 通过调整TRIM寄存器微调RCH输出频率
 * 
 * @param[in] trim 微调值 (0-255)
 * 
 * @note 出厂默认值已经过校准，一般无需修改
 */
void clock_rch_set_trim(uint8_t trim)
{
    uint32_t cr = RCC->RCHCR;
    cr &= ~(0xFFU << 8);              // 清除 RCHTRIML [14:8]
    cr |= ((uint32_t)trim << 8);      // 设置 RCHTRIML [14:8]
    RCC->RCHCR = cr;
}

/**
 * @brief 获取RCH频率微调值
 * 
 * @return uint8_t 当前TRIM值 (0-255)
 */
uint8_t clock_rch_get_trim(void)
{
    return (RCC->RCHCR >> 8) & 0xFF;
}

/**
 * @brief 启用XTH（外部高速晶振/时钟）
 * 
 * 启用XTH并等待稳定
 * 
 * @param[in] freq_hz XTH频率 (Hz)，典型值：8MHz、12MHz、16MHz、24MHz等
 * @param[in] bypass true=旁路模式（外部方波输入），false=晶振模式
 * @return true 启用成功
 * @return false 启用超时失败
 * 
 * @note XTH启动时间较长，超时时间已设置为CLOCK_READY_TIMEOUT*100
 * @note 旁路模式用于直接输入外部时钟信号
 */
bool clock_xth_enable(uint32_t freq_hz, bool bypass)
{
    if (bypass) {
        // 旁路模式：外部方波输入
        RCC->XTHCR |= (1U << 1);   // XTHBYP
        RCC->XTHCR |= (1U << 0);   // XTHEN
    } else {
        // 晶振模式
        RCC->XTHCR &= ~(1U << 1);  // XTHBYP
        RCC->XTHCR |= (1U << 0);   // XTHEN
    }
    
    // 等待XTH就绪
    uint32_t timeout = CLOCK_READY_TIMEOUT * 100;  // XTH需要更长时间
    while (!(RCC->XTHCR & (1U << 4))) {  // XTHRDY
        if (--timeout == 0) {
            return false;
        }
    }
    
    clock_state.xth_hz = freq_hz;
    return true;
}

/**
 * @brief 禁用XTH时钟
 * 
 * @warning 如果系统时钟源使用XTH或基于XTH的PLL，禁用会导致系统停止运行
 */
void clock_xth_disable(void)
{
    RCC->XTHCR &= ~(1U << 0);  // XTHEN
}

/**
 * @brief 启用XTL（外部低速晶振/时钟）
 * 
 * 启用32.768kHz外部低速晶振，通常用于RTC
 * 
 * @param[in] bypass true=旁路模式（外部方波输入），false=晶振模式
 * @return true 启用成功
 * @return false 启用超时失败
 * 
 * @note XTL启动时间很长（可能数秒），超时时间已设置为CLOCK_READY_TIMEOUT*1000
 * @note XTL典型频率：32.768kHz
 */
bool clock_xtl_enable(bool bypass)
{
    if (bypass) {
        RCC->STDBYCTRL |= (1U << 2);   // XTLBYP
        RCC->STDBYCTRL |= (1U << 0);   // XTLEN
    } else {
        RCC->STDBYCTRL &= ~(1U << 2);
        RCC->STDBYCTRL |= (1U << 0);
    }
    
    // 等待XTL就绪（需要很长时间）
    uint32_t timeout = CLOCK_READY_TIMEOUT * 1000;
    while (!(RCC->STDBYCTRL & (1U << 1))) {  // XTLRDY
        if (--timeout == 0) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 禁用XTL时钟
 * 
 * @warning 如果RTC使用XTL，禁用会导致RTC停止工作
 */
void clock_xtl_disable(void)
{
    RCC->STDBYCTRL &= ~(1U << 0);  // XTLEN
}

/**
 * @brief 启用RCL（内部低速RC振荡器）
 * 
 * 启用32kHz内部低速RC振荡器，可作为RTC时钟源
 * 
 * @return true 启用成功
 * @return false 启用超时失败
 * 
 * @note RCL典型频率：32kHz
 */
bool clock_rcl_enable(void)
{
    RCC->STDBYCTRL |= (1U << 8);  // RCLEN
    
    uint32_t timeout = CLOCK_READY_TIMEOUT;
    while (!(RCC->STDBYCTRL & (1U << 9))) {  // RCLRDY
        if (--timeout == 0) {
            return false;
        }
    }
    
    return true;
}

/**
 * @brief 设置RCL频率微调值
 * 
 * @param[in] trim 微调值 (0-255)
 * 
 * @note 用于校准RCL频率，提高RTC精度
 */
void clock_rcl_set_trim(uint8_t trim)
{
    uint32_t ctrl = RCC->STDBYCTRL;
    ctrl &= ~(0x3FU << 10);                     // 清除 RCLTRIM [15:10] (6位)
    ctrl |= ((uint32_t)(trim & 0x3F) << 10);    // 设置 RCLTRIM [15:10]
    RCC->STDBYCTRL = ctrl;
}

/**
 * @brief 获取RCL频率微调值
 * 
 * @return uint8_t 当前TRIM值 (0-255)
 */
uint8_t clock_rcl_get_trim(void)
{
    return (RCC->STDBYCTRL >> 10) & 0x3F;
}

//===========================================
// 基础配置 — PLL配置
//===========================================

/**
 * @brief 配置PLL1锁相环
 * 
 * 配置PLL1的所有参数并等待锁定
 * 
 * @param[in] config PLL配置参数
 *                   - src: 时钟源选择 (0=RCH, 1=XTH)
 *                   - n: 输入分频系数 (1-64)
 *                   - f: VCO倍频系数 (50-511)
 *                   - p: P输出分频 (0-3，对应2/4/6/8分频)
 *                   - q: Q输出分频 (0-15)
 * @return true 配置成功并锁定
 * @return false 配置失败或锁定超时
 * 
 * @note PLL频率计算：Fout = (Fin / N) * F / (2*(P+1))
 * @note PFD频率范围：1-2MHz
 * @note VCO频率范围：100-550MHz
 * @note SSC（扩频）需在 PLL 配置前通过 clock_pll1_ssc_configure() 设置，
 *       配置顺序：SSC → PLL (本函数) → 等待锁定 → 切换系统时钟
 */
bool clock_pll1_configure(const pll_config_t *config)
{
    assert(config != NULL);
    assert(config->n >= 1 && config->n <= 64);
    assert(config->f >= 50 && config->f <= 511);
    assert(config->p <= 3);
    assert(config->q <= 15);
    assert(config->src <= 1);

    // 1. 使能PLL1
    RCC->PLL1CR |= (1U << 0);  // PLL1EN
    
    // 2. 配置时钟源
    if (config->src) {
        RCC->PLL1CR |= (1U << 1);   // PLL1SRCSEL = XTH
    } else {
        RCC->PLL1CR &= ~(1U << 1);  // PLL1SRCSEL = RCH
    }
    
    // 3. 退出休眠模式
    RCC->PLL1CR &= ~(1U << 21);  // PLL1SLEEP = 0
    
    // 4. 等待PLL初始稳定
    delay_cycles(1000);
    
    // 5. 配置PLL参数
    uint32_t cfr = ((config->q & 0xF) << 24) |   // PLL1Q
                   ((config->p & 0x3) << 20) |    // PLL1P
                   ((config->n & 0x3F) << 12) |   // PLL1N
                   (config->f & 0x1FF);           // PLL1F
    RCC->PLL1CFR = cfr;
    
    // 6. 使能输出
    RCC->PLL1CR |= (1U << 4);  // PLL1PCLKEN
    RCC->PLL1CR |= (1U << 5);  // PLL1QCLKEN

    // 7. 判断PLL1是否为系统时钟源，假如是，则切换到RCH，待PLL1配置完成后再切换回来
    sysclk_src_t current_src = clock_get_sysclk_source();
    if (current_src == SYSCLK_SRC_PLL1) {
        clock_set_sysclk_source(SYSCLK_SRC_RCH);
    }
    // 8. 触发配置更新
    RCC->PLL1CR |= (1U << 22);  // PLL1UPDATEEN
    
    // 9. 等待PLL锁定
    uint32_t timeout = CLOCK_READY_TIMEOUT;
    while (!(RCC->PLL1CR & (1U << 29))) {  // PLL1LOCK
        if (--timeout == 0) {
            // 尝试检查FREERUN标志
            if (!(RCC->PLL1CR & (1U << 30))) {
                return false;
            }
            break;
        }
    }
    // 10. 如果PLL1为系统时钟源，则切换回来
    if (current_src == SYSCLK_SRC_PLL1) {
        clock_set_sysclk_source(SYSCLK_SRC_PLL1);
    }
    return true;
}

/**
 * @brief 简化PLL1配置接口
 * 
 * 自动计算PLL参数以达到目标频率
 * 
 * @param[in] target_freq_hz 目标输出频率 (Hz)
 * @return true 配置成功
 * @return false 无法达到目标频率或配置失败
 * 
 * @note 自动使用当前PLL1的时钟源
 * @note 允许±2%的频率误差
 */
bool clock_pll1_configure_simple(uint32_t target_freq_hz)
{
    pll_config_t config;
    
    // 使用当前时钟源
    uint32_t input_hz = (RCC->PLL1CR & (1U << 1)) ? clock_state.xth_hz
                     : ((RCC->RCHCR & (1U << 3)) ? (RCH_FREQ_HZ / 16) : RCH_FREQ_HZ);
    
    if (!clock_calculate_pll_params(input_hz, target_freq_hz, &config)) {
        return false;
    }
    
    // 保留当前时钟源设置
    config.src = (RCC->PLL1CR & (1U << 1)) ? 1 : 0;
    
    return clock_pll1_configure(&config);
}

/**
 * @brief 禁用PLL1
 * 
 * 使PLL1进入休眠模式并关闭
 * 
 * @warning 如果系统时钟源使用PLL1，禁用会导致系统停止运行
 */
void clock_pll1_disable(void)
{
    RCC->PLL1CR |= (1U << 21);   // 进入休眠
    RCC->PLL1CR &= ~(1U << 0);   // 禁用PLL1
}
//===========================================
// 基础配置 — 系统时钟切换与分频
//===========================================

/**
 * @brief 设置系统时钟源
 * 
 * 切换系统时钟到指定的时钟源
 * 
 * @param[in] src 时钟源选择
 *                - SYSCLK_SRC_RCH: 内部RC振荡器
 *                - SYSCLK_SRC_XTH: 外部晶振
 *                - SYSCLK_SRC_PLL1: PLL1输出
 * @return true 切换成功
 * @return false 目标时钟源未就绪
 * 
 * @note 切换前会自动检查目标时钟源是否就绪
 * @warning 切换时钟源可能影响系统稳定性，建议在合适的Flash等待周期下进行
 */
bool clock_set_sysclk_source(sysclk_src_t src)
{
    // 检查目标时钟源是否就绪
    switch (src) {
        case SYSCLK_SRC_RCH:
            if (!(RCC->RCHCR & (1U << 4))) {  // RCHRDY
                return false;
            }
            break;
        case SYSCLK_SRC_XTH:
            if (!(RCC->XTHCR & (1U << 4))) {  // XTHRDY
                return false;
            }
            break;
        case SYSCLK_SRC_PLL1:
            if (!(RCC->PLL1CR & (1U << 29))) {  // PLL1LOCK
                return false;
            }
            break;
    }
    
    // 切换时钟源
    uint32_t ccr1 = RCC->CCR1 & ~0x3;
    if (src == SYSCLK_SRC_XTH) {
        ccr1 |= 0x2;
    } else if (src == SYSCLK_SRC_PLL1) {
        ccr1 |= 0x3;
    }
    RCC->CCR1 = ccr1;
    
    // 等待切换完成
    delay_cycles(100);
    
    return true;
}

/**
 * @brief 获取当前系统时钟源
 * @return sysclk_src_t 当前系统时钟源
 */
sysclk_src_t clock_get_sysclk_source(void)
{
    uint32_t clksel = RCC->CCR1 & 0x3;
    
    switch (clksel) {
        case 0:  // 00 = RCH
        case 1:  // 01 = RCH
            return SYSCLK_SRC_RCH;
        case 2:  // 10 = XTH
            return SYSCLK_SRC_XTH;
        case 3:  // 11 = PLL1
            return SYSCLK_SRC_PLL1;
        default:
            return SYSCLK_SRC_RCH;
    }
}

/**
 * @brief 设置系统时钟分频
 * 
 * 配置SYSCLK到HCLK的两级分频器
 * 
 * @param[in] sysdiv0 第一级分频系数 (1-16)
 * @param[in] sysdiv1 第二级分频系数 (1-16)
 * 
 * @note HCLK = SYSCLK / (sysdiv0 * sysdiv1)
 * @note 函数会自动等待分频配置完成
 * @note 超出范围的值会被自动限制
 */
void clock_set_sys_divider(uint8_t sysdiv0, uint8_t sysdiv1)
{
    if (sysdiv0 < 1) { sysdiv0 = 1; }
    if (sysdiv0 > 16) { sysdiv0 = 16; }
    if (sysdiv1 < 1) { sysdiv1 = 1; }
    if (sysdiv1 > 16) { sysdiv1 = 16; }
    
    uint32_t ccr2 = RCC->CCR2;
    ccr2 &= ~(0xF | (0xF << 4));
    ccr2 |= ((sysdiv0 - 1) & 0xF);
    ccr2 |= (((sysdiv1 - 1) & 0xF) << 4);
    RCC->CCR2 = ccr2;
    
    // 等待分频完成
    while (!(RCC->CCR2 & (1U << 31))) {  // DIVDONE
        __NOP();
    }
    
    // 更新HCLK频率
    clock_state.hclk_hz = clock_state.sysclk_hz / (sysdiv0 * sysdiv1);
}

/**
 * @brief 转换分频值到寄存器编码
 * 
 * 将APB分频值转换为对应的寄存器编码
 * 
 * @param[in] div 分频值 (1/2/4/8/16)
 * @return uint8_t 寄存器编码值
 * 
 * @note 分频映射：1->0, 2->4, 4->5, 8->6, 16(或其他)->7
 */
static uint8_t encode_apb_div(uint8_t div)
{
    if (div <= 1) { return 0; }    // 不分频
    if (div == 2) { return 4; }    // 2分频
    if (div == 4) { return 5; }    // 4分频
    if (div == 8) { return 6; }    // 8分频
    return 7;                      // 16分频
}

/**
 * @brief 设置APB总线分频
 * 
 * 配置HCLK到各APB总线的分频系数
 * 
 * @param[in] pclk1_div APB1分频系数 (1/2/4/8/16)
 * @param[in] pclk2_div APB2分频系数 (1/2/4/8/16)
 * 
 * @note PCLKx = HCLK / pclkx_div
 * @note 函数会自动更新内部时钟频率状态
 */
void clock_set_apb_divider(uint8_t pclk1_div, uint8_t pclk2_div)
{
    uint32_t ccr2 = RCC->CCR2;
    ccr2 &= ~(0x7 << 8 | 0x7 << 11);
    ccr2 |= (encode_apb_div(pclk1_div) << 8);
    ccr2 |= (encode_apb_div(pclk2_div) << 11);
    RCC->CCR2 = ccr2;
    
    // 更新频率状态
    clock_state.pclk1_hz = clock_state.hclk_hz / pclk1_div;
    clock_state.pclk2_hz = clock_state.hclk_hz / pclk2_div;
}

//===========================================
// 第3层: 高级功能 API — 外设时钟使能
//===========================================

/**
 * @brief 使能外设时钟
 * 
 * 打开指定外设的总线时钟
 * 
 * @param[in] periph 外设标识符（参见clock_periph_t枚举）
 * 
 * @note 使用前必须先使能外设时钟，否则访问外设寄存器会导致总线错误
 */
void clock_periph_enable(clock_periph_t periph)
{
    uint8_t reg_idx = (periph >> 8) & 0xF;
    uint8_t bit_pos = periph & 0x1F;
    
    if (reg_idx < 6) {
        *clock_ckenr_regs[reg_idx] |= (1U << bit_pos);
    }
}

/**
 * @brief 禁用外设时钟
 * 
 * 关闭指定外设的总线时钟以降低功耗
 * 
 * @param[in] periph 外设标识符（参见clock_periph_t枚举）
 * 
 * @warning 禁用后访问该外设寄存器会导致总线错误
 */
void clock_periph_disable(clock_periph_t periph)
{
    uint8_t reg_idx = (periph >> 8) & 0xF;
    uint8_t bit_pos = periph & 0x1F;
    
    if (reg_idx < 6) {
        *clock_ckenr_regs[reg_idx] &= ~(1U << bit_pos);
    }
}

/**
 * @brief 批量使能多个外设时钟
 * 
 * @param[in] periphs 外设标识符数组
 * @param[in] count 数组元素个数
 */
void clock_periph_enable_multi(const clock_periph_t *periphs, size_t count)
{
    assert(periphs != NULL);
    
    for (size_t i = 0; i < count; i++) {
        clock_periph_enable(periphs[i]);
    }
}

/**
 * @brief 查询外设时钟使能状态
 * 
 * @param[in] periph 外设标识符
 * @return true 外设时钟已使能
 * @return false 外设时钟未使能
 */
bool clock_periph_is_enabled(clock_periph_t periph)
{
    uint8_t reg_idx = (periph >> 8) & 0xF;
    uint8_t bit_pos = periph & 0x1F;
    
    if (reg_idx < 6) {
        return (*clock_ckenr_regs[reg_idx] >> bit_pos) & 1U;
    }
    return false;
}

//===========================================
// 第4层: 控制与查询 API — 频率查询
//===========================================

/**
 * @brief 获取HCLK频率
 * 
 * 动态计算当前实际的AHB总线时钟频率
 * 
 * @return uint32_t HCLK频率 (Hz)
 */
uint32_t clock_get_hclk_hz(void)
{
    clock_get_hz();  // 更新 clock_state.hclk_hz
    return clock_state.hclk_hz;
}

/**
 * @brief 解码APB分频值
 * 
 * @param div_bits 分频位域值 (0-7)
 * @return 实际分频系数 (1/2/4/8/16)
 */
static uint8_t decode_apb_div(uint8_t div_bits)
{
    if (div_bits == 0) { return 1; }   // 000 = 不分频
    if (div_bits == 4) { return 2; }   // 100 = 2分频
    if (div_bits == 5) { return 4; }   // 101 = 4分频
    if (div_bits == 6) { return 8; }   // 110 = 8分频
    return 16;                         // 111 = 16分频
}

/**
 * @brief 获取PCLK1频率
 * 
 * 动态计算当前实际的APB1总线时钟频率
 * 
 * @return uint32_t APB1总线时钟频率 (Hz)
 */
uint32_t clock_get_pclk1_hz(void)
{
    uint32_t hclk_hz = clock_get_hclk_hz();
    uint8_t div_bits = (RCC->CCR2 >> 8) & 0x7;
    uint8_t div = decode_apb_div(div_bits);
    return hclk_hz / div;
}

/**
 * @brief 获取PCLK2频率
 * 
 * 动态计算当前实际的APB2总线时钟频率
 * 
 * @return uint32_t APB2总线时钟频率 (Hz)
 */
uint32_t clock_get_pclk2_hz(void)
{
    uint32_t hclk_hz = clock_get_hclk_hz();
    uint8_t div_bits = (RCC->CCR2 >> 11) & 0x7;
    uint8_t div = decode_apb_div(div_bits);
    return hclk_hz / div;
}

/**
 * @brief 获取定时器时钟频率
 *
 * 根据 tim_instance_t 枚举值查表确定所挂载的 APB 总线，
 * 结合当前 APB 分频系数计算定时器实际输入时钟频率。
 *
 * @param[in] timer 定时器实例（tim_instance_t 枚举）
 * @return uint32_t 定时器时钟频率 (Hz)
 *
 * @note 定时器时钟倍频规则（ARM Cortex-M 标准）：
 *         APB 分频 = 1 → 定时器时钟 = PCLK
 *         APB 分频 > 1 → 定时器时钟 = PCLK × 2
 */
uint32_t clock_get_timer_hz(uint32_t timer)
{
    assert(timer < (uint32_t)TIM_COUNT);

    /*
     * APB 总线归属查找表，索引对应 tim_instance_t 枚举值（0-10）：
     *   1 = APB2，0 = APB1
     *
     * 枚举值 → 硬件定时器 → APB 总线（依据 RCC_APBxCKENR 寄存器）：
     *   0  TIM_1  → TIM1  → APB2
     *   1  TIM_8  → TIM8  → APB2
     *   2  TIM_2  → TIM2  → APB1
     *   3  TIM_5  → TIM5  → APB1
     *   4  TIM_3  → TIM3  → APB1
     *   5  TIM_4  → TIM4  → APB1
     *   6  TIM_9  → TIM9  → APB2
     *   7  TIM_10 → TIM10 → APB2
     *   8  TIM_6  → TIM6  → APB1
     *   9  TIM_7  → TIM7  → APB1
     *  10  TIM_26 → TIM26 → APB1
     */
    static const uint8_t s_apb2_map[TIM_COUNT] = {
        1,  /* TIM_1  = 0 → APB2 */
        1,  /* TIM_8  = 1 → APB2 */
        0,  /* TIM_2  = 2 → APB1 */
        0,  /* TIM_5  = 3 → APB1 */
        0,  /* TIM_3  = 4 → APB1 */
        0,  /* TIM_4  = 5 → APB1 */
        1,  /* TIM_9  = 6 → APB2 */
        1,  /* TIM_10 = 7 → APB2 */
        0,  /* TIM_6  = 8 → APB1 */
        0,  /* TIM_7  = 9 → APB1 */
        0,  /* TIM_26 = 10 → APB1 */
    };

    uint32_t pclk;
    uint8_t  div_bits;

    if (s_apb2_map[timer]) {
        pclk     = clock_get_pclk2_hz();
        div_bits = (RCC->CCR2 >> 11) & 0x7U;   /* CCR2[13:11] = PCLK2DIV */
    } else {
        pclk     = clock_get_pclk1_hz();
        div_bits = (RCC->CCR2 >> 8)  & 0x7U;   /* CCR2[10:8]  = PCLK1DIV */
    }
    uint8_t div = decode_apb_div(div_bits);
    return (div > 1U) ? (pclk * 2U) : pclk;
}

//===========================================
// 高级功能 — 时钟安全系统 (CSS)
//===========================================

/**
 * @brief 启用XTH时钟安全系统（CSS）
 * 
 * 启用XTH停振检测，当XTH故障时触发NMI中断
 * 
 * @note CSS触发后会通过NMI_Handler处理
 * @see clock_set_xth_css_callback()
 */
void clock_xth_css_enable(void)
{
    RCC->XTHCR |= (1U << 8);  // XTHSDEN
}

/**
 * @brief 禁用XTH时钟安全系统
 */
void clock_xth_css_disable(void)
{
    RCC->XTHCR &= ~(1U << 8);
}

/**
 * @brief 启用XTL时钟安全系统
 * 
 * 启用XTL停振检测，当XTL故障时触发中断
 */
void clock_xtl_css_enable(void)
{
    RCC->STDBYCTRL |= (1U << 6);  // XTLSDEN
}

/**
 * @brief 禁用XTL时钟安全系统
 */
void clock_xtl_css_disable(void)
{
    RCC->STDBYCTRL &= ~(1U << 6);
}

/**
 * @brief 设置XTH停振回调函数
 * 
 * @param[in] callback 回调函数指针，在XTH停振时被调用
 * 
 * @note 回调函数在NMI中断上下文中执行，需保持简短
 */
void clock_set_xth_css_callback(clock_css_callback_t callback)
{
    clock_state.xth_css_cb = callback;
}

void clock_set_xtl_css_callback(clock_css_callback_t callback)
{
    clock_state.xtl_css_cb = callback;
}

void clock_set_rtc_irq_callback(clock_rtc_irq_callback_t callback)
{
    clock_state.rtc_irq_cb = callback;
}

//===========================================
// 时钟就绪中断 API
//===========================================

/**
 * @brief 启用RCH就绪中断
 * 
 * @param[in] callback RCH就绪时的回调函数
 * 
 * @note 需要在NVIC中使能CLKRDY_INT_IRQn中断
 */
void clock_rch_ready_irq_enable(clock_ready_callback_t callback)
{
    clock_state.rch_ready_cb = callback;
    RCC->CIR |= (1U << 10);  // RCHRDYIE
}

/**
 * @brief 禁用RCH就绪中断
 */
void clock_rch_ready_irq_disable(void)
{
    RCC->CIR &= ~(1U << 10);  // RCHRDYIE
    clock_state.rch_ready_cb = NULL;
}

/**
 * @brief 启用XTH就绪中断
 * 
 * @param[in] callback XTH就绪时的回调函数
 * 
 * @note 需要在NVIC中使能CLKRDY_INT_IRQn中断
 */
void clock_xth_ready_irq_enable(clock_ready_callback_t callback)
{
    clock_state.xth_ready_cb = callback;
    RCC->CIR |= (1U << 11);  // XTHRDYIE
}

/**
 * @brief 禁用XTH就绪中断
 */
void clock_xth_ready_irq_disable(void)
{
    RCC->CIR &= ~(1U << 11);  // XTHRDYIE
    clock_state.xth_ready_cb = NULL;
}

/**
 * @brief 启用XTL就绪中断
 * 
 * @param[in] callback XTL就绪时的回调函数
 * 
 * @note 需要在NVIC中使能CLKRDY_INT_IRQn中断
 */
void clock_xtl_ready_irq_enable(clock_ready_callback_t callback)
{
    clock_state.xtl_ready_cb = callback;
    RCC->CIR |= (1U << 9);  // XTLRDYIE
}

/**
 * @brief 禁用XTL就绪中断
 */
void clock_xtl_ready_irq_disable(void)
{
    RCC->CIR &= ~(1U << 9);  // XTLRDYIE
    clock_state.xtl_ready_cb = NULL;
}

/**
 * @brief 启用RCL就绪中断
 * 
 * @param[in] callback RCL就绪时的回调函数
 * 
 * @note 需要在NVIC中使能CLKRDY_INT_IRQn中断
 */
void clock_rcl_ready_irq_enable(clock_ready_callback_t callback)
{
    clock_state.rcl_ready_cb = callback;
    RCC->CIR |= (1U << 8);  // RCLRDYIE
}

/**
 * @brief 禁用RCL就绪中断
 */
void clock_rcl_ready_irq_disable(void)
{
    RCC->CIR &= ~(1U << 8);  // RCLRDYIE
    clock_state.rcl_ready_cb = NULL;
}

/**
 * @brief 启用PLL1锁定中断
 * 
 * @param[in] callback PLL1锁定时的回调函数
 * 
 * @note 需要在NVIC中使能CLKRDY_INT_IRQn中断
 */
void clock_pll1_lock_irq_enable(clock_ready_callback_t callback)
{
    clock_state.pll1_lock_cb = callback;
    RCC->CIR |= (1U << 12);  // PLL1LOCKIE
}

/**
 * @brief 禁用PLL1锁定中断
 */
void clock_pll1_lock_irq_disable(void)
{
    RCC->CIR &= ~(1U << 12);  // PLL1LOCKIE
    clock_state.pll1_lock_cb = NULL;
}



//===========================================
// 高级功能 — 外设复位控制
//===========================================

/**
 * @brief 复位外设
 * 
 * 对指定外设执行复位操作
 * 
 * @param[in] periph 外设标识符
 * 
 * @note 复位序列：写1 -> 写0 -> 写1
 * @note 写0表示进入复位，写1表示释放复位
 * @note 复位会清除外设的所有寄存器状态
 */
void clock_periph_reset(clock_periph_t periph)
{
    uint8_t reg_idx = (periph >> 8) & 0xF;
    uint8_t bit_pos = periph & 0x1F;
    
    if (reg_idx < 6) {
        // 写0复位，写1释放
        *clock_rstr_regs[reg_idx] |= (1U << bit_pos);   // 先写1释放
        delay_cycles(10);
        *clock_rstr_regs[reg_idx] &= ~(1U << bit_pos);  // 写0复位
        delay_cycles(10);
        *clock_rstr_regs[reg_idx] |= (1U << bit_pos);   // 写1释放
    }
}

/**
 * @brief 批量复位多个外设
 * 
 * @param[in] periphs 外设标识符数组
 * @param[in] count 数组元素个数
 */
void clock_periph_reset_multi(const clock_periph_t *periphs, size_t count)
{
    assert(periphs != NULL);
    
    for (size_t i = 0; i < count; i++) {
        clock_periph_reset(periphs[i]);
    }
}

//===========================================
// 高级功能 — 系统复位控制
//===========================================

/**
 * @brief 系统软件复位
 * 
 * 触发软件复位，重启整个系统
 * 
 * @note 此函数不会返回
 * @warning 复位会丢失所有RAM数据
 */
void clock_system_reset(void)
{
    // 软件复位
    RCC->RCR &= ~(1U << 31);  // SRST写0触发复位
    
    while (1) {
        __NOP();  // 等待复位发生
    }
}

/**
 * @brief 获取复位源标志
 * 
 * 读取导致系统复位的原因
 * 
 * @return uint32_t 复位源标志位掩码
 * 
 * @note 标志位在下次复位前保持，需手动清除
 * @see clock_clear_reset_flags()
 */
uint32_t clock_get_reset_source(void)
{
    return RCC->RSR & 0x7FF;  // [10:0]，bit7-6 为保留位（读出始终为0）
}

/**
 * @brief 清除复位源标志
 * 
 * 清除RSR寄存器中的所有复位标志位
 */
void clock_clear_reset_flags(void)
{
    RCC->RSR |= (1U << 16);  // RSTFLAGCLR
}

/**
 * @brief 使能/禁用复位源
 *
 * 控制指定复位源是否能触发系统复位
 *
 * @param[in] source 复位源类型（仅 LVD/WDT/IWDT/LOCKUP 可配置使能，
 *                   SYSREQ/SOFT 为只读复位源标志）
 *                   - RESET_SRC_LVD: 低压检测复位
 *                   - RESET_SRC_WDT: 窗口看门狗复位
 *                   - RESET_SRC_IWDT: 独立看门狗复位
 *                   - RESET_SRC_LOCKUP: CPU锁定复位
 * @param[in] enable true=使能，false=禁用
 */
void clock_reset_source_enable(reset_source_t source, bool enable)
{
    // 根据复位源类型配置RCC_RCR
    if (source == RESET_SRC_LVD) {
        if (enable) {
            RCC->RCR |= (1U << 0);  // LVDRSTEN
        } else {
            RCC->RCR &= ~(1U << 0);
        }
    } else if (source == RESET_SRC_WDT) {
        if (enable) {
            RCC->RCR |= (1U << 1);  // WDTRSTEN
        } else {
            RCC->RCR &= ~(1U << 1);
        }
    } else if (source == RESET_SRC_IWDT) {
        if (enable) {
            RCC->RCR |= (1U << 2);  // IWDTRSTEN
        } else {
            RCC->RCR &= ~(1U << 2);
        }
    } else if (source == RESET_SRC_LOCKUP) {
        if (enable) {
            RCC->RCR |= (1U << 3);  // LOCKRSTEN
        } else {
            RCC->RCR &= ~(1U << 3);
        }
    }
}

//===========================================
// 高级功能 — 外设时钟源配置
//===========================================

/**
 * @brief 配置LPUART时钟源
 * 
 * @param[in] src 时钟源选择
 *                - LPUART_CLK_PCLK1: PCLK1分频
 *                - LPUART_CLK_RCL: 内部低速RC
 *                - LPUART_CLK_XTL: 外部低速晶振
 * @param[in] div 分频系数 (4/8/16/32)，仅当src=PCLK1时有效
 * 
 * @note 使用RCL/XTL时钟源可实现超低功耗串口通信
 */
void clock_lpuart_set_source(lpuart_clk_src_t src, uint8_t div)
{
    uint32_t percfgr = RCC->PERCFGR;
    
    // 配置时钟源 (bit 27:26)
    percfgr &= ~(0x3 << 26);
    percfgr |= ((src & 0x3) << 26);
    
    // 配置分频 (bit 25:24, 仅当src=PCLK1时有效)
    if (src == LPUART_CLK_PCLK1) {
        percfgr &= ~(0x3 << 24);
        if (div == 4) percfgr |= (0x0 << 24);
        else if (div == 8) percfgr |= (0x1 << 24);
        else if (div == 16) percfgr |= (0x2 << 24);
        else percfgr |= (0x3 << 24);  // 32
    }
    
    RCC->PERCFGR = percfgr;
}

/**
 * @brief 配置LPTIM时钟源
 *
 * @param[in] lptim LPTIM编号 (仅支持1)
 * @param[in] src 时钟源选择
 *                - LPTIM_CLK_PCLK: APB时钟
 *                - LPTIM_CLK_RCL: 内部低速RC
 *                - LPTIM_CLK_RCH: RCH (64MHz)
 *                - LPTIM_CLK_XTL: 外部低速晶振
 *
 * @note P4 仅 LPTIM1 有时钟源选择位（PERCFGR[17:16]），LPTIM2-6 不支持独立时钟源配置
 */
void clock_lptim_set_source(uint8_t lptim, lptim_clk_src_t src)
{
    assert(lptim == 1);

    uint32_t percfgr = RCC->PERCFGR;

    percfgr &= ~(0x3 << 16);             // LPTIM1CKS [17:16]
    percfgr |= ((src & 0x3) << 16);

    RCC->PERCFGR = percfgr;
}

/**
 * @brief 配置SDMMC时钟源
 * 
 * @param[in] src 时钟源选择
 *                - SDMMC_CLK_HCLK: HCLK
 *                - SDMMC_CLK_PLLQCLK: PLL Q输出
 * 
 * @note SDMMC需要稳定的时钟源以确保数据传输可靠性
 */
void clock_sdmmc_set_source(sdmmc_clk_src_t src)
{
    if (src == SDMMC_CLK_PLLQCLK) {
        RCC->PERCFGR |= (1U << 11);  // SDMMCCKS
    } else {
        RCC->PERCFGR &= ~(1U << 11);
    }
}

/**
 * @brief 配置SDMMC采样时钟源
 * 
 * 设置SDMMC数据采样时钟的来源
 * 
 * @param[in] src 采样时钟源选择 (0-3)
 * 
 * @note 采样时钟影响SDMMC数据采样时序
 */
void clock_sdmmc_set_sample_source(uint8_t src)
{
    uint32_t percfgr = RCC->PERCFGR;
    percfgr &= ~(0x3 << 8);
    percfgr |= ((src & 0x3) << 8);
    RCC->PERCFGR = percfgr;
}

/**
 * @brief 配置ADC时钟源
 * 
 * @param[in] src ADC时钟源选择
 *                - ADC_CLK_RCH: 内部RC振荡器 (64MHz)
 *                - ADC_CLK_PLLQCLK: PLL Q输出
 * 
 * @note 覆盖寄存器：RCC_PERCFGR.ADCCKS[12]
 * @note ADC时钟不超过75MHz，使用PLLQCLK时需确保分频后不超限
 */
void clock_adc_set_source(adc_clk_src_t src)
{
    if (src == ADC_CLK_PLLQCLK) {
        RCC->PERCFGR |= (1U << 12);   // ADCCKS = 1 (PLLQCLK)
    } else {
        RCC->PERCFGR &= ~(1U << 12);  // ADCCKS = 0 (RCH)
    }
}

/**
 * @brief 设置LCD时钟分频
 * 
 * @param[in] div 分频系数 (2/4/8/16)
 * 
 * @note LCD时钟影响显示刷新率
 */
void clock_lcd_set_divider(uint8_t div)
{
    (void)div;  // P4 无 DCKCFG 寄存器
}

/**
 * @brief 配置RTC时钟源
 * 
 * @param[in] use_xtl true=使用XTL(32.768kHz)，false=使用RCL(32kHz)
 * 
 * @note RTC时钟源选择后不应频繁更改
 * @note XTL精度高，适合长期计时；RCL无需外部晶振但精度较低
 */
void clock_rtc_set_source(bool use_xtl)
{
    uint32_t ctrl = RCC->STDBYCTRL;
    
    // 使能RTC时钟
    ctrl |= (1U << 22);  // RTCEN
    
    // 配置时钟源
    ctrl &= ~(0x3 << 20);
    if (use_xtl) {
        ctrl |= (0x1 << 20);  // XTL
    } else {
        ctrl |= (0x0 << 20);  // RCL
    }
    
    RCC->STDBYCTRL = ctrl;
}

//===========================================
// 高级功能 — MCO / SSC 配置
//===========================================

/**
 * @brief 配置PLL1扩频时钟（SSC）
 * 
 * 启用或禁用PLL1的扩频功能，用于降低EMI
 * 
 * @param[in] config 扩频配置参数
 *                   - enabled: 是否启用扩频
 *                   - step: 扩频步进值 (0-32767)
 *                   - period: 扩频周期 (0-8191)
 *                   - center_mode: true=中心扩频，false=向下扩频
 * @return true 配置成功
 * @return false 配置失败
 * 
 * @note 扩频可有效降低EMI，但会略微增加时钟抖动
 * @note 扩频幅度 = (step * Fvco) / (period * 2^15)
 */
bool clock_pll1_ssc_configure(const pll_ssc_config_t *config)
{
    if (!config->enabled) {
        RCC->PLL1SCR &= ~0x1;  // PLL1SSCEN
        return true;
    }
    
    // 配置扩频参数
    uint32_t scr = 0;
    scr |= ((uint32_t)(config->step & 0x7FFF) << 17);     // PLLSSCSTP [31:17]
    scr |= ((uint32_t)(config->period & 0x1FFF) << 4);    // PLLSSCPER [16:4] (13位)
    scr |= (config->center_mode ? 0 : (1U << 1));  // PLLSSCMD
    scr |= 0x1;  // PLLSSCEN
    
    RCC->PLL1SCR = scr;
    return true;
}

/**
 * @brief 配置MCO1时钟输出
 * 
 * 配置MCO1引脚输出指定时钟源，用于外部观测或提供时钟
 * 
 * @param[in] src 时钟源选择（参见mco_source_t枚举）
 * @param[in] div 分频系数 (1-65536)
 * 
 * @note MCO1输出频率 = 时钟源频率 / div
 * @note 需要配置对应GPIO引脚为MCO1复用功能
 * @note MCO1和MCO2共享MCOCLKS时钟源选择，后配置的会覆盖时钟源
 */
void clock_mco1_configure(mco_source_t src, uint32_t div)
{
    if (div < 1) { div = 1; }
    if (div > 65536) { div = 65536; }
    
    uint32_t clkocr = RCC->CLKOCR;
    
    // 配置时钟源 MCOCLKS[4:0] - MCO1和MCO2共享
    clkocr &= ~(0x1F << 0);
    clkocr |= ((src & 0x1F) << 0);
    
    // 配置分频 MCO1DIV[21:6] - 16位
    clkocr &= ~(0xFFFF << 6);
    clkocr |= (((div - 1) & 0xFFFF) << 6);
    
    // 使能MCO1预分频器选择 MCO1SEL[5]
    clkocr |= (1U << 5);
    
    // 使能MCO1输出 MCO1EN[23]
    clkocr |= (1U << 23);
    
    RCC->CLKOCR = clkocr;
}

/**
 * @brief 配置MCO2时钟输出
 * 
 * 配置MCO2引脚输出指定时钟源
 * 
 * @param[in] src 时钟源选择（参见mco_source_t枚举）
 * @param[in] div 分频系数 (1-64)
 * 
 * @note MCO2输出频率 = 时钟源频率 / div
 * @note 需要配置对应GPIO引脚为MCO2复用功能
 * @note MCO1和MCO2共享MCOCLKS时钟源选择，后配置的会覆盖时钟源
 */
void clock_mco2_configure(mco_source_t src, uint8_t div)
{
    if (div < 1) { div = 1; }
    if (div > 64) { div = 64; }
    
    uint32_t clkocr = RCC->CLKOCR;
    
    // 配置时钟源 MCOCLKS[4:0] - MCO1和MCO2共享
    clkocr &= ~(0x1F << 0);
    clkocr |= ((src & 0x1F) << 0);
    
    // 配置分频 MCO2DIV[29:24] - 6位
    clkocr &= ~(0x3F << 24);
    clkocr |= (((div - 1) & 0x3F) << 24);
    
    // 使能MCO2输出 MCO2EN[31]
    clkocr |= (1U << 31);
    
    RCC->CLKOCR = clkocr;
}

/**
 * @brief 禁用MCO时钟输出
 * 
 * @param[in] mco MCO编号 (1或2)
 */
void clock_mco_disable(uint8_t mco)
{
    if (mco == 1) {
        RCC->CLKOCR &= ~(1U << 23);  // MCO1EN
    } else if (mco == 2) {
        RCC->CLKOCR &= ~(1U << 31);  // MCO2EN
    }
}

//===========================================
// 中断处理（可选）
//===========================================

/**
 * @brief XTH停振检测中断处理函数 (NMI)
 * 
 * @note XTH停振通过NMI触发（不可屏蔽中断）
 * @note 需要在启动文件中声明
 */
void NMI_Handler(void)
{
    uint32_t cir = RCC->CIR;
    
    // XTH停振中断
    if (cir & (1U << 27)) {  // XTHSDF
        RCC->CIR = (1U << 25);  // XTHSDIC - 清除中断
        if (clock_state.xth_css_cb) {
            clock_state.xth_css_cb();
        }
    }
}

/**
 * @brief XTL停振检测中断处理函数
 * 
 * @note 对应中断号 RTC_XTLSD_IRQn (2)
 * @note 需要在启动文件中声明并注册
 */
void RTC_XTLSD_IRQHandler(void)
{
    uint32_t cir = RCC->CIR;

    // XTL停振中断
    if (cir & (1U << 31)) {  // XTLSDF
        RCC->CIR = (1U << 29);  // XTLSDIC - 清除中断
        if (clock_state.xtl_css_cb) {
            clock_state.xtl_css_cb();
        }
    }

    // RTC模块中断（闹钟、周期唤醒、侵入、唤醒定时器等）
    if (clock_state.rtc_irq_cb) {
        clock_state.rtc_irq_cb();
    }
}

/**
 * @brief 时钟就绪中断处理函数
 * 
 * @note 对应中断号 CLKRDY_INT_IRQn (3)
 * @note 处理：时钟就绪中断（RCL/XTL/RCH/XTH）+ PLL锁定中断（PLL1/2/3）
 * @note 需要在启动文件中声明并注册
 */
void CLKRDY_INT_IRQHandler(void)
{
    uint32_t cir = RCC->CIR;
    
    // ===== 时钟就绪中断 =====
    
    // RCL就绪中断
    if (cir & (1U << 0)) {  // RCLRDYIF
        RCC->CIR = (1U << 16);  // RCLRDYIC - 清除中断
        if (clock_state.rcl_ready_cb) {
            clock_state.rcl_ready_cb();
        }
    }
    
    // XTL就绪中断
    if (cir & (1U << 1)) {  // XTLRDYIF
        RCC->CIR = (1U << 17);  // XTLRDYIC - 清除中断
        if (clock_state.xtl_ready_cb) {
            clock_state.xtl_ready_cb();
        }
    }
    
    // RCH就绪中断
    if (cir & (1U << 2)) {  // RCHRDYIF
        RCC->CIR = (1U << 18);  // RCHRDYIC - 清除中断
        if (clock_state.rch_ready_cb) {
            clock_state.rch_ready_cb();
        }
    }
    
    // XTH就绪中断
    if (cir & (1U << 3)) {  // XTHRDYIF
        RCC->CIR = (1U << 19);  // XTHRDYIC - 清除中断
        if (clock_state.xth_ready_cb) {
            clock_state.xth_ready_cb();
        }
    }
    
    // ===== PLL锁定中断 =====
    
    // PLL1锁定中断
    if (cir & (1U << 4)) {  // PLLLOCKIF
        RCC->CIR = (1U << 20);  // PLLLOCKIC - 清除中断
        if (clock_state.pll1_lock_cb) {
            clock_state.pll1_lock_cb();
        }
    }
}
