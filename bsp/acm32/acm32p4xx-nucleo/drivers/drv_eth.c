/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-06     Sisyphus     first version, HAL ETH + LAN8720A + zero-copy
 * 2026-07-15     AisinoChip   harden RX pool free/alloc under concurrency
 */

#include <rtthread.h>
#include <rthw.h>
#include <rtdevice.h>
#include <board.h>

#include <netif/ethernetif.h>
#include <lwip/netifapi.h>
#include <lwip/tcpip.h>
#include <lwip/pbuf.h>
#include <netif/etharp.h>
#include <stdint.h>
#ifdef RT_USING_NETDEV
#include <netdev.h>
#endif

#include "drv_eth.h"
#include "hal_efuse.h"
#include "system_accelerate.h"

#define DBG_TAG             "drv.eth"
#define DBG_LVL             DBG_INFO
#include <rtdbg.h>

/* ===== RX Buffer Pool (Zero-Copy) — placed in PSRAM ETH DMA window ===== */

#define ETH_RX_BUF_ITEM_SIZE    ((ETH_RX_BUFFER_SIZE + 31) & ~31)

static ETH_DMADescTypeDef *DMARxDscrTab;
static ETH_DMADescTypeDef *DMATxDscrTab;
static uint8_t *rx_pool_memory;
static uint32_t *rx_pool_bitmap;
static uint8_t *tx_bounce_bufs[ETH_TX_BOUNCE_CNT];
static volatile uint8_t tx_bounce_busy[ETH_TX_BOUNCE_CNT];

static ETH_HandleTypeDef EthHandle;
static ETH_TxPacketConfigTypeDef TxConfig;

/* ===== Device Structure ===== */

struct acm32_eth
{
    struct eth_device parent;           /* inherit from eth_device */
    rt_uint8_t  dev_addr[6];           /* MAC address */
    uint32_t    eth_speed;
    uint32_t    eth_mode;
};

static struct acm32_eth acm32_eth_device;

/* ===== Semaphores ===== */

static rt_sem_t tx_sem;

/* ===== RX allocation status ===== */

typedef enum { RX_ALLOC_OK = 0, RX_ALLOC_ERROR = 1 } RxAllocStatusTypeDef;
static RxAllocStatusTypeDef RxAllocStatus;

/* ===== Diagnostic counters (shown by eth_ifconfig) ===== */

static volatile uint32_t eth_irq_count = 0;
static volatile uint32_t eth_rx_cplt_count = 0;
static volatile uint32_t eth_tx_cplt_count = 0;
static volatile uint32_t eth_rx_alloc_count = 0;
static volatile uint32_t eth_rx_alloc_fail = 0;
static volatile uint32_t eth_rx_read_ok = 0;
static volatile uint32_t eth_rx_err_count = 0;
static volatile uint32_t eth_tx_count = 0;

/* ===== Forward declarations ===== */

static void ethernet_link_thread(void *parameter);
static int  phy_get_link_state(ETH_HandleTypeDef *heth);

/* ===== Rx buffer pool management ===== */
/* Alloc/free run from ETH IRQ context and tcpip thread; must be atomic. */

static void *rx_pool_alloc(void)
{
    int i;
    rt_base_t level;
    void *ret = NULL;

    level = rt_hw_interrupt_disable();
    for (i = 0; i < ETH_RX_BUFFER_CNT; i++)
    {
        if (!(rx_pool_bitmap[i / 32] & (1u << (i % 32))))
        {
            rx_pool_bitmap[i / 32] |= (1u << (i % 32));
            ret = &rx_pool_memory[i * ETH_RX_BUF_ITEM_SIZE];
            break;
        }
    }
    rt_hw_interrupt_enable(level);
    return ret;
}

/* Returns RT_TRUE if buffer was in-use and is now freed; RT_FALSE if double-free */
static rt_bool_t rx_pool_free(void *ptr)
{
    uint8_t *base = (uint8_t *)rx_pool_memory;
    uint32_t offset;
    int idx;
    rt_base_t level;
    rt_bool_t ok = RT_FALSE;

    if (ptr == NULL || base == NULL)
        return RT_FALSE;

    offset = (uint8_t *)ptr - base;
    if ((offset % ETH_RX_BUF_ITEM_SIZE) != 0)
        return RT_FALSE;

    idx = (int)(offset / ETH_RX_BUF_ITEM_SIZE);
    if (idx < 0 || idx >= ETH_RX_BUFFER_CNT)
        return RT_FALSE;

    level = rt_hw_interrupt_disable();
    if (rx_pool_bitmap[idx / 32] & (1u << (idx % 32)))
    {
        rx_pool_bitmap[idx / 32] &= ~(1u << (idx % 32));
        ok = RT_TRUE;
    }
    rt_hw_interrupt_enable(level);
    return ok;
}

static void rx_buf_free_custom(struct pbuf *p)
{
    struct pbuf_custom *custom_pbuf = (struct pbuf_custom *)p;

    /* Detach free hook first to make accidental re-free a no-op path */
    custom_pbuf->custom_free_function = RT_NULL;
    p->next = RT_NULL;

    if (rx_pool_free(custom_pbuf) != RT_TRUE)
    {
        LOG_W("RX pool double-free ignored %p", custom_pbuf);
        return;
    }

    if (RxAllocStatus == RX_ALLOC_ERROR)
    {
        RxAllocStatus = RX_ALLOC_OK;
        /* Wake RX thread to re-allocate buffers for any stuck descriptors */
        eth_device_ready(&acm32_eth_device.parent);
    }
}

/* ===== PHY nRST GPIO clock (BSP_ETH_PHY_RST_PIN -> port) ===== */

static GPIO_TypeDef *eth_phy_rst_port(void)
{
    switch (ETH_PHY_RST_PORT_IDX)
    {
    case 0: return GPIOA;
    case 1: return GPIOB;
    case 2: return GPIOC;
    case 3: return GPIOD;
    case 4: return GPIOE;
    case 5: return GPIOF;
    case 6: return GPIOG;
    case 7: return GPIOH;
    default: return GPIOA;
    }
}

static void eth_phy_rst_clk_enable(void)
{
    switch (ETH_PHY_RST_PORT_IDX)
    {
    case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
    case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
    case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
    case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
    case 4: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
    case 5: __HAL_RCC_GPIOF_CLK_ENABLE(); break;
    case 6: __HAL_RCC_GPIOG_CLK_ENABLE(); break;
    case 7: __HAL_RCC_GPIOH_CLK_ENABLE(); break;
    default: break;
    }
}

