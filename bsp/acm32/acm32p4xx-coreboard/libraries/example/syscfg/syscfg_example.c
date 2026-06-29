/**
 * @file syscfg_example.c
 * @brief SYSCFG 模块功能示例程序
 *
 * @details
 * 演示 8 个场景：
 * 1. 系统信息查询（芯片版本、启动模式、启动设备、RTC状态）
 * 2. BKPSRAM ECC 监控（中断配置、状态查询、回调注册）
 * 3. 安全功能连接（LVD/LOCKUP/XTHSD/ECC → TIM Break）
 * 4. 以太网 PHY 配置（PHY 接口、RX 时钟、TX 门控、流控）
 * 5. IR 红外配置（时钟源、数据源、极性）
 * 6. BOOT0 引脚上拉控制
 * 7. OSPI2 地址重映射
 * 8. USBPHY 配置查询
 */

#include "acm32p4.h"
#include "SEGGER_RTT.h"

//===========================================
// 配置选项
//===========================================

#define printf(...) SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO 1   // 修改此值选择运行的场景

//===========================================
// 全局变量
//===========================================

static volatile bool ecc_event_occurred = false;

//===========================================
// 辅助函数
//===========================================

void print_separator(void)
{
    printf("========================================\n");
}

//===========================================
// 场景1：系统信息查询
//===========================================

