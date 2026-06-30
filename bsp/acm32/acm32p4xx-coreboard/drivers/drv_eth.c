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

/* ── Device structure (forward reference) ── */
struct acm32_eth {
    struct eth_device parent;
    rt_uint8_t        dev_addr[6];
    rt_uint8_t        link_status;
    struct rt_timer   poll_link_timer;
};

static struct acm32_eth eth_dev;

/**
 * @brief lwIP eth_tx interface: send pbuf to Ethernet
 *
 * 1. Get available TX descriptor
 * 2. Copy data from pbuf chain to TX buffer
 * 3. Set descriptor control word and submit to DMA
 * 4. Poll for completion (timeout 100ms)
 *
 * @param dev  rt_device_t device handle
 * @param p    pbuf chain to send
 * @return RT_EOK on success, RT_EBUSY if no descriptor, RT_ETIMEOUT on timeout
 */
static rt_err_t acm32_eth_tx(rt_device_t dev, struct pbuf *p)
{
    rt_uint8_t  idx;
    rt_uint32_t total_len = 0;
    rt_uint8_t *dst;

    /* Get available descriptor */
    idx = get_next_tx_desc();
    if (idx >= TX_DESC_COUNT) {
        return -RT_EBUSY;
    }

    /* Copy from pbuf chain */
    dst = tx_buf[idx];
    for (struct pbuf *q = p; q != RT_NULL; q = q->next) {
        if (total_len + q->len > ETH_FRAME_SIZE) {
            return -RT_ERROR;
        }
        rt_memcpy(dst + total_len, q->payload, q->len);
        total_len += q->len;
    }

    /* Submit to DMA */
    tx_desc[idx].tdes2 = (rt_uint32_t)dst;
    tx_desc[idx].tdes1 = total_len;
    tx_desc[idx].tdes0 = TDES0_OWN | TDES0_IC | TDES0_FS | TDES0_LS
                       | (tx_desc[idx].tdes0 & (TDES0_TER | TDES0_TCH));

    /* Wake TX DMA if suspended */
    if (ETH->DMASR & (1UL << 1)) {
        *((volatile rt_uint32_t *)&ETH->DMATPDR) = 1U;
    }

    /* Poll for completion (OWN cleared by ISR in interrupt mode) */
    rt_tick_t timeout = rt_tick_from_millisecond(100);
    rt_tick_t start   = rt_tick_get();
    while (tx_desc[idx].tdes0 & TDES0_OWN) {
        if (rt_tick_get() - start > timeout) {
            return -RT_ETIMEOUT;
        }
    }

    return RT_EOK;
}

/**
 * @brief lwIP eth_rx interface: receive frame from Ethernet
 *
 * Iterate RX descriptor ring, read DMA-written frames and wrap in pbuf.
 *
 * @param dev rt_device_t device handle
 * @return received pbuf, or NULL if no frame
 */
static struct pbuf *acm32_eth_rx(rt_device_t dev)
{
    rt_uint8_t    idx;
    rt_uint32_t   frame_len;
    struct pbuf  *p;

    /* Iterate RX descriptor ring */
    for (rt_uint8_t i = 0; i < RX_DESC_COUNT; i++) {
        idx = get_next_rx_desc();
        if (idx >= RX_DESC_COUNT) {
            break;  /* No new frame */
        }

        /* Check error flag */
        if (rx_desc[idx].rdes0 & RDES0_ES) {
            /* Error frame: return descriptor to DMA */
            rx_desc[idx].rdes0 = RDES0_OWN;
            continue;
        }

        /* Get frame length (excluding CRC) */
        frame_len = (rx_desc[idx].rdes0 & RDES0_FL_MSK) >> RDES0_FL_POS;
        if (frame_len >= 4) {
            frame_len -= 4;
        }

        /* Allocate pbuf */
        p = pbuf_alloc(PBUF_RAW, frame_len, PBUF_RAM);
        if (p == RT_NULL) {
            /* Return descriptor, discard frame */
            rx_desc[idx].rdes0 = RDES0_OWN;
            continue;
        }

        /* Copy data */
        pbuf_take(p, rx_buf[idx], frame_len);

        /* Return descriptor to DMA */
        rx_desc[idx].rdes0 = RDES0_OWN;

        /* Wake RX DMA if suspended */
        if (ETH->DMASR & ((1UL << 7) | (1UL << 8))) {
            *((volatile rt_uint32_t *)&ETH->DMARPDR) = 1U;
        }

        return p;
    }

    return RT_NULL;
}

/**
 * @brief PHY link status polling timer callback
 *
 * Runs every PHY_POLL_INTERVAL ms, checks BSR link status changes.
 */
static void phy_poll_link(void *parameter)
{
    struct acm32_eth *eth = (struct acm32_eth *)parameter;
    rt_uint16_t bsr;
    rt_uint8_t  new_status;

    if (!eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr)) {
        return;  /* SMI read failed, keep current status */
    }

    new_status = (bsr & PHY_BSR_LINK_UP) ? 1 : 0;
    if (new_status != eth->link_status) {
        eth->link_status = new_status;
        eth_device_linkchange(&eth->parent, new_status ? RT_TRUE : RT_FALSE);
    }
}

/**
 * @brief Wait for PHY auto-negotiation to complete and link up
 * @param timeout_ms timeout in ms
 * @return RT_TRUE if link established, RT_FALSE on timeout
 */
