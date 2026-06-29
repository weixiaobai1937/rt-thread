/**
 * @file exti_example.c
 * @brief ACM32P4 EXTI 驱动示例
 *
 * @details
 * 演示8个场景：
 * 1. 单个按键中断（最简场景）
 * 2. 多个按键中断
 * 3. 双沿触发（检测按下和释放）
 * 4. 高级配置（使用exti_configure）
 * 5. 事件模式（低功耗唤醒）
 * 6. 软件触发（测试用）
 * 7. 动态使能/禁用
 * 8. 内部模块中断（RTC示例）
 */

#include "acm32p4.h"
#include "hardware/system.h"
#include "hardware/exti.h"
#include "hardware/gpio.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

// 选择要运行的场景（修改此值来切换场景）
#define DEFAULT_SCENARIO 1

//===========================================
// 全局变量
//===========================================

// ============================================================================
// 工具函数
// ============================================================================

/**
 * @brief 打印分隔线
 */
static void print_separator(void)
{
    printf("==========================================\n");
}

// ============================================================================
// 场景1：单个按键中断（最简场景）
// ============================================================================

// 按键状态计数器
static volatile uint32_t button_press_count = 0;

/**
 * @brief 按键中断回调函数
 */
static void button_handler(exti_line_t line)
{
    button_press_count++;
    printf("Button pressed! Count: %lu (Line: %u)\n", button_press_count, line);
}

/**
 * @brief 场景1: 单个按键中断
 * 
 * 硬件连接:
 * - PA5: 按键 (下拉, 按下时高电平)
 */
static void scenario_1_simple_button(void)
{
    print_separator();
    printf("=== 场景1：单个按键中断 ===\n\n");
    
    // 初始化GPIO引脚
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_IN);
    gpio_pull_down(PA5);  // 下拉
    
    // 配置EXTI中断 (上升沿触发)
    exti_gpio_init(EXTI_GPIO_PORTA, 5, EXTI_TRIGGER_RISING);
    exti_set_callback(EXTI_LINE_5, button_handler);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    // 打印配置信息
    printf("PA5配置成功：\n");
    printf("  模式：输入\n");
    printf("  上拉/下拉：下拉\n");
    printf("  EXTI线：5\n");
    printf("  触发方式：上升沿\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
    
    printf("按下按键触发中断...\n");
    printf("(运行30秒后退出)\n\n");
    
    // 运行30秒
    for (int i = 0; i < 30; i++) {
        delay_ms(1000);
        if (i % 5 == 0 && i > 0) {
            printf("[%d秒] 当前计数: %lu\n", i, button_press_count);
        }
    }
}

// ============================================================================
// 场景2：多个按键中断
// ============================================================================

static volatile uint32_t button1_count = 0;
static volatile uint32_t button2_count = 0;

static void button1_handler(exti_line_t line)
{
    button1_count++;
    printf("按键1按下! Count: %lu\n", button1_count);
}

static void button2_handler(exti_line_t line)
{
    button2_count++;
    printf("按键2按下! Count: %lu\n", button2_count);
}

/**
 * @brief 场景2: 多个按键中断
 * 
 * 硬件连接:
 * - PA5:  按键1 (上拉, 按下时低电平)
 * - PB12: 按键2 (上拉, 按下时低电平)
 */
