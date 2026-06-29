/**
 * @file wdt_example.c
 * @brief WDT模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 复位模式基础演示（周期性喂狗，正常运行）
 * 2. 复位模式超时复位（停止喂狗，触发系统复位）
 * 3. 中断模式基础演示（中断中喂狗，系统不复位）
 * 4. 中断模式超时复位（中断产生后不喂狗，触发复位）
 * 5. 结构体配置（wdt_config 完整配置）
 * 6. 动态参数调整（运行时修改分频和加载值）
 * 7. 精确超时计算（使用辅助函数计算超时时间）
 * 8. 中断回调模式（注册回调函数处理WDT中断）
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 修改此值选择运行的场景（1-8）
#define DEFAULT_SCENARIO 1

// 场景1、2的WDT参数（复位模式）
// 计算公式：T = (LOAD + 1) × 2^DIVISOR / PCLK
// PCLK=110MHz, 64分频时：LOAD=0x001A36DD → ~1s 超时
#define SCENARIO_LOAD_RESET    0x001A36DDU
#define SCENARIO_DIVISOR_RESET WDT_DIVISOR_64

// 场景3、4的WDT参数（中断模式）
#define SCENARIO_LOAD_INT      0x007A1200U  // 中断触发时间约1秒
#define SCENARIO_DIVISOR_INT   WDT_DIVISOR_64
#define SCENARIO_INTCLRTIME    0x0000FFFFU  // 中断清除时限（最大）

//===========================================
// 全局变量
//===========================================

static volatile uint32_t wdt_irq_count = 0;    // WDT中断计数
static volatile bool     wdt_irq_flag = false;  // WDT中断标志

//===========================================
// 辅助函数
//===========================================

/**
 * @brief 打印分隔线
 */
void print_separator(void)
{
    printf("==========================================\n");
}

/**
 * @brief 检查并打印上次复位源
 */
void check_reset_source(void)
{
    uint32_t reset_src = clock_get_reset_source();

    printf("复位源检查：\n");
    if (reset_src & RESET_SRC_WDT) {
        printf("  检测到 WDT 看门狗复位！\n");
    }
    if (reset_src & RESET_SRC_IWDT) {
        printf("  检测到 IWDT 独立看门狗复位！\n");
    }
    if (reset_src & RESET_SRC_POR) {
        printf("  上电复位（首次上电或欠压）\n");
    }
    if (reset_src & RESET_SRC_NRST) {
        printf("  外部 NRST 引脚复位\n");
    }
    if (reset_src & RESET_SRC_SOFTWARE) {
        printf("  软件复位\n");
    }
    if (reset_src & RESET_SRC_LVD) {
        printf("  低压检测复位\n");
    }
    if (reset_src & RESET_SRC_LOCKUP) {
        printf("  CPU LOCKUP 复位\n");
    }
    if (reset_src == 0U) {
        printf("  未知复位源\n");
    }

    // 清除复位源标志，避免影响下次判断
    clock_reset_source_enable(RESET_SRC_WDT, false);
    clock_reset_source_enable(RESET_SRC_IWDT, false);
    clock_reset_source_enable(RESET_SRC_LVD, false);
    clock_reset_source_enable(RESET_SRC_LOCKUP, false);
    printf("\n");
}

/**
 * @brief 初始化 WDT 外设时钟和复位信号
 */
void wdt_example_init_clock(void)
{
    // 使能 WDT 外设时钟
    clock_periph_enable(CLK_WDT);

    // 使能 WDT 复位信号输出到系统复位控制器
    // 注：只有置位 RCC_RCR.WDTRSTEN，WDT 产生的复位信号才能复位系统
    clock_reset_source_enable(RESET_SRC_WDT, true);

    printf("WDT 时钟已使能，复位信号已允许\n");
}

/**
 * @brief 演示开始前的准备信息
 */
void print_demo_header(uint8_t scenario, const char *title)
{
    printf("\n");
    print_separator();
    printf("=== 场景%d：%s ===\n\n", scenario, title);
}

/**
 * @brief 演示结束信息
 */
