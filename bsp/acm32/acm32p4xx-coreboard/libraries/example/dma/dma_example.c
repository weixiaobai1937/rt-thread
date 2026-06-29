/**
 * @file dma_example.c
 * @brief ACM32P4 DMA驱动示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 存储器到存储器传输（M2M基础用法）
 * 2. 轮询模式：等待传输完成标志
 * 3. 中断模式：传输完成回调
 * 4. 半传输完成中断（乒乓缓冲）
 * 5. 链表传输（多段不连续数据）
 * 6. 循环传输（自动重复）
 * 7. 双缓冲传输（P2M交替缓冲）
 * 8. 突发传输与数据位宽配置
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#include <string.h>
#include <stdbool.h>
#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "hardware/dma.h"
#include "hardware/system.h"

//===========================================
// 配置选项
//===========================================

#define printf(...)         SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO    4   ///< 修改此值（1-8）选择要运行的场景

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

/**
 * @brief 打印缓冲区前N字节的十六进制内容
 */
static void print_hex(const char *label, const uint8_t *buf, uint32_t len)
{
    printf("%s: ", label);
    uint32_t show = (len > 16U) ? 16U : len;
    for (uint32_t i = 0; i < show; i++) {
        printf("%02X ", buf[i]);
    }
    if (len > 16U) {
        printf("... (%u字节)", len);
    }
    printf("\n");
}

/**
 * @brief 逐字节比较两个缓冲区，首次不匹配时打印错误位置
 */
static bool verify_buf(const uint8_t *expected, const uint8_t *actual, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        if (expected[i] != actual[i]) {
            printf("  验证失败：位置[%u] 期望=0x%02X 实际=0x%02X\n",
                   i, expected[i], actual[i]);
            return false;
        }
    }
    return true;
}

//===========================================
// 场景1：存储器到存储器基础传输
//===========================================

/**
 * @brief 场景1：M2M基础传输
 *
 * 使用第1层API dma_init_mem2mem() 完成256字节的存储器复制，
 * 使用 dma_wait_transfer_complete() 轮询等待完成。
 */
void scenario_1_mem2mem_basic(void)
{
    print_separator();
    printf("=== 场景1：存储器到存储器基础传输 ===\n\n");

    static uint8_t src[256] __attribute__((aligned(4)));
    static uint8_t dst[256] __attribute__((aligned(4)));

    printf("步骤1：初始化源缓冲区（0x00-0xFF）\n");
    for (int i = 0; i < 256; i++) {
        src[i] = (uint8_t)i;
        dst[i] = 0;
    }
    print_hex("  源数据", src, 256);

    printf("\n步骤2：配置DMA（M2M，256字节，32位位宽）\n");
    bool ok = dma_init_mem2mem(DMA_1, DMA_CHANNEL_0,
                               (uint32_t)src, (uint32_t)dst, 256);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤3：启动传输\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_0);
    printf("  结果: 传输已启动\n");

    printf("\n步骤4：轮询等待传输完成（超时1000次）\n");
    ok = dma_wait_transfer_complete(DMA_1, DMA_CHANNEL_0, 1000);
    printf("  结果: %s\n", ok ? "传输完成" : "超时或错误");
    if (!ok) {
        goto done;
    }

    printf("\n步骤5：验证第一次传输数据\n");
    print_hex("  目标数据", dst, 256);
    printf("  验证结果: %s\n", verify_buf(src, dst, 256) ? "✓ 正确" : "✗ 错误");

    printf("\n步骤6：第二次传输（修改源数据后重新配置并启动）\n");
    // 修改源数据，验证第二次传输
    for (int i = 0; i < 256; i++) {
        src[i] = (uint8_t)(0xFF - i);
    }
    print_hex("  新源数据", src, 256);
    // 传输完成后CTRL.TRANSFERSIZE已归零，仅重新写入传输计数（256字节 / 4 = 64次字传输）
    ok = dma_config_transfer_size(DMA_1, DMA_CHANNEL_0, 64);
    printf("  重载传输计数: %s\n", ok ? "成功" : "失败");
    if (!ok) {
        goto done;
    }    
    ok = dma_config_address(DMA_1, DMA_CHANNEL_0, (uint32_t)src, (uint32_t)dst);
    printf("  重载地址: %s\n", ok ? "成功" : "失败");
    if (!ok) {
        goto done;
    }
    dma_start_channel(DMA_1, DMA_CHANNEL_0);
    ok = dma_wait_transfer_complete(DMA_1, DMA_CHANNEL_0, 1000);
    printf("  结果: %s\n", ok ? "传输完成" : "超时或错误");
    if (!ok) {
        goto done;
    }
    print_hex("  目标数据", dst, 256);
    printf("  验证结果: %s\n", verify_buf(src, dst, 256) ? "✓ 正确" : "✗ 错误");
