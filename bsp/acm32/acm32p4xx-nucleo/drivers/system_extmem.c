/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * OSPI1 APS6408L PSRAM early init for SystemInit_ExtMemCtl().
 * Register-level only; no HAL/SDK calls (runs before main / RT-Thread).
 *
 * Pins: PG5 SCK, PG6-13 IO0-7, PG14 CS, PG15 DQS, PF10 RESET
 * Map:  0x80000000 ~ 0x807FFFFF (8MB)
 */

#include "acm32p4xx_hal_conf.h"

#ifdef DATA_IN_ExtSRAM

#ifndef SYSTEM_INIT_OSPI_PSRAM
#define SYSTEM_INIT_OSPI_PSRAM  1U
#endif

/* Non-cacheable PSRAM: required when ETH DMA uses OSPI-mapped buffers */
#ifndef SYSTEM_MPU_OSPI_PSRAM
#define SYSTEM_MPU_OSPI_PSRAM   1U
#endif

#if SYSTEM_INIT_OSPI_PSRAM

#define APS6408L_SYNC_READ_LINEAR_BURST   0x20U
#define APS6408L_SYNC_WRITE_LINEAR_BURST  0xA0U
#define APS6408L_READ_REG                 0x40U
#define APS6408L_WRITE_REG                0xC0U
#define APS6408L_REG_MR0                  0x00U
#define APS6408L_REG_MR1                  0x01U
#define APS6408L_MR0_FIXED_READ_LATENCY   (1U << 5)
#define APS6408L_DEVICE_ID                0x930DU

/* ~1ms @ ~64..180MHz (SystemInit may run before PLL switch) */
#define PSRAM_RESET_DELAY_CYCLES          180000U

#define SYSTEM_PSRAM_BASE_ADDR            0x80000000U
#define SYSTEM_PSRAM_LIMIT_ADDR           0x807FFFFFU

static void psram_nop_delay(volatile uint32_t cycles)
{
    while (cycles != 0U)
    {
        __NOP();
        cycles--;
    }
}

