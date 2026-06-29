/**
 * @file clocks.h
 * @brief ACM32P4 时钟管理驱动
 *
 * @note 设计目标：
 *   - 简洁优先：一行代码完成系统时钟初始化
 *   - 统一接口：所有外设使用相同的时钟控制API
 *   - 类型安全：枚举类型防止参数错误
 *   - 零配置：智能默认值，无需配置文件
 *
 * @note 命名说明：
 *   - 代码中 PLL1CR/PLL1CFR/PLL1SCR 对应手册中的 PLLCR/PLLCFR/PLLSCR
 *     （P4 仅有一个 PLL，设备头文件中带"1"后缀，手册文档省略该后缀）
 *
 * @note 中断架构：
 *   1. CLKRDY_INT_IRQn (3) - 时钟就绪 + PLL锁定
 *      - 处理函数：CLKRDY_INT_IRQHandler()
 *      - 使能方法：NVIC_EnableIRQ(CLKRDY_INT_IRQn)
 *   2. NMI (-14) - XTH停振检测 (CSS)
 *      - 处理函数：NMI_Handler()
 *      - 不可屏蔽，无需NVIC配置
 *   3. RTC_XTLSD_IRQn (2) - XTL停振检测
 *      - 处理函数：RTC_XTLSD_IRQHandler()
 *      - 使能方法：NVIC_EnableIRQ(RTC_XTLSD_IRQn)
 *
 * @author weixiaobai1949
 * @version 1.0
 */

#ifndef _HARDWARE_CLOCKS_H
#define _HARDWARE_CLOCKS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "hardware/tim.h"

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

/**
 * @brief 时钟频率常量
 */
#define RCH_FREQ_HZ    64000000U ///< 内部RC振荡器频率 (64MHz)
#define RCL_FREQ_HZ    32000U    ///< 内部低速RC频率 (32KHz)
#define XTL_FREQ_HZ    32768U    ///< 外部低速晶振频率 (32.768KHz)

/**
 * @brief 系统时钟最大频率
 */
#define SYSCLK_MAX_HZ  220000000U ///< 系统时钟最大频率 (220MHz)
#define HCLK_MAX_HZ    220000000U ///< AHB总线最大频率 (220MHz)
#define PCLK_MAX_HZ    180000000U ///< APB总线最大频率 (180MHz)

/**
 * @brief PLL频率范围
 */
#define PLL_VCO_MIN_HZ 100000000U ///< PLL VCO最小频率 (100MHz)
#define PLL_VCO_MAX_HZ 550000000U ///< PLL VCO最大频率 (550MHz)
#define PLL_OUT_MIN_HZ 30000000U  ///< PLL输出最小频率 (30MHz)
#define PLL_OUT_MAX_HZ 275000000U ///< PLL输出最大频率 (275MHz)

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief 系统时钟源选择
 */
typedef enum
{
    SYSCLK_SRC_RCH = 0,  ///< 内部RC振荡器 (64MHz)
    SYSCLK_SRC_XTH = 1,  ///< 外部高速晶振 (4-32MHz)
    SYSCLK_SRC_PLL1 = 2, ///< PLL1输出
} sysclk_src_t;

/**
 * @brief 外设时钟标识符
 *
 * 编码格式：0x RRRR BBBB
 *   RRRR = 寄存器索引 (0=AHB1, 1=AHB2, 2=AHB3, 3=APB1_1, 4=APB1_2, 5=APB2)
 *   BBBB = 位偏移 (0-31)
 */
