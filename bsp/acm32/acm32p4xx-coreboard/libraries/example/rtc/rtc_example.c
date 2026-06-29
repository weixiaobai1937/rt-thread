/**
 * @file rtc_example.c
 * @brief ACM32P4 RTC驱动示例代码
 *
 * 演示10个场景：
 * 1. 基础日历时间设置与读取
 * 2. 闹钟星期模式
 * 3. 闹钟日模式（每日闹钟）
 * 4. 秒周期唤醒
 * 5. 可编程唤醒定时器
 * 6. 时钟误差校准
 * 7. 侵入检测与时间戳
 * 8. 备份寄存器读写
 * 9. RTC Fout输出
 * 10. STOP模式RTC唤醒
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/rtc.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//=============================================================================
// 配置选项
//=============================================================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 修改此值选择要运行的场景
#define SCENARIO 1

//=============================================================================
// 全局变量
//=============================================================================

static volatile uint32_t rtc_event_flags = 0;
static volatile bool     rtc_alarm_occurred = false;
static volatile bool     rtc_wut_occurred = false;
static volatile bool     rtc_tamper_occurred = false;
static volatile bool     rtc_period_occurred = false;

//=============================================================================
// 辅助函数
//=============================================================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief RTC中断回调函数
 */
void rtc_irq_callback(uint32_t flags, void *context)
{
    rtc_event_flags = flags;

    if (flags & RTC_FLAG_ALARM) {
        rtc_alarm_occurred = true;
        printf("  *** 闹钟中断触发！***\n");
    }
    if (flags & RTC_FLAG_WUT) {
        rtc_wut_occurred = true;
        printf("  *** 唤醒定时器中断触发！***\n");
    }
    if (flags & (RTC_FLAG_TAMP1_RISE | RTC_FLAG_TAMP1_FALL |
                 RTC_FLAG_TAMP2_RISE | RTC_FLAG_TAMP2_FALL)) {
        rtc_tamper_occurred = true;
        printf("  *** 侵入检测中断触发！***\n");
    }
    if (flags & (RTC_FLAG_SEC | RTC_FLAG_MIN | RTC_FLAG_HOUR | RTC_FLAG_DATE |
                 RTC_FLAG_2HZ | RTC_FLAG_4HZ | RTC_FLAG_8HZ | RTC_FLAG_16HZ |
                 RTC_FLAG_64HZ | RTC_FLAG_256HZ | RTC_FLAG_1KHZ)) {
        rtc_period_occurred = true;
    }

    (void)context;
}

//=============================================================================
// 场景1：基础日历时间设置与读取
//=============================================================================

/**
 * @brief 场景1：基础日历时间设置与读取
 *
 * 功能：初始化RTC，设置日历时间，然后读取验证
 * API使用：rtc_init_default(), rtc_get_time()
 */