/* ===== HAL_ETH_MspInit: GPIO, Clock, NVIC ===== */

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (heth->Init.MediaInterface == HAL_ETH_RMII_MODE)
    {
        /* Enable GPIO clocks (RMII fixed pins + configurable PHY nRST) */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        eth_phy_rst_clk_enable();

        /* PA1(REF_CLK), PA2(MDIO), PA7(CRS_DV) -- AF6, Drive Level 3 */
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_6;
        HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

        /* PB11(TX_EN), PB12(TXD0), PB13(TXD1) -- AF6 */
        GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_6;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* PHY nRST: any GPIO via BSP_ETH_PHY_RST_PIN (pin index) */
        GPIO_InitStruct.Pin = ETH_PHY_RST_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_0;
        HAL_GPIO_Init(eth_phy_rst_port(), &GPIO_InitStruct);

        /* PC1(MDC), PC4(RXD0), PC5(RXD1) -- AF6 */
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_6;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* PHY hardware reset: low 15ms -> high -> wait 20ms */
        HAL_GPIO_WritePin(eth_phy_rst_port(), ETH_PHY_RST_PIN, GPIO_PIN_RESET);
        HAL_Delay(15);
        HAL_GPIO_WritePin(eth_phy_rst_port(), ETH_PHY_RST_PIN, GPIO_PIN_SET);
        HAL_Delay(20);
    }

    /* ETH interrupt: priority 14 (low, does not preempt UART) */
    HAL_NVIC_SetPriority(ETH_IRQn, 0x0E, 0);
    HAL_NVIC_ClearPendingIRQ(ETH_IRQn);
    HAL_NVIC_EnableIRQ(ETH_IRQn);

    /* Enable ETH clocks */
    __HAL_RCC_ETHMAC_CLK_ENABLE();
    __HAL_RCC_ETHTX_CLK_ENABLE();
    __HAL_RCC_ETHRX_CLK_ENABLE();
}

/* ===== ISR Entry ===== */

/*
 * Do NOT use stock HAL_ETH_IRQHandler: it returns after RS or TS alone and can
 * drop a co-pending TX/RX completion in the same IRQ entry.
 */
void ETH_IRQHandler(void)
{
    uint32_t dma_flag;
    uint32_t dma_itsource;

    eth_irq_count++;
    rt_interrupt_enter();

    dma_flag = READ_REG(EthHandle.Instance->DMASR);
    dma_itsource = READ_REG(EthHandle.Instance->DMAIER);

    if (((dma_flag & ETH_DMASR_RS) != 0U) && ((dma_itsource & ETH_DMAIER_RIE) != 0U))
    {
        __HAL_ETH_DMA_CLEAR_IT(&EthHandle, ETH_DMASR_RS | ETH_DMASR_NIS);
        HAL_ETH_RxCpltCallback(&EthHandle);
    }

    if (((dma_flag & ETH_DMASR_TS) != 0U) && ((dma_itsource & ETH_DMAIER_TIE) != 0U))
    {
        __HAL_ETH_DMA_CLEAR_IT(&EthHandle, ETH_DMASR_TS | ETH_DMASR_NIS);
        HAL_ETH_TxCpltCallback(&EthHandle);
    }

    /* Error / AIS path still via HAL (may also see already-cleared RS/TS) */
    if (((dma_flag & ETH_DMASR_AIS) != 0U) && ((dma_itsource & ETH_DMAIER_AISE) != 0U))
    {
        HAL_ETH_IRQHandler(&EthHandle);
    }

    rt_interrupt_leave();
}

/* ===== HAL Callbacks ===== */

void HAL_ETH_RxCpltCallback(ETH_HandleTypeDef *heth)
{
    eth_rx_cplt_count++;
    /* Notify ethernetif RX thread to process the received packet */
    eth_device_ready(&acm32_eth_device.parent);
}

void HAL_ETH_TxCpltCallback(ETH_HandleTypeDef *heth)
{
    eth_tx_cplt_count++;
    /* Release TX descriptors + bounce ownership (TxFreeCallback) */
    HAL_ETH_ReleaseTxPacket(heth);
    rt_sem_release(tx_sem);
}

void HAL_ETH_ErrorCallback(ETH_HandleTypeDef *heth)
{
    uint32_t dma_err = HAL_ETH_GetDMAError(heth);
    if (dma_err & ETH_DMASR_RBUS)
    {
        eth_device_ready(&acm32_eth_device.parent);
    }
    if (dma_err & ETH_DMASR_TBUS)
    {
        rt_sem_release(tx_sem);
    }
    /* Only log fatal errors (FBE, access error, etc.).
     * Non-fatal flags (AIS, ET, RBU, TBUS) are normal during operation. */
    if (dma_err & (ETH_DMA_FLAG_FBE | ETH_DMA_FLAG_READWRITEERROR | ETH_DMA_FLAG_ACCESSERROR))
    {
        LOG_E("ETH fatal: DMA=0x%08X", dma_err);
    }
}

/* ===== RxAllocateCallback: allocate from RX_POOL ===== */

void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    eth_rx_alloc_count++;
    void *pool_buf = rx_pool_alloc();
    if (pool_buf)
    {
        struct pbuf_custom *p = (struct pbuf_custom *)pool_buf;
        /* Clear stale pbuf header from previous life (ref/next/flags) */
        rt_memset(p, 0, sizeof(struct pbuf_custom));
        *buff = (uint8_t *)p + sizeof(struct pbuf_custom);
        p->custom_free_function = rx_buf_free_custom;
        if (pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff,
                                ETH_RX_BUFFER_SIZE - sizeof(struct pbuf_custom)) == NULL)
        {
            rx_pool_free(pool_buf);
            eth_rx_alloc_fail++;
            RxAllocStatus = RX_ALLOC_ERROR;
            *buff = NULL;
            return;
        }
    }
    else
    {
        eth_rx_alloc_fail++;
        RxAllocStatus = RX_ALLOC_ERROR;
        *buff = NULL;
        LOG_W("RX pool exhausted!");
    }
}

/* ===== RxLinkCallback: chain pbuf fragments ===== */

