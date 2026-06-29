/**
 * @file lptim_example.c
 * @brief LPTIM 模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基础内部时钟计数器（连续/单次模式）
 * 2. PWM 波形输出与占空比调节
 * 3. 外部时钟脉冲计数器
 * 4. 编码器模式与方向检测
 * 5. 硬件触发启动
 * 6. 结构体配置方式
 * 7. 重复计数器与预装载模式
 * 8. 超时功能与周期计算
 *
 * @note 使用前需使能对应 LPTIM 时钟
 * @note 6 个实例：LPTIM1~6，示例默认使用 LPTIM1
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

// 假设内部时钟频率（用于演示计算）
#define DEMO_CLK_HZ   32000000U  // 32MHz PCLK

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief LPTIM 示例初始化：使能时钟
 */
static void lptim_example_init(void)
{
    clock_periph_enable(CLK_LPTIM1);
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
 * @brief 打印 LPTIM 状态
 */
static void print_lptim_status(lptim_instance_t lptim)
{
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ LPTIM%u 状态                           │\n",
           (unsigned int)(lptim + 1U));
    printf("  ├──────────────────────────────────────┤\n");

    printf("  │ 使能:        %-24s │\n",
           lptim_is_enabled(lptim) ? "是" : "否");

    lptim_clock_sel_t clk = lptim_get_clock_sel(lptim);
    printf("  │ 时钟源:      %-24s │\n",
           clk == LPTIM_CLK_INTERNAL ? "内部时钟" : "外部 Input1");

    lptim_count_mode_t cm = lptim_get_count_mode(lptim);
    printf("  │ 计数模式:    %-24s │\n",
           cm == LPTIM_COUNT_INTERNAL ? "内部脉冲" : "外部边沿");

    lptim_edge_t edge = lptim_get_edge(lptim);
    const char *edge_str;
    switch (edge) {
        case LPTIM_EDGE_RISING:  edge_str = "上升沿"; break;
        case LPTIM_EDGE_FALLING: edge_str = "下降沿"; break;
        case LPTIM_EDGE_BOTH:    edge_str = "双沿";   break;
        default:                 edge_str = "未知";   break;
    }
    printf("  │ 有效边沿:    %-24s │\n", edge_str);

    lptim_prescaler_t psc = lptim_get_prescaler(lptim);
    printf("  │ 预分频:      1/%-21u │\n",
           (unsigned int)(1U << (unsigned int)psc));

    uint16_t arr = lptim_get_autoreload(lptim);
    uint16_t cmp = lptim_get_compare(lptim);
    uint16_t cnt = lptim_get_counter(lptim);
    printf("  │ ARR:         %-24u │\n", (unsigned int)arr);
    printf("  │ CMP:         %-24u │\n", (unsigned int)cmp);
    printf("  │ CNT:         %-24u │\n", (unsigned int)cnt);

    lptim_trig_en_t trig = lptim_get_trigger(lptim);
    const char *trig_str;
    switch (trig) {
        case LPTIM_TRIG_SOFTWARE: trig_str = "软件触发"; break;
        case LPTIM_TRIG_RISING:   trig_str = "上升沿";   break;
        case LPTIM_TRIG_FALLING:  trig_str = "下降沿";   break;
        case LPTIM_TRIG_BOTH:     trig_str = "双沿";     break;
        default:                  trig_str = "未知";     break;
    }
    printf("  │ 触发方式:    %-24s │\n", trig_str);
    printf("  │ 触发源:      %-24u │\n",
           (unsigned int)lptim_get_trigger_sel(lptim));

    printf("  │ 编码器:      %-24s │\n",
           lptim_get_encoder(lptim) ? "使能" : "禁止");
    printf("  │ 超时功能:    %-24s │\n",
           lptim_get_timeout(lptim) ? "使能" : "禁止");
    printf("  │ PRELOAD:     %-24s │\n",
           lptim_get_preload(lptim) == LPTIM_UPDATE_PRELOAD ? "预装载" : "立即");

    uint8_t rep = lptim_get_repeat(lptim);
    printf("  │ 重复值:      %-24u │\n", (unsigned int)rep);

    uint32_t isr = lptim_get_status(lptim);
    printf("  │ ISR:         0x%08lX              │\n", (unsigned long)isr);
    printf("  └──────────────────────────────────────┘\n");
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基础内部时钟计数器
 *
 * 演示 lptim_init_counter() 初始化为内部时钟计数器，
 * 连续模式和单次模式的切换。
 */
void scenario_1_basic_counter(void)
{
    print_demo_header("场景1：基础内部时钟计数器");

    printf("步骤1：快速初始化 LPTIM1 为连续计数器（PSC=0, ARR=999）\n");
    printf("  clk=%uHz, PSC=1分频, ARR=1000\n", DEMO_CLK_HZ);
    printf("  周期 = 1000 / 32MHz = 31.25us\n");
    lptim_init_counter(LPTIM_1, LPTIM_PRESCALER_1, 1000U, true);
    printf("  结果: LPTIM1 已启动（连续模式）\n");

    printf("\n步骤2：查看计数器状态\n");
    delay_ms(1);
    print_lptim_status(LPTIM_1);

    printf("\n步骤3：读取计数器值（多次）\n");
    for (int i = 0; i < 5; i++) {
        uint16_t cnt = lptim_get_counter(LPTIM_1);
        printf("  CNT[%d] = %u\n", i, (unsigned int)cnt);
        delay_ms(1);
    }

    printf("\n步骤4：增大预分频（PSC=3 → 8分频）\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_prescaler(LPTIM_1, LPTIM_PRESCALER_8);
    lptim_enable(LPTIM_1, true);
    lptim_start_continuous(LPTIM_1);
    printf("  结果: 预分频已更新为 1/8\n");
    uint16_t cnt = lptim_get_counter(LPTIM_1);
    printf("  CNT = %u (计数速度降低 8 倍)\n", (unsigned int)cnt);

    printf("\n步骤5：切换为单次模式\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_prescaler(LPTIM_1, LPTIM_PRESCALER_1);
    lptim_set_autoreload(LPTIM_1, 1000U);
    lptim_enable(LPTIM_1, true);
    lptim_start_single(LPTIM_1);
    printf("  结果: 已启动单次计数\n");
    delay_ms(1);
    cnt = lptim_get_counter(LPTIM_1);
    printf("  CNT = %u (单次计数完成后停止)\n", (unsigned int)cnt);

    print_demo_end();
}

/**
 * @brief 场景2：PWM 波形输出与占空比调节
 *
 * 演示 lptim_init_pwm() 初始化 PWM 输出，
 * 动态调节比较值改变占空比。
 */
void scenario_2_pwm_output(void)
{
    print_demo_header("场景2：PWM 波形输出与占空比调节");

    printf("步骤1：初始化 LPTIM1 为 PWM 输出（PSC=0, ARR=999, CMP=499）\n");
    printf("  clk=%uHz, ARR=1000, CMP=500\n", DEMO_CLK_HZ);
    printf("  周期 = 1000/32M = 31.25us (32kHz)\n");
    printf("  占空比 = 500/1000 = 50%%\n");
    lptim_init_pwm(LPTIM_1, LPTIM_PRESCALER_1, 1000U, 500U);
    printf("  结果: PWM 输出已启动（50%% 占空比）\n");

    printf("\n步骤2：PWM 波形说明\n");
    printf("  CNT < CMP 时: 输出低电平 (WAVPOL=0)\n");
    printf("  CNT >= CMP 时: 输出高电平 (WAVPOL=0)\n");
    printf("  CNT == ARR 时: 输出复位，CNT 回到 0\n");

    printf("\n步骤3：调节占空比（10%% → 25%% → 50%% → 75%% → 90%%）\n");
    uint16_t cmp_values[] = {100U, 250U, 500U, 750U, 900U};
    for (int i = 0; i < 5; i++) {
        lptim_set_compare(LPTIM_1, cmp_values[i]);
        printf("  CMP=%u → 占空比≈%u%%\n",
               (unsigned int)cmp_values[i],
               (unsigned int)((uint32_t)cmp_values[i] * 100U / 1000U));
        delay_ms(50);
    }

    printf("\n步骤4：切换波形极性（WAVPOL=1 取反）\n");
    lptim_set_polarity(LPTIM_1, LPTIM_POL_INVERT);
    printf("  WAVPOL=1 → CNT<CMP 输出高, CNT≥CMP 输出低\n");
    printf("  结果: 输出极性已取反\n");

    printf("\n步骤5：切换为置1一次模式（WAVE=1）\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_wave_mode(LPTIM_1, LPTIM_WAVE_SET_ONCE);
    lptim_set_polarity(LPTIM_1, LPTIM_POL_NORMAL);
    lptim_enable(LPTIM_1, true);
    lptim_start_single(LPTIM_1);
    printf("  结果: 输出单个脉冲后保持高电平\n");

    printf("\n步骤6：恢复 PWM 模式\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_wave_mode(LPTIM_1, LPTIM_WAVE_PWM_PULSE);
    lptim_set_compare(LPTIM_1, 500U);
    lptim_enable(LPTIM_1, true);
    lptim_start_continuous(LPTIM_1);

    printf("\n步骤7：降低频率（PSC=4 → 16分频 + ARR=999）\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_prescaler(LPTIM_1, LPTIM_PRESCALER_16);
    lptim_set_autoreload(LPTIM_1, 1000U);
    lptim_set_compare(LPTIM_1, 500U);
    lptim_enable(LPTIM_1, true);
    lptim_start_continuous(LPTIM_1);
    printf("  周期 = 16 × 1000 / 32M = 500us (2kHz)\n");

    print_demo_end();
}

/**
 * @brief 场景3：外部时钟脉冲计数器
 *
 * 演示 lptim_init_ext_counter() 使用外部 Input1 作为时钟源，
 * 无需内部时钟即可计数（脉冲计数器应用）。
 */
void scenario_3_external_counter(void)
{
    print_demo_header("场景3：外部时钟脉冲计数器");

    printf("步骤1：初始化为外部时钟计数器（上升沿，ARR=100）\n");
    printf("  CKSEL=1 (外部时钟), CKPOL=00 (上升沿)\n");
    printf("  注意：使能后前 5 个有效边沿将丢失\n");
    lptim_init_ext_counter(LPTIM_1, LPTIM_EDGE_RISING, 100U, true);
    printf("  结果: 连续模式外部计数器已启动\n");

    printf("\n步骤2：查看配置\n");
    print_lptim_status(LPTIM_1);

    printf("\n步骤3：配置 Input1 选择为 GPIO\n");
    lptim_set_input1_sel(LPTIM_1, 0U);
    printf("  IN1SEL=0 (GPIO)\n");

    printf("\n步骤4：配置数字滤波器（CKFLT=2 → 4 采样）\n");
    printf("  注意：使能滤波器需要内部时钟源\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_filter(LPTIM_1, LPTIM_FILTER_4, LPTIM_FILTER_NONE);
    lptim_enable(LPTIM_1, true);
    lptim_start_continuous(LPTIM_1);
    printf("  结果: CKFLT=2，需 4 个连续相同采样才视为有效边沿\n");

    printf("\n步骤5：外部计数模式说明\n");
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ CKSEL=1, COUNTMODE=X                │\n");
    printf("  │ 时钟源: LPTIM Input1 引脚            │\n");
    printf("  │ 无需内部振荡器即可工作              │\n");
    printf("  │ 支持 STOP 模式运行（RC32K 供电）    │\n");
    printf("  │ 前 5 个边沿丢失（使能同步延迟）     │\n");
    printf("  │ CKPOL 不支持双沿（外部时钟时）      │\n");
    printf("  └──────────────────────────────────────┘\n");

    printf("\n步骤6：读取计数器值\n");
    uint16_t cnt = lptim_get_counter(LPTIM_1);
    printf("  CNT = %u (当前脉冲计数)\n", (unsigned int)cnt);

    lptim_enable(LPTIM_1, false);

    print_demo_end();
}

/**
 * @brief 场景4：编码器模式与方向检测
 *
 * 演示 lptim_init_encoder() 初始化编码器模式，
 * 通过 UP/DOWN 标志检测旋转方向。
 */
void scenario_4_encoder_mode(void)
{
    print_demo_header("场景4：编码器模式与方向检测");

    printf("步骤1：初始化 LPTIM1 为编码器模式（上升沿, ARR=4095）\n");
    printf("  CKSEL=0, PRESC=000 (1分频, 必须)\n");
    printf("  ENC=1, ARR=4096 (对应 1024 线编码器 × 4)\n");
    lptim_init_encoder(LPTIM_1, LPTIM_EDGE_RISING, 4096U);
    printf("  结果: 编码器模式已启动\n");

    printf("\n步骤2：编码器计数方案（CKPOL=00 上升沿）\n");
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ Input1 上升沿 + Input2=高 → 递减    │\n");
    printf("  │ Input1 上升沿 + Input2=低 → 递增    │\n");
    printf("  │ Input2 上升沿 + Input1=高 → 递增    │\n");
    printf("  │ Input2 上升沿 + Input1=低 → 递减    │\n");
    printf("  └──────────────────────────────────────┘\n");

    printf("\n步骤3：使能方向变化中断\n");
    lptim_enable(LPTIM_1, false);
    lptim_irq_enable(LPTIM_1, LPTIM_IRQ_UP, true);
    lptim_irq_enable(LPTIM_1, LPTIM_IRQ_DOWN, true);
    lptim_enable(LPTIM_1, true);
    lptim_start_continuous(LPTIM_1);
    printf("  结果: UPIE=1, DOWNIE=1\n");

    printf("\n步骤4：编码器模式子模式对照\n");
    printf("  ┌──────────┬──────────────────────┐\n");
    printf("  │ CKPOL    │ 编码器子模式         │\n");
    printf("  ├──────────┼──────────────────────┤\n");
    printf("  │ 00       │ 子模式1 (上升沿)     │\n");
    printf("  │ 01       │ 子模式2 (下降沿)     │\n");
    printf("  │ 10       │ 子模式3 (双沿)       │\n");
    printf("  └──────────┴──────────────────────┘\n");

    printf("\n步骤5：编码器限制要求\n");
    printf("  - CKSEL 必须为 0（内部时钟）\n");
    printf("  - PRESC 必须为 000（1分频）\n");
    printf("  - Input1/Input2 频率 ≤ 内部时钟 4分频\n");
    printf("  - 必须先配置为连续模式\n");

    printf("\n步骤6：查看编码器位置\n");
    uint16_t pos = lptim_get_counter(LPTIM_1);
    uint32_t isr = lptim_get_status(LPTIM_1);
    printf("  CNT = %u (编码器位置)\n", (unsigned int)pos);
    printf("  UP   = %s\n", (isr & LPTIM_FLAG_UP) ? "1" : "0");
    printf("  DOWN = %s\n", (isr & LPTIM_FLAG_DOWN) ? "1" : "0");

    lptim_irq_disable_all(LPTIM_1);
    lptim_enable(LPTIM_1, false);

    print_demo_end();
}

/**
 * @brief 场景5：硬件触发启动
 *
 * 演示使用外部触发信号启动 LPTIM，
 * 可配置触发源、触发边沿和触发滤波。
 */
void scenario_5_trigger_hardware(void)
{
    print_demo_header("场景5：硬件触发启动");

    printf("步骤1：初始化 LPTIM1（内部时钟，上升沿触发）\n");
    lptim_enable_clock(LPTIM_1);

    LPTIM_TypeDef *lp = (LPTIM_TypeDef *)LPTIM1_BASE;
    lp->CR_f.ENABLE = 0U;

    lptim_config_t cfg = {
        .clock_sel = LPTIM_CLK_INTERNAL,
        .count_mode = LPTIM_COUNT_INTERNAL,
        .edge = LPTIM_EDGE_RISING,
        .prescaler = LPTIM_PRESCALER_1,
        .trig_en = LPTIM_TRIG_RISING,
        .trig_sel = 0U,
        .ck_filter = LPTIM_FILTER_NONE,
        .trig_filter = LPTIM_FILTER_2,
        .wave = LPTIM_WAVE_PWM_PULSE,
        .polarity = LPTIM_POL_NORMAL,
        .preload = LPTIM_UPDATE_IMMEDIATE,
        .rep_load = LPTIM_REP_LOAD_IMMEDIATE,
        .timeout_en = false,
        .encoder_en = false,
    };
    lptim_config(LPTIM_1, &cfg);

    lp->ARR_f.ARR = 1000U;
    lp->CMP_f.CMP = 500U;
    lp->IER = 0U;

    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
    printf("  结果: 等待 GPIO 上升沿触发启动\n");

    printf("\n步骤2：触发配置说明\n");
    printf("  TRIGEN=01: 上升沿触发\n");
    printf("  TRIGSEL=0: GPIO 触发源\n");
    printf("  TRGFLT=1: 触发需稳定 2 个时钟周期\n");
    printf("  触发器输出在 SNGSTRT/CNTSTRT 置 1 后有效\n");

    printf("\n步骤3：触发源选择列表\n");
    printf("  ┌──────────┬──────────────────┐\n");
    printf("  │ TRIGSEL  │ 触发源           │\n");
    printf("  ├──────────┼──────────────────┤\n");
    printf("  │ 0000     │ GPIO 触发        │\n");
    printf("  │ 0001     │ lptim_ext_trig1  │\n");
    printf("  │ 0010     │ lptim_ext_trig2  │\n");
    printf("  │ ...      │ ...              │\n");
    printf("  │ 1111     │ lptim_ext_trig15 │\n");
    printf("  └──────────┴──────────────────┘\n");

    printf("\n步骤4：切换为下降沿触发\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_trigger(LPTIM_1, LPTIM_TRIG_FALLING, 1U);
    lptim_enable(LPTIM_1, true);
    lp->CR_f.CNTSTRT = 1U;
    printf("  TRIGEN=10, TRIGSEL=1 → 下降沿触发\n");

    printf("\n步骤5：切换回软件触发\n");
    lptim_enable(LPTIM_1, false);
    lptim_set_trigger(LPTIM_1, LPTIM_TRIG_SOFTWARE, 0U);
    lptim_enable(LPTIM_1, true);
    printf("  TRIGEN=00 → 软件触发\n");

    print_demo_end();
}

/**
 * @brief 场景6：结构体配置方式
 *
 * 演示 lptim_config() 结构体完整配置和参数校验。
 */
void scenario_6_config_struct(void)
{
    print_demo_header("场景6：结构体配置方式");

    printf("步骤1：使能 LPTIM1 时钟\n");
    lptim_enable_clock(LPTIM_1);

    printf("\n步骤2：使用结构体完整配置\n");
    LPTIM_TypeDef *lp = (LPTIM_TypeDef *)LPTIM1_BASE;
    lp->CR_f.ENABLE = 0U;

    lptim_config_t cfg = {
        .clock_sel = LPTIM_CLK_INTERNAL,
        .count_mode = LPTIM_COUNT_INTERNAL,
        .edge = LPTIM_EDGE_RISING,
        .prescaler = LPTIM_PRESCALER_2,
        .trig_en = LPTIM_TRIG_SOFTWARE,
        .trig_sel = 0U,
        .ck_filter = LPTIM_FILTER_NONE,
        .trig_filter = LPTIM_FILTER_NONE,
        .wave = LPTIM_WAVE_PWM_PULSE,
        .polarity = LPTIM_POL_NORMAL,
        .preload = LPTIM_UPDATE_PRELOAD,
        .rep_load = LPTIM_REP_LOAD_UNDERFLOW,
        .timeout_en = false,
        .encoder_en = false,
    };
    if (lptim_config(LPTIM_1, &cfg)) {
        printf("  结果: LPTIM1 结构体配置成功\n");
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤3：单独设置 ARR/CMP/预分频\n");
    lptim_set_prescaler(LPTIM_1, LPTIM_PRESCALER_4);
    lp->ARR_f.ARR = 2000U;
    lp->CMP_f.CMP = 1000U;
    printf("  PRESC=4 (16分频), ARR=2000, CMP=1000\n");
    printf("  50%% 占空比, PRELOAD=1 (预装载模式)\n");

    printf("\n步骤4：使能并启动\n");
    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
    printf("  结果: LPTIM1 已启动\n");

    printf("\n步骤5：查看配置状态\n");
    print_lptim_status(LPTIM_1);

    printf("\n步骤6：参数校验测试\n");
    lptim_config_t invalid_cfg;

    memset(&invalid_cfg, 0, sizeof(invalid_cfg));
    invalid_cfg.clock_sel = (lptim_clock_sel_t)99;
    if (!lptim_config(LPTIM_1, &invalid_cfg)) {
        printf("  无效 clock_sel=99 → 正确拒绝\n");
    }

    memset(&invalid_cfg, 0, sizeof(invalid_cfg));
    invalid_cfg.trig_sel = 16U;
    if (!lptim_config(LPTIM_1, &invalid_cfg)) {
        printf("  无效 trig_sel=16 → 正确拒绝\n");
    }

    memset(&invalid_cfg, 0, sizeof(invalid_cfg));
    invalid_cfg.ck_filter = (lptim_filter_t)4;
    if (!lptim_config(LPTIM_1, &invalid_cfg)) {
        printf("  无效 ck_filter=4 → 正确拒绝\n");
    }

    printf("\n步骤7：验证 CMP < ARR 约束\n");
    printf("  CMP(%u) < ARR(%u) %s\n",
           (unsigned int)lptim_get_compare(LPTIM_1),
           (unsigned int)lptim_get_autoreload(LPTIM_1),
           lptim_get_compare(LPTIM_1) < lptim_get_autoreload(LPTIM_1)
           ? "✓" : "✗ 不满足！");

    print_demo_end();
}

/**
 * @brief 场景7：重复计数器与预装载模式
 *
 * 演示重复计数器（REP）功能和 PRELOAD 预装载更新模式，
 * 以及 ARROK/CMPOK/REPOK 更新完成标志。
 */
void scenario_7_repeat_counter(void)
{
    print_demo_header("场景7：重复计数器与预装载模式");

    printf("步骤1：初始化 LPTIM1（PRELOAD=1, REP=3）\n");
    LPTIM_TypeDef *lp = (LPTIM_TypeDef *)LPTIM1_BASE;
    lp->CR_f.ENABLE = 0U;

    lptim_config_t cfg = {
        .clock_sel = LPTIM_CLK_INTERNAL,
        .count_mode = LPTIM_COUNT_INTERNAL,
        .edge = LPTIM_EDGE_RISING,
        .prescaler = LPTIM_PRESCALER_1,
        .trig_en = LPTIM_TRIG_SOFTWARE,
        .trig_sel = 0U,
        .ck_filter = LPTIM_FILTER_NONE,
        .trig_filter = LPTIM_FILTER_NONE,
        .wave = LPTIM_WAVE_PWM_PULSE,
        .polarity = LPTIM_POL_NORMAL,
        .preload = LPTIM_UPDATE_PRELOAD,
        .rep_load = LPTIM_REP_LOAD_UNDERFLOW,
        .timeout_en = false,
        .encoder_en = false,
    };
    lptim_config(LPTIM_1, &cfg);

    lp->ARR_f.ARR = 1000U;
    lp->CMP_f.CMP = 500U;
    lp->RCR_f.REP = 3U;
    lp->IER = 0U;

    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
    printf("  结果: REP=3，每 4 次溢出产生一次 REPUE\n");

    printf("\n步骤2：重复计数器工作原理\n");
    printf("  重复计数器在每次 CNT 溢出时递减\n");
    printf("  REP=0: 每次溢出都产生下溢事件\n");
    printf("  REP=3: 每 4 次溢出产生 1 次下溢事件\n");
    printf("  下溢后 REP 自动从 RCR 寄存器重载\n");

    printf("\n步骤3：使能 REPUE 中断\n");
    lptim_enable(LPTIM_1, false);
    lptim_irq_enable(LPTIM_1, LPTIM_IRQ_REPUE, true);
    lptim_irq_enable(LPTIM_1, LPTIM_IRQ_ARRM, true);
    lptim_enable(LPTIM_1, true);
    lp->CR_f.CNTSTRT = 1U;
    printf("  结果: REPUEIE=1, ARRMIE=1\n");

    printf("\n步骤4：PRELOAD 预装载模式说明\n");
    printf("  PRELOAD=0: 写 ARR/CMP 后立即生效\n");
    printf("  PRELOAD=1: 当前周期结束后生效\n");
    printf("  使用重复计数器时 PRELOAD 必须为 1\n");

    printf("\n步骤5：ARROK/CMPOK 更新完成标志\n");
    lp->ARR_f.ARR = 500U;
    printf("  写入 ARR=500\n");
    if (lptim_wait_arr_ok(LPTIM_1, 100000U)) {
        printf("  ARROK=1 → ARR 更新完成\n");
    } else {
        printf("  ARROK 等待超时\n");
    }

    lp->CMP_f.CMP = 250U;
    printf("  写入 CMP=250\n");
    if (lptim_wait_cmp_ok(LPTIM_1, 100000U)) {
        printf("  CMPOK=1 → CMP 更新完成\n");
    } else {
        printf("  CMPOK 等待超时\n");
    }

    printf("\n步骤6：REPOK 更新完成标志\n");
    lp->RCR_f.REP = 1U;
    printf("  写入 REP=1\n");
    if (lptim_wait_rep_ok(LPTIM_1, 100000U)) {
        printf("  REPOK=1 → REP 更新完成\n");
    } else {
        printf("  REPOK 等待超时\n");
    }

    printf("\n步骤7：查看最终状态\n");
    print_lptim_status(LPTIM_1);

    print_demo_end();
}

/**
 * @brief 场景8：超时功能与周期计算
 *
 * 演示 TIMEOUT 超时功能（触发复位计数器）和
 * lptim_calculate_arr/lptim_calculate_tick_ns 计算函数。
 */
void scenario_8_timeout_calculation(void)
{
    print_demo_header("场景8：超时功能与周期计算");

    printf("步骤1：配置超时功能\n");
    LPTIM_TypeDef *lp = (LPTIM_TypeDef *)LPTIM1_BASE;
    lp->CR_f.ENABLE = 0U;

    lptim_config_t cfg = {
        .clock_sel = LPTIM_CLK_INTERNAL,
        .count_mode = LPTIM_COUNT_INTERNAL,
        .edge = LPTIM_EDGE_RISING,
        .prescaler = LPTIM_PRESCALER_1,
        .trig_en = LPTIM_TRIG_RISING,
        .trig_sel = 0U,
        .ck_filter = LPTIM_FILTER_NONE,
        .trig_filter = LPTIM_FILTER_NONE,
        .wave = LPTIM_WAVE_PWM_PULSE,
        .polarity = LPTIM_POL_NORMAL,
        .preload = LPTIM_UPDATE_IMMEDIATE,
        .rep_load = LPTIM_REP_LOAD_IMMEDIATE,
        .timeout_en = true,
        .encoder_en = false,
    };
    lptim_config(LPTIM_1, &cfg);

    lp->ARR_f.ARR = 1000U;
    lp->CMP_f.CMP = 800U;
    lp->IER = 0U;
    lptim_irq_enable(LPTIM_1, LPTIM_IRQ_CMPM, true);

    lp->CR_f.ENABLE = 1U;
    lp->CR_f.CNTSTRT = 1U;
    printf("  TIMEOUT=1: 触发事件将复位计数器\n");
    printf("  CMP=800: 用作超时门限\n");
    printf("  若在 CNT 达到 800 前收到触发 → 计数器复位\n");
    printf("  若 CNT 达到 800 → 产生比较匹配中断（超时）\n");

    printf("\n步骤2：周期计算 - 各分频下的 tick 周期\n");
    printf("  clk = %u Hz\n\n", DEMO_CLK_HZ);
    printf("  ┌──────────┬────────────┬──────────────────────┐\n");
    printf("  │ 预分频   │ 分频系数   │ tick 周期 (ns)       │\n");
    printf("  ├──────────┼────────────┼──────────────────────┤\n");
    for (uint8_t i = 0U; i <= 7U; i++) {
        uint32_t tick_ns = lptim_calculate_tick_ns((lptim_prescaler_t)i, DEMO_CLK_HZ);
        printf("  │ %-8u │ %-10u │ %-8lu ns         │\n",
               (unsigned int)i,
               (unsigned int)(1U << i),
               (unsigned long)tick_ns);
    }
    printf("  └──────────┴────────────┴──────────────────────┘\n");

    printf("\n步骤3：常用周期对应的 ARR 值\n");
    printf("  ┌────────────┬──────────┬──────────┐\n");
    printf("  │ 目标周期   │ 预分频   │ ARR      │\n");
    printf("  ├────────────┼──────────┼──────────┤\n");
    uint32_t periods[] = {1000000U, 100000U, 10000U, 1000U, 100U};
    const char *period_names[] = {"1ms", "100us", "10us", "1us", "100ns"};
    for (int i = 0; i < 5; i++) {
        uint16_t arr = lptim_calculate_arr(periods[i], LPTIM_PRESCALER_1, DEMO_CLK_HZ);
        printf("  │ %-10s │ %-8u │ %-8u │\n",
               period_names[i],
               (unsigned int)LPTIM_PRESCALER_1,
               (unsigned int)arr);
        if (arr == 0U) {
            printf("  │            │ (需更大预分频)        │\n");
            arr = lptim_calculate_arr(periods[i], LPTIM_PRESCALER_16, DEMO_CLK_HZ);
            printf("  │            │ PSC=4     │ %-8u │\n", (unsigned int)arr);
        }
    }
    printf("  └────────────┴──────────┴──────────┘\n");

    printf("\n步骤4：ARR 极限值\n");
    uint16_t arr_max = lptim_calculate_arr(1000000000U, LPTIM_PRESCALER_1, DEMO_CLK_HZ);
    printf("  32MHz, PSC=1: 最大周期 ARR=%u\n", (unsigned int)arr_max);
    arr_max = lptim_calculate_arr(1000000000U, LPTIM_PRESCALER_128, DEMO_CLK_HZ);
    printf("  32MHz, PSC=128: 最大周期 ARR=%u\n", (unsigned int)arr_max);

    printf("\n步骤5：低功耗超时应用场景\n");
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ RC32K + LPTIM 超时唤醒              │\n");
    printf("  │ clk=32768Hz, PSC=128, ARR=256       │\n");
    printf("  │ 周期 = 128×256/32768 = 1s           │\n");
    printf("  │ STOP 模式下每 1s 唤醒检查一次       │\n");
    printf("  │ 触发信号复位计数器 → 喂狗/清除     │\n");
    printf("  │ 超时未触发 → 比较匹配唤醒 MCU      │\n");
    printf("  └──────────────────────────────────────┘\n");
    uint16_t arr_lp = lptim_calculate_arr(1000000000U, LPTIM_PRESCALER_128, 32768U);
    printf("  验证: RC32K PSC=128 → 1s ARR=%u\n", (unsigned int)arr_lp);

    print_demo_end();
}

//===========================================
// 中断回调
//===========================================

static void lptim_demo_callback(lptim_instance_t lptim, uint32_t flags)
{
    if (flags & LPTIM_FLAG_CMPM) {
        printf("  [IRQ] 比较匹配 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_ARRM) {
        printf("  [IRQ] 自动重载匹配 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_EXTTRIG) {
        printf("  [IRQ] 外部触发 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_CMPOK) {
        printf("  [IRQ] CMP 更新完成 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_ARROK) {
        printf("  [IRQ] ARR 更新完成 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_UP) {
        printf("  [IRQ] 方向变递增 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_DOWN) {
        printf("  [IRQ] 方向变递减 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_REPUE) {
        printf("  [IRQ] 重复计数器下溢 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
    if (flags & LPTIM_FLAG_REPOK) {
        printf("  [IRQ] REP 更新完成 (LPTIM%u)\n", (unsigned int)(lptim + 1U));
    }
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    sys_status_t status = system_init();
    if (status != SYS_OK) {
        while (1) { __NOP(); }
    }
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("LPTIM 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 注册中断回调
    lptim_register_callback(LPTIM_1, lptim_demo_callback);

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_basic_counter();
            break;
        case 2:
            scenario_2_pwm_output();
            break;
        case 3:
            scenario_3_external_counter();
            break;
        case 4:
            scenario_4_encoder_mode();
            break;
        case 5:
            scenario_5_trigger_hardware();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_repeat_counter();
            break;
        case 8:
            scenario_8_timeout_calculation();
            break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            printf("有效值: 1-8\n");
            break;
    }

    while (1) {
        __WFI();
    }
}
