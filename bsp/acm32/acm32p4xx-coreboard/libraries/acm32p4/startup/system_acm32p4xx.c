/**
 * @file system_ACM32P4xx.c
 * @brief 系统初始化和时钟配置
 */

#include "acm32p4xx.h"

#define SYSTEM_INIT_OSPI_PSRAM  1U
//#define SYSTEM_ENABLE_DCACHE	1U

#if SYSTEM_INIT_OSPI_PSRAM
#include "hardware/clocks.h"
#endif

#define SYSTEM_CLOCK_TIMEOUT  100000U

/** OSPI1 PSRAM 硬件初始化（GPIO + OSPI 控制器 + memory-mapped） */
#ifndef SYSTEM_INIT_OSPI_PSRAM
#define SYSTEM_INIT_OSPI_PSRAM  0U
#endif

/** OSPI1 PSRAM 区域 MPU non-cacheable 配置 */
#ifndef SYSTEM_MPU_OSPI_PSRAM
#define SYSTEM_MPU_OSPI_PSRAM   0U
#endif

#if SYSTEM_INIT_OSPI_PSRAM

/* OSPI1 引脚定义 */
#define OSPI_INIT_SCK_PIN   PG5
#define OSPI_INIT_IO0_PIN   PG6
#define OSPI_INIT_IO1_PIN   PG7
#define OSPI_INIT_IO2_PIN   PG8
#define OSPI_INIT_IO3_PIN   PG9
#define OSPI_INIT_IO4_PIN   PG10
#define OSPI_INIT_IO5_PIN   PG11
#define OSPI_INIT_IO6_PIN   PG12
#define OSPI_INIT_IO7_PIN   PG13
#define OSPI_INIT_CS_PIN    PG14
#define OSPI_INIT_DQS_PIN   PG15
#define OSPI_INIT_RESET_PIN PF10

#define OSPI_INIT_AF_MAIN   GPIO_AF_2
#define OSPI_INIT_AF_IO4    GPIO_AF_6
#define OSPI_INIT_AF_IO5    GPIO_AF_5

/* APS6408L PSRAM 命令 */
#define APS6408L_SYNC_READ_LINEAR_BURST   0x20U
#define APS6408L_SYNC_WRITE_LINEAR_BURST  0xA0U
#define APS6408L_READ_REG                 0x40U
#define APS6408L_WRITE_REG                0xC0U
#define APS6408L_REG_MR0                  0x00U
#define APS6408L_REG_MR1                  0x01U
#define APS6408L_MR0_FIXED_READ_LATENCY   (1U << 5)
#define APS6408L_DEVICE_ID                0x930DU

/* PSRAM 复位时序（约 180000 次 NOP ≈ 1ms @ 180MHz） */
#define PSRAM_RESET_DELAY_CYCLES  180000U

/* FIFO 超时 */
#define OSPI_TIMEOUT_COUNT_VAL    1000000U

#endif /* SYSTEM_INIT_OSPI_PSRAM */


#if SYSTEM_MPU_OSPI_PSRAM

#undef SYSTEM_PSRAM_BASE_ADDR
#undef SYSTEM_PSRAM_LIMIT_ADDR
#define SYSTEM_MPU_REGION_PSRAM     0U         /* MPU region 编号 */
#define SYSTEM_MPU_ATTR_PSRAM       1U         /* 内存属性编号 */
#define SYSTEM_PSRAM_BASE_ADDR      0x80000000U
#define SYSTEM_PSRAM_LIMIT_ADDR     0x807FFFFFU /* 8MB */

/**
 * @brief  配置 MPU 将 OSPI1 PSRAM 区域标记为 non-cacheable
 * @note   在 DCache 使能后调用，避免 CPU 缓存 PSRAM 导致一致性问题
 * @note   Region 0，覆盖 0x80000000~0x807FFFFF（8MB）
 */
static void system_mpu_ospi_psram(void)
{
#if defined(__MPU_PRESENT) && (__MPU_PRESENT == 1U)
    ARM_MPU_Disable();

    ARM_MPU_SetMemAttr(SYSTEM_MPU_ATTR_PSRAM,
                       ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE,
                                    ARM_MPU_ATTR_NON_CACHEABLE));

    ARM_MPU_SetRegion(SYSTEM_MPU_REGION_PSRAM,
                      ARM_MPU_RBAR(SYSTEM_PSRAM_BASE_ADDR,
                                   ARM_MPU_SH_NON,
                                   ARM_MPU_AP_RW,
                                   ARM_MPU_AP_NP,
                                   ARM_MPU_XN),
                      ARM_MPU_RLAR(SYSTEM_PSRAM_LIMIT_ADDR,
                                   SYSTEM_MPU_ATTR_PSRAM));

    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
