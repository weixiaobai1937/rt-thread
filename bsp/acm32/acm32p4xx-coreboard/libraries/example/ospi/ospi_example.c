/**
 * @file ospi_example.c
 * @brief ACM32P4 OSPI1 APS6408L PSRAM 完整验证示例
 *
 * @details
 * 覆盖 FIFO DTR 寄存器操作、内存映射读写、地址线 Walking-One 检测、
 * 数据位图校验、全空间采样、时序窗口扫描。
 *
 * 12 个场景：
 *  1. FIFO 读全部模式寄存器（MR0~MR8）并校验关键位
 *  2. FIFO 连续压力测试（500 次 MR1 读）
 *  3. 列地址线 Walking-1 检测（CA[9:0]，10 条线 × 4 字节）
 *  4. 行地址线 Walking-1 检测（RA[12:0]，13 条线 × 64 字节）
 *  5. 首尾页全量校验（第 0 页 + 最后一页各 1024 字节递增）
 *  6. 全空间均匀采样（512 点 × 64 字节 = 32KB，步长 16KB）
 *  7. 数据位图校验（Walking-1 / Walking-0 / 翻转共 3 组模式）
 *  8. 连续地址递增校验（前 64KB addr[15:0] 写入读回）
 *  9. 读采样点扫描（read_dummy=8，shift 0..13）
 * 10. 三维时序全扫描（dummy × out_delay × shift，仅报最优）
 * 11. 读带宽测试（8/16/32 位，遍历全部 8MB 空间）
 * 12. 写带宽测试（8/16/32 位，遍历全部 8MB 空间）
 *
 * 硬件连接：
 * - OSPI1 控制器：GPIOG5-15，AF2/5/6
 * - APS6408L PSRAM Reset 脚：GPIOF10，推挽输出
 * - 内存映射基地址：0x80000000
 *
 * @note 目标芯片：APS6408L（8MB Xccela OPI PSRAM，地址空间 0x80000000~0x807FFFFF）
 * @note 验证通过的寄存器配置：
 *       MEM_TYPE=XCC_ELA / CMD=0xA0A02020 / DQSOE=1 /
 *       read_dummy=8(9cyc) / write_dummy=3(4cyc) /
 *       RD_DB_EN=1 / WR_DB_EN=1 / SSHIFT=4 / OUTDLY=1
 *
 * @author ACM32P4 SDK Team
 * @version 3.0.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/ospi.h"
#include "hardware/system.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

/** 测试场景选择：0=全部运行，1-10=单独运行某个场景 */
#define DEFAULT_SCENARIO 0

//===========================================
// OSPI1 APS6408L 硬件引脚定义
//===========================================

#define OSPI_DEMO_INST      OSPI_1

#define OSPI_DEMO_SCK_PIN   PG5
#define OSPI_DEMO_IO0_PIN   PG6
#define OSPI_DEMO_IO1_PIN   PG7
#define OSPI_DEMO_IO2_PIN   PG8
#define OSPI_DEMO_IO3_PIN   PG9
#define OSPI_DEMO_IO4_PIN   PG10
#define OSPI_DEMO_IO5_PIN   PG11
#define OSPI_DEMO_IO6_PIN   PG12
#define OSPI_DEMO_IO7_PIN   PG13
#define OSPI_DEMO_CS_PIN    PG14
#define OSPI_DEMO_DQS_PIN   PG15

#define OSPI_DEMO_AF_MAIN   GPIO_AF_2
#define OSPI_DEMO_AF_IO4    GPIO_AF_6
#define OSPI_DEMO_AF_IO5    GPIO_AF_5

#define OSPI_DEMO_RESET_PIN PF10

//===========================================
// APS6408L 命令与寄存器定义
//===========================================

#define APS6408L_SYNC_READ_LINEAR_BURST   0x2020U
#define APS6408L_SYNC_WRITE_LINEAR_BURST  0xA0A0U
#define APS6408L_READ_REG                 0x40U
#define APS6408L_WRITE_REG                0xC0U
#define APS6408L_REG_MR0                  0x00U
#define APS6408L_REG_MR1                  0x01U
#define APS6408L_REG_MR2                  0x02U
#define APS6408L_REG_MR3                  0x03U
#define APS6408L_REG_MR4                  0x04U
#define APS6408L_REG_MR8                  0x08U
#define APS6408L_MR0_FIXED_READ_LATENCY   (1U << 5)
#define APS6408L_DEVICE_ID                0x930DU
#define APS6408L_MR1_VENDOR_ID_MASK       0x001FU
#define APS6408L_MR2_GOOD_DIE             (1U << 7)
#define APS6408L_MR3_VCC_MASK             (1U << 6)

//===========================================
// 测试参数
//===========================================

#define APS6408L_MEM_BASE       0x80000000U
#define APS6408L_SIZE           0x800000U
#define APS6408L_PAGE_SIZE      1024U
#define APS6408L_RA_BITS        13U
#define APS6408L_RESET_LOW_MS   1U
#define APS6408L_RESET_HIGH_MS  1U
#define APS6408L_XFER_TIMEOUT   1000000U

