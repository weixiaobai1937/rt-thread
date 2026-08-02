/*
 * Copyright (c) 2006-2026, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "hal.h"

#ifdef HAL_HRNG_MODULE_ENABLED

#define HRNG_CTRL_RBG0_EN       (1U << 0)
#define HRNG_CTRL_RBG1_EN       (1U << 1)
#define HRNG_CTRL_LFSRCLK_SEL   (1U << 2)

void HAL_HRNG_Init(void)
{
    __HAL_RCC_HRNG_CLK_ENABLE();
    HRNG->CTRL |= HRNG_CTRL_RBG0_EN | HRNG_CTRL_RBG1_EN | HRNG_CTRL_LFSRCLK_SEL;
}

void HAL_HRNG_DeInit(void)
{
    HRNG->CTRL &= ~(HRNG_CTRL_RBG0_EN | HRNG_CTRL_RBG1_EN | HRNG_CTRL_LFSRCLK_SEL);
    __HAL_RCC_HRNG_CLK_DISABLE();
}

uint32_t HAL_HRNG_GetHrng_32(void)
{
    return HRNG->LFSR;
}

uint8_t HAL_HRNG_GetHrng_8(void)
{
    return (uint8_t)(HRNG->LFSR & 0xFFU);
}

uint8_t HAL_HRNG_GetHrng(uint8_t *hdata, uint32_t byte_len)
{
    uint32_t i;

    if (hdata == NULL)
        return 1;

    for (i = 0; i < byte_len; i++)
    {
        hdata[i] = (uint8_t)(HAL_HRNG_GetHrng_32() & 0xFFU);
    }
    return 0;
}

#endif /* HAL_HRNG_MODULE_ENABLED */