void print_demo_end(void)
{
    printf("\n演示完成！\n");
    print_separator();
    printf("\n");
}

//===========================================
// 场景1：复位模式基础演示
//===========================================

/**
 * @brief 场景1：复位模式基础演示（周期性喂狗，正常运行）
 *
 * 演示流程：
 * 1. 启动 WDT 复位模式（约1秒超时）
 * 2. 周期性喂狗（每500ms一次）
 * 3. 运行10次喂狗循环，系统不会复位
 *
 * 期望结果：系统正常运行，不产生复位，计数器持续刷新
 */
void scenario_1_reset_mode_normal(void)
{
    print_demo_header(1, "复位模式基础演示（周期性喂狗）");

    printf("步骤1：使能 WDT 时钟并允许复位信号\n");
    wdt_example_init_clock();

    printf("\n步骤2：启动 WDT 复位模式\n");
    printf("  加载值：0x%08X\n", (unsigned int)SCENARIO_LOAD_RESET);
    printf("  预分频：64分频\n");
    printf("  超时时间：约 1 秒\n");
    wdt_init_reset(SCENARIO_LOAD_RESET, SCENARIO_DIVISOR_RESET);
    printf("  结果: WDT 复位模式已启动\n");

    printf("\n步骤3：周期性喂狗（每500ms一次，共10次）\n");
    for (int i = 0; i < 10; i++) {
        wdt_feed();
        uint32_t counter = wdt_get_counter();
        printf("  [%d] 喂狗完成，当前计数值：0x%08X\n", i + 1, (unsigned int)counter);
        delay_ms(500);
    }

    printf("\n步骤4：禁用 WDT\n");
    wdt_disable();
    uint32_t final_counter = wdt_get_counter();
    printf("  结果: WDT 已禁用，停止计数\n");
    printf("  最终计数值：0x%08X\n", (unsigned int)final_counter);

    print_demo_end();
}

//===========================================
// 场景2：复位模式超时复位
//===========================================

/**
 * @brief 场景2：复位模式超时复位（停止喂狗，触发系统复位）
 *
 * 演示流程：
 * 1. 先检查上次复位原因
 * 2. 启动 WDT 复位模式（约2秒超时）
 * 3. 打印2次计数值后停止喂狗
 * 4. WDT 超时产生系统复位
 *
 * 期望结果：系统被 WDT 复位，重新运行时可检测到 WDT 复位源
 *
 * @warning 此场景会导致系统复位，复位后程序会重新运行
 * @warning 请确保 RCC_RCR.WDTRSTEN 已置位
 */
void scenario_2_reset_mode_timeout(void)
{
    print_demo_header(2, "复位模式超时复位（演示系统复位）");

    // 检查上次复位原因
    check_reset_source();

    printf("步骤1：使能 WDT 时钟并允许复位信号\n");
    wdt_example_init_clock();

    printf("\n步骤2：启动 WDT 复位模式\n");
    uint32_t load_val = SCENARIO_LOAD_RESET * 2;  // 约2秒超时
    printf("  加载值：0x%08X\n", (unsigned int)load_val);
    printf("  预分频：64分频\n");
    printf("  超时时间：约 2 秒\n");
    wdt_init_reset(load_val, SCENARIO_DIVISOR_RESET);
    printf("  结果: WDT 复位模式已启动\n");

    printf("\n步骤3：喂狗2次后停止（观察超时复位）\n");
    for (int i = 0; i < 2; i++) {
        wdt_feed();
        uint32_t counter = wdt_get_counter();
        printf("  [%d] 喂狗完成，当前计数值：0x%08X\n", i + 1, (unsigned int)counter);
        delay_ms(1000);
    }

    printf("\n步骤4：停止喂狗，等待WDT超时复位...\n");
    printf("  WDT 将在约 2 秒后产生复位信号\n");
    printf("  复位后请检查打印信息中的「复位源检查」\n");
    printf("  如果看到「检测到 WDT 看门狗复位」，说明复位成功\n\n");

    // 等待复位
    while (1) {
        uint32_t counter = wdt_get_counter();
        printf("  当前计数值：0x%08X（等待复位...）\n", (unsigned int)counter);
        delay_ms(500);
    }
}

