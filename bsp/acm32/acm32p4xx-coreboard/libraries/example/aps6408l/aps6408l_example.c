/**
 * @file aps6408l_example.c
 * @brief APS6408L BSP 使用示例
 *
 * @details
 * 演示 5 个场景：
 * 1. 上电初始化并读取设备 ID
 * 2. 读取 APS6408L 模式寄存器
 * 3. FIFO 同步写读验证
 * 4. 存储映射窗口读写验证
 * 5. 执行 BSP 自检流程
 *
 * @attention
 * 场景 3/4/5 会向 PSRAM 写入测试数据，请确认目标板允许覆盖测试。
 */

#include "acm32p4.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "shell.h"
#include "adapter_shell_port.h"
#include "../../main/board/memory/board_aps6408l.h"

#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...)                           shellPrint(&shell, __VA_ARGS__)
#define DEFAULT_SCENARIO                      4

#define APS6408L_FIFO_TEST_ADDR               0x00000200UL
#define APS6408L_FIFO_TEST_HALFWORD_COUNT     32U
#define APS6408L_MEM_TEST_OFFSET              0x00001000UL
#define APS6408L_MEM_TEST_SIZE                64U
#define APS6408L_MEM_REPRO_TEST_SIZE          256U
#define APS6408L_MEM_REPRO_STRESS_COUNT       128U
#define HAL_EXAMPLE_XTH_HZ                    12000000U

//===========================================
// 工具函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

static void print_halfwords(const char *label, const uint16_t *data, uint32_t count)
{
    printf("%s[%u]: ", label, count);
    for (uint32_t i = 0; i < count; ++i) {
        printf("%04X ", data[i]);
    }
    printf("\n");
}

static void print_bytes(const char *label, const uint8_t *data, uint32_t count)
{
    printf("%s[%u]: ", label, count);
    for (uint32_t i = 0; i < count; ++i) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void read_mapped_bytes(const volatile uint8_t *src, uint8_t *dst, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        dst[i] = src[i];
    }
}

static void write_mapped_bytes(volatile uint8_t *dst, const uint8_t *src, uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        dst[i] = src[i];
    }
}

static void copy_mapped_halfwordwise(volatile uint8_t *dst, const uint8_t *src, uint32_t count)
{
    uint32_t i = 0U;

    while ((i + 2U) <= count) {
        uint16_t v = (uint16_t)((uint16_t)src[i] | ((uint16_t)src[i + 1U] << 8));
        *((volatile uint16_t *)(void *)(&dst[i])) = v;
        i += 2U;
    }

    while (i < count) {
        dst[i] = src[i];
        ++i;
    }
}

static void copy_mapped_wordwise(volatile uint8_t *dst, const uint8_t *src, uint32_t count)
{
    uint32_t i = 0U;

    while ((i + 4U) <= count) {
        uint32_t v = ((uint32_t)src[i]) |
                     ((uint32_t)src[i + 1U] << 8) |
                     ((uint32_t)src[i + 2U] << 16) |
                     ((uint32_t)src[i + 3U] << 24);
        *((volatile uint32_t *)(void *)(&dst[i])) = v;
        i += 4U;
    }

    while (i < count) {
        dst[i] = src[i];
        ++i;
    }
}

static void *call_memcpy_indirect(void *dst, const void *src, uint32_t count)
{
    void *(*memcpy_fn)(void *, const void *, size_t) = memcpy;
    return memcpy_fn(dst, src, (size_t)count);
}

static uint32_t find_first_mismatch(const uint8_t *expected,
                                    const uint8_t *actual,
                                    uint32_t count)
{
    for (uint32_t i = 0; i < count; ++i) {
        if (expected[i] != actual[i]) {
            return i;
        }
    }

    return count;
}

