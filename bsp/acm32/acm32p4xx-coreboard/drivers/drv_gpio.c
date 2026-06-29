#include <rthw.h>
#include <rtdevice.h>
#include "board.h"

#ifdef RT_USING_PIN

#define ACM32_PIN_MAX   128

/* ==================== 引脚编号 → EXTI 线号 ==================== */
#define PIN2EXTILINE(pin)   ((pin) % 16)

/* ==================== GPIO 端口 → EXTI 端口 ==================== */
static exti_gpio_port_t _gpio_port_to_exti(gpio_pin_t pin)
{
    switch (GPIO_GET_PORT(pin))
    {
        case 0: return EXTI_GPIO_PORTA;
        case 1: return EXTI_GPIO_PORTB;
        case 2: return EXTI_GPIO_PORTC;
        case 3: return EXTI_GPIO_PORTD;
        case 4: return EXTI_GPIO_PORTE;
        case 5: return EXTI_GPIO_PORTF;
        case 6: return EXTI_GPIO_PORTG;
        case 7: return EXTI_GPIO_PORTH;
        default: return EXTI_GPIO_PORTA;
    }
}

/* ==================== 中断管理 ==================== */

#define PIN_IRQ_NUM     16

struct pin_irq_map
{
    rt_base_t           pin;        /* 绑定的引脚编号 (-1 = 空闲) */
    rt_uint8_t          mode;       /* PIN_IRQ_MODE_xxx */
    void              (*hdr)(void *args);
    void               *args;
};

static struct pin_irq_map pin_irq_map[PIN_IRQ_NUM] = {0};

static void _pin_irq_init(void)
{
    static rt_bool_t initialized = RT_FALSE;
    if (initialized) return;
    initialized = RT_TRUE;

    for (int i = 0; i < PIN_IRQ_NUM; i++)
    {
        pin_irq_map[i].pin = -1;
    }
}

/* EXTI 回调 — 将 SDK 回调转为 RT-Thread 回调 */
static void _exti_callback(exti_line_t line)
{
    if (line < PIN_IRQ_NUM && pin_irq_map[line].hdr)
    {
        pin_irq_map[line].hdr(pin_irq_map[line].args);
    }
}

/* ==================== RT-Thread PIN Ops ==================== */

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
{
    if (pin >= ACM32_PIN_MAX) return -RT_ENOSYS;

    rt_base_t level;
    rt_int32_t line = PIN2EXTILINE(pin);

    _pin_irq_init();

    level = rt_hw_interrupt_disable();

    /* 检查是否已被占用 */
    if (pin_irq_map[line].pin == pin &&
        pin_irq_map[line].hdr == hdr &&
        pin_irq_map[line].mode == mode &&
        pin_irq_map[line].args == args)
    {
        rt_hw_interrupt_enable(level);
        return RT_EOK;
    }

    if (pin_irq_map[line].pin != -1)
    {
        rt_hw_interrupt_enable(level);
        return -RT_EBUSY;
    }

    /* 保存回调信息 */
    pin_irq_map[line].pin  = pin;
    pin_irq_map[line].mode = mode;
    pin_irq_map[line].hdr  = hdr;
    pin_irq_map[line].args = args;

    rt_hw_interrupt_enable(level);
    return RT_EOK;
}

static rt_err_t _pin_dettach_irq(struct rt_device *device, rt_base_t pin)
{
    if (pin >= ACM32_PIN_MAX) return -RT_ENOSYS;

    rt_base_t level;
    rt_int32_t line = PIN2EXTILINE(pin);

    _pin_irq_init();

    /* 先禁用中断 */
    _pin_irq_enable(device, pin, PIN_IRQ_DISABLE);

    level = rt_hw_interrupt_disable();
    pin_irq_map[line].pin  = -1;
    pin_irq_map[line].mode = 0;
    pin_irq_map[line].hdr  = RT_NULL;
    pin_irq_map[line].args = RT_NULL;
    rt_hw_interrupt_enable(level);

    return RT_EOK;
}

