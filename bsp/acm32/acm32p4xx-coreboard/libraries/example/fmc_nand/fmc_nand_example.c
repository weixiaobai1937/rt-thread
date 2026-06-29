/**
 * @file fmc_nand_example.c
 * @brief FMC_NAND 模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基本 NAND Flash 初始化与命令发送
 * 2. EDO 模式配置
 * 3. 命令/地址/数据通道完整读写流程
 * 4. BCH 编码（写入数据并生成校验码）
 * 5. BCH 自动纠错译码
 * 6. 结构体配置方式
 * 7. RBN 就绪/繁忙信号监测与中断
 * 8. 时序参数计算与优化
 *
 * @note 使用前需配置对应 GPIO 为 FMC 复用功能
 * @note 需使能 FMC 时钟（CLK_FMC）
 * @note BCH SRAM 地址范围：0x20038000 ~ 0x20057FFF
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1  // 修改此值选择运行的场景

// 典型 NAND Flash 命令定义
#define NAND_CMD_READ_ID    0x90U  ///< 读 ID
#define NAND_CMD_READ       0x00U  ///< 读页
#define NAND_CMD_READ_CONF  0x30U  ///< 读确认
#define NAND_CMD_WRITE      0x80U  ///< 写页
#define NAND_CMD_WRITE_CONF 0x10U  ///< 写确认
#define NAND_CMD_ERASE      0x60U  ///< 块擦除
#define NAND_CMD_ERASE_CONF 0xD0U  ///< 擦除确认
#define NAND_CMD_STATUS     0x70U  ///< 读状态
#define NAND_CMD_RESET      0xFFU  ///< 复位

//===========================================
// 全局变量
//===========================================

static volatile bool nand_rbn_event = false;
static volatile bool bch_decode_done = false;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief FMC_NAND 示例初始化：使能 FMC 时钟
 */
static void fmc_nand_example_init(void)
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
 * @brief 打印当前 NAND 配置
 */
static void print_nand_config(void)
{
    fmc_nand_timing_t t;
    fmc_nand_get_timing(&t);

    printf("  ┌──────────────────────────────────┐\n");
    printf("  │ FMC_NAND 当前配置                │\n");
    printf("  ├──────────────────────────────────┤\n");
    printf("  │ 大小端:      %-19s │\n",
           fmc_nand_get_endian() == FMC_NAND_ENDIAN_LITTLE ? "小端" : "大端");
    printf("  │ EDO:         %-19s │\n",
           fmc_nand_get_edo() ? "使能" : "禁止");
    printf("  │ 写保护:      %-19s │\n",
           fmc_nand_get_write_protect() ? "禁止写保护" : "写保护中");
    printf("  │ RBN中断:     %-19s │\n",
           fmc_nand_get_rbn_interrupt() ? "使能" : "禁止");
    printf("  │ NAND就绪:    %-19s │\n",
           fmc_nand_is_ready() ? "是" : "否（忙）");
    printf("  │ 时序:                         │\n");
    printf("  │   TWP=%-3u TWH=%-3u TRP=%-3u TREH=%-3u  │\n",
           t.twp, t.twh, t.trp, t.treh);
    printf("  │   TWHR=%-2u TRHW=%-2u TADL=%-2u          │\n",
           t.twhr, t.trhw, t.tadl);
    printf("  └──────────────────────────────────┘\n");
}

/**
 * @brief 打印 BCH 配置
 */
static void print_bch_config(void)
{
    printf("  ┌──────────────────────────────────┐\n");
    printf("  │ BCH 当前配置                     │\n");
    printf("  ├──────────────────────────────────┤\n");
    printf("  │ BCH类型:     %-19s │\n", "512B+3B+13BECC");
    printf("  │ 模式:        %-19s │\n",
           fmc_nand_bch_get_mode() == FMC_BCH_MODE_ENCODE ? "编码" : "译码");
    printf("  │ 纠错模式:    %-19s │\n",
           fmc_nand_bch_get_correct_mode() == FMC_BCH_CORRECT_AUTO ? "自动" : "手动");
    printf("  │ 译码中断:    %-19s │\n",
           fmc_nand_bch_get_decode_int() ? "使能" : "禁止");
    printf("  │ 忽略全1:     %-19s │\n",
           fmc_nand_bch_get_ignore_all1() ? "是" : "否");
    printf("  │ 基地址:      0x%08lX          │\n",
           (unsigned long)fmc_nand_bch_get_base_addr());
    printf("  └──────────────────────────────────┘\n");
}

