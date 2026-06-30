/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-30     ACM32        first version
 */

#include <rtthread.h>
#include <rtdevice.h>
#include <netif/ethernetif.h>
#include <lwipopts.h>
#include <string.h>

#include "drv_eth.h"
#include "acm32p4.h"
#include "hardware/eth.h"
#include "hardware/syscfg.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/system.h"

#ifdef BSP_USING_ETH

/* ── Static: DMA descriptors and frame buffers ── */
static eth_tx_desc_t tx_desc[TX_DESC_COUNT] __attribute__((aligned(4)));
static eth_rx_desc_t rx_desc[RX_DESC_COUNT] __attribute__((aligned(4)));
static rt_uint8_t    tx_buf[TX_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(4)));
static rt_uint8_t    rx_buf[RX_DESC_COUNT][ETH_FRAME_SIZE] __attribute__((aligned(4)));

/* ── Current descriptor indices ── */
static rt_uint8_t tx_idx;
static rt_uint8_t rx_idx;

/**
 * @brief Initialize RMII GPIO pins and reset PHY
 *
 * Reset sequence: PC0 low >= 25ms → high → wait >= 300ms (LAN8720A PLL lock)
 */
static void gpio_init_eth_rmii(void)
{
    /* Configure RMII function pins (all AF6) */
    static const struct {
        gpio_pin_t pin;
        uint8_t    af;
    } rmii_pins[] = {
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

    for (rt_uint32_t i = 0; i < sizeof(rmii_pins) / sizeof(rmii_pins[0]); i++) {
        gpio_init(rmii_pins[i].pin);
        gpio_set_function(rmii_pins[i].pin, rmii_pins[i].af);
        gpio_set_drive_strength(rmii_pins[i].pin, GPIO_DRIVE_16MA);
    }

    /* LAN8720A hardware reset: PC0 active low */
    gpio_init(ETH_RST_PIN);
    gpio_set_dir(ETH_RST_PIN, GPIO_OUT);
    gpio_put(ETH_RST_PIN, false);
    rt_thread_mdelay(25);
    gpio_put(ETH_RST_PIN, true);
    rt_thread_mdelay(300);
}

/**
 * @brief Enable ETH clocks and configure RMII interface
 *
 * Order constraint: enable CLK_SYSCFG before setting PHY interface,
 * must configure PHY interface before enabling ETH clocks.
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
 * @brief Initialize TX descriptor chain (Ring mode)
 *
 * All descriptors initially owned by CPU (OWN=0). Set OWN per frame when sending.
 * Last entry set TER bit for ring wrap-around.
 */
static void desc_init_tx(void)
{
    tx_idx = 0;
    rt_memset(tx_desc, 0, sizeof(tx_desc));
    for (rt_uint8_t i = 0; i < TX_DESC_COUNT; i++) {
        tx_desc[i].tdes2 = (rt_uint32_t)tx_buf[i];
    }
    tx_desc[TX_DESC_COUNT - 1].tdes0 |= TDES0_TER;
}

/**
 * @brief Initialize RX descriptor chain (Ring mode)
 *
 * All descriptors handed to DMA (OWN=1). DMA clears OWN after writing.
 * Last entry set RER bit for ring wrap-around.
 */
static void desc_init_rx(void)
{
    rx_idx = 0;
    rt_memset(rx_desc, 0, sizeof(rx_desc));
    for (rt_uint8_t i = 0; i < RX_DESC_COUNT; i++) {
        rx_desc[i].rdes0 = RDES0_OWN;
        rx_desc[i].rdes1 = ETH_FRAME_SIZE;
        rx_desc[i].rdes2 = (rt_uint32_t)rx_buf[i];
    }
    rx_desc[RX_DESC_COUNT - 1].rdes1 |= RDES1_RER;
}

/**
 * @brief Get next CPU-available TX descriptor (OWN=0)
 * @return Descriptor index, or TX_DESC_COUNT if all occupied by DMA
 */
static rt_uint8_t get_next_tx_desc(void)
{
    for (rt_uint8_t i = 0; i < TX_DESC_COUNT; i++) {
        rt_uint8_t idx = (tx_idx + i) % TX_DESC_COUNT;
        if (!(tx_desc[idx].tdes0 & TDES0_OWN)) {
            tx_idx = (idx + 1) % TX_DESC_COUNT;
            return idx;
        }
    }
    return TX_DESC_COUNT;
}

/**
 * @brief Get next DMA-written RX descriptor (OWN=0)
 * @return Descriptor index, or RX_DESC_COUNT if no new frame
 */
static rt_uint8_t get_next_rx_desc(void)
{
    rt_uint8_t idx = rx_idx;
    if (!(rx_desc[idx].rdes0 & RDES0_OWN)) {
        rx_idx = (idx + 1) % RX_DESC_COUNT;
        return idx;
    }
    return RX_DESC_COUNT;
}

#endif /* BSP_USING_ETH */
