/*
 * Copyright (c) 2006-2026 RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-24     AisinoChip   first version
 */

#include "board.h"

#if defined(RT_USING_SDIO) && defined(BSP_USING_SDMMC1)

#include <string.h>
#include <drivers/dev_mmcsd_core.h>

#define DBG_TAG              "drv.sdmmc"
#define DBG_LVL               DBG_INFO
#include <rtdbg.h>

#ifndef SDMMC_CLOCK_FREQ
#define SDMMC_CLOCK_FREQ      (40 * 1000 * 1000)   /* FCLK/4 = 40MHz */
#endif

#ifndef SDIO_BUFF_SIZE
#define SDIO_BUFF_SIZE        (4096)
#endif

#ifndef SDIO_ALIGN_LEN
#define SDIO_ALIGN_LEN         (32)
#endif

#define SD_INIT_FREQ           400000U
#define SD_NORMAL_SPEED_FREQ   25000000U
#define SD_HIGH_SPEED_FREQ     50000000U
#define SDIO_MAX_FREQ          SD_HIGH_SPEED_FREQ

#define SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS    (1000000)

/* SDMMC1 pins: PC8=DAT0, PC9=DAT1, PC10=DAT2, PC11=DAT3, PC12=CLK, PD2=CMD (all AF12) */
#define SDMMC1_DAT0_PIN       GPIO_PIN_8
#define SDMMC1_DAT0_PORT      GPIOC
#define SDMMC1_DAT1_PIN       GPIO_PIN_9
#define SDMMC1_DAT1_PORT      GPIOC
#define SDMMC1_DAT2_PIN       GPIO_PIN_10
#define SDMMC1_DAT2_PORT      GPIOC
#define SDMMC1_DAT3_PIN       GPIO_PIN_11
#define SDMMC1_DAT3_PORT      GPIOC
#define SDMMC1_CLK_PIN        GPIO_PIN_12
#define SDMMC1_CLK_PORT       GPIOC
#define SDMMC1_CMD_PIN        GPIO_PIN_2
#define SDMMC1_CMD_PORT       GPIOD

#define SDMMC1_DAT_PINS       (SDMMC1_DAT0_PIN | SDMMC1_DAT1_PIN | SDMMC1_DAT2_PIN | SDMMC1_DAT3_PIN)

static struct rt_mmcsd_host *sdmmc1_host;
static SDMMC_HandleTypeDef sdmmc1_handle;

struct acm32_sdmmc
{
    struct rt_mmcsd_host *host;
    SDMMC_HandleTypeDef *hsdmmc;
};

static struct acm32_sdmmc sdmmc1_priv;

/*
 * General-purpose no-data command using raw register access.
 * cmd: pre-encoded command (e.g. CMD13_GET_STATUS from hal_sdmmc.h)
 */
static rt_err_t sdmmc_send_no_data_cmd(SDMMC_HandleTypeDef *hsdmmc, uint32_t cmd, uint32_t arg)
{
    uint32_t int_status;
    uint32_t timeout;

    /* wait idle with timeout */
    timeout = SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS;
    while ((hsdmmc->Instance->SDMMC_STATUS & (1 << 9)) && timeout--)
    {
        if (timeout == 0)
        {
            LOG_E("SDMMC wait idle timeout");
            return -RT_ETIMEOUT;
        }
    }

    hsdmmc->Instance->SDMMC_RINTSTS = 0x01FFFF;          /* clear all int status */
    hsdmmc->Instance->SDMMC_CMDARG = arg;
    hsdmmc->Instance->SDMMC_CMD = (cmd | ((uint32_t)hsdmmc->Init.Ch << 16));

    /* wait for command completed with timeout */
    timeout = SDIO_TX_RX_COMPLETE_TIMEOUT_LOOPS;
    while (!(hsdmmc->Instance->SDMMC_RINTSTS & SDMMC_RINT_CMD_CMPLT))
    {
        if (timeout == 0)
        {
            LOG_E("SDMMC wait cmd complete timeout");
            return -RT_ETIMEOUT;
        }
    }

    int_status = hsdmmc->Instance->SDMMC_RINTSTS;
    hsdmmc->Instance->SDMMC_RINTSTS = SDMMC_RINT_CMD_CMPLT;

    if (int_status & (SDMMC_RINT_RESP_ERR | SDMMC_RINT_RESP_CRC_ERR | SDMMC_RINT_RESP_TIMOUT))
    {
        hsdmmc->Instance->SDMMC_RINTSTS = (SDMMC_RINT_RESP_ERR | SDMMC_RINT_RESP_CRC_ERR | SDMMC_RINT_RESP_TIMOUT);
        return -RT_ERROR;
    }

    return RT_EOK;
}

