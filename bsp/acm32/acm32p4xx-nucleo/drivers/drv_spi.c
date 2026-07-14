/*
 * Copyright (c) 2006-2024, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-14     AisinoChip   ACM32P4xx-Nucleo SPI1 master poll + soft CS
 */

#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include "board.h"
#include "spi_config.h"

#ifdef BSP_USING_SPI1

#include "hal_spi.h"
#include "hal_gpio.h"
#include "hal_rcc.h"

#define DBG_TAG "drv.spi"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define SPI_XFER_TIMEOUT_MS    1000U

struct acm32_spi_config
{
    const char      *bus_name;
    SPI_TypeDef     *Instance;
    IRQn_Type        irq_type;
    GPIO_TypeDef    *sck_port;
    rt_uint32_t      sck_pin;
    rt_uint32_t      sck_af;
    GPIO_TypeDef    *mosi_port;
    rt_uint32_t      mosi_pin;
    rt_uint32_t      mosi_af;
    GPIO_TypeDef    *miso_port;
    rt_uint32_t      miso_pin;
    rt_uint32_t      miso_af;
};

struct acm32_spi
{
    SPI_HandleTypeDef            handle;
    struct acm32_spi_config     *config;
    struct rt_spi_configuration *cfg;
    struct rt_spi_bus            spi_bus;
};

enum
{
    SPI1_INDEX,
    SPI_MAX_INDEX
};

static struct acm32_spi_config spi_config[] =
{
    SPI1_BUS_CONFIG,
};

static struct acm32_spi spi_bus_obj[SPI_MAX_INDEX] = {0};

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi)
{
    GPIO_InitTypeDef gpio = {0};
    struct acm32_spi *spi;
    struct acm32_spi_config *c;
    int i;

    if (hspi == RT_NULL)
        return;

    spi = RT_NULL;
    for (i = 0; i < SPI_MAX_INDEX; i++)
    {
        if (spi_bus_obj[i].handle.Instance == hspi->Instance ||
            spi_config[i].Instance == hspi->Instance)
        {
            spi = &spi_bus_obj[i];
            break;
        }
    }
    if (spi == RT_NULL)
        return;

    c = spi->config ? spi->config : &spi_config[i];

    if (hspi->Instance == SPI1)
    {
        __HAL_RCC_GPIOE_CLK_ENABLE();
        __HAL_RCC_SPI1_CLK_ENABLE();
    }
    else
    {
        return;
    }

    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_PULLUP;
    gpio.Drive = GPIO_DRIVE_LEVEL3;

    gpio.Pin = c->sck_pin;
    gpio.Alternate = c->sck_af;
    HAL_GPIO_Init(c->sck_port, &gpio);

    gpio.Pin = c->mosi_pin;
    gpio.Alternate = c->mosi_af;
    HAL_GPIO_Init(c->mosi_port, &gpio);

    gpio.Pin = c->miso_pin;
    gpio.Alternate = c->miso_af;
    HAL_GPIO_Init(c->miso_port, &gpio);
}

static rt_uint32_t acm32_spi_get_src_clk(void)
{
    /* SPI1 on AHB1; stage-1 uses SystemCoreClock as source estimate */
    extern volatile uint32_t SystemCoreClock;
    return SystemCoreClock;
}

static rt_uint32_t acm32_spi_baud_prescaler(rt_uint32_t max_hz)
{
    static const rt_uint32_t table[] = {2, 4, 6, 8, 16, 32, 64, 128, 254};
    rt_uint32_t src = acm32_spi_get_src_clk();
    rt_size_t i;

    if (max_hz == 0)
        max_hz = 1;

    for (i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if ((src / table[i]) <= max_hz)
            return table[i];
    }
    return 254;
}

static rt_err_t acm32_spi_init(struct acm32_spi *spi_drv, struct rt_spi_configuration *cfg)
{
    SPI_HandleTypeDef *hspi = &spi_drv->handle;
    rt_uint32_t mode = cfg->mode;

    if (cfg->data_width != 8)
        return -RT_EINVAL;

    if (mode & RT_SPI_SLAVE)
        return -RT_EINVAL;

    hspi->Instance = spi_drv->config->Instance;
    hspi->CSx = SPI_CS_CS0; /* HAL_SPI_Init asserts CSx; HW CS pin unused, soft CS separate */
    hspi->Init.SPI_Mode = SPI_MODE_MASTER;
    hspi->Init.X_Mode = SPI_1X_MODE;
    hspi->Init.Slave_SofteCs_En = SPI_SLAVE_SOFT_CS_DISABLE;
    hspi->Init.Master_SShift = SPI_MASTER_SSHIFT_NONE;
    hspi->Init.Sid_Rst_Val = 0;
    hspi->Init.BaudRate_Prescaler = acm32_spi_baud_prescaler(cfg->max_hz);

    if (mode & RT_SPI_MSB)
        hspi->Init.First_Bit = SPI_FIRSTBIT_MSB;
    else
        hspi->Init.First_Bit = SPI_FIRSTBIT_LSB;

    switch (mode & (RT_SPI_CPOL | RT_SPI_CPHA))
    {
    case RT_SPI_MODE_0:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_0;
        break;
    case RT_SPI_MODE_1:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_1;
        break;
    case RT_SPI_MODE_2:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_2;
        break;
    case RT_SPI_MODE_3:
    default:
        hspi->Init.SPI_Work_Mode = SPI_WORK_MODE_3;
        break;
    }

    if (HAL_SPI_Init(hspi) != HAL_OK)
        return -RT_EIO;

    spi_drv->cfg = cfg;
    return RT_EOK;
}

