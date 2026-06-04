/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author            Notes
 * 2021-09-18     AisinoChip        first version
 * 2026-06-04     AisinoChip        add ACM32P4xx support
 */

#include <rthw.h>
#include <rtdevice.h>
#include "board.h"

#ifdef RT_USING_PIN

#define __ACM32_PIN(index, gpio, gpio_index)                                \
    {                                                                       \
        index, GPIO##gpio, GPIO_PIN_##gpio_index                            \
    }

#define __ACM32_PIN_RESERVE                                                 \
    {                                                                       \
        -1, RT_NULL, 0                                                      \
    }

/* ACM32 GPIO driver */
struct pin_index
{
    int             index;
    GPIO_TypeDef    *gpio;
    uint32_t        pin;
};

struct pin_irq_map
{
    rt_uint16_t     line;
    GPIO_TypeDef    *gpio;
};

static const struct pin_index pins[] =
{
#if defined(BSP_USING_GPIO1)
    __ACM32_PIN(0,   A, 0),
    __ACM32_PIN(1,   A, 1),
    __ACM32_PIN(2,   A, 2),
    __ACM32_PIN(3,   A, 3),
    __ACM32_PIN(4,   A, 4),
    __ACM32_PIN(5,   A, 5),
    __ACM32_PIN(6,   A, 6),
    __ACM32_PIN(7,   A, 7),
    __ACM32_PIN(8,   A, 8),
    __ACM32_PIN(9,   A, 9),
    __ACM32_PIN(10,  A, 10),
    __ACM32_PIN(11,  A, 11),
    __ACM32_PIN(12,  A, 12),
    __ACM32_PIN(13,  A, 13),
    __ACM32_PIN(14,  A, 14),
    __ACM32_PIN(15,  A, 15),
    __ACM32_PIN(16,  B, 0),
    __ACM32_PIN(17,  B, 1),
    __ACM32_PIN(18,  B, 2),
    __ACM32_PIN(19,  B, 3),
    __ACM32_PIN(20,  B, 4),
    __ACM32_PIN(21,  B, 5),
    __ACM32_PIN(22,  B, 6),
    __ACM32_PIN(23,  B, 7),
    __ACM32_PIN(24,  B, 8),
    __ACM32_PIN(25,  B, 9),
    __ACM32_PIN(26,  B, 10),
    __ACM32_PIN(27,  B, 11),
    __ACM32_PIN(28,  B, 12),
    __ACM32_PIN(29,  B, 13),
    __ACM32_PIN(30,  B, 14),
    __ACM32_PIN(31,  B, 15),
#endif /* defined(BSP_USING_GPIO1) */
};

static struct pin_irq_map pin_irq_map[] =
{
    {EXTI_LINE_0,  RT_NULL},
    {EXTI_LINE_1,  RT_NULL},
    {EXTI_LINE_2,  RT_NULL},
    {EXTI_LINE_3,  RT_NULL},
    {EXTI_LINE_4,  RT_NULL},
    {EXTI_LINE_5,  RT_NULL},
    {EXTI_LINE_6,  RT_NULL},
    {EXTI_LINE_7,  RT_NULL},
    {EXTI_LINE_8,  RT_NULL},
    {EXTI_LINE_9,  RT_NULL},
    {EXTI_LINE_10, RT_NULL},
    {EXTI_LINE_11, RT_NULL},
    {EXTI_LINE_12, RT_NULL},
    {EXTI_LINE_13, RT_NULL},
    {EXTI_LINE_14, RT_NULL},
    {EXTI_LINE_15, RT_NULL},
};

static struct rt_pin_irq_hdr pin_irq_hdr_tab[] =
{
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
    {-1, 0, RT_NULL, RT_NULL},
};
static uint32_t pin_irq_enable_mask = 0;

#define ITEM_NUM(items) sizeof(items) / sizeof(items[0])
static const struct pin_index *get_pin(uint8_t pin)
{
    const struct pin_index *index;

    if (pin < ITEM_NUM(pins))
    {
        index = &pins[pin];
        if (index->index == -1)
            index = RT_NULL;
    }
    else
    {
        index = RT_NULL;
    }

    return index;
};

static void _pin_write(rt_device_t dev, rt_base_t pin, rt_uint8_t value)
{
    const struct pin_index *index;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return;
    }

    HAL_GPIO_WritePin(index->gpio, index->pin, (GPIO_PinState)value);
}

static rt_ssize_t _pin_read(rt_device_t dev, rt_base_t pin)
{
    int value;
    const struct pin_index *index;

    value = PIN_LOW;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_EINVAL;
    }

    value = HAL_GPIO_ReadPin(index->gpio, index->pin);

    return value;
}

