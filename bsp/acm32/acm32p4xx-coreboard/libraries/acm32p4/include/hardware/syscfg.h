/**
 * @file syscfg.h
 * @brief ACM32P4 系统配置控制器 (SYSCFG) 驱动
 *
 * @note 功能概览：
 *   - 系统信息查询（芯片版本、启动模式、启动设备、RTC就绪状态）
 *   - 以太网配置（PHY接口、RX时钟源、TX时钟门控、流控）
 *   - 红外 (IR) 配置（时钟源、数据源、极性）
 *   - BOOT0 引脚上拉控制
 *   - OSPI2 地址重映射
 *   - BKPSRAM ECC 错误管理（中断使能、状态查询、标志清除、回调）
 *   - 安全功能连接（LVD / LOCKUP / XTHSD / ECC → 定时器 Break）
 *
 * @author ACM32P4 SDK Team
 * @version 2.0
 */

#ifndef _HARDWARE_SYSCFG_H
#define _HARDWARE_SYSCFG_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

//===========================================
// 枚举类型定义
//===========================================

/**
 * @brief 以太网 PHY 接口类型
 *
 * @note 手册 SYSCR.EPIS：000=MII, 100=RMII
 */
typedef enum {
    ETH_PHY_MII  = 0x00,  ///< MII 接口
    ETH_PHY_RMII = 0x04   ///< RMII 接口
} eth_phy_interface_t;

/**
 * @brief 启动模式
 *
 * @note 手册 WMR.BOOTMODE
 */
typedef enum {
    BOOT_MODE_FLASH = 0,  ///< 从 SPI-FLASH 启动
    BOOT_MODE_ROM   = 1   ///< 从 ROM bootloader 启动
} boot_mode_t;

/**
 * @brief 启动设备（SPI Flash 端口选择）
 *
 * @note 手册 WMR.BOOTDEVICE 表 2-2
 */
typedef enum {
    BOOT_DEV_OSPI2_1 = 0,  ///< OSPI2-1 NOR Flash
    BOOT_DEV_OSPI2_0 = 1,  ///< OSPI2-0 NOR Flash
    BOOT_DEV_QSPI7_1 = 2,  ///< (Q)SPI7-1 NOR Flash
    BOOT_DEV_QSPI7_0 = 3   ///< (Q)SPI7-0 NOR Flash
} boot_device_t;

/**
 * @brief IR 数据源选择
 *
 * @note 手册 SYSCR.IR_DATA_SEL
 */
typedef enum {
    IR_DATA_TIM9_CH1  = 0,  ///< TIM9_CH1
    IR_DATA_UART1_TXD = 1,  ///< UART1_TXD
    IR_DATA_UART2_TXD = 2,  ///< UART2_TXD
    IR_DATA_TIM1_CH1  = 3,  ///< TIM1_CH1
    IR_DATA_LOGIC_1   = 4,  ///< logic 1
    IR_DATA_LOGIC_0   = 5   ///< logic 0
} ir_data_source_t;

/**
 * @brief IR 时钟源选择
 *
 * @note 手册 SYSCR.IR_CLK_SEL
 */
typedef enum {
    IR_CLK_TIM10_CH1 = 0,  ///< 来自 TIM10_CH1
    IR_CLK_MCO       = 1   ///< 来自 MCO
} ir_clock_source_t;

//===========================================
// 结构体类型定义
//===========================================

/**
 * @brief 芯片版本信息
 */
typedef struct {
    uint8_t  product_type;   ///< 产品类型（0x01=MCU）
    uint8_t  chip_series;    ///< 芯片系列
    uint8_t  chip_version;   ///< 芯片版本
    uint32_t raw_version;    ///< 原始版本寄存器值
} chip_version_t;

/**
 * @brief ECC 错误事件信息（仅 BKPSRAM）
 */
typedef struct {
    bool single_bit;  ///< 单 bit 错误（已纠正）
    bool double_bit;  ///< 双 bit 错误（无法纠正）
} ecc_error_event_t;

/**
 * @brief ECC 错误回调函数类型
 *
 * @param event 错误事件信息
 */
typedef void (*ecc_callback_t)(const ecc_error_event_t *event);

//===========================================
// 第1层：快速查询 API
//===========================================

void syscfg_get_chip_version(chip_version_t *version);
boot_mode_t syscfg_get_boot_mode(void);
boot_device_t syscfg_get_boot_device(void);
bool syscfg_is_rtc_ready(void);

//===========================================
// 第2层：基础配置 API
//===========================================

void syscfg_eth_set_phy_interface(eth_phy_interface_t interface);
void syscfg_eth_set_rx_clock_source(bool use_phy_rx_clk);
void syscfg_eth_set_tx_clock_gating(bool disable);
void syscfg_eth_set_flow_control(bool enable);
void syscfg_eth_set_fifo_flow_control(bool enable);
void syscfg_boot0_pullup_disable(bool disable);
void syscfg_ir_set_clock_source(ir_clock_source_t source);
void syscfg_ir_set_data_source(ir_data_source_t source);

//===========================================
// 第3层：高级功能 API
//===========================================

void syscfg_ir_set_polarity(bool invert);
void syscfg_ospi2_set_remap(bool remap);

//===========================================
// 第4层：控制与查询 API
//===========================================

void syscfg_ecc_single_bit_irq_enable(void);
void syscfg_ecc_single_bit_irq_disable(void);
void syscfg_ecc_double_bit_irq_enable(void);
void syscfg_ecc_double_bit_irq_disable(void);
bool syscfg_ecc_has_single_bit_error(void);
bool syscfg_ecc_has_double_bit_error(void);
void syscfg_ecc_clear_single_bit_flag(void);
void syscfg_ecc_clear_double_bit_flag(void);
void syscfg_ecc_set_callback(ecc_callback_t callback);
void syscfg_lvd_connect_to_timer_break(bool enable);
void syscfg_lockup_connect_to_timer_break(bool enable);
void syscfg_xthsd_connect_to_timer_break(bool enable);
void syscfg_ecc_connect_to_timer_break(bool enable);

#ifdef __cplusplus
}
#endif

#endif // _HARDWARE_SYSCFG_H