done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_0);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景2：轮询传输完成标志
//===========================================

/**
 * @brief 场景2：手动轮询 RAW_INT_TC_STATUS 完成标志
 *
 * 不使用 dma_wait_transfer_complete()，直接调用
 * dma_is_transfer_complete() 轮询，演示底层标志使用方式。
 */
void scenario_2_polling_flag(void)
{
    print_separator();
    printf("=== 场景2：手动轮询传输完成标志 ===\n\n");

    static uint8_t src[128] __attribute__((aligned(4)));
    static uint8_t dst[128] __attribute__((aligned(4)));

    for (int i = 0; i < 128; i++) {
        src[i] = (uint8_t)(0x80 + i);
        dst[i] = 0;
    }

    printf("步骤1：配置DMA（M2M，128字节）\n");
    bool ok = dma_init_mem2mem(DMA_1, DMA_CHANNEL_1,
                               (uint32_t)src, (uint32_t)dst, 128);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤2：启动传输\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_1);

    printf("\n步骤3：手动轮询传输完成标志\n");
    uint32_t poll_count = 0;
    while (!dma_is_transfer_complete(DMA_1, DMA_CHANNEL_1)) {
        poll_count++;
        if (poll_count > 10000000U) {
            printf("  错误：轮询超时\n");
            goto done;
        }
    }
    dma_clear_tc_flag(DMA_1, DMA_CHANNEL_1);
    printf("  轮询次数: %u\n", poll_count);
    printf("  结果: 传输完成标志已检测到\n");

    printf("\n步骤4：验证数据\n");
    printf("  验证结果: %s\n", verify_buf(src, dst, 128) ? "✓ 正确" : "✗ 错误");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_1);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景3：中断模式传输
//===========================================

static volatile bool s3_tc_done  = false;
static volatile bool s3_err_done = false;
static volatile bool s3_ht_done = false;

static void scenario3_ht_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s3_ht_done = true;
}


static void scenario3_tc_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s3_tc_done = true;
}

static void scenario3_err_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s3_err_done = true;
}

/**
 * @brief 场景3：中断模式传输
 *
 * 注册传输完成回调，启动传输后进入WFI低功耗等待，
 * 中断触发时回调置位标志，主循环检测后继续执行。
 */
