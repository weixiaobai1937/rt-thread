/**
 * @file i2c_example.c
 * @brief ACM32P4 I2C 驱动功能示例程序
 *
 * 演示16个场景：
 *  1.  主机轮询发送（向EEPROM写字节）
 *  2.  主机轮询接收（从EEPROM读字节）
 *  3.  主机写后读（i2c_master_write_read 读寄存器）
 *  4.  从机中断接收
 *  5.  从机中断发送
 *  6.  结构体完整主模式配置
 *  7.  结构体完整从模式配置（3个地址）
 *  8.  SCL/SDA 滤波配置
 *  9.  SDA 跳变检测
 * 10.  从机拉时钟修正
 * 11.  多主机仲裁配置
 * 12.  主机中断驱动发送（非阻塞）
 * 13.  DMA 主机发送（使能DMA_EN，配合外部DMAC）
 * 14.  DMA 主机接收（使能DMA_EN，配合外部DMAC）
 * 15.  SMBus 从机超时检测
 * 16.  SMBus 主机 tTIMEOUT / tLOW:MEXT 检测
 *
 * @note 硬件连接：
 *   - I2C1（主机）SCL = PB6（AF4，开漏），SDA = PB7（AF4，开漏）
 *   - I2C2（从机）SCL = PB10（AF4，开漏），SDA = PB11（AF4，开漏）
 *   - 外部上拉电阻 4.7KΩ 接 3.3V（或使能内部上拉）
 *   - 从机 EEPROM 地址: 0x50（场景1~3），从机地址1: 0x55（场景4/5/7）
 *   - 如需修改引脚，调整文件顶部的 I2C1_SCL_PIN / I2C2_SCL_PIN 等宏即可
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "acm32p4.h"
#include "hardware/dma.h"
#include "SEGGER_RTT.h"
#include <string.h>
#include <stddef.h>

//===========================================
// 配置选项
//===========================================

#define printf(...)         SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO    1     ///< 修改此值（1-16）选择要运行的场景

/** 总线使用的 I2C 主机实例 */
#define MASTER_INST         I2C_1
/** 从机实例（场景4/5/7/13/14/15中担任从机角色） */
#define SLAVE_INST          I2C_2

/** 虚拟 EEPROM / 目标从机 7 位地址 */
#define EEPROM_ADDR         0x50U
/** 从机自身地址1 */
#define SLAVE_ADDR1         0x55U
/** 从机自身地址2 */
#define SLAVE_ADDR2         0x56U
/** 从机自身地址3 */
#define SLAVE_ADDR3         0x57U

/** SCL 频率（标准模式 100 KHz） */
#define SCL_HZ_STANDARD     100000U
/** SCL 频率（快速模式 400 KHz） */
#define SCL_HZ_FAST         400000U

/**
 * @brief I2C 引脚及复用功能编号
 *
 * I2C1（MASTER_INST）：PB6 = SCL，PB7 = SDA，AF7（开漏 + 内部上拉）
 * I2C2（SLAVE_INST） ：PB10 = SCL，PB11 = SDA，AF7（开漏 + 内部上拉）
 *
 * 若硬件连接与此不同，修改以下6个宏即可，无需改动场景函数。
 *
 * @note 引脚AF编号来源：gpio_example.c 场景4。
 *       I2C 总线需要外部上拉电阻（4.7KΩ 接 3.3V），
 *       或将 GPIO_PULL_UP 替换为 GPIO_PULL_NONE 并使用外部上拉。
 */
#define I2C1_SCL_PIN        PB6
#define I2C1_SDA_PIN        PB7
#define I2C1_GPIO_AF        GPIO_AF_7

#define I2C2_SCL_PIN        PB10
#define I2C2_SDA_PIN        PB11
#define I2C2_GPIO_AF        GPIO_AF_7

/** 轮询超时（毫秒） */
#define TIMEOUT_MS          1000U

/** I2C1 DMA 请求 ID（参见 dma.c 请求映射表） */
#define I2C1_TX_DMA_REQ_ID  9U   ///< DMA1/DMA2 REQ9 = I2C1_TX
#define I2C1_RX_DMA_REQ_ID  10U  ///< DMA1/DMA2 REQ10 = I2C1_RX

/** 场景13/14 使用的 DMA 通道 */
#define DMA_TX_CH           DMA_CHANNEL_0
#define DMA_RX_CH           DMA_CHANNEL_1

//===========================================
// 全局变量
//===========================================

/** 场景4/5/12：中断收发缓冲区 */
static volatile uint8_t  s_slave_rx_buf[32];
static volatile uint8_t  s_slave_tx_buf[32];
static volatile uint32_t s_slave_rx_len;
static volatile uint32_t s_slave_tx_idx;
static volatile bool     s_slave_rx_done;
static volatile bool     s_slave_tx_done;

/** 场景12：主机中断发送状态 */
static const uint8_t    *s_master_tx_ptr;
static volatile size_t   s_master_tx_remain;
static volatile bool     s_master_tx_done;
static uint8_t           s_master_tx_addr;

/** 场景15/16：SMBus 超时标志 */
static volatile bool s_smbus_timeout_a;
static volatile bool s_smbus_timeout_b;


//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief 打印字节数组（十六进制）
 */
static void print_hex(const char *label, const uint8_t *data, size_t len)
{
    printf("%s[%u]: ", label, (uint32_t)len);
    for (size_t i = 0; i < len; i++) {
        printf("%02X ", data[i]);
    }
    printf("\n");
}

/**
 * @brief 初始化 I2C 引脚为复用开漏 + 内部上拉模式
 *
 * 必须在 clock_periph_enable() 和 i2c_init_xxx() 之前调用。
 *
 * 引脚对应关系（可通过文件顶部宏修改）：
 *   I2C_1（MASTER_INST）→ PB6(SCL) / PB7(SDA)，AF4
 *   I2C_2（SLAVE_INST） → PB10(SCL) / PB11(SDA)，AF4
 *
 * @param inst  需要配置的 I2C 实例
 */
static void gpio_init_i2c(i2c_instance_t inst)
{
    gpio_pin_t scl, sda;
    gpio_af_t  af;

    if (inst == (i2c_instance_t)MASTER_INST) {
        scl = I2C1_SCL_PIN;
        sda = I2C1_SDA_PIN;
        af  = I2C1_GPIO_AF;
    } else {
        scl = I2C2_SCL_PIN;
        sda = I2C2_SDA_PIN;
        af  = I2C2_GPIO_AF;
    }
    gpio_init(scl);
    gpio_init(sda);

    /* SCL：复用功能 + 开漏输出 + 内部上拉 */
    gpio_set_function(scl, af);
    gpio_set_output_type(scl, GPIO_OTYPE_OD);
    gpio_pull_up(scl);

    /* SDA：复用功能 + 开漏输出 + 内部上拉 */
    gpio_set_function(sda, af);
    gpio_set_output_type(sda, GPIO_OTYPE_OD);
    gpio_pull_up(sda);
}

//===========================================
// 示例场景
//===========================================

