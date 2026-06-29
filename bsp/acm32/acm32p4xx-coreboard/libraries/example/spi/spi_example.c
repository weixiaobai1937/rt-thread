/**
 * @file spi_example.c
 * @brief ACM32P4 SPI 驱动示例程序
 * @author ACM32P4 SDK Team
 * @version 1.0
 *
 * @details
 * 演示9个场景：
 * 1. 主机阻塞发送
 * 2. 主机阻塞接收
 * 3. 主机全双工收发
 * 4. 结构体完整配置
 * 5. 中断框架演示
 * 6. 内存映射参数配置
 * 7. SPI DMA 轮询回环传输
 * 8. SPI DMA 中断回环传输
 * 9. 单线归零码（SID）LED 控制
 *
 * @note DMA 场景建议将 PA7(MOSI) 与 PA6(MISO) 短接，形成自回环路径，
 *       便于在单板上验证 SPI1 的 DMA 收发链路。
 * @note SID 场景需要连接 WS2812 等 LED 灯带至 MOSI 引脚。
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"
#include "hardware/dma.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

//===========================================
// 配置选项
//===========================================

#define printf(...)                 SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO            1

#define SPI_DEMO_INST               SPI_1
#define SPI_DEMO_SCK_PIN            PA5
#define SPI_DEMO_MISO_PIN           PA6
#define SPI_DEMO_MOSI_PIN           PA7
#define SPI_DEMO_CS_PIN             PA4
#define SPI_DEMO_GPIO_AF            GPIO_AF_0
#define SPI_DEMO_BAUD_HZ            8000000U
#define SPI1_TX_DMA_REQ_ID          1U
#define SPI1_RX_DMA_REQ_ID          2U
#define SPI_DMA_POLL_TX_CH          DMA_CHANNEL_0
#define SPI_DMA_POLL_RX_CH          DMA_CHANNEL_1
#define SPI_DMA_IRQ_TX_CH           DMA_CHANNEL_2
#define SPI_DMA_IRQ_RX_CH           DMA_CHANNEL_3

//===========================================
// 全局变量
//===========================================

static volatile uint32_t s_irq_flags;
static volatile uint32_t s_irq_count;
static volatile bool s_dma_tx_done;
static volatile bool s_dma_rx_done;
static volatile bool s_dma_tx_error;
static volatile bool s_dma_rx_error;

//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s[%u]: ", label, (uint32_t)len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

static void spi1_gpio_init(void)
{
    gpio_init(SPI_DEMO_SCK_PIN);
    gpio_init(SPI_DEMO_MISO_PIN);
    gpio_init(SPI_DEMO_MOSI_PIN);
    gpio_init(SPI_DEMO_CS_PIN);

    gpio_set_function(SPI_DEMO_SCK_PIN, SPI_DEMO_GPIO_AF);
    gpio_set_function(SPI_DEMO_MISO_PIN, SPI_DEMO_GPIO_AF);
    gpio_set_function(SPI_DEMO_MOSI_PIN, SPI_DEMO_GPIO_AF);
    gpio_set_function(SPI_DEMO_CS_PIN, SPI_DEMO_GPIO_AF);

    gpio_set_drive_strength(SPI_DEMO_SCK_PIN, GPIO_DRIVE_16MA);
    gpio_set_drive_strength(SPI_DEMO_MOSI_PIN, GPIO_DRIVE_16MA);
    gpio_set_drive_strength(SPI_DEMO_CS_PIN, GPIO_DRIVE_16MA);
    gpio_pull_up(SPI_DEMO_MISO_PIN);
}

static void on_spi_event(const spi_irq_context_t *ctx)
{
    s_irq_flags = ctx->flags;
    s_irq_count++;

    if (ctx->flags & (uint32_t)SPI_FLAG_RX_BATCH_DONE) {
        spi_clear_flags(ctx->inst, (uint32_t)SPI_FLAG_RX_BATCH_DONE);
    }
    if (ctx->flags & (uint32_t)SPI_FLAG_TX_BATCH_DONE) {
        spi_clear_flags(ctx->inst, (uint32_t)SPI_FLAG_TX_BATCH_DONE);
    }
}

static void spi_dma_prepare(uint16_t transfer_size)
{
    spi_tx_config_t tx_cfg = {
        .enable = false,
        .batch_mode = false,
        .dma_enable = true,
        .dma_level = 1U,
        .dummy_data = 0xFFU,
    };
    spi_rx_config_t rx_cfg = {
        .enable = false,
        .dma_enable = true,
        .dma_level = 1U,
        .sample_shift = SPI_SAMPLE_SHIFT_NONE,
        .multi_sample_enable = false,
        .msda_mask = 0U,
        .msdha_mask = 0U,
    };

    spi_config_tx(SPI_DEMO_INST, &tx_cfg);
    spi_config_rx(SPI_DEMO_INST, &rx_cfg);
    spi_tx_fifo_reset(SPI_DEMO_INST);
    spi_rx_fifo_reset(SPI_DEMO_INST);
    spi_clear_flags(SPI_DEMO_INST,
                    (uint32_t)SPI_FLAG_TX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_RX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_set_batch_size(SPI_DEMO_INST, transfer_size);
    spi_select_cs(SPI_DEMO_INST, SPI_CS_0);
}

static void spi_dma_finish(void)
{
    spi_wait_idle(SPI_DEMO_INST, 100U);
    spi_tx_disable(SPI_DEMO_INST);
    spi_rx_disable(SPI_DEMO_INST);
    spi_release_cs(SPI_DEMO_INST);
    spi_clear_flags(SPI_DEMO_INST,
                    (uint32_t)SPI_FLAG_TX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_RX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_BATCH_DONE);
}

static void on_spi_dma_tx_tc(dma_instance_t instance, dma_channel_t channel)
{
    (void)instance;
    (void)channel;
    s_dma_tx_done = true;
}

static void on_spi_dma_rx_tc(dma_instance_t instance, dma_channel_t channel)
{
    (void)instance;
    (void)channel;
    s_dma_rx_done = true;
}

static void on_spi_dma_tx_error(dma_instance_t instance, dma_channel_t channel)
{
    (void)instance;
    (void)channel;
    s_dma_tx_error = true;
}

static void on_spi_dma_rx_error(dma_instance_t instance, dma_channel_t channel)
{
    (void)instance;
    (void)channel;
    s_dma_rx_error = true;
}

//===========================================
// 示例场景
//===========================================

void scenario_1_master_transmit(void)
{
    uint8_t tx_buf[] = { 0x9FU, 0x00U, 0x00U, 0x00U };
    bool ok;

    print_separator();
    printf("=== 场景1：主机阻塞发送 ===\n\n");

    printf("步骤1：初始化 GPIO 和 SPI1 主模式（8MHz，模式0）\n");
    spi1_gpio_init();
    ok = spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);
    printf("  init_master: %s\n", ok ? "成功" : "失败");

    printf("\n步骤2：选择 CS0，发送 4 字节命令帧\n");
    spi_select_cs(SPI_DEMO_INST, SPI_CS_0);
    print_hex("  发送", tx_buf, sizeof(tx_buf));

    ok = spi_transmit(SPI_DEMO_INST, tx_buf, sizeof(tx_buf), 100U);
    printf("  结果: %s\n", ok ? "发送成功" : "发送失败");

    printf("\n完成！\n");
    print_separator();
}

void scenario_2_master_receive(void)
{
    uint8_t rx_buf[8] = { 0 };
    bool ok;

    print_separator();
    printf("=== 场景2：主机阻塞接收 ===\n\n");

    printf("步骤1：初始化 GPIO 和 SPI1 主模式（4MHz，模式3）\n");
    spi1_gpio_init();
    ok = spi_init_master(SPI_DEMO_INST, 4000000U, SPI_WORK_MODE_3);
    printf("  init_master: %s\n", ok ? "成功" : "失败");

    printf("\n步骤2：配置 Dummy=0xFF，并接收 8 字节数据\n");
    spi_tx_config_t tx_cfg = {
        .enable = false,
        .batch_mode = false,
        .dma_enable = false,
        .dma_level = 0U,
        .dummy_data = 0xFFU,
    };
    spi_config_tx(SPI_DEMO_INST, &tx_cfg);

    ok = spi_receive(SPI_DEMO_INST, rx_buf, sizeof(rx_buf), 100U);
    printf("  结果: %s\n", ok ? "接收成功" : "接收失败");
    if (ok) {
        print_hex("  接收", rx_buf, sizeof(rx_buf));
    }

    printf("\n完成！\n");
    print_separator();
}

void scenario_3_master_transceive(void)
{
    uint8_t tx_buf[] = { 0x9FU, 0xAAU, 0x55U, 0x00U };
    uint8_t rx_buf[sizeof(tx_buf)] = { 0 };
    bool ok;

    print_separator();
    printf("=== 场景3：主机全双工收发 ===\n\n");

    printf("步骤1：初始化 GPIO 和 SPI1 主模式（8MHz，模式0）\n");
    spi1_gpio_init();
    ok = spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);
    printf("  init_master: %s\n", ok ? "成功" : "失败");

    printf("\n步骤2：发送并同步接收 %u 字节（适合回环或外部从机）\n",
           (uint32_t)sizeof(tx_buf));
    print_hex("  发送", tx_buf, sizeof(tx_buf));

    ok = spi_transceive(SPI_DEMO_INST, tx_buf, rx_buf, sizeof(tx_buf), 100U);
    printf("  结果: %s\n", ok ? "收发成功" : "收发失败");
    if (ok) {
        print_hex("  接收", rx_buf, sizeof(rx_buf));
    }

    printf("\n完成！\n");
    print_separator();
}

void scenario_4_struct_configure(void)
{
    spi_config_t cfg;
    spi_baud_config_t baud;
    spi_tx_config_t tx_cfg;
    spi_rx_config_t rx_cfg;

    print_separator();
    printf("=== 场景4：结构体完整配置 ===\n\n");

    printf("步骤1：初始化 SPI1 GPIO\n");
    spi1_gpio_init();
    clock_periph_enable(CLK_SPI1);

    printf("步骤2：完整配置 CTL / BAUD / TX_CTL / RX_CTL\n");
    cfg.mode = SPI_MODE_MASTER;
    cfg.work_mode = SPI_WORK_MODE_0;
    cfg.line_mode = SPI_LINE_MODE_1;
    cfg.bit_order = SPI_BIT_ORDER_MSB_FIRST;
    cfg.io_direction = SPI_IO_DIRECTION_AUTO;
    cfg.slave_enable = false;
    cfg.sck_filter_enable = false;
    cfg.cs_reset_enable = false;
    cfg.cs_filter_enable = false;
    cfg.cs_time = 2U;
    cfg.software_cs_enable = false;
    cfg.software_cs_inactive = true;
    spi_configure(SPI_DEMO_INST, &cfg);

    baud.div1 = 10U;
    baud.div2 = 0U;
    spi_config_baud(SPI_DEMO_INST, &baud);

    tx_cfg.enable = false;
    tx_cfg.batch_mode = false;
    tx_cfg.dma_enable = false;
    tx_cfg.dma_level = 0U;
    tx_cfg.dummy_data = 0xFFU;
    spi_config_tx(SPI_DEMO_INST, &tx_cfg);

    rx_cfg.enable = false;
    rx_cfg.dma_enable = false;
    rx_cfg.dma_level = 0U;
    rx_cfg.sample_shift = SPI_SAMPLE_SHIFT_1_HCLK;
    rx_cfg.multi_sample_enable = false;
    rx_cfg.msda_mask = 0U;
    rx_cfg.msdha_mask = 0U;
    spi_config_rx(SPI_DEMO_INST, &rx_cfg);

    spi_set_tx_delay(SPI_DEMO_INST, 4U);
    spi_set_batch_size(SPI_DEMO_INST, 16U);

    printf("  STATUS = 0x%08X\n", spi_get_flags(SPI_DEMO_INST));
    printf("  配置完成，当前 HCLK = %u Hz\n", clock_get_hclk_hz());

    printf("\n完成！\n");
    print_separator();
}

void scenario_5_interrupt_demo(void)
{
    uint8_t tx_buf[] = { 0xA5U, 0x5AU, 0x11U, 0x22U };
    uint8_t rx_buf[sizeof(tx_buf)] = { 0 };
    uint64_t t0;
    bool ok;
    size_t recv;

    print_separator();
    printf("=== 场景5：中断框架演示 ===\n\n");

    printf("步骤1：初始化系统、GPIO 和 SPI1 主模式\n");
    system_init();
    spi1_gpio_init();
    spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);

    printf("步骤2：注册回调，使能 RX_FIFO_NOT_EMPTY / RX_BATCH_DONE / TX_BATCH_DONE 中断\n");
    s_irq_flags = 0U;
    s_irq_count = 0U;
    spi_irq_register(SPI_DEMO_INST, on_spi_event);
    spi_irq_enable(SPI_DEMO_INST, (spi_irq_t)(
                                              SPI_IRQ_RX_BATCH_DONE |
                                              SPI_IRQ_TX_BATCH_DONE));
    NVIC_SetPriority(SPI1_IRQn, 2U);
    NVIC_EnableIRQ(SPI1_IRQn);

    printf("步骤3：使用中断完成一次 4 字节全双工收发，观察回调计数\n");
    spi_tx_fifo_reset(SPI_DEMO_INST);
    spi_rx_fifo_reset(SPI_DEMO_INST);
    spi_clear_flags(SPI_DEMO_INST,
                    (uint32_t)SPI_FLAG_TX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_RX_BATCH_DONE |
                    (uint32_t)SPI_FLAG_BATCH_DONE);
    spi_set_batch_size(SPI_DEMO_INST, (uint32_t)sizeof(tx_buf));
    spi_select_cs(SPI_DEMO_INST, SPI_CS_0);
    spi_tx_enable(SPI_DEMO_INST);
    spi_rx_enable(SPI_DEMO_INST);

    for (size_t i = 0; i < sizeof(tx_buf); i++) {
        spi_write_data(SPI_DEMO_INST, tx_buf[i]);
    }
    spi_assert_cs(SPI_DEMO_INST);

    ok = false;
    t0 = system_get_tick();
    while (!system_elapsed(t0, 100U)) {
        if ((s_irq_flags & ((uint32_t)SPI_FLAG_TX_BATCH_DONE | (uint32_t)SPI_FLAG_RX_BATCH_DONE)) ==
            ((uint32_t)SPI_FLAG_TX_BATCH_DONE | (uint32_t)SPI_FLAG_RX_BATCH_DONE)) {
            ok = true;
            break;
        }
        __WFI();
    }

    recv = 0U;
    while ((recv < sizeof(rx_buf)) && !spi_flag_is_set(SPI_DEMO_INST, SPI_FLAG_RX_FIFO_EMPTY)) {
        rx_buf[recv++] = spi_read_data(SPI_DEMO_INST);
    }

    spi_tx_disable(SPI_DEMO_INST);
    spi_rx_disable(SPI_DEMO_INST);
    spi_release_cs(SPI_DEMO_INST);
    spi_irq_disable(SPI_DEMO_INST, (spi_irq_t)(SPI_IRQ_RX_BATCH_DONE | SPI_IRQ_TX_BATCH_DONE));

    printf("  收发结果: %s\n", ok ? "成功" : "失败/超时");
    if (recv > 0U) {
        print_hex("  接收", rx_buf, (unsigned int)recv);
    }
    printf("  回调次数: %u\n", s_irq_count);
    printf("  最近一次 flags: 0x%08X\n", s_irq_flags);

    printf("\n完成！\n");
    print_separator();
}

void scenario_6_memory_config(void)
{
    spi_memory_config_t mem_cfg;
    spi_mem_command_t cmd_cfg;

    print_separator();
    printf("=== 场景6：内存映射参数配置 ===\n\n");

    printf("步骤1：初始化系统、GPIO 和 SPI1 主模式\n");
    system_init();
    spi1_gpio_init();
    spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);

    printf("步骤2：配置常见 Nor Flash 读写参数\n");
    mem_cfg.access_enable = false;
    mem_cfg.cs_timeout_enable = true;
    mem_cfg.continuous_mode_enable = false;
    mem_cfg.send_instr_once_enable = false;
    mem_cfg.write_alter_byte_enable = false;
    mem_cfg.read_alter_byte_enable = false;
    mem_cfg.alter_byte_size = SPI_ALTER_BYTE_SIZE_8;
    mem_cfg.write_data_enable = true;
    mem_cfg.read_data_enable = true;
    mem_cfg.dummy_cycle = SPI_DUMMY_CYCLE_8;
    mem_cfg.addr_width = SPI_ADDR_WIDTH_24;
    mem_cfg.instr_mode = SPI_MEM_LINE_MODE_1;
    mem_cfg.addr_mode = SPI_MEM_LINE_MODE_1;
    mem_cfg.alter_byte_mode = SPI_MEM_LINE_MODE_1;
    mem_cfg.data_mode = SPI_MEM_LINE_MODE_1;
    spi_config_memory(SPI_DEMO_INST, &mem_cfg);

    cmd_cfg.read_cmd = 0x03U;
    cmd_cfg.write_cmd = 0x02U;
    spi_config_mem_command(SPI_DEMO_INST, &cmd_cfg);
    spi_set_alter_byte(SPI_DEMO_INST, 0x00000000U);
    spi_set_cs_timeout(SPI_DEMO_INST, 256U);

    printf("  内存映射参数已写入 MEMO_ACC/CMD/ALTER_BYTE/CS_TOUT_VAL\n");
    printf("  STATUS = 0x%08X\n", spi_get_flags(SPI_DEMO_INST));

    printf("\n完成！\n");
    print_separator();
}

void scenario_7_dma_polling_loopback(void)
{
    uint8_t tx_buf[] = { 0x3CU, 0xA5U, 0x5AU, 0xC3U, 0x12U, 0x34U, 0x56U, 0x78U };
    uint8_t rx_buf[sizeof(tx_buf)] = { 0 };
    bool ok_tx;
    bool ok_rx;

    print_separator();
    printf("=== 场景7：SPI DMA 轮询回环传输 ===\n\n");

    printf("步骤1：初始化系统、GPIO 和 SPI1 主模式\n");
    system_init();
    spi1_gpio_init();
    spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);

    printf("步骤2：配置 DMA 通道（TX=DMA1_CH0，RX=DMA1_CH1）\n");
    dma_reset_channel(DMA_1, SPI_DMA_POLL_TX_CH);
    dma_reset_channel(DMA_1, SPI_DMA_POLL_RX_CH);
    ok_tx = dma_init_mem2periph(DMA_1, SPI_DMA_POLL_TX_CH,
                                SPI1_TX_DMA_REQ_ID,
                                (uint32_t)tx_buf,
                                (uint32_t)&SPI1->DAT,
                                (uint16_t)sizeof(tx_buf),
                                DMA_WIDTH_BYTE);
    ok_rx = dma_init_periph2mem(DMA_1, SPI_DMA_POLL_RX_CH,
                                SPI1_RX_DMA_REQ_ID,
                                (uint32_t)&SPI1->DAT,
                                (uint32_t)rx_buf,
                                (uint16_t)sizeof(rx_buf),
                                DMA_WIDTH_BYTE);
    printf("  TX DMA: %s\n", ok_tx ? "配置成功" : "配置失败");
    printf("  RX DMA: %s\n", ok_rx ? "配置成功" : "配置失败");
    if (!ok_tx || !ok_rx) {
        goto done_poll;
    }

    printf("\n步骤3：配置 SPI TX/RX DMA 请求并启动双通道传输\n");
    spi_dma_prepare((uint16_t)sizeof(tx_buf));
    print_hex("  发送", tx_buf, sizeof(tx_buf));

    dma_start_channel(DMA_1, SPI_DMA_POLL_RX_CH);
    dma_start_channel(DMA_1, SPI_DMA_POLL_TX_CH);
    spi_rx_enable(SPI_DEMO_INST);
    spi_tx_enable(SPI_DEMO_INST);
    spi_assert_cs(SPI_DEMO_INST);

    ok_rx = dma_wait_transfer_complete(DMA_1, SPI_DMA_POLL_RX_CH, 1000000U);
    ok_tx = dma_wait_transfer_complete(DMA_1, SPI_DMA_POLL_TX_CH, 1000000U);
    spi_dma_finish();

    printf("  RX DMA 结果: %s\n", ok_rx ? "完成" : "失败/超时");
    printf("  TX DMA 结果: %s\n", ok_tx ? "完成" : "失败/超时");
    if (ok_tx && ok_rx) {
        print_hex("  接收", rx_buf, sizeof(rx_buf));
        printf("  回环校验: %s\n",
               (memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0) ? "成功" : "失败");
    }

done_poll:
    dma_reset_channel(DMA_1, SPI_DMA_POLL_TX_CH);
    dma_reset_channel(DMA_1, SPI_DMA_POLL_RX_CH);
    printf("\n完成！\n");
    print_separator();
}

void scenario_8_dma_interrupt_loopback(void)
{
    uint8_t tx_buf[] = { 0x55U, 0xAAU, 0x11U, 0x22U, 0x33U, 0x44U, 0x77U, 0x88U };
    uint8_t rx_buf[sizeof(tx_buf)] = { 0 };
    bool ok_tx;
    bool ok_rx;

    print_separator();
    printf("=== 场景8：SPI DMA 中断回环传输 ===\n\n");

    printf("步骤1：初始化系统、GPIO 和 SPI1 主模式\n");
    system_init();
    spi1_gpio_init();
    spi_init_master(SPI_DEMO_INST, SPI_DEMO_BAUD_HZ, SPI_WORK_MODE_0);

    printf("步骤2：配置 DMA 通道并注册回调\n");
    s_dma_tx_done = false;
    s_dma_rx_done = false;
    s_dma_tx_error = false;
    s_dma_rx_error = false;

    dma_reset_channel(DMA_1, SPI_DMA_IRQ_TX_CH);
    dma_reset_channel(DMA_1, SPI_DMA_IRQ_RX_CH);
    ok_tx = dma_init_mem2periph(DMA_1, SPI_DMA_IRQ_TX_CH,
                                SPI1_TX_DMA_REQ_ID,
                                (uint32_t)tx_buf,
                                (uint32_t)&SPI1->DAT,
                                (uint16_t)sizeof(tx_buf),
                                DMA_WIDTH_BYTE);
    ok_rx = dma_init_periph2mem(DMA_1, SPI_DMA_IRQ_RX_CH,
                                SPI1_RX_DMA_REQ_ID,
                                (uint32_t)&SPI1->DAT,
                                (uint32_t)rx_buf,
                                (uint16_t)sizeof(rx_buf),
                                DMA_WIDTH_BYTE);
    printf("  TX DMA: %s\n", ok_tx ? "配置成功" : "配置失败");
    printf("  RX DMA: %s\n", ok_rx ? "配置成功" : "配置失败");
    if (!ok_tx || !ok_rx) {
        goto done_irq;
    }

    dma_register_tc_callback(DMA_1, SPI_DMA_IRQ_TX_CH, on_spi_dma_tx_tc);
    dma_register_tc_callback(DMA_1, SPI_DMA_IRQ_RX_CH, on_spi_dma_rx_tc);
    dma_register_error_callback(DMA_1, SPI_DMA_IRQ_TX_CH, on_spi_dma_tx_error);
    dma_register_error_callback(DMA_1, SPI_DMA_IRQ_RX_CH, on_spi_dma_rx_error);
    NVIC_SetPriority(DMA1_CH2_IRQn, 2U);
    NVIC_SetPriority(DMA1_CH3_IRQn, 2U);
    NVIC_EnableIRQ(DMA1_CH2_IRQn);
    NVIC_EnableIRQ(DMA1_CH3_IRQn);

    printf("\n步骤3：启动 DMA 与 SPI，进入 WFI 等待中断完成\n");
    spi_dma_prepare((uint16_t)sizeof(tx_buf));
    print_hex("  发送", tx_buf, sizeof(tx_buf));

    dma_start_channel(DMA_1, SPI_DMA_IRQ_RX_CH);
    dma_start_channel(DMA_1, SPI_DMA_IRQ_TX_CH);
    spi_rx_enable(SPI_DEMO_INST);
    spi_tx_enable(SPI_DEMO_INST);
    spi_assert_cs(SPI_DEMO_INST);

    uint64_t t0 = system_get_tick();
    while (!(s_dma_tx_done && s_dma_rx_done) && !(s_dma_tx_error || s_dma_rx_error)) {
        __WFI();
        if (system_elapsed(t0, 1000U)) {
            printf("  错误：等待 DMA 中断超时\n");
            break;
        }
    }
    spi_dma_finish();

    printf("  TX DMA 完成: %s\n", s_dma_tx_done ? "是" : "否");
    printf("  RX DMA 完成: %s\n", s_dma_rx_done ? "是" : "否");
    printf("  TX DMA 错误: %s\n", s_dma_tx_error ? "是" : "否");
    printf("  RX DMA 错误: %s\n", s_dma_rx_error ? "是" : "否");
    if (s_dma_tx_done && s_dma_rx_done && !s_dma_tx_error && !s_dma_rx_error) {
        print_hex("  接收", rx_buf, sizeof(rx_buf));
        printf("  回环校验: %s\n",
               (memcmp(tx_buf, rx_buf, sizeof(tx_buf)) == 0) ? "成功" : "失败");
    }

done_irq:
    dma_unregister_callback(DMA_1, SPI_DMA_IRQ_TX_CH);
    dma_unregister_callback(DMA_1, SPI_DMA_IRQ_RX_CH);
    dma_reset_channel(DMA_1, SPI_DMA_IRQ_TX_CH);
    dma_reset_channel(DMA_1, SPI_DMA_IRQ_RX_CH);
    printf("\n完成！\n");
    print_separator();
}

/**
 * @brief 场景9：单线归零码（SID）LED 控制
 *
 * 演示使用 SPI 的 SID 模式控制 WS2812 LED 灯带。
 * 硬件连接：PA7(MOSI) -> LED 灯带 DIN
 */
