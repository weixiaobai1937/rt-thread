/**
 * @file clocks_example.c
 * @brief 时钟系统示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 默认初始化
 * 2. 外部晶振 + PLL
 * 3. 时钟查询
 * 4. 外设时钟控制
 * 5. 时钟源校准
 * 6. 系统时钟切换
 * 7. 异步XTH启动
 * 8. PLL锁定中断
 * 9. CSS停振检测
 * 10. 时钟源输出
 */

#include "acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"
#include <stdbool.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 默认运行的场景
#define DEFAULT_SCENARIO 3

// 外部晶振频率选择（8MHz 或 12MHz）
// #define EXTERNAL_XTAL_8MHZ   // 8MHz 晶振
#define EXTERNAL_XTAL_12MHZ // 12MHz 晶振（默认）

//===========================================
// 全局变量
//===========================================

static volatile bool xth_ready = false;
static volatile bool pll1_locked = false;
static volatile bool xth_css_fault = false;
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
 * @brief 打印时钟信息
 */
void print_clock_info(void)
{
    printf("当前时钟配置：\n");
    printf("  SYSCLK: %u Hz\n", clock_get_hz());
    printf("  HCLK:   %u Hz\n", clock_get_hclk_hz());
    printf("  PCLK1:  %u Hz\n", clock_get_pclk1_hz());
    printf("  PCLK2:  %u Hz\n", clock_get_pclk2_hz());

    sysclk_src_t src = clock_get_sysclk_source();
    const char *src_name[] = {"RCH", "未知", "XTH", "PLL1"};
    printf("  时钟源: %s\n", src_name[src]);
}

//===========================================
// 中断回调函数
//===========================================

void xth_ready_callback(void)
{
    xth_ready = true;
    printf("[中断] XTH已就绪！\n");
}

void pll1_lock_callback(void)
{
    pll1_locked = true;
    printf("[中断] PLL1已锁定！\n");
}

