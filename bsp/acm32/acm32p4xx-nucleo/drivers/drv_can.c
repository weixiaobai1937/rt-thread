/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   ACM32P4xx FDCAN driver (CAN classic mode)
 */

#include "drv_can.h"

#if defined(RT_USING_CAN) && (defined(BSP_USING_FDCAN1) || defined(BSP_USING_FDCAN2))

#define DBG_TAG "drv.fdcan"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

/* ==================== 常量 ==================== */

#define CAN_BAUD_1M      1000000U
#define CAN_BAUD_800K     800000U
#define CAN_BAUD_500K     500000U
#define CAN_BAUD_250K     250000U
#define CAN_BAUD_125K     125000U
#define CAN_BAUD_100K     100000U
#define CAN_BAUD_50K       50000U
#define CAN_BAUD_20K       20000U
#define CAN_BAUD_10K       10000U

#define FDCAN_FILTER_LEN_16    16
#define FDCAN_FILTER_LEN_32    32

/* CAN 采样点配置（千分比，875 = 87.5%），可在 rtconfig.h 或 Kconfig 中覆盖 */
#ifndef CAN_SAMPLE_POINT_PER_THOUSAND
#define CAN_SAMPLE_POINT_PER_THOUSAND   875U
#endif

/* ==================== 波特率计算 ==================== */

/*
 * baud = CANCLK / (Prescaler * (1+TS1+TS2))
 * 采样点 = (1+TS1) / (1+TS1+TS2) target 87.5%
 * SJW = min(4, TS2)
 */
static rt_err_t can_baud_rate_calc(uint32_t baud_rate,
    uint32_t *prescaler, uint32_t *tseg1, uint32_t *tseg2, uint32_t *sjw)
{
    uint32_t best_diff = 0xFFFFFFFFU;
    uint32_t best_prescaler = 1;
    uint32_t best_tseg1 = 1;
    uint32_t best_tseg2 = 1;

    for (uint32_t presc = 1; presc <= 512; presc++)
    {
        uint32_t total_tq = (FDCAN_CLOCK_HZ + presc * baud_rate / 2) / (presc * baud_rate);
        if (total_tq < 4 || total_tq > 258)
            continue;

        uint32_t ts2 = (total_tq * (1000U - CAN_SAMPLE_POINT_PER_THOUSAND)) / 1000U;
        if (ts2 < 1)
            ts2 = 1;
        if (ts2 > 128)
            ts2 = 128;

        if (total_tq <= (1 + ts2))
            continue;

        uint32_t ts1 = total_tq - 1 - ts2;
        if (ts1 < 1 || ts1 > 256)
            continue;

        uint32_t actual_baud = FDCAN_CLOCK_HZ / (presc * total_tq);
        uint32_t diff = (actual_baud > baud_rate) ?
                        (actual_baud - baud_rate) : (baud_rate - actual_baud);

        if (diff < best_diff)
        {
            best_diff = diff;
            best_prescaler = presc;
            best_tseg1 = ts1;
            best_tseg2 = ts2;
        }
    }

    *prescaler = best_prescaler;
    *tseg1 = best_tseg1;
    *tseg2 = best_tseg2;
    *sjw = (best_tseg2 > 4) ? 4 : best_tseg2;

    return (best_diff <= (baud_rate / 100U)) ? RT_EOK : -RT_ERROR;
}

/* ==================== DLC 转换 ==================== */

static uint8_t len_to_dlc(uint8_t len)
{
    /* CAN classic: max 8 bytes. FDCAN FD mode: max 64 bytes (not yet supported) */
    return (len > 8) ? 8 : len;
}

/* ==================== 过滤器辅助 ==================== */