static void scenario_2_multiple_buttons(void)
{
    print_separator();
    printf("=== 场景2：多个按键中断 ===\n\n");
    
    // 配置按键1 (PA5)
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_IN);
    gpio_pull_up(PA5);
    exti_gpio_init(EXTI_GPIO_PORTA, 5, EXTI_TRIGGER_FALLING);
    exti_set_callback(EXTI_LINE_5, button1_handler);
    
    // 配置按键2 (PB12)
    gpio_init(PB12);
    gpio_set_dir(PB12, GPIO_IN);
    gpio_pull_up(PB12);
    exti_gpio_init(EXTI_GPIO_PORTB, 12, EXTI_TRIGGER_FALLING);
    exti_set_callback(EXTI_LINE_12, button2_handler);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI9_5_IRQn, 2);   // 按键1 (EXTI5)
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    NVIC_SetPriority(EXTI15_10_IRQn, 2); // 按键2 (EXTI12)
    NVIC_EnableIRQ(EXTI15_10_IRQn);

    // 打印配置信息
    printf("按键1 (PA5) 配置：\n");
    printf("  EXTI线：5\n");
    printf("  触发：下降沿\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
    
    printf("按键2 (PB12) 配置：\n");
    printf("  EXTI线：12\n");
    printf("  触发：下降沿\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_12) ? "已使能" : "未使能");
    
    printf("按下按键触发中断...\n");
    printf("(运行30秒后退出)\n\n");
    
    // 运行30秒
    for (int i = 0; i < 30; i++) {
        delay_ms(1000);
        if (i % 5 == 0 && i > 0) {
            printf("[%d秒] 按键1: %lu, 按键2: %lu\n", i, button1_count, button2_count);
        }
    }
}

// ============================================================================
// 场景3：双沿触发（检测按键按下和释放）
// ============================================================================

static void button_edge_handler(exti_line_t line)
{
    // 读取当前引脚电平
    bool level = gpio_get(PA5);

    if (level) {
        printf("按键 按下 (上升沿)\n");
    } else {
        printf("按键 释放 (下降沿)\n");
    }
}

/**
 * @brief 场景3: 双沿触发
 * 
 * 硬件连接:
 * - PA5: 按键 (下拉)
 */
static void scenario_3_both_edges(void)
{
    print_separator();
    printf("=== 场景3：双沿触发 ===\n\n");
    
    // 配置GPIO
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_IN);
    gpio_pull_down(PA5);
    
    // 配置双沿触发
    exti_gpio_init(EXTI_GPIO_PORTA, 5, EXTI_TRIGGER_BOTH);
    exti_set_callback(EXTI_LINE_5, button_edge_handler);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    // 打印配置信息
    printf("PA5配置成功：\n");
    printf("  EXTI线：5\n");
    printf("  触发方式：双沿 (上升+下降)\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
    
    printf("按下和释放按键都会触发中断\n");
    printf("(运行30秒后退出)\n\n");
    
    // 运行30秒
    for (int i = 0; i < 30; i++) {
        delay_ms(1000);
    }
}

// ============================================================================
// 场景4：高级配置（使用exti_configure）
// ============================================================================

static void advanced_handler(exti_line_t line)
{
    printf("高级配置中断触发 (线: %u)\n", line);
}

/**
 * @brief 场景4: 高级配置
 */
static void scenario_4_advanced_config(void)
{
    print_separator();
    printf("=== 场景4：高级配置 ===\n\n");
    
    // 配置GPIO
    gpio_init(PC8);
    gpio_set_dir(PC8, GPIO_IN);
    gpio_pull_up(PC8);
    
    // 使用配置结构体
    exti_config_t config = {
        .line      = EXTI_LINE_8,
        .trigger   = EXTI_TRIGGER_FALLING,
        .mode      = EXTI_MODE_INTERRUPT,
        .gpio_port = EXTI_GPIO_PORTC,
        .callback  = advanced_handler,
        .enabled   = true
    };
    
    bool result = exti_configure(&config);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    // 打印配置信息
    printf("PC8 (EXTI线8) 配置结果：\n");
    printf("  配置状态：%s\n", result ? "成功" : "失败");
    printf("  GPIO端口：PORTC\n");
    printf("  触发方式：下降沿\n");
    printf("  工作模式：中断模式\n");
    printf("  回调函数：%s\n", config.callback ? "已设置" : "未设置");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_8) ? "已使能" : "未使能");
    
    if (!result) {
        printf("配置失败!\n");
        return;
    }
    
    printf("按下按键触发中断...\n");
    printf("(运行20秒后退出)\n\n");
    
    // 运行20秒
    for (int i = 0; i < 20; i++) {
        delay_ms(1000);
    }
}

// ============================================================================
// 场景5：事件模式（低功耗唤醒）
// ============================================================================

/**
 * @brief 场景5: 事件模式
 * 
 * 事件模式不产生中断,只唤醒CPU
 */
static void scenario_5_event_mode(void)
{
    print_separator();
    printf("=== 场景5：事件模式 ===\n\n");
    
    // 配置GPIO
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_IN);
    gpio_pull_down(PA5);
    
    // 配置为事件模式
    exti_config_t config = {
        .line      = EXTI_LINE_5,
        .trigger   = EXTI_TRIGGER_RISING,
        .mode      = EXTI_MODE_EVENT,       // 事件模式
        .gpio_port = EXTI_GPIO_PORTA,
        .callback  = NULL,                  // 事件模式不需要回调
        .enabled   = true
    };
    
    bool result = exti_configure(&config);
    
    // 打印配置信息
    printf("PA5 (EXTI线5) 配置结果：\n");
    printf("  配置状态：%s\n", result ? "成功" : "失败");
    printf("  工作模式：事件模式 (无中断)\n");
    printf("  触发方式：上升沿\n");
    printf("  用途：低功耗唤醒\n\n");
    
    if (!result) {
        printf("配置失败!\n");
        return;
    }
    
    printf("进入WFE循环 (按下按键唤醒CPU)\n");
    printf("(运行10次后退出)\n\n");
    
    uint32_t wakeup_count = 0;
    for (int i = 0; i < 10; i++) {
        printf("[%d] 进入WFE...\n", i + 1);
        __WFE();  // 等待事件唤醒
        wakeup_count++;
        printf("    唤醒! 总计: %lu次\n\n", wakeup_count);
        delay_ms(500);
    }
    
    printf("事件模式测试完成\n");
}