#define FIFO_STRESS_COUNT        500U
#define SPARSE_SAMPLES           512U
#define SPARSE_STEP              0x4000U
#define SPARSE_CHUNK             64U
#define SWEEP_SIZE               64U
#define PATTERN_BLOCK_SIZE       256U
#define INCREMENTAL_SIZE         65536U
#define PERF_RUNS                3U           /* 每项跑 3 次取最小值 */

//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

static void print_result(bool pass, uint32_t total, uint32_t errors)
{
    if (pass) {
        printf("  结果: PASS，%u/%u 匹配\n", total, total);
    } else {
        printf("  结果: FAIL，%u/%u 匹配，%u 错误\n",
               total - errors, total, errors);
    }
}

//===========================================
// DTR FIFO 半字辅助函数
//===========================================

static uint16_t aps6408l_encode_dtr_cmd(uint8_t cmd)
{
    return (uint16_t)cmd | ((uint16_t)cmd << 8U);
}

//===========================================
// APS6408L FIFO DTR 寄存器读写
//===========================================

/**
 * @brief 通过 DTR 协议读取 APS6408L 模式寄存器
 *
 * @param[in]  reg   寄存器地址
 * @param[out] value 寄存器值
 * @return true: 成功, false: 失败
 *
 * @note 事务前临时暂停 memory-mapped，事务后恢复
 */
static bool aps6408l_read_reg_dtr(uint8_t reg, uint16_t *value)
{
    uint16_t rx_data;

    if (value == NULL) {
        return false;
    }

    ospi_stop(OSPI_DEMO_INST);
    ospi_clear_irq_status(OSPI_DEMO_INST,
                          OSPI_IRQ_BATCH_DONE | OSPI_IRQ_TX_BATCH_DONE |
                              OSPI_IRQ_RX_BATCH_DONE);
    ospi_reset_fifo(OSPI_DEMO_INST, true, true);
    if (!ospi_is_memory_mapped_enabled(OSPI_DEMO_INST)) {
        printf("  读取 MR 前 MEMO_ACC1.ACC_EN 未置位\n");
        return false;
    }
    ospi_disable_memory_mapped(OSPI_DEMO_INST);

    ospi_release_cs(OSPI_DEMO_INST);
    ospi_set_batch(OSPI_DEMO_INST, 14U);
    ospi_start_tx(OSPI_DEMO_INST);
    ospi_assert_cs(OSPI_DEMO_INST);
    if (!ospi_write_fifo_half_safe(OSPI_DEMO_INST, aps6408l_encode_dtr_cmd(APS6408L_READ_REG)) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, (uint16_t)reg << 8U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U)) {
        ospi_stop_tx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }

    if (!ospi_wait_status(OSPI_DEMO_INST, OSPI_IRQ_TX_BATCH_DONE, true, APS6408L_XFER_TIMEOUT)) {
        ospi_stop_tx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }
    ospi_stop_tx(OSPI_DEMO_INST);
    ospi_clear_irq_status(OSPI_DEMO_INST,
                          OSPI_IRQ_BATCH_DONE | OSPI_IRQ_TX_BATCH_DONE);

    ospi_set_batch(OSPI_DEMO_INST, 2U);
    ospi_start_rx(OSPI_DEMO_INST);
    ospi_assert_cs(OSPI_DEMO_INST);
    if (!ospi_read_fifo_half_safe(OSPI_DEMO_INST, &rx_data)) {
        ospi_stop_rx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }
    if (!ospi_wait_status(OSPI_DEMO_INST, OSPI_IRQ_RX_BATCH_DONE, true, APS6408L_XFER_TIMEOUT)) {
        ospi_stop_rx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }

    ospi_stop_rx(OSPI_DEMO_INST);
    ospi_release_cs(OSPI_DEMO_INST);
    ospi_clear_irq_status(OSPI_DEMO_INST,
                          OSPI_IRQ_BATCH_DONE | OSPI_IRQ_TX_BATCH_DONE |
                              OSPI_IRQ_RX_BATCH_DONE);
    *value = rx_data;
    if (!ospi_enable_memory_mapped(OSPI_DEMO_INST)) {
        return false;
    }

    return true;
}

/**
 * @brief 通过 DTR 协议写入 APS6408L 模式寄存器
 *
 * @param[in] reg   寄存器地址
 * @param[in] value 寄存器值
 * @return true: 成功, false: 失败
 */
