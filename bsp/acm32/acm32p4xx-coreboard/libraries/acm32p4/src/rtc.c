/**
 * @file rtc.c
 * @brief ACM32P4 RTC驱动 - 实时时钟驱动实现
 *
 * 实现了RTC驱动的全部功能，包括：
 * - 日历时钟（秒、分、时、日、周、月、年，BCD格式，自动闰年补偿）
 * - 闹钟功能（星期/日双模式，字段屏蔽比较）
 * - 周期唤醒（1s/1min/1h/1d 及 2Hz~1024Hz多级频率）
 * - 可编程唤醒定时器（16位递减，多时钟源）
 * - 时钟误差数字校准（0.95ppm精度，32秒平滑分布）
 * - 双通道侵入检测（可配置滤波、时间戳、备份寄存器清除）
 * - RTC Fout输出（PC13引脚，15种信号可选）
 * - 16个备份寄存器（64字节，侵入/备份域复位保护）
 * - 全部中断管理（INT2 NVIC通道 + STOP/STANDBY唤醒）
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "hardware/rtc.h"
#include "device/acm32p4xx.h"
#include <assert.h>
#include <stddef.h>

//===========================================
// 内部宏定义
//===========================================

/* RTC_CR 寄存器位定义 */
#define RTC_CR_FSEL_POS             (0U)
#define RTC_CR_FSEL_MSK             (0xFUL << RTC_CR_FSEL_POS)
#define RTC_CR_ALM_EN_MSK           (1UL << 4)
#define RTC_CR_TAMP1EN_MSK          (1UL << 5)
#define RTC_CR_TAMP2EN_MSK          (1UL << 6)
#define RTC_CR_TAMP1RCLR_MSK        (1UL << 7)
#define RTC_CR_TAMP1FCLR_MSK        (1UL << 8)
#define RTC_CR_ALM_MSKM_MSK         (1UL << 9)
#define RTC_CR_ALM_MSKH_MSK         (1UL << 10)
#define RTC_CR_ALM_MSKD_MSK         (1UL << 11)
#define RTC_CR_TAMP1FLTEN_MSK       (1UL << 12)
#define RTC_CR_TAMP1FLT_POS         (13U)
#define RTC_CR_TAMP1FLT_MSK         (3UL << RTC_CR_TAMP1FLT_POS)
#define RTC_CR_TS1EDGE_MSK          (1UL << 15)
#define RTC_CR_TAMP2RCLR_MSK        (1UL << 16)
#define RTC_CR_TAMP2FCLR_MSK        (1UL << 17)
#define RTC_CR_TAMP2FLTEN_MSK       (1UL << 18)
#define RTC_CR_TAMP2FLT_POS         (19U)
#define RTC_CR_TAMP2FLT_MSK         (3UL << RTC_CR_TAMP2FLT_POS)
#define RTC_CR_TS2EDGE_MSK          (1UL << 21)
#define RTC_CR_TAMPFLTCLK_MSK       (1UL << 22)
#define RTC_CR_WUTE_MSK             (1UL << 23)
#define RTC_CR_WUCKSEL_POS          (24U)
#define RTC_CR_WUCKSEL_MSK          (7UL << RTC_CR_WUCKSEL_POS)

/* RTC_ALARM (ALM) 寄存器位定义 */
#define RTC_ALM_ALMSEC_POS          (0U)
#define RTC_ALM_ALMSEC_MSK          (0x7FUL << RTC_ALM_ALMSEC_POS)
#define RTC_ALM_ALMMIN_POS          (8U)
#define RTC_ALM_ALMMIN_MSK          (0x7FUL << RTC_ALM_ALMMIN_POS)
#define RTC_ALM_ALMHOUR_POS         (16U)
#define RTC_ALM_ALMHOUR_MSK         (0x3FUL << RTC_ALM_ALMHOUR_POS)
#define RTC_ALM_ALMWEEKALMDAY_POS   (24U)
#define RTC_ALM_ALMWEEKALMDAY_MSK   (0x7FUL << RTC_ALM_ALMWEEKALMDAY_POS)
#define RTC_ALM_ALM_WDS_MSK         (1UL << 31)