// ============================================================================
// 场景6：软件触发（测试用）
// ============================================================================

static volatile uint32_t sw_trigger_count = 0;

static void software_trigger_handler(exti_line_t line)
{
    sw_trigger_count++;
    printf("  [回调] 软件触发中断 (线: %u, 计数: %lu)\n", line, sw_trigger_count);
}

/**
 * @brief 场景6: 软件触发
 */
static void scenario_6_software_trigger(void)
{
    print_separator();
    printf("=== 场景6：软件触发 ===\n\n");
    
    // 配置EXTI (不连接GPIO)
    exti_set_trigger(EXTI_LINE_10, EXTI_TRIGGER_RISING);
    exti_set_mode(EXTI_LINE_10, EXTI_MODE_INTERRUPT);
    exti_set_callback(EXTI_LINE_10, software_trigger_handler);
    exti_enable(EXTI_LINE_10);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI15_10_IRQn, 2);
    NVIC_EnableIRQ(EXTI15_10_IRQn);
    
    // 打印配置信息
    printf("EXTI线10配置：\n");
    printf("  用途：软件触发测试\n");
    printf("  触发方式：上升沿\n");
    printf("  模式：中断模式\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_10) ? "已使能" : "未使能");
    
    printf("开始软件触发测试 (5次)...\n\n");
    
    // 定时软件触发
    for (uint32_t i = 0; i < 5; i++) {
        printf("[%lu] 触发软件中断...\n", i + 1);
        exti_trigger_software(EXTI_LINE_10);
        
        // 等待中断处理
        delay_ms(100);
        
        // 检查挂起标志
        if (exti_get_pending(EXTI_LINE_10)) {
            printf("  警告: 挂起标志未清除!\n");
            exti_clear_pending(EXTI_LINE_10);
        }
        
        delay_ms(500);
    }
    
    printf("\n软件触发测试完成\n");
    printf("总触发次数: %lu\n", sw_trigger_count);
}

// ============================================================================
// 场景7：动态使能/禁用
// ============================================================================

static void dynamic_handler(exti_line_t line)
{
    static uint32_t count = 0;
    count++;
    printf("中断触发! 计数: %lu\n", count);
    
    // 触发3次后自动禁用
    if (count >= 3) {
        exti_disable(EXTI_LINE_5);
        printf(">>> EXTI已自动禁用 (触发3次后) <<<\n\n");
    }
}

/**
 * @brief 场景7: 动态使能/禁用
 */
