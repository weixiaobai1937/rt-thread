/**
 * @file lpuart_example.c
 * @brief LPUART 模块功能示例程序
 *
 * @details
 * 演示8个场景：
 * 1. 基础 9600 波特率通信（RCL 32.768KHz）
 * 2. PCLK 分频高速波特率通信
 * 3. STOP 模式 START 位检测唤醒
 * 4. STOP 模式地址匹配唤醒
 * 5. 校验位与数据极性配置
 * 6. 结构体配置方式
 * 7. Bit Count Timeout 与总线空闲检测
 * 8. 波特率计算与精度验证
 *
 * @note 使用前需使能 LPUART1 时钟（CLK_LPUART1）
 * @note LPUART1 基地址：0x40008000
 */

#include <stdint.h>
#include <stdbool.h>

#include "acm32p4.h"
#include "hardware/lpuart.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1U  // 修改此值选择运行的场景

// LPUART1 引脚配置
// TX: PA9  (AF6)
// RX: PA10 (AF6)
// 备选: PB6 (TX, AF10), PB7 (RX, AF10)

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief LPUART 示例初始化：使能 LPUART1 时钟
 */
static void lpuart_example_init(void)
{
    clock_periph_enable(CLK_LPUART1);
}

/**
 * @brief 配置 LPUART1 的 GPIO 引脚
 *
 * PA9  → LPUART1_TX (AF6)
 * PA10 → LPUART1_RX (AF6)
 */
static void lpuart1_gpio_init(void)
{
    gpio_init(PA9);
    gpio_set_function(PA9, GPIO_AF_6);

    gpio_init(PA10);
    gpio_set_function(PA10, GPIO_AF_6);
    gpio_pull_up(PA10);
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
 * @brief 打印 LPUART 通道配置状态
 */
static void print_lpuart_status(lpuart_instance_t lpuart)
{
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ LPUART%u 状态                           │\n",
           (unsigned int)(lpuart + 1U));
    printf("  ├──────────────────────────────────────┤\n");

    printf("  │ TX 使能:     %-24s │\n",
           lpuart_tx_is_enabled(lpuart) ? "是" : "否");
    printf("  │ RX 使能:     %-24s │\n",
           lpuart_rx_is_enabled(lpuart) ? "是" : "否");
    printf("  │ DMA 使能:    %-24s │\n",
           lpuart_dma_is_enabled(lpuart) ? "是" : "否");

    lpuart_wlen_t wlen = lpuart_get_wlen(lpuart);
    printf("  │ 数据字长:    %-24s │\n",
           wlen == LPUART_WLEN_8 ? "8 位" : "7 位");

    lpuart_stop_t stop = lpuart_get_stop(lpuart);
    printf("  │ 停止位:      %-24s │\n",
           stop == LPUART_STOP_1 ? "1 位" : "2 位");

    lpuart_parity_t parity = lpuart_get_parity(lpuart);
    const char *parity_str;
    switch (parity) {
        case LPUART_PARITY_NONE: parity_str = "无"; break;
        case LPUART_PARITY_ODD:  parity_str = "奇校验"; break;
        case LPUART_PARITY_EVEN: parity_str = "偶校验"; break;
        case LPUART_PARITY_0:    parity_str = "强制 0"; break;
        case LPUART_PARITY_1:    parity_str = "强制 1"; break;
        default:                 parity_str = "未知"; break;
    }
    printf("  │ 校验模式:    %-24s │\n", parity_str);

    printf("  │ TX 极性取反: %-24s │\n",
           lpuart_get_tx_polarity(lpuart) ? "是" : "否");
    printf("  │ RX 极性取反: %-24s │\n",
           lpuart_get_rx_polarity(lpuart) ? "是" : "否");

    lpuart_wake_mode_t wake = lpuart_get_wake_mode(lpuart);
    const char *wake_str;
    switch (wake) {
        case LPUART_WAKE_START: wake_str = "START 位"; break;
        case LPUART_WAKE_1BYTE: wake_str = "1 字节完成"; break;
        case LPUART_WAKE_MATCH: wake_str = "地址匹配"; break;
        case LPUART_WAKE_NONE:  wake_str = "无"; break;
        default:                wake_str = "未知"; break;
    }
    printf("  │ 唤醒方式:    %-24s │\n", wake_str);
    printf("  │ 唤醒校验检查: %-23s │\n",
           lpuart_get_wake_check(lpuart) ? "是" : "否");

    printf("  │ IBAUD:        %-23u │\n",
           (unsigned int)lpuart_get_ibaud(lpuart));
    printf("  │ FBAUD:        0x%03X              │\n",
           (unsigned int)lpuart_get_fbaud(lpuart));
    printf("  │ RXSAM:        %-23u │\n",
           (unsigned int)lpuart_get_rxsam(lpuart));
    printf("  │ ADDR:         0x%02X                   │\n",
           (unsigned int)lpuart_get_addr(lpuart));

    printf("  │ 自动计时:    %-24s │\n",
           lpuart_get_auto_start(lpuart) ? "使能" : "禁止");
    printf("  │ BCNT 值:     %-23u │\n",
           (unsigned int)lpuart_get_bcnt_value(lpuart));

    printf("  │ RX 可用:     %-24s │\n",
           lpuart_rx_available(lpuart) ? "是" : "否");
    printf("  │ TX 空:       %-24s │\n",
           lpuart_tx_empty(lpuart) ? "是" : "否");
    printf("  │ TX 完成:     %-24s │\n",
           lpuart_tx_done(lpuart) ? "是" : "否");

    printf("  └──────────────────────────────────────┘\n");
}