/* RTC_ADJUST 寄存器位定义 */
#define RTC_ADJUST_ADJVALUE_POS     (0U)
#define RTC_ADJUST_ADJVALUE_MSK     (0x1FFUL << RTC_ADJUST_ADJVALUE_POS)
#define RTC_ADJUST_ADJSIGN_MSK      (1UL << 9)

/* RTC_SR 全部可清除标志位掩码 */
#define RTC_SR_CLEARABLE_MSK        (0x01FFFFFFUL)

/* RTC_IE 全部有效使能位掩码 */
#define RTC_IE_ALL_MSK              (0x0003FFFFUL)

/* PMU IOSEL 相关 */
#define PMU_IOSEL_PC13_SEL_RTC_FOUT (1UL << 0)   /* PC13_SEL[1:0] = 01 = RTC Fout */
#define PMU_IOSEL_PC13_SEL_TAMPER1  (2UL << 0)   /* PC13_SEL[1:0] = 10 = TAMP1 侵入检测 */
#define PMU_IOSEL_PC13_VALUE_PP     (1UL << 8)   /* PC13_VALUE = 1 = push-pull */
#define PMU_IOSEL_PC13_VALUE_OD     (0UL << 8)   /* PC13_VALUE = 0 = open-drain */

/* RCC STDBYCTRL 相关 */
#define RCC_STDBYCTRL_RTCSEL_RC32K  (0UL << 20)  /* RTCSEL = 00 = RC32K */
#define RCC_STDBYCTRL_RTCSEL_XTL    (1UL << 20)  /* RTCSEL = 01 = XTL */

//===========================================
// 内部状态变量
//===========================================

static struct {
    rtc_callback_t callback;    ///< 用户注册的RTC中断回调
    void          *context;     ///< 回调上下文指针
} s_state;

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 获取RTC外设基地址指针
 */
static inline RTC_TypeDef *rtc_periph(void)
{
    return RTC;
}

/**
 * @brief 写入RTC写保护寄存器（设备头中WP为__IM，需强制写入）
 */
static void write_wp(uint32_t value)
{
    *((volatile uint32_t *)&RTC->WP) = value;
}

/**
 * @brief 写入RTC中断标志寄存器（设备头中SR为__IM，RCW1需强制写入）
 */
static void write_sr(uint32_t value)
{
    *((volatile uint32_t *)&RTC->SR) = value;
}

/**
 * @brief 等待唤醒定时器可写标志（WUTWF=1）
 * @return true: 可写, false: 超时
 */
static bool wait_wut_writable(void)
{
    uint32_t timeout = RTC_WUT_WAIT_TIMEOUT;
    while (!(RTC->SR & (1UL << 24)) && (--timeout != 0U)) {
    }
    return (timeout != 0U);
}

/**
 * @brief 等待XTL时钟就绪
 */
static bool wait_xtl_ready(void)
{
    uint32_t timeout = 0xFFFFU;
    while (!(RCC->STDBYCTRL & (1UL << 1)) && (--timeout != 0U)) {
    }
    return (timeout != 0U);
}

/**
 * @brief 等待RC32K时钟就绪
 */
static bool wait_rcl_ready(void)
{
    uint32_t timeout = 0xFFFFU;
    while (!(RCC->STDBYCTRL & (1UL << 9)) && (--timeout != 0U)) {
    }
    return (timeout != 0U);
}

/**
 * @brief BCD字节转二进制
 */
static inline uint8_t bcd_to_bin(uint8_t bcd)
{
    return ((bcd >> 4U) * 10U) + (bcd & 0x0FU);
}

/**
 * @brief 二进制转BCD字节
 */
static inline uint8_t bin_to_bcd(uint8_t bin)
{
    return (uint8_t)(((bin / 10U) << 4U) | (bin % 10U));
}