/*
 * 注意: sdmmc_cache_buf 是共享缓冲区，RT-Thread mmcsd 框架通过 host->lock
 * 保证同一时刻只有一个请求在处理，因此无需额外的互斥保护。
 * 如果未来需要支持多线程并发访问，需要添加互斥锁。
 */
rt_align(SDIO_ALIGN_LEN)
static rt_uint32_t sdmmc_cache_buf[SDIO_BUFF_SIZE / sizeof(rt_uint32_t)];

static void sdmmc_request(struct rt_mmcsd_host *host, struct rt_mmcsd_req *req)
{
    struct acm32_sdmmc *sdmmc = (struct acm32_sdmmc *)host->private_data;
    SDMMC_HandleTypeDef *hsdmmc = sdmmc->hsdmmc;
    struct rt_mmcsd_cmd *cmd = req->cmd;
    struct rt_mmcsd_data *data;
    HAL_StatusTypeDef status = HAL_OK;

    if (cmd != RT_NULL)
    {
        data = cmd->data;

        LOG_D("CMD:%d ARG:0x%08x data:%c len:%d blksize:%d",
              cmd->cmd_code,
              cmd->arg,
              data ? ((data->flags & DATA_DIR_WRITE) ? 'w' : 'r') : '-',
              data ? data->blks * data->blksize : 0,
              data ? data->blksize : 0);

        switch (cmd->cmd_code)
        {
        case GO_IDLE_STATE:   /* CMD0 */
            status = HAL_SDMMC_Cmd0_GoIdle(hsdmmc);
            break;

        case ALL_SEND_CID:    /* CMD2 */
            status = HAL_SDMMC_Cmd2_GetCid(hsdmmc);
            break;

        case SD_SEND_RELATIVE_ADDR:   /* CMD3 */
            status = HAL_SDMMC_Cmd3_Rca(hsdmmc);
            break;

        case SELECT_CARD:     /* CMD7 */
            hsdmmc->SdEmmcRegInfo.rca = cmd->arg >> 16;
            status = HAL_SDMMC_Cmd7_Sel(hsdmmc);
            break;

        case SD_SEND_IF_COND:    /* CMD8 */
            status = HAL_SDMMC_Cmd8_GetExtCsd(hsdmmc, NULL);
            break;

        case SEND_CSD:        /* CMD9 */
            status = HAL_SDMMC_Cmd9_GetCsd(hsdmmc);
            break;

        case STOP_TRANSMISSION:  /* CMD12 */
            status = (HAL_StatusTypeDef)sdmmc_send_no_data_cmd(hsdmmc,
                CMD12_STOP_STEARM, hsdmmc->SdEmmcRegInfo.rca << 16);
            break;

        case SEND_STATUS:     /* CMD13 */
            status = (HAL_StatusTypeDef)sdmmc_send_no_data_cmd(hsdmmc,
                CMD13_GET_STATUS, hsdmmc->SdEmmcRegInfo.rca << 16);
            break;

        case SET_BLOCKLEN:    /* CMD16 */
            status = HAL_SDMMC_Cmd16_SetBlkLen(hsdmmc, cmd->arg);
            break;

        case READ_SINGLE_BLOCK:   /* CMD17 */
            if (data)
            {
                status = HAL_SDMMC_Cmd17_RdSingle(hsdmmc, cmd->arg,
                    sdmmc_cache_buf);
                if (status == HAL_OK)
                {
                    rt_memcpy(data->buf, sdmmc_cache_buf,
                        data->blks * data->blksize);
                }
            }
            else
            {
                status = HAL_ERROR;
            }
            break;

        case READ_MULTIPLE_BLOCK:   /* CMD18 */
            if (data)
            {
                status = HAL_SDMMC_Cmd18_RdMul(hsdmmc, cmd->arg,
                    data->blks, sdmmc_cache_buf);
                if (status == HAL_OK)
                {
                    rt_memcpy(data->buf, sdmmc_cache_buf,
                        data->blks * data->blksize);
                }
            }
            else
            {
                status = HAL_ERROR;
            }
            break;

        case WRITE_BLOCK:   /* CMD24 */
            if (data)
            {
                rt_memcpy(sdmmc_cache_buf, data->buf,
                    data->blks * data->blksize);
                status = HAL_SDMMC_Cmd24_WrSingle(hsdmmc, cmd->arg,
                    sdmmc_cache_buf);
            }
            else
            {
                status = HAL_ERROR;
            }
            break;

        case WRITE_MULTIPLE_BLOCK:   /* CMD25 */
            if (data)
            {
                rt_memcpy(sdmmc_cache_buf, data->buf,
                    data->blks * data->blksize);
                status = HAL_SDMMC_Cmd25_WrMul(hsdmmc, cmd->arg,
                    data->blks, sdmmc_cache_buf);
            }
            else
            {
                status = HAL_ERROR;
            }
            break;

        case APP_CMD:         /* CMD55 */
            status = HAL_SDMMC_cmd55_app(hsdmmc);
            break;

        /* ACMD6: SET_BUS_WIDTH (CMD55 + CMD6) */
        case SD_APP_SET_BUS_WIDTH:   /* ACMD6 */
            {
                rt_uint32_t bw_arg = 0;
                if (hsdmmc->Init.TransBW == SDMMC_TRANS_BW_4)
                    bw_arg = 2;
                else
                    bw_arg = 0;

                status = HAL_SDMMC_cmd55_app(hsdmmc);
                if (status == HAL_OK)
                {
                    status = (HAL_StatusTypeDef)sdmmc_send_no_data_cmd(hsdmmc,
                        ACMD6_SET_BUS_WIDTH, bw_arg);
                }
            }
            break;

        /* ACMD41: SD_SEND_OP_COND (CMD55 + CMD41) */
        case SD_APP_OP_COND:     /* ACMD41 */
            status = HAL_SDMMC_Acmd41_SendCond(hsdmmc, SD_IO_VCC_3V3);
            break;

        default:
            LOG_W("unsupported cmd %d", cmd->cmd_code);
            cmd->err = -RT_EINVAL;
            break;
        }

        /* read response from registers */
        if (status == HAL_OK)
        {
            cmd->resp[0] = hsdmmc->Instance->SDMMC_RESP0;
            cmd->resp[1] = hsdmmc->Instance->SDMMC_RESP1;
            cmd->resp[2] = hsdmmc->Instance->SDMMC_RESP2;
            cmd->resp[3] = hsdmmc->Instance->SDMMC_RESP3;
            cmd->err = RT_EOK;

            if (data)
            {
                data->err = RT_EOK;
            }

            LOG_D("CMD:%d OK resp[0]:0x%08x", cmd->cmd_code, cmd->resp[0]);
        }
        else
        {
            cmd->err = -RT_ERROR;
            LOG_E("CMD:%d failed, status:%d", cmd->cmd_code, status);
        }
    }

    if (req->stop != RT_NULL)
    {
        struct rt_mmcsd_cmd *stop = req->stop;
        status = (HAL_StatusTypeDef)sdmmc_send_no_data_cmd(hsdmmc,
            CMD12_STOP_STEARM, hsdmmc->SdEmmcRegInfo.rca << 16);

        stop->resp[0] = hsdmmc->Instance->SDMMC_RESP0;
        stop->resp[1] = hsdmmc->Instance->SDMMC_RESP1;
        stop->resp[2] = hsdmmc->Instance->SDMMC_RESP2;
        stop->resp[3] = hsdmmc->Instance->SDMMC_RESP3;
        stop->err = (status == HAL_OK) ? RT_EOK : -RT_ERROR;
    }

    mmcsd_req_complete(host);
}

