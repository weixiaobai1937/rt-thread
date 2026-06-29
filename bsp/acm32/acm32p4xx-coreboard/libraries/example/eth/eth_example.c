/**
 * @file eth_example.c
 * @brief ACM32P4 ETH 驱动功能示例程序
 *
 * 演示16个场景：
 *  1.  快速初始化 + 轮询单帧发送
 *  2.  PHY 配置与链路状态读取
 *  3.  完整 MAC 配置结构体
 *  4.  完整 DMA 配置结构体
 *  5.  帧过滤 + 多 MAC 地址配置
 *  6.  流量控制（PAUSE 帧）配置
 *  7.  VLAN 过滤配置
 *  8.  L3/L4 过滤配置
 *  9.  多 MAC 地址注册（index 0~3）
 * 10.  MMC 硬件统计计数读取
 * 11.  PTP 时间戳基本配置与时间读写
 * 12.  PTP 频率微调与目标触发时间
 * 13.  LPI / EEE（802.3az）节能配置
 * 14.  PMT 唤醒帧配置（Magic Packet + 唤醒帧过滤）
 * 15.  DMA 中断驱动收发（ETH_IRQHandler 回调）
 * 16.  调试信息与 DMA 状态查询
 *
 * @note 硬件连接（RMII 模式，全部 ETH 引脚使用 AF6）：
 *   Pin   Signal            AF   说明
 *   PC0   ETH_RST          GPIO  LAN8720A 硬件复位（低有效，输出）
 *   PA1   ETH_RMII_REF_CLK  6   50 MHz REF_CLK（PHY → MCU）
 *   PA2   ETH_MDIO          6   管理数据 IO（需板级 10 kΩ 上拉）
 *   PA7   ETH_RMII_CRS_DV   6
 *   PB11  ETH_RMII_TX_EN    6
 *   PB12  ETH_RMII_TXD0     6
 *   PB13  ETH_RMII_TXD1     6
 *   PC1   ETH_MDC           6   管理时钟（MCU → PHY，≤2.5 MHz）
 *   PC4   ETH_RMII_RXD0     6
 *   PC5   ETH_RMII_RXD1     6
 *
 * @note PHY 芯片：SMSC LAN8720A（仅支持 RMII，内置 50 MHz 振荡器可驱动 REF_CLK）
 *   - PHYAD0 引脚接 GND → PHY 地址 0x00
 *   - PHYAD0 引脚接 VDD  → PHY 地址 0x01
 *   - PHY ID1 = 0x0007，ID2 = 0xC0Fx（Microchip/SMSC OUI）
 *   - 上电或硬件复位后需等待 ≥ 300 ms 再访问 SMI
 *   可修改 PHY_ADDR 宏匹配实际电路
 *
 * @author ACM32P4 SDK Team
 * @version 1.0
 */

#include "acm32p4.h"
#include "hardware/eth.h"
#include "hardware/syscfg.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/system.h"
#include "SEGGER_RTT.h"
#include <string.h>
#include <stddef.h>

//===========================================
// 配置选项
//===========================================

#define printf(...)             SEGGER_RTT_printf(0, __VA_ARGS__)
#define DEFAULT_SCENARIO        2     ///< 修改此值（1-16）选择要运行的场景

/** LAN8720A PHY 7 位地址（PHYAD0 接 GND=0x00，接 VDD=0x01）*/
#define PHY_ADDR                0x00U

/** IEEE 802.3 标准寄存器地址 */
#define PHY_REG_BCR             0x00U  ///< 基本控制寄存器（Basic Control）
#define PHY_REG_BSR             0x01U  ///< 基本状态寄存器（Basic Status）
#define PHY_REG_ID1             0x02U  ///< PHY 标识符 1（OUI 高位）
#define PHY_REG_ID2             0x03U  ///< PHY 标识符 2（OUI 低位 + 型号）
#define PHY_REG_ANAR            0x04U  ///< 自动协商通告寄存器
#define PHY_REG_ANLPAR          0x05U  ///< 自动协商对端能力寄存器

/** LAN8720A 专有寄存器地址 */
#define PHY_REG_SCSR            0x1FU  ///< 特殊控制/状态（速度/双工实际指示）
#define PHY_REG_ISFR            0x1DU  ///< 中断源标志寄存器
#define PHY_REG_IMR             0x1EU  ///< 中断屏蔽寄存器

/** LAN8720A 芯片 ID（寄存器2/3期望值）*/
#define LAN8720_PHY_ID1         0x0007U  ///< ID1 固定为 0x0007
#define LAN8720_PHY_ID2_MASK    0xFFF0U  ///< ID2 高12位为型号（0xC0F0）
#define LAN8720_PHY_ID2_VAL     0xC0F0U  ///< LAN8720A 型号标识

/** PHY BCR（Reg 0）位定义 */
#define PHY_BCR_RESET           (1U << 15)  ///< 软件复位（自清，复位完成后变0）
#define PHY_BCR_LOOPBACK        (1U << 14)  ///< 内部回环测试
#define PHY_BCR_SPEED_100       (1U << 13)  ///< 速度选择：1=100Mbps，0=10Mbps
#define PHY_BCR_AN_EN           (1U << 12)  ///< 自动协商使能
#define PHY_BCR_POWER_DOWN      (1U << 11)  ///< 省电模式
#define PHY_BCR_ISOLATE         (1U << 10)  ///< 电气隔离 MII 接口
#define PHY_BCR_AN_RESTART      (1U << 9)   ///< 重启自动协商
#define PHY_BCR_FULL_DUPLEX     (1U << 8)   ///< 全双工（AN 关闭时有效）

/** PHY BSR（Reg 1）位定义 */
#define PHY_BSR_100BASE_TX_FD   (1U << 14)  ///< 支持 100BASE-TX 全双工
#define PHY_BSR_100BASE_TX_HD   (1U << 13)  ///< 支持 100BASE-TX 半双工
#define PHY_BSR_10BASE_T_FD     (1U << 12)  ///< 支持 10BASE-T 全双工
#define PHY_BSR_10BASE_T_HD     (1U << 11)  ///< 支持 10BASE-T 半双工
#define PHY_BSR_AN_COMPLETE     (1U << 5)   ///< 自动协商完成
#define PHY_BSR_REMOTE_FAULT    (1U << 4)   ///< 远端故障
#define PHY_BSR_LINK_UP         (1U << 2)   ///< 链路建立

/** LAN8720A SCSR（Reg 31）速度/双工指示位 */
#define PHY_SCSR_SPEED_MASK     (0x7U << 2) ///< bit[4:2] 速度+双工指示
#define PHY_SCSR_10HD           (0x1U << 2) ///< 10 Mbps 半双工
#define PHY_SCSR_100HD          (0x2U << 2) ///< 100 Mbps 半双工
#define PHY_SCSR_10FD           (0x5U << 2) ///< 10 Mbps 全双工
#define PHY_SCSR_100FD          (0x6U << 2) ///< 100 Mbps 全双工

/** TX/RX 描述符数量 */
#define TX_DESC_COUNT           4U
#define RX_DESC_COUNT           4U
/** 每个描述符缓冲区最大帧字节数（不含 CRC） */
#define ETH_FRAME_SIZE          1514U

/** 场景 1 使用的测试帧目的地址（广播） */
#define DEST_MAC_BROADCAST      { 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU }
/** 本机 MAC 地址 */
#define LOCAL_MAC_ADDR          { 0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U }

//===========================================
// RMII 引脚定义（引脚/AF 编号以 BSP 为准）
//===========================================
/** PHY 硬件复位引脚（低有效，GPIO 普通输出） */
#define ETH_RST_PIN             PC0
/** RMII 功能引脚：引脚名 + 对应 AF 编号（全部使用 AF6） */
#define ETH_REF_CLK_PIN         PA1
#define ETH_REF_CLK_AF          GPIO_AF_6
#define ETH_MDIO_PIN            PA2
#define ETH_MDIO_AF             GPIO_AF_6
#define ETH_CRS_DV_PIN          PA7
#define ETH_CRS_DV_AF           GPIO_AF_6
#define ETH_MDC_PIN             PC1
#define ETH_MDC_AF              GPIO_AF_6
#define ETH_RXD0_PIN            PC4
#define ETH_RXD0_AF             GPIO_AF_6
#define ETH_RXD1_PIN            PC5
#define ETH_RXD1_AF             GPIO_AF_6
#define ETH_TX_EN_PIN           PB11
#define ETH_TX_EN_AF            GPIO_AF_6
#define ETH_TXD0_PIN            PB12
#define ETH_TXD0_AF             GPIO_AF_6
#define ETH_TXD1_PIN            PB13
#define ETH_TXD1_AF             GPIO_AF_6

/** SMI 时钟档位选择（HCLK=180 MHz → MDC=HCLK/102≈1.76 MHz，满足 LAN8720A ≤2.5 MHz 要求） */
#define ETH_SMI_CLK             ETH_SMI_CR_150_250MHZ

/** DMA 描述符控制位（与 eth.c 内部定义一致） */
#define TDES0_OWN               (1UL << 31)  ///< DMA 拥有权位
#define TDES0_TER               (1UL << 21)  ///< 发送环结束
#define TDES0_FS                (1UL << 28)  ///< 第一段
#define TDES0_LS                (1UL << 29)  ///< 最后段
#define TDES0_IC                (1UL << 30)  ///< 完成中断
#define RDES0_OWN               (1UL << 31)  ///< DMA 拥有权位
#define RDES1_RER               (1UL << 15)  ///< 接收环结束（注意：bit15，非 bit25）

//===========================================
// 全局描述符与缓冲区
//===========================================

/** TX 描述符数组（4 字节对齐） */
static eth_tx_desc_t s_tx_desc[TX_DESC_COUNT] __attribute__((aligned(4)));
/** RX 描述符数组（4 字节对齐） */
static eth_rx_desc_t s_rx_desc[RX_DESC_COUNT] __attribute__((aligned(4)));
/** TX 帧缓冲区 */
static uint8_t s_tx_buf[TX_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(4)));
/** RX 帧缓冲区 */
static uint8_t s_rx_buf[RX_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(4)));
/** 当前 TX 描述符索引 */
static uint8_t s_tx_idx;
/** 当前 RX 描述符索引 */
static uint8_t s_rx_idx;

