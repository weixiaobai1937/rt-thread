/**
 * @file pmu.h
 * @brief ACM32P4 电源管理单元（PMU）驱动头文件
 * 
 * 本文件提供了ACM32P4芯片的PMU功能接口，包括：
 * - 低功耗模式控制（SLEEP、STOP、STANDBY）
 * - 唤醒源管理（WAKEUP引脚、RTC、IWDT）
 * - 电源监控（BOR、LVD）
 * - 待机域IO控制
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_PMU_H
#define _HARDWARE_PMU_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

// 注：PMU寄存器基地址(PMU_BASE)和结构体定义(PMU_TypeDef)在 device/acm32p4xx.h 中
// PMU寄存器访问：PMU->CTRL0, PMU->CTRL1, PMU->CTRL2, PMU->CTRL3, PMU->SR, PMU->STCLR, PMU->IOSEL

// 默认超时值
#define PMU_BOR_STABLE_US       1U     ///< BOR稳定时间（微秒）
#define PMU_LVD_STABLE_US       2U     ///< LVD稳定时间（微秒）

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief 低功耗模式枚举
 */
typedef enum {
    PMU_MODE_SLEEP    = 0,  ///< 睡眠模式：CPU停止，外设工作
    PMU_MODE_STOP     = 1,  ///< 停止模式：RCH可选关闭，内核LDO可调压
    PMU_MODE_STANDBY  = 2,  ///< 待机模式：内核域掉电，待机域保持
} pmu_lowpower_mode_t;

/**
 * @brief 唤醒引脚枚举（位掩码）
 */
typedef enum {
    PMU_WAKEUP1 = (1U << 0),  ///< WKUP1 (PA0)
    PMU_WAKEUP2 = (1U << 1),  ///< WKUP2 (PA2)
    PMU_WAKEUP3 = (1U << 2),  ///< WKUP3 (PE6)
    PMU_WAKEUP4 = (1U << 3),  ///< WKUP4 (PC13)
    PMU_WAKEUP5 = (1U << 4),  ///< WKUP5 (PB5)
    PMU_WAKEUP6 = (1U << 5),  ///< WKUP6 (PC5)
    PMU_WAKEUP7 = (1U << 6),  ///< WKUP7 (PB15)
    PMU_WAKEUP8 = (1U << 7),  ///< WKUP8 (PF8)
} pmu_wakeup_pin_t;

/**
 * @brief 唤醒源枚举（位掩码）
 */
typedef enum {
    PMU_WAKEUP_SOURCE_PIN   = (1U << 0),  ///< WAKEUP引脚唤醒
    PMU_WAKEUP_SOURCE_RTC   = (1U << 1),  ///< RTC唤醒
    PMU_WAKEUP_SOURCE_IWDT  = (1U << 2),  ///< IWDT唤醒
    PMU_WAKEUP_SOURCE_RSTN  = (1U << 3),  ///< RSTN复位唤醒
    PMU_WAKEUP_SOURCE_BOR   = (1U << 4),  ///< BOR唤醒
} pmu_wakeup_source_t;

/**
 * @brief BOR电压阈值枚举
 */
typedef enum {
    PMU_BOR_LEVEL_2V0  = 0,  ///< 2.0V~2.1V
    PMU_BOR_LEVEL_2V2  = 1,  ///< 2.2V~2.3V
    PMU_BOR_LEVEL_2V5  = 2,  ///< 2.49V~2.61V
    PMU_BOR_LEVEL_2V8  = 3,  ///< 2.77V~2.90V
} pmu_bor_level_t;

/**
 * @brief LVD电压阈值枚举
 */
typedef enum {
    PMU_LVD_LEVEL_1V7  = 0,  ///< 1.71V
    PMU_LVD_LEVEL_2V0  = 1,  ///< 2.01V
    PMU_LVD_LEVEL_2V2  = 2,  ///< 2.23V
    PMU_LVD_LEVEL_2V4  = 3,  ///< 2.43V
    PMU_LVD_LEVEL_2V5  = 4,  ///< 2.51V
    PMU_LVD_LEVEL_2V7  = 5,  ///< 2.73V
    PMU_LVD_LEVEL_2V8  = 6,  ///< 2.80V
    PMU_LVD_LEVEL_2V9  = 7,  ///< 2.90V
} pmu_lvd_level_t;