#endif
}

#endif /* SYSTEM_MPU_OSPI_PSRAM */

#if SYSTEM_INIT_OSPI_PSRAM

/**
 * @brief  NOP 循环延时（SystemInit 阶段替代 delay_ms）
 * @note   约 180000 次 NOP ≈ 1ms @ 180MHz
 */
static void psram_nop_delay(volatile uint32_t cycles)
{
    for (volatile uint32_t i = cycles; i != 0U; --i) {
        __NOP();
    }
}

/**
 * @brief  初始化 OSPI1 的 GPIO 引脚并复位 PSRAM
 * @note   直接寄存器操作，不调用 SDK API
 */
static void psram_init_gpio_and_reset(void)
{
    /* 1. 使能 GPIOF、GPIOG、OSPI1 时钟
     *   CLK_GPIOF = 0x0105: reg_idx=1 → AHB2CKENR, bit=5
     *   CLK_GPIOG = 0x0106: reg_idx=1 → AHB2CKENR, bit=6
     *   CLK_OSPI1 = 0x0208: reg_idx=2 → AHB3CKENR, bit=8 */
    RCC->AHB2CKENR |= (1UL << 5) | (1UL << 6);   /* GPIOF + GPIOG */
    RCC->AHB3CKENR |= (1UL << 8);                /* OSPI1 */

    /* OSPI1 硬件复位（写 0 复位，写 1 释放） */
    RCC->AHB3RSTR &= ~(1UL << 8);  /* 拉低复位 */
    for (volatile uint32_t i = 0; i < 100; i++) { __NOP(); }
    RCC->AHB3RSTR |= (1UL << 8);   /* 释放复位 */
    for (volatile uint32_t i = 0; i < 100; i++) { __NOP(); }

    /* 2. 配置 PG5-15 为复用功能（AF2/AF5/AF6）
     *   GPIO MD 寄存器：00=输入, 01=输出, 10=复用, 11=模拟
     *   GPIO AF0/AF1 寄存器：复用功能选择 */
    GPIOG->MD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                    (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                    (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                    (0x3UL << 28) | (0x3UL << 30));  /* PG5-15 清零 */
    GPIOG->MD |= (2UL << 10) | (2UL << 12) | (2UL << 14) |
                  (2UL << 16) | (2UL << 18) | (2UL << 20) |
                  (2UL << 22) | (2UL << 24) | (2UL << 26) |
                  (2UL << 28) | (2UL << 30);  /* PG5-15 设为复用功能 */

    /* AF0: PG5-7 → AF2 (0010)
     * AF1: PG8-15 → AF2/AF6/AF5... */
    GPIOG->AF0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));  /* PG5-7 */
    GPIOG->AF0 |= (2UL << 20) | (2UL << 24) | (2UL << 28);  /* AF2 */

    GPIOG->AF1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                     (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                     (0xFUL << 24) | (0xFUL << 28));  /* PG8-15 */
    GPIOG->AF1 |= (2UL << 0)  |  /* PG8  → AF2 */
                  (2UL << 4)  |  /* PG9  → AF2 */
                  (6UL << 8)  |  /* PG10 → AF6 */
                  (5UL << 12) |  /* PG11 → AF5 */
                  (2UL << 16) |  /* PG12 → AF2 */
                  (2UL << 20) |  /* PG13 → AF2 */
                  (2UL << 24) |  /* PG14 → AF2 */
                  (2UL << 28);   /* PG15 → AF2 */

    /* 3. 设置上拉（PUPD=01）和驱动强度（DS=4 → 8mA）
     *   PUPD: 每个引脚 2 位，01=上拉
     *   DS0/DS1: 每个引脚 4 位，4=8mA */
    GPIOG->PUPD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                      (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                      (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                      (0x3UL << 28) | (0x3UL << 30));  /* 清零 */
    GPIOG->PUPD |= (1UL << 10) | (1UL << 12) | (1UL << 14) |
                    (1UL << 16) | (1UL << 18) | (1UL << 20) |
                    (1UL << 22) | (1UL << 24) | (1UL << 26) |
                    (1UL << 28) | (1UL << 30);  /* 上拉 */

    /* 驱动强度：PG5-7 在 DS0，PG8-15 在 DS1 */
    GPIOG->DS0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));  /* PG5-7 */
    GPIOG->DS0 |= (4UL << 20) | (4UL << 24) | (4UL << 28);  /* 8mA */

    GPIOG->DS1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                     (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                     (0xFUL << 24) | (0xFUL << 28));  /* PG8-15 */
    GPIOG->DS1 |= (4UL << 0) | (4UL << 4) | (4UL << 8) |
                  (4UL << 12) | (4UL << 16) | (4UL << 20) |
                  (4UL << 24) | (4UL << 28);  /* 8mA */

    /* 4. 配置 PF10 为推挽输出（PSRAM Reset 脚）
     *   MD=01（输出）, OTYP=0（推挽）, PUPD=00（无上下拉） */
    GPIOF->MD &= ~(3UL << 20);     /* PF10 清零 */
    GPIOF->MD |= (1UL << 20);      /* PF10 输出 */
    GPIOF->OTYP &= ~(1UL << 10);   /* PF10 推挽 */
    GPIOF->PUPD &= ~(3UL << 20);   /* PF10 无上下拉 */

    /* 5. 复位 PSRAM：拉低 1ms → 拉高 1ms */
    GPIOF->ODATA &= ~(1UL << 10);  /* PF10 = 0 */
    psram_nop_delay(PSRAM_RESET_DELAY_CYCLES);
    GPIOF->ODATA |= (1UL << 10);   /* PF10 = 1 */
    psram_nop_delay(PSRAM_RESET_DELAY_CYCLES);
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */

