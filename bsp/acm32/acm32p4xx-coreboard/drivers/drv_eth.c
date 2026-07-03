/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-06-30     ACM32        first version
 * 2026-07-03     ACM32        adapt to new SDK eth.h API (delegated desc mgmt)
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

/* ── Current descriptor indices (managed via SDK API pointers) ── */
static rt_uint8_t tx_idx;
static rt_uint8_t rx_idx;

/* ── Device structure ── */
struct acm32_eth {
    struct eth_device parent;
    rt_uint8_t        dev_addr[6];
    rt_uint8_t        link_status;
    struct rt_timer   poll_link_timer;
};

static struct acm32_eth eth_dev;

/*===========================================================================
 * 底层初始化函数
 *===========================================================================*/

/**
 * @brief 初始化 RMII GPIO 引脚并复位 PHY
 *
 * 复位序列：PC0 拉低 ≥25ms → 拉高 → 等待 ≥300ms（LAN8720A PLL 锁定）
 */
static void gpio_init_eth_rmii(void)
{
    /* 逐一配置 RMII 功能引脚（全部 AF6） */
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

    /* LAN8720A 硬件复位：PC0 低有效 */
    gpio_init(ETH_RST_PIN);
    gpio_set_dir(ETH_RST_PIN, GPIO_OUT);
    gpio_put(ETH_RST_PIN, false);
    rt_thread_mdelay(25);
    gpio_put(ETH_RST_PIN, true);
    rt_thread_mdelay(300);
}

/**
 * @brief 使能 ETH 所需时钟并配置 RMII 接口
 *
 * 顺序约束：先使能 CLK_SYSCFG 再设置 PHY 接口，
 * 设置必须在 ETH 时钟使能之前完成。
 */
static void eth_clock_init(void)
{
    clock_periph_enable(CLK_SYSCFG);
    syscfg_eth_set_phy_interface(ETH_PHY_RMII);
    clock_periph_enable(CLK_ETH_MAC);
    clock_periph_enable(CLK_ETH_TX);
    clock_periph_enable(CLK_ETH_RX);
}

/*===========================================================================
 * 描述符管理（委托给 SDK：eth_desc_init_xxx / eth_get_next_xxx_desc）
 *
 * SDK 负责：
 *   - eth_desc_init_tx() / eth_desc_init_rx()：初始化描述符链表
 *   - eth_get_next_tx_desc()：返回 CPU 可用 TX 描述符索引
 *   - eth_get_next_rx_desc()：返回 DMA 已写入 RX 描述符索引
 *   - eth_transmit_frame()：设置描述符控制字并提交给 DMA
 *   - eth_receive_frame()：读取帧、清除 OWN、归还描述符
 *===========================================================================*/

/*===========================================================================
 * 控制回调
 *===========================================================================*/

/**
 * @brief eth_device 框架控制回调
 */
static rt_err_t eth_control(rt_device_t dev, int cmd, void *args)
{
    struct acm32_eth *eth = (struct acm32_eth *)dev;

    switch (cmd) {
    case NIOCTL_GADDR:
        rt_memcpy(args, eth->dev_addr, 6);
        return RT_EOK;
    default:
        return -RT_EINVAL;
    }
}

/*===========================================================================
 * 发送
 *===========================================================================*/

/**
 * @brief lwIP eth_tx 接口：发送 pbuf
 *
 * 1. 通过 SDK 获取可用 TX 描述符
 * 2. 从 pbuf 链拷贝数据
 * 3. SDK eth_transmit_frame() 设置描述符并提交 DMA
 * 4. 轮询 OWN 位等待完成（超时 ETH_TX_POLL_TIMEOUT ms）
 */
static rt_err_t acm32_eth_tx(rt_device_t dev, struct pbuf *p)
{
    rt_uint8_t  idx;
    rt_uint32_t total_len = 0;
    rt_uint8_t *dst;

    idx = eth_get_next_tx_desc(tx_desc, TX_DESC_COUNT, &tx_idx);
    if (idx >= TX_DESC_COUNT) {
        return -RT_EBUSY;
    }

    /* 拷贝 pbuf 链数据到 TX 缓冲区 */
    dst = tx_buf[idx];
    for (struct pbuf *q = p; q != RT_NULL; q = q->next) {
        if (total_len + q->len > ETH_FRAME_SIZE) {
            return -RT_ERROR;
        }
        rt_memcpy(dst + total_len, q->payload, q->len);
        total_len += q->len;
    }

    /* SDK 设置描述符（OWN/FS/LS/IC）并唤醒 DMA */
    if (!eth_transmit_frame(&tx_desc[idx], dst, total_len)) {
        return -RT_ERROR;
    }

    /* 轮询等待 DMA 发送完成 */
    rt_tick_t timeout = rt_tick_from_millisecond(ETH_TX_POLL_TIMEOUT);
    rt_tick_t start   = rt_tick_get();
    while (tx_desc[idx].tdes0 & ETH_TDES0_OWN) {
        if (rt_tick_get() - start > timeout) {
            return -RT_ETIMEOUT;
        }
    }

    return RT_EOK;
}

/*===========================================================================
 * 接收
 *===========================================================================*/

/**
 * @brief lwIP eth_rx 接口：接收帧
 *
 * 1. 通过 SDK 获取 DMA 已写入的 RX 描述符
 * 2. SDK eth_receive_frame() 校验并拷贝帧数据
 * 3. 封装为 pbuf 返回
 */
static struct pbuf *acm32_eth_rx(rt_device_t dev)
{
    rt_uint8_t  idx;
    rt_uint32_t frame_len;
    struct pbuf *p;

    idx = eth_get_next_rx_desc(rx_desc, RX_DESC_COUNT, &rx_idx);
    if (idx >= RX_DESC_COUNT) {
        return RT_NULL;
    }