static void psram_init_gpio_and_reset(void)
{
    /* GPIOF/GPIOG clocks (AHB2), OSPI1 clock (AHB3) */
    RCC->AHB2CKENR |= (1UL << 5) | (1UL << 6);
    RCC->AHB3CKENR |= (1UL << 8);

    /* OSPI1 reset pulse */
    RCC->AHB3RSTR &= ~(1UL << 8);
    psram_nop_delay(100U);
    RCC->AHB3RSTR |= (1UL << 8);
    psram_nop_delay(100U);

    /* PG5-15 AF mode (MD=10) */
    GPIOG->MD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                   (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                   (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                   (0x3UL << 28) | (0x3UL << 30));
    GPIOG->MD |=  (2UL << 10) | (2UL << 12) | (2UL << 14) |
                  (2UL << 16) | (2UL << 18) | (2UL << 20) |
                  (2UL << 22) | (2UL << 24) | (2UL << 26) |
                  (2UL << 28) | (2UL << 30);

    /* AF: PG5-9/12-15=AF2, PG10=AF6, PG11=AF5 */
    GPIOG->AF0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    GPIOG->AF0 |=  (2UL << 20) | (2UL << 24) | (2UL << 28);

    GPIOG->AF1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                    (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                    (0xFUL << 24) | (0xFUL << 28));
    GPIOG->AF1 |=  (2UL << 0)  | (2UL << 4)  | (6UL << 8)  |
                   (5UL << 12) | (2UL << 16) | (2UL << 20) |
                   (2UL << 24) | (2UL << 28);

    /* Pull-up + drive 8mA */
    GPIOG->PUPD &= ~((0x3UL << 10) | (0x3UL << 12) | (0x3UL << 14) |
                     (0x3UL << 16) | (0x3UL << 18) | (0x3UL << 20) |
                     (0x3UL << 22) | (0x3UL << 24) | (0x3UL << 26) |
                     (0x3UL << 28) | (0x3UL << 30));
    GPIOG->PUPD |=  (1UL << 10) | (1UL << 12) | (1UL << 14) |
                    (1UL << 16) | (1UL << 18) | (1UL << 20) |
                    (1UL << 22) | (1UL << 24) | (1UL << 26) |
                    (1UL << 28) | (1UL << 30);

    GPIOG->DS0 &= ~((0xFUL << 20) | (0xFUL << 24) | (0xFUL << 28));
    GPIOG->DS0 |=  (4UL << 20) | (4UL << 24) | (4UL << 28);

    GPIOG->DS1 &= ~((0xFUL << 0) | (0xFUL << 4) | (0xFUL << 8) |
                    (0xFUL << 12) | (0xFUL << 16) | (0xFUL << 20) |
                    (0xFUL << 24) | (0xFUL << 28));
    GPIOG->DS1 |=  (4UL << 0) | (4UL << 4) | (4UL << 8) |
                   (4UL << 12) | (4UL << 16) | (4UL << 20) |
                   (4UL << 24) | (4UL << 28);

    /* PF10 push-pull output: PSRAM RESET */
    GPIOF->MD &= ~(3UL << 20);
    GPIOF->MD |=  (1UL << 20);
    GPIOF->OTYP &= ~(1UL << 10);
    GPIOF->PUPD &= ~(3UL << 20);

    GPIOF->ODATA &= ~(1UL << 10);
    psram_nop_delay(PSRAM_RESET_DELAY_CYCLES);
    GPIOF->ODATA |=  (1UL << 10);
    psram_nop_delay(PSRAM_RESET_DELAY_CYCLES);
}

static uint16_t psram_dtr_cmd(uint8_t cmd)
{
    return (uint16_t)cmd | ((uint16_t)cmd << 8);
}

static int psram_wait_status(uint32_t mask)
{
    uint32_t t = 2000000U;
    while (((OSPI1->STATUS & mask) == 0U) && (t != 0U))
        t--;
    return (t != 0U) ? 1 : 0;
}

/* STATUS: bit3=TX_FIFO_FULL, bit4=RX_FIFO_EMPTY (wait until clear) */
static int psram_write_fifo_half(uint16_t data)
{
    uint32_t t = 2000000U;
    while ((OSPI1->STATUS & OSPI_STATUS_TX_FIFO_FULL) && (t != 0U))
        t--;
    if (t == 0U)
        return 0;
    OSPI1->DAT = data;
    return 1;
}

static int psram_read_fifo_half(uint16_t *data)
{
    uint32_t t = 2000000U;
    while ((OSPI1->STATUS & OSPI_STATUS_RX_FIFO_EMPTY) && (t != 0U))
        t--;
    if (t == 0U)
        return 0;
    *data = (uint16_t)(OSPI1->DAT & 0xFFFFU);
    return 1;
}

static volatile uint16_t g_psram_mr0;
static volatile uint16_t g_psram_mr1;
static volatile int g_psram_mr_ok;
static volatile int g_psram_init_ok;

static int aps6408l_read_reg_dtr(uint8_t reg_addr, uint16_t *value)
{
    uint16_t rx_data;

    if (value == 0)
        return 0;

    /* stop + clear + fifo reset (align SDK ospi_stop / reset_fifo) */
    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = 0xFFFFU;
    OSPI1->TX_CTL |= (1UL << 1);
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);
    OSPI1->RX_CTL &= ~(1UL << 1);

    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U)
        return 0;

    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    OSPI1->CS = 0U;
    OSPI1->BATCH = 14U;
    OSPI1->TX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!psram_write_fifo_half(psram_dtr_cmd(APS6408L_READ_REG)) ||
        !psram_write_fifo_half(0x0000U) ||
        !psram_write_fifo_half((uint16_t)reg_addr << 8) ||
        !psram_write_fifo_half(0x0000U) ||
        !psram_write_fifo_half(0x0000U) ||
        !psram_write_fifo_half(0x0000U) ||
        !psram_write_fifo_half(0x0000U))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    if (!psram_wait_status(OSPI_STATUS_TX_BATCH_DONE))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = OSPI_STATUS_BATCH_DONE | OSPI_STATUS_TX_BATCH_DONE;

    OSPI1->BATCH = 2U;
    OSPI1->RX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!psram_read_fifo_half(&rx_data) ||
        !psram_wait_status(OSPI_STATUS_RX_BATCH_DONE))
    {
        OSPI1->RX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->CS = 0U;
    OSPI1->STATUS = 0xFFFFU;

    *value = rx_data;
    OSPI1->MEMO_ACC1 |= (1UL << 0);
    return 1;
}