/**
 * @brief 使能RTC模块时钟（PMU + RTC总线时钟 + PMU写使能）
 */
static void enable_rtc_clocks(void)
{
    /* 使能PMU模块时钟 */
    RCC->APB1CKENR1 |= (1UL << 27);  /* PMUCKEN */
    /* 使能RTC模块总线时钟 */
    RCC->APB1CKENR1 |= (1UL << 10);  /* RTCCKEN */
    /* PMU中使能RTC寄存器写访问 */
    PMU->CTRL0 |= (1UL << 7);        /* RTC_WE */
}

/**
 * @brief 选择并启用RTC时钟源
 *
 * @param use_xtl  true: XTL晶振, false: RC32K
 * @return true: 成功, false: 时钟就绪超时
 */
static bool select_rtc_clock(bool use_xtl)
{
    if (use_xtl) {
        /* 选择XTL作为RTC时钟源 */
        RCC->STDBYCTRL = (RCC->STDBYCTRL & ~(3UL << 20)) | RCC_STDBYCTRL_RTCSEL_XTL;
        /* 使能XTL振荡器 */
        RCC->STDBYCTRL |= (1UL << 0);  /* XTLEN */
        return wait_xtl_ready();
    } else {
        /* 选择RC32K作为RTC时钟源 */
        RCC->STDBYCTRL = (RCC->STDBYCTRL & ~(3UL << 20)) | RCC_STDBYCTRL_RTCSEL_RC32K;
        /* 使能RC32K */
        RCC->STDBYCTRL |= (1UL << 8);  /* RCLEN */
        return wait_rcl_ready();
    }
}

//===========================================
// 第1层：快速初始化 API
//===========================================

bool rtc_init_default(const rtc_time_t *time)
{
    /* 使能时钟 */
    enable_rtc_clocks();

    /* 选择默认RC32K时钟源 */
    if (!select_rtc_clock(false)) {
        return false;
    }

    if (time != NULL) {
        /* 解除写保护 */
        rtc_unlock();

        /* 写入日历时间 */
        RTC->SEC   = (uint32_t)time->sec;
        RTC->MIN   = (uint32_t)time->min;
        RTC->HOUR  = (uint32_t)time->hour;
        RTC->DAY   = (uint32_t)time->day;
        RTC->WEEK  = (uint32_t)time->week;
        RTC->MONTH = (uint32_t)time->month;
        RTC->YEAR  = (uint32_t)time->year;

        /* 恢复写保护 */
        rtc_lock();
    }

    return true;
}

bool rtc_init(const rtc_init_config_t *config)
{
    assert(config != NULL);

    /* 使能时钟 */
    enable_rtc_clocks();

    /* 选择时钟源 */
    if (!select_rtc_clock(config->use_xtl)) {
        return false;
    }

    /* 解除写保护 */
    rtc_unlock();

    /* 写入日历时间 */
    RTC->SEC   = (uint32_t)config->time.sec;
    RTC->MIN   = (uint32_t)config->time.min;
    RTC->HOUR  = (uint32_t)config->time.hour;
    RTC->DAY   = (uint32_t)config->time.day;
    RTC->WEEK  = (uint32_t)config->time.week;
    RTC->MONTH = (uint32_t)config->time.month;
    RTC->YEAR  = (uint32_t)config->time.year;

    /* 恢复写保护 */
    rtc_lock();

    /* 配置Fout输出 */
    if (config->output_enable) {
        rtc_config_fout(config->fout_signal, config->fout_drive);
    }

    return true;
}

//===========================================
// 第2层：基础配置 API
//===========================================

bool rtc_config_time(const rtc_time_t *time)
{
    assert(time != NULL);

    if (!rtc_is_unlocked()) {
        return false;
    }

    /* 写入所有日历寄存器（写入SEC时硬件自动清零亚秒计数器） */
    RTC->SEC   = (uint32_t)time->sec;
    RTC->MIN   = (uint32_t)time->min;
    RTC->HOUR  = (uint32_t)time->hour;
    RTC->DAY   = (uint32_t)time->day;
    RTC->WEEK  = (uint32_t)time->week;
    RTC->MONTH = (uint32_t)time->month;
    RTC->YEAR  = (uint32_t)time->year;

    return true;
}