void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd, uint8_t *buff,
                            uint16_t Length)
{
    struct pbuf **ppStart = (struct pbuf **)pStart;
    struct pbuf **ppEnd = (struct pbuf **)pEnd;
    struct pbuf *p;

    /* ETH DMA wrote payload in PSRAM; drop stale D-Cache lines before CPU reads */
    if (buff != RT_NULL && Length > 0)
        System_InvalidateDAccelerate_by_Addr((volatile void *)buff, (int32_t)Length);

    p = (struct pbuf *)(buff - sizeof(struct pbuf_custom));
    /* Keep ref/type/flags from pbuf_alloced_custom; only fill length chain */
    p->next = NULL;
    p->len = Length;
    p->tot_len = 0;

    if (!*ppStart)
    {
        *ppStart = p;
    }
    else
    {
        (*ppEnd)->next = p;
    }
    *ppEnd = p;

    /* Same tot_len accumulation as STM32 zero-copy ETH examples */
    {
        struct pbuf *q;
        for (q = *ppStart; q != NULL; q = q->next)
            q->tot_len = (u16_t)(q->tot_len + Length);
    }
}

/* ===== TxFreeCallback: release pbuf after TX ===== */

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    int i;

    if (buff == RT_NULL)
        return;

    for (i = 0; i < ETH_TX_BOUNCE_CNT; i++)
    {
        if ((uint8_t *)buff == tx_bounce_bufs[i])
        {
            tx_bounce_busy[i] = 0;
            return;
        }
    }
    pbuf_free((struct pbuf *)buff);
}

static int eth_tx_alloc_bounce(void)
{
    int i;
    rt_base_t level;

    level = rt_hw_interrupt_disable();
    for (i = 0; i < ETH_TX_BOUNCE_CNT; i++)
    {
        if (!tx_bounce_busy[i] && tx_bounce_bufs[i] != RT_NULL)
        {
            tx_bounce_busy[i] = 1;
            rt_hw_interrupt_enable(level);
            return i;
        }
    }
    rt_hw_interrupt_enable(level);
    return -1;
}

/*
 * RMII RX clock delay block (same programming as HAL private ETH_DelayBlockConfig).
 * HAL_ETH_Init hardcodes (10,15); LAN8720 boards often need a different tap.
 */
static void eth_rx_delay_config(uint32_t unit, uint32_t sel)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    SYSCFG->SYSCR |= SYSCFG_SYSCR_ETHMAC_RX_DLYSEL;
    ETH_DLYB->CR = DLYB_CR_DEN;
    ETH_DLYB->CR |= DLYB_CR_SEN;
    ETH_DLYB->CFGR = ((unit << DLYB_CFGR_UNIT_Pos) & DLYB_CFGR_UNIT_Msk) |
                     ((sel << DLYB_CFGR_SEL_Pos) & DLYB_CFGR_SEL_Msk);
    HAL_SimpleDelay(10);
    ETH_DLYB->CR &= ~DLYB_CR_SEN;
    LOG_I("ETH RX delay unit=%u sel=%u CFGR=0x%08X",
          (unsigned)unit, (unsigned)sel, (unsigned)ETH_DLYB->CFGR);
}

/* ===== PHY Operations ===== */

static HAL_StatusTypeDef phy_soft_reset(ETH_HandleTypeDef *heth)
{
    uint32_t tick_start;
    uint32_t value;

    if (HAL_ETH_WritePHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BCR,
                                 PHY_BCR_RESET) != HAL_OK)
        return HAL_ERROR;

    tick_start = HAL_GetTick();
    do {
        if ((HAL_GetTick() - tick_start) > 500U)
            return HAL_TIMEOUT;
        if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BCR,
                                    &value) != HAL_OK)
            return HAL_ERROR;
    } while (value & PHY_BCR_RESET);

    /* Advertise 10/100 H/F + restart AN (works for LAN8720 / SZ18201 / generic) */
    if (HAL_ETH_WritePHYRegister(heth, ETH_PHY_ADDR, PHY_REG_ANAR,
                                 (uint32_t)(PHY_ANAR_10HD | PHY_ANAR_10FD |
                                            PHY_ANAR_100HD | PHY_ANAR_100FD |
                                            0x0001U)) != HAL_OK)
        return HAL_ERROR;

    if (HAL_ETH_WritePHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BCR,
                                 (uint32_t)(PHY_BCR_AN_EN | PHY_BCR_AN_RESTART)) != HAL_OK)
        return HAL_ERROR;

    return HAL_OK;
}

static HAL_StatusTypeDef phy_get_id(ETH_HandleTypeDef *heth,
                                    uint16_t *id1, uint16_t *id2)
{
    uint32_t val;
    if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_ID1, &val) != HAL_OK)
        return HAL_ERROR;
    *id1 = (uint16_t)val;

    if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_ID2, &val) != HAL_OK)
        return HAL_ERROR;
    *id2 = (uint16_t)val;

    return HAL_OK;
}

/* Resolve speed/duplex from IEEE ANAR∩ANLPAR (portable across PHYs). */
static int phy_state_from_anlpar(uint32_t anar, uint32_t anlpar)
{
    uint32_t ab = anar & anlpar;

    if (ab & PHY_ANAR_100FD)
        return PHY_LINK_100M_FULL_DUPLEX;
    if (ab & PHY_ANAR_100HD)
        return PHY_LINK_100M_HALF_DUPLEX;
    if (ab & PHY_ANAR_10FD)
        return PHY_LINK_10M_FULL_DUPLEX;
    if (ab & PHY_ANAR_10HD)
        return PHY_LINK_10M_HALF_DUPLEX;
    return PHY_LINK_100M_FULL_DUPLEX;
}

static int phy_state_from_lan8720_scsr(uint32_t scsr)
{
    switch (scsr & PHY_SCSR_SPEED_MASK)
    {
    case PHY_SCSR_10HD:  return PHY_LINK_10M_HALF_DUPLEX;
    case PHY_SCSR_100HD: return PHY_LINK_100M_HALF_DUPLEX;
    case PHY_SCSR_10FD:  return PHY_LINK_10M_FULL_DUPLEX;
    case PHY_SCSR_100FD: return PHY_LINK_100M_FULL_DUPLEX;
    default:             return -1;
    }
}

