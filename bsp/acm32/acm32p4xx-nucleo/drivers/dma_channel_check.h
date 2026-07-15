/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Compile-time check: ACM32P4 DMA1/DMA2 each have CH0~CH3 only.
 * Two users must not share the same (unit, channel).
 * Slot id = (unit << 4) | channel.
 */

#ifndef __DMA_CHANNEL_CHECK_H__
#define __DMA_CHANNEL_CHECK_H__

#include "rtconfig.h"

#define ACM32_DMA_SLOT(unit, ch)    (((unit) << 4) | (ch))

/* Defaults if Kconfig ints not yet in rtconfig.h */
#if defined(BSP_USING_UART1_DMA)
#ifndef BSP_UART1_TX_DMA_UNIT
#define BSP_UART1_TX_DMA_UNIT 1
#endif
#ifndef BSP_UART1_TX_DMA_CH
#define BSP_UART1_TX_DMA_CH   1
#endif
#ifndef BSP_UART1_RX_DMA_UNIT
#define BSP_UART1_RX_DMA_UNIT 1
#endif
#ifndef BSP_UART1_RX_DMA_CH
#define BSP_UART1_RX_DMA_CH   2
#endif
#endif
#if defined(BSP_USING_UART2_DMA)
#ifndef BSP_UART2_TX_DMA_UNIT
#define BSP_UART2_TX_DMA_UNIT 1
#endif
#ifndef BSP_UART2_TX_DMA_CH
#define BSP_UART2_TX_DMA_CH   3
#endif
#ifndef BSP_UART2_RX_DMA_UNIT
#define BSP_UART2_RX_DMA_UNIT 2
#endif
#ifndef BSP_UART2_RX_DMA_CH
#define BSP_UART2_RX_DMA_CH   0
#endif
#endif
#if defined(BSP_USING_UART3_DMA)
#ifndef BSP_UART3_TX_DMA_UNIT
#define BSP_UART3_TX_DMA_UNIT 2
#endif
#ifndef BSP_UART3_TX_DMA_CH
#define BSP_UART3_TX_DMA_CH   2
#endif
#ifndef BSP_UART3_RX_DMA_UNIT
#define BSP_UART3_RX_DMA_UNIT 2
#endif
#ifndef BSP_UART3_RX_DMA_CH
#define BSP_UART3_RX_DMA_CH   3
#endif
#endif
#if defined(BSP_USING_UART4_DMA)
#ifndef BSP_UART4_TX_DMA_UNIT
#define BSP_UART4_TX_DMA_UNIT 2
#endif
#ifndef BSP_UART4_TX_DMA_CH
#define BSP_UART4_TX_DMA_CH   1
#endif
#ifndef BSP_UART4_RX_DMA_UNIT
#define BSP_UART4_RX_DMA_UNIT 1
#endif
#ifndef BSP_UART4_RX_DMA_CH
#define BSP_UART4_RX_DMA_CH   0
#endif
#endif
#if defined(BSP_USING_SPI1_DMA)
#ifndef BSP_SPI1_TX_DMA_UNIT
#define BSP_SPI1_TX_DMA_UNIT 2
#endif
#ifndef BSP_SPI1_TX_DMA_CH
#define BSP_SPI1_TX_DMA_CH   1
#endif
#ifndef BSP_SPI1_RX_DMA_UNIT
#define BSP_SPI1_RX_DMA_UNIT 2
#endif
#ifndef BSP_SPI1_RX_DMA_CH
#define BSP_SPI1_RX_DMA_CH   3
#endif
#endif
#if defined(BSP_USING_SPI2_DMA)
#ifndef BSP_SPI2_TX_DMA_UNIT
#define BSP_SPI2_TX_DMA_UNIT 1
#endif
#ifndef BSP_SPI2_TX_DMA_CH
#define BSP_SPI2_TX_DMA_CH   0
#endif
#ifndef BSP_SPI2_RX_DMA_UNIT
#define BSP_SPI2_RX_DMA_UNIT 1
#endif
#ifndef BSP_SPI2_RX_DMA_CH
#define BSP_SPI2_RX_DMA_CH   1
#endif
#endif
#if defined(BSP_USING_SPI3_DMA)
#ifndef BSP_SPI3_TX_DMA_UNIT
#define BSP_SPI3_TX_DMA_UNIT 2
#endif
#ifndef BSP_SPI3_TX_DMA_CH
#define BSP_SPI3_TX_DMA_CH   0
#endif
#ifndef BSP_SPI3_RX_DMA_UNIT
#define BSP_SPI3_RX_DMA_UNIT 2
#endif
#ifndef BSP_SPI3_RX_DMA_CH
#define BSP_SPI3_RX_DMA_CH   2
#endif
#endif
#if defined(BSP_USING_SPI4_DMA)
#ifndef BSP_SPI4_TX_DMA_UNIT
#define BSP_SPI4_TX_DMA_UNIT 1
#endif
#ifndef BSP_SPI4_TX_DMA_CH
#define BSP_SPI4_TX_DMA_CH   2
#endif
#ifndef BSP_SPI4_RX_DMA_UNIT
#define BSP_SPI4_RX_DMA_UNIT 1
#endif
#ifndef BSP_SPI4_RX_DMA_CH
#define BSP_SPI4_RX_DMA_CH   3
#endif
#endif