void scenario_3_interrupt_mode(void)
{
    print_separator();
    printf("=== 场景3：中断模式传输 ===\n\n");

    static uint8_t src[512] __attribute__((aligned(4)));
    static uint8_t dst[512] __attribute__((aligned(4)));

    for (int i = 0; i < 512; i++) {
        src[i] = (uint8_t)(i & 0xFF);
        dst[i] = 0;
    }

    printf("步骤1：配置DMA通道\n");
    dma_enable(DMA_1);
    dma_channel_config_t cfg = {
        .direction          = DMA_MEM_TO_MEM,
        .src_address        = (uint32_t)src,
        .dest_address       = (uint32_t)dst,
        .src_addr_mode      = DMA_ADDR_INCREMENT,
        .dest_addr_mode     = DMA_ADDR_INCREMENT,
        .src_width          = DMA_WIDTH_WORD,
        .dest_width         = DMA_WIDTH_WORD,
        .src_burst          = DMA_BURST_16,
        .dest_burst         = DMA_BURST_16,
        .transfer_size      = 128,  // 128次字传输 = 512字节
        .src_periph_id      = 0,
        .dest_periph_id     = 0,
        .src_master         = DMA_MASTER_1,
        .dest_master        = DMA_MASTER_1,
        .bus_lock           = false,
        .tc_interrupt_enable    = true,
        .ht_interrupt_enable    = true,
        .error_interrupt_enable = true,
    };
    bool ok = dma_config_channel(DMA_1, DMA_CHANNEL_2, &cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤2：注册回调函数并使能NVIC中断\n");
    s3_tc_done  = false;
    s3_err_done = false;
    s3_ht_done = false;
    dma_register_ht_callback(DMA_1, DMA_CHANNEL_2, scenario3_ht_callback);
    dma_register_tc_callback(DMA_1, DMA_CHANNEL_2, scenario3_tc_callback);
    dma_register_error_callback(DMA_1, DMA_CHANNEL_2, scenario3_err_callback);
    NVIC_EnableIRQ(DMA1_CH2_IRQn);
    printf("  结果: 回调已注册，NVIC中断已使能\n");

    printf("\n步骤3：启动传输，进入WFI等待中断\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_2);

    uint64_t t0 = system_get_tick();
    while (!s3_tc_done && !s3_err_done) {
        __WFI();
        if (system_elapsed(t0, 1000U)) {
            printf("  错误：等待中断超时（确认已使能NVIC？）\n");
            goto done;
        }
    }

    if (s3_err_done) {
        printf("  错误：传输错误中断触发\n");
        goto done;
    }
    
    if (!s3_ht_done) {
        printf("  错误：半传输中断未触发\n");
        goto done;
    } else {
        printf("  结果: 半传输中断已触发\n");
    }
    if (!s3_tc_done) {
        printf("  错误：传输完成中断未触发\n");
        goto done;
    } else {
        printf("  结果: 传输完成中断已触发\n");
    }

    printf("\n步骤4：验证数据\n");
    printf("  验证结果: %s\n", verify_buf(src, dst, 512) ? "✓ 正确" : "✗ 错误");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_2);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景4：半传输中断（乒乓缓冲处理）
//===========================================

static volatile bool s4_ht_done = false;
static volatile bool s4_tc_done = false;

static void scenario4_ht_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s4_ht_done = true;
}

static void scenario4_tc_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s4_tc_done = true;
}

/**
 * @brief 场景4：半传输中断演示乒乓缓冲处理模式
 *
 * DMA传输256字节数据时，在完成前128字节（半传输）时触发中断，
 * 应用可在后半段传输进行中处理前半段数据，实现乒乓缓冲。
 */