typedef enum
{
    // AHB1 外设 (RCC_AHB1CKENR)
    CLK_DMA1     = 0x0000, ///< DMA1
    CLK_DMA2     = 0x0001, ///< DMA2
    CLK_CRC      = 0x0005, ///< CRC
    CLK_ETH_MAC  = 0x0006, ///< Ethernet MAC
    CLK_ETH_TX   = 0x0007, ///< Ethernet TX
    CLK_ETH_RX   = 0x0008, ///< Ethernet RX
    CLK_SPI1     = 0x000A, ///< SPI1
    CLK_SPI2     = 0x000B, ///< SPI2
    CLK_SPI3     = 0x000C, ///< SPI3
    CLK_SPI4     = 0x000D, ///< SPI4
    CLK_USB1     = 0x0014, ///< USB OTG1
    CLK_FDCAN1   = 0x0016, ///< FDCAN1
    CLK_FDCAN2   = 0x0017, ///< FDCAN2
    CLK_BKPSRAM  = 0x001B, ///< 备份SRAM

    // AHB2 外设 (RCC_AHB2CKENR)
    CLK_GPIOA    = 0x0100, ///< GPIOA
    CLK_GPIOB    = 0x0101, ///< GPIOB
    CLK_GPIOC    = 0x0102, ///< GPIOC
    CLK_GPIOD    = 0x0103, ///< GPIOD
    CLK_GPIOE    = 0x0104, ///< GPIOE
    CLK_GPIOF    = 0x0105, ///< GPIOF
    CLK_GPIOG    = 0x0106, ///< GPIOG
    CLK_GPIOH    = 0x0107, ///< GPIOH
    CLK_ADC      = 0x0111, ///< ADC
    CLK_DAC1     = 0x0113, ///< DAC1
    CLK_AES_SPI1 = 0x0118, ///< AES SPI1
    CLK_AES      = 0x0119, ///< AES
    CLK_HRNG     = 0x011A, ///< 硬件随机数

    // AHB3 外设 (RCC_AHB3CKENR)
    CLK_QSPI7    = 0x0200, ///< QSPI7
    CLK_SDMMC    = 0x0204, ///< SDMMC
    CLK_OSPI1    = 0x0208, ///< OSPI1
    CLK_OSPI2    = 0x0209, ///< OSPI2

    // APB1 外设1 (RCC_APB1CKENR1)
    CLK_TIM2     = 0x0300, ///< TIM2
    CLK_TIM3     = 0x0301, ///< TIM3
    CLK_TIM4     = 0x0302, ///< TIM4
    CLK_TIM5     = 0x0303, ///< TIM5
    CLK_TIM6     = 0x0304, ///< TIM6
    CLK_TIM7     = 0x0305, ///< TIM7
    CLK_WDT      = 0x030B, ///< WDT
    CLK_I2S1     = 0x030E, ///< I2S1
    CLK_I2S2     = 0x030F, ///< I2S2
    CLK_UART2    = 0x0311, ///< UART2
    CLK_UART3    = 0x0312, ///< UART3
    CLK_UART4    = 0x0313, ///< UART4
    CLK_UART5    = 0x0314, ///< UART5
    CLK_I2C1     = 0x0315, ///< I2C1
    CLK_I2C2     = 0x0316, ///< I2C2
    CLK_PMU      = 0x031B, ///< PMU
    CLK_LPTIM1   = 0x031E, ///< LPTIM1
    CLK_LPUART1  = 0x031F, ///< LPUART1

    // APB1 外设2 (RCC_APB1CKENR2)
    CLK_UART7    = 0x0402, ///< UART7
    CLK_UART8    = 0x0403, ///< UART8
    CLK_TIM26    = 0x0405, ///< TIM26
    CLK_EFUSE1   = 0x0406, ///< EFUSE1

    // APB2 外设 (RCC_APB2CKENR)
    CLK_SYSCFG   = 0x0500, ///< SYSCFG
    CLK_CMP1     = 0x0502, ///< CMP1
    CLK_EXTI     = 0x0504, ///< EXTI
    CLK_TIM1     = 0x0506, ///< TIM1
    CLK_TIM8     = 0x0508, ///< TIM8
    CLK_UART1    = 0x0509, ///< UART1
    CLK_UART6    = 0x050A, ///< UART6
    CLK_TIM9     = 0x0515, ///< TIM9
    CLK_TIM10    = 0x0516, ///< TIM10
} clock_periph_t;

/**
 * @brief 复位源类型
 */
typedef enum
{
    RESET_SRC_LVD = (1U << 0),      ///< 低压检测复位
    RESET_SRC_WDT = (1U << 1),      ///< 系统看门狗复位
    RESET_SRC_IWDT = (1U << 2),     ///< 独立看门狗复位
    RESET_SRC_LOCKUP = (1U << 3),   ///< CPU LOCKUP复位
    RESET_SRC_SYSREQ = (1U << 4),   ///< CPU SYSREQ复位
    RESET_SRC_NRST = (1U << 5),     ///< 外部NRST引脚复位
    RESET_SRC_SOFTWARE = (1U << 8), ///< 软件复位
    RESET_SRC_POR12 = (1U << 9),    ///< POR12复位
    RESET_SRC_POR = (1U << 10),     ///< 上电/欠压复位
} reset_source_t;