/**
 * @brief 打印波特率计算精度
 */
static void print_baudrate_precision(uint32_t baud, uint32_t clk_hz, uint8_t bits_per_frame)
{
    uint8_t ibaud;
    uint16_t fbaud;
    uint8_t rxsam;

    lpuart_baudrate_calc(baud, clk_hz, bits_per_frame, &ibaud, &fbaud, &rxsam);
    uint32_t actual = lpuart_get_actual_baudrate(clk_hz, ibaud, fbaud, bits_per_frame);
    int32_t error = (int32_t)actual - (int32_t)baud;

    printf("  │ %-8lu │ %-10u │ %-8u │ 0x%03X    │ %-4u      │ %-10lu │ %-10ld │\n",
           (unsigned long)baud, (unsigned int)bits_per_frame,
           (unsigned int)ibaud, (unsigned int)fbaud, (unsigned int)rxsam,
           (unsigned long)actual, (unsigned long)error);
}

//===========================================
// 示例场景
//===========================================

/**
 * @brief 场景1：基础 9600 波特率通信（RCL 32.768KHz）
 *
 * 演示使用 lpuart_init_9600() 以 RCL 32.768KHz 时钟初始化 LPUART，
 * 进行基本的数据收发操作。
 */
void scenario_1_basic_9600(void)
{
    print_demo_header("场景1：基础 9600 波特率通信（RCL 32.768KHz）");

    printf("步骤1：快速初始化 LPUART（9600, 8N1）\n");
    lpuart_init_9600(LPUART_1, LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
    printf("  结果: LPUART1 已配置（RCL 32.768KHz, 9600 baud）\n");

    printf("\n步骤2：查看 LPUART 配置状态\n");
    print_lpuart_status(LPUART_1);

    printf("\n步骤3：发送单字节数据\n");
    lpuart_send_byte(LPUART_1, 0x55U);
    printf("  结果: 已发送 0x55\n");

    printf("\n步骤4：发送字符串\n");
    lpuart_send_string(LPUART_1, "Hello LPUART!\r\n");
    printf("  结果: 已发送 \"Hello LPUART!\\r\\n\"\n");

    printf("\n步骤5：发送缓冲区数据\n");
    uint8_t test_data[] = {0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    lpuart_send_buffer(LPUART_1, test_data, 5U);
    printf("  结果: 已发送 5 字节缓冲区 [01 02 03 04 05]\n");

    printf("\n步骤6：等待发送完成\n");
    if (lpuart_flush_tx(LPUART_1, 100000U)) {
        printf("  结果: 发送完成（TCIF=1）\n");
    } else {
        printf("  结果: 等待超时\n");
    }

    printf("\n步骤7：检查接收状态\n");
    if (lpuart_rx_available(LPUART_1)) {
        uint8_t data = lpuart_read_byte(LPUART_1);
        printf("  接收数据: 0x%02X\n", (unsigned int)data);
    } else {
        printf("  结果: 无接收数据（正常——未连接对端设备）\n");
    }

    printf("\n步骤8：禁止发送和接收\n");
    lpuart_tx_enable(LPUART_1, false);
    lpuart_rx_enable(LPUART_1, false);
    printf("  结果: TX/RX 已禁止\n");

    print_demo_end();
}

/**
 * @brief 场景2：PCLK 分频高速波特率通信
 *
 * 演示使用 lpuart_init_pclk() 以 PCLK 分频时钟初始化 LPUART，
 * 实现更高的波特率通信（如 115200）。
 */
void scenario_2_pclk_high_baud(void)
{
    print_demo_header("场景2：PCLK 分频高速波特率通信");

    printf("步骤1：使用 PCLK 分频初始化 LPUART（115200 baud）\n");
    printf("  假设 PCLK 经 LPUARTDIV 分频后 clk_hz = 2MHz\n");
    lpuart_init_pclk(LPUART_1, 115200U, 2000000U,
                     LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
    printf("  结果: LPUART1 已配置（clk=2MHz, 115200 baud, 8N1）\n");

    printf("\n步骤2：查看波特率配置参数\n");
    uint8_t ibaud = lpuart_get_ibaud(LPUART_1);
    uint16_t fbaud = lpuart_get_fbaud(LPUART_1);
    uint8_t rxsam = lpuart_get_rxsam(LPUART_1);
    printf("  IBAUD = %u (实际分频 = %u)\n",
           (unsigned int)ibaud, (unsigned int)(ibaud + 1U));
    printf("  FBAUD = 0x%03X\n", (unsigned int)fbaud);
    printf("  RXSAM = %u\n", (unsigned int)rxsam);

    printf("\n步骤3：计算实际波特率\n");
    uint32_t actual = lpuart_get_actual_baudrate(2000000U, ibaud, fbaud, 10U);
    printf("  目标: 115200, 实际: %lu, 误差: %ld\n",
           (unsigned long)actual, (long)((int32_t)actual - 115200));

    printf("\n步骤4：发送高速数据\n");
    for (uint8_t i = 0U; i < 10U; i++) {
        lpuart_send_byte(LPUART_1, 0x30U + i);
    }
    lpuart_flush_tx(LPUART_1, 100000U);
    printf("  结果: 已连续发送 10 字节\n");

    printf("\n步骤5：动态切换波特率为 9600\n");
    printf("  假设切换回 RCL 32.768KHz 时钟\n");
    lpuart_tx_enable(LPUART_1, false);
    lpuart_rx_enable(LPUART_1, false);
    lpuart_set_baudrate(LPUART_1, 2U, 0x952U, 1U);
    lpuart_tx_enable(LPUART_1, true);
    lpuart_rx_enable(LPUART_1, true);
    printf("  结果: 波特率已切换为 9600（IBAUD=2, FBAUD=0x952）\n");

    printf("\n步骤6：验证低速模式\n");
    print_lpuart_status(LPUART_1);

    lpuart_tx_enable(LPUART_1, false);
    lpuart_rx_enable(LPUART_1, false);

    print_demo_end();
}

/**
 * @brief 场景3：STOP 模式 START 位检测唤醒
 *
 * 演示使用 lpuart_init_wakeup() 配置 LPUART 为唤醒接收模式，
 * 在 STOP 模式下检测 START 位唤醒系统。
 */
void scenario_3_wakeup_start(void)
{
    print_demo_header("场景3：STOP 模式 START 位检测唤醒");

    printf("步骤1：初始化为 START 位唤醒模式\n");
    lpuart_init_wakeup(LPUART_1, LPUART_WAKE_START, 0U,
                       LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
    printf("  结果: LPUART1 配置为 START 位检测唤醒\n");

    printf("\n步骤2：查看唤醒配置\n");
    printf("  唤醒方式: START 位检测（RXWKS=00）\n");
    printf("  TX 使能:  %s\n", lpuart_tx_is_enabled(LPUART_1) ? "是" : "否（低功耗）");
    printf("  RX 使能:  %s\n", lpuart_rx_is_enabled(LPUART_1) ? "是" : "否");

    printf("\n步骤3：使能 START 位检测中断\n");
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_START, true);
    printf("  结果: STARTIE=1，检测到 START 位将触发中断\n");

    printf("\n步骤4：唤醒方式说明\n");
    printf("  ┌──────────────────────────────────────┐\n");
    printf("  │ 唤醒方式       │ 条件                │\n");
    printf("  ├──────────────────────────────────────┤\n");
    printf("  │ START 位检测   │ 检测到 START 位      │\n");
    printf("  │ 1 字节完成     │ 接收完成 1 字节     │\n");
    printf("  │ 地址匹配       │ RX 数据 == ADDR     │\n");
    printf("  │ 无唤醒         │ 禁用唤醒功能        │\n");
    printf("  └──────────────────────────────────────┘\n");

    printf("\n步骤5：切换为 1 字节接收唤醒\n");
    lpuart_set_wake_mode(LPUART_1, LPUART_WAKE_1BYTE);
    printf("  结果: 唤醒方式已切换为 1 字节接收完成\n");

    printf("\n步骤6：使能接收中断\n");
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_RX, true);
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_START, false);
    printf("  结果: RXIE=1, STARTIE=0\n");

    printf("\n步骤7：禁用唤醒功能\n");
    lpuart_set_wake_mode(LPUART_1, LPUART_WAKE_NONE);
    printf("  结果: 唤醒功能已禁用（RXWKS=11）\n");

    print_demo_end();
}

/**
 * @brief 场景4：STOP 模式地址匹配唤醒
 *
 * 演示地址匹配唤醒功能，通过设置 ADDR 寄存器匹配特定地址唤醒设备。
 */
void scenario_4_wakeup_match(void)
{
    print_demo_header("场景4：STOP 模式地址匹配唤醒");

    printf("步骤1：初始化为地址匹配唤醒模式（地址=0x55）\n");
    lpuart_init_wakeup(LPUART_1, LPUART_WAKE_MATCH, 0x55U,
                       LPUART_WLEN_8, LPUART_PARITY_NONE, LPUART_STOP_1);
    printf("  结果: LPUART1 配置为地址匹配唤醒（ADDR=0x55）\n");

    printf("\n步骤2：查看地址匹配配置\n");
    printf("  匹配地址: 0x%02X\n", (unsigned int)lpuart_get_addr(LPUART_1));
    printf("  唤醒方式: 地址匹配\n");
    printf("  说明: 收到 1 字节数据且 == 0x55 时唤醒\n");

    printf("\n步骤3：使能地址匹配中断\n");
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_MATCH, true);
    printf("  结果: MATCHIE=1\n");

    printf("\n步骤4：使能唤醒校验检查\n");
    printf("  WKCK=0: 不检查校验位和 STOP 位，直接触发唤醒/中断\n");
    printf("  WKCK=1: 检查校验位和 STOP 位都正确才触发\n");
    lpuart_set_wake_check(LPUART_1, true);
    printf("  结果: WKCK=1（已使能校验检查）\n");

    printf("\n步骤5：切换匹配地址\n");
    printf("  旧地址: 0x55\n");
    lpuart_set_addr(LPUART_1, 0xAAU);
    printf("  新地址: 0xAA\n");
    printf("  结果: 收到 0xAA 时唤醒\n");

    printf("\n步骤6：地址范围说明\n");
    printf("  ADDR 范围: 0x00 ~ 0xFF (0~255)\n");
    printf("  匹配方式: 接收数据 == ADDR 时触发\n");

    printf("\n步骤7：禁用地址匹配中断\n");
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_MATCH, false);
    printf("  结果: MATCHIE=0\n");

    print_demo_end();
}

