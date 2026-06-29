/**
 * @file comp_example.c
 * @brief COMP模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基本比较器（PB0 vs PB1）
 * 2. 迟滞比较器
 * 3. 滤波输出
 * 4. VREF内部分压
 * 5. 极性反转
 * 6. 结构体配置方式
 * 7. 中断与唤醒
 * 8. 综合应用：消隐 + 滤波 + 迟滞
 *
 * @note 使用前需配置对应 GPIO 为模拟功能
 * @note 负端选择 DAC1 时需先使能 DAC
 * @note 滤波时钟由 RCC_CCR2.FLTCLK_SEL 选择
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

static volatile uint32_t comp_irq_count = 0;
static volatile bool comp_triggered = false;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief COMP 示例初始化：使能 COMP 时钟
 */
static void comp_example_init(void)
{
    // 使能 COMP1 时钟（APB2, bit 2）
    clock_periph_enable(CLK_CMP1);
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

/**
 * @brief 打印当前 COMP 完整状态
 */
static void print_comp_status(void)
{
    printf("  ┌──────────────────────────────┐\n");
    printf("  │ COMP 状态                    │\n");
    printf("  ├──────────────────────────────┤\n");
    printf("  │ 使能:        %-14s │\n", comp_is_enabled() ? "是" : "否");
    printf("  │ 锁定:        %-14s │\n", comp_is_locked() ? "是" : "否");
    printf("  │ 正端输入:    %-14s │\n",
           comp_get_positive_input() == COMP_INP_PB0 ? "PB0" : "PB2");
    printf("  │ 负端输入:    %-14s │\n",
           comp_get_negative_input() == COMP_INM_DAC1    ? "DAC1" :
           comp_get_negative_input() == COMP_INM_PB1     ? "PB1" :
           comp_get_negative_input() == COMP_INM_PC4     ? "PC4" : "VREF分压");
    printf("  │ 极性:        %-14s │\n",
           comp_get_polarity() == COMP_POL_NORMAL ? "同相" : "反相");
    printf("  │ 迟滞:        %-14d │\n", (int)comp_get_hysteresis());
    printf("  │ 滤波输出:    %-14s │\n", comp_get_output() ? "高" : "低");
    printf("  │ 原始输出:    %-14s │\n", comp_get_output_raw() ? "高" : "低");
    printf("  └──────────────────────────────┘\n");
}

//===========================================
// COMP 中断回调
//===========================================

static void comp_isr_callback(void)
{
    comp_irq_count++;
    comp_triggered = true;
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基本比较器
 *
 * 演示 COMP 的最基本使用：比较两个模拟输入电压。
 */
void scenario_1_basic_comparator(void)
{
    print_demo_header("场景1：基本比较器（PB0 vs PB1）");

    printf("步骤1：初始化基本比较器（正端 PB0, 负端 PB1）\n");
    printf("  说明: GPIO PB0/PB1 需配置为模拟功能\n");
    comp_init_basic(COMP_INP_PB0, COMP_INM_PB1);
    printf("  结果: 比较器已使能\n");

    printf("\n步骤2：连续读取比较器输出 10 次（间隔 500ms）\n");
    for (int i = 1; i <= 10; i++) {
        delay_ms(500);
        bool out = comp_get_output();
        bool raw = comp_get_output_raw();
        printf("  第 %d 次: VCOUT1=%d, VCOUT1_ORG=%d %s\n",
               i, out, raw,
               out ? "(正端 > 负端)" : "(正端 < 负端)");
    }

    printf("\n步骤3：查看完整状态\n");
    print_comp_status();

    printf("\n步骤4：禁止比较器\n");
    comp_disable();
    printf("  结果: COMP 已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景2：迟滞比较器
 *
 * 演示迟滞比较功能：防止输入电压接近时输出振荡。
 */
void scenario_2_hysteresis(void)
{
    print_demo_header("场景2：迟滞比较器");

    printf("步骤1：初始化迟滞比较器（20mV 迟滞窗口）\n");
    comp_init_hysteresis(COMP_INP_PB0, COMP_INM_PB1, COMP_HYS_20MV);
    printf("  结果: 迟滞比较器已使能，迟滞窗口 20mV\n");

    printf("\n步骤2：验证迟滞配置\n");
    comp_hysteresis_t hys = comp_get_hysteresis();
    printf("  当前迟滞设置: ");
    switch (hys) {
        case COMP_HYS_DISABLE: printf("禁止\n"); break;
        case COMP_HYS_10MV:    printf("10mV\n"); break;
        case COMP_HYS_20MV:    printf("20mV\n"); break;
        case COMP_HYS_30MV:    printf("30mV\n"); break;
        case COMP_HYS_40MV:    printf("40mV\n"); break;
        default:               printf("未知\n"); break;
    }

    printf("\n步骤3：切换不同迟滞窗口\n");
    comp_hysteresis_t hys_values[] = {
        COMP_HYS_10MV, COMP_HYS_20MV, COMP_HYS_30MV, COMP_HYS_40MV
    };
    const char *hys_names[] = {"10mV", "20mV", "30mV", "40mV"};

    for (int i = 0; i < 4; i++) {
        comp_set_hysteresis(hys_values[i]);
        delay_ms(100);
        printf("  迟滞 %s: VCOUT1=%d\n",
               hys_names[i], comp_get_output());
    }

    printf("\n步骤4：禁止迟滞\n");
    comp_set_hysteresis(COMP_HYS_DISABLE);
    printf("  结果: 迟滞已禁止\n");

    comp_disable();
    print_demo_end();
}

/**
 * @brief 场景3：滤波输出
 *
 * 演示比较器数字滤波功能：滤除输出尖峰毛刺。
 */
void scenario_3_filtered_output(void)
{
    print_demo_header("场景3：滤波输出");

    printf("步骤1：初始化滤波比较器（64 个滤波时钟周期）\n");
    printf("  说明: 滤波时钟由 RCC_CCR2.FLTCLK_SEL 选择\n");
    comp_init_filtered(COMP_INP_PB0, COMP_INM_PB1, COMP_FLT_64CYCLES);
    printf("  结果: 滤波比较器已使能\n");

    printf("\n步骤2：验证滤波配置\n");
    comp_filter_time_t ft;
    bool flt_en = comp_get_filter(&ft);
    printf("  滤波使能: %s\n", flt_en ? "是" : "否");
    printf("  滤波周期数: ");
    switch (ft) {
        case COMP_FLT_1CYCLE:    printf("1\n");    break;
        case COMP_FLT_2CYCLES:   printf("2\n");    break;
        case COMP_FLT_4CYCLES:   printf("4\n");    break;
        case COMP_FLT_16CYCLES:  printf("16\n");   break;
        case COMP_FLT_64CYCLES:  printf("64\n");   break;
        case COMP_FLT_256CYCLES: printf("256\n");  break;
        case COMP_FLT_1024CYCLES: printf("1024\n"); break;
        case COMP_FLT_4095CYCLES: printf("4095\n"); break;
        default:                  printf("未知\n"); break;
    }

    printf("\n步骤3：测试不同滤波时间\n");
    comp_filter_time_t ft_values[] = {
        COMP_FLT_1CYCLE, COMP_FLT_16CYCLES, COMP_FLT_256CYCLES, COMP_FLT_4095CYCLES
    };
    const char *ft_names[] = {"1", "16", "256", "4095"};

    for (int i = 0; i < 4; i++) {
        comp_set_filter(true, ft_values[i]);
        delay_ms(50);
        printf("  滤波 %s 周期: VCOUT1=%d, VCOUT1_ORG=%d\n",
               ft_names[i], comp_get_output(), comp_get_output_raw());
    }

    printf("\n步骤4：禁止滤波\n");
    comp_set_filter(false, COMP_FLT_1CYCLE);
    printf("  结果: 滤波已禁止\n");

    comp_disable();
    print_demo_end();
}

/**
 * @brief 场景4：VREF内部分压
 *
 * 演示使用 VREF/AVDD 可编程分压作为比较器负端输入。
 */
void scenario_4_vref_divider(void)
{
    print_demo_header("场景4：VREF内部分压");

    printf("步骤1：初始化 VREF 分压模式（分压系数 10，VREF=3.3V）\n");
    printf("  负端电压 = (10+1)/20 × VREF = 11/20 × 3.3V ≈ 1.815V\n");
    comp_init_vref_divider(COMP_INP_PB0, COMP_VREF_VREF, 10);
    printf("  结果: 内部分压比较器已使能\n");

    printf("\n步骤2：计算各分压系数对应的负端电压（VREF=3300mV）\n");
    printf("  ┌──────────┬────────────┐\n");
    printf("  │ 分压系数 │ 负端电压    │\n");
    printf("  ├──────────┼────────────┤\n");
    for (uint8_t cfg = 0; cfg <= 15; cfg++) {
        uint32_t mv = comp_calculate_vref_divider_mv(COMP_VREF_VREF, 3300, cfg);
        printf("  │ %-8u │ %-5umV     │\n", cfg, mv);
    }
    printf("  └──────────┴────────────┘\n");

    printf("\n步骤3：动态修改分压系数\n");
    for (uint8_t cfg = 5; cfg <= 15; cfg += 5) {
        comp_set_vref_divider(true, COMP_VREF_VREF, cfg);
        delay_ms(100);
        printf("  分压系数 %u: VCOUT1=%d\n", cfg, comp_get_output());
    }

    printf("\n步骤4：切换到 AVDD 分压源\n");
    comp_set_vref_divider(true, COMP_VREF_AVDD, 8);
    printf("  分压源: AVDD, 分压系数: %u\n", comp_get_vref_cfg());

    comp_disable();
    print_demo_end();
}

/**
 * @brief 场景5：极性反转
 *
 * 演示输出极性配置：同相输出 vs 反相输出。
 */
void scenario_5_polarity_inversion(void)
{
    print_demo_header("场景5：极性反转");

    printf("步骤1：初始化基本比较器（同相输出）\n");
    comp_init_basic(COMP_INP_PB0, COMP_INM_PB1);
    printf("  极性: %s\n",
           comp_get_polarity() == COMP_POL_NORMAL ? "同相" : "反相");

    printf("\n步骤2：读取同相输出 5 次\n");
    for (int i = 1; i <= 5; i++) {
        delay_ms(200);
        printf("  第 %d 次: VCOUT1=%d (INP>INM → 高)\n", i, comp_get_output());
    }

    printf("\n步骤3：切换为反相输出\n");
    comp_set_polarity(COMP_POL_INVERTED);
    printf("  极性: 反相\n");

    printf("\n步骤4：读取反相输出 5 次\n");
    for (int i = 1; i <= 5; i++) {
        delay_ms(200);
        printf("  第 %d 次: VCOUT1=%d (INP>INM → 低)\n", i, comp_get_output());
    }

    printf("\n步骤5：恢复同相输出\n");
    comp_set_polarity(COMP_POL_NORMAL);
    printf("  极性: 同相\n");

    comp_disable();
    print_demo_end();
}

/**
 * @brief 场景6：结构体配置方式
 *
 * 演示使用 comp_config_t 结构体进行完整配置。
 */
void scenario_6_config_struct(void)
{
    print_demo_header("场景6：结构体配置方式");

    printf("步骤1：使用结构体配置 COMP（迟滞 + 滤波）\n");
    comp_config_t cfg = {
        .inp_sel = COMP_INP_PB0,
        .inm_sel = COMP_INM_PB1,
        .polarity = COMP_POL_NORMAL,
        .hysteresis = COMP_HYS_20MV,
        .filter_enable = true,
        .filter_time = COMP_FLT_64CYCLES,
        .blank_sel = COMP_BLANK_NONE,
        .vref_enable = false,
        .vref_sel = COMP_VREF_AVDD,
        .vref_cfg = 0,
        .lock = false,
    };
    if (comp_config(&cfg)) {
        printf("  结果: 配置成功\n");
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤2：查看配置后的状态\n");
    print_comp_status();

    printf("\n步骤3：使用结构体配置 COMP（VREF 分压 + 消隐）\n");
    comp_config_t cfg2 = {
        .inp_sel = COMP_INP_PB2,
        .inm_sel = COMP_INM_VREFDIV,
        .polarity = COMP_POL_INVERTED,
        .hysteresis = COMP_HYS_DISABLE,
        .filter_enable = false,
        .filter_time = COMP_FLT_1CYCLE,
        .blank_sel = COMP_BLANK_TIM1_OC3,
        .vref_enable = true,
        .vref_sel = COMP_VREF_VREF,
        .vref_cfg = 10,
        .lock = false,
    };
    if (comp_config(&cfg2)) {
        printf("  结果: 配置成功\n");
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤4：验证参数校验\n");
    comp_config_t invalid_cfg = {
        .inp_sel = COMP_INP_PB0,
        .inm_sel = COMP_INM_VREFDIV,
        .polarity = COMP_POL_NORMAL,
        .hysteresis = COMP_HYS_DISABLE,
        .filter_enable = false,
        .filter_time = COMP_FLT_1CYCLE,
        .blank_sel = COMP_BLANK_NONE,
        .vref_enable = true,
        .vref_sel = COMP_VREF_VREF,
        .vref_cfg = 20,   // 超出范围（> 15）
        .lock = false,
    };
    if (!comp_config(&invalid_cfg)) {
        printf("  结果: 无效参数被正确拒绝（vref_cfg=20 > 15）\n");
    }

    comp_disable();
    print_demo_end();
}

/**
 * @brief 场景7：中断与唤醒
 *
 * 演示比较器输出触发中断，以及配置为 EXTI 唤醒源。
 */
void scenario_7_interrupt_wakeup(void)
{
    print_demo_header("场景7：中断与唤醒");

    printf("步骤1：初始化基本比较器\n");
    comp_init_basic(COMP_INP_PB0, COMP_INM_PB1);
    printf("  结果: 比较器已使能\n");

    printf("\n步骤2：注册中断回调并使能 NVIC\n");
    comp_register_callback(comp_isr_callback);
    comp_irq_count = 0;
    comp_triggered = false;
    NVIC_EnableIRQ(COMP1_IRQn);
    printf("  结果: COMP1_IRQn 已使能\n");

    printf("\n步骤3：等待比较器触发（最多等待 5 秒）\n");
    uint64_t t0 = system_get_tick();
    while (!comp_triggered && !system_elapsed(t0, 5000)) {
        __NOP();
    }
    if (comp_triggered) {
        printf("  结果: 比较器已触发，中断次数: %u\n",
               (unsigned int)comp_irq_count);
        printf("  VCOUT1=%d, VCOUT1_ORG=%d\n",
               comp_get_output(), comp_get_output_raw());
    } else {
        printf("  结果: 5 秒内未触发（输入电压可能未越过阈值）\n");
    }

    printf("\n步骤4：配置 EXTI 作为 STOP 模式唤醒源\n");
    // 配置 EXTI line 19 (COMP1) 为上升沿中断
    exti_internal_init(EXTI_LINE_COMP1, EXTI_TRIGGER_RISING);
    exti_clear_pending(EXTI_LINE_COMP1);
    printf("  结果: EXTI_LINE_COMP1 已配置为上升沿触发\n");
    printf("  说明: COMP 可在 STOP 模式下保持工作并唤醒 MCU\n");

    printf("\n步骤5：清理\n");
    NVIC_DisableIRQ(COMP1_IRQn);
    comp_register_callback(NULL);
    comp_disable();
    printf("  结果: 已清理\n");

    print_demo_end();
}

/**
 * @brief 场景8：综合应用
 *
 * 演示消隐 + 滤波 + 迟滞的组合使用。
 */
void scenario_8_combined_features(void)
{
    print_demo_header("场景8：综合应用（消隐 + 滤波 + 迟滞）");

    printf("步骤1：完整配置（迟滞 + 滤波 + 消隐）\n");
    comp_config_t cfg = {
        .inp_sel = COMP_INP_PB0,
        .inm_sel = COMP_INM_PB1,
        .polarity = COMP_POL_NORMAL,
        .hysteresis = COMP_HYS_20MV,
        .filter_enable = true,
        .filter_time = COMP_FLT_256CYCLES,
        .blank_sel = COMP_BLANK_TIM3_OC3,
        .vref_enable = false,
        .vref_sel = COMP_VREF_AVDD,
        .vref_cfg = 0,
        .lock = false,
    };
    if (comp_config(&cfg)) {
        printf("  结果: 综合配置成功\n");
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤2：查看完整状态\n");
    print_comp_status();

    printf("\n步骤3：锁定配置\n");
    comp_lock();
    printf("  锁定状态: %s\n", comp_is_locked() ? "已锁定" : "未锁定");
    printf("  说明: 锁定后 COMP_CR 变为只读，复位前无法修改\n");

    printf("\n步骤4：逐一说明各功能的作用\n");
    printf("  ┌──────────┬────────────────────────────────┐\n");
    printf("  │ 功能     │ 说明                           │\n");
    printf("  ├──────────┼────────────────────────────────┤\n");
    printf("  │ 迟滞20mV │ 防止输入接近时输出振荡        │\n");
    printf("  │ 滤波256  │ 滤除 <256 个滤波时钟的毛刺    │\n");
    printf("  │ 消隐TIM3 │ TIM3_OC3 有效时切断比较器输出 │\n");
    printf("  │ 锁定     │ 防止误操作修改配置            │\n");
    printf("  └──────────┴────────────────────────────────┘\n");

    printf("\n提示: 只有复位 MCU 才能解除锁定\n");

    comp_disable();
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
        while (1) { __NOP(); }
    }
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("COMP 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 初始化 COMP 时钟
    comp_example_init();

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_basic_comparator();
            break;
        case 2:
            scenario_2_hysteresis();
            break;
        case 3:
            scenario_3_filtered_output();
            break;
        case 4:
            scenario_4_vref_divider();
            break;
        case 5:
            scenario_5_polarity_inversion();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_interrupt_wakeup();
            break;
        case 8:
            scenario_8_combined_features();
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
// COMP1_IRQHandler 已在 comp.c 中实现
// 若需自定义中断处理，可在此重新定义