static void print_mismatch_window(const char *label,
                                  const uint8_t *expected,
                                  const uint8_t *actual,
                                  uint32_t count,
                                  uint32_t mismatch_idx)
{
    uint32_t start;
    uint32_t window;

    if (mismatch_idx >= count) {
        return;
    }

    start = (mismatch_idx > 8U) ? (mismatch_idx - 8U) : 0U;
    window = count - start;
    if (window > 24U) {
        window = 24U;
    }

    printf("  %s 首个错误偏移 = %lu\n", label, mismatch_idx);
    print_bytes("  错误点附近期望", &expected[start], window);
    print_bytes("  错误点附近实际", &actual[start], window);
}

//===========================================
// 场景实现
//===========================================

static void scenario_1_power_up_read_id(void)
{
    uint16_t device_id;

    print_separator();
    printf("=== 场景1：上电初始化并读取设备 ID ===\n\n");

    printf("步骤1：执行 APS6408L 上电初始化\n");
    bsp_aps6408l_power_up_init();
    printf("  结果: 已完成\n");

    printf("\n步骤2：发送全局复位命令\n");
    bsp_aps6408l_global_reset();
    delay_us(10U);
    printf("  结果: 已发送\n");

    printf("\n步骤3：读取设备 ID\n");
    device_id = bsp_aps6408l_read_id();
    printf("  Device ID = 0x%04X\n", device_id);
    printf("  校验结果 = %s\n", device_id == APS6408L_DEVICE_ID ? "匹配" : "不匹配");

    printf("\n完成！\n");
}

static void scenario_2_read_mode_registers(void)
{
    static const uint8_t reg_addrs[] = {
        APS6408L_MA_ADDR_0,
        APS6408L_MA_ADDR_1,
        APS6408L_MA_ADDR_2,
        APS6408L_MA_ADDR_3,
        APS6408L_MA_ADDR_4,
        APS6408L_MA_ADDR_8,
    };

    print_separator();
    printf("=== 场景2：读取 APS6408L 模式寄存器 ===\n\n");

    printf("步骤1：执行 APS6408L 上电初始化\n");
    bsp_aps6408l_power_up_init();
    printf("  结果: 已完成\n");

    printf("\n步骤2：读取 MR0/MR1/MR2/MR3/MR4/MR8\n");
    for (uint32_t i = 0; i < (uint32_t)(sizeof(reg_addrs) / sizeof(reg_addrs[0])); ++i) {
        uint16_t value = 0U;
        bsp_aps6408l_read_register(reg_addrs[i], &value);
        printf("  MR%u = 0x%04X\n", reg_addrs[i], value);
    }

    printf("\n完成！\n");
}

static void scenario_3_fifo_rw(void)
{
    uint16_t tx_buf[APS6408L_FIFO_TEST_HALFWORD_COUNT];
    uint16_t rx_buf[APS6408L_FIFO_TEST_HALFWORD_COUNT];

    print_separator();
    printf("=== 场景3：FIFO 同步写读验证 ===\n\n");

    printf("步骤1：执行 APS6408L 上电初始化\n");
    bsp_aps6408l_power_up_init();
    printf("  结果: 已完成\n");

    for (uint32_t i = 0; i < APS6408L_FIFO_TEST_HALFWORD_COUNT; ++i) {
        tx_buf[i] = (uint16_t)(0x1200U + i);
        rx_buf[i] = 0U;
    }

    printf("\n步骤2：向地址 0x%08lX 写入 %u 个半字\n",
           (unsigned long)APS6408L_FIFO_TEST_ADDR,
           APS6408L_FIFO_TEST_HALFWORD_COUNT);
    bsp_aps6408l_sync_write(APS6408L_FIFO_TEST_ADDR,
                            tx_buf,
                            APS6408L_FIFO_TEST_HALFWORD_COUNT);
    printf("  写入完成\n");

    printf("\n步骤3：读回并比对\n");
    bsp_aps6408l_sync_read(APS6408L_FIFO_TEST_ADDR,
                           rx_buf,
                           APS6408L_FIFO_TEST_HALFWORD_COUNT);
    print_halfwords("  写入前8项", tx_buf, 8U);
    print_halfwords("  读回前8项", rx_buf, 8U);
    printf("  比对结果: %s\n",
           memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0 ? "一致" : "不一致");

    printf("\n完成！\n");
}