static void sdmmc_set_iocfg(struct rt_mmcsd_host *host, struct rt_mmcsd_io_cfg *io_cfg)
{
    struct acm32_sdmmc *sdmmc = (struct acm32_sdmmc *)host->private_data;
    SDMMC_HandleTypeDef *hsdmmc = sdmmc->hsdmmc;
    rt_uint32_t clk = io_cfg->clock;
    rt_uint32_t src_clk = SDMMC_CLOCK_FREQ;
    rt_uint32_t div;

    LOG_D("set_iocfg: clk=%u power=%d bus_width=%d timing=%d",
          clk, io_cfg->power_mode, io_cfg->bus_width, io_cfg->timing);

    switch (io_cfg->power_mode & 0x03)
    {
    case MMCSD_POWER_OFF:
        LOG_D("power off");
        hsdmmc->Instance->SDMMC_PWREN &= ~SDMMC_POWER_ON(hsdmmc->Init.Ch);
        return;

    case MMCSD_POWER_UP:
        LOG_D("power up");
        hsdmmc->Instance->SDMMC_PWREN |= SDMMC_POWER_ON(hsdmmc->Init.Ch);
        return;

    case MMCSD_POWER_ON:
        break;

    default:
        break;
    }

    if (clk == 0)
    {
        return;
    }

    if (clk > host->freq_max)
        clk = host->freq_max;

    /* Identification mode */
    if (clk <= SD_INIT_FREQ)
    {
        clk = SD_INIT_FREQ;
    }
    else if (io_cfg->timing == MMCSD_TIMING_SD_HS)
    {
        if (clk > SD_HIGH_SPEED_FREQ)
            clk = SD_HIGH_SPEED_FREQ;
    }
    else if (io_cfg->timing == MMCSD_TIMING_LEGACY)
    {
        if (clk > SD_NORMAL_SPEED_FREQ)
            clk = SD_NORMAL_SPEED_FREQ;
    }

    /* SDMMC clock: SDMMC_CK = SrcClk / (2 * div)
     * For P4xx HAL, the actual formula depends on the divider implementation.
     * div should satisfy: clk >= SrcClk / (2 * div), so div >= SrcClk / (2 * clk)
     * Adding 2 to match HAL_SD_Enum's pattern: div = SrcClk/400000 + 4 / 2
     */
    div = src_clk / (2 * clk);
    if (div == 0)
        div = 1;
    if (div > 0xFF)
        div = 0xFF;

    HAL_SDMMC_SetClk(hsdmmc, div, SDMMC_CLK_SRC_DIV0);

    /* set bus width */
    if (io_cfg->bus_width == MMCSD_BUS_WIDTH_4)
    {
        hsdmmc->Init.TransBW = SDMMC_TRANS_BW_4;
        hsdmmc->Instance->SDMMC_CTYPE &= ~((1UL << hsdmmc->Init.Ch) |
            (1UL << (16 + hsdmmc->Init.Ch)));
        hsdmmc->Instance->SDMMC_CTYPE |= (SDMMC_BW_4 << hsdmmc->Init.Ch);
    }
    else if (io_cfg->bus_width == MMCSD_BUS_WIDTH_8)
    {
        hsdmmc->Init.TransBW = SDMMC_TRANS_BW_8;
        hsdmmc->Instance->SDMMC_CTYPE &= ~((1UL << hsdmmc->Init.Ch) |
            (1UL << (16 + hsdmmc->Init.Ch)));
        hsdmmc->Instance->SDMMC_CTYPE |= (SDMMC_BW_8 << hsdmmc->Init.Ch);
    }
    else
    {
        hsdmmc->Init.TransBW = SDMMC_TRANS_BW_1;
        hsdmmc->Instance->SDMMC_CTYPE &= ~((1UL << hsdmmc->Init.Ch) |
            (1UL << (16 + hsdmmc->Init.Ch)));
        hsdmmc->Instance->SDMMC_CTYPE |= (SDMMC_BW_1 << hsdmmc->Init.Ch);
    }
}

