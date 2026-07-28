/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx I2C master driver
 */

#include "board.h"

#if defined(RT_USING_I2C)
#if defined(BSP_USING_I2C1) || defined(BSP_USING_I2C2)

#include <rtdevice.h>
#include "i2c_config.h"

#define DBG_TAG "drv.i2c"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

enum
{
#ifdef BSP_USING_I2C1
    I2C1_INDEX,
#endif
#ifdef BSP_USING_I2C2
    I2C2_INDEX,
#endif
    I2C_MAX_INDEX
};

struct acm32_i2c_config
{
    I2C_TypeDef      *Instance;
    const char       *name;
    IRQn_Type         irq_type;
    uint32_t          clock_speed;
    GPIO_TypeDef     *scl_port;
    uint32_t          scl_pin;
    uint32_t          scl_af;
    GPIO_TypeDef     *sda_port;
    uint32_t          sda_pin;
    uint32_t          sda_af;
};

struct acm32_i2c
{
    I2C_HandleTypeDef        handle;
    struct acm32_i2c_config *config;
    struct rt_i2c_bus_device i2c_bus;
    struct rt_mutex          lock;
};

static struct acm32_i2c_config i2c_config[] =
{
#ifdef BSP_USING_I2C1
    I2C1_BUS_CONFIG,
#endif
#ifdef BSP_USING_I2C2
    I2C2_BUS_CONFIG,
#endif
};

static struct acm32_i2c i2c_objs[sizeof(i2c_config) / sizeof(i2c_config[0])] = {0};

/* Lookup i2c_obj by HAL handle Instance pointer (safe alternative to rt_container_of) */
static struct acm32_i2c *i2c_find(I2C_TypeDef *Instance)
{
    size_t i;
    for (i = 0; i < sizeof(i2c_objs) / sizeof(i2c_objs[0]); i++)
    {
        if (i2c_objs[i].config && i2c_objs[i].config->Instance == Instance)
            return &i2c_objs[i];
    }
    return NULL;
}