//===========================================
// 场景3：中断模式基础演示
//===========================================

/**
 * @brief 场景3的回调函数：在中断中喂狗
 */
static void scenario_3_callback(void)
{
    wdt_irq_count++;
    wdt_irq_flag = true;
    wdt_feed();  // 必须在回调中喂狗，否则会触发复位
}

/**
 * @brief 场景3：中断模式基础演示（中断中喂狗，系统不复位）
 *
 * 演示流程：
 * 1. 注册回调函数（回调中自动喂狗）
 * 2. 启动 WDT 中断模式（约1秒触发中断）
 * 3. 回调中喂狗，系统不会复位
 *
 * 期望结果：WDT 周期性产生中断，在回调中喂狗，计数器重置，系统不产生复位
 */
void scenario_3_interrupt_mode_normal(void)
{
    print_demo_header(3, "中断模式基础演示（中断中喂狗）");

    printf("步骤1：使能 WDT 时钟\n");
    clock_periph_enable(CLK_WDT);

    printf("\n步骤2：注册中断回调函数\n");
    wdt_register_callback(scenario_3_callback);
    printf("  回调函数已注册（回调中自动喂狗）\n");

    printf("\n步骤3：配置 WDT 中断\n");
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0x03);
    NVIC_EnableIRQ(WDT_IRQn);
    printf("  结果: NVIC 中断已配置，优先级 3\n");

    printf("\n步骤4：启动 WDT 中断模式\n");
    printf("  加载值：0x%08X\n", (unsigned int)SCENARIO_LOAD_INT);
    printf("  预分频：64分频\n");
    printf("  中断清除时限：0x%04X\n", (unsigned int)SCENARIO_INTCLRTIME);
    wdt_init_interrupt(SCENARIO_LOAD_INT, SCENARIO_DIVISOR_INT, SCENARIO_INTCLRTIME);
    printf("  结果: WDT 中断模式已启动\n");

    printf("\n步骤5：等待 WDT 中断触发（观察中断计数）\n");
    uint32_t last_count = wdt_irq_count;
    for (int i = 0; i < 20; i++) {
        delay_ms(500);
        if (wdt_irq_count != last_count) {
            printf("  WDT 回调触发！中断计数：%lu\n", (unsigned long)wdt_irq_count);
            printf("    回调中已完成喂狗，计数器已重置\n");
            last_count = wdt_irq_count;
        }
        uint32_t counter = wdt_get_counter();
        printf("    当前计数值：0x%08X\n", (unsigned int)counter);
    }

    printf("\n步骤6：清理\n");
    NVIC_DisableIRQ(WDT_IRQn);
    wdt_disable();
    wdt_register_callback(NULL);
    printf("  结果: WDT 已禁用，回调已取消\n");
    printf("  总中断次数：%lu\n", (unsigned long)wdt_irq_count);

    print_demo_end();
}

//===========================================
// 场景4：中断模式超时复位
//===========================================

/**
 * @brief 场景4：中断模式超时复位（不注册回调，ISR不做任何操作导致超时复位）
 *
 * 演示流程：
 * 1. 先检查上次复位原因
 * 2. 启动 WDT 中断模式但不注册回调
 * 3. 计数器减到0后产生原始中断 RIS
 * 4. ISR 被调用但无回调，未喂狗，中断清除时限超时后复位
 *
 * 期望结果：中断产生后，经过中断清除时限后系统复位
 *
 * @warning 此场景会导致系统复位
 */
