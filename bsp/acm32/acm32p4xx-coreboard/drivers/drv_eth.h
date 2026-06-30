/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-30     ACM32        first version
 */

#ifndef __DRV_ETH_H__
#define __DRV_ETH_H__

#include <rtthread.h>
#include "board.h"

#ifdef BSP_USING_ETH

/* DMA descriptor count (from Kconfig, default 4) */
#ifndef BSP_ETH_TX_DESC_COUNT
#define TX_DESC_COUNT   4
#else
#define TX_DESC_COUNT   BSP_ETH_TX_DESC_COUNT
#endif

#ifndef BSP_ETH_RX_DESC_COUNT
#define RX_DESC_COUNT   4
#else
#define RX_DESC_COUNT   BSP_ETH_RX_DESC_COUNT
#endif

/* Max frame size (MTU 1500 + 14B header + 4B CRC + alignment margin) */
#define ETH_FRAME_SIZE  1520

/* PHY address (from Kconfig, default 0x00) */
#ifndef BSP_ETH_PHY_ADDR
#define PHY_ADDR        0x00
#else
#define PHY_ADDR        BSP_ETH_PHY_ADDR
#endif

/* Link detection parameters */
#define PHY_LINK_TIMEOUT    3000    /* Link up timeout (ms) */
#define PHY_POLL_INTERVAL   2000    /* Link poll interval (ms) */

/* ── LAN8720A IEEE 802.3 standard registers ── */
#define PHY_REG_BCR         0x00    /* Basic Control Register */
#define PHY_REG_BSR         0x01    /* Basic Status Register */

/* BCR bit definitions */
#define PHY_BCR_RESET       (1U << 15)
#define PHY_BCR_AN_EN       (1U << 12)
#define PHY_BCR_AN_RESTART  (1U << 9)

/* BSR bit definitions */
#define PHY_BSR_AN_COMPLETE (1U << 5)
#define PHY_BSR_LINK_UP     (1U << 2)

/* DMA descriptor control bits */
#define TDES0_OWN           (1UL << 31)
#define TDES0_IC            (1UL << 30)
#define TDES0_FS            (1UL << 28)
#define TDES0_LS            (1UL << 29)
#define TDES0_TER           (1UL << 21)
#define TDES0_TCH           (1UL << 20)

#define RDES0_OWN           (1UL << 31)
#define RDES0_ES            (1UL << 15)
#define RDES0_FL_POS        16
#define RDES0_FL_MSK        (0x3FFFUL << 16)
#define RDES1_RER           (1UL << 15)

/* DMA status register bits */
#define DMA_SR_NIS          (1UL << 16)
#define DMA_SR_RI           (1UL << 6)
#define DMA_SR_TI           (1UL << 0)

#endif /* BSP_USING_ETH */

#endif /* __DRV_ETH_H__ */