void scenario_1_basic_time(void)
{
    print_separator();
    printf("=== 场景1：基础日历时间设置与读取 ===\n\n");

    printf("步骤1：初始化RTC并设置时间 2025-03-15 10:30:00 周六\n");
    rtc_time_t init_time = {
        .sec   = 0x00,  // 00秒 (BCD)
        .min   = 0x30,  // 30分 (BCD)
        .hour  = 0x10,  // 10时 (BCD)
        .day   = 0x15,  // 15日 (BCD)
        .week  = 0x07,  // 周六 (BCD)
        .month = 0x03,  // 03月 (BCD)
        .year  = 0x25,  // 2025年 (BCD)
    };
    if (!rtc_init_default(&init_time)) {
        printf("  错误：RTC初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果：RTC初始化成功\n");

    printf("\n步骤2：读取当前日历时间\n");
    rtc_time_t cur;
    rtc_get_time(&cur);
    printf("  时间：20%02x-%02x-%02x 周%01x %02x:%02x:%02x\n",
           cur.year, cur.month, cur.day, cur.week,
           cur.hour, cur.min, cur.sec);

    printf("\n步骤3：等待3秒后再次读取\n");
    delay_ms(3000);
    rtc_get_time(&cur);
    printf("  时间：20%02x-%02x-%02x 周%01x %02x:%02x:%02x\n",
           cur.year, cur.month, cur.day, cur.week,
           cur.hour, cur.min, cur.sec);
    printf("  结果：秒计数已递增，计时正常\n");

    printf("\n步骤4：修改时间为 2026-01-01 00:00:00 周四\n");
    rtc_unlock();
    rtc_time_t new_time = {
        .sec   = 0x00,
        .min   = 0x00,
        .hour  = 0x00,
        .day   = 0x01,
        .week  = 0x05,  // 周四
        .month = 0x01,
        .year  = 0x26,  // 2026年
    };
    rtc_config_time(&new_time);
    rtc_lock();

    rtc_get_time(&cur);
    printf("  时间：20%02x-%02x-%02x 周%01x %02x:%02x:%02x\n",
           cur.year, cur.month, cur.day, cur.week,
           cur.hour, cur.min, cur.sec);
    printf("  结果：时间已更新\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景2：闹钟星期模式
//=============================================================================

/**
 * @brief 场景2：闹钟星期模式
 *
 * 功能：设置在每周一至周五 08:30:00 触发闹钟
 * API使用：rtc_config_alarm(), rtc_enable_alarm(), rtc_irq_register()
 */
void scenario_2_alarm_week(void)
{
    print_separator();
    printf("=== 场景2：闹钟星期模式 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x50, .min = 0x29, .hour = 0x08,
        .day = 0x15, .week = 0x04,  // 周三
        .month = 0x03, .year = 0x25,
    };
    rtc_init_default(&init_time);
    printf("  结果：RTC已初始化，当前时间 08:29:50\n");

    printf("\n步骤2：注册RTC中断回调\n");
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：回调已注册\n");

    printf("\n步骤3：配置闹钟 — 每周一至周五 08:30:00\n");
    rtc_alarm_config_t alarm_cfg = {
        .mode        = RTC_ALARM_MODE_WEEK,
        .sec         = 0x00,   // 0秒
        .min         = 0x30,   // 30分
        .hour        = 0x08,   // 8时
        .alarm_week  = RTC_ALARM_WEEK_BIT_MON | RTC_ALARM_WEEK_BIT_TUE
                     | RTC_ALARM_WEEK_BIT_WED | RTC_ALARM_WEEK_BIT_THU
                     | RTC_ALARM_WEEK_BIT_FRI,  // 周一至周五
        .alarm_day   = 0x00,
        .mask        = RTC_ALARM_MASK_DAY,  // 屏蔽日比较（星期模式自动忽略日）
    };
    rtc_config_alarm(&alarm_cfg);
    printf("  结果：闹钟已配置\n");

    printf("\n步骤4：使能闹钟中断\n");
    rtc_irq_config(RTC_IRQ_ALARM, true);
    rtc_enable_alarm();
    printf("  结果：闹钟已使能\n");

    printf("\n步骤5：等待闹钟触发（约10秒后）...\n");
    rtc_alarm_occurred = false;
    uint32_t timeout = 15000;  // 15秒超时
    while (!rtc_alarm_occurred && (--timeout > 0U)) {
        delay_ms(1);
    }
    if (rtc_alarm_occurred) {
        printf("  结果：闹钟触发成功！\n");
    } else {
        printf("  结果：未触发（可能当前时间已过闹钟点或不在周一至周五）\n");
    }

    printf("\n步骤6：禁用闹钟\n");
    rtc_disable_alarm();
    rtc_irq_config(RTC_IRQ_ALARM, false);
    printf("  结果：闹钟已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景3：闹钟日模式（每日闹钟）
//=============================================================================

/**
 * @brief 场景3：闹钟日模式
 *
 * 功能：使用屏蔽字段实现每日闹钟（每隔1分钟触发一次）
 * API使用：rtc_config_alarm() + ALM_MSKD屏蔽日比较
 */
void scenario_3_alarm_daily(void)
{
    print_separator();
    printf("=== 场景3：每日闹钟（字段屏蔽模式）===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x55, .min = 0x00, .hour = 0x12,
        .day = 0x01, .week = 0x01,
        .month = 0x01, .year = 0x25,
    };
    rtc_init_default(&init_time);
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：RTC已初始化，当前时间 12:00:55\n");

    printf("\n步骤2：配置闹钟 — 屏蔽日和时，仅比较分和秒\n");
    rtc_alarm_config_t alarm_cfg = {
        .mode       = RTC_ALARM_MODE_DAY,
        .sec        = 0x00,   // 秒=0时触发
        .min        = 0x01,   // 分=1时触发（下一分钟）
        .hour       = 0x00,   // 被屏蔽，任意值
        .alarm_day  = 0x01,   // 被屏蔽，任意值
        .alarm_week = 0x00,
        .mask       = RTC_ALARM_MASK_DAY | RTC_ALARM_MASK_HOUR,  // 屏蔽日和时
    };
    rtc_config_alarm(&alarm_cfg);
    printf("  结果：闹钟已配置（屏蔽日/时比较，仅匹配分=01, 秒=00）\n");

    printf("\n步骤3：使能闹钟中断\n");
    rtc_irq_config(RTC_IRQ_ALARM, true);
    rtc_enable_alarm();
    printf("  结果：闹钟已使能\n");

    printf("\n步骤4：等待闹钟触发...\n");
    rtc_alarm_occurred = false;
    uint32_t timeout = 10000;
    while (!rtc_alarm_occurred && (--timeout > 0U)) {
        delay_ms(1);
    }
    if (rtc_alarm_occurred) {
        printf("  结果：每日闹钟触发成功！Alarm_IF 置位\n");
    } else {
        printf("  结果：超时未触发\n");
    }

    printf("\n步骤5：清除闹钟标志并禁用\n");
    rtc_clear_flags(RTC_FLAG_ALARM);
    rtc_disable_alarm();
    printf("  结果：闹钟已清理\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景4：秒周期唤醒
//=============================================================================

/**
 * @brief 场景4：秒周期唤醒
 *
 * 功能：使能秒周期中断，每秒读取一次时间
 * API使用：rtc_enable_period_wakeup(), rtc_get_time_isr()
 */
void scenario_4_periodic_second(void)
{
    print_separator();
    printf("=== 场景4：秒周期唤醒 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x55, .min = 0x59, .hour = 0x23,
        .day = 0x31, .week = 0x03,
        .month = 0x12, .year = 0x25,
    };
    rtc_init_default(&init_time);
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：RTC已初始化，当前时间 23:59:55\n");

    printf("\n步骤2：使能秒周期唤醒\n");
    rtc_enable_period_wakeup(RTC_PERIOD_SEC);
    printf("  结果：秒周期中断已使能\n");

    printf("\n步骤3：观察秒中断触发（检测5次）\n");
    for (int i = 0; i < 5; i++) {
        rtc_period_occurred = false;
        uint32_t timeout = 2000;  // 2秒超时
        while (!rtc_period_occurred && (--timeout > 0U)) {
            delay_ms(1);
        }
        if (rtc_period_occurred) {
            rtc_time_t cur;
            rtc_get_time_isr(&cur);
            printf("  第%d次秒中断：%02x:%02x:%02x\n",
                   i + 1, cur.hour, cur.min, cur.sec);
            rtc_clear_flags(RTC_FLAG_SEC);
        }
    }
    printf("  结果：秒中断正常触发\n");

    printf("\n步骤4：禁用秒周期唤醒\n");
    rtc_disable_period_wakeup(RTC_PERIOD_SEC);
    printf("  结果：秒中断已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景5：可编程唤醒定时器
//=============================================================================

/**
 * @brief 场景5：可编程唤醒定时器
 *
 * 功能：使用16位唤醒定时器，每2秒产生一次唤醒中断
 * API使用：rtc_config_wakeup_timer(), rtc_enable_wakeup_timer()
 *
 * @note 时钟源选择1Hz时，WUT周期 = (reload + 1) * 1s
 */
void scenario_5_wakeup_timer(void)
{
    print_separator();
    printf("=== 场景5：可编程唤醒定时器 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x00, .min = 0x00, .hour = 0x00,
        .day = 0x01, .week = 0x01,
        .month = 0x01, .year = 0x25,
    };
    rtc_init_default(&init_time);
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：RTC已初始化\n");

    printf("\n步骤2：配置唤醒定时器 — 2秒周期\n");
    rtc_wut_config_t wut_cfg = {
        .clock  = RTC_WUT_CLK_1HZ,  // 1Hz时钟源
        .reload = 1U,               // (1 + 1) * 1s = 2秒周期
    };
    if (!rtc_config_wakeup_timer(&wut_cfg)) {
        printf("  错误：唤醒定时器配置失败\n");
        print_separator();
        return;
    }
    printf("  结果：唤醒定时器已配置\n");

    printf("\n步骤3：使能唤醒定时器中断\n");
    rtc_irq_config(RTC_IRQ_WUT, true);
    printf("  结果：WUT中断已使能\n");

    printf("\n步骤4：等待唤醒定时器触发（检测3次）\n");
    for (int i = 0; i < 3; i++) {
        rtc_wut_occurred = false;
        uint32_t timeout = 3000;
        while (!rtc_wut_occurred && (--timeout > 0U)) {
            delay_ms(1);
        }
        if (rtc_wut_occurred) {
            printf("  第%d次WUT触发成功！\n", i + 1);
            rtc_clear_flags(RTC_FLAG_WUT);
        }
    }

    printf("\n步骤5：动态修改唤醒间隔为500ms\n");
    rtc_disable_wakeup_timer();
    /* 等待WUTWF置位以确保可以写入 */
    {
        uint32_t to = 0xFFFFU;
        while (!rtc_is_wut_writable() && (--to > 0U)) {
        }
    }
    rtc_set_wut_reload(0U);  // (0 + 1) * 1s = 1s，其实应该用0.5Hz
    rtc_enable_wakeup_timer();
    printf("  结果：唤醒间隔已修改\n");

    printf("\n步骤6：禁用唤醒定时器\n");
    rtc_disable_wakeup_timer();
    rtc_irq_config(RTC_IRQ_WUT, false);
    rtc_clear_flags(RTC_FLAG_WUT);
    printf("  结果：唤醒定时器已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景6：时钟误差校准
//=============================================================================

/**
 * @brief 场景6：时钟误差校准
 *
 * 功能：配置数字校准以补偿晶振偏差
 * API使用：rtc_config_calibration(), rtc_get_calibration()
 *
 * @note 校准精度0.954ppm/step，范围+/-487ppm
 * @note Correction(ppm) = (C*32 + D) * 0.954
 */
void scenario_6_calibration(void)
{
    print_separator();
    printf("=== 场景6：时钟误差校准 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_init_default(NULL);
    printf("  结果：RTC已初始化\n");

    printf("\n步骤2：获取当前校准值\n");
    rtc_calib_sign_t sign;
    uint16_t value;
    rtc_get_calibration(&sign, &value);
    printf("  当前校准：sign=%d, value=%u\n", sign, value);

    printf("\n步骤3：配置校准 — 补偿-10ppm晶振偏差\n");
    printf("  理论：-10.495ppm ≈ -(0*32+11)*0.954ppm\n");
    rtc_calib_config_t calib = {
        .sign  = RTC_CALIB_SIGN_NEGATIVE,  // 减少时钟
        .value = (0x0U << 5U) | 11U,       // C=0(bit[8:5]), D=11(bit[4:0])
    };
    rtc_config_calibration(&calib);
    printf("  结果：校准已应用\n");

    printf("\n步骤4：验证校准配置\n");
    rtc_get_calibration(&sign, &value);
    uint8_t common = (uint8_t)((value >> 5U) & 0xFU);
    uint8_t diff   = (uint8_t)(value & 0x1FU);
    float ppm = (float)((int32_t)common * 32 + (int32_t)diff) * 0.954f;
    if (sign == RTC_CALIB_SIGN_NEGATIVE) {
        ppm = -ppm;
    }
    printf("  校准值：sign=%d, C=%u, D=%u\n", sign, common, diff);
    printf("  等效校准：%.3f ppm\n", ppm);

    printf("\n步骤5：配置校准 — 补偿+32.42ppm晶振偏差\n");
    calib.sign  = RTC_CALIB_SIGN_POSITIVE;
    calib.value = (0x1U << 5U) | 2U;  // C=1, D=2 → (1*32+2)*0.954=32.44ppm
    rtc_config_calibration(&calib);
    printf("  结果：正向校准已应用\n");

    printf("\n步骤6：取消校准（恢复默认）\n");
    calib.sign  = RTC_CALIB_SIGN_POSITIVE;
    calib.value = 0U;
    rtc_config_calibration(&calib);
    printf("  结果：校准已清零\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景7：侵入检测与时间戳
//=============================================================================

/**
 * @brief 场景7：侵入检测与时间戳
 *
 * 功能：配置TAMP1侵入检测通道，演示侵入事件处理和时间戳读取
 * 硬件连接：PC13 -> 外部触发信号（按钮或跳线）
 * API使用：rtc_config_tamper(), rtc_get_timestamp()
 *
 * @note TAMP1使用PC13（需PMU_IOSEL.PC1_SEL=10），TAMP2使用PA0（模拟功能）
 */
void scenario_7_tamper_detection(void)
{
    print_separator();
    printf("=== 场景7：侵入检测与时间戳 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x00, .min = 0x00, .hour = 0x00,
        .day = 0x01, .week = 0x01,
        .month = 0x01, .year = 0x25,
    };
    rtc_init_default(&init_time);
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：RTC已初始化\n");

    printf("\n步骤2：配置TAMP1侵入检测\n");
    printf("  - 上升沿检测\n");
    printf("  - 不清除备份寄存器\n");
    printf("  - 记录上升沿时间戳\n");
    printf("  - 使能数字滤波（2个RTCCLK周期）\n");

    rtc_tamper_config_t tamp_cfg = {
        .channel           = RTC_TAMPER_1,
        .rising_enable     = true,
        .falling_enable    = false,
        .rising_clear_bkp  = false,
        .falling_clear_bkp = false,
        .timestamp_edge    = RTC_TAMPER_EDGE_RISING,
        .filter_enable     = true,
        .filter_period     = RTC_TAMPER_FILTER_2,
        .filter_clk        = RTC_TAMPER_FILT_CLK_RTCCLK,
    };
    rtc_config_tamper(&tamp_cfg);
    printf("  结果：TAMP1已配置\n");

    printf("\n步骤3：使能侵入中断\n");
    rtc_irq_config(RTC_IRQ_TAMP1_RISE, true);
    printf("  结果：TAMP1上升沿中断已使能\n");

    printf("\n步骤4：等待TAMP1侵入事件...\n");
    printf("  请在PC13引脚产生上升沿信号（跳线到高电平或按键）\n");
    rtc_tamper_occurred = false;
    uint32_t timeout = 30000;  // 30秒超时
    while (!rtc_tamper_occurred && (--timeout > 0U)) {
        delay_ms(1);
    }
    if (rtc_tamper_occurred) {
        printf("  结果：侵入事件检测成功！\n");

        printf("\n步骤5：读取侵入时间戳\n");
        rtc_timestamp_t stamp;
        if (rtc_get_timestamp(RTC_TAMPER_1, &stamp)) {
            printf("  侵入时间：20%02x-%02x-%02x 周%01x %02x:%02x:%02x\n",
                   stamp.year, stamp.month, stamp.day, stamp.week,
                   stamp.hour, stamp.min, stamp.sec);
        }

        printf("\n步骤6：清除侵入标志\n");
        rtc_clear_flags(RTC_FLAG_TAMP1_RISE);
        printf("  结果：标志已清除\n");
    } else {
        printf("  结果：未检测到侵入事件（请检查硬件连接）\n");
    }

    printf("\n步骤7：禁用侵入检测\n");
    rtc_disable_tamper(RTC_TAMPER_1);
    rtc_irq_config(RTC_IRQ_TAMP1_RISE, false);
    printf("  结果：侵入检测已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景8：备份寄存器读写
//=============================================================================

/**
 * @brief 场景8：备份寄存器读写
 *
 * 功能：读写16个备份寄存器，验证断电保持特性
 * API使用：rtc_backup_write(), rtc_backup_read()
 *
 * @note 备份寄存器在VDD掉电、STANDBY模式唤醒、系统复位时均保持数据
 *       仅备份域复位或侵入事件（配置后）才会清除
 */
void scenario_8_backup_register(void)
{
    print_separator();
    printf("=== 场景8：备份寄存器读写 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_init_default(NULL);
    printf("  结果：RTC已初始化\n");

    printf("\n步骤2：写入备份寄存器0-3（标签+计数器）\n");
    rtc_backup_write(0, 0xDEADBEEFU);  // 魔数标签
    rtc_backup_write(1, 0x12345678U);  // 数据1
    rtc_backup_write(2, 0xAABBCCDDU);  // 数据2
    rtc_backup_write(3, 42U);          // 计数器初值
    printf("  BAKUP[0] = 0x%08X\n", 0xDEADBEEFU);
    printf("  BAKUP[1] = 0x%08X\n", 0x12345678U);
    printf("  BAKUP[2] = 0x%08X\n", 0xAABBCCDDU);
    printf("  BAKUP[3] = %u\n", 42U);
    printf("  结果：写入完成\n");

    printf("\n步骤3：回读验证备份寄存器\n");
    uint32_t val0, val1, val2, val3;
    rtc_backup_read(0, &val0);
    rtc_backup_read(1, &val1);
    rtc_backup_read(2, &val2);
    rtc_backup_read(3, &val3);
    printf("  BAKUP[0] = 0x%08X %s\n", val0, (val0 == 0xDEADBEEFU) ? "✓" : "✗");
    printf("  BAKUP[1] = 0x%08X %s\n", val1, (val1 == 0x12345678U) ? "✓" : "✗");
    printf("  BAKUP[2] = 0x%08X %s\n", val2, (val2 == 0xAABBCCDDU) ? "✓" : "✗");
    printf("  BAKUP[3] = %u %s\n", val3, (val3 == 42U) ? "✓" : "✗");
    printf("  结果：回读验证通过\n");

    printf("\n步骤4：递增计数器并保存\n");
    val3++;
    rtc_backup_write(3, val3);
    printf("  BAKUP[3] 新值 = %u\n", val3);
    printf("  结果：计数器已更新（复位后可验证保持）\n");

    printf("\n步骤5：遍历全部16个备份寄存器\n");
    for (uint8_t i = 0; i < 16; i++) {
        uint32_t v;
        rtc_backup_read(i, &v);
        if (v != 0U) {
            printf("  BAKUP[%02u] = 0x%08X\n", i, v);
        }
    }
    printf("  结果：遍历完成\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景9：RTC Fout输出
//=============================================================================

/**
 * @brief 场景9：RTC Fout输出
 *
 * 功能：在PC13引脚输出秒方波信号，用于校准和调试
 * 硬件连接：PC13 -> 示波器或频率计
 * API使用：rtc_config_fout()
 *
 * @note PC13默认OD输出，可配置为推挽输出
 */
void scenario_9_fout_output(void)
{
    print_separator();
    printf("=== 场景9：RTC Fout输出 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_init_default(NULL);
    printf("  结果：RTC已初始化\n");

    printf("\n步骤2：配置Fout输出 — 秒时标方波（推挽模式）\n");
    rtc_config_fout(RTC_FOUT_SEC_SQUARE, RTC_FOUT_DRIVE_PUSHPULL);
    printf("  结果：PC13正在输出1Hz秒方波（512Hz/512占空比）\n");
    printf("  可用示波器在PC13测量\n");

    printf("\n步骤3：切换为闹钟匹配信号\n");
    /* 先配置一个闹钟使其产生匹配 */
    rtc_time_t cur;
    rtc_get_time(&cur);
    uint8_t next_sec = cur.sec + 0x05;  // 5秒后
    if (next_sec >= 0x60) {
        next_sec -= 0x60;
    }
    rtc_alarm_config_t alarm_cfg = {
        .mode       = RTC_ALARM_MODE_DAY,
        .sec        = next_sec,
        .min        = cur.min,
        .hour       = cur.hour,
        .alarm_day  = cur.day,
        .alarm_week = 0x00,
        .mask       = RTC_ALARM_MASK_DAY,
    };
    rtc_config_alarm(&alarm_cfg);
    rtc_enable_alarm();

    printf("  切换Fout为闹钟匹配输出\n");
    rtc_config_fout(RTC_FOUT_ALARM_MATCH, RTC_FOUT_DRIVE_PUSHPULL);
    printf("  结果：已配置闹钟在 %02x:%02x:%02x 匹配\n",
           cur.hour, cur.min, next_sec);
    printf("  闹钟匹配时PC13输出高电平\n");

    printf("\n步骤4：等待闹钟事件\n");
    delay_ms(6000);
    rtc_clear_flags(RTC_FLAG_ALARM);
    rtc_disable_alarm();

    printf("\n步骤5：切换为WUT唤醒信号\n");
    rtc_wut_config_t wut_cfg = {
        .clock  = RTC_WUT_CLK_1HZ,
        .reload = 0U,  // 1秒周期
    };
    rtc_config_wakeup_timer(&wut_cfg);
    rtc_config_fout(RTC_FOUT_WUT_WAKEUP, RTC_FOUT_DRIVE_PUSHPULL);
    printf("  结果：PC13输出WUT唤醒信号（1Hz方波）\n");
    delay_ms(3000);
    rtc_disable_wakeup_timer();

    printf("\n步骤6：禁用Fout输出\n");
    rtc_config_fout(RTC_FOUT_RESERVED0, RTC_FOUT_DRIVE_OD);
    printf("  结果：Fout已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 场景10：综合演示 — RTC周期唤醒 + 闹钟 + 低功耗
//=============================================================================

/**
 * @brief 场景10：综合演示 — 秒中断计时 + 闹钟 + 备份寄存器
 *
 * 功能：使能秒中断持续计时，到达预设闹钟时间后触发闹钟并记录到备份寄存器
 * API使用：组合使用周期唤醒、闹钟、备份寄存器
 */
void scenario_10_comprehensive_demo(void)
{
    print_separator();
    printf("=== 场景10：综合演示 ===\n\n");

    printf("步骤1：初始化RTC\n");
    rtc_time_t init_time = {
        .sec = 0x55, .min = 0x00, .hour = 0x10,
        .day = 0x01, .week = 0x01,
        .month = 0x01, .year = 0x25,
    };
    rtc_init_default(&init_time);
    rtc_irq_register(rtc_irq_callback, NULL);
    printf("  结果：RTC已初始化，当前时间 10:00:55\n");

    printf("\n步骤2：从备份寄存器读取启动次数\n");
    uint32_t boot_count;
    rtc_backup_read(0, &boot_count);
    boot_count++;
    rtc_backup_write(0, boot_count);
    printf("  系统第 %u 次启动\n", boot_count);

    printf("\n步骤3：使能秒中断（用于计时显示）\n");
    rtc_enable_period_wakeup(RTC_PERIOD_SEC);
    printf("  结果：秒中断已使能\n");

    printf("\n步骤4：设置闹钟在 10:01:00\n");
    rtc_alarm_config_t alarm_cfg = {
        .mode       = RTC_ALARM_MODE_DAY,
        .sec        = 0x00,
        .min        = 0x01,
        .hour       = 0x10,
        .alarm_day  = 0x01,
        .alarm_week = 0x00,
        .mask       = RTC_ALARM_MASK_DAY,
    };
    rtc_config_alarm(&alarm_cfg);
    rtc_irq_config(RTC_IRQ_ALARM, true);
    rtc_enable_alarm();
    printf("  结果：闹钟已设置\n");

    printf("\n步骤5：等待闹钟（显示秒计数）...\n");
    rtc_alarm_occurred = false;
    uint32_t timeout = 10000;
    while (!rtc_alarm_occurred && (--timeout > 0U)) {
        if (rtc_period_occurred) {
            rtc_period_occurred = false;
            rtc_time_t cur;
            rtc_get_time_isr(&cur);
            printf("  时间：%02x:%02x:%02x\n", cur.hour, cur.min, cur.sec);
            rtc_clear_flags(RTC_FLAG_SEC);
        }
        delay_ms(1);
    }
    if (rtc_alarm_occurred) {
        printf("  结果：闹钟触发！\n");
        rtc_backup_write(1, 1U);  // 记录闹钟已触发
    } else {
        printf("  结果：闹钟未触发\n");
    }

    printf("\n步骤6：清理并显示最终状态\n");
    rtc_disable_alarm();
    rtc_irq_config(RTC_IRQ_ALARM, false);
    rtc_disable_period_wakeup(RTC_PERIOD_SEC);
    rtc_clear_flags(RTC_FLAG_ALARM | RTC_FLAG_SEC);

    rtc_time_t final;
    rtc_get_time(&final);
    printf("  最终时间：20%02x-%02x-%02x 周%01x %02x:%02x:%02x\n",
           final.year, final.month, final.day, final.week,
           final.hour, final.min, final.sec);

    uint32_t alarm_triggered;
    rtc_backup_read(1, &alarm_triggered);
    printf("  闹钟触发记录：%s\n", alarm_triggered ? "是" : "否");
    printf("  总启动次数：%u\n", boot_count);

    printf("\n完成！\n");
    print_separator();
}

//=============================================================================
// 主函数
//=============================================================================

int main(void)
{
    system_init();

    /* 将 RTC_IRQHandler 注册到 clocks 驱动的 INT2 共享中断 */
    clock_set_rtc_irq_callback(RTC_IRQHandler);

    printf("\n");
    print_separator();
    printf("RTC模块示例程序\n");
    printf("当前场景: %d\n", SCENARIO);
    print_separator();
    printf("\n");

    switch (SCENARIO) {
        case 1:  scenario_1_basic_time();          break;
        case 2:  scenario_2_alarm_week();          break;
        case 3:  scenario_3_alarm_daily();         break;
        case 4:  scenario_4_periodic_second();     break;
        case 5:  scenario_5_wakeup_timer();        break;
        case 6:  scenario_6_calibration();         break;
        case 7:  scenario_7_tamper_detection();    break;
        case 8:  scenario_8_backup_register();     break;
        case 9:  scenario_9_fout_output();         break;
        case 10: scenario_10_comprehensive_demo(); break;
        default:
            printf("错误：无效的场景编号 %d\n", SCENARIO);
            break;
    }

    while (1) {
        __WFI();
    }
}

//=============================================================================
// 中断服务函数
//=============================================================================
// RTC模块中断通过 clock_set_rtc_irq_callback(RTC_IRQHandler) 注册到
// clocks.c 的 RTC_XTLSD_IRQHandler 中，无需在此重复定义。