static void filter_set_default(FDCAN_NewFilterTypeDef *filter)
{
    rt_memset(filter, 0, sizeof(*filter));
    filter->FilterIndex = 0;
    filter->FilterMask_Enable = 1;
    filter->Filter_Length = FDCAN_FILTER_LEN_16;
    filter->Filter_Count = 1;

    filter->filter16_0.basic.id = 0;
    filter->filter16_0.basic.IDE = 0;
    filter->filter16_0.basic.RTR = 0;

    filter->mask16_0.basic.id = 0x7FF;
    filter->mask16_0.basic.IDE = 1;
    filter->mask16_0.basic.RTR = 1;
}

static void filter_set_from_item(FDCAN_NewFilterTypeDef *filter,
    struct rt_can_filter_item *item, uint32_t index)
{
    rt_memset(filter, 0, sizeof(*filter));
    filter->FilterIndex = (item->hdr_bank < 0) ? (uint8_t)index : (uint8_t)item->hdr_bank;
    filter->FilterMask_Enable = (item->mode == 0) ? 1 : 0;
    filter->Filter_Count = 1;

    if (item->ide == RT_CAN_EXTID)
    {
        /* 29-bit extended ID: 32-bit filter */
        filter->Filter_Length = FDCAN_FILTER_LEN_32;
        filter->filter32_0.ext.id = item->id & 0x1FFFFFFFU;
        filter->filter32_0.ext.IDE = 1;
        filter->filter32_0.ext.RTR = item->rtr;

        if (filter->FilterMask_Enable)
        {
            filter->mask32_0.ext.id = item->mask & 0x1FFFFFFFU;
            filter->mask32_0.ext.IDE = 1;
            filter->mask32_0.ext.RTR = 1;
        }
        else
        {
            filter->filter32_1.ext.id = item->mask & 0x1FFFFFFFU;
            filter->filter32_1.ext.IDE = 1;
            filter->filter32_1.ext.RTR = item->rtr;
        }
    }
    else
    {
        /* 11-bit standard ID: 16-bit filter */
        filter->Filter_Length = FDCAN_FILTER_LEN_16;
        filter->filter16_0.basic.id = item->id & 0x7FFU;
        filter->filter16_0.basic.IDE = 0;
        filter->filter16_0.basic.RTR = item->rtr;

        if (filter->FilterMask_Enable)
        {
            filter->mask16_0.basic.id = item->mask & 0x7FFU;
            filter->mask16_0.basic.IDE = 1;
            filter->mask16_0.basic.RTR = 1;
        }
        else
        {
            filter->filter16_1.basic.id = item->mask & 0x7FFU;
            filter->filter16_1.basic.IDE = 0;
            filter->filter16_1.basic.RTR = item->rtr;
        }
    }
}

/* ==================== 实例 ==================== */

#ifdef BSP_USING_FDCAN1
static acm32_can_t st_DrvCan1 =
{
    .name = "fdcan1",
    .fdcanHandle.Instance = FDCAN1,
};
#endif

#ifdef BSP_USING_FDCAN2
static acm32_can_t st_DrvCan2 =
{
    .name = "fdcan2",
    .fdcanHandle.Instance = FDCAN2,
};
#endif

/* ==================== MSP 初始化 ==================== */