/**
 * @brief LPUART时钟源
 */
typedef enum
{
    LPUART_CLK_RCL = 0,   ///< RCL (32KHz)
    LPUART_CLK_XTL = 1,   ///< XTL (32.768KHz)
    LPUART_CLK_PCLK1 = 2, ///< PCLK1分频
} lpuart_clk_src_t;

/**
 * @brief LPTIM时钟源
 */
typedef enum
{
    LPTIM_CLK_PCLK = 0, ///< PCLK (APB总线时钟)
    LPTIM_CLK_RCL = 1,  ///< RCL (32KHz)
    LPTIM_CLK_RCH = 2,  ///< RCH (64MHz)
    LPTIM_CLK_XTL = 3,  ///< XTL (32.768KHz)
} lptim_clk_src_t;

/**
 * @brief SDMMC时钟源
 */
typedef enum
{
    SDMMC_CLK_HCLK = 0,    ///< HCLK
    SDMMC_CLK_PLLQCLK = 1, ///< PLL Q输出
} sdmmc_clk_src_t;

/**
 * @brief ADC时钟源
 */
typedef enum
{
    ADC_CLK_RCH = 0,    ///< RCH (64MHz)
    ADC_CLK_PLLQCLK = 1, ///< PLL Q输出
} adc_clk_src_t;

/**
 * @brief MCO时钟输出源
 */
typedef enum
{
    MCO_SRC_HCLK = 0x00,         ///< HCLK
    MCO_SRC_RCH = 0x01,          ///< RCH
    MCO_SRC_RCL = 0x02,          ///< RCL
    MCO_SRC_XTH = 0x03,          ///< XTH
    MCO_SRC_XTL = 0x04,          ///< XTL
    MCO_SRC_PLL1PCLK = 0x05,     ///< PLL1 P输出
    MCO_SRC_PLL1QCLK = 0x06,     ///< PLL1 Q输出
    MCO_SRC_LPUART2CLK = 0x09,   ///< LPUART2 CLK
    MCO_SRC_SYSCLK = 0x0A,       ///< SYSCLK
    MCO_SRC_LPUART1CLK = 0x0B,   ///< LPUART1 CLK
    MCO_SRC_FCLK_DIV8 = 0x0C,    ///< FCLK/8
    MCO_SRC_USB_SOF = 0x0D,      ///< USB SOF
    MCO_SRC_USBCLK48M = 0x0E,    ///< USB 48MHz
    MCO_SRC_RTCPCLK = 0x0F,      ///< RTC PCLK
    MCO_SRC_SDMMC_SAMPLE = 0x10, ///< SDMMC采样时钟
    MCO_SRC_SDMMC_DRIVE = 0x11,  ///< SDMMC驱动时钟
} mco_source_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief PLL配置结构体
 */
typedef struct
{
    uint8_t src; ///< 时钟源: 0=RCH/16, 1=XTH
    uint8_t n;   ///< 输入分频 (1-64)
    uint16_t f;  ///< 反馈倍频 (50-511)
    uint8_t p;   ///< P输出分频 (2/4/6/8)
    uint8_t q;   ///< Q输出分频 (1-15)
} pll_config_t;

/**
 * @brief PLL扩频配置 (减少EMI)
 */
typedef struct
{
    bool enabled;     ///< 使能扩频
    bool center_mode; ///< true=中心扩频, false=向下扩频
    uint16_t period;  ///< 扩频周期 (>48)
    uint16_t step;    ///< 扩频步长 (<682)
} pll_ssc_config_t;

/**
 * @brief 时钟停振检测回调函数类型
 */
typedef void (*clock_css_callback_t)(void);

/**
 * @brief 时钟就绪中断回调函数类型
 */
typedef void (*clock_ready_callback_t)(void);

/**
 * @brief RTC模块中断回调函数类型
 *
 * RTC_XTLSD_IRQn (INT2) 由 XTL 停振检测和 RTC 模块共用。
 * 通过此回调注册 RTC 模块的中断处理函数（通常为 RTC_IRQHandler）。
 */
typedef void (*clock_rtc_irq_callback_t)(void);

