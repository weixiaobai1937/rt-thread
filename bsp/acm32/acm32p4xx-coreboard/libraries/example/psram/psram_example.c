/**
 * @file psram_example.c
 * @brief ACM32P4 OSPI1 APS6408L PSRAM SystemInit 验证示例
 *
 * @details
 * 验证 SystemInit 中初始化的 PSRAM 在 main() 中可通过 memory-mapped 正常读写。
 * 所有场景仅使用直接内存访问，不调用 OSPI 驱动库。
 *
 * 5 个场景：
 *  1. 基础读写验证（0xAA / 0x55 + 16 字节递增模式）
 *  2. Walking-1 / Walking-0 数据模式（数据线完整性，32 位宽）
 *  3. 连续地址递增校验（前 1KB）
 *  4. 全 0x00 / 全 0xFF 边界测试（256 字节）
 *  5. 首尾页验证（第 0 页 + 最后一页各 256 字节）
 *
 * 前提条件：
 *  - Keil 工程中定义 SYSTEM_INIT_OSPI_PSRAM=1
 *  - PSRAM memory-mapped 基地址：0x80000000（8MB）
 *
 * @note 本文件不调用 ospi_init_psram()，直接访问 0x80000000。
 *       PSRAM 初始化由 SystemInit 完成，main() 仅做验证。
 *
 * @author ACM32P4 SDK Team
 * @version 1.0.0
 */

#include <stdbool.h>
#include <stdint.h>
#include "SEGGER_RTT.h"
#include "acm32p4.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

/** PSRAM memory-mapped 基地址（8MB） */
#define PSRAM_BASE      0x80000000U
#define PSRAM_SIZE      (8U * 1024U * 1024U)
#define PSRAM_PAGE_SIZE 1024U

/** 测试场景选择：0=全部运行，1-5=单独运行 */
#define DEFAULT_SCENARIO 0

//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

//===========================================
// 场景 1：基础读写验证
//===========================================

/**
 * @brief 场景1：基础读写验证（0xAA / 0x55 模式）
 */
static void scenario_1_basic_rw(void)
{
    print_separator();
    printf("=== 场景1：基础读写验证 ===\n\n");

    volatile uint8_t *psram = (volatile uint8_t *)PSRAM_BASE;
    uint32_t pass_count = 0;
    uint32_t fail_count = 0;

    printf("步骤1：写入 0xAA 到地址 0x%08X，读回验证\n", PSRAM_BASE);
    psram[0] = 0xAAU;
    if (psram[0] == 0xAAU) {
        printf("  结果: PASS，读回 0xAA 匹配\n");
        pass_count++;
    } else {
        printf("  结果: FAIL，期望 0xAA 实际 0x%02X\n", psram[0]);
        fail_count++;
    }

    printf("\n步骤2：写入 0x55 到地址 0x%08X，读回验证\n", PSRAM_BASE);
    psram[0] = 0x55U;
    if (psram[0] == 0x55U) {
        printf("  结果: PASS，读回 0x55 匹配\n");
        pass_count++;
    } else {
        printf("  结果: FAIL，期望 0x55 实际 0x%02X\n", psram[0]);
        fail_count++;
    }

    printf("\n步骤3：连续写入 16 字节递增模式，读回验证\n");
    uint32_t sub_pass = 0;
    for (uint32_t i = 0; i < 16; i++) {
        psram[i] = (uint8_t)(i);
    }
    for (uint32_t i = 0; i < 16; i++) {
        if (psram[i] == (uint8_t)(i)) {
            sub_pass++;
        }
    }
    if (sub_pass == 16) {
        printf("  结果: PASS，16/16 匹配\n");
        pass_count++;
    } else {
        printf("  结果: FAIL，%u/16 匹配\n", sub_pass);
        fail_count++;
    }

    printf("\n总结果: %u PASS, %u FAIL\n", pass_count, fail_count);
    printf("完成！\n");
    print_separator();
}