/* Pair of endpoints collide if any TX/RX slot matches */
#define ACM32_DMA_CROSS(a_tx_u, a_tx_c, a_rx_u, a_rx_c, b_tx_u, b_tx_c, b_rx_u, b_rx_c) \
    ((ACM32_DMA_SLOT(a_tx_u, a_tx_c) == ACM32_DMA_SLOT(b_tx_u, b_tx_c)) || \
     (ACM32_DMA_SLOT(a_tx_u, a_tx_c) == ACM32_DMA_SLOT(b_rx_u, b_rx_c)) || \
     (ACM32_DMA_SLOT(a_rx_u, a_rx_c) == ACM32_DMA_SLOT(b_tx_u, b_tx_c)) || \
     (ACM32_DMA_SLOT(a_rx_u, a_rx_c) == ACM32_DMA_SLOT(b_rx_u, b_rx_c)))

/* ---- self TX/RX ---- */
#if defined(BSP_USING_UART1_DMA)
#if (ACM32_DMA_SLOT(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH))
#error "DMA conflict: UART1 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA)
#if (ACM32_DMA_SLOT(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH))
#error "DMA conflict: UART2 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_UART3_DMA)
#if (ACM32_DMA_SLOT(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH))
#error "DMA conflict: UART3 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_UART4_DMA)
#if (ACM32_DMA_SLOT(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH))
#error "DMA conflict: UART4 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI1_DMA)
#if (ACM32_DMA_SLOT(BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH))
#error "DMA conflict: SPI1 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI2_DMA)
#if (ACM32_DMA_SLOT(BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH))
#error "DMA conflict: SPI2 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI3_DMA)
#if (ACM32_DMA_SLOT(BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH))
#error "DMA conflict: SPI3 TX and RX use the same unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI4_DMA)
#if (ACM32_DMA_SLOT(BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH) == \
     ACM32_DMA_SLOT(BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH))
#error "DMA conflict: SPI4 TX and RX use the same unit+channel"
#endif
#endif

/* ---- cross-peripheral (UART1~4, SPI1~4) ---- */
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_UART2_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH)
#error "DMA conflict: UART1 vs UART2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_UART3_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH)
#error "DMA conflict: UART1 vs UART3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_UART4_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH)
#error "DMA conflict: UART1 vs UART4 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_SPI1_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH)
#error "DMA conflict: UART1 vs SPI1 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_SPI2_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH)
#error "DMA conflict: UART1 vs SPI2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: UART1 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART1_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_UART1_TX_DMA_UNIT, BSP_UART1_TX_DMA_CH, BSP_UART1_RX_DMA_UNIT, BSP_UART1_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: UART1 vs SPI4 share a unit+channel"
#endif
#endif