void scenario_4_interrupt_mode_timeout(void)
{
    print_demo_header(4, "中断模式超时复位（不喂狗触发复位）");

    // 检查上次复位原因
    check_reset_source();

    printf("步骤1：使能 WDT 时钟并允许复位信号\n");
    wdt_example_init_clock();

    printf("\n步骤2：启动 WDT 中断模式（不注册回调 → ISR不喂狗 → 超时复位）\n");
    // 使用 wdt_config 实现：中断模式使能中断但不注册回调
    wdt_config_t cfg = {
        .load = SCENARIO_LOAD_INT,
        .divisor = SCENARIO_DIVISOR_INT,
        .mode = WDT_MODE_INTERRUPT,
        .int_enable = true,    // 使能中断，WDT_IRQHandler 会被调用
        .intclrtime = 0x1000,  // 较短的清除时限
    };
    wdt_config(&cfg);
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0x03);
    NVIC_EnableIRQ(WDT_IRQn);

    printf("  加载值：0x%08X\n", (unsigned int)cfg.load);
    printf("  预分频：64分频\n");
    printf("  中断使能：true（WDT_IRQHandler 将被调用）\n");
    printf("  回调函数：未注册（ISR 中无操作，不喂狗）\n");
    printf("  中断清除时限：0x1000（较短）\n");
    printf("  结果: WDT 已启动\n");

    printf("\n步骤3：等待中断产生，然后等待超时复位\n");
    printf("  流程：计数器减到0 → 产生原始中断 RIS → ISR被调用但无回调\n");
    printf("       → 未喂狗 → 中断清除时限到期 → 系统复位\n");

    // 等待原始中断或计数器递减
    for (int i = 0; i < 20; i++) {
        uint32_t counter = wdt_get_counter();
        bool ris = wdt_get_ris();
        printf("  [%d] 计数值：0x%08X, RIS：%d\n", i + 1, (unsigned int)counter, (int)ris);
        if (ris) {
            printf("  >>> 原始中断已产生！继续不喂狗将导致复位 <<<\n");
        }
        delay_ms(200);
    }

    printf("\n  等待复位...\n");
    while (1) {
        __NOP();
    }
}

//===========================================
// 场景5：结构体配置（wdt_config）
//===========================================

/**
 * @brief 场景5：使用 wdt_config 结构体完整配置 WDT
 *
 * 演示流程：
 * 1. 使用 wdt_config_t 结构体配置复位模式
 * 2. 查看各寄存器的当前值
 * 3. 修改配置为中断模式
 * 4. 验证配置切换
 *
 * 期望结果：结构体配置方式灵活切换复位/中断模式
 */