//===========================================
// 场景 2：Walking-1 / Walking-0 数据模式
//===========================================

/**
 * @brief 场景2：Walking-1 / Walking-0 数据模式（数据线完整性）
 */
static void scenario_2_walking_one_data(void)
{
    print_separator();
    printf("=== 场景2：Walking-1 / Walking-0 数据模式 ===\n\n");

    volatile uint32_t *psram = (volatile uint32_t *)PSRAM_BASE;
    uint32_t pass = 0;
    uint32_t fail = 0;

    printf("测试 Walking-1（每组 32 位，32 组）：\n");
    for (uint32_t bit = 0; bit < 32; bit++) {
        uint32_t pattern = 1UL << bit;
        psram[bit] = pattern;
        uint32_t readback = psram[bit];

        if (readback == pattern) {
            pass++;
        } else {
            printf("  bit=%2u FAIL: wrote=0x%08X read=0x%08X\n",
                   bit, pattern, readback);
            fail++;
        }
    }

    printf("\n测试 Walking-0（每组 32 位，32 组）：\n");
    for (uint32_t bit = 0; bit < 32; bit++) {
        uint32_t pattern = ~(1UL << bit);
        uint32_t addr = 32 + bit;
        psram[addr] = pattern;
        uint32_t readback = psram[addr];

        if (readback == pattern) {
            pass++;
        } else {
            printf("  bit=%2u FAIL: wrote=0x%08X read=0x%08X\n",
                   bit, pattern, readback);
            fail++;
        }
    }

    printf("\n结果: %u PASS, %u FAIL（共 64 组）\n", pass, fail);
    printf("完成！\n");
    print_separator();
}

//===========================================
// 场景 3：连续地址递增校验（前 1KB）
//===========================================

/**
 * @brief 场景3：连续地址递增校验（前 1024 字节）
 */
static void scenario_3_incremental_1kb(void)
{
    print_separator();
    printf("=== 场景3：连续地址递增校验（前 1KB）===\n\n");

    volatile uint8_t *psram = (volatile uint8_t *)PSRAM_BASE;
    uint32_t pass = 0;
    uint32_t fail = 0;

    printf("写入 1024 字节地址偏移值...\n");
    for (uint32_t i = 0; i < 1024; i++) {
        psram[i] = (uint8_t)(i & 0xFFU);
    }

    printf("读回并校验...\n");
    for (uint32_t i = 0; i < 1024; i++) {
        uint8_t expected = (uint8_t)(i & 0xFFU);
        if (psram[i] == expected) {
            pass++;
        } else {
            if (fail < 5) {
                printf("  地址 0x%08X: 期望 0x%02X 实际 0x%02X\n",
                       PSRAM_BASE + i, expected, psram[i]);
            }
            fail++;
        }
    }

    printf("\n结果: %u PASS, %u FAIL（共 1024 字节）\n", pass, fail);
    printf("完成！\n");
    print_separator();
}

//===========================================
// 场景 4：全 0x00 / 全 0xFF 边界测试
//===========================================

/**
 * @brief 场景4：全 0x00 / 全 0xFF 边界测试
 */