#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_UART3_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH)
#error "DMA conflict: UART2 vs UART3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_UART4_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH)
#error "DMA conflict: UART2 vs UART4 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_SPI1_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH)
#error "DMA conflict: UART2 vs SPI1 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_SPI2_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH)
#error "DMA conflict: UART2 vs SPI2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: UART2 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART2_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_UART2_TX_DMA_UNIT, BSP_UART2_TX_DMA_CH, BSP_UART2_RX_DMA_UNIT, BSP_UART2_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: UART2 vs SPI4 share a unit+channel"
#endif
#endif

#if defined(BSP_USING_UART3_DMA) && defined(BSP_USING_UART4_DMA)
#if ACM32_DMA_CROSS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, \
                    BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH)
#error "DMA conflict: UART3 vs UART4 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART3_DMA) && defined(BSP_USING_SPI1_DMA)
#if ACM32_DMA_CROSS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, \
                    BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH)
#error "DMA conflict: UART3 vs SPI1 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART3_DMA) && defined(BSP_USING_SPI2_DMA)
#if ACM32_DMA_CROSS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, \
                    BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH)
#error "DMA conflict: UART3 vs SPI2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART3_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: UART3 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART3_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_UART3_TX_DMA_UNIT, BSP_UART3_TX_DMA_CH, BSP_UART3_RX_DMA_UNIT, BSP_UART3_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: UART3 vs SPI4 share a unit+channel"
#endif
#endif

#if defined(BSP_USING_UART4_DMA) && defined(BSP_USING_SPI1_DMA)
#if ACM32_DMA_CROSS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, \
                    BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH)
#error "DMA conflict: UART4 vs SPI1 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART4_DMA) && defined(BSP_USING_SPI2_DMA)
#if ACM32_DMA_CROSS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, \
                    BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH)
#error "DMA conflict: UART4 vs SPI2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART4_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: UART4 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_UART4_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_UART4_TX_DMA_UNIT, BSP_UART4_TX_DMA_CH, BSP_UART4_RX_DMA_UNIT, BSP_UART4_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: UART4 vs SPI4 share a unit+channel"
#endif
#endif

#if defined(BSP_USING_SPI1_DMA) && defined(BSP_USING_SPI2_DMA)
#if ACM32_DMA_CROSS(BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH, \
                    BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH)
#error "DMA conflict: SPI1 vs SPI2 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI1_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: SPI1 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI1_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_SPI1_TX_DMA_UNIT, BSP_SPI1_TX_DMA_CH, BSP_SPI1_RX_DMA_UNIT, BSP_SPI1_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: SPI1 vs SPI4 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI2_DMA) && defined(BSP_USING_SPI3_DMA)
#if ACM32_DMA_CROSS(BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH, \
                    BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH)
#error "DMA conflict: SPI2 vs SPI3 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI2_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_SPI2_TX_DMA_UNIT, BSP_SPI2_TX_DMA_CH, BSP_SPI2_RX_DMA_UNIT, BSP_SPI2_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: SPI2 vs SPI4 share a unit+channel"
#endif
#endif
#if defined(BSP_USING_SPI3_DMA) && defined(BSP_USING_SPI4_DMA)
#if ACM32_DMA_CROSS(BSP_SPI3_TX_DMA_UNIT, BSP_SPI3_TX_DMA_CH, BSP_SPI3_RX_DMA_UNIT, BSP_SPI3_RX_DMA_CH, \
                    BSP_SPI4_TX_DMA_UNIT, BSP_SPI4_TX_DMA_CH, BSP_SPI4_RX_DMA_UNIT, BSP_SPI4_RX_DMA_CH)
#error "DMA conflict: SPI3 vs SPI4 share a unit+channel"
#endif
#endif

#endif /* __DMA_CHANNEL_CHECK_H__ */