void scenario_4_half_transfer(void)
{
    print_separator();
    printf("=== 场景4：半传输中断（乒乓缓冲）===\n\n");

    static uint8_t src[256] __attribute__((aligned(4)));
    static uint8_t dst[256] __attribute__((aligned(4)));

    for (int i = 0; i < 256; i++) {
        src[i] = (uint8_t)i;
        dst[i] = 0;
    }
    printf("缓冲区大小: 256字节（前半=0-127，后半=128-255）\n\n");

    printf("步骤1：配置DMA，同时使能半传输和全传输中断\n");
    bool ok = dma_init_mem2mem(DMA_1, DMA_CHANNEL_1,
                               (uint32_t)src, (uint32_t)dst, 256);
    if (!ok) {
        printf("  错误：DMA配置失败\n");
        goto done;
    }

    s4_ht_done = false;
    s4_tc_done = false;
    dma_register_ht_callback(DMA_1, DMA_CHANNEL_1, scenario4_ht_callback);
    dma_register_tc_callback(DMA_1, DMA_CHANNEL_1, scenario4_tc_callback);
    dma_enable_ht_interrupt(DMA_1, DMA_CHANNEL_1);
    NVIC_EnableIRQ(DMA1_CH1_IRQn);
    printf("  结果: 配置成功，已使能HT和TC中断，NVIC已使能\n");

    printf("\n步骤2：启动传输，等待半传输中断\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_1);

    uint64_t t0 = system_get_tick();
    while (!s4_ht_done) {
        __WFI();
        if (system_elapsed(t0, 1000U)) {
            printf("  注意：半传输中断未触发（需使能NVIC）\n");
            goto done;
        }
    }
    printf("  半传输中断触发，前半段已完成\n");
    printf("  → 此时可安全处理 dst[0-127]，同时DMA继续传输 dst[128-255]\n");

    // 验证前半段
    bool half_ok = verify_buf(src, dst, 128);
    printf("  前半段验证: %s\n", half_ok ? "✓ 正确" : "✗ 错误");

    printf("\n步骤3：等待全传输完成中断\n");
    t0 = system_get_tick();
    while (!s4_tc_done) {
        __WFI();
        if (system_elapsed(t0, 1000U)) {
            printf("  注意：全传输中断未触发\n");
            goto done;
        }
    }
    printf("  全传输完成中断触发\n");

    // 验证后半段
    bool full_ok = verify_buf(src + 128, dst + 128, 128);
    printf("  后半段验证: %s\n", full_ok ? "✓ 正确" : "✗ 错误");
    printf("\n  乒乓缓冲工作状态: %s\n",
           (half_ok && full_ok) ? "✓ 正常" : "✗ 异常");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_1);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景5：链表传输（多段不连续数据）
//===========================================

/**
 * @brief 场景5：链表传输演示多段不连续数据的DMA搬移
 *
 * 创建3个链表节点，分别传输3段独立数据块，
 * 节点间通过next_lli指针自动链接，最后节点next_lli=0表示结束。
 */
void scenario_5_linked_list(void)
{
    print_separator();
    printf("=== 场景5：链表传输（多段不连续数据）===\n\n");

    static uint8_t src0[32], src1[32], src2[32];
    static uint8_t dst0[32], dst1[32], dst2[32];

    printf("步骤1：准备3段源数据\n");
    for (int i = 0; i < 32; i++) {
        src0[i] = (uint8_t)(0x10 + i);
        src1[i] = (uint8_t)(0x40 + i);
        src2[i] = (uint8_t)(0x70 + i);
        dst0[i] = dst1[i] = dst2[i] = 0;
    }
    print_hex("  段0源数据", src0, 32);
    print_hex("  段1源数据", src1, 32);
    print_hex("  段2源数据", src2, 32);

    printf("\n步骤2：构建链表节点\n");
    // CTRL: 字节位宽、地址递增、突发1、使能ITC
    uint32_t ctrl = 0;
    ctrl |= (1U << 31);   // ITC
    ctrl |= (1U << 28);   // DIORDD=01 目标递增
    ctrl |= (1U << 26);   // SIORSD=01 源递增
    ctrl |= (0U << 24);   // DWIDTH=00 字节
    ctrl |= (0U << 22);   // SWIDTH=00 字节
    ctrl |= (0U << 19);   // DBSIZE=000 burst=1
    ctrl |= (0U << 16);   // SBSIZE=000 burst=1
    ctrl |= 32U;          // TRANSFERSIZE=32

    static dma_lli_node_t nodes[3];
    dma_init_lli_node(&nodes[0], (uint32_t)src0, (uint32_t)dst0, ctrl,
                      &nodes[1], DMA_MASTER_1);
    dma_init_lli_node(&nodes[1], (uint32_t)src1, (uint32_t)dst1, ctrl,
                      &nodes[2], DMA_MASTER_1);
    dma_init_lli_node(&nodes[2], (uint32_t)src2, (uint32_t)dst2, ctrl,
                      NULL, DMA_MASTER_1);

    printf("  节点0 @ 0x%08X  →  节点1 @ 0x%08X  →  节点2 @ 0x%08X  →  NULL\n",
           (uint32_t)&nodes[0], (uint32_t)&nodes[1], (uint32_t)&nodes[2]);

    printf("\n步骤3：配置链表传输通道\n");
    dma_enable(DMA_1);
    dma_linked_list_config_t ll_cfg = {
        .nodes               = nodes,
        .node_count          = 3,
        .circular            = false,
        .lli_master          = DMA_MASTER_1,
        .direction           = DMA_MEM_TO_MEM,
        .src_periph_id       = 0,
        .dest_periph_id      = 0,
        .src_master          = DMA_MASTER_1,
        .dest_master         = DMA_MASTER_1,
        .bus_lock            = false,
        .tc_interrupt_enable    = true,
        .ht_interrupt_enable    = false,
        .error_interrupt_enable = false,
    };
    bool ok = dma_config_linked_list(DMA_1, DMA_CHANNEL_0, &ll_cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤4：启动并等待所有节点传输完成\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_0);
    ok = dma_wait_transfer_complete(DMA_1, DMA_CHANNEL_0, 1000);
    printf("  结果: %s\n", ok ? "链表传输完成" : "超时或错误");
    if (!ok) {
        goto done;
    }

    printf("\n步骤5：验证各段数据\n");
    print_hex("  段0目标数据", dst0, 32);
    print_hex("  段1目标数据", dst1, 32);
    print_hex("  段2目标数据", dst2, 32);

    bool all_ok = verify_buf(src0, dst0, 32) &
                  verify_buf(src1, dst1, 32) &
                  verify_buf(src2, dst2, 32);
    printf("  验证结果: %s\n", all_ok ? "✓ 所有段正确" : "✗ 存在错误");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_0);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景6：循环传输
//===========================================

static volatile uint32_t s6_cycle_count = 0;

static void scenario6_tc_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s6_cycle_count++;
    
    // 为了防止中断过于频繁，达到一定次数后禁用中断
    if (s6_cycle_count >= 1000) {
        dma_disable_tc_interrupt(DMA_1, DMA_CHANNEL_2);
    }
}