static int phy_get_link_state(ETH_HandleTypeDef *heth)
{
    uint32_t bsr, bcr, anar, anlpar, scsr;
    int state;
    uint16_t id1 = 0, id2 = 0;

    if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BSR, &bsr) != HAL_OK)
        return PHY_LINK_DOWN;

    /* Latch: some PHYs need double-read of BSR for sticky link bit */
    if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BSR, &bsr) != HAL_OK)
        return PHY_LINK_DOWN;

    if (!(bsr & PHY_BSR_LINK_UP))
        return PHY_LINK_DOWN;

    if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_BCR, &bcr) != HAL_OK)
        return PHY_LINK_DOWN;

    if (bcr & PHY_BCR_AN_EN)
    {
        if (!(bsr & PHY_BSR_AN_COMPLETE))
            return PHY_LINK_AUTO_NEGOTIATION;

        /*
         * Always prefer IEEE ANAR∩ANLPAR for speed/duplex.
         * Do NOT override with LAN8720 SCSR: under load / mid-AN it can report
         * a wrong mode and force MAC half while PHY is full (LAN8720-only fail;
         * SZ18201 never took that path and worked).
         */
        if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_ANAR, &anar) == HAL_OK &&
            HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_ANLPAR, &anlpar) == HAL_OK &&
            (anar & anlpar & (PHY_ANAR_10HD | PHY_ANAR_10FD |
                              PHY_ANAR_100HD | PHY_ANAR_100FD)) != 0U)
        {
            return phy_state_from_anlpar(anar, anlpar);
        }

        /* Fallback only when ANLPAR empty: LAN8720 SCSR */
        if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_SCSR, &scsr) == HAL_OK)
        {
            state = phy_state_from_lan8720_scsr(scsr);
            if (state >= 0)
                return state;
        }
        return PHY_LINK_100M_FULL_DUPLEX;
    }

    if (bcr & PHY_BCR_SPEED_100)
        return (bcr & PHY_BCR_FULL_DUPLEX) ? PHY_LINK_100M_FULL_DUPLEX
                                           : PHY_LINK_100M_HALF_DUPLEX;
    return (bcr & PHY_BCR_FULL_DUPLEX) ? PHY_LINK_10M_FULL_DUPLEX
                                       : PHY_LINK_10M_HALF_DUPLEX;
}

/* ===== ETH TX callback (eth_device -> low_level_output) ===== */
/*
 * Async IT TX + bounce ring in SRAM1.
 * Do NOT busy-poll here: etx and erx share priority; spinning starves RX
 * and MAC drops frames under load (host sees ~256KB then tcp write failed).
 */
static rt_err_t rt_acm32_eth_tx(rt_device_t dev, struct pbuf *p)
{
    ETH_BufferTypeDef Txbuffer;
    rt_err_t errval = RT_EOK;
    u16_t copy_len;
    int bi;
    uint8_t *bounce;

    RT_ASSERT(p != RT_NULL);

    eth_tx_count++;

    if (p->tot_len == 0 || p->tot_len > ETH_TX_BOUNCE_SIZE)
        return -RT_ERROR;

    for (;;)
    {
        bi = eth_tx_alloc_bounce();
        if (bi >= 0)
            break;
        /* Sleep until TxCplt frees a slot (allows erx to run) */
        if (rt_sem_take(tx_sem, rt_tick_from_millisecond(ETHIF_TX_TIMEOUT)) != RT_EOK)
            return -RT_ETIMEOUT;
    }
    bounce = tx_bounce_bufs[bi];

    copy_len = pbuf_copy_partial(p, bounce, p->tot_len, 0);
    if (copy_len != p->tot_len)
    {
        tx_bounce_busy[bi] = 0;
        return -RT_ERROR;
    }

    rt_memset(&Txbuffer, 0, sizeof(Txbuffer));
    Txbuffer.buffer = bounce;
    Txbuffer.len = p->tot_len;
    Txbuffer.next = NULL;

    TxConfig.Length = p->tot_len;
    TxConfig.TxBuffer = &Txbuffer;
    TxConfig.pData = (uint32_t *)bounce;

    do {
        if (HAL_ETH_Transmit_IT(&EthHandle, &TxConfig) == HAL_OK)
        {
            errval = RT_EOK;
        }
        else if (EthHandle.ErrorCode & HAL_ETH_ERROR_BUSY)
        {
            if (rt_sem_take(tx_sem, rt_tick_from_millisecond(ETHIF_TX_TIMEOUT)) != RT_EOK)
            {
                tx_bounce_busy[bi] = 0;
                return -RT_ETIMEOUT;
            }
            errval = -RT_EBUSY;
        }
        else
        {
            tx_bounce_busy[bi] = 0;
            errval = -RT_ERROR;
        }
    } while (errval == -RT_EBUSY);

    return errval;
}

/* ===== ETH RX callback (eth_device -> low_level_input) ===== */

static struct pbuf *rt_acm32_eth_rx(rt_device_t dev)
{
    struct pbuf *p = NULL;

    (void)dev;
    HAL_ETH_ReadData(&EthHandle, (void **)&p);
    if (p != RT_NULL)
        eth_rx_read_ok++;
    return p;
}

/* ===== eth_device control callback ===== */

static rt_err_t rt_acm32_eth_control(rt_device_t dev, int cmd, void *args)
{
    switch (cmd)
    {
    case NIOCTL_GADDR:
        rt_memcpy(args, acm32_eth_device.dev_addr, 6);
        return RT_EOK;

    case RT_DEVICE_CTRL_CONFIG:
        return RT_EOK;

    default:
        return -RT_ENOSYS;
    }
}

/* ===== ETH hardware init ===== */