//===========================================
// 全局变量声明 (如果需要)
//===========================================

// 注：实际实现中可能需要一些全局变量，但对外不暴露

//===========================================
// 函数声明
//===========================================

//-------------------------------------------
// 第1层: 快速初始化 API
//-------------------------------------------

/**
 * @brief 初始化系统时钟到默认配置
 *
 * 默认配置：
 *   - 使用内部RCH (64MHz)
 *   - 启动PLL1倍频到180MHz
 *   - SYSCLK = 180MHz
 *   - HCLK = 180MHz
 *   - PCLK1/2 = 90MHz (2分频)
 *
 * @note 一行代码完成所有系统时钟配置
 */
void clock_init_default(void);

/**
 * @brief 使用外部晶振初始化系统时钟
 *
 * @param xtal_freq_hz 外部晶振频率 (支持4-32MHz)
 *
 * 配置：
 *   - 使用XTH外部晶振
 *   - 启动PLL1倍频到180MHz
 *   - SYSCLK = 180MHz
 *   - HCLK = 180MHz
 *   - PCLK = 90MHz (2分频)
 */
void clock_init_xtal(uint32_t xtal_freq_hz);

/**
 * @brief 获取当前系统时钟频率
 * @return 系统时钟频率 (Hz)
 */
uint32_t clock_get_hz(void);

//-------------------------------------------
// 第2层: 基础配置 API — 时钟源控制
//-------------------------------------------

/**
 * @brief 启动内部高速RC振荡器 (RCH)
 * @param div16 是否16分频 (用于PLL输入)
 * @return true=成功, false=超时
 */
bool clock_rch_enable(bool div16);

/**
 * @brief 关闭RCH
 */
void clock_rch_disable(void);

/**
 * @brief 设置RCH频率校准值
 * @param trim 校准值 (0-255)
 * @note 出厂校准值在复位时自动加载，用于温度/电压补偿
 */
void clock_rch_set_trim(uint8_t trim);

/**
 * @brief 读取当前RCH频率校准值
 * @return 校准值 (0-255)
 */
uint8_t clock_rch_get_trim(void);

/**
 * @brief 启动外部高速晶振 (XTH)
 * @param freq_hz 晶振频率 (4-32MHz)
 * @param bypass 是否旁路模式 (外部方波输入)
 * @return true=成功, false=超时/停振
 */
bool clock_xth_enable(uint32_t freq_hz, bool bypass);

/**
 * @brief 关闭XTH
 */
void clock_xth_disable(void);

/**
 * @brief 启动外部低速晶振 (XTL, 32.768KHz)
 * @param bypass 是否旁路模式
 * @return true=成功, false=超时/停振
 */
bool clock_xtl_enable(bool bypass);

/**
 * @brief 关闭XTL
 */
void clock_xtl_disable(void);

/**
 * @brief 启动内部低速RC (RCL, 32KHz)
 * @return true=成功, false=超时
 */
bool clock_rcl_enable(void);

/**
 * @brief 设置RCL频率校准值
 * @param trim 校准值 (0-255)
 * @note 出厂校准值在复位时自动加载，用于温度/电压补偿
 */
void clock_rcl_set_trim(uint8_t trim);

/**
 * @brief 读取当前RCL频率校准值
 * @return 校准值 (0-255)
 */
uint8_t clock_rcl_get_trim(void);

//-------------------------------------------
// 基础配置 — PLL配置
//-------------------------------------------

/**
 * @brief 配置并启动PLL1 (主PLL)
 *
 * @param config PLL配置参数
 * @return true=成功, false=配置错误/超时
 *
 * @note 计算公式：
 *   F_pfd = F_in / N (1-2MHz)
 *   F_vco = F_pfd * F (100-550MHz)
 *   F_pclk = F_vco / P (30-275MHz)
 *   F_qclk = F_vco / Q (16-550MHz)
 */
bool clock_pll1_configure(const pll_config_t* config);

/**
 * @brief 使用简化参数配置PLL1
 *
 * @param target_freq_hz 目标系统频率 (Hz)
 * @return true=成功, false=无法达到目标频率
 *
 * @note 自动计算最优N/F/P参数
 */
bool clock_pll1_configure_simple(uint32_t target_freq_hz);

/**
 * @brief 关闭PLL1
 */
void clock_pll1_disable(void);