bool rtc_config_alarm(const rtc_alarm_config_t *config)
{
    assert(config != NULL);

    /* 关闭闹钟后再配置 */
    RTC->CR &= ~RTC_CR_ALM_EN_MSK;

    /* 构建闹钟寄存器值 */
    uint32_t alm = 0U;
    alm |= ((uint32_t)config->sec  << RTC_ALM_ALMSEC_POS) & RTC_ALM_ALMSEC_MSK;
    alm |= ((uint32_t)config->min  << RTC_ALM_ALMMIN_POS) & RTC_ALM_ALMMIN_MSK;
    alm |= ((uint32_t)config->hour << RTC_ALM_ALMHOUR_POS) & RTC_ALM_ALMHOUR_MSK;

    /* 设置闹钟日/周字段 */
    if (config->mode == RTC_ALARM_MODE_DAY) {
        /* 日模式：ALMDAY[29:24]为日的BCD值 */
        alm |= ((uint32_t)config->alarm_day << RTC_ALM_ALMWEEKALMDAY_POS) & RTC_ALM_ALMWEEKALMDAY_MSK;
        alm |= RTC_ALM_ALM_WDS_MSK;  /* ALM_WDS=1: 选择日模式 */
    } else {
        /* 星期模式：ALMWEEK[30:24]为星期位掩码(b24=周日..b30=周六) */
        alm |= ((uint32_t)config->alarm_week << RTC_ALM_ALMWEEKALMDAY_POS) & RTC_ALM_ALMWEEKALMDAY_MSK;
        /* ALM_WDS=0: 选择星期模式（默认） */
    }

    RTC->ALM = alm;

    /* 配置闹钟比较屏蔽位 */
    uint32_t cr = RTC->CR;
    cr &= ~(RTC_CR_ALM_MSKM_MSK | RTC_CR_ALM_MSKH_MSK | RTC_CR_ALM_MSKD_MSK);
    if (config->mask & RTC_ALARM_MASK_MIN) {
        cr |= RTC_CR_ALM_MSKM_MSK;
    }
    if (config->mask & RTC_ALARM_MASK_HOUR) {
        cr |= RTC_CR_ALM_MSKH_MSK;
    }
    if (config->mask & RTC_ALARM_MASK_DAY) {
        cr |= RTC_CR_ALM_MSKD_MSK;
    }
    RTC->CR = cr;

    return true;
}

bool rtc_config_wakeup_timer(const rtc_wut_config_t *config)
{
    assert(config != NULL);

    /* 1）禁止唤醒定时器 */
    RTC->CR &= ~RTC_CR_WUTE_MSK;

    /* 2）等待WUTWF=1以确保可以访问WUTR和WUCKSEL */
    if (!wait_wut_writable()) {
        return false;
    }

    /* 3）写入重载值，选择时钟 */
    RTC->WUTR = (uint32_t)config->reload;

    uint32_t cr = RTC->CR;
    cr &= ~RTC_CR_WUCKSEL_MSK;
    cr |= ((uint32_t)config->clock << RTC_CR_WUCKSEL_POS) & RTC_CR_WUCKSEL_MSK;

    /* 使能唤醒定时器 */
    cr |= RTC_CR_WUTE_MSK;
    RTC->CR = cr;

    return true;
}

void rtc_config_calibration(const rtc_calib_config_t *config)
{
    assert(config != NULL);

    uint32_t adj = 0U;
    adj |= ((uint32_t)config->value << RTC_ADJUST_ADJVALUE_POS) & RTC_ADJUST_ADJVALUE_MSK;
    if (config->sign == RTC_CALIB_SIGN_NEGATIVE) {
        adj |= RTC_ADJUST_ADJSIGN_MSK;
    }
    RTC->ADJUST = adj;
}

