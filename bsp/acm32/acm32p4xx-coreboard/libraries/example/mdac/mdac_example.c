/**
 * @file mdac_example.c
 * @brief MDAC 模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. VDAC 基本初始化与电压输出
 * 2. IDAC 电流输出
 * 3. VDAC 多通道独立输出
 * 4. VDAC 校准功能
 * 5. TRIM 修调与采样保持模式
 * 6. 结构体配置方式
 * 7. 电压计算与精度验证
 * 8. 多通道综合应用（电压 + 电流）
 *
 * @note 使用前需使能 MDAC 时钟（CLK_MDAC）
 * @note 参考电压 VREF+ 为外部输入，典型值 3.3V
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

// 假设参考电压 VREF+ = 3.3V
#define VREF_MV  3300U

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief MDAC 示例初始化：使能 MDAC 时钟
 */
static void mdac_example_init(void)
{
    clock_periph_enable(CLK_MDAC);
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
 * @brief 打印 VDAC 通道配置
 */
static void print_vdac_channel(mdac_channel_t ch)
{
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ VDAC 通道 %-2u 状态                    │\n", (unsigned int)ch);
    printf("  ├──────────────────────────────────────┤\n");
    printf("  │ 使能:        %-24s │\n",
           mdac_is_enabled(ch) ? "是" : "否");
    printf("  │ 工作模式:    %-24s │\n",
           mdac_vdac_get_mode(ch) == MDAC_VDAC_MODE_BUF_ON ? "Buffer 使能" : "Buffer 禁止");
    printf("  │ TRIM 值:     %-24u │\n",
           (unsigned int)mdac_vdac_get_otrim(ch));
    printf("  │ 采样保持:    %-24s │\n",
           mdac_vdac_get_sample(ch) ? "采样模式" : "保持模式");
    printf("  │ 校准完成:    %-24s │\n",
           mdac_vdac_is_cal_done(ch) ? "是" : "否");

    uint16_t val = mdac_get_value(ch);
    uint32_t mv = mdac_calculate_voltage_mv(val, VREF_MV);
    printf("  │ 输出值:      %-4u (约 %-5lumV)       │\n",
           (unsigned int)val, (unsigned long)mv);
    printf("  └──────────────────────────────────────┘\n");
}

/**
 * @brief 打印 IDAC 通道配置
 */
static void print_idac_channel(mdac_channel_t ch)
{
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ IDAC 通道 %-2u 状态                    │\n",
           (unsigned int)(ch - MDAC_CH_12));
    printf("  ├──────────────────────────────────────┤\n");
    printf("  │ 使能:        %-24s │\n",
           mdac_is_enabled(ch) ? "是" : "否");
    printf("  │ ITURE 修调:  %-24u │\n",
           (unsigned int)mdac_idac_get_iture(ch));
    printf("  │ 输出值:      %-4u (10位)              │\n",
           (unsigned int)mdac_get_value(ch));
    printf("  └──────────────────────────────────────┘\n");
}

/**
 * @brief 打印电压值与 DAC 值的对照表
 */
