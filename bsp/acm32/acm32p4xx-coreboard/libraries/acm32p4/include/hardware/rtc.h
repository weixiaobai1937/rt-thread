/**
 * @file rtc.h
 * @brief ACM32P4 RTC驱动 - 实时时钟驱动接口
 *
 * @details
 * 本驱动支持ACM32P4系列芯片的RTC模块所有功能：
 * - 日历时钟（秒、分、时、日、周、月、年，BCD格式）
 * - 自动闰年补偿（28/29/30/31天）
 * - 闹钟功能（星期模式/日模式，可屏蔽字段比较）
 * - 周期唤醒（1s、1min、1h、1d及1024Hz~2Hz多种频率）
 * - 可编程唤醒定时器（16位自动重载递减计数器）
 * - 时钟误差数字校准（0.95ppm精度，+/-487ppm范围）
 * - 双通道侵入检测（TAMP1/2，带可配置滤波和时间戳）
 * - RTC Fout输出（PC13引脚，可选多种信号）
 * - 16个32位备份寄存器（共64字节）
 * - 中断/事件支持（闹钟、周期唤醒、侵入、唤醒定时器）
 * - 低功耗模式唤醒（SLEEP/STOP/STANDBY）
 *
 * 特性：
 * - ✅ 4层API架构（快速初始化 -> 基础配置 -> 高级功能 -> 控制查询）
 * - ✅ 类型安全（枚举类型，无魔法数字）
 * - ✅ 中断支持（单一RTC全局中断INT2）
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#ifndef _HARDWARE_RTC_H
#define _HARDWARE_RTC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 宏定义
//===========================================

#define RTC_WP_UNLOCK_KEY           0xCA53CA53U  ///< 写保护解锁密钥

#define RTC_BAKUP_COUNT             16U           ///< 备份寄存器数量
#define RTC_ALARM_WEEK_BIT_SUN      (1U << 0)     ///< 闹钟周日位
#define RTC_ALARM_WEEK_BIT_MON      (1U << 1)     ///< 闹钟周一位
#define RTC_ALARM_WEEK_BIT_TUE      (1U << 2)     ///< 闹钟周二位
#define RTC_ALARM_WEEK_BIT_WED      (1U << 3)     ///< 闹钟周三位置（实际寄存器从bit24开始映射）
#define RTC_ALARM_WEEK_BIT_THU      (1U << 4)     ///< 闹钟周四位置
#define RTC_ALARM_WEEK_BIT_FRI      (1U << 5)     ///< 闹钟周五位置
#define RTC_ALARM_WEEK_BIT_SAT      (1U << 6)     ///< 闹钟周六位

#define RTC_CALIB_VALUE_MAX         511U          ///< 校准值最大值
#define RTC_CALIB_PPM_UNIT          0.954f        ///< 校准最小单位(ppm)
#define RTC_CALIB_PPM_RANGE_MAX     487.0f        ///< 校准最大范围(+/-ppm)

#define RTC_WUT_WAIT_TIMEOUT        0xFFFFU       ///< WUTWF等待超时

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief 闹钟模式
 */
typedef enum {
    RTC_ALARM_MODE_WEEK = 0,  ///< 闹钟星期模式：按周几触发
    RTC_ALARM_MODE_DAY  = 1,  ///< 闹钟日模式：按每月几号触发
} rtc_alarm_mode_t;

/**
 * @brief 闹钟屏蔽位
 */
typedef enum {
    RTC_ALARM_MASK_MIN    = (1U << 0),  ///< 屏蔽分数值比较（ALM_MSKM）
    RTC_ALARM_MASK_HOUR   = (1U << 1),  ///< 屏蔽时数值比较（ALM_MSKH）
    RTC_ALARM_MASK_DAY    = (1U << 2),  ///< 屏蔽日/周数值比较（ALM_MSKD）
    RTC_ALARM_MASK_NONE   = 0U,         ///< 不屏蔽任何字段
    RTC_ALARM_MASK_ALL    = (RTC_ALARM_MASK_MIN | RTC_ALARM_MASK_HOUR | RTC_ALARM_MASK_DAY),
} rtc_alarm_mask_t;

/**
 * @brief 周期唤醒源选择（日历周期唤醒）
 */