#if SYSTEM_INIT_OSPI_PSRAM

/** DTR 命令编码：单字节命令复制为双字节 */
static uint16_t psram_dtr_cmd(uint8_t cmd)
{
    return (uint16_t)cmd | ((uint16_t)cmd << 8U);
}

/** 等待 OSPI 状态标志（轮询，无超时保护） */
static void psram_wait_status(uint32_t mask)
{
    while ((OSPI1->STATUS & mask) == 0U) { /* 等待 */ }
}

/** 写入 FIFO 半字（直接寄存器） */
static void psram_write_fifo_half(uint16_t data)
{
    while (OSPI1->STATUS & (1UL << 3)) { }  /* 等待 TX FIFO 非满 */
    OSPI1->TX_DAT = data;
}

/** 读取 FIFO 半字（直接寄存器） */
static uint16_t psram_read_fifo_half(void)
{
    while (OSPI1->STATUS & (1UL << 4)) { }  /* 等待 RX FIFO 非空 */
    return (uint16_t)(OSPI1->RX_DAT & 0xFFFFU);
}

/**
 * @brief  通过 OSPI1 FIFO DTR 读取 PSRAM 模式寄存器
 * @param[in]  reg_addr  寄存器地址（MR0=0x00 ~ MR8=0x08）
 * @param[out] value     读出的 16 位值
 * @return true: 成功, false: 失败
 * @note   直接寄存器操作，不调用 SDK API
 */
static bool aps6408l_init_read_reg_dtr(uint8_t reg_addr, uint16_t *value)
{
    uint16_t rx_data;

    if (value == NULL) { return false; }

    /* 停止传输 */
    OSPI1->TX_CTL &= ~(1UL << 0);  /* TX_EN=0 */
    OSPI1->RX_CTL &= ~(1UL << 0);  /* RX_EN=0 */

    /* 清除中断状态 */
    OSPI1->STATUS = 0xFFFFU;

    /* 复位 FIFO */
    OSPI1->TX_CTL |= (1UL << 1);   /* TX_FIFO_RESET=1 */
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);   /* RX_FIFO_RESET=1 */
    OSPI1->RX_CTL &= ~(1UL << 1);

    /* 检查 ACC_EN */
    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U) { return false; }

    /* 禁用 memory-mapped */
    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    /* 发送 DTR 命令帧：cmd(2B) + dummy(2B) + addr(2B) + 4×dummy(8B) = 14B */
    OSPI1->CS = 0U;                 /* 释放 CS */
    OSPI1->BATCH = 14U;             /* 批量传输 14 字节 */
    OSPI1->TX_CTL |= (1UL << 0);   /* TX_EN=1 */
    OSPI1->CS = 0x01U;              /* 断言 CS0 */

    psram_write_fifo_half(psram_dtr_cmd(APS6408L_READ_REG));
    psram_write_fifo_half(0x0000U);
    psram_write_fifo_half((uint16_t)reg_addr << 8U);
    psram_write_fifo_half(0x0000U);
    psram_write_fifo_half(0x0000U);
    psram_write_fifo_half(0x0000U);
    psram_write_fifo_half(0x0000U);

    psram_wait_status(1UL << 14);   /* 等待 TX_BATCH_DONE */

    OSPI1->TX_CTL &= ~(1UL << 0);  /* TX_EN=0 */
    OSPI1->STATUS = (1UL << 1) | (1UL << 14);  /* 清除 BATCH_DONE + TX_BATCH_DONE */

    /* 接收 2 字节数据 */
    OSPI1->BATCH = 2U;
    OSPI1->RX_CTL |= (1UL << 0);   /* RX_EN=1 */
    OSPI1->CS = 0x01U;              /* 断言 CS0 */

    rx_data = psram_read_fifo_half();

    psram_wait_status(1UL << 15);   /* 等待 RX_BATCH_DONE */

    OSPI1->RX_CTL &= ~(1UL << 0);  /* RX_EN=0 */
    OSPI1->CS = 0U;                 /* 释放 CS */
    OSPI1->STATUS = 0xFFFFU;        /* 清除所有中断 */

    *value = rx_data;

    /* 重新使能 memory-mapped */
    OSPI1->MEMO_ACC1 |= (1UL << 0);

    return true;
}

