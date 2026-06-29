/**
 * @file iwdt_example.c
 * @brief IWDT模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基础复位模式喂狗
 * 2. 超时复位演示
 * 3. 窗口看门狗模式
 * 4. STOP模式唤醒中断
 * 5. 结构体配置方式
 * 6. 运行时动态调整参数
 * 7. 超时时间计算
 * 8. STANDBY模式唤醒恢复
 *
 * @note 需确保 RCC_RCR.IWDTRSTEN = 1（使能 IWDT 复位信号）
 * @note 需确保 RC32K 时钟已使能（RCC_STDBYCTRL.RCLEN = 1）
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

//===========================================
// 全局变量
//===========================================

static volatile uint32_t iwdt_irq_count = 0;
static volatile bool iwdt_wakeup_flag = false;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief 检查复位源
 */
static void check_reset_source(void)
{
    uint32_t src = clock_get_reset_source();

    printf("复位源检查:\n");
    if (src & RESET_SRC_IWDT) {
        printf("  - IWDT 复位（独立看门狗超时）\n");
    }
    if (src & RESET_SRC_POR) {
        printf("  - POR 上电/欠压复位\n");
    }
    if (src & RESET_SRC_NRST) {
        printf("  - NRST 外部复位\n");
    }
    if (src & RESET_SRC_SOFTWARE) {
        printf("  - 软件复位\n");
    }
    if (src & RESET_SRC_LVD) {
        printf("  - LVD 低电压复位\n");
    }
    if (src == 0U) {
        printf("  - 无特殊复位源\n");
    }

    // 清除复位标志
    clock_clear_reset_flags();
}

/**
 * @brief IWDT 示例初始化：使能 RC32K 时钟和 IWDT 复位信号
 */
static void iwdt_example_init(void)
{
    // 使能 RC32K 时钟
    RCC->STDBYCTRL |= (1U << 8);  // RCLEN = 1

    // 等待 RC32K 就绪
    while ((RCC->STDBYCTRL & (1U << 9)) == 0U) {
        __NOP();
    }

    // 使能 IWDT 复位信号
    RCC->RCR |= (1U << 2);  // IWDTRSTEN = 1
}

/**
 * @brief 打印示例标题
 */
static void print_demo_header(const char *title)
{
    print_separator();
    printf("=== %s ===\n\n", title);
}

/**
 * @brief 打印示例结束
 */