//-------------------------------------------
// 基础配置 — 系统时钟切换与分频
//-------------------------------------------

/**
 * @brief 切换系统时钟源
 *
 * @param src 目标时钟源
 * @return true=切换成功, false=时钟源未就绪
 *
 * @note 自动处理切换顺序，确保安全
 */
bool clock_set_sysclk_source(sysclk_src_t src);

/**
 * @brief 读取当前系统时钟源
 * @return 当前使用的系统时钟源
 */
sysclk_src_t clock_get_sysclk_source(void);

/**
 * @brief 配置系统时钟分频
 *
 * @param sysdiv0 第一级分频 (1-16)
 * @param sysdiv1 第二级分频 (1-16)
 *
 * @note 生成HCLK = SYSCLK / (sysdiv0 * sysdiv1)
 */
void clock_set_sys_divider(uint8_t sysdiv0, uint8_t sysdiv1);

/**
 * @brief 配置APB时钟分频
 *
 * @param pclk1_div APB1分频 (1/2/4/8/16)
 * @param pclk2_div APB2分频 (1/2/4/8/16)
 */
void clock_set_apb_divider(uint8_t pclk1_div, uint8_t pclk2_div);

//-------------------------------------------
// 第4层: 控制与查询 API — 频率查询
//-------------------------------------------

/**
 * @brief 获取HCLK频率 (AHB总线时钟)
 * @return HCLK频率 (Hz)
 */
uint32_t clock_get_hclk_hz(void);

/**
 * @brief 获取PCLK1频率 (APB1总线时钟)
 * @return PCLK1频率 (Hz)
 */
uint32_t clock_get_pclk1_hz(void);

/**
 * @brief 获取PCLK2频率
 * @return PCLK2频率 (Hz)
 */
uint32_t clock_get_pclk2_hz(void);

/**
 * @brief 获取定时器时钟频率
 *
 * @param timer 定时器实例（tim_instance_t 枚举）
 * @return 定时器时钟频率 (Hz)
 *
 * @note 定时器时钟 = APB时钟 * 2 (当APB有分频时)
 */
uint32_t clock_get_timer_hz(uint32_t timer);

//-------------------------------------------
// 第3层: 高级功能 API — 外设时钟使能与时钟源配置
//-------------------------------------------

/**
 * @brief 使能外设时钟
 *
 * @param periph 外设时钟标识符
 *
 * @note 自动根据标识符操作对应的RCC寄存器位
 *
 * 示例：
 *   clock_periph_enable(CLK_GPIOA);
 *   clock_periph_enable(CLK_UART1);
 */
void clock_periph_enable(clock_periph_t periph);

/**
 * @brief 禁用外设时钟
 *
 * @param periph 外设时钟标识符
 *
 * @warning 禁用后访问该外设寄存器会导致总线错误
 */
void clock_periph_disable(clock_periph_t periph);

/**
 * @brief 批量使能多个外设时钟
 *
 * @param periphs 外设时钟标识符数组
 * @param count 数组长度
 */
void clock_periph_enable_multi(const clock_periph_t* periphs, size_t count);

/**
 * @brief 检查外设时钟是否已使能
 *
 * @param periph 外设时钟标识符
 * @return true=已使能, false=未使能
 */
bool clock_periph_is_enabled(clock_periph_t periph);

/**
 * @brief 配置SDMMC时钟源
 *
 * @param src 时钟源 (HCLK或PLLQCLK)
 *
 * @note 覆盖寄存器：RCC_PERCFGR.SDMMCCKS
 */
void clock_sdmmc_set_source(sdmmc_clk_src_t src);

/**
 * @brief 配置ADC时钟源
 *
 * @param src 时钟源 (RCH或PLLQCLK)
 *
 * @note 覆盖寄存器：RCC_PERCFGR.ADCCKS[12]
 * @note ADC时钟不超过75MHz
 */
void clock_adc_set_source(adc_clk_src_t src);

/**
 * @brief 配置SDMMC采样时钟源
 *
 * @param src 采样时钟选择：
 *   0 = SDMMC控制器时钟（经过延时单元）
 *   1 = SD卡1外部驱动器反馈时钟
 *   2 = SD卡2外部驱动器反馈时钟
 *   3 = SDMMC控制器时钟（未经延时单元）
 */
void clock_sdmmc_set_sample_source(uint8_t src);

