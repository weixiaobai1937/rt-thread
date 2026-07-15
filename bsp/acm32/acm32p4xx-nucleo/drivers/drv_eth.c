/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-06     Sisyphus     first version, HAL ETH + LAN8720A + zero-copy
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

#define LOG_TAG             "drv.eth"
#include <rtdbg.h>

/* ===== RX Buffer Pool (Zero-Copy) — must be in DMA-accessible SRAM ===== */

#define ETH_RX_BUF_ITEM_SIZE    ((ETH_RX_BUFFER_SIZE + 31) & ~31)

static ETH_DMADescTypeDef *DMARxDscrTab;
static ETH_DMADescTypeDef *DMATxDscrTab;
static uint8_t *rx_pool_memory;
static uint32_t *rx_pool_bitmap;

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
static volatile uint32_t eth_tx_count = 0;

/* ===== Forward declarations ===== */

static void ethernet_link_thread(void *parameter);
static int  phy_lan8720_get_link_state(ETH_HandleTypeDef *heth);

/* ===== Rx buffer pool management ===== */

static void *rx_pool_alloc(void)
{
    int i;
    for (i = 0; i < ETH_RX_BUFFER_CNT; i++)
    {
        if (!(rx_pool_bitmap[i / 32] & (1u << (i % 32))))
        {
            rx_pool_bitmap[i / 32] |= (1u << (i % 32));
            return &rx_pool_memory[i * ETH_RX_BUF_ITEM_SIZE];
        }
    }
    return NULL;
}

static void rx_pool_free(void *ptr)
{
    uint8_t *base = (uint8_t *)rx_pool_memory;
    uint32_t offset = (uint8_t *)ptr - base;
    int idx = offset / ETH_RX_BUF_ITEM_SIZE;
    if (idx >= 0 && idx < ETH_RX_BUFFER_CNT)
        rx_pool_bitmap[idx / 32] &= ~(1u << (idx % 32));
}

static void rx_buf_free_custom(struct pbuf *p)
{
    struct pbuf_custom *custom_pbuf = (struct pbuf_custom *)p;
    rx_pool_free(custom_pbuf);
    if (RxAllocStatus == RX_ALLOC_ERROR)
    {
        RxAllocStatus = RX_ALLOC_OK;
        /* Wake RX thread to re-allocate buffers for any stuck descriptors */
        eth_device_ready(&acm32_eth_device.parent);
    }
}

/* ===== HAL_ETH_MspInit: GPIO, Clock, NVIC ===== */

void HAL_ETH_MspInit(ETH_HandleTypeDef *heth)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (heth->Init.MediaInterface == HAL_ETH_RMII_MODE)
    {
        /* Enable GPIO clocks */
        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();

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

        /* PB14(RST) -- GPIO output for PHY hardware reset */
        GPIO_InitStruct.Pin = GPIO_PIN_14;
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_0;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* PC1(MDC), PC4(RXD0), PC5(RXD1) -- AF6 */
        GPIO_InitStruct.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
        GPIO_InitStruct.Alternate = GPIO_FUNCTION_6;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

        /* PHY hardware reset: low 15ms -> high -> wait 20ms */
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
        HAL_Delay(15);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
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

void ETH_IRQHandler(void)
{
    eth_irq_count++;
    rt_interrupt_enter();
    HAL_ETH_IRQHandler(&EthHandle);
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
        *buff = (uint8_t *)p + sizeof(struct pbuf_custom);
        p->custom_free_function = rx_buf_free_custom;
        pbuf_alloced_custom(PBUF_RAW, 0, PBUF_REF, p, *buff,
                            ETH_RX_BUFFER_SIZE - sizeof(struct pbuf_custom));
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

    p = (struct pbuf *)(buff - sizeof(struct pbuf_custom));
    p->next = NULL;
    p->tot_len = 0;
    p->len = Length;

    if (!*ppStart)
    {
        *ppStart = p;
    }
    else
    {
        (*ppEnd)->next = p;
    }
    *ppEnd = p;

    for (p = *ppStart; p != NULL; p = p->next)
    {
        p->tot_len += Length;
    }
}

/* ===== TxFreeCallback: release pbuf after TX ===== */

void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    pbuf_free((struct pbuf *)buff);
}

/* ===== PHY Operations ===== */

