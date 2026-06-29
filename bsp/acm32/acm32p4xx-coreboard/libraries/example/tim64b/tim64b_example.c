/**
 * @file tim64b_example.c
 * @brief TIM64B（64位定时器）功能示例程序
 *
 * 演示10个场景：
 *  1. 自由计数模式（高精度时间戳）
 *  2. 周期中断模式（1秒定时）
 *  3. 结构体方式完整配置
 *  4. 测量代码/延时函数执行时间
 *  5. 运行中动态修改计数周期
 *  6. 轮询更新标志（无中断）
 *  7. 不同预分频配置演示
 *  8. ARR预装载缓冲模式（ARPE）
 *  9. 系统延时函数精度验证（delay_ms / delay_us）
 * 10. 手动超时等待（system_get_tick / system_elapsed）
 *
 * @note 系统时基说明：
 *   system_init() 使用 TIM64B（TIM26）作为系统时基（自由计数，prescaler=1）。
 *   场景 1/2/3/5/6/7/8 会临时重配 TIM64B，场景结束时自动调用
 *   system_timebase_init() 恢复时基，保证场景间 delay_ms/delay_us 正常工作。
 *   场景 4/9/10 不重配 TIM64B，直接复用系统时基。
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#include "acm32p4.h"
#include "hardware/tim64b.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...)     SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 9  ///< 修改此值（1-10）选择要运行的场景

//===========================================
// 全局变量
//===========================================

static volatile uint32_t s_irq_count = 0;  ///< TIM64B 中断触发次数

//===========================================
// 内部辅助函数
//===========================================

/**
 * @brief 基于当前 TIM64B 计数器的 tick 级忙等待
 *
 * 在 TIM64B 被重配为自定义预分频时使用，不依赖 system 的换算系数，
 * 直接等待硬件计数器走过指定 tick 数。64 位无符号减法自然处理回绕。
 */
static void wait_ticks(uint64_t ticks)
{
    uint64_t start = tim64b_get_counter();
    while ((tim64b_get_counter() - start) < ticks)
    {
        /* 忙等待 */
    }
}

static void print_separator(void)
{
    printf("========================================\n");
}

//===========================================
// 中断回调
//===========================================