static rt_err_t rt_acm32_eth_init(rt_device_t dev)
{
    uint8_t macaddress[6];

    /* Generate unique MAC from chip EFUSE.
     * OUI prefix 02:00:00 (locally administered), last 3 bytes from EFUSE data.
     * Prefer validated ChipSN; fall back to raw EFUSE bytes if validation fails
     * (e.g. unprogrammed EFUSE on dev boards). */
    {
        uint8_t chipsn[16] = {0};
        if (System_Get_ChipSN(chipsn) == HAL_OK)
        {
            macaddress[0] = 0x02;
            macaddress[1] = 0x00;
            macaddress[2] = 0x00;
            macaddress[3] = chipsn[0] ^ chipsn[3];
            macaddress[4] = chipsn[1] ^ chipsn[4];
            macaddress[5] = chipsn[2] ^ chipsn[5];
        }
        else
        {
            /* EFUSE validation failed — try raw EFUSE bytes for entropy */
            uint8_t raw[16] = {0};
            HAL_EFUSE_ReadBytes(EFUSE1, 0x40, raw, 13, 500);

            macaddress[0] = 0x02;
            macaddress[1] = 0x00;
            macaddress[2] = 0x00;
            macaddress[3] = raw[0] ^ raw[3] ^ raw[6] ^ raw[9];
            macaddress[4] = raw[1] ^ raw[4] ^ raw[7] ^ raw[10];
            macaddress[5] = raw[2] ^ raw[5] ^ raw[8] ^ raw[12];

            /* If all EFUSE bytes are zero (unprogrammed), use a fixed default */
            if (macaddress[3] == 0 && macaddress[4] == 0 && macaddress[5] == 0)
            {
                LOG_W("EFUSE unprogrammed, using default MAC");
                macaddress[3] = 0x33;
                macaddress[4] = 0x44;
                macaddress[5] = 0x55;
            }
            else
            {
                LOG_I("MAC from raw EFUSE (ChipSN validation failed)");
            }
        }
    }
    uint16_t phy_id1, phy_id2;
    int phy_state;
    uint32_t wait;
    uint32_t speed, duplex;
    ETH_MACConfigTypeDef MACConf = {0};

    /* Copy MAC address to device struct and sync to lwIP netif / netdev */
    rt_memcpy(acm32_eth_device.dev_addr, macaddress, 6);
    if (acm32_eth_device.parent.netif != RT_NULL)
    {
        rt_memcpy(acm32_eth_device.parent.netif->hwaddr, macaddress, 6);
#ifdef RT_USING_NETDEV
        struct netdev *nd = netdev_get_by_name("e0");
        if (nd != RT_NULL)
        {
            rt_memcpy(nd->hwaddr, macaddress, 6);
            nd->hwaddr_len = 6;
        }
#endif
    }

    LOG_I("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
          macaddress[0], macaddress[1], macaddress[2],
          macaddress[3], macaddress[4], macaddress[5]);

    /* Hybrid DMA layout:
     *   SRAM1 top: TX/RX descriptors + TX bounce ring
     *   PSRAM:     RX zero-copy pool
     */
    {
        size_t desc_size = sizeof(ETH_DMADescTypeDef) * (ETH_RX_DESC_CNT + ETH_TX_DESC_CNT);
        size_t bounce_size = (size_t)ETH_TX_BOUNCE_CNT * ETH_TX_BOUNCE_SIZE;
        size_t sram_need = ((desc_size + bounce_size + 31U) & ~31U);
        size_t pool_size = ETH_RX_POOL_SIZE;
        size_t bitmap_size = ((ETH_RX_BUFFER_CNT + 31) / 32) * sizeof(uint32_t);
        uintptr_t sram_base = (ETH_SRAM1_DESC_BASE + 31U) & ~31U;
        uintptr_t psram_base = (ETH_DMA_BUF_BASE + 31U) & ~31U;
        uint8_t *sbuf;
        uint8_t *pbuf;
        size_t off;
        int i;

#if defined(__ARMCC_VERSION)
        {
            extern const uint32_t Image$$RW_DTCM$$ZI$$Limit;
            uintptr_t zi = (uintptr_t)&Image$$RW_DTCM$$ZI$$Limit;
            if (zi > sram_base)
            {
                LOG_E("BSS/ZI 0x%08X overlaps ETH SRAM1 0x%08X",
                      (unsigned)zi, (unsigned)sram_base);
                return -RT_ENOMEM;
            }
        }
#endif
        if (sram_base + sram_need > ETH_DMA_SRAM_END)
        {
            LOG_E("ETH SRAM1 no room: base=0x%08X need=%u end=0x%08X",
                  (unsigned)sram_base, (unsigned)sram_need, (unsigned)ETH_DMA_SRAM_END);
            return -RT_ENOMEM;
        }
        if (psram_base + pool_size + bitmap_size > ETH_DMA_BUF_END)
        {
            LOG_E("ETH PSRAM no room: base=0x%08X need=%u end=0x%08X",
                  (unsigned)psram_base, (unsigned)(pool_size + bitmap_size),
                  (unsigned)ETH_DMA_BUF_END);
            return -RT_ENOMEM;
        }

        sbuf = (uint8_t *)sram_base;
        rt_memset(sbuf, 0, sram_need);
        DMATxDscrTab = (ETH_DMADescTypeDef *)sbuf;
        DMARxDscrTab = (ETH_DMADescTypeDef *)(sbuf + sizeof(ETH_DMADescTypeDef) * ETH_TX_DESC_CNT);
        for (i = 0; i < ETH_TX_BOUNCE_CNT; i++)
        {
            tx_bounce_bufs[i] = sbuf + desc_size + (size_t)i * ETH_TX_BOUNCE_SIZE;
            tx_bounce_busy[i] = 0;
        }

        pbuf = (uint8_t *)psram_base;
        rt_memset(pbuf, 0, pool_size + bitmap_size);
        System_CleanDAccelerate_by_Addr((volatile void *)pbuf, (int32_t)(pool_size + bitmap_size));
        rx_pool_memory = pbuf;
        off = pool_size;
        rx_pool_bitmap = (uint32_t *)(pbuf + off);

        LOG_I("ETH DMA sram=%p (%u) psram=%p rx_pool=%u bounce=%ux%u",
              (void *)sbuf, (unsigned)sram_need,
              (void *)pbuf, (unsigned)pool_size,
              (unsigned)ETH_TX_BOUNCE_CNT, (unsigned)ETH_TX_BOUNCE_SIZE);
    }

    /* Configure ETH handle */
    EthHandle.Instance = ETH;
    EthHandle.Init.MACAddr = acm32_eth_device.dev_addr;
    EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
    EthHandle.Init.TxDesc = DMATxDscrTab;
    EthHandle.Init.RxDesc = DMARxDscrTab;
    /* Payload after pbuf_custom; must fit max frame (1518) in one descriptor */
    EthHandle.Init.RxBuffLen = ETH_RX_BUFFER_SIZE - sizeof(struct pbuf_custom);
    if (EthHandle.Init.RxBuffLen < 1524U)
    {
        LOG_E("RxBuffLen %u too small for max frame", (unsigned)EthHandle.Init.RxBuffLen);
        return -RT_ERROR;
    }
    LOG_I("RxBuffLen=%u (pbuf_custom=%u)",
          (unsigned)EthHandle.Init.RxBuffLen,
          (unsigned)sizeof(struct pbuf_custom));

    HAL_ETH_DeInit(&EthHandle);
    if (HAL_ETH_Init(&EthHandle) != HAL_OK)
    {
        LOG_E("HAL_ETH_Init FAILED!");
        return -RT_ERROR;
    }
    LOG_I("HAL_ETH_Init OK");
    /* HAL_ETH_Init already programmed RX delay (10,15); may reselect after PHY ID */

    HAL_ETH_SetMDIOClockRange(&EthHandle);

    /* TxConfig defaults — disable HW checksum offload because lwIP
     * calculates checksums in software (RT_LWIP_USING_HW_CHECKSUM not set).
     * Only CRC and padding are handled by hardware. */
    rt_memset(&TxConfig, 0, sizeof(ETH_TxPacketConfigTypeDef));
    TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

    /* PHY reset & init */
    phy_soft_reset(&EthHandle);

    /* Wait for PHY to stabilize */
    HAL_Delay(300);

    /* Verify PHY ID and pick RMII RX delay tap */
    if (phy_get_id(&EthHandle, &phy_id1, &phy_id2) == HAL_OK)
    {
        LOG_I("PHY ID1=0x%04X ID2=0x%04X", phy_id1, phy_id2);
        if (phy_id1 == LAN8720_PHY_ID1 &&
            (phy_id2 & LAN8720_PHY_ID2_MASK) == LAN8720_PHY_ID2_VAL)
        {
            LOG_I("PHY: LAN8720A-compatible");
            eth_rx_delay_config(ETH_RX_DLY_LAN8720_UNIT, ETH_RX_DLY_LAN8720_SEL);
        }
        else
        {
            LOG_I("PHY: generic (e.g. SZ18201), keep HAL RX delay");
            eth_rx_delay_config(ETH_RX_DLY_HAL_UNIT, ETH_RX_DLY_HAL_SEL);
        }
    }
    else
    {
        LOG_W("PHY ID read failed, keep HAL RX delay");
        eth_rx_delay_config(ETH_RX_DLY_HAL_UNIT, ETH_RX_DLY_HAL_SEL);
    }

    /* Wait for auto-negotiation */
    for (wait = 0; wait < 50; wait++)
    {
        phy_state = phy_get_link_state(&EthHandle);
        if (phy_state != PHY_LINK_DOWN && phy_state != PHY_LINK_AUTO_NEGOTIATION)
            break;
        rt_thread_mdelay(100);
    }

    if (phy_state == PHY_LINK_DOWN)
    {
        LOG_W("PHY link down, using defaults");
        speed = ETH_SPEED_100M;
        duplex = ETH_FULLDUPLEX_MODE;
    }
    else
    {
        switch (phy_state)
        {
        case PHY_LINK_100M_FULL_DUPLEX:
            speed = ETH_SPEED_100M; duplex = ETH_FULLDUPLEX_MODE; break;
        case PHY_LINK_100M_HALF_DUPLEX:
            speed = ETH_SPEED_100M; duplex = ETH_HALFDUPLEX_MODE; break;
        case PHY_LINK_10M_FULL_DUPLEX:
            speed = ETH_SPEED_10M;  duplex = ETH_FULLDUPLEX_MODE; break;
        case PHY_LINK_10M_HALF_DUPLEX:
            speed = ETH_SPEED_10M;  duplex = ETH_HALFDUPLEX_MODE; break;
        default:
            speed = ETH_SPEED_100M; duplex = ETH_FULLDUPLEX_MODE; break;
        }
    }

    acm32_eth_device.eth_speed = speed;
    acm32_eth_device.eth_mode  = duplex;

    /* Configure MAC */
    HAL_ETH_GetMACConfig(&EthHandle, &MACConf);
    MACConf.DuplexMode = duplex;
    MACConf.Speed = speed;
    HAL_ETH_SetMACConfig(&EthHandle, &MACConf);

    /* Start ETH in IT mode (HAL_ETH_RxAllocateCallback called internally) */
    if (HAL_ETH_Start_IT(&EthHandle) != HAL_OK)
    {
        LOG_E("HAL_ETH_Start_IT failed!");
        return -RT_ERROR;
    }

    /* Mark link up */
    eth_device_linkchange(&acm32_eth_device.parent, RT_TRUE);

    LOG_I("ready: %s %s",
          speed == ETH_SPEED_100M ? "100M" : "10M",
          duplex == ETH_FULLDUPLEX_MODE ? "Full" : "Half");

    return RT_EOK;
}