static rt_bool_t phy_wait_link_up(rt_uint32_t timeout_ms)
{
    rt_uint16_t bsr;
    rt_tick_t   start = rt_tick_get();
    rt_tick_t   timeout = rt_tick_from_millisecond(timeout_ms);

    while (rt_tick_get() - start < timeout) {
        if (eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr)) {
            if ((bsr & PHY_BSR_AN_COMPLETE) && (bsr & PHY_BSR_LINK_UP)) {
                return RT_TRUE;
            }
        }
        rt_thread_mdelay(10);
    }
    return RT_FALSE;
}

/**
 * @brief ETH driver initialization entry (auto-called via INIT_DEVICE_EXPORT)
 *
 * Init sequence:
 * 1. GPIO + PHY reset
 * 2. Clock enable + RMII config
 * 3. Descriptor chain init
 * 4. eth_init() SDK one-shot init
 * 5. PHY soft reset + auto-negotiation
 * 6. Wait for link up
 * 7. Configure MAC address
 * 8. Enable MAC TX/RX + start DMA
 * 9. Register interrupt callback (managed by SDK built-in ISR)
 * 10. Register to RT-Thread eth_device framework
 * 11. Start PHY link polling timer
 */
static int rt_hw_eth_init(void)
{
    rt_err_t     result;
    rt_uint16_t  bcr;

    /* 1. GPIO init */
    gpio_init_eth_rmii();

    /* 2. Clock init */
    eth_clock_init();

    /* 3. Descriptor chain init */
    desc_init_tx();
    desc_init_rx();

    /* 4. SDK one-shot init */
    eth_init_config_t eth_cfg = {
        .phy_interface    = ETH_PHY_RMII,
        .smi_clk_range    = ETH_SMI_CR_150_250MHZ,
        .speed            = ETH_SPEED_100M,
        .duplex           = ETH_DUPLEX_FULL,
        .checksum_offload = ETH_CHECKSUM_NONE,
        .tx_desc_base     = tx_desc,
        .rx_desc_base     = rx_desc,
        .tx_desc_count    = TX_DESC_COUNT,
        .rx_desc_count    = RX_DESC_COUNT,
        .enhanced_desc    = false,
    };

    if (!eth_init(&eth_cfg)) {
        rt_kprintf("[eth] eth_init failed\n");
        return -RT_ERROR;
    }

    /* 5. PHY soft reset + start auto-negotiation */
    eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_RESET);
    /* Poll for reset completion (BCR bit15 self-clears) */
    {
        rt_tick_t rst_start = rt_tick_get();
        do {
            eth_phy_read(PHY_ADDR, PHY_REG_BCR, &bcr);
        } while ((bcr & PHY_BCR_RESET)
                 && (rt_tick_get() - rst_start < rt_tick_from_millisecond(100)));
    }

    eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_AN_EN | PHY_BCR_AN_RESTART);

    /* 6. Wait for link up */
    if (!phy_wait_link_up(PHY_LINK_TIMEOUT)) {
        rt_kprintf("[eth] PHY link timeout\n");
        /* Do not return error: allow later link recovery */
    }
    eth_dev.link_status = 1;

    /* 7. Configure MAC address (default 00:11:22:33:44:55) */
    eth_dev.dev_addr[0] = 0x00;
    eth_dev.dev_addr[1] = 0x11;
    eth_dev.dev_addr[2] = 0x22;
    eth_dev.dev_addr[3] = 0x33;
    eth_dev.dev_addr[4] = 0x44;
    eth_dev.dev_addr[5] = 0x55;

    eth_mac_addr_config_t mac_cfg = {
        .addr   = {
            eth_dev.dev_addr[0], eth_dev.dev_addr[1], eth_dev.dev_addr[2],
            eth_dev.dev_addr[3], eth_dev.dev_addr[4], eth_dev.dev_addr[5]
        },
        .enable = true,
    };
    eth_config_mac_addr(0, &mac_cfg);

    /* 8. Enable MAC TX/RX + start DMA */
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();

    /* 9. ISR managed by SDK built-in handler (polling mode for now) */

    /* 10. Register to RT-Thread eth_device framework */
    eth_dev.parent.parent.init       = RT_NULL;
    eth_dev.parent.parent.open       = RT_NULL;
    eth_dev.parent.parent.close      = RT_NULL;
    eth_dev.parent.parent.read       = RT_NULL;
    eth_dev.parent.parent.write      = RT_NULL;
    eth_dev.parent.parent.control    = RT_NULL;
    eth_dev.parent.parent.user_data  = RT_NULL;
    eth_dev.parent.eth_rx            = acm32_eth_rx;
    eth_dev.parent.eth_tx            = acm32_eth_tx;

    result = eth_device_init(&eth_dev.parent, "e0");
    if (result != RT_EOK) {
        rt_kprintf("[eth] eth_device_init failed: %d\n", result);
        return result;
    }

    /* 11. Start PHY link polling timer */
    rt_timer_init(&eth_dev.poll_link_timer, "eth_link",
                  phy_poll_link, &eth_dev,
                  rt_tick_from_millisecond(PHY_POLL_INTERVAL),
                  RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(&eth_dev.poll_link_timer);

    /* Notify framework device is ready */
    eth_device_ready(&eth_dev.parent);

    rt_kprintf("[eth] initialized, MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth_dev.dev_addr[0], eth_dev.dev_addr[1], eth_dev.dev_addr[2],
               eth_dev.dev_addr[3], eth_dev.dev_addr[4], eth_dev.dev_addr[5]);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_eth_init);

#endif /* BSP_USING_ETH */