/**
 * @brief  通过 OSPI1 FIFO DTR 写入 PSRAM 模式寄存器
 * @param[in] reg_addr  寄存器地址
 * @param[in] value     要写入的 16 位值
 * @return true: 成功, false: 失败
 * @note   直接寄存器操作，不调用 SDK API
 */
static bool aps6408l_init_write_reg_dtr(uint8_t reg_addr, uint16_t value)
{
    /* 停止传输 */
    OSPI1->TX_CTL &= ~(1UL << 0);  /* TX_EN=0 */
    OSPI1->RX_CTL &= ~(1UL << 0);  /* RX_EN=0 */

    /* 清除中断状态 */
    OSPI1->STATUS = 0xFFFFU;

    /* 复位 FIFO */
    OSPI1->TX_CTL |= (1UL << 1);   /* TX_FIFO_RESET=1 */
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);   /* RX_FIFO_RESET=1 */
    OSPI1->RX_CTL &= ~(1UL << 1);

    /* 检查 ACC_EN */
    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U) { return false; }

    /* 禁用 memory-mapped */
    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    /* 发送 DTR 命令帧：cmd(2B) + dummy(2B) + addr(2B) + data(2B) = 8B */
    OSPI1->CS = 0U;                 /* 释放 CS */
    OSPI1->BATCH = 8U;              /* 批量传输 8 字节 */
    OSPI1->TX_CTL |= (1UL << 0);   /* TX_EN=1 */
    OSPI1->CS = 0x01U;              /* 断言 CS0 */

    psram_write_fifo_half(psram_dtr_cmd(APS6408L_WRITE_REG));
    psram_write_fifo_half(0x0000U);
    psram_write_fifo_half((uint16_t)reg_addr << 8U);
    psram_write_fifo_half(value);

    psram_wait_status(1UL << 14);   /* 等待 TX_BATCH_DONE */

    OSPI1->TX_CTL &= ~(1UL << 0);  /* TX_EN=0 */
    OSPI1->CS = 0U;                 /* 释放 CS */
    OSPI1->STATUS = 0xFFFFU;        /* 清除所有中断 */

    /* 重新使能 memory-mapped */
    OSPI1->MEMO_ACC1 |= (1UL << 0);

    return true;
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */

#if SYSTEM_INIT_OSPI_PSRAM

/**
 * @brief  通过 FIFO DTR 配置 APS6408L MR0 的固定读延时位
 * @return true: 成功, false: 失败
 * @note   从 ospi_example.c 的 aps6408l_config_fixed_read_latency() 移植，
 *         去掉了 printf 调试输出（SystemInit 阶段 RTT 不可用）
 */