/* ===== Link detection thread ===== */

static void ethernet_link_thread(void *parameter)
{
    struct netif *netif = ((struct acm32_eth *)parameter)->parent.netif;
    ETH_MACConfigTypeDef MACConf = {0};
    uint32_t speed, duplex;
    int phy_state;

    for (;;)
    {
        phy_state = phy_get_link_state(&EthHandle);

        if (netif_is_link_up(netif) && (phy_state == PHY_LINK_DOWN))
        {
            HAL_ETH_Stop_IT(&EthHandle);
            netifapi_netif_set_down(netif);
            netifapi_netif_set_link_down(netif);
            LOG_I("link down");
        }
        else if (!netif_is_link_up(netif) && (phy_state >= PHY_LINK_10M_HALF_DUPLEX))
        {
            switch (phy_state)
            {
            case PHY_LINK_100M_FULL_DUPLEX:
                speed = ETH_SPEED_100M; duplex = ETH_FULLDUPLEX_MODE; break;
            case PHY_LINK_100M_HALF_DUPLEX:
                speed = ETH_SPEED_100M; duplex = ETH_HALFDUPLEX_MODE; break;
            case PHY_LINK_10M_FULL_DUPLEX:
                speed = ETH_SPEED_10M;  duplex = ETH_FULLDUPLEX_MODE; break;
            case PHY_LINK_10M_HALF_DUPLEX:
                speed = ETH_SPEED_10M;  duplex = ETH_HALFDUPLEX_MODE; break;
            default:
                speed = ETH_SPEED_100M; duplex = ETH_FULLDUPLEX_MODE; break;
            }

            HAL_ETH_GetMACConfig(&EthHandle, &MACConf);
            MACConf.DuplexMode = duplex;
            MACConf.Speed = speed;
            HAL_ETH_SetMACConfig(&EthHandle, &MACConf);

            if (HAL_ETH_Start_IT(&EthHandle) != HAL_OK)
                LOG_E("link-up: HAL_ETH_Start_IT failed");
            netifapi_netif_set_up(netif);
            netifapi_netif_set_link_up(netif);
            LOG_I("link up: %s %s",
                  speed == ETH_SPEED_100M ? "100M" : "10M",
                  duplex == ETH_FULLDUPLEX_MODE ? "Full" : "Half");
        }

        rt_thread_mdelay(100);
    }
}