/**
 * @brief 配置LCD时钟分频
 *
 * @param div 分频系数 (2/4/8/16)
 *
 * @note P4 无 DCKCFG 寄存器，LCD 时钟分频配置不支持（空函数，参数被忽略）
 */
void clock_lcd_set_divider(uint8_t div);

/**
 * @brief 配置RTC时钟源
 * @param use_xtl true=使用XTL, false=使用RCL
 */
void clock_rtc_set_source(bool use_xtl);

//-------------------------------------------
// 高级功能 — MCO / SSC
//-------------------------------------------

/**
 * @brief 配置PLL1扩频
 * @param config 扩频配置
 * @return true=成功, false=参数错误
 */
bool clock_pll1_ssc_configure(const pll_ssc_config_t* config);

/**
 * @brief 配置MCO1时钟输出
 * @param src 时钟源
 * @param div 分频系数 (1-65536)
 */
void clock_mco1_configure(mco_source_t src, uint32_t div);

/**
 * @brief 配置MCO2时钟输出
 * @param src 时钟源
 * @param div 分频系数 (1-64)
 */
void clock_mco2_configure(mco_source_t src, uint8_t div);

/**
 * @brief 关闭MCO输出
 * @param mco MCO编号 (1-2)
 */
void clock_mco_disable(uint8_t mco);

//-------------------------------------------
// 高级功能 — 复位控制
//-------------------------------------------

/**
 * @brief 复位外设模块
 *
 * @param periph 外设时钟标识符（与clock_periph_t相同）
 *
 * @note 复位位低电平有效：0=复位, 1=不复位
 * @note 复位外设会清除其寄存器为默认值
 */
void clock_periph_reset(clock_periph_t periph);

/**
 * @brief 批量复位多个外设
 * @param periphs 外设标识符数组
 * @param count 数组长度
 */
void clock_periph_reset_multi(const clock_periph_t* periphs, size_t count);

/**
 * @brief 软件复位系统
 *
 * @note 此函数不会返回
 */
void clock_system_reset(void) __attribute__((noreturn));

/**
 * @brief 获取复位源
 *
 * @return 复位源标志位（可能有多个，需要用掩码判断）
 */
uint32_t clock_get_reset_source(void);

/**
 * @brief 清除复位源标志
 *
 * @note 清除后才能正确判断下次复位原因
 */
void clock_clear_reset_flags(void);

/**
 * @brief 使能复位源
 *
 * @param source 复位源类型
 * @param enable true=使能, false=禁用
 */
void clock_reset_source_enable(reset_source_t source, bool enable);

/**
 * @brief 配置LPUART时钟源
 * @param src 时钟源
 * @param div PCLK1分频系数 (4/8/16/32, 仅当src=LPUART_CLK_PCLK1时有效)
 */
void clock_lpuart_set_source(lpuart_clk_src_t src, uint8_t div);

/**
 * @brief 配置LPTIM时钟源
 * @param lptim LPTIM编号
 * @param src 时钟源
 */
void clock_lptim_set_source(uint8_t lptim, lptim_clk_src_t src);

//-------------------------------------------
// 时钟安全系统 (CSS) API
//-------------------------------------------

void clock_xth_css_enable(void);
void clock_xth_css_disable(void);
void clock_xtl_css_enable(void);
void clock_xtl_css_disable(void);
void clock_set_xth_css_callback(clock_css_callback_t callback);
void clock_set_xtl_css_callback(clock_css_callback_t callback);
void clock_set_rtc_irq_callback(clock_rtc_irq_callback_t callback);

//-------------------------------------------
// 时钟就绪中断 API
//-------------------------------------------

void clock_rch_ready_irq_enable(clock_ready_callback_t callback);
void clock_rch_ready_irq_disable(void);
void clock_xth_ready_irq_enable(clock_ready_callback_t callback);
void clock_xth_ready_irq_disable(void);
void clock_xtl_ready_irq_enable(clock_ready_callback_t callback);
void clock_xtl_ready_irq_disable(void);
void clock_rcl_ready_irq_enable(clock_ready_callback_t callback);
void clock_rcl_ready_irq_disable(void);
void clock_pll1_lock_irq_enable(clock_ready_callback_t callback);
void clock_pll1_lock_irq_disable(void);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_CLOCKS_H