static int aps6408l_write_reg_dtr(uint8_t reg_addr, uint16_t value)
{
    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->RX_CTL &= ~(1UL << 0);
    OSPI1->STATUS = 0xFFFFU;
    OSPI1->TX_CTL |= (1UL << 1);
    OSPI1->TX_CTL &= ~(1UL << 1);
    OSPI1->RX_CTL |= (1UL << 1);
    OSPI1->RX_CTL &= ~(1UL << 1);

    if ((OSPI1->MEMO_ACC1 & (1UL << 0)) == 0U)
        return 0;

    OSPI1->MEMO_ACC1 &= ~(1UL << 0);

    OSPI1->CS = 0U;
    OSPI1->BATCH = 8U;
    OSPI1->TX_CTL |= (1UL << 0);
    OSPI1->CS = 0x01U;

    if (!psram_write_fifo_half(psram_dtr_cmd(APS6408L_WRITE_REG)) ||
        !psram_write_fifo_half(0x0000U) ||
        !psram_write_fifo_half((uint16_t)reg_addr << 8) ||
        !psram_write_fifo_half(value) ||
        !psram_wait_status(OSPI_STATUS_TX_BATCH_DONE))
    {
        OSPI1->TX_CTL &= ~(1UL << 0);
        OSPI1->CS = 0U;
        OSPI1->MEMO_ACC1 |= (1UL << 0);
        return 0;
    }

    OSPI1->TX_CTL &= ~(1UL << 0);
    OSPI1->CS = 0U;
    OSPI1->STATUS = 0xFFFFU;

    OSPI1->MEMO_ACC1 |= (1UL << 0);
    return 1;
}

static int psram_config_mr0_fixed_latency(void)
{
    uint16_t mr0_val = 0;
    uint16_t mr1 = 0;
    uint16_t verify = 0;

    g_psram_mr0 = 0;
    g_psram_mr1 = 0;
    g_psram_mr_ok = 0;

    if (!aps6408l_read_reg_dtr(APS6408L_REG_MR0, &mr0_val))
        return 0;
    if (!aps6408l_read_reg_dtr(APS6408L_REG_MR1, &mr1))
        return 0;

    g_psram_mr0 = mr0_val;
    g_psram_mr1 = mr1;

    /* Accept valid MR0 and known device ID (or already-fixed MR0) */
    if ((mr0_val == 0x0000U) || (mr0_val == 0xFFFFU))
        return 0;
    if ((mr1 != APS6408L_DEVICE_ID) && (mr1 != 0x0D93U))
    {
        /* still try if MR0 looks sane */
        if ((mr0_val & 0xFF00U) == 0U)
            return 0;
    }

    if ((mr0_val & APS6408L_MR0_FIXED_READ_LATENCY) == 0U)
    {
        mr0_val |= APS6408L_MR0_FIXED_READ_LATENCY;
        if (!aps6408l_write_reg_dtr(APS6408L_REG_MR0, mr0_val))
            return 0;
        if (!aps6408l_read_reg_dtr(APS6408L_REG_MR0, &verify))
            return 0;
        g_psram_mr0 = verify;
        if ((verify & APS6408L_MR0_FIXED_READ_LATENCY) == 0U)
            return 0;
    }

    OSPI1->CS = 0x01U;
    g_psram_mr_ok = 1;
    return 1;
}

/* BAUD.div1: OSPI_CLK = HCLK / (2 * (div1 + 1)); target ~18MHz */
static uint32_t psram_calc_baud_div(void)
{
    extern volatile uint32_t SystemCoreClock;
    uint32_t hclk = SystemCoreClock;

    if (hclk == 0U)
        hclk = 180000000U;
    /* match SDK: 180MHz HCLK -> div1=4 -> 18MHz OSPI */
    if (hclk >= 160000000U)
        return 4U;
    if (hclk >= 100000000U)
        return 2U;
    return 1U;
}