static bool psram_config_mr0_fixed_latency(void)
{
    /* 读 MR0 */
    uint16_t mr0_val;
    if (!aps6408l_init_read_reg_dtr(APS6408L_REG_MR0, &mr0_val)) {
        return false;
    }

    /* 读 MR1 校验设备 ID */
    uint16_t mr1;
    if (!aps6408l_init_read_reg_dtr(APS6408L_REG_MR1, &mr1)) {
        return false;
    }

    /* 校验 PSRAM 是否就绪 */
    if ((mr0_val == 0x0000U) || (mr0_val == 0xFFFFU) ||
        (mr1 != APS6408L_DEVICE_ID)) {
        return false;
    }

    /* 如果已设置，直接返回 */
    if ((mr0_val & APS6408L_MR0_FIXED_READ_LATENCY) != 0U) {
        OSPI1->CS = 0x01U;
        return true;
    }

    /* 设置 MR0[5] = 1 */
    mr0_val |= APS6408L_MR0_FIXED_READ_LATENCY;
    if (!aps6408l_init_write_reg_dtr(APS6408L_REG_MR0, mr0_val)) {
        return false;
    }

    /* 回读验证 */
    uint16_t verify;
    if (!aps6408l_init_read_reg_dtr(APS6408L_REG_MR0, &verify)) {
        return false;
    }
    if ((verify & APS6408L_MR0_FIXED_READ_LATENCY) == 0U) {
        return false;
    }

    OSPI1->CS = 0x01U;
    return true;
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */

#if SYSTEM_INIT_OSPI_PSRAM

/**
 * @brief  配置 OSPI1 控制器为 8 线 DTR Xccela OPI 模式并启用 memory-mapped
 * @return true: 成功, false: 失败（静默失败，SystemInit 阶段无输出能力）
 *
 * @note   直接寄存器配置，不调用 SDK API，避免 SystemInit 阶段栈溢出
 *         配置值：MEM_TYPE=XCC_ELA / CMD=0xA0A02020 / DQSOE=1 /
 *         read_dummy=8 / write_dummy=3 / RD_DB_EN=1 / WR_DB_EN=1 /
 *         SSHIFT=4 / OUTDLY=1
 */
static bool psram_config_ospi_controller(void)
{
    OSPI_TypeDef *ospi = OSPI1;

    /* 1. 软件复位 OSPI 控制器 */
    ospi->CTL = 0U;
    ospi->TX_CTL = 0U;
    ospi->RX_CTL = 0U;

    /* 2. 配置波特率：div1=4 → OSPI CLK = HCLK/(2*(4+1)) = 18MHz */
    ospi->BAUD = 4U;

    /* 3. 配置控制寄存器
     *   MST_MODE=1: 主机模式
     *   CPHA=0, CPOL=0: SPI Mode 0
     *   LSB_FIRST=0: MSB 在前
     *   X_MODE=3: 8 线模式
     *   IO_MODE=1: 硬件自动切换
     *   DTRM=1: DTR 双沿采样
     *   DQSOE=1: DQS 输出使能
     *   FW_MODE=1: FIFO 半字模式
     *   FR_MODE=1: FIFO 半字模式
     *   MEM_MODE=0: Xccela OPI */
    ospi->CTL = (1U << 0)  |  /* MST_MODE */
                (3U << 5)  |  /* X_MODE = 8线 */
                (1U << 7)  |  /* IO_MODE = 硬件自动 */
                (1U << 8)  |  /* DTRM = DTR模式 */
                (1U << 9)  |  /* DQSOE = DQS使能 */
                (1U << 28) |  /* FW_MODE = 1 (half) */
                (1U << 30) |  /* FR_MODE = 1 (half) */
                (0U << 21);   /* MEM_MODE = XCC_ELA */

    /* 4. 配置 TX_CTL
     *   OUTDLY=1: 输出延时半 HCLK */
    ospi->TX_CTL = (1U << 16);

    /* 5. 配置 RX_CTL
     *   SSHIFT=3: 采样点 2.5 HCLK 延迟 */
    ospi->RX_CTL = (3U << 28);

    /* 6. 配置输出使能：全部 8 线 */
    ospi->OUT_EN = 0xFFU;

    /* 7. 配置 CS0 */
    ospi->CS = 0x01U;

    /* 8. 配置 CMD 寄存器（DTR 双字节编码）
     *   读命令：0x20 → 0x2020
     *   写命令：0xA0 → 0xA0A0 */
    ospi->CMD = 0xA0A02020U;

    /* 9. 配置 MEMO_ACC1（内存访问寄存器 1）
     *   ACC_EN=1: 使能内存访问
     *   CON_MODE_EN=1: 连续模式
     *   WR_DB_EN=1: 写 dummy 按字节计量
     *   RD_DB_EN=1: 读 dummy 按字节计量
     *   DUMMY_CYCLE_SIZE=4: 读 dummy 初始值（MR0 配置后改为 8）
     *   ADDR_WIDTH=3: 32 位地址 */
    ospi->MEMO_ACC1 = (1U << 0)  |  /* ACC_EN */
                      (1U << 3)  |  /* CON_MODE_EN */
                      (1U << 10) |  /* WR_DB_EN */
                      (1U << 11) |  /* RD_DB_EN */
                      (4U << 12) |  /* DUMMY_CYCLE_SIZE = 4（初始值） */
                      (3U << 17);   /* ADDR_WIDTH = 32bit */

    /* 10. 配置 MEMO_ACC2（内存访问寄存器 2）
     *   HXSPI_LC0=1: HyperBus 延迟控制 0
     *   HXSPI_LC1=1: HyperBus 延迟控制 1
     *   RBL=7: 读突发长度 1024B
     *   WBL=7: 写突发长度 1024B
     *   WLC=3: 写 dummy 4 周期 */
    ospi->MEMO_ACC2 = (1U << 0)  |  /* HXSPI_LC0 = 1 */
                      (1U << 4)  |  /* HXSPI_LC1 = 1 */
                      (7U << 8)  |  /* RBL = 1024B */
                      (7U << 15) |  /* WBL = 1024B */
                      (3U << 27);   /* WLC = 4周期 */

    /* 11. 配置 MEMO_ACC3（内存访问寄存器 3）
     *   CMD_SD=1: 命令串行数据
     *   CMD_BYTE=1: 双字节命令 */
    ospi->MEMO_ACC3 = (1U << 0) |  /* CMD_SD */
                      (1U << 1);   /* CMD_BYTE */

    /* 12. 配置 CS 超时 */
    ospi->CS_TOUT_VAL = (1U << 16) |  /* CS_DELAY_VAL = 1 */
                        (500U);        /* CS_TOUT_VAL = 500 */

    /* 13. 配置 MR0 固定读延时 */
    if (!psram_config_mr0_fixed_latency()) {
        return false;
    }

    /* 14. 更新 MEMO_ACC1 的 DUMMY_CYCLE_SIZE 为 8（MR0 配置后需要更多 dummy） */
    ospi->MEMO_ACC1 = (ospi->MEMO_ACC1 & ~(0x1FU << 12)) |
                      (8U << 12);

    /* 15. 使能 memory-mapped */
    ospi->MEMO_ACC1 |= (1U << 0);

    return true;
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */

#if SYSTEM_INIT_OSPI_PSRAM

/**
 * @brief  初始化 OSPI1 的 GPIO 引脚和 PSRAM 控制器
 * @note   SystemInit 阶段调用，不可用 printf / delay_ms / 中断
 * @note   失败时静默返回，不影响后续启动流程
 */
static void system_init_ospi_psram(void)
{
    volatile uint32_t *psram = (volatile uint32_t *)0x80000000U;

    psram_init_gpio_and_reset();
    psram_config_ospi_controller();
    __DSB();
    __ISB();
    for (volatile uint32_t i = 0; i < 10000; i++) { __NOP(); }
		#if 0
    /* PSRAM 全片读写验证（8MB），失败则禁用 memory-mapped */
    {
        volatile uint32_t *p = psram;
        uint32_t count = 0x800000U / 4U;  /* 8MB / 4B = 2M 次 */
        bool pass = true;

        /* 全片写入地址递增模式 */
        for (uint32_t i = 0U; i < count; i++) {
            p[i] = i;
        }
        __DSB();
        __ISB();

        /* 全片读回校验 */
        for (uint32_t i = 0U; i < count; i++) {
            if (p[i] != i) {
                pass = false;
                break;
            }
        }

        if (!pass) {
            OSPI1->MEMO_ACC1 &= ~(1U << 0);
        }
    }
		#endif
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */


/* 系统时钟频率变量 */
uint32_t SystemCoreClock = 180000000U; /* 默认 180MHz */


/**
 * @brief  系统初始化函数
 * @note   在 Reset_Handler 中调用，在进入 main 之前执行
 * @retval None
 */
void SystemInit(void)
{
    /* 配置向量表偏移（使用启动文件中的 g_pfnVectors 符号） */
    extern uint32_t __Vectors[];
    SCB->VTOR = (uint32_t)__Vectors;

    EMSS->ITCMCR |= (1UL << 0U); // ITCM 使能
    EMSS->DTCMCR |= (1UL << 0U); // DTCM 使能


    /* 使能指令缓存（ICache）*/
    SCB->ICIALLU = 0UL; /* 无效化全部 ICache 行 */
    __DSB();
    __ISB();
    SCB->CCR |= (1UL << 17U); /* CCR.IC = 1：使能 ICache */
    __DSB();
    __ISB();

#if SYSTEM_ENABLE_DCACHE
    /* 使能数据缓存（DCache）*/
    SCB->DCCIMVAC = 0UL; /* 无效化全部 DCache 行 */
    __DSB();
    __ISB();
    SCB->CCR |= (1UL << 18U); /* CCR.DC = 1：使能 DCache */
    __DSB();
    __ISB();
#endif

    /* FPU 配置 */
#if defined(__FPU_PRESENT) && (__FPU_PRESENT == 1U)
    SCB->CPACR |= ((3UL << 20U) | (3UL << 22U)); /* CP10/CP11 Full Access */
    __DSB();
    __ISB();
#endif

    /* ----------------------------------------------------------------
     * 时钟初始化：RCH(64MHz) → PLL1 → SYSCLK 180MHz
     *   与 clock_init_default() 等效的寄存器级展开序列：
     *     F_in  = RCH / RCHDIV(÷16) = 4 MHz
     *     PFD   = F_in / N(÷2)      = 2 MHz  （满足 1~2 MHz 约束）
     *     VCO   = PFD × F(×180)     = 360 MHz （满足 100~550 MHz 约束）
     *     PCLK  = VCO / P(÷2)       = 180 MHz → SYSCLK / HCLK
     *     QCLK  = VCO / Q(÷3)       = 120 MHz → 外设时钟备用
     *     PCLK1~2 = HCLK / 2        = 90 MHz
     * ---------------------------------------------------------------- */

    /* Step 1 ── clock_rch_enable(true)
     *   使能 RCH 并开启 16 分频，为 PLL1 提供 4 MHz 输入源。
     *   RCHCR[0] RCHEN  = 1 → 启动 RCH 振荡器
     *   RCHCR[3] RCHDIV = 1 → 输出 ÷16（64 MHz / 16 = 4 MHz）
     *   RCHCR[4] RCHRDY     → 轮询等待振荡器输出稳定 */
    RCC->RCHCR |= (1U << 0);
    RCC->RCHCR |= (1U << 3);
    { uint32_t timeout = SYSTEM_CLOCK_TIMEOUT;
      while (!(RCC->RCHCR & (1U << 4))) { if (--timeout == 0U) break; } }

    /* Step 2 ── clock_pll1_configure(src=RCH, N=2, F=180, P=0, Q=2)
     *   PLL1CR[0]    PLL1EN     = 1 → 使能 PLL1
     *   PLL1CR[1]    PLL1SRCSEL = 0 → 时钟源选 RCH/16（4 MHz）
     *   PLL1CR[21]   PLL1SLEEP  = 0 → 退出休眠，VCO 开始工作
     *   （延时约 1000 周期等待 VCO 初始稳定）
     *   PLL1CFR[8:0]  F  = 180  → VCO 倍频系数
     *   PLL1CFR[17:12] N = 2   → 输入分频系数
     *   PLL1CFR[21:20] P = 0   → P 输出分频，实际 ÷(2×(P+1)) = ÷2
     *   PLL1CFR[27:24] Q = 2   → Q 输出分频，实际 Q+1 = ÷3
     *   PLL1CR[4]    PLL1PCLKEN  = 1 → 使能 P 时钟输出（180 MHz）
     *   PLL1CR[5]    PLL1QCLKEN  = 1 → 使能 Q 时钟输出（120 MHz）
     *   PLL1CR[22]   PLL1UPDATEEN    → 触发参数更新，写 1 生效
     *   PLL1CR[29]   PLL1LOCK        → 轮询等待 PLL 锁定 */
    RCC->PLL1CR |=  (1U << 0);
    RCC->PLL1CR &= ~(1U << 1);
    RCC->PLL1CR &= ~(1U << 21);
    for (volatile uint32_t i = 1000U; i != 0U; --i) { __NOP(); }
    RCC->PLL1CFR = (2U   << 24)
                 | (0U   << 20)
                 | (2U   << 12)
                 | (180U <<  0);
    RCC->PLL1CR |= (1U << 4);
    RCC->PLL1CR |= (1U << 5);
    RCC->PLL1CR |= (1U << 22);
    { uint32_t timeout = SYSTEM_CLOCK_TIMEOUT;
      while (!(RCC->PLL1CR & (1U << 29))) { if (--timeout == 0U) break; } }

    /* Step 3 ── clock_set_sys_divider(1, 1)
     *   HCLK = SYSCLK / (SYSDIV0+1) / (SYSDIV1+1)
     *   CCR2[3:0]  SYSDIV0 = 0 → ÷1（寄存器值 = 分频系数 − 1）
     *   CCR2[7:4]  SYSDIV1 = 0 → ÷1，HCLK = SYSCLK = 180 MHz
     *   CCR2[31]   DIVDONE     → 轮询等待分频切换完成 */
    RCC->CCR2 &= ~((0xFU << 0) | (0xFU << 4));
    { uint32_t timeout = SYSTEM_CLOCK_TIMEOUT;
      while (!(RCC->CCR2 & (1U << 31))) { if (--timeout == 0U) break; } }

    /* Step 4 ── clock_set_apb_divider(2, 2)
     *   PCLKx = HCLK / 2 = 90 MHz
     *   寄存器编码：分频值 2 → encode_apb_div(2) = 4（二进制 100）
     *   CCR2[10:8]  PCLK1DIV = 4 → APB1 ÷2
     *   CCR2[13:11] PCLK2DIV = 4 → APB2 ÷2 */
    RCC->CCR2 &= ~((0x7U << 8) | (0x7U << 11));
    RCC->CCR2 |=  ((4U   << 8) | (4U   << 11));

    /* Step 5 ── clock_set_sysclk_source(SYSCLK_SRC_PLL1)
     *   PLL1LOCK 已在 Step 2 确认，直接切换时钟源。
     *   CCR1[1:0] SYSCLKSEL = 11 → 系统时钟切换至 PLL1（180 MHz）
     *   切换后短暂等待流水线刷新稳定 */
    RCC->CCR1 = (RCC->CCR1 & ~0x3U) | 0x3U;
    for (volatile uint32_t i = 100U; i != 0U; --i) { __NOP(); }

    /* 更新 CMSIS SystemCoreClock 变量（clock_update_system_core_clock 等效） */
    SystemCoreClock = 180000000U;

    /* ----------------------------------------------------------------
     * TIM64B（TIM26）初始化：自由计数模式
     *   与 tim64b_init_freerun(TIM64B_TIMEBASE_PRESCALER=1) 等效的展开序列：
     *   计数频率 = PCLK2 ÷ 1 = 180 MHz（分辨率约 5.6 ns）
     *   计数范围：0 → 2^64-1，计满后自然回零，无溢出中断。
     * ---------------------------------------------------------------- */

    /* Step A ── clock_periph_enable(CLK_TIM26)
     *   CLK_TIM26 = 0x0405：reg_idx=4 → APB1CKENR2，bit=5 → TIM26CKEN
     *   APB1CKENR2[5] = 1 → 打开 TIM26 外设时钟，使寄存器可访问 */
    RCC->APB1CKENR2 |= (1U << 5);

    /* Step B ── 配置 CTRL（CEN=0，停止状态，自由计数模式）
     *   prescaler=1 → PSC 寄存器值 = prescaler - 1 = 0
     *   CTRL[7:2] PSC  = 0 → 计数时钟 = PCLK2 ÷ 1（不分频）
     *   CTRL[1]   CMOD = 0 → 自由计数模式，计满 2^64-1 后回零
     *   CTRL[8]   IE   = 0 → 禁用更新中断（时基不需要中断）
     *   CTRL[9]   ARPE = 0 → 不使用 ARR 预加载缓冲
     *   整体写 0，一次清除所有字段，避免残留配置干扰 */
    TIM26->CTRL = 0U;

    /* Step C ── write_cnt(0)：计数器清零并触发硬件加载
     *   先写 CNTL（低 32 位），再写 CNTH（高 32 位）
     *   CTRL[11] CNTLOAD = 1 → 触发将 CNTL/CNTH 载入计数器（只写，硬件自动清零） */
    TIM26->CNTL = 0U;
    TIM26->CNTH = 0U;
    TIM26->CTRL |= (1U << 11);

    /* Step D ── 启动计数器
     *   CTRL[0] CEN = 1 → TIM26 从 0 开始持续向上自由计数 */
    TIM26->CTRL |= (1U << 0);

#if SYSTEM_MPU_OSPI_PSRAM
    system_mpu_ospi_psram();
#endif
#if SYSTEM_INIT_OSPI_PSRAM
    system_init_ospi_psram();
#endif
}

/**
 * @brief  更新 SystemCoreClock 变量
 * @retval None
 */
void SystemCoreClockUpdate(void)
{
    uint32_t sysclk_hz;
    uint32_t src = RCC->CCR1 & 0x3U;

    switch (src) {
        case 0x0: sysclk_hz = 64000000U; break;   /* RCH 64MHz */
        case 0x1: sysclk_hz = 24000000U; break;   /* XTH 24MHz */
        case 0x2: sysclk_hz = 32768U;    break;   /* XTL 32.768KHz */
        case 0x3: {                                /* PLL1 */
            uint32_t f  = (RCC->PLL1CFR & 0x1FFU);
            uint32_t n  = (RCC->PLL1CFR >> 12) & 0x3FU;
            uint32_t p  = (RCC->PLL1CFR >> 20) & 0x3U;
            uint32_t src_sel = (RCC->PLL1CR >> 1) & 0x1U;
            uint32_t fin = (src_sel == 0U) ? (64000000U / 16U) : 24000000U;
            uint32_t pfd = fin / (n + 1U);
            uint32_t vco = pfd * (f + 1U);
            sysclk_hz = vco / (2U * (p + 1U));
            break;
        }
        default: sysclk_hz = 64000000U; break;
    }

    uint32_t sysdiv0 = RCC->CCR2 & 0xFU;
    uint32_t sysdiv1 = (RCC->CCR2 >> 4) & 0xFU;
    uint32_t hclk = sysclk_hz / (sysdiv0 + 1U) / (sysdiv1 + 1U);

    SystemCoreClock = hclk;
}
