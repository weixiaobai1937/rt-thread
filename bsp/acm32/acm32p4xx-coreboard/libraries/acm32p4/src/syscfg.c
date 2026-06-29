/**
 * @file syscfg.c
 * @brief ACM32P4 系统配置控制器 (SYSCFG) 驱动实现
 *
 * 本文件实现了 ACM32P4 芯片的系统配置功能：
 * - 芯片版本信息查询
 * - 启动模式和启动设备检测
 * - 以太网 PHY 接口配置
 * - 红外 (IR) 配置
 * - BOOT0 引脚上拉控制
 * - OSPI2 地址重映射
 * - BKPSRAM ECC 错误检测和中断管理
 * - 安全功能连接（LVD / LOCKUP / XTHSD / ECC → 定时器 Break）
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#include "hardware/syscfg.h"
#include "acm32p4xx.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>

//===========================================
// 内部状态
//===========================================

static ecc_callback_t ecc_callback = ((void *)0);  ///< ECC 错误回调函数

//===========================================
// 第1层：快速查询 API
//===========================================

void syscfg_get_chip_version(chip_version_t *version)
{
    assert(version != ((void *)0));

    uint32_t ver = SYSCFG->VER;

    version->raw_version  = ver;
    version->chip_version = (uint8_t)((ver >> 0) & 0x0FU);   // bit[3:0]
    version->chip_series  = (uint8_t)((ver >> 4) & 0x0FU);   // bit[7:4]
    version->product_type = (uint8_t)((ver >> 8) & 0xFFU);   // bit[15:8]
}

boot_mode_t syscfg_get_boot_mode(void)
{
    return (SYSCFG->WMR & (1U << 3)) ? BOOT_MODE_ROM : BOOT_MODE_FLASH;
}

boot_device_t syscfg_get_boot_device(void)
{
    return (boot_device_t)((SYSCFG->WMR >> 8) & 0x03U);
}

bool syscfg_is_rtc_ready(void)
{
    return (SYSCFG->WMR & (1U << 6)) != 0;
}

//===========================================
// 第2层：基础配置 API
//===========================================

void syscfg_eth_set_phy_interface(eth_phy_interface_t interface)
{
    uint32_t syscr = SYSCFG->SYSCR;

    syscr &= ~(0x07U << 13);              // 清除 EPIS bit[15:13]
    syscr |= ((uint32_t)interface << 13);
    SYSCFG->SYSCR = syscr;
}

void syscfg_eth_set_rx_clock_source(bool use_phy_rx_clk)
{
    if (use_phy_rx_clk) {
        SYSCFG->SYSCR |= (1U << 20);
    } else {
        SYSCFG->SYSCR &= ~(1U << 20);
    }
}

void syscfg_eth_set_tx_clock_gating(bool disable)
{
    if (disable) {
        SYSCFG->SYSCR |= (1U << 12);
    } else {
        SYSCFG->SYSCR &= ~(1U << 12);
    }
}

void syscfg_eth_set_flow_control(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 22);
    } else {
        SYSCFG->SYSCR &= ~(1U << 22);
    }
}

void syscfg_eth_set_fifo_flow_control(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 21);
    } else {
        SYSCFG->SYSCR &= ~(1U << 21);
    }
}

void syscfg_boot0_pullup_disable(bool disable)
{
    if (disable) {
        SYSCFG->SYSCR |= (1U << 19);    // BOOT_PUN = 1：禁止上拉
    } else {
        SYSCFG->SYSCR &= ~(1U << 19);   // BOOT_PUN = 0：使能上拉
    }
}

void syscfg_ir_set_clock_source(ir_clock_source_t source)
{
    if (source == IR_CLK_MCO) {
        SYSCFG->SYSCR |= (1U << 28);
    } else {
        SYSCFG->SYSCR &= ~(1U << 28);
    }
}

void syscfg_ir_set_data_source(ir_data_source_t source)
{
    uint32_t syscr = SYSCFG->SYSCR;

    syscr &= ~(0x07U << 25);              // 清除 IR_DATA_SEL bit[27:25]
    syscr |= ((uint32_t)source << 25);
    SYSCFG->SYSCR = syscr;
}

//===========================================
// 第3层：高级功能 API
//===========================================

void syscfg_ir_set_polarity(bool invert)
{
    if (invert) {
        SYSCFG->SYSCR |= (1U << 24);
    } else {
        SYSCFG->SYSCR &= ~(1U << 24);
    }
}

void syscfg_ospi2_set_remap(bool remap)
{
    if (remap) {
        SYSCFG->OSPI2REMAP = 0xAEC59FD3U;  // 重映射到 0x0800_0000
    } else {
        SYSCFG->OSPI2REMAP = 0x00000000U;  // 默认地址 0x1000_0000
    }
}

//===========================================
// 第4层：控制与查询 API
//===========================================

void syscfg_ecc_single_bit_irq_enable(void)
{
    SYSCFG->RAMECCIR |= (1U << 8);   // BKPSRAM_SEC_IE
}

void syscfg_ecc_single_bit_irq_disable(void)
{
    SYSCFG->RAMECCIR &= ~(1U << 8);
}

void syscfg_ecc_double_bit_irq_enable(void)
{
    SYSCFG->RAMECCIR |= (1U << 9);   // BKPSRAM_DED_IE
}

void syscfg_ecc_double_bit_irq_disable(void)
{
    SYSCFG->RAMECCIR &= ~(1U << 9);
}

bool syscfg_ecc_has_single_bit_error(void)
{
    return (SYSCFG->RAMECCSR & (1U << 8)) != 0;
}

bool syscfg_ecc_has_double_bit_error(void)
{
    return (SYSCFG->RAMECCSR & (1U << 9)) != 0;
}

void syscfg_ecc_clear_single_bit_flag(void)
{
    SYSCFG->RAMECCICR = (1U << 8);   // BKPSRAM_SEC_IC
}

void syscfg_ecc_clear_double_bit_flag(void)
{
    SYSCFG->RAMECCICR = (1U << 9);   // BKPSRAM_DED_IC
}

void syscfg_ecc_set_callback(ecc_callback_t callback)
{
    ecc_callback = callback;
}

//===========================================
// 安全功能连接 API
//===========================================

void syscfg_lvd_connect_to_timer_break(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 2);    // LVD_LOCK
    } else {
        SYSCFG->SYSCR &= ~(1U << 2);
    }
}

void syscfg_lockup_connect_to_timer_break(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 0);    // LOCKUP_LOCK
    } else {
        SYSCFG->SYSCR &= ~(1U << 0);
    }
}

void syscfg_xthsd_connect_to_timer_break(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 1);    // XTHSD_LOCK
    } else {
        SYSCFG->SYSCR &= ~(1U << 1);
    }
}

void syscfg_ecc_connect_to_timer_break(bool enable)
{
    if (enable) {
        SYSCFG->SYSCR |= (1U << 11);   // BKPSRAM_LOCK
    } else {
        SYSCFG->SYSCR &= ~(1U << 11);
    }
}

//===========================================
// ECC 中断处理函数
//===========================================

void BKPSRAM_SEC_IRQHandler(void)
{
    if (syscfg_ecc_has_single_bit_error()) {
        syscfg_ecc_clear_single_bit_flag();

        if (ecc_callback) {
            ecc_error_event_t event = {
                .single_bit = true,
                .double_bit = false
            };
            ecc_callback(&event);
        }
    }
}

void BKPSRAM_DED_IRQHandler(void)
{
    if (syscfg_ecc_has_double_bit_error()) {
        syscfg_ecc_clear_double_bit_flag();

        if (ecc_callback) {
            ecc_error_event_t event = {
                .single_bit = false,
                .double_bit = true
            };
            ecc_callback(&event);
        }
    }
}