/**
 * @brief 场景5：校验位与数据极性配置
 *
 * 演示 LPUART 的各种校验模式（奇/偶/0/1）和数据极性控制
 * 以及 7 位字长/2 位停止位配置。
 */
void scenario_5_parity_polarity(void)
{
    print_demo_header("场景5：校验位与数据极性配置");

    printf("步骤1：配置奇校验\n");
    lpuart_init_9600(LPUART_1, LPUART_WLEN_8, LPUART_PARITY_ODD, LPUART_STOP_1);
    printf("  PEN=1, SPS=0, EPS=0 → 奇校验\n");
    printf("  每帧比特数: Start(1) + Data(8) + Parity(1) + Stop(1) = 11\n");

    printf("\n步骤2：切换为偶校验\n");
    lpuart_set_parity(LPUART_1, LPUART_PARITY_EVEN);
    printf("  PEN=1, SPS=0, EPS=1 → 偶校验\n");

    printf("\n步骤3：切换为校验位强制 0\n");
    lpuart_set_parity(LPUART_1, LPUART_PARITY_0);
    printf("  PEN=1, SPS=1, EPS=1 → 校验位强制为 0\n");

    printf("\n步骤4：切换为校验位强制 1\n");
    lpuart_set_parity(LPUART_1, LPUART_PARITY_1);
    printf("  PEN=1, SPS=1, EPS=0 → 校验位强制为 1\n");

    printf("\n步骤5：禁用校验\n");
    lpuart_set_parity(LPUART_1, LPUART_PARITY_NONE);
    printf("  PEN=0 → 无校验\n");

    printf("\n步骤6：配置 7 位字长 + 2 位停止位\n");
    lpuart_set_wlen(LPUART_1, LPUART_WLEN_7);
    lpuart_set_stop(LPUART_1, LPUART_STOP_2);
    printf("  WLEN=1, STP2=1 → 7 位数据 + 2 位停止位\n");

    printf("\n步骤7：数据极性控制\n");
    printf("  TXPOL=0: 发送数据不取反（默认）\n");
    lpuart_set_tx_polarity(LPUART_1, true);
    printf("  TXPOL=1: 发送数据取反\n");
    lpuart_set_tx_polarity(LPUART_1, false);
    printf("  TXPOL=0: 恢复不取反\n");

    printf("\n  RXPOL=0: 接收数据不取反（默认）\n");
    lpuart_set_rx_polarity(LPUART_1, true);
    printf("  RXPOL=1: 接收数据取反\n");
    lpuart_set_rx_polarity(LPUART_1, false);
    printf("  RXPOL=0: 恢复不取反\n");

    printf("\n步骤8：校验模式对照表\n");
    printf("  ┌──────────┬─────┬─────┬─────┬──────────┐\n");
    printf("  │ 校验模式 │ PEN │ SPS │ EPS │ 说明     │\n");
    printf("  ├──────────┼─────┼─────┼─────┼──────────┤\n");
    printf("  │ 无校验   │ 0   │ -   │ -   │ 无校验位 │\n");
    printf("  │ 奇校验   │ 1   │ 0   │ 0   │ 奇校验   │\n");
    printf("  │ 偶校验   │ 1   │ 0   │ 1   │ 偶校验   │\n");
    printf("  │ 强制 0   │ 1   │ 1   │ 1   │ 校验位=0 │\n");
    printf("  │ 强制 1   │ 1   │ 1   │ 0   │ 校验位=1 │\n");
    printf("  └──────────┴─────┴─────┴─────┴──────────┘\n");

    // 恢复默认
    lpuart_set_wlen(LPUART_1, LPUART_WLEN_8);
    lpuart_set_stop(LPUART_1, LPUART_STOP_1);

    print_demo_end();
}

