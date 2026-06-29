/**
 * @file fmc_norsram_example.c
 * @brief FMC_NORSRAM 模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. SRAM 异步基本初始化与读写
 * 2. NOR Flash 异步模式
 * 3. PSRAM 同步突发模式
 * 4. NOR Flash 地址/数据复用模式
 * 5. 结构体配置与扩展模式（读写独立时序）
 * 6. 时序模型切换
 * 7. NWAIT 等待信号配置
 * 8. 时序参数计算
 *
 * @note 使用前需配置对应 GPIO 为 FMC 复用功能
 * @note 需使能 FMC 时钟（CLK_FMC）和 GPIO 时钟
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

// 测试用的存储块
#define TEST_BANK  FMC_NORSRAM_BANK_1

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief FMC_NORSRAM 示例初始化：使能 FMC 时钟
 */
static void fmc_example_init(void)
{
    clock_periph_enable(CLK_FMC);
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
 * @brief 打印当前 FMC_NORSRAM 存储块完整配置
 */
static void print_bank_config(fmc_norsram_bank_t bank)
{
    fmc_norsram_timing_t timing;
    fmc_norsram_get_timing(bank, &timing);

    printf("  ┌──────────────────────────────────┐\n");
    printf("  │ FMC_NORSRAM Bank%u 配置           │\n", (unsigned int)bank + 1U);
    printf("  ├──────────────────────────────────┤\n");
    printf("  │ 使能:        %-19s │\n",
           fmc_norsram_bank_is_enabled(bank) ? "是" : "否");

    fmc_mem_type_t mt = fmc_norsram_get_mem_type(bank);
    printf("  │ 存储器类型:  %-19s │\n",
           mt == FMC_MEM_TYPE_SRAM  ? "SRAM" :
           mt == FMC_MEM_TYPE_PSRAM ? "PSRAM" : "NOR Flash");

    fmc_data_width_t dw = fmc_norsram_get_data_width(bank);
    printf("  │ 数据宽度:    %-19s │\n",
           dw == FMC_DATA_WIDTH_8 ? "8位" : "16位");

    printf("  │ 基地址:      0x%08lX          │\n",
           (unsigned long)fmc_norsram_get_base_address(bank));

    printf("  │ 读时序:                        │\n");
    printf("  │   ASET=%u, AHLD=%u, DSET=%u       │\n",
           timing.aset, timing.ahld, timing.dset);
    printf("  │   BUSLAT=%u, CKDIV=%u, DLAT=%u    │\n",
           timing.buslat, timing.clkdiv, timing.dlat);
    printf("  │   异步模式=%u                     │\n",
           (unsigned int)timing.async_mode);
    printf("  └──────────────────────────────────┘\n");
}

/**
 * @brief 向指定地址写入 16 位测试数据并回读验证
 *
 * @note 假设 FMC 已正确配置，外部存储器已连接
 */
static bool memory_test_16bit(uint32_t base_addr, uint32_t offset, uint16_t pattern)
{
    volatile uint16_t *addr = (volatile uint16_t *)(base_addr + offset);
    *addr = pattern;
    // 读回验证
    uint16_t readback = *addr;
    return (readback == pattern);
}

/**
 * @brief 简单存储器扫描测试
 */
static void memory_basic_test(uint32_t base_addr, const char *label)
{
    printf("\n%s 基本读写测试（基地址 0x%08lX）:\n", label, (unsigned long)base_addr);

    bool pass = true;
    uint16_t patterns[] = { 0x55AA, 0xAA55, 0x0000, 0xFFFF, 0x5A5A };

    for (int i = 0; i < 5; i++) {
        if (memory_test_16bit(base_addr, (uint32_t)i * 2U, patterns[i])) {
            printf("  偏移 0x%02lX: 写入 0x%04X → 通过\n",
                   (unsigned long)i * 2U, patterns[i]);
        } else {
            printf("  偏移 0x%02lX: 写入 0x%04X → 失败！\n",
                   (unsigned long)i * 2U, patterns[i]);
            pass = false;
        }
    }

    printf("  结果: %s\n", pass ? "全部通过" : "存在失败");
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：SRAM 异步基本初始化与读写
 *
 * 演示使用 fmc_norsram_init_sram() 快速初始化外部 SRAM，
 * 并进行基本的读写操作。
 */
void scenario_1_sram_basic(void)
{
    print_demo_header("场景1：SRAM 异步基本初始化与读写");

    printf("步骤1：快速初始化外部 SRAM（16位，ASET=1, DSET=4, BUSLAT=1）\n");
    fmc_norsram_init_sram(TEST_BANK, FMC_DATA_WIDTH_16, 1U, 4U, 1U);
    printf("  结果: SRAM 模式已配置\n");

    printf("\n步骤2：查看 Bank1 配置\n");
    print_bank_config(TEST_BANK);

    printf("\n步骤3：获取 Bank1 基地址\n");
    uint32_t base = fmc_norsram_get_base_address(TEST_BANK);
    printf("  Bank1 基地址: 0x%08lX（最大 64MB）\n", (unsigned long)base);

    printf("\n步骤4：基本读写测试\n");
    memory_basic_test(base, "外部 SRAM");

    printf("\n步骤5：动态调整时序参数\n");
    printf("  修改 DSET 从 4 → 8\n");
    fmc_norsram_set_dset(TEST_BANK, 8U);
    fmc_norsram_timing_t t;
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  当前 DSET=%u\n", t.dset);

    printf("\n步骤6：禁止 Bank1\n");
    fmc_norsram_bank_enable(TEST_BANK, false);
    printf("  结果: Bank1 已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景2：NOR Flash 异步模式
 *
 * 演示使用 fmc_norsram_init_nor() 初始化外部 NOR Flash。
 */
void scenario_2_nor_flash(void)
{
    print_demo_header("场景2：NOR Flash 异步模式");

    printf("步骤1：快速初始化 NOR Flash（16位，ASET=1, DSET=8, BUSLAT=1）\n");
    fmc_norsram_init_nor(TEST_BANK, FMC_DATA_WIDTH_16, 1U, 8U, 1U, false);
    printf("  结果: NOR Flash 模式已配置\n");

    printf("\n步骤2：查看 NOR Flash 配置\n");
    print_bank_config(TEST_BANK);

    printf("\n步骤3：验证 NOR Flash 特定配置\n");
    fmc_mem_type_t mt = fmc_norsram_get_mem_type(TEST_BANK);
    printf("  存储器类型: %s\n", mt == FMC_MEM_TYPE_NOR ? "NOR Flash" : "其他");
    printf("  说明: NOR Flash 模式下 NREN=1（NOR Flash 访问使能）\n");

    printf("\n步骤4：读取 NOR Flash 制造商/设备 ID\n");
    uint32_t base = fmc_norsram_get_base_address(TEST_BANK);
    printf("  Bank1 基地址: 0x%08lX\n", (unsigned long)base);
    printf("  提示: 连接实际 NOR Flash 后，可通过读特定地址获取 ID\n");

    printf("\n步骤5：修改数据建立时间以匹配不同 NOR Flash\n");
    printf("  快速器件 DSET=4, 慢速器件 DSET=15\n");
    fmc_norsram_set_dset(TEST_BANK, 15U);
    fmc_norsram_timing_t t;
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  当前 DSET=%u（适合慢速 NOR Flash）\n", t.dset);

    printf("\n步骤6：写保护测试\n");
    fmc_norsram_set_write_enable(TEST_BANK, false);
    printf("  写使能: 已禁止（NOR Flash 写保护）\n");
    fmc_norsram_set_write_enable(TEST_BANK, true);
    printf("  写使能: 已恢复\n");

    print_demo_end();
}

/**
 * @brief 场景3：PSRAM 同步突发模式
 *
 * 演示使用 fmc_norsram_init_psram_sync() 初始化 PSRAM，
 * 配置同步时钟和突发传输。
 */
void scenario_3_psram_sync(void)
{
    print_demo_header("场景3：PSRAM 同步突发模式");

    printf("步骤1：快速初始化 PSRAM 同步模式（CKDIV=2, DLAT=2, BUSLAT=1）\n");
    fmc_norsram_init_psram_sync(TEST_BANK, FMC_DATA_WIDTH_16, 2U, 2U, 1U);
    printf("  结果: PSRAM 同步突发模式已配置\n");

    printf("\n步骤2：查看同步模式配置\n");
    print_bank_config(TEST_BANK);

    printf("\n步骤3：计算 FMC_CLK 频率\n");
    // 假设 HCLK = 160MHz
    uint32_t hclk = 160000000U;
    uint32_t fmc_clk = fmc_norsram_calculate_clk_hz(hclk, 2U);
    printf("  HCLK = %lu MHz\n", (unsigned long)hclk / 1000000U);
    printf("  CKDIV = 2, FMC_CLK = HCLK / 2 = %lu MHz\n",
           (unsigned long)fmc_clk / 1000000U);

    printf("\n步骤4：配置 CRAM 页大小\n");
    printf("  PSRAM/CRAM 支持页边界自动分割\n");
    fmc_norsram_set_cps(TEST_BANK, FMC_CPS_256BYTES);
    printf("  当前页大小: 256 字节\n");

    printf("\n步骤5：动态修改时钟分频\n");
    for (uint8_t div = 2U; div <= 6U; div += 2U) {
        fmc_norsram_set_clkdiv(TEST_BANK, div);
        uint32_t clk = fmc_norsram_calculate_clk_hz(hclk, div);
        printf("  CKDIV=%u → FMC_CLK=%lu MHz\n",
               div, (unsigned long)clk / 1000000U);
        delay_ms(10);
    }

    printf("\n步骤6：禁止突发模式\n");
    fmc_norsram_set_burst(TEST_BANK, false);
    printf("  结果: 突发模式已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景4：NOR Flash 地址/数据复用模式
 *
 * 演示地址/数据总线复用模式的配置和使用。
 * 复用模式下地址低16位与数据共用 D[15:0] 总线。
 */
void scenario_4_nor_mux(void)
{
    print_demo_header("场景4：NOR Flash 地址/数据复用模式");

    printf("步骤1：初始化 NOR Flash 复用模式（ASET=1, DSET=10, BUSLAT=2）\n");
    fmc_norsram_init_nor(TEST_BANK, FMC_DATA_WIDTH_16, 1U, 10U, 2U, true);
    printf("  结果: NOR Flash 复用模式已配置（NRMUX=1）\n");

    printf("\n步骤2：验证复用配置\n");
    printf("  复用模式: 使能\n");
    printf("  说明: 地址低16位与数据共用 D[15:0]，由 NADV 信号锁存地址\n");

    printf("\n步骤3：配置地址保持时间（复用模式关键参数）\n");
    fmc_norsram_set_ahld(TEST_BANK, 4U);
    fmc_norsram_timing_t t;
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  AHLD=%u（地址保持周期数，复用模式需 > 0）\n", t.ahld);

    printf("\n步骤4：查看完整时序配置\n");
    print_bank_config(TEST_BANK);

    printf("\n步骤5：切换到非复用模式对比\n");
    fmc_norsram_set_mux(TEST_BANK, false);
    printf("  复用模式: 禁止\n");

    fmc_norsram_set_mux(TEST_BANK, true);
    printf("  复用模式: 重新使能\n");

    printf("\n提示: 复用模式节省地址引脚，适用于引脚受限的设计\n");

    fmc_norsram_bank_enable(TEST_BANK, false);
    print_demo_end();
}

/**
 * @brief 场景5：结构体配置与扩展模式
 *
 * 演示使用 fmc_norsram_config() 结构体进行完整配置，
 * 以及扩展模式下读写独立时序的配置。
 */
void scenario_5_config_struct(void)
{
    print_demo_header("场景5：结构体配置与扩展模式（读写独立时序）");

    printf("步骤1：使用结构体配置 NOR Flash 扩展模式\n");
    fmc_norsram_config_t cfg = {
        .mem_type = FMC_MEM_TYPE_NOR,
        .data_width = FMC_DATA_WIDTH_16,
        .mux_enable = false,
        .sync_write = false,
        .ext_mode = true,        // 使能扩展模式
        .burst_enable = false,
        .wait_enable = false,
        .wait_polarity = false,
        .wait_config = false,
        .async_wait = false,
        .wrap_enable = false,
        .nor_enable = true,
        .cps = FMC_CPS_DISABLE,
        .write_enable = true,
    };

    fmc_norsram_timing_t read_timing = {
        .aset = 1U,
        .ahld = 2U,
        .dset = 8U,
        .buslat = 1U,
        .clkdiv = 1U,
        .dlat = 0U,
        .async_mode = FMC_ASYNC_MODE_B,  // NOR Flash 扩展读模式
    };

    if (fmc_norsram_config(TEST_BANK, &cfg, &read_timing)) {
        printf("  结果: 读时序配置成功\n");
    } else {
        printf("  结果: 配置失败\n");
    }

    printf("\n步骤2：配置扩展模式写时序（独立于读时序）\n");
    fmc_norsram_write_timing_t write_timing = {
        .waset = 2U,
        .wahld = 2U,
        .wdset = 12U,       // 写数据建立时间 > 读数据建立时间
        .wbuslat = 2U,
        .wasync_mode = FMC_ASYNC_MODE_B,
    };

    if (fmc_norsram_config_write_timing(TEST_BANK, &write_timing)) {
        printf("  结果: 写时序配置成功\n");
    } else {
        printf("  结果: 写时序配置失败\n");
    }

    printf("\n步骤3：查看扩展模式配置\n");
    print_bank_config(TEST_BANK);
    printf("  说明: 扩展模式下读时序用 FMC_SNTCFG，写时序用 FMC_SNWTCFG\n");

    printf("\n步骤4：验证参数校验功能\n");
    fmc_norsram_config_t invalid_cfg = {
        .mem_type = FMC_MEM_TYPE_SRAM,
        .data_width = FMC_DATA_WIDTH_16,
        .mux_enable = false,
        .sync_write = false,
        .ext_mode = false,
        .burst_enable = false,
        .wait_enable = false,
        .wait_polarity = false,
        .wait_config = false,
        .async_wait = false,
        .wrap_enable = false,
        .nor_enable = false,
        .cps = FMC_CPS_DISABLE,
        .write_enable = true,
    };
    fmc_norsram_timing_t invalid_timing = {
        .aset = 20U,  // 超出范围（> 16）
        .ahld = 2U,
        .dset = 8U,
        .buslat = 1U,
        .clkdiv = 1U,
        .dlat = 0U,
        .async_mode = FMC_ASYNC_MODE_A,
    };
    if (!fmc_norsram_config(TEST_BANK, &invalid_cfg, &invalid_timing)) {
        printf("  结果: 无效参数被正确拒绝（aset=20 > 16）\n");
    }

    fmc_norsram_bank_enable(TEST_BANK, false);
    print_demo_end();
}

/**
 * @brief 场景6：时序模型切换
 *
 * 演示使用 fmc_norsram_set_timing_mode() 在不同时序模型间切换。
 * 支持模式 1/2/A/B/C/D/AM/SM/E 共9种预定义模型。
 */
void scenario_6_timing_modes(void)
{
    print_demo_header("场景6：时序模型切换");

    printf("步骤1：初始化为 SRAM 基础配置\n");
    fmc_norsram_init_sram(TEST_BANK, FMC_DATA_WIDTH_16, 1U, 4U, 1U);
    printf("  当前: 模式1（SRAM/PSRAM 异步）\n");

    printf("\n步骤2：遍历各时序模型\n");
    printf("  ┌──────────┬────────────────────────────────────┐\n");
    printf("  │ 模型     │ 描述                               │\n");
    printf("  ├──────────┼────────────────────────────────────┤\n");

    const char *mode_names[] = {
        "模式1 ", "模式2 ", "模式A ", "模式B ",
        "模式C ", "模式D ", "复用AM", "同步E ", "复用SM"
    };
    const char *mode_descs[] = {
        "SRAM/PSRAM 异步",
        "Nor Flash 异步",
        "SRAM NOE翻转（扩展）",
        "Nor Flash（扩展）",
        "Nor Flash NOE翻转（扩展）",
        "带地址保持（扩展）",
        "Nor Flash 地址/数据复用",
        "NOR/PSRAM 同步读写",
        "Nor Flash 同步复用"
    };

    for (int i = 0; i < 9; i++) {
        printf("  │ %-8s │ %-34s │\n", mode_names[i], mode_descs[i]);
    }
    printf("  └──────────┴────────────────────────────────────┘\n");

    printf("\n步骤3：切换到同步模式 E\n");
    fmc_norsram_set_timing_mode(TEST_BANK, FMC_TIMING_MODE_E, 1U, 2U, 1U);
    fmc_norsram_timing_t t;
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  当前 CKDIV=%u, DLAT=%u（同步模式专用参数）\n", t.clkdiv, t.dlat);
    printf("  同步突发读: 已使能\n");

    printf("\n步骤4：切换到扩展模式 C（Nor Flash NOE 翻转）\n");
    fmc_norsram_set_timing_mode(TEST_BANK, FMC_TIMING_MODE_C, 1U, 8U, 1U);
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  当前 ASYNCMOD=%u（C = Nor Flash NOE 翻转）\n",
           (unsigned int)t.async_mode);

    printf("\n步骤5：切换到复用模式 AM\n");
    fmc_norsram_set_timing_mode(TEST_BANK, FMC_TIMING_MODE_AM, 1U, 10U, 2U);
    fmc_norsram_get_timing(TEST_BANK, &t);
    printf("  复用模式: 已使能\n");
    printf("  当前 ASET=%u, DSET=%u, BUSLAT=%u\n", t.aset, t.dset, t.buslat);

    fmc_norsram_bank_enable(TEST_BANK, false);
    print_demo_end();
}

/**
 * @brief 场景7：NWAIT 等待信号配置
 *
 * 演示 NWAIT 信号的配置，包括使能、极性、
 * 有效时机和异步等待功能。
 */
void scenario_7_wait_signal(void)
{
    print_demo_header("场景7：NWAIT 等待信号配置");

    printf("步骤1：初始化 SRAM 并查看 NWAIT 默认配置\n");
    fmc_norsram_init_sram(TEST_BANK, FMC_DATA_WIDTH_16, 1U, 4U, 1U);
    printf("  结果: SRAM 已配置（NWAIT 默认禁止）\n");

    printf("\n步骤2：使能 NWAIT 信号（低有效，等待状态期间有效）\n");
    fmc_norsram_config_wait(TEST_BANK, true, false, true);
    printf("  NRWTEN=1（使能）\n");
    printf("  NRWTPOL=0（低电平有效）\n");
    printf("  NRWTCFG=1（等待状态期间有效）\n");

    printf("\n步骤3：切换 NWAIT 极性\n");
    fmc_norsram_config_wait(TEST_BANK, true, true, true);
    printf("  NRWTPOL=1（高电平有效）\n");
    fmc_norsram_config_wait(TEST_BANK, true, false, true);
    printf("  恢复: NRWTPOL=0（低电平有效）\n");

    printf("\n步骤4：切换 NWAIT 有效时机\n");
    fmc_norsram_config_wait(TEST_BANK, true, false, false);
    printf("  NRWTCFG=0（提前1个 FMC_CLK 周期有效）\n");
    printf("  说明: 适用于对时序要求更严格的存储器\n");

    printf("\n步骤5：使能异步等待\n");
    fmc_norsram_set_async_wait(TEST_BANK, true);
    printf("  ASYNCWAIT=1（异步等待使能）\n");
    printf("  说明: 异步模式下，NWAIT 在数据建立阶段被检测\n");

    printf("\n步骤6：禁止 NWAIT\n");
    fmc_norsram_config_wait(TEST_BANK, false, false, false);
    fmc_norsram_set_async_wait(TEST_BANK, false);
    printf("  结果: NWAIT 已禁止\n");

    printf("\n步骤7：各种 NWAIT 配置组合说明\n");
    printf("  ┌────────┬────────┬────────┬─────────────────────────┐\n");
    printf("  │ NRWTEN │ POL    │ CFG    │ 行为                    │\n");
    printf("  ├────────┼────────┼────────┼─────────────────────────┤\n");
    printf("  │ 0      │ x      │ x      │ NWAIT 禁止              │\n");
    printf("  │ 1      │ 0      │ 0      │ 低有效，提前1周期       │\n");
    printf("  │ 1      │ 0      │ 1      │ 低有效，等待状态期间    │\n");
    printf("  │ 1      │ 1      │ 0      │ 高有效，提前1周期       │\n");
    printf("  │ 1      │ 1      │ 1      │ 高有效，等待状态期间    │\n");
    printf("  └────────┴────────┴────────┴─────────────────────────┘\n");

    fmc_norsram_bank_enable(TEST_BANK, false);
    print_demo_end();
}

/**
 * @brief 场景8：时序参数计算
 *
 * 演示 FMC_CLK 频率计算和读访问时间计算，
 * 帮助用户根据目标存储器规格确定正确的时序参数。
 */
void scenario_8_timing_calculation(void)
{
    print_demo_header("场景8：时序参数计算");

    printf("步骤1：FMC_CLK 频率计算\n");
    uint32_t hclk = 160000000U;
    printf("  系统 HCLK = %lu MHz\n", (unsigned long)hclk / 1000000U);

    printf("\n  ┌────────┬──────────────┬────────────┐\n");
    printf("  │ CKDIV  │ FMC_CLK      │ 周期       │\n");
    printf("  ├────────┼──────────────┼────────────┤\n");
    for (uint8_t ckdiv = 1U; ckdiv <= 8U; ckdiv++) {
        uint32_t clk = fmc_norsram_calculate_clk_hz(hclk, ckdiv);
        uint32_t period_ns = 1000000000U / clk;
        printf("  │ %-6u │ %-6lu MHz   │ %-5lu ns   │\n",
               ckdiv, (unsigned long)clk / 1000000U, (unsigned long)period_ns);
    }
    printf("  └────────┴──────────────┴────────────┘\n");

    printf("\n步骤2：异步读访问时间计算\n");
    printf("  公式: T_read = (ASET + DSET + BUSLAT) × T_HCLK\n");

    printf("\n  ┌──────┬──────┬────────┬──────────┬──────────┐\n");
    printf("  │ ASET │ DSET │ BUSLAT │ 总周期数 │ 时间(ns) │\n");
    printf("  ├──────┼──────┼────────┼──────────┼──────────┤\n");

    uint8_t test_configs[][3] = {
        {1, 4, 1}, {2, 6, 1}, {1, 8, 2}, {2, 15, 2}, {1, 4, 0}
    };

    for (int i = 0; i < 5; i++) {
        uint8_t a = test_configs[i][0];
        uint8_t d_val = test_configs[i][1];
        uint8_t b = test_configs[i][2];
        uint32_t time_ns = fmc_norsram_calculate_read_time_ns(hclk, a, d_val, b);
        printf("  │ %-4u │ %-4u │ %-6u │ %-8u │ %-5lu    │\n",
               a, d_val, b, (unsigned int)(a + d_val + b), (unsigned long)time_ns);
    }
    printf("  └──────┴──────┴────────┴──────────┴──────────┘\n");

    printf("\n步骤3：根据目标访问时间反推时序参数\n");
    printf("  示例: 目标读访问时间 < 100ns\n");
    // HCLK = 160MHz, T_HCLK = 6.25ns
    // 总周期数 = 100ns / 6.25ns ≈ 16
    // 可分配: ASET=1, DSET=12, BUSLAT=1
    uint32_t t_hclk_ns = 1000000000U / hclk;  // ~6.25ns
    printf("  HCLK 周期 = %lu ns\n", (unsigned long)t_hclk_ns);
    printf("  最大 HCLK 周期数 = 100ns / %luns ≈ %lu\n",
           (unsigned long)t_hclk_ns, (unsigned long)(100U / t_hclk_ns));

    printf("\n步骤4：参数推荐\n");
    fmc_norsram_timing_t rec;
    fmc_norsram_get_timing(TEST_BANK, &rec);
    uint32_t actual_ns = fmc_norsram_calculate_read_time_ns(
        hclk, rec.aset, rec.dset, rec.buslat);
    printf("  当前配置: ASET=%u, DSET=%u, BUSLAT=%u\n",
           rec.aset, rec.dset, rec.buslat);
    printf("  读访问时间 ≈ %lu ns\n", (unsigned long)actual_ns);

    printf("\n步骤5：NOR Flash 典型时序参考\n");
    printf("  ┌──────────────┬──────┬──────┬────────┐\n");
    printf("  │ 器件速度     │ ASET │ DSET │ BUSLAT │\n");
    printf("  ├──────────────┼──────┼──────┼────────┤\n");
    printf("  │ 快速(55ns)   │ 1    │ 8    │ 1      │\n");
    printf("  │ 中速(70ns)   │ 1    │ 10   │ 1      │\n");
    printf("  │ 慢速(90ns)   │ 2    │ 13   │ 1      │\n");
    printf("  │ 极慢(120ns)  │ 2    │ 15   │ 2      │\n");
    printf("  └──────────────┴──────┴──────┴────────┘\n");

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
    printf("FMC_NORSRAM 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 初始化 FMC 时钟
    fmc_example_init();

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_sram_basic();
            break;
        case 2:
            scenario_2_nor_flash();
            break;
        case 3:
            scenario_3_psram_sync();
            break;
        case 4:
            scenario_4_nor_mux();
            break;
        case 5:
            scenario_5_config_struct();
            break;
        case 6:
            scenario_6_timing_modes();
            break;
        case 7:
            scenario_7_wait_signal();
            break;
        case 8:
            scenario_8_timing_calculation();
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