static int psram_config_ospi_controller(void)
{
    OSPI_TypeDef *ospi = OSPI1;
    uint32_t baud = psram_calc_baud_div();

    ospi->CTL = 0U;
    ospi->TX_CTL = 0U;
    ospi->RX_CTL = 0U;

    ospi->BAUD = baud;

    /* master, 8-line, IO auto, DTR, DQS, half-word FIFO, Xccela — match SDK */
    ospi->CTL = (1U << 0) | (3U << 5) | (1U << 7) | (1U << 8) |
                (1U << 9) | (1U << 28) | (1U << 30);

    /* OUTDLY=1, SSHIFT=3 (SDK working value; doc also lists 4) */
    ospi->TX_CTL = (1U << 16);
    ospi->RX_CTL = (3U << 28);
    ospi->OUT_EN = 0xFFU;
    ospi->CS = 0x01U;
    ospi->CMD = 0xA0A02020U;

    ospi->MEMO_ACC1 = (1U << 0) | (1U << 3) | (1U << 10) | (1U << 11) |
                      (4U << 12) | (3U << 17);

    ospi->MEMO_ACC2 = (1U << 0) | (1U << 4) | (7U << 8) | (7U << 15) |
                      (3U << 27);

    ospi->MEMO_ACC3 = (1U << 0) | (1U << 1);

    ospi->CS_TIMEOUT_VAL = (1U << 16) | 500U;

    if (!psram_config_mr0_fixed_latency())
    {
        /* Still enable map with dummy=8 for probe; MR values left in g_psram_mr* */
        ospi->MEMO_ACC1 = (ospi->MEMO_ACC1 & ~(0x1FUL << 12)) | (8U << 12);
        ospi->MEMO_ACC1 |= (1U << 0);
        return 0;
    }

    ospi->MEMO_ACC1 = (ospi->MEMO_ACC1 & ~(0x1FUL << 12)) | (8U << 12);
    ospi->MEMO_ACC1 |= (1U << 0);
    return 1;
}

static int system_init_ospi_psram(void)
{
    psram_init_gpio_and_reset();
    if (!psram_config_ospi_controller())
    {
        g_psram_init_ok = 0;
        return 0;
    }

    __DSB();
    __ISB();
    psram_nop_delay(10000U);
    g_psram_init_ok = 1;
    return 1;
}

int system_ospi_psram_ready(void)
{
    return g_psram_init_ok;
}

void system_ospi_psram_get_mr(uint16_t *mr0, uint16_t *mr1, int *mr_ok)
{
    if (mr0)
        *mr0 = g_psram_mr0;
    if (mr1)
        *mr1 = g_psram_mr1;
    if (mr_ok)
        *mr_ok = g_psram_mr_ok;
}

/* Full re-init after SystemClock_Config() */
void system_ospi_psram_reclock(void)
{
#if SYSTEM_INIT_OSPI_PSRAM
    (void)system_init_ospi_psram();
#endif
}

#endif /* SYSTEM_INIT_OSPI_PSRAM */

#if SYSTEM_MPU_OSPI_PSRAM
static void system_mpu_ospi_psram(void)
{
#if defined(__MPU_PRESENT) && (__MPU_PRESENT == 1U)
    /* ARMv8-M RBAR: RO=0 (RW), NP=1 (unpriv OK), XN=1 (no-exec) */
    ARM_MPU_Disable();
    ARM_MPU_SetMemAttr(1U,
                       ARM_MPU_ATTR(ARM_MPU_ATTR_NON_CACHEABLE,
                                    ARM_MPU_ATTR_NON_CACHEABLE));
    ARM_MPU_SetRegion(0U,
                      ARM_MPU_RBAR(SYSTEM_PSRAM_BASE_ADDR,
                                   ARM_MPU_SH_NON,
                                   0U,
                                   1U,
                                   1U),
                      ARM_MPU_RLAR(SYSTEM_PSRAM_LIMIT_ADDR, 1U));
    ARM_MPU_Enable(MPU_CTRL_PRIVDEFENA_Msk);
#endif
}
#endif

/**
 * Strong override of weak SystemInit_ExtMemCtl() in system_acm32p4xx.c.
 * Called from SystemInit() when DATA_IN_ExtSRAM is defined.
 *
 * Only MPU (optional) here: full PSRAM init is done after SystemClock_Config()
 * via system_ospi_psram_reclock() so BAUD/SSHIFT match final HCLK.
 */
void SystemInit_ExtMemCtl(void)
{
#if SYSTEM_MPU_OSPI_PSRAM
    system_mpu_ospi_psram();
#endif
#if SYSTEM_INIT_OSPI_PSRAM
    /* GPIO+clock early so pad state is known; controller fully configured later */
    psram_init_gpio_and_reset();
#endif
}

#endif /* DATA_IN_ExtSRAM */