/**
 * @brief 场景6：结构体配置方式
 *
 * 演示使用 lpuart_config() 结构体进行完整配置，
 * 以及参数校验功能。
 */
void scenario_6_config_struct(void)
{
    print_demo_header("场景6：结构体配置方式");

    printf("步骤1：使能 LPUART 时钟\n");
    lpuart_enable_clock(LPUART_1);
    printf("  结果: CLK_LPUART1 已使能\n");

    printf("\n步骤2：使用结构体完整配置 LPUART\n");
    lpuart_config_t cfg = {
        .wlen = LPUART_WLEN_8,
        .stop = LPUART_STOP_1,
        .parity = LPUART_PARITY_NONE,
        .txpol = false,
        .rxpol = false,
        .wake_mode = LPUART_WAKE_START,
        .wake_check = true,
        .auto_start = false,
        .bcnt_value = 11U,
        .addr = 0x00U,
    };
    if (lpuart_config(LPUART_1, &cfg)) {
        printf("  结果: LPUART1 配置成功\n");
    } else {
        printf("  结果: LPUART1 配置失败\n");
    }

    printf("\n步骤3：单独配置波特率\n");
    lpuart_set_baudrate(LPUART_1, 2U, 0x952U, 1U);
    printf("  结果: 波特率已设置（9600, IBAUD=2, FBAUD=0x952）\n");

    printf("\n步骤4：使能收发\n");
    lpuart_tx_enable(LPUART_1, true);
    lpuart_rx_enable(LPUART_1, true);
    printf("  结果: TX/RX 已使能\n");

    printf("\n步骤5：查看完整配置\n");
    print_lpuart_status(LPUART_1);

    printf("\n步骤6：参数校验测试\n");
    printf("  测试无效字长...\n");
    lpuart_config_t invalid_cfg1 = {
        .wlen = (lpuart_wlen_t)99,
        .parity = LPUART_PARITY_NONE,
    };
    if (!lpuart_config(LPUART_1, &invalid_cfg1)) {
        printf("  结果: 无效 WLEN 被正确拒绝\n");
    }

    printf("  测试无效校验模式...\n");
    lpuart_config_t invalid_cfg2 = {
        .wlen = LPUART_WLEN_8,
        .parity = (lpuart_parity_t)99,
    };
    if (!lpuart_config(LPUART_1, &invalid_cfg2)) {
        printf("  结果: 无效校验模式被正确拒绝\n");
    }

    printf("  测试 BCNT 超范围...\n");
    lpuart_config_t invalid_cfg3 = {
        .wlen = LPUART_WLEN_8,
        .parity = LPUART_PARITY_NONE,
        .bcnt_value = 0xFFFFU,  // 超出最大值
    };
    if (!lpuart_config(LPUART_1, &invalid_cfg3)) {
        printf("  结果: 超范围 BCNT 被正确拒绝\n");
    }

    printf("\n步骤7：发送测试数据\n");
    lpuart_send_string(LPUART_1, "LPUART Config OK\r\n");
    printf("  结果: 已发送测试字符串\n");

    lpuart_tx_enable(LPUART_1, false);
    lpuart_rx_enable(LPUART_1, false);

    print_demo_end();
}

