/**
 * @file pmu_example.c
 * @brief PMU（电源管理单元）功能示例程序
 *
 * @details
 * 演示6个场景：
 * 1. SLEEP模式基础用法
 * 2. STOP模式低功耗应用
 * 3. STANDBY模式最低功耗应用
 * 4. BOR欠压保护应用
 * 5. LVD低压检测+中断应用
 * 6. 唤醒源判断应用
 * 
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

//===========================================
// 头文件包含
//===========================================

#include "acm32p4.h"
#include "hardware/pmu.h"
#include "hardware/system.h"
#include "hardware/gpio.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（1~6）
#define DEFAULT_SCENARIO 1

//===========================================
// 全局变量
//===========================================

//===========================================
// 辅助函数
//===========================================

/**
 * @brief 打印分隔线
 */
void print_separator(void)
{
    printf("========================================\n");
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：SLEEP模式基础用法
 * 
 * 演示内容：
 * - 配置GPIO唤醒中断
 * - 进入SLEEP模式
 * - GPIO中断唤醒
 */
void scenario_1_sleep_basic(void)
{
    print_separator();
    printf("=== 场景1：SLEEP模式基础用法 ===\n\n");

    // 1. 配置按键引脚 PA0 为上拉输入（WKUP1 引脚）
    gpio_init_input_pullup(PA0);

    // 2. 配置 EXTI 中断——PA0 上升沿触发
    exti_gpio_init(EXTI_GPIO_PORTA, 0, EXTI_TRIGGER_RISING);
    NVIC_EnableIRQ(EXTI0_IRQn);

    printf("按 PA0 按键进入 SLEEP 模式...\n");
    delay_ms(500);

    // 3. 进入 SLEEP 模式（按键中断唤醒）
    pmu_enter_sleep();

    // 4. 唤醒后提示
    printf("已从 SLEEP 唤醒！\n");
    printf("完成！\n");
    print_separator();
}

/**
 * @brief 场景2：STOP模式低功耗应用
 */
void scenario_2_stop_lowpower(void)
{
    pmu_stop_config_t stop_cfg;

    print_separator();
    printf("=== 场景2：STOP模式低功耗应用 ===\n\n");

    // 1. 配置唤醒源——PA0 上升沿
    gpio_init_input_pullup(PA0);
    exti_gpio_init(EXTI_GPIO_PORTA, 0, EXTI_TRIGGER_RISING);
    NVIC_EnableIRQ(EXTI0_IRQn);

    // 2. 自定义 STOP 配置：RCH 自动关闭 + 降低 MLDO 电压
    pmu_get_default_stop_config(&stop_cfg);
    stop_cfg.rch_auto_off = true;       // STOP 后自动关闭 RCH 省电
    stop_cfg.mldo_lowpower = true;       // MLDO 进入低功耗模式
    stop_cfg.mldo_voltage = PMU_MLDO_1V0; // 电压降至 1.0V
    stop_cfg.wakeup_wait_cycles = 0x3C0;

    printf("STOP 配置：RCH=自动关闭, MLDO=1.0V 低功耗\n");
    printf("唤醒后需重新初始化系统时钟\n");

    delay_ms(500);

    // 3. 进入 STOP 模式
    pmu_enter_stop(&stop_cfg);

    // 4. 唤醒后重新初始化时钟（RCH 已恢复但需重配 PLL 等）
    printf("已从 STOP 唤醒，重新初始化时钟...\n");
    clock_init_default();

    printf("完成！\n");
    print_separator();
}

/**
 * @brief 场景3：STANDBY模式最低功耗应用
 */
void scenario_3_standby_ultralow(void)
{
    pmu_standby_config_t standby_cfg;

    print_separator();
    printf("=== 场景3：STANDBY模式最低功耗应用 ===\n\n");

    // 检查是否从 STANDBY 唤醒
    if (pmu_is_standby_wakeup()) {
        uint32_t flags = pmu_get_wakeup_flags();

        printf("从 STANDBY 唤醒！唤醒源：");
        if (flags & PMU_WAKEUP_SOURCE_PIN) {
            uint32_t pin_flags = pmu_get_wakeup_pin_flags();
            printf("WAKEUP 引脚 0x%02lX ", pin_flags);
        }
        if (flags & PMU_WAKEUP_SOURCE_RTC) {
            printf("RTC ");
        }
        if (flags & PMU_WAKEUP_SOURCE_IWDT) {
            printf("IWDT ");
        }
        if (flags & PMU_WAKEUP_SOURCE_RSTN) {
            printf("RSTN ");
        }
        printf("\n");

        pmu_clear_all_wakeup_flags();
        pmu_clear_standby_flag();
    } else {
        // 首次上电：配置 STANDBY 参数
        pmu_get_default_standby_config(&standby_cfg);

        // 使能 WKUP1(PA0) 高电平唤醒，使能滤波
        standby_cfg.wakeup_pins = PMU_WAKEUP1;
        standby_cfg.wakeup_polarity = 0;
        standby_cfg.wakeup_filter = PMU_WAKEUP1;
        standby_cfg.bkp_sram_retain = true;  // 备份 SRAM 保持
        standby_cfg.wakeup_delay_rtc = 10;

        // 配置 WAKEUP 引脚极性
        pmu_wakeup_pin_configure(PMU_WAKEUP1, true, true);

        // 使能 WAKEUP 引脚
        pmu_wakeup_pin_enable(PMU_WAKEUP1);

        // 清除所有唤醒标志
        pmu_clear_all_wakeup_flags();

        printf("即将进入 STANDBY...\n");
        printf("唤醒方式：PA0(WKUP1) 高电平\n\n");
        delay_ms(500);

        // 进入 STANDBY（唤醒后程序从头执行）
        pmu_enter_standby(&standby_cfg);
    }

    printf("完成！\n");
    print_separator();
}

/**
 * @brief 场景4：BOR欠压保护应用
 */
void scenario_4_bor_protection(void)
{
    print_separator();
    printf("=== 场景4：BOR欠压保护应用 ===\n\n");

    // 1. 配置 BOR：阈值 2.5V，使能复位
    pmu_bor_enable(PMU_BOR_LEVEL_2V5, true);

    // 2. 读取 BOR 比较器状态
    if (pmu_bor_get_status()) {
        printf("BOR 状态：电压高于 2.5V，供电正常\n");
    } else {
        printf("BOR 状态：电压低于 2.5V，请注意！\n");
    }

    printf("BOR 配置完成：阈值=2.5V, 复位=使能\n");
    printf("完成！\n");
    print_separator();
}

/**
 * @brief 场景5：LVD低压检测+中断应用
 */
void scenario_5_lvd_interrupt(void)
{
    print_separator();
    printf("=== 场景5：LVD低压检测+中断应用 ===\n\n");

    // 1. 配置 LVD：阈值 2.7V，使能滤波（64 周期）
    pmu_lvd_config_t lvd_cfg = {
        .enabled = true,
        .level = PMU_LVD_LEVEL_2V7,
        .filter_enabled = true,
        .filter_time = PMU_LVD_FILTER_64_CYCLE,
    };
    pmu_lvd_configure(&lvd_cfg);

    // 2. 配置 EXTI LVD 内部线中断
    exti_internal_init(EXTI_LINE_LVD, EXTI_TRIGGER_BOTH);
    NVIC_EnableIRQ(LVD_IRQn);

    // 3. 读取当前 LVD 状态
    if (pmu_lvd_get_filtered_status()) {
        printf("LVD 状态：电压低于 2.7V，已触发报警！\n");
    } else {
        printf("LVD 状态：电压正常（>2.8V）\n");
    }
    printf("LVD 配置完成：阈值=2.7V, 滤波=64周期, 中断=使能\n");

    printf("完成！\n");
    print_separator();
}

/**
 * @brief 场景6：唤醒源判断应用
 */
void scenario_6_wakeup_source(void)
{
    pmu_standby_config_t standby_cfg;

    print_separator();
    printf("=== 场景6：唤醒源判断应用 ===\n\n");

    if (pmu_is_standby_wakeup()) {
        uint32_t flags = pmu_get_wakeup_flags();
        uint32_t pin_flags = pmu_get_wakeup_pin_flags();

        printf("检测到 STANDBY 唤醒！\n");

        // 判断具体唤醒源
        if (flags & PMU_WAKEUP_SOURCE_PIN) {
            printf("  -> WAKEUP 引脚唤醒：");
            if (pin_flags & PMU_WAKEUP1) printf("WKUP1(PA0) ");
            if (pin_flags & PMU_WAKEUP2) printf("WKUP2(PA2) ");
            if (pin_flags & PMU_WAKEUP3) printf("WKUP3(PE6) ");
            if (pin_flags & PMU_WAKEUP4) printf("WKUP4(PC13) ");
            if (pin_flags & PMU_WAKEUP5) printf("WKUP5(PB5) ");
            printf("\n");
        }
        if (flags & PMU_WAKEUP_SOURCE_RTC) {
            printf("  -> RTC 唤醒\n");
        }
        if (flags & PMU_WAKEUP_SOURCE_IWDT) {
            printf("  -> IWDT 唤醒\n");
        }
        if (flags & PMU_WAKEUP_SOURCE_RSTN) {
            printf("  -> RSTN 唤醒\n");
        }
        if (flags & PMU_WAKEUP_SOURCE_BOR) {
            printf("  -> BOR 唤醒\n");
        }

        pmu_clear_all_wakeup_flags();
        pmu_clear_standby_flag();
    } else {
        // 首次运行：配置多个唤醒源
        pmu_get_default_standby_config(&standby_cfg);

        // 使能 WKUP1 + WKUP2
        standby_cfg.wakeup_pins = PMU_WAKEUP1 | PMU_WAKEUP2;
        standby_cfg.wakeup_delay_rtc = 10;

        pmu_wakeup_pin_configure(PMU_WAKEUP1 | PMU_WAKEUP2, true, true);
        pmu_wakeup_pin_enable(PMU_WAKEUP1 | PMU_WAKEUP2);

        pmu_clear_all_wakeup_flags();

        printf("已配置 WKUP1(PA0) + WKUP2(PA2) 高电平唤醒\n");
        printf("即将进入 STANDBY...\n");
        delay_ms(500);

        pmu_enter_standby(&standby_cfg);
    }

    printf("完成！\n");
    print_separator();
}

//===========================================
// LVD中断服务函数（示例）
//===========================================

/**
 * @brief LVD中断服务函数
 */
void LVD_IRQHandler(void)
{
    if (pmu_lvd_get_filtered_status()) {
        printf("[LVD中断] 电压低于阈值！执行紧急操作...\n");
    } else {
        printf("[LVD中断] 电压已恢复正常\n");
    }

    // 清除 EXTI 中断标志
    exti_clear_pending(EXTI_LINE_LVD);
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("PMU电源管理单元示例程序\n");
    printf("SDK版本: 1.0\n");
    print_separator();
    
    // 运行选定的场景
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_sleep_basic();
            break;
        case 2:
            scenario_2_stop_lowpower();
            break;
        case 3:
            scenario_3_standby_ultralow();
            break;
        case 4:
            scenario_4_bor_protection();
            break;
        case 5:
            scenario_5_lvd_interrupt();
            break;
        case 6:
            scenario_6_wakeup_source();
            break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            break;
    }
    
    // 主循环
    printf("\n进入主循环...\n");
    while (1) {
        // 根据不同场景执行不同逻辑
        delay_ms(1000);
    }
}