static void print_demo_end(void)
{
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// IWDT 唤醒中断回调
//===========================================

static void iwdt_wakeup_callback(void)
{
    iwdt_irq_count++;
    iwdt_wakeup_flag = true;
    iwdt_feed();  // 喂狗清中断
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基础复位模式喂狗
 *
 * 演示 IWDT 复位模式的基本使用：初始化后定期喂狗，防止复位。
 */
void scenario_1_basic_reset(void)
{
    print_demo_header("场景1：基础复位模式喂狗");

    printf("步骤1：初始化 IWDT 复位模式（32分频, reload=2000, 约2秒超时）\n");
    iwdt_init_reset(IWDT_PRESCALER_32, 2000);
    printf("  结果: IWDT 已启动，超时时间约 %ums\n",
           iwdt_calculate_timeout_ms(IWDT_PRESCALER_32, 2000));

    printf("\n步骤2：定期喂狗 10 次（每次间隔 1 秒）\n");
    for (int i = 1; i <= 10; i++) {
        delay_ms(1000);
        iwdt_feed();
        printf("  第 %d 次喂狗，状态寄存器: 0x%08lX\n", i, iwdt_get_status());
    }
    printf("  结果: 10 次喂狗完成，系统未复位\n");

    printf("\n步骤3：停止喂狗，等待复位\n");
    printf("  提示: 约 2 秒后将发生 IWDT 复位...\n");

    print_demo_end();
}

/**
 * @brief 场景2：超时复位演示
 *
 * 演示 IWDT 不喂狗导致系统复位的过程。
 */
void scenario_2_timeout_reset(void)
{
    print_demo_header("场景2：超时复位演示");

    printf("步骤1：初始化 IWDT 短超时（4分频, reload=500, 约62.5ms超时）\n");
    iwdt_init_reset(IWDT_PRESCALER_4, 500);
    uint32_t timeout = iwdt_calculate_timeout_ms(IWDT_PRESCALER_4, 500);
    printf("  结果: IWDT 已启动，超时时间约 %ums\n", timeout);

    printf("\n步骤2：喂狗 3 次后停止\n");
    for (int i = 1; i <= 3; i++) {
        delay_ms(30);
        iwdt_feed();
        printf("  第 %d 次喂狗\n", i);
    }
    printf("  结果: 停止喂狗\n");

    printf("\n步骤3：等待超时复位\n");
    printf("  提示: 约 %ums 后系统将复位...\n", timeout);
    delay_ms(500);

    print_demo_end();
}

/**
 * @brief 场景3：窗口看门狗模式
 *
 * 演示窗口看门狗：在窗口外喂狗会触发复位。
 */
void scenario_3_window_mode(void)
{
    print_demo_header("场景3：窗口看门狗模式");

    printf("步骤1：初始化窗口模式（32分频, reload=4000, window=2000）\n");
    printf("  窗口区域: 计数器 0~2000 可安全喂狗\n");
    printf("  非窗口区域: 计数器 2001~4000 喂狗触发复位\n");
    iwdt_init_window(IWDT_PRESCALER_32, 4000, 2000);
    printf("  结果: 窗口模式 IWDT 已启动\n");
    printf("  当前 reload=%u, window=%u\n",
           iwdt_get_reload(), iwdt_get_window());

    printf("\n步骤2：延迟 1 秒使计数器进入窗口区域\n");
    delay_ms(1000);
    printf("  状态寄存器: 0x%08lX\n", iwdt_get_status());

    printf("\n步骤3：在窗口内喂狗（安全）\n");
    iwdt_feed();
    printf("  结果: 喂狗成功，无复位\n");

    printf("\n步骤4：立即再次喂狗（计数器刚重载，在窗口外）\n");
    iwdt_feed();
    printf("  结果: 窗口外喂狗，正常情况下系统将立即复位\n");
    printf("  提示: 若系统未复位，表明窗口机制未使能（window >= reload）\n");

    print_demo_end();
}

/**
 * @brief 场景4：STOP模式唤醒中断
 *
 * 演示 IWDT 唤醒功能：将 MCU 从 STOP 模式唤醒。
 */
void scenario_4_wakeup_interrupt(void)
{
    print_demo_header("场景4：STOP模式唤醒中断");

    printf("步骤1：初始化 IWDT 唤醒模式（32分频, reload=4000, wakeup=1000）\n");
    iwdt_init_wakeup(IWDT_PRESCALER_32, 4000, 1000);
    printf("  结果: 唤醒模式已启动\n");
    printf("  唤醒时间: 约 %ums\n",
           iwdt_calculate_wakeup_ms(IWDT_PRESCALER_32, 4000, 1000));

    printf("\n步骤2：注册唤醒中断回调\n");
    iwdt_register_callback(iwdt_wakeup_callback);
    iwdt_irq_count = 0;
    iwdt_wakeup_flag = false;
    printf("  结果: 回调已注册\n");

    printf("\n步骤3：配置 EXTI 唤醒线（line 18）\n");
    exti_internal_init(EXTI_LINE_IWDT, EXTI_TRIGGER_RISING);
    NVIC_EnableIRQ(IWDT_WKUP_IRQn);
    printf("  结果: EXTI 和 NVIC 已配置\n");

    printf("\n步骤4：进入 STOP 模式，等待 IWDT 唤醒...\n");
    exti_clear_pending(EXTI_LINE_IWDT);
    delay_ms(100);
    // 进入 STOP 模式前等待喂狗完成
    while (iwdt_is_flag_set(IWDT_FLAG_RLF)) {
        __NOP();
    }
    pmu_enter_stop_simple();
    // 从 STOP 唤醒后继续
    printf("  结果: 已从 STOP 模式唤醒\n");
    printf("  中断次数: %u\n", (unsigned int)iwdt_irq_count);

    printf("\n步骤5：再等待 3 次唤醒\n");
    for (int i = 1; i <= 3; i++) {
        iwdt_wakeup_flag = false;
        // 等待唤醒标志（带超时保护）
        uint32_t timeout = 10000000U;
        while (!iwdt_wakeup_flag) {
            if (--timeout == 0U) {
                printf("  第 %d 次唤醒超时，中断未到来\n", i);
                break;
            }
            __NOP();
        }
        if (iwdt_wakeup_flag) {
            printf("  第 %d 次唤醒，中断次数: %u\n",
                   i, (unsigned int)iwdt_irq_count);
        }
    }

    printf("\n步骤6：清理\n");
    NVIC_DisableIRQ(IWDT_WKUP_IRQn);
    iwdt_register_callback(NULL);
    iwdt_wakeup_disable();
    printf("  结果: 已清理\n");

    print_demo_end();
}

/**
 * @brief 场景5：结构体配置方式
 *
 * 演示使用 iwdt_config_t 结构体进行完整配置。
 */
void scenario_5_config_struct(void)
{
    print_demo_header("场景5：结构体配置方式");

    printf("步骤1：使用结构体配置 IWDT（复位 + 窗口模式）\n");
    iwdt_config_t cfg = {
        .prescaler = IWDT_PRESCALER_64,
        .reload = 3000,
        .window = 1500,
        .wakeup = 0xFFF,
        .wakeup_enable = false,
    };
    if (iwdt_config(&cfg)) {
        printf("  结果: 配置成功\n");
        printf("  prescaler=%d, reload=%u, window=%u\n",
               cfg.prescaler, cfg.reload, cfg.window);
        printf("  timeout=%ums\n",
               iwdt_calculate_timeout_ms(IWDT_PRESCALER_64, 3000));
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤2：查询当前配置\n");
    printf("  prescaler=%d, reload=%u, window=%u, wakeup=%u\n",
           iwdt_get_prescaler(), iwdt_get_reload(),
           iwdt_get_window(), iwdt_get_wakeup());

    printf("\n步骤3：使用结构体配置 IWDT（唤醒模式）\n");
    iwdt_config_t cfg2 = {
        .prescaler = IWDT_PRESCALER_32,
        .reload = 4000,
        .window = 0xFFF,
        .wakeup = 1000,
        .wakeup_enable = true,
    };
    if (iwdt_config(&cfg2)) {
        printf("  结果: 唤醒模式配置成功\n");
        printf("  wakeup=%u, 唤醒时间约 %ums\n",
               cfg2.wakeup,
               iwdt_calculate_wakeup_ms(IWDT_PRESCALER_32, 4000, 1000));
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤4：验证参数校验\n");
    iwdt_config_t invalid_cfg = {
        .prescaler = IWDT_PRESCALER_64,
        .reload = 0xFFFF,   // 超出范围（> 0xFFF）
        .window = 0xFFF,
        .wakeup = 0xFFF,
        .wakeup_enable = false,
    };
    if (!iwdt_config(&invalid_cfg)) {
        printf("  结果: 无效参数被正确拒绝\n");
    }

    print_demo_end();
}

/**
 * @brief 场景6：运行时动态调整参数
 *
 * 演示在运行时修改 IWDT 的预分频、重装载值、窗口值等参数。
 */
void scenario_6_dynamic_adjust(void)
{
    print_demo_header("场景6：运行时动态调整参数");

    printf("步骤1：初始配置（32分频, reload=2000）\n");
    iwdt_init_reset(IWDT_PRESCALER_32, 2000);
    printf("  超时时间: %ums\n",
           iwdt_calculate_timeout_ms(IWDT_PRESCALER_32, 2000));

    printf("\n步骤2：修改预分频为 64 分频\n");
    iwdt_set_prescaler(IWDT_PRESCALER_64);
    printf("  当前 prescaler=%d\n", iwdt_get_prescaler());
    printf("  超时时间变化: %ums\n",
           iwdt_calculate_timeout_ms(IWDT_PRESCALER_64, 2000));

    printf("\n步骤3：修改重装载值为 1000\n");
    iwdt_set_reload(1000);
    printf("  当前 reload=%u\n", iwdt_get_reload());
    printf("  超时时间变化: %ums\n",
           iwdt_calculate_timeout_ms(IWDT_PRESCALER_64, 1000));

    printf("\n步骤4：喂狗后修改窗口值\n");
    iwdt_feed();
    delay_ms(100);
    iwdt_set_window(500);
    printf("  当前 window=%u\n", iwdt_get_window());
    printf("  窗口模式: %s\n",
           iwdt_get_window() < iwdt_get_reload() ? "已使能" : "未使能");

    printf("\n步骤5：动态设置为唤醒模式\n");
    iwdt_set_wakeup(300);
    iwdt_wakeup_enable();
    printf("  当前 wakeup=%u\n", iwdt_get_wakeup());
    printf("  唤醒时间: %ums\n",
           iwdt_calculate_wakeup_ms(IWDT_PRESCALER_64, 1000, 300));

    printf("\n步骤6：恢复为复位模式\n");
    iwdt_wakeup_disable();
    iwdt_set_window(0xFFF);
    iwdt_set_wakeup(0xFFF);
    iwdt_set_reload(2000);
    iwdt_set_prescaler(IWDT_PRESCALER_32);
    iwdt_feed();
    printf("  当前 prescaler=%d, reload=%u, window=%u, wakeup=%u\n",
           iwdt_get_prescaler(), iwdt_get_reload(),
           iwdt_get_window(), iwdt_get_wakeup());

    print_demo_end();
}

/**
 * @brief 场景7：超时时间计算
 *
 * 演示使用辅助计算函数计算超时时间和重装载值。
 */
void scenario_7_timeout_calculation(void)
{
    print_demo_header("场景7：超时时间计算");

    printf("步骤1：计算各种预分频下的超时范围\n");
    printf("  RC32K 标称频率: 32kHz\n");
    printf("  ┌──────────┬──────────┬──────────┐\n");
    printf("  │ 预分频   │ 最短(ms) │ 最长(ms) │\n");
    printf("  ├──────────┼──────────┼──────────┤\n");

    iwdt_prescaler_t prescalers[] = {
        IWDT_PRESCALER_4, IWDT_PRESCALER_8, IWDT_PRESCALER_16,
        IWDT_PRESCALER_32, IWDT_PRESCALER_64, IWDT_PRESCALER_128,
        IWDT_PRESCALER_256
    };
    const char *names[] = {"4", "8", "16", "32", "64", "128", "256"};

    for (int i = 0; i < 7; i++) {
        uint32_t t_min = iwdt_calculate_timeout_ms(prescalers[i], 0);
        uint32_t t_max = iwdt_calculate_timeout_ms(prescalers[i], 0xFFF);
        printf("  │ %-8s │ %-8u │ %-8u │\n", names[i], t_min, t_max);
    }
    printf("  └──────────┴──────────┴──────────┘\n");

    printf("\n步骤2：根据目标超时时间反算重装载值\n");
    printf("  目标: 2000ms @ 32分频\n");
    uint16_t r1 = iwdt_calculate_reload(IWDT_PRESCALER_32, 2000);
    printf("  计算 reload=%u, 实际超时=%ums\n",
           r1, iwdt_calculate_timeout_ms(IWDT_PRESCALER_32, r1));

    printf("\n  目标: 5000ms @ 64分频\n");
    uint16_t r2 = iwdt_calculate_reload(IWDT_PRESCALER_64, 5000);
    printf("  计算 reload=%u, 实际超时=%ums\n",
           r2, iwdt_calculate_timeout_ms(IWDT_PRESCALER_64, r2));

    printf("\n  目标: 10000ms @ 256分频\n");
    uint16_t r3 = iwdt_calculate_reload(IWDT_PRESCALER_256, 10000);
    printf("  计算 reload=%u, 实际超时=%ums\n",
           r3, iwdt_calculate_timeout_ms(IWDT_PRESCALER_256, r3));

    printf("\n步骤3：计算唤醒时间\n");
    printf("  reload=4000, wakeup=500 @ 32分频\n");
    uint32_t tw = iwdt_calculate_wakeup_ms(IWDT_PRESCALER_32, 4000, 500);
    printf("  唤醒时间: %ums（从启动到唤醒）\n", tw);

    printf("  reload=4000, wakeup=3000 @ 64分频\n");
    tw = iwdt_calculate_wakeup_ms(IWDT_PRESCALER_64, 4000, 3000);
    printf("  唤醒时间: %ums（从启动到唤醒）\n", tw);

    print_demo_end();
}

/**
 * @brief 场景8：STANDBY模式唤醒恢复
 *
 * 演示 IWDT 超时复位将系统从 STANDBY 模式唤醒。
 */
void scenario_8_standby_wakeup(void)
{
    print_demo_header("场景8：STANDBY模式唤醒恢复");

    printf("步骤1：检查是否由 IWDT 从 STANDBY 唤醒\n");
    uint32_t wakeup_flags = pmu_get_wakeup_flags();
    if (wakeup_flags & PMU_WAKEUP_SOURCE_IWDT) {
        printf("  结果: 检测到 IWDT 唤醒标志\n");
        printf("  说明: 上次 STANDBY 被 IWDT 超时复位唤醒\n");
        pmu_clear_iwdt_wakeup_flag();
        printf("  已清除 IWDT 唤醒标志\n");
    } else {
        printf("  结果: 无 IWDT 唤醒标志（首次运行）\n");
    }

    printf("\n步骤2：配置 IWDT 复位模式（128分频, reload=2000, 约8秒超时）\n");
    iwdt_init_reset(IWDT_PRESCALER_128, 2000);
    uint32_t timeout = iwdt_calculate_timeout_ms(IWDT_PRESCALER_128, 2000);
    printf("  结果: IWDT 已启动，超时时间约 %ums\n", timeout);

    printf("\n步骤3：喂狗 5 次后停止\n");
    for (int i = 1; i <= 5; i++) {
        delay_ms(1000);
        iwdt_feed();
        printf("  第 %d 次喂狗\n", i);
    }
    printf("  结果: 停止喂狗，%ums 后 IWDT 将超时复位\n", timeout);

    printf("\n步骤4：超时后系统复位，若进入 STANDBY 模式");
    printf(" 则 IWDT 复位会将系统唤醒\n");
    printf("  提示: 复位后重新运行本场景，步骤1 将检测到唤醒源\n");

    print_demo_end();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    // 系统初始化
    sys_status_t status = system_init();
    if (status != SYS_OK) {
        // 系统初始化失败，进入死循环
        while (1) { __NOP(); }
    }
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("IWDT 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 检查复位源
    check_reset_source();

    // 初始化 IWDT 所需的时钟和复位信号
    iwdt_example_init();

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_basic_reset();
            break;
        case 2:
            scenario_2_timeout_reset();
            break;
        case 3:
            scenario_3_window_mode();
            break;
        case 4:
            scenario_4_wakeup_interrupt();
            break;
        case 5:
            scenario_5_config_struct();
            break;
        case 6:
            scenario_6_dynamic_adjust();
            break;
        case 7:
            scenario_7_timeout_calculation();
            break;
        case 8:
            scenario_8_standby_wakeup();
            break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            printf("有效值: 1-8\n");
            break;
    }

    // 主循环
    while (1) {
        __WFI();
    }
}

//===========================================
// 中断服务函数
//===========================================
// IWDT_WKUP_IRQHandler 已在 iwdt.c 中实现
// 若需自定义中断处理，可在此重新定义