static HAL_StatusTypeDef phy_lan8720_soft_reset(ETH_HandleTypeDef *heth)
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

    return HAL_OK;
}

static HAL_StatusTypeDef phy_lan8720_get_id(ETH_HandleTypeDef *heth,
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

static int phy_lan8720_get_link_state(ETH_HandleTypeDef *heth)
{
    uint32_t bsr, bcr, scsr;
    int state = PHY_LINK_DOWN;

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

        /* AN complete, read speed/duplex from SCSR */
        if (HAL_ETH_ReadPHYRegister(heth, ETH_PHY_ADDR, PHY_REG_SCSR, &scsr) != HAL_OK)
            return PHY_LINK_100M_FULL_DUPLEX;

        switch (scsr & PHY_SCSR_SPEED_MASK)
        {
        case PHY_SCSR_10HD:   state = PHY_LINK_10M_HALF_DUPLEX; break;
        case PHY_SCSR_100HD:  state = PHY_LINK_100M_HALF_DUPLEX; break;
        case PHY_SCSR_10FD:   state = PHY_LINK_10M_FULL_DUPLEX; break;
        case PHY_SCSR_100FD:  state = PHY_LINK_100M_FULL_DUPLEX; break;
        default:              state = PHY_LINK_100M_FULL_DUPLEX; break;
        }
    }
    else
    {
        /* Forced mode */
        if (bcr & PHY_BCR_SPEED_100)
            state = (bcr & PHY_BCR_FULL_DUPLEX) ? PHY_LINK_100M_FULL_DUPLEX : PHY_LINK_100M_HALF_DUPLEX;
        else
            state = (bcr & PHY_BCR_FULL_DUPLEX) ? PHY_LINK_10M_FULL_DUPLEX : PHY_LINK_10M_HALF_DUPLEX;
    }

    return state;
}

/* ===== ETH TX callback (eth_device -> low_level_output) ===== */

static rt_err_t rt_acm32_eth_tx(rt_device_t dev, struct pbuf *p)
{
    uint32_t i = 0;
    struct pbuf *q;
    ETH_BufferTypeDef Txbuffer[ETH_TX_DESC_CNT];
    rt_err_t errval = RT_EOK;

    RT_ASSERT(p != RT_NULL);

    eth_tx_count++;

    rt_memset(Txbuffer, 0, sizeof(Txbuffer));

    for (q = p; q != NULL; q = q->next)
    {
        if (i >= ETH_TX_DESC_CNT)
            return -RT_ENOMEM;

        Txbuffer[i].buffer = q->payload;
        Txbuffer[i].len = q->len;

        if (i > 0)
            Txbuffer[i - 1].next = &Txbuffer[i];

        if (q->next == NULL)
            Txbuffer[i].next = NULL;

        i++;
    }

    TxConfig.Length = p->tot_len;
    TxConfig.TxBuffer = Txbuffer;
    TxConfig.pData = p;
    /* Attributes and CRCPadCtrl are pre-configured in rt_acm32_eth_init;
     * do NOT re-enable HW checksum offload here. */

    pbuf_ref(p);

    do {
        if (HAL_ETH_Transmit_IT(&EthHandle, &TxConfig) == HAL_OK)
        {
            errval = RT_EOK;
        }
        else
        {
            if (EthHandle.ErrorCode & HAL_ETH_ERROR_BUSY)
            {
                rt_sem_take(tx_sem, rt_tick_from_millisecond(ETHIF_TX_TIMEOUT));
                HAL_ETH_ReleaseTxPacket(&EthHandle);
                errval = -RT_EBUSY;
            }
            else
            {
                pbuf_free(p);
                errval = -RT_ERROR;
            }
        }
    } while (errval == -RT_EBUSY);

    return errval;
}

/* ===== ETH RX callback (eth_device -> low_level_input) ===== */

static struct pbuf *rt_acm32_eth_rx(rt_device_t dev)
{
    struct pbuf *p = NULL;

    /* Always call HAL_ETH_ReadData so ETH_UpdateDescriptor can refill
     * RX descriptors even after a previous pool-exhaustion recovery.
     * Returns NULL when no packet is available. */
    HAL_ETH_ReadData(&EthHandle, (void **)&p);

    if (p != NULL)
    {
        eth_rx_read_ok++;
    }

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