/**
 * @brief LVD滤波时间枚举
 */
typedef enum {
    PMU_LVD_FILTER_1_CYCLE    = 0,  ///< 1个周期
    PMU_LVD_FILTER_2_CYCLE    = 1,  ///< 2个周期
    PMU_LVD_FILTER_4_CYCLE    = 2,  ///< 4个周期
    PMU_LVD_FILTER_16_CYCLE   = 3,  ///< 16个周期
    PMU_LVD_FILTER_64_CYCLE   = 4,  ///< 64个周期
    PMU_LVD_FILTER_256_CYCLE  = 5,  ///< 256个周期
    PMU_LVD_FILTER_1K_CYCLE   = 6,  ///< 1024个周期
    PMU_LVD_FILTER_4K_CYCLE   = 7,  ///< 4095个周期
} pmu_lvd_filter_t;

/**
 * @brief 待机域IO功能枚举
 */
typedef enum {
    PMU_STBY_IO_GPIO      = 0,  ///< GPIO功能（STANDBY下输入状态）
    PMU_STBY_IO_RTC_FOUT  = 1,  ///< RTC Fout输出（PC13_Value选择输出模式：0=开漏，1=推挽）
    PMU_STBY_IO_TAMPER    = 2,  ///< RTC Tamper输入（PC13）/ 输出关闭（PC14/15，其他值也为关闭）
    PMU_STBY_IO_OUTPUT    = 3,  ///< 输出PCx_Value推挽输出
} pmu_stby_io_func_t;

/**
 * @brief MLDO12电压调整枚举（STOP模式）
 */
typedef enum {
    PMU_MLDO_1V2 = 0,  ///< 保持1.2V
    PMU_MLDO_1V0 = 1,  ///< 调整为1.0V
    PMU_MLDO_0V9 = 2,  ///< 调整为0.9V
    PMU_MLDO_0V8 = 3,  ///< 调整为0.8V
} pmu_mldo_voltage_t;

/**
 * @brief LPLDO12电压调整枚举（STANDBY模式）
 * 
 * @note 仅 1.1V（默认值）有效，其余值保留
 */
typedef enum {
    PMU_LPLDO_1V1 = 4,  ///< 调整为1.1V（默认值）
} pmu_lpldo_voltage_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief STOP模式配置结构体
 */
typedef struct {
    bool rch_auto_off;               ///< STOP模式下RCH是否自动关闭
    bool rch_div16_mode;             ///< STOP模式下RCH是否16分频输出
    bool mldo_lowpower;              ///< STOP模式下MLDO12是否进入低功耗模式
    pmu_mldo_voltage_t mldo_voltage; ///< MLDO12电压调整
    uint16_t wakeup_wait_cycles;     ///< 唤醒等待时间（RC64M时钟周期数，若使能分频则为RC64M/16周期数）
} pmu_stop_config_t;

/**
 * @brief STANDBY模式配置结构体
 */
typedef struct {
    uint32_t wakeup_pins;       ///< 使能的WAKEUP引脚掩码（pmu_wakeup_pin_t组合）
    uint32_t wakeup_polarity;   ///< 唤醒极性：0=高电平，1=低电平（位掩码）
    uint32_t wakeup_filter;     ///< 唤醒滤波使能掩码
    uint8_t wakeup_delay_rtc;   ///< 唤醒延时（RC32K周期数，0~15，0表示无效不等待）
    pmu_lpldo_voltage_t lpldo_voltage; ///< LPLDO12电压调整（仅PMU_LPLDO_1V1有效）
    bool bkp_sram_retain;       ///< 备份SRAM保持状态（true=保持，false=睡眠/数据丢失）
} pmu_standby_config_t;

/**
 * @brief BOR配置结构体
 */
