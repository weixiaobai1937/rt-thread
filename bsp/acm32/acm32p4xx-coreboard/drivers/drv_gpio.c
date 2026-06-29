#include <rthw.h>
#include <rtdevice.h>
#include "board.h"

#ifdef RT_USING_PIN

#define ACM32_PIN_MAX   128

static void _pin_write(rt_device_t dev, rt_base_t pin, rt_uint8_t value)
{
    if (pin >= ACM32_PIN_MAX) return;
    gpio_put((gpio_pin_t)pin, value != PIN_LOW);
}

static rt_ssize_t _pin_read(rt_device_t dev, rt_base_t pin)
{
    if (pin >= ACM32_PIN_MAX) return -RT_EINVAL;
    return gpio_get((gpio_pin_t)pin) ? PIN_HIGH : PIN_LOW;
}

static void _pin_mode(rt_device_t dev, rt_base_t pin, rt_uint8_t mode)
{
    if (pin >= ACM32_PIN_MAX) return;
    gpio_pin_t p = (gpio_pin_t)pin;

    switch (mode)
    {
    case PIN_MODE_OUTPUT:
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        break;
    case PIN_MODE_INPUT:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        break;
    case PIN_MODE_INPUT_PULLUP:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        gpio_pull_up(p);
        break;
    case PIN_MODE_INPUT_PULLDOWN:
        gpio_init(p);
        gpio_set_dir(p, GPIO_IN);
        gpio_pull_down(p);
        break;
    case PIN_MODE_OUTPUT_OD:
        gpio_init(p);
        gpio_set_dir(p, GPIO_OUT);
        gpio_set_output_type(p, GPIO_OTYPE_OD);
        break;
    default:
        break;
    }
}

static rt_err_t _pin_attach_irq(struct rt_device *device, rt_base_t pin,
                                 rt_uint8_t mode, void (*hdr)(void *args), void *args)
{ return -RT_ENOSYS; }

static rt_err_t _pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{ return -RT_ENOSYS; }

static rt_err_t _pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint8_t enabled)
{ return -RT_ENOSYS; }

static const struct rt_pin_ops _acm32_pin_ops =
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

#endif /* RT_USING_PIN */