/**
 * @brief NAND 中断回调
 */
static void nand_isr_callback(void)
{
    nand_rbn_event = true;
}

/**
 * @brief BCH 中断回调
 */
static void bch_isr_callback(void)
{
    bch_decode_done = true;
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基本 NAND Flash 初始化与命令发送
 *
 * 演示 fmc_nand_init_basic() 快速初始化，
 * 并发送 NAND Flash 基本命令。
 */
void scenario_1_basic_init(void)
{
    print_demo_header("场景1：基本 NAND Flash 初始化与命令发送");

    printf("步骤1：快速初始化 NAND Flash（TWP=4, TWH=2, TRP=4, TREH=2）\n");
    fmc_nand_init_basic(4U, 2U, 4U, 2U);
    printf("  结果: NAND Flash 已配置（小端模式、NCE0 选中）\n");

    printf("\n步骤2：查看当前配置\n");
    print_nand_config();

    printf("\n步骤3：发送复位命令\n");
    fmc_nand_send_command(NAND_CMD_RESET);
    printf("  结果: 复位命令 (0xFF) 已发送\n");

    printf("\n步骤4：等待 NAND Flash 就绪\n");
    bool ready = fmc_nand_wait_ready(5000U);
    printf("  结果: NAND Flash %s\n", ready ? "已就绪" : "超时");

    printf("\n步骤5：发送读 ID 命令并读取数据\n");
    fmc_nand_send_command(NAND_CMD_READ_ID);
    fmc_nand_send_address(0x00U);  // 发送地址
    uint32_t id_data = fmc_nand_read_data();
    printf("  读 ID 命令 (0x90) 已发送\n");
    printf("  地址 0x00 已发送\n");
    printf("  读回数据: 0x%08lX\n", (unsigned long)id_data);

    printf("\n步骤6：切换大小端模式\n");
    fmc_nand_set_endian(FMC_NAND_ENDIAN_BIG);
    printf("  当前: %s\n",
           fmc_nand_get_endian() == FMC_NAND_ENDIAN_BIG ? "大端模式" : "小端模式");
    fmc_nand_set_endian(FMC_NAND_ENDIAN_LITTLE);
    printf("  恢复: 小端模式\n");

    print_demo_end();
}

/**
 * @brief 场景2：EDO 模式配置
 *
 * 演示 fmc_nand_init_edo() 初始化 EDO 模式，
 * EDO 模式提供更短读脉冲以提升读性能。
 */
void scenario_2_edo_mode(void)
{
    print_demo_header("场景2：EDO 模式配置");

    printf("步骤1：基础模式 vs EDO 模式时序对比\n");
    printf("  ┌─────────┬──────┬──────┬──────┬──────┐\n");
    printf("  │ 模式    │ TWP  │ TWH  │ TRP  │ TREH │\n");
    printf("  ├─────────┼──────┼──────┼──────┼──────┤\n");
    printf("  │ 基础    │ 4    │ 2    │ 4    │ 2    │\n");
    printf("  │ EDO     │ 4    │ 2    │ 3    │ 2    │\n");
    printf("  └─────────┴──────┴──────┴──────┴──────┘\n");
    printf("  说明: EDO 模式下 TRP 可以更短，因为数据在 NOE\n");
    printf("        上升沿后仍保持有效\n");

    printf("\n步骤2：初始化为 EDO 模式\n");
    fmc_nand_init_edo(4U, 2U, 3U, 2U);
    printf("  结果: EDO 模式已配置\n");

    printf("\n步骤3：验证 EDO 配置\n");
    print_nand_config();
    printf("  EDO 状态: %s\n", fmc_nand_get_edo() ? "已使能" : "已禁止");

    printf("\n步骤4：动态切换 EDO 模式\n");
    fmc_nand_set_edo(false);
    printf("  EDO 禁止 → TRP=%u（标准读脉冲）\n", 4U);
    fmc_nand_set_edo(true);
    printf("  EDO 使能 → TRP 可降至更低值\n");

    printf("\n步骤5：EDO 读操作示例\n");
    fmc_nand_send_command(NAND_CMD_READ);
    fmc_nand_send_address(0x00U);
    fmc_nand_send_address(0x00U);
    uint32_t val = fmc_nand_read_data();
    printf("  读回数据: 0x%08lX（EDO 模式下数据保持更稳定）\n",
           (unsigned long)val);

    print_demo_end();
}

/**
 * @brief 场景3：命令/地址/数据通道完整读写流程
 *
 * 演示 NAND Flash 完整页读取流程：
 * 发送命令 → 发送多字节地址 → 等待就绪 → 读取数据。
 */
void scenario_3_read_write_flow(void)
{
    print_demo_header("场景3：命令/地址/数据通道完整读写流程");

    printf("步骤1：初始化 NAND Flash\n");
    fmc_nand_init_basic(4U, 2U, 4U, 2U);
    printf("  结果: 初始化完成\n");

    printf("\n步骤2：NAND Flash 典型读页流程\n");
    printf("  ┌────────────────────────────────────┐\n");
    printf("  │ 步骤 │ 操作                        │\n");
    printf("  ├────────────────────────────────────┤\n");
    printf("  │  1   │ 发送命令 0x00（读页第1周期） │\n");
    printf("  │  2   │ 发送5字节地址（列+行+块）   │\n");
    printf("  │  3   │ 发送命令 0x30（读确认）     │\n");
    printf("  │  4   │ 等待 RBN 就绪              │\n");
    printf("  │  5   │ 循环读取数据               │\n");
    printf("  └────────────────────────────────────┘\n");

    printf("\n步骤3：演示命令发送\n");
    fmc_nand_send_command(NAND_CMD_READ);
    printf("  发送读命令: 0x%02X\n", NAND_CMD_READ);

    printf("\n步骤4：演示多字节地址发送\n");
    printf("  典型 5 字节地址格式: [列低字节] [列高字节] [行低] [行中] [行高]\n");
    for (uint8_t i = 0; i < 5U; i++) {
        fmc_nand_send_address(i);
        printf("  发送地址字节 %u: 0x%02X\n", i, i);
    }

    printf("\n步骤5：发送读确认命令\n");
    fmc_nand_send_command(NAND_CMD_READ_CONF);
    printf("  发送读确认: 0x%02X\n", NAND_CMD_READ_CONF);

    printf("\n步骤6：等待就绪并读取数据\n");
    if (fmc_nand_wait_ready(10000U)) {
        printf("  NAND Flash 已就绪\n");
        uint32_t d0 = fmc_nand_read_data();
        printf("  读出数据[0]: 0x%08lX\n", (unsigned long)d0);
    } else {
        printf("  NAND Flash 超时（可能未连接实际器件）\n");
    }

    printf("\n步骤7：ECC vs 非ECC 通道说明\n");
    printf("  ┌──────────────┬──────────────────────────┐\n");
    printf("  │ 通道         │ 功能                     │\n");
    printf("  ├──────────────┼──────────────────────────┤\n");
    printf("  │ NFMECCDATA   │ 数据发送到 BCH 纠错模块  │\n");
    printf("  │ NFMNECCDATA  │ 仅数据收发，不经过 BCH   │\n");
    printf("  └──────────────┴──────────────────────────┘\n");

    print_demo_end();
}

/**
 * @brief 场景4：BCH 编码（写入数据并生成校验码）
 *
 * 演示 BCH 编码流程：
 * 配置 → 复位通道 → 通过 ECC 通道写入数据 → 读取校验码。
 */
void scenario_4_bch_encode(void)
{
    print_demo_header("场景4：BCH 编码（写入数据并生成校验码）");

    printf("步骤1：初始化为 NAND + BCH 模式\n");
    fmc_nand_init_with_bch(4U, 2U, 4U, 2U);
    printf("  结果: NAND + BCH 已配置\n");

    printf("\n步骤2：配置 BCH 为编码模式\n");
    fmc_bch_config_t bch_cfg = {
        .bch_type = FMC_BCH_TYPE_512B_8BIT,
        .mode = FMC_BCH_MODE_ENCODE,
        .correct_mode = FMC_BCH_CORRECT_MANUAL,
        .decode_int_enable = false,
        .ignore_all1 = false,
    };
    fmc_nand_bch_config(&bch_cfg);
    printf("  结果: BCH 编码模式已配置\n");

    printf("\n步骤3：开始新编码\n");
    fmc_nand_bch_clear_encode();
    printf("  结果: 编码通道已清除\n");

    printf("\n步骤4：通过 ECC 通道写入数据（模拟写入一页 512+3 字节）\n");
    fmc_nand_send_command(NAND_CMD_WRITE);
    uint8_t test_data[16];
    for (int i = 0; i < 16; i++) {
        test_data[i] = (uint8_t)(i * 17U);
    }
    fmc_nand_write_ecc_data_multi(test_data, 16U);
    printf("  结果: 测试数据已写入 ECC 通道\n");
    printf("  说明: 实际应用需写入完整的 515 字节\n");

    printf("\n步骤5：读取 BCH 校验码\n");
    uint8_t ecc_code[13];
    fmc_nand_bch_read_code(ecc_code, 13U);
    printf("  BCH 校验码（13字节）：\n  ");
    for (int i = 0; i < 13; i++) {
        printf("%02X ", ecc_code[i]);
        if ((i + 1) % 8 == 0) printf("\n  ");
    }
    printf("\n");

    printf("\n步骤6：查看 BCH 配置\n");
    print_bch_config();

    print_demo_end();
}

/**
 * @brief 场景5：BCH 自动纠错译码
 *
 * 演示 BCH 译码自动纠错完整流程：
 * 设置译码模式 → 通过 ECC 读取数据 → 等待完成 → 查看纠错结果。
 */
void scenario_5_bch_decode(void)
{
    print_demo_header("场景5：BCH 自动纠错译码");

    printf("步骤1：配置 BCH 为译码自动纠错模式\n");
    fmc_nand_init_with_bch(4U, 2U, 4U, 2U);
    fmc_nand_bch_set_mode(FMC_BCH_MODE_DECODE);
    fmc_nand_bch_set_correct_mode(FMC_BCH_CORRECT_AUTO);
    fmc_nand_bch_set_ignore_all1(true);
    printf("  结果: BCH 译码 + 自动纠错已配置\n");

    printf("\n步骤2：启动译码\n");
    fmc_nand_bch_decode_auto_start(0x20038000U);
    printf("  基地址: 0x20038000\n");
    printf("  页计数: %u\n", fmc_nand_bch_get_page_num());

    printf("\n步骤3：通过 ECC 通道读取数据\n");
    uint8_t rbuf[8];
    fmc_nand_read_ecc_data_multi(rbuf, 8U);
    printf("  结果: 已通过 ECC 通道读取数据\n");

    printf("\n步骤4：等待译码完成并查看结果\n");
    fmc_bch_result_t result;
    bool done = fmc_nand_bch_wait_decode_done(&result, 10000U);
    if (done) {
        printf("  译码状态: 完成\n");
        printf("  总错误比特数 (ERN): %u\n", result.err_total);
        printf("  数据错误比特数 (DEN): %u\n", result.err_data);
        printf("  校验码错误比特数 (CEN): %u\n", result.err_code);
        printf("  全0标志: %s\n", result.data_all0 ? "是" : "否");
        printf("  全1标志: %s\n", result.data_all1 ? "是" : "否");
        printf("  BCH失败: %s\n", result.decode_fail ? "是（超出纠错能力）" : "否");
    } else {
        printf("  译码状态: 超时（可能未连接实际器件）\n");
    }

    printf("\n步骤5：BCH 状态标志说明\n");
    printf("  ┌────────────────┬─────────────────────────────┐\n");
    printf("  │ 标志           │ 含义                        │\n");
    printf("  ├────────────────┼─────────────────────────────┤\n");
    printf("  │ ENCODE_CLR     │ 编码通道清除                │\n");
    printf("  │ SYNDROME_DONE  │ 伴随式求解完成              │\n");
    printf("  │ BM_DONE        │ 错误多项式求解完成          │\n");
    printf("  │ CORRECT_DONE   │ 纠错完成（手动模式用）      │\n");
    printf("  │ BCH_FAIL       │ 超出纠错能力                │\n");
    printf("  │ DATA_ALL1      │ 数据全1（擦除页）           │\n");
    printf("  │ DATA_ALL0      │ 数据全0                     │\n");
    printf("  └────────────────┴─────────────────────────────┘\n");

    printf("\n步骤6：手动纠错模式说明\n");
    printf("  手动纠错流程: CORRECT_DONE → 读 ERN →\n");
    printf("  读 ERRADR/ERRVEC → 原数据 XOR 错误向量 = 正确数据\n");

    print_demo_end();
}

/**
 * @brief 场景6：结构体配置方式
 *
 * 演示使用 fmc_nand_config() 和 fmc_nand_bch_config()
 * 结构体进行完整的 NAND + BCH 配置。
 */
void scenario_6_config_struct(void)
{
    print_demo_header("场景6：结构体配置方式");

    printf("步骤1：使用结构体配置 NAND 控制与时序\n");
    fmc_nand_config_t nand_cfg = {
        .endian = FMC_NAND_ENDIAN_LITTLE,
        .edo_enable = true,
        .write_protect = true,
        .rbn_interrupt = true,
        .cs = FMC_NAND_CS_0,
    };
    fmc_nand_timing_t timing = {
        .twp = 5U, .twh = 3U, .trp = 5U, .treh = 3U,
        .twhr = 6U, .trhw = 8U, .tadl = 5U,
    };
    if (fmc_nand_config(&nand_cfg, &timing)) {
        printf("  结果: NAND 配置成功\n");
    } else {
        printf("  结果: NAND 配置失败\n");
    }

    printf("\n步骤2：使用结构体配置 BCH\n");
    fmc_bch_config_t bch_cfg = {
        .bch_type = FMC_BCH_TYPE_512B_8BIT,
        .mode = FMC_BCH_MODE_DECODE,
        .correct_mode = FMC_BCH_CORRECT_AUTO,
        .decode_int_enable = true,
        .ignore_all1 = true,
    };
    if (fmc_nand_bch_config(&bch_cfg)) {
        printf("  结果: BCH 配置成功\n");
    } else {
        printf("  结果: BCH 配置失败\n");
    }

    printf("\n步骤3：验证完整配置\n");
    print_nand_config();
    print_bch_config();

    printf("\n步骤4：参数校验测试\n");
    fmc_nand_timing_t invalid_timing = {
        .twp = 20U,  // 超出范围（> 16）
        .twh = 3U, .trp = 5U, .treh = 3U,
        .twhr = 6U, .trhw = 8U, .tadl = 5U,
    };
    if (!fmc_nand_config(&nand_cfg, &invalid_timing)) {
        printf("  结果: 无效时序参数被正确拒绝（TWP=20 > 16）\n");
    }

    printf("\n步骤5：设置 BCH 纠错基地址\n");
    fmc_nand_bch_set_base_addr(0x20038000U);
    printf("  基地址: 0x%08lX\n",
           (unsigned long)fmc_nand_bch_get_base_addr());

    print_demo_end();
}

/**
 * @brief 场景7：RBN 就绪/繁忙信号监测与中断
 *
 * 演示使用 RBN 信号监测 NAND Flash 状态，
 * 以及 RBN 上升沿中断的使用。
 */
void scenario_7_rbn_monitoring(void)
{
    print_demo_header("场景7：RBN 就绪/繁忙信号监测与中断");

    printf("步骤1：初始化并查询 RBN 状态\n");
    fmc_nand_init_basic(4U, 2U, 4U, 2U);
    bool rb = fmc_nand_is_ready();
    printf("  RBN 状态: %s\n", rb ? "就绪（NAND Flash 空闲）" : "繁忙");

    printf("\n步骤2：发送复位命令并轮询 RBN\n");
    fmc_nand_send_command(NAND_CMD_RESET);
    printf("  复位命令已发送\n");

    printf("  等待就绪中...\n");
    bool ready = fmc_nand_wait_ready(5000U);
    printf("  结果: %s\n", ready ? "已就绪" : "超时");

    printf("\n步骤3：使能 RBN 上升沿中断\n");
    fmc_nand_set_rbn_interrupt(true);
    fmc_nand_register_callback(nand_isr_callback);
    NVIC_EnableIRQ(NAND_IRQn);
    nand_rbn_event = false;
    printf("  结果: RBN 中断已使能\n");

    printf("\n步骤4：触发操作并等待 RBN 中断\n");
    fmc_nand_send_command(NAND_CMD_RESET);
    printf("  复位命令已发送，等待 RBN 上升沿中断...\n");

    uint64_t t0 = system_get_tick();
    while (!nand_rbn_event && !system_elapsed(t0, 5000U)) {
        __NOP();
    }
    if (nand_rbn_event) {
        printf("  结果: RBN 上升沿中断已触发\n");
        uint32_t status = fmc_nand_get_status();
        printf("  NFMSTATUS = 0x%08lX（POS_RBN=%lu, RBN=%lu）\n",
               (unsigned long)status,
               (unsigned long)((status >> 4U) & 0xFU),
               (unsigned long)(status & 0xFU));
    } else {
        printf("  结果: 5秒内未触发中断（可能未连接实际器件）\n");
    }

    printf("\n步骤5：POS_RBN 标志操作\n");
    fmc_nand_clear_rbn_rising();
    printf("  POS_RBN 已清除\n");
    printf("  当前 RBN: %s\n", fmc_nand_is_ready() ? "就绪" : "繁忙");

    printf("\n步骤6：RBN 信号说明\n");
    printf("  ┌──────────┬──────────────────────────────┐\n");
    printf("  │ 信号     │ 说明                         │\n");
    printf("  ├──────────┼──────────────────────────────┤\n");
    printf("  │ RBN=0    │ NAND Flash 忙（正在编程/擦除）│\n");
    printf("  │ RBN=1    │ NAND Flash 就绪              │\n");
    printf("  │ POS_RBN  │ RBN 上升沿事件（写1清除）    │\n");
    printf("  │ RBNINTEN │ 上升沿中断使能               │\n");
    printf("  └──────────┴──────────────────────────────┘\n");

    // 清理
    NVIC_DisableIRQ(NAND_IRQn);
    fmc_nand_register_callback(NULL);
    fmc_nand_set_rbn_interrupt(false);

    print_demo_end();
}

/**
 * @brief 场景8：时序参数计算与优化
 *
 * 演示 NAND Flash 时序参数计算，
 * 以及根据目标读写速度优化时序参数。
 */
void scenario_8_timing_calculation(void)
{
    print_demo_header("场景8：时序参数计算与优化");

    uint32_t fmc_clk = 160000000U;  // FMC_CLK = HCLK = 160MHz
    printf("假设 FMC_CLK = %lu MHz\n", (unsigned long)fmc_clk / 1000000U);

    printf("\n步骤1：写时序参数计算\n");
    printf("  公式: T_write = (TWP + TWH) × T_FMC_CLK\n");
    printf("  T_FMC_CLK = %lu ns\n", (unsigned long)(1000000000U / fmc_clk));

    printf("\n  ┌─────┬─────┬──────────┬──────────┐\n");
    printf("  │ TWP │ TWH │ 总周期数 │ 时间(ns) │\n");
    printf("  ├─────┼─────┼──────────┼──────────┤\n");
    uint8_t write_configs[][2] = {{4,2}, {5,3}, {6,3}, {8,4}, {10,5}};
    for (int i = 0; i < 5; i++) {
        uint32_t ns = fmc_nand_calculate_write_time_ns(
            fmc_clk, write_configs[i][0], write_configs[i][1]);
        printf("  │ %-3u │ %-3u │ %-8u │ %-5lu    │\n",
               write_configs[i][0], write_configs[i][1],
               write_configs[i][0] + write_configs[i][1],
               (unsigned long)ns);
    }
    printf("  └─────┴─────┴──────────┴──────────┘\n");

    printf("\n步骤2：读时序参数计算\n");
    printf("  公式: T_read = (TRP + TREH) × T_FMC_CLK\n");

    printf("\n  ┌─────┬──────┬──────────┬──────────┐\n");
    printf("  │ TRP │ TREH │ 总周期数 │ 时间(ns) │\n");
    printf("  ├─────┼──────┼──────────┼──────────┤\n");
    uint8_t read_configs[][2] = {{4,2}, {5,3}, {6,3}, {8,4}, {10,5}};
    for (int i = 0; i < 5; i++) {
        uint32_t ns = fmc_nand_calculate_read_time_ns(
            fmc_clk, read_configs[i][0], read_configs[i][1]);
        printf("  │ %-3u │ %-4u │ %-8u │ %-5lu    │\n",
               read_configs[i][0], read_configs[i][1],
               read_configs[i][0] + read_configs[i][1],
               (unsigned long)ns);
    }
    printf("  └─────┴──────┴──────────┴──────────┘\n");

    printf("\n步骤3：读写切换时序\n");
    printf("  ┌──────────────┬──────┬──────────────────────┐\n");
    printf("  │ 参数         │ 范围 │ 说明                 │\n");
    printf("  ├──────────────┼──────┼──────────────────────┤\n");
    printf("  │ TWHR (1~32)  │ 推荐6│ WEN高到REN低等待时间 │\n");
    printf("  │ TRHW (1~64)  │ 推荐8│ REN高到WEN低等待时间 │\n");
    printf("  │ TADL (1~32)  │ 推荐5│ ALE到数据输出延时    │\n");
    printf("  └──────────────┴──────┴──────────────────────┘\n");

    printf("\n步骤4：典型 NAND Flash 时序参考\n");
    printf("  ┌────────────────┬─────┬─────┬─────┬──────┐\n");
    printf("  │ 器件类型       │ TWP │ TWH │ TRP │ TREH │\n");
    printf("  ├────────────────┼─────┼─────┼─────┼──────┤\n");
    printf("  │ 快速SLC (25ns) │ 4   │ 2   │ 4   │ 2    │\n");
    printf("  │ 中速MLC (35ns) │ 6   │ 3   │ 6   │ 3    │\n");
    printf("  │ 慢速TLC (50ns) │ 8   │ 4   │ 8   │ 4    │\n");
    printf("  └────────────────┴─────┴─────┴─────┴──────┘\n");
    printf("  注: 上表基于 FMC_CLK = 160MHz (T_cycle ≈ 6.25ns)\n");

    printf("\n步骤5：动态配置时序\n");
    fmc_nand_init_basic(4U, 2U, 4U, 2U);
    fmc_nand_timing_t t;
    fmc_nand_get_timing(&t);
    printf("  当前: TWP=%u TWH=%u TRP=%u TREH=%u\n", t.twp, t.twh, t.trp, t.treh);

    // 优化为高速器件时序
    fmc_nand_timing_t fast_timing = {
        .twp = 4U, .twh = 2U, .trp = 4U, .treh = 2U,
        .twhr = 4U, .trhw = 6U, .tadl = 4U,
    };
    fmc_nand_set_timing(&fast_timing);
    printf("  切换: 快速 SLC 器件时序\n");

    uint32_t write_ns = fmc_nand_calculate_write_time_ns(fmc_clk, 4U, 2U);
    uint32_t read_ns = fmc_nand_calculate_read_time_ns(fmc_clk, 4U, 2U);
    printf("  写访问时间 ≈ %lu ns\n", (unsigned long)write_ns);
    printf("  读访问时间 ≈ %lu ns\n", (unsigned long)read_ns);

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
    printf("FMC_NAND 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 初始化 FMC 时钟
    fmc_nand_example_init();

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_basic_init();
            break;
        case 2:
            scenario_2_edo_mode();
            break;
        case 3:
            scenario_3_read_write_flow();
            break;
        case 4:
            scenario_4_bch_encode();
            break;
        case 5:
            scenario_5_bch_decode();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_rbn_monitoring();
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