static rt_int32_t sdmmc_get_card_status(struct rt_mmcsd_host *host)
{
    struct acm32_sdmmc *sdmmc = (struct acm32_sdmmc *)host->private_data;
    SDMMC_HandleTypeDef *hsdmmc = sdmmc->hsdmmc;

    if (sdmmc_send_no_data_cmd(hsdmmc, CMD13_GET_STATUS,
        hsdmmc->SdEmmcRegInfo.rca << 16) != RT_EOK)
    {
        return -RT_ERROR;
    }

    return (rt_int32_t)hsdmmc->Instance->SDMMC_RESP0;
}

static void sdmmc_enable_sdio_irq(struct rt_mmcsd_host *host, rt_int32_t en)
{
    struct acm32_sdmmc *sdmmc = (struct acm32_sdmmc *)host->private_data;
    SDMMC_HandleTypeDef *hsdmmc = sdmmc->hsdmmc;

    if (en)
    {
        LOG_D("enable sdmmc irq");
        NVIC_EnableIRQ(SDMMC_IRQn);
    }
    else
    {
        LOG_D("disable sdmmc irq");
        NVIC_DisableIRQ(SDMMC_IRQn);
    }
}

static const struct rt_mmcsd_host_ops sdmmc_ops =
{
    sdmmc_request,
    sdmmc_set_iocfg,
    sdmmc_get_card_status,
    sdmmc_enable_sdio_irq,
    RT_NULL,  /* execute_tuning */
    RT_NULL,  /* card_busy */
    RT_NULL,  /* signal_voltage_switch */
};