static void print_voltage_table(uint32_t vref_mv)
{
    printf("  ┌──────────────┬──────────┬──────────────┐\n");
    printf("  │ 目标电压(mV) │ DAC 值   │ 实际电压(mV) │\n");
    printf("  ├──────────────┼──────────┼──────────────┤\n");

    uint32_t voltages[] = {0, 500, 1000, 1500, 1650, 2000, 2500, 3000, 3300};
    for (int i = 0; i < 9; i++) {
        uint16_t dac_val = mdac_calculate_vdac_value(voltages[i], vref_mv);
        uint32_t actual = mdac_calculate_voltage_mv(dac_val, vref_mv);
        printf("  │ %-12lu │ %-8u │ %-12lu │\n",
               (unsigned long)voltages[i],
               (unsigned int)dac_val,
               (unsigned long)actual);
    }
    printf("  └──────────────┴──────────┴──────────────┘\n");
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：VDAC 基本初始化与电压输出
 *
 * 演示使用 mdac_vdac_init() 初始化电压 DAC，
 * 并通过 mdac_set_value() 和 mdac_vdac_set_voltage() 设置输出电压。
 */
void scenario_1_vdac_basic(void)
{
    print_demo_header("场景1：VDAC 基本初始化与电压输出");

    printf("步骤1：初始化 VDAC 通道 0（Buffer 使能，TRIM=0）\n");
    mdac_vdac_init(MDAC_CH_0, MDAC_VDAC_MODE_BUF_ON, 0U);
    printf("  结果: VDAC0 已配置并使能\n");

    printf("\n步骤2：查看通道配置\n");
    print_vdac_channel(MDAC_CH_0);

    printf("\n步骤3：设置输出电压为 1.65V（VREF=3.3V）\n");
    uint16_t dac_val = mdac_calculate_vdac_value(1650U, VREF_MV);
    printf("  DAC 值 = 1650 × 4096 / 3300 = %u\n", (unsigned int)dac_val);
    mdac_set_value(MDAC_CH_0, dac_val);
    printf("  结果: 已写入数据寄存器\n");

    uint32_t actual_mv = mdac_vdac_get_voltage_mv(MDAC_CH_0, VREF_MV);
    printf("  实际输出电压 ≈ %lu mV\n", (unsigned long)actual_mv);

    printf("\n步骤4：输出多个电压值（VREF=3.3V）\n");
    uint32_t test_mv[] = {0U, 825U, 1650U, 2475U, 3300U};
    const char *usage[] = {"0V", "0.825V", "1.65V", "2.475V", "3.3V"};
    for (int i = 0; i < 5; i++) {
        mdac_vdac_set_voltage(MDAC_CH_0, test_mv[i], VREF_MV);
        delay_ms(200);
        printf("  目标 %s: DAC值=%u, 实际≈%lumV\n",
               usage[i],
               (unsigned int)mdac_get_value(MDAC_CH_0),
               (unsigned long)mdac_vdac_get_voltage_mv(MDAC_CH_0, VREF_MV));
    }

    printf("\n步骤5：禁止 VDAC 通道 0\n");
    mdac_enable(MDAC_CH_0, false);
    printf("  结果: VDAC0 已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景2：IDAC 电流输出
 *
 * 演示使用 mdac_idac_init() 初始化电流 DAC，
 * 并通过 mdac_set_value() 设置输出电流。
 */
void scenario_2_idac_basic(void)
{
    print_demo_header("场景2：IDAC 电流输出");

    printf("步骤1：初始化 IDAC 通道 0（ITURE=0）\n");
    mdac_idac_init(MDAC_CH_12, 0U);
    printf("  结果: IDAC0 已配置并使能\n");

    printf("\n步骤2：查看通道配置\n");
    print_idac_channel(MDAC_CH_12);

    printf("\n步骤3：设置不同输出电流\n");
    printf("  ┌──────────┬──────────┬──────────────┐\n");
    printf("  │ 百分比   │ DAC 值   │ 相对电流     │\n");
    printf("  ├──────────┼──────────┼──────────────┤\n");
    for (uint8_t pct = 0U; pct <= 100U; pct += 25U) {
        uint16_t val = mdac_calculate_idac_value(pct);
        mdac_set_value(MDAC_CH_12, val);
        printf("  │ %-3u%%     │ %-8u │ %-3u%% 满量程   │\n",
               (unsigned int)pct, (unsigned int)val, (unsigned int)pct);
        delay_ms(50);
    }
    printf("  └──────────┴──────────┴──────────────┘\n");

    printf("\n步骤4：ITURE 电流修调（每档 2%）\n");
    printf("  ITURE 范围: 0~31，每档调整 2%\n");
    for (uint8_t it = 5U; it <= 20U; it += 5U) {
        mdac_idac_set_iture(MDAC_CH_12, it);
        printf("  ITURE=%u: 电流调整约 %u%%\n", it, it * 2U);
        delay_ms(50);
    }
    mdac_idac_set_iture(MDAC_CH_12, 0U);
    printf("  恢复: ITURE=0\n");

    printf("\n步骤5：禁止 IDAC 通道 0\n");
    mdac_enable(MDAC_CH_12, false);
    printf("  结果: IDAC0 已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景3：VDAC 多通道独立输出
 *
 * 演示多个 VDAC 通道同时独立输出不同电压。
 */
void scenario_3_vdac_multi_channel(void)
{
    print_demo_header("场景3：VDAC 多通道独立输出");

    printf("步骤1：初始化 4 个 VDAC 通道\n");
    mdac_channel_t channels[] = {MDAC_CH_0, MDAC_CH_1, MDAC_CH_2, MDAC_CH_3};
    uint32_t voltages[] = {825U, 1650U, 2475U, 3300U};

    for (int i = 0; i < 4; i++) {
        mdac_vdac_init(channels[i], MDAC_VDAC_MODE_BUF_ON, 0U);
        mdac_vdac_set_voltage(channels[i], voltages[i], VREF_MV);
        printf("  VDAC%u: 目标 %lu mV\n",
               (unsigned int)i, (unsigned long)voltages[i]);
    }
    printf("  结果: 4 个通道已独立配置\n");

    printf("\n步骤2：查看各通道状态\n");
    printf("  ┌────────┬──────────┬──────────┬──────────┐\n");
    printf("  │ 通道   │ 使能     │ DAC 值   │ 电压(mV) │\n");
    printf("  ├────────┼──────────┼──────────┼──────────┤\n");
    for (int i = 0; i < 4; i++) {
        uint16_t val = mdac_get_value(channels[i]);
        uint32_t mv = mdac_vdac_get_voltage_mv(channels[i], VREF_MV);
        printf("  │ VDAC%-3u│ %-8s │ %-8u │ %-5lu    │\n",
               (unsigned int)i,
               mdac_is_enabled(channels[i]) ? "是" : "否",
               (unsigned int)val,
               (unsigned long)mv);
    }
    printf("  └────────┴──────────┴──────────┴──────────┘\n");

    printf("\n步骤3：独立更新各通道电压\n");
    uint32_t new_voltages[] = {500U, 2000U, 1000U, 2800U};
    for (int i = 0; i < 4; i++) {
        mdac_vdac_set_voltage(channels[i], new_voltages[i], VREF_MV);
        printf("  VDAC%u: %lu mV → %lu mV\n",
               (unsigned int)i,
               (unsigned long)voltages[i],
               (unsigned long)new_voltages[i]);
    }

    printf("\n步骤4：禁止所有通道\n");
    for (int i = 0; i < 4; i++) {
        mdac_enable(channels[i], false);
    }
    printf("  结果: 4 个通道已全部禁止\n");

    print_demo_end();
}

/**
 * @brief 场景4：VDAC 校准功能
 *
 * 演示 VDAC 校准功能的启动、等待和查询。
 */
void scenario_4_vdac_calibration(void)
{
    print_demo_header("场景4：VDAC 校准功能");

    printf("步骤1：初始化 VDAC 通道并启动校准\n");
    mdac_vdac_init(MDAC_CH_0, MDAC_VDAC_MODE_BUF_ON, 0U);
    printf("  结果: VDAC0 已使能\n");

    mdac_vdac_calibrate_start(MDAC_CH_0);
    printf("  校准已启动（CEN=1）\n");

    printf("\n步骤2：等待校准完成\n");
    if (mdac_vdac_wait_cal_done(MDAC_CH_0, 10000U)) {
        printf("  结果: 校准完成（CAL_FLAG=1）\n");
    } else {
        printf("  结果: 校准超时\n");
    }

    printf("\n步骤3：校准标志说明\n");
    printf("  CAL_FLAG = 0: 校准值 < offset value，校准未完成\n");
    printf("  CAL_FLAG = 1: 校准值 ≥ offset value，校准完成\n");

    printf("\n步骤4：重新校准\n");
    mdac_vdac_calibrate_stop(MDAC_CH_0);
    printf("  校准已停止（CEN=0）\n");
    delay_ms(10);

    mdac_vdac_calibrate_start(MDAC_CH_0);
    printf("  重新启动校准\n");
    if (mdac_vdac_wait_cal_done(MDAC_CH_0, 10000U)) {
        printf("  重新校准完成\n");
    }

    printf("\n步骤5：多通道校准\n");
    bool all_done = true;
    for (mdac_channel_t ch = MDAC_CH_0; ch <= MDAC_CH_3; ch = (mdac_channel_t)((int)ch + 1)) {
        mdac_vdac_init(ch, MDAC_VDAC_MODE_BUF_ON, 0U);
        mdac_vdac_calibrate_start(ch);
    }
    // 等待所有通道校准完成
    for (mdac_channel_t ch = MDAC_CH_0; ch <= MDAC_CH_3; ch = (mdac_channel_t)((int)ch + 1)) {
        if (!mdac_vdac_wait_cal_done(ch, 5000U)) {
            printf("  VDAC%u: 校准超时\n", (unsigned int)ch);
            all_done = false;
        }
    }
    printf("  多通道校准: %s\n", all_done ? "全部完成" : "部分超时");

    // 清理
    for (mdac_channel_t ch = MDAC_CH_0; ch <= MDAC_CH_3; ch = (mdac_channel_t)((int)ch + 1)) {
        mdac_enable(ch, false);
    }

    print_demo_end();
}

/**
 * @brief 场景5：TRIM 修调与采样保持模式
 *
 * 演示 VDAC OTRIM 修调和 SAMPLE_EN 采样保持功能。
 */
void scenario_5_trim_and_sample(void)
{
    print_demo_header("场景5：TRIM 修调与采样保持模式");

    printf("步骤1：初始化 VDAC0（Buffer ON, TRIM=0）\n");
    mdac_vdac_init(MDAC_CH_0, MDAC_VDAC_MODE_BUF_ON, 0U);
    printf("  结果: VDAC0 已配置\n");

    printf("\n步骤2：TRIM 修调（OTRIM 0~31）\n");
    printf("  OTRIM 用于微调 VDAC 输出电压精度\n");
    printf("  ┌────────┬──────────────────────┐\n");
    printf("  │ OTRIM  │ 说明                 │\n");
    printf("  ├────────┼──────────────────────┤\n");
    for (uint8_t t = 0U; t <= 31U; t += 8U) {
        mdac_vdac_set_otrim(MDAC_CH_0, t);
        printf("  │ %-6u │ TRIM 值 = %-2u          │\n", t, t);
        delay_ms(10);
    }
    printf("  └────────┴──────────────────────┘\n");

    printf("\n步骤3：设置中间值并查看\n");
    mdac_vdac_set_otrim(MDAC_CH_0, 16U);
    print_vdac_channel(MDAC_CH_0);

    printf("\n步骤4：采样保持模式切换\n");
    printf("  当前: %s\n",
           mdac_vdac_get_sample(MDAC_CH_0) ? "采样模式" : "保持模式（默认）");

    mdac_vdac_set_sample(MDAC_CH_0, true);
    printf("  切换: 采样模式（SAMPLE_EN=1）\n");
    printf("  说明: 采样模式下 VDAC 持续跟踪输入\n");

    mdac_vdac_set_sample(MDAC_CH_0, false);
    printf("  恢复: 保持模式（SAMPLE_EN=0）\n");
    printf("  说明: 保持模式下 VDAC 输出保持上次值\n");

    printf("\n步骤5：Buffer 模式切换\n");
    mdac_vdac_set_mode(MDAC_CH_0, MDAC_VDAC_MODE_BUF_OFF);
    printf("  当前: Buffer 禁止模式\n");
    printf("  说明: Buffer 禁止时输出阻抗较高\n");

    mdac_vdac_set_mode(MDAC_CH_0, MDAC_VDAC_MODE_BUF_ON);
    printf("  恢复: Buffer 使能模式\n");

    mdac_enable(MDAC_CH_0, false);
    print_demo_end();
}

/**
 * @brief 场景6：结构体配置方式
 *
 * 演示使用 mdac_vdac_config() 和 mdac_idac_config()
 * 结构体进行完整配置。
 */
void scenario_6_config_struct(void)
{
    print_demo_header("场景6：结构体配置方式");

    printf("步骤1：使用结构体配置 VDAC 通道\n");
    mdac_vdac_config_t vdac_cfg = {
        .mode = MDAC_VDAC_MODE_BUF_ON,
        .cal_enable = true,
        .otrim = 10U,
        .sample_en = false,
    };
    if (mdac_vdac_config(MDAC_CH_1, &vdac_cfg)) {
        printf("  结果: VDAC1 配置成功\n");
        mdac_enable(MDAC_CH_1, true);
    } else {
        printf("  结果: VDAC1 配置失败\n");
    }

    printf("\n步骤2：使用结构体配置 IDAC 通道\n");
    mdac_idac_config_t idac_cfg = {
        .iture = 15U,
    };
    if (mdac_idac_config(MDAC_CH_12, &idac_cfg)) {
        printf("  结果: IDAC0 配置成功\n");
        mdac_enable(MDAC_CH_12, true);
    } else {
        printf("  结果: IDAC0 配置失败\n");
    }

    printf("\n步骤3：查看配置结果\n");
    print_vdac_channel(MDAC_CH_1);
    print_idac_channel(MDAC_CH_12);

    printf("\n步骤4：参数校验测试\n");
    mdac_vdac_config_t invalid_cfg = {
        .mode = MDAC_VDAC_MODE_BUF_ON,
        .cal_enable = false,
        .otrim = 40U,  // 超出范围（> 31）
        .sample_en = false,
    };
    if (!mdac_vdac_config(MDAC_CH_1, &invalid_cfg)) {
        printf("  结果: 无效参数被正确拒绝（OTRIM=40 > 31）\n");
    }

    printf("\n步骤5：通道类型校验\n");
    // 尝试用 IDAC 配置 VDAC 通道
    mdac_idac_config_t wrong_cfg = { .iture = 10U };
    if (!mdac_idac_config(MDAC_CH_0, &wrong_cfg)) {
        printf("  结果: VDAC 通道被 IDAC 配置正确拒绝\n");
    }

    // 清理
    mdac_enable(MDAC_CH_1, false);
    mdac_enable(MDAC_CH_12, false);

    print_demo_end();
}

/**
 * @brief 场景7：电压计算与精度验证
 *
 * 演示 mdac_calculate_vdac_value() 和
 * mdac_calculate_voltage_mv() 计算函数。
 */
void scenario_7_voltage_calculation(void)
{
    print_demo_header("场景7：电压计算与精度验证");

    printf("步骤1：VDAC 电压 ←→ DAC 值对照表\n");
    printf("  VREF = %u mV, 分辨率 = 12 位 (4096 级)\n", VREF_MV);
    printf("  1 LSB = %u mV / 4096 = %lu mV\n",
           VREF_MV, (unsigned long)(VREF_MV * 1000U / 4096U));
    printf("  1 LSB ≈ %.3f mV\n\n", (float)VREF_MV / 4096.0f);

    print_voltage_table(VREF_MV);

    printf("\n步骤2：不同 VREF 下的电压分辨率\n");
    printf("  ┌──────────┬──────────────┬──────────────┐\n");
    printf("  │ VREF(mV) │ LSB (mV)    │ 精度         │\n");
    printf("  ├──────────┼──────────────┼──────────────┤\n");
    uint32_t vrefs[] = {2500U, 3000U, 3300U, 3600U};
    for (int i = 0; i < 4; i++) {
        uint32_t lsb = vrefs[i] * 1000U / 4096U;
        printf("  │ %-8lu │ %-4lu.%03lu mV   │ 12位         │\n",
               (unsigned long)vrefs[i],
               (unsigned long)(lsb / 1000U),
               (unsigned long)(lsb % 1000U));
    }
    printf("  └──────────┴──────────────┴──────────────┘\n");

    printf("\n步骤3：IDAC 电流百分比 ←→ DAC 值对照表\n");
    printf("  满量程 = 1024 级（10位）\n");
    printf("  ┌──────────┬──────────┬──────────────┐\n");
    printf("  │ 百分比   │ DAC 值   │ 相对电流     │\n");
    printf("  ├──────────┼──────────┼──────────────┤\n");
    for (uint8_t p = 0U; p <= 100U; p += 10U) {
        uint16_t val = mdac_calculate_idac_value(p);
        printf("  │ %-3u%%     │ %-8u │ %-3u%%          │\n",
               (unsigned int)p, (unsigned int)val, (unsigned int)p);
    }
    printf("  └──────────┴──────────┴──────────────┘\n");

    printf("\n步骤4：边界值测试\n");
    // VDAC 最大值
    uint16_t max_vdac = mdac_calculate_vdac_value(VREF_MV, VREF_MV);
    printf("  VDAC: Vout=VREF → DAC=%u（应为4095）\n",
           (unsigned int)max_vdac);

    // VDAC 0V
    uint16_t min_vdac = mdac_calculate_vdac_value(0U, VREF_MV);
    printf("  VDAC: Vout=0V   → DAC=%u（应为0）\n",
           (unsigned int)min_vdac);

    // IDAC 100%
    uint16_t max_idac = mdac_calculate_idac_value(100U);
    printf("  IDAC: 100%%      → DAC=%u（应为1023）\n",
           (unsigned int)max_idac);

    // IDAC 0%
    uint16_t min_idac = mdac_calculate_idac_value(0U);
    printf("  IDAC: 0%%        → DAC=%u（应为0）\n",
           (unsigned int)min_idac);

    print_demo_end();
}

/**
 * @brief 场景8：多通道综合应用
 *
 * 演示 VDAC + IDAC 综合使用，模拟实际应用场景。
 */
void scenario_8_combined_application(void)
{
    print_demo_header("场景8：多通道综合应用（电压 + 电流）");

    printf("步骤1：初始化所有通道\n");
    mdac_init_all();
    printf("  结果: 所有通道已复位（禁用、数据清零）\n");

    printf("\n步骤2：配置多个 VDAC 通道输出不同电压\n");
    // 模拟多路电压输出应用（如电源管理、传感器偏置）
    struct {
        mdac_channel_t ch;
        uint32_t mv;
        const char *desc;
    } vdac_outputs[] = {
        {MDAC_CH_0,  1200U, "传感器偏置 1.2V"},
        {MDAC_CH_1,  1800U, "ADC 参考 1.8V"},
        {MDAC_CH_2,  2500U, "运放中点 2.5V"},
        {MDAC_CH_3,  3300U, "满量程参考 3.3V"},
    };

    for (int i = 0; i < 4; i++) {
        mdac_vdac_init(vdac_outputs[i].ch, MDAC_VDAC_MODE_BUF_ON, 0U);
        mdac_vdac_set_voltage(vdac_outputs[i].ch, vdac_outputs[i].mv, VREF_MV);
        printf("  VDAC%u: %-20s → %lu mV\n",
               (unsigned int)vdac_outputs[i].ch,
               vdac_outputs[i].desc,
               (unsigned long)vdac_outputs[i].mv);
    }

    printf("\n步骤3：配置 IDAC 输出不同电流\n");
    struct {
        mdac_channel_t ch;
        uint8_t pct;
        const char *desc;
    } idac_outputs[] = {
        {MDAC_CH_12, 25U, "LED 背光（25%）"},
        {MDAC_CH_13, 50U, "LED 指示（50%）"},
        {MDAC_CH_14, 75U, "IR LED（75%）"},
        {MDAC_CH_15, 10U, "参考电流源（10%）"},
    };

    for (int i = 0; i < 4; i++) {
        mdac_idac_init(idac_outputs[i].ch, 0U);
        uint16_t val = mdac_calculate_idac_value(idac_outputs[i].pct);
        mdac_set_value(idac_outputs[i].ch, val);
        printf("  IDAC%u: %-20s → %u%% 满量程\n",
               (unsigned int)(idac_outputs[i].ch - MDAC_CH_12),
               idac_outputs[i].desc,
               (unsigned int)idac_outputs[i].pct);
    }

    printf("\n步骤4：总览所有通道状态\n");
    printf("  ┌────────┬──────────┬──────────┬────────────────┐\n");
    printf("  │ 通道   │ 类型     │ 使能     │ 输出           │\n");
    printf("  ├────────┼──────────┼──────────┼────────────────┤\n");
    for (mdac_channel_t ch = MDAC_CH_0; ch <= MDAC_CH_15;
         ch = (mdac_channel_t)((int)ch + 1)) {
        if (mdac_is_enabled(ch)) {
            if (mdac_is_vdac(ch)) {
                uint32_t mv = mdac_vdac_get_voltage_mv(ch, VREF_MV);
                printf("  │ VDAC%-2u │ 电压DAC  │ 是       │ %-5lumV        │\n",
                       (unsigned int)ch, (unsigned long)mv);
            } else {
                uint16_t val = mdac_get_value(ch);
                printf("  │ IDAC%-2u │ 电流DAC  │ 是       │ %-4u/1023      │\n",
                       (unsigned int)(ch - MDAC_CH_12), (unsigned int)val);
            }
        }
    }
    printf("  └────────┴──────────┴──────────┴────────────────┘\n");

    printf("\n步骤5：全局禁止\n");
    for (mdac_channel_t ch = MDAC_CH_0; ch <= MDAC_CH_15;
         ch = (mdac_channel_t)((int)ch + 1)) {
        mdac_enable(ch, false);
        mdac_set_value(ch, 0U);
    }
    printf("  结果: 所有通道已禁止并清零\n");

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
    printf("MDAC 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 初始化 MDAC 时钟
    mdac_example_init();

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_vdac_basic();
            break;
        case 2:
            scenario_2_idac_basic();
            break;
        case 3:
            scenario_3_vdac_multi_channel();
            break;
        case 4:
            scenario_4_vdac_calibration();
            break;
        case 5:
            scenario_5_trim_and_sample();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_voltage_calculation();
            break;
        case 8:
            scenario_8_combined_application();
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