typedef enum {
    RTC_PERIOD_SEC    = 0,   ///< 每秒唤醒
    RTC_PERIOD_MIN    = 1,   ///< 每分钟唤醒
    RTC_PERIOD_HOUR   = 2,   ///< 每小时唤醒
    RTC_PERIOD_DATE   = 3,   ///< 每天唤醒
    RTC_PERIOD_2HZ    = 4,   ///< 2Hz唤醒（每500ms）
    RTC_PERIOD_4HZ    = 5,   ///< 4Hz唤醒（每250ms）
    RTC_PERIOD_8HZ    = 6,   ///< 8Hz唤醒（每125ms）
    RTC_PERIOD_16HZ   = 7,   ///< 16Hz唤醒（每62.5ms）
    RTC_PERIOD_64HZ   = 8,   ///< 64Hz唤醒（每15.625ms）
    RTC_PERIOD_256HZ  = 9,   ///< 256Hz唤醒（每3.9ms）
    RTC_PERIOD_1KHZ   = 10,  ///< 1024Hz唤醒（每0.98ms）
} rtc_period_t;

/**
 * @brief 唤醒定时器时钟源选择
 */
typedef enum {
    RTC_WUT_CLK_RTCCLK_DIV16 = 0,  ///< RTCCLK/16
    RTC_WUT_CLK_RTCCLK_DIV8  = 1,  ///< RTCCLK/8
    RTC_WUT_CLK_RTCCLK_DIV4  = 2,  ///< RTCCLK/4
    RTC_WUT_CLK_RTCCLK_DIV2  = 3,  ///< RTCCLK/2
    RTC_WUT_CLK_1HZ           = 4,  ///< 1Hz时钟
    RTC_WUT_CLK_HALFHZ        = 5,  ///< 0.5Hz时钟
    RTC_WUT_CLK_2HZ           = 6,  ///< 2Hz时钟
} rtc_wut_clock_t;

/**
 * @brief RTC Fout输出信号选择
 */
typedef enum {
    RTC_FOUT_RESERVED0          = 0x0,  ///< 保留
    RTC_FOUT_RESERVED1          = 0x1,  ///< 保留
    RTC_FOUT_SEC_CARRY          = 0x2,  ///< 秒计数器进位信号
    RTC_FOUT_MIN_CARRY          = 0x3,  ///< 分计数器进位信号
    RTC_FOUT_HOUR_CARRY         = 0x4,  ///< 小时计数器进位信号
    RTC_FOUT_DATE_CARRY         = 0x5,  ///< 天计数器进位信号
    RTC_FOUT_ALARM_MATCH        = 0x6,  ///< 闹钟匹配信号
    RTC_FOUT_32S_SQUARE         = 0x7,  ///< 32秒方波信号
    RTC_FOUT_WUT_WAKEUP         = 0x8,  ///< WUT唤醒信号
    RTC_FOUT_SEC_CARRY_INV      = 0x9,  ///< 反相秒计数器进位信号
    RTC_FOUT_MIN_CARRY_INV      = 0xA,  ///< 反相分计数器进位信号
    RTC_FOUT_HOUR_CARRY_INV     = 0xB,  ///< 反相小时计数器进位信号
    RTC_FOUT_DATE_CARRY_INV     = 0xC,  ///< 反相天计数器进位信号
    RTC_FOUT_ALARM_MATCH_INV    = 0xD,  ///< 反相闹钟匹配信号
    RTC_FOUT_WUT_WAKEUP_INV     = 0xE,  ///< 反相WUT唤醒信号
    RTC_FOUT_SEC_SQUARE         = 0xF,  ///< RTC内部秒时标方波
} rtc_fout_signal_t;

/**
 * @brief Fout输出驱动模式
 */
typedef enum {
    RTC_FOUT_DRIVE_OD      = 0,  ///< 开漏输出
    RTC_FOUT_DRIVE_PUSHPULL = 1,  ///< 推挽输出
} rtc_fout_drive_t;

/**
 * @brief 校准符号
 */
typedef enum {
    RTC_CALIB_SIGN_POSITIVE = 0,  ///< 增加时钟周期
    RTC_CALIB_SIGN_NEGATIVE = 1,  ///< 减少时钟周期
} rtc_calib_sign_t;

/**
 * @brief 侵入检测通道
 */
typedef enum {
    RTC_TAMPER_1 = 0,  ///< TAMP1（PC13引脚）
    RTC_TAMPER_2 = 1,  ///< TAMP2（PA0引脚）
} rtc_tamper_channel_t;

/**
 * @brief 侵入检测边沿
 */
