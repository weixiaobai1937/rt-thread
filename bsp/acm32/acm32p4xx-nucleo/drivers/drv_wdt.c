/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx WDT/IWDT driver
 */

#include <board.h>
#include <rtthread.h>
#include <rtdevice.h>

#ifdef RT_USING_WDT
#if defined(BSP_USING_WDT) || defined(BSP_USING_IWDT)

#define DBG_TAG "drv.wdt"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

#define TYPE_WDT     0
#define TYPE_IWDT    1
#define IWDT_FREQ    (32000U)

struct acm32_wdt_obj
{
    union
    {
        WDT_HandleTypeDef  wdt;
        IWDT_HandleTypeDef iwdt;
    } handle;
    rt_uint16_t   is_start;
    rt_uint16_t   type;
    rt_watchdog_t watchdog;
};

#ifdef BSP_USING_WDT
static struct acm32_wdt_obj acm32_wdt;
#endif
#ifdef BSP_USING_IWDT
static struct acm32_wdt_obj acm32_iwdt;
#endif

/* Prescaler bitfield values and corresponding 2^n shift */
static const uint32_t wdt_psc_val[] =
{
    WDT_CLOCK_PRESCALER_1,
    WDT_CLOCK_PRESCALER_2,
    WDT_CLOCK_PRESCALER_4,
    WDT_CLOCK_PRESCALER_8,
    WDT_CLOCK_PRESCALER_16,
    WDT_CLOCK_PRESCALER_32,
    WDT_CLOCK_PRESCALER_64,
    WDT_CLOCK_PRESCALER_128,
};

static const uint32_t iwdt_psc_val[] =
{
    IWDT_CLOCK_PRESCALER_4,
    IWDT_CLOCK_PRESCALER_8,
    IWDT_CLOCK_PRESCALER_16,
    IWDT_CLOCK_PRESCALER_32,
    IWDT_CLOCK_PRESCALER_64,
    IWDT_CLOCK_PRESCALER_128,
    IWDT_CLOCK_PRESCALER_256,
};

/*
 * HAL SDK: resetTime = PCLK1 / 4 with PRESCALER_4 -> ~1s
 * General: Load = sec * PCLK / 2^div
 */
static rt_err_t calc_wdt_load(rt_uint32_t pclk, rt_uint32_t sec,
                              uint32_t *psc, uint32_t *load)
{
    rt_uint32_t i;
    rt_uint64_t ticks;

    if ((sec == 0) || (pclk == 0))
    {
        return -RT_EINVAL;
    }

    for (i = 0; i < (sizeof(wdt_psc_val) / sizeof(wdt_psc_val[0])); i++)
    {
        ticks = ((rt_uint64_t)sec * pclk) >> i;
        if ((ticks > 0) && (ticks <= 0xFFFFFFFFULL))
        {
            *psc = wdt_psc_val[i];
            *load = (uint32_t)ticks;
            return RT_EOK;
        }
    }
    return -RT_ERROR;
}

static rt_err_t calc_iwdt_load(rt_uint32_t sec, uint32_t *psc, uint32_t *load)
{
    rt_uint32_t i;
    rt_uint32_t div;
    rt_uint64_t ticks;

    if (sec == 0)
    {
        return -RT_EINVAL;
    }

    /* LSI ~32kHz, prescaler starts at /4 */
    for (i = 0; i < (sizeof(iwdt_psc_val) / sizeof(iwdt_psc_val[0])); i++)
    {
        div = 4U << i;
        ticks = ((rt_uint64_t)sec * IWDT_FREQ) / div;
        if ((ticks > 0) && (ticks <= IWDT_RELOAD_MAX_VALUE))
        {
            *psc = iwdt_psc_val[i];
            *load = (uint32_t)ticks;
            return RT_EOK;
        }
    }
    return -RT_ERROR;
}

static rt_uint32_t wdt_timeout_get(rt_uint32_t pclk, uint32_t psc, uint32_t load)
{
    rt_uint32_t i;
    for (i = 0; i < (sizeof(wdt_psc_val) / sizeof(wdt_psc_val[0])); i++)
    {
        if (wdt_psc_val[i] == psc)
        {
            if (pclk == 0)
            {
                return 0;
            }
            return (rt_uint32_t)(((rt_uint64_t)load << i) / pclk);
        }
    }
    return 0;
}

