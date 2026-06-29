/**
 * @file norflash_example.c
 * @brief ACM32P4 NOR Flash 驱动使用示例
 *
 * @details
 * 演示 5 个场景：
 * 1. 读取制造商 ID 和设备 ID
 * 2. 读取芯片唯一 ID
 * 3. 读写验证（擦除 → 编程 → 读取 → 比对）
 * 4. 修改数据（norflash_modify 自动读-改-擦-写）
 * 5. 加密编程（设置密钥 → 擦除 → 加密写入）
 *
 * @attention
 * - 场景 3/4/5 会在 TEST_ADDR 处擦除并写入数据，请确认该地址不含重要数据
 * - TEST_ADDR 为相对于 Flash 起始的偏移地址（不含 0x08000000）
 * - 场景 5 的加密编程地址需填写绝对地址（含 0x08000000）
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...)  SEGGER_RTT_printf(0, __VA_ARGS__)

/* 选择要运行的场景（1~5） */
#define DEFAULT_SCENARIO  3

/* 读写测试区的 Flash 偏移地址（相对地址，距 Flash 起始 1 MB 处） */
#define TEST_ADDR         0x00100000U

/* 加密编程测试使用绝对地址（须 128 字节对齐，且目标区域空闲） */
#define TEST_ADDR_ABS     (0x08000000U + TEST_ADDR)

/* 测试数据长度 */
#define TEST_LEN          256U

//===========================================
// 工具函数
//===========================================

/* 打印十六进制 Dump */
static void print_hex(const char *label, const uint8_t *data, uint32_t len)
{
    printf("%s:", label);
    for (uint32_t i = 0; i < len; i++) {
        if ((i % 16U) == 0U) {
            printf("\r\n  %04X: ", i);
        }
        printf("%02X ", data[i]);
    }
    printf("\r\n");
}

/* 比对两块缓冲区，返回是否完全一致 */
static bool buf_equal(const uint8_t *a, const uint8_t *b, uint32_t len)
{
    return (memcmp(a, b, len) == 0);
}

//===========================================
// 场景 1：读取制造商 ID 和设备 ID
//===========================================

static void scenario_1_read_id(void)
{
    printf("\r\n======== 场景 1：读取制造商 ID ========\r\n");

    uint8_t id[3] = {0};

    if (norflash_read_id(id)) {
        printf("制造商 ID : 0x%02X\r\n", id[0]);
        printf("设备 ID   : 0x%02X 0x%02X\r\n", id[1], id[2]);
        printf("结果: 成功\r\n");
    } else {
        printf("结果: 失败\r\n");
    }
}

//===========================================
// 场景 2：读取芯片唯一 ID
//===========================================

static void scenario_2_read_unique_id(void)
{
    printf("\r\n======== 场景 2：读取唯一 ID ========\r\n");

    uint8_t uid[8] = {0};

    if (norflash_read_unique_id(uid, sizeof(uid))) {
        print_hex("唯一 ID", uid, sizeof(uid));
        printf("结果: 成功\r\n");
    } else {
        printf("结果: 失败\r\n");
    }
}

//===========================================
// 场景 3：擦除 → 编程 → 读取 → 验证
//===========================================

static void scenario_3_write_read_verify(void)
{
    printf("\r\n======== 场景 3：读写验证 ========\r\n");
    printf("测试地址: 0x%08X  长度: %u 字节\r\n", TEST_ADDR, TEST_LEN);

    /* 准备测试数据（填充递增值） */
    static uint8_t tx_buf[TEST_LEN];
    static uint8_t rx_buf[TEST_LEN];

    for (uint32_t i = 0; i < TEST_LEN; i++) {
        tx_buf[i] = (uint8_t)(i & 0xFFU);
    }
    memset(rx_buf, 0, sizeof(rx_buf));

    /* 步骤 1：擦除目标区域 */
    printf("步骤 1: 擦除...");
    if (!norflash_erase(TEST_ADDR, TEST_LEN)) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 2：验证擦除后为全 0xFF */
    if (!norflash_read(TEST_ADDR, rx_buf, TEST_LEN)) {
        printf("步骤 2: 读取擦除后数据失败\r\n");
        return;
    }
    bool erase_ok = true;
    for (uint32_t i = 0; i < TEST_LEN; i++) {
        if (rx_buf[i] != 0xFFU) {
            erase_ok = false;
            break;
        }
    }
    printf("步骤 2: 擦除验证 %s\r\n", erase_ok ? "通过" : "失败");

    /* 步骤 3：编程测试数据 */
    printf("步骤 3: 编程...");
    if (!norflash_program(TEST_ADDR, tx_buf, TEST_LEN)) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 4：读回并比对 */
    memset(rx_buf, 0, sizeof(rx_buf));
    if (!norflash_read(TEST_ADDR, rx_buf, TEST_LEN)) {
        printf("步骤 4: 读取失败\r\n");
        return;
    }

    if (buf_equal(tx_buf, rx_buf, TEST_LEN)) {
        printf("步骤 4: 数据对比 通过\r\n");
        printf("结果: 成功\r\n");
    } else {
        printf("步骤 4: 数据对比 失败\r\n");
        print_hex("期望", tx_buf, 16U);
        print_hex("实际", rx_buf, 16U);
        printf("结果: 失败\r\n");
    }
}