static void _pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
{
    const struct pin_index *index;
    GPIO_InitTypeDef GPIO_InitStruct;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return;
    }

    /* Enable GPIO clock */
    if (index->gpio == GPIOA)
        __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (index->gpio == GPIOB)
        __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (index->gpio == GPIOC)
        __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (index->gpio == GPIOD)
        __HAL_RCC_GPIOD_CLK_ENABLE();
    else if (index->gpio == GPIOE)
        __HAL_RCC_GPIOE_CLK_ENABLE();
    else if (index->gpio == GPIOF)
        __HAL_RCC_GPIOF_CLK_ENABLE();
    else if (index->gpio == GPIOG)
        __HAL_RCC_GPIOG_CLK_ENABLE();
    else if (index->gpio == GPIOH)
        __HAL_RCC_GPIOH_CLK_ENABLE();

    /* Configure GPIO_InitStructure */
    GPIO_InitStruct.Pin = index->pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Drive = GPIO_DRIVE_LEVEL3;
    GPIO_InitStruct.Alternate = GPIO_FUNCTION_0;

    if (mode == PIN_MODE_OUTPUT)
    {
        /* output setting */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT)
    {
        /* input setting: not pull. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }
    else if (mode == PIN_MODE_INPUT_PULLUP)
    {
        /* input setting: pull up. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLUP;
    }
    else if (mode == PIN_MODE_INPUT_PULLDOWN)
    {
        /* input setting: pull down. */
        GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pull = GPIO_PULLDOWN;
    }
    else if (mode == PIN_MODE_OUTPUT_OD)
    {
        /* output setting: od. */
        GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
    }

    HAL_GPIO_Init(index->gpio, &GPIO_InitStruct);
}

#define     PIN2INDEX(pin)      ((pin) % 16)

static rt_err_t _pin_attach_irq(struct rt_device *device, rt_base_t pin,
                                     rt_uint8_t mode, void (*hdr)(void *args), void *args)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    irqindex = PIN2INDEX(pin);

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == pin &&
            pin_irq_hdr_tab[irqindex].hdr == hdr &&
            pin_irq_hdr_tab[irqindex].mode == mode &&
            pin_irq_hdr_tab[irqindex].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }

    if (pin_irq_hdr_tab[irqindex].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }

    pin_irq_hdr_tab[irqindex].pin = pin;
    pin_irq_hdr_tab[irqindex].hdr = hdr;
    pin_irq_hdr_tab[irqindex].mode = mode;
    pin_irq_hdr_tab[irqindex].args = args;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t _pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    irqindex = PIN2INDEX(pin);

    level = rt_hw_interrupt_disable();
    if (pin_irq_hdr_tab[irqindex].pin == -1)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }
    pin_irq_hdr_tab[irqindex].pin = -1;
    pin_irq_hdr_tab[irqindex].hdr = RT_NULL;
    pin_irq_hdr_tab[irqindex].mode = 0;
    pin_irq_hdr_tab[irqindex].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t _pin_irq_enable(struct rt_device *device, rt_base_t pin,
                                     rt_uint8_t enabled)
{
    const struct pin_index *index;
    rt_base_t level;
    rt_int32_t irqindex = -1;
    uint32_t exti_mode;

    index = get_pin(pin);
    if (index == RT_NULL)
    {
        return -RT_ENOSYS;
    }

    irqindex = PIN2INDEX(pin);

    if (enabled == PIN_IRQ_ENABLE)
    {
        level = rt_hw_interrupt_disable();

        if (pin_irq_hdr_tab[irqindex].pin == -1)
        {
            rt_hw_interrupt_enable(level);
            return -RT_ENOSYS;
        }

        pin_irq_map[irqindex].gpio = index->gpio;

        switch (pin_irq_hdr_tab[irqindex].mode)
        {
        case PIN_IRQ_MODE_RISING:
            exti_mode = EXTI_MODE_IT_RISING;
            break;
        case PIN_IRQ_MODE_FALLING:
            exti_mode = EXTI_MODE_IT_FALLING;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            exti_mode = EXTI_MODE_IT_RISING_FALLING;
            break;
        default:
            rt_hw_interrupt_enable(level);
            return -RT_EINVAL;
        }

        HAL_EXTI_SetConfigLine(index->gpio, pin_irq_map[irqindex].line, exti_mode);

        pin_irq_enable_mask |= 1 << irqindex;

        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        if ((pin_irq_enable_mask & (1 << irqindex)) == 0)
        {
            return -RT_ENOSYS;
        }

        level = rt_hw_interrupt_disable();

        HAL_EXTI_ClearConfigLine(pin_irq_map[irqindex].line);

        pin_irq_enable_mask &= ~(1 << irqindex);

        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_ENOSYS;
    }

    return RT_EOK;
}