void scenario_1_system_info(void)
{
    print_separator();
    printf("=== 场景1：系统信息查询 ===\n\n");

    printf("步骤1：查询芯片版本\n");
    chip_version_t ver;
    syscfg_get_chip_version(&ver);
    printf("  结果：\n");
    printf("    产品类型：0x%02X ", ver.product_type);
    if (ver.product_type == 0x01) {
        printf("(MCU)\n");
    } else {
        printf("(未知)\n");
    }
    printf("    芯片系列：%d\n", ver.chip_series);
    printf("    芯片版本：V%d\n", ver.chip_version);
    printf("    原始值：0x%08lX\n", (unsigned long)ver.raw_version);

    printf("\n步骤2：查询启动模式\n");
    boot_mode_t mode = syscfg_get_boot_mode();
    printf("  结果：%s\n", mode == BOOT_MODE_FLASH ? "从 SPI-FLASH 启动" : "从 ROM Bootloader 启动");

    printf("\n步骤3：查询启动设备\n");
    boot_device_t dev = syscfg_get_boot_device();
    printf("  结果：");
    switch (dev) {
        case BOOT_DEV_OSPI2_1: printf("OSPI2-1 NOR Flash\n");  break;
        case BOOT_DEV_OSPI2_0: printf("OSPI2-0 NOR Flash\n");  break;
        case BOOT_DEV_QSPI7_1: printf("(Q)SPI7-1 NOR Flash\n"); break;
        case BOOT_DEV_QSPI7_0: printf("(Q)SPI7-0 NOR Flash\n"); break;
        default:               printf("未知\n");                break;
    }

    printf("\n步骤4：查询 RTC 域状态\n");
    printf("  结果：%s\n", syscfg_is_rtc_ready() ? "已就绪" : "未就绪");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景2：BKPSRAM ECC 监控
//===========================================

void ecc_event_handler(const ecc_error_event_t *event)
{
    ecc_event_occurred = true;

    if (event->single_bit) {
        printf("\n  [ECC] 单 bit 错误已自动纠正\n");
    }
    if (event->double_bit) {
        printf("\n  [ECC] 严重错误：双 bit 错误，数据已损坏！\n");
    }
}

void scenario_2_ecc_monitor(void)
{
    print_separator();
    printf("=== 场景2：BKPSRAM ECC 监控 ===\n\n");

    printf("步骤1：注册 ECC 回调函数\n");
    syscfg_ecc_set_callback(ecc_event_handler);
    printf("  结果：回调函数已注册\n");

    printf("\n步骤2：使能 BKPSRAM ECC 中断\n");
    syscfg_ecc_single_bit_irq_enable();
    syscfg_ecc_double_bit_irq_enable();
    NVIC_EnableIRQ(BKPSRAM_SEC_IRQn);
    NVIC_EnableIRQ(BKPSRAM_DED_IRQn);
    NVIC_SetPriority(BKPSRAM_SEC_IRQn, 3);
    NVIC_SetPriority(BKPSRAM_DED_IRQn, 2);
    printf("  结果：\n");
    printf("    BKPSRAM_SEC_IRQn (66) - 已使能，优先级 3\n");
    printf("    BKPSRAM_DED_IRQn (67) - 已使能，优先级 2\n");

    printf("\n步骤3：查询当前 ECC 状态\n");
    printf("  结果：\n");
    printf("    单 bit 错误：%s\n", syscfg_ecc_has_single_bit_error() ? "有错误" : "正常");
    printf("    双 bit 错误：%s\n", syscfg_ecc_has_double_bit_error() ? "有错误" : "正常");

    printf("\n步骤4：轮询等待 ECC 事件（演示模式）\n");
    printf("  注意：实际 ECC 错误需要硬件故障才会触发\n");
    printf("  正在监控中...\n");

    uint32_t count = 0;
    while (count < 3) {
        if (syscfg_ecc_has_single_bit_error()) {
            printf("\n  ! 检测到单 bit 错误\n");
            syscfg_ecc_clear_single_bit_flag();
        }
        if (syscfg_ecc_has_double_bit_error()) {
            printf("\n  !! 检测到双 bit 错误\n");
            syscfg_ecc_clear_double_bit_flag();
        }
        delay_ms(1000);
        count++;
    }

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景3：安全功能连接
//===========================================

void scenario_3_safety_features(void)
{
    print_separator();
    printf("=== 场景3：安全功能连接 ===\n\n");

    printf("此场景演示安全关键应用的配置\n\n");

    printf("步骤1：连接 LVD 检测到定时器 Break\n");
    syscfg_lvd_connect_to_timer_break(true);
    printf("  → LVD 欠压时自动停止 TIM1/8 PWM 输出\n\n");

    printf("步骤2：连接 Core LOCKUP 到定时器 Break\n");
    syscfg_lockup_connect_to_timer_break(true);
    printf("  → CPU 死锁时自动停止 TIM1/8 PWM 输出\n\n");

    printf("步骤3：连接 XTH 停振检测到定时器 Break\n");
    syscfg_xthsd_connect_to_timer_break(true);
    printf("  → 高速晶振停振时自动停止 TIM1/8 PWM 输出\n\n");

    printf("步骤4：连接 BKPSRAM ECC 双 bit 错误到定时器 Break\n");
    syscfg_ecc_connect_to_timer_break(true);
    printf("  → BKPSRAM 数据损坏时自动停止 TIM1/8 PWM 输出\n\n");

    printf("已配置 4 条安全连接，TIM1/8 在任何异常时都会自动停止输出\n\n");

    delay_ms(2000);

    printf("步骤5：断开所有连接\n");
    syscfg_lvd_connect_to_timer_break(false);
    syscfg_lockup_connect_to_timer_break(false);
    syscfg_xthsd_connect_to_timer_break(false);
    syscfg_ecc_connect_to_timer_break(false);
    printf("  结果：所有安全连接已断开\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景4：以太网 PHY 配置
//===========================================

void scenario_4_eth_phy_config(void)
{
    print_separator();
    printf("=== 场景4：以太网 PHY 配置 ===\n\n");

    printf("步骤1：配置 PHY 接口为 RMII\n");
    syscfg_eth_set_phy_interface(ETH_PHY_RMII);
    printf("  结果：PHY 接口 = RMII\n");

    printf("\n步骤2：配置 RX 时钟源为 PHY RX 时钟\n");
    syscfg_eth_set_rx_clock_source(true);
    printf("  结果：RX 时钟源 = PHY RX CLK\n");

    printf("\n步骤3：禁止 TX 时钟门控（始终提供时钟）\n");
    syscfg_eth_set_tx_clock_gating(true);
    printf("  结果：TX_CLK 门控 = 禁止\n");

    printf("\n步骤4：使能软件流控\n");
    syscfg_eth_set_flow_control(true);
    printf("  结果：流控 = 使能\n");

    printf("\n步骤5：使能 FIFO 满自动流控\n");
    syscfg_eth_set_fifo_flow_control(true);
    printf("  结果：FIFO 满自动流控 = 使能\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景5：IR 红外配置
//===========================================

void scenario_5_ir_config(void)
{
    print_separator();
    printf("=== 场景5：IR 红外配置 ===\n\n");

    printf("步骤1：选择 IR 时钟源为 TIM10_CH1\n");
    syscfg_ir_set_clock_source(IR_CLK_TIM10_CH1);
    printf("  结果：IR_CLK = TIM10_CH1\n");

    printf("\n步骤2：选择 IR 数据源为 UART1_TXD\n");
    syscfg_ir_set_data_source(IR_DATA_UART1_TXD);
    printf("  结果：IR_DATA = UART1_TXD\n");

    printf("\n步骤3：设置 IR 输出极性为取反\n");
    syscfg_ir_set_polarity(true);
    printf("  结果：IR_OUT = ~(IR_CLK & IR_DATA)\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景6：BOOT0 引脚上拉控制
//===========================================

void scenario_6_boot_config(void)
{
    print_separator();
    printf("=== 场景6：BOOT0 引脚上拉控制 ===\n\n");

    printf("步骤1：使能 BOOT0 上拉（默认从 Flash 启动）\n");
    syscfg_boot0_pullup_disable(false);
    printf("  结果：BOOT_PUN = 0（上拉使能）\n");

    delay_ms(1000);

    printf("\n步骤2：禁止 BOOT0 上拉\n");
    syscfg_boot0_pullup_disable(true);
    printf("  结果：BOOT_PUN = 1（上拉禁止）\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景7：OSPI2 地址重映射
//===========================================

void scenario_7_ospi2_remap(void)
{
    print_separator();
    printf("=== 场景7：OSPI2 地址重映射 ===\n\n");

    printf("步骤1：查询当前映射\n");
    printf("  默认地址：0x1000_0000 ~ 0x17FF_FFFF\n\n");

    printf("步骤2：使能重映射（映射到代码区）\n");
    syscfg_ospi2_set_remap(true);
    printf("  → OSPI2 现在映射到 0x0800_0000 ~ 0x0FFF_FFFF\n");
    printf("  → OSPI2 可作为启动设备使用\n");

    delay_ms(1000);

    printf("\n步骤3：恢复默认映射\n");
    syscfg_ospi2_set_remap(false);
    printf("  → OSPI2 恢复到 0x1000_0000 ~ 0x17FF_FFFF\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 场景8：USBPHY 配置查询
//===========================================

void scenario_8_usb_phy_query(void)
{
    print_separator();
    printf("=== 场景8：USBPHY 配置查询 ===\n\n");

    printf("通过直接读取 PHYCFG 寄存器查询 USBPHY 状态\n\n");

    uint32_t phycfg = SYSCFG->PHYCFG;

    printf("PHYCFG 寄存器值：0x%08lX\n\n", (unsigned long)phycfg);

    printf("状态位：\n");
    printf("  PHY_RSTN          : %lu (0=复位中, 1=正常)\n",
           (unsigned long)(phycfg & 1U));
    printf("  FPLL_H60M_L48M    : %lu (0=48M, 1=60M)\n",
           (unsigned long)((phycfg >> 1) & 1U));
    printf("  PD_PLL            : %lu (0=正常工作, 1=PowerDown)\n",
           (unsigned long)((phycfg >> 2) & 1U));
    printf("  OSC_MODE          : %lu\n",
           (unsigned long)((phycfg >> 3) & 3U));
    printf("  DMPD (DM下拉电阻) : %lu\n",
           (unsigned long)((phycfg >> 6) & 1U));
    printf("  DUPD (DP下拉电阻) : %lu\n",
           (unsigned long)((phycfg >> 7) & 1U));
    printf("  CLKSEL_LRC_HXO    : %lu (0=内部RC, 1=外部晶振)\n",
           (unsigned long)((phycfg >> 16) & 1U));
    printf("  PLL_LOCK          : %lu (0=未锁定, 1=已锁定)\n",
           (unsigned long)((phycfg >> 17) & 1U));
    printf("  ADJ_END           : %lu (0=校准中, 1=校准完成)\n",
           (unsigned long)((phycfg >> 18) & 1U));
    printf("  CLKSEL_END        : %lu (0=查询中, 1=查询完成)\n",
           (unsigned long)((phycfg >> 19) & 1U));
    printf("  USB_EN            : 0x%02lX\n",
           (unsigned long)((phycfg >> 24) & 0xFFU));

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

    printf("\n\n");
    print_separator();
    printf("  ACM32P4 SYSCFG 示例程序\n");
    printf("  当前场景：%d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO) {
        case 1:
            scenario_1_system_info();
            break;
        case 2:
            scenario_2_ecc_monitor();
            break;
        case 3:
            scenario_3_safety_features();
            break;
        case 4:
            scenario_4_eth_phy_config();
            break;
        case 5:
            scenario_5_ir_config();
            break;
        case 6:
            scenario_6_boot_config();
            break;
        case 7:
            scenario_7_ospi2_remap();
            break;
        case 8:
            scenario_8_usb_phy_query();
            break;
        default:
            printf("错误：无效的场景编号 %d\n", DEFAULT_SCENARIO);
            break;
    }

    printf("\n进入主循环...\n");
    while (1) {
        delay_ms(1000);
    }

    return 0;
}