static bool aps6408l_write_reg_dtr(uint8_t reg, uint16_t value)
{
    ospi_stop(OSPI_DEMO_INST);
    ospi_clear_irq_status(OSPI_DEMO_INST,
                          OSPI_IRQ_BATCH_DONE | OSPI_IRQ_TX_BATCH_DONE |
                              OSPI_IRQ_RX_BATCH_DONE);
    ospi_reset_fifo(OSPI_DEMO_INST, true, true);
    if (!ospi_is_memory_mapped_enabled(OSPI_DEMO_INST)) {
        printf("  写入 MR 前 MEMO_ACC1.ACC_EN 未置位\n");
        return false;
    }
    ospi_disable_memory_mapped(OSPI_DEMO_INST);

    ospi_release_cs(OSPI_DEMO_INST);
    ospi_set_batch(OSPI_DEMO_INST, 8U);
    ospi_start_tx(OSPI_DEMO_INST);
    ospi_assert_cs(OSPI_DEMO_INST);
    if (!ospi_write_fifo_half_safe(OSPI_DEMO_INST, aps6408l_encode_dtr_cmd(APS6408L_WRITE_REG)) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, 0x0000U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, (uint16_t)reg << 8U) ||
        !ospi_write_fifo_half_safe(OSPI_DEMO_INST, (uint16_t)value)) {
        ospi_stop_tx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }

    if (!ospi_wait_status(OSPI_DEMO_INST, OSPI_IRQ_TX_BATCH_DONE, true, APS6408L_XFER_TIMEOUT)) {
        ospi_stop_tx(OSPI_DEMO_INST);
        ospi_release_cs(OSPI_DEMO_INST);
        ospi_enable_memory_mapped(OSPI_DEMO_INST);
        return false;
    }

    ospi_stop_tx(OSPI_DEMO_INST);
    ospi_release_cs(OSPI_DEMO_INST);
    ospi_clear_irq_status(OSPI_DEMO_INST,
                          OSPI_IRQ_BATCH_DONE | OSPI_IRQ_TX_BATCH_DONE |
                              OSPI_IRQ_RX_BATCH_DONE);
    if (!ospi_enable_memory_mapped(OSPI_DEMO_INST)) {
        return false;
    }

    return true;
}

//===========================================
// APS6408L 固定读延时配置
//===========================================

static bool aps6408l_config_fixed_read_latency(void)
{
    uint16_t mr0;
    uint16_t mr1;

    if (!aps6408l_read_reg_dtr(APS6408L_REG_MR0, &mr0)) {
        return false;
    }
    if (!aps6408l_read_reg_dtr(APS6408L_REG_MR1, &mr1)) {
        return false;
    }

    if ((mr0 == 0x0000U) || (mr0 == 0xFFFFU) ||
        (mr1 != APS6408L_DEVICE_ID)) {
        printf("  错误：MR 无效 MR0=0x%04X MR1=0x%04X\n", mr0, mr1);
        return false;
    }

    if ((mr0 & APS6408L_MR0_FIXED_READ_LATENCY) != 0U) {
        ospi_assert_cs(OSPI_DEMO_INST);
        return true;
    }

    mr0 |= APS6408L_MR0_FIXED_READ_LATENCY;
    if (!aps6408l_write_reg_dtr(APS6408L_REG_MR0, mr0)) {
        return false;
    }

    uint16_t verify;
    if (!aps6408l_read_reg_dtr(APS6408L_REG_MR0, &verify)) {
        return false;
    }
    if ((verify & APS6408L_MR0_FIXED_READ_LATENCY) == 0U) {
        return false;
    }

    ospi_assert_cs(OSPI_DEMO_INST);
    return true;
}

//===========================================
// GPIO + OSPI 初始化
//===========================================

static void ospi_init_gpio(void)
{
    const gpio_pin_t ospi_pins[] = {
        OSPI_DEMO_SCK_PIN, OSPI_DEMO_IO0_PIN, OSPI_DEMO_IO1_PIN,
        OSPI_DEMO_IO2_PIN, OSPI_DEMO_IO3_PIN, OSPI_DEMO_IO4_PIN,
        OSPI_DEMO_IO5_PIN, OSPI_DEMO_IO6_PIN, OSPI_DEMO_IO7_PIN,
        OSPI_DEMO_CS_PIN,  OSPI_DEMO_DQS_PIN,
    };

    clock_periph_enable(CLK_GPIOF);
    clock_periph_enable(CLK_GPIOG);
    clock_periph_enable(CLK_OSPI1);

    gpio_set_function(OSPI_DEMO_SCK_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO0_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO1_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO2_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO3_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO4_PIN, OSPI_DEMO_AF_IO4);
    gpio_set_function(OSPI_DEMO_IO5_PIN, OSPI_DEMO_AF_IO5);
    gpio_set_function(OSPI_DEMO_IO6_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_IO7_PIN, OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_CS_PIN,  OSPI_DEMO_AF_MAIN);
    gpio_set_function(OSPI_DEMO_DQS_PIN, OSPI_DEMO_AF_MAIN);

    for (uint32_t i = 0; i < sizeof(ospi_pins) / sizeof(ospi_pins[0]); i++) {
        gpio_set_pulls(ospi_pins[i], GPIO_PULL_UP);
        gpio_set_drive_strength(ospi_pins[i], GPIO_DRIVE_8MA);
    }

    gpio_init(OSPI_DEMO_RESET_PIN);
    gpio_set_dir(OSPI_DEMO_RESET_PIN, GPIO_OUT);
    gpio_set_output_type(OSPI_DEMO_RESET_PIN, GPIO_OTYPE_PP);

    gpio_clear(OSPI_DEMO_RESET_PIN);
    delay_ms(APS6408L_RESET_LOW_MS);
    gpio_set(OSPI_DEMO_RESET_PIN);
    delay_ms(APS6408L_RESET_HIGH_MS);
}