static void on_tim64b_update(void)
{
    s_irq_count++;
    printf("  [中断] 第%u次触发\n", s_irq_count);
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：自由计数模式（高精度时间戳）
 *
 * 以最大预分频64（PCLK2=110MHz → 计数频率≈1.72MHz）初始化自由计数，
 * 读取两次原始计数器并换算为微秒，验证单调递增。
 */
void scenario_1_freerun_timestamp(void)
{
    print_separator();
    printf("=== 场景1：自由计数模式（时间戳） ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = 64U;                        /* 使用最大预分频 */
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t res_ns    = 1000000000U / count_hz;

    printf("步骤1：初始化自由计数（PCLK2=%u Hz / %u = %u Hz，分辨率%u ns）\n",
           pclk2_hz, prescaler, count_hz, res_ns);
    tim64b_init_freerun(prescaler);

    printf("\n步骤2：读取第一个时间戳\n");
    uint64_t t1    = tim64b_get_counter();
    uint32_t t1_us = (uint32_t)(t1 / (count_hz / 1000000U));
    printf("  计数值: %u (低32位) = %u us\n", (uint32_t)t1, t1_us);

    printf("\n步骤3：等待约100ms后读取第二个时间戳\n");
    wait_ticks((uint64_t)count_hz / 10U);           /* 100ms */
    uint64_t t2       = tim64b_get_counter();
    uint32_t elapsed  = (uint32_t)((t2 - t1) / (count_hz / 1000000U));
    printf("  经过时间: %u us（预期约100000 us）\n", elapsed);

    printf("\n步骤4：单调递增验证（每次约10ms）\n");
    for (int i = 0; i < 3; i++)
    {
        wait_ticks((uint64_t)count_hz / 100U);       /* 10ms */
        uint64_t ts = tim64b_get_counter();
        printf("  计数值: %u (低32位)\n", (uint32_t)ts);
    }

    /* 恢复系统时基 */
    system_timebase_init();
    printf("\n完成！（系统时基已恢复）\n");
    print_separator();
}

/**
 * @brief 场景2：周期中断模式（1秒定时）
 *
 * 配置 TIM64B 为循环计数，ARR = PCLK2，预分频=1，每秒触发一次中断。
 */
void scenario_2_periodic_interrupt(void)
{
    print_separator();
    printf("=== 场景2：周期中断模式（1秒定时） ===\n\n");

    s_irq_count = 0;
    uint32_t pclk2_hz = clock_get_pclk2_hz();

    printf("步骤1：配置1秒周期中断（ARR=%u，预分频=1）\n", pclk2_hz);
    tim64b_init_periodic((uint64_t)pclk2_hz, 1);
    tim64b_irq_register(on_tim64b_update);
    NVIC_SetPriority(TIM64B_IRQn, 3);
    NVIC_EnableIRQ(TIM64B_IRQn);
    printf("  已启动，等待触发...\n");

    printf("\n步骤2：等待3次中断\n");
    while (s_irq_count < 3)
    {
        __WFI();
    }
    printf("  共触发 %u 次\n", s_irq_count);

    printf("\n步骤3：停止并清理\n");
    NVIC_DisableIRQ(TIM64B_IRQn);
    tim64b_stop();
    tim64b_irq_unregister();
    system_timebase_init();
    printf("  TIM64B 已停止，系统时基已恢复\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景3：结构体方式完整配置
 *
 * 使用 tim64b_config_t 对所有参数进行精细控制，读回验证一致性。
 */
void scenario_3_config_struct(void)
{
    print_separator();
    printf("=== 场景3：结构体完整配置 ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U); /* 目标计数频率10MHz */
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint64_t arr_1s    = (uint64_t)count_hz;               /* 1秒对应 tick 数 */

    printf("步骤1：填充配置结构体\n");
    printf("  PCLK2=%u Hz，预分频=%u，计数频率=%u Hz\n", pclk2_hz, prescaler, count_hz);
    printf("  ARR=%u（1秒周期），ARPE=使能，中断=禁用\n", (uint32_t)arr_1s);

    tim64b_config_t cfg = {
        .mode       = TIM64B_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_1s,
        .counter    = 0,
        .arpe       = true,
        .irq_enable = false,
    };

    printf("\n步骤2：应用配置\n");
    bool ok = tim64b_config(&cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");

    printf("\n步骤3：读回验证\n");
    printf("  预分频值: %u（预期%u）\n",   tim64b_get_prescaler(), prescaler);
    printf("  ARR值:    %u（预期%u）\n",   (uint32_t)tim64b_get_arr(), (uint32_t)arr_1s);
    printf("  运行状态: %s\n",             tim64b_is_running() ? "运行中" : "已停止");

    printf("\n步骤4：停止\n");
    tim64b_stop();
    system_timebase_init();
    printf("  已停止，系统时基已恢复\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景4：测量代码/延时函数执行时间
 *
 * 复用系统时基（prescaler=1，计数频率=PCLK2=110MHz），
 * 直接读取 tim64b_get_counter() 来测量实际耗时，无需重配 TIM64B。
 */
void scenario_4_elapsed_time(void)
{
    print_separator();
    printf("=== 场景4：测量代码执行时间 ===\n\n");

    uint32_t pclk2_hz    = clock_get_pclk2_hz();
    uint32_t count_hz    = pclk2_hz / TIM64B_TIMEBASE_PRESCALER;
    uint32_t ticks_per_us = count_hz / 1000000U;

    printf("系统时基：计数频率=%u Hz，分辨率约%u ns\n\n",
           count_hz, 1000000000U / count_hz);

    printf("步骤1：测量 delay_ms(50) 实际耗时\n");
    uint64_t t0 = tim64b_get_counter();
    delay_ms(50);
    uint64_t t1 = tim64b_get_counter();
    printf("  预期: 50000 us，实测: %u us\n",
           (uint32_t)((t1 - t0) / ticks_per_us));

    printf("\n步骤2：测量 delay_us(1000) 实际耗时\n");
    t0 = tim64b_get_counter();
    delay_us(1000);
    t1 = tim64b_get_counter();
    printf("  预期: 1000 us，实测: %u us\n",
           (uint32_t)((t1 - t0) / ticks_per_us));

    printf("\n步骤3：测量 100000 次循环累加耗时\n");
    t0 = tim64b_get_counter();
    volatile uint32_t dummy = 0;
    for (uint32_t i = 0; i < 100000U; i++)
    {
        dummy += i;
    }
    t1 = tim64b_get_counter();
    (void)dummy;
    printf("  耗时: %u us\n", (uint32_t)((t1 - t0) / ticks_per_us));

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景5：运行中动态修改计数周期
 *
 * 先以500ms周期启动，触发一次后改为1秒周期，验证动态 ARR 修改生效。
 */
void scenario_5_change_period(void)
{
    print_separator();
    printf("=== 场景5：运行中动态修改 ARR ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t arr_500ms = count_hz / 2U;
    uint32_t arr_1s    = count_hz;

    printf("步骤1：启动500ms周期（ARR=%u，轮询模式）\n", arr_500ms);
    tim64b_config_t cfg = {
        .mode       = TIM64B_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_500ms,
        .counter    = 0,
        .arpe       = false,
        .irq_enable = false,
    };
    tim64b_config(&cfg);
    printf("  计数频率=%u Hz\n", count_hz);

    printf("\n步骤2：等待第1次触发（约500ms）\n");
    while (!tim64b_flag_is_set()) { __NOP(); }
    tim64b_flag_clear();
    printf("  第1次触发完成\n");

    printf("\n步骤3：修改 ARR 为1秒（ARR=%u）\n", arr_1s);
    tim64b_set_arr(arr_1s);
    printf("  当前 ARR=%u\n", (uint32_t)tim64b_get_arr());

    printf("\n步骤4：等待第2次触发（约1秒）\n");
    while (!tim64b_flag_is_set()) { __NOP(); }
    tim64b_flag_clear();
    printf("  第2次触发完成\n");

    tim64b_stop();
    system_timebase_init();
    printf("\n完成！（系统时基已恢复）\n");
    print_separator();
}

/**
 * @brief 场景6：轮询更新标志（无中断）
 *
 * 配置1ms周期的循环计数模式，禁用中断，通过轮询 UF 标志统计更新次数。
 */
void scenario_6_polling_mode(void)
{
    print_separator();
    printf("=== 场景6：轮询更新标志（无中断） ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t arr_1ms   = count_hz / 1000U;

    printf("步骤1：配置1ms循环计数（禁用中断，ARR=%u）\n", arr_1ms);
    tim64b_config_t cfg = {
        .mode       = TIM64B_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_1ms,
        .counter    = 0,
        .arpe       = false,
        .irq_enable = false,
    };
    tim64b_config(&cfg);
    printf("  计数频率=%u Hz，周期1ms\n", count_hz);

    printf("\n步骤2：轮询等待5次更新事件\n");
    uint32_t count = 0;
    while (count < 5)
    {
        if (tim64b_flag_is_set())
        {
            tim64b_flag_clear();
            count++;
            printf("  第%u次更新触发（计数器已回零）\n", count);
        }
    }

    tim64b_stop();
    system_timebase_init();
    printf("\n完成！（系统时基已恢复）\n");
    print_separator();
}

/**
 * @brief 场景7：不同预分频配置演示
 *
 * 遍历典型预分频值，验证读回值正确；同时测试边界修正（0→1，100→64）。
 */
void scenario_7_prescaler_config(void)
{
    print_separator();
    printf("=== 场景7：不同预分频配置演示 ===\n\n");

    uint32_t pclk2_hz = clock_get_pclk2_hz();
    tim64b_init_freerun(1);

    const uint8_t  prescalers[] = {1, 4, 16, 64};
    const int      n = (int)(sizeof(prescalers) / sizeof(prescalers[0]));

    for (int i = 0; i < n; i++)
    {
        tim64b_set_prescaler(prescalers[i]);
        uint32_t count_hz = pclk2_hz / prescalers[i];
        printf("预分频=%-3u → 计数频率=%-9u Hz，读回=%-3u（%s）\n",
               prescalers[i], count_hz, tim64b_get_prescaler(),
               tim64b_get_prescaler() == prescalers[i] ? "OK" : "FAIL");
    }

    printf("\n边界修正验证：\n");
    tim64b_set_prescaler(0);
    printf("  输入0   → 读回 %u（应为1，%s）\n",
           tim64b_get_prescaler(),
           tim64b_get_prescaler() == 1 ? "OK" : "FAIL");

    tim64b_set_prescaler(100);
    printf("  输入100 → 读回 %u（应为64，%s）\n",
           tim64b_get_prescaler(),
           tim64b_get_prescaler() == 64 ? "OK" : "FAIL");

    tim64b_stop();
    system_timebase_init();
    printf("\n完成！（系统时基已恢复）\n");
    print_separator();
}

/**
 * @brief 场景8：ARR预装载缓冲模式（ARPE=1）
 *
 * 演示使能 ARPE 后，写入新 ARR 不立即生效，
 * 而是在当前计数周期结束（更新事件）后才切换。
 */
void scenario_8_arpe_buffered(void)
{
    print_separator();
    printf("=== 场景8：ARR预装载缓冲模式（ARPE=1） ===\n\n");

    uint32_t pclk2_hz  = clock_get_pclk2_hz();
    uint8_t  prescaler = (uint8_t)(pclk2_hz / 10000000U);
    uint32_t count_hz  = pclk2_hz / prescaler;
    uint32_t arr_200ms = count_hz / 5U;
    uint32_t arr_1s    = count_hz;

    printf("步骤1：启动（ARPE=1，初始ARR=%u，200ms周期）\n", arr_200ms);
    tim64b_config_t cfg = {
        .mode       = TIM64B_MODE_PERIODIC,
        .prescaler  = prescaler,
        .arr        = arr_200ms,
        .counter    = 0,
        .arpe       = true,
        .irq_enable = false,
    };
    tim64b_config(&cfg);
    printf("  计数频率=%u Hz\n", count_hz);

    printf("\n步骤2：写入新ARR=%u（1秒），ARPE=1 → 不立即生效\n", arr_1s);
    tim64b_set_arr(arr_1s);
    printf("  立即读回 ARR=%u（仍是旧值%u，新值在预装载缓冲中）\n",
           (uint32_t)tim64b_get_arr(), arr_200ms);

    printf("\n步骤3：等待第1次更新（旧200ms周期结束）→ 新ARR生效\n");
    while (!tim64b_flag_is_set()) { __NOP(); }
    tim64b_flag_clear();
    printf("  更新完成，ARR已切换为 %u（1秒）\n", (uint32_t)tim64b_get_arr());

    printf("\n步骤4：等待第2次更新（确认已使用新1秒周期）\n");
    while (!tim64b_flag_is_set()) { __NOP(); }
    tim64b_flag_clear();
    printf("  第2次更新完成（1秒周期确认）\n");

    tim64b_stop();
    tim64b_set_arpe(false);
    system_timebase_init();
    printf("\n完成！（系统时基已恢复）\n");
    print_separator();
}

/**
 * @brief 场景9：系统延时函数精度验证（delay_ms / delay_us）
 *
 * 复用系统时基，在延时前后读取 tim64b_get_counter() 原始值，
 * 换算为微秒并与期望值对比，输出误差。
 */
void scenario_9_delay_accuracy(void)
{
    print_separator();
    printf("=== 场景9：延时函数精度验证 ===\n\n");

    uint32_t pclk2_hz    = clock_get_pclk2_hz();
    uint32_t count_hz    = pclk2_hz / TIM64B_TIMEBASE_PRESCALER;
    uint32_t ticks_per_us = count_hz / 1000000U;

    printf("系统时基：PCLK2=%u Hz，计数频率=%u Hz，精度≈%u ns\n\n",
           pclk2_hz, count_hz, 1000000000U / count_hz);

    /* delay_ms 精度 */
    const uint32_t ms_cases[]     = {1, 5, 10, 50, 100};
    const int      ms_cases_count = (int)(sizeof(ms_cases) / sizeof(ms_cases[0]));

    printf("delay_ms 精度验证：\n");
    for (int i = 0; i < ms_cases_count; i++)
    {
        uint32_t ms        = ms_cases[i];
        uint64_t t0        = tim64b_get_counter();
        delay_ms(ms);
        uint64_t t1        = tim64b_get_counter();
        uint32_t actual_us = (uint32_t)((t1 - t0) / ticks_per_us);
        uint32_t expect_us = ms * 1000U;
        int32_t  error_us  = (int32_t)actual_us - (int32_t)expect_us;
        printf("  delay_ms(%4u): 预期%6u us，实测%6u us，误差%+d us\n",
               ms, expect_us, actual_us, error_us);
    }

    /* delay_us 精度 */
    const uint32_t us_cases[]     = {10, 50, 100, 500, 1000};
    const int      us_cases_count = (int)(sizeof(us_cases) / sizeof(us_cases[0]));

    printf("\ndelay_us 精度验证：\n");
    for (int i = 0; i < us_cases_count; i++)
    {
        uint32_t us        = us_cases[i];
        uint64_t t0        = tim64b_get_counter();
        delay_us(us);
        uint64_t t1        = tim64b_get_counter();
        uint32_t actual_us = (uint32_t)((t1 - t0) / ticks_per_us);
        int32_t  error_us  = (int32_t)actual_us - (int32_t)us;
        printf("  delay_us(%5u): 预期%5u us，实测%5u us，误差%+d us\n",
               us, us, actual_us, error_us);
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景10：手动超时等待（system_get_tick / system_elapsed）
 *
 * 演示以 system_get_tick() 为基准实现非阻塞超时等待的三种情形：
 *   A. 条件在超时前满足（成功）
 *   B. 条件永不满足，超时退出
 *   C. system_get_tick() 连续读取，验证单调递增
 */
void scenario_10_timeout_wait(void)
{
    print_separator();
    printf("=== 场景10：手动超时等待（system_get_tick） ===\n\n");

    /* --- 情形A：条件在超时前满足 --- */
    printf("情形A：轮询计数器达到10000（超时50ms）\n");
    volatile uint32_t counter = 0;
    uint64_t t0 = system_get_tick();
    bool ok = false;
    while (!system_elapsed(t0, 50))
    {
        counter++;
        if (counter >= 10000U)
        {
            ok = true;
            break;
        }
    }
    uint32_t elapsed_ms = (uint32_t)(system_get_tick() - t0);
    printf("  结果: %s，轮询次数=%u，耗时≈%u ms\n",
           ok ? "条件已满足" : "超时退出", counter, elapsed_ms);

    /* --- 情形B：条件永不满足，超时退出 --- */
    printf("\n情形B：等待永不满足的条件（超时100ms）\n");
    t0 = system_get_tick();
    while (!system_elapsed(t0, 100))
    {
        /* 条件永不满足，超时退出 */
    }
    elapsed_ms = (uint32_t)(system_get_tick() - t0);
    printf("  结果: 超时退出，耗时≈%u ms（预期100ms）\n", elapsed_ms);

    /* --- 情形C：system_get_tick 单调递增验证 --- */
    printf("\n情形C：system_get_tick() 单调递增验证\n");
    uint64_t prev = system_get_tick();
    for (int i = 0; i < 5; i++)
    {
        delay_ms(10);
        uint64_t curr = system_get_tick();
        printf("  tick=%u ms（+%u ms）\n",
               (uint32_t)curr, (uint32_t)(curr - prev));
        prev = curr;
    }

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    /* 系统时钟（PLL 220MHz）+ TIM64B 时基（自由计数，prescaler=1） */
    system_init();
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("TIM64B（64位定时器）示例程序 v2.0\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO)
    {
    case 1:  scenario_1_freerun_timestamp();  break;
    case 2:  scenario_2_periodic_interrupt(); break;
    case 3:  scenario_3_config_struct();      break;
    case 4:  scenario_4_elapsed_time();       break;
    case 5:  scenario_5_change_period();      break;
    case 6:  scenario_6_polling_mode();       break;
    case 7:  scenario_7_prescaler_config();   break;
    case 8:  scenario_8_arpe_buffered();      break;
    case 9:  scenario_9_delay_accuracy();     break;
    case 10: scenario_10_timeout_wait();      break;
    default:
        printf("错误：无效的场景编号 %d（有效范围：1-10）\n", DEFAULT_SCENARIO);
        break;
    }

    while (1)
    {
        __WFI();
    }
}