typedef enum {
    RTC_TAMPER_EDGE_RISING  = 0,  ///< 上升沿检测
    RTC_TAMPER_EDGE_FALLING = 1,  ///< 下降沿检测
} rtc_tamper_edge_t;

/**
 * @brief 侵入信号滤波周期
 */
typedef enum {
    RTC_TAMPER_FILTER_1  = 0,  ///< 1个滤波时钟周期
    RTC_TAMPER_FILTER_2  = 1,  ///< 2个滤波时钟周期
    RTC_TAMPER_FILTER_4  = 2,  ///< 4个滤波时钟周期
    RTC_TAMPER_FILTER_8  = 3,  ///< 8个滤波时钟周期
} rtc_tamper_filter_t;

/**
 * @brief 侵入滤波时钟选择
 */
typedef enum {
    RTC_TAMPER_FILT_CLK_RTCCLK = 0,  ///< RTCCLK
    RTC_TAMPER_FILT_CLK_64HZ   = 1,  ///< 64Hz（512个RTCCLK）
} rtc_tamper_filter_clk_t;

/**
 * @brief RTC中断类型（位掩码，可组合）
 */
typedef enum {
    RTC_IRQ_DATE        = (1U << 0),   ///< 天中断
    RTC_IRQ_HOUR        = (1U << 1),   ///< 小时中断
    RTC_IRQ_MIN         = (1U << 2),   ///< 分中断
    RTC_IRQ_SEC         = (1U << 3),   ///< 秒中断
    RTC_IRQ_2HZ         = (1U << 4),   ///< 2Hz中断
    RTC_IRQ_4HZ         = (1U << 5),   ///< 4Hz中断
    RTC_IRQ_8HZ         = (1U << 6),   ///< 8Hz中断
    RTC_IRQ_16HZ        = (1U << 7),   ///< 16Hz中断
    RTC_IRQ_64HZ        = (1U << 8),   ///< 64Hz中断
    RTC_IRQ_256HZ       = (1U << 9),   ///< 256Hz中断
    RTC_IRQ_1KHZ        = (1U << 10),  ///< 1024Hz中断
    RTC_IRQ_ALARM       = (1U << 11),  ///< 闹钟中断
    RTC_IRQ_ADJ32       = (1U << 12),  ///< 32秒校准中断
    RTC_IRQ_TAMP1_FALL  = (1U << 13),  ///< TAMP1下降沿中断
    RTC_IRQ_TAMP1_RISE  = (1U << 14),  ///< TAMP1上升沿中断
    RTC_IRQ_TAMP2_FALL  = (1U << 15),  ///< TAMP2下降沿中断
    RTC_IRQ_TAMP2_RISE  = (1U << 16),  ///< TAMP2上升沿中断
    RTC_IRQ_WUT         = (1U << 17),  ///< 唤醒定时器中断
} rtc_irq_type_t;

/**
 * @brief RTC状态标志（位掩码，可组合）
 */
typedef enum {
    RTC_FLAG_DATE        = (1U << 0),   ///< 天标志
    RTC_FLAG_HOUR        = (1U << 1),   ///< 小时标志
    RTC_FLAG_MIN         = (1U << 2),   ///< 分标志
    RTC_FLAG_SEC         = (1U << 3),   ///< 秒标志
    RTC_FLAG_2HZ         = (1U << 4),   ///< 2Hz标志
    RTC_FLAG_4HZ         = (1U << 5),   ///< 4Hz标志
    RTC_FLAG_8HZ         = (1U << 6),   ///< 8Hz标志
    RTC_FLAG_16HZ        = (1U << 7),   ///< 16Hz标志
    RTC_FLAG_64HZ        = (1U << 8),   ///< 64Hz标志
    RTC_FLAG_256HZ       = (1U << 9),   ///< 256Hz标志
    RTC_FLAG_1KHZ        = (1U << 10),  ///< 1024Hz标志
    RTC_FLAG_ALARM       = (1U << 11),  ///< 闹钟标志
    RTC_FLAG_ADJ32       = (1U << 12),  ///< 32秒校准标志
    RTC_FLAG_TAMP1_FALL  = (1U << 13),  ///< TAMP1下降沿标志
    RTC_FLAG_TAMP1_RISE  = (1U << 14),  ///< TAMP1上升沿标志
    RTC_FLAG_TAMP2_FALL  = (1U << 15),  ///< TAMP2下降沿标志
    RTC_FLAG_TAMP2_RISE  = (1U << 16),  ///< TAMP2上升沿标志
    RTC_FLAG_WUT         = (1U << 17),  ///< 唤醒定时器标志
} rtc_flag_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief 日历时间结构体（BCD格式）
 *
 * @note 所有字段均为BCD格式（如27日=0x27，59秒=0x59）
 */