void rtc_config_tamper(const rtc_tamper_config_t *config)
{
    assert(config != NULL);

    bool is_tamper1 = (config->channel == RTC_TAMPER_1);

    if (is_tamper1) {
        /* 关闭TAMP1 */
        RTC->CR &= ~RTC_CR_TAMP1EN_MSK;

        /* 配置PC13为TAMP1功能（PMU_IOSEL PC13_SEL=10） */
        uint32_t iosel = PMU->IOSEL;
        iosel &= ~0x3U;  /* 清除PC13_SEL[1:0] */
        iosel |= PMU_IOSEL_PC13_SEL_TAMPER1;
        PMU->IOSEL = iosel;
    } else {
        /* 关闭TAMP2 */
        RTC->CR &= ~RTC_CR_TAMP2EN_MSK;
        /* TAMP2使用PA0，需配置PA0为模拟功能（由GPIO驱动另行配置） */
    }

    /* 配置滤波时钟（两个通道共用） */
    uint32_t cr = RTC->CR;
    if (config->filter_enable && config->filter_clk == RTC_TAMPER_FILT_CLK_64HZ) {
        cr |= RTC_CR_TAMPFLTCLK_MSK;
    } else {
        cr &= ~RTC_CR_TAMPFLTCLK_MSK;
    }

    if (is_tamper1) {
        /* 清除备份寄存器控制 */
        cr &= ~(RTC_CR_TAMP1RCLR_MSK | RTC_CR_TAMP1FCLR_MSK);
        if (config->rising_clear_bkp) {
            cr |= RTC_CR_TAMP1RCLR_MSK;
        }
        if (config->falling_clear_bkp) {
            cr |= RTC_CR_TAMP1FCLR_MSK;
        }

        /* 时间戳边沿 */
        cr &= ~RTC_CR_TS1EDGE_MSK;
        if (config->timestamp_edge == RTC_TAMPER_EDGE_FALLING) {
            cr |= RTC_CR_TS1EDGE_MSK;
        }

        /* 滤波配置 */
        if (config->filter_enable) {
            cr |= RTC_CR_TAMP1FLTEN_MSK;
        } else {
            cr &= ~RTC_CR_TAMP1FLTEN_MSK;
        }
        cr &= ~RTC_CR_TAMP1FLT_MSK;
        cr |= ((uint32_t)config->filter_period << RTC_CR_TAMP1FLT_POS) & RTC_CR_TAMP1FLT_MSK;

        RTC->CR = cr;

        /* 使能TAMP1（仅使能对应边沿检测由RTC_CR的TAMP1EN和中断使能控制） */
        if (config->rising_enable || config->falling_enable) {
            RTC->CR |= RTC_CR_TAMP1EN_MSK;
        }
    } else {
        /* 清除备份寄存器控制 */
        cr &= ~(RTC_CR_TAMP2RCLR_MSK | RTC_CR_TAMP2FCLR_MSK);
        if (config->rising_clear_bkp) {
            cr |= RTC_CR_TAMP2RCLR_MSK;
        }
        if (config->falling_clear_bkp) {
            cr |= RTC_CR_TAMP2FCLR_MSK;
        }

        /* 时间戳边沿 */
        cr &= ~RTC_CR_TS2EDGE_MSK;
        if (config->timestamp_edge == RTC_TAMPER_EDGE_FALLING) {
            cr |= RTC_CR_TS2EDGE_MSK;
        }

        /* 滤波配置 */
        if (config->filter_enable) {
            cr |= RTC_CR_TAMP2FLTEN_MSK;
        } else {
            cr &= ~RTC_CR_TAMP2FLTEN_MSK;
        }
        cr &= ~RTC_CR_TAMP2FLT_MSK;
        cr |= ((uint32_t)config->filter_period << RTC_CR_TAMP2FLT_POS) & RTC_CR_TAMP2FLT_MSK;

        RTC->CR = cr;

        /* 使能TAMP2 */
        if (config->rising_enable || config->falling_enable) {
            RTC->CR |= RTC_CR_TAMP2EN_MSK;
        }
    }
}