//===========================================
// 场景 15 中断收发状态
//===========================================

static volatile bool     s_tx_done;     ///< TX 完成标志（场景15）
static volatile bool     s_rx_done;     ///< RX 完成标志（场景15）
static volatile uint32_t s_rx_len;      ///< 最近一帧接收字节数（场景15）

//===========================================
// 辅助函数
//===========================================

static void print_separator(void)
{
    printf("========================================\n");
}

/**
 * @brief 初始化 RMII 模式所需的全部 GPIO 引脚（含 PHY 硬件复位）
 *
 * 各信号引脚 AF 编号不同，需逐一配置。
 * 复位序列：RST 拉低 ≥ 25 ms → 拉高 → 等待 ≥ 300 ms 后 LAN8720A 完成上电初始化。
 */
static void gpio_init_eth_rmii(void)
{
    /* 每个 RMII 信号引脚及其对应 AF 编号（不同引脚 AF 不同，不可混用）*/
    static const struct {
        gpio_pin_t pin;
        uint8_t    af;
    } rmii_af_pins[] = {
        { ETH_REF_CLK_PIN, ETH_REF_CLK_AF },
        { ETH_MDIO_PIN,    ETH_MDIO_AF    },
        { ETH_CRS_DV_PIN,  ETH_CRS_DV_AF  },
        { ETH_MDC_PIN,     ETH_MDC_AF     },
        { ETH_RXD0_PIN,    ETH_RXD0_AF    },
        { ETH_RXD1_PIN,    ETH_RXD1_AF    },
        { ETH_TX_EN_PIN,   ETH_TX_EN_AF   },
        { ETH_TXD0_PIN,    ETH_TXD0_AF    },
        { ETH_TXD1_PIN,    ETH_TXD1_AF    },
    };
    for (uint32_t i = 0U;
         i < (uint32_t)(sizeof(rmii_af_pins) / sizeof(rmii_af_pins[0]));
         i++) {
        gpio_init(rmii_af_pins[i].pin);
        gpio_set_function(rmii_af_pins[i].pin, rmii_af_pins[i].af);
        gpio_set_drive_strength(rmii_af_pins[i].pin, GPIO_DRIVE_16MA);
    }

    /* LAN8720A 硬件复位（PC0，低有效）
     * 拉低 ≥ 25 ms 触发复位，拉高后等待 ≥ 300 ms 完成 PLL 及内部上电时序。 */
    gpio_init(ETH_RST_PIN);
    gpio_set_dir(ETH_RST_PIN, GPIO_OUT);
    gpio_put(ETH_RST_PIN, false);   /* 复位有效（低） */
    delay_ms(25U);
    gpio_put(ETH_RST_PIN, true);    /* 释放复位 */
    delay_ms(300U);                 /* 等待 LAN8720A PLL 锁定 + 内部初始化完成 */
}

/**
 * @brief 使能 ETH 所需时钟并配置 SYSCFG 接口选择
 *
 * @note 顺序约束：
 *   1. 先使能 CLK_SYSCFG，否则写 SYSCFG 寄存器无效
 *   2. syscfg_eth_set_phy_interface 必须在 ETH 时钟使能之前调用
 */
static void eth_clock_init(void)
{
    clock_periph_enable(CLK_SYSCFG);
    syscfg_eth_set_phy_interface(ETH_PHY_RMII);
    clock_periph_enable(CLK_ETH_MAC);
    clock_periph_enable(CLK_ETH_TX);
    clock_periph_enable(CLK_ETH_RX);
}

/**
 * @brief 初始化 TX 描述符链表（Ring 模式：最后一项设 TER）
 *
 * 所有描述符交给 CPU 管理（TDES0.OWN=0），在 eth_transmit_frame 中按帧设置 OWN。
 */
static void desc_init_tx(void)
{
    s_tx_idx = 0U;
    memset(s_tx_desc, 0, sizeof(s_tx_desc));
    for (uint8_t i = 0U; i < TX_DESC_COUNT; i++) {
        s_tx_desc[i].tdes2 = (uint32_t)s_tx_buf[i];
    }
    /* 最后一项置 TER（环形回绕），TDES0 bit21 */
    s_tx_desc[TX_DESC_COUNT - 1U].tdes0 |= TDES0_TER;
}

/**
 * @brief 初始化 RX 描述符链表（Ring 模式：最后一项设 RER），所有描述符交给 DMA（OWN=1）
 */
static void desc_init_rx(void)
{
    s_rx_idx = 0U;
    memset(s_rx_desc, 0, sizeof(s_rx_desc));
    for (uint8_t i = 0U; i < RX_DESC_COUNT; i++) {
        s_rx_desc[i].rdes0 = RDES0_OWN;                    /* 交给 DMA */
        s_rx_desc[i].rdes1 = (uint32_t)ETH_FRAME_SIZE;     /* 缓冲区大小 */
        s_rx_desc[i].rdes2 = (uint32_t)s_rx_buf[i];
    }
    /* 最后一项置 RER（环形回绕），RDES1 bit15 */
    s_rx_desc[RX_DESC_COUNT - 1U].rdes1 |= RDES1_RER;
}

/**
 * @brief 获取下一个可用 TX 描述符索引（CPU 已拥有，即 OWN=0）
 * @return 描述符索引；如果全部被 DMA 占用则返回 TX_DESC_COUNT
 */
static uint8_t get_next_tx_desc(void)
{
    for (uint8_t i = 0U; i < TX_DESC_COUNT; i++) {
        uint8_t idx = (s_tx_idx + i) % TX_DESC_COUNT;
        if (!(s_tx_desc[idx].tdes0 & TDES0_OWN)) {
            s_tx_idx = (uint8_t)((idx + 1U) % TX_DESC_COUNT);
            return idx;
        }
    }
    return TX_DESC_COUNT;
}

/**
 * @brief 获取下一个 DMA 已完成写入的 RX 描述符索引（OWN=0）
 * @return 描述符索引；无新帧则返回 RX_DESC_COUNT
 */
static uint8_t get_next_rx_desc(void)
{
    uint8_t idx = s_rx_idx;
    if (!(s_rx_desc[idx].rdes0 & RDES0_OWN)) {
        s_rx_idx = (uint8_t)((idx + 1U) % RX_DESC_COUNT);
        return idx;
    }
    return RX_DESC_COUNT;
}

/**
 * @brief 等待 PHY 自动协商完成，超时返回 false
 */
static bool phy_wait_an(uint32_t timeout_ms)
{
    uint64_t t0 = system_get_tick();
    uint16_t bsr;
    while (!system_elapsed(t0, timeout_ms)) {
        if (eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr)) {
            if ((bsr & PHY_BSR_AN_COMPLETE) && (bsr & PHY_BSR_LINK_UP)) {
                return true;
            }
        }
        delay_ms(10U);
    }
    return false;
}

//===========================================
// 示例场景
//===========================================