void scenario_9_sid_led_control(void)
{
    /* 定义几个常用颜色（GRB 格式，WS2812 使用 GRB 顺序） */
    static const uint32_t colors[] = {
        0xFF0000U,  /* 红色 */
        0x00FF00U,  /* 绿色 */
        0x0000FFU,  /* 蓝色 */
        0xFFFFFFU,  /* 白色 */
        0x000000U,  /* 熄灭 */
    };
    static const size_t led_count = 8U;  /* 控制 8 个 LED */
    bool ok;

    print_separator();
    printf("=== 场景9：单线归零码（SID）LED 控制 ===\n\n");

    printf("步骤1：初始化系统、GPIO 和 SPI1 主模式（3.2MHz，模式0）\n");
    system_init();
    spi1_gpio_init();
    /* WS2812 要求 SCK = 3.2MHz */
    ok = spi_init_master(SPI_DEMO_INST, 3200000U, SPI_WORK_MODE_0);
    printf("  init_master: %s\n", ok ? "成功" : "失败");

    printf("\n步骤2：配置 SID 模式参数\n");
    /* 设置复位码计数值：280µs * 64MHz = 17920 */
    spi_sid_set_reset_val(SPI_DEMO_INST, 17920U);
    /* 使能 SID 模式 */
    spi_sid_enable(SPI_DEMO_INST, true);
    printf("  SID 模式已使能\n");

    printf("\n步骤3：发送 RGB 数据（红色流水灯）\n");
    for (size_t i = 0; i < led_count; i++) {
        printf("  LED[%u] = 红色\n", (uint32_t)i);
        for (size_t j = 0; j < led_count; j++) {
            if (j == i) {
                spi_sid_write_rgb(SPI_DEMO_INST, colors[0]);  /* 红色 */
            } else {
                spi_sid_write_rgb(SPI_DEMO_INST, colors[4]);  /* 熄灭 */
            }
        }
        /* 等待批量传输完成 */
        spi_wait_idle(SPI_DEMO_INST, 100U);
        delay_ms(200U);
    }

    printf("\n步骤4：发送彩虹渐变效果\n");
    for (size_t i = 0; i < led_count; i++) {
        spi_sid_write_rgb(SPI_DEMO_INST, colors[i % 3]);  /* 红绿蓝循环 */
    }
    spi_wait_idle(SPI_DEMO_INST, 100U);
    printf("  彩虹效果已发送\n");

    printf("\n步骤5：熄灭所有 LED\n");
    for (size_t i = 0; i < led_count; i++) {
        spi_sid_write_rgb(SPI_DEMO_INST, colors[4]);  /* 熄灭 */
    }
    spi_wait_idle(SPI_DEMO_INST, 100U);
    printf("  所有 LED 已熄灭\n");

    /* 禁用 SID 模式 */
    spi_sid_enable(SPI_DEMO_INST, false);

    printf("\n完成！\n");
    print_separator();
}

int main(void)
{
    system_init();
    SEGGER_RTT_Init();

    printf("\n\n");
    print_separator();
    printf("  ACM32P4 SPI 示例程序\n");
    printf("  当前场景：%d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_master_transmit();
            break;
        case 2:
            scenario_2_master_receive();
            break;
        case 3:
            scenario_3_master_transceive();
            break;
        case 4:
            scenario_4_struct_configure();
            break;
        case 5:
            scenario_5_interrupt_demo();
            break;
        case 6:
            scenario_6_memory_config();
            break;
        case 7:
            scenario_7_dma_polling_loopback();
            break;
        case 8:
            scenario_8_dma_interrupt_loopback();
            break;
        case 9:
            scenario_9_sid_led_control();
            break;
        default:
            print_separator();
            printf("错误：无效的场景编号 %d（有效范围：1-9）\n", DEFAULT_SCENARIO);
            print_separator();
            break;
    }

    while (1) {
        delay_ms(1000U);
    }
}