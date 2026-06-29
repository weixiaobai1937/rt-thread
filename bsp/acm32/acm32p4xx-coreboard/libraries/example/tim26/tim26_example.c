/**
 * @file tim26_example.c
 * @brief TIM26（64位定时器）功能示例程序
 *
 * 演示10个场景：
 * 1.  自由计数模式获取时间戳
 * 2.  周期中断模式（1秒定时）
 * 3.  结构体方式完整配置
 * 4.  测量代码执行时间
 * 5.  运行中动态修改计数周期
 * 6.  轮询更新标志（无中断）
 * 7.  不同预分频配置演示
 * 8.  ARR预装载缓冲模式演示
 * 9.  延时函数（tim26_delay_us / tim26_delay_ms）
 * 10. 超时等待函数（tim26_wait_us / tim26_wait_ms）
 *
 * @author ACM32P4 SDK Team
 * @version 1.1
 */

#include "acm32p4.h"
#include "hardware/tim26.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 10 ///< 修改此值（1-8）选择要运行的场景

//===========================================
// 全局变量
//===========================================

static volatile uint32_t tim26_irq_count = 0; ///< TIM26 中断触发次数

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
// 中断回调函数
//===========================================

/**
 * @brief 场景2使用的TIM26更新中断回调
 */
static void on_tim26_update_scenario2(void)
{
    tim26_irq_count++;
    printf("TIM26中断触发次数: %u\n", tim26_irq_count);
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：自由计数模式获取时间戳
 *
 * 演示如何使用 TIM26 的自由计数模式作为高精度时间戳计数器，
 * 配置1MHz计数频率，分辨率为1微秒。
 */
void scenario_1_freerun_timestamp(void)
{
    print_separator();
    printf("=== 场景1：自由计数模式（时间戳） ===\n\n");

    uint32_t pclk2_hz = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 1000000U);
    printf("步骤1：初始化为自由计数模式（PCLK2=%u Hz，预分频=%u）\n",
           pclk2_hz, prescaler);
    tim26_init_freerun(prescaler);
    uint32_t count_freq   = pclk2_hz / prescaler;
    uint32_t res_ns       = 1000000000U / count_freq;
    uint32_t overflow_yrs = (uint32_t)(UINT64_MAX / count_freq / (365UL * 24 * 3600));
    printf("  结果: TIM26启动，计数频率%u Hz，分辨率%u ns，溢出周期约%u年\n",
           count_freq, res_ns, overflow_yrs);

    printf("\n步骤2：读取第一个时间戳\n");
    uint64_t t1 = tim26_get_counter();
    printf("  时间戳1: %u us\n", (uint32_t)t1);

    printf("\n步骤3：延时100ms后读取第二个时间戳\n");
    delay_ms(100);
    uint64_t t2 = tim26_get_counter();
    printf("  时间戳2: %u us\n", (uint32_t)t2);
    printf("  间隔时间: %u us（预期约100000us）\n", (uint32_t)(t2 - t1));

    printf("\n步骤4：继续读取时间戳（单调递增验证）\n");
    for (int i = 0; i < 3; i++)
    {
        delay_ms(10);
        printf("  时间戳: %u us\n", (uint32_t)tim26_get_counter());
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景2：周期中断模式（1秒定时）
 *
 * 演示如何使用 TIM26 的循环计数模式产生1秒定时中断，
 * 通过回调函数统计中断次数。
 */
void scenario_2_periodic_interrupt(void)
{
    print_separator();
    printf("=== 场景2：周期中断模式（1秒定时） ===\n\n");

    tim26_irq_count = 0;

    printf("步骤1：初始化为1秒周期中断（PCLK2=%u Hz / 1分频 / PCLK2计数 = 1秒）\n", clock_get_pclk2_hz());
    tim26_init_periodic((uint64_t)(clock_get_pclk2_hz() / 1), 1);
    tim26_irq_register(on_tim26_update_scenario2);
    NVIC_SetPriority(TIM26_IRQn, 3);
    NVIC_EnableIRQ(TIM26_IRQn);
    printf("  结果: 周期中断已配置，等待触发...\n");

    printf("\n步骤2：等待3次中断触发\n");
    uint32_t last = tim26_irq_count;
    while ((tim26_irq_count - last) < 3)
    {
        __WFI();
    }
    printf("  结果: 3次中断已触发，总计数 = %u\n", tim26_irq_count);

    printf("\n步骤3：停止定时器并注销回调\n");
    tim26_stop();
    NVIC_DisableIRQ(TIM26_IRQn);
    tim26_irq_unregister();
    printf("  结果: TIM26已停止\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景3：结构体方式完整配置
 *
 * 演示如何使用 tim26_config_t 结构体对 TIM26 进行精细化配置，
 * 覆盖所有可配置参数。
 */
void scenario_3_config_struct(void)
{
    print_separator();
    printf("=== 场景3：结构体完整配置 ===\n\n");

    printf("步骤1：填充配置结构体\n");
    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);  // 使计数频率约为10MHz
    uint64_t arr_1s    = (uint64_t)(pclk2_hz / prescaler); // 10MHz下计1秒所需tick数
    tim26_config_t cfg = {
        .mode       = TIM26_MODE_PERIODIC,  // 循环计数
        .prescaler  = prescaler,            // PCLK2 / prescaler ≈ 10MHz
        .arr        = arr_1s,               // 计满1秒触发更新
        .counter    = 0,                    // 从0开始计数
        .arpe       = true,                 // ARR带缓冲
        .irq_enable = false,                // 轮询模式，不使能中断
    };
    printf("  模式: 循环计数\n");
    printf("  PCLK2: %u Hz\n", pclk2_hz);
    printf("  预分频: %u（计数频率=%u Hz）\n", cfg.prescaler, pclk2_hz / prescaler);
    printf("  ARR: %u（1秒周期）\n", (uint32_t)cfg.arr);
    printf("  ARPE: %s\n", cfg.arpe ? "使能" : "禁用");

    printf("\n步骤2：应用配置\n");
    bool ok = tim26_config(&cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");

    printf("\n步骤3：读回并验证配置\n");
    printf("  读回预分频值: %u（预期%u）\n", tim26_get_prescaler(), cfg.prescaler);
    printf("  读回ARR值: %u（预期%u）\n", (uint32_t)tim26_get_arr(), (uint32_t)cfg.arr);
    printf("  运行状态: %s\n", tim26_is_running() ? "运行中" : "已停止");

    printf("\n步骤4：停止\n");
    tim26_stop();
    printf("  结果: 已停止，状态=%s\n", tim26_is_running() ? "运行中" : "已停止");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景4：测量代码执行时间
 *
 * 演示如何使用 TIM26 精确测量一段代码的执行时间（微秒级精度）。
 */
void scenario_4_elapsed_time(void)
{
    print_separator();
    printf("=== 场景4：测量代码执行时间 ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t res_ns    = 1000000000U / count_hz;
    printf("步骤1：初始化高分辨率计时器（PCLK2=%u Hz / 预分频%u = %u Hz，分辨率%u ns）\n",
           pclk2_hz, prescaler, count_hz, res_ns);
    tim26_init_freerun(prescaler);
    printf("  结果: 已启动\n");

    printf("\n步骤2：测量delay_ms(50)的耗时\n");
    uint64_t t_start = tim26_get_counter();
    delay_ms(50);
    uint64_t t_end = tim26_get_counter();
    uint32_t elapsed_us = (uint32_t)((t_end - t_start) / (count_hz / 1000000U));
    printf("  预期耗时: 50000 us\n");
    printf("  实际耗时: %u us\n", elapsed_us);

    printf("\n步骤3：测量简单循环的耗时\n");
    t_start = tim26_get_counter();
    volatile uint32_t dummy = 0;
    for (uint32_t i = 0; i < 100000; i++)
    {
        dummy += i;
    }
    t_end = tim26_get_counter();
    (void)dummy;
    printf("  100000次循环耗时: %u us\n",
           (uint32_t)((t_end - t_start) / (count_hz / 1000000U)));

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景5：运行中动态修改计数周期
 *
 * 演示如何在定时器运行期间通过 tim26_set_arr() 动态修改计数周期。
 */
void scenario_5_change_period(void)
{
    print_separator();
    printf("=== 场景5：运行中动态修改ARR ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);
    uint32_t arr_500ms = pclk2_hz / prescaler / 2U;   // 计数频率下500ms的tick数
    uint32_t arr_1s    = pclk2_hz / prescaler;         // 计数频率下1秒的tick数

    printf("步骤1：启动500ms周期（轮询模式，ARR=%u）\n", arr_500ms);
    tim26_config_t cfg = {
        .mode       = TIM26_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_500ms,
        .counter    = 0,
        .arpe       = false,
        .irq_enable = false,
    };
    tim26_config(&cfg);
    printf("  已配置（PCLK2=%u Hz，预分频=%u，计数频率=%u Hz）\n",
           pclk2_hz, prescaler, pclk2_hz / prescaler);

    printf("\n步骤2：等待第1次触发（约500ms）\n");
    while (!tim26_flag_is_set())
    {
        __NOP();
    }
    tim26_flag_clear();
    printf("  第1次触发完成（500ms周期）\n");

    printf("\n步骤3：修改ARR为1秒（ARR=%u）\n", arr_1s);
    tim26_set_arr(arr_1s);
    printf("  ARR已更新为: %u\n", (uint32_t)tim26_get_arr());

    printf("\n步骤4：等待第2次触发（约1秒）\n");
    while (!tim26_flag_is_set())
    {
        __NOP();
    }
    tim26_flag_clear();
    printf("  第2次触发完成（1秒周期）\n");

    tim26_stop();
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景6：轮询更新标志（无中断）
 *
 * 演示在不使用中断的情况下，通过轮询 UF 标志实现定时功能。
 */
void scenario_6_polling_mode(void)
{
    print_separator();
    printf("=== 场景6：轮询更新标志（无中断） ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);  // 目标计数频率 10MHz
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t arr_1ms   = count_hz / 1000U;                  // 计数频率下 1ms 的 tick 数

    printf("步骤1：配置循环模式（禁用中断，ARR=%u）\n", arr_1ms);
    tim26_config_t cfg = {
        .mode       = TIM26_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_1ms,
        .counter    = 0,
        .arpe       = false,
        .irq_enable = false,
    };
    tim26_config(&cfg);
    printf("  已配置（PCLK2=%u Hz，预分频=%u，计数频率=%u Hz，周期1ms）\n",
           pclk2_hz, prescaler, count_hz);

    printf("\n步骤2：轮询等待5次更新事件\n");
    uint32_t count = 0;
    while (count < 5)
    {
        if (tim26_flag_is_set())
        {
            tim26_flag_clear();
            count++;
            printf("  第%u次更新触发（CNT已回零）\n", count);
        }
    }

    tim26_stop();
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景7：不同预分频配置演示
 *
 * 演示预分频器的边界值和典型值配置，验证读回值正确。
 */
void scenario_7_prescaler_config(void)
{
    print_separator();
    printf("=== 场景7：不同预分频配置演示 ===\n\n");

    uint32_t pclk2_hz = clock_get_pclk2_hz();

    tim26_init_freerun(1);

    const uint8_t prescalers[] = {1, 4, 44, 64};

    for (int i = 0; i < 4; i++)
    {
        uint32_t count_hz = pclk2_hz / prescalers[i];
        printf("步骤%d：预分频=%u（计数频率=%u Hz）\n", i + 1, prescalers[i], count_hz);
        tim26_set_prescaler(prescalers[i]);
        printf("  读回预分频值: %u\n", tim26_get_prescaler());
    }

    printf("\n步骤5：边界修正验证（输入0，期望修正为1）\n");
    tim26_set_prescaler(0);
    printf("  输入0，读回预分频值: %u（应为1）\n", tim26_get_prescaler());

    printf("\n步骤6：边界修正验证（输入100，期望修正为64）\n");
    tim26_set_prescaler(100);
    printf("  输入100，读回预分频值: %u（应为64）\n", tim26_get_prescaler());

    tim26_stop();
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景8：ARR预装载缓冲模式演示
 *
 * 演示 ARPE=1 时，写入新 ARR 值不立即生效，而是在下次更新事件后
 * 才切换到新周期，避免周期切换时的竞争问题。
 */
void scenario_8_arpe_buffered(void)
{
    print_separator();
    printf("=== 场景8：ARR预装载缓冲模式（ARPE=1） ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);  // 目标计数频率 10MHz
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t arr_200ms = count_hz / 5U;   // 200ms 的 tick 数
    uint32_t arr_1s    = count_hz;         // 1秒 的 tick 数

    printf("步骤1：启动定时器，使能ARPE缓冲（ARR=%u，初始200ms）\n", arr_200ms);
    tim26_config_t cfg = {
        .mode       = TIM26_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_200ms,
        .counter    = 0,
        .arpe       = true,
        .irq_enable = false,
    };
    tim26_config(&cfg);
    printf("  已配置（PCLK2=%u Hz，预分频=%u，计数频率=%u Hz）\n",
           pclk2_hz, prescaler, count_hz);
    printf("  当前ARR: %u（200ms）\n", (uint32_t)tim26_get_arr());

    printf("\n步骤2：写入新ARR=%u（1秒），因ARPE=1，当前周期不变\n", arr_1s);
    tim26_set_arr(arr_1s);
    printf("  写入后立即读取ARR: %u（仍是%u，新值在缓冲中）\n",
           (uint32_t)tim26_get_arr(), arr_200ms);

    printf("\n步骤3：等待第1次更新事件（使用旧200ms周期）\n");
    while (!tim26_flag_is_set())
    {
        __NOP();
    }
    tim26_flag_clear();
    printf("  第1次更新完成，ARR已切换为: %u（1秒）\n", (uint32_t)tim26_get_arr());

    printf("\n步骤4：等待第2次更新事件（使用新1秒周期）\n");
    while (!tim26_flag_is_set())
    {
        __NOP();
    }
    tim26_flag_clear();
    printf("  第2次更新完成（1秒周期确认）\n");

    tim26_stop();
    tim26_set_arpe(false);
    printf("  ARPE已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景9：延时函数演示
 *
 * 演示 tim26_init_freerun() + tim26_delay_us() + tim26_delay_ms() 的用法，
 * 并与 SysTick 计时结果对比验证延时精度。
 */
void scenario_9_delay_functions(void)
{
    print_separator();
    printf("=== 场景9：延时函数演示 ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);  // 目标计数频率 10MHz
    uint32_t count_hz  = pclk2_hz / prescaler;

    printf("步骤1：初始化自由计数模式（PCLK2=%u Hz，预分频=%u，计数频率=%u Hz）\n",
           pclk2_hz, prescaler, count_hz);
    tim26_init_freerun(prescaler);
    printf("  结果: 延时功能已就绪\n");

    printf("\n步骤2：微秒延时验证（100us）\n");
    uint64_t t0 = system_get_tick();
    tim26_delay_us(100);
    printf("  耗时: %u ms（100us应接近0ms）\n", (uint32_t)(system_get_tick() - t0));

    printf("\n步骤3：毫秒延时验证（50ms）\n");
    t0 = system_get_tick();
    tim26_delay_ms(50);
    printf("  耗时: %u ms（预期50ms）\n", (uint32_t)(system_get_tick() - t0));

    printf("\n步骤4：毫秒延时验证（200ms）\n");
    t0 = system_get_tick();
    tim26_delay_ms(200);
    printf("  耗时: %u ms（预期200ms）\n", (uint32_t)(system_get_tick() - t0));

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景9辅助：模拟一个需要等待的事件（计数器超过阈值时返回true）
 */
static volatile uint32_t wait_counter = 0;

static bool condition_counter_ready(void)
{
    wait_counter++;
    return (wait_counter >= 1000U);  // 实测轮询速率约 228次/ms，1000次约 4ms，在 10ms 超时内可靠完成
}

static bool condition_always_false(void)
{
    return false;
}

/**
 * @brief 场景10：超时等待函数演示
 *
 * 演示 tim26_wait_us() / tim26_wait_ms() 的两种使用情形：
 * - 条件在超时前满足（返回 true）
 * - 条件从未满足，超时退出（返回 false）
 */
void scenario_10_timeout_wait(void)
{
    print_separator();
    printf("=== 场景10：超时等待函数演示 ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);  // 目标计数频率 10MHz
    uint32_t count_hz  = pclk2_hz / prescaler;

    printf("步骤1：初始化自由计数模式（PCLK2=%u Hz，预分频=%u，计数频率=%u Hz）\n",
           pclk2_hz, prescaler, count_hz);
    tim26_init_freerun(prescaler);
    printf("  结果: 已就绪\n");

    printf("\n步骤2：等待条件满足（微秒超时，预期在超时前完成）\n");
    wait_counter = 0;
    uint64_t t0 = system_get_tick();
    bool ok = tim26_wait_us(condition_counter_ready, 10000); // 超时10ms
    printf("  结果: %s（耗时约%u ms）\n", ok ? "条件已满足" : "超时退出", (uint32_t)(system_get_tick() - t0));
    printf("  轮询次数: %u\n", wait_counter);

    printf("\n步骤3：等待一个永远不满足的条件（毫秒超时，预期超时）\n");
    t0 = system_get_tick();
    ok = tim26_wait_ms(condition_always_false, 50); // 超时50ms
    printf("  结果: %s（耗时约%u ms，预期50ms）\n", ok ? "条件已满足" : "超时退出", (uint32_t)(system_get_tick() - t0));

    printf("\n步骤4：条件立即满足的快速路径验证\n");
    ok = tim26_wait_ms(condition_counter_ready, 100); // counter已>=50000，立即true
    printf("  结果: %s（应为[条件已满足]，无延迟）\n", ok ? "条件已满足" : "超时退出");

    printf("\n完成！\n");
    print_separator();
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
    printf("TIM26（64位定时器）示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO)
    {
    case 1:
        scenario_1_freerun_timestamp();
        break;
    case 2:
        scenario_2_periodic_interrupt();
        break;
    case 3:
        scenario_3_config_struct();
        break;
    case 4:
        scenario_4_elapsed_time();
        break;
    case 5:
        scenario_5_change_period();
        break;
    case 6:
        scenario_6_polling_mode();
        break;
    case 7:
        scenario_7_prescaler_config();
        break;
    case 8:
        scenario_8_arpe_buffered();
        break;
    case 9:
        scenario_9_delay_functions();
        break;
    case 10:
        scenario_10_timeout_wait();
        break;
    default:
        printf("错误：无效的场景编号 %d（有效范围：1-10）\n", DEFAULT_SCENARIO);
        break;
    }

    while (1)
    {
        __WFI();
    }
}

//=========================================== 
// 中断服务函数
//===========================================