void rtc_config_fout(rtc_fout_signal_t signal, rtc_fout_drive_t drive)
{
    /* 配置FSEL输出信号 */
    uint32_t cr = RTC->CR;
    cr &= ~RTC_CR_FSEL_MSK;
    cr |= ((uint32_t)signal << RTC_CR_FSEL_POS) & RTC_CR_FSEL_MSK;
    RTC->CR = cr;

    /* 配置PC13 IO功能 */
    uint32_t iosel = PMU->IOSEL;
    /* PC13_SEL[1:0] = 01 选择RTC Fout */
    iosel &= ~0x3U;
    iosel |= PMU_IOSEL_PC13_SEL_RTC_FOUT;
    /* PC13_VALUE 输出驱动模式 */
    if (drive == RTC_FOUT_DRIVE_PUSHPULL) {
        iosel |= PMU_IOSEL_PC13_VALUE_PP;
    } else {
        iosel &= ~PMU_IOSEL_PC13_VALUE_PP;
    }
    PMU->IOSEL = iosel;
}

//===========================================
// 第3层：高级功能 API
//===========================================

void rtc_get_time(rtc_time_t *time)
{
    assert(time != NULL);

    uint8_t sec1, sec2;

    /* 方式1：任意时刻安全读取（两次读秒一致性判断） */
    do {
        time->min   = (uint8_t)(RTC->MIN   & 0x7FU);
        time->hour  = (uint8_t)(RTC->HOUR  & 0x3FU);
        time->day   = (uint8_t)(RTC->DAY   & 0x3FU);
        time->week  = (uint8_t)(RTC->WEEK  & 0x7U);
        time->month = (uint8_t)(RTC->MONTH & 0x1FU);
        time->year  = (uint8_t)(RTC->YEAR  & 0xFFU);

        sec1 = (uint8_t)(RTC->SEC & 0x7FU);
        sec2 = (uint8_t)(RTC->SEC & 0x7FU);
    } while (sec1 != sec2);

    time->sec = sec1;
}

void rtc_get_time_isr(rtc_time_t *time)
{
    assert(time != NULL);

    /* 方式2：中断中读取，直接读取不需要重试 */
    time->sec   = (uint8_t)(RTC->SEC   & 0x7FU);
    time->min   = (uint8_t)(RTC->MIN   & 0x7FU);
    time->hour  = (uint8_t)(RTC->HOUR  & 0x3FU);
    time->day   = (uint8_t)(RTC->DAY   & 0x3FU);
    time->week  = (uint8_t)(RTC->WEEK  & 0x7U);
    time->month = (uint8_t)(RTC->MONTH & 0x1FU);
    time->year  = (uint8_t)(RTC->YEAR  & 0xFFU);
}

bool rtc_get_timestamp(rtc_tamper_channel_t channel, rtc_timestamp_t *stamp)
{
    assert(stamp != NULL);

    if (channel == RTC_TAMPER_1) {
        uint32_t clk  = RTC->CLKSTAMP1;
        uint32_t cal  = RTC->CALSTAMP1;
        stamp->sec   = (uint8_t)(clk & 0x7FU);
        stamp->min   = (uint8_t)((clk >> 8U) & 0x7FU);
        stamp->hour  = (uint8_t)((clk >> 16U) & 0x3FU);
        stamp->day   = (uint8_t)(cal & 0x3FU);
        stamp->week  = (uint8_t)((cal >> 8U) & 0x7U);
        stamp->month = (uint8_t)((cal >> 16U) & 0x1FU);
        stamp->year  = (uint8_t)((cal >> 21U) & 0xFFU);
    } else {
        uint32_t clk  = RTC->CLKSTAMP2;
        uint32_t cal  = RTC->CALSTAMP2;
        stamp->sec   = (uint8_t)(clk & 0x7FU);
        stamp->min   = (uint8_t)((clk >> 8U) & 0x7FU);
        stamp->hour  = (uint8_t)((clk >> 16U) & 0x3FU);
        stamp->day   = (uint8_t)(cal & 0x3FU);
        stamp->week  = (uint8_t)((cal >> 8U) & 0x7U);
        stamp->month = (uint8_t)((cal >> 16U) & 0x1FU);
        stamp->year  = (uint8_t)((cal >> 24U) & 0xFFU);
    }
    return true;
}