static rt_err_t _pin_irq_enable(struct rt_device *device, rt_base_t pin, rt_uint8_t enabled)
{
    if (pin >= ACM32_PIN_MAX) return -RT_ENOSYS;

    rt_base_t level;
    rt_int32_t line = PIN2EXTILINE(pin);

    _pin_irq_init();

    if (enabled == PIN_IRQ_ENABLE)
    {
        if (pin_irq_map[line].pin == -1)
            return -RT_ENOSYS;

        /* 转换触发模式 */
        exti_trigger_t trigger;
        switch (pin_irq_map[line].mode)
        {
        case PIN_IRQ_MODE_RISING:
            trigger = EXTI_TRIGGER_RISING;
            break;
        case PIN_IRQ_MODE_FALLING:
            trigger = EXTI_TRIGGER_FALLING;
            break;
        case PIN_IRQ_MODE_RISING_FALLING:
            trigger = EXTI_TRIGGER_BOTH;
            break;
        default:
            return -RT_EINVAL;
        }

        level = rt_hw_interrupt_disable();

        /* 配置 EXTI: GPIO端口 + 引脚号 + 触发方式 */
        exti_gpio_port_t port = _gpio_port_to_exti((gpio_pin_t)pin);
        exti_gpio_init(port, line, trigger);

        /* 注册回调 */
        exti_set_callback((exti_line_t)line, _exti_callback);

        rt_hw_interrupt_enable(level);
    }
    else if (enabled == PIN_IRQ_DISABLE)
    {
        level = rt_hw_interrupt_disable();
        exti_disable((exti_line_t)line);
        exti_remove_callback((exti_line_t)line);
        rt_hw_interrupt_enable(level);
    }
    else
    {
        return -RT_EINVAL;
    }

    return RT_EOK;
}

static rt_base_t _pin_get(const char *name)
{
    rt_base_t pin = 0;
    int port, pin_num;
    rt_uint8_t hw_port_num;

    if (rt_strlen(name) < 4) return -RT_EINVAL;

    /* 格式: "PX.NN" 例如 "PA.0", "PB.15", "PH.7" */
    port    = name[1] - 'A';
    pin_num = (name[3] - '0') * 10 + (name[4] - '0');

    if (port < 0 || port > 7 || pin_num < 0 || pin_num > 15)
        return -RT_EINVAL;

    pin = port * 16 + pin_num;
    return pin;
}

static const struct rt_pin_ops _acm32_pin_ops =
{
    _pin_mode,
    _pin_write,
    _pin_read,
    _pin_attach_irq,
    _pin_dettach_irq,
    _pin_irq_enable,
    _pin_get,
};

int rt_hw_pin_init(void)
{
    _pin_irq_init();
    rt_device_pin_register("pin", &_acm32_pin_ops, RT_NULL);
    return 0;
}
INIT_BOARD_EXPORT(rt_hw_pin_init);

/* ==================== EXTI 中断服务函数 ==================== */

#if defined(EXTI0_IRQn)
void EXTI0_IRQHandler(void)
{
    rt_interrupt_enter();
    exti_clear_pending(EXTI_LINE_0);
    exti_get_callback(EXTI_LINE_0)(EXTI_LINE_0);
    rt_interrupt_leave();
}
#endif

#if defined(EXTI1_IRQn)
void EXTI1_IRQHandler(void)
{
    rt_interrupt_enter();
    exti_clear_pending(EXTI_LINE_1);
    exti_get_callback(EXTI_LINE_1)(EXTI_LINE_1);
    rt_interrupt_leave();
}
#endif

#if defined(EXTI2_IRQn)
void EXTI2_IRQHandler(void)
{
    rt_interrupt_enter();
    exti_clear_pending(EXTI_LINE_2);
    exti_get_callback(EXTI_LINE_2)(EXTI_LINE_2);
    rt_interrupt_leave();
}
#endif

#if defined(EXTI3_IRQn)
void EXTI3_IRQHandler(void)
{
    rt_interrupt_enter();
    exti_clear_pending(EXTI_LINE_3);
    exti_get_callback(EXTI_LINE_3)(EXTI_LINE_3);
    rt_interrupt_leave();
}
#endif

#if defined(EXTI4_IRQn)
void EXTI4_IRQHandler(void)
{
    rt_interrupt_enter();
    exti_clear_pending(EXTI_LINE_4);
    exti_get_callback(EXTI_LINE_4)(EXTI_LINE_4);
    rt_interrupt_leave();
}
#endif

#if defined(EXTI9_5_IRQn)
void EXTI9_5_IRQHandler(void)
{
    rt_interrupt_enter();
    for (int line = 5; line <= 9; line++)
    {
        if (exti_get_pending((exti_line_t)line))
        {
            exti_clear_pending((exti_line_t)line);
            exti_callback_t cb = exti_get_callback((exti_line_t)line);
            if (cb) cb((exti_line_t)line);
        }
    }
    rt_interrupt_leave();
}
#endif

#if defined(EXTI15_10_IRQn)
void EXTI15_10_IRQHandler(void)
{
    rt_interrupt_enter();
    for (int line = 10; line <= 15; line++)
    {
        if (exti_get_pending((exti_line_t)line))
        {
            exti_clear_pending((exti_line_t)line);
            exti_callback_t cb = exti_get_callback((exti_line_t)line);
            if (cb) cb((exti_line_t)line);
        }
    }
    rt_interrupt_leave();
}
#endif

#endif /* RT_USING_PIN */