static void scenario_4_all_zeros_ones(void)
{
    print_separator();
    printf("=== 场景4：全 0x00 / 全 0xFF 边界测试 ===\n\n");

    volatile uint8_t *psram = (volatile uint8_t *)PSRAM_BASE;
    uint32_t pass;
    uint32_t fail;

    printf("步骤1：写入全 0x00（256 字节）...\n");
    pass = 0; fail = 0;
    for (uint32_t i = 0; i < 256; i++) {
        psram[i] = 0x00U;
    }
    for (uint32_t i = 0; i < 256; i++) {
        if (psram[i] == 0x00U) { pass++; } else { fail++; }
    }
    printf("  全 0x00: %s（%u/256 正确）\n",
           (pass == 256) ? "PASS" : "FAIL", pass);

    printf("\n步骤2：写入全 0xFF（256 字节）...\n");
    pass = 0; fail = 0;
    for (uint32_t i = 0; i < 256; i++) {
        psram[i] = 0xFFU;
    }
    for (uint32_t i = 0; i < 256; i++) {
        if (psram[i] == 0xFFU) { pass++; } else { fail++; }
    }
    printf("  全 0xFF: %s（%u/256 正确）\n",
           (pass == 256) ? "PASS" : "FAIL", pass);

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景 5：首尾页验证
//===========================================

/**
 * @brief 场景5：首尾页验证（第 0 页 + 最后一页各 256 字节）
 */
static void scenario_5_first_last_page(void)
{
    print_separator();
    printf("=== 场景5：首尾页验证 ===\n\n");

    volatile uint8_t *psram = (volatile uint8_t *)PSRAM_BASE;
    uint32_t last_page_offset = PSRAM_SIZE - PSRAM_PAGE_SIZE;
    uint32_t pass = 0;
    uint32_t fail = 0;

    printf("步骤1：验证第 0 页（地址 0x%08X，256 字节）\n", PSRAM_BASE);
    for (uint32_t i = 0; i < 256; i++) {
        psram[i] = (uint8_t)(i & 0xFFU);
    }
    for (uint32_t i = 0; i < 256; i++) {
        uint8_t expected = (uint8_t)(i & 0xFFU);
        if (psram[i] == expected) { pass++; } else { fail++; }
    }
    printf("  第 0 页: %s（%u/256 正确）\n",
           (pass == 256) ? "PASS" : "FAIL", pass);

    printf("\n步骤2：验证最后一页（地址 0x%08X，256 字节）\n",
           PSRAM_BASE + last_page_offset);
    volatile uint8_t *last_page = psram + last_page_offset;
    uint32_t last_pass = 0;
    uint32_t last_fail = 0;

    for (uint32_t i = 0; i < 256; i++) {
        last_page[i] = (uint8_t)(255 - (i & 0xFFU));
    }
    for (uint32_t i = 0; i < 256; i++) {
        uint8_t expected = (uint8_t)(255 - (i & 0xFFU));
        if (last_page[i] == expected) { last_pass++; } else { last_fail++; }
    }
    printf("  最后一页: %s（%u/256 正确）\n",
           (last_pass == 256) ? "PASS" : "FAIL", last_pass);

    uint32_t total_pass = pass + last_pass;
    uint32_t total_fail = fail + last_fail;
    printf("\n总结果: %u PASS, %u FAIL（共 512 字节）\n", total_pass, total_fail);
    printf("完成！\n");
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
    printf("ACM32P4 OSPI1 PSRAM SystemInit 验证\n");
    printf("PSRAM 基地址: 0x%08X（%u MB）\n",
           PSRAM_BASE, PSRAM_SIZE / (1024U * 1024U));
    printf("场景选择: %d，0=全部\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    /* 验证 PSRAM 是否可访问 */
    volatile uint8_t *psram = (volatile uint8_t *)PSRAM_BASE;
    psram[0] = 0xA5U;

    if (psram[0] != 0xA5U) {
        printf("错误：PSRAM 不可访问（地址 0x%08X 写入 0xA5 读回 0x%02X）\n",
               PSRAM_BASE, psram[0]);
        printf("请确认 Keil 工程已定义 SYSTEM_INIT_OSPI_PSRAM=1\n");
        print_separator();
        while (1) { __WFI(); }
    }
    printf("PSRAM 可访问，开始测试...\n\n");

    /* 场景 1-5 */
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 1)) {
        scenario_1_basic_rw();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 2)) {
        scenario_2_walking_one_data();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 3)) {
        scenario_3_incremental_1kb();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 4)) {
        scenario_4_all_zeros_ones();
    }
    if ((DEFAULT_SCENARIO == 0) || (DEFAULT_SCENARIO == 5)) {
        scenario_5_first_last_page();
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