static void acm32_i2c_gpio_clk_enable(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (port == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (port == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (port == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
}

static void acm32_i2c_periph_clk_enable(I2C_TypeDef *inst)
{
    if (inst == I2C1)
        __HAL_RCC_I2C1_CLK_ENABLE();
#ifdef BSP_USING_I2C2
    else if (inst == I2C2)
        __HAL_RCC_I2C2_CLK_ENABLE();
#endif
}

/* HAL expects 8-bit address (7-bit << 1); RT-Thread msg->addr is 7-bit. */
static rt_uint16_t acm32_i2c_addr8(struct rt_i2c_msg *msg)
{
    return (rt_uint16_t)(msg->addr << 1);
}

/* 根据数据长度和时钟速度计算 I2C 操作超时时间（ms） */
static uint32_t acm32_i2c_calc_timeout(struct acm32_i2c *hi2c, rt_uint16_t data_byte)
{
    /* 最坏情况：每字节 10 bits (8 data + ACK + 开销)，加上 start/stop */
    rt_uint32_t bits = 10UL * data_byte + 20UL;
    rt_uint32_t timeout = (bits * 1000UL) / (hi2c->config->clock_speed / 1000UL);
    /* 至少 10ms，最多 1000ms */
    if (timeout < 10) timeout = 10;
    if (timeout > 1000) timeout = 1000;
    return timeout;
}

static int acm32_i2c_read(struct acm32_i2c *hi2c, rt_uint16_t slave_address,
                          rt_uint8_t *p_buffer, rt_uint16_t data_byte)
{
    uint32_t timeout = acm32_i2c_calc_timeout(hi2c, data_byte);
    if (HAL_I2C_Master_Receive(&hi2c->handle, slave_address, p_buffer, data_byte, timeout) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

static int acm32_i2c_write(struct acm32_i2c *hi2c, rt_uint16_t slave_address,
                           rt_uint8_t *p_buffer, rt_uint16_t data_byte)
{
    uint32_t timeout = acm32_i2c_calc_timeout(hi2c, data_byte);
    if (HAL_I2C_Master_Transmit(&hi2c->handle, slave_address, p_buffer, data_byte, timeout) != HAL_OK)
    {
        return -1;
    }
    return 0;
}

static rt_ssize_t _i2c_xfer(struct rt_i2c_bus_device *bus, struct rt_i2c_msg msgs[], rt_uint32_t num)
{
    struct rt_i2c_msg *msg;
    rt_uint32_t i;
    struct acm32_i2c *i2c_obj;
    rt_uint16_t addr8;

    RT_ASSERT(bus != RT_NULL);
    RT_ASSERT(msgs != RT_NULL);

    i2c_obj = rt_container_of(bus, struct acm32_i2c, i2c_bus);

    /* 获取互斥锁保护总线事务 */
    rt_mutex_take(&i2c_obj->lock, RT_WAITING_FOREVER);

    /* 优化：检测 write-then-read 模式（同一设备），使用 HAL_I2C_Mem_Read 发送 Sr */
    if (num == 2 &&
        !(msgs[0].flags & RT_I2C_RD) && (msgs[1].flags & RT_I2C_RD) &&
        msgs[0].addr == msgs[1].addr &&
        msgs[0].len > 0)
    {
        addr8 = acm32_i2c_addr8(&msgs[0]);
        rt_uint16_t mem_size = (msgs[0].len >= 2) ? I2C_MEMADD_SIZE_16BIT : I2C_MEMADD_SIZE_8BIT;
        rt_uint16_t mem_addr = (msgs[0].len >= 2)
            ? (rt_uint16_t)((msgs[0].buf[0] << 8) | msgs[0].buf[1])
            : (rt_uint16_t)msgs[0].buf[0];

        if (HAL_I2C_Mem_Read(&i2c_obj->handle, addr8, mem_addr, mem_size,
                             msgs[1].buf, msgs[1].len,
                             acm32_i2c_calc_timeout(i2c_obj, msgs[0].len + msgs[1].len)) == HAL_OK)
        {
            rt_mutex_release(&i2c_obj->lock);
            return 2;
        }
        /* Mem_Read 失败，回退到逐消息传输 */
    }

    for (i = 0; i < num; i++)
    {
        msg = &msgs[i];
        addr8 = acm32_i2c_addr8(msg);

        if (msg->flags & RT_I2C_RD)
        {
            if (acm32_i2c_read(i2c_obj, addr8, msg->buf, msg->len) != 0)
            {
                break;
            }
        }
        else
        {
            if (acm32_i2c_write(i2c_obj, addr8, msg->buf, msg->len) != 0)
            {
                break;
            }
        }
    }

    rt_mutex_release(&i2c_obj->lock);

    return (rt_ssize_t)i;
}

static const struct rt_i2c_bus_device_ops i2c_ops =
{
    _i2c_xfer,
    RT_NULL,
    RT_NULL
};

void HAL_I2C_MspInit(I2C_HandleTypeDef *hi2c)
{
    GPIO_InitTypeDef GPIO_Handle = {0};
    struct acm32_i2c *i2c_obj;
    struct acm32_i2c_config *cfg;

    RT_ASSERT(hi2c != RT_NULL);

    i2c_obj = i2c_find(hi2c->Instance);
    if (i2c_obj == RT_NULL || i2c_obj->config == RT_NULL)
        return;

    cfg = i2c_obj->config;

    acm32_i2c_periph_clk_enable(cfg->Instance);
    acm32_i2c_gpio_clk_enable(cfg->scl_port);
    acm32_i2c_gpio_clk_enable(cfg->sda_port);

    GPIO_Handle.Mode = GPIO_MODE_AF_OD;
    GPIO_Handle.Pull = GPIO_PULLUP;
    GPIO_Handle.Drive = GPIO_DRIVE_LEVEL3;

    GPIO_Handle.Pin = cfg->sda_pin;
    GPIO_Handle.Alternate = cfg->sda_af;
    HAL_GPIO_Init(cfg->sda_port, &GPIO_Handle);

    GPIO_Handle.Pin = cfg->scl_pin;
    GPIO_Handle.Alternate = cfg->scl_af;
    HAL_GPIO_Init(cfg->scl_port, &GPIO_Handle);

    NVIC_ClearPendingIRQ(cfg->irq_type);
    NVIC_EnableIRQ(cfg->irq_type);
}

int rt_hw_i2c_init(void)
{
    rt_err_t result;
    int i;

    for (i = 0; i < (int)(sizeof(i2c_config) / sizeof(i2c_config[0])); i++)
    {
        i2c_objs[i].config = &i2c_config[i];
        i2c_objs[i].handle.Instance = i2c_config[i].Instance;
        i2c_objs[i].i2c_bus.ops = &i2c_ops;

        /* 初始化互斥锁 */
        rt_mutex_init(&i2c_objs[i].lock, i2c_config[i].name, RT_IPC_FLAG_FIFO);

        /* Align with HAL SDK I2C Master example (app.c / MspInit) */
        i2c_objs[i].handle.Init.I2C_Mode = I2C_MODE_MASTER;
        i2c_objs[i].handle.Init.Tx_Auto_En = TX_AUTO_ENABLE;
        i2c_objs[i].handle.Init.Stretch_Mode = STRETCH_MODE_DISABLE;
        i2c_objs[i].handle.Init.Own_Address = 0;
        i2c_objs[i].handle.Init.Clock_Speed = i2c_config[i].clock_speed;
        i2c_objs[i].handle.Init.filter_enable = FILTER_ALGO_DISABLE;

        if (HAL_I2C_Init(&i2c_objs[i].handle) != HAL_OK)
        {
            LOG_E("%s HAL_I2C_Init failed, skipping", i2c_config[i].name);
            continue;
        }

        result = rt_i2c_bus_device_register(&i2c_objs[i].i2c_bus, i2c_config[i].name);
        if (result != RT_EOK)
        {
            LOG_E("%s register failed, skipping", i2c_config[i].name);
            continue;
        }

        LOG_I("%s init ok", i2c_config[i].name);
    }

    return RT_EOK;
}
INIT_DEVICE_EXPORT(rt_hw_i2c_init);

#endif /* BSP_USING_I2C1 || BSP_USING_I2C2 */
#endif /* RT_USING_I2C */