typedef struct {
    uint8_t sec;     ///< 秒 (0x00-0x59, BCD)
    uint8_t min;     ///< 分 (0x00-0x59, BCD)
    uint8_t hour;    ///< 时 (0x00-0x23, BCD, 24小时制)
    uint8_t day;     ///< 日 (0x01-0x31, BCD)
    uint8_t week;    ///< 周 (0x01-0x07, BCD, 1=周日)
    uint8_t month;   ///< 月 (0x01-0x12, BCD)
    uint8_t year;    ///< 年 (0x00-0x99, BCD, 后两位)
} rtc_time_t;

/**
 * @brief 闹钟配置结构体
 */
typedef struct {
    rtc_alarm_mode_t mode;        ///< 闹钟模式（星期/日）
    uint8_t          sec;         ///< 闹钟秒 (0x00-0x59, BCD)
    uint8_t          min;         ///< 闹钟分 (0x00-0x59, BCD)
    uint8_t          hour;        ///< 闹钟时 (0x00-0x23, BCD)
    uint8_t          alarm_day;   ///< 闹钟日 (0x01-0x31, BCD, 日模式有效)
    uint8_t          alarm_week;  ///< 闹钟星期位掩码 (bit0=周日..bit6=周六, 星期模式有效)
    rtc_alarm_mask_t mask;        ///< 闹钟比较屏蔽位
} rtc_alarm_config_t;

/**
 * @brief 唤醒定时器配置结构体
 */
typedef struct {
    rtc_wut_clock_t clock;     ///< 唤醒定时器时钟源
    uint16_t        reload;    ///< 自动重载值 (0x0000-0xFFFF)
} rtc_wut_config_t;

/**
 * @brief 时钟校准配置结构体
 */
typedef struct {
    rtc_calib_sign_t sign;   ///< 校准方向（增加/减少）
    uint16_t         value;  ///< 校准值 (0-511)，bit[8:5]=公共值C，bit[4:0]=差分值D
} rtc_calib_config_t;

/**
 * @brief 侵入检测配置结构体
 */
typedef struct {
    rtc_tamper_channel_t   channel;          ///< 侵入通道
    bool                   rising_enable;    ///< 使能上升沿检测
    bool                   falling_enable;   ///< 使能下降沿检测
    bool                   rising_clear_bkp; ///< 上升沿清除备份寄存器
    bool                   falling_clear_bkp;///< 下降沿清除备份寄存器
    rtc_tamper_edge_t      timestamp_edge;   ///< 时间戳记录边沿
    bool                   filter_enable;    ///< 使能数字滤波
    rtc_tamper_filter_t    filter_period;    ///< 滤波周期
    rtc_tamper_filter_clk_t filter_clk;      ///< 滤波时钟
} rtc_tamper_config_t;

/**
 * @brief 侵入时间戳结构体
 */
typedef struct {
    uint8_t sec;    ///< 秒 (BCD)
    uint8_t min;    ///< 分 (BCD)
    uint8_t hour;   ///< 时 (BCD)
    uint8_t day;    ///< 日 (BCD)
    uint8_t week;   ///< 周 (BCD)
    uint8_t month;  ///< 月 (BCD)
    uint8_t year;   ///< 年 (BCD)
} rtc_timestamp_t;

/**
 * @brief RTC初始化配置结构体
 */
typedef struct {
    rtc_time_t   time;          ///< 初始日历时间
    bool         use_xtl;       ///< true: 使用外部XTL晶振, false: 使用内部RC32K
    bool         output_enable; ///< 使能RTC Fout输出到PC13
    rtc_fout_signal_t  fout_signal;   ///< Fout输出信号选择
    rtc_fout_drive_t   fout_drive;    ///< Fout输出驱动模式
} rtc_init_config_t;

/**
 * @brief RTC中断回调函数类型
 *
 * @param[in] flags  触发中断的标志位组合（rtc_flag_t按位或）
 * @param[in] context 用户注册的上下文指针
 */
typedef void (*rtc_callback_t)(uint32_t flags, void *context);

//===========================================
// 第1层：快速初始化 API
//===========================================