typedef struct {
    bool enabled;             ///< BOR使能
    pmu_bor_level_t level;    ///< BOR电压阈值
    bool reset_enabled;       ///< BOR复位使能
} pmu_bor_config_t;

/**
 * @brief LVD配置结构体
 */
typedef struct {
    bool enabled;               ///< LVD使能
    pmu_lvd_level_t level;      ///< LVD电压阈值
    bool filter_enabled;        ///< 数字滤波使能
    pmu_lvd_filter_t filter_time; ///< 滤波时间
} pmu_lvd_config_t;

/**
 * @brief 待机域IO配置结构体
 */
typedef struct {
    pmu_stby_io_func_t func;  ///< IO功能选择
    bool output_value;        ///< 输出值（当func=PMU_STBY_IO_OUTPUT或PMU_STBY_IO_RTC_FOUT时有效）
    bool output_pushpull;     ///< 输出模式：true=推挽，false=开漏（仅PC13_RTC_FOUT模式有效）
} pmu_stby_io_config_t;

//===========================================
// 第1层: 快速初始化 / 简单控制 API
//===========================================

/**
 * @brief 进入SLEEP模式（最简单）
 * 
 * CPU休眠，外设继续工作，任何中断可唤醒。
 * 
 * @note 此函数使用WFI指令进入SLEEP模式
 * @note 唤醒后继续从下一条指令执行
 * 
 * @return void
 */
void pmu_enter_sleep(void);

/**
 * @brief 进入STOP模式（默认配置）
 * 
 * 使用默认配置进入STOP模式：
 * - RCH不自动关闭
 * - MLDO保持1.2V
 * - 唤醒等待默认值
 * 
 * @note EXTI配置的中断可唤醒
 * @note 唤醒后时钟需重新配置
 * 
 * @return void
 */
void pmu_enter_stop_simple(void);

/**
 * @brief 进入STANDBY模式（默认配置）
 * 
 * 使用默认配置进入STANDBY模式，内核域掉电。
 * 
 * @param[in] wakeup_pins 唤醒引脚掩码（pmu_wakeup_pin_t组合）
 *                        - 0: 仅RTC/IWDT/RSTN可唤醒
 *                        - 非0: 指定的WAKEUP引脚+RTC/IWDT/RSTN可唤醒
 * 
 * @note 唤醒后系统复位，程序从头执行
 * @note 进入前请清除唤醒标志，否则可能立即唤醒
 * 
 * @return void（不会返回）
 */
void pmu_enter_standby_simple(uint32_t wakeup_pins);

//===========================================
// 第2层: 基础配置 API
//===========================================

/**
 * @brief 配置并进入STOP模式
 * 
 * 根据配置参数进入STOP模式。
 * 
 * @param[in] config STOP模式配置参数
 * 
 * @note RCH自动关闭后唤醒时间会增加
 * @note 降低MLDO电压可节省功耗，但可能影响稳定性
 * 
 * @see pmu_get_default_stop_config()
 * 
 * @return void
 */
void pmu_enter_stop(const pmu_stop_config_t *config);

/**
 * @brief 配置并进入STANDBY模式
 * 
 * 根据配置参数进入STANDBY模式。
 * 
 * @param[in] config STANDBY模式配置参数
 * 
 * @note 进入前必须清除唤醒标志
 * @note 备份SRAM保持会增加功耗
 * @note PC13~PC15总驱动能力不超过5mA
 * 
 * @see pmu_get_default_standby_config()
 * 
 * @return void（不会返回）
 */
void pmu_enter_standby(const pmu_standby_config_t *config);

/**
 * @brief 获取默认STOP配置
 * 
 * 填充默认的STOP模式配置参数：
 * - rch_auto_off = false
 * - rch_div16_mode = false
 * - mldo_voltage = PMU_MLDO_1V2
 * - wakeup_wait_cycles = 0x3C0
 * 
 * @param[out] config 输出默认配置
 * 
 * @return void
 */
void pmu_get_default_stop_config(pmu_stop_config_t *config);