static void fdcan_msp_init(FDCAN_HandleTypeDef *hfdcan)
{
    GPIO_InitTypeDef gpio = {0};

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Drive = GPIO_DRIVE_LEVEL3;

    if (hfdcan->Instance == FDCAN1)
    {
#ifdef BSP_USING_FDCAN1
        __HAL_RCC_FDCAN1_CLK_ENABLE();
        if (FDCAN1_TX_PORT == GPIOA || FDCAN1_RX_PORT == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
        if (FDCAN1_TX_PORT == GPIOB || FDCAN1_RX_PORT == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        if (FDCAN1_TX_PORT == GPIOD || FDCAN1_RX_PORT == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();

        gpio.Pin = FDCAN1_TX_PIN;
        gpio.Alternate = FDCAN1_AF;
        HAL_GPIO_Init(FDCAN1_TX_PORT, &gpio);

        gpio.Pin = FDCAN1_RX_PIN;
        gpio.Alternate = FDCAN1_AF;
        HAL_GPIO_Init(FDCAN1_RX_PORT, &gpio);

        HAL_NVIC_SetPriority(FDCAN1_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(FDCAN1_IRQn);
#endif
    }
    else if (hfdcan->Instance == FDCAN2)
    {
#ifdef BSP_USING_FDCAN2
        __HAL_RCC_FDCAN2_CLK_ENABLE();
        if (FDCAN2_TX_PORT == GPIOE || FDCAN2_RX_PORT == GPIOE) __HAL_RCC_GPIOE_CLK_ENABLE();
        if (FDCAN2_TX_PORT == GPIOD || FDCAN2_RX_PORT == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();

        gpio.Pin = FDCAN2_TX_PIN;
        gpio.Alternate = FDCAN2_AF;
        HAL_GPIO_Init(FDCAN2_TX_PORT, &gpio);

        gpio.Pin = FDCAN2_RX_PIN;
        gpio.Alternate = FDCAN2_AF;
        HAL_GPIO_Init(FDCAN2_RX_PORT, &gpio);

        HAL_NVIC_SetPriority(FDCAN2_IRQn, 2, 0);
        HAL_NVIC_EnableIRQ(FDCAN2_IRQn);
#endif
    }
}

void HAL_FDCAN_MspInit(FDCAN_HandleTypeDef *hfdcan)
{
    fdcan_msp_init(hfdcan);
}

/* ==================== rt_can_ops ==================== */

static rt_err_t _can_configure(struct rt_can_device *can, struct can_configure *cfg)
{
    acm32_can_t *pdrv_can;
    uint32_t prescaler, tseg1, tseg2, sjw;

    RT_ASSERT(can);
    RT_ASSERT(cfg);

    pdrv_can = (acm32_can_t *)can->parent.user_data;
    RT_ASSERT(pdrv_can);

    if (can_baud_rate_calc(cfg->baud_rate, &prescaler, &tseg1, &tseg2, &sjw) != RT_EOK)
    {
        LOG_E("baud rate %d not supported (CANCLK=%d Hz)", cfg->baud_rate, FDCAN_CLOCK_HZ);
        return -RT_ERROR;
    }

    LOG_D("baud=%d, presc=%d, tseg1=%d, tseg2=%d, sjw=%d",
          cfg->baud_rate, prescaler, tseg1, tseg2, sjw);

    pdrv_can->fdcanHandle.Init.Mode = FDCAN_MODE_NORMAL;
    pdrv_can->fdcanHandle.Init.FrameISOType = FDCAN_FRAME_NONISO;
    pdrv_can->fdcanHandle.Init.RxBufOverFlowMode = FDCAN_RX_BUF_OVERWRITE;
    pdrv_can->fdcanHandle.Init.AutoRetransmission = ENABLE;
    pdrv_can->fdcanHandle.Init.NominalPrescaler = prescaler;
    pdrv_can->fdcanHandle.Init.NominalSyncJumpWidth = sjw;
    pdrv_can->fdcanHandle.Init.NominalTimeSeg1 = tseg1;
    pdrv_can->fdcanHandle.Init.NominalTimeSeg2 = tseg2;
    pdrv_can->fdcanHandle.Init.DataPrescaler = prescaler;
    pdrv_can->fdcanHandle.Init.DataSyncJumpWidth = sjw;
    pdrv_can->fdcanHandle.Init.DataTimeSeg1 = tseg1;
    pdrv_can->fdcanHandle.Init.DataTimeSeg2 = tseg2;
    pdrv_can->fdcanHandle.Init.TransferDelay = 0;

    switch (cfg->mode)
    {
    case RT_CAN_MODE_LISTEN:
        pdrv_can->fdcanHandle.Init.Mode = FDCAN_MODE_LOM;
        break;
    case RT_CAN_MODE_LOOPBACK:
        pdrv_can->fdcanHandle.Init.Mode = FDCAN_MODE_LBMI;
        break;
    case RT_CAN_MODE_LOOPBACKANLISTEN:
        pdrv_can->fdcanHandle.Init.Mode = FDCAN_MODE_LBME;
        break;
    case RT_CAN_MODE_NORMAL:
    default:
        pdrv_can->fdcanHandle.Init.Mode = FDCAN_MODE_NORMAL;
        break;
    }

    if (HAL_FDCAN_Init(&pdrv_can->fdcanHandle) != HAL_OK)
    {
        LOG_E("HAL_FDCAN_Init failed");
        return -RT_ERROR;
    }

    /* default filter: pass all standard frames */
    filter_set_default(&pdrv_can->FilterConfig);
    HAL_FDCAN_NewConfigFilter(&pdrv_can->fdcanHandle, &pdrv_can->FilterConfig);

    /* init TxHeader (默认标准帧，与 filter_set_default 一致) */
    pdrv_can->TxHeader.ID.w = 0;
    pdrv_can->TxHeader.FrameInfo.w = 0;
    pdrv_can->TxHeader.FrameInfo.b.DLC = FDCAN_DLC_BYTES_8;
    pdrv_can->TxHeader.FrameInfo.b.IDE = FDCAN_STANDARD_ID;

    HAL_FDCAN_Start(&pdrv_can->fdcanHandle);

    return RT_EOK;
}

static rt_err_t _can_control(struct rt_can_device *can, int cmd, void *arg)
{
    rt_uint32_t argval;
    acm32_can_t *pdrv_can;
    struct rt_can_filter_config *filter_cfg;
    uint32_t prescaler, tseg1, tseg2, sjw;

    RT_ASSERT(can != RT_NULL);
    pdrv_can = (acm32_can_t *)can->parent.user_data;
    RT_ASSERT(pdrv_can != RT_NULL);

    switch (cmd)
    {
    case RT_CAN_CMD_SET_FILTER:
        if (RT_NULL == arg)
        {
            filter_set_default(&pdrv_can->FilterConfig);
            HAL_FDCAN_NewConfigFilter(&pdrv_can->fdcanHandle,
                                       &pdrv_can->FilterConfig);
        }
        else
        {
            filter_cfg = (struct rt_can_filter_config *)arg;
            for (int i = 0; i < filter_cfg->count; i++)
            {
                filter_set_from_item(&pdrv_can->FilterConfig,
                                     &filter_cfg->items[i], i);
                if (HAL_FDCAN_NewConfigFilter(&pdrv_can->fdcanHandle,
                                              &pdrv_can->FilterConfig) != HAL_OK)
                {
                    LOG_E("filter config failed, index=%d", i);
                    return -RT_ERROR;
                }
            }
        }
        break;

    case RT_CAN_CMD_SET_BAUD:
        argval = (rt_uint32_t)arg;
        if (can_baud_rate_calc(argval, &prescaler, &tseg1, &tseg2, &sjw) != RT_EOK)
        {
            LOG_E("baud rate %d not supported", argval);
            return -RT_ERROR;
        }
        if (argval != pdrv_can->device.config.baud_rate)
        {
            pdrv_can->device.config.baud_rate = argval;
            return _can_configure(&pdrv_can->device, &pdrv_can->device.config);
        }
        break;

    case RT_CAN_CMD_SET_MODE:
        argval = (rt_uint32_t)arg;
        if (argval != RT_CAN_MODE_NORMAL &&
            argval != RT_CAN_MODE_LISTEN &&
            argval != RT_CAN_MODE_LOOPBACK &&
            argval != RT_CAN_MODE_LOOPBACKANLISTEN)
        {
            return -RT_ERROR;
        }
        if (argval != pdrv_can->device.config.mode)
        {
            pdrv_can->device.config.mode = argval;
            return _can_configure(&pdrv_can->device, &pdrv_can->device.config);
        }
        break;

    case RT_CAN_CMD_START:
        argval = (rt_uint32_t)arg;
        if (argval == 1)
            HAL_FDCAN_Start(&pdrv_can->fdcanHandle);
        else
            HAL_FDCAN_Stop(&pdrv_can->fdcanHandle);
        break;

    case RT_CAN_CMD_GET_STATUS:
    {
        if (arg == RT_NULL)
            return -RT_EINVAL;

        fdcan_ecc_u ecc;
        ecc.w = pdrv_can->fdcanHandle.Instance->ECC;

        pdrv_can->device.status.rcverrcnt = ecc.b.RECNT;
        pdrv_can->device.status.snderrcnt = ecc.b.TECNT;
        pdrv_can->device.status.lasterrtype = ecc.b.KOER;

        rt_memcpy(arg, &pdrv_can->device.status,
                  sizeof(pdrv_can->device.status));
    }
    break;

    case RT_DEVICE_CTRL_SET_INT:
    {
        argval = (rt_uint32_t)arg;
        if (argval == RT_DEVICE_FLAG_INT_RX)
        {
            __HAL_FDCAN_ENABLE_IT(&pdrv_can->fdcanHandle,
                                  FDCAN_IE_RX_BUFFER_NEW_MESSAGE);
        }
        else if (argval == RT_DEVICE_FLAG_INT_TX)
        {
            __HAL_FDCAN_ENABLE_IT(&pdrv_can->fdcanHandle,
                                  FDCAN_IE_TX_PTB_COMPLETE);
            __HAL_FDCAN_ENABLE_IT(&pdrv_can->fdcanHandle,
                                  FDCAN_IE_TX_STB_COMPLETE);
        }
        else if (argval == RT_DEVICE_CAN_INT_ERR)
        {
            __HAL_FDCAN_ENABLE_IT(&pdrv_can->fdcanHandle,
                                  FDCAN_IE_ERROR);
        }
    }
    break;

    case RT_DEVICE_CTRL_CLR_INT:
    {
        argval = (rt_uint32_t)arg;
        if (argval == RT_DEVICE_FLAG_INT_RX)
        {
            __HAL_FDCAN_DISABLE_IT(&pdrv_can->fdcanHandle,
                                   FDCAN_IE_RX_BUFFER_NEW_MESSAGE);
        }
        else if (argval == RT_DEVICE_FLAG_INT_TX)
        {
            __HAL_FDCAN_DISABLE_IT(&pdrv_can->fdcanHandle,
                                   FDCAN_IE_TX_PTB_COMPLETE);
            __HAL_FDCAN_DISABLE_IT(&pdrv_can->fdcanHandle,
                                   FDCAN_IE_TX_STB_COMPLETE);
        }
        else if (argval == RT_DEVICE_CAN_INT_ERR)
        {
            __HAL_FDCAN_DISABLE_IT(&pdrv_can->fdcanHandle,
                                   FDCAN_IE_ERROR);
        }
    }
    break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

/* 填充 FDCAN 发送报文头（阻塞/非阻塞发送共用） */
static void fill_tx_header(FDCAN_TxHeaderTypeDef *hdr, const struct rt_can_msg *pmsg)
{
    hdr->ID.w = 0;
    hdr->FrameInfo.w = 0;
    hdr->ID.b.ID = pmsg->id;

    if (pmsg->ide == RT_CAN_EXTID)
        hdr->FrameInfo.b.IDE = FDCAN_EXTENDED_ID;
    else
        hdr->FrameInfo.b.IDE = FDCAN_STANDARD_ID;

    hdr->FrameInfo.b.RTR = (pmsg->rtr == RT_CAN_RTR) ? 1 : 0;
    hdr->FrameInfo.b.DLC = len_to_dlc(pmsg->len);
    hdr->FrameInfo.b.FDF = 0;
    hdr->FrameInfo.b.BRS = 0;
}

static rt_ssize_t _can_sendmsg(struct rt_can_device *can, const void *buf, rt_uint32_t boxno)
{
    acm32_can_t *pdrv_can;
    struct rt_can_msg *pmsg;
    FDCAN_TxHeaderTypeDef tx_header;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pdrv_can = (acm32_can_t *)can->parent.user_data;
    RT_ASSERT(pdrv_can);

    pmsg = (struct rt_can_msg *)buf;

    fill_tx_header(&tx_header, pmsg);

    if (HAL_FDCAN_TransmitMessageByPTB(&pdrv_can->fdcanHandle,
                                        &tx_header,
                                        pmsg->data) != HAL_OK)
    {
        return -RT_ERROR;
    }

    if (HAL_FDCAN_WaitTxCompleted(&pdrv_can->fdcanHandle,
                                   FDCAN_TRANSMIT_PTB, 100) != HAL_OK)
    {
        LOG_W("tx timeout");
        return -RT_EBUSY;
    }

    return RT_EOK;
}

static rt_ssize_t _can_recvmsg(struct rt_can_device *can, void *buf, rt_uint32_t fifo)
{
    struct rt_can_msg *pmsg;
    acm32_can_t *pdrv_can;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pdrv_can = (acm32_can_t *)can->parent.user_data;
    pmsg = (struct rt_can_msg *)buf;

    if (HAL_FDCAN_GetRxMessage(&pdrv_can->fdcanHandle,
                                &pdrv_can->RxHeader,
                                pmsg->data) != HAL_OK)
    {
        return 0;
    }

    pmsg->ide = (pdrv_can->RxHeader.FrameInfo.b.IDE == FDCAN_EXTENDED_ID) ?
                RT_CAN_EXTID : RT_CAN_STDID;
    pmsg->rtr = pdrv_can->RxHeader.FrameInfo.b.RTR ? RT_CAN_RTR : RT_CAN_DTR;
    pmsg->id = pdrv_can->RxHeader.ID.b.ID;

    uint8_t dlc = pdrv_can->RxHeader.FrameInfo.b.DLC;
    pmsg->len = (dlc > 8) ? 8 : dlc;

    return sizeof(struct rt_can_msg);
}

static rt_ssize_t _can_sendmsg_nonblocking(struct rt_can_device *can, const void *buf)
{
    acm32_can_t *pdrv_can;
    struct rt_can_msg *pmsg;
    FDCAN_TxHeaderTypeDef tx_header;

    RT_ASSERT(can);
    RT_ASSERT(buf);

    pdrv_can = (acm32_can_t *)can->parent.user_data;
    RT_ASSERT(pdrv_can);

    pmsg = (struct rt_can_msg *)buf;

    fill_tx_header(&tx_header, pmsg);

    if (HAL_FDCAN_AddMessageToSTB(&pdrv_can->fdcanHandle,
                                   &tx_header,
                                   pmsg->data) != HAL_OK)
    {
        return -RT_EBUSY;
    }

    return RT_EOK;
}

static const struct rt_can_ops _can_ops =
{
    _can_configure,
    _can_control,
    _can_sendmsg,
    _can_recvmsg,
    _can_sendmsg_nonblocking,
};

/* ==================== HAL 中断回调 ==================== */

void HAL_FDCAN_RxBufferNewMessageCallback(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->Instance == FDCAN1)
    {
#ifdef BSP_USING_FDCAN1
        rt_hw_can_isr(&st_DrvCan1.device, RT_CAN_EVENT_RX_IND | (0 << 8));
#endif
    }
    else if (hfdcan->Instance == FDCAN2)
    {
#ifdef BSP_USING_FDCAN2
        rt_hw_can_isr(&st_DrvCan2.device, RT_CAN_EVENT_RX_IND | (0 << 8));
#endif
    }
}

void HAL_FDCAN_TXPTBCompletedCallback(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->Instance == FDCAN1)
    {
#ifdef BSP_USING_FDCAN1
        rt_hw_can_isr(&st_DrvCan1.device, RT_CAN_EVENT_TX_DONE);
#endif
    }
    else if (hfdcan->Instance == FDCAN2)
    {
#ifdef BSP_USING_FDCAN2
        rt_hw_can_isr(&st_DrvCan2.device, RT_CAN_EVENT_TX_DONE);
#endif
    }
}

void HAL_FDCAN_TXSTBCompletedCallback(FDCAN_HandleTypeDef *hfdcan)
{
    if (hfdcan->Instance == FDCAN1)
    {
#ifdef BSP_USING_FDCAN1
        rt_hw_can_isr(&st_DrvCan1.device, RT_CAN_EVENT_TX_DONE);
#endif
    }
    else if (hfdcan->Instance == FDCAN2)
    {
#ifdef BSP_USING_FDCAN2
        rt_hw_can_isr(&st_DrvCan2.device, RT_CAN_EVENT_TX_DONE);
#endif
    }
}

void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan)
{
    fdcan_ecc_u ecc;
    ecc.w = hfdcan->Instance->ECC;

    if (hfdcan->Instance == FDCAN1)
    {
#ifdef BSP_USING_FDCAN1
        st_DrvCan1.device.status.rcverrcnt = ecc.b.RECNT;
        st_DrvCan1.device.status.snderrcnt = ecc.b.TECNT;
        st_DrvCan1.device.status.lasterrtype = ecc.b.KOER;
        rt_hw_can_isr(&st_DrvCan1.device, RT_CAN_EVENT_TX_FAIL);
#endif
    }
    else if (hfdcan->Instance == FDCAN2)
    {
#ifdef BSP_USING_FDCAN2
        st_DrvCan2.device.status.rcverrcnt = ecc.b.RECNT;
        st_DrvCan2.device.status.snderrcnt = ecc.b.TECNT;
        st_DrvCan2.device.status.lasterrtype = ecc.b.KOER;
        rt_hw_can_isr(&st_DrvCan2.device, RT_CAN_EVENT_TX_FAIL);
#endif
    }
}

/* ==================== 中断服务函数 ==================== */

#ifdef BSP_USING_FDCAN1
void FDCAN1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_FDCAN_IRQHandler(&st_DrvCan1.fdcanHandle);
    rt_interrupt_leave();
}
#endif

#ifdef BSP_USING_FDCAN2
void FDCAN2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_FDCAN_IRQHandler(&st_DrvCan2.fdcanHandle);
    rt_interrupt_leave();
}
#endif

/* ==================== 设备注册 ==================== */

static int rt_hw_can_init(void)
{
    struct can_configure config;
    config.baud_rate = CAN_BAUD_1M;
    config.msgboxsz = 48;
    config.sndboxnumber = 1;
    config.mode = RT_CAN_MODE_NORMAL;
    config.privmode = RT_CAN_MODE_NOPRIV;
    config.ticks = 50;
#ifdef RT_CAN_USING_HDR
    config.maxhdr = 14;
#endif

    FDCAN_NewFilterTypeDef default_filter;
    filter_set_default(&default_filter);

#ifdef BSP_USING_FDCAN1
    st_DrvCan1.FilterConfig = default_filter;
    st_DrvCan1.device.config = config;
    rt_hw_can_register(&st_DrvCan1.device, st_DrvCan1.name, &_can_ops, &st_DrvCan1);
#endif

#ifdef BSP_USING_FDCAN2
    st_DrvCan2.FilterConfig = default_filter;
    st_DrvCan2.device.config = config;
    rt_hw_can_register(&st_DrvCan2.device, st_DrvCan2.name, &_can_ops, &st_DrvCan2);
#endif

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_can_init);

#endif /* RT_USING_CAN && (BSP_USING_FDCAN1 || BSP_USING_FDCAN2) */
