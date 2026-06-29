/**
 * @file dlyb_example.c
 * @brief DLYB模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基础使能/禁止（透传模式）
 * 2. 手动配置UNIT和SEL（无校准）
 * 3. 自动延迟线校准（SDMMC单速率实例）
 * 4. 校准后相位居中选择（最优采样点）
 * 5. 动态切换输出时钟相位
 * 6. 多实例配置（ETH DLYB）
 * 7. dlyb_configure()一步完成配置
 * 8. 完整状态读取与全实例扫描
 */

#include "acm32p4.h"
#include "hardware/dlyb.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 3

//===========================================
// 全局变量
//===========================================

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

const char *dlyb_inst_name(dlyb_inst_t inst)
{
    static const char * const names[] = {
        "ETH_DLYB", "SDMMC_DLYBS",
        "SDMMC_DLYBD", "OSPI1_DLYB", "OSPI2_DLYB"
    };
    if ((uint32_t)inst < 5U) {
        return names[(uint32_t)inst];
    }
    return "UNKNOWN";
}

void print_dlyb_status(dlyb_inst_t inst)
{
    dlyb_status_t st;
    dlyb_get_status(inst, &st);
    printf("  实例  : %s\n", dlyb_inst_name(inst));
    printf("  DEN   : %s\n", st.den  ? "使能" : "禁止");
    printf("  SEN   : %s（%s）\n",
           st.sen ? "采样器使能" : "采样器禁止",
           st.sen ? "输出时钟被禁止" : "输出时钟正常");
    printf("  UNIT  : %u\n", st.unit);
    printf("  SEL   : %u\n", st.sel);
    printf("  LENF  : %s\n", st.lenf ? "有效" : "无效");
    if (st.lenf) {
        printf("  LEN   : 0x%03X（", st.len);
        for (int i = 11; i >= 0; i--) {
            printf("%c", (st.len & (1U << i)) ? '1' : '0');
        }
        printf("）\n");
    }
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基础使能与禁止（透传模式）
 *
 * DEN=0时输出时钟=输入时钟（无延迟）。
 * DEN=1、SEN=0时输出时钟经过当前配置的延迟。
 */
void scenario_1_basic_enable_disable(void)
{
    print_separator();
    printf("=== 场景1：基础使能与禁止（透传模式） ===\n\n");

    printf("步骤1：初始化SDMMC_DLYBS（DEN=1，SEN=0）\n");
    dlyb_init(DLYB_INST_SDMMCS);
    printf("  结果: %s\n", dlyb_is_enabled(DLYB_INST_SDMMCS) ? "成功" : "失败");

    printf("步骤2：读取初始状态\n");
    print_dlyb_status(DLYB_INST_SDMMCS);

    printf("步骤3：禁止DLYB模块（DEN=0，输出=输入时钟直通）\n");
    dlyb_disable(DLYB_INST_SDMMCS);
    printf("  结果: DEN=%s\n", dlyb_is_enabled(DLYB_INST_SDMMCS) ? "1（已使能）" : "0（已禁止）");

    printf("步骤4：重新使能（DEN=1）\n");
    dlyb_enable(DLYB_INST_SDMMCS);
    printf("  结果: DEN=%s\n", dlyb_is_enabled(DLYB_INST_SDMMCS) ? "1（已使能）" : "0（已禁止）");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景2：手动配置UNIT和SEL（不执行自动校准）
 *
 * 直接设置UNIT=5（单位延迟步数）和SEL=3（输出相位），
 * 演示SEN置位/清零的操作流程。
 */
void scenario_2_manual_config(void)
{
    print_separator();
    printf("=== 场景2：手动配置UNIT和SEL ===\n\n");

    const uint8_t target_unit = 5U;
    const uint8_t target_sel  = 3U;

    printf("步骤1：使能DLYB模块\n");
    dlyb_init(DLYB_INST_SDMMCS);

    printf("步骤2：置位SEN=1（禁止输出时钟，允许修改UNIT/SEL）\n");
    dlyb_sampler_enable(DLYB_INST_SDMMCS);
    printf("  结果: SEN=%s\n", dlyb_is_sampler_enabled(DLYB_INST_SDMMCS) ? "1" : "0");

    printf("步骤3：设置UNIT=%u\n", target_unit);
    dlyb_set_unit(DLYB_INST_SDMMCS, target_unit);

    printf("步骤4：等待LENF=1\n");
    bool lenf_ok = dlyb_wait_lenf(DLYB_INST_SDMMCS);
    printf("  结果: LENF=%s，LEN=0x%03X\n",
           lenf_ok ? "1（有效）" : "0（超时）",
           dlyb_get_len(DLYB_INST_SDMMCS));

    printf("步骤5：设置SEL=%u（输出时钟相位=输入时钟+%u×单位延迟）\n",
           target_sel, target_sel);
    dlyb_set_sel(DLYB_INST_SDMMCS, target_sel);

    printf("步骤6：清除SEN=0（使能输出时钟）\n");
    dlyb_sampler_disable(DLYB_INST_SDMMCS);
    printf("  结果: SEN=%s\n", dlyb_is_sampler_enabled(DLYB_INST_SDMMCS) ? "1" : "0");

    printf("步骤7：验证最终状态\n");
    print_dlyb_status(DLYB_INST_SDMMCS);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景3：自动延迟线校准
 *
 * 按手册步骤自动搜索最优UNIT值，使延迟线覆盖一个完整输入时钟周期。
 * @warning 硬件 BUG（芯片勘误 2.4.1）：使能 1+ 延时单元时初始延迟约 9ns，
 * 校准结果可能不实用。建议优先使用场景1的 SEL=0 透传模式。
 */
void scenario_3_auto_calibrate(void)
{
    print_separator();
    printf("=== 场景3：自动延迟线校准 ===\n\n");

    dlyb_calib_t calib;

    printf("步骤1：初始化并执行自动校准（SDMMC单速率DLYB）\n");
    dlyb_init_with_calib(DLYB_INST_SDMMCS, &calib);

    printf("步骤2：校准结果\n");
    if (calib.valid) {
        printf("  结果: 成功\n");
        printf("  UNIT=%u（最优单位延迟步数）\n", calib.unit);
        printf("  N=%u（覆盖一个输入时钟周期的单位延迟数量）\n", calib.len_count);
    } else {
        printf("  结果: 失败（未找到合适的UNIT值）\n");
        printf("  提示: 请确认输入时钟已存在且频率在15~245MHz范围内\n");
    }

    printf("步骤3：当前寄存器状态\n");
    print_dlyb_status(DLYB_INST_SDMMCS);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景4：校准后选择居中相位（最优采样点）
 *
 * 延迟线校准完成后，将输出时钟相位设置为len_count/2，
 * 使采样点位于数据稳定窗口中央，获得最佳时序裕量。
 * @warning 硬件 BUG（芯片勘误 2.4.1）：校准结果受初始 9ns 延迟影响，
 * 居中相位可能不准确。建议优先使用 SEL=0 透传模式。
 */
void scenario_4_center_phase_after_calib(void)
{
    print_separator();
    printf("=== 场景4：校准后选择居中相位（最优采样点） ===\n\n");

    dlyb_calib_t calib;

    printf("步骤1：执行自动校准\n");
    dlyb_init_with_calib(DLYB_INST_SDMMCS, &calib);

    if (!calib.valid) {
        printf("  结果: 校准失败，跳过相位配置\n");
        printf("\n完成！\n");
        print_separator();
        return;
    }
    printf("  结果: UNIT=%u，覆盖周期单位数N=%u\n", calib.unit, calib.len_count);

    /* 居中相位：N/2，确保不超过 DLYB_SEL_MAX */
    uint8_t center_sel = calib.len_count / 2U;
    if (center_sel > DLYB_SEL_MAX) {
        center_sel = DLYB_SEL_MAX;
    }

    printf("步骤2：选择居中相位SEL=%u（=N/2=%u/2）\n",
           center_sel, calib.len_count);
    dlyb_config_phase(DLYB_INST_SDMMCS, center_sel);

    printf("步骤3：验证状态\n");
    print_dlyb_status(DLYB_INST_SDMMCS);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景5：动态切换输出时钟相位
 *
 * 在运行时逐步改变相位（0→最大），演示动态相位调整能力。
 * 每次切换均经过SEN=1→写SEL→SEN=0三个步骤。
 */
void scenario_5_dynamic_phase_switch(void)
{
    print_separator();
    printf("=== 场景5：动态切换输出时钟相位 ===\n\n");

    /* 先校准确定合法相位范围 */
    dlyb_calib_t calib;
    dlyb_init_with_calib(DLYB_INST_SDMMCS, &calib);

    uint8_t max_sel = calib.valid ? calib.len_count : DLYB_SEL_MAX;
    if (max_sel > DLYB_SEL_MAX) {
        max_sel = DLYB_SEL_MAX;
    }

    printf("步骤1：从SEL=0逐步切换到SEL=%u\n", max_sel);
    for (uint8_t sel = 0U; sel <= max_sel; sel++) {
        dlyb_config_phase(DLYB_INST_SDMMCS, sel);
        dlyb_status_t st;
        dlyb_get_status(DLYB_INST_SDMMCS, &st);
        printf("  SEL=%2u → 实际SEL=%2u，SEN=%u\n", sel, st.sel, (unsigned)st.sen);
        delay_ms(1U); /* 快速演示，仅延迟 1ms */
    }

    printf("步骤2：复位到SEL=0\n");
    dlyb_config_phase(DLYB_INST_SDMMCS, 0U);
    dlyb_status_t st5;
    dlyb_get_status(DLYB_INST_SDMMCS, &st5);
    printf("  最终SEL=%u\n", st5.sel);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景6：多实例配置（ETH DLYB）
 *
 * 演示同时操作两个不同的DLYB实例（SDMMC和ETH）。
 */
void scenario_6_multi_instance(void)
{
    print_separator();
    printf("=== 场景6：多实例配置（ETH + SDMMC） ===\n\n");

    printf("步骤1：初始化SDMMC_DLYBS\n");
    dlyb_init(DLYB_INST_SDMMCS);
    printf("  %s DEN=%d\n",
           dlyb_inst_name(DLYB_INST_SDMMCS),
           (int)dlyb_is_enabled(DLYB_INST_SDMMCS));

    printf("步骤2：初始化ETH_DLYB\n");
    dlyb_init(DLYB_INST_ETH);
    printf("  %s DEN=%d\n",
           dlyb_inst_name(DLYB_INST_ETH),
           (int)dlyb_is_enabled(DLYB_INST_ETH));

    printf("步骤3：分别配置不同的UNIT和SEL\n");

    dlyb_config_t sdmmcs_cfg = { .unit = 4U, .sel = 2U };
    bool sdmmcs_ok = dlyb_configure(DLYB_INST_SDMMCS, &sdmmcs_cfg);
    printf("  SDMMC_DLYBS: UNIT=%u SEL=%u → %s\n",
           sdmmcs_cfg.unit, sdmmcs_cfg.sel, sdmmcs_ok ? "成功" : "失败");

    dlyb_config_t eth_cfg = { .unit = 8U, .sel = 4U };
    bool eth_ok = dlyb_configure(DLYB_INST_ETH, &eth_cfg);
    printf("  ETH_DLYB  : UNIT=%u SEL=%u → %s\n",
           eth_cfg.unit, eth_cfg.sel, eth_ok ? "成功" : "失败");

    printf("步骤4：读取两个实例的状态\n");
    print_dlyb_status(DLYB_INST_SDMMCS);
    printf("\n");
    print_dlyb_status(DLYB_INST_ETH);

    printf("步骤5：禁止两个实例\n");
    dlyb_disable(DLYB_INST_SDMMCS);
    dlyb_disable(DLYB_INST_ETH);
    printf("  结果: SDMMC_DLYBS DEN=%d，ETH_DLYB DEN=%d\n",
           (int)dlyb_is_enabled(DLYB_INST_SDMMCS),
           (int)dlyb_is_enabled(DLYB_INST_ETH));

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景7：dlyb_configure()一步完成配置
 *
 * 展示dlyb_configure()封装了SEN控制、UNIT/SEL写入和LENF等待的完整流程。
 */
void scenario_7_configure_api(void)
{
    print_separator();
    printf("=== 场景7：dlyb_configure() 一步完成配置 ===\n\n");

    dlyb_init(DLYB_INST_OSPI1);

    printf("步骤1：一步配置OSPI1_DLYB（UNIT=10，SEL=5）\n");
    dlyb_config_t cfg = { .unit = 10U, .sel = 5U };
    bool ok = dlyb_configure(DLYB_INST_OSPI1, &cfg);
    printf("  结果: %s\n", ok ? "成功" : "失败（LENF等待超时）");

    printf("步骤2：验证UNIT和SEL写入是否正确\n");
    dlyb_status_t st;
    dlyb_get_status(DLYB_INST_OSPI1, &st);
    printf("  UNIT期望=%u，实际=%u → %s\n",
           cfg.unit, st.unit, (st.unit == cfg.unit) ? "一致" : "不一致");
    printf("  SEL 期望=%u，实际=%u → %s\n",
           cfg.sel, st.sel, (st.sel == cfg.sel) ? "一致" : "不一致");

    printf("步骤3：参数范围截断验证（UNIT=100超过最大值63）\n");
    dlyb_config_t overflow_cfg = { .unit = 100U, .sel = 20U };
    dlyb_configure(DLYB_INST_OSPI1, &overflow_cfg);
    dlyb_get_status(DLYB_INST_OSPI1, &st);
    printf("  结果: UNIT传入=100，实际=%u（最大=%u）\n", st.unit, DLYB_UNIT_MAX);
    printf("  SEL 传入=20， 实际=%u（最大=%u）\n", st.sel, DLYB_SEL_MAX);

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景8：完整状态读取与全实例扫描
 *
 * 初始化全部5个DLYB实例，逐一读取并打印完整状态。
 */
void scenario_8_full_status_scan(void)
{
    print_separator();
    printf("=== 场景8：全实例状态扫描 ===\n\n");

    const dlyb_inst_t all_insts[] = {
        DLYB_INST_ETH,
        DLYB_INST_SDMMCS, DLYB_INST_SDMMCD,
        DLYB_INST_OSPI1, DLYB_INST_OSPI2
    };
    const uint32_t inst_count = sizeof(all_insts) / sizeof(all_insts[0]);

    printf("步骤1：使能全部%u个实例\n", inst_count);
    for (uint32_t i = 0U; i < inst_count; i++) {
        dlyb_init(all_insts[i]);
    }

    printf("步骤2：逐实例读取状态\n");
    for (uint32_t i = 0U; i < inst_count; i++) {
        print_dlyb_status(all_insts[i]);
        printf("\n");
    }

    printf("步骤3：禁止全部实例\n");
    for (uint32_t i = 0U; i < inst_count; i++) {
        dlyb_disable(all_insts[i]);
        printf("  %s DEN=%d\n",
               dlyb_inst_name(all_insts[i]),
               (int)dlyb_is_enabled(all_insts[i]));
    }

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
    printf("DLYB 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1: scenario_1_basic_enable_disable(); break;
        case 2: scenario_2_manual_config();        break;
        case 3: scenario_3_auto_calibrate();       break;
        case 4: scenario_4_center_phase_after_calib(); break;
        case 5: scenario_5_dynamic_phase_switch(); break;
        case 6: scenario_6_multi_instance();       break;
        case 7: scenario_7_configure_api();        break;
        case 8: scenario_8_full_status_scan();     break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            printf("请选择 1-8 之间的场景编号\n");
            break;
    }

    while (1) {
        __WFI();
    }
}