/* ─────────────────────────────────────────────────────────────────────────
 * 场景1：主机轮询发送
 * 演示：i2c_init_master() + i2c_master_transmit()
 * 模拟向 EEPROM 地址 0x50 写入 4 字节数据（含1字节内部地址）
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_1_master_polling_transmit(void)
{
    print_separator();
    printf("=== 场景1：主机轮询发送 ===\n\n");

    printf("步骤1：初始化 GPIO，使能时钟，初始化 I2C1 主模式（100 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    bool ok = i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_STANDARD);
    printf("  init_master: %s（PCLK1=%u Hz）\n",
           ok ? "成功" : "失败", clock_get_pclk1_hz());

    /* 发送格式：[内部地址(1B)] + [数据(4B)] */
    uint8_t payload[] = { 0x00U, 0x11U, 0x22U, 0x33U, 0x44U };
    printf("\n步骤2：向从机 0x%02X 发送 %u 字节\n", EEPROM_ADDR, (uint32_t)sizeof(payload));
    print_hex("  发送", payload, sizeof(payload));

    ok = i2c_master_transmit(MASTER_INST, EEPROM_ADDR, payload, sizeof(payload), TIMEOUT_MS);
    printf("  结果: %s\n\n", ok ? "传输成功" : "传输失败（总线无应答或超时）");

    printf("完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景2：主机轮询接收
 * 演示：i2c_master_receive() 连续读取数据
 * 模拟从 EEPROM 地址 0x50 读取 8 字节
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_2_master_polling_receive(void)
{
    print_separator();
    printf("=== 场景2：主机轮询接收 ===\n\n");

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（400 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_FAST);
    printf("  PCLK1=%u Hz，SCL=400 KHz\n", clock_get_pclk1_hz());

    uint8_t rx_buf[8] = { 0 };
    printf("\n步骤2：从从机 0x%02X 接收 %u 字节\n", EEPROM_ADDR, (uint32_t)sizeof(rx_buf));

    bool ok = i2c_master_receive(MASTER_INST, EEPROM_ADDR, rx_buf, sizeof(rx_buf), TIMEOUT_MS);
    printf("  结果: %s\n", ok ? "接收成功" : "接收失败（总线无应答或超时）");
    if (ok) {
        print_hex("  接收", rx_buf, sizeof(rx_buf));
    }

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景3：主机写后读（Repeated START）
 * 演示：i2c_master_write_read() 读寄存器
 * 先写 2 字节内部寄存器地址，再重复START读 4 字节
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_3_master_write_read(void)
{
    print_separator();
    printf("=== 场景3：主机写后读（Repeated START） ===\n\n");

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（400 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_FAST);

    /* 写阶段：发送 2 字节寄存器地址 0x10 0x00 */
    uint8_t reg_addr[2] = { 0x11U, 0x22U };
    uint8_t rx_buf[4]   = { 0 };

    printf("步骤2：write_read —— 写 %u 字节寄存器地址，读 %u 字节数据\n",
           (uint32_t)sizeof(reg_addr), (uint32_t)sizeof(rx_buf));
    print_hex("  写", reg_addr, sizeof(reg_addr));

    bool ok = i2c_master_write_read(MASTER_INST, EEPROM_ADDR,
                                    reg_addr, sizeof(reg_addr),
                                    rx_buf,   sizeof(rx_buf),
                                    TIMEOUT_MS);
    printf("  结果: %s\n", ok ? "成功" : "失败");
    if (ok) {
        print_hex("  读", rx_buf, sizeof(rx_buf));
    }

    printf("\n步骤3：等待总线空闲后再次验证状态\n");
    ok = i2c_wait_bus_free(MASTER_INST, TIMEOUT_MS);
    printf("  总线状态: %s\n", ok ? "空闲" : "仍然繁忙");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景4：从机中断接收
 * 演示：i2c_init_slave() + i2c_irq_register() + I2C_IRQ_RXNE + I2C_IRQ_STOPF
 * I2C2 配置为从机地址 0x55，等待主机写入数据
 * ───────────────────────────────────────────────────────────────────────── */

static void on_slave_rx_event(const i2c_irq_context_t *ctx)
{
    if (ctx->flags & (uint32_t)I2C_FLAG_RX_ADDR1) {
        /* 地址匹配：清标志，重置接收索引 */
        i2c_flag_clear(ctx->inst, I2C_FLAG_RX_ADDR1);
        s_slave_rx_len = 0U;
    }

    if (ctx->flags & (uint32_t)I2C_FLAG_RXNE) {
        /* 接收新字节（读DR自动清 RXNE） */
        if (s_slave_rx_len < sizeof(s_slave_rx_buf)) {
            s_slave_rx_buf[s_slave_rx_len++] = i2c_read_dr(ctx->inst);
        } else {
            (void)i2c_read_dr(ctx->inst);  /* 缓冲满时丢弃 */
        }
    }

    if (ctx->flags & (uint32_t)I2C_FLAG_STOPF) {
        /* STOP 条件：接收完成 */
        i2c_flag_clear(ctx->inst, I2C_FLAG_STOPF);
        s_slave_rx_done = true;
    }
}

void scenario_4_slave_interrupt_receive(void)
{
    print_separator();
    printf("=== 场景4：从机中断接收 ===\n\n");

    s_slave_rx_len  = 0U;
    s_slave_rx_done = false;

    printf("步骤1：初始化 GPIO，初始化 I2C2 从模式（地址=0x%02X）\n", SLAVE_ADDR1);
    gpio_init_i2c(SLAVE_INST);
    clock_periph_enable(CLK_I2C2);
    i2c_init_slave(SLAVE_INST, SLAVE_ADDR1);

    printf("步骤2：注册回调，使能 RXNE/STOPF/RX_ADDR1 中断\n");
    i2c_irq_register(SLAVE_INST, on_slave_rx_event);
    i2c_irq_enable(SLAVE_INST,
                   (i2c_irq_type_t)(I2C_IRQ_RXNE | I2C_IRQ_STOPF | I2C_IRQ_RX_ADDR1));
    NVIC_SetPriority(I2C2_IRQn, 2U);
    NVIC_EnableIRQ(I2C2_IRQn);
    printf("  NVIC 已使能 I2C2_IRQn（优先级2）\n");

    printf("\n步骤3：等待主机发送数据（最多 500 ms）\n");
    uint64_t t0 = system_get_tick();
    while (!s_slave_rx_done && !system_elapsed(t0, 500U)) { __WFI(); }

    if (s_slave_rx_done) {
        printf("  接收完成，共 %u 字节\n", s_slave_rx_len);
        print_hex("  数据", (const uint8_t *)s_slave_rx_buf, s_slave_rx_len);
    } else {
        printf("  等待超时（未收到主机数据）\n");
    }

    printf("\n步骤4：关闭中断，注销回调\n");
    NVIC_DisableIRQ(I2C2_IRQn);
    i2c_irq_disable(SLAVE_INST, I2C_IRQ_ALL);
    i2c_irq_unregister(SLAVE_INST);

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景5：从机中断发送
 * 演示：I2C_IRQ_TXE + I2C_IRQ_MTF（发送完成）
 * I2C2 从机准备一段数据，等待主机读取
 * ───────────────────────────────────────────────────────────────────────── */

static void on_slave_tx_event(const i2c_irq_context_t *ctx)
{
    if (ctx->flags & (uint32_t)I2C_FLAG_RX_ADDR1) {
        /* 地址匹配（读方向）：重置发送索引 */
        i2c_flag_clear(ctx->inst, I2C_FLAG_RX_ADDR1);
        s_slave_tx_idx = 0U;
    }

    if (ctx->flags & (uint32_t)I2C_FLAG_TXE) {
        /* 发送寄存器空：填入下一字节（写DR清 TXE） */
        if (s_slave_tx_idx < sizeof(s_slave_tx_buf)) {
            i2c_write_dr(ctx->inst, s_slave_tx_buf[s_slave_tx_idx++]);
        } else {
            i2c_write_dr(ctx->inst, 0xFFU);  /* 缓冲发完后填 0xFF */
        }
    }

    if (ctx->flags & (uint32_t)I2C_FLAG_MTF) {
        /* 字节发送完成 */
        i2c_flag_clear(ctx->inst, I2C_FLAG_MTF);
    }

    if (ctx->flags & (uint32_t)I2C_FLAG_NACKF) {
        /* 主机最后一字节发 NACK（主机接收结束信号） */
        i2c_flag_clear(ctx->inst, I2C_FLAG_NACKF);
        s_slave_tx_done = true;
    }
}

void scenario_5_slave_interrupt_transmit(void)
{
    print_separator();
    printf("=== 场景5：从机中断发送 ===\n\n");

    /* 预置发送缓冲 */
    for (uint8_t i = 0U; i < (uint8_t)sizeof(s_slave_tx_buf); i++) {
        s_slave_tx_buf[i] = (uint8_t)(0xA0U + i);
    }
    s_slave_tx_idx  = 0U;
    s_slave_tx_done = false;

    printf("步骤1：初始化 GPIO，初始化 I2C2 从模式（地址=0x%02X），准备发送 %u 字节\n",
           SLAVE_ADDR1, (uint32_t)sizeof(s_slave_tx_buf));
    print_hex("  发送内容", (const uint8_t *)s_slave_tx_buf, sizeof(s_slave_tx_buf));

    gpio_init_i2c(SLAVE_INST);
    clock_periph_enable(CLK_I2C2);
    i2c_init_slave(SLAVE_INST, SLAVE_ADDR1);

    printf("\n步骤2：注册回调，使能 TXE/MTF/NACKF/RX_ADDR1 中断\n");
    i2c_irq_register(SLAVE_INST, on_slave_tx_event);
    i2c_irq_enable(SLAVE_INST,
                   (i2c_irq_type_t)(I2C_IRQ_TXE | I2C_IRQ_MTF |
                                    I2C_IRQ_NACKF | I2C_IRQ_RX_ADDR1));
    NVIC_SetPriority(I2C2_IRQn, 2U);
    NVIC_EnableIRQ(I2C2_IRQn);

    printf("\n步骤3：等待主机读取（最多 500 ms）\n");
    uint64_t t0 = system_get_tick();
    while (!s_slave_tx_done && !system_elapsed(t0, 500U)) { __WFI(); }

    printf("  结果: %s（已发 %u 字节）\n",
           s_slave_tx_done ? "发送完成" : "等待超时",
           s_slave_tx_idx);

    printf("\n步骤4：关闭中断，注销回调\n");
    NVIC_DisableIRQ(I2C2_IRQn);
    i2c_irq_disable(SLAVE_INST, I2C_IRQ_ALL);
    i2c_irq_unregister(SLAVE_INST);

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景6：结构体完整主模式配置
 * 演示：i2c_config_master() —— 精细控制所有主模式参数
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_6_master_config_struct(void)
{
    print_separator();
    printf("=== 场景6：结构体完整主模式配置 ===\n\n");

    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);

    /* 手动计算分频：Fpclk=120MHz，Fscl=400KHz → div=120000000/(4×400000)-1=74 */
    uint32_t pclk = clock_get_pclk1_hz();
    uint16_t div  = (uint16_t)(pclk / (4U * SCL_HZ_FAST) - 1U);

    printf("步骤1：填充主模式配置结构体\n");
    printf("  PCLK1=%u Hz，目标SCL=400 KHz，clk_div=%u\n", pclk, div);

    i2c_master_config_t cfg = {
        .clk_div         = div,
        .tx_auto_en      = true,   /* 方向自动切换 */
        .multi_master_en = true,   /* 支持多主机仲裁 */
        .high_wait_en    = true,   /* 高电平等待，兼容慢速从机 */
        .marlo_sel       = false,  /* 仲裁丢失后不自动切为从机 */
    };

    printf("\n步骤2：应用配置\n");
    bool ok = i2c_config_master(MASTER_INST, &cfg);
    printf("  i2c_config_master: %s\n", ok ? "成功" : "失败");

    printf("\n步骤3：读回 CR 和 CLK_DIV 验证\n");
    printf("  CLK_DIV 寄存器原始值: 0x%08X\n", i2c_get_clk_div(MASTER_INST));
    printf("  CR 寄存器原始值: 0x%08X\n", i2c_get_cr(MASTER_INST));
    printf("  I2C 使能状态: %s\n", i2c_is_enabled(MASTER_INST) ? "已使能" : "未使能");

    printf("\n步骤4：动态修改时钟分频（切换为 100 KHz）\n");
    uint16_t div_100k = (uint16_t)(pclk / (4U * SCL_HZ_STANDARD) - 1U);
    i2c_config_clock_div(MASTER_INST, div_100k);
    printf("  新 clk_div=%u（100 KHz）\n", div_100k);
    printf("  CLK_DIV 读回: 0x%08X\n", i2c_get_clk_div(MASTER_INST));

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景7：结构体完整从模式配置（3个地址）
 * 演示：i2c_config_slave() —— addr1/addr2/addr3 独立使能
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_7_slave_config_struct(void)
{
    print_separator();
    printf("=== 场景7：结构体完整从模式配置（3个地址） ===\n\n");

    gpio_init_i2c(SLAVE_INST);
    clock_periph_enable(CLK_I2C2);

    printf("步骤1：填充从模式配置结构体\n");
    printf("  addr1=0x%02X，addr2=0x%02X（使能），addr3=0x%02X（使能）\n",
           SLAVE_ADDR1, SLAVE_ADDR2, SLAVE_ADDR3);

    i2c_slave_config_t cfg = {
        .addr1      = SLAVE_ADDR1,
        .addr2      = SLAVE_ADDR2,
        .addr2_en   = true,
        .addr3      = SLAVE_ADDR3,
        .addr3_en   = true,
        .nostretch  = false,   /* 允许时钟延长（从机准备不足时拉低SCL） */
        .tx_auto_en = true,
        .txe_sel    = false,
    };

    printf("\n步骤2：应用配置\n");
    bool ok = i2c_config_slave(SLAVE_INST, &cfg);
    printf("  i2c_config_slave: %s\n", ok ? "成功" : "失败");

    printf("\n步骤3：读回从机地址验证\n");
    printf("  SLAVE_ADDR1 读回: 0x%02X（预期 0x%02X）\n",
           i2c_get_slave_addr1(SLAVE_INST), SLAVE_ADDR1);
    printf("  SLAVE_ADDR2 读回: 0x%02X（预期 0x%02X）\n",
           i2c_get_slave_addr2(SLAVE_INST), SLAVE_ADDR2);
    printf("  SLAVE_ADDR3 读回: 0x%02X（预期 0x%02X）\n",
           i2c_get_slave_addr3(SLAVE_INST), SLAVE_ADDR3);

    printf("\n步骤4：动态修改地址2，关闭地址3\n");
    i2c_config_slave_addr2(SLAVE_INST, 0x60U, true);
    i2c_config_slave_addr3(SLAVE_INST, 0x00U, false);
    printf("  addr2 修改为 0x60（使能），addr3 关闭\n");
    printf("  SLAVE_ADDR2 读回: 0x%02X\n", i2c_get_slave_addr2(SLAVE_INST));
    printf("  SLAVE_ADDR3 读回: 0x%02X\n", i2c_get_slave_addr3(SLAVE_INST));

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景8：SCL/SDA 滤波配置
 * 演示：i2c_config_filter() —— 设置毛刺滤波级数
 * 适用场景：EMC 环境或长线驱动，需要过滤 SCL/SDA 上的高频毛刺
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_8_filter_config(void)
{
    print_separator();
    printf("=== 场景8：SCL/SDA 滤波配置 ===\n\n");

    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_STANDARD);

    printf("步骤1：配置滤波（SCL_FILTER=4，SDA_IN_DELAY=4）\n");
    printf("  说明：PCLK/8为Tcntc，滤除宽度 < 4×Tcntc 的毛刺\n");
    printf("  建议：SDA输入延时与SCL滤波值保持一致，维持相位同步\n");

    i2c_filter_config_t flt = {
        .scl_filter   = 4U,  /* 滤波级数4，滤除宽度 < 4×Tcntc 的毛刺 */
        .sda_in_delay = 4U,  /* SDA输入延时与SCL滤波同步 */
    };
    i2c_config_filter(MASTER_INST, &flt);
    printf("  配置完成\n");

    printf("\n步骤2：验证滤波生效后进行一次传输\n");
    uint8_t data[] = { 0xAAU, 0x55U };
    bool ok = i2c_master_transmit(MASTER_INST, EEPROM_ADDR, data, sizeof(data), TIMEOUT_MS);
    printf("  传输结果: %s\n", ok ? "成功" : "失败（无真实从机）");

    printf("\n步骤3：关闭滤波（恢复默认）\n");
    i2c_filter_config_t flt_off = { .scl_filter = 0U, .sda_in_delay = 0U };
    i2c_config_filter(MASTER_INST, &flt_off);
    printf("  滤波已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景9：SDA 跳变检测
 * 演示：i2c_config_sda_detect() —— 设置 SCL 低电平期间 SDA 上升沿检测阈值
 * 当检测到的上升沿次数 ≥ 阈值时，产生 DETR 中断
 * ───────────────────────────────────────────────────────────────────────── */

static volatile bool s_detr_fired;

static void on_detr_event(const i2c_irq_context_t *ctx)
{
    if (ctx->flags & (uint32_t)I2C_FLAG_DETR) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_DETR);
        s_detr_fired = true;
    }
}

void scenario_9_sda_detect(void)
{
    print_separator();
    printf("=== 场景9：SDA 跳变检测 ===\n\n");

    s_detr_fired = false;

    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_STANDARD);

    printf("步骤1：设置 SDA 跳变检测阈值为 10\n");
    printf("  说明：SCL低电平期间，SDA上升沿次数 ≥ 10 时产生 DETR 中断\n");
    i2c_config_sda_detect(MASTER_INST, 10U);

    printf("\n步骤2：注册 DETR 中断回调\n");
    i2c_irq_register(MASTER_INST, on_detr_event);
    i2c_irq_enable(MASTER_INST, I2C_IRQ_DETR);
    NVIC_SetPriority(I2C1_IRQn, 2U);
    NVIC_EnableIRQ(I2C1_IRQn);

    printf("\n步骤3：进行若干次总线传输，观察是否触发 DETR（等待 200 ms）\n");
    uint64_t t0 = system_get_tick();
    uint8_t  dummy = 0x00U;
    while (!system_elapsed(t0, 200U)) {
        (void)i2c_master_transmit(MASTER_INST, EEPROM_ADDR, &dummy, 1U, 10U);
        delay_ms(5U);
    }
    printf("  DETR 触发: %s\n", s_detr_fired ? "是" : "否（正常总线无毛刺）");

    printf("\n步骤4：清除检测阈值，关闭中断\n");
    i2c_config_sda_detect(MASTER_INST, 0U);
    NVIC_DisableIRQ(I2C1_IRQn);
    i2c_irq_disable(MASTER_INST, I2C_IRQ_DETR);
    i2c_irq_unregister(MASTER_INST);
    printf("  已清除\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景10：从机拉时钟修正
 * 演示：i2c_config_stretch_fix() —— STRETCH_FIXEN + SU_HD_FIXEN + DLY_TIME
 * 从机在拉低SCL结束后，延迟释放SCL，修正 set-up/hold 时序裕量
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_10_stretch_fix(void)
{
    print_separator();
    printf("=== 场景10：从机拉时钟修正 ===\n\n");

    gpio_init_i2c(SLAVE_INST);
    clock_periph_enable(CLK_I2C2);
    i2c_init_slave(SLAVE_INST, SLAVE_ADDR1);

    printf("步骤1：使能从机拉时钟修正（DLY_TIME=16，STRETCH_FIXEN + SU_HD_FIXEN）\n");
    printf("  说明：从机拉低SCL结束后，延迟16个APB时钟周期再释放SCL\n");
    printf("  DLY_TIME=16，PCLK1=%u Hz → 延迟 ≈ %u ns\n",
           clock_get_pclk1_hz(),
           (uint32_t)(16000000000ULL / clock_get_pclk1_hz()));

    i2c_stretch_fix_config_t fix_cfg = {
        .stretch_fix_en = true,
        .dly_time       = 16U,   /* APB时钟计数，典型值8~32 */
        .su_hd_fix_en   = true,
    };
    i2c_config_stretch_fix(SLAVE_INST, &fix_cfg);
    printf("  配置完成\n");

    printf("\n步骤2：读回 CLK_DIV 寄存器，验证 DLY_TIME 写入\n");
    printf("  CLK_DIV 读回: 0x%08X\n", i2c_get_clk_div(SLAVE_INST));

    printf("\n步骤3：关闭修正（恢复默认）\n");
    i2c_stretch_fix_config_t fix_off = { .stretch_fix_en = false, .dly_time = 0U, .su_hd_fix_en = false };
    i2c_config_stretch_fix(SLAVE_INST, &fix_off);
    printf("  已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景11：多主机仲裁配置
 * 演示：i2c_config_multi_master() + i2c_config_marlo_sel() + I2C_IRQ_MARLO
 * 模拟两个主机共享同一I2C总线，配置仲裁丢失中断
 * ───────────────────────────────────────────────────────────────────────── */

static volatile bool s_marlo_detected;

static void on_marlo_event(const i2c_irq_context_t *ctx)
{
    if (ctx->flags & (uint32_t)I2C_FLAG_MARLO) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_MARLO);
        s_marlo_detected = true;
        printf("  [中断] 仲裁丢失！inst=%d，SR=0x%08X\n",
               (int)ctx->inst, ctx->flags);
    }
}

void scenario_11_multi_master_arbitration(void)
{
    print_separator();
    printf("=== 场景11：多主机仲裁配置 ===\n\n");

    s_marlo_detected = false;

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式，使能多主机仲裁\n");
    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_STANDARD);
    i2c_config_multi_master(MASTER_INST, true);
    printf("  多主机仲裁已使能\n");

    printf("\n步骤2：配置仲裁丢失后自动切换为从机模式（MARLO_SEL=false: 不自动切）\n");
    i2c_config_marlo_sel(MASTER_INST, false);
    printf("  MARLO_SEL=false：仲裁丢失后保持主机模式，由软件处理重发\n");

    printf("\n步骤3：注册仲裁丢失中断回调\n");
    i2c_irq_register(MASTER_INST, on_marlo_event);
    i2c_irq_enable(MASTER_INST, I2C_IRQ_MARLO);
    NVIC_SetPriority(I2C1_IRQn, 2U);
    NVIC_EnableIRQ(I2C1_IRQn);

    printf("\n步骤4：发送数据（若总线上有第二主机同时发送则触发仲裁）\n");
    uint8_t data[] = { 0x11U, 0x22U, 0x33U };
    bool ok = i2c_master_transmit(MASTER_INST, EEPROM_ADDR, data, sizeof(data), TIMEOUT_MS);
    printf("  传输结果: %s\n", ok ? "成功（本机仲裁胜出）" : "失败（可能发生仲裁丢失）");
    printf("  仲裁丢失检测: %s\n", s_marlo_detected ? "已触发" : "未触发");

    printf("\n步骤5：关闭中断，注销回调\n");
    NVIC_DisableIRQ(I2C1_IRQn);
    i2c_irq_disable(MASTER_INST, I2C_IRQ_MARLO);
    i2c_irq_unregister(MASTER_INST);
    printf("  已清理\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景12：主机中断驱动发送（非阻塞）
 * 演示：I2C_IRQ_TXE + I2C_IRQ_MTF 实现异步发送，发送完成通过标志通知
 * 对比轮询方式，CPU 在等待总线期间可执行其他任务
 * ───────────────────────────────────────────────────────────────────────── */

static uint8_t s_irq_tx_buf[] = {
    0x00U, 0xC0U, 0xFFU, 0xEEU, 0x01U, 0x02U, 0x03U, 0x04U,
};

/** 中断状态机状态 */
typedef enum { TX_STATE_IDLE, TX_STATE_ADDR, TX_STATE_DATA, TX_STATE_DONE } tx_state_t;
static volatile tx_state_t s_tx_state;

static void on_master_tx_irq(const i2c_irq_context_t *ctx)
{
    switch (s_tx_state) {
    case TX_STATE_ADDR:
        /* MTF：地址字节发完，检查NACK */
        if (ctx->flags & (uint32_t)I2C_FLAG_MTF) {
            i2c_flag_clear(ctx->inst, I2C_FLAG_MTF);
            if (ctx->flags & (uint32_t)I2C_FLAG_NACKF) {
                i2c_flag_clear(ctx->inst, I2C_FLAG_NACKF);
                i2c_stop(ctx->inst);
                s_tx_state = TX_STATE_DONE;
                s_master_tx_done = true;
            } else {
                s_tx_state = TX_STATE_DATA;
            }
        }
        break;

    case TX_STATE_DATA:
        if (ctx->flags & (uint32_t)I2C_FLAG_MTF) {
            i2c_flag_clear(ctx->inst, I2C_FLAG_MTF);
            if ((ctx->flags & (uint32_t)I2C_FLAG_NACKF) || s_master_tx_remain == 0U) {
                /* NACK 或全部发完 */
                i2c_flag_clear(ctx->inst, I2C_FLAG_NACKF);
                i2c_stop(ctx->inst);
                i2c_irq_disable(ctx->inst, (i2c_irq_type_t)(I2C_IRQ_MTF | I2C_IRQ_TXE));
                s_tx_state = TX_STATE_DONE;
                s_master_tx_done = true;
            }
        }
        if ((s_tx_state == TX_STATE_DATA) && (ctx->flags & (uint32_t)I2C_FLAG_TXE)) {
            if (s_master_tx_remain > 0U) {
                i2c_write_dr(ctx->inst, *s_master_tx_ptr++);
                s_master_tx_remain--;
            }
        }
        break;

    default:
        break;
    }
}

void scenario_12_master_irq_transmit(void)
{
    print_separator();
    printf("=== 场景12：主机中断驱动发送（非阻塞） ===\n\n");

    s_master_tx_done   = false;
    s_master_tx_ptr    = s_irq_tx_buf;
    s_master_tx_remain = sizeof(s_irq_tx_buf);
    s_master_tx_addr   = EEPROM_ADDR;
    s_tx_state         = TX_STATE_ADDR;

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（400 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_FAST);

    printf("\n步骤2：注册回调，使能 MTF/TXE 中断\n");
    i2c_irq_register(MASTER_INST, on_master_tx_irq);
    i2c_irq_enable(MASTER_INST, (i2c_irq_type_t)(I2C_IRQ_MTF | I2C_IRQ_TXE));
    NVIC_SetPriority(I2C1_IRQn, 2U);
    NVIC_EnableIRQ(I2C1_IRQn);

    printf("\n步骤3：手动产生 START，写地址字节（中断接管后续流程）\n");
    print_hex("  发送内容", s_irq_tx_buf, sizeof(s_irq_tx_buf));
    /* 写DR触发总线时序：先由中断处理函数在MTF时填充数据字节 */
    i2c_start(MASTER_INST);
    i2c_write_dr(MASTER_INST, (uint8_t)((s_master_tx_addr << 1) | 0U));

    printf("\n步骤4：等待中断完成（最多 500 ms），期间 CPU 可做其他事\n");
    uint64_t t0 = system_get_tick();
    while (!s_master_tx_done && !system_elapsed(t0, 500U)) {
        /* 模拟 CPU 执行其他任务 */
        __NOP();
    }
    printf("  结果: %s（耗时 %u ms）\n",
           s_master_tx_done ? "发送完成" : "超时",
           (uint32_t)(system_get_tick() - t0));

    printf("\n步骤5：关闭中断，注销回调\n");
    NVIC_DisableIRQ(I2C1_IRQn);
    i2c_irq_disable(MASTER_INST, I2C_IRQ_ALL);
    i2c_irq_unregister(MASTER_INST);

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景13：DMA 主机发送
 * 流程：
 *  1. dma_init_mem2periph 配置 DMA1_CH0（内存→I2C1_DR，M2P，8位）
 *  2. 注册 TC 回调，使能 DMA1_CH0 NVIC 中断
 *  3. I2C DMA_EN=1 → 产生 START + 地址字节
 *  4. dma_start_channel 启动 DMA，进入 WFI 等待 TC 中断
 *  5. TC 回调：清标志 → 禁 DMA_EN → 发 STOP → 置完成标志
 * ───────────────────────────────────────────────────────────────────────── */

static volatile bool s_dma_tx_done;

static void on_dma_tx_complete(dma_instance_t inst, dma_channel_t ch)
{
    dma_clear_tc_flag(inst, ch);
    i2c_config_dma(MASTER_INST, false);
    i2c_stop(MASTER_INST);
    s_dma_tx_done = true;
}

void scenario_13_dma_master_transmit(void)
{
    print_separator();
    printf("=== 场景13：DMA 主机发送 ===\n\n");

    static const uint8_t dma_tx_data[] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U,
        0x08U, 0x09U, 0x0AU, 0x0BU, 0x0CU, 0x0DU, 0x0EU, 0x0FU,
    };

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（400 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_FAST);

    printf("\n步骤2：配置 DMA1_CH0（M2P：内存→I2C1_DR，8位）\n");
    dma_enable(DMA_1);
    bool ok = dma_init_mem2periph(
        DMA_1, DMA_TX_CH,
        I2C1_TX_DMA_REQ_ID,
        (uint32_t)dma_tx_data,
        (uint32_t)&I2C1->DR,
        (uint16_t)sizeof(dma_tx_data),
        DMA_WIDTH_BYTE
    );
    if (!ok) {
        printf("  错误：DMA 配置失败\n");
        print_separator();
        return;
    }
    dma_config_burst(DMA_1, DMA_TX_CH, DMA_BURST_1, DMA_BURST_1);
    s_dma_tx_done = false;
    dma_register_tc_callback(DMA_1, DMA_TX_CH, on_dma_tx_complete);
    NVIC_EnableIRQ(DMA1_CH0_IRQn);
    printf("  DMA1_CH0 配置完成（突发1，每TXE搬运1字节），TC 回调已注册\n");
    print_hex("  待发送数据", dma_tx_data, sizeof(dma_tx_data));

    printf("\n步骤3：使能 DMA_EN 并启动 DMA，再产生 START + 地址字节\n");
    i2c_config_txe_sel(MASTER_INST, true);
    i2c_config_dma(MASTER_INST, true);
    dma_start_channel(DMA_1, DMA_TX_CH);

    (void)i2c_wait_bus_free(MASTER_INST, TIMEOUT_MS);
    i2c_start(MASTER_INST);
    i2c_write_dr(MASTER_INST, (uint8_t)((EEPROM_ADDR << 1) | 0U));

    uint64_t tw = system_get_tick();
    while (!i2c_flag_is_set(MASTER_INST, I2C_FLAG_MTF)) {
        if (system_elapsed(tw, 50U)) {
            printf("  超时等待 MTF\n");
            dma_stop_channel(DMA_1, DMA_TX_CH);
            i2c_config_dma(MASTER_INST, false);
            i2c_stop(MASTER_INST);
            dma_reset_channel(DMA_1, DMA_TX_CH);
            print_separator();
            return;
        }
    }
    if (i2c_flag_is_set(MASTER_INST, I2C_FLAG_RACK)) {
        printf("  从机 NACK，地址无应答\n");
        i2c_flag_clear(MASTER_INST, I2C_FLAG_MTF);
        dma_stop_channel(DMA_1, DMA_TX_CH);
        i2c_config_dma(MASTER_INST, false);
        i2c_stop(MASTER_INST);
        dma_reset_channel(DMA_1, DMA_TX_CH);
        print_separator();
        return;
    }
    i2c_flag_clear(MASTER_INST, I2C_FLAG_MTF);
    printf("  START + 地址 0x%02X 已 ACK，DMA 开始搬运数据\n", (EEPROM_ADDR << 1) | 0U);

    printf("\n步骤4：等待 TC 中断（最多 500 ms）\n");
    uint64_t t0 = system_get_tick();
    while (!s_dma_tx_done) {
        __WFI();
        if (system_elapsed(t0, 500U)) {
            break;
        }
    }

    if (!s_dma_tx_done) {
        dma_stop_channel(DMA_1, DMA_TX_CH);
        i2c_config_dma(MASTER_INST, false);
        i2c_stop(MASTER_INST);
        printf("  超时！（确认 I2C1_TX_DMA_REQ_ID 与 NVIC 配置正确）\n");
    } else {
        printf("  DMA 发送完成！\n");
    }

    dma_reset_channel(DMA_1, DMA_TX_CH);
    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景14：DMA 主机接收
 * 流程：
 *  1. dma_init_periph2mem 配置 DMA1_CH1（I2C1_DR→内存，P2M，8位）
 *  2. 注册 TC 回调，使能 DMA1_CH1 NVIC 中断
 *  3. I2C DMA_EN=1 → 产生 START + 读地址 → 等待并清 TX_RX_FLAG
 *  4. dma_start_channel 启动 DMA，进入 WFI 等待 TC 中断
 *  5. TC 回调：清标志 → NACK+STOP → 禁 DMA_EN → 置完成标志
 * ───────────────────────────────────────────────────────────────────────── */

static volatile bool s_dma_rx_done;
static uint8_t       s_dma_rx_buf[16];

static void on_dma_rx_complete(dma_instance_t inst, dma_channel_t ch)
{
    dma_clear_tc_flag(inst, ch);
    i2c_set_tack(MASTER_INST, I2C_NACK);
    i2c_stop(MASTER_INST);
    i2c_config_dma(MASTER_INST, false);
    i2c_flag_clear(MASTER_INST, I2C_FLAG_TX_RX_FLAG);
    s_dma_rx_done = true;
}

void scenario_14_dma_master_receive(void)
{
    print_separator();
    printf("=== 场景14：DMA 主机接收 ===\n\n");

    memset(s_dma_rx_buf, 0, sizeof(s_dma_rx_buf));

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（400 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_FAST);

    printf("\n步骤2：配置 DMA1_CH1（P2M：I2C1_DR→内存，8位）\n");
    dma_enable(DMA_1);
    bool ok = dma_init_periph2mem(
        DMA_1, DMA_RX_CH,
        I2C1_RX_DMA_REQ_ID,
        (uint32_t)&I2C1->DR,
        (uint32_t)s_dma_rx_buf,
        (uint16_t)sizeof(s_dma_rx_buf),
        DMA_WIDTH_BYTE
    );
    if (!ok) {
        printf("  错误：DMA 配置失败\n");
        print_separator();
        return;
    }
    dma_config_burst(DMA_1, DMA_RX_CH, DMA_BURST_1, DMA_BURST_1);
    s_dma_rx_done = false;
    dma_register_tc_callback(DMA_1, DMA_RX_CH, on_dma_rx_complete);
    NVIC_EnableIRQ(DMA1_CH1_IRQn);
    printf("  DMA1_CH1 配置完成（突发1，每RXNE搬运1字节），TC 回调已注册\n");

    printf("\n步骤3：使能 I2C DMA 请求，等待总线空闲，产生 START + 读地址\n");
    i2c_config_dma(MASTER_INST, true);
    (void)i2c_wait_bus_free(MASTER_INST, TIMEOUT_MS);
    i2c_start(MASTER_INST);
    i2c_write_dr(MASTER_INST, (uint8_t)((EEPROM_ADDR << 1) | 1U));
    printf("  START + 地址字节（读方向）0x%02X 已写入\n", (EEPROM_ADDR << 1) | 1U);

    /* 等待并清除 TX_RX_FLAG（主机 TX→RX 切换完成标志） */
    uint64_t tw = system_get_tick();
    while (!i2c_flag_is_set(MASTER_INST, I2C_FLAG_TX_RX_FLAG)) {
        if (system_elapsed(tw, TIMEOUT_MS)) {
            printf("  超时等待 TX_RX_FLAG\n");
            i2c_config_dma(MASTER_INST, false);
            dma_reset_channel(DMA_1, DMA_RX_CH);
            print_separator();
            return;
        }
    }
    i2c_flag_clear(MASTER_INST, I2C_FLAG_TX_RX_FLAG);
    printf("  TX_RX_FLAG 已清除，RX 阶段开始\n");

    printf("\n步骤4：启动 DMA 通道，等待 TC 中断（最多 500 ms）\n");
    dma_start_channel(DMA_1, DMA_RX_CH);

    uint64_t t0 = system_get_tick();
    while (!s_dma_rx_done) {
        __WFI();
        if (system_elapsed(t0, 500U)) {
            break;
        }
    }

    if (!s_dma_rx_done) {
        dma_stop_channel(DMA_1, DMA_RX_CH);
        i2c_set_tack(MASTER_INST, I2C_NACK);
        i2c_stop(MASTER_INST);
        i2c_config_dma(MASTER_INST, false);
        printf("  超时！（确认 I2C1_RX_DMA_REQ_ID 与 NVIC 配置正确）\n");
    } else {
        print_hex("  接收数据", s_dma_rx_buf, sizeof(s_dma_rx_buf));
    }

    i2c_set_tack(MASTER_INST, I2C_ACK);
    dma_reset_channel(DMA_1, DMA_RX_CH);
    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景15：SMBus 从机超时检测
 * 演示：i2c_config_smbus_timeout() 配置 TimeoutA（tTIMEOUT）和
 *        TimeoutB（tLOW:SEXT），并通过中断检测超时事件
 *
 * 典型值：tTIMEOUT ≤ 35 ms → TIMEOUTA ≥ (35ms × PCLK) / 2048
 * ───────────────────────────────────────────────────────────────────────── */

static void on_smbus_slave_event(const i2c_irq_context_t *ctx)
{
    uint32_t flags = ctx->flags;

    if (flags & (uint32_t)I2C_FLAG_TIMEOUTAF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_TIMEOUTAF);
        s_smbus_timeout_a = true;
        printf("  [中断] TimeoutA 超时（tTIMEOUT / tLOW:SEXT）！\n");
    }

    if (flags & (uint32_t)I2C_FLAG_TIMEOUTBF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_TIMEOUTBF);
        s_smbus_timeout_b = true;
        printf("  [中断] TimeoutB 超时（tLOW:SEXT 累积）！\n");
    }

    if (flags & (uint32_t)I2C_FLAG_RX_ADDR1) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_RX_ADDR1);
    }

    if (flags & (uint32_t)I2C_FLAG_RXNE) {
        (void)i2c_read_dr(ctx->inst);
    }

    if (flags & (uint32_t)I2C_FLAG_STOPF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_STOPF);
    }

    if (flags & (uint32_t)I2C_FLAG_NACKF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_NACKF);
    }
}