static rt_uint32_t iwdt_timeout_get(uint32_t psc, uint32_t load)
{
    rt_uint32_t i;
    rt_uint32_t div;
    for (i = 0; i < (sizeof(iwdt_psc_val) / sizeof(iwdt_psc_val[0])); i++)
    {
        if (iwdt_psc_val[i] == psc)
        {
            div = 4U << i;
            return (rt_uint32_t)(((rt_uint64_t)load * div) / IWDT_FREQ);
        }
    }
    return 0;
}

void HAL_WDT_MspInit(WDT_HandleTypeDef *hwdt)
{
    RT_UNUSED(hwdt);
    /* HAL SDK wdt_reset.c */
    HAL_RCC_WDTResetConfig(ENABLE);
    __HAL_RCC_WDT_CLK_ENABLE();
    __HAL_RCC_WDT_RESET();
}

void HAL_IWDT_MspInit(IWDT_HandleTypeDef *hiwdt)
{
    RT_UNUSED(hiwdt);
    /* HAL SDK iwdt_reset.c */
    HAL_RCC_RCLConfig(ENABLE);
    HAL_RCC_IWDTResetConfig(ENABLE);
}

static rt_err_t wdt_init(rt_watchdog_t *wdt)
{
    struct acm32_wdt_obj *obj;

    RT_ASSERT(wdt != RT_NULL);
    obj = rt_container_of(wdt, struct acm32_wdt_obj, watchdog);

    /* 初始化看门狗硬件配置 */
    if (obj->type == TYPE_WDT)
    {
        obj->handle.wdt.Instance = WDT;
    }
    else
    {
        obj->handle.iwdt.Instance = IWDT;
    }

    return RT_EOK;
}

