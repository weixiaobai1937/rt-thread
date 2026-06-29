#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>
#include <drivers/dev_serial_v2.h>
#include "board.h"
#include "uart_config.h"

#ifdef RT_USING_SERIAL_V2

struct acm32_uart
{
    struct acm32_uart_config    *config;
    struct rt_serial_device     serial;
};
#define raw_to_uart(raw) rt_container_of(raw, struct acm32_uart, serial)

static rt_err_t _uart_configure(struct rt_serial_device *serial, struct serial_configure *cfg)
{
    struct acm32_uart *uart = raw_to_uart(serial);

    gpio_init(uart->config->tx_pin);
    gpio_set_function(uart->config->tx_pin, uart->config->af);
    gpio_init(uart->config->rx_pin);
    gpio_set_function(uart->config->rx_pin, uart->config->af);
    gpio_pull_up(uart->config->rx_pin);

    uart_init_default(uart->config->uart_num, cfg->baud_rate);
    return RT_EOK;
}

static rt_err_t _uart_control(struct rt_serial_device *serial, int cmd, void *arg)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    switch (cmd)
    {
    case RT_DEVICE_CTRL_CLR_INT:
        NVIC_DisableIRQ(uart->config->irq_type);
        break;
    case RT_DEVICE_CTRL_SET_INT:
        NVIC_EnableIRQ(uart->config->irq_type);
        break;
    }
    return RT_EOK;
}

static int _uart_putc(struct rt_serial_device *serial, char c)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    uart_putc(uart->config->uart_num, (uint8_t)c);
    return 1;
}

static int _uart_getc(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    if (!uart_is_rx_fifo_empty(uart->config->uart_num))
        return (int)uart_getc(uart->config->uart_num);
    return -1;
}

static rt_ssize_t _uart_transmit(struct rt_serial_device *serial, rt_uint8_t *buf, rt_size_t size, rt_uint32_t tx_flag)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    rt_size_t i;
    for (i = 0; i < size; i++)
        uart_putc(uart->config->uart_num, buf[i]);
    return size;
}

static const struct rt_uart_ops acm32_uart_ops =
{
    .configure = _uart_configure,
    .control   = _uart_control,
    .putc      = _uart_putc,
    .getc      = _uart_getc,
    .transmit  = _uart_transmit,
};

static void uart_isr(struct rt_serial_device *serial)
{
    struct acm32_uart *uart = raw_to_uart(serial);
    if (!uart_is_rx_fifo_empty(uart->config->uart_num))
        rt_hw_serial_isr(serial, RT_SERIAL_EVENT_RX_IND);
}

enum {
#ifdef BSP_USING_UART1
    UART1_INDEX,
#endif
    UART_MAX_INDEX,
};

static struct acm32_uart_config uart_config[] = {
#ifdef BSP_USING_UART1
    UART1_CONFIG,
#endif
};

static struct acm32_uart uart_obj[UART_MAX_INDEX] = {0};

rt_err_t rt_hw_uart_init(void)
{
    rt_size_t n = sizeof(uart_obj) / sizeof(struct acm32_uart);
    struct serial_configure c = RT_SERIAL_CONFIG_DEFAULT;
    for (int i = 0; i < n; i++)
    {
        uart_obj[i].config = &uart_config[i];
        uart_obj[i].serial.ops = &acm32_uart_ops;
        uart_obj[i].serial.config = c;
        rt_err_t rc = rt_hw_serial_register(&uart_obj[i].serial,
            uart_obj[i].config->name,
            RT_DEVICE_FLAG_RDWR | RT_DEVICE_FLAG_INT_RX | RT_DEVICE_FLAG_INT_TX,
            NULL);
        RT_ASSERT(rc == RT_EOK);
    }
    return RT_EOK;
}

#ifdef BSP_USING_UART1
void UART1_IRQHandler(void)
{
    rt_interrupt_enter();
    uart_irq_handler(UART_1);
    uart_isr(&uart_obj[UART1_INDEX].serial);
    rt_interrupt_leave();
}
#endif

#endif /* RT_USING_SERIAL_V2 */