void scenario_5_struct_config(void)
{
    print_demo_header(5, "结构体配置（wdt_config 完整配置）");

    printf("步骤1：使能 WDT 时钟\n");
    wdt_example_init_clock();

    printf("\n步骤2：使用结构体配置复位模式\n");
    wdt_config_t cfg1 = {
        .load = 0x00FFFFFF,
        .divisor = WDT_DIVISOR_128,
        .mode = WDT_MODE_RESET,
        .int_enable = false,
        .intclrtime = 0,
    };
    printf("  配置参数：\n");
    printf("    load      : 0x%08X\n", (unsigned int)cfg1.load);
    printf("    divisor   : %d（128分频）\n", (int)cfg1.divisor);
    printf("    mode      : %d（复位模式）\n", (int)cfg1.mode);
    printf("    int_enable: false\n");
    printf("    intclrtime: %u（复位模式不适用）\n", (unsigned int)cfg1.intclrtime);

    if (!wdt_config(&cfg1)) {
        printf("  错误：配置失败！\n");
        return;
    }

    printf("  结果: 配置成功\n");
    printf("  当前加载值：0x%08X\n", (unsigned int)wdt_get_load());
    printf("  当前计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    // 喂一次狗验证
    wdt_feed();
    printf("  喂狗后计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    printf("\n步骤3：禁用 WDT，切换为中断模式\n");
    wdt_disable();
    printf("  WDT 已禁用\n");

    wdt_config_t cfg2 = {
        .load = 0x0000FFFF,
        .divisor = WDT_DIVISOR_16,
        .mode = WDT_MODE_INTERRUPT,
        .int_enable = true,
        .intclrtime = 0x2000,
    };
    printf("  新配置参数：\n");
    printf("    load      : 0x%08X\n", (unsigned int)cfg2.load);
    printf("    divisor   : %d（16分频）\n", (int)cfg2.divisor);
    printf("    mode      : %d（中断模式）\n", (int)cfg2.mode);
    printf("    int_enable: true\n");
    printf("    intclrtime: 0x%04X\n", (unsigned int)cfg2.intclrtime);

    if (!wdt_config(&cfg2)) {
        printf("  错误：配置失败！\n");
        return;
    }

    printf("  结果: 配置成功，已切换为中断模式\n");
    printf("  当前加载值：0x%08X\n", (unsigned int)wdt_get_load());

    printf("\n步骤4：验证配置后禁用 WDT\n");
    wdt_disable();
    printf("  WDT 已禁用\n");

    print_demo_end();
}

//===========================================
// 场景6：动态参数调整
//===========================================

/**
 * @brief 场景6：运行时动态调整 WDT 参数
 *
 * 演示流程：
 * 1. 启动 WDT 后，在运行中修改加载值
 * 2. 切换到不同的预分频
 * 3. 切换到不同的工作模式
 * 4. 全程观察计数器变化
 *
 * 期望结果：每次参数修改后计数器行为相应变化
 */
void scenario_6_dynamic_adjust(void)
{
    print_demo_header(6, "动态参数调整（运行时修改）");

    printf("步骤1：使能 WDT 时钟\n");
    clock_periph_enable(CLK_WDT);

    printf("\n步骤2：启动 WDT（64分频）\n");
    wdt_init_reset(0x00FFFFFF, WDT_DIVISOR_64);
    printf("  当前加载值：0x%08X\n", (unsigned int)wdt_get_load());
    printf("  当前计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    printf("\n步骤3：运行时修改加载值\n");
    delay_ms(100);
    printf("  修改前计数值：0x%08X\n", (unsigned int)wdt_get_counter());
    wdt_set_load(0x0000FFFF);
    wdt_feed();  // 让新加载值生效
    printf("  修改后加载值：0x%08X\n", (unsigned int)wdt_get_load());
    printf("  加载新值后计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    printf("\n步骤4：运行时修改预分频（64分频 → 8分频）\n");
    delay_ms(100);
    printf("  修改前计数值：0x%08X\n", (unsigned int)wdt_get_counter());
    wdt_set_divisor(WDT_DIVISOR_8);
    printf("  预分频已修改为 8分频\n");
    printf("  修改后计数值：0x%08X（计数速度变化）\n", (unsigned int)wdt_get_counter());

    printf("\n步骤5：验证不同分频下的计数速度\n");
    // 8分频下计数较快，观察明显
    wdt_set_load(0x00FFFFFF);
    wdt_feed();
    printf("  8分频初始计数值：0x%08X\n", (unsigned int)wdt_get_counter());
    delay_ms(10);
    printf("  10ms后计数值：0x%08X\n", (unsigned int)wdt_get_counter());
    delay_ms(10);
    printf("  20ms后计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    printf("\n步骤6：切换工作模式（复位 → 中断）\n");
    wdt_disable();
    wdt_set_mode(WDT_MODE_INTERRUPT);
    wdt_set_intclrtime(0x1000);
    wdt_set_interrupt(true);
    printf("  模式已切换为中断模式\n");
    printf("  中断清除时限：0x%04X\n", (unsigned int)0x1000);
    printf("  中断使能：true\n");
    printf("  （此场景不使能 WDT，避免实际触发中断影响演示）\n");

    print_demo_end();
}

//===========================================
// 场景7：精确超时计算
//===========================================

/**
 * @brief 场景7：使用辅助函数计算精确的超时时间
 *
 * 演示流程：
 * 1. 使用 wdt_calculate_load() 根据目标时间反算加载值
 * 2. 使用 wdt_calculate_timeout_us() 验证计算结果
 * 3. 对比不同预分频下的参数差异
 *
 * 期望结果：计算出的加载值能使 WDT 在目标时间内超时
 */
void scenario_7_timeout_calculation(void)
{
    print_demo_header(7, "精确超时计算（辅助函数使用）");

    // 假设 PCLK = 110MHz
    const uint32_t pclk_hz = 110000000U;

    printf("假设 APB 时钟频率：%lu Hz (110MHz)\n\n", (unsigned long)pclk_hz);

    printf("步骤1：计算不同目标超时时间所需的加载值\n");
    printf("  ┌──────────────┬──────────┬──────────────────┐\n");
    printf("  │ 目标超时     │ 预分频   │ 所需加载值       │\n");
    printf("  ├──────────────┼──────────┼──────────────────┤\n");

    // 测试多种组合
    static const struct {
        uint64_t timeout_us;
        wdt_divisor_t divisor;
        const char *div_name;
    } test_cases[] = {
        { 1000000ULL, WDT_DIVISOR_64,  "64分频" },
        { 1000000ULL, WDT_DIVISOR_128, "128分频" },
        { 500000ULL,  WDT_DIVISOR_16,  "16分频" },
        { 2000000ULL, WDT_DIVISOR_64,  "64分频" },
        { 100000ULL,  WDT_DIVISOR_4,   "4分频" },
        { 5000000ULL, WDT_DIVISOR_128, "128分频" },
        { 100000ULL,  WDT_DIVISOR_1,   "不分频" },
        { 10000000ULL, WDT_DIVISOR_128,"128分频" },
    };

    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint32_t load = wdt_calculate_load(pclk_hz, test_cases[i].timeout_us,
                                           test_cases[i].divisor);
        printf("  │ %5lu ms     │ %-8s │ 0x%08X (%10lu) │\n",
               (unsigned long)(test_cases[i].timeout_us / 1000),
               test_cases[i].div_name,
               (unsigned int)load,
               (unsigned long)load);
    }
    printf("  └──────────────┴──────────┴──────────────────┘\n");

    printf("\n步骤2：验证计算结果的准确性（反向计算超时时间）\n");
    for (size_t i = 0; i < sizeof(test_cases) / sizeof(test_cases[0]); i++) {
        uint32_t load = wdt_calculate_load(pclk_hz, test_cases[i].timeout_us,
                                           test_cases[i].divisor);
        uint64_t actual_us = wdt_calculate_timeout_us(pclk_hz, load,
                                                      test_cases[i].divisor);
        int64_t error_us = (int64_t)actual_us - (int64_t)test_cases[i].timeout_us;

        printf("  [%d] 目标：%lu us → load：0x%08X → 实际：%lu us（误差：%ld us）\n",
               (int)(i + 1),
               (unsigned long)test_cases[i].timeout_us,
               (unsigned int)load,
               (unsigned long)actual_us,
               (long)error_us);
    }

    printf("\n步骤3：使用计算出的值配置 WDT\n");
    // 使用第1组参数（1000ms, 64分频）
    uint32_t calc_load = wdt_calculate_load(pclk_hz, 1000000ULL, WDT_DIVISOR_64);
    uint64_t calc_timeout = wdt_calculate_timeout_us(pclk_hz, calc_load, WDT_DIVISOR_64);

    printf("  目标：1000ms 超时\n");
    printf("  计算加载值：0x%08X (%lu)\n", (unsigned int)calc_load, (unsigned long)calc_load);
    printf("  实际超时：%lu us (%lu ms)\n",
           (unsigned long)calc_timeout,
           (unsigned long)(calc_timeout / 1000));

    // 实际配置并验证
    clock_periph_enable(CLK_WDT);
    wdt_init_reset(calc_load, WDT_DIVISOR_64);
    printf("  当前加载值：0x%08X\n", (unsigned int)wdt_get_load());
    printf("  当前计数值：0x%08X\n", (unsigned int)wdt_get_counter());

    wdt_disable();
    printf("  结果: 计算值与实际寄存器值一致\n");

    print_demo_end();
}

//===========================================
// 场景8：中断回调模式
//===========================================

/**
 * @brief WDT 中断回调函数（场景8使用）
 *
 * 在回调中喂狗并记录中断信息。
 * 注意：回调中必须调用 wdt_feed() 以清除中断。
 */
static void demo_wdt_callback(void)
{
    wdt_irq_count++;
    wdt_irq_flag = true;

    // 在回调中喂狗，清除中断并重置计数器
    wdt_feed();
}

/**
 * @brief 场景8：中断回调模式（注册回调函数处理WDT中断）
 *
 * 演示流程：
 * 1. 注册 WDT 回调函数
 * 2. 启动 WDT 中断模式
 * 3. 在回调中处理中断并喂狗
 * 4. 观察回调触发次数
 *
 * 期望结果：每次 WDT 中断触发时回调被调用，计数器被重置
 */
void scenario_8_callback_mode(void)
{
    print_demo_header(8, "中断回调模式（注册回调函数）");

    printf("步骤1：使能 WDT 时钟\n");
    clock_periph_enable(CLK_WDT);

    printf("\n步骤2：注册回调函数\n");
    wdt_register_callback(demo_wdt_callback);
    printf("  回调函数已注册\n");

    printf("\n步骤3：配置 NVIC 中断\n");
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0x03);
    NVIC_EnableIRQ(WDT_IRQn);
    printf("  结果: NVIC 中断已配置\n");

    printf("\n步骤4：启动 WDT 中断模式\n");
    printf("  加载值：0x%08X\n", (unsigned int)SCENARIO_LOAD_INT);
    printf("  预分频：64分频\n");
    printf("  中断清除时限：0x%04X\n", (unsigned int)SCENARIO_INTCLRTIME);
    wdt_init_interrupt(SCENARIO_LOAD_INT, SCENARIO_DIVISOR_INT, SCENARIO_INTCLRTIME);
    printf("  结果: WDT 中断模式已启动\n");

    printf("\n步骤5：观察回调触发\n");
    printf("  预期：WDT 每秒触发一次中断，回调中喂狗\n");
    printf("  系统不会复位\n\n");

    uint32_t prev_count = wdt_irq_count;
    uint32_t start_tick = prev_count;

    while (wdt_irq_count - start_tick < 5) {
        if (wdt_irq_count != prev_count) {
            printf("  WDT 回调触发！计数：%lu\n", (unsigned long)wdt_irq_count);
            printf("    回调中已完成喂狗，计数器已重置\n");
            uint32_t counter = wdt_get_counter();
            printf("    当前计数值：0x%08X\n", (unsigned int)counter);
            prev_count = wdt_irq_count;
        }
        // 也输出 RIS 状态
        if (wdt_get_ris()) {
            printf("    原始中断状态：1（即将触发回调）\n");
        }
        delay_ms(100);
    }

    printf("\n步骤6：清理\n");
    NVIC_DisableIRQ(WDT_IRQn);
    wdt_disable();
    wdt_register_callback(NULL);
    printf("  结果: WDT 已禁用，回调已取消\n");
    printf("  总回调次数：%lu（含之前场景）\n", (unsigned long)wdt_irq_count);

    print_demo_end();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    // 打印欢迎信息
    printf("\n\n");
    print_separator();
    printf("  ACM32P4 WDT 模块示例程序\n");
    printf("  SDK Version: 1.0\n");
    printf("  当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 检查上次复位原因
    check_reset_source();

    // 根据选择运行场景
    switch (DEFAULT_SCENARIO) {
    case 1:
        scenario_1_reset_mode_normal();
        break;
    case 2:
        scenario_2_reset_mode_timeout();
        break;
    case 3:
        scenario_3_interrupt_mode_normal();
        break;
    case 4:
        scenario_4_interrupt_mode_timeout();
        break;
    case 5:
        scenario_5_struct_config();
        break;
    case 6:
        scenario_6_dynamic_adjust();
        break;
    case 7:
        scenario_7_timeout_calculation();
        break;
    case 8:
        scenario_8_callback_mode();
        break;
    default:
        printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
        printf("请选择 1-8 之间的场景编号\n");
        break;
    }

    // 主循环
    printf("\n进入主循环（空闲等待）...\n");
    while (1) {
        // 如果WDT中断标志置位但场景已结束，重置标志
        if (wdt_irq_flag) {
            wdt_feed();
            wdt_irq_flag = false;
        }
        __WFI();
    }

    return 0;
}