static rt_err_t wdt_control(rt_watchdog_t *wdt, int cmd, void *arg)
{
    struct acm32_wdt_obj *obj;
    rt_uint32_t pclk;
    uint32_t psc, load;

    RT_ASSERT(wdt != RT_NULL);
    obj = rt_container_of(wdt, struct acm32_wdt_obj, watchdog);
    pclk = HAL_RCC_GetPCLK1Freq();

    switch (cmd)
    {
    case RT_DEVICE_CTRL_WDT_KEEPALIVE:
        if (obj->type == TYPE_WDT)
        {
            HAL_WDT_Refresh(&obj->handle.wdt);
        }
        else
        {
            HAL_IWDT_Refresh(&obj->handle.iwdt);
        }
        break;

    case RT_DEVICE_CTRL_WDT_SET_TIMEOUT:
        if (arg == RT_NULL)
        {
            return -RT_EINVAL;
        }
        if (obj->type == TYPE_WDT)
        {
            if (calc_wdt_load(pclk, *(rt_uint32_t *)arg, &psc, &load) != RT_EOK)
            {
                return -RT_ERROR;
            }
            obj->handle.wdt.Init.Prescaler = psc;
            obj->handle.wdt.Init.Load = load;
            obj->handle.wdt.Init.Mode = WDT_MODE_RESET;
            obj->handle.wdt.Init.IntClrTime = 0xFFFF;
            if (obj->is_start)
            {
                obj->handle.wdt.Instance = WDT;
                HAL_WDT_Init(&obj->handle.wdt);
                HAL_WDT_Refresh(&obj->handle.wdt);
            }
        }
        else
        {
            if (calc_iwdt_load(*(rt_uint32_t *)arg, &psc, &load) != RT_EOK)
            {
                return -RT_ERROR;
            }
            /* SDK: Window/Wakeup >= Reload disables window/wakeup */
            obj->handle.iwdt.Init.Prescaler = psc;
            obj->handle.iwdt.Init.Reload = load;
            obj->handle.iwdt.Init.Window = load;
            obj->handle.iwdt.Init.Wakeup = load;
            if (obj->is_start)
            {
                obj->handle.iwdt.Instance = IWDT;
                HAL_IWDT_Init(&obj->handle.iwdt);
                HAL_IWDT_Refresh(&obj->handle.iwdt);
            }
        }
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMEOUT:
        if (arg == RT_NULL)
        {
            return -RT_EINVAL;
        }
        if (obj->type == TYPE_WDT)
        {
            *(rt_uint32_t *)arg = wdt_timeout_get(pclk,
                                  obj->handle.wdt.Init.Prescaler,
                                  obj->handle.wdt.Init.Load);
        }
        else
        {
            *(rt_uint32_t *)arg = iwdt_timeout_get(obj->handle.iwdt.Init.Prescaler,
                                  obj->handle.iwdt.Init.Reload);
        }
        break;

    case RT_DEVICE_CTRL_WDT_GET_TIMELEFT:
        if (arg == RT_NULL)
        {
            return -RT_EINVAL;
        }
        if (obj->type == TYPE_WDT)
        {
            *(rt_uint32_t *)arg = wdt_timeout_get(pclk,
                                  obj->handle.wdt.Init.Prescaler,
                                  obj->handle.wdt.Instance->COUNT);
        }
        else
        {
            return -RT_EINVAL;
        }
        break;

    case RT_DEVICE_CTRL_WDT_START:
        if (obj->type == TYPE_WDT)
        {
            if (obj->handle.wdt.Init.Load == 0)
            {
                /* default ~1s */
                if (calc_wdt_load(pclk, 1, &psc, &load) != RT_EOK)
                    return -RT_ERROR;
                obj->handle.wdt.Init.Prescaler = psc;
                obj->handle.wdt.Init.Load = load;
            }
            obj->handle.wdt.Instance = WDT;
            obj->handle.wdt.Init.Mode = WDT_MODE_RESET;
            obj->handle.wdt.Init.IntClrTime = 0xFFFF;
            HAL_WDT_Init(&obj->handle.wdt);
        }
        else
        {
            if (obj->handle.iwdt.Init.Reload == 0)
            {
                /* SDK default-ish: /32 * 4000 ≈ 4s @32kHz */
                if (calc_iwdt_load(4, &psc, &load) != RT_EOK)
                    return -RT_ERROR;
                obj->handle.iwdt.Init.Prescaler = psc;
                obj->handle.iwdt.Init.Reload = load;
            }
            obj->handle.iwdt.Instance = IWDT;
            obj->handle.iwdt.Init.Window = obj->handle.iwdt.Init.Reload;
            obj->handle.iwdt.Init.Wakeup = obj->handle.iwdt.Init.Reload;
            HAL_IWDT_Init(&obj->handle.iwdt);
            HAL_IWDT_Refresh(&obj->handle.iwdt);
        }
        obj->is_start = 1;
        break;

    case RT_DEVICE_CTRL_WDT_STOP:
        if (obj->type == TYPE_WDT)
        {
            obj->handle.wdt.Instance->CTRL &= ~WDT_CTRL_EN;
            obj->is_start = 0;
        }
        else
        {
            /* IWDT 一旦启动无法停止，返回错误并保持 is_start 不变 */
            LOG_W("IWDT cannot be stopped once started");
            return -RT_EPERM;
        }
        break;

    default:
        return -RT_ERROR;
    }

    return RT_EOK;
}

static struct rt_watchdog_ops ops =
{
    .init = wdt_init,
    .control = wdt_control,
};

static int rt_hw_wdt_init(void)
{
#ifdef BSP_USING_WDT
    acm32_wdt.type = TYPE_WDT;
    acm32_wdt.is_start = 0;
    acm32_wdt.watchdog.ops = &ops;
    if (rt_hw_watchdog_register(&acm32_wdt.watchdog, "wdt",
                                RT_DEVICE_FLAG_DEACTIVATE, RT_NULL) != RT_EOK)
    {
        LOG_E("wdt register failed");
        return -RT_ERROR;
    }
#endif
#ifdef BSP_USING_IWDT
    acm32_iwdt.type = TYPE_IWDT;
    acm32_iwdt.is_start = 0;
    acm32_iwdt.watchdog.ops = &ops;
    if (rt_hw_watchdog_register(&acm32_iwdt.watchdog, "iwdt",
                                RT_DEVICE_FLAG_DEACTIVATE, RT_NULL) != RT_EOK)
    {
        LOG_E("iwdt register failed");
        return -RT_ERROR;
    }
#endif
    return RT_EOK;
}
INIT_BOARD_EXPORT(rt_hw_wdt_init);

#endif /* BSP_USING_WDT || BSP_USING_IWDT */
#endif /* RT_USING_WDT */