static void scenario_4_memory_mapped_rw(void)
{
    uint8_t *psram_raw = (uint8_t *)(APS6408L_MEMORY_ADDR + APS6408L_MEM_TEST_OFFSET);
    volatile uint8_t *psram = (volatile uint8_t *)psram_raw;
    uint8_t tx_buf[APS6408L_MEM_TEST_SIZE];
    uint8_t rx_buf[APS6408L_MEM_TEST_SIZE];
    uint8_t expected_buf[APS6408L_MEM_REPRO_TEST_SIZE];
    uint8_t actual_buf[APS6408L_MEM_REPRO_TEST_SIZE];
    uint8_t ram_src[APS6408L_MEM_REPRO_TEST_SIZE];
    static const char volume_label[] = "ACM32P4 MSC";
    static const char file_name[] = "README  TXT";
    static const char readme_like[] = "Welcome to USB RAM Disk!\r\n"
                                      "\r\n"
                                      "Device: ACM32P4 + APS6408L PSRAM\r\n"
                                      "Format: FAT12 (1.44MB Floppy)\r\n"
                                      "Stack: Azure RTOS USBX\r\n"
                                      "\r\n"
                                      "This payload is used to reproduce memcpy issues.\r\n";
    uint32_t readme_len = (uint32_t)(sizeof(readme_like) - 1U);
    uint32_t mismatch_idx;
    uint32_t fail_iter;
    uint32_t first_fail_len_flash;
    uint32_t first_fail_len_ram;

    print_separator();
    printf("=== 场景4：存储映射窗口读写验证 ===\n\n");

    memset(ram_src, 0, sizeof(ram_src));
    memcpy(ram_src, readme_like, readme_len);

    printf("步骤1：执行 APS6408L 上电初始化\n");
    bsp_aps6408l_power_up_init();
    printf("  结果: 已完成\n");

    printf("\n步骤2：使能存储映射模式\n");
    bsp_aps6408l_memory_mode_init();
    printf("  基址: 0x%08lX\n", (unsigned long)APS6408L_MEMORY_ADDR);
    printf("  偏移: 0x%08lX\n", (unsigned long)APS6408L_MEM_TEST_OFFSET);

    for (uint32_t i = 0; i < APS6408L_MEM_TEST_SIZE; ++i) {
        tx_buf[i] = (uint8_t)(i ^ 0x5AU);
        rx_buf[i] = 0U;
    }

    printf("\n步骤3：基线测试，逐字节写入 64 字节\n");
    for (uint32_t i = 0; i < APS6408L_MEM_TEST_SIZE; ++i) {
        psram[i] = tx_buf[i];
    }
    delay_ms(1U);

    printf("\n步骤4：读回并比对\n");
    read_mapped_bytes(psram, rx_buf, APS6408L_MEM_TEST_SIZE);
    print_bytes("  写入前16字节", tx_buf, 16U);
    print_bytes("  读回前16字节", rx_buf, 16U);
    printf("  比对结果: %s\n",
           memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0 ? "一致" : "不一致");

    printf("\n步骤5：复现测试A，直接对映射窗口调用 memset\n");
    memset(tx_buf, 0xA5, sizeof(tx_buf));
    memset(psram_raw, 0xA5, APS6408L_MEM_TEST_SIZE);
    delay_ms(1U);
    read_mapped_bytes(psram, rx_buf, APS6408L_MEM_TEST_SIZE);
    mismatch_idx = find_first_mismatch(tx_buf, rx_buf, APS6408L_MEM_TEST_SIZE);
    print_bytes("  期望前16字节", tx_buf, 16U);
    print_bytes("  实际前16字节", rx_buf, 16U);
    if (mismatch_idx == APS6408L_MEM_TEST_SIZE) {
        printf("  memset 结果: 一致\n");
    } else {
        printf("  memset 结果: 不一致，首个错误偏移 = %lu\n", mismatch_idx);
    }

    printf("\n步骤6：复现测试B，模拟卷标目录项的 memset + memcpy(11字节)\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[0], volume_label, 11U);
    expected_buf[11] = 0x08U;

    fail_iter = APS6408L_MEM_REPRO_STRESS_COUNT;
    for (uint32_t iter = 0; iter < APS6408L_MEM_REPRO_STRESS_COUNT; ++iter) {
        memset(psram_raw, 0, 64U);
        memcpy(psram_raw, volume_label, 11U);
        psram[11] = 0x08U;
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, 64U);
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, 64U);
        if (mismatch_idx != 64U) {
            fail_iter = iter;
            break;
        }
    }

    print_bytes("  期望前16字节", expected_buf, 16U);
    print_bytes("  实际前16字节", actual_buf, 16U);
    if (fail_iter == APS6408L_MEM_REPRO_STRESS_COUNT) {
        printf("  卷标测试结果: %u 次均一致\n", APS6408L_MEM_REPRO_STRESS_COUNT);
    } else {
        printf("  卷标测试结果: 在第 %lu 次复现，首个错误偏移 = %lu\n",
               fail_iter,
               mismatch_idx);
    }

    printf("\n步骤7：复现测试C，仅执行 README 长 memcpy\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[64], readme_like, sizeof(readme_like) - 1U);

    fail_iter = APS6408L_MEM_REPRO_STRESS_COUNT;
    for (uint32_t iter = 0; iter < APS6408L_MEM_REPRO_STRESS_COUNT; ++iter) {
        memset(psram_raw, 0, sizeof(expected_buf));
        memcpy(psram_raw + 64U, readme_like, sizeof(readme_like) - 1U);
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
        if (mismatch_idx != sizeof(actual_buf)) {
            fail_iter = iter;
            break;
        }
    }

    print_bytes("  README期望前16字节", &expected_buf[64], 16U);
    print_bytes("  README实际前16字节", &actual_buf[64], 16U);
    if (fail_iter == APS6408L_MEM_REPRO_STRESS_COUNT) {
        printf("  README长 memcpy 结果: %u 次均一致\n", APS6408L_MEM_REPRO_STRESS_COUNT);
    } else {
        printf("  README长 memcpy 结果: 在第 %lu 次复现，首个错误偏移 = %lu\n",
               fail_iter,
               mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("README长 memcpy", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤8：定位测试D，扫描 README memcpy 失败长度（Flash 源）\n");
    first_fail_len_flash = 0U;
    for (uint32_t len = 1U; len <= readme_len; ++len) {
        memset(expected_buf, 0, sizeof(expected_buf));
        memcpy(&expected_buf[64], readme_like, len);

        memset(psram_raw, 0, sizeof(expected_buf));
        memcpy(psram_raw + 64U, readme_like, len);
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
        if (mismatch_idx != sizeof(actual_buf)) {
            first_fail_len_flash = len;
            break;
        }
    }

    if (first_fail_len_flash == 0U) {
        printf("  Flash 源长度扫描结果: 1~%lu 字节均一致\n", readme_len);
    } else {
        printf("  Flash 源长度扫描结果: 首个失败长度 = %lu\n", first_fail_len_flash);
        printf("  首个错误偏移 = %lu\n", mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("Flash 源长度扫描", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤9：定位测试E，使用 RAM 源重复失败长度\n");
    first_fail_len_ram = 0U;
    if (first_fail_len_flash == 0U) {
        printf("  未找到 Flash 源失败长度，跳过 RAM 源对照\n");
    } else {
        memset(expected_buf, 0, sizeof(expected_buf));
        memcpy(&expected_buf[64], ram_src, first_fail_len_flash);

        memset(psram_raw, 0, sizeof(expected_buf));
        memcpy(psram_raw + 64U, ram_src, first_fail_len_flash);
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
        if (mismatch_idx != sizeof(actual_buf)) {
            first_fail_len_ram = first_fail_len_flash;
        }

        if (first_fail_len_ram == 0U) {
            printf("  RAM 源对照结果: 长度 %lu 一致\n", first_fail_len_flash);
        } else {
            printf("  RAM 源对照结果: 长度 %lu 也失败\n", first_fail_len_ram);
            printf("  首个错误偏移 = %lu\n", mismatch_idx);
            if (mismatch_idx >= 64U) {
                printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
            }
            print_mismatch_window("RAM 源对照", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
        }
    }

    printf("\n步骤10：定位测试F，间接函数指针调用 memcpy（Flash 源）\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[64], readme_like, first_fail_len_flash);

    memset(psram_raw, 0, sizeof(expected_buf));
    call_memcpy_indirect(psram_raw + 64U, readme_like, first_fail_len_flash);
    delay_us(10U);
    read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
    mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
    if (mismatch_idx == sizeof(actual_buf)) {
        printf("  间接 memcpy 结果: 长度 %lu 一致\n", first_fail_len_flash);
    } else {
        printf("  间接 memcpy 结果: 长度 %lu 失败\n", first_fail_len_flash);
        printf("  首个错误偏移 = %lu\n", mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("间接 memcpy", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤11：定位测试G，16位循环拷贝（Flash 源）\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[64], readme_like, first_fail_len_flash);

    memset(psram_raw, 0, sizeof(expected_buf));
    copy_mapped_halfwordwise(&psram[64], (const uint8_t *)readme_like, first_fail_len_flash);
    delay_us(10U);
    read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
    mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
    if (mismatch_idx == sizeof(actual_buf)) {
        printf("  16位循环拷贝结果: 长度 %lu 一致\n", first_fail_len_flash);
    } else {
        printf("  16位循环拷贝结果: 长度 %lu 失败\n", first_fail_len_flash);
        printf("  首个错误偏移 = %lu\n", mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("16位循环拷贝", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤12：定位测试H，32位循环拷贝（Flash 源）\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[64], readme_like, first_fail_len_flash);

    memset(psram_raw, 0, sizeof(expected_buf));
    copy_mapped_wordwise(&psram[64], (const uint8_t *)readme_like, first_fail_len_flash);
    delay_us(10U);
    read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
    mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
    if (mismatch_idx == sizeof(actual_buf)) {
        printf("  32位循环拷贝结果: 长度 %lu 一致\n", first_fail_len_flash);
    } else {
        printf("  32位循环拷贝结果: 长度 %lu 失败\n", first_fail_len_flash);
        printf("  首个错误偏移 = %lu\n", mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("32位循环拷贝", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤13：复现测试I，仅执行 README 逐字节写入\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[64], readme_like, readme_len);

    fail_iter = APS6408L_MEM_REPRO_STRESS_COUNT;
    for (uint32_t iter = 0; iter < APS6408L_MEM_REPRO_STRESS_COUNT; ++iter) {
        memset(psram_raw, 0, sizeof(expected_buf));
        write_mapped_bytes(&psram[64], (const uint8_t *)readme_like, readme_len);
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
        if (mismatch_idx != sizeof(actual_buf)) {
            fail_iter = iter;
            break;
        }
    }

    print_bytes("  README期望前16字节", &expected_buf[64], 16U);
    print_bytes("  README实际前16字节", &actual_buf[64], 16U);
    if (fail_iter == APS6408L_MEM_REPRO_STRESS_COUNT) {
        printf("  README逐字节写结果: %u 次均一致\n", APS6408L_MEM_REPRO_STRESS_COUNT);
    } else {
        printf("  README逐字节写结果: 在第 %lu 次复现，首个错误偏移 = %lu\n",
               fail_iter,
               mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("README逐字节写", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n步骤14：复现测试J，模拟 MSC 初始化中的目录项和 README 多次 memcpy\n");
    memset(expected_buf, 0, sizeof(expected_buf));
    memcpy(&expected_buf[0], volume_label, 11U);
    expected_buf[11] = 0x08U;
    memcpy(&expected_buf[32], file_name, 11U);
    expected_buf[43] = 0x20U;
    memcpy(&expected_buf[64], readme_like, readme_len);

    fail_iter = APS6408L_MEM_REPRO_STRESS_COUNT;
    for (uint32_t iter = 0; iter < APS6408L_MEM_REPRO_STRESS_COUNT; ++iter) {
        memset(psram_raw, 0, sizeof(expected_buf));
        memcpy(psram_raw, volume_label, 11U);
        psram[11] = 0x08U;
        memcpy(psram_raw + 32U, file_name, 11U);
        psram[43] = 0x20U;
        memcpy(psram_raw + 64U, readme_like, readme_len);
        delay_us(10U);
        read_mapped_bytes(psram, actual_buf, sizeof(actual_buf));
        mismatch_idx = find_first_mismatch(expected_buf, actual_buf, sizeof(actual_buf));
        if (mismatch_idx != sizeof(actual_buf)) {
            fail_iter = iter;
            break;
        }
    }

    print_bytes("  目录项期望前32字节", expected_buf, 32U);
    print_bytes("  目录项实际前32字节", actual_buf, 32U);
    print_bytes("  README期望前16字节", &expected_buf[64], 16U);
    print_bytes("  README实际前16字节", &actual_buf[64], 16U);
    if (fail_iter == APS6408L_MEM_REPRO_STRESS_COUNT) {
        printf("  组合测试结果: %u 次均一致\n", APS6408L_MEM_REPRO_STRESS_COUNT);
    } else {
        printf("  组合测试结果: 在第 %lu 次复现，首个错误偏移 = %lu\n",
               fail_iter,
               mismatch_idx);
        if (mismatch_idx >= 64U) {
            printf("  README相对偏移 = %lu\n", mismatch_idx - 64U);
        }
        print_mismatch_window("组合测试", expected_buf, actual_buf, sizeof(actual_buf), mismatch_idx);
    }

    printf("\n完成！\n");
}

static void scenario_5_bsp_self_test(void)
{
    aps6408l_status_t result;

    print_separator();
    printf("=== 场景5：执行 BSP 自检流程 ===\n\n");

    printf("步骤1：执行 BSP 自检\n");
    result = bsp_aps6408l_test();
    printf("  自检结果: %s\n", result == APS6408L_OK ? "通过" : "失败");

    printf("\n完成！\n");
}

static void init_example_entry(void)
{
    /* 与 HAL 范例入口保持一致：使用 12MHz XTH 作为 PLL1 输入源。 */
    clock_init_xtal(HAL_EXAMPLE_XTH_HZ);
    (void)system_timebase_init();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    init_example_entry();
    shell_port_init();

    printf("\n");
    print_separator();
    printf("ACM32P4 APS6408L BSP 示例\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();

    switch (DEFAULT_SCENARIO) {
    case 1:
        scenario_1_power_up_read_id();
        break;
    case 2:
        scenario_2_read_mode_registers();
        break;
    case 3:
        scenario_3_fifo_rw();
        break;
    case 4:
        scenario_4_memory_mapped_rw();
        break;
    case 5:
        scenario_5_bsp_self_test();
        break;
    default:
        printf("错误：无效的场景编号 %d（有效范围：1-5）\n", DEFAULT_SCENARIO);
        break;
    }

    while (1) {
        __WFI();
    }
}