/**
 * @brief 获取默认STANDBY配置
 * 
 * 填充默认的STANDBY模式配置参数：
 * - wakeup_pins = 0
 * - wakeup_polarity = 0（高电平唤醒）
 * - wakeup_filter = 0（不滤波）
 * - wakeup_delay_rtc = 10
 * - bkp_sram_retain = false
 * 
 * @param[out] config 输出默认配置
 * 
 * @return void
 */
void pmu_get_default_standby_config(pmu_standby_config_t *config);

/**
 * @brief 配置WAKEUP引脚
 * 
 * 配置WAKEUP引脚的唤醒极性和滤波功能。
 * 
 * @param[in] pins 要配置的引脚掩码（pmu_wakeup_pin_t组合）
 * @param[in] high_active true=高电平唤醒，false=低电平唤醒
 * @param[in] filter_enable 是否使能滤波（使用RTCCLK滤波1拍毛刺）
 * 
 * @note 滤波功能可防止误触发，但会略微增加功耗
 * 
 * @return void
 */
void pmu_wakeup_pin_configure(uint32_t pins, bool high_active, bool filter_enable);

/**
 * @brief 使能WAKEUP引脚
 * 
 * 使能指定的WAKEUP引脚唤醒功能。
 * 
 * @param[in] pins 要使能的引脚掩码（pmu_wakeup_pin_t组合）
 * 
 * @note 使能前请先配置极性和滤波
 * 
 * @see pmu_wakeup_pin_configure()
 * 
 * @return void
 */
void pmu_wakeup_pin_enable(uint32_t pins);

/**
 * @brief 禁用WAKEUP引脚
 * 
 * 禁用指定的WAKEUP引脚唤醒功能。
 * 
 * @param[in] pins 要禁用的引脚掩码（pmu_wakeup_pin_t组合）
 * 
 * @return void
 */
void pmu_wakeup_pin_disable(uint32_t pins);

/**
 * @brief 设置STANDBY唤醒延时
 * 
 * 设置从STANDBY模式唤醒后的稳定延时。
 * 
 * @param[in] rtc_cycles 延时周期数（RC32K时钟，0~15）
 *                       - 0: 无效，不等待
 *                       - 1~15: 等待1~15个RC32K周期
 * 
 * @note 适当的延时可确保电源稳定
 * 
 * @return void
 */
void pmu_set_standby_wakeup_delay(uint8_t rtc_cycles);

//===========================================
// 第3层: 高级功能 API
//===========================================
/**
 * @brief 配置BOR（欠压复位）
 * 
 * 配置BOR功能，包括阈值和复位使能。
 * 
 * @param[in] config BOR配置参数
 * 
 * @note BOR使能后需等待1us稳定
 * @note 关闭BOR前必须先关闭BOR复位
 * @note 使能顺序：BOR使能 -> 等待1us -> BOR复位使能
 * @note 关闭顺序：BOR复位禁用 -> BOR禁用
 * 
 * @warning 不正确的配置可能导致系统无法启动
 */
void pmu_bor_configure(const pmu_bor_config_t *config);

/**
 * @brief 使能BOR
 * 
 * 使能BOR功能并配置阈值。
 * 
 * @param[in] level BOR电压阈值
 * @param[in] reset_enable 是否使能BOR复位
 * 
 * @note 会自动等待BOR稳定时间
 * 
 * @see pmu_bor_configure()
 */
void pmu_bor_enable(pmu_bor_level_t level, bool reset_enable);

/**
 * @brief 禁用BOR
 * 
 * 禁用BOR功能。
 * 
 * @note 会自动先禁用BOR复位再禁用BOR
 * 
 * @return void
 */
void pmu_bor_disable(void);

/**
 * @brief 配置LVD（低压检测）
 * 
 * 配置LVD功能，包括阈值、滤波等参数。
 * 
 * @param[in] config LVD配置参数
 * 
 * @note LVD使能后需等待2us稳定，期间可能误报警
 * @note 在STOP模式用LVD唤醒，需禁止滤波或使用RCL时钟滤波
 * @note LVD中断需在EXTI中配置
 * @note LVD复位需在RCC_RCR中配置
 * 
 * @warning 使能后2us内可能误报警，请忽略
 */
