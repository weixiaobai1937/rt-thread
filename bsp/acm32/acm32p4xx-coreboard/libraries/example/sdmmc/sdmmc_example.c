/**
 * @file sdmmc_example.c
 * @brief ACM32P4 SDMMC 驱动示例程序
 * @author ACM32P4 SDK Team
 * @version 1.0
 *
 * @details
 * 演示 14 个场景：
 * 1. 主机控制器初始化
 * 2. 卡检测
 * 3. SD 卡识别
 * 4. SD 卡信息查询
 * 5. SD 卡单块读写（轮询）
 * 6. SD 卡多块读写（轮询）
 * 7. SD 卡 4 位总线
 * 8. SD 卡中断模式读写
 * 9. SD 卡 IDMA 模式读写
 * 10. SD 卡擦除
 * 11. SDIO 卡通信
 * 12. MMC 卡初始化与读写
 * 13. eMMC DDR 模式
 * 14. 错误处理与超时
 */

#include "acm32p4.h"
#include "hardware/sdmmc.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...)                 SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO            1
#define SDMMC_TEST_BLOCK_SIZE       512U
#define SDMMC_TEST_BLOCK_COUNT      4U
#define SDMMC_TEST_CLK_DIV          4U

//===========================================
// 全局变量
//===========================================

static volatile bool s_transfer_done = false;
static volatile bool s_transfer_success = false;
static volatile bool s_card_inserted = false;

static uint32_t s_test_buffer[SDMMC_TEST_BLOCK_SIZE * SDMMC_TEST_BLOCK_COUNT / 4U];
static uint32_t s_verify_buffer[SDMMC_TEST_BLOCK_SIZE * SDMMC_TEST_BLOCK_COUNT / 4U];
static sdmmc_card_info_t s_card_info;

//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

static void on_transfer_complete(bool success)
{
    s_transfer_done = true;
    s_transfer_success = success;
}

static void on_irq_event(sdmmc_irq_t event)
{
    if (event & SDMMC_IRQ_CDET) {
        s_card_inserted = sdmmc_is_card_inserted();
    }
}

static void fill_test_pattern(uint32_t *buf, uint32_t word_count, uint32_t seed)
{
    uint32_t i;
    for (i = 0U; i < word_count; i++) {
        buf[i] = seed + i;
    }
}

static bool verify_data(const uint32_t *src, const uint32_t *dst, uint32_t word_count)
{
    uint32_t i;
    for (i = 0U; i < word_count; i++) {
        if (src[i] != dst[i]) {
            printf("  数据不匹配 @ word %lu: 期望 0x%08lX, 实际 0x%08lX\n",
                   i, src[i], dst[i]);
            return false;
        }
    }
    return true;
}