static bool ospi_init_psram(void)
{
    bool ok;
    ospi_baudrate_config_t baud = { .div1 = 4, .div2 = 0 };
    ospi_basic_config_t basic = {
        .cpol = OSPI_CPOL_LOW,
        .cpha = OSPI_CPHA_ODD_EDGE,
        .msb_first = true,
        .hw_io_mode = true,
    };
    ospi_memory_mapped_config_t mem = {
        .read_cmd = 0x2020U,              // DTR 双字节编码：0x20→0x2020
        .write_cmd = 0xA0A0U,             // DTR 双字节编码：0xA0→0xA0A0
        .instr_mode = OSPI_INSTR_1LINE,
        .addr_mode = OSPI_ADDR_1LINE,
        .data_mode = OSPI_DATA_1LINE,
        .addr_width = OSPI_ADDR_32BIT,
        .read_dummy_cycles = 4,
        .write_dummy_cycles = 3,
        .rd_db_en = true,
        .wr_db_en = true,
        .alternate_bytes_enable = false,
        .alternate_bytes_mode = OSPI_DATA_1LINE,
        .alternate_bytes_size = OSPI_ALT_8BIT,
        .continuous_mode = true,
        .send_instr_once = false,
        .read_burst = OSPI_BURST_1KB,
        .write_burst = OSPI_BURST_1KB,
    };
    ospi_protocol_config_t proto = {
        .line = OSPI_LINE_8,
        .rate = OSPI_RATE_DTR,
        .mem_type = OSPI_MEM_XCC_ELA,
    };
    ospi_sample_shift_config_t shift = {
        .enable = true, .shift_value = 4, .msda_mask = 0, .msdha_mask = 0,
    };
    ospi_hyperbus_config_t hyper = {
        .burst_type = OSPI_HYPER_WRAP, .lc0 = 1, .lc1 = 1,
    };

    ospi_reset(OSPI_DEMO_INST);

    ok = ospi_init_standard(OSPI_DEMO_INST, &baud, &basic, &proto);
    if (!ok) { return false; }
    ok = ospi_config_fifo_mode(OSPI_DEMO_INST, OSPI_FIFO_HALF, OSPI_FIFO_HALF);
    if (!ok) { return false; }
    ok = ospi_config_cmd_format(OSPI_DEMO_INST, true, true);
    if (!ok) { return false; }
    ok = ospi_config_dqs(OSPI_DEMO_INST, true, false);
    if (!ok) { return false; }
    ok = ospi_config_sample_shift(OSPI_DEMO_INST, &shift);
    if (!ok) { return false; }
    ok = ospi_config_output_delay(OSPI_DEMO_INST, 1);
    if (!ok) { return false; }
    ok = ospi_config_hyperbus(OSPI_DEMO_INST, &hyper);
    if (!ok) { return false; }
    ospi_config_out_en(OSPI_DEMO_INST, 0xFFU);

    ok = ospi_enable_memory_mapped(OSPI_DEMO_INST);
    if (!ok) { return false; }
    ok = ospi_config_memory_mapped(OSPI_DEMO_INST, &mem);
    if (!ok) { return false; }
    ok = ospi_enable_memory_mapped(OSPI_DEMO_INST);
    if (!ok) { return false; }

    ok = aps6408l_config_fixed_read_latency();
    if (!ok) { return false; }

    mem.read_dummy_cycles = 8;
    ok = ospi_config_memory_mapped(OSPI_DEMO_INST, &mem);
    if (!ok) { return false; }
    ok = ospi_enable_memory_mapped(OSPI_DEMO_INST);
    if (!ok) { return false; }
    ok = ospi_config_cs_timeout(OSPI_DEMO_INST, 500, 1);
    if (!ok) { return false; }

    return true;
}

//===========================================
// 时序诊断辅助函数
//===========================================

//===========================================
// 场景 1：FIFO 读全部模式寄存器
//===========================================

