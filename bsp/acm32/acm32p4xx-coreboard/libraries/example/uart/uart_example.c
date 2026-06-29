/**
 * @file uart_example.c
 * @brief ACM32P4 UART驱动示例程序
 *
 * @details
 * 演示10个场景：
 * 1. 基础收发（轮询模式）
 * 2. FIFO模式收发
 * 3. 中断模式接收
 * 4. 空闲中断接收不定长数据
 * 5. DMA模式收发
 * 6. 硬件流控（CTS/RTS）
 * 7. RS485半双工通信
 * 8. LIN总线通信
 * 9. 多机通信（地址匹配）
 * 10. 波特率自适应
 */

#include <string.h>
#include "SEGGER_RTT.h"
#include "acm32p4.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/system.h"
#include "hardware/dma.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)

#define DEFAULT_SCENARIO 5

// UART1引脚配置
// TX: PA9 (AF1)
// RX: PA10 (AF1)
// CTS: PA11 (AF1)
// RTS: PA12 (AF1)

//===========================================
// 全局变量
//===========================================

static volatile bool rx_complete = false;
static volatile bool tx_complete = false;
static volatile bool idle_detected = false;
static volatile bool error_occurred = false;

static uint8_t rx_buffer[256];
static uint32_t rx_count = 0;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

void uart1_gpio_init(void)
{
    gpio_init(PA9);
    gpio_set_function(PA9, GPIO_AF_1);

    gpio_init(PA10);
    gpio_set_function(PA10, GPIO_AF_1);
    gpio_pull_up(PA10);
}

void uart1_flow_control_gpio_init(void)
{
    gpio_init(PA11);
    gpio_set_function(PA11, GPIO_AF_1);
    gpio_init(PA12);
    gpio_set_function(PA12, GPIO_AF_1);
}

void print_hex(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    if (len % 16 != 0) {
        printf("\n");
    }
}

//===========================================
// 回调函数
//===========================================

void uart_rx_handler(uart_num_t uart, uint8_t data)
{
    (void)uart;
    if (rx_count < sizeof(rx_buffer)) {
        rx_buffer[rx_count++] = data;
    }
}

void uart_tx_handler(uart_num_t uart)
{
    (void)uart;
    tx_complete = true;
}

void uart_error_handler(uart_num_t uart, uart_error_t error)
{
    (void)uart;
    error_occurred = true;
    printf("[错误] UART错误: 0x%02X\n", error);
}

void uart_idle_handler(uart_num_t uart)
{
    /* 限制单次中断最多读取 256 字节，防止死循环 */
    uint32_t read_limit = 256;
    while (!uart_is_rx_fifo_empty(uart) && read_limit--) {
        if (rx_count < sizeof(rx_buffer)) {
            rx_buffer[rx_count++] = uart_getc(uart);
        } else {
            uart_getc(uart);  /* 丢弃超出缓冲区的数据，防止 FIFO 溢出 */
        }
    }
    idle_detected = true;
}

//===========================================
// 场景1：基础收发（轮询模式）
//===========================================