static void sdmmc_basic_init(void)
{
    sdmmc_init_config_t cfg = {
        .clk_div = SDMMC_TEST_CLK_DIV,
        .bus_width = SDMMC_BUS_WIDTH_1BIT,
        .transfer_mode = SDMMC_TRANSFER_MODE_POLLING,
        .enable_ddr = false,
        .enable_hw_reset = false,
        .irq_callback = on_irq_event,
    };
    sdmmc_init(&cfg);
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：主机控制器初始化
 */
static void scenario_1_host_init(void)
{
    print_separator();
    printf("=== 场景1：主机控制器初始化 ===\n\n");

    printf("步骤1：初始化 SDMMC 主机控制器\n");
    sdmmc_basic_init();
    printf("  结果: 初始化成功\n");

    printf("\n步骤2：检查寄存器状态\n");
    printf("  CTRL     = 0x%08lX\n", (uint32_t)SDMMC->CTRL);
    printf("  CLKDIV   = 0x%08lX\n", (uint32_t)SDMMC->CLKDIV);
    printf("  CLKENA   = 0x%08lX\n", (uint32_t)SDMMC->CLKENA);
    printf("  CTYPE    = 0x%08lX\n", (uint32_t)SDMMC->CTYPE);
    printf("  TMOUT    = 0x%08lX\n", (uint32_t)SDMMC->TMOUT);
    printf("  FIFOTH   = 0x%08lX\n", (uint32_t)SDMMC->FIFOTH);

    printf("\n步骤3：发送初始化序列（80 个时钟）\n");
    sdmmc_send_init_sequence();
    printf("  结果: 初始化序列已发送\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景2：卡检测
 */
static void scenario_2_card_detect(void)
{
    print_separator();
    printf("=== 场景2：卡检测 ===\n\n");

    sdmmc_basic_init();

    printf("步骤1：检查卡是否插入\n");
    if (sdmmc_is_card_inserted()) {
        printf("  结果: 卡已插入\n");
    } else {
        printf("  结果: 未检测到卡（请插入 SD 卡）\n");
    }

    printf("\n步骤2：使能卡检测中断\n");
    sdmmc_irq_enable(SDMMC_IRQ_CDET);
    printf("  结果: 卡检测中断已使能\n");

    printf("\n步骤3：轮询卡状态（5 秒）\n");
    uint32_t start = system_get_tick();
    while (system_elapsed(start) < 5000U) {
        if (sdmmc_is_card_inserted()) {
            printf("  卡已插入！\n");
            break;
        }
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景3：SD 卡识别
 */
static void scenario_3_sd_identify(void)
{
    print_separator();
    printf("=== 场景3：SD 卡识别 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_is_card_inserted()) {
        printf("错误：未检测到 SD 卡\n");
        print_separator();
        return;
    }

    printf("步骤1：CMD0 复位卡\n");
    if (sdmmc_cmd0_go_idle()) {
        printf("  结果: 成功\n");
    }

    printf("\n步骤2：CMD8 检查接口条件\n");
    uint32_t resp;
    if (sdmmc_cmd8_send_if_cond(1U, 0xAAU, &resp)) {
        printf("  结果: 成功, 响应 = 0x%08lX\n", resp);
    }

    printf("\n步骤3：ACMD41 初始化（等待卡就绪）\n");
    uint32_t ocr;
    uint32_t timeout = 1000U;
    do {
        sdmmc_acmd41_send_op_cond(true, &ocr);
        if (--timeout == 0U) {
            printf("  结果: 超时\n");
            print_separator();
            return;
        }
    } while ((ocr & (1UL << 31)) == 0U);
    printf("  结果: 就绪, OCR = 0x%08lX\n", ocr);

    printf("\n步骤4：CMD2 获取 CID\n");
    uint32_t cid[4];
    if (sdmmc_cmd2_all_send_cid(cid)) {
        printf("  结果: CID[0]=0x%08lX, CID[1]=0x%08lX, CID[2]=0x%08lX, CID[3]=0x%08lX\n",
               cid[0], cid[1], cid[2], cid[3]);
    }

    printf("\n步骤5：CMD3 获取 RCA\n");
    uint32_t rca;
    if (sdmmc_cmd3_send_relative_addr(&rca)) {
        printf("  结果: RCA = 0x%08lX\n", rca);
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景4：SD 卡信息查询
 */
static void scenario_4_sd_info(void)
{
    print_separator();
    printf("=== 场景4：SD 卡信息查询 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_is_card_inserted()) {
        printf("错误：未检测到 SD 卡\n");
        print_separator();
        return;
    }

    printf("步骤1：初始化 SD 卡\n");
    if (!sdmmc_sd_init(&s_card_info)) {
        printf("  结果: 初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果: 成功\n");

    printf("\n步骤2：获取卡信息\n");
    printf("  卡类型:     %s\n",
           s_card_info.card_type == SDMMC_CARD_TYPE_SD ? "SD" :
           s_card_info.card_type == SDMMC_CARD_TYPE_MMC ? "MMC" : "其他");
    printf("  RCA:        0x%08lX\n", s_card_info.rca);
    printf("  OCR:        0x%08lX\n", s_card_info.ocr);
    printf("  高容量:     %s\n", s_card_info.high_capacity ? "是" : "否");
    printf("  块大小:     %lu 字节\n", s_card_info.block_size);
    printf("  容量:       %lu 块\n", s_card_info.capacity);

    uint64_t total_bytes = sdmmc_sd_get_capacity();
    printf("  总容量:     %llu MB\n", (unsigned long long)(total_bytes / 1024ULL / 1024ULL));

    printf("\n步骤3：CMD13 读取卡状态\n");
    uint32_t status;
    if (sdmmc_cmd13_send_status(s_card_info.rca, &status)) {
        printf("  卡状态:     0x%08lX\n", status);
        printf("  卡 FSM:     %d\n", sdmmc_get_card_state(status));
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景5：SD 卡单块读写（轮询）
 */
static void scenario_5_sd_single_rw(void)
{
    print_separator();
    printf("=== 场景5：SD 卡单块读写（轮询） ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤1：写入测试数据到块 0\n");
    fill_test_pattern(s_test_buffer, SDMMC_TEST_BLOCK_SIZE / 4U, 0xDEADBEEFU);
    if (sdmmc_sd_write_blocks(0U, s_test_buffer, 1U)) {
        printf("  结果: 写入成功\n");
    } else {
        printf("  结果: 写入失败\n");
        print_separator();
        return;
    }

    printf("\n步骤2：读回块 0 数据\n");
    if (sdmmc_sd_read_blocks(0U, s_verify_buffer, 1U)) {
        printf("  结果: 读取成功\n");
    } else {
        printf("  结果: 读取失败\n");
        print_separator();
        return;
    }

    printf("\n步骤3：验证数据一致性\n");
    if (verify_data(s_test_buffer, s_verify_buffer, SDMMC_TEST_BLOCK_SIZE / 4U)) {
        printf("  结果: 数据一致，读写验证通过！\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景6：SD 卡多块读写（轮询）
 */
static void scenario_6_sd_multi_rw(void)
{
    print_separator();
    printf("=== 场景6：SD 卡多块读写（轮询） ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    uint32_t word_count = SDMMC_TEST_BLOCK_SIZE * SDMMC_TEST_BLOCK_COUNT / 4U;

    printf("步骤1：写入 %lu 块测试数据到块 0\n", SDMMC_TEST_BLOCK_COUNT);
    fill_test_pattern(s_test_buffer, word_count, 0xCAFEBABEU);
    if (sdmmc_sd_write_blocks(0U, s_test_buffer, SDMMC_TEST_BLOCK_COUNT)) {
        printf("  结果: 写入成功\n");
    } else {
        printf("  结果: 写入失败\n");
        print_separator();
        return;
    }

    printf("\n步骤2：读回 %lu 块数据\n", SDMMC_TEST_BLOCK_COUNT);
    if (sdmmc_sd_read_blocks(0U, s_verify_buffer, SDMMC_TEST_BLOCK_COUNT)) {
        printf("  结果: 读取成功\n");
    } else {
        printf("  结果: 读取失败\n");
        print_separator();
        return;
    }

    printf("\n步骤3：验证数据一致性\n");
    if (verify_data(s_test_buffer, s_verify_buffer, word_count)) {
        printf("  结果: 数据一致，多块读写验证通过！\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景7：SD 卡 4 位总线
 */
static void scenario_7_sd_4bit(void)
{
    print_separator();
    printf("=== 场景7：SD 卡 4 位总线 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤1：切换到 4 位总线\n");
    sdmmc_config_bus_width(SDMMC_BUS_WIDTH_4BIT);
    if (sdmmc_acmd6_set_bus_width(s_card_info.rca, SDMMC_BUS_WIDTH_4BIT)) {
        printf("  结果: 已切换到 4 位模式\n");
    }

    printf("\n步骤2：4 位模式下多块读写测试\n");
    uint32_t word_count = SDMMC_TEST_BLOCK_SIZE * SDMMC_TEST_BLOCK_COUNT / 4U;
    fill_test_pattern(s_test_buffer, word_count, 0x12345678U);

    if (!sdmmc_sd_write_blocks(0U, s_test_buffer, SDMMC_TEST_BLOCK_COUNT)) {
        printf("  结果: 写入失败\n");
        print_separator();
        return;
    }
    if (!sdmmc_sd_read_blocks(0U, s_verify_buffer, SDMMC_TEST_BLOCK_COUNT)) {
        printf("  结果: 读取失败\n");
        print_separator();
        return;
    }
    if (verify_data(s_test_buffer, s_verify_buffer, word_count)) {
        printf("  结果: 4 位总线读写验证通过！\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景8：SD 卡中断模式读写
 */
static void scenario_8_sd_interrupt(void)
{
    print_separator();
    printf("=== 场景8：SD 卡中断模式读写 ===\n\n");

    /* 使用中断模式的初始化 */
    sdmmc_init_config_t cfg = {
        .clk_div = SDMMC_TEST_CLK_DIV,
        .bus_width = SDMMC_BUS_WIDTH_4BIT,
        .transfer_mode = SDMMC_TRANSFER_MODE_INTERRUPT,
        .enable_ddr = false,
        .enable_hw_reset = false,
        .irq_callback = on_irq_event,
    };
    sdmmc_init(&cfg);
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤1：使能数据传输中断\n");
    sdmmc_irq_enable(SDMMC_IRQ_TXDR | SDMMC_IRQ_RXDR | SDMMC_IRQ_DTO);

    printf("步骤2：中断模式下写入数据\n");
    /* 注意：当前 sdmmc_send_cmd 实现使用轮询等待 CDONE，
     * 中断模式的数据传输需要在 send_cmd 内部扩展。
     * 此处演示基本中断使能配置。
     */
    printf("  结果: 中断已配置（完整中断传输需扩展 send_cmd 内部逻辑）\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景9：SD 卡 IDMA 模式读写
 */
static void scenario_9_sd_idma(void)
{
    print_separator();
    printf("=== 场景9：SD 卡 IDMA 模式读写 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤1：配置 IDMAC\n");
    if (sdmmc_dma_config((uint32_t)s_test_buffer, 3U)) {
        printf("  结果: IDMAC 配置成功（PBL=8）\n");
    }

    printf("\n步骤2：启动 IDMAC\n");
    sdmmc_dma_start();
    printf("  结果: IDMAC 已启动\n");

    printf("\n步骤3：检查 IDMA 状态\n");
    uint32_t idma_status = sdmmc_dma_get_status();
    printf("  IDSTS = 0x%08lX\n", idma_status);

    printf("\n步骤4：停止 IDMAC\n");
    sdmmc_dma_stop();
    printf("  结果: IDMAC 已停止\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景10：SD 卡擦除
 */
static void scenario_10_sd_erase(void)
{
    print_separator();
    printf("=== 场景10：SD 卡擦除 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    if (!sdmmc_sd_init(&s_card_info)) {
        printf("错误：SD 卡初始化失败\n");
        print_separator();
        return;
    }

    printf("步骤1：写入测试数据到块 100\n");
    fill_test_pattern(s_test_buffer, SDMMC_TEST_BLOCK_SIZE / 4U, 0xAAAAAAAAU);
    if (!sdmmc_sd_write_blocks(100U, s_test_buffer, 1U)) {
        printf("  结果: 写入失败\n");
        print_separator();
        return;
    }
    printf("  结果: 写入成功\n");

    printf("\n步骤2：擦除块 100\n");
    if (sdmmc_sd_erase(100U, 100U)) {
        printf("  结果: 擦除成功\n");
    } else {
        printf("  结果: 擦除失败\n");
        print_separator();
        return;
    }

    printf("\n步骤3：读回块 100 验证擦除\n");
    if (sdmmc_sd_read_blocks(100U, s_verify_buffer, 1U)) {
        printf("  结果: 读取成功\n");
        /* 擦除后数据应为 0xFF 或 0x00（取决于卡实现） */
        printf("  首字: 0x%08lX\n", s_verify_buffer[0]);
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景11：SDIO 卡通信
 */
static void scenario_11_sdio(void)
{
    print_separator();
    printf("=== 场景11：SDIO 卡通信 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    printf("步骤1：初始化 SDIO 卡\n");
    sdmmc_card_info_t sdio_card;
    if (sdmmc_sdio_init(&sdio_card)) {
        printf("  结果: SDIO 卡初始化成功\n");
        printf("  RCA: 0x%08lX\n", sdio_card.rca);
    } else {
        printf("  结果: SDIO 卡初始化失败（可能无 SDIO 卡）\n");
        print_separator();
        return;
    }

    printf("\n步骤2：读取 CCCR (CMD52, func=0, addr=0x00)\n");
    uint8_t ccrc;
    if (sdmmc_sdio_read_direct(0U, 0x00U, &ccrc)) {
        printf("  结果: CCCR = 0x%02X\n", ccrc);
    }

    printf("\n步骤3：使能 SDIO 中断\n");
    sdmmc_sdio_enable_irq();
    printf("  结果: SDIO 中断已使能\n");

    printf("\n步骤4：禁用 SDIO 中断\n");
    sdmmc_sdio_disable_irq();
    printf("  结果: SDIO 中断已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景12：MMC 卡初始化与读写
 */
static void scenario_12_mmc(void)
{
    print_separator();
    printf("=== 场景12：MMC 卡初始化与读写 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    printf("步骤1：初始化 MMC 卡\n");
    sdmmc_card_info_t mmc_card;
    if (sdmmc_mmc_init(&mmc_card)) {
        printf("  结果: MMC 卡初始化成功\n");
        printf("  RCA: 0x%08lX, OCR: 0x%08lX\n", mmc_card.rca, mmc_card.ocr);
        printf("  容量: %lu 块, 块大小: %lu 字节\n", mmc_card.capacity, mmc_card.block_size);
    } else {
        printf("  结果: MMC 卡初始化失败（可能无 MMC 卡）\n");
        print_separator();
        return;
    }

    printf("\n步骤2：MMC 卡多块读写\n");
    uint32_t word_count = SDMMC_TEST_BLOCK_SIZE * 2U / 4U;
    fill_test_pattern(s_test_buffer, word_count, 0xMMC00000U);
    if (sdmmc_mmc_write_blocks(0U, s_test_buffer, 2U)) {
        printf("  结果: 写入成功\n");
    }
    if (sdmmc_mmc_read_blocks(0U, s_verify_buffer, 2U)) {
        printf("  结果: 读取成功\n");
    }
    if (verify_data(s_test_buffer, s_verify_buffer, word_count)) {
        printf("  结果: MMC 读写验证通过！\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景13：eMMC DDR 模式
 */
static void scenario_13_emmc_ddr(void)
{
    print_separator();
    printf("=== 场景13：eMMC DDR 模式 ===\n\n");

    sdmmc_basic_init();
    sdmmc_send_init_sequence();

    printf("步骤1：初始化 MMC 卡\n");
    sdmmc_card_info_t mmc_card;
    if (!sdmmc_mmc_init(&mmc_card)) {
        printf("  结果: MMC 卡初始化失败\n");
        print_separator();
        return;
    }
    printf("  结果: 成功\n");

    printf("\n步骤2：使能 eMMC DDR 模式\n");
    sdmmc_ddr_enable(true);
    if (sdmmc_emmc_set_ddr(true)) {
        printf("  结果: DDR 模式已使能\n");
    }

    printf("\n步骤3：切换到 8 位总线\n");
    sdmmc_config_bus_width(SDMMC_BUS_WIDTH_8BIT);
    printf("  结果: 8 位总线已配置\n");

    printf("\n步骤4：DDR 8 位模式下读写测试\n");
    fill_test_pattern(s_test_buffer, SDMMC_TEST_BLOCK_SIZE / 4U, 0xDDR80000U);
    if (sdmmc_mmc_write_blocks(0U, s_test_buffer, 1U) &&
        sdmmc_mmc_read_blocks(0U, s_verify_buffer, 1U)) {
        if (verify_data(s_test_buffer, s_verify_buffer, SDMMC_TEST_BLOCK_SIZE / 4U)) {
            printf("  结果: DDR 8 位读写验证通过！\n");
        }
    }

    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景14：错误处理与超时
 */
static void scenario_14_error_handling(void)
{
    print_separator();
    printf("=== 场景14：错误处理与超时 ===\n\n");

    sdmmc_basic_init();

    printf("步骤1：无卡时发送 CMD8（应超时）\n");
    if (!sdmmc_is_card_inserted()) {
        uint32_t resp;
        if (!sdmmc_cmd8_send_if_cond(1U, 0xAAU, &resp)) {
            printf("  结果: 预期超时，错误处理正确\n");
        }
    }

    printf("\n步骤2：无效命令索引测试\n");
    /* 发送不存在的命令索引 63 */
    bool result = sdmmc_send_cmd(63U, 0U, SDMMC_RESPONSE_SHORT, NULL, 100U);
    printf("  结果: %s（预期超时）\n", result ? "意外成功" : "超时");

    printf("\n步骤3：清除所有中断标志\n");
    sdmmc_irq_clear(SDMMC_IRQ_ALL);
    sdmmc_irq_t status = sdmmc_get_raw_irq_status();
    printf("  结果: RINTSTS = 0x%08lX（应全部清除）\n", (uint32_t)status);

    printf("\n步骤4：控制器复位恢复\n");
    sdmmc_reset_controller();
    sdmmc_reset_fifo();
    printf("  结果: 控制器和 FIFO 已复位\n");

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
    printf("SDMMC 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1:  scenario_1_host_init();       break;
        case 2:  scenario_2_card_detect();     break;
        case 3:  scenario_3_sd_identify();     break;
        case 4:  scenario_4_sd_info();         break;
        case 5:  scenario_5_sd_single_rw();    break;
        case 6:  scenario_6_sd_multi_rw();     break;
        case 7:  scenario_7_sd_4bit();         break;
        case 8:  scenario_8_sd_interrupt();    break;
        case 9:  scenario_9_sd_idma();         break;
        case 10: scenario_10_sd_erase();       break;
        case 11: scenario_11_sdio();           break;
        case 12: scenario_12_mmc();            break;
        case 13: scenario_13_emmc_ddr();       break;
        case 14: scenario_14_error_handling(); break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            break;
    }

    while (1) {
        delay_ms(1000U);
    }
}

//===========================================
// 中断服务函数
//===========================================

void SDMMC_IRQHandler(void)
{
    sdmmc_irq_t irq = sdmmc_get_masked_irq_status();

    if ((irq & SDMMC_IRQ_CDET) != 0U) {
        s_card_inserted = sdmmc_is_card_inserted();
    }

    sdmmc_irq_clear(irq);
}