/**
 * @brief 快速初始化RTC（使用默认内部RC32K时钟）
 *
 * @param[in] time  初始日历时间指针（若为NULL则仅使能模块，不设置时间）
 * @return true: 成功, false: 失败
 *
 * @note 自动解锁/上锁写保护，使能PMU和RTC时钟
 */
bool rtc_init_default(const rtc_time_t *time);

/**
 * @brief 完整配置初始化RTC
 *
 * @param[in] config  初始化配置结构体
 * @return true: 成功, false: 失败
 *
 *       RTC_CR.FSEL, PMU_IOSEL
 */
bool rtc_init(const rtc_init_config_t *config);

//===========================================
// 第2层：基础配置 API
//===========================================

/**
 * @brief 设置RTC日历时间
 *
 * @param[in] time  日历时间结构体指针（BCD格式）
 * @return true: 成功, false: 失败
 *
 * @note 写入秒寄存器时硬件自动清零亚秒计数器
 * @note 建议在秒中断发生后调用此函数以提高设置精度
 */
bool rtc_config_time(const rtc_time_t *time);

/**
 * @brief 配置闹钟
 *
 * @param[in] config  闹钟配置结构体
 * @return true: 成功, false: 失败
 *
 *       RTC_CR (ALM_MSKD, ALM_MSKH, ALM_MSKM)
 */
bool rtc_config_alarm(const rtc_alarm_config_t *config);

/**
 * @brief 配置唤醒定时器
 *
 * @param[in] config  唤醒定时器配置结构体
 * @return true: 成功, false: 失败
 *
 * @note 内部自动执行 WUTE清零 -> 等待WUTWF -> 写入WUTR/WUCKSEL -> WUTE置1 的完整流程
 */
bool rtc_config_wakeup_timer(const rtc_wut_config_t *config);

/**
 * @brief 配置时钟误差数字校准
 *
 * @param[in] config  校准配置结构体
 *
 * @note 校准精度0.954ppm，范围+/-487ppm
 * @note Correction(ppm) = (C*32 + D) * 0.954 / 1000000，其中C=value[8:5], D=value[4:0]
 */
void rtc_config_calibration(const rtc_calib_config_t *config);

/**
 * @brief 配置侵入检测通道
 *
 * @param[in] config  侵入检测配置结构体
 *
 *       TS1EDGE/TS2EDGE, TAMP1FLTEN/TAMP2FLTEN, TAMP1FLT/TAMP2FLT, TAMPFLTCLK)
 * @note TAMP1使用PC13引脚（需配置PMU_IOSEL.PC1_SEL=10），TAMP2使用PA0引脚（需配置为模拟功能）
 */
void rtc_config_tamper(const rtc_tamper_config_t *config);

/**
 * @brief 配置RTC Fout输出
 *
 * @param[in] signal  输出信号选择
 * @param[in] drive   输出驱动模式
 *
 * @note 输出通过PC13引脚，需配置PMU_IOSEL.PC1_SEL=01选择RTC Fout功能
 */
void rtc_config_fout(rtc_fout_signal_t signal, rtc_fout_drive_t drive);

//===========================================
// 第3层：高级功能 API
//===========================================

/**
 * @brief 获取RTC日历时间（中断安全读取方式）
 *
 * @param[out] time  日历时间结构体指针
 *
 * @note 实现任意时刻安全读取：两次读秒，不一致则重读
 */
void rtc_get_time(rtc_time_t *time);

/**
 * @brief 获取RTC日历时间（在周期中断中调用，无需重读保护）
 *
 * @param[out] time  日历时间结构体指针
 *
 * @note 仅在中ISR中调用时使用此函数
 */
void rtc_get_time_isr(rtc_time_t *time);

/**
 * @brief 获取侵入时间戳
 *
 * @param[in]  channel  侵入通道
 * @param[out] stamp    时间戳结构体指针
 * @return true: 时间戳有效, false: 参数错误
 *
 */
bool rtc_get_timestamp(rtc_tamper_channel_t channel, rtc_timestamp_t *stamp);

/**
 * @brief 设置周期唤醒
 *
 * @param[in] period  唤醒周期
 *
 * @note 此函数使能对应周期的中断使能位
 */
void rtc_enable_period_wakeup(rtc_period_t period);

/**
 * @brief 禁用周期唤醒
 *
 * @param[in] period  唤醒周期
 *
 */
void rtc_disable_period_wakeup(rtc_period_t period);

/**
 * @brief 使能闹钟
 *
 */
void rtc_enable_alarm(void);

/**
 * @brief 禁用闹钟
 *
 */