static void sdmmc_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* CLK: PC12 AF12, high drive */
    gpio.Pin       = SDMMC1_CLK_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Drive     = GPIO_DRIVE_LEVEL7;
    gpio.Alternate = GPIO_FUNCTION_12;
    HAL_GPIO_Init(SDMMC1_CLK_PORT, &gpio);

    /* CMD: PD2 AF12, pull-up */
    gpio.Pin       = SDMMC1_CMD_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Drive     = GPIO_DRIVE_LEVEL3;
    gpio.Alternate = GPIO_FUNCTION_12;
    HAL_GPIO_Init(SDMMC1_CMD_PORT, &gpio);

    /* DAT[3:0]: PC8-PC11 AF12, pull-up (all on GPIOC) */
    gpio.Pin       = SDMMC1_DAT_PINS;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Drive     = GPIO_DRIVE_LEVEL3;
    gpio.Alternate = GPIO_FUNCTION_12;
    HAL_GPIO_Init(SDMMC1_DAT0_PORT, &gpio);
}

void SDMMC_IRQHandler(void)
{
    rt_interrupt_enter();
    /* P4xx HAL uses polling mode by default, no interrupt handling needed */
    rt_interrupt_leave();
}

int rt_hw_sdio_init(void)
{
    HAL_StatusTypeDef status;
    struct rt_mmcsd_host *host;

    LOG_I("SDMMC init start");

    sdmmc_gpio_init();

    rt_memset(&sdmmc1_handle, 0, sizeof(sdmmc1_handle));
    sdmmc1_handle.Instance = SDMMC;
    sdmmc1_handle.Init.Ch = SDMMC_CH0;
    sdmmc1_handle.Init.CardType = SD_CARD;
    sdmmc1_handle.Init.TransMode = SDMMC_MODE_SDR;
    sdmmc1_handle.Init.TransBW = SDMMC_TRANS_BW_1;
    sdmmc1_handle.Init.IDmaEn = 1;
    sdmmc1_handle.Init.SDVConvEn = 0;
    sdmmc1_handle.Init.SDSigVoltage = SD_IO_VCC_3V3;
    sdmmc1_handle.Init.BusClk = 400000;
    sdmmc1_handle.Init.LowPowerMode = 0;

    status = HAL_SDMMC_Init(&sdmmc1_handle);
    if (status != HAL_OK)
    {
        LOG_E("HAL_SDMMC_Init failed: %d", status);
        return -RT_ERROR;
    }

    HAL_NVIC_SetPriority(SDMMC_IRQn, 2, 0);
    NVIC_EnableIRQ(SDMMC_IRQn);

    LOG_I("enumerating SD card ...");
    status = HAL_SD_Enum(&sdmmc1_handle, SDMMC_CLOCK_FREQ);
    if (status != HAL_OK)
    {
        LOG_E("HAL_SD_Enum failed: %d", status);
        return -RT_ERROR;
    }
    LOG_I("SD card detected, RCA=0x%04x", sdmmc1_handle.SdEmmcRegInfo.rca);

    host = mmcsd_alloc_host();
    if (host == RT_NULL)
    {
        LOG_E("mmcsd_alloc_host failed");
        return -RT_ERROR;
    }

    host->ops = &sdmmc_ops;
    host->freq_min = 400 * 1000;
    host->freq_max = SDIO_MAX_FREQ;
    host->valid_ocr = 0x00FF8000;   /* 2.7V ~ 3.6V */
    host->flags = MMCSD_BUSWIDTH_4 | MMCSD_MUTBLKWRITE | MMCSD_SUP_HIGHSPEED;
    host->max_seg_size = SDIO_BUFF_SIZE;
    host->max_dma_segs = 1;
    host->max_blk_size = 512;
    host->max_blk_count = SDIO_BUFF_SIZE / 512;

    sdmmc1_priv.host = host;
    sdmmc1_priv.hsdmmc = &sdmmc1_handle;
    host->private_data = &sdmmc1_priv;

    sdmmc1_host = host;

    mmcsd_change(host);
    LOG_I("SDMMC init done");

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_sdio_init);

#endif /* RT_USING_SDIO && BSP_USING_SDMMC1 */