/* ─────────────────────────────────────────────────────────────────────────
 * 场景1：快速初始化 + 轮询单帧发送
 * 演示：eth_init() 一键初始化 → eth_phy_write 触发 PHY 软复位
 *       → eth_start_rx/eth_start_tx → eth_transmit_frame 发送测试帧
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_1_quick_init_poll_tx(void)
{
    print_separator();
    printf("=== 场景1：快速初始化 + 轮询单帧发送 ===\n\n");

    printf("步骤1：初始化 GPIO、时钟、PHY 接口\n");
    gpio_init_eth_rmii();
    eth_clock_init();
		delay_ms(100);

    printf("步骤2：初始化描述符链表\n");
    desc_init_tx();
    desc_init_rx();
		delay_ms(100);
	
    printf("步骤3：调用 eth_init() 快速初始化（RMII，100M，全双工）\n");
    eth_init_config_t cfg = {
        .phy_interface   = ETH_PHY_RMII,
        .smi_clk_range   = ETH_SMI_CLK,
        .speed           = ETH_SPEED_100M,
        .duplex          = ETH_DUPLEX_FULL,
        .checksum_offload= ETH_CHECKSUM_NONE,
        .tx_desc_base    = s_tx_desc,
        .rx_desc_base    = s_rx_desc,
        .tx_desc_count   = TX_DESC_COUNT,
        .rx_desc_count   = RX_DESC_COUNT,
        .enhanced_desc   = false,
    };
    bool ok = eth_init(&cfg);
    printf("  eth_init: %s\n", ok ? "成功" : "失败（复位超时）");
    if (!ok) { print_separator(); return; }
		delay_ms(100);
		
    printf("\n步骤4：LAN8720A 软复位（BCR.Reset，自清位）\n");
    ok = eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_RESET);
    printf("  PHY 软复位写入: %s\n", ok ? "成功" : "失败");
    /* 轮询等待 BCR bit15 自动清零（LAN8720A 典型 < 0.5 ms，保守超时 100 ms）*/
    {
        uint16_t bcr_chk = PHY_BCR_RESET;
        uint64_t t_rst = system_get_tick();
        while ((bcr_chk & PHY_BCR_RESET) && !system_elapsed(t_rst, 100U)) {
            eth_phy_read(PHY_ADDR, PHY_REG_BCR, &bcr_chk);
        }
        printf("  复位完成: %s\n",
               (bcr_chk & PHY_BCR_RESET) ? "超时（检查 MDIO 连接）" : "是");
    }
    ok = eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_AN_EN | PHY_BCR_AN_RESTART);
    printf("  自动协商启动: %s\n", ok ? "成功" : "失败");

    printf("\n步骤5：等待链路建立（最多 3 s）\n");
    bool link_up = phy_wait_an(3000U);
    printf("  链路状态: %s\n", link_up ? "已建立" : "未建立（无 PHY 或超时）");

    printf("\n步骤6：配置本机 MAC 地址 0（00:11:22:33:44:55）\n");
    uint8_t local_mac[] = LOCAL_MAC_ADDR;
    eth_mac_addr_config_t addr0 = { .enable = true };
    memcpy(addr0.addr, local_mac, 6U);
    eth_config_mac_addr(0U, &addr0);

    printf("\n步骤7：使能 MAC TX/RX，启动 DMA 引擎\n");
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();

    printf("\n步骤8：构造最小以太网帧并发送（目的地址=广播，类型=0x0800，数据=0x55×42）\n");
    uint8_t dest_mac[]   = DEST_MAC_BROADCAST;
    uint8_t *frame       = s_tx_buf[0];
    uint32_t frame_len   = 60U;  /* 最小帧：14 字节头 + 46 字节数据 */

    memcpy(frame,     dest_mac,  6U);
    memcpy(frame + 6, local_mac, 6U);
    frame[12] = 0x08U;
    frame[13] = 0x00U;
    memset(frame + 14, 0x55U, 46U);

    uint8_t desc_idx = get_next_tx_desc();
    if (desc_idx < TX_DESC_COUNT) {
        ok = eth_transmit_frame(&s_tx_desc[desc_idx], frame, frame_len);
        printf("  eth_transmit_frame(%u): %s\n", desc_idx, ok ? "成功" : "失败");
    } else {
        printf("  所有 TX 描述符被 DMA 占用\n");
    }

    printf("\n步骤9：轮询接收（最多 100 ms，本场景通常无回环，演示接口用法）\n");
    uint64_t t0 = system_get_tick();
    uint32_t rx_len = 0U;
    uint8_t  rx_frame[128];
    bool     got_frame = false;
    while (!system_elapsed(t0, 100U) && !got_frame) {
        uint8_t ridx = get_next_rx_desc();
        if (ridx < RX_DESC_COUNT) {
            got_frame = eth_receive_frame(&s_rx_desc[ridx], rx_frame, &rx_len);
        }
    }
    if (got_frame) {
        printf("  接收帧: %u 字节，目的地址 %02X:%02X:%02X:%02X:%02X:%02X\n",
               rx_len,
               rx_frame[0], rx_frame[1], rx_frame[2],
               rx_frame[3], rx_frame[4], rx_frame[5]);
    } else {
        printf("  100 ms 内未收到帧（正常，无外部回环）\n");
    }

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景2：PHY 配置与链路状态读取
 * 演示：eth_phy_read/write 读取 PHY ID、BCR/BSR 寄存器，
 *        强制设置 100M 全双工、读回链路速度和双工信息
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_2_phy_config_link_status(void)
{
    print_separator();
    printf("=== 场景2：PHY 配置与链路状态读取 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();

    eth_init_config_t cfg = {
        .phy_interface    = ETH_PHY_RMII,
        .smi_clk_range    = ETH_SMI_CLK,
        .speed            = ETH_SPEED_100M,
        .duplex           = ETH_DUPLEX_FULL,
        .tx_desc_base     = s_tx_desc,
        .rx_desc_base     = s_rx_desc,
        .tx_desc_count    = TX_DESC_COUNT,
        .rx_desc_count    = RX_DESC_COUNT,
    };
    eth_init(&cfg);

    printf("步骤1：读取 LAN8720A PHY 标识符（ID1 / ID2）\n");
    uint16_t id1 = 0U, id2 = 0U;
    bool ok1 = eth_phy_read(PHY_ADDR, PHY_REG_ID1, &id1);
    bool ok2 = eth_phy_read(PHY_ADDR, PHY_REG_ID2, &id2);
    printf("  PHY 地址 0x%02X: ID1=0x%04X %s, ID2=0x%04X %s\n",
           PHY_ADDR,
           id1, ok1 ? "(OK)" : "(读取失败)",
           id2, ok2 ? "(OK)" : "(读取失败)");
    if (ok1 && ok2) {
        bool id_ok = (id1 == LAN8720_PHY_ID1) &&
                     ((id2 & LAN8720_PHY_ID2_MASK) == LAN8720_PHY_ID2_VAL);
        printf("  LAN8720A 识别: %s（期望 ID1=0x0007, ID2=0xC0Fx）\n",
               id_ok ? "通过" : "不匹配（请检查 PHY_ADDR 或硬件连接）");
    }

    printf("\n步骤2：读取 BCR（基本控制寄存器）\n");
    uint16_t bcr = 0U;
    eth_phy_read(PHY_ADDR, PHY_REG_BCR, &bcr);
    printf("  BCR=0x%04X  AN_EN=%d  SPEED_SEL=%d  FULL_DPX=%d  ISOLATE=%d  PWR_DOWN=%d\n",
           bcr,
           (int)((bcr >> 12) & 1U),
           (int)((bcr >> 13) & 1U),
           (int)((bcr >>  8) & 1U),
           (int)((bcr >> 10) & 1U),
           (int)((bcr >> 11) & 1U));

    printf("\n步骤3：读取 BSR（基本状态寄存器）\n");
    uint16_t bsr = 0U;
    eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr);
    printf("  BSR=0x%04X  LINK_UP=%d  AN_COMPLETE=%d  100TX_FD=%d  100TX_HD=%d  10T_FD=%d  10T_HD=%d\n",
           bsr,
           (int)((bsr >>  2) & 1U),
           (int)((bsr >>  5) & 1U),
           (int)((bsr >> 14) & 1U),
           (int)((bsr >> 13) & 1U),
           (int)((bsr >> 12) & 1U),
           (int)((bsr >> 11) & 1U));

    printf("\n步骤4：读取 SCSR（Reg 31，LAN8720A 专有）获取实际协商速率和双工\n");
    uint16_t scsr = 0U;
    eth_phy_read(PHY_ADDR, PHY_REG_SCSR, &scsr);
    printf("  SCSR=0x%04X  SPEED_IND=0x%X  ",
           scsr, (unsigned)((scsr & PHY_SCSR_SPEED_MASK) >> 2));
    switch (scsr & PHY_SCSR_SPEED_MASK) {
        case PHY_SCSR_10HD:   printf("10 Mbps 半双工\n");        break;
        case PHY_SCSR_100HD:  printf("100 Mbps 半双工\n");       break;
        case PHY_SCSR_10FD:   printf("10 Mbps 全双工\n");        break;
        case PHY_SCSR_100FD:  printf("100 Mbps 全双工\n");       break;
        default:              printf("（协商中或无物理连接）\n"); break;
    }

    printf("\n步骤5：强制设置 100 Mbps 全双工（禁用自动协商）\n");
    uint16_t bcr_forced = PHY_BCR_SPEED_100 | PHY_BCR_FULL_DUPLEX;
    bool ok = eth_phy_write(PHY_ADDR, PHY_REG_BCR, bcr_forced);
    printf("  写 BCR=0x%04X: %s\n", bcr_forced, ok ? "成功" : "失败");
    delay_ms(100U);   /* 等待 LAN8720A 状态机稳定 */
    eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr);
    printf("  BSR=0x%04X  LINK_UP=%d\n", bsr, (int)((bsr >> 2) & 1U));

    printf("\n步骤6：恢复自动协商，等待 LAN8720A SCSR 显示最终结果\n");
    eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_AN_EN | PHY_BCR_AN_RESTART);
    bool an_done = phy_wait_an(3000U);
    printf("  自动协商: %s\n", an_done ? "完成" : "超时（无物理连接）");
    eth_phy_read(PHY_ADDR, PHY_REG_SCSR, &scsr);
    printf("  协商结果（SCSR）: ");
    switch (scsr & PHY_SCSR_SPEED_MASK) {
        case PHY_SCSR_10HD:   printf("10 Mbps 半双工\n");        break;
        case PHY_SCSR_100HD:  printf("100 Mbps 半双工\n");       break;
        case PHY_SCSR_10FD:   printf("10 Mbps 全双工\n");        break;
        case PHY_SCSR_100FD:  printf("100 Mbps 全双工\n");       break;
        default:              printf("（协商中或无物理连接）\n"); break;
    }

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景3：完整 MAC 配置结构体
 * 演示：eth_config_mac() 精细控制所有 MACCR 字段
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_3_mac_config_struct(void)
{
    print_separator();
    printf("=== 场景3：完整 MAC 配置结构体 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：构建完整 eth_mac_config_t，覆盖所有 MACCR 字段\n");
    eth_mac_config_t mac_cfg = {
        .speed              = ETH_SPEED_100M,
        .duplex             = ETH_DUPLEX_FULL,
        .checksum_offload   = ETH_CHECKSUM_NONE,
        .preamble_length    = ETH_PREAMBLE_7BYTE,   /* 标准 7 字节前导码 */
        .inter_frame_gap    = ETH_IFG_96BIT,        /* 标准 96 bit IFG */
        .backoff_limit      = ETH_BACKOFF_LIMIT_10, /* 最多重传 BL=10（1024个时隙） */
        .sa_insert          = ETH_SA_INSERT_NONE,
        .loopback           = false,
        .jumbo_frame        = false,
        .watchdog_disable   = false,
        .jabber_disable     = false,
        .carrier_sense_disable = false,
        .retry_disable      = false,
        .pad_crc_strip      = true,                 /* 接收时自动去除 PAD/FCS */
        .deferral_check     = false,
        .crc_strip_type2    = false,
        .s2kp_enable        = false,
        .rx_own_disable     = false,
    };

    printf("步骤2：应用 MAC 配置\n");
    eth_config_mac(&mac_cfg);
    printf("  已配置：100M 全双工，7B 前导码，IFG=96bit，CRC 自动剥离\n");

    printf("\n步骤3：切换为 10 Mbps 半双工演示动态修改\n");
    mac_cfg.speed              = ETH_SPEED_10M;
    mac_cfg.duplex             = ETH_DUPLEX_HALF;
    mac_cfg.inter_frame_gap    = ETH_IFG_64BIT;
    eth_config_mac(&mac_cfg);
    printf("  已切换：10M 半双工，IFG=64bit\n");

    printf("\n步骤4：使能 Jumbo Frame（最大 16 KB）\n");
    mac_cfg.speed              = ETH_SPEED_100M;
    mac_cfg.duplex             = ETH_DUPLEX_FULL;
    mac_cfg.inter_frame_gap    = ETH_IFG_96BIT;
    mac_cfg.jumbo_frame        = true;
    mac_cfg.watchdog_disable   = true;   /* Jumbo Frame 时需同时禁用看门狗 */
    mac_cfg.jabber_disable     = true;
    eth_config_mac(&mac_cfg);
    printf("  Jumbo Frame 已使能（需 watchdog_disable + jabber_disable）\n");

    printf("\n步骤5：恢复标准帧配置\n");
    mac_cfg.jumbo_frame        = false;
    mac_cfg.watchdog_disable   = false;
    mac_cfg.jabber_disable     = false;
    eth_config_mac(&mac_cfg);
    printf("  已恢复为标准帧模式\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景4：完整 DMA 配置结构体
 * 演示：eth_config_dma() 精细控制 DMABMR 和 DMAOMR 所有字段
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_4_dma_config_struct(void)
{
    print_separator();
    printf("=== 场景4：完整 DMA 配置结构体 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：构建高性能 DMA 配置（PBL=32，存储转发，对齐突发）\n");
    eth_dma_config_t dma_cfg = {
        .pbl                  = 32U,               /* 突发长度 32 个节拍 */
        .rx_dma_pbl           = 32U,
        .desc_skip_length     = 0U,
        .priority             = ETH_DMA_PRIORITY_RX_2_TX_1, /* RX 优先级 2:1 */
        .rx_threshold         = ETH_RX_THRESHOLD_64,
        .tx_threshold         = ETH_TX_THRESHOLD_192,
        .address_aligned      = true,
        .mixed_burst          = false,
        .fixed_burst          = true,
        .enhanced_desc        = false,
        .use_separate_pbl     = true,
        .rx_store_forward     = true,              /* 接收存储转发模式 */
        .tx_store_forward     = true,              /* 发送存储转发模式 */
        .flush_rx_frame       = false,
        .fwd_error_frames     = false,
        .fwd_undersized_gf    = false,
        .drop_tcp_cs_err      = true,              /* 丢弃 TCP/IP 校验和错误帧 */
        .operate_on_2nd_frame = true,              /* 直接操作第二帧，提高吞吐量 */
        .flush_tx_fifo        = false,
        .epm                  = false,
        .rib                  = false,
    };

    eth_config_dma(&dma_cfg);
    printf("  已配置：PBL=32，TX/RX 存储转发，对齐突发，丢弃 CS 错误帧\n");

    printf("\n步骤2：演示切换为低延迟阈值模式（Cut-Through）\n");
    dma_cfg.rx_store_forward  = false;   /* 关闭存储转发，使用阈值触发 */
    dma_cfg.tx_store_forward  = false;
    dma_cfg.rx_threshold      = ETH_RX_THRESHOLD_32;
    dma_cfg.tx_threshold      = ETH_TX_THRESHOLD_64;
    eth_config_dma(&dma_cfg);
    printf("  已切换至 Cut-Through 模式（RX阈值32B / TX阈值64B）\n");

    printf("\n步骤3：设置 RX 接收看门狗定时器（RIWTC）\n");
    /* RIWTC = N：等待 256N 个时钟后若无描述符则产生 RWT 中断 */
    eth_set_rx_watchdog_timer(0x08U);
    printf("  RIWTC=0x08（256×8 个时钟周期）\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景5：帧过滤配置
 * 演示：eth_config_filter() + eth_set_hash_table()
 *        设置混杂模式、广播过滤、哈希多播过滤、IP DA 过滤
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_5_frame_filter(void)
{
    print_separator();
    printf("=== 场景5：帧过滤配置 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：混杂模式（接收全部帧，无过滤）\n");
    eth_filter_config_t filter = {
        .promiscuous = true,
    };
    eth_config_filter(&filter);
    printf("  混杂模式已使能\n");

    printf("\n步骤2：关闭混杂模式，使能多播哈希过滤\n");
    memset(&filter, 0, sizeof(filter));
    filter.hash_multicast = true;
    filter.broadcast_disable = false;     /* 允许广播帧 */

    /* 计算多播 MAC 地址哈希值：ETH_MULTICAST_01:00:5e:xx:xx:xx
     * 哈希表仅演示，实际值应按 CRC32 计算后取高 6 位映射 */
    eth_config_filter(&filter);
    /* 设置 64 位哈希表（每位对应一个 CRC32 高 6 bit 的组合）*/
    eth_set_hash_table(0x00000001UL, 0x00000000UL);  /* bit 0: hash=0 的多播 */
    printf("  多播哈希过滤已使能，哈希表=0x00000001/0x00000000\n");

    printf("\n步骤3：使能 IP DA 过滤 + L3/L4 过滤总开关\n");
    filter.ip_da_filter = true;
    eth_config_filter(&filter);
    printf("  IP DA 过滤已使能（需与 L3L4 过滤配合，见场景8）\n");

    printf("\n步骤4：使能源地址过滤（SA 过滤模式）\n");
    filter.sa_filter            = true;
    filter.sa_inverse_filter    = false;
    eth_config_filter(&filter);
    printf("  源地址过滤已使能（仅接收来自 MACA0/1/2/3 的帧）\n");

    printf("\n步骤5：恢复默认（仅过滤目的地址，允许广播，接收本机 MAC 帧）\n");
    memset(&filter, 0, sizeof(filter));
    eth_config_filter(&filter);
    printf("  已恢复默认过滤配置\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景6：流量控制（PAUSE 帧）配置
 * 演示：eth_config_flow_ctrl() 配置接收/发送 PAUSE 帧处理
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_6_flow_control(void)
{
    print_separator();
    printf("=== 场景6：流量控制（PAUSE 帧）配置 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：全双工模式下使能 TX/RX 流量控制，PAUSE_TIME=0x0100\n");
    eth_flow_ctrl_config_t fc = {
        .tx_flow_ctrl          = true,     /* 接收缓冲区满时自动发送 PAUSE 帧 */
        .rx_flow_ctrl          = true,     /* 收到 PAUSE 帧时暂停发送 */
        .pause_time            = 0x0100U,  /* PAUSE 时间量子 = 0x0100（512 位时间） */
        .pause_low_threshold   = ETH_PAUSE_THRESHOLD_MINUS_4, /* 触发低阈值 */
        .unicast_pause_detect  = false,    /* 不检测单播 PAUSE 帧 */
        .zero_quanta_pause     = false,
        .back_pressure         = false,    /* 全双工模式下不使用背压 */
    };
    eth_config_flow_ctrl(&fc);
    printf("  TX 流控已使能，PAUSE_TIME=0x0100\n");
    printf("  RX 流控已使能（收到 PAUSE 帧则暂停发送）\n");

    printf("\n步骤2：发送零时间量子 PAUSE 帧（立即解除对端暂停）\n");
    fc.zero_quanta_pause = true;
    eth_config_flow_ctrl(&fc);
    printf("  零时间量子 PAUSE 帧已触发\n");

    printf("\n步骤3：半双工背压模式配置\n");
    fc.zero_quanta_pause  = false;
    fc.tx_flow_ctrl       = false;
    fc.rx_flow_ctrl       = false;
    fc.back_pressure      = true;    /* 半双工模式下通过抢占载波来拥塞对端 */
    eth_config_flow_ctrl(&fc);
    printf("  半双工背压模式已使能（MAC 通过拥塞信号通知对端暂停）\n");

    printf("\n步骤4：关闭流控（恢复默认）\n");
    memset(&fc, 0, sizeof(fc));
    eth_config_flow_ctrl(&fc);
    printf("  流量控制已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景7：VLAN 过滤配置
 * 演示：eth_config_vlan() 配置 VLAN 标签过滤与哈希过滤
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_7_vlan_filter(void)
{
    print_separator();
    printf("=== 场景7：VLAN 过滤配置 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：完整 16 位 VLAN ID 匹配（VLAN ID = 0x0064）\n");
    eth_vlan_config_t vlan = {
        .vlan_id            = 0x0064U,  /* VLAN ID = 100 */
        .vlan_12bit_compare = false,    /* 完整 16 位比较（含 PCP/DEI） */
        .vlan_tag_invert    = false,
        .svlan_enable       = false,    /* 不使用 S-VLAN (0x88A8) */
        .vlan_hash_filter   = false,
        .vlan_hash_table    = 0x0000U,
    };
    eth_config_vlan(&vlan);
    printf("  VLAN ID=0x%04X，完整 16 位匹配\n", vlan.vlan_id);

    printf("\n步骤2：切换为 12 位 VLAN ID 匹配（仅比较 VID 字段，忽略 PCP/DEI）\n");
    vlan.vlan_12bit_compare = true;
    eth_config_vlan(&vlan);
    printf("  已切换至 12 位 VID 匹配模式\n");

    printf("\n步骤3：反向 VLAN 过滤（过滤掉匹配项，仅接收不带 VID=100 的帧）\n");
    vlan.vlan_tag_invert = true;
    eth_config_vlan(&vlan);
    printf("  反向过滤已使能（丢弃 VID=100 的帧）\n");

    printf("\n步骤4：使能 VLAN 哈希过滤，设置哈希表匹配多个 VLAN\n");
    vlan.vlan_tag_invert = false;
    vlan.vlan_id         = 0x0000U;   /* 哈希模式下 VL 忽略 */
    vlan.vlan_hash_filter = true;
    /* 假设 VLAN 200 的哈希位 = 3，VLAN 300 的哈希位 = 7 */
    vlan.vlan_hash_table = (1U << 3) | (1U << 7);
    eth_config_vlan(&vlan);
    printf("  VLAN 哈希过滤已使能，哈希表=0x%04X（接收 VLAN 200 和 300）\n",
           vlan.vlan_hash_table);

    printf("\n步骤5：关闭 VLAN 过滤（通过帧过滤中 VTFE 位控制，此处恢复默认）\n");
    memset(&vlan, 0, sizeof(vlan));
    eth_config_vlan(&vlan);
    eth_filter_config_t filter = {0};
    filter.vlan_tag_filter = false;
    eth_config_filter(&filter);
    printf("  VLAN 过滤已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景8：L3/L4 过滤配置
 * 演示：eth_config_l3l4_filter() 配置两个过滤器实例
 *   过滤器0：IPv4 DA 匹配 192.168.1.100，TCP 目的端口 80
 *   过滤器1：IPv6 SA 匹配前 64 位前缀 + UDP 源端口 5000
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_8_l3l4_filter(void)
{
    print_separator();
    printf("=== 场景8：L3/L4 过滤配置 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);
    /* 使能帧过滤中的 IP DA 过滤总开关 */
    eth_filter_config_t filter = { .ip_da_filter = true };
    eth_config_filter(&filter);

    printf("步骤1：过滤器0 —— IPv4 DA = 192.168.1.100，TCP 目的端口 80\n");
    eth_l3l4_filter_config_t f0 = {
        .l3_protocol_ipv6  = false,   /* IPv4 */
        .l3_da_match       = true,    /* DA 匹配 */
        .l3_da_prefix_len  = 32U,     /* 完整 32 位匹配 */
        /* 192.168.1.100 = 0xC0A80164 */
        .l3_addr           = { 0xC0A80164UL, 0, 0, 0 },
        .l4_protocol_udp   = false,   /* TCP（为 false） */
        .l4_dst_match      = true,    /* 目的端口匹配 */
        .l4_dst_port       = 80U,
    };
    eth_config_l3l4_filter(0U, &f0);
    printf("  过滤器0：IPv4 DA=192.168.1.100，TCP DST_PORT=80\n");

    printf("\n步骤2：过滤器1 —— IPv6 SA 前 64 位前缀 2001:DB8::/64，UDP 源端口 5000\n");
    eth_l3l4_filter_config_t f1 = {
        .l3_protocol_ipv6  = true,    /* IPv6 */
        .l3_sa_match       = true,    /* SA 匹配 */
        .l3_sa_prefix_len  = 64U,     /* 匹配 SA 前 64 位 */
        /* 2001:0DB8:0000:0000 → 四个 32 位字（大端） */
        .l3_addr           = { 0x20010DB8UL, 0x00000000UL, 0x00000000UL, 0x00000000UL },
        .l4_protocol_udp   = true,    /* UDP */
        .l4_src_match      = true,    /* 源端口匹配 */
        .l4_src_port       = 5000U,
    };
    eth_config_l3l4_filter(1U, &f1);
    printf("  过滤器1：IPv6 SA=2001:DB8::/64（前64位），UDP SRC_PORT=5000\n");

    printf("\n步骤3：覆盖过滤器0 为反向 IP DA 过滤（过滤掉目标 IP）\n");
    f0.l3_da_inverse = true;   /* 不匹配该 IP 的帧才通过 */
    eth_config_l3l4_filter(0U, &f0);
    printf("  过滤器0 反向使能（丢弃 DA=192.168.1.100 的包）\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景9：多 MAC 地址注册（index 0~3）
 * 演示：eth_config_mac_addr() 配置 4 个 MAC 地址过滤条目
 *        - index 0：本机主 MAC（无 MBC/SA 位）
 *        - index 1：附加单播地址（使用 SA 过滤匹配源地址）
 *        - index 2：多播地址，MBC=0x38（仅比较低 3 字节）
 *        - index 3：禁用该条目
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_9_mac_addr_multi(void)
{
    print_separator();
    printf("=== 场景9：多 MAC 地址注册（index 0~3） ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：index 0 —— 主 MAC 地址 00:11:22:33:44:55\n");
    eth_mac_addr_config_t addr0 = {
        .addr   = { 0x00U, 0x11U, 0x22U, 0x33U, 0x44U, 0x55U },
        .enable = true,
    };
    eth_config_mac_addr(0U, &addr0);
    printf("  MACA0 = %02X:%02X:%02X:%02X:%02X:%02X（已使能）\n",
           addr0.addr[0], addr0.addr[1], addr0.addr[2],
           addr0.addr[3], addr0.addr[4], addr0.addr[5]);

    printf("\n步骤2：index 1 —— 附加单播 MAC，SA 过滤（过滤源地址匹配的帧）\n");
    eth_mac_addr_config_t addr1 = {
        .addr     = { 0xAAU, 0xBBU, 0xCCU, 0xDDU, 0xEEU, 0xFFU },
        .enable   = true,
        .src_addr = true,   /* SA 模式：匹配源地址而非目的地址 */
        .mask_bits= 0x00U,  /* 所有字节参与比较 */
    };
    eth_config_mac_addr(1U, &addr1);
    printf("  MACA1 = AA:BB:CC:DD:EE:FF（SA 模式，使能）\n");

    printf("\n步骤3：index 2 —— 多播地址，MBC=0x3F（忽略所有字节 → 接收全部多播）\n");
    eth_mac_addr_config_t addr2 = {
        .addr     = { 0x01U, 0x00U, 0x5EU, 0x00U, 0x00U, 0x01U },
        .enable   = true,
        .src_addr = false,
        .mask_bits= 0x3FU,  /* bit[5:0]=1：全部字节均被屏蔽（通配）*/
    };
    eth_config_mac_addr(2U, &addr2);
    printf("  MACA2 = 01:00:5E:00:00:01（MBC=0x3F，接收所有多播帧）\n");

    printf("\n步骤4：index 3 —— 禁用（AE=0）\n");
    eth_mac_addr_config_t addr3 = {
        .addr   = { 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U },
        .enable = false,
    };
    eth_config_mac_addr(3U, &addr3);
    printf("  MACA3 已禁用\n");

    printf("\n步骤5：动态修改 index 1 为 DAM 模式（目的地址匹配）\n");
    addr1.src_addr = false;
    addr1.mask_bits= 0x00U;
    memcpy(addr1.addr, (uint8_t[]){0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x07U}, 6U);
    eth_config_mac_addr(1U, &addr1);
    printf("  MACA1 更新为 02:03:04:05:06:07（DA 模式，使能）\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景10：MMC 硬件统计计数读取
 * 演示：eth_config_mmc() 配置计数器策略 + eth_mmc_get_stats() 读取统计
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_10_mmc_stats(void)
{
    print_separator();
    printf("=== 场景10：MMC 硬件统计计数读取 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：复位所有 MMC 计数器\n");
    eth_mmc_config_t mmc_cfg = {
        .counter_reset       = true,         /* 复位所有计数器 */
        .stop_on_rollover    = false,        /* 计数器溢出时继续（不停止） */
        .reset_on_read       = true,         /* 读后自动清零（适合持续监控） */
        .freeze_on_preset    = false,
        .counter_preset      = false,
        .counter_preset_full = false,
        .ucast_detect_bcast  = false,
    };
    eth_config_mmc(&mmc_cfg);
    printf("  MMC 计数器已复位，read-on-clear 模式已使能\n");

    printf("\n步骤2：使能 ETH，运行 200 ms 产生流量统计数据\n");
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();
    /* 发送几帧以产生 MMC TX 统计 */
    uint8_t local_mac[] = LOCAL_MAC_ADDR;
    uint8_t test_frame[60];
    uint8_t bcast[] = DEST_MAC_BROADCAST;
    memcpy(test_frame, bcast,      6U);
    memcpy(test_frame + 6, local_mac, 6U);
    test_frame[12] = 0x08U;
    test_frame[13] = 0x00U;
    memset(test_frame + 14, 0xAAU, 46U);
    for (uint8_t i = 0U; i < 3U; i++) {
        uint8_t idx = get_next_tx_desc();
        if (idx < TX_DESC_COUNT) {
            eth_transmit_frame(&s_tx_desc[idx], test_frame, 60U);
        }
        delay_ms(10U);
    }
    delay_ms(200U);

    printf("\n步骤3：读取 MMC 统计计数\n");
    eth_mmc_stats_t stats;
    memset(&stats, 0, sizeof(stats));
    eth_mmc_get_stats(&stats);
    printf("  TX 好帧（单/多次冲突后成功发送）: %u\n", stats.tx_good_frames);
    printf("  TX 单次冲突好帧: %u\n", stats.tx_single_collision_gf);
    printf("  TX 多次冲突好帧: %u\n", stats.tx_multi_collision_gf);
    printf("  RX CRC 错误帧:   %u\n", stats.rx_crc_error);
    printf("  RX 对齐错误帧:   %u\n", stats.rx_align_error);
    printf("  RX 单播好帧:     %u\n", stats.rx_unicast_good_frames);

    printf("\n步骤4：演示 MMC 中断屏蔽（屏蔽并重新使能 TX 单次冲突中断）\n");
    eth_mmc_tx_irq_config((1UL << 14), true);   /* 屏蔽 TGFSCIM（TX 单次冲突）*/
    printf("  TX 单次冲突 MMC 中断已屏蔽\n");
    eth_mmc_tx_irq_config((1UL << 14), false);  /* 解除屏蔽 */
    printf("  TX 单次冲突 MMC 中断已解除屏蔽\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景11：PTP 时间戳基本配置与时间读写
 * 演示：eth_config_ptp() + eth_ptp_set_time() + eth_ptp_get_time()
 *        → 验证秒/纳秒读取正确性，演示跨秒二次读保护
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_11_ptp_basic(void)
{
    print_separator();
    printf("=== 场景11：PTP 时间戳基本配置与时间读写 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    /* PTP 时间戳时钟源 = ETH_MAC 所在 AHB 总线时钟（HCLK），与 RMII REF_CLK 无关。
     * 粗调模式：每个 HCLK 周期，亚秒寄存器直接 += SSIR（ns）。
     * 正确的 SSIR = round(1e9 / HCLK)，使每 HCLK 周期恰好代表 1 个真实 ns。
     * 因 SSIR 取整，时钟精度误差 < 1 ULP / HCLK，对粗调场景完全够用。 */
    uint32_t hclk_hz = clock_get_hclk_hz();
    /* 四舍五入：(1e9 + hclk/2) / hclk */
    uint32_t ssir_ns = (1000000000U + hclk_hz / 2U) / hclk_hz;

    printf("步骤1：配置 PTP 模块\n");
    printf("  PTP 时钟源：HCLK = %u MHz（ETH_MAC AHB 总线时钟）\n", hclk_hz / 1000000U);
    printf("  PTPSSIR = %u ns（= round(1e9 / HCLK)，粗调模式）\n", ssir_ns);
    printf("  理论速率误差：%.2f ppm\n",
           (float)((int32_t)((int64_t)ssir_ns * hclk_hz - 1000000000LL)) / 1000.0f);

    eth_ptp_config_t ptp_cfg = {
        .timestamp_enable   = true,
        .fine_update        = false,   /* 粗调模式：每 HCLK 周期直接 += SSIR */
        .subsecond_increment= (uint8_t)ssir_ns,
        .addend             = 0U,      /* 粗调模式下 addend 无意义，传 0 跳过写入 */
        .snapshot_all       = true,    /* 对所有收/发帧打时间戳 */
        .ptp_v2             = false,
        .eth_frames_enable  = false,
        .ipv4_enable        = false,
        .ipv6_enable        = false,
        .event_msgs_only    = false,
        .snap_master_only   = false,
        .clock_type         = ETH_PTP_CLOCK_ORDINARY_SLAVE,
        .mac_addr_filter    = false,
        .pps_signal_enable  = true,    /* 使能 PPS（每秒脉冲） */
    };
    eth_config_ptp(&ptp_cfg);
    printf("  PTP 粗调模式已配置，SSIR=%u ns，all-frames 时间戳，PPS 已使能\n", ssir_ns);

    printf("\n步骤2：设置初始时间 = 1970-01-01 00:00:10.000000000\n");
    eth_ptp_time_t t_set = { .seconds = 10U, .nanoseconds = 0U };
    eth_ptp_set_time(&t_set);
    printf("  时间已初始化：sec=%u, nsec=%u\n", t_set.seconds, t_set.nanoseconds);

    printf("\n步骤3：延迟 500 ms 后读取当前时间\n");
    delay_ms(500U);
    eth_ptp_time_t t_now;
    eth_ptp_get_time(&t_now);
    printf("  当前时间：sec=%u, nsec=%u\n", t_now.seconds, t_now.nanoseconds);
    printf("  预期：sec≈10，nsec≈500000000（500 ms）\n");

    printf("\n步骤4：时间正向调整 +1 s（update_time）\n");
    eth_ptp_time_t delta = { .seconds = 1U, .nanoseconds = 0U };
    eth_ptp_update_time(&delta);
    eth_ptp_get_time(&t_now);
    printf("  调整后：sec=%u, nsec=%u（预期 sec≈11）\n",
           t_now.seconds, t_now.nanoseconds);

    printf("\n步骤5：读取 PTP 状态寄存器\n");
    uint32_t ptp_status = eth_get_ptp_status();
    printf("  PTPTSSR=0x%08X（TSO=%d, TTTR=%d, ATTS=%d, TTTE=%d）\n",
           ptp_status,
           (int)(ptp_status & 1U),
           (int)((ptp_status >> 1) & 1U),
           (int)((ptp_status >> 2) & 1U),
           (int)((ptp_status >> 3) & 1U));

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景12：PTP 频率微调与目标触发时间
 * 演示：eth_ptp_adjust_freq(ppb) + eth_ptp_set_target_time()
 *         + eth_ptp_set_pps_output() + eth_ptp_get_aux_timestamp()
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_12_ptp_fine_adjust(void)
{
    print_separator();
    printf("=== 场景12：PTP 频率微调与目标触发时间 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    /* 精调模式原理：每个 HCLK 周期，32 位累加器 += addend；
     * 累加器溢出（≥ 2^32）时，亚秒寄存器 += SSIR（ns）。
     * 等效速率 = addend × SSIR_ns × HCLK / 2^32 / 1e9（需 = 1 for real-time）。
     * 正确公式（整数无浮点）：
     *   ssir_ns = ceil(1e9 / HCLK)          → 保证 addend ≤ 2^32
     *   addend  = 2^32 × 1e9 / (HCLK × ssir_ns)  → uint64 中间量，安全 */
    uint32_t hclk_hz = clock_get_hclk_hz();
    /* 向上取整：ceil(1e9 / HCLK) */
    uint32_t ssir_ns = (1000000000U + hclk_hz - 1U) / hclk_hz;
    /* addend = 2^32 × 1e9 / (HCLK × ssir)；中间量 ≤ 4.295e18 < uint64_t max */
    uint32_t base_addend = (uint32_t)(
        (uint64_t)4294967296ULL * 1000000000ULL /
        ((uint64_t)hclk_hz * (uint64_t)ssir_ns));

    printf("步骤1：使能 PTP 精调模式（fine update），设置初始加法器\n");
    printf("  PTP 时钟源：HCLK = %u MHz\n", hclk_hz / 1000000U);
    printf("  SSIR = %u ns（= ceil(1e9 / HCLK)）\n", ssir_ns);
    printf("  base_addend = 0x%08X（= 2^32 × 1e9 / (HCLK × SSIR)）\n", base_addend);
    printf("  理论速率误差：%.2f ppm\n",
           (float)((int64_t)base_addend * ssir_ns * (int64_t)(hclk_hz / 1000U)
                   - (int64_t)4294967296LL * 1000000LL)
           / (float)(4294967296LL * 1000LL) * 1e6f);

    eth_ptp_config_t ptp_cfg = {
        .timestamp_enable    = true,
        .fine_update         = true,        /* 精调模式 */
        .subsecond_increment = (uint8_t)ssir_ns,
        .addend              = base_addend, /* 写入 PTPTSAR，触发 TARU 更新 */
        .snapshot_all        = true,
        .pps_signal_enable   = true,
    };
    eth_config_ptp(&ptp_cfg);

    eth_ptp_time_t t_init = { .seconds = 100U, .nanoseconds = 0U };
    eth_ptp_set_time(&t_init);
    printf("  精调模式已使能，addend=0x%08X，初始时间 sec=%u\n",
           base_addend, t_init.seconds);

    printf("\n步骤2：模拟 GPS/PTP 主时钟纠偏 +500 ppb（晶振偏慢，加速补偿）\n");
    eth_ptp_adjust_freq(500);     /* 加速 500 ppb：addend 增大，时钟走快 */
    printf("  频率调整 +500 ppb（加速 500 ppb）完成\n");

    printf("\n步骤3：模拟频率偏慢，调整 -1000 ppb\n");
    eth_ptp_adjust_freq(-1000);
    printf("  频率调整 -1000 ppb（减速 1000 ppb）完成\n");

    printf("\n步骤4：配置目标时间 = 当前时间 + 2 s，触发 TTTE 中断 / PPS 输出\n");
    eth_ptp_time_t t_now;
    eth_ptp_get_time(&t_now);
    eth_ptp_time_t t_target = { .seconds = t_now.seconds + 2U, .nanoseconds = 0U };
    eth_ptp_set_target_time(&t_target);
    printf("  目标时间设置为 sec=%u（当前 sec=%u + 2）\n",
           t_target.seconds, t_now.seconds);

    printf("\n步骤5：配置 PPS 输出（固定周期：interval=1 s = 10^9 ns，width=500 ms）\n");
    eth_ptp_set_pps_output(1000000000UL, 500000000UL);
    printf("  PPS 脉冲输出：周期=1 s，高电平宽度=500 ms\n");

    printf("\n步骤6：延迟 500 ms 后读取辅助时间戳（若配置了外部触发）\n");
    delay_ms(500U);
    eth_ptp_time_t aux_ts;
    eth_ptp_get_aux_timestamp(&aux_ts);
    printf("  辅助时间戳：sec=%u, nsec=%u（无外部触发时为上次触发缓存值）\n",
           aux_ts.seconds, aux_ts.nanoseconds);

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景13：LPI / EEE（IEEE 802.3az）节能配置
 * 演示：eth_config_lpi() 配置 LPI 使能、LS 计时器、TW 计时器
 *         + eth_get_lpi_status() 读取 LPI 状态
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_13_lpi_eee(void)
{
    print_separator();
    printf("=== 场景13：LPI / EEE（802.3az）节能配置 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    /* EEE 需要先协商（PHY和对端均支持），此处仅演示寄存器配置 */
    printf("步骤1：读取当前 LPI 状态（MACLPICSR）\n");
    uint32_t lpi_status = eth_get_lpi_status();
    printf("  MACLPICSR=0x%08X\n", lpi_status);
    printf("    TLPIST=%d（TX 处于 LPI 状态）\n", (int)((lpi_status >> 8) & 1U));
    printf("    RLPIST=%d（RX 处于 LPI 状态）\n", (int)((lpi_status >> 9) & 1U));
    printf("    LPIEN=%d（LPI 使能）\n",           (int)((lpi_status >> 16) & 1U));
    printf("    PLS=%d（PHY 链路状态）\n",          (int)((lpi_status >> 17) & 1U));

    printf("\n步骤2：配置 LPI 计时器（EEE 标准值）\n");
    printf("  ls_timer_ms=1000：链路状态计时器 1 s（发送完最后一帧后等待 1 s 才进入 LPI）\n");
    printf("  tw_timer_us=21  ：唤醒计时器 21 μs（从 LPI 退出后等待 21 μs 再发帧）\n");
    eth_lpi_config_t lpi_cfg = {
        .ls_timer_ms  = 1000U,   /* 1000 ms = 1 s（标准最大值） */
        .tw_timer_us  = 21U,     /* 21 μs（IEEE 802.3az 规格） */
        .lpi_enable   = false,   /* 先不使能，等待链路协商完成后再使能 */
        .pls          = false,   /* PHY_LINK_STATUS: 0=down，需 PHY 连接后置 1 */
        .lpitxa       = true,    /* TX 自动触发：发送空闲时自动进入 LPI */
    };
    eth_config_lpi(&lpi_cfg);
    printf("  LPI 计时器已配置（LPI 暂不使能，等待链路UP后再使能）\n");

    printf("\n步骤3：模拟链路 UP，使能 LPI\n");
    lpi_cfg.lpi_enable = true;
    lpi_cfg.pls        = true;  /* 置 1 表示 PHY 链路已 UP */
    eth_config_lpi(&lpi_cfg);
    printf("  LPI 已使能（PLS=1，系统空闲时自动进入低功耗节能模式）\n");

    printf("\n步骤4：等待 100 ms 后读取 LPI 状态\n");
    delay_ms(100U);
    lpi_status = eth_get_lpi_status();
    printf("  MACLPICSR=0x%08X  TLPIST=%d  RLPIST=%d  LPIEN=%d\n",
           lpi_status,
           (int)((lpi_status >> 8) & 1U),
           (int)((lpi_status >> 9) & 1U),
           (int)((lpi_status >> 16) & 1U));

    printf("\n步骤5：关闭 LPI（链路断开时清 PLS 再禁用）\n");
    lpi_cfg.lpi_enable = false;
    lpi_cfg.pls        = false;
    eth_config_lpi(&lpi_cfg);
    printf("  LPI 已关闭\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景14：PMT 唤醒配置（Magic Packet + 唤醒帧过滤）
 * 演示：eth_set_wake_up_frame() + eth_config_pmt()
 *         + eth_config_watchdog_timer() + 功率下降寄存器配置
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_14_pmt_wakeup(void)
{
    print_separator();
    printf("=== 场景14：PMT 唤醒配置（Magic Packet + 唤醒帧过滤） ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();
    eth_init_config_t base_cfg = {
        .phy_interface = ETH_PHY_RMII,
        .smi_clk_range = ETH_SMI_CLK,
        .tx_desc_base  = s_tx_desc,
        .rx_desc_base  = s_rx_desc,
        .tx_desc_count = TX_DESC_COUNT,
        .rx_desc_count = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);

    printf("步骤1：配置 MAC 可编程看门狗定时器\n");
    printf("  接收帧最长等待 WTO=0x005 个 256 字节时间片\n");
    eth_watchdog_timer_config_t wdt = {
        .timeout_val     = 0x005U,
        .prog_wdt_enable = true,
    };
    eth_config_watchdog_timer(&wdt);
    printf("  WDT 已配置（MACWTR.WTO=0x005，PWE=1）\n");

    printf("\n步骤2：写入唤醒帧过滤器（8×32-bit 寄存器，MACRWUFFR）\n");
    /* 唤醒帧过滤器格式（按硬件顺序写入8个字）：
     *   word[0]: 过滤器0 使能位 + CRC 偏移
     *   word[1]: 字节掩码 [31:0]
     *   word[2]: 字节掩码 [63:32]
     *   word[3]: CRC-16 目标值
     *   word[4~7]: 过滤器1（重复上述格式）
     * 本示例配置为：接收 UDP 端口 7（Echo，常用作 Wake-on-LAN 触发包含此端口） */
    uint32_t wuf_regs[8] = {
        /* 过滤器0：单播，偏移 36（UDP目的端口字节位置），使能 */
        0x00000021UL,  /* byte[0]: [4:0]=pkt_type(1=unicast), [12:8]=byte_offset, byte[1]保留 filter_enable=bit31 */
        0xFF000000UL,  /* 字节掩码 [31:0]：匹配第28~31字节 */
        0x00000000UL,  /* 字节掩码 [63:32] */
        0x0007C407UL,  /* UDP 端口 7（0x0007） CRC-16 目标 */
        /* 过滤器1：禁用 */
        0x00000000UL, 0x00000000UL, 0x00000000UL, 0x00000000UL,
    };
    eth_set_wake_up_frame(wuf_regs);
    printf("  唤醒帧过滤器已写入（过滤器0：UDP 端口7匹配）\n");

    printf("\n步骤3：注册 PMT 唤醒中断回调\n");
    printf("  （回调在场景15展示，此处仅演示配置）\n");

    printf("\n步骤4：使能 Magic Packet + 唤醒帧检测，进入 PMT 低功耗模式\n");
    eth_config_pmt(
        true,   /* power_down = true：MAC 进入低功耗挂起，直到唤醒事件 */
        true,   /* magic_pkt_en：使能 Magic Packet 检测 */
        true,   /* wake_frame_en：使能唤醒帧过滤 */
        false,  /* global_ucast：不使用全局单播唤醒 */
        false   /* rwffpr：地址指针不复位 */
    );
    printf("  PMT 已使能：Power Down + Magic Packet + 唤醒帧检测\n");
    printf("  MAC 将在收到以下帧时唤醒：\n");
    printf("    1. Magic Packet（目的地址为本机 MAC 的特殊广播帧）\n");
    printf("    2. 匹配过滤器0 CRC 的 UDP 目的端口=7 帧\n");

    printf("\n步骤5：模拟系统收到唤醒事件后，取消 PMT 低功耗模式\n");
    delay_ms(50U);   /* 模拟等待唤醒（实际应在 ETH_WKUP_IRQHandler 中处理） */
    eth_config_pmt(false, false, false, false, false);
    printf("  PMT 低功耗模式已解除\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景15：DMA 中断驱动收发（ETH_IRQHandler 回调）
 * 演示：eth_irq_register() + eth_dma_irq_config()
 *        在 DMA TX 完成（DMA_IT_TI）和 RX 完成（DMA_IT_RI）时触发回调
 *        TX 流程：构造帧 → eth_transmit_frame() → 等待 s_tx_done 标志
 *        RX 流程：DMA 填充描述符 → 回调读帧 → 更新 s_rx_done/s_rx_len
 * ───────────────────────────────────────────────────────────────────────── */

/**
 * @brief ETH DMA 中断回调（ETH_IRQn = 73 触发）
 *
 * event 低 16 位 = DMASR（已由 ISR 写 1 清零前读取）
 * event 高 16 位 = MACISR（若 DMASR.MMCS=1 时有效）
 */
static void eth_dma_callback(void *context, uint32_t event)
{
    (void)context;

    /* DMA_IT_TI = DMASR bit 0 (Transmit Interrupt) */
    if (event & (1UL << 0)) {
        s_tx_done = true;
    }

    /* DMA_IT_RI = DMASR bit 6 (Receive Interrupt) */
    if (event & (1UL << 6)) {
        uint8_t ridx = get_next_rx_desc();
        if (ridx < RX_DESC_COUNT) {
            uint32_t len = 0U;
            (void)eth_receive_frame(&s_rx_desc[ridx], s_rx_buf[ridx], &len);
            s_rx_len  = len;
            s_rx_done = true;
        }
    }

    /* DMA_IT_AIS = DMASR bit 15 (Abnormal Interrupt Summary) */
    if (event & (1UL << 15)) {
        printf("  [ETH IRQ] DMA 异常: DMASR=0x%08X\n", event & 0xFFFFU);
    }
}

void scenario_15_dma_interrupt_tx_rx(void)
{
    print_separator();
    printf("=== 场景15：DMA 中断驱动收发 ===\n\n");

    s_tx_done = false;
    s_rx_done = false;
    s_rx_len  = 0U;

    printf("步骤1：初始化 GPIO、时钟、描述符、eth_init\n");
    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();

    eth_init_config_t cfg = {
        .phy_interface   = ETH_PHY_RMII,
        .smi_clk_range   = ETH_SMI_CLK,
        .speed           = ETH_SPEED_100M,
        .duplex          = ETH_DUPLEX_FULL,
        .tx_desc_base    = s_tx_desc,
        .rx_desc_base    = s_rx_desc,
        .tx_desc_count   = TX_DESC_COUNT,
        .rx_desc_count   = RX_DESC_COUNT,
        .enhanced_desc   = false,
    };
    bool ok = eth_init(&cfg);
    printf("  eth_init: %s\n", ok ? "成功" : "失败");
    if (!ok) { print_separator(); return; }

    printf("\n步骤2：注册 DMA 中断回调，使能 TX/RX/AIS/NIS 中断\n");
    eth_irq_register(ETH_IRQ_DMA, eth_dma_callback, NULL);
    /* NIS（正常汇总）= bit 16，TI = bit 0，RI = bit 6，AIS = bit 15，FBE = bit 13 */
    eth_dma_irq_config(
        (1UL << 0)  |   /* TI   : Transmit Interrupt */
        (1UL << 6)  |   /* RI   : Receive Interrupt */
        (1UL << 13) |   /* FBE  : Fatal Bus Error */
        (1UL << 15) |   /* AIS  : Abnormal Interrupt Summary */
        (1UL << 16),    /* NIS  : Normal Interrupt Summary（必须使能才能触发子中断） */
        true
    );
    NVIC_SetPriority(ETH_IRQn, 4U);
    NVIC_EnableIRQ(ETH_IRQn);
    printf("  ETH_IRQn（%d）已使能，优先级4\n", ETH_IRQn);

    printf("\n步骤3：配置本机 MAC，使能 DMA 引擎\n");
    uint8_t local_mac[] = LOCAL_MAC_ADDR;
    eth_mac_addr_config_t addr0 = { .enable = true };
    memcpy(addr0.addr, local_mac, 6U);
    eth_config_mac_addr(0U, &addr0);
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();

    printf("\n步骤4：发送测试帧，等待 TX 中断（最多 500 ms）\n");
    uint8_t bcast[] = DEST_MAC_BROADCAST;
    uint8_t *frame  = s_tx_buf[0];
    memcpy(frame, bcast,      6U);
    memcpy(frame + 6, local_mac, 6U);
    frame[12] = 0x08U;
    frame[13] = 0x00U;
    memset(frame + 14, 0xBBU, 46U);

    uint8_t desc_idx = get_next_tx_desc();
    if (desc_idx < TX_DESC_COUNT) {
        eth_transmit_frame(&s_tx_desc[desc_idx], frame, 60U);
        printf("  已提交 TX 描述符[%u]，等待 DMA TX 完成中断\n", desc_idx);
    }

    uint64_t t0 = system_get_tick();
    while (!s_tx_done && !system_elapsed(t0, 500U)) {
        __WFI();
    }
    printf("  TX 完成: %s（耗时 %u ms）\n",
           s_tx_done ? "是" : "超时（无中断触发）",
           (uint32_t)(system_get_tick() - t0));

    printf("\n步骤5：等待 RX 中断（最多 3 s，需有外部设备向本机发帧）\n");
    t0 = system_get_tick();
    while (!s_rx_done && !system_elapsed(t0, 3000U)) {
        __WFI();
    }
    if (s_rx_done) {
        printf("  接收到帧：%u 字节\n", s_rx_len);
        printf("  目的地址: %02X:%02X:%02X:%02X:%02X:%02X\n",
               s_rx_buf[0][0], s_rx_buf[0][1], s_rx_buf[0][2],
               s_rx_buf[0][3], s_rx_buf[0][4], s_rx_buf[0][5]);
    } else {
        printf("  3 s 内未收到帧（正常，无外部设备发帧）\n");
    }

    printf("\n步骤6：停止 DMA 引擎，关闭 NVIC 中断，注销回调\n");
    eth_stop_tx();
    eth_stop_rx();
    eth_mac_tx_disable();
    eth_mac_rx_disable();
    NVIC_DisableIRQ(ETH_IRQn);
    eth_dma_irq_config(0xFFFFFFFFUL, false);
    eth_irq_register(ETH_IRQ_DMA, NULL, NULL);
    printf("  已清理\n");

    printf("\n完成！\n");
    print_separator();
}

/* ─────────────────────────────────────────────────────────────────────────
 * 场景16：调试信息与 DMA 状态查询
 * 演示：eth_get_mac_debug_info() + eth_get_dma_missed_frame_count()
 *       + eth_get_dma_status() + eth_get_dma_tx/rx_desc/buf_ptr()
 * ───────────────────────────────────────────────────────────────────────── */
void scenario_16_debug_status(void)
{
    print_separator();
    printf("=== 场景16：调试信息与 DMA 状态查询 ===\n\n");

    gpio_init_eth_rmii();
    eth_clock_init();
    desc_init_tx();
    desc_init_rx();

    eth_init_config_t base_cfg = {
        .phy_interface  = ETH_PHY_RMII,
        .smi_clk_range  = ETH_SMI_CLK,
        .speed          = ETH_SPEED_100M,
        .duplex         = ETH_DUPLEX_FULL,
        .tx_desc_base   = s_tx_desc,
        .rx_desc_base   = s_rx_desc,
        .tx_desc_count  = TX_DESC_COUNT,
        .rx_desc_count  = RX_DESC_COUNT,
    };
    eth_init(&base_cfg);
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();

    /* 发几帧产生数据 */
    uint8_t bcast[] = DEST_MAC_BROADCAST;
    uint8_t local_mac[] = LOCAL_MAC_ADDR;
    uint8_t frame[60];
    memcpy(frame, bcast,       6U);
    memcpy(frame + 6, local_mac, 6U);
    frame[12] = 0x08U;
    frame[13] = 0x00U;
    memset(frame + 14, 0xCCU, 46U);
    for (uint8_t i = 0U; i < 2U; i++) {
        uint8_t idx = get_next_tx_desc();
        if (idx < TX_DESC_COUNT) {
            eth_transmit_frame(&s_tx_desc[idx], frame, 60U);
        }
        delay_ms(5U);
    }
    delay_ms(20U);

    printf("步骤1：读取 DMASR（DMA 状态寄存器），不清除标志\n");
    uint32_t dmasr = eth_get_dma_status(0U);
    printf("  DMASR=0x%08X\n", dmasr);
    printf("    TS (TX stop)  = %d\n", (int)((dmasr >>  1) & 1U));
    printf("    TBU (TX buf)  = %d\n", (int)((dmasr >>  2) & 1U));
    printf("    RS (RX stop)  = %d\n", (int)((dmasr >>  8) & 1U));
    printf("    RBU (RX buf)  = %d\n", (int)((dmasr >>  7) & 1U));
    printf("    FBE (bus err) = %d\n", (int)((dmasr >> 13) & 1U));

    printf("\n步骤2：读取 MAC 调试信息（MACDBGR）\n");
    eth_mac_debug_t dbg;
    eth_get_mac_debug_info(&dbg);
    printf("  TX FIFO: empty=%d  full=%d  write_active=%d  mac_active=%d\n",
           (int)!dbg.tx_fifo_not_empty, (int)dbg.tx_fifo_full,
           (int)dbg.tx_fifo_write_active, (int)dbg.tx_mac_active);
    printf("  TX FIFO 读状态: %d（0=空闲 1=等读 2=读数据 3=刷新）\n",
           dbg.tx_fifo_read_state);
    printf("  TX 帧控制状态: %d（0=空闲 1=等待状态/退避 2:3=PAUSE）\n",
           dbg.tx_frame_ctrl_state);
    printf("  TX 暂停（PAUSE）: %d\n", (int)dbg.tx_paused);
    printf("  RX FIFO 充填等级: %d（0=空 1=<阈值 2/3=≥阈值/满）\n",
           dbg.rx_fifo_fill_level);
    printf("  RX FIFO 溢出: %d\n", (int)dbg.rx_fifo_overflow);

    printf("\n步骤3：读取 DMA 丢帧计数（DMAMFBOCR）\n");
    uint16_t missed_cnt = 0U, overflow_cnt = 0U;
    uint8_t  overflow_flag = 0U;
    eth_get_dma_missed_frame_count(&missed_cnt, &overflow_cnt, &overflow_flag);
    printf("  DMA 丢帧计数（host buffer满丢帧）  : %u\n", missed_cnt);
    printf("  DMA 丢帧计数（FIFO 溢出丢帧）     : %u\n", overflow_cnt);
    printf("  溢出标志: 0x%02X（bit0=host溢出, bit1=FIFO溢出）\n", overflow_flag);

    printf("\n步骤4：读取 DMA 当前描述符 / 缓冲区指针\n");
    printf("  TX 当前描述符地址: 0x%08X（期望在 s_tx_desc 范围内）\n",
           eth_get_dma_tx_desc_ptr());
    printf("  RX 当前描述符地址: 0x%08X\n", eth_get_dma_rx_desc_ptr());
    printf("  TX 当前缓冲区地址: 0x%08X\n", eth_get_dma_tx_buf_ptr());
    printf("  RX 当前缓冲区地址: 0x%08X\n", eth_get_dma_rx_buf_ptr());
    printf("  s_tx_desc 基地址: 0x%08X\n", (uint32_t)s_tx_desc);
    printf("  s_rx_desc 基地址: 0x%08X\n", (uint32_t)s_rx_desc);

    printf("\n步骤5：读取 MAC 子中断状态（MACISR），验证 LPI/PTP/PMT 子中断\n");
    uint32_t macisr = eth_get_mac_isr();
    printf("  MACISR=0x%08X\n", macisr);
    printf("    PMTIS  (PMT)  = %d\n", (int)((macisr >> 3) & 1U));
    printf("    MMCIS  (MMC)  = %d\n", (int)((macisr >> 4) & 1U));
    printf("    MMCRXIS(MMC RX) = %d\n", (int)((macisr >> 5) & 1U));
    printf("    MMCTXIS(MMC TX) = %d\n", (int)((macisr >> 6) & 1U));
    printf("    TSIS   (时间戳) = %d\n", (int)((macisr >> 9) & 1U));
    printf("    LPIIS  (LPI)  = %d\n", (int)((macisr >> 10) & 1U));

    printf("\n步骤6：清除 DMASR 中所有可清除标志\n");
    (void)eth_get_dma_status(0xFFFFFFFFUL);   /* clear_mask=全部 */
    printf("  DMASR 标志已清除\n");

    printf("\n完成！\n");
    print_separator();
}

//===========================================
// 主函数
//===========================================

int main(void)
{
    sys_status_t status = system_init();
    if (status != SYS_OK) {
        /* 系统初始化失败（时钟或时基异常），阻塞等待复位 */
        while (1) { __WFI(); }
    }
    SEGGER_RTT_Init();

    printf("\n");
    print_separator();
    printf("ACM32P4 ETH 驱动示例程序\n");
    printf("当前场景: %d\n", DEFAULT_SCENARIO);
    print_separator();
    printf("\n");

    switch (DEFAULT_SCENARIO)
    {
    case  1: scenario_1_quick_init_poll_tx();      break;
    case  2: scenario_2_phy_config_link_status();  break;
    case  3: scenario_3_mac_config_struct();        break;
    case  4: scenario_4_dma_config_struct();        break;
    case  5: scenario_5_frame_filter();             break;
    case  6: scenario_6_flow_control();             break;
    case  7: scenario_7_vlan_filter();              break;
    case  8: scenario_8_l3l4_filter();              break;
    case  9: scenario_9_mac_addr_multi();           break;
    case 10: scenario_10_mmc_stats();               break;
    case 11: scenario_11_ptp_basic();               break;
    case 12: scenario_12_ptp_fine_adjust();         break;
    case 13: scenario_13_lpi_eee();                 break;
    case 14: scenario_14_pmt_wakeup();              break;
    case 15: scenario_15_dma_interrupt_tx_rx();     break;
    case 16: scenario_16_debug_status();            break;
    default:
        printf("错误：无效的场景编号 %d（有效范围：1-16）\n", DEFAULT_SCENARIO);
        break;
    }

    while (1) { __WFI(); }
}