    /* SDK 校验错误、提取长度、拷贝数据、归还描述符 */
    if (!eth_receive_frame(&rx_desc[idx], rx_buf[idx], &frame_len)) {
        return RT_NULL;
    }

    /* 帧长度合理性检查 */
    if (frame_len < ETH_MIN_FRAME_LEN || frame_len > ETH_MAX_FRAME_LEN) {
        return RT_NULL;
    }

    /* 分配 pbuf 并拷贝数据 */
    p = pbuf_alloc(PBUF_RAW, frame_len, PBUF_RAM);
    if (p == RT_NULL) {
        return RT_NULL;
    }
    pbuf_take(p, rx_buf[idx], frame_len);

    return p;
}

/*===========================================================================
 * PHY 管理
 *===========================================================================*/

/**
 * @brief PHY 链路轮询定时器回调
 *
 * 每 PHY_POLL_INTERVAL ms 读取 BSR 寄存器检测链路状态变化。
 */
static void phy_poll_link(void *parameter)
{
    struct acm32_eth *eth = (struct acm32_eth *)parameter;
    rt_uint16_t bsr;
    rt_uint8_t  new_status;

    if (!eth_phy_read(PHY_ADDR, PHY_REG_BSR, &bsr)) {
        return;
    }

    new_status = (bsr & PHY_BSR_LINK_UP) ? 1 : 0;
    if (new_status != eth->link_status) {
        eth->link_status = new_status;
        eth_device_linkchange(&eth->parent, new_status ? RT_TRUE : RT_FALSE);
    }
}

/**
 * @brief 等待 PHY 自动协商完成并建立链路
 *
 * @param timeout_ms  超时时间（ms）
 * @return RT_TRUE 链路已建立，RT_FALSE 超时
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

/*===========================================================================
 * 驱动初始化入口
 *===========================================================================*/

/**
 * @brief ETH 驱动初始化（INIT_DEVICE_EXPORT 自动调用）
 *
 * 初始化序列：
 *   1. GPIO + PHY 复位
 *   2. 时钟 + RMII 配置
 *   3. SDK 描述符链表初始化
 *   4. eth_init() 一键初始化
 *   5. PHY 软复位 + 自动协商
 *   6. 等待链路建立
 *   7. 配置 MAC 地址
 *   8. 启动 MAC + DMA 引擎
 *   9. 注册 eth_device
 *  10. 启动链路轮询定时器
 */
static int rt_hw_eth_init(void)
{
    rt_err_t    result;
    rt_uint16_t bcr;

    /* 1. GPIO + PHY 复位 */
    gpio_init_eth_rmii();

    /* 2. 时钟 + RMII */
    eth_clock_init();

    /* 3. SDK 描述符链表初始化 */
    eth_desc_init_tx(tx_desc, TX_DESC_COUNT, tx_buf, ETH_FRAME_SIZE, &tx_idx);
    eth_desc_init_rx(rx_desc, RX_DESC_COUNT, rx_buf, ETH_FRAME_SIZE, &rx_idx);

    /* 4. eth_init() 一键初始化 MAC + DMA + SMI */
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

    /* 5. PHY 软复位 + 自动协商 */
    eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_RESET);
    {
        rt_tick_t rst_start = rt_tick_get();
        do {
            eth_phy_read(PHY_ADDR, PHY_REG_BCR, &bcr);
        } while ((bcr & PHY_BCR_RESET)
                 && (rt_tick_get() - rst_start < rt_tick_from_millisecond(100)));
    }
    eth_phy_write(PHY_ADDR, PHY_REG_BCR, PHY_BCR_AN_EN | PHY_BCR_AN_RESTART);

    /* 6. 等待链路建立 */
    eth_dev.link_status = phy_wait_link_up(PHY_LINK_TIMEOUT) ? 1 : 0;
    if (!eth_dev.link_status) {
        rt_kprintf("[eth] PHY link timeout\n");
    }

    /* 7. 配置 MAC 地址 */
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

    /* 8. 启动 MAC + DMA */
    eth_mac_tx_enable();
    eth_mac_rx_enable();
    eth_start_tx();
    eth_start_rx();

    /* 9. 注册到 RT-Thread eth_device 框架 */
    eth_dev.parent.parent.init       = RT_NULL;
    eth_dev.parent.parent.open       = RT_NULL;
    eth_dev.parent.parent.close      = RT_NULL;
    eth_dev.parent.parent.read       = RT_NULL;
    eth_dev.parent.parent.write      = RT_NULL;
    eth_dev.parent.parent.control    = eth_control;
    eth_dev.parent.parent.user_data  = RT_NULL;
    eth_dev.parent.eth_rx            = acm32_eth_rx;
    eth_dev.parent.eth_tx            = acm32_eth_tx;

    result = eth_device_init(&eth_dev.parent, "e0");
    if (result != RT_EOK) {
        rt_kprintf("[eth] eth_device_init failed: %d\n", result);
        return result;
    }

    /* 10. 启动链路轮询定时器 */
    rt_timer_init(&eth_dev.poll_link_timer, "eth_link",
                  phy_poll_link, &eth_dev,
                  rt_tick_from_millisecond(PHY_POLL_INTERVAL),
                  RT_TIMER_FLAG_PERIODIC);
    rt_timer_start(&eth_dev.poll_link_timer);

    eth_device_ready(&eth_dev.parent);

    rt_kprintf("[eth] initialized, MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               eth_dev.dev_addr[0], eth_dev.dev_addr[1], eth_dev.dev_addr[2],
               eth_dev.dev_addr[3], eth_dev.dev_addr[4], eth_dev.dev_addr[5]);

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_eth_init);

#endif /* BSP_USING_ETH */