static void scenario_7_dynamic_control(void)
{
    print_separator();
    printf("=== 场景7：动态使能/禁用 ===\n\n");
    
    // 配置GPIO
    gpio_init(PA5);
    gpio_set_dir(PA5, GPIO_IN);
    gpio_pull_down(PA5);
    
    // 配置EXTI但初始禁用
    exti_gpio_init(EXTI_GPIO_PORTA, 5, EXTI_TRIGGER_RISING);
    exti_set_callback(EXTI_LINE_5, dynamic_handler);
    exti_disable(EXTI_LINE_5);
    
    // 配置NVIC中断
    NVIC_SetPriority(EXTI9_5_IRQn, 2);
    NVIC_EnableIRQ(EXTI9_5_IRQn);
    
    // 打印配置信息
    printf("PA5 (EXTI线5) 配置：\n");
    printf("  初始状态：禁用\n");
    printf("  触发方式：上升沿\n");
    printf("  自动禁用：触发3次后\n\n");
    
    printf("当前状态: %s\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
    
    // 延时2秒
    printf("等待2秒后使能...\n\n");
    delay_ms(2000);
    
    // 使能EXTI
    exti_enable(EXTI_LINE_5);
    printf(">>> EXTI已使能 <<<\n");
    printf("当前状态: %s\n\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
    
    printf("按下按键触发中断 (将在3次后自动禁用)\n\n");
    
    // 等待自动禁用
    for (int i = 0; i < 30; i++) {
        if (!exti_is_enabled(EXTI_LINE_5)) {
            printf("检测到EXTI已禁用\n");
            printf("当前状态: %s\n\n", exti_is_enabled(EXTI_LINE_5) ? "已使能" : "未使能");
            break;
        }
        delay_ms(1000);
    }
    
    printf("动态控制测试完成\n");
}

// ============================================================================
// 场景8：内部模块中断（RTC唤醒示例框架）
// ============================================================================

static volatile uint32_t rtc_wakeup_count = 0;

static void rtc_wakeup_handler(exti_line_t line)
{
    rtc_wakeup_count++;
    printf("RTC唤醒中断! 计数: %lu\n", rtc_wakeup_count);
}

/**
 * @brief 场景8: 内部模块中断
 * 
 * @note 此示例仅演示EXTI配置,实际使用需要先配置RTC模块
 */
static void scenario_8_internal_interrupt(void)
{
    print_separator();
    printf("=== 场景8：内部模块中断 (RTC) ===\n\n");
    
    // 配置RTC EXTI中断
    exti_internal_init(EXTI_LINE_RTC, EXTI_TRIGGER_RISING);
    exti_set_callback(EXTI_LINE_RTC, rtc_wakeup_handler);

    // 配置NVIC中断（RTC使用独立向量 RTC_XTLSD_IRQn）
    NVIC_SetPriority(RTC_XTLSD_IRQn, 2);
    NVIC_EnableIRQ(RTC_XTLSD_IRQn);
    
    // 打印配置信息
    printf("RTC EXTI配置：\n");
    printf("  EXTI线：17 (RTC中断/XTL停振)\n");
    printf("  触发方式：上升沿\n");
    printf("  工作模式：中断模式\n");
    printf("  状态：%s\n\n", exti_is_enabled(EXTI_LINE_RTC) ? "已使能" : "未使能");

    printf("注意: 需要先配置RTC模块才能产生中断\n");
    printf("此示例仅演示EXTI侧的配置\n\n");

    // 列出其他可用的内部模块线
    printf("其他可用的内部模块EXTI线：\n");
    printf("  线16: LVD (低电压检测)\n");
    printf("  线17: RTC中断/XTL停振\n");
    printf("  线18: IWDT (看门狗)\n");
    printf("  线19: COMP (比较器)\n");
    printf("  线20: USB 唤醒\n");
    printf("  线21: LPTIM1 唤醒\n");
    printf("  线22: LPUART1 唤醒\n");
    printf("  线23: LPUART2 唤醒\n");
    printf("  线24: 以太网唤醒\n\n");
    
    printf("等待RTC中断...\n");
    printf("(10秒后退出)\n\n");
    
    // 等待10秒
    for (int i = 0; i < 10; i++) {
        delay_ms(1000);
        if (rtc_wakeup_count > 0) {
            printf("[%d秒] RTC唤醒计数: %lu\n", i + 1, rtc_wakeup_count);
        }
    }
    
    printf("内部模块中断示例完成\n");
}

// ============================================================================
// 主函数
// ============================================================================

int main(void)
{
    system_init();
    SEGGER_RTT_Init();
    
    // 1. 打印欢迎信息
    printf("\n\n");
    print_separator();
    printf("  ACM32P4 EXTI 示例程序\n");
    printf("  SDK Version: 1.0\n");
    print_separator();
    printf("\n");
    
    // 2. 根据选择运行场景
    switch (DEFAULT_SCENARIO) {
    case 1:
        scenario_1_simple_button();
        break;
    case 2:
        scenario_2_multiple_buttons();
        break;
    case 3:
        scenario_3_both_edges();
        break;
    case 4:
        scenario_4_advanced_config();
        break;
    case 5:
        scenario_5_event_mode();
        break;
    case 6:
        scenario_6_software_trigger();
        break;
    case 7:
        scenario_7_dynamic_control();
        break;
    case 8:
        scenario_8_internal_interrupt();
        break;
    default:
        scenario_1_simple_button();
        break;
    }
    
    // 3. 主循环
    printf("\n");
    print_separator();
    printf("场景执行完成\n");
    printf("进入主循环...\n");
    print_separator();
    printf("\n");
    
    while (1) {
        // LED闪烁指示程序运行
        gpio_toggle(PA0);
        delay_ms(1000);
    }
    
    return 0;
}