    /* ETH DMA buffer pool at fixed address in upper SRAM.
     * ETH DMA can only access SRAM in 0x20010000-0x2001FFFF range.
     * Buffer is placed near end of SRAM; board.c limits heap to ETH_DMA_HEAP_END.
     * RT_ASSERT ensures heap has not grown into the DMA region. */
    {
        uint8_t *buf = (uint8_t *)0x20016800U;
        size_t desc_size = sizeof(ETH_DMADescTypeDef) * (ETH_RX_DESC_CNT + ETH_TX_DESC_CNT);
        size_t pool_size = ETH_RX_POOL_SIZE;
        size_t bitmap_size = ((ETH_RX_BUFFER_CNT + 31) / 32) * sizeof(uint32_t);
        size_t total = desc_size + pool_size + bitmap_size;

        /* Verify no overlap with heap (heap end must be <= buf start) */
        extern const uint32_t Image$$RW_IRAM1$$ZI$$Limit;
        RT_ASSERT((uintptr_t)&Image$$RW_IRAM1$$ZI$$Limit <= (uintptr_t)buf);

        if ((uintptr_t)buf + total > 0x20020000U)
        {
            LOG_E("DMA buffer overflows SRAM!");
            return -RT_ENOMEM;
        }

        DMATxDscrTab = (ETH_DMADescTypeDef *)buf;
        DMARxDscrTab = (ETH_DMADescTypeDef *)(buf + sizeof(ETH_DMADescTypeDef) * ETH_TX_DESC_CNT);
        rx_pool_memory = buf + desc_size;
        rx_pool_bitmap = (uint32_t *)(buf + desc_size + pool_size);
        rt_memset(buf, 0, total);

        LOG_D("DMA buf at %p, size %u", (void *)buf, (unsigned)total);
    }

    /* Configure ETH handle */
    EthHandle.Instance = ETH;
    EthHandle.Init.MACAddr = acm32_eth_device.dev_addr;
    EthHandle.Init.MediaInterface = HAL_ETH_RMII_MODE;
    EthHandle.Init.TxDesc = DMATxDscrTab;
    EthHandle.Init.RxDesc = DMARxDscrTab;
    EthHandle.Init.RxBuffLen = ETH_RX_BUFFER_SIZE - sizeof(struct pbuf_custom);

    HAL_ETH_DeInit(&EthHandle);
    if (HAL_ETH_Init(&EthHandle) != HAL_OK)
    {
        LOG_E("HAL_ETH_Init FAILED!");
        return -RT_ERROR;
    }
    LOG_I("HAL_ETH_Init OK");

    HAL_ETH_SetMDIOClockRange(&EthHandle);

    /* TxConfig defaults — disable HW checksum offload because lwIP
     * calculates checksums in software (RT_LWIP_USING_HW_CHECKSUM not set).
     * Only CRC and padding are handled by hardware. */
    rt_memset(&TxConfig, 0, sizeof(ETH_TxPacketConfigTypeDef));
    TxConfig.Attributes = ETH_TX_PACKETS_FEATURES_CRCPAD;
    TxConfig.CRCPadCtrl = ETH_CRC_PAD_INSERT;

    /* PHY reset & init */
    phy_lan8720_soft_reset(&EthHandle);

    /* Wait for PHY to stabilize */
    HAL_Delay(300);

    /* Verify PHY ID */
    if (phy_lan8720_get_id(&EthHandle, &phy_id1, &phy_id2) == HAL_OK)
    {
        LOG_I("PHY ID1=0x%04X ID2=0x%04X", phy_id1, phy_id2);
    }

    /* Wait for auto-negotiation */
    for (wait = 0; wait < 50; wait++)
    {
        phy_state = phy_lan8720_get_link_state(&EthHandle);
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
        phy_state = phy_lan8720_get_link_state(&EthHandle);

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
        rt_kprintf("ETH IRQ=%u RX=%u TX=%u alloc=%u fail=%u read=%u txcnt=%u\n",
                   (unsigned)eth_irq_count, (unsigned)eth_rx_cplt_count,
                   (unsigned)eth_tx_cplt_count, (unsigned)eth_rx_alloc_count,
                   (unsigned)eth_rx_alloc_fail, (unsigned)eth_rx_read_ok,
                   (unsigned)eth_tx_count);
    }
    return 0;
}
MSH_CMD_EXPORT(eth_ifconfig, show/set ETH interface);

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