/**
 * @brief 场景7：Bit Count Timeout 与总线空闲检测
 *
 * 演示 BCNT（Bit Count Timeout）和总线空闲检测功能，
 * 以及 AUTO_START_EN 自动计时控制。
 */
void scenario_7_bcnt_idle(void)
{
    print_demo_header("场景7：Bit Count Timeout 与总线空闲检测");

    printf("步骤1：初始化 LPUART\n");
    lpuart_enable_clock(LPUART_1);

    lpuart_config_t cfg = {
        .wlen = LPUART_WLEN_8,
        .stop = LPUART_STOP_1,
        .parity = LPUART_PARITY_NONE,
        .txpol = false,
        .rxpol = false,
        .wake_mode = LPUART_WAKE_NONE,
        .wake_check = false,
        .auto_start = false,
        .bcnt_value = 11U,  // 默认值：11 比特位时长
        .addr = 0U,
    };
    lpuart_config(LPUART_1, &cfg);

    printf("  BCNT_VALUE: %u 比特位时长\n",
           (unsigned int)lpuart_get_bcnt_value(LPUART_1));
    printf("  AUTO_START_EN: %s\n",
           lpuart_get_auto_start(LPUART_1) ? "使能" : "禁止");

    printf("\n步骤2：配置 BCNT 值\n");
    printf("  BCNT 用于检测接收数据帧之间的超时\n");
    printf("  BCNT_VALUE = 0: 不清除 BCNT_VALUE\n");
    printf("  BCNT_VALUE = N: 计时 N 个比特位时长\n");
    printf("  示例: 8N1 每帧 10 比特，BCNT=11 表示约 1.1 帧时长\n");

    printf("  ┌──────────┬──────────────────────┐\n");
    printf("  │ BCNT 值  │ 在 9600 baud 的时长  │\n");
    printf("  ├──────────┼──────────────────────┤\n");
    uint16_t bcnt_values[] = {10U, 11U, 20U, 50U, 100U};
    for (uint8_t i = 0U; i < 5U; i++) {
        uint32_t us = (uint32_t)bcnt_values[i] * 1000000U / 9600U;
        printf("  │ %-8u │ %-6lu us            │\n",
               (unsigned int)bcnt_values[i], (unsigned long)us);
    }
    printf("  └──────────┴──────────────────────┘\n");

    printf("\n步骤3：设置 BCNT=50（约 5.2ms @ 9600）\n");
    lpuart_set_bcnt_value(LPUART_1, 50U);
    printf("  结果: BCNT_VALUE=%u\n",
           (unsigned int)lpuart_get_bcnt_value(LPUART_1));

    printf("\n步骤4：使能硬件自动计时\n");
    printf("  AUTO_START_EN=0: 需手动触发比特计时\n");
    printf("  AUTO_START_EN=1: 接收到 STOP 位自动启动计时\n");
    lpuart_set_auto_start(LPUART_1, true);
    printf("  结果: AUTO_START_EN=1\n");

    printf("\n步骤5：使能 BCNT 和 IDLE 中断\n");
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_BCNT, true);
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_IDLE, true);
    printf("  结果: BCNTIE=1, IDLEIE=1\n");

    printf("\n步骤6：总线空闲检测说明\n");
    printf("  IDLEIF: 检测到总线空闲时置位\n");
    printf("  空闲判定: 总线在至少 1 帧时长内无活动\n");
    printf("  IDLEIF 写 1 清零\n");

    printf("\n步骤7：清除中断使能和标志\n");
    uint32_t sr = lpuart_get_status(LPUART_1);
    printf("  SR = 0x%08lX\n", (unsigned long)sr);
    if (sr & (1U << 16)) {
        printf("  BCNTIF=1 → 清除\n");
        lpuart_clear_flag(LPUART_1, LPUART_FLAG_BCNTIF);
    }
    if (sr & (1U << 17)) {
        printf("  IDLEIF=1 → 清除\n");
        lpuart_clear_flag(LPUART_1, LPUART_FLAG_IDLEIF);
    }
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_BCNT, false);
    lpuart_irq_enable(LPUART_1, LPUART_IRQ_IDLE, false);

    print_demo_end();
}