void xth_css_callback(void)
{
    xth_css_fault = true;
    printf("[中断] XTH停振检测到！\n");
    printf("  系统已自动切换到RCH\n");
    printf("  当前时钟源: ");

    sysclk_src_t src = clock_get_sysclk_source();
    const char *src_name[] = {"RCH", "未知", "XTH", "PLL1"};
    printf("%s\n", src_name[src]);
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：默认初始化
 */
void scenario_1_default_init(void)
{
    print_separator();
    printf("=== 场景1：默认初始化 ===\n\n");

    printf("初始化系统时钟到180MHz...\n");
    clock_init_default();

    print_clock_info();
    printf("\n初始化完成！\n");
    print_separator();
}

/**
 * @brief 场景2：外部晶振 + PLL
 */
void scenario_2_external_xtal(void)
{
    print_separator();
    printf("=== 场景2：外部晶振 + PLL ===\n\n");

    // 根据条件编译选择晶振频率和PLL参数
#ifdef EXTERNAL_XTAL_8MHZ
#define XTAL_FREQ_HZ 8000000
#define XTAL_FREQ_STR "8MHz"
#define PLL_N_DIV 4   // 8MHz / 4 = 2MHz
#define PLL_F_MUL 220 // 2MHz * 220 = 440MHz
#elif defined(EXTERNAL_XTAL_12MHZ)
#define XTAL_FREQ_HZ 12000000
#define XTAL_FREQ_STR "12MHz"
#define PLL_N_DIV 6   // 12MHz / 6 = 2MHz
#define PLL_F_MUL 220 // 2MHz * 220 = 440MHz
#else
#error "请定义 EXTERNAL_XTAL_8MHZ 或 EXTERNAL_XTAL_12MHZ"
#endif

    printf("启动%s外部晶振...\n", XTAL_FREQ_STR);
    if (clock_xth_enable(XTAL_FREQ_HZ, false))
    {
        printf("XTH启动成功\n");

        printf("配置PLL到220MHz...\n");
        pll_config_t pll = {
            .src = 1,       // XTH
            .n = PLL_N_DIV, // 输入分频
            .f = PLL_F_MUL, // VCO倍频
            .p = 0,         // 440MHz / 2 = 220MHz
            .q = 3,         // 440MHz / 4 = 110MHz
        };

        if (clock_pll1_configure(&pll))
        {
            printf("PLL配置成功\n");

            // 配置分频
            clock_set_sys_divider(1, 1);
            clock_set_apb_divider(2, 2);

            // 切换到PLL
            clock_set_sysclk_source(SYSCLK_SRC_PLL1);

            printf("系统时钟切换完成\n\n");
            print_clock_info();
        }
        else
        {
            printf("PLL配置失败\n");
        }
    }
    else
    {
        printf("XTH启动失败（可能未连接晶振）\n");
        printf("降级到RCH模式\n");
        clock_init_default();
    }

    print_separator();

    // 清理宏定义
#undef XTAL_FREQ_HZ
#undef XTAL_FREQ_STR
#undef PLL_N_DIV
#undef PLL_F_MUL
}

/**
 * @brief 场景3：时钟查询
 */
void scenario_3_clock_query(void)
{
    print_separator();
    printf("=== 场景3：时钟查询 ===\n\n");

    print_clock_info();

    printf("\n定时器时钟：\n");
    printf("  TIM1:  %u Hz\n", clock_get_timer_hz(TIM_1));
    printf("  TIM2:  %u Hz\n", clock_get_timer_hz(TIM_2));
    printf("  TIM6:  %u Hz\n", clock_get_timer_hz(TIM_6));

    printf("\n时钟源校准值：\n");
    printf("  RCH TRIM: %u\n", clock_rch_get_trim());
    printf("  RCL TRIM: %u\n", clock_rcl_get_trim());

    print_separator();
}

/**
 * @brief 场景4：外设时钟控制
 */
void scenario_4_periph_clock(void)
{
    print_separator();
    printf("=== 场景4：外设时钟控制 ===\n\n");

    printf("使能GPIO时钟...\n");
    clock_periph_enable(CLK_GPIOA);
    clock_periph_enable(CLK_GPIOB);
    clock_periph_enable(CLK_GPIOC);
    printf("  GPIOA: %s\n", clock_periph_is_enabled(CLK_GPIOA) ? "已使能" : "已禁用");
    printf("  GPIOB: %s\n", clock_periph_is_enabled(CLK_GPIOB) ? "已使能" : "已禁用");
    printf("  GPIOC: %s\n", clock_periph_is_enabled(CLK_GPIOC) ? "已使能" : "已禁用");

    printf("\n批量使能外设时钟...\n");
    const clock_periph_t periphs[] = {
        CLK_UART1, CLK_UART2, CLK_SPI1, CLK_I2C1};
    clock_periph_enable_multi(periphs, 4);
    printf("  UART1, UART2, SPI1, I2C1 时钟已使能\n");

    printf("\n禁用GPIOC时钟...\n");
    clock_periph_disable(CLK_GPIOC);
    printf("  GPIOC: %s\n", clock_periph_is_enabled(CLK_GPIOC) ? "已使能" : "已禁用");

    print_separator();
}

/**
 * @brief 场景5：时钟源校准
 */
void scenario_5_clock_trim(void)
{
    print_separator();
    printf("=== 场景5：时钟源校准 ===\n\n");

    printf("当前RCH校准值: %u\n", clock_rch_get_trim());
    printf("当前RCL校准值: %u\n", clock_rcl_get_trim());

    printf("\n微调RCH频率（+5）...\n");
    uint8_t old_trim = clock_rch_get_trim();
    clock_rch_set_trim(old_trim + 5);
    printf("新的校准值: %u\n", clock_rch_get_trim());

    printf("\n恢复原始校准值...\n");
    clock_rch_set_trim(old_trim);
    printf("恢复到: %u\n", clock_rch_get_trim());

    printf("\n应用场景：\n");
    printf("  - 温度补偿（-40°C ~ 85°C）\n");
    printf("  - 电压补偿（2.7V ~ 3.6V）\n");
    printf("  - 提高RC振荡器精度\n");

    print_separator();
}

/**
 * @brief 场景6：系统时钟切换
 */
void scenario_6_clock_switch(void)
{
    print_separator();
    printf("=== 场景6：系统时钟切换 ===\n\n");

    printf("当前时钟源: ");
    sysclk_src_t src = clock_get_sysclk_source();
    const char *src_name[] = {"RCH", "未知", "XTH", "PLL1"};
    printf("%s\n", src_name[src]);
    printf("当前频率: %u Hz\n\n", clock_get_hz());

    printf("切换到RCH（64MHz）...\n");
    clock_set_sysclk_source(SYSCLK_SRC_RCH);
    printf("  新频率: %u Hz\n", clock_get_hz());
    delay_ms(10);

    printf("\n切换回PLL1...\n");
    clock_set_sysclk_source(SYSCLK_SRC_PLL1);
    printf("  新频率: %u Hz\n", clock_get_hz());

    printf("\n时钟切换完成\n");
    print_separator();
}

/**
 * @brief 场景7：异步XTH启动
 */
void scenario_7_async_xth(void)
{
    print_separator();
    printf("=== 场景7：异步XTH启动 ===\n\n");

    xth_ready = false;
    uint64_t start_time = system_get_tick();

    printf("启动XTH（异步模式）...\n");

    // 使能时钟就绪中断
    NVIC_EnableIRQ(CLKRDY_INT_IRQn);
    NVIC_SetPriority(CLKRDY_INT_IRQn, 3);

    // 注册中断回调并启动XTH
    clock_xth_ready_irq_enable(xth_ready_callback);
#ifdef EXTERNAL_XTAL_8MHZ
    clock_xth_enable(8000000, false);
#elif defined(EXTERNAL_XTAL_12MHZ)
    clock_xth_enable(12000000, false);
#else
#error "请定义 EXTERNAL_XTAL_8MHZ 或 EXTERNAL_XTAL_12MHZ"
#endif

    printf("XTH启动中，CPU可以继续工作...\n");

    // CPU继续执行其他任务
    for (int i = 0; i < 5 && !xth_ready; i++)
    {
        printf("  执行其他任务... (%d)\n", i + 1);
        delay_ms(2);
    }

    // 等待XTH就绪
    while (!xth_ready)
    {
        __WFI(); // 进入休眠等待中断
    }

    uint32_t elapsed = (uint32_t)(system_get_tick() - start_time);
    printf("  耗时: %u ms\n", elapsed);

    clock_xth_ready_irq_disable();
    printf("\nXTH启动完成，可以使用了\n");

    print_separator();
}

/**
 * @brief 场景8：PLL锁定中断
 */
void scenario_8_async_pll(void)
{
    print_separator();
    printf("=== 场景8：PLL锁定中断 ===\n\n");

    pll1_locked = false;
    uint64_t start_time = system_get_tick();

    printf("配置PLL1（异步模式）...\n");

    // 使能时钟就绪中断
    NVIC_EnableIRQ(CLKRDY_INT_IRQn);
    NVIC_SetPriority(CLKRDY_INT_IRQn, 3);

    // 注册中断回调
    clock_pll1_lock_irq_enable(pll1_lock_callback);

    // 配置PLL（不等待锁定）
    pll_config_t pll = {
        .src = 0, // RCH/16
        .n = 2,
        .f = 180,
        .p = 0,
        .q = 2,
    };
    clock_pll1_configure(&pll);

    printf("PLL配置中，CPU可以继续工作...\n");

    // 等待PLL锁定
    while (!pll1_locked)
    {
        //        __WFI();
    }

    uint32_t elapsed = (uint32_t)(system_get_tick() - start_time);
    printf("  锁定耗时: %u ms\n", elapsed);

    clock_pll1_lock_irq_disable();

    printf("\nPLL已锁定，可以切换系统时钟了\n");
    clock_set_sysclk_source(SYSCLK_SRC_PLL1);

    print_separator();
}

/**
 * @brief 场景9：CSS停振检测
 */
void scenario_9_css_detection(void)
{
#ifdef EXTERNAL_XTAL_8MHZ
    const uint32_t xtal_freq = 8000000;
    const uint8_t  pll_n_div = 4;
#elif defined(EXTERNAL_XTAL_12MHZ)
    const uint32_t xtal_freq = 12000000;
    const uint8_t  pll_n_div = 6;
#else
#error "请定义 EXTERNAL_XTAL_8MHZ 或 EXTERNAL_XTAL_12MHZ"
#endif

    print_separator();
    printf("=== 场景9：CSS停振检测 ===\n\n");

    printf("注意：此场景需要XTH晶振支持\n\n");

    // 先切换到XTH+PLL模式
    printf("初始化XTH+PLL模式...\n");
    if (clock_xth_enable(xtal_freq, false)) {
        pll_config_t pll = {.src = 1, .n = pll_n_div, .f = 180, .p = 0, .q = 2};
        clock_pll1_configure(&pll);
        clock_set_sys_divider(1, 1);
        clock_set_apb_divider(2, 2);
        clock_set_sysclk_source(SYSCLK_SRC_PLL1);

        printf("系统时钟: PLL1 (基于XTH)\n\n");

        // 使能CSS (XTH停振通过NMI触发，无需NVIC配置)
        printf("启用XTH停振检测...\n");
        clock_set_xth_css_callback(xth_css_callback);
        clock_xth_css_enable();
        printf("CSS已使能，系统受保护\n\n");

        printf("[警告] 模拟XTH停振...\n");
        printf("（实际应用中，这会在晶振故障时自动触发）\n\n");

        // 注意：在真实硬件上，需要物理断开晶振才能触发CSS
        printf("如果XTH真的停振，系统会：\n");
        printf("  1. 自动切换到RCH\n");
        printf("  2. 禁用PLL\n");
        printf("  3. 触发中断回调\n");
        printf("  4. 生成NMI（如果配置）\n");

        // 禁用CSS
        delay_ms(1000);
        clock_xth_css_disable();
        printf("\nCSS已禁用\n");
    }
    else
    {
        printf("XTH启动失败（未连接晶振）\n");
        printf("无法演示CSS功能\n");
    }

    print_separator();
}

/**
 * @brief 场景10：时钟源输出
 */
void scenario_10_clock_output(void)
{
    print_separator();
    printf("=== 场景10：时钟源输出 ===\n\n");

    printf("当前时钟源: ");
    sysclk_src_t src = clock_get_sysclk_source();
    const char *src_name[] = {"RCH", "未知", "XTH", "PLL1"};
    printf("%s\n", src_name[src]);

    printf("当前频率: %u Hz\n\n", clock_get_hz());

    printf("配置MCO1时钟输出...\n");
    // 将系统时钟源转换为MCO时钟源
    mco_source_t mco_src;
    switch (src)
    {
    case SYSCLK_SRC_RCH:
        mco_src = MCO_SRC_RCH;
        break;
    case SYSCLK_SRC_XTH:
        mco_src = MCO_SRC_XTH;
        break;
    case SYSCLK_SRC_PLL1:
        mco_src = MCO_SRC_PLL1PCLK;
        break;
    default:
        mco_src = MCO_SRC_RCH;
        break;
    }
    clock_mco1_configure(mco_src, 220);
    printf("MCO1时钟输出配置完成\n");
}

//===========================================
// 主函数
//===========================================
uint8_t debug_flag = 0;
int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    // 打印欢迎信息
    printf("\n\n");
    print_separator();
    printf("  ACM32P4 时钟系统示例程序\n");
    printf("  SDK Version: 1.0\n");
    print_separator();
    printf("\n");

    // 自动运行默认场景
    printf("自动运行模式（场景 %d）\n", DEFAULT_SCENARIO);

    switch (DEFAULT_SCENARIO)
    {
    case 1:
        scenario_1_default_init();
        break;
    case 2:
        scenario_2_external_xtal();
        break;
    case 3:
        scenario_3_clock_query();
        break;
    case 4:
        scenario_4_periph_clock();
        break;
    case 5:
        scenario_5_clock_trim();
        break;
    case 6:
        scenario_6_clock_switch();
        break;
    case 7:
        scenario_7_async_xth();
        break;
    case 8:
        scenario_8_async_pll();
        break;	
    case 9:
        scenario_9_css_detection();
        break;
    case 10:
        scenario_10_clock_output();
        break;
    default:
        scenario_1_default_init();
        break;
    }

    printf("\n示例运行完成！\n");
    printf("进入主循环...\n\n");

    // 主循环
    while (1)
    {
        // 简单的心跳指示
        delay_ms(1000);
        printf(".");
    }

    return 0;
}

//===========================================
// 中断处理器（需要在启动文件中注册）
//===========================================

/**
 * @brief 中断架构说明
 *
 * 1. CLKRDY_INT_IRQHandler() - CLKRDY_INT_IRQn (3)
 *    - 处理：时钟就绪中断（RCL/XTL/RCH/XTH ready）
 *    - 处理：PLL锁定中断（PLL1/2/3 lock）
 *    - 需要：NVIC_EnableIRQ(CLKRDY_INT_IRQn)
 *
 * 2. NMI_Handler() - NMI (-14)
 *    - 处理：XTH停振检测（CSS - Clock Security System）
 *    - 无需：不可屏蔽中断，无需NVIC配置
 *
 * 3. RTC_XTLSD_IRQHandler() - RTC_XTLSD_IRQn (2)
 *    - 处理：XTL停振检测
 *    - 需要：NVIC_EnableIRQ(RTC_XTLSD_IRQn)
 *
 * @note 所有中断处理函数已在 clocks.c 中实现
 */