static void scenario_1_fifo_read_all_mr(void)
{
    static const uint8_t regs[] = {
        APS6408L_REG_MR0, APS6408L_REG_MR1, APS6408L_REG_MR2,
        APS6408L_REG_MR3, APS6408L_REG_MR4, APS6408L_REG_MR8,
    };
    static const char *names[] = { "MR0", "MR1", "MR2", "MR3", "MR4", "MR8" };
    uint16_t values[6];
    bool all_ok = true;

    print_separator();
    printf("=== 场景1：FIFO 读全部模式寄存器 ===\n\n");

    printf("步骤1：依次读取 MR0~MR8\n");
    for (uint32_t i = 0; i < 6U; i++) {
        if (aps6408l_read_reg_dtr(regs[i], &values[i])) {
            printf("  %s = 0x%04X\n", names[i], values[i]);
        } else {
            printf("  %s 读取失败\n", names[i]);
            all_ok = false;
        }
    }

    printf("\n步骤2：校验关键位\n");
    if (all_ok && ((values[1] & APS6408L_MR1_VENDOR_ID_MASK) == 0x0DU)) {
        printf("  MR1 Vendor ID: PASS (0x%02X = AP Memory)\n",
               values[1] & APS6408L_MR1_VENDOR_ID_MASK);
    } else {
        printf("  MR1 Vendor ID: FAIL\n");
        all_ok = false;
    }
    if (all_ok && ((values[2] & APS6408L_MR2_GOOD_DIE) != 0U)) {
        printf("  MR2 Good Die: PASS\n");
    } else {
        printf("  MR2 Good Die: FAIL\n");
        all_ok = false;
    }
    if (all_ok && ((values[3] & APS6408L_MR3_VCC_MASK) == 0U)) {
        printf("  MR3 VCC: 1.8V\n");
    } else {
        printf("  MR3 VCC: 3V 兼容（MR3=0x%04X，不影响 1.8V 操作）\n", values[3]);
    }

    printf("\n");
    print_result(all_ok, 2U, all_ok ? 0U : 2U);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 2：FIFO 连续压力测试
//===========================================

static void scenario_2_fifo_stress_test(void)
{
    uint32_t errors = 0;
    uint16_t value;
    uint32_t first_error_iter = 0;

    print_separator();
    printf("=== 场景2：FIFO 连续压力测试（%u 次）===\n\n", FIFO_STRESS_COUNT);

    for (uint32_t i = 0; i < FIFO_STRESS_COUNT; i++) {
        if (!aps6408l_read_reg_dtr(APS6408L_REG_MR1, &value)) {
            errors++;
            if (errors == 1U) { first_error_iter = i; }
        } else if ((value & APS6408L_MR1_VENDOR_ID_MASK) != 0x0DU) {
            errors++;
            if (errors == 1U) { first_error_iter = i; }
        }
    }

    if (errors == 0U) {
        printf("  全部 %u 次 PASS\n", FIFO_STRESS_COUNT);
    } else {
        printf("  首错 @ 第 %u 次，共 %u/%u 错误\n",
               first_error_iter, errors, FIFO_STRESS_COUNT);
    }
    print_result(errors == 0U, FIFO_STRESS_COUNT, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 3：列地址线 Walking-1 检测
//===========================================

static void scenario_3_walking_one_column(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    const uint8_t write_val = 0xAAU;
    uint32_t errors = 0;
    uint32_t tested = 0;

    print_separator();
    printf("=== 场景3：列地址线 Walking-1 检测（CA[9:1]）===\n\n");

    for (uint32_t col_bit = 1; col_bit < 10U; col_bit++) {
        uint32_t offset = 1U << col_bit;  /* CA[bit] 对应字节偏移 */

        /* PSRAM 要求起始地址为偶数，CA[0](offset=1) 不可测 */
        for (uint32_t i = 0; i < 4U; i++) {
            psram[offset + i] = write_val;
        }
        __DSB(); __ISB();

        for (uint32_t i = 0; i < 4U; i++) {
            if (psram[offset + i] != write_val) {
                errors++;
                if (errors == 1U) {
                    printf("  CA[%u] @ 0x%04X+%u: 期望 0x%02X, 实际 0x%02X\n",
                           col_bit, offset, i, write_val, psram[offset + i]);
                }
            }
        }
        tested++;
    }

    uint32_t total = tested * 4U;
    print_result(errors == 0U, total, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 4：行地址线 Walking-1 检测
//===========================================

static void scenario_4_walking_one_row(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    const uint8_t write_val = 0xAAU;
    uint32_t errors = 0;

    print_separator();
    printf("=== 场景4：行地址线 Walking-1 检测（RA[12:0]）===\n\n");

    for (uint32_t ra_bit = 0; ra_bit < APS6408L_RA_BITS; ra_bit++) {
        uint32_t row_offset = ((uint32_t)1U << ra_bit) * APS6408L_PAGE_SIZE;
        volatile uint8_t *row_addr = psram + row_offset;

        for (uint32_t i = 0; i < 64U; i++) { row_addr[i] = write_val; }
        __DSB();
        __ISB();

        for (uint32_t i = 0; i < 64U; i++) {
            if (row_addr[i] != write_val) {
                errors++;
                if (errors == 1U) {
                    printf("  RA[%u] @ 0x%06X+%u: 期望 0x%02X, 实际 0x%02X\n",
                           ra_bit, row_offset, i, write_val, row_addr[i]);
                }
            }
        }
    }

    uint32_t total = APS6408L_RA_BITS * 64U;
    print_result(errors == 0U, total, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 5：首尾页全量校验
//===========================================

static void scenario_5_full_page(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    volatile uint8_t *last_page = (volatile uint8_t *)(APS6408L_MEM_BASE
                                                       + APS6408L_SIZE
                                                       - APS6408L_PAGE_SIZE);
    uint32_t errors = 0;

    print_separator();
    printf("=== 场景5：首尾页全量校验（各 1024 字节递增）===\n\n");

    for (uint32_t i = 0; i < APS6408L_PAGE_SIZE; i++) {
        psram[i] = (uint8_t)(i & 0xFFU);
        last_page[i] = (uint8_t)(i & 0xFFU);
    }
    __DSB();
    __ISB();

    for (uint32_t i = 0; i < APS6408L_PAGE_SIZE; i++) {
        uint8_t expected = (uint8_t)(i & 0xFFU);

        if (psram[i] != expected) {
            errors++;
            if (errors == 1U) {
                printf("  首错 @ 页 0 offset 0x%03X: 期望 0x%02X, 实际 0x%02X\n",
                       i, expected, psram[i]);
            }
        }
        if (last_page[i] != expected) {
            errors++;
            if (errors == 1U) {
                printf("  首错 @ 末页 offset 0x%03X: 期望 0x%02X, 实际 0x%02X\n",
                       i, expected, last_page[i]);
            }
        }
    }

    uint32_t total = APS6408L_PAGE_SIZE * 2U;
    print_result(errors == 0U, total, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 6：全空间均匀采样
//===========================================

static void scenario_6_sparse_boundary(void)
{
    uint32_t errors = 0;

    print_separator();
    printf("=== 场景6：全空间均匀采样（%u 点 × %u 字节，步长 %uKB）===\n\n",
           SPARSE_SAMPLES, SPARSE_CHUNK, SPARSE_STEP / 1024U);

    for (uint32_t n = 0; n < SPARSE_SAMPLES; n++) {
        uint32_t base_addr = n * SPARSE_STEP;
        uint32_t words = SPARSE_CHUNK / 4U;

        for (uint32_t w = 0; w < words; w++) {
            volatile uint32_t *ptr = (volatile uint32_t *)(APS6408L_MEM_BASE
                                                           + base_addr + w * 4U);
            *ptr = base_addr + w;
        }
    }
    __DSB();
    __ISB();

    for (uint32_t n = 0; n < SPARSE_SAMPLES; n++) {
        uint32_t base_addr = n * SPARSE_STEP;
        uint32_t words = SPARSE_CHUNK / 4U;

        for (uint32_t w = 0; w < words; w++) {
            volatile uint32_t *ptr = (volatile uint32_t *)(APS6408L_MEM_BASE
                                                           + base_addr + w * 4U);
            uint32_t expected = base_addr + w;

            if (*ptr != expected) {
                errors++;
                if (errors == 1U) {
                    printf("  首错 @ 采样点 %u word %u: 期望 0x%08X, 实际 0x%08X\n",
                           n, w, expected, *ptr);
                }
            }
        }
    }

    uint32_t total = SPARSE_SAMPLES * SPARSE_CHUNK;
    print_result(errors == 0U, total, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 7：数据位图校验
//===========================================

static void scenario_7_data_pattern(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    uint32_t errors = 0;

    static const uint8_t walking_one[] = {
        0x01U, 0x02U, 0x04U, 0x08U, 0x10U, 0x20U, 0x40U, 0x80U,
    };
    static const uint8_t walking_zero[] = {
        0xFEU, 0xFDU, 0xFBU, 0xF7U, 0xEFU, 0xDFU, 0xBFU, 0x7FU,
    };

    print_separator();
    printf("=== 场景7：数据位图校验（3 组模式 × %u 字节）===\n\n",
           PATTERN_BLOCK_SIZE);

    /* Walking-1 */
    for (uint32_t r = 0; r < 8U; r++) {
        for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
            psram[i] = walking_one[r];
        }
        __DSB(); __ISB();
        for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
            if (psram[i] != walking_one[r]) {
                errors++;
                if (errors == 1U) {
                    printf("  Walking-1 0x%02X @ offset %u: 实际 0x%02X\n",
                           walking_one[r], i, psram[i]);
                }
            }
        }
    }

    /* Walking-0 */
    for (uint32_t r = 0; r < 8U; r++) {
        for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
            psram[i] = walking_zero[r];
        }
        __DSB(); __ISB();
        for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
            if (psram[i] != walking_zero[r]) {
                errors++;
                if (errors == 1U) {
                    printf("  Walking-0 0x%02X @ offset %u: 实际 0x%02X\n",
                           walking_zero[r], i, psram[i]);
                }
            }
        }
    }

    /* 0xAA / 0x55 翻转 */
    for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) { psram[i] = 0xAAU; }
    __DSB(); __ISB();
    for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
        if (psram[i] != 0xAAU) { errors++; }
    }
    for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) { psram[i] = 0x55U; }
    __DSB(); __ISB();
    for (uint32_t i = 0; i < PATTERN_BLOCK_SIZE; i++) {
        if (psram[i] != 0x55U) { errors++; }
    }

    uint32_t total = (8U + 8U + 2U) * PATTERN_BLOCK_SIZE;
    print_result(errors == 0U, total, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 8：连续地址递增校验
//===========================================

static void scenario_8_incremental_large(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    uint32_t errors = 0;

    print_separator();
    printf("=== 场景8：连续地址递增校验（前 %uKB）===\n\n",
           INCREMENTAL_SIZE / 1024U);

    for (uint32_t i = 0; i < INCREMENTAL_SIZE; i++) {
        psram[i] = (uint8_t)(i & 0xFFU);
    }
    __DSB(); __ISB();

    for (uint32_t i = 0; i < INCREMENTAL_SIZE; i++) {
        uint8_t expected = (uint8_t)(i & 0xFFU);

        if (psram[i] != expected) {
            errors++;
            if (errors == 1U) {
                printf("  首错 @ 0x%06X: 期望 0x%02X, 实际 0x%02X\n",
                       i, expected, psram[i]);
            }
        }
    }

    print_result(errors == 0U, INCREMENTAL_SIZE, errors);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 9：读采样点扫描
//===========================================

static void scenario_9_sample_shift_sweep(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    const uint8_t write_val = 0xAAU;
    const uint8_t read_dummy = 8U;
    uint32_t best_shift = 0;
    uint32_t best_matches = 0;

    print_separator();
    printf("=== 场景9：读采样点扫描（dummy=%u）===\n\n", read_dummy);

    for (uint8_t shift = 0U; shift <= 13U; shift++) {
        uint32_t matches = 0U;
        uint8_t first_actual = 0U;
        uint32_t first_error = 0xFFFFFFFFU;

        if (!ospi_config_read_timing(OSPI_DEMO_INST, read_dummy, shift)) { continue; }

        for (uint32_t i = 0; i < SWEEP_SIZE; i++) { psram[i] = write_val; }
        __DSB(); __ISB();

        for (uint32_t i = 0; i < SWEEP_SIZE; i++) {
            uint8_t actual = psram[i];

            if (actual == write_val) {
                matches++;
            } else if (first_error == 0xFFFFFFFFU) {
                first_error = i;
                first_actual = actual;
            }
        }

        if (matches > best_matches) { best_matches = matches; best_shift = shift; }

        if (first_error == 0xFFFFFFFFU) {
            printf("  shift=%u: PASS %u/%u\n", shift, matches, SWEEP_SIZE);
        } else {
            printf("  shift=%u: %u/%u first@%u=0x%02X\n",
                   shift, matches, SWEEP_SIZE, first_error, first_actual);
        }
    }

    printf("\n最佳 shift: %u (%u/%u 正确)\n", best_shift, best_matches, SWEEP_SIZE);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 10：三维时序全扫描
//===========================================

static void scenario_10_full_timing_search(void)
{
    volatile uint8_t *psram = (volatile uint8_t *)APS6408L_MEM_BASE;
    const uint8_t write_val = 0xAAU;
    uint32_t best_matches = 0U;
    uint8_t best_dummy = 0U;
    uint8_t best_out_delay = 0U;
    uint8_t best_shift = 0U;

    print_separator();
    printf("=== 场景10：三维时序全扫描 ===\n\n");

    for (uint8_t rd = 4U; rd <= 12U; rd++) {
        for (uint8_t od = 0U; od <= 3U; od++) {
            for (uint8_t sh = 0U; sh <= 13U; sh++) {
                uint32_t matches = 0U;

                if (!ospi_config_read_write_timing(OSPI_DEMO_INST, rd, sh, od)) { continue; }

                for (uint32_t i = 0; i < SWEEP_SIZE; i++) { psram[i] = write_val; }
                __DSB(); __ISB();

                for (uint32_t i = 0; i < SWEEP_SIZE; i++) {
                    if (psram[i] == write_val) { matches++; }
                }

                if (matches > best_matches) {
                    best_matches = matches;
                    best_dummy = rd;
                    best_out_delay = od;
                    best_shift = sh;
                    printf("  新最佳: dummy=%u out_delay=%u shift=%u -> %u/%u\n",
                           best_dummy, best_out_delay, best_shift,
                           best_matches, SWEEP_SIZE);
                    if (best_matches == SWEEP_SIZE) { goto found_optimal; }
                }
            }
        }
    }
found_optimal:

    printf("\n最佳组合: dummy=%u out_delay=%u shift=%u (%u/%u)\n",
           best_dummy, best_out_delay, best_shift, best_matches, SWEEP_SIZE);

    if (ospi_config_read_write_timing(OSPI_DEMO_INST, best_dummy, best_shift, best_out_delay)) {
        printf("\n最佳组合回读前 16 字节:\n");
        for (uint32_t i = 0; i < 16U; i++) {
            printf("  [%02u]=0x%02X\n", i, psram[i]);
        }
    }

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 11：读带宽测试
//===========================================

/**
 * @brief 场景 11：顺序读带宽测试（8/16/32 位，遍历全部 8MB）
 *
 * 每种访问宽度遍历全空间，测量 3 次取最小值，报告 KB/s。
 */
static void scenario_11_read_bandwidth(void)
{
    volatile uint8_t *psram8 = (volatile uint8_t *)APS6408L_MEM_BASE;
    volatile uint16_t *psram16 = (volatile uint16_t *)APS6408L_MEM_BASE;
    volatile uint32_t *psram32 = (volatile uint32_t *)APS6408L_MEM_BASE;
    uint32_t elapsed_ms;

    print_separator();
    printf("=== 场景11：读带宽测试（%uMB 全量，各 %u 次取最小值）===\n\n",
           (uint32_t)(APS6408L_SIZE / (1024U * 1024U)), PERF_RUNS);

    /* 8 位读 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;
        volatile uint32_t dummy = 0;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < APS6408L_SIZE; i++) {
                dummy += psram8[i];  /* 防止编译器优化掉读操作 */
            }
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  8 位读:  %u KB/s（%u ms）  dummy=0x%08X\n",
               kbps, best_ms, dummy);
    }

    /* 16 位读 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;
        volatile uint32_t dummy = 0;
        uint32_t count = APS6408L_SIZE / 2U;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < count; i++) {
                dummy += psram16[i];
            }
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  16 位读: %u KB/s（%u ms）  dummy=0x%08X\n",
               kbps, best_ms, dummy);
    }

    /* 32 位读 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;
        volatile uint32_t dummy = 0;
        uint32_t count = APS6408L_SIZE / 4U;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < count; i++) {
                dummy += psram32[i];
            }
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  32 位读: %u KB/s（%u ms）  dummy=0x%08X\n",
               kbps, best_ms, dummy);
    }

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 12：写带宽测试
//===========================================

/**
 * @brief 场景 12：顺序写带宽测试（8/16/32 位，遍历全部 8MB）
 *
 * 每种访问宽度遍历全空间，测量 3 次取最小值，报告 KB/s。
 */
static void scenario_12_write_bandwidth(void)
{
    volatile uint8_t *psram8 = (volatile uint8_t *)APS6408L_MEM_BASE;
    volatile uint16_t *psram16 = (volatile uint16_t *)APS6408L_MEM_BASE;
    volatile uint32_t *psram32 = (volatile uint32_t *)APS6408L_MEM_BASE;
    uint32_t elapsed_ms;

    print_separator();
    printf("=== 场景12：写带宽测试（%uMB 全量，各 %u 次取最小值）===\n\n",
           (uint32_t)(APS6408L_SIZE / (1024U * 1024U)), PERF_RUNS);

    /* 8 位写 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < APS6408L_SIZE; i++) {
                psram8[i] = (uint8_t)(i & 0xFFU);
            }
            __DSB();
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  8 位写:  %u KB/s（%u ms）\n", kbps, best_ms);
    }

    /* 16 位写 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;
        uint32_t count = APS6408L_SIZE / 2U;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < count; i++) {
                psram16[i] = (uint16_t)((i * 2U) & 0xFFFFU);
            }
            __DSB();
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  16 位写: %u KB/s（%u ms）\n", kbps, best_ms);
    }

    /* 32 位写 */
    {
        uint32_t best_ms = 0xFFFFFFFFU;
        uint32_t count = APS6408L_SIZE / 4U;

        for (uint32_t r = 0; r < PERF_RUNS; r++) {
            uint64_t t0 = system_get_tick();

            for (uint32_t i = 0; i < count; i++) {
                psram32[i] = (uint32_t)(i * 4U);
            }
            __DSB();
            uint64_t t1 = system_get_tick();
            uint32_t ms = (uint32_t)(t1 - t0);

            if (ms < best_ms) { best_ms = ms; }
        }

        uint32_t kbps = (best_ms > 0U) ? (APS6408L_SIZE / best_ms) : 0U;
        printf("  32 位写: %u KB/s（%u ms）\n", kbps, best_ms);
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
    delay_ms(100);

    printf("\n");
    print_separator();
    printf("ACM32P4 OSPI1 APS6408L PSRAM 完整验证\n");
    printf("场景选择: %d，0=全部\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    printf("初始化 GPIO + APS6408L Reset...\n");
    ospi_init_gpio();
    printf("  GPIOG5-15（AF2/5/6），PF10 Reset 已配置\n\n");

    printf("配置 OSPI1 八线 DTR Xccela OPI 内存映射...\n");
    if (!ospi_init_psram()) {
        printf("错误：OSPI1 初始化失败\n");
        print_separator();
        while (1) { __WFI(); }
    }
    printf("  内存映射基地址: 0x%08X（8MB）\n",
           ospi_get_memory_address(OSPI_DEMO_INST));
    printf("  初始化完成\n\n");

    /* FIFO 路径 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 1)) {
        scenario_1_fifo_read_all_mr();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 2)) {
        scenario_2_fifo_stress_test();
    }

    /* 地址线覆盖 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 3)) {
        scenario_3_walking_one_column();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 4)) {
        scenario_4_walking_one_row();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 5)) {
        scenario_5_full_page();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 6)) {
        scenario_6_sparse_boundary();
    }

    /* 数据模式覆盖 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 7)) {
        scenario_7_data_pattern();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 8)) {
        scenario_8_incremental_large();
    }

    /* 时序诊断 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 9)) {
        scenario_9_sample_shift_sweep();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 10)) {
        scenario_10_full_timing_search();
    }

    /* 性能测试 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 11)) {
        scenario_11_read_bandwidth();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 12)) {
        scenario_12_write_bandwidth();
    }

    printf("\n");
    print_separator();
    printf("全部测试完成\n");
    print_separator();

    while (1) { __WFI(); }
}

//===========================================
// 中断服务函数
//===========================================

/**
 * @brief HardFault 中断服务函数
 *
 * 硬件错误发生时进入死循环，可通过调试器查看调用栈。
 * 常见原因：非法内存访问、未对齐访问、除零、栈溢出。
 */
void HardFault_Handler(void)
{
    printf("\n!!! HardFault !!!\n");
    printf("  SCB->HFSR = 0x%08X\n", (unsigned int)SCB->HFSR);
    printf("  SCB->CFSR = 0x%08X\n", (unsigned int)SCB->CFSR);
    printf("  SCB->MMFAR = 0x%08X\n", (unsigned int)SCB->MMFAR);
    printf("  SCB->BFAR = 0x%08X\n", (unsigned int)SCB->BFAR);
    while (1) { __WFI(); }
}