void rtc_disable_alarm(void);

/**
 * @brief 使能唤醒定时器
 *
 */
void rtc_enable_wakeup_timer(void);

/**
 * @brief 禁用唤醒定时器
 *
 */
void rtc_disable_wakeup_timer(void);

/**
 * @brief 使能指定侵入通道
 *
 * @param[in] channel  侵入通道
 *
 */
void rtc_enable_tamper(rtc_tamper_channel_t channel);

/**
 * @brief 禁用指定侵入通道
 *
 * @param[in] channel  侵入通道
 *
 */
void rtc_disable_tamper(rtc_tamper_channel_t channel);

/**
 * @brief 设置唤醒定时器自动重载值（快速更新，无需走WUTE流程）
 *
 * @param[in] reload  自动重载值 (0x0000-0xFFFF)
 *
 * @note 建议在WUTE已使能且WUTWF=0时调用，用于动态调整唤醒间隔
 */
void rtc_set_wut_reload(uint16_t reload);

/**
 * @brief 写入备份寄存器
 *
 * @param[in] index  备份寄存器索引 (0-15)
 * @param[in] value  要写入的32位值
 * @return true: 成功, false: 索引越界
 *
 */
bool rtc_backup_write(uint8_t index, uint32_t value);

/**
 * @brief 读取备份寄存器
 *
 * @param[in]  index  备份寄存器索引 (0-15)
 * @param[out] value  读取的32位值
 * @return true: 成功, false: 索引越界
 *
 */
bool rtc_backup_read(uint8_t index, uint32_t *value);

//===========================================
// 第4层：控制与查询 API
//===========================================

/**
 * @brief 解除RTC写保护
 *
 */
void rtc_unlock(void);

/**
 * @brief 使能RTC写保护
 *
 */
void rtc_lock(void);

/**
 * @brief 查询RTC写保护状态
 *
 * @return true: 写保护已解除, false: 写保护使能中
 *
 */
bool rtc_is_unlocked(void);

/**
 * @brief 获取RTC中断标志状态
 *
 * @return 当前置位的所有中断标志（rtc_flag_t按位或）
 *
 */
uint32_t rtc_get_flags(void);

/**
 * @brief 清除指定RTC中断标志
 *
 * @param[in] flags  要清除的标志位组合（rtc_flag_t按位或）
 *
 */
void rtc_clear_flags(uint32_t flags);

/**
 * @brief 配置RTC中断使能
 *
 * @param[in] irq_mask  要配置的中断类型组合（rtc_irq_type_t按位或）
 * @param[in] enable    true: 使能, false: 禁用
 *
 */
void rtc_irq_config(uint32_t irq_mask, bool enable);

/**
 * @brief 获取当前RTC中断使能状态
 *
 * @return 当前已使能的中断类型（rtc_irq_type_t按位或）
 *
 */
uint32_t rtc_get_irq_enabled(void);

/**
 * @brief 注册RTC中断回调函数
 *
 * @param[in] callback  中断回调函数指针（NULL则取消注册）
 * @param[in] context   用户上下文指针（回调时传回）
 *
 * @note RTC全局中断连接到NVIC INT2通道
 */
void rtc_irq_register(rtc_callback_t callback, void *context);

/**
 * @brief 检查唤醒定时器是否可写入配置
 *
 * @return true: 允许更新唤醒定时器配置（WUTWF=1）, false: 不允许
 *
 */
bool rtc_is_wut_writable(void);

/**
 * @brief 获取当前校准配置
 *
 * @param[out] sign   校准方向
 * @param[out] value  校准值
 *
 */
void rtc_get_calibration(rtc_calib_sign_t *sign, uint16_t *value);

/**
 * @brief 获取亚秒计数器值
 *
 * @return 亚秒计数值（0-255，分辨率1/256秒）
 *
 */
uint8_t rtc_get_subsecond(void);

//===========================================
// 中断服务程序声明
//===========================================

/**
 * @brief RTC全局中断处理程序（INT2）
 *
 * 处理所有使能的RTC中断事件，包括：
 * - 日历周期中断（秒、分、时、日）
 * - 高频周期中断（2Hz~1KHz）
 * - 闹钟中断
 * - 侵入检测中断（TAMP1/2上升/下降沿）
 * - 唤醒定时器中断
 * - 32秒校准中断
 *
 * 处理完成后调用用户注册的回调函数。
 */
void RTC_IRQHandler(void);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_RTC_H
