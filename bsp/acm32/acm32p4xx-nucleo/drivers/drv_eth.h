#ifndef __DRV_ETH_H__
#define __DRV_ETH_H__

#include <rtthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== PHY Address ========== */
#define ETH_PHY_ADDR                0x00U   /* LAN8720A, PHYAD0=GND */

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

/* ========== LAN8720A PHY ID ========== */
#define LAN8720_PHY_ID1             0x0007U
#define LAN8720_PHY_ID2_MASK        0xFFF0U
#define LAN8720_PHY_ID2_VAL         0xC0F0U

/* ========== SCSR Speed/Duplex Indication ========== */
#define PHY_SCSR_SPEED_MASK         (0x7U << 2)
#define PHY_SCSR_10HD               (0x1U << 2)   /* 10M Half Duplex */
#define PHY_SCSR_100HD              (0x2U << 2)   /* 100M Half Duplex */
#define PHY_SCSR_10FD               (0x5U << 2)   /* 10M Full Duplex */
#define PHY_SCSR_100FD              (0x6U << 2)   /* 100M Full Duplex */

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

///* ========== Constants for ETH driver ========== */
//#define ETH_RX_BUFFER_SIZE          1600U
//#define ETH_RX_BUFFER_CNT           24U
//#define ETH_RX_BUF_ITEM_SIZE        ((ETH_RX_BUFFER_SIZE + 31) & ~31)
//#define ETH_RX_POOL_SIZE            (ETH_RX_BUFFER_CNT * ETH_RX_BUF_ITEM_SIZE)
//#define ETH_TX_DESC_CNT             8U
//#define ETH_RX_DESC_CNT             4U
//#define ETHIF_TX_TIMEOUT            2000U

#ifdef __cplusplus
}
#endif

#endif /* __DRV_ETH_H__ */