void rtc_enable_period_wakeup(rtc_period_t period)
{
    switch (period) {
        case RTC_PERIOD_SEC:
            RTC->IE |= (1UL << 3);   /* SEC_IE */
            break;
        case RTC_PERIOD_MIN:
            RTC->IE |= (1UL << 2);   /* MIN_IE */
            break;
        case RTC_PERIOD_HOUR:
            RTC->IE |= (1UL << 1);   /* HOUR_IE */
            break;
        case RTC_PERIOD_DATE:
            RTC->IE |= (1UL << 0);   /* DATE_IE */
            break;
        case RTC_PERIOD_2HZ:
            RTC->IE |= (1UL << 4);   /* 2HZ_IE */
            break;
        case RTC_PERIOD_4HZ:
            RTC->IE |= (1UL << 5);   /* 4HZ_IE */
            break;
        case RTC_PERIOD_8HZ:
            RTC->IE |= (1UL << 6);   /* 8HZ_IE */
            break;
        case RTC_PERIOD_16HZ:
            RTC->IE |= (1UL << 7);   /* 16HZ_IE */
            break;
        case RTC_PERIOD_64HZ:
            RTC->IE |= (1UL << 8);   /* 64HZ_IE */
            break;
        case RTC_PERIOD_256HZ:
            RTC->IE |= (1UL << 9);   /* 256HZ_IE */
            break;
        case RTC_PERIOD_1KHZ:
            RTC->IE |= (1UL << 10);  /* 1KHZ_IE */
            break;
        default:
            break;
    }
}

void rtc_disable_period_wakeup(rtc_period_t period)
{
    switch (period) {
        case RTC_PERIOD_SEC:
            RTC->IE &= ~(1UL << 3);
            break;
        case RTC_PERIOD_MIN:
            RTC->IE &= ~(1UL << 2);
            break;
        case RTC_PERIOD_HOUR:
            RTC->IE &= ~(1UL << 1);
            break;
        case RTC_PERIOD_DATE:
            RTC->IE &= ~(1UL << 0);
            break;
        case RTC_PERIOD_2HZ:
            RTC->IE &= ~(1UL << 4);
            break;
        case RTC_PERIOD_4HZ:
            RTC->IE &= ~(1UL << 5);
            break;
        case RTC_PERIOD_8HZ:
            RTC->IE &= ~(1UL << 6);
            break;
        case RTC_PERIOD_16HZ:
            RTC->IE &= ~(1UL << 7);
            break;
        case RTC_PERIOD_64HZ:
            RTC->IE &= ~(1UL << 8);
            break;
        case RTC_PERIOD_256HZ:
            RTC->IE &= ~(1UL << 9);
            break;
        case RTC_PERIOD_1KHZ:
            RTC->IE &= ~(1UL << 10);
            break;
        default:
            break;
    }
}

void rtc_enable_alarm(void)
{
    RTC->CR |= RTC_CR_ALM_EN_MSK;
}

void rtc_disable_alarm(void)
{
    RTC->CR &= ~RTC_CR_ALM_EN_MSK;
}

void rtc_enable_wakeup_timer(void)
{
    RTC->CR |= RTC_CR_WUTE_MSK;
}

void rtc_disable_wakeup_timer(void)
{
    RTC->CR &= ~RTC_CR_WUTE_MSK;
}

void rtc_enable_tamper(rtc_tamper_channel_t channel)
{
    if (channel == RTC_TAMPER_1) {
        RTC->CR |= RTC_CR_TAMP1EN_MSK;
    } else {
        RTC->CR |= RTC_CR_TAMP2EN_MSK;
    }
}