/**
 * @brief 场景8：波特率计算与精度验证
 *
 * 演示 lpuart_baudrate_calc() 计算函数和
 * lpuart_get_actual_baudrate() 验证函数。
 */
void scenario_8_baudrate_calculation(void)
{
    print_demo_header("场景8：波特率计算与精度验证");

    printf("步骤1：波特率计算原理\n");
    printf("  IBAUD = floor(Fclk / Baud) - 1（最小 2，即 3 分频）\n");
    printf("  FBAUD 每位对应 Start→STOP，1=加一个时钟\n");
    printf("  RXSAM = IBAUD >> 1（可微调）\n");
    printf("  实际分频 = (IBAUD+1) + popcount(FBAUD)/bits_per_frame\n\n");

    printf("步骤2：RCL 32.768KHz → 9600 baud 计算\n");
    printf("  divisor = 32768/9600 = 3.413\n");
    printf("  IBAUD = 3 - 1 = 2（3 分频）\n");
    printf("  fraction = 0.413\n");
    printf("  8N1 (10 比特): ones = round(0.413 × 10) ≈ 4\n");
    printf("  FBAUD 分布: 从 Start→STOP，4 个 1 均匀分布\n\n");

    printf("步骤3：各种时钟频率下的波特率精度\n");
    printf("  ┌──────────┬────────────┬──────────┬───────────┬──────────┬────────────┬────────────┐\n");
    printf("  │ 目标波特 │ 每帧比特数 │ IBAUD    │ FBAUD    │ RXSAM    │ 实际波特   │ 误差       │\n");
    printf("  ├──────────┼────────────┼──────────┼───────────┼──────────┼────────────┼────────────┤\n");

    // RCL 32.768KHz → 9600
    print_baudrate_precision(9600U, 32768U, 10U);
    // PCLK 2MHz → 115200
    print_baudrate_precision(115200U, 2000000U, 10U);
    // PCLK 2MHz → 57600
    print_baudrate_precision(57600U, 2000000U, 10U);
    // PCLK 32MHz → 115200
    print_baudrate_precision(115200U, 32000000U, 10U);
    // PCLK 32MHz → 460800
    print_baudrate_precision(460800U, 32000000U, 10U);
    // RCL 32.768KHz → 4800
    print_baudrate_precision(4800U, 32768U, 10U);
    // RCL 32.768KHz → 2400
    print_baudrate_precision(2400U, 32768U, 10U);

    printf("  └──────────┴────────────┴──────────┴───────────┴──────────┴────────────┴────────────┘\n");

    printf("\n步骤4：不同帧格式对波特率精度的影响（32.768KHz → 9600）\n");
    printf("  ┌────────────────┬────────────┬──────────┬───────────┬────────────┬────────────┐\n");
    printf("  │ 格式           │ 每帧比特数 │ IBAUD    │ FBAUD    │ 实际波特   │ 误差       │\n");
    printf("  ├────────────────┼────────────┼──────────┼───────────┼────────────┼────────────┤\n");
    print_baudrate_precision(9600U, 32768U, 10U);  // 8N1
    print_baudrate_precision(9600U, 32768U, 12U);  // 8E1
    printf("  └────────────────┴────────────┴──────────┴───────────┴────────────┴────────────┘\n");
    printf("  注: 8N1=Start+8bit+Stop=10, 8E1=Start+8bit+Parity+Stop=12\n");

    printf("\n步骤5：IBAUD 极限值\n");
    printf("  IBAUD_MIN = 2（3 分频）\n");
    printf("  IBAUD_MAX = 254（255 分频）\n");
    printf("  在 32.768KHz 下:\n");
    printf("    最小波特率 = 32768/256 = 128\n");
    printf("    最大波特率 = 32768/3 = ~10923\n");
    printf("  但实际最大 = 9600（RCL 限制），更高请用 PCLK\n");

    printf("\n步骤6：FBAUD 均匀分布算法\n");
    printf("  用 Bresenham 算法均匀分布 1:\n");
    uint8_t ibaud;
    uint16_t fbaud;
    uint8_t rxsam;
    lpuart_baudrate_calc(115200U, 2000000U, 10U, &ibaud, &fbaud, &rxsam);
    printf("  目标: 115200 @ 2MHz, bits=10\n");
    printf("  IBAUD=%u, FBAUD=0x%03X, RXSAM=%u\n",
           (unsigned int)ibaud, (unsigned int)fbaud, (unsigned int)rxsam);
    printf("  FBAUD 二进制: 0b");
    for (int8_t i = 11; i >= 0; i--) {
        printf("%u", (unsigned int)((fbaud >> (unsigned int)i) & 1U));
    }
    printf("\n");
    uint32_t actual = lpuart_get_actual_baudrate(2000000U, ibaud, fbaud, 10U);
    printf("  实际波特率: %lu, 误差: %ld\n",
           (unsigned long)actual, (long)((int32_t)actual - 115200));

    print_demo_end();
}