void pmu_lvd_configure(const pmu_lvd_config_t *config);

/**
 * @brief 使能LVD
 * 
 * 使能LVD功能并配置阈值。
 * 
 * @param[in] level LVD电压阈值
 * 
 * @note 会自动等待LVD稳定时间
 * 
 * @see pmu_lvd_configure()
 */
void pmu_lvd_enable(pmu_lvd_level_t level);

/**
 * @brief 禁用LVD
 * 
 * 禁用LVD功能。
 * 
 * @return void
 */
void pmu_lvd_disable(void);

/**
 * @brief 获取LVD原始状态
 * 
 * 读取LVD原始检测状态（未经滤波）。
 * 
 * @return bool LVD状态
 *         - true: 电压低于阈值
 *         - false: 电压正常
 */
bool pmu_lvd_get_status(void);

/**
 * @brief 获取LVD滤波后状态
 * 
 * 读取LVD滤波后的检测状态。
 * 
 * @return bool LVD滤波状态
 *         - true: 电压低于阈值
 *         - false: 电压正常
 * 
 * @note 如果未使能滤波，此函数返回与pmu_lvd_get_status()相同的值
 */
bool pmu_lvd_get_filtered_status(void);

/**
 * @brief 获取BOR比较器原始状态
 * 
 * 读取BOR比较器输出的原始状态（BORN）。
 * 
 * @return bool BOR比较器状态
 *         - true: 电压高于BOR阈值
 *         - false: 电压低于BOR阈值
 * 
 * @note 此位反映BOR比较器的实时输出，不是唤醒标志
 */
bool pmu_bor_get_status(void);

/**
 * @brief 配置待机域IO（PC13）
 * 
 * 配置PC13引脚在STANDBY模式下的功能。
 * 
 * @param[in] config IO配置参数
 * 
 * @note PC13可配置为：GPIO、RTC Fout、Tamper输入、固定输出
 * @note 配置为RTC Fout时，output_pushpull控制输出模式
 * @note PMU_IOSEL优先级高于GPIO配置
 * 
 * @return void
 */
void pmu_stby_io_pc13_configure(const pmu_stby_io_config_t *config);

/**
 * @brief 配置待机域IO（PC14）
 * 
 * 配置PC14引脚在STANDBY模式下的功能。
 * 
 * @param[in] config IO配置参数
 * 
 * @note PC14可配置为：GPIO、固定输出、关闭
 * @note 需要选择XTL_IN时，请配置为模拟端口（GPIO配置）
 * 
 * @return void
 */
void pmu_stby_io_pc14_configure(const pmu_stby_io_config_t *config);

/**
 * @brief 配置待机域IO（PC15）
 * 
 * 配置PC15引脚在STANDBY模式下的功能。
 * 
 * @param[in] config IO配置参数
 * 
 * @note PC15可配置为：GPIO、固定输出、关闭
 * @note 需要选择XTL_OUT时，请配置为模拟端口（GPIO配置）
 * 
 * @return void
 */
void pmu_stby_io_pc15_configure(const pmu_stby_io_config_t *config);

/**
 * @brief 控制备份SRAM状态
 * 
 * 控制备份SRAM在低功耗模式下的状态。
 * 
 * @param[in] retain true=保持状态，false=睡眠状态（数据丢失）
 * 
 * @note 保持状态会增加功耗
 * @note 睡眠状态功耗更低，但数据会丢失
 * @note 同时设置保持和睡眠时，睡眠优先级更高
 * 
 * @return void
 */
void pmu_backup_sram_control(bool retain);

/**
 * @brief 使能RTC模块写操作
 * 
 * 使能对RTC模块的写访问。
 * 
 * @note RTC寄存器写保护，需先调用此函数才能写入
 * 
 * @return void
 */
void pmu_rtc_write_enable(void);

/**
 * @brief 禁用RTC模块写操作
 * 
 * 禁用对RTC模块的写访问。
 * 
 * @note 写入RTC后应调用此函数恢复写保护
 * 
 * @return void
 */
void pmu_rtc_write_disable(void);

//===========================================
// 第4层: 控制与查询 API
//===========================================