void scenario_1_basic_polling(void)
{
    print_separator();
    printf("=== 场景1：基础收发（轮询模式） ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();
    printf("  TX: PA9 (AF1)\n");
    printf("  RX: PA10 (AF1)\n");

    printf("\n步骤2：初始化UART_1（115200, 8N1）\n");
    uart_init_default(UART_1, 115200);
    printf("  结果: 成功\n");

    printf("\n步骤3：发送字符串\n");
    const char *msg = "Hello, UART!\r\n";
    uart_puts(UART_1, msg);
    printf("  已发送: %s", msg);

    printf("\n步骤4：发送单个字符\n");
    uart_putc(UART_1, 'A');
    uart_putc(UART_1, 'B');
    uart_putc(UART_1, 'C');
    printf("  已发送: ABC\n");

    printf("\n步骤5：发送数据缓冲区\n");
    uint8_t data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    uint32_t sent = uart_write(UART_1, data, sizeof(data));
    printf("  已发送 %u 字节: ", sent);
    print_hex(data, sizeof(data));

    printf("\n步骤6：等待接收数据（超时5秒）\n");
    printf("  请在UART终端发送数据...\n");
    uint32_t timeout = 5000;
    while (timeout-- && uart_is_rx_fifo_empty(UART_1)) {
        delay_ms(1);
    }

    if (!uart_is_rx_fifo_empty(UART_1)) {
        printf("  接收到数据:\n");
        while (!uart_is_rx_fifo_empty(UART_1)) {
            uint8_t ch = uart_getc(UART_1);
            printf("    0x%02X ('%c')\n", ch, (ch >= 32 && ch < 127) ? ch : '.');
        }
    } else {
        printf("  超时，未收到数据\n");
    }

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景2：FIFO模式收发
//===========================================

void scenario_2_fifo_mode(void)
{
    print_separator();
    printf("=== 场景2：FIFO模式收发 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1\n");
    uart_config_t config = {
        .baudrate = 115200,
        .word_length = UART_WORD_LENGTH_8BIT,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .endian = UART_ENDIAN_LSB_FIRST
    };
    uart_init_custom(UART_1, &config);

    printf("\n步骤3：配置FIFO\n");
    uart_fifo_config_t fifo_config = {
        .enable = true,
        .rx_threshold = UART_RX_FIFO_THRESHOLD_8_BYTES,
        .tx_threshold = UART_TX_FIFO_THRESHOLD_8_BYTES
    };
    uart_config_fifo(UART_1, &fifo_config);
    printf("  RX阈值: 8字节 (1/2满)\n");
    printf("  TX阈值: 8字节 (1/2剩余)\n");

    printf("\n步骤4：批量发送数据\n");
    uint8_t test_data[32];
    for (uint32_t i = 0; i < sizeof(test_data); i++) {
        test_data[i] = i;
    }

    printf("  发送32字节数据:\n  ");
    print_hex(test_data, sizeof(test_data));

    uint32_t sent = 0;
    while (sent < sizeof(test_data)) {
        if (!uart_is_tx_fifo_full(UART_1)) {
            uart_putc(UART_1, test_data[sent]);
            sent++;
        }
    }

    while (uart_is_busy(UART_1)) {
        delay_ms(1);
    }
    printf("  发送完成\n");

    printf("\n步骤5：检查FIFO状态\n");
    printf("  TX FIFO空: %s\n", uart_is_tx_fifo_empty(UART_1) ? "是" : "否");
    printf("  TX FIFO满: %s\n", uart_is_tx_fifo_full(UART_1) ? "是" : "否");
    printf("  RX FIFO空: %s\n", uart_is_rx_fifo_empty(UART_1) ? "是" : "否");
    printf("  RX FIFO满: %s\n", uart_is_rx_fifo_full(UART_1) ? "是" : "否");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景3：中断模式接收
//===========================================

void scenario_3_interrupt_rx(void)
{
    print_separator();
    printf("=== 场景3：中断模式接收 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1\n");
    uart_init_default(UART_1, 115200);

    printf("\n步骤3：配置FIFO和中断\n");
    uart_fifo_config_t fifo_config = {
        .enable = true,
        .rx_threshold = UART_RX_FIFO_THRESHOLD_1_BYTE,
        .tx_threshold = UART_TX_FIFO_THRESHOLD_8_BYTES
    };
    uart_config_fifo(UART_1, &fifo_config);

    printf("\n步骤4：注册回调函数\n");
    uart_register_rx_callback(UART_1, uart_rx_handler);
    uart_register_error_callback(UART_1, uart_error_handler);

    printf("\n步骤5：使能接收中断\n");
    uart_interrupt_enable(UART_1, UART_INT_RX);
    printf("  接收中断已使能\n");

    NVIC_SetPriority(UART1_IRQn, 2U);
    NVIC_EnableIRQ(UART1_IRQn);

    printf("\n步骤6：等待接收数据（10秒）\n");
    printf("  请在UART终端发送数据...\n");

    rx_count = 0;
    error_occurred = false;

    for (uint32_t i = 0; i < 100; i++) {
        delay_ms(100);

        if (rx_count > 0) {
            printf("  收到 %u 字节: ", rx_count);
            print_hex(rx_buffer, rx_count);
            rx_count = 0;
        }

        if (error_occurred) {
            printf("  检测到错误\n");
            error_occurred = false;
        }
    }

    printf("\n步骤7：禁用中断\n");
    uart_interrupt_disable(UART_1, UART_INT_RX);
    NVIC_DisableIRQ(UART1_IRQn);

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景4：空闲中断接收不定长数据
//===========================================

void scenario_4_idle_interrupt(void)
{
    print_separator();
    printf("=== 场景4：空闲中断接收不定长数据 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1\n");
    uart_init_default(UART_1, 115200);

    printf("\n步骤3：配置FIFO\n");
    uart_fifo_config_t fifo_config = {
        .enable = true,
        .rx_threshold = UART_RX_FIFO_THRESHOLD_8_BYTES,
        .tx_threshold = UART_TX_FIFO_THRESHOLD_8_BYTES
    };
    uart_config_fifo(UART_1, &fifo_config);

    printf("\n步骤4：注册回调函数\n");
    uart_register_rx_callback(UART_1, uart_rx_handler);
    uart_register_idle_callback(UART_1, uart_idle_handler);

    printf("\n步骤5：使能接收和空闲中断\n");
    uart_interrupt_enable(UART_1, UART_INT_RX | UART_INT_IDLE);
    printf("  接收中断和空闲中断已使能\n");

    NVIC_SetPriority(UART1_IRQn, 2U);
    NVIC_EnableIRQ(UART1_IRQn);

    printf("\n步骤6：等待接收数据包（10秒）\n");
    printf("  说明：当检测到总线空闲时，表示一包数据接收完成\n");
    printf("  请在UART终端发送数据...\n");

    rx_count = 0;
    idle_detected = false;

    for (uint32_t i = 0; i < 100; i++) {
        delay_ms(100);

        if (idle_detected) {
            printf("\n  [空闲中断] 接收到完整数据包\n");
            printf("  数据长度: %u 字节\n", rx_count);
            if (rx_count > 0) {
                printf("  数据内容:\n  ");
                print_hex(rx_buffer, rx_count);
            }
            rx_count = 0;
            idle_detected = false;
        }
    }

    printf("\n步骤7：禁用中断\n");
    uart_interrupt_disable(UART_1, UART_INT_RX | UART_INT_IDLE);
    NVIC_DisableIRQ(UART1_IRQn);

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景5：DMA模式收发
//===========================================

void scenario_5_dma_mode(void)
{
    print_separator();
    printf("=== 场景5：DMA模式收发 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1（115200, 8N1）\n");
    uart_init_default(UART_1, 115200);

    printf("\n步骤3：配置FIFO\n");
    uart_fifo_config_t fifo_config = {
        .enable = true,
        .rx_threshold = UART_RX_FIFO_THRESHOLD_8_BYTES,
        .tx_threshold = UART_TX_FIFO_THRESHOLD_8_BYTES
    };
    uart_config_fifo(UART_1, &fifo_config);

    printf("\n步骤4：使能UART DMA请求\n");
    uart_dma_config_t dma_config = {
        .tx_dma_enable = true,
        .rx_dma_enable = true,
        .dma_disable_on_error = false
    };
    uart_config_dma(UART_1, &dma_config);
    printf("  TX DMA请求: 使能\n");
    printf("  RX DMA请求: 使能\n");

    printf("\n步骤5：使能DMA控制器\n");
    dma_enable(DMA_1);
    printf("  DMA1控制器: 使能\n");

    printf("\n步骤6：配置DMA通道（内存→UART TX）\n");
    static uint8_t tx_buf[32];
    for (uint32_t i = 0; i < sizeof(tx_buf); i++) {
        tx_buf[i] = 'A' + (i % 26);
    }

    uint32_t uart_tx_addr = uart_get_base_address(UART_1);
    printf("  UART TX寄存器地址: 0x%08X\n", uart_tx_addr);
    printf("  发送缓冲区地址: 0x%08X\n", (uint32_t)tx_buf);
    printf("  传输长度: %u 字节\n", sizeof(tx_buf));

    dma_channel_config_t tx_dma = {
        .direction          = DMA_MEM_TO_PERIPH,
        .src_address        = (uint32_t)tx_buf,
        .dest_address       = uart_tx_addr,
        .src_addr_mode      = DMA_ADDR_INCREMENT,
        .dest_addr_mode     = DMA_ADDR_FIXED,
        .src_width          = DMA_WIDTH_BYTE,
        .dest_width         = DMA_WIDTH_BYTE,
        .src_burst          = DMA_BURST_1,
        .dest_burst         = DMA_BURST_1,
        .transfer_size      = sizeof(tx_buf),
        .src_periph_id      = 0,
        .dest_periph_id     = 5,    // UART1_TX
        .src_master         = DMA_MASTER_1,
        .dest_master        = DMA_MASTER_1,
        .bus_lock           = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = true
    };
    dma_config_channel(DMA_1, DMA_CHANNEL_0, &tx_dma);
    printf("  TX通道: DMA1_CH0\n");

    printf("\n步骤7：配置DMA通道（UART RX→内存）\n");
    static uint8_t rx_buf[32];
    memset(rx_buf, 0, sizeof(rx_buf));

    uint32_t uart_rx_addr = uart_tx_addr;  // UART数据寄存器地址相同
    printf("  UART RX寄存器地址: 0x%08X\n", uart_rx_addr);
    printf("  接收缓冲区地址: 0x%08X\n", (uint32_t)rx_buf);

    dma_channel_config_t rx_dma = {
        .direction          = DMA_PERIPH_TO_MEM,
        .src_address        = uart_rx_addr,
        .dest_address       = (uint32_t)rx_buf,
        .src_addr_mode      = DMA_ADDR_FIXED,
        .dest_addr_mode     = DMA_ADDR_INCREMENT,
        .src_width          = DMA_WIDTH_BYTE,
        .dest_width         = DMA_WIDTH_BYTE,
        .src_burst          = DMA_BURST_1,
        .dest_burst         = DMA_BURST_1,
        .transfer_size      = sizeof(rx_buf),
        .src_periph_id      = 6,    // UART1_RX
        .dest_periph_id     = 0,
        .src_master         = DMA_MASTER_1,
        .dest_master        = DMA_MASTER_1,
        .bus_lock           = false,
        .tc_interrupt_enable = true,
        .ht_interrupt_enable = false,
        .error_interrupt_enable = true
    };
    dma_config_channel(DMA_1, DMA_CHANNEL_1, &rx_dma);
    printf("  RX通道: DMA1_CH1\n");

    printf("\n步骤8：启动DMA传输\n");

    // 先启动RX DMA，等待接收
    dma_enable_tc_interrupt(DMA_1, DMA_CHANNEL_1);
    dma_start_channel(DMA_1, DMA_CHANNEL_1);
    printf("  RX DMA已启动，等待接收...\n");

    // 启动TX DMA发送数据
    dma_enable_tc_interrupt(DMA_1, DMA_CHANNEL_0);
    dma_start_channel(DMA_1, DMA_CHANNEL_0);
    printf("  TX DMA已启动，发送 %u 字节\n", sizeof(tx_buf));

    printf("\n步骤9：等待DMA传输完成（超时5秒）\n");
    uint32_t timeout = 5000;

    // 等待TX完成
    while (timeout-- && !dma_is_transfer_complete(DMA_1, DMA_CHANNEL_0)) {
        delay_ms(1);
    }
    if (dma_is_transfer_complete(DMA_1, DMA_CHANNEL_0)) {
        printf("  TX DMA传输完成\n");
    } else {
        printf("  TX DMA传输超时\n");
    }

    // 等待RX完成（需要对端发送数据）
    printf("  等待RX DMA接收（超时5秒，请在对端发送数据）...\n");
    timeout = 5000;
    while (timeout-- && !dma_is_transfer_complete(DMA_1, DMA_CHANNEL_1)) {
        delay_ms(1);
    }
    if (dma_is_transfer_complete(DMA_1, DMA_CHANNEL_1)) {
        printf("  RX DMA接收完成\n");
        printf("  接收到 %u 字节:\n  ", sizeof(rx_buf));
        print_hex(rx_buf, sizeof(rx_buf));
    } else {
        printf("  RX DMA接收超时（对端未发送数据）\n");
    }

    printf("\n步骤10：检查DMA状态\n");
    printf("  TX通道完成: %s\n", dma_is_transfer_complete(DMA_1, DMA_CHANNEL_0) ? "是" : "否");
    printf("  RX通道完成: %s\n", dma_is_transfer_complete(DMA_1, DMA_CHANNEL_1) ? "是" : "否");
    printf("  TX通道错误: %s\n", dma_has_error(DMA_1, DMA_CHANNEL_0) ? "是" : "否");
    printf("  RX通道错误: %s\n", dma_has_error(DMA_1, DMA_CHANNEL_1) ? "是" : "否");

    printf("\n步骤11：停止DMA通道\n");
    dma_stop_channel(DMA_1, DMA_CHANNEL_0);
    dma_stop_channel(DMA_1, DMA_CHANNEL_1);
    printf("  DMA通道已停止\n");

    printf("\n步骤12：禁用UART DMA\n");
    dma_config.tx_dma_enable = false;
    dma_config.rx_dma_enable = false;
    uart_config_dma(UART_1, &dma_config);
    printf("  UART DMA已禁用\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景6：硬件流控（CTS/RTS）
//===========================================

void scenario_6_flow_control(void)
{
    print_separator();
    printf("=== 场景6：硬件流控（CTS/RTS） ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();
    uart1_flow_control_gpio_init();
    printf("  TX:  PA9 (AF1)\n");
    printf("  RX:  PA10 (AF1)\n");
    printf("  CTS: PA11 (AF1)\n");
    printf("  RTS: PA12 (AF1)\n");

    printf("\n步骤2：初始化UART_1\n");
    uart_init_default(UART_1, 115200);

    printf("\n步骤3：配置硬件流控\n");
    uart_flow_control_config_t flow_config = {
        .cts_enable = true,
        .rts_enable = true
    };
    uart_config_flow_control(UART_1, &flow_config);
    printf("  CTS流控: 使能（检查对方是否准备好）\n");
    printf("  RTS流控: 使能（告诉对方是否准备好）\n");

    printf("\n步骤4：检查CTS状态\n");
    bool cts_state = uart_get_cts(UART_1);
    printf("  CTS当前状态: %s\n", cts_state ? "有效（可以发送）" : "无效（暂停发送）");

    printf("\n步骤5：手动控制RTS（测试）\n");
    uart_set_rts(UART_1, true);
    printf("  RTS设为有效（准备接收）\n");
    delay_ms(100);

    uart_set_rts(UART_1, false);
    printf("  RTS设为无效（暂停接收）\n");
    delay_ms(100);

    printf("\n步骤6：发送测试数据\n");
    const char *msg = "Flow control test\r\n";
    uart_puts(UART_1, msg);
    printf("  已发送: %s", msg);
    printf("  说明：如果CTS无效，发送会自动暂停\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景7：RS485半双工通信
//===========================================

void scenario_7_rs485_mode(void)
{
    print_separator();
    printf("=== 场景7：RS485半双工通信 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();
    uart1_flow_control_gpio_init();
    printf("  TX:  PA9 (AF1)\n");
    printf("  RX:  PA10 (AF1)\n");
    printf("  DE:  PA12 (AF1, 复用RTS)\n");

    printf("\n步骤2：初始化UART_1\n");
    uart_init_default(UART_1, 9600);
    printf("  波特率: 9600 (RS485常用)\n");

    printf("\n步骤3：配置RS485模式\n");
    uart_rs485_config_t rs485_config = {
        .enable = true,
        .de_polarity = UART_DE_POLARITY_HIGH,
        .de_assert = 1,
        .de_deassert = 1
    };
    uart_config_rs485(UART_1, &rs485_config);
    printf("  RS485模式: 使能\n");
    printf("  DE极性: 高电平有效\n");
    printf("  DE提前时间: 1比特\n");
    printf("  DE延迟时间: 1比特\n");

    printf("\n步骤4：发送数据（自动控制DE）\n");
    uint8_t rs485_data[] = {0x01, 0x03, 0x00, 0x00, 0x00, 0x02, 0xC4, 0x0B};
    printf("  发送Modbus RTU查询帧:\n  ");
    print_hex(rs485_data, sizeof(rs485_data));

    uart_write(UART_1, rs485_data, sizeof(rs485_data));

    while (uart_is_busy(UART_1)) {
        delay_ms(1);
    }
    printf("  发送完成（DE自动控制）\n");

    printf("\n步骤5：切换到接收模式\n");
    printf("  说明：RS485半双工，DE自动在发送时拉高，发送完成后拉低\n");
    printf("        接收时DE保持低电平\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景8：LIN总线通信
//===========================================

void scenario_8_lin_mode(void)
{
    print_separator();
    printf("=== 场景8：LIN总线通信 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1\n");
    uart_init_default(UART_1, 19200);
    printf("  波特率: 19200 (LIN常用)\n");

    printf("\n步骤3：配置LIN模式\n");
    uart_lin_config_t lin_config = {
        .enable = true,
        .break_length = 13
    };
    uart_config_lin(UART_1, &lin_config);
    printf("  LIN模式: 使能\n");
    printf("  Break长度: 13比特\n");

    printf("\n步骤4：发送LIN帧头（主机模式）\n");

    printf("  (1) 发送Break信号（间隔场）\n");
    uart_send_lin_break(UART_1, 13);
    delay_ms(1);

    printf("  (2) 发送同步场 (0x55)\n");
    uart_putc(UART_1, 0x55);
    delay_ms(1);

    printf("  (3) 发送标识符场 (0x3C)\n");
    uart_putc(UART_1, 0x3C);
    delay_ms(1);

    printf("  (4) 发送数据场\n");
    uint8_t lin_data[] = {0x01, 0x02, 0x03, 0x04};
    uart_write(UART_1, lin_data, sizeof(lin_data));

    printf("  (5) 发送校验场 (0x0A)\n");
    uart_putc(UART_1, 0x0A);

    printf("\n步骤5：LIN帧发送完成\n");
    printf("  完整帧结构: Break + 0x55 + ID + Data + Checksum\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景9：多机通信（地址匹配）
//===========================================

void scenario_9_multiprocessor(void)
{
    print_separator();
    printf("=== 场景9：多机通信（地址匹配） ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1\n");
    uart_config_t config = {
        .baudrate = 9600,
        .word_length = UART_WORD_LENGTH_9BIT,
        .stop_bits = UART_STOP_BITS_1,
        .parity = UART_PARITY_NONE,
        .endian = UART_ENDIAN_LSB_FIRST
    };
    uart_init_custom(UART_1, &config);
    printf("  字长: 9位（第9位为地址标记）\n");

    printf("\n步骤3：配置多机通信模式\n");
    uart_multiprocessor_config_t mp_config = {
        .enable = true,
        .wakeup_method = UART_WAKEUP_ADDRESS,
        .address_mode = UART_ADDRESS_MODE_4BIT,
        .address = 0x05
    };
    uart_config_multiprocessor(UART_1, &mp_config);
    printf("  唤醒方式: 地址标记\n");
    printf("  地址模式: 4位\n");
    printf("  本设备地址: 0x05\n");

    printf("\n步骤4：进入静默模式\n");
    uart_enter_mute_mode(UART_1);
    printf("  设备进入静默模式，只接收地址匹配的数据\n");

    printf("\n步骤5：等待地址匹配（模拟）\n");
    printf("  说明：当接收到地址为0x05的数据时，自动退出静默模式\n");
    printf("        地址不匹配的数据将被忽略\n");

    delay_ms(1000);

    printf("\n步骤6：退出静默模式（手动）\n");
    uart_exit_mute_mode(UART_1);
    printf("  设备退出静默模式\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景10：波特率自适应
//===========================================

void scenario_10_autobaud(void)
{
    print_separator();
    printf("=== 场景10：波特率自适应 ===\n\n");

    printf("步骤1：配置GPIO引脚\n");
    uart1_gpio_init();

    printf("\n步骤2：初始化UART_1（初始波特率9600）\n");
    uart_init_default(UART_1, 9600);
    printf("  初始波特率: 9600\n");

    printf("\n步骤3：启动波特率自适应\n");
    uart_autobaud_start(UART_1);
    printf("  自适应已启动\n");
    printf("  说明：需要主机发送0x7F字符进行波特率检测\n");

    printf("\n步骤4：等待自适应完成（超时5秒）\n");
    printf("  请在UART终端发送0x7F...\n");

    uint32_t timeout = 5000;
    while (timeout-- && !uart_is_autobaud_complete(UART_1)) {
        delay_ms(1);
    }

    if (uart_is_autobaud_complete(UART_1)) {
        printf("  自适应完成！\n");

        printf("\n步骤5：停止自适应\n");
        uart_autobaud_stop(UART_1);

        printf("\n步骤6：测试新波特率\n");
        const char *msg = "Autobaud success!\r\n";
        uart_puts(UART_1, msg);
        printf("  已发送: %s", msg);
        printf("  说明：波特率已自动调整为主机的发送速率\n");
    } else {
        printf("  超时，自适应失败\n");
        uart_autobaud_stop(UART_1);
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

    print_separator();
    printf("ACM32P4 UART驱动示例\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1:  scenario_1_basic_polling();    break;
        case 2:  scenario_2_fifo_mode();        break;
        case 3:  scenario_3_interrupt_rx();     break;
        case 4:  scenario_4_idle_interrupt();   break;
        case 5:  scenario_5_dma_mode();         break;
        case 6:  scenario_6_flow_control();     break;
        case 7:  scenario_7_rs485_mode();       break;
        case 8:  scenario_8_lin_mode();         break;
        case 9:  scenario_9_multiprocessor();   break;
        case 10: scenario_10_autobaud();        break;
        default:
            printf("无效的场景编号！\n");
            break;
    }

    printf("\n");
    print_separator();
    printf("示例程序结束\n");
    print_separator();

    while (1) {
        delay_ms(1000);
    }

    return 0;
}

//===========================================
// 中断服务函数
//===========================================

void UART1_IRQHandler(void)
{
    uart_irq_handler(UART_1);
}