const static struct rt_pin_ops _acm32_pin_ops =
{
    _pin_mode,
    _pin_write,
    _pin_read,
    _pin_attach_irq,
    _pin_dettach_irq,
    _pin_irq_enable,
    RT_NULL,
};

int rt_hw_pin_init(void)
{
    rt_device_pin_register("pin", &_acm32_pin_ops, RT_NULL);

    return 0;
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

/* EXTI IRQ handlers for line 0-4 (individual) */
#if defined(EXTI0_IRQn)
void EXTI0_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_0);
    if (pin_irq_hdr_tab[0].hdr)
    {
        pin_irq_hdr_tab[0].hdr(pin_irq_hdr_tab[0].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI1_IRQn)
void EXTI1_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_1);
    if (pin_irq_hdr_tab[1].hdr)
    {
        pin_irq_hdr_tab[1].hdr(pin_irq_hdr_tab[1].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI2_IRQn)
void EXTI2_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_2);
    if (pin_irq_hdr_tab[2].hdr)
    {
        pin_irq_hdr_tab[2].hdr(pin_irq_hdr_tab[2].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI3_IRQn)
void EXTI3_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_3);
    if (pin_irq_hdr_tab[3].hdr)
    {
        pin_irq_hdr_tab[3].hdr(pin_irq_hdr_tab[3].args);
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI4_IRQn)
void EXTI4_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_4);
    if (pin_irq_hdr_tab[4].hdr)
    {
        pin_irq_hdr_tab[4].hdr(pin_irq_hdr_tab[4].args);
    }
    rt_interrupt_leave();
}
#endif

/* EXTI IRQ handler for lines 5-9 */
#if defined(EXTI9_5_IRQn)
void EXTI9_5_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_5);
    HAL_EXTI_IRQHandler(EXTI_LINE_6);
    HAL_EXTI_IRQHandler(EXTI_LINE_7);
    HAL_EXTI_IRQHandler(EXTI_LINE_8);
    HAL_EXTI_IRQHandler(EXTI_LINE_9);
    if (pin_irq_hdr_tab[5].hdr)  pin_irq_hdr_tab[5].hdr(pin_irq_hdr_tab[5].args);
    if (pin_irq_hdr_tab[6].hdr)  pin_irq_hdr_tab[6].hdr(pin_irq_hdr_tab[6].args);
    if (pin_irq_hdr_tab[7].hdr)  pin_irq_hdr_tab[7].hdr(pin_irq_hdr_tab[7].args);
    if (pin_irq_hdr_tab[8].hdr)  pin_irq_hdr_tab[8].hdr(pin_irq_hdr_tab[8].args);
    if (pin_irq_hdr_tab[9].hdr)  pin_irq_hdr_tab[9].hdr(pin_irq_hdr_tab[9].args);
    rt_interrupt_leave();
}
#endif

/* EXTI IRQ handler for lines 10-15 */
#if defined(EXTI15_10_IRQn)
void EXTI15_10_IRQHandler(void)
{
    rt_interrupt_enter();
    HAL_EXTI_IRQHandler(EXTI_LINE_10);
    HAL_EXTI_IRQHandler(EXTI_LINE_11);
    HAL_EXTI_IRQHandler(EXTI_LINE_12);
    HAL_EXTI_IRQHandler(EXTI_LINE_13);
    HAL_EXTI_IRQHandler(EXTI_LINE_14);
    HAL_EXTI_IRQHandler(EXTI_LINE_15);
    if (pin_irq_hdr_tab[10].hdr) pin_irq_hdr_tab[10].hdr(pin_irq_hdr_tab[10].args);
    if (pin_irq_hdr_tab[11].hdr) pin_irq_hdr_tab[11].hdr(pin_irq_hdr_tab[11].args);
    if (pin_irq_hdr_tab[12].hdr) pin_irq_hdr_tab[12].hdr(pin_irq_hdr_tab[12].args);
    if (pin_irq_hdr_tab[13].hdr) pin_irq_hdr_tab[13].hdr(pin_irq_hdr_tab[13].args);
    if (pin_irq_hdr_tab[14].hdr) pin_irq_hdr_tab[14].hdr(pin_irq_hdr_tab[14].args);
    if (pin_irq_hdr_tab[15].hdr) pin_irq_hdr_tab[15].hdr(pin_irq_hdr_tab[15].args);
    rt_interrupt_leave();
}
#endif

#endif /* RT_USING_PIN */