void rtc_disable_tamper(rtc_tamper_channel_t channel)
{
    if (channel == RTC_TAMPER_1) {
        RTC->CR &= ~RTC_CR_TAMP1EN_MSK;
    } else {
        RTC->CR &= ~RTC_CR_TAMP2EN_MSK;
    }
}

void rtc_set_wut_reload(uint16_t reload)
{
    RTC->WUTR = (uint32_t)reload;
}

bool rtc_backup_write(uint8_t index, uint32_t value)
{
    if (index >= RTC_BAKUP_COUNT) {
        return false;
    }

    /* BAKUP寄存器数组从偏移0x70开始 */
    volatile uint32_t *bakup_regs = &RTC->BAKUP0;
    bakup_regs[index] = value;
    return true;
}

bool rtc_backup_read(uint8_t index, uint32_t *value)
{
    assert(value != NULL);

    if (index >= RTC_BAKUP_COUNT) {
        return false;
    }

    volatile uint32_t *bakup_regs = &RTC->BAKUP0;
    *value = bakup_regs[index];
    return true;
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void rtc_unlock(void)
{
    write_wp(RTC_WP_UNLOCK_KEY);
}

void rtc_lock(void)
{
    write_wp(0x00000000U);
}

bool rtc_is_unlocked(void)
{
    return (RTC->WP & 0x1U) != 0U;
}

uint32_t rtc_get_flags(void)
{
    return RTC->SR & RTC_SR_CLEARABLE_MSK;
}

void rtc_clear_flags(uint32_t flags)
{
    /* 只清除可清除的位（RCW1：写1清零） */
    write_sr(flags & RTC_SR_CLEARABLE_MSK);
}

void rtc_irq_config(uint32_t irq_mask, bool enable)
{
    if (enable) {
        RTC->IE |= (irq_mask & RTC_IE_ALL_MSK);
    } else {
        RTC->IE &= ~(irq_mask & RTC_IE_ALL_MSK);
    }
}

uint32_t rtc_get_irq_enabled(void)
{
    return RTC->IE & RTC_IE_ALL_MSK;
}

void rtc_irq_register(rtc_callback_t callback, void *context)
{
    s_state.callback = callback;
    s_state.context  = context;
}

bool rtc_is_wut_writable(void)
{
    return (RTC->SR & (1UL << 24)) != 0U;
}

void rtc_get_calibration(rtc_calib_sign_t *sign, uint16_t *value)
{
    uint32_t adj = RTC->ADJUST;

    if (sign != NULL) {
        *sign = (adj & RTC_ADJUST_ADJSIGN_MSK) ? RTC_CALIB_SIGN_NEGATIVE : RTC_CALIB_SIGN_POSITIVE;
    }
    if (value != NULL) {
        *value = (uint16_t)(adj & RTC_ADJUST_ADJVALUE_MSK);
    }
}

uint8_t rtc_get_subsecond(void)
{
    return (uint8_t)(RTC->MSECCNT & 0x3FU);
}

//===========================================
// 中断处理函数
//===========================================

/**
 * @brief RTC全局中断处理程序（NVIC INT2）
 *
 * 读取RTC_SR获取全部置位标志，清除已处理标志，调用用户回调。
 * RTC模块产生单一全局中断信号RTC_INT，为所有使能中断标志相或。
 * 连接到：NVIC INT2（中断功能）、EXTI线17（STOP唤醒）、PMU（STANDBY唤醒）。
 */
void RTC_IRQHandler(void)
{
    /* 读取当前所有中断标志 */
    uint32_t flags = rtc_get_flags();

    /* 获取已使能的中断 */
    uint32_t enabled = rtc_get_irq_enabled();

    /* 保留已使能且已置位的标志 */
    uint32_t pending = flags & enabled;

    if (pending != 0U) {
        /* 清除已处理的中断标志 */
        rtc_clear_flags(pending);

        /* 调用用户注册的回调函数 */
        if (s_state.callback != NULL) {
            s_state.callback(pending, s_state.context);
        }
    }
}
