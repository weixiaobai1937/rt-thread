/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2026-07-23     AisinoChip   ACM32P4xx RTC driver
 */

#include "board.h"
#include <sys/time.h>
#include <rtdevice.h>

#if defined(BSP_USING_RTC)

#define DBG_TAG "drv.rtc"
#define DBG_LVL DBG_INFO
#include <rtdbg.h>

rt_inline rt_uint8_t dec2bcd(rt_uint8_t dec)
{
    return (rt_uint8_t)(((dec / 10) << 4) + (dec % 10));
}

rt_inline rt_uint8_t bcd2dec(rt_uint8_t bcd)
{
    return (rt_uint8_t)(((bcd >> 4) * 10) + (bcd & 0x0F));
}

static time_t get_rtc_timestamp(void)
{
    RTC_TimeTypeDef t = {0};
    RTC_TimeTypeDef t2 = {0};
    RTC_DateTypeDef d = {0};
    struct tm tm_new = {0};

    /* HAL SDK rtc_date_time: re-read if second rolls during read */
    HAL_RTC_GetTime(&t);
    HAL_RTC_GetDate(&d);
    HAL_RTC_GetTime(&t2);
    if (t.Second != t2.Second)
    {
        HAL_RTC_GetTime(&t);
        HAL_RTC_GetDate(&d);
    }

    tm_new.tm_sec  = bcd2dec(t.Second);
    tm_new.tm_min  = bcd2dec(t.Minute);
    tm_new.tm_hour = bcd2dec(t.Hour);
    tm_new.tm_mday = bcd2dec(d.Date);
    tm_new.tm_mon  = bcd2dec(d.Month) - 1;
    tm_new.tm_year = bcd2dec(d.Year) + 100;

    return timegm(&tm_new);
}

static rt_err_t set_rtc_time_stamp(time_t time_stamp)
{
    RTC_TimeTypeDef t = {0};
    RTC_DateTypeDef d = {0};
    struct tm now;

    gmtime_r(&time_stamp, &now);
    if (now.tm_year < 100)
    {
        return -RT_ERROR;
    }

    t.Second = dec2bcd((rt_uint8_t)now.tm_sec);
    t.Minute = dec2bcd((rt_uint8_t)now.tm_min);
    t.Hour   = dec2bcd((rt_uint8_t)now.tm_hour);
    d.Date   = dec2bcd((rt_uint8_t)now.tm_mday);
    d.Month  = dec2bcd((rt_uint8_t)(now.tm_mon + 1));
    d.Year   = dec2bcd((rt_uint8_t)(now.tm_year - 100));
    d.WeekDay = (rt_uint8_t)((now.tm_wday == 0) ? 7 : now.tm_wday);

    if (HAL_RTC_SetTime(&t) != HAL_OK)
    {
        LOG_E("HAL_RTC_SetTime failed");
        return -RT_ERROR;
    }
    if (HAL_RTC_SetDate(&d) != HAL_OK)
    {
        LOG_E("HAL_RTC_SetDate failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t _rtc_init(void)
{
    RTC_ConfigTypeDef cfg = {0};

#if defined(BSP_RTC_USING_RC32K)
    cfg.ClockSource = RTC_CLOCK_RC32K;
#else
    cfg.ClockSource = RTC_CLOCK_XTL;
#endif
    cfg.Compensation = COMPENSATION_INCREASE;
    cfg.CompensationValue = 0;

    if (HAL_RTC_Config(&cfg) != HAL_OK)
    {
        LOG_E("HAL_RTC_Config failed");
        return -RT_ERROR;
    }

    return RT_EOK;
}

static rt_err_t _rtc_get_secs(time_t *sec)
{
    RT_ASSERT(sec != RT_NULL);
    *sec = get_rtc_timestamp();
    return RT_EOK;
}

static rt_err_t _rtc_set_secs(time_t *sec)
{
    RT_ASSERT(sec != RT_NULL);
    return set_rtc_time_stamp(*sec);
}

static const struct rt_rtc_ops acm32_rtc_ops =
{
    _rtc_init,
    _rtc_get_secs,
    _rtc_set_secs,
    RT_NULL,
    RT_NULL,
    RT_NULL,
    RT_NULL,
};

static rt_rtc_dev_t acm32_rtc_dev;

static int rt_hw_rtc_init(void)
{
    acm32_rtc_dev.ops = &acm32_rtc_ops;
    return rt_hw_rtc_register(&acm32_rtc_dev, "rtc", RT_DEVICE_FLAG_RDWR, RT_NULL);
}
INIT_DEVICE_EXPORT(rt_hw_rtc_init);

#endif /* BSP_USING_RTC */