//===========================================
// 中断回调
//===========================================

static void lpuart_demo_callback(lpuart_instance_t lpuart, uint32_t flags)
{
    (void)lpuart;
    if (flags & (uint32_t)LPUART_FLAG_RXIF) {
        printf("  [IRQ] 接收完成\n");
    }
    if (flags & (uint32_t)LPUART_FLAG_TCIF) {
        printf("  [IRQ] 发送完成\n");
    }
    if (flags & (uint32_t)LPUART_FLAG_STARTIF) {
        printf("  [IRQ] START 位检测\n");
    }
    if (flags & (uint32_t)LPUART_FLAG_MATCHIF) {
        printf("  [IRQ] 地址匹配\n");
    }
    if (flags & (uint32_t)LPUART_FLAG_BCNTIF) {
        printf("  [IRQ] Bit Count Timeout\n");
    }
    if (flags & (uint32_t)LPUART_FLAG_IDLEIF) {
        printf("  [IRQ] 总线空闲\n");
    }
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
    printf("LPUART 模块示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    // 配置 GPIO 引脚
    lpuart1_gpio_init();

    // 注册中断回调
    lpuart_register_callback(LPUART_1, lpuart_demo_callback);

    // 根据场景号运行对应的示例
    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_basic_9600();
            break;
        case 2:
            scenario_2_pclk_high_baud();
            break;
        case 3:
            scenario_3_wakeup_start();
            break;
        case 4:
            scenario_4_wakeup_match();
            break;
        case 5:
            scenario_5_parity_polarity();
            break;
        case 6:
            scenario_6_config_struct();
            break;
        case 7:
            scenario_7_bcnt_idle();
            break;
        case 8:
            scenario_8_baudrate_calculation();
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