void scenario_15_smbus_slave_timeout(void)
{
    print_separator();
    printf("=== 场景15：SMBus 从机超时检测 ===\n\n");

    s_smbus_timeout_a = false;
    s_smbus_timeout_b = false;

    gpio_init_i2c(SLAVE_INST);
    clock_periph_enable(CLK_I2C2);
    i2c_init_slave(SLAVE_INST, SLAVE_ADDR1);

    /* 计算 TimeoutA：tTIMEOUT ≤ 35 ms
     * TIMEOUTA = ceil(35ms × PCLK / 2048) */
    uint32_t pclk    = clock_get_pclk1_hz();
    uint16_t tmoa    = (uint16_t)((uint64_t)35U * pclk / 1000U / 2048U);
    /* TimeoutB：tLOW:SEXT ≤ 25 ms（从机累积延长时间）
     * TIMEOUTB = ceil(25ms × PCLK / 2048) */
    uint16_t tmob    = (uint16_t)((uint64_t)25U * pclk / 1000U / 2048U);

    printf("步骤1：计算 SMBus 超时值\n");
    printf("  PCLK=%u Hz\n", pclk);
    printf("  TimeoutA（tTIMEOUT≤35ms）: %u\n", tmoa);
    printf("  TimeoutB（tLOW:SEXT≤25ms）: %u\n", tmob);

    printf("\n步骤2：配置 SMBus 超时（从机模式：ext_mode=false→SEXT）\n");
    i2c_smbus_timeout_config_t smb = {
        .timeouta        = tmoa,
        .timeouta_en     = true,
        .timeouta_irq_en = true,
        .timeoutb        = tmob,
        .timeoutb_en     = true,
        .timeoutb_irq_en = true,
        .ext_mode        = false,  /* 从机模式：监测 tLOW:SEXT */
    };
    bool ok = i2c_config_smbus_timeout(SLAVE_INST, &smb);
    printf("  配置结果: %s\n", ok ? "成功" : "失败");

    printf("\n步骤3：注册中断回调，使能 TIMEOUTAF/TIMEOUTBF 及从机事件中断\n");
    i2c_irq_register(SLAVE_INST, on_smbus_slave_event);
    i2c_irq_enable(SLAVE_INST,
                   (i2c_irq_type_t)(I2C_IRQ_TIMEOUTAF | I2C_IRQ_TIMEOUTBF |
                                    I2C_IRQ_RX_ADDR1   | I2C_IRQ_RXNE |
                                    I2C_IRQ_STOPF       | I2C_IRQ_NACKF));
    NVIC_SetPriority(I2C2_IRQn, 2U);
    NVIC_EnableIRQ(I2C2_IRQn);

    printf("\n步骤4：等待超时或主机通信事件（最多 1000 ms）\n");
    uint64_t t0 = system_get_tick();
    while (!s_smbus_timeout_a && !s_smbus_timeout_b && !system_elapsed(t0, 1000U)) {
        __WFI();
    }
    printf("  TimeoutA 超时: %s\n", s_smbus_timeout_a ? "已触发" : "未触发");
    printf("  TimeoutB 超时: %s\n", s_smbus_timeout_b ? "已触发" : "未触发");

    printf("\n步骤5：清除超时标志，禁用 SMBus 超时，关闭中断\n");
    i2c_smbus_timeout_clear(SLAVE_INST, true);
    i2c_smbus_timeout_clear(SLAVE_INST, false);

    i2c_smbus_timeout_config_t smb_off = { 0 };
    (void)i2c_config_smbus_timeout(SLAVE_INST, &smb_off);

    NVIC_DisableIRQ(I2C2_IRQn);
    i2c_irq_disable(SLAVE_INST, I2C_IRQ_ALL);
    i2c_irq_unregister(SLAVE_INST);
    printf("  已清理\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景16：SMBus 主机 tTIMEOUT / tLOW:MEXT 检测
 * 演示：ext_mode=true → TimeoutB 检测 tLOW:MEXT（主机累积时钟低电平延长）
 *        TimeoutA 检测 tTIMEOUT（SCL 持续低电平超过 35 ms）
 * ───────────────────────────────────────────────────────────────────────── */

static void on_smbus_master_event(const i2c_irq_context_t *ctx)
{
    uint32_t flags = ctx->flags;

    if (flags & (uint32_t)I2C_FLAG_TIMEOUTAF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_TIMEOUTAF);
        s_smbus_timeout_a = true;
        printf("  [中断] TimeoutA 超时（tTIMEOUT：SCL低电平持续超限）！\n");
        /* 超时后应产生 STOP，释放总线 */
        i2c_stop(ctx->inst);
    }

    if (flags & (uint32_t)I2C_FLAG_TIMEOUTBF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_TIMEOUTBF);
        s_smbus_timeout_b = true;
        printf("  [中断] TimeoutB 超时（tLOW:MEXT：主机累积延长超限）！\n");
        i2c_stop(ctx->inst);
    }

    if (flags & (uint32_t)I2C_FLAG_NACKF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_NACKF);
    }

    if (flags & (uint32_t)I2C_FLAG_MTF) {
        i2c_flag_clear(ctx->inst, I2C_FLAG_MTF);
    }
}