/* ===== eth_device ops table ===== */

static const struct rt_device_ops eth_ops =
{
    .init    = rt_acm32_eth_init,
    .open    = RT_NULL,
    .close   = RT_NULL,
    .read    = RT_NULL,
    .write   = RT_NULL,
    .control = rt_acm32_eth_control,
};

/* ===== Driver entry point ===== */

static int rt_hw_acm32_eth_init(void)
{
    rt_err_t ret;

    /* Create semaphore */
    tx_sem = rt_sem_create("eth_tx", 0, RT_IPC_FLAG_PRIO);
    if (!tx_sem)
    {
        LOG_E("sem create failed");
        return -RT_ENOMEM;
    }

    /* Set device ops */
    acm32_eth_device.parent.parent.ops = &eth_ops;

    /* MAC address is already set in rt_acm32_eth_init (from EFUSE)
     * and copied to dev_addr before HAL_ETH_Init. Do NOT overwrite here. */

    /* Set eth_device function pointers */
    acm32_eth_device.parent.eth_rx  = rt_acm32_eth_rx;
    acm32_eth_device.parent.eth_tx  = rt_acm32_eth_tx;

    /* Register with RT-Thread + lwIP */
    ret = eth_device_init_with_flag(&(acm32_eth_device.parent), "e0",
                                    NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP);
    if (ret != RT_EOK)
    {
        return ret;
    }

#ifdef BSP_ETH_STATIC_IP
    /* Set static IP address when DHCP is not available.
     * Must be done after eth_device_init_with_flag which calls netif_add.
     * Configure via menuconfig or rtconfig.h:
     *   BSP_ETH_STATIC_IP, BSP_ETH_STATIC_GW, BSP_ETH_STATIC_NM */
    {
        ip4_addr_t ipaddr, netmask, gw;
        struct netif *netif = acm32_eth_device.parent.netif;
        if (netif != RT_NULL)
        {
            IP4_ADDR(&ipaddr, BSP_ETH_STATIC_IP0, BSP_ETH_STATIC_IP1,
                              BSP_ETH_STATIC_IP2, BSP_ETH_STATIC_IP3);
            IP4_ADDR(&gw, BSP_ETH_STATIC_GW0, BSP_ETH_STATIC_GW1,
                      BSP_ETH_STATIC_GW2, BSP_ETH_STATIC_GW3);
            IP4_ADDR(&netmask, BSP_ETH_STATIC_NM0, BSP_ETH_STATIC_NM1,
                              BSP_ETH_STATIC_NM2, BSP_ETH_STATIC_NM3);
            netifapi_netif_set_addr(netif, &ipaddr, &netmask, &gw);
            LOG_I("Static IP: %d.%d.%d.%d",
                  BSP_ETH_STATIC_IP0, BSP_ETH_STATIC_IP1,
                  BSP_ETH_STATIC_IP2, BSP_ETH_STATIC_IP3);
        }
    }
#endif

    /* Create link detection thread */
    rt_thread_t link_thread = rt_thread_create("eth_link",
                                ethernet_link_thread, &acm32_eth_device,
                                1024, RT_THREAD_PRIORITY_MAX - 2, 10);
    if (link_thread)
        rt_thread_startup(link_thread);

    LOG_I("ETH driver registered as e0");
    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_acm32_eth_init);

/* ===== MSH network commands ===== */

#include <lwip/netif.h>
#include <lwip/ip4_addr.h>
#include <lwip/prot/ip4.h>
#include <lwip/dhcp.h>
#include <lwip/netifapi.h>

