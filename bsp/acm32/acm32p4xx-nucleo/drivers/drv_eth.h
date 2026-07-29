#ifndef __DRV_ETH_H__
#define __DRV_ETH_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== PHY Address ========== */
#define ETH_PHY_ADDR                0x00U   /* LAN8720A, PHYAD0=GND */

/*
 * PHY nRST: any GPIO via Kconfig BSP_ETH_PHY_RST_PIN (RT-Thread pin index).
 *   PXn = port*16 + n  (PA0=0 .. PB14=30, PC0=32, ...)
 * Port is resolved via eth_phy_rst_port() in drv_eth.c (switch on PORT_IDX).
 */
#ifndef BSP_ETH_PHY_RST_PIN
#define BSP_ETH_PHY_RST_PIN         30  /* PB14 default */
#endif

#define ETH_PHY_RST_PORT_IDX        ((BSP_ETH_PHY_RST_PIN) / 16)
#define ETH_PHY_RST_PIN_NUM         ((BSP_ETH_PHY_RST_PIN) % 16)
#define ETH_PHY_RST_PIN             ((uint32_t)(1UL << ETH_PHY_RST_PIN_NUM))

/* ========== IEEE 802.3 Standard Registers ========== */
#define PHY_REG_BCR                 0x00U   /* Basic Control */
#define PHY_REG_BSR                 0x01U   /* Basic Status */
#define PHY_REG_ID1                 0x02U   /* PHY Identifier 1 */
#define PHY_REG_ID2                 0x03U   /* PHY Identifier 2 */
#define PHY_REG_ANAR                0x04U   /* Auto-Negotiation Advertisement */
#define PHY_REG_ANLPAR              0x05U   /* Auto-Negotiation Link Partner */

/* ========== LAN8720A Specific Registers ========== */
#define PHY_REG_SCSR                0x1FU   /* Special Control/Status Register */

/* ========== BCR Bit Definitions ========== */
#define PHY_BCR_RESET               (1U << 15)
#define PHY_BCR_LOOPBACK            (1U << 14)
#define PHY_BCR_SPEED_100           (1U << 13)
#define PHY_BCR_AN_EN               (1U << 12)
#define PHY_BCR_POWER_DOWN          (1U << 11)
#define PHY_BCR_ISOLATE             (1U << 10)
#define PHY_BCR_AN_RESTART          (1U << 9)
#define PHY_BCR_FULL_DUPLEX         (1U << 8)

/* ========== BSR Bit Definitions ========== */
#define PHY_BSR_100BASE_TX_FD       (1U << 14)
#define PHY_BSR_100BASE_TX_HD       (1U << 13)
#define PHY_BSR_10BASE_T_FD         (1U << 12)
#define PHY_BSR_10BASE_T_HD         (1U << 11)
#define PHY_BSR_AN_COMPLETE         (1U << 5)
#define PHY_BSR_REMOTE_FAULT        (1U << 4)
#define PHY_BSR_LINK_UP             (1U << 2)

/* ========== Common PHY IDs (OUI in ID1/ID2) ========== */
#define LAN8720_PHY_ID1             0x0007U
#define LAN8720_PHY_ID2_MASK        0xFFF0U
#define LAN8720_PHY_ID2_VAL         0xC0F0U

/* ========== SCSR (LAN8720 vendor reg 0x1F) ========== */
#define PHY_SCSR_SPEED_MASK         (0x7U << 2)
#define PHY_SCSR_10HD               (0x1U << 2)
#define PHY_SCSR_100HD              (0x2U << 2)
#define PHY_SCSR_10FD               (0x5U << 2)
#define PHY_SCSR_100FD              (0x6U << 2)

/* IEEE ANAR / ANLPAR ability bits (regs 4/5) */
#define PHY_ANAR_10HD               (1U << 5)
#define PHY_ANAR_10FD               (1U << 6)
#define PHY_ANAR_100HD              (1U << 7)
#define PHY_ANAR_100FD              (1U << 8)

/* ========== PHY Status Enum ========== */
enum acm32_phy_link_state
{
    PHY_LINK_DOWN               = 0,
    PHY_LINK_AUTO_NEGOTIATION,
    PHY_LINK_10M_HALF_DUPLEX,
    PHY_LINK_10M_FULL_DUPLEX,
    PHY_LINK_100M_HALF_DUPLEX,
    PHY_LINK_100M_FULL_DUPLEX,
};

/* ========== Constants for ETH driver ========== */
/*
 * Bus masters (ETH/SDMMC/...) can access SRAM1 and OSPI external memory.
 * They cannot access DTCM (0x20000000-0x2000FFFF).
 *
 * All ETH DMA memory (descriptors, TX bounce, RX pool) is allocated
 * from the psram memheap at runtime.
 * Requires DATA_IN_ExtSRAM + System_OSPI_PSRAM_Reclock() before eth init.
 */

/*
 * Pool item holds pbuf_custom header + DMA payload.
 * Max Ethernet frame with FCS is 1518; RxBuffLen must be >= that so a frame
 * never spans two descriptors (HAL only has valid FL on last descriptor).
 * Item size 1600 => payload room after pbuf_custom (~20B) still >= 1536.
 */
#define ETH_RX_BUFFER_SIZE          1600U
#define ETH_RX_BUFFER_CNT           12U
#define ETH_RX_BUF_ITEM_SIZE        ((ETH_RX_BUFFER_SIZE + 31) & ~31)
#define ETH_RX_POOL_SIZE            (ETH_RX_BUFFER_CNT * ETH_RX_BUF_ITEM_SIZE)
#undef ETH_TX_DESC_CNT
#define ETH_TX_DESC_CNT             4U
#undef ETH_RX_DESC_CNT
#define ETH_RX_DESC_CNT             8U
#define ETH_TX_BOUNCE_SIZE          1536U
#define ETH_TX_BOUNCE_CNT           ETH_TX_DESC_CNT
#define ETHIF_TX_TIMEOUT            2000U

/*
 * RMII RX delay taps (ETH_DLYB). HAL_ETH_Init defaults to (10,15).
 * LAN8720A often needs the alternate (1,5); other PHYs keep HAL default.
 * Override at runtime with MSH: eth_rx_dly <unit> <sel>
 */
#define ETH_RX_DLY_HAL_UNIT         10U
#define ETH_RX_DLY_HAL_SEL          15U
#define ETH_RX_DLY_LAN8720_UNIT     1U
#define ETH_RX_DLY_LAN8720_SEL      5U

#ifdef __cplusplus
}
#endif

#endif /* __DRV_ETH_H__ */