void scenario_16_smbus_master_timeout(void)
{
    print_separator();
    printf("=== 场景16：SMBus 主机 tTIMEOUT / tLOW:MEXT 检测 ===\n\n");

    s_smbus_timeout_a = false;
    s_smbus_timeout_b = false;

    printf("步骤1：初始化 GPIO，初始化 I2C1 主模式（100 KHz）\n");
    gpio_init_i2c(MASTER_INST);
    clock_periph_enable(CLK_I2C1);
    i2c_init_master(MASTER_INST, clock_get_pclk1_hz(), SCL_HZ_STANDARD);

    uint32_t pclk = clock_get_pclk1_hz();
    /* TimeoutA：tTIMEOUT ≤ 35 ms → 检测 SCL 持续低电平 */
    uint16_t tmoa = (uint16_t)((uint64_t)35U * pclk / 1000U / 2048U);
    /* TimeoutB：tLOW:MEXT ≤ 10 ms（主机累积延长，SMBus规格较小）*/
    uint16_t tmob = (uint16_t)((uint64_t)10U * pclk / 1000U / 2048U);

    printf("步骤2：配置 SMBus 超时（主机模式：ext_mode=true→MEXT）\n");
    printf("  PCLK=%u Hz，TimeoutA=%u，TimeoutB=%u\n", pclk, tmoa, tmob);

    i2c_smbus_timeout_config_t smb = {
        .timeouta        = tmoa,
        .timeouta_en     = true,
        .timeouta_irq_en = true,
        .timeoutb        = tmob,
        .timeoutb_en     = true,
        .timeoutb_irq_en = true,
        .ext_mode        = true,   /* 主机模式：监测 tLOW:MEXT */
    };
    bool ok = i2c_config_smbus_timeout(MASTER_INST, &smb);
    printf("  配置结果: %s\n", ok ? "成功" : "失败");

    printf("\n步骤3：注册中断回调，使能 TIMEOUTAF/TIMEOUTBF/MTF/NACKF\n");
    i2c_irq_register(MASTER_INST, on_smbus_master_event);
    i2c_irq_enable(MASTER_INST,
                   (i2c_irq_type_t)(I2C_IRQ_TIMEOUTAF | I2C_IRQ_TIMEOUTBF |
                                    I2C_IRQ_MTF        | I2C_IRQ_NACKF));
    NVIC_SetPriority(I2C1_IRQn, 2U);
    NVIC_EnableIRQ(I2C1_IRQn);

    printf("\n步骤4：发起一次传输（若从机拉低SCL超过阈值则触发超时中断）\n");
    uint8_t data[] = { 0x5AU, 0xA5U };
    ok = i2c_master_transmit(MASTER_INST, EEPROM_ADDR, data, sizeof(data), TIMEOUT_MS);
    printf("  传输结果: %s\n", ok ? "成功" : "失败（无从机应答或超时）");

    printf("\n步骤5：等待超时中断触发（最多 200 ms）\n");
    uint64_t t0 = system_get_tick();
    while (!s_smbus_timeout_a && !s_smbus_timeout_b && !system_elapsed(t0, 200U)) {
        __WFI();
    }
    printf("  TimeoutA 触发: %s\n", s_smbus_timeout_a ? "是" : "否");
    printf("  TimeoutB 触发: %s\n", s_smbus_timeout_b ? "是" : "否");

    printf("\n步骤6：轮询查询超时标志（另一种不使用中断的方式）\n");
    printf("  TIMEOUTAF 标志: %s\n",
           i2c_smbus_timeout_is_set(MASTER_INST, true)  ? "已置位" : "未置位");
    printf("  TIMEOUTBF 标志: %s\n",
           i2c_smbus_timeout_is_set(MASTER_INST, false) ? "已置位" : "未置位");

    printf("\n步骤7：清除标志，禁用 SMBus 超时，关闭中断\n");
    i2c_smbus_timeout_clear(MASTER_INST, true);
    i2c_smbus_timeout_clear(MASTER_INST, false);

    i2c_smbus_timeout_config_t smb_off = { 0 };
    (void)i2c_config_smbus_timeout(MASTER_INST, &smb_off);

    NVIC_DisableIRQ(I2C1_IRQn);
    i2c_irq_disable(MASTER_INST, I2C_IRQ_ALL);
    i2c_irq_unregister(MASTER_INST);
    printf("  已清理\n");

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
    printf("ACM32P4 I2C 驱动示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO)
    {
    case  1: scenario_1_master_polling_transmit();  break;
    case  2: scenario_2_master_polling_receive();   break;
    case  3: scenario_3_master_write_read();        break;
    case  4: scenario_4_slave_interrupt_receive();  break;
    case  5: scenario_5_slave_interrupt_transmit(); break;
    case  6: scenario_6_master_config_struct();     break;
    case  7: scenario_7_slave_config_struct();      break;
    case  8: scenario_8_filter_config();            break;
    case  9: scenario_9_sda_detect();               break;
    case 10: scenario_10_stretch_fix();             break;
    case 11: scenario_11_multi_master_arbitration();break;
    case 12: scenario_12_master_irq_transmit();     break;
    case 13: scenario_13_dma_master_transmit();     break;
    case 14: scenario_14_dma_master_receive();      break;
    case 15: scenario_15_smbus_slave_timeout();     break;
    case 16: scenario_16_smbus_master_timeout();    break;
    default:
        printf("错误：无效的场景编号 %d（有效范围：1-16）\n", DEFAULT_SCENARIO);
        break;
    }

    while (1) { __WFI(); }
}