static int eth_ifconfig(int argc, char **argv)
{
    struct netif *netif = netif_list;

    /* set: eth_ifconfig set <ip> <gw> <nm>
     * Uses "set" subcommand because MSH/FINSH treats '.' as
     * member-access operator and truncates dotted IP arguments. */
    if (argc == 5 && rt_strcmp(argv[1], "set") == 0)
    {
        ip4_addr_t ip, gw, nm;
        if (ip4addr_aton(argv[2], &ip) && ip4addr_aton(argv[3], &gw) && ip4addr_aton(argv[4], &nm))
        {
            netif = netif_list;
            if (netif != RT_NULL)
            {
                netifapi_netif_set_addr(netif, &ip, &nm, &gw);
                rt_kprintf("IP set: %s / %s / %s\n", argv[2], argv[3], argv[4]);
            }
            return 0;
        }
        rt_kprintf("Invalid address format\n");
        return -1;
    }

    while (netif != RT_NULL)
    {
        rt_kprintf("netif: %c%c%s\n", netif->name[0], netif->name[1],
                   (netif == netif_default) ? " (Default)" : "");
        rt_kprintf("  IP: %s\n", ipaddr_ntoa(&netif->ip_addr));
        rt_kprintf("  GW: %s\n", ipaddr_ntoa(&netif->gw));
        rt_kprintf("  NM: %s\n", ipaddr_ntoa(&netif->netmask));
        rt_kprintf("  MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                   netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
                   netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
        rt_kprintf("  FLAGS: %s%s%s\n",
                   (netif->flags & NETIF_FLAG_UP) ? "UP " : "DOWN ",
                   (netif->flags & NETIF_FLAG_LINK_UP) ? "LINK_UP " : "LINK_DOWN ",
                   (netif->flags & NETIF_FLAG_ETHARP) ? "ETHARP" : "");
#if LWIP_DHCP
        rt_kprintf("  DHCP: %s\n",
                   dhcp_supplied_address(netif) ? "bound" : "searching");
#endif
        netif = netif->next;
    }
    /* Show ETH DMA status */
    if (EthHandle.Instance == ETH)
    {
        rt_kprintf("ETH IRQ=%u RX=%u TX=%u alloc=%u fail=%u read=%u err=%u txcnt=%u\n",
                   (unsigned)eth_irq_count, (unsigned)eth_rx_cplt_count,
                   (unsigned)eth_tx_cplt_count, (unsigned)eth_rx_alloc_count,
                   (unsigned)eth_rx_alloc_fail, (unsigned)eth_rx_read_ok,
                   (unsigned)eth_rx_err_count, (unsigned)eth_tx_count);
        rt_kprintf("  MAC speed=%s duplex=%s phy_state=%d\n",
                   acm32_eth_device.eth_speed == ETH_SPEED_100M ? "100M" : "10M",
                   acm32_eth_device.eth_mode == ETH_FULLDUPLEX_MODE ? "Full" : "Half",
                   phy_get_link_state(&EthHandle));
    }
    return 0;
}
MSH_CMD_EXPORT(eth_ifconfig, show/set ETH interface);

/* eth_rx_dly <unit> <sel>  — RMII RX delay taps (try if iperf drops frames) */
static int eth_rx_dly(int argc, char **argv)
{
    uint32_t unit, sel;

    if (argc < 3)
    {
        rt_kprintf("Usage: eth_rx_dly <unit 0-127> <sel 0-15>\n");
        rt_kprintf("  boot auto: LAN8720 -> %u %u; other PHY -> %u %u (HAL)\n",
                   (unsigned)ETH_RX_DLY_LAN8720_UNIT,
                   (unsigned)ETH_RX_DLY_LAN8720_SEL,
                   (unsigned)ETH_RX_DLY_HAL_UNIT,
                   (unsigned)ETH_RX_DLY_HAL_SEL);
        rt_kprintf("  CFGR now=0x%08X\n", (unsigned)ETH_DLYB->CFGR);
        return 0;
    }
    unit = (uint32_t)atoi(argv[1]);
    sel = (uint32_t)atoi(argv[2]);
    if (unit > 127U || sel > 15U)
    {
        rt_kprintf("unit 0..127, sel 0..15\n");
        return -1;
    }
    eth_rx_delay_config(unit, sel);
    return 0;
}
MSH_CMD_EXPORT(eth_rx_dly, "set ETH RMII RX delay unit sel");

/* ===== Diagnostic ping (bypasses netdev layer) ===== */

#include <lwip/sockets.h>
#include <lwip/inet.h>
#include <lwip/icmp.h>
#include <lwip/ip.h>
#include <lwip/inet_chksum.h>

static void dping(int argc, char **argv)
{
    int s, ret;
    struct sockaddr_in to;
    struct in_addr ina;
    int ping_size = sizeof(struct icmp_echo_hdr) + 32;
    char sbuf[sizeof(struct icmp_echo_hdr) + 32];
    char rbuf[64];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    struct timeval tv = { 2, 0 };
    rt_tick_t t0, t1;

    if (argc < 2) { rt_kprintf("Usage: dping <ip>\n"); return; }
    if (!inet_aton(argv[1], &ina)) { rt_kprintf("bad IP\n"); return; }

    s = lwip_socket(AF_INET, SOCK_RAW, IP_PROTO_ICMP);
    if (s < 0) { rt_kprintf("socket=%d fail\n", s); return; }
    rt_kprintf("socket=%d\n", s);

    lwip_setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    {
        struct icmp_echo_hdr *eh = (struct icmp_echo_hdr *)sbuf;
        ICMPH_TYPE_SET(eh, ICMP_ECHO);
        ICMPH_CODE_SET(eh, 0);
        eh->id = htons(0xAFAF);
        eh->seqno = htons(1);
        memset(sbuf + sizeof(struct icmp_echo_hdr), 0x42, 32);
        eh->chksum = 0;
        eh->chksum = inet_chksum(eh, ping_size);
    }

    to.sin_len = sizeof(to);
    to.sin_family = AF_INET;
    to.sin_addr = ina;

    t0 = rt_tick_get();
    ret = lwip_sendto(s, sbuf, ping_size, 0, (struct sockaddr *)&to, sizeof(to));
    t1 = rt_tick_get();
    rt_kprintf("sendto=%d (%dms)\n", ret,
               (t1 - t0) * 1000 / RT_TICK_PER_SECOND);

    if (ret == ping_size)
    {
        struct ip_hdr *ip;
        struct icmp_echo_hdr *eh;
        int hl, matched = 0;
        rt_tick_t t_start = rt_tick_get();
        rt_tick_t t_now, t_max = rt_tick_from_millisecond(2500);

        for (;;)
        {
            t_now = rt_tick_get();
            if (t_now - t_start >= t_max)
                break;

            ret = lwip_recvfrom(s, rbuf, sizeof(rbuf), 0, (struct sockaddr *)&from, &fromlen);
            t_now = rt_tick_get();
            if (ret <= 0)
                break;

            if (ret < (int)(sizeof(struct ip_hdr) + sizeof(struct icmp_echo_hdr)))
                continue;

            ip = (struct ip_hdr *)rbuf;
            hl = IPH_HL(ip) * 4;
            eh = (struct icmp_echo_hdr *)(rbuf + hl);

            if (ICMPH_TYPE(eh) == ICMP_ER && eh->id == htons(0xAFAF) && eh->seqno == htons(1))
            {
                rt_kprintf("recv=%d from %s id=0x%04X seq=%d ttl=%d (%dms)\n",
                           ret, inet_ntoa(from.sin_addr),
                           ntohs(eh->id), ntohs(eh->seqno),
                           IPH_TTL(ip),
                           (t_now - t_start) * 1000 / RT_TICK_PER_SECOND);
                matched = 1;
                break;
            }
        }
        if (!matched)
            rt_kprintf("timeout (%dms)\n", (t_now - t_start) * 1000 / RT_TICK_PER_SECOND);
    }
    else
    {
        rt_kprintf("send failed\n");
    }

    lwip_close(s);
}
MSH_CMD_EXPORT(dping, dping - raw ping diagnostic);

/* ===== ARP table dump ===== */

#include <lwip/etharp.h>

static void arp(int argc, char **argv)
{
    int i;
    for (i = 0; i < ARP_TABLE_SIZE; i++)
    {
        ip4_addr_t *ip = NULL;
        struct netif *n = NULL;
        struct eth_addr *mac = NULL;

        if (etharp_get_entry(i, &ip, &n, &mac) > 0)
        {
            rt_kprintf("%d: %d.%d.%d.%d -> %02X:%02X:%02X:%02X:%02X:%02X\n",
                       i,
                       ip4_addr1(ip), ip4_addr2(ip), ip4_addr3(ip), ip4_addr4(ip),
                       mac->addr[0], mac->addr[1], mac->addr[2],
                       mac->addr[3], mac->addr[4], mac->addr[5]);
        }
    }
}
MSH_CMD_EXPORT(arp, show ARP table);