static rt_err_t spi_configure(struct rt_spi_device *device,
                              struct rt_spi_configuration *configuration)
{
    struct acm32_spi *spi_drv;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(configuration != RT_NULL);

    spi_drv = rt_container_of(device->bus, struct acm32_spi, spi_bus);
    return acm32_spi_init(spi_drv, configuration);
}

static rt_ssize_t spixfer(struct rt_spi_device *device, struct rt_spi_message *message)
{
    struct acm32_spi *spi_drv;
    SPI_HandleTypeDef *hspi;
    HAL_StatusTypeDef state = HAL_OK;
    rt_uint8_t *recv_buf;
    const rt_uint8_t *send_buf;

    RT_ASSERT(device != RT_NULL);
    RT_ASSERT(device->bus != RT_NULL);
    RT_ASSERT(message != RT_NULL);

    spi_drv = rt_container_of(device->bus, struct acm32_spi, spi_bus);
    hspi = &spi_drv->handle;

    if (message->cs_take && !(device->config.mode & RT_SPI_NO_CS) &&
        (device->cs_pin != PIN_NONE))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            rt_pin_write(device->cs_pin, PIN_HIGH);
        else
            rt_pin_write(device->cs_pin, PIN_LOW);
    }

    recv_buf = message->recv_buf;
    send_buf = message->send_buf;

    if (message->length > 0)
    {
        if (send_buf && recv_buf)
        {
            state = HAL_SPI_TransmitReceive(hspi, (uint8_t *)send_buf, recv_buf,
                                            message->length, SPI_XFER_TIMEOUT_MS);
        }
        else if (send_buf)
        {
            state = HAL_SPI_Transmit(hspi, (uint8_t *)send_buf,
                                     message->length, SPI_XFER_TIMEOUT_MS);
        }
        else if (recv_buf)
        {
            /* Master RX needs clocks: full-duplex with 0xFF dummy TX */
            rt_uint8_t *dummy = RT_NULL;
            dummy = (rt_uint8_t *)rt_malloc(message->length);
            if (dummy == RT_NULL)
            {
                state = HAL_ERROR;
            }
            else
            {
                rt_memset(dummy, 0xFF, message->length);
                state = HAL_SPI_TransmitReceive(hspi, dummy, recv_buf,
                                                message->length, SPI_XFER_TIMEOUT_MS);
                rt_free(dummy);
            }
        }
    }

    if (message->cs_release && !(device->config.mode & RT_SPI_NO_CS) &&
        (device->cs_pin != PIN_NONE))
    {
        if (device->config.mode & RT_SPI_CS_HIGH)
            rt_pin_write(device->cs_pin, PIN_LOW);
        else
            rt_pin_write(device->cs_pin, PIN_HIGH);
    }

    if (state != HAL_OK)
        return 0;

    return message->length;
}

static const struct rt_spi_ops acm32_spi_ops =
{
    .configure = spi_configure,
    .xfer = spixfer,
};

rt_err_t rt_hw_spi_device_attach(const char *bus_name, const char *device_name, rt_base_t cs_pin)
{
    struct rt_spi_device *spi_device;
    rt_err_t result;

    RT_ASSERT(bus_name != RT_NULL);
    RT_ASSERT(device_name != RT_NULL);

    spi_device = (struct rt_spi_device *)rt_malloc(sizeof(struct rt_spi_device));
    if (spi_device == RT_NULL)
        return -RT_ENOMEM;
    rt_memset(spi_device, 0, sizeof(struct rt_spi_device));

    result = rt_spi_bus_attach_device_cspin(spi_device, device_name, bus_name, cs_pin, RT_NULL);
    if (result != RT_EOK)
    {
        rt_free(spi_device);
        LOG_E("%s attach to %s failed: %d", device_name, bus_name, result);
        return result;
    }

    /* Idle CS high for active-low devices (default) */
    if (cs_pin != PIN_NONE)
        rt_pin_write(cs_pin, PIN_HIGH);

    LOG_D("%s attach to %s, cs=%d", device_name, bus_name, (int)cs_pin);
    return RT_EOK;
}

int rt_hw_spi_init(void)
{
    rt_err_t result;
    rt_size_t i;

    for (i = 0; i < sizeof(spi_config) / sizeof(spi_config[0]); i++)
    {
        spi_bus_obj[i].config = &spi_config[i];
        spi_bus_obj[i].handle.Instance = spi_config[i].Instance;
        spi_bus_obj[i].spi_bus.parent.user_data = &spi_config[i];

        result = rt_spi_bus_register(&spi_bus_obj[i].spi_bus,
                                     spi_config[i].bus_name,
                                     &acm32_spi_ops);
        RT_ASSERT(result == RT_EOK);
        LOG_D("%s bus register done", spi_config[i].bus_name);
    }
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_spi_init);

#endif /* BSP_USING_SPI1 */