//===========================================
// 场景 4：使用 norflash_modify 修改部分数据
//===========================================

static void scenario_4_modify(void)
{
    printf("\r\n======== 场景 4：修改数据 ========\r\n");

    /* 在偏移 TEST_ADDR 处写入已知初始数据 */
    static uint8_t init_buf[NORFLASH_SECTOR_SIZE];
    static uint8_t patch_buf[8];
    static uint8_t readback[16];

    memset(init_buf, 0xAAU, sizeof(init_buf));
    memset(patch_buf, 0x55U, sizeof(patch_buf));

    /* 填充初始数据（整扇区） */
    printf("步骤 1: 写入初始数据...");
    if (!norflash_erase(TEST_ADDR, NORFLASH_SECTOR_SIZE) ||
        !norflash_program(TEST_ADDR, init_buf, NORFLASH_SECTOR_SIZE))
    {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 使用 modify 在 +0x10 处改写 8 字节 */
    uint32_t patch_addr = TEST_ADDR + 0x10U;
    printf("步骤 2: 修改地址 0x%08X 处 %u 字节...", patch_addr, (uint32_t)sizeof(patch_buf));
    if (!norflash_modify(patch_addr, patch_buf, sizeof(patch_buf))) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 3：读回并验证 */
    if (!norflash_read(patch_addr - 4U, readback, sizeof(readback))) {
        printf("步骤 3: 读取失败\r\n");
        return;
    }
    print_hex("修改区域附近数据", readback, sizeof(readback));

    /* 前 4 字节应仍为 0xAA，中间 8 字节应为 0x55，后 4 字节应为 0xAA */
    bool ok = true;
    for (uint32_t i = 0; i < 4U;  i++) { if (readback[i]      != 0xAAU) { ok = false; } }
    for (uint32_t i = 4; i < 12U; i++) { if (readback[i]      != 0x55U) { ok = false; } }
    for (uint32_t i = 12; i < 16U; i++) { if (readback[i]     != 0xAAU) { ok = false; } }

    printf("结果: %s\r\n", ok ? "成功" : "失败");
}

//===========================================
// 场景 5：加密编程
//===========================================

static void scenario_5_encrypt_program(void)
{
    printf("\r\n======== 场景 5：加密编程 ========\r\n");
    printf("加密地址: 0x%08X  长度: 128 字节\r\n", TEST_ADDR_ABS);

    /* 128 字节对齐检查 */
    if ((TEST_ADDR_ABS & 0x7FU) != 0U) {
        printf("错误: TEST_ADDR_ABS 未 128 字节对齐，请修改配置\r\n");
        return;
    }

    /* 测试密钥（AES-128，16 字节） */
    static const uint8_t key[16] = {
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10
    };

    /* 明文数据（128 字节，4 字节对齐） */
    static uint32_t plain_data[32] __attribute__((aligned(4)));
    for (uint32_t i = 0; i < 32U; i++) {
        plain_data[i] = 0xA5A5A500U | i;
    }

    /* 步骤 1：设置加密密钥 */
    printf("步骤 1: 设置密钥...");
    if (!norflash_set_encrypt_key(key)) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 2：擦除目标扇区（使用相对地址） */
    printf("步骤 2: 擦除目标区域...");
    if (!norflash_erase(TEST_ADDR, NORFLASH_SECTOR_SIZE)) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 3：加密编程（使用绝对地址） */
    static uint32_t cipher_out[32];
    printf("步骤 3: 加密编程...");
    if (!norflash_encrypt_program(TEST_ADDR_ABS, plain_data, 128U, cipher_out)) {
        printf("失败\r\n");
        return;
    }
    printf("完成\r\n");

    /* 步骤 4：打印密文（写入 Flash 的内容） */
    print_hex("密文（前 16 字节）", (const uint8_t *)cipher_out, 16U);

    printf("结果: 成功（加密数据已写入 Flash，读回为密文）\r\n");
}

//===========================================
// 入口函数
//===========================================

int main(void)
{
    printf("\r\n========================================\r\n");
    printf("  ACM32P4 NOR Flash 驱动示例\r\n");
    printf("========================================\r\n");
    system_init();
    SEGGER_RTT_Init();

    norflash_init_default();

    int scenario = DEFAULT_SCENARIO;

    switch (scenario) {
    case 1:  scenario_1_read_id();              break;
    case 2:  scenario_2_read_unique_id();       break;
    case 3:  scenario_3_write_read_verify();    break;
    case 4:  scenario_4_modify();               break;
    case 5:  scenario_5_encrypt_program();      break;
    default:
        printf("无效场景编号: %d（有效范围 1~5）\r\n", scenario);
        break;
    }

    printf("\r\n========================================\r\n");
}