/**
 * @brief 场景6：循环传输模式
 *
 * 使用单节点自循环链表实现M2M数据的连续重复传输，
 * 每次传输完成触发回调计数，观察5秒内完成的循环次数。
 */
void scenario_6_circular(void)
{
    print_separator();
    printf("=== 场景6：循环传输模式 ===\n\n");

    // 增加传输数据量，降低中断频率
    static uint8_t src[4096] __attribute__((aligned(4)));
    static uint8_t dst[4096] __attribute__((aligned(4)));

    for (int i = 0; i < 4096; i++) {
        src[i] = (uint8_t)i;
        dst[i] = 0xFF;
    }

    printf("步骤1：配置循环DMA（4096字节，自动重复）\n");
    bool ok = dma_init_circular(DMA_1, DMA_CHANNEL_2,
                                (uint32_t)src, (uint32_t)dst, 4096);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤2：注册传输完成回调并使能NVIC中断\n");
    s6_cycle_count = 0;
    dma_register_tc_callback(DMA_1, DMA_CHANNEL_2, scenario6_tc_callback);
    // TC中断已在dma_init_circular中启用，无需再次调用dma_enable_tc_interrupt
    NVIC_EnableIRQ(DMA1_CH2_IRQn);
    printf("  结果: 回调已注册，NVIC已使能\n");

    printf("\n步骤3：启动循环传输，观察5秒\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_2);
    printf("  传输已启动，等待中...\n");
    printf("  注意：每次传输完成都会触发中断，循环计数最多1000次\n");

    uint32_t last = 0;
    for (int t = 0; t < 50; t++) {
        delay_ms(100);
        uint32_t current = s6_cycle_count;
        if (current != last) {
            printf("  [%u.%u秒] 已完成 %u 次循环\n",
                   t / 10, t % 10, current);
            last = current;
        }
        
        // 如果达到限制，提前退出
        if (current >= 1000) {
            printf("  已达到循环次数限制(1000)，提前结束观察\n");
            break;
        }
    }

    printf("\n步骤4：停止循环传输\n");
    dma_stop_channel(DMA_1, DMA_CHANNEL_2);
    printf("  总循环次数: %u\n", s6_cycle_count);
    
    if (s6_cycle_count == 0) {
        printf("  ⚠ 警告：循环次数为0，可能的原因：\n");
        printf("    1. 中断未触发（检查NVIC配置）\n");
        printf("    2. DMA未启动（检查使能位）\n");
        printf("    3. 链表配置错误\n");
        
        // 调试信息
        printf("  [调试] CONFIG = 0x%08X\n", (unsigned)DMAC1->C2CONFIG);
        printf("  [调试] CTRL   = 0x%08X\n", (unsigned)DMAC1->C2CTRL);
        printf("  [调试] LLI    = 0x%08X\n", (unsigned)DMAC1->C2LLI);
        printf("  [调试] INT_TC = 0x%08X\n", (unsigned)DMAC1->INTTCSTATUS);
        printf("  [调试] RAW_TC = 0x%08X\n", (unsigned)DMAC1->RAWINTTCSTATUS);
    }

    printf("\n步骤5：验证最后一次传输数据（检查前256字节）\n");
    printf("  验证结果: %s\n", verify_buf(src, dst, 256) ? "✓ 正确" : "✗ 错误");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_2);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景7：双缓冲传输（P2M模拟）
//===========================================

static volatile uint32_t s7_buf_index = 0;

static void scenario7_tc_callback(dma_instance_t inst, dma_channel_t ch)
{
    (void)inst;
    (void)ch;
    s7_buf_index++;
    
    // 防止中断过于频繁，达到一定次数后禁用中断
    if (s7_buf_index >= 100) {
        dma_disable_tc_interrupt(DMA_1, DMA_CHANNEL_3);
    }
}

/**
 * @brief 场景7：双缓冲传输
 *
 * 演示DMA双缓冲的API配置方式，适用于使用 dma_config_double_buffer()
 * 实现 P2M 方向的外设数据采集场景（如连续ADC采样）。
 *
 * @note 本场景仅演示 API 配置，P2M 模式需要真实外设提供请求信号
 *       （如 ADC、UART、SPI 等）。无外设时 DMA 将停在等待握手状态，
 *       不会产生传输和中断。如需在无外设环境下观察 DMA 工作状态，
 *       请参考场景6（循环传输）或场景1（M2M 基础传输）。
 */
void scenario_7_double_buffer(void)
{
    print_separator();
    printf("=== 场景7：双缓冲传输（P2M模式配置示例）===\n\n");

    // 注意：P2M模式需要真实外设提供DMA请求信号。
    // 无外设时DMA将停在等待握手状态，传输不会启动。
    // 本场景仅演示配置方式，实际使用时需连接外设并配置periph_id。

    // 增加缓冲区大小，降低中断频率
    static uint8_t buf0[2048];
    static uint8_t buf1[2048];
    static volatile uint8_t adc_reg = 0xAB;  // 模拟ADC数据寄存器

    memset(buf0, 0, 2048);
    memset(buf1, 0, 2048);

    printf("步骤1：配置双缓冲DMA（每个缓冲区2048字节）\n");
    printf("  缓冲区0 @ 0x%08X，缓冲区1 @ 0x%08X\n",
           (uint32_t)buf0, (uint32_t)buf1);

    dma_double_buffer_config_t db = {
        .buffer0_address     = (uint32_t)buf0,
        .buffer1_address     = (uint32_t)buf1,
        .periph_address      = (uint32_t)&adc_reg,
        .buffer_size         = 2048,  // 每缓冲区2048次传输（BYTE位宽=2048字节）
        .data_width          = DMA_WIDTH_BYTE,
        .direction           = DMA_PERIPH_TO_MEM,
        .periph_id           = 0,    // P2M模式下需替换为真实外设请求ID（如ADC1_REQ）
        .circular            = true,
        .tc_interrupt_enable    = true,
        .error_interrupt_enable = false,
    };

    bool ok = dma_config_double_buffer(DMA_1, DMA_CHANNEL_3, &db);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");
    if (!ok) {
        goto done;
    }

    printf("\n步骤2：注册传输完成回调并使能NVIC中断\n");
    s7_buf_index = 0;
    dma_register_tc_callback(DMA_1, DMA_CHANNEL_3, scenario7_tc_callback);
    NVIC_EnableIRQ(DMA1_CH3_IRQn);

    printf("\n步骤3：启动传输（注意：P2M模式需外设请求信号，此处仅演示配置）\n");
    printf("  如需在无外设环境下验证DMA，请使用场景1或场景6。\n");
    dma_start_channel(DMA_1, DMA_CHANNEL_3);

    printf("\n步骤4：停止双缓冲传输\n");
    dma_stop_channel(DMA_1, DMA_CHANNEL_3);
    printf("  双缓冲配置完成（需连接真实外设才能观察传输效果）\n");

done:
    dma_reset_channel(DMA_1, DMA_CHANNEL_3);
    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景8：不同数据位宽的封装/解封传输
//===========================================

/**
 * @brief 场景8：源目标不同数据位宽（自动封装/解封）
 *
 * 演示源为字节（8位）、目标为字（32位）的DMA传输，
 * 硬件自动将4个字节打包为1个32位字写入目标。
 * 以及源为字（32位）、目标为字节（8位）的反向传输。
 */
void scenario_8_data_width_packing(void)
{
    print_separator();
    printf("=== 场景8：不同数据位宽封装/解封 ===\n\n");

    // 字节到字的打包传输
    printf("--- 子场景A：字节源 → 字目标（8位→32位打包）---\n\n");

    static uint8_t  src_bytes[16];
    static uint32_t dst_words[4];

    for (int i = 0; i < 16; i++) {
        src_bytes[i] = (uint8_t)(i + 1);
    }
    memset(dst_words, 0, sizeof(dst_words));

    printf("步骤1：配置DMA（SWIDTH=字节，DWIDTH=字，传输4次）\n");
    dma_enable(DMA_1);
    dma_channel_config_t cfg = {
        .direction          = DMA_MEM_TO_MEM,
        .src_address        = (uint32_t)src_bytes,
        .dest_address       = (uint32_t)dst_words,
        .src_addr_mode      = DMA_ADDR_INCREMENT,
        .dest_addr_mode     = DMA_ADDR_INCREMENT,
        .src_width          = DMA_WIDTH_BYTE,
        .dest_width         = DMA_WIDTH_WORD,
        .src_burst          = DMA_BURST_1,
        .dest_burst         = DMA_BURST_1,
        .transfer_size      = 16,  // 16次字节读 → 自动打包为4个字
        .src_periph_id      = 0,
        .dest_periph_id     = 0,
        .src_master         = DMA_MASTER_1,
        .dest_master        = DMA_MASTER_1,
        .bus_lock           = false,
        .tc_interrupt_enable    = true,
        .ht_interrupt_enable    = false,
        .error_interrupt_enable = false,
    };
    bool ok = dma_config_channel(DMA_1, DMA_CHANNEL_0, &cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");

    if (ok) {
        dma_start_channel(DMA_1, DMA_CHANNEL_0);
        ok = dma_wait_transfer_complete(DMA_1, DMA_CHANNEL_0, 1000);
        printf("  传输: %s\n", ok ? "完成" : "失败");

        printf("\n步骤2：查看打包结果（小端模式）\n");
        for (int i = 0; i < 4; i++) {
            printf("  dst_words[%d] = 0x%08X  "
                   "（字节：0x%02X 0x%02X 0x%02X 0x%02X）\n",
                   i, dst_words[i],
                   (uint8_t)(dst_words[i]),
                   (uint8_t)(dst_words[i] >> 8),
                   (uint8_t)(dst_words[i] >> 16),
                   (uint8_t)(dst_words[i] >> 24));
        }
    }
    dma_reset_channel(DMA_1, DMA_CHANNEL_0);

    // 字到字节的解包传输
    printf("\n--- 子场景B：字源 → 字节目标（32位→8位解包）---\n\n");

    static uint32_t src_words[4]  = {0x04030201U, 0x08070605U,
                                     0x0C0B0A09U, 0x100F0E0DU};
    static uint8_t  dst_bytes[16];
    memset(dst_bytes, 0, sizeof(dst_bytes));

    printf("步骤1：配置DMA（SWIDTH=字，DWIDTH=字节，传输4次）\n");
    cfg.src_address  = (uint32_t)src_words;
    cfg.dest_address = (uint32_t)dst_bytes;
    cfg.src_width    = DMA_WIDTH_WORD;
    cfg.dest_width   = DMA_WIDTH_BYTE;
    cfg.transfer_size = 4;  // 4次字读 → 解包为16字节
    ok = dma_config_channel(DMA_1, DMA_CHANNEL_0, &cfg);
    printf("  结果: %s\n", ok ? "配置成功" : "配置失败");

    if (ok) {
        dma_start_channel(DMA_1, DMA_CHANNEL_0);
        ok = dma_wait_transfer_complete(DMA_1, DMA_CHANNEL_0, 1000);
        printf("  传输: %s\n", ok ? "完成" : "失败");

        printf("\n步骤2：查看解包结果\n");
        print_hex("  dst_bytes", dst_bytes, 16);

        bool data_ok = true;
        for (int i = 0; i < 16; i++) {
            if (dst_bytes[i] != (uint8_t)(i + 1)) {
                data_ok = false;
                break;
            }
        }
        printf("  验证结果: %s\n", data_ok ? "✓ 正确" : "✗ 错误");
    }

    dma_reset_channel(DMA_1, DMA_CHANNEL_0);
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
    printf("ACM32P4 DMA驱动示例程序 v2.0\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1: scenario_1_mem2mem_basic();      break;
        case 2: scenario_2_polling_flag();       break;
        case 3: scenario_3_interrupt_mode();     break;
        case 4: scenario_4_half_transfer();      break;
        case 5: scenario_5_linked_list();        break;
        case 6: scenario_6_circular();           break;
        case 7: scenario_7_double_buffer();      break;
        case 8: scenario_8_data_width_packing(); break;
        default:
            printf("错误：无效的场景编号 %d（有效范围: 1-8）\n", DEFAULT_SCENARIO);
            break;
    }

    while (1) {
        __WFI();
    }
}

//===========================================
// 中断服务函数
//===========================================