/**
 * @brief 获取唤醒源标志
 * 
 * 获取所有唤醒源的状态标志。
 * 
 * @return uint32_t 唤醒源掩码（pmu_wakeup_source_t组合）
 *         - 位0: WAKEUP引脚唤醒
 *         - 位1: RTC唤醒
 *         - 位2: IWDT唤醒
 *         - 位3: RSTN唤醒
 *         - 位4: BOR唤醒
 * 
 * @note 返回值为位掩码，可同时检查多个唤醒源
 * 
 * @see pmu_get_wakeup_pin_flags()
 */
uint32_t pmu_get_wakeup_flags(void);

/**
 * @brief 获取WAKEUP引脚唤醒标志
 * 
 * 获取各个WAKEUP引脚的唤醒标志。
 * 
 * @return uint32_t 引脚标志掩码（pmu_wakeup_pin_t组合）
 *         - 位0: WKUP1
 *         - 位1: WKUP2
 *         - 位2: WKUP3
 *         - 位3: WKUP4
 *         - 位4: WKUP5
 *         - 位5: WKUP6
 *         - 位6: WKUP7
 *         - 位7: WKUP8
 * 
 * @note 标志如果为高，进入STANDBY后会立即唤醒，需先清除
 * 
 * @see pmu_clear_wakeup_pin_flags()
 */
uint32_t pmu_get_wakeup_pin_flags(void);

/**
 * @brief 清除WAKEUP引脚唤醒标志
 * 
 * 清除指定WAKEUP引脚的唤醒标志。
 * 
 * @param[in] pins 要清除的引脚掩码（pmu_wakeup_pin_t组合）
 * 
 * @note 进入STANDBY前必须清除标志，否则会立即唤醒
 * 
 * @return void
 */
void pmu_clear_wakeup_pin_flags(uint32_t pins);

/**
 * @brief 清除RTC唤醒标志
 * 
 * 清除RTC唤醒标志。
 * 
 * @note RTC唤醒标志即使不清除，也不会引起STANDBY模式立即唤醒
 * 
 * @return void
 */
void pmu_clear_rtc_wakeup_flag(void);

/**
 * @brief 清除IWDT唤醒标志
 * 
 * 清除IWDT唤醒标志。
 * 
 * @note 不清除会引起STANDBY模式立即唤醒
 * 
 * @return void
 */
void pmu_clear_iwdt_wakeup_flag(void);

/**
 * @brief 清除RSTN唤醒标志
 * 
 * 清除RSTN唤醒标志。
 * 
 * @note 不清除会引起STANDBY模式立即唤醒
 * 
 * @return void
 */
void pmu_clear_rstn_wakeup_flag(void);

/**
 * @brief 清除BOR唤醒标志
 * 
 * 清除BOR唤醒标志。
 * 
 * @note 不清除会引起STANDBY模式立刻唤醒
 * 
 * @return void
 */
void pmu_clear_bor_wakeup_flag(void);

/**
 * @brief 检查是否从STANDBY唤醒
 * 
 * 检查系统是否从STANDBY模式唤醒。
 * 
 * @return bool 唤醒状态
 *         - true: 从STANDBY模式唤醒
 *         - false: 正常启动或从其他模式唤醒
 * 
 * @note 此标志只能被POR或pmu_clear_standby_flag()清除
 * 
 * @see pmu_clear_standby_flag()
 */
bool pmu_is_standby_wakeup(void);

/**
 * @brief 清除STANDBY标志
 * 
 * 清除STANDBY模式标志。
 * 
 * @note 此标志会在进入STANDBY时自动置位
 * @note 除POR外，只能通过此函数清除
 * 
 * @return void
 */
void pmu_clear_standby_flag(void);

/**
 * @brief 清除所有唤醒标志
 * 
 * 清除所有唤醒相关标志（WAKEUP引脚、RTC、IWDT、RSTN、BOR、STANDBY）。
 * 
 * @note 进入低功耗模式前建议调用此函数
 * 
 * @return void
 */
void pmu_clear_all_wakeup_flags(void);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_PMU_H
